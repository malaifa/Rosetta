// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file ./src/protocols/fldsgn/filters/SheetTopologyFilter.hh
/// @brief header file for SheetTopologyFilter class.
/// @details
/// @author Nobuyasu Koga ( nobuyasu@uw.edu )


#ifndef INCLUDED_protocols_fldsgn_filters_SheetTopologyFilter_hh
#define INCLUDED_protocols_fldsgn_filters_SheetTopologyFilter_hh

// Unit Headers
#include <protocols/fldsgn/filters/SheetTopologyFilter.fwd.hh>

// Package Headers
#include <protocols/filters/Filter.hh>
#include <protocols/fldsgn/topology/SS_Info2.hh>

// Project Headers
#include <core/pose/Pose.fwd.hh>
#include <protocols/fldsgn/topology/StrandPairing.fwd.hh>

// Utility headers

// Parser headers
#include <basic/datacache/DataMap.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>
#include <utility/tag/Tag.fwd.hh>

#include <utility/vector1.hh>


//// C++ headers

namespace protocols {
namespace fldsgn {
namespace filters {

class SheetTopologyFilter : public protocols::filters::Filter {
public:

	typedef protocols::filters::Filter Super;
	typedef protocols::filters::Filter Filter;
	typedef std::string String;
	typedef protocols::filters::FilterOP FilterOP;
	typedef core::pose::Pose Pose;
	typedef protocols::fldsgn::topology::StrandPairingSet StrandPairingSet;
	typedef protocols::fldsgn::topology::StrandPairingSetOP StrandPairingSetOP;
	typedef protocols::fldsgn::topology::SS_Info2 SS_Info2;
	typedef protocols::fldsgn::topology::SS_Info2_OP SS_Info2_OP;

	typedef utility::tag::TagCOP TagCOP;
	typedef protocols::filters::Filters_map Filters_map;
	typedef basic::datacache::DataMap DataMap;
	typedef protocols::moves::Movers_map Movers_map;


public:// constructor/destructor


	// @brief default constructor
	SheetTopologyFilter();

	// @brief constructor with arguments
	SheetTopologyFilter( StrandPairingSetOP const & sps );

	// @brief constructor with arguments
	SheetTopologyFilter( String const & sheet_topology );

	// @brief copy constructor
	SheetTopologyFilter( SheetTopologyFilter const & rval );

	virtual ~SheetTopologyFilter(){}


public:// virtual constructor


	// @brief make clone
	virtual FilterOP clone() const { return FilterOP( new SheetTopologyFilter( *this ) ); }

	// @brief make fresh instance
	virtual FilterOP fresh_instance() const { return FilterOP( new SheetTopologyFilter() ); }


public:// mutator


	// @brief set filtered sheet_topology by SrandPairingSetOP
	void filtered_sheet_topology( StrandPairingSetOP const & sps );

	// @brief set filtered sheet_topology by SrandPairingSetOP
	void filtered_sheet_topology( String const & sheet_topology );

	void set_secstruct( std::string const & ss );

	/// @brief if true, and secstruct is unset, dssp is used on the input.  Otherwise, the pose.secstruct() is used
	void set_use_dssp( bool const use_dssp );
public:// accessor


	// @brief get name of this filter
	virtual std::string name() const { return "SheetTopologyFilter"; }


public:// parser

	virtual void parse_my_tag( TagCOP tag,
		basic::datacache::DataMap &,
		Filters_map const &,
		Movers_map const &,
		Pose const & );


public:// virtual main operation

	/// @brief returns the fraction of pairings that pass the filter
	virtual core::Real compute( Pose const & pose ) const;

	/// @brief returns the fraction of pairings that pass
	virtual core::Real report_sm( Pose const & pose ) const;

	// @brief returns true if the given pose passes the filter, false otherwise.
	// In this case, the test is whether the give pose is the topology we want.
	virtual bool apply( Pose const & pose ) const;


private:

	String filtered_sheet_topology_;

	String secstruct_input_;

	bool ignore_register_shift_;

	bool use_dssp_;

	SS_Info2_OP ssinfo_;

};

core::Size
compute_paired_residues( topology::StrandPairingCOP filt_pair, topology::StrandPairingCOP pair );

core::Size
compute_total_paired_residues( topology::StrandPairingCOP filt_pair );

} // filters
} // fldsgn
} // protocols

#endif
