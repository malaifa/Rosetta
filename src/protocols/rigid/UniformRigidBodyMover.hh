// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file src/protocols/rigid/UniformRigidBodyMover.hh
/// @author Justin Porter

#ifndef INCLUDED_protocols_rigid_UniformRigidBodyMover_hh
#define INCLUDED_protocols_rigid_UniformRigidBodyMover_hh

// Unit Headers
#include <protocols/rigid/UniformRigidBodyMover.fwd.hh>
#include <protocols/environment/ClientMover.hh>

// Package headers
#include <protocols/environment/claims/EnvClaim.hh>

#include <protocols/canonical_sampling/ThermodynamicMover.hh>

// Project headers
#include <core/pose/Pose.hh>

#include <basic/datacache/WriteableCacheableMap.fwd.hh>

// C++ Headers

// ObjexxFCL Headers

namespace protocols {
namespace rigid {

class UniformRigidBodyMover : public canonical_sampling::ThermodynamicMover {
	typedef int JumpNumber;

public:
	UniformRigidBodyMover();

	UniformRigidBodyMover( JumpNumber target_jump,
		core::Real rotation_mag_ = 3.0,
		core::Real translation_mag_ = 8.0 );

	virtual
	~UniformRigidBodyMover() {};

	virtual std::string get_name() const;

	virtual void apply( core::pose::Pose& );

	virtual void
	parse_my_tag( utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose );
	virtual
	moves::MoverOP fresh_instance() const;

	virtual
	moves::MoverOP clone() const;

	void jump_number( JumpNumber );

	JumpNumber jump_number() const;

	virtual
	void
	set_preserve_detailed_balance( bool ) {};

	virtual
	bool
	preserve_detailed_balance() const { return true; }

	virtual
	utility::vector1<core::id::TorsionID_Range>
	torsion_id_ranges( core::pose::Pose & pose );

private:
	JumpNumber target_jump_;
	core::Real rotation_mag_, translation_mag_;

}; // end UniformRigidBodyMover base class

} // abinitio
} // protocols

#endif //INCLUDED_protocols_rigid_UniformRigidBodyMover_hh
