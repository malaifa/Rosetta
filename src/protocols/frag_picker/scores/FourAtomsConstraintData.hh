// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file   protocols/frag_picker/scores/FourAtomsConstraintData.hh
/// @brief  provides a holder for data necessary to create a single constraint based on four atoms
/// @author Dominik Gront (dgront@chem.uw.edu.pl)

#ifndef INCLUDED_protocols_frag_picker_scores_FourAtomsConstraintData_hh
#define INCLUDED_protocols_frag_picker_scores_FourAtomsConstraintData_hh

#include <protocols/frag_picker/scores/FourAtomsConstraintData.fwd.hh>

// mini
#include <core/scoring/func/Func.hh>

// utility headers
#include <utility/pointer/ReferenceCount.hh>


namespace protocols {
namespace frag_picker {
namespace scores {

/// @brief Holds data about a single four-body constraint in the form usefull for InterbondAngleScore and DihedralConstraintsScore classes
/// @details This class is used to store data obtained from file before score method is created
class FourAtomsConstraintData: public utility::pointer::ReferenceCount {
public:

	/// @brief makes a new object
	FourAtomsConstraintData(core::scoring::func::FuncOP function,
		Size first_atom, Size second_offset, Size second_atom,
		Size third_offset, Size third_atom, Size fourth_offset,
		Size fourth_atom);

	inline Size get_first_atom() {
		return first_atom_;
	}

	inline Size get_second_atom() {
		return second_atom_;
	}

	inline Size get_third_atom() {
		return third_atom_;
	}

	inline Size get_fourth_atom() {
		return fourth_atom_;
	}

	inline Size get_second_offset() {
		return second_offset_;
	}

	inline Size get_third_offset() {
		return third_offset_;
	}

	inline Size get_fourth_offset() {
		return fourth_offset_;
	}
	inline core::scoring::func::FuncOP get_function() {
		return func_;
	}

	virtual ~FourAtomsConstraintData();
private:
	core::scoring::func::FuncOP func_;
	Size first_atom_;
	Size second_atom_;
	Size third_atom_;
	Size fourth_atom_;
	Size second_offset_;
	Size third_offset_;
	Size fourth_offset_;
};

} // scores
} // frag_picker
} // protocols

#endif // INCLUDED_protocols_frag_picker_scores_FourAtomsConstraintData_HH
