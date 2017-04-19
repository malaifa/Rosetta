// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/simple_filters/FileRemoveFilter.hh
/// @brief Simple filter that tests whether a file exists. Useful to test whether we're recovering from a checkpoint
/// @author Sarel Fleishman

#ifndef INCLUDED_protocols_simple_filters_FileRemoveFilter_hh
#define INCLUDED_protocols_simple_filters_FileRemoveFilter_hh

//unit headers
#include <protocols/simple_filters/FileRemoveFilter.fwd.hh>

// Project Headers
#include <core/scoring/ScoreFunction.hh>
#include <core/types.hh>
#include <protocols/filters/Filter.hh>
#include <core/pose/Pose.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>

namespace protocols {
namespace simple_filters {

class FileRemoveFilter : public filters::Filter
{
public:
	//default ctor
	FileRemoveFilter();
	bool apply( core::pose::Pose const & pose ) const;
	filters::FilterOP clone() const {
		return filters::FilterOP( new FileRemoveFilter( *this ) );
	}
	filters::FilterOP fresh_instance() const{
		return filters::FilterOP( new FileRemoveFilter() );
	}

	void report( std::ostream & out, core::pose::Pose const & pose ) const;
	core::Real report_sm( core::pose::Pose const & pose ) const;
	core::Real compute( core::pose::Pose const &pose ) const;
	virtual ~FileRemoveFilter();
	void parse_my_tag( utility::tag::TagCOP tag, basic::datacache::DataMap &, protocols::filters::Filters_map const &, protocols::moves::Movers_map const &, core::pose::Pose const & );
	utility::vector1< std::string > file_names() const;
	void file_names( utility::vector1< std::string > const & f );
	bool delete_content_only() const{ return delete_content_only_; }
	void delete_content_only( bool const b ){ delete_content_only_ = b; }
private:
	utility::vector1< std::string > file_names_;
	bool delete_content_only_; //dflt false; if true, deletes the file but leaves a 0b placeholder for the file
};

}
}

#endif
