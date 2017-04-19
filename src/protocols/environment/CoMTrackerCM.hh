// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file src/protocols/environment/CoMTrackerCM.hh
/// @brief header file for CoMTracker
/// @author Justin R. Porter
/// @author Brian D. Weitzner
/// @author Oliver F. Lange

#ifndef INCLUDED_protocols_environment_CoMTrackerCM_hh
#define INCLUDED_protocols_environment_CoMTrackerCM_hh

// Unit Headers
#include <protocols/environment/CoMTrackerCM.fwd.hh>
#include <protocols/environment/ClientMover.hh>

// Package headers
#include <protocols/environment/claims/EnvClaim.hh>

// Project headers
#include <core/select/residue_selector/ResidueSelector.hh>
#include <core/pose/Pose.hh>

#include <basic/datacache/WriteableCacheableMap.fwd.hh>

// C++ Headers

// ObjexxFCL Headers

namespace protocols {
namespace environment {

class CoMTrackerCM : public environment::ClientMover {
	typedef environment::claims::EnvClaims EnvClaims;
	typedef int JumpNumber;

public:
	CoMTrackerCM();

	CoMTrackerCM( std::string const& name,
		core::select::residue_selector::ResidueSelectorCOP mobile_selector,
		std::string const& stationary_label );

	CoMTrackerCM( std::string const& name,
		core::select::residue_selector::ResidueSelectorCOP mobile_selector );

	virtual
	~CoMTrackerCM() {};

	virtual
	EnvClaims yield_claims( core::pose::Pose const&,
		basic::datacache::WriteableCacheableMapOP );

	virtual std::string get_name() const;

	virtual void initialize( core::pose::Pose& pose );

	virtual void apply( core::pose::Pose& );

	virtual void
	parse_my_tag( utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose );

	std::string const& name() const { return name_; }

	void name( std::string const& name ) { name_ = name; }

	virtual
	moves::MoverOP fresh_instance() const;

	virtual
	moves::MoverOP clone() const;

protected:
	virtual void passport_updated();

private:
	void update_tracking_residue( core::kinematics::RT::Vector new_position,
		core::Size tracking_residue_id,
		core::pose::Pose & pose ) const;

	void update_com( core::pose::Pose& ) const;

	std::string name_;
	std::string stationary_label_;
	std::string com_name_, com_jump_name_;
	core::select::residue_selector::ResidueSubset mobile_residues_;
	core::select::residue_selector::ResidueSelectorCOP mobile_selector_;

}; // end CoMTrackerCM base class

} // environment
} // protocols

#endif //INCLUDED_protocols_environment_CoMTrackerCM_hh
