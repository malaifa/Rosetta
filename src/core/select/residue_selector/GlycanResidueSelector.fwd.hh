// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file core/select/residue_selector/GlycanResidueSelector.fwd.hh
/// @brief A ResidueSelector for carbohydrates and individual carbohydrate trees.
/// @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)


#ifndef INCLUDED_core_select_residue_selector_GlycanResidueSelector_fwd_hh
#define INCLUDED_core_select_residue_selector_GlycanResidueSelector_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>



// Forward
namespace core {
namespace select {
namespace residue_selector {

class GlycanResidueSelector;

typedef utility::pointer::shared_ptr< GlycanResidueSelector > GlycanResidueSelectorOP;
typedef utility::pointer::shared_ptr< GlycanResidueSelector const > GlycanResidueSelectorCOP;



} //core
} //select
} //residue_selector


#endif //INCLUDED_core_select_residue_selector_GlycanResidueSelector_fwd_hh





