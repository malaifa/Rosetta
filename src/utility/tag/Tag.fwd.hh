// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file
/// @brief
/// @author Sarel Fleishman


#ifndef INCLUDED_utility_tag_Tag_fwd_hh
#define INCLUDED_utility_tag_Tag_fwd_hh

#include <utility/pointer/owning_ptr.hh>
#include <utility/pointer/access_ptr.hh>

namespace utility {
namespace tag {

class Tag;
typedef utility::pointer::shared_ptr< Tag > TagPtr;
typedef utility::pointer::shared_ptr< Tag > TagOP;
typedef utility::pointer::shared_ptr< Tag const > TagCOP;
typedef utility::pointer::weak_ptr< Tag > TagAP;
typedef utility::pointer::weak_ptr< Tag const > TagCAP;

}
}
#endif

