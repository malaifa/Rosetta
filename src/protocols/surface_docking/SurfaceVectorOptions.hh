// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/surface_docking/SurfaceVectorOptions.hh
/// @brief
/// @author Michael Pacella (mpacella88@gmail.com)

#ifndef INCLUDED_protocols_surface_docking_SurfaceVectorOptions_hh
#define INCLUDED_protocols_surface_docking_SurfaceVectorOptions_hh

//unit headers
#include <protocols/surface_docking/SurfaceVectorOptions.fwd.hh>

//project headers
#include <basic/resource_manager/ResourceOptions.hh>

//utility headers
#include <utility/pointer/ReferenceCount.hh>
#include <utility/tag/Tag.fwd.hh>

//C++ headers

namespace protocols {
namespace surface_docking {

class SurfaceVectorOptions : public basic::resource_manager::ResourceOptions
{
public:
	SurfaceVectorOptions();
	virtual ~SurfaceVectorOptions();

	virtual
	void
	parse_my_tag(
		utility::tag::TagCOP tag
	);

	virtual
	std::string
	type() const;

};

} // namespace surface_docking
} // namespace protocols


#endif //INCLUDED_protocols_surface_docking_SurfaceVectorOptions_hh
