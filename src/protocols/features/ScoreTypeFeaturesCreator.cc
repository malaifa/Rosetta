// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/feature/ScoreTypeFeaturesCreator.hh
/// @brief  Header for ScoreTypeFeaturesCreator for the ScoreTypeFeatures load-time factory registration scheme
/// @author Matthew O'Meara

// Unit Headers
#include <protocols/features/ScoreTypeFeaturesCreator.hh>

// Package Headers

#include <protocols/features/FeaturesReporterCreator.hh>

#include <protocols/features/ScoreTypeFeatures.hh>
#include <utility/vector1.hh>


namespace protocols {
namespace features {

ScoreTypeFeaturesCreator::ScoreTypeFeaturesCreator() {}
ScoreTypeFeaturesCreator::~ScoreTypeFeaturesCreator() {}
FeaturesReporterOP ScoreTypeFeaturesCreator::create_features_reporter() const {
	return FeaturesReporterOP( new ScoreTypeFeatures );
}

std::string ScoreTypeFeaturesCreator::type_name() const {
	return "ScoreTypeFeatures";
}

} //namespace features
} //namespace protocols
