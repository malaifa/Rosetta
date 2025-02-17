// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file protocols/filters/ContingentFilter.hh
/// @brief A filter that is contingent on some other mover to set its pass/fail value
/// @author Sarel Fleishman (sarelf@uw.edu)

#ifndef INCLUDED_protocols_filters_ContingentFilter_hh
#define INCLUDED_protocols_filters_ContingentFilter_hh


// Project Headers
#include <protocols/filters/Filter.hh>
#include <core/pose/Pose.fwd.hh>
#include <utility/tag/Tag.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>

#include <utility/vector1.hh>

#ifdef WIN32
#include <utility/tag/Tag.hh>
#endif


// Unit headers

namespace protocols {
namespace filters {

class ContingentFilter : public protocols::filters::Filter
{
private:
	typedef protocols::filters::Filter parent;
public:
	/// @brief default ctor
	ContingentFilter();
	/// @brief Constructor with a single target residue
	virtual bool apply( core::pose::Pose const & pose ) const;
	virtual void report( std::ostream & out, core::pose::Pose const & pose ) const;
	virtual core::Real report_sm( core::pose::Pose const & pose ) const;
	virtual protocols::filters::FilterOP clone() const {
		return protocols::filters::FilterOP( new ContingentFilter( *this ) );
	}
	virtual protocols::filters::FilterOP fresh_instance() const{
		return protocols::filters::FilterOP( new ContingentFilter() );
	}

	virtual ~ContingentFilter(){};
	void parse_my_tag( utility::tag::TagCOP tag,
		basic::datacache::DataMap &,
		protocols::filters::Filters_map const &,
		protocols::moves::Movers_map const &,
		core::pose::Pose const & );
	virtual void set_value( bool const value );
	virtual bool get_value() const;
private:
	bool value_; // defaults to false
};

} // filters
} // protocols

#endif //INCLUDED_protocols_Filters_ContingentFilter_HH_
