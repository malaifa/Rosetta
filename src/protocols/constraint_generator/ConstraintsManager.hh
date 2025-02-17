// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/constraint_generator/ConstraintsManager.hh
/// @brief Manages lists of constraints generated by ConstraintGenerators
/// @author Tom Linsky (tlinsky@uw.edu)

#ifndef INCLUDED_protocols_constraint_generator_ConstraintsManager_hh
#define INCLUDED_protocols_constraint_generator_ConstraintsManager_hh

// Unit headers
#include <protocols/constraint_generator/ConstraintsManager.fwd.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>
#include <core/pose/datacache/CacheableDataType.hh>
#include <core/scoring/constraints/Constraint.fwd.hh>

// Utility headers
#include <utility/SingletonBase.hh>

namespace protocols {
namespace constraint_generator {

///@brief Manages lists of constraints generated by ConstraintGenerators
class ConstraintsManager : public utility::SingletonBase< ConstraintsManager > {
	typedef core::scoring::constraints::ConstraintCOPs ConstraintCOPs;

public: // constructors/destructors
	ConstraintsManager();
	virtual ~ConstraintsManager();

	static ConstraintsManager *
	create_singleton_instance();

public:
	/// @brief Stores the given constraints in the pose datacache, under the name given.
	/// @param[in,out] pose  Pose where constraints will be cached
	/// @param[in]     name  Name under which constraints will be stored
	/// @param[in]     csts  Constraints to cache
	void
	store_constraints(
		core::pose::Pose & pose,
		std::string const & name,
		ConstraintCOPs const & csts ) const;

	/// @brief Retrieves constraints from the pose datacache with the given name.
	/// @param[in] pose  Pose where constraints are cached
	/// @param[in] name  Name under which constraints are stored
	/// @returns   Const reference to list of stored constraints
	ConstraintCOPs const &
	retrieve_constraints(
		core::pose::Pose const & pose,
		std::string const & name ) const;

	/// @brief Checks to see whether constraints exist in datacache under the given name
	/// @param[in] pose  Pose where constraints are cached
	/// @param[in] name  Name under which constraints may be stored
	bool
	has_stored_constraints( core::pose::Pose const & pose, std::string const & name ) const;

	/// @brief Clears constraints stored under the given name
	/// @details  If constraints are not found under the given name, or there is no
	///           cached data, this will do nothing.
	/// @param[in,out] pose  Pose where constraints are cached
	/// @param[in]     name  Name under which constraints are cached
	void
	remove_constraints( core::pose::Pose & pose, std::string const & name ) const;

private:
	/// @brief non-const access to cached map
	ConstraintsMap &
	retrieve_constraints_map( core::pose::Pose & pose ) const;

	/// @brief const access to cached map
	ConstraintsMap const &
	retrieve_constraints_map( core::pose::Pose const & pose ) const;

	/// @brief adds an empty constraints map to the pose datacache
	void
	store_empty_constraints_map( core::pose::Pose & pose ) const;

public:
	// const data
	static core::pose::datacache::CacheableDataType::Enum const
		MY_TYPE;

};


} //protocols
} //constraint_generator

#endif //INCLUDED_protocols_constraint_generator_ConstraintsManager_hh
