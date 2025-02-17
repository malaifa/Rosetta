// -*- mode:c++;tab-width:2;indent-tabs-mode:t;show-trailing-whitespace:t;rm-trailing-spaces:t -*-
// vi: set ts=2 noet:
//
// (c) Copyright Rosetta Commons Member Institutions.
// (c) This file is part of the Rosetta software suite and is made available
// (c) under license. The Rosetta software is developed by the contributing
// (c) members of the Rosetta Commons. For more information, see
// (c) http://www.rosettacommons.org. Questions about this can be addressed to
// (c) University of Washington UW TechTransfer,email:license@u.washington.edu.

/// @file protocols/denovo_design/movers/FoldTreeFromFoldGraphMover.cc
/// @brief Creates and sets a new fold tree for the pose by traversing a FoldGraph
/// @author Tom Linsky (tlinsky@uw.edu)

// Unit headers
#include <protocols/denovo_design/movers/FoldTreeFromFoldGraphMover.hh>
#include <protocols/denovo_design/movers/FoldTreeFromFoldGraphMoverCreator.hh>

// Protocol headers
#include <protocols/denovo_design/components/FoldGraph.hh>
#include <protocols/denovo_design/components/Segment.hh>
#include <protocols/denovo_design/components/StructureData.hh>
#include <protocols/denovo_design/components/StructureDataFactory.hh>
#include <protocols/denovo_design/components/StructureDataFactory.hh>
#include <protocols/denovo_design/util.hh>
#include <protocols/loops/Loop.hh>
#include <protocols/loops/Loops.hh>

// Core headers
#include <core/kinematics/FoldTree.hh>
#include <core/pose/Pose.hh>
#include <core/pose/util.hh>

// Basic/Utility headers
#include <basic/Tracer.hh>
#include <utility/tag/Tag.hh>

static THREAD_LOCAL basic::Tracer TR( "protocols.denovo_design.movers.FoldTreeFromFoldGraphMover" );

namespace protocols {
namespace denovo_design {
namespace movers {

FoldTreeFromFoldGraphMover::FoldTreeFromFoldGraphMover():
	protocols::moves::Mover( FoldTreeFromFoldGraphMover::class_name() ),
	roots_(),
	loops_()
{
}

FoldTreeFromFoldGraphMover::FoldTreeFromFoldGraphMover(
	SegmentNames const & roots,
	protocols::loops::Loops const & loops ):
	protocols::moves::Mover( FoldTreeFromFoldGraphMover::class_name() ),
	roots_( roots ),
	loops_( loops )
{
}

FoldTreeFromFoldGraphMover::~FoldTreeFromFoldGraphMover(){}

void
FoldTreeFromFoldGraphMover::parse_my_tag(
	utility::tag::TagCOP ,
	basic::datacache::DataMap& ,
	protocols::filters::Filters_map const & ,
	protocols::moves::Movers_map const & ,
	core::pose::Pose const & )
{
}

protocols::moves::MoverOP
FoldTreeFromFoldGraphMover::clone() const
{
	return protocols::moves::MoverOP( new FoldTreeFromFoldGraphMover( *this ) );
}

protocols::moves::MoverOP
FoldTreeFromFoldGraphMover::fresh_instance() const
{
	return protocols::moves::MoverOP( new FoldTreeFromFoldGraphMover );
}

std::string
FoldTreeFromFoldGraphMover::get_name() const
{
	return FoldTreeFromFoldGraphMover::class_name();
}

std::string
FoldTreeFromFoldGraphMover::class_name()
{
	return "FoldTreeFromFoldGraph";
}

void
FoldTreeFromFoldGraphMover::show( std::ostream & output ) const
{
	protocols::moves::Mover::show( output );
}

std::ostream &
operator<<( std::ostream & os, FoldTreeFromFoldGraphMover const & mover )
{
	mover.show(os);
	return os;
}

void
FoldTreeFromFoldGraphMover::apply( core::pose::Pose & pose )
{
	if ( roots_.empty() ) {
		utility_exit_with_message( "FoldTreeFromFoldGraphMover:: roots must contain at least one segment!\n" );
	}

	components::StructureDataOP sd =
		components::StructureDataFactory::get_instance()->create_from_pose( pose );

	components::FoldGraph fg( *sd );
	core::kinematics::FoldTree const ft = fg.fold_tree( roots_ );

	debug_assert( ft.check_fold_tree() );
	pose.fold_tree( ft );

	prepare_termini_for_remodel( pose );

	TR << "Set fold tree to " << pose.fold_tree() << std::endl;
}

int
new_jump_and_cutpoint( core::pose::Pose & pose, core::Size const saferes1, core::Size const saferes2, core::Size const cutres )
{
	debug_assert( saferes1 < cutres );
	debug_assert( saferes2 > cutres );
	TR << "Inserting jump " << saferes1 << "__" << saferes2 << " with cut at " << cutres << std::endl;

	// modify fold tree
	core::kinematics::FoldTree ft = pose.fold_tree();
	int const newjump = ft.new_jump( saferes1, saferes2, cutres );
	if ( !ft.check_fold_tree() ) {
		TR.Error << "FOLDTREE=" << ft << std::endl;
	}
	debug_assert( ft.check_fold_tree() );
	pose.fold_tree( ft );

	// remove terminal variants (if applicable)
	core::pose::remove_upper_terminus_type_from_pose_residue( pose, cutres );
	if ( cutres + 1 <= pose.total_residue() ) {
		core::pose::remove_lower_terminus_type_from_pose_residue( pose, cutres + 1 );
	}

	// add cutpoint variant types
	core::pose::add_variant_type_to_pose_residue( pose, core::chemical::CUTPOINT_LOWER, cutres );
	core::pose::add_variant_type_to_pose_residue( pose, core::chemical::CUTPOINT_UPPER, cutres + 1 );
	rebuild_missing_atoms( pose, cutres );
	rebuild_missing_atoms( pose, cutres + 1 );

	pose.conformation().chains_from_termini();

	// they should ALWAYS be on the same chain at this point
	TR.Debug << pose.fold_tree() << std::endl;
	TR.Debug << "chain of " << saferes1 << " = " << pose.chain( saferes1 ) << " chain of " << saferes2 << " = " << pose.chain( saferes2 ) << std::endl;
	for ( core::Size i=1; i<=pose.total_residue(); ++i ) {
		TR.Debug << i << " " << pose.residue(i).name() << std::endl;
	}
	debug_assert( pose.chain( saferes1 ) == pose.chain( saferes2 ) );

	return newjump;
}

/// @brief sets terminal variants for broken-chain folding using remodel
void
FoldTreeFromFoldGraphMover::prepare_termini_for_remodel( core::pose::Pose & pose )
{
	last_jump_info_ = JumpInfo();

	components::StructureData const & sd =
		components::StructureDataFactory::get_instance()->get_from_pose( pose );

	// this will be a vector of the centers of the area between loops, used for constructing fold tree
	for ( protocols::loops::Loops::iterator l=loops_.v_begin(); l!=loops_.v_end(); ++l ) {
		TR << "Loop: " << *l << std::endl;
		debug_assert( l->is_terminal(pose) || ( l->cut() >= 1 ) );
		debug_assert( l->is_terminal(pose) || ( l->cut() <= pose.total_residue() ) );
		debug_assert( l->start() >= 1 );
		debug_assert( l->stop() <= pose.total_residue() );

		// no cutpoints for terminal loops
		if ( l->is_terminal( pose ) ) {
			continue;
		}

		/*
		if ( l->cut() == 0 ) {
		// stupid hack because some remodel aspects don't work properly without a cutpoint set
		while ( ( l->cut() == 0 ) || ( l->cut() == pose.total_residue() ) ) {
		choose_cutpoint( *l, pose );
		}
		continue;
		}
		*/

		// find the closest two safe residues to the loop and add a jump between them.
		core::Size saferes1 = 0;
		core::Size saferes1_dist = 0;
		for ( SegmentNameList::const_iterator s=sd.segments_begin(); s!=sd.segments_end(); ++s ) {
			components::Segment const & seg = sd.segment( *s );
			if ( seg.safe() >= l->start() ) break;

			if ( !saferes1 || ( l->start() - seg.safe() < saferes1_dist ) ) {
				saferes1 = seg.safe();
				saferes1_dist = l->start() - saferes1;
			}
		}

		core::Size saferes2_dist = 0;
		core::Size saferes2 = 0;
		for ( SegmentNameList::const_iterator s=sd.segments_begin(); s!=sd.segments_end(); ++s ) {
			components::Segment const & seg = sd.segment( *s );
			if ( seg.safe() <= l->start() ) continue;

			if ( !saferes2 || ( seg.safe() - l->stop() < saferes2_dist ) ) {
				saferes2 = seg.safe();
				saferes2_dist = saferes2 - l->stop();
			}
		}

		// insert jump/cut
		last_jump_info_.push_back( CutAndJump( l->cut(), new_jump_and_cutpoint( pose, saferes1, saferes2, l->cut() ) ) );
	}
}

void
FoldTreeFromFoldGraphMover::set_roots( SegmentNames const & roots )
{
	roots_ = roots;
}

/////////////// Creator ///////////////

protocols::moves::MoverOP
FoldTreeFromFoldGraphMoverCreator::create_mover() const
{
	return protocols::moves::MoverOP( new FoldTreeFromFoldGraphMover );
}

std::string
FoldTreeFromFoldGraphMoverCreator::keyname() const
{
	return FoldTreeFromFoldGraphMover::class_name();
}

} //protocols
} //denovo_design
} //movers

