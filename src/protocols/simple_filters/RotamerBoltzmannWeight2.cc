// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/simple_filters/RotamerBoltzmannWeight2.cc
/// @brief Next-generation RotamerBoltzmannWeight filter
/// @author Tom Linsky (tlinsky@uw.edu)

#include <protocols/simple_filters/RotamerBoltzmannWeight2.hh>
#include <protocols/simple_filters/RotamerBoltzmannWeight2Creator.hh>

// Protocol headers
#include <protocols/rosetta_scripts/util.hh>
#include <protocols/simple_filters/AlaScan.hh>
#include <protocols/simple_filters/DdgFilter.hh>
#include <protocols/toolbox/EnergyLandscapeEvaluator.hh>
#include <protocols/toolbox/pose_metric_calculators/RotamerBoltzCalculator.hh>

// Core headers
#include <core/conformation/Residue.hh>
#include <core/pose/metrics/CalculatorFactory.hh>
#include <core/pose/PDBInfo.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/scoring/ScoreFunctionFactory.hh>
#include <core/select/residue_selector/TrueResidueSelector.hh>

// Basic/Utility headers
#include <basic/MetricValue.hh>
#include <basic/Tracer.hh>
#include <utility/tag/Tag.hh>

static THREAD_LOCAL basic::Tracer TR( "protocols.simple_filters.RotamerBoltzmannWeight2" );

namespace protocols {
namespace simple_filters {

core::Real const RotamerBoltzmannWeight2::default_temperature_ = 0.8;
core::Real const RotamerBoltzmannWeight2::default_lambda_ = 0.5;

RotamerBoltzmannWeight2::RotamerBoltzmannWeight2():
	protocols::filters::Filter( "RotamerBoltzmannWeight2" ),
	score_type_( MEAN_PROBABILITY ),
	calculator_id_( new_calculator_id() ),
	scorefxn_( core::scoring::get_score_function() ),
	calculator_( new protocols::toolbox::pose_metric_calculators::RotamerBoltzCalculator( core::scoring::get_score_function(), default_temperature_ ) )
{
	calculator_->set_lazy( false );
	TR << "Created new calculator id: " << calculator_id_ << std::endl;
}

RotamerBoltzmannWeight2::~RotamerBoltzmannWeight2()
{
}

RotamerBoltzmannWeight2::RotamerBoltzmannWeight2( RotamerBoltzmannWeight2 const & rval ):
	protocols::filters::Filter( rval ),
	score_type_( rval.score_type_ ),
	calculator_id_( new_calculator_id() ),
	calculator_( new protocols::toolbox::pose_metric_calculators::RotamerBoltzCalculator( *rval.calculator_ ) )
{
	TR << "Created new calculator id: " << calculator_id_ << std::endl;
}

void
RotamerBoltzmannWeight2::parse_my_tag(
	utility::tag::TagCOP tag,
	basic::datacache::DataMap & data,
	protocols::filters::Filters_map const & ,
	protocols::moves::Movers_map const & ,
	core::pose::Pose const & )
{
	std::string const probability_type = tag->getOption< std::string >( "probability_type", "" );
	if ( !probability_type.empty() ) {
		if ( probability_type == "BOLTZMANN_SUM" ) {
			set_energy_landscape_evaluator( protocols::toolbox::EnergyLandscapeEvaluatorOP(
				new protocols::toolbox::RotamerBoltzmannWeightEvaluator( default_temperature_, true ) ) );
		} else if ( probability_type == "PNEAR" ) {
			set_energy_landscape_evaluator( protocols::toolbox::EnergyLandscapeEvaluatorOP(
				new protocols::toolbox::MulliganPNearEvaluator( default_temperature_, default_lambda_ ) ) );
		} else {
			std::stringstream msg;
			msg << "RotamerBoltzmannWeight2::parse_my_tag(): invalid probability_type specified: " << probability_type << std::endl;
			msg << "Valid score types are: BOLTZMANN_SUM, PNEAR" << std::endl;
			throw utility::excn::EXCN_RosettaScriptsOption( msg.str() );
		}
	}

	std::string const score_type = tag->getOption< std::string >( "score_type", "" );
	if ( !score_type.empty() ) {
		if ( score_type == "MEAN_PROBABILITY" ) {
			set_score_type( MEAN_PROBABILITY );
		} else if ( score_type == "MAX_PROBABILITY" ) {
			set_score_type( MAX_PROBABILITY );
		} else if ( score_type == "MODIFIED_DDG" ) {
			set_score_type( MODIFIED_DDG );
		} else {
			std::stringstream msg;
			msg << "RotamerBoltzmannWeight2::parse_my_tag(): invalid score_type specified: " << score_type << std::endl;
			msg << "Valid score types are: MEAN_PROBABILITY, MAX_PROBABILITY, MODIFIED_DDG" << std::endl;
			throw utility::excn::EXCN_RosettaScriptsOption( msg.str() );
		}
	}

	core::select::residue_selector::ResidueSelectorCOP selector = protocols::rosetta_scripts::parse_residue_selector( tag, data );
	if ( selector ) {
		set_residue_selector( selector );
	}

	core::scoring::ScoreFunctionOP scorefxn = protocols::rosetta_scripts::parse_score_function( tag, data );
	if ( scorefxn ) {
		set_scorefxn( scorefxn );
	}
}

protocols::filters::FilterOP
RotamerBoltzmannWeight2::clone() const
{
	return protocols::filters::FilterOP( new RotamerBoltzmannWeight2( *this ) );
}


protocols::filters::FilterOP
RotamerBoltzmannWeight2::fresh_instance() const
{
	return protocols::filters::FilterOP( new RotamerBoltzmannWeight2 );
}

std::string
RotamerBoltzmannWeight2::get_name() const
{
	return RotamerBoltzmannWeight2Creator::filter_name();
}

bool
RotamerBoltzmannWeight2::apply( core::pose::Pose const & ) const
{
	return true;
}

core::Real
RotamerBoltzmannWeight2::report_sm( core::pose::Pose const & pose ) const
{
	using protocols::toolbox::pose_metric_calculators::RotamerProbabilities;

	if ( !calculator_ ) {
		utility_exit_with_message( "RotamerBoltzmannWeight2::report_sm(): calculator is not set!" );
	}

	// create and register calculator if it's not already registered
	register_calculator();

	// retrieve calculator value
	basic::MetricValue< RotamerProbabilities > probabilities;
	pose.metric( calculator_id_, "probabilities", probabilities );

	return compute_score( pose, probabilities.value() );
}

void
RotamerBoltzmannWeight2::report( std::ostream & out, core::pose::Pose const & pose ) const
{
	using protocols::toolbox::pose_metric_calculators::RotamerProbabilities;

	// create and register calculator if it's not already registered
	register_calculator();

	basic::MetricValue< RotamerProbabilities > probabilities_metricvalue;
	pose.metric( calculator_id_, "probabilities", probabilities_metricvalue );
	RotamerProbabilities const & probabilities = probabilities_metricvalue.value();

	out << "Residue\tProbability\tEnergy_Reduction" << std::endl;;
	for ( RotamerProbabilities::const_iterator rp=probabilities.begin(); rp!=probabilities.end(); ++rp ) {
		core::Real const energy_reduction = compute_modified_residue_energy( rp->second );
		out << pose.residue(rp->first).name() << rp->first << '\t'
			<< rp->second << '\t' << energy_reduction << std::endl;
	}

	out << "Final score: " << compute_score( pose, probabilities ) << std::endl;
}

core::Real
compute_mean_probability( protocols::toolbox::pose_metric_calculators::RotamerProbabilities const & probs )
{
	using protocols::toolbox::pose_metric_calculators::RotamerProbabilities;

	core::Real sum = 0.0;
	for ( RotamerProbabilities::const_iterator rp=probs.begin(); rp!=probs.end(); ++rp ) {
		sum += rp->second;
	}
	return sum / static_cast< core::Real >( probs.size() );
}

core::Real
compute_max_probability( protocols::toolbox::pose_metric_calculators::RotamerProbabilities const & probs )
{
	using protocols::toolbox::pose_metric_calculators::RotamerProbabilities;

	if ( probs.empty() ) return 0.0;

	core::Real max = probs.begin()->second;
	for ( RotamerProbabilities::const_iterator rp=(++probs.begin()); rp!=probs.end(); ++rp ) {
		if ( rp->second > max ) max = rp->second;
	}
	return max;
}

core::Real
RotamerBoltzmannWeight2::compute_modified_residue_energy(
	toolbox::pose_metric_calculators::RotamerProbability const & probability ) const
{
	using protocols::toolbox::pose_metric_calculators::RotamerProbability;
	static core::Real const energy_reduction_factor = 0.5;
	static RotamerProbability const threshold_prob = 1.0;
	if ( probability <= threshold_prob ) {
		return -1 * default_temperature_ * log( probability ) * energy_reduction_factor;
	} else {
		return 0.0;
	}
}

core::Real
RotamerBoltzmannWeight2::compute_modified_ddg(
	core::pose::Pose const & pose,
	toolbox::pose_metric_calculators::RotamerProbabilities const & probs ) const
{
	using protocols::toolbox::pose_metric_calculators::RotamerProbability;
	using protocols::toolbox::pose_metric_calculators::RotamerProbabilities;

	core::Real const ddg_in = compute_ddg( pose );
	core::Real modified_ddG = ddg_in;
	for ( RotamerProbabilities::const_iterator rp=probs.begin(); rp!=probs.end(); ++rp ) {
		core::Size const res( rp->first );
		core::Real const energy = compute_modified_residue_energy( rp->second );
		modified_ddG += energy;
		TR.Debug << pose.residue( res ).name3() << pose.pdb_info()->number( res ) << '\t'
			<< '\t' << rp->second << '\t' << energy << std::endl;
	}
	TR.Debug << "ddG before, after modification: " << ddg_in << ", " << modified_ddG << std::endl;
	return modified_ddG;
}

core::Real
RotamerBoltzmannWeight2::compute_ddg( core::pose::Pose const & pose ) const
{
	static int const jump = 1;
	static core::Size const nrepeats = 3;
	protocols::simple_filters::DdgFilter ddg_filter( 100/*ddg_threshold*/, scorefxn_->clone(), jump, nrepeats );
	ddg_filter.repack( true );

	return ddg_filter.compute( pose );
}

core::Real
RotamerBoltzmannWeight2::compute_score(
	core::pose::Pose const & pose,
	protocols::toolbox::pose_metric_calculators::RotamerProbabilities const & probabilities ) const
{
	if ( score_type_ == MEAN_PROBABILITY ) {
		return -compute_mean_probability( probabilities );
	} else if ( score_type_ == MAX_PROBABILITY ) {
		return -compute_max_probability( probabilities );
	} else if ( score_type_ == MODIFIED_DDG ) {
		return compute_modified_ddg( pose, probabilities );
	} else {
		utility_exit_with_message( "Unknown score type" );
	}

	return static_cast< core::Real >(probabilities.size());
}

void
RotamerBoltzmannWeight2::set_residue_selector( core::select::residue_selector::ResidueSelectorCOP selector )
{
	calculator_->set_residue_selector( selector );
	unregister_calculator();
}

void
RotamerBoltzmannWeight2::set_energy_landscape_evaluator( protocols::toolbox::EnergyLandscapeEvaluatorCOP evaluator )
{
	calculator_->set_energy_landscape_evaluator( evaluator );
	unregister_calculator();
}

void
RotamerBoltzmannWeight2::set_scorefxn( core::scoring::ScoreFunctionCOP scorefxn )
{
	calculator_->set_scorefxn( scorefxn->clone() );
	scorefxn_ = scorefxn->clone();
	unregister_calculator();
}

void
RotamerBoltzmannWeight2::set_score_type( ScoreType const scoretype )
{
	score_type_ = scoretype;
}

void
RotamerBoltzmannWeight2::register_calculator() const
{
	// register calculator if it doesn't already exist
	if ( !core::pose::metrics::CalculatorFactory::Instance().check_calculator_exists( calculator_id_ ) ) {
		core::pose::metrics::CalculatorFactory::Instance().register_calculator( calculator_id_, calculator_ );
		TR << "Registered " << calculator_id_ << std::endl;
	}
}

void
RotamerBoltzmannWeight2::unregister_calculator()
{
	// un-register calculator if it exists
	if ( core::pose::metrics::CalculatorFactory::Instance().check_calculator_exists( calculator_id_ ) ) {
		core::pose::metrics::CalculatorFactory::Instance().remove_calculator( calculator_id_ );
		TR << "Unregistered " << calculator_id_ << std::endl;
		calculator_id_ = new_calculator_id();
	}
}

std::string
RotamerBoltzmannWeight2::new_calculator_id()
{
	core::Size const id = IdManager< core::Size >::get_instance()->register_new_id();
	std::stringstream calc_id;
	calc_id << RotamerBoltzmannWeight2Creator::filter_name() << "_" << id;
	return calc_id.str();
}

std::string const &
RotamerBoltzmannWeight2::calculator_id() const
{
	return calculator_id_;
}

/////////////// Creator ///////////////

protocols::filters::FilterOP
RotamerBoltzmannWeight2Creator::create_filter() const
{
	return protocols::filters::FilterOP( new RotamerBoltzmannWeight2 );
}

std::string
RotamerBoltzmannWeight2Creator::keyname() const
{
	return RotamerBoltzmannWeight2Creator::filter_name();
}

std::string
RotamerBoltzmannWeight2Creator::filter_name()
{
	return "RotamerBoltzmannWeight2";
}

//////////////// Calculator id management /////////////////////

template< class T >
IdManager< T >::IdManager():
utility::SingletonBase< IdManager< T > >(),
used_ids_()
{}

template< class T >
T const &
IdManager< T >::register_new_id()
{
	T test_id = 0;
	while ( true ) {
		if ( ! id_exists( test_id ) ) {
			return *(used_ids_.insert( test_id ).first);
		}
		++test_id;
	}
	runtime_assert( false );
	return *(used_ids_.end());
}

template< class T >
bool
IdManager< T >::id_exists( T const & id ) const
{
	return ( used_ids_.find( id ) != used_ids_.end() );
}

template< class T >
void
IdManager< T >::register_id( T const & id )
{
	if ( id_exists( id ) ) {
		std::stringstream msg;
		msg << "IdManager::register_id(): Attempting to register an id (" << id << ") that already exists!" << std::endl;
		utility_exit_with_message( msg.str() );
	}
	used_ids_.insert( id );
}

template< class T >
void
IdManager< T >::unregister_id( T const & id )
{
	std::stringstream msg;
	msg << "IdManager::register_id(): Attempting to unregister an id (" << id << ") that doesn't exist!" << std::endl;
	utility_exit_with_message( msg.str() );
}

template< class T >
IdManager< T > *
IdManager< T >::create_singleton_instance()
{
	return new IdManager< T >;
}

} //protocols
} //simple_filters

// Singleton instance and mutex static data members
namespace utility {

using protocols::simple_filters::IdManager;

#if defined MULTI_THREADED && defined CXX11
template<> std::mutex utility::SingletonBase< IdManager< core::Size > >::singleton_mutex_{};
template<> std::atomic< IdManager< core::Size > * > utility::SingletonBase< IdManager< core::Size > >::instance_( 0 );
#else
template<> IdManager< core::Size > * utility::SingletonBase< IdManager< core::Size > >::instance_( 0 );
#endif

}


