// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.
//
/// @file protocols/flxbb/FlxbbDesign.fwd.hh
/// @brief perform cycles of design and relax with filter
/// @author Nobuyasu Koga ( nobuyasu@uw.edu )

#ifndef INCLUDED_protocols_flxbb_FlxbbDesign_fwd_hh
#define INCLUDED_protocols_flxbb_FlxbbDesign_fwd_hh

#include <utility/pointer/owning_ptr.hh>

namespace protocols {
namespace flxbb {

class FlxbbDesign;
typedef utility::pointer::shared_ptr< FlxbbDesign > FlxbbDesignOP;


} // namespace flxbb
} // namespace protocols

#endif
