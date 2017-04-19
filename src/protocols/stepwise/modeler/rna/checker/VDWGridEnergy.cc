// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/stepwise/modeler/rna/checker/VDWGridEnergy.cc
/// @brief  VDWGridEnergy energy method implementation
/// @details Aligns the pose to a 3D grid and counts up the clashes
/// @author Kalli Kappel, kappel@stanford.edu


//// NOTE ABOUT WHY THIS CLASS LIVES HERE /////
// It really makes no sense at all for this energy method to be located here
// It requires VDW_CachedRepScreenInfo which currently also lives in this directory
// Ideally VDW_CachedRepScreenInfo would be moved to core/pose/rna (or somewhere in core)
// but it depends on import_pose which lives in core.5
// Eventually VDW_CachedRepScreenInfo should probably be refactored so that it does not depend on
// import_pose
// The other option would be to have this energy method live in protocols/scoring, but this lives
// in protocols.3, so VDW_CachedRepScreenInfo would have to move somewhere below that

// Unit headers
#include <protocols/stepwise/modeler/rna/checker/VDWGridEnergy.hh>
#include <protocols/stepwise/modeler/rna/checker/VDWGridEnergyCreator.hh>

// Package Headers
#include <core/scoring/EnergyMap.hh>
#include <core/scoring/ScoreType.hh>
#include <core/scoring/rms_util.tmpl.hh>
#include <core/pose/full_model_info/FullModelInfo.hh>
#include <core/pose/full_model_info/util.hh>
#include <protocols/stepwise/modeler/rna/checker/VDW_CachedRepScreenInfo.hh>
#include <core/pose/rna/VDW_RepScreenInfo.hh>
#include <core/pose/rna/VDW_Grid.hh>
#include <core/pose/rna/util.hh>

// Project headers
#include <core/conformation/Residue.hh>

#include <utility/vector1.hh>

#include <basic/options/option.hh>
#include <basic/options/keys/score.OptionKeys.gen.hh>
#include <basic/Tracer.hh>

static basic::Tracer TR( "protocols.stepwise.modeler.rna.checker.VDWGridEnergy" );

namespace protocols {
namespace stepwise {
namespace modeler {
namespace rna {
namespace checker {


/// @details This must return a fresh instance of the VDWGridEnergy class,
/// never an instance already in use
core::scoring::methods::EnergyMethodOP
VDWGridEnergyCreator::create_energy_method(
	core::scoring::methods::EnergyMethodOptions const &
) const {
	return core::scoring::methods::EnergyMethodOP( new VDWGridEnergy );
}

core::scoring::ScoreTypes
VDWGridEnergyCreator::score_types_for_method() const {
	core::scoring::ScoreTypes sts;
	sts.push_back( core::scoring::grid_vdw );
	return sts;
}


/// ctor
VDWGridEnergy::VDWGridEnergy() :
	parent( core::scoring::methods::EnergyMethodCreatorOP( new VDWGridEnergyCreator ) ),
	clash_penalty_( 1.0 ) /*Totally made up for now*/
{}

VDWGridEnergy::~VDWGridEnergy() {}

/// clone
core::scoring::methods::EnergyMethodOP
VDWGridEnergy::clone() const
{
	return core::scoring::methods::EnergyMethodOP( new VDWGridEnergy );
}

/////////////////////////////////////////////////////////////////////////////
// methods for WholeStructureEnergies
/////////////////////////////////////////////////////////////////////////////

// The whole scoring method is going in finalize to ensure that alignment happens only once per time scoring
// And to make sure that we're comparing residues from the aligned pose to the grid
void
VDWGridEnergy::finalize_total_energy(
	core::pose::Pose & pose,
	core::scoring::ScoreFunction const &,
	core::scoring::EnergyMap & emap
) const {

	//using namespace protocols::stepwise::modeler::rna::checker;
	using namespace core::pose::rna;

	// Check that the pose actually has vdw rep screen info, if it doesn't, can't compute this score
	if ( !pose.data().has( core::pose::datacache::CacheableDataType::VDW_REP_SCREEN_INFO ) ) return;

	// Get the VDW info
	protocols::stepwise::modeler::rna::checker::VDW_CachedRepScreenInfo vdw_info( stepwise::modeler::rna::checker::const_vdw_cached_rep_screen_info_from_pose( pose ) );

	utility::vector1< VDW_RepScreenInfo > vdw_rep_screen_info;
	vdw_rep_screen_info = vdw_info.VDW_rep_screen_info_list();

	VDW_GridCOP vdw_screen_bin;
	vdw_screen_bin = vdw_info.VDW_screen_bin();

	// Check that the grid is actually set to something: if not, can't compute this score term
	if ( vdw_screen_bin->size() == 0 ) return;

	// Align the pose to the VDW pose, so that the grid will be lined up
	// Do the alignment to the first pose in the list
	core::id::AtomID_Map < core::id::AtomID > atom_id_map;
	// Check if there is an alignment map already stored in the pose
	if ( vdw_rep_screen_info[1].align_working_to_vdw_atom_id_map.size() == 0 ) {
		// The alignment map should have already been set up if the pose should be aligned
		TR << "WARNING" << std::endl;
		TR << "Not computing grid_vdw energy because there is no alignment map stored in the pose!!" << std::endl;
		// If this isn't set up, shouldn't be computing this score
		// Otherwise risk getting a lot of garbage
		return;
	} else { // There is an alignment map stored in the pose: use it!
		atom_id_map = vdw_rep_screen_info[1].align_working_to_vdw_atom_id_map;
		core::scoring::superimpose_pose( pose, *(vdw_rep_screen_info[1].VDW_pose), atom_id_map );
	}

	// Count up the number of the clashes the aligned pose (nonconst_pose) has with the grid
	numeric::xyzVector< core::Real > ref_xyz = vdw_screen_bin->get_ref_xyz(); // This will be 0 if it wasn't explicitly set
	// Loop through all the residues in the pose
	for ( Size nres = 1; nres <= pose.total_residue(); ++nres ) {
		core::conformation::Residue const & rsd = pose.residue( nres );
		// Loop through all the atoms in the residue
		for ( Size i = 1; i <= rsd.natoms(); ++i ) {
			if ( rsd.is_virtual( i ) ) continue;
			Atom_Bin const atom_pos_bin = get_atom_bin( rsd.xyz( i ), ref_xyz, vdw_screen_bin->get_atom_bin_size(), vdw_screen_bin->get_bin_offset() );

			// Check that the atom falls inside the grid
			if ( is_atom_bin_in_range( atom_pos_bin, vdw_screen_bin->get_bin_max() ) == false ) {
				continue;
			}

			if ( vdw_screen_bin->get_xyz_bin( atom_pos_bin ) ) {
				// There was a clash, add score penalty
				emap[ core::scoring::grid_vdw ] += clash_penalty_;
			}
		}
	}
}


/// @brief VDWGridEnergy is context independent; indicates that no context graphs are required
void
VDWGridEnergy::indicate_required_context_graphs(
	utility::vector1< bool > & /*context_graphs_required*/
) const
{}

core::Size
VDWGridEnergy::version() const
{
	return 1;
}


} //checker
} //rna
} //modeler
} //stepwise
} //protocols

