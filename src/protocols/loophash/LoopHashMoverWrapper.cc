// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.


/// @file protocols/loophash/movers/LoopHashMoverWrapper.cc
/// @brief
/// @author Sarel Fleishman (sarelf@u.washington.edu)

// Unit headers
#include <protocols/loophash/LoopHashMoverWrapper.hh>
#include <protocols/loophash/LoopHashMoverWrapperCreator.hh>

// Project headers
#include <core/pose/Pose.hh>
#include <protocols/rosetta_scripts/util.hh>
#include <core/pose/selection.hh>
#include <utility/tag/Tag.hh>
#include <boost/foreach.hpp>
#include <basic/Tracer.hh>
#include <basic/datacache/DataMap.hh>
#include <protocols/filters/Filter.hh>
#include <core/scoring/ScoreFunction.hh>
#include <core/pose/symmetry/util.hh>

#include <core/io/silent/SilentStruct.fwd.hh>
#include <core/io/silent/silent.fwd.hh>
#include <core/io/silent/SilentStructFactory.hh>
#include <core/io/silent/SilentStruct.hh>
#include <protocols/loophash/LoopHashLibrary.fwd.hh>
#include <protocols/loophash/LoopHashLibrary.hh>
#include <protocols/loophash/LoopHashMap.hh>
#include <protocols/loophash/LoopHashSampler.hh>
#include <protocols/loophash/LocalInserter.hh>
#include <protocols/loophash/BackboneDB.hh>
#include <protocols/relax/FastRelax.hh>
#include <core/util/SwitchResidueTypeSet.hh>
#include <utility/string_util.hh>


#include <numeric/random/random.hh>
#include <numeric/random/random_permutation.hh>

#include <utility/sort_predicates.hh>

#include <core/chemical/ChemicalManager.fwd.hh>
#include <core/pose/util.hh>
#include <utility/vector0.hh>
#include <utility/vector1.hh>

//Auto Headers
#include <core/id/AtomID.hh>

namespace protocols {
namespace loophash {

using core::pose::Pose;
using namespace utility;
using namespace protocols::moves;
using core::Real;
using core::Size;
using std::string;
using core::scoring::ScoreFunction;
using core::scoring::ScoreFunctionOP;

static THREAD_LOCAL basic::Tracer TR( "protocols.loophash.LoopHashMoverWrapper" );

std::string
LoopHashMoverWrapperCreator::keyname() const
{
	return LoopHashMoverWrapperCreator::mover_name();
}

protocols::moves::MoverOP
LoopHashMoverWrapperCreator::create_mover() const {
	return protocols::moves::MoverOP( new LoopHashMoverWrapper );
}

std::string
LoopHashMoverWrapperCreator::mover_name()
{
	return "LoopHash";
}


LoopHashMoverWrapper::LoopHashMoverWrapper() :
	protocols::moves::Mover( LoopHashMoverWrapperCreator::mover_name() ),
	library_( /* NULL */ ),
	fastrelax_( /* NULL */ ),
	min_bbrms_( 0 ),
	max_bbrms_( 0 ),
	min_rms_( 0 ),
	max_rms_( 0 ),
	start_res_( 2 ),
	stop_res_( 0 ),
	max_nstruct_( 1000000 ),
	cenfilter_( /* NULL */ ),
	ranking_cenfilter_( /* NULL */ ),
	fafilter_( /* NULL */ ),
	ranking_fafilter_( /* NULL */ ),
	nprefilter_( 0 ),
	prefilter_scorefxn_( /* NULL */ ),
	ideal_( false ),
	sample_weight_const_( 1 )
{
	loop_sizes_.clear();
}

LoopHashMoverWrapper::~LoopHashMoverWrapper() {}

void
LoopHashMoverWrapper::apply( Pose & pose )
{
	using namespace core::io::silent;
	runtime_assert( library_ != 0 );
	Pose const saved_pose( pose );

	core::util::switch_to_residue_type_set( pose, core::chemical::CENTROID );

	// symmetric->asymmetric
	if ( core::pose::symmetry::is_symmetric( pose ) ) {
		core::pose::Pose pose_asu;
		core::pose::symmetry::extract_asymmetric_unit(pose, pose_asu);
		pose = pose_asu;
	}

	core::pose::set_ss_from_phipsi( pose );

	LocalInserter_SimpleMinOP simple_inserter( new LocalInserter_SimpleMin() );
	LoopHashSampler lsampler( library_, simple_inserter );

	lsampler.set_start_res( start_res_ );
	lsampler.set_stop_res ( stop_res_ );
	lsampler.set_min_bbrms( min_bbrms() );
	lsampler.set_max_bbrms( max_bbrms() );
	lsampler.set_min_rms( min_rms() );
	lsampler.set_max_rms( max_rms() );
	lsampler.set_max_nstruct( max_nstruct() );
	lsampler.set_nonideal( !ideal_ );
	lsampler.use_prefiltering( prefilter_scorefxn_, nprefilter_ );
	lsampler.set_max_radius( max_radius_ );//all hardcoded for testing
	lsampler.set_max_struct( max_struct_ );
	lsampler.set_max_struct_per_radius( max_struct_per_radius_ );
	lsampler.set_filter_by_phipsi( filter_by_phipsi_ );

	std::vector< SilentStructOP > lib_structs;
	Size starttime = time( NULL );
	std::string sample_weight( "" );
	for ( Size resi = 1; resi <= pose.total_residue(); ++resi ) {
		sample_weight += utility::to_string( sample_weight_const_ ) + " ";
	}
	core::pose::add_comment( pose, "sample_weight", sample_weight );
	lsampler.build_structures( pose, lib_structs );
	Size endtime = time( NULL );
	Size nstructs = lib_structs.size();
	TR << "Found " << nstructs << " alternative states in time: " << endtime - starttime << std::endl;
	//std::random__shuffle( lib_structs.begin(), lib_structs.end() );
	numeric::random::random_permutation( lib_structs.begin(), lib_structs.end(), numeric::random::rg() );

	std::vector< std::pair< Real, SilentStructOP > > cen_scored_structs;
	BOOST_FOREACH ( SilentStructOP structure, lib_structs ) {
		Pose rpose;
		structure->fill_pose( rpose );

		// asymmetric->symmetric (if input was symmetric)
		if ( core::pose::symmetry::is_symmetric( saved_pose ) ) {
			core::pose::Pose pose_asu = rpose;
			rpose = saved_pose;

			// xyz copy
			utility::vector1< core::id::AtomID > atm_ids;
			utility::vector1< numeric::xyzVector< core::Real> > atm_xyzs;
			for ( Size i=1; i<=pose_asu.total_residue(); ++i ) {
				if ( pose_asu.residue_type(i).aa() == core::chemical::aa_vrt ) continue;
				for ( Size j=1; j<=pose_asu.residue_type(i).natoms(); ++j ) {
					core::id::AtomID atm_ij(j,i);
					atm_ids.push_back( atm_ij );
					atm_xyzs.push_back( pose_asu.xyz( atm_ij ) );
				}
			}
			rpose.batch_set_xyz( atm_ids, atm_xyzs );
		}

		// apply selection criteria
		bool passed_i = cenfilter_->apply( rpose );
		if ( passed_i ) {
			core::Real score_i = ranking_cenfilter()->report_sm( rpose );
			core::util::switch_to_residue_type_set( rpose, core::chemical::FA_STANDARD );

			core::io::silent::SilentStructOP new_struct = ideal_?
				core::io::silent::SilentStructFactory::get_instance()->get_silent_struct_out() :
				core::io::silent::SilentStructFactory::get_instance()->get_silent_struct("binary");
			new_struct->fill_struct( rpose );
			cen_scored_structs.push_back( std::pair<Real, SilentStructOP >(-score_i,new_struct) );
		}
	} // foreach structure

	// sort by centroid criteria
	std::sort( cen_scored_structs.begin(), cen_scored_structs.end(), utility::SortFirst<Real, SilentStructOP>() );
	if ( ncentroid_ >= cen_scored_structs.size() ) {
		all_structs_ = cen_scored_structs;
	} else {
		std::vector< std::pair< Real, SilentStructOP > >::const_iterator first = cen_scored_structs.end() - ncentroid_;
		std::vector< std::pair< Real, SilentStructOP > >::const_iterator last = cen_scored_structs.end();
		all_structs_ = std::vector< std::pair< Real, SilentStructOP > >(first, last);
	}
	TR << "After centroid filter: " << all_structs_.size() << " of " << cen_scored_structs.size() << " structures" << std::endl;

	// if a relax mover is specified, apply to each structure passing the filter
	if ( fastrelax_ ) {
		std::vector< std::pair< Real, SilentStructOP > > fa_scored_structs;
		std::vector< std::pair< Real, SilentStructOP > >::const_iterator it = all_structs_.begin();

		while ( it != all_structs_.end() ) {
			// prepare local batch
			std::vector < SilentStructOP > relax_structs;  // the local batch

			for ( int i=0; i<(int)batch_size_; ++i ) {
				SilentStructOP new_struct;
				new_struct = (*it).second->clone();

				relax_structs.push_back( new_struct );
				if ( ++it == all_structs_.end() ) break; // early exit
			}

			TR << "BATCHSIZE: " <<  relax_structs.size() << std::endl;
			fastrelax_->batch_apply( relax_structs );

			// Now save the resulting decoys
			BOOST_FOREACH ( SilentStructOP structure, relax_structs ) {
				// inflate ...
				Pose rpose;
				structure->fill_pose( rpose );

				// ... check against filter ...
				bool passed_i = fafilter_->apply( rpose );

				// ... and insert
				if ( passed_i ) {
					core::Real score_i = ranking_fafilter_->report_sm( rpose );
					core::io::silent::SilentStructOP new_struct =  ideal_?
						core::io::silent::SilentStructFactory::get_instance()->get_silent_struct_out() :
						core::io::silent::SilentStructFactory::get_instance()->get_silent_struct("binary");
					new_struct->fill_struct( rpose );
					fa_scored_structs.push_back( std::pair<Real, SilentStructOP >(-score_i,new_struct) );
				}
			}
		}

		// sort by fa criteria
		std::sort( fa_scored_structs.begin(), fa_scored_structs.end(), utility::SortFirst<Real, SilentStructOP>() );
		if ( nfullatom_ >= fa_scored_structs.size() ) {
			all_structs_ = fa_scored_structs;
		} else {
			std::vector< std::pair< Real, SilentStructOP > >::const_iterator first = fa_scored_structs.end() - nfullatom_;
			std::vector< std::pair< Real, SilentStructOP > >::const_iterator last = fa_scored_structs.end();
			all_structs_ = std::vector< std::pair< Real, SilentStructOP > >(first, last);
		}
		TR << "After fullatom filter: " << all_structs_.size() << " of " << cen_scored_structs.size() << " structures" << std::endl;
	}

	if ( all_structs_.size() == 0 ) {
		TR<<"No structures survived fullatom filter. Consider relaxing filters"<<std::endl;
		set_last_move_status( protocols::moves::FAIL_RETRY );
		return;
	}
	// pop best
	std::pair< Real, SilentStructOP > currbest = all_structs_.back();
	all_structs_.pop_back();
	TR << "Returning score = " << -currbest.first << std::endl;

	currbest.second->fill_pose( pose );
}

core::pose::PoseOP
LoopHashMoverWrapper::get_additional_output() {
	if ( all_structs_.size() == 0 ) {
		return NULL;
	}

	// pop best
	core::pose::PoseOP pose( new core::pose::Pose() );
	std::pair< core::Real, core::io::silent::SilentStructOP > currbest = all_structs_.back();
	all_structs_.pop_back();
	TR << "Returning score = " << -currbest.first << std::endl;
	currbest.second->fill_pose( *pose );

	return pose;
}


std::string
LoopHashMoverWrapper::get_name() const {
	return LoopHashMoverWrapperCreator::mover_name();
}

void
LoopHashMoverWrapper::parse_my_tag( TagCOP const tag,
	basic::datacache::DataMap & data,
	protocols::filters::Filters_map const &filters,
	Movers_map const &movers,
	Pose const & pose )
{
	min_bbrms_ = tag->getOption< Real >( "min_bbrms", 0 );
	max_bbrms_ = tag->getOption< Real >( "max_bbrms", 100000 );
	min_rms_ = tag->getOption< Real >( "min_rms",   0.0 );
	max_rms_ = tag->getOption< Real >( "max_rms",   4.0 );
	max_nstruct_ = tag->getOption< Size >( "max_nstruct",   1000000 );
	ideal_ = tag->getOption< bool >( "ideal",  false );  // by default, assume structure is nonideal
	max_radius_ = tag->getOption< Size >( "max_radius", 4 );
	max_struct_ = tag->getOption< Size >( "max_struct", 10 );
	max_struct_per_radius_ = tag->getOption< Size >( "max_struct_per_radius", 10 );
	filter_by_phipsi_ = tag->getOption< bool >( "filter_by_phipsi", 1 );
	sample_weight_const_ = tag->getOption< Real >( "sample_weight_const", 1.0 );

	start_res_ = 2;
	stop_res_ = 0;
	if ( tag->hasOption( "start_res_num" ) || tag->hasOption( "start_pdb_num") ) {
		start_res_ = core::pose::get_resnum( tag, pose, "start_" );
	}
	if ( tag->hasOption( "stop_res_num" ) || tag->hasOption( "stop_pdb_num") ) {
		stop_res_ = core::pose::get_resnum( tag, pose, "stop_" );
	}

	string const loop_sizes_str( tag->getOption< string >( "loop_sizes" ) );
	vector1< string > const loop_sizes_split( utility::string_split( loop_sizes_str, ',' ) );
	BOOST_FOREACH ( string const loop_size, loop_sizes_split ) {
		add_loop_size( (Size)std::atoi(loop_size.c_str()) ) ;
	}

	// path to DB -- if not specified then command-line flag is used
	library_ = LoopHashLibraryOP( new LoopHashLibrary( loop_sizes() , 1 , 0 ) );
	if ( tag->hasOption( "db_path" ) ) {
		std::string db_path = tag->getOption< string >( "db_path" );
		library_->set_db_path( db_path );
	}
	library_->load_mergeddb();
	library_->mem_foot_print();

	// FILTERING STEP 1 --- filter with chainbreak
	nprefilter_ = tag->getOption< Size >( "nprefilter", 0 );
	if ( tag->hasOption( "prefilter_scorefxn" ) ) {
		string const prefilter_scorefxn_name( tag->getOption< string >( "prefilter_scorefxn" ) );
		prefilter_scorefxn_ = data.get_ptr< ScoreFunction >( "scorefxns", prefilter_scorefxn_name );
	}

	// FILTERING STEP 2 --- filter after idealization (+symmetrization)
	// number of structures to accept
	// 0 = accept everything passing the filter
	ncentroid_ = tag->getOption< Size >( "ncentroid",  0 );
	nfullatom_ = ncentroid_;

	// centroid filter
	string const centroid_filter_name( tag->getOption< string >( "centroid_filter", "true_filter" ) );
	Filters_map::const_iterator find_cenfilter( filters.find( centroid_filter_name ) );
	if ( find_cenfilter == filters.end() ) {
		utility_exit_with_message( "Filter " + centroid_filter_name + " not found in LoopHashMoverWrapper" );
	}
	cenfilter( find_cenfilter->second );
	ranking_cenfilter( protocols::rosetta_scripts::parse_filter( tag->getOption< std::string >( "ranking_cenfilter", centroid_filter_name ), filters ) );

	// batch relax mover
	if ( tag->hasOption( "relax_mover" ) ) {
		string const relax_mover_name( tag->getOption< string >( "relax_mover" ) );
		Movers_map::const_iterator find_mover( movers.find( relax_mover_name ) );
		bool const mover_found( find_mover != movers.end() );
		if ( mover_found ) {
			relax_mover( utility::pointer::dynamic_pointer_cast< protocols::relax::FastRelax > ( find_mover->second ) );
		} else {
			utility_exit_with_message( "Mover " + relax_mover_name + " not found in LoopHashMoverWrapper" );
		}

		// nonideal
		fastrelax_->set_force_nonideal( !ideal_ );

		// batch size
		batch_size_ = tag->getOption< Size >( "batch_size", 32 );

		// maximum number of structures to accept
		// may be less depending on # of centroid structures actually generated, ncentroid_, and batch_size_;
		nfullatom_ = tag->getOption< Size >( "nfullatom",  ncentroid_ );

		// fullatom filter <<<< used to select best 'nfullatom' from all decoys
		string const fullatom_filter_name( tag->getOption< string >( "fullatom_filter", "true_filter" ) );
		Filters_map::const_iterator find_fafilter( filters.find( fullatom_filter_name ) );
		if ( find_fafilter == filters.end() ) {
			utility_exit_with_message( "Filter " + fullatom_filter_name + " not found in LoopHashMoverWrapper" );
		}
		fafilter( find_fafilter->second );
		ranking_fafilter( protocols::rosetta_scripts::parse_filter( tag->getOption< std::string> ( "ranking_fafilter", fullatom_filter_name ), filters ) );
	} else {
		if ( tag->hasOption(  "batch_size" ) ) TR << "Ignoring option batch_size" << std::endl;
		if ( tag->hasOption(  "nfullatom" ) ) TR << "Ignoring option nfullatom" << std::endl;
		if ( tag->hasOption(  "fullatom_filter" ) ) TR << "Ignoring option fullatom_filter" << std::endl;
	}
}

Real
LoopHashMoverWrapper::min_bbrms() const{
	return min_bbrms_;
}

Real
LoopHashMoverWrapper::max_bbrms() const{
	return max_bbrms_;
}

Real
LoopHashMoverWrapper::min_rms() const{
	return min_rms_;
}

Real
LoopHashMoverWrapper::max_rms() const{
	return max_rms_;
}

void
LoopHashMoverWrapper::min_bbrms( Real const min_bbrms ){
	min_bbrms_ = min_bbrms;
}

void
LoopHashMoverWrapper::max_bbrms( Real const max_bbrms ){
	max_bbrms_ = max_bbrms;
}

void
LoopHashMoverWrapper::min_rms( Real const min_rms ){
	min_rms_ = min_rms;
}

void
LoopHashMoverWrapper::max_rms( Real const max_rms ){
	max_rms_ = max_rms;
}

Size
LoopHashMoverWrapper::max_nstruct() const {
	return max_nstruct_;
}

void
LoopHashMoverWrapper::max_nstruct( Size const max_nstruct ) {
	max_nstruct_ = max_nstruct;
}

void
LoopHashMoverWrapper::relax_mover( protocols::relax::FastRelaxOP relax_mover ){
	fastrelax_ = relax_mover;
}

void
LoopHashMoverWrapper::cenfilter( protocols::filters::FilterOP cenfilter ) {
	cenfilter_ = cenfilter;
}

void
LoopHashMoverWrapper::fafilter( protocols::filters::FilterOP fafilter ) {
	fafilter_ = fafilter;
}

void
LoopHashMoverWrapper::ranking_fafilter( protocols::filters::FilterOP ranking_fafilter ) {
	ranking_fafilter_ = ranking_fafilter;
}

utility::vector1< Size >
LoopHashMoverWrapper::loop_sizes() const{
	return loop_sizes_;
}

void
LoopHashMoverWrapper::add_loop_size( Size const loop_size ){
	loop_sizes_.push_back( loop_size );
}

} //loophash
} //protocols

