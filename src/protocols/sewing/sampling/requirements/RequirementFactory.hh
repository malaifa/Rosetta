// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file src/protocols/features/RequirementFactory.hh
/// @brief Factory for creating Requirement objects
/// @author Matthew O'Meara (mattjomeara@gmail.com)


#ifndef INCLUDED_protocols_sewing_sampling_requirements_RequirementFactory_hh
#define INCLUDED_protocols_sewing_sampling_requirements_RequirementFactory_hh

// Unit Headers
#include <protocols/sewing/sampling/requirements/RequirementFactory.fwd.hh>

// Package Headers
#include <protocols/sewing/sampling/requirements/GlobalRequirement.hh>
#include <protocols/sewing/sampling/requirements/IntraSegmentRequirement.hh>
#include <protocols/sewing/sampling/requirements/RequirementCreator.hh>

// Platform Headers
#include <core/pose/Pose.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>
#include <utility/tag/Tag.fwd.hh>
#include <utility/factory/WidgetRegistrator.hh>

// C++ Headers
#include <map>

#include <utility/vector1.hh>

#ifdef MULTI_THREADED
#ifdef CXX11
// C++11 Headers
#include <atomic>
#include <mutex>
#endif
#endif

namespace protocols {
namespace sewing  {
namespace sampling {
namespace requirements {

/// Create Features Reporters
class RequirementFactory {

private:
	// Private constructor to make it singleton managed
	RequirementFactory();
	RequirementFactory(const RequirementFactory & src); // unimplemented

	RequirementFactory const &
	operator=( RequirementFactory const & ); // unimplemented

	/// @brief private singleton creation function to be used with
	/// utility::thread::threadsafe_singleton
	static RequirementFactory * create_singleton_instance();

public:

	// Warning this is not called because of the singleton pattern
	virtual ~RequirementFactory();

	static RequirementFactory * get_instance();

	void factory_register(
		GlobalRequirementCreatorCOP creator
	);

	void factory_register(
		IntraSegmentRequirementCreatorCOP creator
	);

	GlobalRequirementOP get_global_requirement(
		std::string const & type_name
	);

	IntraSegmentRequirementOP get_intra_segment_requirement(
		std::string const & type_name
	);

	//utility::vector1<std::string> get_all_features_names();

#ifdef MULTI_THREADED
#ifdef CXX11
public:

	/// @brief This public method is meant to be used only by the
	/// utility::thread::safely_create_singleton function and not meant
	/// for any other purpose.  Do not use.
	static std::mutex & singleton_mutex();

private:
	static std::mutex singleton_mutex_;
#endif
#endif

private:

#if defined MULTI_THREADED && defined CXX11
	static std::atomic< RequirementFactory * > instance_;
#else
	static RequirementFactory * instance_;
#endif

	typedef std::map< std::string, GlobalRequirementCreatorCOP > GlobalRequirementCreatorMap;
	GlobalRequirementCreatorMap global_types_;

	typedef std::map< std::string, IntraSegmentRequirementCreatorCOP > IntraSegmentRequirementCreatorMap;
	IntraSegmentRequirementCreatorMap intra_segment_types_;
};


/// @brief This templated class will register an instance of an
/// RequirementCreator (class T) with the
/// RequirementFactory.  It will ensure that no
/// RequirementCreator is registered twice, and, centralizes this
/// registration logic so that thread safety issues can be handled in
/// one place
template < class T >
class RequirementRegistrator : public utility::factory::WidgetRegistrator< RequirementFactory, T >
{

public:
	typedef utility::factory::WidgetRegistrator< RequirementFactory, T > parent;
	RequirementRegistrator() : parent() {}

};


} //namesapce
} //namesapce
} //namesapce
} //namespace

#endif
