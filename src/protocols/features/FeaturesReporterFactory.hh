// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file src/protocols/features/FeaturesReporterFactory.hh
/// @brief Factory for creating FeaturesReporter objects
/// @author Matthew O'Meara (mattjomeara@gmail.com)


#ifndef INCLUDED_protocols_features_FeaturesReporterFactory_hh
#define INCLUDED_protocols_features_FeaturesReporterFactory_hh

// Unit Headers
#include <protocols/features/FeaturesReporterFactory.fwd.hh>

// Project Headers
#include <protocols/features/FeaturesReporter.fwd.hh>
#include <protocols/features/FeaturesReporterCreator.fwd.hh>

// Platform Headers
#include <core/pose/Pose.fwd.hh>
#include <protocols/filters/Filter.fwd.hh>
#include <protocols/moves/Mover.fwd.hh>
#include <basic/datacache/DataMap.fwd.hh>

// Utility headers
#include <utility/SingletonBase.hh>
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
namespace features {

/// Create Features Reporters
class FeaturesReporterFactory {
private:
	// Private constructor to make it singleton managed
	FeaturesReporterFactory();
	FeaturesReporterFactory(const FeaturesReporterFactory & src); // unimplemented

	FeaturesReporterFactory const &
	operator=( FeaturesReporterFactory const & ); // unimplemented

	/// @brief private singleton creation function to be used with
	/// utility::thread::threadsafe_singleton
	static FeaturesReporterFactory * create_singleton_instance();

public:

	// Warning this is not called because of the singleton pattern
	virtual ~FeaturesReporterFactory();

	static FeaturesReporterFactory * get_instance();

	void factory_register( FeaturesReporterCreatorCOP creator );
	FeaturesReporterOP get_features_reporter( std::string const & type_name );

	/// @brief convienence header for use with RosettaScripts parse_my_tag interface
	FeaturesReporterOP
	get_features_reporter(
		utility::tag::TagCOP tag,
		basic::datacache::DataMap & data,
		protocols::filters::Filters_map const & filters,
		protocols::moves::Movers_map const & movers,
		core::pose::Pose const & pose);

	/// @brief Replace the load-time FeaturesReporterCreator with another creator.
	// undefined, commenting out to fix PyRosetta build  void replace_creator( FeaturesReporterCreatorCOP creator );

	// undefined, commenting out to fix PyRosetta build  FeaturesReporterCreatorCOP
	// get_creator( std::string const & type_name );

	utility::vector1<std::string> get_all_features_names();

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
	static std::atomic< FeaturesReporterFactory * > instance_;
#else
	static FeaturesReporterFactory * instance_;
#endif

	typedef std::map< std::string, protocols::features::FeaturesReporterCreatorCOP > FeaturesReporterCreatorMap;
	FeaturesReporterCreatorMap types_;
};


/// @brief This templated class will register an instance of an
/// FeaturesReporterCreator (class T) with the
/// FeaturesReporterFactory.  It will ensure that no
/// FeaturesReporterCreator is registered twice, and, centralizes this
/// registration logic so that thread safety issues can be handled in
/// one place
template < class T >
class FeaturesReporterRegistrator : public utility::factory::WidgetRegistrator< FeaturesReporterFactory, T >
{
public:
	typedef utility::factory::WidgetRegistrator< FeaturesReporterFactory, T > parent;
public:
	FeaturesReporterRegistrator() : parent() {}
};


} // namespace
} // namespace

#endif
