// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/pack/rotamer_set/RotamerSets.fwd.hh
/// @brief  Residue Sets class foward declarations
/// @author Andrew Leaver-Fay (leaverfa@email.unc.edu)


#ifndef INCLUDED_core_pack_rotamer_set_RotamerSets_fwd_hh
#define INCLUDED_core_pack_rotamer_set_RotamerSets_fwd_hh

// Utility Headers
#include <utility/pointer/owning_ptr.hh>

namespace core {
namespace pack {
namespace rotamer_set {

class RotamerSets;

typedef utility::pointer::shared_ptr< RotamerSets > RotamerSetsOP;
typedef utility::pointer::shared_ptr< RotamerSets const > RotamerSetsCOP;

} // namespace rotamer_set
} // namespace pack
} // namespace core


#endif // INCLUDED_core_pack_RotamerSet_RotamerSets_fwd_HH
