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
/// @author

// Unit headers
#include <core/scoring/etable/EtableOptions.hh>

#include <basic/options/option.hh>

#include <basic/Tracer.hh>

// option key includes

#include <basic/options/keys/score.OptionKeys.gen.hh>
#include <basic/options/keys/corrections.OptionKeys.gen.hh>

#include <utility/vector1.hh>
#include <utility/tag/Tag.hh>
#include <utility/tag/XMLSchemaGeneration.hh>


#ifdef SERIALIZATION
// Utility serialization headers
#include <utility/serialization/serialization.hh>

// Cereal headers
#include <cereal/types/base_class.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#endif // SERIALIZATION


namespace core {
namespace scoring {
namespace etable {

EtableOptions::EtableOptions() :
	etable_type( "FA_STANDARD_DEFAULT" ), // this string must be kept the same as the variable in ScoringManager.cc named FA_STANDARD_DEFAULT
	analytic_etable_evaluation( false ),
	max_dis( 6.0 ),
	bins_per_A2( 20 ),
	Wradius( 1.0 ),
	lj_switch_dis2sigma( 0.6 ),
	no_lk_polar_desolvation( false ),
	lj_hbond_OH_donor_dis(3.0),
	lj_hbond_hdis(1.95),
	enlarge_h_lj_wdepth(false)
{
	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	analytic_etable_evaluation = option[ score::analytic_etable_evaluation ];
	max_dis = option[ score::fa_max_dis ];
	if ( option[ score::no_smooth_etables ] && !option[ score::fa_max_dis ].user() ) {
		basic::T("core.scoring.etable") << "no_smooth_etables requested and fa_max_dis not specified: using 5.5 as default" << std::endl;
		max_dis = 5.5;
	}
	if ( option[ score::no_lk_polar_desolvation ] ) {
		no_lk_polar_desolvation = false;
	}

	lj_hbond_OH_donor_dis = option[ corrections::score::lj_hbond_OH_donor_dis ];
	lj_hbond_hdis = option[ corrections::score::lj_hbond_hdis ];

}

// Another constructor
EtableOptions::EtableOptions( EtableOptions const & src ) :
	ReferenceCount( src )
{
	*this = src;
}

EtableOptions::~EtableOptions(){}

EtableOptions &
EtableOptions::operator=( EtableOptions const & src )
{
	if ( this != & src ) {
		etable_type = src.etable_type;
		analytic_etable_evaluation = src.analytic_etable_evaluation;
		max_dis = src.max_dis;
		bins_per_A2 = src.bins_per_A2;
		Wradius = src.Wradius;
		lj_switch_dis2sigma = src.lj_switch_dis2sigma;
		no_lk_polar_desolvation = src.no_lk_polar_desolvation;
		lj_hbond_OH_donor_dis = src.lj_hbond_OH_donor_dis;
		lj_hbond_hdis = src.lj_hbond_hdis;
		enlarge_h_lj_wdepth = src.enlarge_h_lj_wdepth;
	}
	return *this;
}

bool
operator < ( EtableOptions const & a, EtableOptions const & b )
{
	if      ( a.etable_type < b.etable_type )  { return true;  }
	else if ( a.etable_type != b.etable_type ) { return false; }

	if      ( a.analytic_etable_evaluation < b.analytic_etable_evaluation ) { return true; }
	else if ( a.analytic_etable_evaluation != b.analytic_etable_evaluation ) { return false; }

	if      ( a.max_dis < b.max_dis )  { return true;  }
	else if ( a.max_dis != b.max_dis ) { return false; }

	if      ( a.bins_per_A2 < b.bins_per_A2 )  { return true;  }
	else if ( a.bins_per_A2 != b.bins_per_A2 ) { return false; }

	if      ( a.Wradius < b.Wradius )  { return true;  }
	else if ( a.Wradius != b.Wradius ) { return false; }

	if      ( a.lj_switch_dis2sigma < b.lj_switch_dis2sigma )  { return true;  }
	else if ( a.lj_switch_dis2sigma != b.lj_switch_dis2sigma ) { return false; }

	if      ( a.no_lk_polar_desolvation < b.no_lk_polar_desolvation )  { return true;  }
	else if ( a.no_lk_polar_desolvation != b.no_lk_polar_desolvation ) { return false; }

	if      ( a.lj_hbond_OH_donor_dis < b.lj_hbond_OH_donor_dis )  { return true;  }
	else if ( a.lj_hbond_OH_donor_dis != b.lj_hbond_OH_donor_dis ) { return false; }

	if      ( a.lj_hbond_hdis < b.lj_hbond_hdis )  { return true;  }
	else if ( a.lj_hbond_hdis != b.lj_hbond_hdis ) { return false; }

	if      ( a.enlarge_h_lj_wdepth < b.enlarge_h_lj_wdepth ) { return true; }
	else return false;
}


bool
operator==( EtableOptions const & a, EtableOptions const & b )
{
	return ((a.etable_type == b.etable_type ) &&
		(a.analytic_etable_evaluation == b.analytic_etable_evaluation ) &&
		( a.max_dis == b.max_dis ) &&
		( a.bins_per_A2 == b.bins_per_A2 ) &&
		( a.Wradius == b.Wradius ) &&
		( a.lj_switch_dis2sigma == b.lj_switch_dis2sigma ) &&
		( a.no_lk_polar_desolvation == b.no_lk_polar_desolvation ) &&
		( a.lj_hbond_OH_donor_dis == b.lj_hbond_OH_donor_dis ) &&
		( a.lj_hbond_hdis == b.lj_hbond_hdis ) &&
		( a.enlarge_h_lj_wdepth == b.enlarge_h_lj_wdepth ));
}


////////////////////////////
std::ostream &
operator<< ( std::ostream & out, const EtableOptions & options ){
	options.show( out );
	return out;
}


void
EtableOptions::show( std::ostream & out ) const
{
	out <<"EtableOptions::max_dis: " << max_dis << std::endl;
	out <<"EtableOptions::bins_per_A2: " << bins_per_A2 << std::endl;
	out <<"EtableOptions::Wradius: " << Wradius << std::endl;
	out <<"EtableOptions::lj_switch_dis2sigma: " << lj_switch_dis2sigma << std::endl;
	out <<"EtableOptions::no_lk_polar_desolvation: " << no_lk_polar_desolvation << std::endl;
	out <<"EtableOptions::lj_hbond_OH_donor_dis: " << lj_hbond_OH_donor_dis << std::endl;
	out <<"EtableOptions::lj_hbond_hdis: " << lj_hbond_hdis << std::endl;
	out <<"EtableOptions::enlarge_h_lj_wdepth: " << enlarge_h_lj_wdepth << std::endl;
}

void
EtableOptions::parse_my_tag(
	utility::tag::TagCOP tag
) {
	if ( tag->hasOption( "lj_hbond_OH_donor_dis" ) ) {
		lj_hbond_OH_donor_dis = tag->getOption<core::Real>( "lj_hbond_OH_donor_dis" );
	}

	if ( tag->hasOption( "lj_hbond_hdis" ) ) {
		lj_hbond_hdis = tag->getOption<core::Real>( "lj_hbond_hdis" );
	}
}

void
EtableOptions::append_schema_attributes( utility::tag::AttributeList & attributes )
{
	using namespace utility::tag;
	attributes
		+ XMLSchemaAttribute( "lj_hbond_OH_donor_dis", xs_decimal )
		+ XMLSchemaAttribute( "lj_hbond_hdis", xs_decimal );
}

} // etable
} // scoring
} // core



#ifdef    SERIALIZATION

/// @brief Automatically generated serialization method
template< class Archive >
void
core::scoring::etable::EtableOptions::save( Archive & arc ) const {
	arc( CEREAL_NVP( etable_type ) ); // std::string
	arc( CEREAL_NVP( analytic_etable_evaluation ) ); // _Bool
	arc( CEREAL_NVP( max_dis ) ); // Real
	arc( CEREAL_NVP( bins_per_A2 ) ); // int
	arc( CEREAL_NVP( Wradius ) ); // Real
	arc( CEREAL_NVP( lj_switch_dis2sigma ) ); // Real
	arc( CEREAL_NVP( no_lk_polar_desolvation ) ); // _Bool
	arc( CEREAL_NVP( lj_hbond_OH_donor_dis ) ); // Real
	arc( CEREAL_NVP( lj_hbond_hdis ) ); // Real
	arc( CEREAL_NVP( enlarge_h_lj_wdepth ) ); // _Bool
}

/// @brief Automatically generated deserialization method
template< class Archive >
void
core::scoring::etable::EtableOptions::load( Archive & arc ) {
	arc( etable_type ); // std::string
	arc( analytic_etable_evaluation ); // _Bool
	arc( max_dis ); // Real
	arc( bins_per_A2 ); // int
	arc( Wradius ); // Real
	arc( lj_switch_dis2sigma ); // Real
	arc( no_lk_polar_desolvation ); // _Bool
	arc( lj_hbond_OH_donor_dis ); // Real
	arc( lj_hbond_hdis ); // Real
	arc( enlarge_h_lj_wdepth ); // _Bool
}

SAVE_AND_LOAD_SERIALIZABLE( core::scoring::etable::EtableOptions );
CEREAL_REGISTER_TYPE( core::scoring::etable::EtableOptions )

CEREAL_REGISTER_DYNAMIC_INIT( core_scoring_etable_EtableOptions )
#endif // SERIALIZATION
