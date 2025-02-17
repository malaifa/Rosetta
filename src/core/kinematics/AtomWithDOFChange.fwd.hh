// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   core/kinematics/AtomWithDOFChange.fwd.hh
/// @brief  Data structure for output-sensitie refold data class forward declaration
/// @author Andrew Leaver-Fay


#ifndef INCLUDED_core_kinematics_AtomWithDOFChange_fwd_hh
#define INCLUDED_core_kinematics_AtomWithDOFChange_fwd_hh

/// Utility Headers
#include <utility/vector1.fwd.hh>

namespace core {
namespace kinematics {


class AtomWithDOFChange;

typedef utility::vector1< AtomWithDOFChange > AtomDOFChangeSet;


} // namespace kinematics
} // namespace core


#endif // INCLUDED_core_kinematics_AtomWithDOFChange_FWD_HH
