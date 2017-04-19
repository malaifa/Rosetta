// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/simple_filters/PoseInfoFilter.hh
/// @brief Filter for outputing information about the pose.
/// @author Rocco Moretti (rmoretti@uw.edu)

#ifndef INCLUDED_protocols_simple_filters_PoseInfoFilter_hh
#define INCLUDED_protocols_simple_filters_PoseInfoFilter_hh


// Project Headers
#include <protocols/filters/Filter.hh>
#include <core/pose/Pose.fwd.hh>
#include <core/types.hh>
#include <utility/tag/Tag.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>

namespace protocols {
namespace simple_filters {

/// @brief detects atomic contacts between two atoms of two residues
class PoseInfoFilter : public protocols::filters::Filter
{
private:
	typedef protocols::filters::Filter parent;
public:
	PoseInfoFilter();
	virtual bool apply( core::pose::Pose const & pose ) const;
	core::Real compute( core::pose::Pose const & pose ) const;
	virtual void report( std::ostream & out, core::pose::Pose const & pose ) const;
	virtual core::Real report_sm( core::pose::Pose const & pose ) const;
	virtual filters::FilterOP clone() const {
		return filters::FilterOP( new PoseInfoFilter( *this ) );
	}
	virtual filters::FilterOP fresh_instance() const{
		return filters::FilterOP( new PoseInfoFilter() );
	}

	virtual ~PoseInfoFilter(){};
	void parse_my_tag( utility::tag::TagCOP tag,
		basic::datacache::DataMap &,
		protocols::filters::Filters_map const &,
		protocols::moves::Movers_map const &,
		core::pose::Pose const & );
private:
};

} // filters
} // protocols

#endif
