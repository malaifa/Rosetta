// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file   core/chemical/carbohydrates/carbohydrate_data_structures.hh
/// @brief  Data structure declarations for carbohdrates
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

#ifndef INCLUDED_core_chemical_carbohydrates_carbohydrate_data_structures_hh
#define INCLUDED_core_chemical_carbohydrates_carbohydrate_data_structures_hh

#include <core/id/types.hh>
#include <utility/vector1.hh>
#include <map>

namespace core {
namespace chemical {
namespace carbohydrates {

/// @brief Holds original conformer data from GlycanRelax
struct LinkageConformerData{
	Real population;

	utility::vector1< std::pair< core::Real, core::Real > > mean_sd;

	//@brief There can be none or multiple omega data
	utility::vector1< std::pair< core::Real, core::Real > > omega_mean_sd;


	//////////////////////////////////////////////////////////////////
	core::Real
	get_torsion_mean( core::id::MainchainTorsionType torsion ) const {
		return get_torsion_mean( torsion, 1);
	}

	core::Real
	get_torsion_mean( core::id::MainchainTorsionType torsion, core::Size torsion_num ) const {
		if ( torsion == core::id::omega_dihedral ) {
			if ( torsion_num > omega_mean_sd.size() ) {
				utility_exit_with_message( "Torsion num larger than total number of omegas in glycosidic bond!" );
			}
			return omega_mean_sd[ torsion_num ].first;
		} else {
			return mean_sd[ torsion ].first;
		}
	}


	core::Real
	get_torsion_sd( core::id::MainchainTorsionType torsion ) const {
		return get_torsion_sd( torsion, 1 );
	}


	core::Real
	get_torsion_sd( core::id::MainchainTorsionType torsion, core::Size torsion_num ) const {
		if ( torsion == core::id::omega_dihedral ) {
			if ( torsion_num > omega_mean_sd.size() ) {
				utility_exit_with_message( "Torsion num larger than total number of omegas in glycosidic bond!" );
			}
			return omega_mean_sd[ torsion_num ].second;
		} else {
			return mean_sd[ torsion ].second;
		}
	}

	bool
	has_omega() const {
		if ( omega_mean_sd.size() > 0 ) {
			return true;
		} else {
			return false;
		}
	}

	core::Size
	n_omega() const{
		return omega_mean_sd.size();
	}

	core::Size
	n_torsions() const {
		if ( ! mean_sd.size() ) {
			return 0;
		}
		return 2 + n_omega();
	}
}; // End Struct


// @brief   Maps a non-reducing-end residue/reducing-end residue pair to a vector of linkage conformers.
// @details The reducing-end residue has a linkage number (unless it is an AA); the non-reducing-end does not.
typedef std::map< std::pair< std::string, std::string >, utility::vector1< LinkageConformerData > >
	LinkageConformers;

} //carbohydrates
} //chemical
} //core

#endif //core/chemical/carbohydrates_carbohydrate_data_structures_hh
