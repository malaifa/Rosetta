// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/rotamer_recovery/RotamerRecoveryFactory.fwd.hh
/// @brief  report atom-atom pair geometry and scores to rotamer_recovery Statistics Scientific Benchmark
/// @author Matthew O'Meara

#ifndef INCLUDED_protocols_rotamer_recovery_RotamerRecoveryFactory_fwd_hh
#define INCLUDED_protocols_rotamer_recovery_RotamerRecoveryFactory_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace rotamer_recovery {

class RotamerRecoveryFactory;
typedef utility::pointer::shared_ptr< RotamerRecoveryFactory > RotamerRecoveryFactoryOP;
typedef utility::pointer::shared_ptr< RotamerRecoveryFactory const > RotamerRecoveryFactoryCOP;

}// namespace
}// namespace

#endif // include guard
