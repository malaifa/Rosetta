// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/toolbox/task_operations/StoreResidueSubsetMoverCreator.hh
/// @brief This class will create instances of Mover StoreResidueSubsetMover for the MoverFactory
/// @author Tom Linsky ( tlinsky at uw dot edu )

#ifndef INCLUDED_protocols_residue_selectors_StoreResidueSubsetMoverCreator_hh
#define INCLUDED_protocols_residue_selectors_StoreResidueSubsetMoverCreator_hh

#include <protocols/moves/MoverCreator.hh>

namespace protocols {
namespace residue_selectors {

class StoreResidueSubsetMoverCreator : public protocols::moves::MoverCreator {
public:
	virtual protocols::moves::MoverOP create_mover() const;
	virtual std::string keyname() const;
	static std::string mover_name();
};

}
}

#endif

