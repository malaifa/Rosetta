// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

/// @file core/io/silent/SilentStruct.cc
///
/// @brief silent input file reader for mini.
/// @author James Thompson

// C++ Headers
#include <iostream>
#include <string>
#include <map>

#ifdef WIN32
#include <ctime>
#endif

#ifdef __CYGWIN__
#include <ctime>
#endif

// mini headers

#include <ObjexxFCL/char.functions.hh>
#include <ObjexxFCL/string.functions.hh>
#include <core/io/silent/ProteinSilentStruct.hh>

#include <core/io/silent/SilentStruct.hh>
#include <core/io/silent/EnergyNames.hh>
#include <core/io/silent/SilentFileData.hh>
#include <core/io/silent/SilentStructFactory.hh>

#include <basic/Tracer.hh>
#include <basic/datacache/BasicDataCache.hh>
#include <basic/datacache/CacheableString.hh>
#include <basic/datacache/CacheableStringFloatMap.hh>
#include <basic/datacache/WriteableCacheableMap.hh>
#include <basic/datacache/WriteableCacheableDataFactory.hh>

#include <basic/options/option.hh>

#include <core/pose/Pose.hh>
#include <core/pose/PDBInfo.hh>
#include <core/pose/util.hh>
#include <core/pose/datacache/CacheableDataType.hh>
#include <core/pose/full_model_info/FullModelInfo.hh>
#include <core/pose/full_model_info/SubMotifInfo.hh>
#include <core/pose/full_model_info/util.hh>

#include <core/chemical/ChemicalManager.hh>
#include <core/conformation/Residue.fwd.hh>
#include <core/scoring/Energies.hh>
#include <core/scoring/EnergyMap.hh>

#include <core/scoring/ScoreType.hh>
#include <core/scoring/ScoreTypeManager.hh>


// option key includes

#include <basic/options/keys/out.OptionKeys.gen.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>

#include <utility/vector1.hh>
#include <utility/string_util.hh>
#include <core/sequence/AnnotatedSequence.hh>

#include <ObjexxFCL/format.hh>


namespace core {
namespace io {
namespace silent {

static THREAD_LOCAL basic::Tracer tr( "core.io.silent.SilentStruct" );


template <>
bool ProteinSilentStruct_Template < float >::is_single_precision() { return true; }

template <>
bool ProteinSilentStruct_Template< core::Real >::is_single_precision() { return false; }


SilentStruct::SilentStruct()
: force_bitflip_(false), strict_column_mode_(false), nres_(0), decoy_tag_(""), sequence_(""), precision_(3), scoreline_prefix_("SCORE: ")
{}

SilentStruct::~SilentStruct() {}

SilentStruct::SilentStruct( SilentStruct const& src ) :
	ReferenceCount(),
	utility::pointer::enable_shared_from_this< SilentStruct >()
{
	*this = src;
}

SilentStruct& SilentStruct::operator= ( SilentStruct const& src ) {
	force_bitflip_ = src.force_bitflip_;
	strict_column_mode_ = src.strict_column_mode_;
	nres_ = src.nres_;
	decoy_tag_ = src.decoy_tag_;
	sequence_ = src.sequence_;
	silent_comments_ = src.silent_comments_;
	silent_energies_ = src.silent_energies_;
	cache_remarks_ = src.cache_remarks_;
	precision_ = src.precision_;
	scoreline_prefix_ = src.scoreline_prefix_;
	residue_numbers_ = src.residue_numbers_;
	chains_ = src.chains_;
	full_model_parameters_ = src.full_model_parameters_;
	return *this;
}

void SilentStruct::fill_pose(
	core::pose::Pose & pose
) const {
	runtime_assert( nres_ != 0 );
	using namespace core::chemical;
	ResidueTypeSetCOP residue_set
		= ChemicalManager::get_instance()->residue_type_set( FA_STANDARD );
	fill_pose( pose, residue_set );
}

/// @brief Fill a Pose with the conformation information in this SilentStruct
/// and the ResidueTypeSetCOP provided by the caller
void SilentStruct::fill_pose(
	core::pose::Pose & pose,
	core::chemical::ResidueTypeSetCOP rts
) const {
	fill_pose( pose, *rts );
}

/// @brief Fill a Pose with the conformation information in this SilentStruct
/// and the ResidueTypeSet provided by the caller. This is a virtual method
/// which must be implemented by classes derived from SilentStruct.
void SilentStruct::fill_pose(
	core::pose::Pose &,
	core::chemical::ResidueTypeSet const &
) const {
	tr.Error << "SilentStruct::fill_pose method stubbed out!" << std::endl;
}

void SilentStruct::fill_struct( core::pose::Pose const & pose,
	std::string tag ) {
	decoy_tag( tag );
	if ( tag == "empty_tag" ) set_tag_from_pose( pose );

	tr.Trace << "get energies from pose..." << std::endl;
	energies_from_pose( pose );

	sequence( pose.annotated_sequence( true /* show-all-variants */ ) );

	extract_writeable_cacheable_data( pose );

}

void SilentStruct::extract_writeable_cacheable_data( core::pose::Pose const& pose ){
	using namespace basic::datacache;
	using namespace pose::datacache;

	// Pull out WriteableCacheable datacache items and add them as comments
	BasicDataCache const& cache = pose.data();
	if ( ! cache.has( CacheableDataType::WRITEABLE_DATA ) ) return;

	using namespace basic::datacache;
	typedef std::map< std::string, std::set< WriteableCacheableDataOP > > DataMap;
	DataMap const& map = cache.get< WriteableCacheableMap >( CacheableDataType::WRITEABLE_DATA ).map();

	for ( DataMap::const_iterator datamap_it = map.begin(), end = map.end();
			datamap_it != end; ++datamap_it ) {
		std::set< WriteableCacheableDataOP > const& dataset = datamap_it->second;

		for ( std::set< WriteableCacheableDataOP >::const_iterator set_it = dataset.begin(), set_end = dataset.end();
				set_it != set_end; ++set_it ) {
			std::stringstream ss;

			ss << "CACHEABLE_DATA ";
			(*set_it)->write( ss );

			add_comment( "CACHEABLE_DATA", ss.str() );
		};
	}
}

void SilentStruct::finish_pose(
	core::pose::Pose & pose
) const
{
	using namespace basic::options;
	using namespace basic::options::OptionKeys;

	pose.pdb_info( core::pose::PDBInfoOP( new core::pose::PDBInfo(pose) ) );

	if ( option[ in::file::keep_input_scores ]() ) {
		tr.Debug << "keep input scores... call energies into pose " << std::endl;
		energies_into_pose( pose );
	}

	residue_numbers_into_pose( pose );
	full_model_info_into_pose( pose );

	basic::datacache::BasicDataCache& cache = pose.data();

	if ( !cache_remarks_.empty() &&
			!cache.has( core::pose::datacache::CacheableDataType::WRITEABLE_DATA ) ) {
		using namespace basic::datacache;
		cache.set( core::pose::datacache::CacheableDataType::WRITEABLE_DATA,
			DataCache_CacheableData::DataOP( new basic::datacache::WriteableCacheableMap() ) );
	}

	for ( utility::vector1< std::string >::const_iterator comment_it = cache_remarks_.begin();
			comment_it != cache_remarks_.end(); ++comment_it ) {
		using namespace basic::datacache;
		using namespace pose::datacache;

		std::stringstream comment_stream( *comment_it );

		std::string data_type;
		comment_stream >> data_type;

		WriteableCacheableDataOP data = WriteableCacheableDataFactory::get_instance()->new_data_instance( data_type, comment_stream );

		WriteableCacheableMap& map = cache.get< WriteableCacheableMap >( CacheableDataType::WRITEABLE_DATA );

		map[ data_type ].insert( data );
	}

}

bool
SilentStruct::read_sequence( std::string const & line ) {
	tr.Debug << "reading sequence from " << line << std::endl;
	std::istringstream line_stream( line );

	std::string tag;
	std::string temp_seq;
	line_stream >> tag >> temp_seq;
	if ( line_stream.fail() || tag != "SEQUENCE:" ) {
		tr.Error << "bad format in sequence line of silent file" << std::endl;
		tr.Error << "line = " << line << std::endl;
		tr.Error << "tag = " << tag << std::endl;
		return false;
	}
	sequence( temp_seq );
	return true;
}

void SilentStruct::read_score_headers( std::string const & line, utility::vector1< std::string > & energy_names, SilentFileData & container ) {
	// second line is a list of score names
	std::istringstream score_line_stream( line );
	tr.Debug << "reading score names from " << line << std::endl;
	std::string tag;
	score_line_stream >> tag; // SCORE:
	if ( score_line_stream.fail() || tag != "SCORE:" ) {
		tr.Error << "bad format in second line of silent file" << std::endl;
		tr.Error << "tag = "  << tag << std::endl;
		tr.Error << "line = " << line << std::endl;
	}

	score_line_stream >> tag; // first score name
	while ( ! score_line_stream.fail() ) {
		energy_names.push_back( tag );
		score_line_stream >> tag; // try to get next score name
	}

	EnergyNamesOP enames( new EnergyNames() );
	SimpleSequenceDataOP seqdata( new SimpleSequenceData() );

	enames ->energy_names( energy_names );
	seqdata->set_sequence( sequence()    );

	container.set_shared_silent_data( energynames       , enames  );
	container.set_shared_silent_data( simplesequencedata, seqdata );

}

void
SilentStruct::print_header( std::ostream & out ) const {
	out << "SEQUENCE: ";
	if ( full_model_parameters_ != 0 ) {
		out << full_model_parameters_->full_sequence(); // better representative of full modeling problem
	} else {
		out << one_letter_sequence();
	}
	out << std::endl;
	print_score_header( out );
}

void SilentStruct::print_score_header( std::ostream & out ) const {
	out << scoreline_prefix();
	using utility::vector1;

	using namespace ObjexxFCL::format;
	typedef vector1< SilentEnergy >::const_iterator iter;
	for ( iter it = silent_energies_.begin(), end = silent_energies_.end();
			it != end; ++it
			) {
		out << " " << A( it->width(), it->name() );
	}

	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	/// print out a column for the user tag if specified.
	if ( option[ out::user_tag ].user() ) {
		std::string const tag = option[ out::user_tag ]();
		int width = std::max( 11, static_cast< int > (tag.size()) );
		out << ' ' << A( width, "user_tag" );
	}

	int width = decoy_tag().size();
	if ( width < 11 ) width = 11; //size of "description"
	out << ' ' << A( width, "description" ) << std::endl;
}

void
SilentStruct::print_scores( std::ostream & out ) const {
	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	using namespace ObjexxFCL;
	using namespace ObjexxFCL::format;
	//int precision = 3; // number of digits after decimal place

	out << scoreline_prefix();

	typedef utility::vector1< SilentEnergy >::const_iterator iter;
	for ( iter it = silent_energies_.begin(), end = silent_energies_.end();
			it != end; ++it
			) {
		if ( it->string_value() == "" ) {
			core::Real weight = 1.0;
			if ( option[ out::file::weight_silent_scores ]() ) { //default true
				weight = it->weight();
			}
			out << " " << F( it->width(), precision(), it->value() * weight );
		} else {
			out << " " << std::setw( it->width() ) << it->string_value(); //  << " ";
		}
	}

	/// print out a column for the user tag if specified.
	if ( option[ out::user_tag ].user() ) {
		std::string const tag = option[ out::user_tag ]();
		int width = std::min( 11, static_cast< int > (tag.size()) );
		out << ' ' << A( width, tag );
	}

	int width = decoy_tag().size();
	if ( width < 11 ) width = 11; // size of "description"
	out << ' ' << A( width, decoy_tag() )  << "\n";
	print_comments( out );
}

void
SilentStruct::print_comments( std::ostream & out ) const {
	using std::map;
	using std::string;
	string const remark( "REMARK" );

	typedef map< string, string >::const_iterator c_iter;
	for ( c_iter it = silent_comments_.begin(), end = silent_comments_.end();
			it != end; ++it
			) {
		out << remark << ' ' << it->first << ' ' << it->second << std::endl;
	}

	for ( utility::vector1< std::string >::const_iterator it = cache_remarks_.begin();
			it != cache_remarks_.end(); ++it ) {
		out << remark << ' ' << *it << std::endl;
	}

} // print_comments

bool SilentEnergy_sort_by_name( const SilentEnergy &score_energy1, const SilentEnergy &score_energy2 ){
	// these two if statements ensure that score always sorts to the beginning
	if ( score_energy1.name() == "score" ) return true;
	if ( score_energy2.name() == "score" ) return false;
	// if above not met, compare strings in ascii
	return score_energy1.name() < score_energy2.name();
}

void SilentStruct::sort_silent_scores( ){
	std::sort( silent_energies_.begin(), silent_energies_.end(), SilentEnergy_sort_by_name);
}

bool SilentStruct::has_energy( std::string const & scorename ) const {
	for ( utility::vector1< SilentEnergy >::const_iterator
			it = silent_energies_.begin(), end = silent_energies_.end();
			it != end; ++it
			) {
		if ( it->name() == scorename ) {
			return true;
		}
	}
	return false;
}

void
SilentStruct::add_energy( std::string const & scorename, Real value, Real weight ) {
	// check if we already have a SilentEnergy with this scorename
	bool replace( false );
	typedef utility::vector1< SilentEnergy >::iterator iter;
	for ( iter it = silent_energies_.begin(), end = silent_energies_.end();
			it != end; ++it
			) {
		if ( it->name() == scorename ) {
			it->value( value );
			replace = true;
		}
	} // for (silent_energies_)

	// add this energy if we haven't added it already
	if ( !replace ) {
		int width = std::max( 10, (int) scorename.length() + 3 );
		SilentEnergy new_se( scorename, value, weight, width );
		silent_energies_.push_back( new_se );
	} // if (!replace)
} // add_energy

void
SilentStruct::add_string_value(
	std::string const & scorename, std::string const & value
) {
	// check if we already have a SilentEnergy with this scorename
	bool replace( false );
	typedef utility::vector1< SilentEnergy >::iterator iter;
	for ( iter it = silent_energies_.begin(), end = silent_energies_.end();
			it != end; ++it
			) {
		if ( it->name() == scorename ) {
			it->value( value );
			replace = true;
		}
	} // for (silent_energies_)

	// add this energy if we haven't added it already
	if ( !replace ) {
		int width = std::max( 10, (int) scorename.length() + 3 );
		SilentEnergy new_se( scorename, value, width );
		silent_energies_.push_back( new_se );
	}
} // add_string_value

core::Real SilentStruct::get_energy( std::string const & scorename ) const {
	if ( has_energy( scorename ) ) {
		return get_silent_energy( scorename ).value();
	} else {
		return 0.0;
	}
}

std::string const & SilentStruct::get_string_value(
	std::string const & scorename
) const {
	static const std::string empty_string("");
	if ( has_energy( scorename ) ) {
		return get_silent_energy( scorename ).string_value();
	} else {
		return empty_string;
	}
}


SilentEnergy const & SilentStruct::get_silent_energy(
	std::string const & scorename
) const {
	debug_assert( has_energy( scorename ) );
	using utility::vector1;
	for ( vector1< SilentEnergy >::const_iterator it = silent_energies_.begin(),
			end = silent_energies_.end();
			it != end; ++it ) {
		if ( it->name() == scorename ) return *it;
	}
	return (*silent_energies_.end()); // keep compiler happy
}


void SilentStruct::set_valid_energies( utility::vector1< std::string > valid ) {
	using utility::vector1;

	vector1< SilentEnergy > new_energies;
	for ( vector1< std::string >::const_iterator it = valid.begin(),
			end = valid.end();
			it != end; ++it
			) {
		SilentEnergy ener;
		if ( *it == "description" ) continue; // hack!
		if ( has_energy( *it ) ) {
			ener = get_silent_energy( *it );
		}
		ener.name( *it );
		new_energies.push_back( ener );
	}
	silent_energies( new_energies );
} // set_valid_energies

///////////////////////////////////////////////////////////////////////
void
SilentStruct::copy_scores( const SilentStruct & src_ss ) {
	typedef utility::vector1< SilentEnergy >::const_iterator const_iter;
	typedef utility::vector1< SilentEnergy >::iterator iter;

	for ( const_iter it = src_ss.silent_energies_.begin(),
			end = src_ss.silent_energies_.end();
			it != end; ++it
			) {
		bool replace( false );

		//Check if score column already present.
		for ( iter it2 = silent_energies_.begin(), end = silent_energies_.end();
				it2 != end; ++it2
				) {
			if ( it2->name() == it->name() ) {
				it2->value( it->value() );
				replace = true;
			}
		} // for it2

		if ( !replace ) silent_energies_.push_back( *it );
	} // for it
} // copy_scores

///////////////////////////////////////////////////////////////////////
void SilentStruct::add_comment( std::string name, std::string value ) {
	if ( name == "CACHEABLE_DATA" ) {
		cache_remarks_.push_back( value );
	} else {
		if ( silent_comments_.find( name ) != silent_comments_.end() ) {
			tr.Debug << "SilentStruct::add_comment is overwriting comment with type "
				<< name << std::endl;
		}
		silent_comments_.insert( std::make_pair( name, value ) );
	}
}

void SilentStruct::comment_from_line( std::string const & line ) {
	std::istringstream line_stream( line );
	std::string dummy, key, val, remark_tag;
	line_stream >> remark_tag;
	line_stream >> key;
	line_stream >> val;
	if ( line_stream.fail() ) {
		tr.Error << "[ERROR] reading comment from line: " << line << std::endl;
		return;
	}
	if ( val == "SILENTFILE" ) {
		tr.Debug << "ignoring silent struct type specifier when reading comments: " << line << std::endl;
		return;
	}
	line_stream >> dummy;
	while ( line_stream.good() ) {
		val += " ";
		val += dummy;
		line_stream >> dummy;
	}

	add_comment( key, val );
}

std::map< std::string, std::string > SilentStruct::get_all_comments() const {
	return silent_comments_;
}

bool SilentStruct::has_comment( std::string const & name ) const {
	return ( silent_comments_.find( name ) != silent_comments_.end() );
}

std::string SilentStruct::get_comment( std::string const & name ) const {
	std::map< std::string, std::string >::const_iterator entry
		= silent_comments_.find( name );
	std::string comment( "" );
	if ( entry != silent_comments_.end() ) {
		comment = entry->second;
	}

	return comment;
}

void SilentStruct::erase_comment( std::string const & name ) {
	silent_comments_.erase( name );
}

void SilentStruct::clear_comments() {
	silent_comments_.clear();
}


void SilentStruct::parse_energies(
	std::istream & input,
	utility::vector1< std::string > const & energy_names
) {
	std::string tag;
	utility::vector1< std::string >::const_iterator energy_iter;
	Size input_count = 0;
	Size const energy_names_count( energy_names.size() );

	using namespace ObjexxFCL;
	for ( energy_iter  = energy_names.begin();
			energy_iter != energy_names.end();
			++energy_iter
			) {
		input >> tag;
		if ( is_float( tag ) ) {
			Real score_val = static_cast< Real > ( float_of( tag ) );
			add_energy( *energy_iter, score_val );
		} else if ( *energy_iter == "description" ) {
			decoy_tag( tag );
		} else {
			add_string_value( *energy_iter, tag );
		}
		++input_count;
	} // for ( energy_iter ... )

	if ( energy_names_count != input_count ) {
		tr.Warning << "Warning: I have " << energy_names_count
			<< " energy names but I have " << input_count
			<< " energy values." << std::endl;
	}
} // parse_energies

void SilentStruct::update_score() {
	runtime_assert( silent_energies_.begin()->name() == "score" );

	// set the "score" term to its appropriate value
	typedef utility::vector1< SilentEnergy >::iterator energy_it;

	Real score( 0.0 );
	for ( energy_it it = silent_energies_.begin(), end = silent_energies_.end();
			it != end; ++it
			) {
		score += it->weight() * it->value();
	}

	silent_energies_.begin()->value( score );
} // update_score()

void SilentStruct::energies_from_pose( core::pose::Pose const & pose ) {
	using namespace core::pose::datacache;

	core::scoring::EnergyMap const emap = pose.energies().total_energies();
	core::scoring::EnergyMap const wts  = pose.energies().weights();

	clear_energies();

	// "score" is the total of weighted scores. always the first element in
	// silent_energies_
	SilentEnergy score_energy( "score", 0.0, 1.0, 8 );
	silent_energies_.push_back( score_energy );
	core::scoring::EnergyMap::const_iterator emap_iter, wts_iter;
	for ( emap_iter = emap.begin(), wts_iter = wts.begin();
			emap_iter != emap.end() && wts_iter!= wts.end();
			++emap_iter && ++wts_iter
			) {

		// only grab scores that have non-zero weights.
		if ( *wts_iter != 0.0 ) {
			core::scoring::ScoreType sc_type
				= core::scoring::ScoreType( emap_iter - emap.begin() + 1 );
			std::string name = core::scoring::name_from_score_type( sc_type );

			int width = std::max( 10, (int) name.length() + 3 );
			SilentEnergy new_se( name, *emap_iter, *wts_iter, width );
			silent_energies_.push_back( new_se );
		} // if ( *wts_iter != 0.0 )
	} // for ( emap_iter ...)

	// set the "score" term to its appropriate value
	update_score();

	// get arbitrary floating point scores from the map stored in the Pose data cache
	// these can be accessed through setPoseExtraScore and getPoseExtraScore in core/pose/util.hh
	if ( pose.data().has( CacheableDataType::ARBITRARY_FLOAT_DATA ) ) {
		basic::datacache::CacheableStringFloatMapCOP data
			= utility::pointer::dynamic_pointer_cast< basic::datacache::CacheableStringFloatMap const >
			( pose.data().get_const_ptr(CacheableDataType::ARBITRARY_FLOAT_DATA) );

		using std::map;
		using std::string;
		for ( map< string, float >::const_iterator iter = data->map().begin(),
				end = data->map().end(); iter != end; ++iter
				) {
			// skip score entry, as it gets confusing
			if ( iter->first == "score" ) continue;
			SilentEnergy new_se(
				iter->first, iter->second, 1.0,
				static_cast< int > (iter->first.size() + 3) /* width */
			);
			tr.Trace << " score energy from pose-cache: " << iter->first << " " << iter->second << std::endl;
			silent_energies_.push_back( new_se );
		}
	} //  if ( pose.data().has( CacheableDataType::ARBITRARY_FLOAT_DATA ) ) )

	// add comments from the Pose
	using std::map;
	using std::string;
	map< string, string > comments = core::pose::get_all_comments( pose );
	for ( map< string, string >::const_iterator it = comments.begin(),
			end = comments.end(); it != end; ++it
			) {
		add_comment( it->first, it->second );
	}

	// add score_line_strings from Pose
	map< string, string > score_line_strings(
		core::pose::get_all_score_line_strings( pose )
	);
	for ( map< string, string >::const_iterator it = score_line_strings.begin(),
			end = score_line_strings.end();
			it != end; ++it
			) {
		add_string_value( it->first, it->second );
	}

} // void energies_from_pose( core::pose::Pose const & pose )

void SilentStruct::energies_into_pose( core::pose::Pose & pose ) const {
	using namespace basic::options;
	using namespace basic::options::OptionKeys;
	using namespace core::pose::datacache;
	using namespace core::scoring;
	using std::string;
	using utility::vector1;
	// make sure that the pose has ARBITRARY_FLOAT_DATA in the DataCache
	if ( !pose.data().has( ( CacheableDataType::ARBITRARY_FLOAT_DATA ) ) ) {
		using namespace basic::datacache;
		pose.data().set(
			CacheableDataType::ARBITRARY_FLOAT_DATA,
			DataCache_CacheableData::DataOP( new basic::datacache::CacheableStringFloatMap() )
		);
	}

	basic::datacache::CacheableStringFloatMapOP data
		= utility::pointer::dynamic_pointer_cast< basic::datacache::CacheableStringFloatMap >
		( pose.data().get_ptr(CacheableDataType::ARBITRARY_FLOAT_DATA) );

	runtime_assert( data.get() != NULL );

	EnergyMap weights;
	EnergyMap & emap( pose.energies().total_energies() );

	string const input_score_prefix( option[ in::file::silent_score_prefix ]() );

	std::set< string > wanted_scores;
	if ( option[ in::file::silent_scores_wanted ].user() ) {
		vector1< string > wanted = option[ in::file::silent_scores_wanted ]();
		for ( vector1< string >::iterator it = wanted.begin(), end = wanted.end();
				it != end; ++it
				) {
			wanted_scores.insert( *it );
		}
	}

	typedef vector1< SilentEnergy > elist;
	elist es = energies();
	for ( elist::const_iterator it = es.begin(), end = es.end();
			it != end; ++it
			) {
		// only keep this score if we want it.
		if ( !wanted_scores.empty() && //size() > 0 &&
				wanted_scores.find(it->name()) == wanted_scores.end()
				) continue;

		// logic is:
		// - if it->name() points to a ScoreType, put the value into the EnergyMap
		// - otherwise, add the value to the Pose DataCache ARBITRARY_FLOAT_DATA
		string const proper_name( input_score_prefix + it->name() );
		if ( ScoreTypeManager::is_score_type( proper_name ) )  {
			ScoreType sc_type( ScoreTypeManager::score_type_from_name( proper_name ) );
			emap   [ sc_type ] = it->value();
			weights[ sc_type ] = it->weight();
		} else if ( it->string_value() != "" ) {
			tr.Trace << "add score line string : " << proper_name << " " << it->string_value() << std::endl;
			core::pose::add_score_line_string(
				pose, proper_name, it->string_value()
			);
		} else {
			tr.Trace << "add score : " << proper_name << " " << it->value() << std::endl;
			data->map()[ proper_name ] = it->value();
		}
	} // for silent_energies

	pose.energies().weights( weights );
	pose.data().set( CacheableDataType::ARBITRARY_FLOAT_DATA, data );

	using std::map;
	using std::string;
	map< string, string > comments = get_all_comments();
	for ( map< string, string >::const_iterator it = comments.begin(),
			end = comments.end(); it != end; ++it
			) {
		// only keep this score if we want it.
		if ( !wanted_scores.empty() && //size() > 0 &&
				wanted_scores.find(it->first) == wanted_scores.end()
				) continue;
		string const proper_name( input_score_prefix + it->first );
		core::pose::add_comment( pose, proper_name, it->second );
	}
} // energies_into_pose

EnergyNames SilentStruct::energy_names() const {
	utility::vector1< SilentEnergy > se = energies();

	utility::vector1< std::string > names;
	for ( utility::vector1< SilentEnergy >::const_iterator it = se.begin(),
			end = se.end(); it != end; ++it
			) {
		names.push_back( it->name() );
	}
	EnergyNames enames;
	enames.energy_names( names );
	return enames;
}


void SilentStruct::rename_energies() {
	using namespace std;

	static map< string, string > scorename_conversions;
	static bool init( false );
	if ( !init ) {
		scorename_conversions["cb"] = "cbeta";
		scorename_conversions["hs"] = "hs_pair";
		scorename_conversions["ss"] = "ss_pair";
	}

	// iterate over the silent-file energies, convert them when appropriate.
	map< string, string >::const_iterator conv_it,
		conv_end( scorename_conversions.end() );
	for ( utility::vector1< SilentEnergy >::iterator it = silent_energies_.begin(),
			end = silent_energies_.end(); it != end; ++it
			) {
		conv_it = scorename_conversions.find( it->name() );
		if ( conv_it != conv_end ) {
			it->name( conv_it->second );
		}
	}
} // rename_energies

void SilentStruct::set_tag_from_pose(
	const core::pose::Pose & pose
) {
	using namespace core::pose::datacache;
	std::string tag( "empty_tag" );
	if ( pose.data().has( CacheableDataType::JOBDIST_OUTPUT_TAG ) ) {
		tag =
			static_cast< basic::datacache::CacheableString const & >
			( pose.data().get( CacheableDataType::JOBDIST_OUTPUT_TAG ) ).str();
	}
	decoy_tag(tag);
}

//void
//SilentStruct::print_sequence( bool print_seq ) {
// print_sequence_ = print_seq;
//}
//
//bool SilentStruct::print_sequence() const {
// return print_sequence_;
//}

void
SilentStruct::precision( core::Size precision ) {
	precision_ = precision;
}

core::Size SilentStruct::precision() const {
	return precision_;
}

void
SilentStruct::scoreline_prefix( std::string const & prefix )
{
	scoreline_prefix_ = prefix;
}

std::string SilentStruct::scoreline_prefix() const {
	return scoreline_prefix_;
}

std::string
SilentStruct::one_letter_sequence() const {
	// if ( full_model_parameters_ != 0 ) return full_model_parameters_->full_sequence();
	return sequence().one_letter_sequence();
}

/// @ brief helper to detect fullatom input
/// @ detail
//* removing dependendence from input flag -in:file:Fullatom which made no sense, since reading inconsistent
//* will break anyhow... moreover the test was wrong before: GLY and ALA have 7 or 10 atoms respectively.
// strategy to detect fullatom-ness now:
//   look only at unpatched residues -- check for protein...
//      (no modification in annotated sequence )
//
//       centroid: GLY 6 atoms, all others 7 atoms
//          centroid_HA patch: +1 (without showing up as "patched" residue in annotated sequence... )
//    -> natoms>=9 on unpatched proteins residue should indicate fullatom
//  if it remains unclear --> use the cmd-line flag to determine fullatom-ness...
//
//* additional note (rhiju) -- checked for DNA, RNA, some patches, which all go with fullatom -- this is getting byzantine, so
//  perhaps we should set FA_STANDARD (fullatom) as default residue_type_set unless some kind of CENTROID tags is specified.
//
void SilentStruct::detect_fullatom( core::Size pos, core::Size natoms, bool& fullatom, bool& well_defined ) {

	if ( sequence().aa( pos ) > core::chemical::num_canonical_aas ) {
		if ( sequence().aa( pos ) >= core::chemical::aa_vrt  ) return;
		well_defined = true;
		fullatom = true;
		tr.Debug << "found RNA or DNA " << sequence().one_letter( pos )  << std::endl;
		return;
	}
	if ( sequence().is_patched( pos ) ) {
		utility::vector1< std::string > const patches = utility::string_split(  sequence().patch_str( pos ), ':' );
		if ( patches.has_value( "Virtual_Protein_SideChain" ) ||
				patches.has_value( "C_methylamidated" ) ||
				patches.has_value( "N_acetylated" ) ) {
			well_defined = true;
			fullatom = true;
		}
		return;
	}
	// if ( well_defined ) return; // no more checks necessary
	//okay an unpatched regular aminoacid
	if ( natoms <= 6 ) {
		well_defined=true;
		fullatom=false;
		tr.Debug << "found less than 7 atoms on " << sequence().one_letter( pos ) << " switch to centroid mode" << std::endl;
	} else if ( natoms>=9 ) {
		tr.Debug << "found " << natoms << " atoms (more than 8) on " << sequence().one_letter( pos ) << " switch to fullatom mode" << std::endl;
		well_defined=true;
		fullatom=true;
	}
}


///////////////////////////////////////////////////////////////////////////
void
SilentStruct::print_parent_remarks( std::ostream & out ) const {
	using std::map;
	using std::string;
	string const remark( "PARENT REMARK" );

	typedef map< string, string >::const_iterator c_iter;
	for ( c_iter it = parent_remarks_map_.begin(), end = parent_remarks_map_.end(); it != end; ++it ) {
		out << remark << ' ' << it->first << ' ' << it->second << std::endl;
	}
}

///////////////////////////////////////////////////////////////////////////

std::string
SilentStruct::get_parent_remark( std::string const & name ) const {

	if ( parent_remarks_map_.count(name)==0 ) utility_exit_with_message( "The key (" + name +") doesn't exist in the parent_remarks_map_!");

	std::map< std::string, std::string >::const_iterator entry= parent_remarks_map_.find( name );

	if ( entry == parent_remarks_map_.end() ) {
		utility_exit_with_message( "entry == parent_remarks_map_.end()  for the the key (" + name +").");
	}

	std::string const parent_remark = entry->second;

	return parent_remark;
}

///////////////////////////////////////////////////////////////////////////
bool SilentStruct::has_parent_remark( std::string const & name ) const {
	return ( parent_remarks_map_.find( name ) != parent_remarks_map_.end() );
}

///////////////////////////////////////////////////////////////////////////

void
SilentStruct::add_parent_remark( std::string const & name, std::string const & value ){

	// if(parent_remarks_map_.count(name)>0) utility_exit_with_message( "The key (" + name +") already exist in the parent_remarks_map_!");

	if ( parent_remarks_map_.count(name)>0 ) {
		tr << "The key (" + name +") already exist in the parent_remarks_map_!";
	} else {
		parent_remarks_map_.insert( std::make_pair( name, value ) );
	}

}
///////////////////////////////////////////////////////////////////////////

void
SilentStruct::get_parent_remark_from_line( std::string const & line ){

	std::istringstream line_stream( line );

	std::string key, val, remark_tag;
	line_stream >> remark_tag;
	line_stream >> key;
	line_stream >> val;

	if ( line_stream.fail() )  utility_exit_with_message("[ERROR] reading REMARK from line: " + line );

	if ( remark_tag!="REMARK" ) utility_exit_with_message(remark_tag!="REMARK"); //Extra precaution!

	std::string dummy;

	line_stream >> dummy;
	while ( line_stream.good() ) {
		std::cout << "Extra characters in REMARK LINE!" << std::endl;
		std::cout << "remark_tag= " << remark_tag << std::endl;
		std::cout << "key= " << key << std::endl;
		std::cout << "val= " << val << std::endl;
		std::cout << "dummy= " << dummy << std::endl;
		utility_exit_with_message("Extra characters in REMARK LINE!");
	}

	add_parent_remark( key, val );
}

///////////////////////////////////////////////////////////////////////////
void
SilentStruct::fill_struct_with_residue_numbers( pose::Pose const & pose ){

	pose::PDBInfoCOP pdb_info = pose.pdb_info();

	if ( !pdb_info ) return;

	utility::vector1< Size > residue_numbers;
	utility::vector1< char > chains;
	bool residue_numbering_is_interesting( false );
	for ( core::uint i = 1; i <= pose.total_residue(); ++i ) {
		residue_numbers.push_back( pdb_info->number( i ) );
		chains.push_back( pdb_info->chain( i ) );
		if ( pdb_info->number( i ) != static_cast<int>(i) ||
				( pdb_info->chain( i ) != ' ' &&
				pdb_info->chain( i ) != 'A' ) ) {
			residue_numbering_is_interesting = true;
		}
	}

	utility::vector1< std::string > seg_ids;
	bool segment_IDs_are_interesting( false );
	for ( core::uint i = 1; i <= pose.total_residue(); ++i ) {
		seg_ids.push_back( pdb_info->segmentID( i ) );
		if ( pdb_info->segmentID( i ) != "    " ) {
			segment_IDs_are_interesting = true;
		}
	}

	if ( residue_numbering_is_interesting ) { // leave residue_numbers_ blank.

		set_residue_numbers( residue_numbers );
		set_chains( chains );
	}

	if ( segment_IDs_are_interesting ) {
		set_segment_IDs( seg_ids );
	}

}

///////////////////////////////////////////////////////////////////////////
void
SilentStruct::fill_other_struct_list( pose::Pose const & pose ){
	using namespace core::pose::full_model_info;
	if ( !full_model_info_defined( pose ) ) return;

	utility::vector1< Size > dummy;
	// if full_model_parameters is 'boring' don't save it.
	FullModelParameters full_model_parameters_standard( pose, dummy );
	if ( full_model_parameters_standard != *const_full_model_info( pose ).full_model_parameters() ) {
		set_full_model_parameters( const_full_model_info( pose ).full_model_parameters() );
	}

	utility::vector1< core::pose::PoseOP > const & other_pose_list = core::pose::full_model_info::const_full_model_info( pose ).other_pose_list();
	for ( Size n = 1; n <= other_pose_list.size(); n++ ) {
		SilentStructOP other_struct =  this->clone();
		other_struct->scoreline_prefix( "OTHER:" ); // prevents confusion when grepping file for "SCORE:"
		other_struct->fill_struct( *other_pose_list[ n ], decoy_tag_ /*use same tag*/ );
		other_struct->add_comment( "OTHER_POSE", ObjexxFCL::string_of( n ) ); // will be used as consistency check when read from disk.
		other_struct->fill_struct_with_submotif_info_list( *other_pose_list[ n ] );
		add_other_struct( other_struct );
	}
}


///////////////////////////////////////////////////////////////////////////
void
SilentStruct::residue_numbers_into_pose( pose::Pose & pose ) const{

	if ( residue_numbers_.size() == 0 ) return;

	if ( pose.total_residue() != residue_numbers_.size() ) {
		std::cout << "Number of residues in pose: " << pose.total_residue() << std::endl;
		std::cout << "Number of residues in silent_struct residue_numbers: " << residue_numbers_.size() << std::endl;
		utility_exit_with_message( "Problem with residue_numbers in silent_struct!" );
	}

	pose::PDBInfoOP pdb_info = pose.pdb_info();
	pdb_info->set_numbering( residue_numbers_ );

	runtime_assert( residue_numbers_.size() == chains_.size() );
	pdb_info->set_chains( chains_ );

	if ( segment_IDs_.size() == 0 ) return;

	if ( pose.total_residue() != segment_IDs_.size() ) {
		std::cout << "Number of residues in pose: " << pose.total_residue() << std::endl;
		std::cout << "Number of residues in silent_struct segment_IDs: " << segment_IDs_.size() << std::endl;
		utility_exit_with_message( "Problem with segment_IDs_ in silent_struct!" );
	}

	pdb_info->set_segment_ids( segment_IDs_ );
}

///////////////////////////////////////////////////////////////////////////
void
SilentStruct::full_model_info_into_pose( pose::Pose & pose ) const {
	using namespace core::pose::full_model_info;
	if ( full_model_parameters_ == 0 )  return;

	FullModelInfoOP full_model_info( new FullModelInfo( full_model_parameters_ ) );
	if ( residue_numbers_.size() > 0 ) {
		full_model_info->set_res_list( full_model_parameters_->conventional_to_full( std::make_pair( residue_numbers_, chains_ ) ) );
	} else {
		utility::vector1< Size > res_list;
		for ( Size n = 1; n <= pose.total_residue(); n++ ) res_list.push_back( n );
		full_model_info->set_res_list( res_list );
	}

	// calebgeniesse: setup submotif_info_list in full_model_info
	full_model_info->add_submotif_info( submotif_info_list_ );

	set_full_model_info( pose, full_model_info );
	update_constraint_set_from_full_model_info( pose );
}

///////////////////////////////////////////////////////////////////////////
void
SilentStruct::print_residue_numbers( std::ostream & out ) const {

	if ( residue_numbers_.size() == 0 ) return;

	if ( nres_ != residue_numbers_.size() ) {

		// This happens with symmetric poses! Could make
		// symmetric poses compatible with RES_NUM but
		// would need to put in asserts to ensure numbering
		// matched up across all symmetry mates.
		return;

		//  std::cout << "Silent_struct has nres_: " << nres_  << std::endl;
		//  std::cout << "but number of residues in silent_struct residue_numbers: " << residue_numbers_.size() << std::endl;
		//  std::cout << "Residue_numbers: " << make_tag_with_dashes( residue_numbers_ ) << std::endl;
		//  utility_exit_with_message( "Problem with residue_numbers in silent_struct!" );
	}

	out << "RES_NUM " << make_tag_with_dashes( residue_numbers_, chains_ ) <<  " " << decoy_tag() << std::endl;

	if ( segment_IDs_.size() == 0 ) return;

	out << "SEGMENT_IDS " << make_segtag_with_dashes( residue_numbers_, segment_IDs_ ) <<  " " << decoy_tag() << std::endl;
}

void
SilentStruct::print_submotif_info( std::ostream & out ) const {
	using namespace core::pose::full_model_info;
	if ( submotif_info_list_.size() == 0 ) return;

	utility::vector1< SubMotifInfoOP >::iterator itr;
	for ( itr = submotif_info_list_.begin();
			itr != submotif_info_list_.end();
			++itr ) {
		out << *itr << " " << decoy_tag() << std::endl;
	}

}

///////////////////////////////////////////////////////////////////////////
void
SilentStruct::figure_out_residue_numbers_from_line( std::istream & line_stream ) {
	utility::vector1< Size > residue_numbers;
	utility::vector1< char > chains;
	std::string resnum_string;
	line_stream >> resnum_string; // the tag (RES_NUM)
	line_stream >> resnum_string;
	while ( !line_stream.fail() ) {
		bool string_ok( false );
		std::pair< std::vector< int >, std::vector< char > > resnum_and_chain = utility::get_resnum_and_chain( resnum_string, string_ok );
		std::vector< int >  const & resnums      = resnum_and_chain.first;
		std::vector< char > const & chainchars  = resnum_and_chain.second;
		if ( string_ok ) {
			for ( Size i = 0; i < resnums.size(); i++ ) residue_numbers.push_back( resnums[i] );
			for ( Size i = 0; i < chainchars.size(); i++ ) chains.push_back( chainchars[i] );
		} else break;
		line_stream >> resnum_string;
	}

	set_residue_numbers( residue_numbers );
	set_chains( chains );
}

void
SilentStruct::figure_out_segment_ids_from_line( std::istream & line_stream ) {
	utility::vector1< Size > residue_numbers;
	utility::vector1< std::string > segment_ids;
	std::string resnum_string;
	line_stream >> resnum_string; // the tag (SEGMENT_ID)
	line_stream >> resnum_string;
	while ( !line_stream.fail() ) {
		bool string_ok( false );
		std::pair< std::vector< int >, std::vector< std::string > > resnum_and_segid = utility::get_resnum_and_segid( resnum_string, string_ok );
		std::vector< int >  const & resnums       = resnum_and_segid.first;
		std::vector< std::string > const & segid  = resnum_and_segid.second;
		if ( string_ok ) {
			for ( Size i = 0; i < resnums.size(); i++ ) residue_numbers.push_back( resnums[i] );
			for ( Size i = 0; i < segid.size(); i++ ) segment_ids.push_back( segid[i] );
		} else break;
		line_stream >> resnum_string;
	}

	if ( residue_numbers_.size() > 0 ) {
		runtime_assert( residue_numbers_ == residue_numbers );
	}
	set_segment_IDs( segment_ids );
}
///////////////////////////////////////////////////////////////////////////
// @details: construct submotif_info from line, add to submotif_info_list_
void
SilentStruct::add_submotif_info_from_line( std::istream & line_stream ) {
	using namespace core::pose::full_model_info;
	if ( full_model_parameters_ == 0 )  return;

	SubMotifInfoOP submotif_info( new SubMotifInfo() );
	line_stream >> submotif_info;
	submotif_info_list_.push_back( submotif_info );
}

///////////////////////////////////////////////////////////////////////////
// @details: save submotif_info_list from pose, in struct
void
SilentStruct::fill_struct_with_submotif_info_list( core::pose::Pose const & pose ) {
	using namespace core::pose::full_model_info;
	if ( !full_model_info_defined( pose ) ) return;

	submotif_info_list_ = const_full_model_info( pose ).submotif_info_list();
}


void
SilentStruct::add_other_struct( SilentStructOP silent_struct ){
	// figure out the right order here.
	Size const new_idx = ObjexxFCL::int_of( silent_struct->get_all_comments().find( "OTHER_POSE" )->second );

	utility::vector1< SilentStructOP >::iterator it, end;
	for ( it = other_struct_list_.begin(), end = other_struct_list_.end();
			it != end; ++it ) {
		Size const list_idx = ObjexxFCL::int_of( (*it)->get_all_comments().find( "OTHER_POSE" )->second );
		if ( new_idx < list_idx ) break; // insert here.
	}
	other_struct_list_.insert( it, silent_struct );
}


} // namespace silent
} // namespace io
} // namespace core
