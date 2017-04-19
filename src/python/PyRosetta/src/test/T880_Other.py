# :noTabs=true:
# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

## @author Sergey Lyskov

from __future__ import print_function

from rosetta import *
from pyrosetta import *

init(extra_options = "-constant_seed")  # WARNING: option '-constant_seed' is for testing only! MAKE SURE TO REMOVE IT IN PRODUCTION RUNS!!!!!
import os; os.chdir('.test.output')

print('testing ligand modeling')
params_list = ['../test/data/ligand.params']
res_set = generate_nonstandard_residue_set(params_list)
ligand_p = core.import_pose.pose_from_file(res_set, "../test/data/ligand_test.pdb")

scorefxn = create_score_function("ligand")
scorefxn(ligand_p)

print('testing DNA modeling')
dna_p = Pose()
core.import_pose.pose_from_file(dna_p, "../test/data/dna_test.pdb")

scorefxn = create_score_function("dna")
scorefxn(dna_p)

print('Done!')
