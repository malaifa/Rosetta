// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/simple_moves/hbs/HbsPatcher.cc
/// @brief HbsPatcher methods implemented
/// @author Andy Watkins, amw579@nyu.edu

// Unit Headers
#include <protocols/simple_moves/hbs/HbsPatcher.hh>
// Package Headers

// Project Headers
#include <core/scoring/constraints/ConstraintSet.hh>
#include <core/scoring/constraints/AtomPairConstraint.hh>
#include <core/scoring/constraints/DihedralConstraint.hh>
#include <core/scoring/constraints/AngleConstraint.hh>
#include <core/scoring/constraints/ConstraintSet.fwd.hh>
#include <core/scoring/func/HarmonicFunc.hh>
#include <core/scoring/func/CircularHarmonicFunc.hh>
#include <core/chemical/ChemicalManager.hh>
#include <core/chemical/ResidueType.hh>
#include <core/chemical/ResidueTypeFinder.hh>
#include <core/chemical/ResidueTypeSet.hh>
#include <core/chemical/VariantType.hh>
#include <core/chemical/Patch.hh>
#include <core/conformation/Residue.hh>
#include <core/conformation/Conformation.hh>
#include <core/conformation/util.hh>
#include <core/pose/Pose.hh>
#include <core/id/AtomID.hh>
// Utility Headers
#include <numeric/xyz.functions.hh>
#include <numeric/NumericTraits.hh>
#include <basic/Tracer.hh>
#include <basic/basic.hh>
#include <core/types.hh>

// C++ Headers

using basic::T;
using basic::Error;
using basic::Warning;

static THREAD_LOCAL basic::Tracer TR( "protocols.simple_moves.hbs.HbsPatcher" );


using namespace core;
using namespace conformation;
using namespace chemical;
using namespace core::id;

namespace protocols {
namespace simple_moves {
namespace hbs {

void add_hbs_constraint( core::pose::Pose & pose, core::Size hbs_pre_position )
{
	add_hbs_constraint( pose, hbs_pre_position, 1.52, 0.05 );
}
void add_hbs_constraint( core::pose::Pose & pose, core::Size hbs_pre_position, core::Real distance, core::Real std )
{
	using namespace core::id;
	using namespace core::scoring;
	using namespace core::scoring::func;
	using namespace core::scoring::constraints;

	//kdrew: add constraint
	HarmonicFuncOP harm_func( new HarmonicFunc( distance, std ) );
	HarmonicFuncOP harm_func_0( new HarmonicFunc( 0, std ) );
	CircularHarmonicFuncOP ang_func_120( new CircularHarmonicFunc( numeric::NumericTraits<float>::pi_2_over_3(), 0.02 ) );
	//CircularHarmonicFuncOP ang_func_60( new CircularHarmonicFunc( numeric::NumericTraits<float>::pi_over_3(), 0.02 ) );
	CircularHarmonicFuncOP dih_func_180( new CircularHarmonicFunc( numeric::NumericTraits<float>::pi(), 0.02 ) );
	CircularHarmonicFuncOP dih_func_0( new CircularHarmonicFunc( 0, 0.02 ) );

	AtomID aidCYH( pose.residue( hbs_pre_position ).atom_index("CYH"), hbs_pre_position );
	AtomID aidHYH( pose.residue( hbs_pre_position ).atom_index("HYH"), hbs_pre_position );
	AtomID aidCZH( pose.residue( hbs_pre_position+2 ).atom_index("CZH"), hbs_pre_position+2 );
	AtomID aidVZH( pose.residue( hbs_pre_position ).atom_index("VZH"), hbs_pre_position );
	AtomID aidVYH( pose.residue( hbs_pre_position+2 ).atom_index("VYH"), hbs_pre_position+2 );
	AtomID aidN( pose.residue( hbs_pre_position+2 ).atom_index("N"), hbs_pre_position+2 );
	AtomID aidCY2( pose.residue( hbs_pre_position ).atom_index("CY2"), hbs_pre_position );
	AtomID aidCY1( pose.residue( hbs_pre_position ).atom_index("CY1"), hbs_pre_position );

	ConstraintCOP atompair( ConstraintOP( new AtomPairConstraint( aidCYH, aidCZH, harm_func ) ) );
	ConstraintCOP atompair2( ConstraintOP( new AtomPairConstraint( aidCYH, aidVYH, harm_func_0 ) ) );
	ConstraintCOP atompair3( ConstraintOP( new AtomPairConstraint( aidCZH, aidVZH, harm_func_0 ) ) );
	//ConstraintCOP angle = new AngleConstraint( aidCZH, aidCYH, aidCY2, ang_func2 );
	ConstraintCOP angle( ConstraintOP( new AngleConstraint( aidCZH, aidCYH, aidCY2, ang_func_120 ) ) );
	//ConstraintCOP angle2( ConstraintOP( new AngleConstraint( aidHYH, aidCYH, aidCY2, ang_func_120 ) ) );
	//ConstraintCOP angle2 = new AngleConstraint( aidN, aidCZH, aidCYH, ang_func2 );
	ConstraintCOP dihedral( ConstraintOP( new DihedralConstraint( aidCZH, aidCYH, aidCY2, aidCY1, dih_func_180 ) ) );
	//ConstraintCOP dihedral2( ConstraintOP( new DihedralConstraint( aidN, aidCZH, aidCYH, aidHYH, dih_func_0 ) ) );
	ConstraintCOP dihedral2( ConstraintOP( new DihedralConstraint( aidHYH, aidCZH, aidCY2, aidCYH, dih_func_0 ) ) );


	pose.add_constraint( atompair );
	pose.add_constraint( atompair2 );
	pose.add_constraint( atompair3 );
	pose.add_constraint( angle );
	//pose.add_constraint( angle2 );
	pose.add_constraint( dihedral );
	pose.add_constraint( dihedral2 );

	TR << "added atom pair constraint to hbs with distance: " << distance << " and std: "<< std << std::endl;
	TR << "and atom pair constraints with the virtual atoms" << std::endl;

}

void HbsPatcher::apply( core::pose::Pose & pose )
{
	TR<< "patching residues" <<std::endl;

	//awatkins: an hbs pre position cannot be last position
	runtime_assert_msg ( hbs_pre_pos_ != pose.total_residue(), "beginning of hbs cannot be last residue" );
	// I believe that this should be terminal, but since we're manually cutting off later residues
	// I bet we can't require it that way, so we have to trust hbs creator not to be dumb.
	//awatkins: an hbs post position cannot be first position
	runtime_assert ( hbs_post_pos_ != 1 );

	chemical::ResidueTypeSetCOP restype_set = chemical::ChemicalManager::get_instance()->residue_type_set( core::chemical::FA_STANDARD );

	std::string const pre_base_name( core::chemical::residue_type_base_name( pose.residue_type( hbs_pre_pos_ ) ) );
	std::string const post_base_name( core::chemical::residue_type_base_name( pose.residue_type( hbs_post_pos_ ) ) );
	TR << "pre restype basename: " << pre_base_name << std::endl;
	TR << "post restype basename: " << post_base_name << std::endl;

	//awatkins: check for proline
	if ( pre_base_name == "PRO" || pre_base_name == "DPRO" ||
			post_base_name == "PRO" || post_base_name == "DPRO" ) {
		utility_exit_with_message("Cannot patch proline");
	}
	if ( pose.residue(hbs_pre_pos_).has_variant_type(chemical::HBS_POST) == 1 ) {
		utility_exit_with_message("Cannot patch HBS_PRE on an HBS_POST");
	}
	if ( pose.residue(hbs_post_pos_).has_variant_type(chemical::HBS_PRE) == 1 ) {
		utility_exit_with_message("Cannot patch HBS_POST on an HBS_PRE");
	}

	//awatkins: check if already patched
	if ( !pose.residue( hbs_pre_pos_ ).has_variant_type( chemical::HBS_PRE ) ) {
		TR<< "patching pre" <<std::endl;

		//awatkins: get base residue type
		chemical::ResidueType const & pre_base_type = pose.residue(hbs_pre_pos_).type();
		TR<< pre_base_type.name() << std::endl;

		//awatkins: add variant

		std::string const base_name( core::chemical::residue_type_base_name( pre_base_type ) );

		// the desired set of variant types:
		utility::vector1< std::string > target_variants( pre_base_type.properties().get_list_of_variants() );
		if ( !pre_base_type.has_variant_type( chemical::HBS_PRE ) ) {
			target_variants.push_back( "HBS_PRE" );
			target_variants.push_back( "LOWER_TERMINUS_VARIANT" );
		}
		if ( pre_base_type.has_variant_type( chemical::ACETYLATED_NTERMINUS_VARIANT ) ) {
			target_variants.erase( std::find( target_variants.begin(), target_variants.end(), "ACETYLATED_NTERMINUS_VARIANT" ) );
		}
		if ( pre_base_type.has_variant_type( chemical::METHYLATED_NTERM_VARIANT ) ) {
			target_variants.erase( std::find( target_variants.begin(), target_variants.end(), "METHYLATED_NTERM_VARIANT" ) );
		}
		if ( pre_base_type.has_variant_type( chemical::N_ACETYLATION ) ) {
			target_variants.erase( std::find( target_variants.begin(), target_variants.end(), "N_ACETYLATION" ) );
		}

		ResidueTypeCOP rsd = ResidueTypeFinder( *restype_set ).residue_base_name( base_name ).variants( target_variants ).get_representative_type();
		conformation::Residue replace_res_pre( *rsd, true );
		replace_res_pre.set_all_chi(pose.residue(hbs_pre_pos_).chi());
		replace_res_pre.residue_connection_partner( 2, hbs_post_pos_, 3 );
		//replace_res_pre.mainchain_torsions(pose.residue(hbs_pre_pos_).mainchain_torsions());
		//replace_res_pre.update_residue_connection_mapping();
		pose.replace_residue( hbs_pre_pos_, replace_res_pre, true );
		conformation::idealize_position( hbs_pre_pos_, pose.conformation() );
		TR<< replace_res_pre.name() << std::endl;

		//conformation::Residue replace_res_pre( *new_type, true );
		//pose.dump_pdb( "rosetta_out_hbs_post_patch.pdb" );

	}// if pre
	if ( !pose.residue(hbs_post_pos_).has_variant_type( chemical::HBS_POST ) ) {
		TR<< "patching post" <<std::endl;
		//awatkins: get base residue type
		chemical::ResidueType const & post_base_type = pose.residue(hbs_post_pos_).type();
		TR<< post_base_type.name() << std::endl;

		std::string const base_name( core::chemical::residue_type_base_name( post_base_type ) );

		// the desired set of variant types:
		utility::vector1< std::string > target_variants( post_base_type.properties().get_list_of_variants() );
		if ( !post_base_type.has_variant_type( chemical::HBS_POST ) ) {
			target_variants.push_back( "HBS_POST" );
		}

		ResidueTypeCOP rsd = ResidueTypeFinder( *restype_set ).residue_base_name( base_name ).variants( target_variants ).get_representative_type();
		//restype_set->make_sure_instantiated( rsd.get_self_ptr() );
		//*new_type = rsd;
		conformation::Residue replace_res_post( *rsd, true );
		replace_res_post.set_all_chi(pose.residue(hbs_post_pos_).chi());
		replace_res_post.residue_connection_partner( 3, hbs_pre_pos_, 2 );
		//replace_res_pre.update_residue_connection_mapping();
		//replace_res_pre.mainchain_torsions(pose.residue(hbs_pre_pos_).mainchain_torsions());

		pose.replace_residue( hbs_post_pos_, replace_res_post, true );
		conformation::idealize_position( hbs_post_pos_, pose.conformation() );
		TR<< replace_res_post.name() << std::endl;

	}// if post

	add_hbs_constraint( pose, hbs_pre_pos_ );

	//awatkins: need to do all at once at the end because occasionally will get error:
	//awatkins: Unable to handle change in the number of residue connections in the presence of pseudobonds!
	//pose.conformation().detect_bonds();
	//pose.conformation().detect_pseudobonds();
	pose.conformation().update_polymeric_connection( hbs_pre_pos_ );
	pose.conformation().update_polymeric_connection( hbs_post_pos_ );

}


std::string
HbsPatcher::get_name() const {
	return "HbsPatcher";
}

/// @brief
HbsPatcher::HbsPatcher(
	core::Size hbs_seq_position
): Mover(), hbs_pre_pos_(hbs_seq_position), hbs_post_pos_(hbs_seq_position+2)
{
	Mover::type( "HbsPatcher" );

}

HbsPatcher::~HbsPatcher(){}

}//hbs
}//simple_moves
}//protocols
