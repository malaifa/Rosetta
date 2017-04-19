// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/pose/rna/RNA_Util.cc
/// @author Rhiju Das

// Unit headers
#include <core/pose/rna/util.hh>
#include <core/pose/util.hh>
#include <core/types.hh>

// Package headers
#include <core/chemical/ResidueTypeFinder.hh>
#include <core/chemical/VariantType.hh>
#include <core/conformation/Residue.hh>
#include <core/conformation/ResidueFactory.hh>
#include <core/id/TorsionID.hh>
#include <core/kinematics/AtomTree.hh>
#include <core/kinematics/AtomPointer.fwd.hh>
#include <core/kinematics/tree/Atom.hh>
#include <core/kinematics/FoldTree.hh>
#include <core/kinematics/Stub.hh>
#include <core/chemical/ResidueType.hh>
#include <core/chemical/AtomType.hh>
#include <core/pose/Pose.hh>
#include <core/pose/PDBInfo.hh>
#include <core/chemical/rna/RNA_FittedTorsionInfo.hh>
#include <core/chemical/rna/RNA_ResidueType.hh>
#include <core/pose/rna/RNA_IdealCoord.hh>
#include <core/pose/annotated_sequence.hh>
#include <core/chemical/AA.hh>

// Project headers
#include <numeric/constants.hh>
#include <numeric/xyz.functions.hh>
#include <utility/vector1.hh>

#include <basic/options/option.hh>
#include <basic/options/keys/rna.OptionKeys.gen.hh>
#include <basic/Tracer.hh>

#include <ObjexxFCL/string.functions.hh>
#include <ObjexxFCL/format.hh>

static THREAD_LOCAL basic::Tracer TR( "core.pose.rna.RNA_Util" );

// Utility headers

// C++

using namespace core::chemical::rna;
static const RNA_FittedTorsionInfo torsion_info;

namespace core {
namespace pose {
namespace rna {

////////////////////////////////////////////////////////////////////////////////////////
bool
mutate_position( pose::Pose & pose, Size const i, char const & new_seq ){

	using namespace core::conformation;
	using namespace core::chemical;

	if ( new_seq == pose.sequence()[i-1] ) return false;

	ResidueTypeSetCOP rsd_set = pose.residue_type( i ).residue_type_set();

	ResidueProperty base_property = ( pose.residue_type( i ).is_RNA() ) ? RNA : NO_PROPERTY;
	ResidueTypeCOP new_rsd_type( ResidueTypeFinder( *rsd_set ).name1( new_seq ).variants( pose.residue_type(i).variant_types() ).base_property( base_property ).get_representative_type() );

	ResidueOP new_rsd( ResidueFactory::create_residue( *new_rsd_type, pose.residue( i ), pose.conformation() ) );

	Real const save_chi = pose.chi(i);
	pose.replace_residue( i, *new_rsd, false );
	pose.set_chi( i, save_chi );

	return true;
}

//////////////////////////////////////////////////////
void
figure_out_reasonable_rna_fold_tree( pose::Pose & pose )
{
	using namespace core::conformation;
	using namespace core::chemical;

	//Look for chainbreaks in PDB.
	Size const nres = pose.total_residue();
	kinematics::FoldTree f( nres );

	Size m( 0 );

	for ( Size i = 1; i < nres; ++i ) {
		bool new_jump( false );
		if ( (  pose.residue_type(i).is_RNA() != pose.residue_type(i+1).is_RNA() ) ||
				(  pose.residue_type(i).is_protein() != pose.residue_type(i+1).is_protein() ) ||
				(  pose.pdb_info() && ( pose.pdb_info()->chain( i ) != pose.pdb_info()->chain( i+1 ) ) ) ||
				(  pose.residue_type(i).has_variant_type( CUTPOINT_LOWER ) && pose.residue_type(i+1).has_variant_type( CUTPOINT_UPPER ) ) ) {
			new_jump = true;
		}

		if ( !new_jump &&
				pose.residue_type(i).is_RNA() && pose.residue_type(i+1).is_RNA() &&
				pose::rna::is_rna_chainbreak( pose, i ) ) {
			new_jump = true;
		}

		if ( new_jump ) {

			f.new_jump( i, i+1, i );
			m++;

			if ( pose.residue_type(i).is_RNA() && pose.residue_type(i+1).is_RNA() ) {
				ResidueType const & current_rsd( pose.residue_type( i   ) ) ;
				ResidueType const &    next_rsd( pose.residue_type( i+1 ) ) ;
				f.set_jump_atoms( m,
					chemical::rna::chi1_torsion_atom( current_rsd ),
					chemical::rna::chi1_torsion_atom( next_rsd )   );
			}
		}

	}

	pose.fold_tree( f );
}

////////////////////////////////////////////////////////
void
virtualize_5prime_phosphates( pose::Pose & pose ){

	for ( Size i = 1; i <= pose.total_residue(); i++ ) {

		if ( i==1 || ( pose.fold_tree().is_cutpoint( i-1 ) &&
				!pose.residue_type( i-1 ).has_variant_type( chemical::CUTPOINT_LOWER ) &&
				!pose.residue_type( i   ).has_variant_type( chemical::CUTPOINT_UPPER ) ) ) {
			if ( pose.residue_type(i).is_RNA() ) {
				pose::add_variant_type_to_pose_residue( pose, chemical::VIRTUAL_PHOSPHATE, i );
			}
		}

	}

}


//////////////////////////////////////////////////////
bool
is_cutpoint_open( Pose const & pose, Size const i ) {

	if ( i < 1 ) return true; // user may pass zero -- sometimes checking if we are at a chain terminus, and this would corresponde to n-1 with n=1.

	if ( i >= pose.total_residue() ) return true;

	if ( ! pose.fold_tree().is_cutpoint(i) ) return false;

	if ( pose.residue_type( i   ).has_variant_type( chemical::CUTPOINT_LOWER ) ||
			pose.residue_type( i+1 ).has_variant_type( chemical::CUTPOINT_UPPER ) ) return false;

	return true;
}

//////////////////////////////////////////////////////
bool
is_rna_chainbreak( Pose const & pose, Size const i ) {

	static Real const CHAINBREAK_CUTOFF2 ( 2.5 * 2.5 );

	if ( i >= pose.total_residue() ) return true;
	if ( i < 1 ) return true;

	conformation::Residue const & current_rsd( pose.residue( i   ) ) ;
	conformation::Residue const &    next_rsd( pose.residue( i+1 ) ) ;
	runtime_assert ( current_rsd.is_RNA() );
	runtime_assert ( next_rsd.is_RNA() );

	Size atom_O3prime = current_rsd.type().RNA_type().o3prime_atom_index();
	Size atom_P       =    next_rsd.type().RNA_type().p_atom_index();
	Real const dist2 =
		( current_rsd.atom( atom_O3prime ).xyz() - next_rsd.atom( atom_P ).xyz() ).length_squared();

	if ( dist2 > CHAINBREAK_CUTOFF2 ) {
		TR.Debug << "Found chainbreak at residue "<< i << " .  O3'-P distance: " << sqrt( dist2 ) << std::endl;
		return true;
	}

	if ( pose.pdb_info() ) {
		if ( pose.pdb_info()->number( i ) + 1 != pose.pdb_info()->number( i+1 ) ) return true;
		if ( pose.pdb_info()->chain( i ) != pose.pdb_info()->chain( i+1 ) ) return true;
	}

	return false;

}

////////////////////////////////////////////////////////////////////////////
// Following is quite slow, because it works on a residue in the context
//  of a full pose -- a lot of time wasted on refolding everything.
void
fix_sugar_coords_WORKS_BUT_SLOW(
	utility::vector1< std::string> atoms_for_which_we_need_new_dofs,
	utility::vector1< utility::vector1< id::DOF_Type > > which_dofs,
	utility::vector1< Vector > const & non_main_chain_sugar_coords,
	Pose & pose,
	Size const i )
{

	using namespace core::id;

	conformation::Residue const & rsd( pose.residue( i ) );

	//Yup, hard-wired...
	kinematics::Stub const input_stub( rsd.xyz( " C3'" ), rsd.xyz( " C3'" ), rsd.xyz( " C4'" ), rsd.xyz( " C5'" ) );

	utility::vector1< Vector > start_vectors;
	utility::vector1< utility::vector1< Real > > new_dof_sets;

	for ( Size n = 1; n <= non_main_chain_sugar_atoms.size(); n++  ) {
		Size const j = rsd.atom_index( non_main_chain_sugar_atoms[ n ] );
		//Save a copy of starting location
		Vector v = rsd.xyz( non_main_chain_sugar_atoms[ n ]  );
		start_vectors.push_back( v );

		//Desired location
		Vector v2 = input_stub.local2global( non_main_chain_sugar_coords[ n ] );
		pose.set_xyz( id::AtomID( j,i ), v2 );
	}

	for ( Size n = 1; n <= atoms_for_which_we_need_new_dofs.size(); n++  ) {
		utility::vector1< Real > dof_set;
		Size const j = rsd.atom_index( atoms_for_which_we_need_new_dofs[ n ] );
		for ( Size m = 1; m <= which_dofs[n].size(); m++ ) {
			dof_set.push_back( pose.atom_tree().dof( DOF_ID( AtomID( j,i) , which_dofs[ n ][ m ] ) ) );
		}
		new_dof_sets.push_back( dof_set );
	}


	// Now put the ring atoms in the desired spots, but by changing internal DOFS --
	// rest of the atoms (e.g., in base and 2'-OH) will scoot around as well, preserving
	// ideal bond lengths and angles.
	for ( Size n = 1; n <= non_main_chain_sugar_atoms.size(); n++  ) {
		Size const j = rsd.atom_index( non_main_chain_sugar_atoms[ n ] );
		pose.set_xyz( id::AtomID( j,i ), start_vectors[n] );
	}

	for ( Size n = 1; n <= atoms_for_which_we_need_new_dofs.size(); n++  ) {
		Size const j = rsd.atom_index( atoms_for_which_we_need_new_dofs[ n ] );

		for ( Size m = 1; m <= which_dofs[n].size(); m++ ) {
			pose.set_dof(  DOF_ID( AtomID( j,i) , which_dofs[ n ][ m ] ), new_dof_sets[ n ][ m ] );
		}

	}
}

/////////////////////////////////////////////////////////
void
prepare_scratch_residue(
	core::conformation::ResidueOP & scratch_rsd,
	core::conformation::Residue const & start_rsd,
	utility::vector1< Vector > const & non_main_chain_sugar_coords,
	Pose const & pose)
{

	for ( Size j = 1; j < scratch_rsd->first_sidechain_atom(); j++ ) {
		scratch_rsd->set_xyz( j , start_rsd.xyz( j ) );
	}

	//Yup, hard-wired...
	// AMW: changing to grab this from the RNA_ResidueType
	// reasoning: if we do any artful stuff making these indices refer to other named atoms, it should go through only one point
	// (i.e. perhaps for a TNA there's an equivalent for one or more of these?)
	// AMW TODO: shouldn't RNA_ResidueType have members for C3' and C5' as well?
	kinematics::Stub const input_stub( scratch_rsd->xyz( " C3'" ), scratch_rsd->xyz( " C3'" ), scratch_rsd->xyz( scratch_rsd->type().RNA_type().c4prime_atom_index() ), scratch_rsd->xyz( " C5'" ) );

	for ( Size n = 1; n <= non_main_chain_sugar_atoms.size(); n++  ) {
		//Desired location
		Size const j = scratch_rsd->atom_index( non_main_chain_sugar_atoms[ n ] );
		Vector v2 = input_stub.local2global( non_main_chain_sugar_coords[ n ] );
		scratch_rsd->set_xyz( j, v2 );
	}

	Size const o2prime_index( scratch_rsd->type().RNA_type().o2prime_index() );
	scratch_rsd->set_xyz( o2prime_index, scratch_rsd->build_atom_ideal( o2prime_index, pose.conformation() ) );

}


/////////////////////////////////////////////////////////
void
fix_sugar_coords(
	utility::vector1< std::string> atoms_for_which_we_need_new_dofs,
	utility::vector1< Vector > const & non_main_chain_sugar_coords,
	Pose & pose,
	Pose const & reference_pose,
	Size const i )
{

	using namespace core::id;
	using namespace core::conformation;

	conformation::Residue const & start_rsd( reference_pose.residue( i ) );

	static ResidueOP scratch_rsd( new Residue( start_rsd.type(), false /*dummy arg*/ ) );

	prepare_scratch_residue( scratch_rsd, start_rsd, non_main_chain_sugar_coords, pose );

	for ( Size n = 1; n <= atoms_for_which_we_need_new_dofs.size(); n++  ) {
		Size const j = scratch_rsd->atom_index( atoms_for_which_we_need_new_dofs[ n ] );

		// "Don't do update" --> my hack to prevent lots of refolds. I just want information about whether the
		// atom is a jump_atom, what its stub atoms are, etc... in principle could try to use input_stub_atom1_id(), etc.
		kinematics::tree::AtomCOP current_atom ( reference_pose.atom_tree().atom_dont_do_update( AtomID(j,i) ).get_self_ptr() );
		kinematics::tree::AtomCOP input_stub_atom1( current_atom->input_stub_atom1() );

		if ( !input_stub_atom1 ) continue;
		if ( (input_stub_atom1->id()).rsd() != (current_atom->id()).rsd() ) continue;
		if ( (input_stub_atom1->id()).atomno() > scratch_rsd->first_sidechain_atom() ) continue;

		Real const d = ( scratch_rsd->xyz( (input_stub_atom1->id()).atomno() ) -
			scratch_rsd->xyz( (current_atom->id()).atomno() ) ).length();
		pose.set_dof( DOF_ID( AtomID( j, i), D), d );

		if ( input_stub_atom1->is_jump() ) continue;

		kinematics::tree::AtomCOP input_stub_atom2( current_atom->input_stub_atom2() );
		if ( !input_stub_atom2 ) continue;
		if ( (input_stub_atom2->id()).rsd() != (current_atom->id()).rsd() ) continue;
		if ( (input_stub_atom2->id()).atomno() > scratch_rsd->first_sidechain_atom() ) continue;

		Real const theta = numeric::angle_radians(
			scratch_rsd->xyz( (current_atom->id()).atomno() ) ,
			scratch_rsd->xyz( (input_stub_atom1->id()).atomno() ),
			scratch_rsd->xyz( (input_stub_atom2->id()).atomno() ) );

		pose.set_dof( DOF_ID( AtomID( j, i), THETA), numeric::constants::d::pi - theta );

		// I commented out the following because otherwise, O4' at the 5' end of a pose did not get set properly. (RD, Nov. 2010)
		//  but there may be fallout.
		// if ( input_stub_atom2->is_jump() ) continue; //HEY NEED TO BE CAREFUL HERE.

		kinematics::tree::AtomCOP input_stub_atom3( current_atom->input_stub_atom3() );

		if ( !input_stub_atom3 ) continue;
		if ( (input_stub_atom3->id()).rsd() != (current_atom->id()).rsd() ) continue;
		if ( (input_stub_atom3->id()).atomno() > scratch_rsd->first_sidechain_atom() ) continue;

		Real const phi = numeric::dihedral_radians(
			scratch_rsd->xyz( (current_atom->id()).atomno() ),
			scratch_rsd->xyz( (input_stub_atom1->id()).atomno() ),
			scratch_rsd->xyz( (input_stub_atom2->id()).atomno() ),
			scratch_rsd->xyz( (input_stub_atom3->id()).atomno() ) );

		pose.set_dof( DOF_ID( AtomID( j, i), PHI), phi );

	}
}

//////////////////////////////////////////////////////////////////////////////
void
initialize_atoms_for_which_we_need_new_dofs(
	utility::vector1< std::string > & atoms_for_which_we_need_new_dofs,
	Pose const & pose,  Size const i)
{
	using namespace id;
	using namespace conformation;
	using namespace chemical;

	// Which way does atom_tree connectivity flow, i.e. is sugar drawn after base,
	// or after backbone?
	// This is admittedly very ugly, and very RNA specific.
	// ... perhaps this will be figured out in the Cartesian Fragment class?
	//
	ResidueType const & rsd( pose.residue_type( i ) );
	RNA_ResidueType const & rna_type( rsd.RNA_type() );

	kinematics::tree::AtomCOP c1prime_atom ( pose.atom_tree().atom( AtomID( rna_type.c1prime_atom_index(), i ) ).get_self_ptr() );
	kinematics::tree::AtomCOP o2prime_atom ( pose.atom_tree().atom( AtomID( rna_type.o2prime_index(), i ) ).get_self_ptr() );
	kinematics::tree::AtomCOP c2prime_atom ( pose.atom_tree().atom( AtomID( rna_type.c2prime_atom_index(), i ) ).get_self_ptr() );

	if ( (c1prime_atom->parent()->id()).atomno() == first_base_atom_index( rsd ) ) {
		// There's a jump to this residue.
		//std::cout << "RESIDUE WITH JUMP CONNECTIVITY : " <<  i << std::endl;
		atoms_for_which_we_need_new_dofs.push_back( " C2'" );
		atoms_for_which_we_need_new_dofs.push_back( " C3'" );
		atoms_for_which_we_need_new_dofs.push_back( rsd.atom_name( rna_type.o4prime_atom_index() ) );
		atoms_for_which_we_need_new_dofs.push_back( " C4'" );
		atoms_for_which_we_need_new_dofs.push_back( " C5'" );
		atoms_for_which_we_need_new_dofs.push_back( " O3'" );
	} else if ( (c2prime_atom->parent()->id()).atomno() ==  (o2prime_atom->id()).atomno() ) {
		atoms_for_which_we_need_new_dofs.push_back( " C1'" );
		atoms_for_which_we_need_new_dofs.push_back( " C3'" );
		atoms_for_which_we_need_new_dofs.push_back( rsd.atom_name( rna_type.o4prime_atom_index() ) );
		atoms_for_which_we_need_new_dofs.push_back( " C4'" );
		atoms_for_which_we_need_new_dofs.push_back( " C5'" );
		atoms_for_which_we_need_new_dofs.push_back( " O3'" );
	} else {
		atoms_for_which_we_need_new_dofs.push_back( " C1'" );
		atoms_for_which_we_need_new_dofs.push_back( " C2'" );
		atoms_for_which_we_need_new_dofs.push_back( rsd.atom_name( rna_type.o4prime_atom_index() ) );
	}
}


/////////////////////////////////////////////////////////////////////
//* Passing in a "reference" pose is a dirty trick to prevent
// computationally expensive refolds whenever we change a DOF in the pose.
void
apply_non_main_chain_sugar_coords(
	utility::vector1< Vector > const & non_main_chain_sugar_coords,
	Pose & pose,
	Pose const & reference_pose,
	Size const i
) {
	using namespace id;

	/////////////////////////////////////////////
	// Save desired torsion values.
	utility::vector1< Real > start_torsions;
	for ( Size j = 1; j <= NUM_RNA_TORSIONS; j++ ) {
		id::TorsionID rna_torsion_id( i, id::BB, j );
		if ( j > NUM_RNA_MAINCHAIN_TORSIONS ) rna_torsion_id = id::TorsionID( i, id::CHI, j - NUM_RNA_MAINCHAIN_TORSIONS );
		start_torsions.push_back( reference_pose.torsion( rna_torsion_id ) );
	}

	/////////////////////////////////////////////
	//What DOFS do I need to get the ring atoms where I want them?
	utility::vector1< std::string > atoms_for_which_we_need_new_dofs;

	initialize_atoms_for_which_we_need_new_dofs( atoms_for_which_we_need_new_dofs,  pose, i );

	fix_sugar_coords( atoms_for_which_we_need_new_dofs, non_main_chain_sugar_coords, pose, reference_pose, i );

	/////////////////////////////////////////////
	// Reapply desired torsion values.
	for ( Size j = 1; j <= NUM_RNA_TORSIONS; j++ ) {
		id::TorsionID rna_torsion_id( i, id::BB, j );
		if ( j > NUM_RNA_MAINCHAIN_TORSIONS ) rna_torsion_id = id::TorsionID( i, id::CHI, j - NUM_RNA_MAINCHAIN_TORSIONS );
		pose.set_torsion( rna_torsion_id, start_torsions[ j ] );
	}

}

////////////////////////////////////////////////////////////////////
//FANG: All these sugar coord stuffs should be deprecated in favor of
//RNA_IdealCoord class? Are there any performance concern for using
//copy_dof_match_atom_name there?
void
apply_ideal_c2endo_sugar_coords(
	Pose & pose,
	Size const i
)
{

	static bool const use_phenix_geo = basic::options::option[  basic::options::OptionKeys::rna::corrected_geo ]();
	if ( use_phenix_geo ) {
		apply_pucker( pose, i, SOUTH, false /*skip_same_state*/, true /*idealize_coord*/ );
		return;
	}

	//Torsion angles associated with a 2'-endo sugar in 1jj2 (large ribosomal subunit xtal structure ).
	pose.set_torsion( id::TorsionID( i, id::BB, 1), 69.404192 );
	pose.set_torsion( id::TorsionID( i, id::BB, 2), -173.031790 );
	pose.set_torsion( id::TorsionID( i, id::BB, 3), 58.877828 );
	pose.set_torsion( id::TorsionID( i, id::BB, 4), 147.202313 );
	pose.set_torsion( id::TorsionID( i, id::BB, 5), -85.360367 );
	pose.set_torsion( id::TorsionID( i, id::BB, 6), -38.381256 );
	pose.set_torsion( id::TorsionID( i, id::CHI, 1), 111.708846 );
	pose.set_torsion( id::TorsionID( i, id::CHI, 2), -36.423711 );
	pose.set_torsion( id::TorsionID( i, id::CHI, 3), 156.438552 );
	pose.set_torsion( id::TorsionID( i, id::CHI, 4), 179.890442 );

	static utility::vector1< Vector > _non_main_chain_sugar_coords;
	_non_main_chain_sugar_coords.push_back( Vector(  0.329122,   -0.190929,   -1.476983  ) );
	_non_main_chain_sugar_coords.push_back( Vector( -0.783512,   -1.142556,   -1.905737  ) );
	_non_main_chain_sugar_coords.push_back( Vector( -1.928054,   -0.731911,   -1.195034  ) );

	apply_non_main_chain_sugar_coords( _non_main_chain_sugar_coords, pose, pose, i );
}

////////////////////////////////////////////////////////////////////
PuckerState
assign_pucker(
	Pose const & pose,
	Size const rsd_id
) {
	Real const delta = pose.torsion( id::TorsionID( rsd_id, id::BB,  4 ) );
	PuckerState const pucker_state = ( delta < torsion_info.delta_cutoff() ) ? NORTH : SOUTH;
	return pucker_state;
}
////////////////////////////////////////////////////////////////////
void
apply_pucker(
	Pose & pose,
	Size const i,
	PuckerState pucker_state, //0 for using the current pucker
	bool const skip_same_state,
	bool const idealize_coord
) {
	debug_assert( pucker_state <= 2 );

	static const RNA_IdealCoord ideal_coord;
	Real delta, nu1, nu2;

	PuckerState const curr_pucker = assign_pucker( pose, i );
	if ( skip_same_state && pucker_state == curr_pucker ) return;

	if ( pucker_state == core::chemical::rna::ANY_PUCKER ) pucker_state = curr_pucker;

	if ( idealize_coord ) {
		ideal_coord.apply_pucker(pose, i, pucker_state);
	} else {
		if ( pucker_state == NORTH ) {
			delta = torsion_info.delta_north();
			nu2 = torsion_info.nu2_north();
			nu1 = torsion_info.nu1_north();
		} else {
			delta = torsion_info.delta_south();
			nu2 = torsion_info.nu2_south();
			nu1 = torsion_info.nu1_south();
		}
		pose.set_torsion( id::TorsionID( i, id::BB,  4 ), delta );
		pose.set_torsion( id::TorsionID( i, id::CHI, 2 ), nu2 );
		pose.set_torsion( id::TorsionID( i, id::CHI, 3 ), nu1 );
	}
}
////////////////////////////////////////////////////////////////////


//When a CUTPOINT_UPPER is added to 3' chain_break residue, the EXISTENCE of the CUTPOINT_UPPER atoms means that the alpha torsion which previously DOES NOT exist due to the chain_break now exist. The alpha value is automatically defined to the A-form value by Rosetta. However Rosetta does not automatically adjust the OP2 and OP1 atom position to account for this fact. So it is important that the OP2 and OP1 atoms position are correctly set to be consistent with A-form alpha torsion before the CUTPOINT_UPPER IS ADDED Parin Jan 2, 2009
void
correctly_position_cutpoint_phosphate_torsions( pose::Pose & current_pose, Size const five_prime_chainbreak ){

	using namespace core::chemical;
	using namespace core::conformation;
	using namespace core::id;
	using namespace core::io::pdb;

	static const ResidueTypeSetCOP rsd_set = core::chemical::ChemicalManager::get_instance()->residue_type_set( core::chemical::FA_STANDARD );

	chemical::AA res_aa = aa_from_name( "RAD" );
	ResidueOP new_rsd = conformation::ResidueFactory::create_residue( *( rsd_set->get_representative_type_aa( res_aa ) ) );

	Size three_prime_chainbreak = five_prime_chainbreak + 1;
	current_pose.prepend_polymer_residue_before_seqpos( *new_rsd, three_prime_chainbreak, true );
	chemical::rna::RNA_FittedTorsionInfo const rna_fitted_torsion_info;

	//Actually just by prepending the residue causes the alpha torsion to automatically be set to -64.0274,
	//so the manual setting below is actually not needed, May 24, 2010.. Parin S.
	//These are the initial value of virtual upper and lower cutpoint atom.
	//Actually only the alpha (id::BB, 1) is important here since it set the position of O3' (LOWER) atom which in turn determines  OP2 and OP1 atom
	current_pose.set_torsion( TorsionID( three_prime_chainbreak + 1, id::BB, 1 ), -64.027359 );

	/* BEFORE AUG 24, 2011
	//Where the hell did I get these numbers from value...by appending with ideal geometry and look at the initalized value? Oct 13, 2009
	current_pose.set_torsion( TorsionID( five_prime_chainbreak + 1, id::BB, 5 ), -151.943 ); //Not Important?
	current_pose.set_torsion( TorsionID( five_prime_chainbreak + 1, id::BB, 6 ), -76.4185 ); //Not Important?
	current_pose.set_torsion( TorsionID( three_prime_chainbreak + 1, id::BB, 1 ), -64.0274 );
	*/

	//RAD.params
	//ICOOR_INTERNAL  LOWER  -64.027359   71.027062    1.593103   P     O5'   C5'
	//ICOOR_INTERNAL    OP2 -111.509000   71.937134    1.485206   P     O5' LOWER
	//ICOOR_INTERNAL    OP1 -130.894000   71.712189    1.485010   P     O5'   OP2

	//RCY.params
	//ICOOR_INTERNAL  LOWER  -64.027359   71.027062    1.593103   P     O5'   C5'
	//ICOOR_INTERNAL    OP2 -111.509000   71.937134    1.485206   P     O5' LOWER
	//ICOOR_INTERNAL    OP1 -130.894000   71.712189    1.485010   P     O5'   OP2

	//RGU.params
	//ICOOR_INTERNAL  LOWER  -64.027359   71.027062    1.593103   P     O5'   C5'
	//ICOOR_INTERNAL    OP2 -111.509000   71.937134    1.485206   P     O5' LOWER
	//ICOOR_INTERNAL    OP1 -130.894000   71.712189    1.485010   P     O5'   OP2

	//URA.parms
	//ICOOR_INTERNAL  LOWER  -64.027359   71.027062    1.593103   P     O5'   C5'
	//ICOOR_INTERNAL    OP2 -111.509000   71.937134    1.485206   P     O5' LOWER
	//ICOOR_INTERNAL    OP1 -130.894000   71.712189    1.485010   P     O5'   OP2

	current_pose.delete_polymer_residue( five_prime_chainbreak + 1 );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// try to unify all cutpoint addition into this function.
void
correctly_add_cutpoint_variants( core::pose::Pose & pose,
	Size const res_to_add,
	bool const check_fold_tree /* = true*/){

	using namespace core::chemical;

	runtime_assert( res_to_add < pose.total_residue() );
	if ( check_fold_tree ) runtime_assert( pose.fold_tree().is_cutpoint( res_to_add ) );

	remove_variant_type_from_pose_residue( pose, UPPER_TERMINUS_VARIANT, res_to_add );
	remove_variant_type_from_pose_residue( pose, LOWER_TERMINUS_VARIANT, res_to_add + 1 );

	remove_variant_type_from_pose_residue( pose, THREE_PRIME_PHOSPHATE, res_to_add );
	remove_variant_type_from_pose_residue( pose, VIRTUAL_PHOSPHATE, res_to_add + 1 );
	remove_variant_type_from_pose_residue( pose, FIVE_PRIME_PHOSPHATE, res_to_add + 1 );

	if ( pose.residue_type( res_to_add ).is_RNA() ) {
		// could also keep track of alpha, beta, etc.
		runtime_assert( pose.residue_type( res_to_add + 1 ).is_RNA() );
		correctly_position_cutpoint_phosphate_torsions( pose, res_to_add );
	}
	add_variant_type_to_pose_residue( pose, CUTPOINT_LOWER, res_to_add   );
	add_variant_type_to_pose_residue( pose, CUTPOINT_UPPER, res_to_add + 1 );
}

///////
bool
is_cutpoint_closed_by_atom_name(
	chemical::ResidueType const & rsd_1,
	chemical::ResidueType const & rsd_2,
	chemical::ResidueType const & rsd_3,
	chemical::ResidueType const & rsd_4,
	id::AtomID const & id1,
	id::AtomID const & id2,
	id::AtomID const & id3,
	id::AtomID const & id4 ){
	return(
		is_cutpoint_closed_atom( rsd_1, id1 ) ||
		is_cutpoint_closed_atom( rsd_2, id2 ) ||
		is_cutpoint_closed_atom( rsd_3, id3 ) ||
		is_cutpoint_closed_atom( rsd_4, id4 ) );
}

// Useful functions to torsional potential
bool
is_torsion_valid(
	pose::Pose const & pose,
	id::TorsionID const & torsion_id,
	bool verbose,
	bool skip_chainbreak_torsions
) {

	id::AtomID id1, id2, id3, id4;

	bool is_fail = pose.conformation().get_torsion_angle_atom_ids( torsion_id, id1, id2, id3, id4 );
	if ( verbose ) TR << "torsion_id: " << torsion_id << std::endl;
	if ( is_fail ) {
		if ( verbose ) TR << "fail to get torsion!, perhap this torsion is located at a chain_break " << std::endl;
		return false;
	}

	chemical::ResidueType const & rsd_1 = pose.residue_type( id1.rsd() );
	chemical::ResidueType const & rsd_2 = pose.residue_type( id2.rsd() );
	chemical::ResidueType const & rsd_3 = pose.residue_type( id3.rsd() );
	chemical::ResidueType const & rsd_4 = pose.residue_type( id4.rsd() );

	if ( !rsd_1.is_RNA() || !rsd_2.is_RNA() ||
			!rsd_3.is_RNA() || !rsd_4.is_RNA() ) return false;

	bool is_virtual_torsion = (
		rsd_1.is_virtual( id1.atomno() ) ||
		rsd_2.is_virtual( id2.atomno() ) ||
		rsd_3.is_virtual( id3.atomno() ) ||
		rsd_4.is_virtual( id4.atomno() ) );

	if ( is_virtual_torsion && verbose ) {
		print_torsion_info( pose, torsion_id );
	}

	// Check for cutpoint_closed
	// (Since these torsions will contain virtual atom(s),
	// but want to score these torsions
	bool const is_cutpoint_closed1 = is_cutpoint_closed_torsion( pose, torsion_id );
	debug_assert( is_cutpoint_closed1 == is_cutpoint_closed_by_atom_name( rsd_1, rsd_2, rsd_3, rsd_4, id1, id2, id3, id4) ); // takes time

	if ( is_cutpoint_closed1 && !is_virtual_torsion ) {
		print_torsion_info( pose, torsion_id );
		utility_exit_with_message( "is_cutpoint_closed1 == true && is_virtual_torsion == false!!" );
	}


	runtime_assert( id1.rsd() <= id2.rsd() );
	runtime_assert( id2.rsd() <= id3.rsd() );
	runtime_assert( id3.rsd() <= id4.rsd() );
	runtime_assert( ( id1.rsd() == id4.rsd() )
		|| ( id1.rsd() == ( id4.rsd() - 1 ) ) );

	bool const inter_residue_torsion = ( id1.rsd() != id4.rsd() );

	bool is_chain_break_torsion( false );

	if ( inter_residue_torsion ) {
		// Note that chain_break_torsion does not neccessarily have to be located
		// at a cutpoint_open. For example, in RNA might contain multiple strands,
		// but the user not have specified them as cutpoint_open
		// This happen frequently, for example when modeling single-stranded RNA
		// loop (PNAS December 20, 2011 vol. 108 no. 51 20573-20578).
		// Actually if chain_break is cutpoint_open,
		// pose.conformation().get_torsion_angle_atom_ids() should fail, which
		// leads to the EARLY RETURN FALSE statement at the beginning of
		// this function.
		bool const violate_max_O3_prime_to_P_bond_dist =
			pose::rna::is_rna_chainbreak( pose, id1.rsd() );

		// Note that cutpoint_closed_torsions are NOT considered as
		// chain_break_torsion since we want to score them EVEN when
		// skip_chainbreak_torsions_=true!
		// Necessary since for cutpoint_closed_torsions, the max
		// O3_prime_to_P_bond_dist might be violated during stages of the
		// Fragment Assembly and Stepwise Assembly where the chain is
		// not yet closed.
		is_chain_break_torsion = ( violate_max_O3_prime_to_P_bond_dist
			&& !is_cutpoint_closed1 );
	}

	// Warning before Jan 20, 2012, this used to be
	// "Size should_score_this_torsion= true;"
	bool should_score_this_torsion = true;

	if ( is_virtual_torsion && !is_cutpoint_closed1 ) should_score_this_torsion = false;

	if ( skip_chainbreak_torsions && is_chain_break_torsion ) should_score_this_torsion = false;

	if ( verbose ) {
		output_boolean( " should_score_torsion = ", should_score_this_torsion );
		output_boolean( " | is_cutpoint_closed = ", is_cutpoint_closed1 );
		output_boolean( " | is_virtual_torsion = ", is_virtual_torsion );
		output_boolean( " | skip_chainbreak_torsions = ", skip_chainbreak_torsions );
		output_boolean( " | is_chain_break_torsion   = ", is_chain_break_torsion ) ;
		TR << std::endl;
	}
	return should_score_this_torsion;
}

void
print_torsion_info(
	pose::Pose const & pose,
	id::TorsionID const & torsion_id
) {
	id::AtomID id1, id2, id3, id4;
	pose.conformation().get_torsion_angle_atom_ids(
		torsion_id, id1, id2, id3, id4 );

	TR << "torsion_id: " << torsion_id << std::endl;

	bool is_fail = pose.conformation().get_torsion_angle_atom_ids(
		torsion_id, id1, id2, id3, id4 );
	if ( is_fail ) {
		TR << "fail to get torsion!, perhap this torsion is located at a chain_break " << std::endl;
		return;
	}

	chemical::ResidueType const & rsd_1 = pose.residue_type( id1.rsd() );
	chemical::ResidueType const & rsd_2 = pose.residue_type( id2.rsd() );
	chemical::ResidueType const & rsd_3 = pose.residue_type( id3.rsd() );
	chemical::ResidueType const & rsd_4 = pose.residue_type( id4.rsd() );

	TR << " Torsion containing one or more virtual atom( s )" << std::endl;
	TR << "  torsion_id: " << torsion_id;
	TR << "  atom_id: " << id1 << " " << id2 << " " << id3 << " " << id4 << std::endl;
	TR << "  name: " << rsd_1.atom_name( id1.atomno() ) << " " <<
		rsd_2.atom_name( id2.atomno() ) << " " <<
		rsd_3.atom_name( id3.atomno() ) << " " <<
		rsd_4.atom_name( id4.atomno() ) << std::endl;
	TR << "  type: " << rsd_1.atom_type( id1.atomno() ).name() << " " <<
		rsd_2.atom_type( id2.atomno() ).name() << " " <<
		rsd_3.atom_type( id3.atomno() ).name() << " " <<
		rsd_4.atom_type( id4.atomno() ).name() << std::endl;
	TR << "\t\tatom_type_index: " << rsd_1.atom( id1.atomno() ).atom_type_index() <<
		" " << rsd_2.atom( id2.atomno() ).atom_type_index() << " " <<
		rsd_3.atom( id3.atomno() ).atom_type_index() << " " <<
		rsd_4.atom( id4.atomno() ).atom_type_index() << std::endl;
	TR << "\t\tatomic_charge: " << rsd_1.atom( id1.atomno() ).charge() <<
		" " << rsd_2.atom( id2.atomno() ).charge() << " " <<
		rsd_3.atom( id3.atomno() ).charge() << " " <<
		rsd_4.atom( id4.atomno() ).charge() << std::endl;
}

void
output_boolean( std::string const & tag, bool boolean ) {
	using namespace ObjexxFCL;
	using namespace ObjexxFCL::format;
	TR << tag;
	if ( boolean ) {
		TR << A( 4, "T" );
	} else {
		TR << A( 4, "F" );
	}
}

bool
is_cutpoint_closed_atom(
	core::chemical::ResidueType const & rsd,
	id::AtomID const & id
) {
	std::string const & atom_name = rsd.atom_name( id.atomno() );
	if ( atom_name == "OVU1" || atom_name == "OVL1" || atom_name == "OVL2" ) {
		return true;
	} else {
		return false;
	}
}

bool
is_cutpoint_closed_torsion(
	pose::Pose const & pose,
	id::TorsionID const & torsion_id
) {
	using namespace ObjexxFCL;
	Size torsion_seq_num = torsion_id.rsd();
	Size lower_seq_num = 0;
	Size upper_seq_num = 0;

	if ( torsion_id.type() != id::BB ) return false;

	if ( torsion_id.torsion() == ALPHA ) { //COULD BE A UPPER RESIDUE OF A CHAIN_BREAK_CLOSE

		lower_seq_num = torsion_seq_num - 1;
		upper_seq_num = torsion_seq_num;

	} else if ( torsion_id.torsion() == EPSILON || torsion_id.torsion() == ZETA ) {
		lower_seq_num = torsion_seq_num;
		upper_seq_num = torsion_seq_num + 1;
	} else {
		if ( torsion_id.torsion() != DELTA && torsion_id.torsion() != BETA && torsion_id.torsion() != GAMMA ) {
			utility_exit_with_message( "The torsion should be DELTA( lower ), BETA( upper ) or GAMMA( upper ) !!" );
		}
		return false;
	}

	if ( upper_seq_num == 1 ) return false;

	if ( lower_seq_num == pose.total_residue() ) return false;

	if ( pose.residue_type( lower_seq_num ).has_variant_type( chemical::CUTPOINT_LOWER ) ) {
		if ( pose.residue_type( upper_seq_num ).has_variant_type( chemical::CUTPOINT_UPPER ) == false ) {
			utility_exit_with_message( "seq_num " + string_of( lower_seq_num ) + " is a CUTPOINT_LOWER but seq_num " + string_of( upper_seq_num ) + " is not a cutpoint CUTPOINT_UPPER??" );
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
apply_virtual_rna_residue_variant_type( core::pose::Pose & pose, core::Size const & seq_num, bool const apply_check ){

	utility::vector1< Size > working_cutpoint_closed_list;
	working_cutpoint_closed_list.clear(); //empty list
	apply_virtual_rna_residue_variant_type( pose, seq_num, working_cutpoint_closed_list, apply_check );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
apply_virtual_rna_residue_variant_type( core::pose::Pose & pose, core::Size const & seq_num, utility::vector1< core::Size > const & working_cutpoint_closed_list, bool const apply_check ){

	using namespace core::chemical;
	using namespace ObjexxFCL;

	runtime_assert( pose.total_residue() >= seq_num );
	if ( pose.residue_type( seq_num ).has_variant_type( VIRTUAL_RNA_RESIDUE ) ) return;
	// AMW TODO: no reason these should actually be incompatible!
	//Basically the two variant type are not compatible, VIRTUAL_RNA_RESIDUE variant type currently does not virtualize the protonated H1 atom.
	if ( pose.residue( seq_num ).has_variant_type( PROTONATED_N1_ADENOSINE ) ) {
		TR << "Removing PROTONATED_N1_ADENOSINE variant_type from seq_num = " << seq_num <<
			" before adding VIRTUAL_RNA_RESIDUE variant_type since the two variant_types are not compatible!" <<
			std::endl;
		pose::remove_variant_type_from_pose_residue( pose, PROTONATED_N1_ADENOSINE, seq_num );
	}

	//OK PROTONATED_N1_ADENOSINE variant type should also be removed when adding VIRTUAL_RNA_RESIDUE_EXCLUDE_PHOSPHATE variant type or BULGE variant type.
	//However these two variant type are not currently used in standard SWA run (May 04, 2011)
	bool is_cutpoint_closed = false;
	if ( pose.residue_type( seq_num ).has_variant_type( chemical::CUTPOINT_LOWER ) ) {
		runtime_assert( pose.residue_type( seq_num + 1 ).has_variant_type( chemical::CUTPOINT_UPPER ) );
		is_cutpoint_closed = true;
	}

	//Ok another possibility is that the CUTPOINT_LOWER and CUTPOINT_UPPER variant type had not been applied yet..so check the working_cutpoint_closed_list
	for ( Size n = 1; n <= working_cutpoint_closed_list.size(); n++ ) {
		if ( seq_num == working_cutpoint_closed_list[n] ) {
			is_cutpoint_closed = true;
			break;
		}
	}

	bool const is_cutpoint_open = ( pose.fold_tree().is_cutpoint( seq_num ) && !is_cutpoint_closed );
	if ( apply_check ) {
		if ( is_cutpoint_open ) {
			std::cerr << pose.annotated_sequence() << std::endl;
			std::cerr << pose.fold_tree();
			utility_exit_with_message( "Cannot apply VIRTUAL_RNA_RESIDUE VARIANT TYPE to seq_num: " + string_of( seq_num ) + ". The residue is 5' of a OPEN cutpoint" );
		}
		if ( pose.total_residue() == seq_num ) {
			utility_exit_with_message( "Cannot apply VIRTUAL_RNA_RESIDUE VARIANT TYPE to seq_num: " + string_of( seq_num ) + ". pose.total_residue() == seq_num" );
		}
	}

	pose::add_variant_type_to_pose_residue( pose, VIRTUAL_RNA_RESIDUE, seq_num );

	if ( ( pose.total_residue() != seq_num ) &&  ( !is_cutpoint_open ) ) { //April 6, 2011
		pose::add_variant_type_to_pose_residue( pose, VIRTUAL_PHOSPHATE, seq_num + 1 );
	}
}

//////////////////////////////////////////////////////////////////////////////////////
void
remove_virtual_rna_residue_variant_type( pose::Pose & pose, Size const & seq_num ){

	using namespace core::chemical;
	using namespace ObjexxFCL;

	runtime_assert( seq_num < pose.total_residue() );
	pose::remove_variant_type_from_pose_residue( pose, VIRTUAL_RNA_RESIDUE, seq_num );
	pose::remove_variant_type_from_pose_residue( pose, VIRTUAL_PHOSPHATE, seq_num + 1 );
}

//////////////////////////////////////////////////////////////////////////////////////
bool
has_virtual_rna_residue_variant_type( pose::Pose & pose, Size const & seq_num ){

	using namespace ObjexxFCL;

	if ( ! pose.residue_type( seq_num ).has_variant_type( chemical::VIRTUAL_RNA_RESIDUE ) ) return false;

	if ( ( seq_num + 1 ) > pose.total_residue() ) { //Check in range
		TR << "( seq_num + 1 ) = " << ( seq_num + 1 )  << std::endl;
		utility_exit_with_message( "( seq_num + 1 ) > pose.total_residue()!" );
	}

	if ( ! pose.residue_type( seq_num + 1 ).has_variant_type( chemical::VIRTUAL_PHOSPHATE ) ) {
		TR << "Problem seq_num = " << seq_num << std::endl;
		utility_exit_with_message( "res ( " + string_of( seq_num ) +
			" ) has_variant_type VIRTUAL_RNA_RESIDUE but res seq_num + 1 ( " + string_of( seq_num + 1 ) +
			" )does not have variant_type VIRTUAL_PHOSPHATE" );
	}

	return true;

}

//////////////////////////////////////////////////////////////////////////////////////
void
apply_Aform_torsions( pose::Pose & pose, Size const n ){
	using namespace core::id;
	// following does not really matter, since sugar & phosphate will be erased anyway.
	core::chemical::rna::RNA_FittedTorsionInfo const torsion_info_;
	pose.set_torsion( TorsionID( n, BB, 1), torsion_info_.alpha_aform() );
	pose.set_torsion( TorsionID( n, BB, 2), torsion_info_.beta_aform() );
	pose.set_torsion( TorsionID( n, BB, 3), torsion_info_.gamma_aform() );
	pose.set_torsion( TorsionID( n, BB, 4), torsion_info_.delta_north() );
	pose.set_torsion( TorsionID( n, BB, 5), torsion_info_.epsilon_aform() );
	pose.set_torsion( TorsionID( n, BB, 6), torsion_info_.zeta_aform() );
	apply_pucker(pose, n, NORTH, false /*skip_same_state*/, true /*use_phenix_geo_*/);
}


//////////////////////////////////////////////////////////////////////////////////////
ChiState
get_residue_base_state( core::pose::Pose const & pose, Size const seq_num ){
	return core::chemical::rna::get_residue_base_state( pose.residue( seq_num ) );
}

//////////////////////////////////////////////////////////////////////////////////////
PuckerState
get_residue_pucker_state( core::pose::Pose const & pose, Size const seq_num ){
	return core::chemical::rna::get_residue_pucker_state( pose.residue( seq_num ) );
}

/////////////////////////////////////////////////////////////////////////////////////
/// Could this be made more general? Figured out through knowledge of where side
///  chain atoms connect to polymeric backbone?
utility::vector1< std::pair< core::id::TorsionID, core::Real > >
get_suite_torsion_info( core::pose::Pose const & pose, Size const i )
{
	using namespace utility;
	using namespace core::id;
	vector1< TorsionID > torsion_ids;
	if ( pose.residue_type( i ).is_NA() ) {
		torsion_ids.push_back( TorsionID( i  , BB, 5 ) ); // epsilon
		torsion_ids.push_back( TorsionID( i  , BB, 6 ) ); // zeta
		torsion_ids.push_back( TorsionID( i+1, BB, 1 ) ); // alpha
		torsion_ids.push_back( TorsionID( i+1, BB, 2 ) ); // beta
		torsion_ids.push_back( TorsionID( i+1, BB, 3 ) ); // gamma
	} else if ( pose.residue_type( i ).is_protein() ) {
		torsion_ids.push_back( TorsionID( i  , BB, 2 ) ); // psi
		torsion_ids.push_back( TorsionID( i  , BB, 3 ) ); // omega
		torsion_ids.push_back( TorsionID( i+1, BB, 1 ) ); // phi
	}
	vector1< std::pair< TorsionID, Real > > suite_torsion_info;
	for ( Size n = 1; n <= torsion_ids.size(); n++ ) {
		suite_torsion_info.push_back( std::make_pair( torsion_ids[ n ], pose.torsion( torsion_ids[ n ] ) ) );
	}
	return suite_torsion_info;
}

/////////////////////////////////////////////////////////////////////////////////////
void
apply_suite_torsion_info( core::pose::Pose & pose,
	utility::vector1< std::pair< core::id::TorsionID, core::Real > > const & suite_torsion_info ) {
	for ( Size n = 1; n <= suite_torsion_info.size(); n++ ) {
		pose.set_torsion( suite_torsion_info[ n ].first, suite_torsion_info[ n ].second );
	}
}

////////////////////////////////////////////////////////////////////////////////////
Real
get_op2_op1_sign( pose::Pose const & pose ) {

	Real sign= 0;
	bool found_valid_sign=false;

	for ( Size i = 2; i <= pose.total_residue(); i++ ) {

		conformation::Residue const & rsd( pose.residue( i )  );
		if ( !rsd.is_RNA() ) continue;

		sign = dot( rsd.xyz( " O5'" ) - rsd.xyz( " P  " ), cross( rsd.xyz( " OP1" ) - rsd.xyz( " P  " ), rsd.xyz( " OP2" ) - rsd.xyz( " P  " ) ) );
		found_valid_sign = true;
		break;
	}

	if ( found_valid_sign==false ) utility_exit_with_message("found_valid_sign==false");

	return sign;
}

////////////////////////////////////////////////////////////////////////////////////
//This version used to be called get_op2_op1_sign_parin()
Real
get_op2_op1_sign( pose::Pose const & pose , Size res_num) {

	if ( res_num > pose.total_residue() ) utility_exit_with_message("res_num > pose.total_residue()");

	conformation::Residue const & rsd( pose.residue(res_num)  );

	//SML PHENIX conference cleanup
	if ( basic::options::option[basic::options::OptionKeys::rna::rna_prot_erraser].value() ) {
		if ( !rsd.is_RNA() ) return 0.0;
	} else {
		if ( rsd.is_RNA()==false ) utility_exit_with_message("rsd.is_RNA()==false!");
	}

	Real const sign = dot( rsd.xyz( " O5'" ) - rsd.xyz( " P  " ), cross( rsd.xyz( " OP1" ) - rsd.xyz( " P  " ), rsd.xyz( " OP2" ) - rsd.xyz( " P  " ) ) );

	return sign;
}

////////////////////////////////////////////////////////////////
void
make_phosphate_nomenclature_matches_mini( pose::Pose & pose)
{
	using namespace ObjexxFCL;
	pose::Pose mini_pose;
	make_pose_from_sequence( mini_pose, "aa", pose.residue_type( 1 ).residue_type_set());
	Real const sign2 = get_op2_op1_sign( mini_pose);

	for ( Size res_num=1; res_num<=pose.total_residue(); res_num++ ) {

		if ( !pose.residue( res_num ).is_RNA() ) continue;
		Real sign1 = get_op2_op1_sign( pose,  res_num);

		if ( sign1 * sign2 < 0 ) {
			//std::cout << " Flipping OP2 <--> OP1 " << "res_num " << res_num << " | sign1: " << sign1 << " | sign2: " << sign2 << std::endl;
			conformation::Residue const & rsd( pose.residue(res_num) );

			if ( rsd.is_RNA()==false ) { //Consistency check!
				std::cout << "residue # " << res_num << " should be a RNA nucleotide!" << std::endl;
				utility_exit_with_message("residue # " + string_of(res_num)+ " should be a RNA nucleotide!");
			};

			Vector const temp1 = rsd.xyz( " OP2" );
			Vector const temp2 = rsd.xyz( " OP1" );
			pose.set_xyz( id::AtomID( rsd.atom_index( " OP2" ), res_num ), temp2 );
			pose.set_xyz( id::AtomID( rsd.atom_index( " OP1" ), res_num ), temp1 );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
add_virtual_O2Prime_hydrogen( core::pose::Pose & pose ){
	for ( core::Size i = 1; i <= pose.total_residue(); i++ ) {
		if ( !pose.residue_type( i ).is_RNA() ) continue;
		pose::add_variant_type_to_pose_residue( pose, core::chemical::VIRTUAL_O2PRIME_HYDROGEN, i );
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Took these functions out of the class so that they are accessible in the VDWGridEnergy for scoring
// without actually needing to construct an RNA_VDW_BinChecker object
Atom_Bin
get_atom_bin( numeric::xyzVector< core::Real > const & atom_pos, numeric::xyzVector< core::Real > const & ref_xyz,
	core::Real const atom_bin_size, int const bin_offset ) {

	numeric::xyzVector< core::Real > const atom_pos_ref_frame = atom_pos - ref_xyz;

	Atom_Bin atom_bin;
	atom_bin.x = int( atom_pos_ref_frame[0]/atom_bin_size );
	atom_bin.y = int( atom_pos_ref_frame[1]/atom_bin_size );
	atom_bin.z = int( atom_pos_ref_frame[2]/atom_bin_size );


	if ( atom_pos_ref_frame[0] < 0 ) atom_bin.x--;
	if ( atom_pos_ref_frame[1] < 0 ) atom_bin.y--;
	if ( atom_pos_ref_frame[2] < 0 ) atom_bin.z--;

	//////////////////////////////////////////////////////////
	atom_bin.x += bin_offset; //Want min bin to be at one.
	atom_bin.y += bin_offset; //Want min bin to be at one.
	atom_bin.z += bin_offset; //Want min bin to be at one.


	//////////////////////////////////////////////////////////
	return atom_bin;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool
is_atom_bin_in_range( Atom_Bin const & atom_pos_bin, int const bin_max ) {

	if ( atom_pos_bin.x < 1 || ( atom_pos_bin.x > ( bin_max*2 ) ) ||
			atom_pos_bin.y < 1 || ( atom_pos_bin.y > ( bin_max*2 ) ) ||
			atom_pos_bin.z < 1 || ( atom_pos_bin.z > ( bin_max*2 ) ) ) {

		return false;

	} else {
		return true;
	}

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
utility::vector1< std::string >
tokenize( std::string const & str, std::string const & delimiters ){
	using namespace std;

	utility::vector1< std::string > tokens;

	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of( delimiters, 0 );
	// Find first "non-delimiter".
	string::size_type pos     = str.find_first_of( delimiters, lastPos );

	while ( string::npos != pos || string::npos != lastPos ) {
		// Found a token, add it to the vector.
		tokens.push_back( str.substr( lastPos, pos - lastPos ) );
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of( delimiters, pos );
		// Find next "non-delimiter"
		pos = str.find_first_of( delimiters, lastPos );
	}
	return tokens;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
core::Size
string_to_int( std::string const & input_string ){

	Size int_of_string; //misnomer
	std::stringstream ss ( std::stringstream::in | std::stringstream::out );

	ss << input_string;

	if ( ss.fail() ) utility_exit_with_message( "In string_to_real(): ss.fail() for ss << input_string | string ( " + input_string + " )" );

	ss >> int_of_string;

	if ( ss.fail() ) utility_exit_with_message( "In string_to_real(): ss.fail() for ss >> int_of_string | string ( " + input_string + " )" );

	return int_of_string;
}

std::string
remove_bracketed( std::string const & sequence ) {
	std::string return_val = sequence;

	std::string::size_type i = return_val.find( '[' );
	while ( i != std::string::npos ) {

		std::string::size_type j = return_val.find( ']' );
		if ( j == std::string::npos ) utility_exit_with_message( "String with imbalanced brackets passed to remove_bracketed! (" + sequence + ")" );

		return_val.erase( i, j - i + 1 );

		i = return_val.find( '[' );
	}

	return return_val;
}

void
remove_and_store_bracketed( std::string const & working_sequence, std::string & working_sequence_clean, std::map< Size, std::string > & special_res ) {
	working_sequence_clean = working_sequence;

	std::string::size_type i = working_sequence_clean.find( '[' );
	while ( i != std::string::npos ) {

		std::string::size_type j = working_sequence_clean.find( ']' );
		if ( j == std::string::npos ) utility_exit_with_message( "String with imbalanced brackets passed to remove_bracketed! (" + working_sequence + ")" );

		std::string temp = working_sequence_clean.substr( i + 1, j - i - 1 );
		special_res[ i - 1 ] = temp;
		working_sequence_clean.erase( i, j - i + 1 );

		i = working_sequence_clean.find( '[' );
	}
}

} //ns rna
} //ns pose
} //ns core
