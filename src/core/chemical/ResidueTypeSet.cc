// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available under license.
// (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
// (c) For more information, see http://www.rosettacommons.org. Questions about this can be
// (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

//////////////////////////////////////////////////////////////////////
/// @file ResidueTypeSet.cc
///
/// @brief
/// ResidueTypeSet class
///
/// @details
/// This class is responsible for iterating through the sets of residue types, including, but not limited to, amino
///  acids, nucleic acids, peptoid residues, and monosaccharides.  It first reads through a file that contains the
///  location of residue types in the database.  At the beginning of that file are the atom types, mm atom types,
///  element sets, and orbital types that will be used.  The sets are all for fa_standard.  If a new type of atom is
///  added for residues, this is where they would be added.  Once it assigns the types, it then reads in extra residue
///  params that are passed through the command line.
/// Then, the class reads in patches.
/// There can be an exponentially large number of possible residue types that can be built from base_residue_types and
///  patch applications -- they are created on the fly when requested by accessor functions like has_name() and
///  get_all_types_with_variants_aa().
/// Later, the class can accept 'custom residue types' (to which patches will not be added).
///
/// @author
/// Phil Bradley
/// Steven Combs - these comments
/// Rocco Moretti - helper functions for more efficient ResidueTypeSet use
/// Rhiju Das - 'on-the-fly' residue type generation; allowing custom residue types; ResidueTypeSetCache.
/////////////////////////////////////////////////////////////////////////


// Rosetta headers
#include <core/chemical/ResidueTypeSet.hh>
#include <core/chemical/ResidueTypeSetCache.hh>
#include <core/chemical/ResidueTypeFinder.hh>
#include <core/chemical/ResidueProperties.hh>
#include <core/chemical/MergeBehaviorManager.hh>
#include <core/chemical/Metapatch.hh>
#include <core/chemical/Patch.hh>
#include <core/chemical/ChemicalManager.hh>
#include <core/chemical/residue_io.hh>
#include <core/chemical/sdf/MolFileIOReader.hh>
#include <core/chemical/adduct_util.hh>
#include <core/chemical/util.hh>
#include <core/chemical/Orbital.hh> /* for copying ResidueType */
#include <core/chemical/ResidueConnection.hh> /* for copying ResidueType */

#include <core/chemical/gasteiger/GasteigerAtomTyper.hh>
#include <core/chemical/mmCIF/mmCIFParser.hh>

// Basic headers
#include <basic/database/open.hh>
#include <basic/options/option.hh>
#include <basic/Tracer.hh>

// Utility headers
#include <utility/file/FileName.hh>
#include <utility/io/izstream.hh>

// C++ headers
#include <fstream>
#include <string>
//#include <sstream>
#include <set>
#include <algorithm>

// option key includes
#include <basic/options/keys/pH.OptionKeys.gen.hh>
#include <basic/options/keys/chemical.OptionKeys.gen.hh>
#include <basic/options/keys/in.OptionKeys.gen.hh>
#include <core/chemical/orbitals/AssignOrbitals.hh>

// Boost Headers
#include <boost/foreach.hpp>

#include <utility/vector1.hh>
#include <utility/file/file_sys_util.hh>

using namespace basic::options;

namespace core {
namespace chemical {

static THREAD_LOCAL basic::Tracer tr( "core.chemical.ResidueTypeSet" );

///////////////////////////////////////////////////////////////////////////////
/// @brief c-tor from directory
ResidueTypeSet::ResidueTypeSet(
	std::string const & name,
	std::string const & directory
) :
	name_( name ),
	database_directory_(directory),
	merge_behavior_manager_( new MergeBehaviorManager( directory ) ),
	cache_( ResidueTypeSetCacheOP( new ResidueTypeSetCache( *this ) ) )
{
	load_shadowed_ids( directory );
}

bool sort_patchop_by_name( PatchOP p, PatchOP q ) {
	return ( p->name() < q->name() );
}

void ResidueTypeSet::init(
	std::vector< std::string > const & extra_res_param_files,  // defaults to empty
	std::vector< std::string > const & extra_patch_files )  // defaults to empty
{
	using namespace basic::options;

	clock_t const time_start( clock() );

	// read ResidueTypes
	{
		std::string const list_filename( database_directory_ + "residue_types.txt" );
		utility::io::izstream data( list_filename.c_str() );
		if ( !data.good() ) {
			utility_exit_with_message( "Unable to open file: " + list_filename + '\n' );
		}
		std::string line, tag;
		while ( getline( data, line ) ) {
			// Skip empty lines and comments.
			if ( line.size() < 1 || line[0] == '#' ) continue;

			// kp don't consider files for protonation versions of the residues if flag pH_mode is not used
			// to make sure even applications that use ResidueTypeSet directly never run into problems
			// AMW: I have to exclude these purely for the Matcher--could a Matcher expert weigh in?
			// At least they all work with mm_params etc. now.
			// This just means that the "enumerate RTs" app should run with pH mode
			bool no_proton_states = false;
			if ( line.size() > 20 ) {
				if ( ( !option[ OptionKeys::pH::pH_mode ].user() ) &&
						( line.substr (14,6) == "proton" ) ) {
					no_proton_states = true;
				}
			}
			if ( no_proton_states ) continue;

			// Skip carbohydrate ResidueTypes unless included with include_sugars flag.
			if ( ( ! option[ OptionKeys::in::include_sugars ] ) &&
					( line.substr( 0, 27 ) == "residue_types/carbohydrates" ) ) {
				continue;
			}

			// Skip lipid ResidueTypes unless included with include_lipids flag.
			if ( ( ! option[ OptionKeys::in::include_lipids ] ) &&
					( line.substr( 0, 20 ) == "residue_types/lipids" ) ) {
				continue;
			}

			//Skip mineral surface ResidueTypes unless included with surface_mode flag.
			if ( (!option[OptionKeys::in::include_surfaces]) &&
					(line.substr(0, 29) == "residue_types/mineral_surface") ) {
				continue;
			}

			// Parse lines.
			std::istringstream l( line );
			l >> tag;
			if ( tag == "ATOM_TYPE_SET" ) {
				l >> tag;
				atom_types_ = ChemicalManager::get_instance()->atom_type_set( tag );
			} else if ( tag == "ELEMENT_SET" ) {
				l >> tag;
				elements_ = ChemicalManager::get_instance()->element_set( tag );
			} else if ( tag == "MM_ATOM_TYPE_SET" ) {
				l >> tag;
				mm_atom_types_ = ChemicalManager::get_instance()->mm_atom_type_set( tag );
			} else if ( tag == "ORBITAL_TYPE_SET" ) {
				l >> tag;
				orbital_types_ = ChemicalManager::get_instance()->orbital_type_set(tag);
			} else {
				std::string const filename( database_directory_ + line );

				ResidueTypeOP rsd_type( read_topology_file(
					filename, atom_types_, elements_, mm_atom_types_, orbital_types_, get_self_weak_ptr() ) );
				if ( option[ OptionKeys::in::file::assign_gasteiger_atom_types ] ) {
					gasteiger::GasteigerAtomTypeSetCOP gasteiger_set(
						ChemicalManager::get_instance()->gasteiger_atom_type_set() );
					gasteiger::assign_gasteiger_atom_types( *rsd_type, gasteiger_set, false );
				}
				base_residue_types_.push_back( rsd_type );
				cache_->add_residue_type( rsd_type );
			}
		}

		BOOST_FOREACH ( std::string filename, extra_res_param_files ) {
			ResidueTypeOP rsd_type( read_topology_file(
				filename, atom_types_, elements_, mm_atom_types_, orbital_types_, get_self_weak_ptr() ) );
			base_residue_types_.push_back( rsd_type );
			cache_->add_residue_type( rsd_type );
		}
	}  // ResidueTypes read

	// now apply patches
	{
		std::string const list_filename( database_directory_+"/patches.txt" );
		utility::io::izstream data( list_filename.c_str() );

		if ( !data.good() ) {
			utility_exit_with_message( "Unable to open patch list file: "+list_filename );
		}

		// Read the command line and avoid applying patches that the user has requested be
		// skipped.  The flag allows the user to specify a patch by its name or by its file.
		// E.g. "SpecialRotamer" or "SpecialRotamer.txt".  Directory names will be ignored if given.
		std::set< std::string > patches_to_avoid;
		if ( option[ OptionKeys::chemical::exclude_patches ].user() ) {
			utility::vector1< std::string > avoidlist =
				option[ OptionKeys::chemical::exclude_patches ];
			for ( Size ii = 1; ii <= avoidlist.size(); ++ii ) {
				utility::file::FileName fname( avoidlist[ ii ] );
				patches_to_avoid.insert( fname.base() );
			}
		}

		utility::vector1< std::string > patch_filenames(extra_patch_files);
		utility::vector1< std::string > metapatch_filenames;
		// Unconditional loading of listed patches is deliberate --
		// if you specified it explicitly, you probably want it to load.
		std::string line;
		while ( getline( data,line) ) {
			if ( line.size() < 1 || line[ 0 ] == '#' ) continue;

			// get rid of any comment lines.
			line = utility::string_split( line, '#' )[ 1 ];
			line = utility::string_split( line, ' ' )[ 1 ];

			// Skip carbohydrate patches unless included with include_sugars flag.
			if ( ( ! option[ OptionKeys::in::include_sugars ] ) &&
					( line.substr( 0, 21 ) == "patches/carbohydrates" ) ) {
				continue;
			}

			// Skip this patch if the "patches_to_avoid" set contains the named patch.
			// AMW: keeping this because "patches_to_avoid" is explicitly asked for
			// on the command line.
			utility::file::FileName fname( line );
			if ( patches_to_avoid.find( fname.base() ) != patches_to_avoid.end() ) {
				tr << "While generating ResidueTypeSet " << name_ <<
					": Skipping patch " << fname.base() << " as requested" << std::endl;
				continue;
			}

			patch_filenames.push_back( database_directory_ + line );
		}

		// kdrew: include list allows patches to be included while being commented out in patches.txt,
		// useful for testing non-canonical patches.
		// Retaining this just because maybe you haven't put your patch in the list yet
		//tr << "include_patches activated? " <<
		//  option[ OptionKeys::chemical::include_patches ].active() << std::endl;
		if ( option[ OptionKeys::chemical::include_patches ].active() ) {
			utility::vector1< std::string > includelist =
				option[ OptionKeys::chemical::include_patches ];
			for ( Size ii = 1; ii <= includelist.size(); ++ii ) {
				utility::file::FileName fname( includelist[ ii ] );
				if ( !utility::file::file_exists( database_directory_ + includelist[ ii ] ) ) {
					tr.Warning << "Could not find: " << database_directory_+includelist[ii]  << std::endl;
					continue;
				}
				patch_filenames.push_back( database_directory_ + includelist[ ii ]);
				tr << "While generating ResidueTypeSet " << name_ <<
					": Including patch " << fname << " as requested" << std::endl;
			}
		}

		//fpd  if missing density is to be read correctly, we will have to also load the terminal truncation variants
		if ( option[ OptionKeys::in::missing_density_to_jump ]()
				|| option[ OptionKeys::in::use_truncated_termini ]() ) {
			if ( std::find( patch_filenames.begin(), patch_filenames.end(), database_directory_ + "patches/NtermTruncation.txt" )
					== patch_filenames.end() ) {
				patch_filenames.push_back( database_directory_ + "patches/NtermTruncation.txt" );
			}
			if ( std::find( patch_filenames.begin(), patch_filenames.end(), database_directory_ + "patches/CtermTruncation.txt" )
					== patch_filenames.end() ) {
				patch_filenames.push_back( database_directory_ + "patches/CtermTruncation.txt" );
			}

		}

		// Also obtain metapatch filenames, for FA_STANDARD only.
		if ( name_ == "fa_standard" ) {
			std::string const meta_filename( database_directory_+"/metapatches.txt" );
			utility::io::izstream data2( meta_filename.c_str() );

			if ( !data2.good() ) {
				utility_exit_with_message( "Unable to open metapatch list file: "+meta_filename );
			}
			std::string mpline;
			while ( getline( data2, mpline ) ) {
				if ( mpline.size() < 1 || mpline[0] == '#' ) continue;
				metapatch_filenames.push_back( database_directory_ + mpline );
			}
		}

		apply_patches( patch_filenames, metapatch_filenames );
	}  // patches applied



	// Generate combinations of adducts as specified by the user
	place_adducts( *this );

	ResidueTypeCOPs residue_types = cache_->generated_residue_types();
	if ( option[ OptionKeys::in::add_orbitals] ) {
		for ( Size ii = 1 ; ii <= residue_types.size() ; ++ii ) {
			if ( residue_types[ii]->finalized() ) {
				ResidueTypeOP rsd_type_clone = residue_types[ii]->clone();
				orbitals::AssignOrbitals( rsd_type_clone ).assign_orbitals();
				cache_->update_residue_type( residue_types[ ii ], rsd_type_clone );
				update_base_residue_types_if_replaced( residue_types[ ii ], rsd_type_clone );
			}
		}
	}

	// Components file? (Willl be empty if we're not doing this.)
	if ( option[ OptionKeys::in::file::load_PDB_components ] || option[ OptionKeys::in::file::PDB_components_file ].user() ) {
		pdb_components_filename_ = option[ OptionKeys::in::file::PDB_components_file ].value();
	}

	tr << "Finished initializing " << name_ << " residue type set.  ";
	tr << "Created " << residue_types.size() << " residue types" << std::endl;
	tr << "Total time to initialize " << static_cast<Real>( clock() - time_start ) / CLOCKS_PER_SEC << " seconds." << std::endl;

}

ResidueTypeSet::ResidueTypeSet() {}

ResidueTypeSet::~ResidueTypeSet() {}

///////////////////////////////////////////////////////////////////////////////
/// @details the file contains a list of names of residue type parameter files
/// stored in the database path
void
ResidueTypeSet::read_list_of_residues(
	std::string const & list_filename
)
{
	// read the files
	utility::vector1< std::string > filenames;
	{
		utility::io::izstream data( list_filename.c_str() );
		std::string line;
		while ( getline( data, line ) ) {
			// add full database path to the AA.params filename
			filenames.push_back( basic::database::full_name( line ) );
		}
		data.close();
	}

	read_files( filenames );
}

///////////////////////////////////////////////////////////////////////////////
void
ResidueTypeSet::read_files(
	utility::vector1< std::string > const & filenames
)
{
	for ( Size ii=1; ii<= filenames.size(); ++ii ) {
		ResidueTypeCOP rsd_type( read_topology_file( filenames[ii], atom_types_, elements_, mm_atom_types_,orbital_types_, get_self_weak_ptr() ) );
		base_residue_types_.push_back( rsd_type );
	}
}

///////////////////////////////////////////////////////////////////////////////
void
ResidueTypeSet::apply_patches(
	utility::vector1< std::string > const & patch_filenames,
	utility::vector1< std::string > const & metapatch_filenames
)
{
	for ( Size ii=1; ii<= patch_filenames.size(); ++ii ) {
		PatchOP p( new Patch(name_) );
		p->read_file( patch_filenames[ii] );
		patches_.push_back( p );
		patch_map_[ p->name() ].push_back( p );
	}

	for ( Size ii=1; ii <= metapatch_filenames.size(); ++ii ) {
		MetapatchOP p( new Metapatch );
		p->read_file( metapatch_filenames[ii] );
		metapatches_.push_back( p );
		metapatch_map_[ p->name() ] = p;
	}


	// separate this to handle a set of base residue types and a set of patches...
	// this would allow addition of patches and/or base_residue_types at stages after initialization.

	// The "replace_residue" patches are a special case, and barely in use anymore.
	// In their current implementation, they actually do *not* change the name of the ResidueType.
	// But we probably should keep track of their application
	//  by updating name() of residue; and hold copies of the replaced residues without patch applied in, e.g., replaced_name_map_.
	// That's going to require a careful refactoring of how residue types are accessed (e.g., can't just use name3_map() anymore),
	//  which I might do later. For example, there's code in SwitchResidueTypeSet that will look for "MET" when it really should
	//  look for "MET:protein_centroid_with_HA" and know about this patch.
	// For now, apply them first, and force application/instantiation later.
	// -- rhiju
	for ( Size ii=1; ii<= patches_.size(); ++ii ) {
		PatchCOP p( patches_[ ii ] );
		for ( Size i=1; i<= base_residue_types_.size(); ++i ) {
			ResidueType const & rsd_type( *base_residue_types_[ i ] );
			if ( p->applies_to( rsd_type ) ) {
				if ( p->replaces( rsd_type ) ) {
					runtime_assert( rsd_type.finalized() );
					ResidueTypeCOP rsd_type_new = p->apply( rsd_type );
					cache_->update_residue_type( base_residue_types_[ i ], rsd_type_new );
					runtime_assert( update_base_residue_types_if_replaced( base_residue_types_[ i ], rsd_type_new ) );
				}
			}
		}
	}

	// separate this to handle a set of base residue types and a set of patches.
	// this would allow addition of patches and/or base_residue_types at stages after initialization.
	for ( Size ii=1; ii<= patches_.size(); ++ii ) {
		PatchCOP p( patches_[ ii ] );
		for ( Size i=1; i<= base_residue_types_.size(); ++i ) {
			ResidueType const & rsd_type( *base_residue_types_[ i ] );
			if ( p->applies_to( rsd_type ) && p->adds_properties( rsd_type ).has_value( "D_AA" ) ) {
				ResidueTypeOP new_rsd_type_op = p->apply( rsd_type );
				new_rsd_type_op->base_name( new_rsd_type_op->name() ); //D-residues have their own base names.
				new_rsd_type_op->reset_base_type_cop(); //This is now a base type, so its base type pointer must be NULL.

				ResidueTypeCOP new_rsd_type( new_rsd_type_op );  //Make const-access pointer.
				base_residue_types_.push_back( new_rsd_type );
				cache_->add_residue_type( new_rsd_type );

				// Store the D-to-L and L-to-D mappings:
				runtime_assert_string_msg(
					l_to_d_mapping_.count( base_residue_types_[i] ) == 0,
					"Error in core::chemical::ResidueTypeSet::apply_patches: A D-equivalent for " + rsd_type.name() + " has already been defined."
				);
				l_to_d_mapping_[ base_residue_types_[i] ] = new_rsd_type;
				runtime_assert_string_msg(
					d_to_l_mapping_.count( new_rsd_type ) == 0,
					"Error in core::chemical::ResidueTypeSet::apply_patches: An L-equivalent for " + new_rsd_type->name() + " has already been defined."
				);
				d_to_l_mapping_[ new_rsd_type ] = base_residue_types_[i];
			}
		}
	}

	// separate this to handle a set of base residue types and a set of patches.
	// this would allow addition of patches and/or base_residue_types at stages after initialization.
	update_info_on_name3_and_interchangeability_group( base_residue_types_ );
}

/// @details following assumes that all new name3 and interchangeability groups for residue types
///    can be discovered by applying patches to base residue types -- i.e. on the 'first patch'.
///    Probably should set up a runtime_assert in ResidueTypeFinder to check this assumption.
void
ResidueTypeSet::update_info_on_name3_and_interchangeability_group( ResidueTypeCOPs base_residue_types ) {

	for ( Size ii=1; ii<= patches_.size(); ++ii ) {
		PatchCOP p( patches_[ ii ] );
		for ( Size i=1; i<= base_residue_types.size(); ++i ) {
			ResidueType const & rsd_type( *base_residue_types[ i ] );
			if ( p->applies_to( rsd_type ) ) {

				// Check if any patches change name3 of residue_types.
				// Such patches must be applicable to base residue types -- probably should
				// check this somewhere (e.g. ResidueTypeFinder).
				std::string const new_name3 = p->generates_new_name3( rsd_type );
				if ( new_name3.size() > 0 && rsd_type.name3() != new_name3 ) {
					name3_generated_by_base_residue_name_[ rsd_type.name() ].insert( new_name3 );
				}

				// similarly, check if any patches create interchangeability_groups from this base residue type.
				// Used by ResidueTypeFinder.
				std::string const interchangeability_group = p->generates_interchangeability_group( rsd_type );
				if ( interchangeability_group.size() > 0 ) {
					interchangeability_group_generated_by_base_residue_name_[ rsd_type.name() ].insert( interchangeability_group );
				}
			}
		}
	}

}


//////////////////////////////////////////////////////////////////
/// @details since residue id is unique, it only returns
/// one residue type or exit without match.
///
//////////////////////////////////////////////////////////////////
ResidueType const &
ResidueTypeSet::name_map( std::string const & name_in ) const
{
	ResidueTypeCOP restype( name_mapOP( name_in ) );
	runtime_assert_string_msg( restype != 0, "The residue " + name_in + " could not be generated.  Has a suitable params file been loaded?  (Note that custom params files not in the Rosetta database can be loaded with the -extra_res or -extra_res_fa command-line flags.)"  );
	return *restype;
}

//////////////////////////////////////////////////////////////////
///
/// MOST RESIDUE TYPES WILL BE GENERATED THROUGH THIS FUNCTION.
///
//////////////////////////////////////////////////////////////////
ResidueTypeCOP
ResidueTypeSet::name_mapOP( std::string const & name_in ) const
{
	std::string const name = fixup_patches( name_in );
	if ( generate_residue_type( name ) ) {
		return cache_->name_map( name );
	} else {
		return ResidueTypeCOP( 0 );
	}
}

/// @details Instantiates ResidueType
bool
ResidueTypeSet::generate_residue_type( std::string const & rsd_name ) const
{
	if ( cache_->has_generated_residue_type( rsd_name ) ) return true; // already generated

	// get name (which holds patch information)
	std::string rsd_name_base, patch_name;
	figure_out_last_patch_from_name( rsd_name, rsd_name_base, patch_name );

	if ( patch_name.size() == 0 ) { // If this is the non-patched base type
		if ( ! cache_->has_generated_residue_type( rsd_name_base ) ) {
			lazy_load_base_type( rsd_name_base );
		}
		return cache_->has_generated_residue_type( rsd_name ); // Is generated?
	}

	// now apply patches.
	ResidueTypeCOP rsd_base_ptr = name_mapOP( rsd_name_base );
	if ( ! rsd_base_ptr ) { return false; }

	ResidueType const & rsd_base( *rsd_base_ptr );
	runtime_assert( rsd_base.finalized() );

	// I may have to create this patch, if it is metapatch-derived!
	// This version preserves this function's constness by not
	// adding to patches_ or the patch_map.
	// You are looking for the name of the base residue type
	if ( patch_name.find( "MP-" ) != std::string::npos ) {

		std::string metapatch_name = utility::string_split( patch_name, '-' )[3];

		// This patch needs to be generated by this metapatch.
		// PHE-CD1-methylated
		// Atom name: second element if you split the patch name by '-'
		std::string atom_name = utility::string_split( patch_name, '-' )[2];

		if ( metapatch_map_.find( metapatch_name ) == metapatch_map_.end() ) {
			utility_exit_with_message(  "Metapatch " + metapatch_name +
				" not in the metapatch map!" );
		}

		// Add buffering spaces just in case the resultant patch is PDB-naming sensitive
		// we need enough whitespace -- will trim later
		PatchCOP needed_patch = metapatch_map_.at( metapatch_name )->get_one_patch( /*rsd_base, */"  " + atom_name + "  " );
		//patches_.push_back( needed_patch );
		//patch_map_[ needed_patch->name() ] = needed_patch;

		ResidueTypeOP rsd_instantiated ( needed_patch->apply( rsd_base ) );

		if ( rsd_instantiated == 0 ) {
			return false; // utility_exit_with_message(  "Failed to apply: " + p->name() + " to " + rsd_base.name() );
		}
		if ( option[ OptionKeys::in::file::assign_gasteiger_atom_types ] ) {
			gasteiger::GasteigerAtomTypeSetCOP gasteiger_set(
				ChemicalManager::get_instance()->gasteiger_atom_type_set() );
			gasteiger::assign_gasteiger_atom_types( *rsd_instantiated, gasteiger_set, false );
		}
		if ( option[ OptionKeys::in::add_orbitals] ) {
			orbitals::AssignOrbitals( rsd_instantiated ).assign_orbitals();
		}

		//Set the pointer to the base type:
		if ( rsd_base.get_base_type_cop() ) {
			rsd_instantiated->set_base_type_cop( rsd_base.get_base_type_cop() );
		} else {
			rsd_instantiated->set_base_type_cop( rsd_base.get_self_ptr() );
		}

		cache_->add_residue_type( rsd_instantiated );
		return true;

	} else {
		if ( patch_map_.find( patch_name ) == patch_map_.end() ) return false;
		utility::vector1< PatchCOP > const & patches = patch_map_.find( patch_name )->second;
		bool patch_applied( false );

		// sometimes patch cases are split between several patches -- look through all:
		for ( Size n = 1; n <= patches.size(); n++ ) {
			PatchCOP p = patches[ n ];

			if ( !p->applies_to( rsd_base ) ) continue;
			runtime_assert( !patch_applied ); // patch cannot be applied twice.

			ResidueTypeOP rsd_instantiated = p->apply( rsd_base );

			if ( rsd_instantiated == 0 ) {
				return false; // utility_exit_with_message(  "Failed to apply: " + p->name() + " to " + rsd_base.name() );
			}
			if ( option[ OptionKeys::in::file::assign_gasteiger_atom_types ] ) {
				gasteiger::GasteigerAtomTypeSetCOP gasteiger_set(
					ChemicalManager::get_instance()->gasteiger_atom_type_set() );
				gasteiger::assign_gasteiger_atom_types( *rsd_instantiated, gasteiger_set, false );
			}
			if ( option[ OptionKeys::in::add_orbitals] ) {
				orbitals::AssignOrbitals( rsd_instantiated ).assign_orbitals();
			}

			//Set the pointer to the base type:
			if ( rsd_base.get_base_type_cop() ) {
				rsd_instantiated->set_base_type_cop( rsd_base.get_base_type_cop() );
			} else {
				rsd_instantiated->set_base_type_cop( rsd_base.get_self_ptr() );
			}

			cache_->add_residue_type( rsd_instantiated );
			patch_applied = true;
		}
		return patch_applied;
	}
}

/// @brief Check if a base type (like "SER") generates any types with another name3 (like "SEP")
bool
ResidueTypeSet::generates_patched_residue_type_with_name3( std::string const & base_residue_name,
	std::string const & name3 ) const
{
	if ( name3_generated_by_base_residue_name_.find( base_residue_name ) ==
			name3_generated_by_base_residue_name_.end() ) return false;
	std::set< std::string> const & name3_set = name3_generated_by_base_residue_name_.find( base_residue_name )->second;
	return ( name3_set.count( name3 ) );
}

/// @brief Check if a base type (like "CYS") generates any types with a new
/// interchangeability group (like "SCY" (via cys_acetylated))
bool
ResidueTypeSet::generates_patched_residue_type_with_interchangeability_group( std::string const & base_residue_name,
	std::string const & interchangeability_group ) const
{
	if ( interchangeability_group_generated_by_base_residue_name_.find( base_residue_name ) ==
			interchangeability_group_generated_by_base_residue_name_.end() ) {
		return false;
	}
	std::set< std::string> const & interchangeability_group_set =
		interchangeability_group_generated_by_base_residue_name_.find( base_residue_name )->second;
	return interchangeability_group_set.count( interchangeability_group );
}


//////////////////////////////////////////////////////////////////////////////
void
ResidueTypeSet::figure_out_last_patch_from_name( std::string const & rsd_name,
	std::string & rsd_name_base,
	std::string & patch_name ) const
{
	Size pos = rsd_name.find_last_of( PATCH_LINKER );
	if ( pos != std::string::npos ) {
		rsd_name_base = rsd_name.substr( 0, pos );
		patch_name    = rsd_name.substr( pos + 1 );
	} else { // Patch linker not found
		rsd_name_base = rsd_name;
		patch_name    = "";
	}

	// For D patch, it's the first letter.
	if ( patch_name.size() == 0 && rsd_name[ 0 ] == 'D' && has_name( rsd_name.substr( 1 ) ) ) {
		rsd_name_base = rsd_name.substr( 1 );
		patch_name    = "D";
	}
}

//////////////////////////////////////////////////////////////////////////////
/// @details helper function used during replacing residue types after, e.g., orbitals. Could possibly expand to update all maps.
bool
ResidueTypeSet::update_base_residue_types_if_replaced( ResidueTypeCOP rsd_type, ResidueTypeCOP rsd_type_new )
{
	if ( !base_residue_types_.has_value( rsd_type ) ) return false;
	base_residue_types_[ base_residue_types_.index( rsd_type ) ] = rsd_type_new;
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// The useful stuff:  Accessor functions
//////////////////////////////////////////////////////////////////////////////

/// @brief   checks if name exists.
/// @details actually instantiates the residue type if it does not exist.
bool
ResidueTypeSet::has_name( std::string const & name ) const
{
	return generate_residue_type( name );
}

/// @brief Get the base ResidueType with the given aa type and variants
/// @details Returns 0 if one does not exist.
ResidueTypeCOP
ResidueTypeSet::get_representative_type_aa( AA aa, utility::vector1< std::string > const & variants ) const {
	return ResidueTypeFinder( *this ).aa( aa ).variants( variants ).get_representative_type();
}

ResidueTypeCOP
ResidueTypeSet::get_representative_type_aa( AA aa ) const {
	utility::vector1< std::string > const variants; // blank
	return get_representative_type_aa( aa, variants );
}


/// @brief Get the base ResidueType with the given name1 and variants
/// @details Returns 0 if one does not exist.
ResidueTypeCOP
ResidueTypeSet::get_representative_type_name1( char name1, utility::vector1< std::string > const & variants  ) const {
	return ResidueTypeFinder( *this ).name1( name1 ).variants( variants ).get_representative_type();
}

ResidueTypeCOP
ResidueTypeSet::get_representative_type_name1( char name1  ) const {
	utility::vector1< std::string > const variants; // blank
	return get_representative_type_name1( name1, variants );
}

/// @brief Get the base ResidueType with the given name3 and variants
/// @details Returns 0 if one does not exist.
ResidueTypeCOP
ResidueTypeSet::get_representative_type_name3( std::string const &  name3, utility::vector1< std::string > const & variants  ) const {
	return ResidueTypeFinder( *this ).name3( name3 ).variants( variants ).get_representative_type();
}

ResidueTypeCOP
ResidueTypeSet::get_representative_type_name3( std::string const &  name3  ) const {
	utility::vector1< std::string > const variants; // blank
	return get_representative_type_name3( name3, variants );
}

/// @brief Gets all non-patched types with the given aa type
ResidueTypeCOPs
ResidueTypeSet::get_base_types_aa( AA aa ) const {
	return ResidueTypeFinder( *this ).aa( aa ).get_possible_base_residue_types();
}

/// @brief Get all non-patched ResidueTypes with the given name1
ResidueTypeCOPs
ResidueTypeSet::get_base_types_name1( char name1 ) const {
	return ResidueTypeFinder( *this ).name1( name1 ).get_possible_base_residue_types();
}

/// @brief Get all non-patched ResidueTypes with the given name3
ResidueTypeCOPs
ResidueTypeSet::get_base_types_name3( std::string const & name3 ) const {
	return ResidueTypeFinder( *this ).name3( name3 ).get_possible_base_residue_types();
}

/// @brief Given a D-residue, get its L-equivalent.
/// @details Returns NULL if there is no equivalent, true otherwise.  Throws an error if this is not a D-residue.
/// Preserves variant types.
/// @author Vikram K. Mulligan (vmullig@uw.edu).
ResidueTypeCOP
ResidueTypeSet::get_d_equivalent(
	ResidueTypeCOP l_rsd
) const {
	runtime_assert_string_msg( l_rsd, "Error in core::chemical::ResidueTypeSet::get_d_equivalent(): A null pointer was passed to this function!" );
	runtime_assert_string_msg( l_rsd->is_l_aa(), "Error in core::chemical::ResidueTypeSet::get_d_equivalent(): The residue passed to this function is not an L_AA!" );

	ResidueTypeCOP l_basetype( l_rsd->get_base_type_cop() );
	if ( !l_to_d_mapping_.count(l_basetype) ) return ResidueTypeCOP(); //Returns NULL pointer if there's no D-equivalent.

	ResidueTypeCOP d_basetype( l_to_d_mapping_.at(l_basetype) );

	//Add back the variants, as efficiently as possible:
	utility::vector1<VariantType> const variant_type_list( l_rsd->variant_type_enums() );
	utility::vector1<std::string> const & custom_variant_type_list( l_rsd->custom_variant_types() );

	ResidueTypeCOP d_rsd ( ResidueTypeFinder( *this ).base_type(d_basetype).variants( variant_type_list, custom_variant_type_list ).get_representative_type() );

	return d_rsd;
}

/// @brief Given an L-residue, get its D-equivalent.
/// @details Returns NULL if there is no equivalent, true otherwise.  Throws an error if this is not an L-residue.
/// Preserves variant types.
/// @author Vikram K. Mulligan (vmullig@uw.edu).
ResidueTypeCOP
ResidueTypeSet::get_l_equivalent(
	ResidueTypeCOP d_rsd
) const {
	runtime_assert_string_msg( d_rsd, "Error in core::chemical::ResidueTypeSet::get_l_equivalent(): A null pointer was passed to this function!" );
	runtime_assert_string_msg( d_rsd->is_d_aa(), "Error in core::chemical::ResidueTypeSet::get_l_equivalent(): The residue passed to this function is not a D_AA!" );

	ResidueTypeCOP d_basetype( d_rsd->get_base_type_cop() );
	if ( !d_to_l_mapping_.count(d_basetype) ) return ResidueTypeCOP(); //Returns NULL pointer if there's no D-equivalent.

	ResidueTypeCOP l_basetype( d_to_l_mapping_.at(d_basetype) );

	//Add back the variants, as efficiently as possible:
	utility::vector1<VariantType> const variant_type_list( d_rsd->variant_type_enums() );
	utility::vector1<std::string> const & custom_variant_type_list( d_rsd->custom_variant_types() );

	ResidueTypeCOP l_rsd ( ResidueTypeFinder( *this ).base_type(l_basetype).variants( variant_type_list, custom_variant_type_list ).get_representative_type() );

	return l_rsd;
}

/// @brief Given a residue, get its mirror-image type.
/// @details Returns the same residue if this is an ACHIRAL type (e.g. gly), the D-equivalent for an L-residue, the L-equivalent of a D-residue,
/// or NULL if this is an L-residue with no D-equivalent (or a D- with no L-equivalent).  Preserves variant types.
ResidueTypeCOP
ResidueTypeSet::get_mirrored_type(
	ResidueTypeCOP original_rsd
) const {
	if ( original_rsd->is_achiral_backbone() ) return original_rsd;

	runtime_assert_string_msg( original_rsd->is_d_aa() || original_rsd->is_l_aa(), "Error in core::chemical::ResidueTypeSet::get_mirror_type(): The residue type must be achiral, or must have the D_AA or L_AA property." );

	if ( original_rsd->is_d_aa() ) return get_l_equivalent(original_rsd);
	if ( original_rsd->is_l_aa() ) return get_d_equivalent(original_rsd);

	return ResidueTypeCOP();
}

/// @brief Gets all types with the given aa type and variants
/// @details The number of variants must match exactly.
/// (It's assumed that the passed VariantTypeList contains no duplicates.)
ResidueTypeCOPs
ResidueTypeSet::get_all_types_with_variants_aa( AA aa, utility::vector1< std::string > const & variants ) const
{
	utility::vector1< VariantType > exceptions;
	return cache_->get_all_types_with_variants_aa( aa, variants, exceptions );
}

ResidueTypeCOPs
ResidueTypeSet::get_all_types_with_variants_aa( AA aa,
	utility::vector1< std::string > const & variants,
	utility::vector1< VariantType > const & exceptions ) const
{
	return cache_->get_all_types_with_variants_aa( aa, variants, exceptions );
}

/// @brief Gets all types with the given name1 and variants
/// @brief Get all non-patched ResidueTypes with the given name1
/// @details The number of variants must match exactly.
/// (It's assumed that the passed VariantTypeList contains no duplicates.)
ResidueTypeCOPs
ResidueTypeSet::get_all_types_with_variants_name1( char name1, utility::vector1< std::string > const & variants ) const {
	return ResidueTypeFinder( *this ).name1( name1 ).variants( variants ).get_all_possible_residue_types();
}

/// @brief Gets all types with the given name3 and variants
/// @details The number of variants must match exactly.
/// (It's assumed that the passed VariantTypeList contains no duplicates.)
ResidueTypeCOPs
ResidueTypeSet::get_all_types_with_variants_name3( std::string const &  name3, utility::vector1< std::string > const & variants ) const {
	return ResidueTypeFinder( *this ).name3( name3 ).variants( variants ).get_all_possible_residue_types();
}

/// @brief any residue types with this name3?
bool
ResidueTypeSet::has_name3( std::string const & name3 ) const
{
	return ( get_representative_type_name3( name3 ) != 0 );
}

bool
ResidueTypeSet::has_interchangeability_group( std::string const & interchangeability_group ) const
{
	ResidueTypeCOP rsd_type = ResidueTypeFinder( *this ).interchangeability_group( interchangeability_group ).get_representative_type();
	return ( rsd_type != 0 );
}


///////////////////////////////////////////////////////////////////////////////
/// @details Return the first match with both base ResidueType id and variant_type name.  Abort if there is no match.
/// @note    Currently, this will not work for variant types defined as alternate base residues (i.e., different .params
///          files).
/// @remark  TODO: This should be refactored to make better use of the new ResidueProperties system. ~Labonte
ResidueType const &
ResidueTypeSet::get_residue_type_with_variant_added(
	ResidueType const & init_rsd,
	VariantType const new_type ) const
{
	if ( init_rsd.has_variant_type( new_type ) ) return init_rsd;

	// Find all residues with the same base name as init_rsd.
	std::string const base_name( residue_type_base_name( init_rsd ) );

	// the desired set of variant types:
	utility::vector1< std::string > target_variants( init_rsd.properties().get_list_of_variants() );
	if ( !init_rsd.has_variant_type(new_type) ) {
		target_variants.push_back( ResidueProperties::get_string_from_variant( new_type ) );
	}

	ResidueTypeCOP rsd_type = ResidueTypeFinder( *this ).residue_base_name( base_name ).variants( target_variants ).get_representative_type();

	if ( rsd_type == 0 ) {
		utility_exit_with_message( "unable to find desired variant residue: " + init_rsd.name() + " " + base_name + " " +
			ResidueProperties::get_string_from_variant( new_type ) );
	}

	return *rsd_type;
}

std::string const &
ResidueTypeSet::database_directory() const
{
	return database_directory_;
}

MergeBehaviorManager const &
ResidueTypeSet::merge_behavior_manager() const
{
	return *merge_behavior_manager_;
}


///////////////////////////////////////////////////////////////////////////////
/// @note    Currently, this will not work for variant types defined as alternate base residues (i.e., different .params
///          files).
/// @remark  TODO: This should be refactored to make better use of the new ResidueProperties system. ~Labonte
ResidueType const &
ResidueTypeSet::get_residue_type_with_variant_removed(
	ResidueType const & init_rsd,
	VariantType const old_type) const
{
	if ( !init_rsd.has_variant_type( old_type ) ) return init_rsd;

	// find all residues with the same base name as init_rsd
	std::string const base_name( residue_type_base_name( init_rsd ) );

	// the desired set of variant types:
	utility::vector1< std::string > target_variants( init_rsd.properties().get_list_of_variants() );
	target_variants.erase( std::find( target_variants.begin(), target_variants.end(),
		ResidueProperties::get_string_from_variant( old_type ) ) );

	ResidueTypeCOP rsd_type = ResidueTypeFinder( *this ).residue_base_name( base_name ).variants( target_variants ).get_representative_type();

	if ( rsd_type == 0 ) {
		utility_exit_with_message( "unable to find desired non-variant residue: " + init_rsd.name() + " " + base_name +
			" " + ResidueProperties::get_string_from_variant( old_type ) );
	}

	return *rsd_type;
}

void
ResidueTypeSet::add_custom_residue_type( std::string const & filename )
{
	ResidueTypeOP rsd_type( read_topology_file( filename, atom_types_, elements_, mm_atom_types_, orbital_types_, get_self_weak_ptr() ) );
	add_custom_residue_type( rsd_type );
}

void
ResidueTypeSet::read_files_for_custom_residue_types(
	utility::vector1< std::string > const & filenames
)
{
	for ( Size n = 1; n <= filenames.size(); n++ ) {
		add_custom_residue_type( filenames[ n ] );
	}
}

void
ResidueTypeSet::add_custom_residue_type( ResidueTypeOP new_type )
{
	new_type->residue_type_set( get_self_weak_ptr() );

	if ( option[ OptionKeys::in::file::assign_gasteiger_atom_types ] ) {
		gasteiger::GasteigerAtomTypeSetCOP gasteiger_set(
			ChemicalManager::get_instance()->gasteiger_atom_type_set() );
		gasteiger::assign_gasteiger_atom_types( *new_type, gasteiger_set, false );
	}
	if ( option[ OptionKeys::in::add_orbitals] ) {
		orbitals::AssignOrbitals add_orbitals_to_residue(new_type);
		add_orbitals_to_residue.assign_orbitals();
	}
	custom_residue_types_.push_back( new_type );
	cache_->add_residue_type( new_type );
	cache_->clear_cached_maps();
}

void
ResidueTypeSet::remove_custom_residue_type( std::string const & name )
{
	ResidueTypeCOP rsd_type( cache_->name_map( name ) );
	ResidueTypeCOPs::iterator res_it = std::find( custom_residue_types_.begin(), custom_residue_types_.end(), rsd_type );
	runtime_assert( res_it != custom_residue_types_.end() );
	custom_residue_types_.erase( res_it );
	cache_->remove_residue_type( name );
}

void
ResidueTypeSet::remove_base_residue_type_DO_NOT_USE( std::string const & name )
{
	ResidueTypeCOP rsd_type( cache_->name_map( name ) );
	ResidueTypeCOPs::iterator res_it = std::find( base_residue_types_.begin(), base_residue_types_.end(), rsd_type );
	runtime_assert( res_it != base_residue_types_.end() );
	base_residue_types_.erase( res_it );
	cache_->remove_residue_type( name );
	// Note danger here -- could still have patched versions of residue in base_residue_types, but they may no longer be accessible.
}

/// @brief From a file, read which IDs shouldn't be loaded from the components.
void
ResidueTypeSet::load_shadowed_ids( std::string const & directory, std::string const & filename /* = "shadow_list.txt" */ ) {

	tr.Debug << "Loading shadowed PDB IDs from " << directory + filename << std::endl;

	shadowed_ids_.clear();

	utility::io::izstream file( directory + filename );
	if ( ! file.good()  ) {
		tr << "For ResidueTypeSet " << name() << " there is no " << filename << " file to list known PDB ids." << std::endl;
		tr << "    This will turn off PDB component loading for ResidueTypeSet " << name() << std::endl;
		tr << "    Expected file: " << directory + filename << std::endl;
		return;
	}
	std::string line;
	getline( file, line );
	while ( file.good() ) {
		utility::trim( line ); // inplace;
		if ( line[0] != '#' ) {
			shadowed_ids_.insert( line );
		}
		getline( file, line );
	}
	if ( shadowed_ids_.size() == 0 ) {
		tr.Warning << "For ResidueTypeSet " << name() << ", " << filename << " doesn't have any entries." << std::endl;
		tr.Warning << "    This will turn off PDB component loading for ResidueTypeSet " << name() << std::endl;
	}
}

/// @brief Attempt to lazily load the given residue type from data.
bool
ResidueTypeSet::lazy_load_base_type( std::string const & rsd_base_name ) const
{
	if ( cache_->has_generated_residue_type( rsd_base_name ) ) { return true; }
	if ( cache_->is_prohibited( rsd_base_name ) ) { return false; }

	core::chemical::ResidueTypeOP new_rsd_type;

	// These are heuristics to figure out where to load the data from.

	// Heuristic: if the ResidueTypeName begins with 'pdb_', then it's loaded from the chemical components directory
	if ( rsd_base_name.find("pdb_") == 0 ) {
		std::string short_name( utility::strip( rsd_base_name.substr( 4, rsd_base_name.size() ) ) );
		if ( shadowed_ids_.size() > 0 && shadowed_ids_.count( short_name ) == 0 ) {
			new_rsd_type = load_pdb_component( short_name );
			if ( new_rsd_type ) {
				// Duplicate detection is handled by the shadowed file -- if it's not shadowed, we load the component
				new_rsd_type->name( "pdb_" + short_name );
				tr << "Loading '" << short_name << "' from the PDB components dictionary for residue type '" << rsd_base_name << "'" << std::endl;
			}
		} else {
			if ( shadowed_ids_.size() == 0 ) {
				tr.Debug << "Not loading '" << short_name << "' from PDB components dictionary because components are turned off for this ResidueTypeSet." << std::endl;
			} else {
				tr.Debug << "Not loading '" << short_name << "' from PDB components dictionary because it is shadowed in the ResidueTypeSet." << std::endl;
			}
			cache_->add_prohibited( rsd_base_name );
			return false;
		}
	}

	// Finish up with the new residue type.
	if ( new_rsd_type ) {
		new_rsd_type->residue_type_set( this->get_self_weak_ptr() );
		cache_->add_residue_type( new_rsd_type );
	}
	return cache_->has_generated_residue_type( rsd_base_name );

}


/// @brief Load a residue type from the components dictionary.
ResidueTypeOP
ResidueTypeSet::load_pdb_component( std::string const & pdb_id ) const {
	static THREAD_LOCAL bool warned_about_missing_file( false );
	if ( pdb_components_filename_.size() ) {
		utility::io::izstream filestream( pdb_components_filename_ );
		if ( !filestream.good() ) {
			std::string db_filename( basic::database::full_name( pdb_components_filename_, false ) );
			filestream.open( db_filename );

			if ( !filestream.good() ) {
				if ( ! warned_about_missing_file ) {
					warned_about_missing_file = true;
					tr.Warning << "PDB component dictionary file not found at (./)" << pdb_components_filename_ << std::endl;
					tr.Warning << "   or in the Rosetta database at " << db_filename << std::endl;
					tr.Warning << "   For information on how to obtain the file and set it for use with Rosetta, visit: \n\n";
					tr.Warning << "  https://www.rosettacommons.org/docs/latest/build_documentation/Build-Documentation#setting-up-rosetta-3_obtaining-additional-files_pdb-chemical-components-dictionary  \n" << std::endl;
				}
				return ResidueTypeOP( 0 );
			}
		}

		std::string entry( "data_" + pdb_id );
		std::string line;
		std::string lines;
		//std::cout << "Finding '" << entry <<"' " << entry.size() << std::endl;
		mmCIF::mmCIFParser mmCIF_parser;
		while ( filestream.good() ) {
			getline( filestream, line );
			if ( line.size() == entry.size() ) {
				//std::cout << line << std::endl;
				if ( line == entry ) {
					lines += line;
					//lines.push_back( line);
					getline( filestream, line);
					while ( line.substr(0, 5) != "data_" && filestream.good() ) {
						lines += line + '\n';
						//lines.push_back( line);
						getline( filestream, line);
					}
					break;
				}
			}
		}

		if ( lines.size() == 0 ) {
			tr.Warning << "Could not find: '" << pdb_id << "' in pdb components file '" << pdb_components_filename_
				<< "'! Skipping residue..." << std::endl;
			return ResidueTypeOP(0);
		}

		utility::vector1< core::chemical::sdf::MolFileIOMoleculeOP> molecules;
		molecules.push_back( mmCIF_parser.parse( lines, pdb_id) );
		core::chemical::ResidueTypeOP new_rsd_type( core::chemical::sdf::convert_to_ResidueType( molecules ) );

		return new_rsd_type;
	}
	return ResidueTypeOP(0);
}

} // pose
} // core
