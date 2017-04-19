// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/stepwise/screener/VDW_BinScreener.hh
/// @brief
/// @details
/// @author Rhiju Das, rhiju@stanford.edu


#ifndef INCLUDED_protocols_stepwise_screener_VDW_BinScreener_HH
#define INCLUDED_protocols_stepwise_screener_VDW_BinScreener_HH

#include <protocols/stepwise/screener/StepWiseScreener.hh>
#include <protocols/stepwise/screener/VDW_BinScreener.fwd.hh>
#include <protocols/stepwise/modeler/rna/checker/RNA_VDW_BinChecker.fwd.hh>
#include <core/conformation/Residue.fwd.hh>
#include <core/kinematics/Stub.fwd.hh>
#include <core/pose/Pose.fwd.hh>

// To Author(s) of this code: our coding convention explicitly forbid of using ‘using namespace ...’ in header files outside class or function body, please make sure to refactor this out!
using namespace core;

namespace protocols {
namespace stepwise {
namespace screener {

class VDW_BinScreener: public StepWiseScreener {

public:

	//constructor
	// Undefined, commenting out to fix PyRosetta build  VDW_BinScreener();

	//constructor
	VDW_BinScreener( modeler::rna::checker::RNA_VDW_BinCheckerOP vdw_bin_checker,
		pose::Pose & screening_pose,
		Size const moving_res );

	VDW_BinScreener( modeler::rna::checker::RNA_VDW_BinCheckerOP vdw_bin_checker,
		pose::Pose & screening_pose,
		Size const moving_res,
		core::conformation::ResidueCOP screening_moving_rsd_at_origin,
		core::kinematics::Stub const & moving_res_base_stub );

	//destructor
	~VDW_BinScreener();

public:

	bool
	check_screen();

	std::string
	name() const { return "VDW_BinScreener"; }

	StepWiseScreenerType
	type() const { return VDW_BIN; }

	void
	fast_forward( sampler::StepWiseSamplerBaseOP sampler );

private:

	modeler::rna::checker::RNA_VDW_BinCheckerOP vdw_bin_checker_;
	pose::Pose & screening_pose_;
	Size const moving_res_;

	bool const using_stub_;
	core::conformation::ResidueCOP screening_moving_rsd_at_origin_;
	core::kinematics::Stub const & moving_res_base_stub_;

};

} //screener
} //stepwise
} //protocols

#endif
