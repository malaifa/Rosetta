// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/stepwise/screener/TagDefinition.hh
/// @brief
/// @details
/// @author Rhiju Das, rhiju@stanford.edu


#ifndef INCLUDED_protocols_stepwise_screener_TagDefinition_HH
#define INCLUDED_protocols_stepwise_screener_TagDefinition_HH

#include <protocols/stepwise/screener/StepWiseScreener.hh>
#include <protocols/stepwise/screener/TagDefinition.fwd.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/types.hh>

// To Author(s) of this code: our coding convention explicitly forbid of using ‘using namespace ...’ in header files outside class or function body, please make sure to refactor this out!
using namespace core;

namespace protocols {
namespace stepwise {
namespace screener {

class TagDefinition: public StepWiseScreener {

public:

	//constructor
	TagDefinition( pose::Pose & pose,
		StepWiseScreenerOP first_sampler,
		bool const sampler_include_torsion_value_in_tag_ = false,
		Size const moving_res = 0,
		Size const reference_res = 0,
		std::string const & extra_tag = "" );

	//destructor
	~TagDefinition();

public:

	std::string tag() const { return tag_; }

	bool
	check_screen();

	std::string
	name() const { return "TagDefinition"; }

	StepWiseScreenerType
	type() const { return TAG_DEFINITION; }

	void
	append_to_tag( std::string const & value ){ tag_ += value; }

private:

	pose::Pose & pose_;
	StepWiseScreenerOP first_sampler_;
	bool const sampler_include_torsion_value_in_tag_;
	Size const moving_res_, reference_res_;
	std::string const extra_tag_;
	std::string tag_;

};

} //screener
} //stepwise
} //protocols

#endif
