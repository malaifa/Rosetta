// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/denovo_design/components/SegmentPairing.fwd.hh
/// @brief Handles user-specified pairing between/among segments
/// @author Tom Linsky (tlinsky@uw.edu)

#ifndef INCLUDED_protocols_denovo_design_components_SegmentPairing_fwd_hh
#define INCLUDED_protocols_denovo_design_components_SegmentPairing_fwd_hh

// Utility headers
#include <utility/pointer/owning_ptr.hh>
#include <utility/vector1.fwd.hh>

// Forward
namespace protocols {
namespace denovo_design {
namespace components {

class SegmentPairing;

typedef utility::pointer::shared_ptr< SegmentPairing > SegmentPairingOP;
typedef utility::pointer::shared_ptr< SegmentPairing const > SegmentPairingCOP;

typedef utility::vector1< SegmentPairingOP > SegmentPairingOPs;
typedef utility::vector1< SegmentPairingCOP > SegmentPairingCOPs;

class HelixPairing;
typedef utility::pointer::shared_ptr< HelixPairing > HelixPairingOP;
typedef utility::pointer::shared_ptr< HelixPairing const > HelixPairingCOP;

class StrandPairing;
typedef utility::pointer::shared_ptr< StrandPairing > StrandPairingOP;
typedef utility::pointer::shared_ptr< StrandPairing const > StrandPairingCOP;


} //protocols
} //denovo_design
} //components

#endif //INCLUDED_protocols_denovo_design_components_SegmentPairing_fwd_hh

