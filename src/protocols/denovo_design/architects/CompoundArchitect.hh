// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/denovo_design/architects/CompoundArchitect.hh
/// @brief Architect that creates a StructureData using multiple architects
/// @author Tom Linsky (tlinsky@uw.edu)

#ifndef INCLUDED_protocols_denovo_design_architects_CompoundArchitect_hh
#define INCLUDED_protocols_denovo_design_architects_CompoundArchitect_hh

// Unit headers
#include <protocols/denovo_design/architects/CompoundArchitect.fwd.hh>
#include <protocols/denovo_design/architects/DeNovoArchitect.hh>

// Protocol headers
#include <protocols/denovo_design/components/StructureData.fwd.hh>
#include <protocols/denovo_design/connection/ConnectionArchitect.fwd.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>

// Utility headers
#include <utility/pointer/owning_ptr.hh>
#include <utility/pointer/ReferenceCount.hh>

namespace protocols {
namespace denovo_design {
namespace architects {

///@brief Architect that creates a StructureData using multiple architects
class CompoundArchitect : public protocols::denovo_design::architects::DeNovoArchitect {
public:
	typedef protocols::denovo_design::architects::DeNovoArchitect DeNovoArchitect;
	typedef protocols::denovo_design::architects::DeNovoArchitectOP DeNovoArchitectOP;
	typedef protocols::denovo_design::components::StructureData StructureData;
	typedef protocols::denovo_design::components::StructureDataOP StructureDataOP;

public:
	CompoundArchitect( std::string const & id_value );

	virtual ~CompoundArchitect();

	static std::string
	class_name() { return "CompoundArchitect"; }

	virtual std::string
	type() const;

	DeNovoArchitectOP
	clone() const;

	virtual StructureDataOP
	design( core::pose::Pose const & pose, core::Real & random ) const;

protected:
	virtual void
	parse_tag( utility::tag::TagCOP tag, basic::datacache::DataMap & data );

public:
	void
	add_architect( DeNovoArchitect const & architect );

	void
	add_connection( connection::ConnectionArchitect const & connection );

private:
	void
	parse_architect_tags( utility::tag::TagCOP tag, basic::datacache::DataMap & data );

	void
	parse_architect_tag( utility::tag::TagCOP tag, basic::datacache::DataMap & data );

	void
	parse_connection_tags( utility::tag::TagCOP tag, basic::datacache::DataMap & data );

	void
	parse_connection_tag( utility::tag::TagCOP tag, basic::datacache::DataMap & data );

	void
	add_parent_name( StructureArchitect & architect ) const;

private:
	DeNovoArchitectCOPs architects_;
	connection::ConnectionArchitectCOPs connections_;

};

} //protocols
} //denovo_design
} //architects

#endif //INCLUDED_protocols_denovo_design_architects_CompoundArchitect_hh

