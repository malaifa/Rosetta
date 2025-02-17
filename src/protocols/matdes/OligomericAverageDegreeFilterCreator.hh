// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/matdes/OligomericAverageDegreeFilterCreator.hh
/// @brief  FilterCreator for the OligomericAverageDegreeFilter
/// @author Neil King (neilking@u.washington.edu)

#ifndef INCLUDED_protocols_matdes_OligomericAverageDegreeFilterCreator_hh
#define INCLUDED_protocols_matdes_OligomericAverageDegreeFilterCreator_hh


// Package Headers
#include <protocols/filters/FilterCreator.hh>

// Utility Headers
#include <utility/pointer/ReferenceCount.hh>

// c++ headers
#include <string>

namespace protocols {
namespace matdes {

class OligomericAverageDegreeFilterCreator : public protocols::filters::FilterCreator
{
public:
	virtual protocols::filters::FilterOP create_filter() const;
	virtual std::string keyname() const;
};


} //namespace matdes
} //namespace protocols

#endif
