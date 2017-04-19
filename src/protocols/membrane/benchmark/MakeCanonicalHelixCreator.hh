// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/membrane/benchmark/MakeCanonicalHelixCreator.hh
/// @brief Creates an ideal a-helix from sequence given a range of residues
/// @author Rebecca Alford (rfalford12@gmail.com)

#ifndef INCLUDED_protocols_membrane_benchmark_MakeCanonicalHelixCreator_hh
#define INCLUDED_protocols_membrane_benchmark_MakeCanonicalHelixCreator_hh

#include <protocols/moves/MoverCreator.hh>

namespace protocols {
namespace membrane {
namespace benchmark {

class MakeCanonicalHelixCreator : public protocols::moves::MoverCreator {

public:

	virtual protocols::moves::MoverOP create_mover() const;
	virtual std::string keyname() const;
	static std::string mover_name();

};

} //protocols
} //membrane
} //benchmark

#endif // INCLUDED_protocols_membrane_benchmark_MakeCanonicalHelix_fwd_hh



