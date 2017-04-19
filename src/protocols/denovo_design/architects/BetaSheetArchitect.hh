// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/denovo_design/architects/BetaSheetArchitect.hh
/// @brief Architect that creates a beta sheet
/// @author Tom Linsky (tlinsky@uw.edu)

#ifndef INCLUDED_protocols_denovo_design_architects_BetaSheetArchitect_hh
#define INCLUDED_protocols_denovo_design_architects_BetaSheetArchitect_hh

// Unit headers
#include <protocols/denovo_design/architects/BetaSheetArchitect.fwd.hh>
#include <protocols/denovo_design/architects/StructureArchitect.hh>

// Protocol headers
#include <protocols/denovo_design/architects/StrandArchitect.hh>
#include <protocols/denovo_design/components/SheetDB.fwd.hh>
#include <protocols/denovo_design/components/StructureData.fwd.hh>
#include <protocols/denovo_design/types.hh>

// Core headers
#include <core/pose/Pose.fwd.hh>

// Utility headers
#include <utility/excn/EXCN_Base.hh>
#include <utility/pointer/owning_ptr.hh>
#include <utility/pointer/ReferenceCount.hh>

namespace protocols {
namespace denovo_design {
namespace architects {

///@brief Architect that creates a beta sheet
class BetaSheetArchitect : public protocols::denovo_design::architects::DeNovoArchitect {
public:
	typedef protocols::denovo_design::architects::DeNovoArchitect DeNovoArchitect;
	typedef protocols::denovo_design::architects::DeNovoArchitectOP DeNovoArchitectOP;
	typedef protocols::denovo_design::components::StructureData StructureData;
	typedef protocols::denovo_design::components::StructureDataOP StructureDataOP;
	typedef utility::vector1< components::StructureDataCOP > StructureDataCOPs;
	typedef utility::vector1< StrandArchitectOP > StrandArchitectOPs;

public:
	BetaSheetArchitect( std::string const & id_value );

	virtual ~BetaSheetArchitect();

	static std::string
	class_name() { return "BetaSheetArchitect"; }

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
	Lengths
	retrieve_lengths( components::StructureData const & perm ) const;

	StrandOrientations
	retrieve_orientations( components::StructureData const & perm ) const;

	RegisterShifts
	retrieve_register_shifts( components::StructureData const & perm ) const;

private:
	void
	setup_strand_pairings();

	void
	add_strand( StrandArchitect const & strand );

	/// @brief generates and stores a vector of permutations based on strands
	void
	enumerate_permutations();

	/// @brief merges a list of permutations
	StructureDataOP
	combine_permutations( components::StructureDataCOPs const & chain ) const;

	/// @brief combines the given set of permutations with the current set
	void
	combine_permutations_rec(
		components::StructureDataCOPs const & chain,
		utility::vector1< components::StructureDataCOPs > const & plist );

	/// @brief modifies/stores data into a permutation and adds it
	void
	modify_and_add_permutation( StructureData const & perm );

	/// @brief checks permutations
	/// @throws EXCN_PreFilterFailed if something goes wrong
	void
	check_permutation( StructureData const & perm ) const;

private:
	void
	needs_update();

	void
	store_strand_pairings( StructureData & sd ) const;

	void
	store_sheet_idx( StructureData & sd, core::Size const sheet_idx ) const;

private:
	StructureDataCOPs permutations_;
	StrandArchitectOPs strands_;
	components::SheetDBOP sheetdb_;
	bool use_sheetdb_;
	bool updated_;
};

class EXCN_PreFilterFailed : public utility::excn::EXCN_Base {
public:
	EXCN_PreFilterFailed( std::string const & msg ) :
		utility::excn::EXCN_Base(), msg_( msg )
	{}

	virtual void
	show( std::ostream & os ) const { os << msg_ << std::endl; }

private:
	std::string const msg_;
};

} //protocols
} //denovo_design
} //architects

#endif //INCLUDED_protocols_denovo_design_architects_BetaSheetArchitect_hh

