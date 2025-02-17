// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   ./src/protocols/fldsgn/topology/HSSTriplet.fwd.hh
/// @brief
/// @author Nobuyasu Koga ( nobuyasu@u.washington.edu )

#ifndef INCLUDED_protocols_fldsgn_topology_HSSTriplet_fwd_hh
#define INCLUDED_protocols_fldsgn_topology_HSSTriplet_fwd_hh

// utitlity headers
#include <utility/pointer/owning_ptr.hh>
#include <utility/vector1.hh>

namespace protocols {
namespace fldsgn {
namespace topology {

class HSSTriplet;
class HSSTripletSet;

typedef utility::pointer::shared_ptr< HSSTriplet > HSSTripletOP;
typedef utility::pointer::shared_ptr< HSSTripletSet > HSSTripletSetOP;
typedef utility::pointer::shared_ptr< HSSTriplet const > HSSTripletCOP;
typedef utility::pointer::shared_ptr< HSSTripletSet const > HSSTripletSetCOP;
typedef utility::vector1< HSSTripletOP > HSSTriplets;

typedef HSSTriplets::const_iterator HSSConstIterator;
typedef HSSTriplets::iterator HSSIterator;

} // namespace topology
} // namespace fldsgn
} // namespace protocols

#endif
