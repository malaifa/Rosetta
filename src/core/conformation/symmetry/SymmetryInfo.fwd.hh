// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
/// @brief
/// @author Ingemar Andre, Phil Bradley

#ifndef INCLUDED_core_conformation_symmetry_SymmetryInfo_fwd_hh
#define INCLUDED_core_conformation_symmetry_SymmetryInfo_fwd_hh

#include <utility/pointer/owning_ptr.hh>

namespace core {
namespace conformation {
namespace symmetry {

class SymmetryInfo;
typedef utility::pointer::shared_ptr< SymmetryInfo > SymmetryInfoOP;
typedef utility::pointer::shared_ptr< SymmetryInfo const > SymmetryInfoCOP;

} // symmetry
} // conformation
} // core

#endif

