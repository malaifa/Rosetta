// vi: set ts=2 noet;
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file AddStartnodeFragments.hh
///
/// @brief A Mover that uses loophash to find fragments that can
/// bridge a gap with minimal modifications to the original pose.
/// @author Tim Jacobs


#ifndef INCLUDED_protocols_sewing_AddStartnodeFragments_HH
#define INCLUDED_protocols_sewing_AddStartnodeFragments_HH

// Unit Headers
#include <protocols/sewing/sampling/AddStartnodeFragments.fwd.hh>
#include <protocols/moves/Mover.hh>

//Protocol headers
#include <core/pose/Pose.hh>

namespace protocols {
namespace sewing  {

class AddStartnodeFragments : public protocols::moves::Mover {

public:

	AddStartnodeFragments();

	protocols::moves::MoverOP
	clone() const;

	protocols::moves::MoverOP
	fresh_instance() const;

	std::string
	get_name() const;

	virtual
	void
	apply(
		core::pose::Pose & pose
	);

	virtual
	void
	parse_my_tag(
		TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose
	);

private:

	core::Size start_res_;
	core::Size end_res_;

};

} //sewing
} //protocols

#endif
