# 01_pdb_mtz.py
# Copyright 2007, 2008 by The University of York
# Author: Bernhard Lohkamp
# Copyright 2007, 2008 by The University of Oxford
# Author: Paul Emsley

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.
 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA

import unittest
import os
# and test that gobject is in place.  
#import gobject

global rnase_pdb
rnase_pdb = os.path.join(unittest_data_dir, "tutorial-modern.pdb")
global rnase_mtz
rnase_mtz = os.path.join(unittest_data_dir, "rnasa-1.8-all_refmac1.mtz")
global terminal_residue_test_pdb
terminal_residue_test_pdb = os.path.join(unittest_data_dir, "tutorial-add-terminal-1-test.pdb")
base_imol = graphics_n_molecules()
rnase_seq = os.path.join(unittest_data_dir, "rnase.seq")

global have_ccp4_qm
have_ccp4_qm = False
global imol_rnase
imol_rnase = -1
global imol_rnase_map
imol_rnase_map = -1
global imol_ligand
imol_ligand = -1
global imol_terminal_residue_test
imol_terminal_residue_test = -1

horne_works_cif   = os.path.join(unittest_data_dir, "lib-B3A.cif")
horne_cif         = os.path.join(unittest_data_dir, "lib-both.cif")
horne_pdb         = os.path.join(unittest_data_dir, "coords-B3A.pdb")
global ins_code_frag_pdb
ins_code_frag_pdb = os.path.join(unittest_data_dir, "ins-code-fragment-pre.pdb")

# CCP4 is set up? If so, set have-ccp4? True
try:
	#global have_ccp4_qm
	ccp4_master = os.getenv("CCP4_MASTER")
	if (os.path.isdir(ccp4_master)):
		print "==== It seems CCP$_MASTER is setup ==="
		print "==== CCP$_MASTER:", ccp4_master
		have_ccp4_qm = True
except:
	print "BL INFO:: Dont have CCP4 master"

class PdbMtzTestFunctions(unittest.TestCase):

    # tests are executed alphanumerical, so we shall give them number,
    # rather than names. We add a 0 in the end to give space for later
    # addition
    def test00_0(self):
	    """Post Go To Atom no molecule"""
	    post_go_to_atom_window()

	    
    def test01_0(self):
	    """Close bad molecule"""
	    close_molecule(-2)
	

    def test02_0(self):
	    """Read coordinates test"""
	    global imol_rnase
	    imol = read_pdb(rnase_pdb)
	    imol_rnase = imol
	    self.failUnless(valid_model_molecule_qm(imol))


    def test03_0(self):
	    """New molecule from bogus molecule"""
	    pre_n_molecules = graphics_n_molecules()
	    new_mol = new_molecule_by_atom_selection(-5, "//A/0")
	    post_n_molecules = graphics_n_molecules()
	    self.failUnlessEqual(new_mol, -1 ,"fail on non-matching n molecules (-1)")
	    self.failUnlessEqual(pre_n_molecules, post_n_molecules, "fail on non-matching n molecules (=)")


    def test04_0(self):
	    """New molecule from bogus atom selection"""
	    pre_n_molecules = graphics_n_molecules()
	    new_molecule = new_molecule_by_atom_selection(imol_rnase, "//A/0")
	    post_n_molecules = graphics_n_molecules()
	    # what should happen if there are no atoms in the new-mol?
	    print "   INFO:: pre_n_molecules %s   post_n_molecules %s" %(pre_n_molecules, post_n_molecules)
	    self.failUnlessEqual(new_molecule, -1 ,"fail on non-matching n molecules (-1)")
	    self.failUnlessEqual(pre_n_molecules, post_n_molecules, "fail on non-matching n molecules (=)")


    def test04_1(self):
	    """ins code change and Goto atom over an ins code break"""

	    def matches_attributes(atts_obs, atts_ref):
		    if (atts_obs == atts_ref):
			    return True
		    else:
			    return False

	    # main line
	    self.failUnless(os.path.isfile(ins_code_frag_pdb), "   WARNING:: file not found: %s" %ins_code_frag_pdb)
	    frag_pdb = handle_read_draw_molecule_with_recentre(ins_code_frag_pdb, 0)
	    set_go_to_atom_molecule(frag_pdb)
	    set_go_to_atom_chain_residue_atom_name("A", 68, " CA ")
	    ar_1 = active_residue()
	    ins_1 = ar_1[3]
	    change_residue_number(frag_pdb, "A", 68, "", 68, "A")
	    change_residue_number(frag_pdb, "A", 69, "", 68, "B")
	    change_residue_number(frag_pdb, "A", 67, "", 68, "")
	    ar_2 = active_residue()
	    ins_2 = ar_2[3]
	    print "pre and post ins codes: ", ins_1, ins_2
	    self.failUnlessEqual(ins_2, "A", " Fail ins code set: %s is not 'A'" %ins_2)

	    test_expected_results = [[goto_next_atom_maybe("A", 67, "",  " CA "), ["A", 68, "",  " CA "]],
				     [goto_next_atom_maybe("A", 68, "A", " CA "), ["A", 68, "B", " CA "]],
				     [goto_next_atom_maybe("A", 68, "B", " CA "), ["A", 70, "",  " CA "]],
				     [goto_next_atom_maybe("D", 10, "",  " O  "), ["A", 62, "",  " CA "]],
				     [goto_prev_atom_maybe("A", 70, "",  " CA "), ["A", 68, "B", " CA "]],
				     [goto_prev_atom_maybe("A", 68, "B", " CA "), ["A", 68, "A", " CA "]],
				     [goto_prev_atom_maybe("A", 68, "A", " CA "), ["A", 68, "",  " CA "]],
				     [goto_prev_atom_maybe("A", 68, "",  " CA "), ["A", 66, "",  " CA "]]]

	    for test_item in test_expected_results:
		    real_result     = test_item[0]
		    expected_result = test_item[1]

		    self.failUnless(real_result)
		    self.failUnlessEqual(real_result, expected_result, "   fail: real: %s expected: %s" %(real_result, expected_result))
		    print "   pass: ", expected_result


    def test05_0(self):
	    """Read a bogus map"""
	    pre_n_molecules = graphics_n_molecules()
	    imol = handle_read_ccp4_map("bogus.map", 0)
	    self.failUnlessEqual(imol, -1, "bogus ccp4 map returns wrong molecule number")
	    now_n_molecules = graphics_n_molecules()
	    self.failUnlessEqual(now_n_molecules, pre_n_molecules, "bogus ccp4 map creates extra map %s %s " %(pre_n_molecules, now_n_molecules))
	    

    def test06_0(self):
	    """Read MTZ test"""

	    global imol_rnase_map
	    # bogus map test
	    pre_n_molecules = graphics_n_molecules()
	    imol_map = make_and_draw_map("bogus.mtz", "FWT", "PHWT", "", 5, 6)
	    self.failUnlessEqual(imol_map, -1, "   bogus MTZ returns wrong molecule number")
	    now_n_molecules = graphics_n_molecules()
	    self.failUnlessEqual(now_n_molecules, pre_n_molecules, "   bogus MTZ creates extra map %s %s" %(pre_n_molecules, now_n_molecules))
	    
	    # correct mtz test
	    imol_map = make_and_draw_map(rnase_mtz, "FWT","PHWT","",0,0)
	    change_contour_level(0)
	    change_contour_level(0)
	    change_contour_level(0)
	    set_imol_refinement_map(imol_map)
	    imol_rnase_map = imol_map
	    self.failUnless(valid_map_molecule_qm(imol_map))


    def test06_1(self):
	    """Map Sigma """

	    global imol_rnase_map
	    global imol_rnase
	    self.failUnless(valid_map_molecule_qm(imol_rnase_map))
	    v = map_sigma(imol_rnase_map)
	    self.failUnless(v > 0.2 and v < 1.0)
	    v2 = map_sigma(imol_rnase)
	    print "INFO:: map sigmas", v, v2
	    self.failIf(v2)

    def test07_0(self):
	    """Another Level Test"""
	    imol_map_2 = another_level()
	    self.failUnless(valid_map_molecule_qm(imol_map_2))

	    
    def test08_0(self):
	    """Set Atom Atribute Test"""
	    atom_ls = []
	    global imol_rnase
	    set_atom_attribute(imol_rnase, "A", 11, "", " CA ", "", "x", 64.5) # an Angstrom or so
	    atom_ls = residue_info(imol_rnase, "A", 11, "")
	    self.failIfEqual(atom_ls, [])
	    atom = atom_ls[0]
	    compound_name = atom[0]
	    atom_name = compound_name[0]
	    if (atom_name == " CA "):
		    x = atom_ls[0][2][0]
		    self.failUnlessAlmostEqual(x, 64.5)


    def test09_0(self):
	"""Add Terminal Residue Test"""
	import types

	if (recentre_on_read_pdb() == 0):
		set_recentre_on_read_pdb(1)
	if (have_test_skip):
		self.skipIf(not type(terminal_residue_test_pdb) is StringType, 
			    "%s does not exist - skipping test" %terminal_residue_test_pdb)
		self.skipIf(not os.path.isfile(terminal_residue_test_pdb),
			    "%s does not exist - skipping test" %terminal_residue_test_pdb)

		# OK, file exists
		imol = read_pdb(terminal_residue_test_pdb)
		self.skipIf(not valid_model_molecule_qm(imol), 
			"%s bad pdb read - skipping test" %terminal_residue_test_pdb)
	else:
		if (not type(terminal_residue_test_pdb) is StringType): 
			print "%s does not exist - skipping test (actually passing!)" %terminal_residue_test_pdb
			skipped_tests.append("Add Terminal Residue Test")
			return
		if (not os.path.isfile(terminal_residue_test_pdb)):
			print "%s does not exist - skipping test (actually passing!)" %terminal_residue_test_pdb
			skipped_tests.append("Add Terminal Residue Test")
			return

		# OK, file exists
		imol = read_pdb(terminal_residue_test_pdb)
		if (not valid_model_molecule_qm(imol)):
			print "%s bad pdb read - skipping test (actually passing!)" %terminal_residue_test_pdb
			skipped_tests.append("Add Terminal Residue Test")
			return

	add_terminal_residue(imol, "A", 1, "ALA", 1)
	write_pdb_file(imol, "regression-test-terminal-residue.pdb")
	# where did that extra residue go?
	# 
	# Let's copy a fragment from imol and go to
	# the centre of that fragment, and check where
	# we are and compare it to where we expected
	# to be.
	#
	# we shall make sure that we recentre on read pdb (may be overwritten
	# in preferences!!!
	new_mol = new_molecule_by_atom_selection(imol, "//A/0")
	self.failUnless(valid_model_molecule_qm(new_mol))
	move_molecule_here(new_mol)
	# not sure we want to move the molecule to the screen centre?!
	#set_go_to_atom_molecule(new_mol)
	#set_go_to_atom_chain_residue_atom_name("A", 0, " CA ")
	rc = rotation_centre()
	ls = [45.6, 15.8, 11.8]
	r = sum([rc[i] - ls[i] for i in range(len(rc))])
	#print "BL DEBUG:: r and imol is", r, imol
	
	self.failIf(r > 0.66, "Bad placement of terminal residue")


    def test10_0(self):
	    """Select by Sphere"""

	    global imol_rnase
	    imol_sphere = new_molecule_by_sphere_selection(imol_rnase, 
							   24.6114959716797, 24.8355808258057, 7.43978214263916,
							   3.6)

	    self.failUnless(valid_model_molecule_qm(imol_sphere), "Bad sphere molecule")
	    n_atoms = 0
	    for chain_id in chain_ids(imol_sphere):
		    # add the number of atoms in the chains
		    n_residues = chain_n_residues(chain_id, imol_sphere)
		    print "   Sphere mol: there are %s residues in chain %s" %(n_residues, chain_id)
		    for serial_number in range(n_residues):
			    res_name = resname_from_serial_number(imol_sphere, chain_id, serial_number)
			    res_no   = seqnum_from_serial_number(imol_sphere, chain_id, serial_number)
			    ins_code = insertion_code_from_serial_number(imol_sphere, chain_id, serial_number)
			    residue_atoms_info = residue_info(imol_sphere, chain_id, res_no, ins_code)
			    n_atoms += len(residue_atoms_info)
			
	    print "Found %s sphere atoms" %n_atoms
	    self.failUnlessEqual(n_atoms, 20)
			

    def test11_0(self):
	"""Test Views"""
	view_number = add_view([32.0488, 21.6531, 13.7343],
			       [-0.12784, -0.491866, -0.702983, -0.497535],
			       20.3661,
			       "B11 View")
	go_to_view_number(view_number, 1)
	# test for something??


    def test12_0(self):
	"""Label Atoms and Delete"""

	imol_frag = new_molecule_by_atom_selection(imol_rnase, "//B/10-12")
	set_rotation_centre(31.464, 21.413, 14.824)
	map(lambda n: label_all_atoms_in_residue(imol_frag, "B", n, ""), [10, 11, 12])
	rotate_y_scene(rotate_n_frames(200), 0.1)
	delete_residue(imol_frag, "B", 10, "")
	delete_residue(imol_frag, "B", 11, "")
	delete_residue(imol_frag, "B", 12, "")
	rotate_y_scene(rotate_n_frames(200), 0.1)
	# ???? what do we sheck for?


    def test13_0(self):
	    """Rotamer outliers"""

	    # pre-setup so that residues 1 and 2 are not rotamers "t" but 3
	    # to 8 are (1 and 2 are more than 40 degrees away). 9-16 are
	    # other residues and rotamers.
	    imol_rotamers = read_pdb(os.path.join(unittest_data_dir, "rotamer-test-fragment.pdb"))
				     
	    rotamer_anal = rotamer_graphs(imol_rotamers)
	    self.failUnless(type(rotamer_anal) is ListType)
	    self.failUnlessEqual(len(rotamer_anal), 14)
	    a_1 = rotamer_anal[0]
	    a_2 = rotamer_anal[1]
	    a_last = rotamer_anal[len(rotamer_anal)-1]

	    anal_str_a1 = a_1[-1]
	    anal_str_a2 = a_2[-1]
	    anal_str_a3 = a_last[-1]

	    # with new Molprobity rotamer probabilities,
	    # residues 1 and 2 are no longer "not recognised"
	    # they are in fact, just low probabilites.
	    # 
	    self.failUnless((anal_str_a1 == "VAL" and
			     anal_str_a2 == "VAL" and
			     anal_str_a3 == "Missing Atoms"),
			    "  failure rotamer test: %s %s %s" %(a_1, a_2, a_last))

	    # Now we test that the probabilites of the
	    # rotamer is correct:
	    pr_1 = rotamer_anal[0][3]
	    pr_2 = rotamer_anal[1][3]
	    self.failUnless((pr_1 < 0.3 and pr_1 > 0.0), "Failed rotamer outlier test for residue 1")
	    self.failUnless((pr_2 < 0.3 and pr_2 > 0.0), "Failed rotamer outlier test for residue 2")

 
    # Don't reset the occupancies of the other parts of the residue
    # on regularization using alt confs
    #
    def test14_0(self):
	    """Alt Conf Occ Sum Reset"""

	    def get_occ_sum(imol_frag):
		    
		    def occ(att_atom_name, att_alt_conf, atom_ls):

			    self.failUnless(type(atom_ls) is ListType) # repetition??
			    for atom in atom_ls:
				    atom_name = atom[0][0]
				    alt_conf = atom[0][1]
				    occ = atom[1][0]
				    if ((atom_name == att_atom_name) and
					(alt_conf == att_alt_conf)):
					    print "   For atom %s %s returning occ %s" %(att_atom_name, att_alt_conf, occ)
					    return occ

		    # get_occ_sum body
		    atom_ls = residue_info(imol_frag, "X", 15, "")
		    self.failUnless(type(atom_ls) is ListType)
		    return sum([occ(" CE ", "A", atom_ls), occ(" CE ", "B", atom_ls),
				occ(" NZ ", "A", atom_ls), occ(" NZ ", "B", atom_ls)])

	    # main body
	    #
	    imol_fragment = read_pdb(os.path.join(unittest_data_dir, "res098.pdb"))

	    self.failUnless(valid_model_molecule_qm(imol_fragment), "bad molecule for reading coords in Alt Conf Occ test")
	    occ_sum_pre = get_occ_sum(imol_fragment)

	    replace_state = refinement_immediate_replacement_state()
	    set_refinement_immediate_replacement(1)
	    regularize_zone(imol_fragment, "X", 15, 15, "A")
	    accept_regularizement()
	    if (replace_state == 0):
		    set_refinement_immediate_replacement(0)

	    occ_sum_post = get_occ_sum(imol_fragment)
	    self.failUnlessAlmostEqual(occ_sum_pre, occ_sum_post, 1, "   test for closeness: %s %s" %(occ_sum_pre, occ_sum_post))


    # This test we expect to fail until the CISPEP correction code is in
    # place (using mmdb-1.10+).
    # 
    def test15_0(self):
	    """Correction of CISPEP test"""

	    # In this test the cis-pep-12A has indeed a CIS pep and has been
	    # refined with refmac and is correctly annotated.  There was 4 on
	    # read.  There should be 3 after we fix it and write it out.
	    # 
	    # Here we sythesise user actions:
	    # 1) CIS->TRANS on the residue 
	    # 2) convert leading residue to a ALA
	    # 3) mutate the leading residue and auto-fit it to correct the chiral
	    #    volume error :)

	    chain_id = "A"
	    resno = 11
	    ins_code = ""

	    cis_pep_mol = read_pdb(os.path.join(unittest_data_dir, "tutorial-modern-cis-pep-12A_refmac0.pdb"))
	    if (have_test_skip):
		    self.skipIf(cis_pep_mol < 0, 
				"skipping CIS test as problem on read pdb")
	    else:
		if (cis_pep_mol < 0): 
			print "skipping CIS test (actually passing!) as problem on read pdb"
			skipped_tests.append("Correction of CISPEP test")
			return

	    view_number = add_view([63.455, 11.764, 1.268],
				   [-0.760536, -0.0910907, 0.118259, 0.631906],
				   15.7374,
				   "CIS-TRANS cispep View")
	    go_to_view_number(view_number, 1)

	    cis_trans_convert(cis_pep_mol, chain_id, resno, ins_code)
	    pepflip(cis_pep_mol, chain_id, resno, ins_code)
	    res_type = residue_name(cis_pep_mol, chain_id, resno, ins_code)
	    self.failUnless(type(res_type) is StringType)
	    mutate(cis_pep_mol, chain_id, resno, "", "GLY")
	    with_auto_accept(
		    [refine_zone, cis_pep_mol, chain_id, resno, (resno + 1), ""],
		    [accept_regularizement],
		    [mutate, cis_pep_mol, chain_id, resno, "", res_type],
		    [auto_fit_best_rotamer, resno, "", ins_code, chain_id, cis_pep_mol,
		     imol_refinement_map(), 1, 1],
		    [refine_zone, cis_pep_mol, chain_id, resno, (resno + 1), ""],
		    [accept_regularizement]
		    )
	    
	    tmp_file = "tmp-fixed-cis.pdb"
	    write_pdb_file(cis_pep_mol, tmp_file)
	    # assuming we do not always have grep we extract information with
	    # a python funcn
	    # first arg: search string
	    # second arg: filename
	    def grep_to_list(pattern_str, filen):
		    import re
		    ret = []
		    go = False
		    try:
			    fin = open(filen, 'r')
			    lines = fin.readlines()
			    fin.close()
			    go = True
		    except IOError:
			    print "file no found"
		    if go:
			    pattern = re.compile(pattern_str)
			    for line in lines:
				    match = pattern.search(line)
				    if match:
					    ret.append(line)
		    return ret
			    
	    o = grep_to_list("CISPEP", tmp_file)
	    self.failUnlessEqual(len(o), 3)
	    #txt_str = "CISPEPs: "
	    #for cis in o:
		#    txt_str += cis
	    #print txt_str
		
	    # self.failUnlessEqual("4", parts)   # this temporarily until MMDB works properly
	    #self.failUnlessEqual("3", parts)   # this is once MMDB works properly with CIS
	    

    def test15_1(self):
	    """Refine Zone with Alt conf"""

	    imol = unittest_pdb("tutorial-modern.pdb")
	    mtz_file_name = (os.path.join(unittest_data_dir, "rnasa-1.8-all_refmac1.mtz"))
	    imol_map = make_and_draw_map(mtz_file_name, "FWT", "PHWT", "", 0, 0)

	    set_imol_refinement_map(imol_map)
	    at_1 = get_atom(imol, "B", 72, " SG ", "B")
	    with_auto_accept([refine_zone, imol, "B", 72, 72, "B"])
	    at_2 = get_atom(imol, "B", 72, " SG ", "B")
	    d = bond_length_from_atoms(at_1, at_2)
	    # the atom should move in the refinement
	    print "   refined moved: d=", d
	    self.failIf(d < 0.4, "   refined atom failed to move: d=%s" %d)
	    

    def test16_0(self):
	    """Rigid Body Refine Alt Conf Waters"""

	    imol_alt_conf_waters = read_pdb(os.path.join(unittest_data_dir, "alt-conf-waters.pdb"))

	    rep_state = refinement_immediate_replacement_state()
	    set_refinement_immediate_replacement(1)
	    refine_zone(imol_alt_conf_waters, "D", 71, 71, "A")
	    accept_regularizement()
	    refine_zone(imol_alt_conf_waters, "D", 2, 2, "")
	    accept_regularizement()
	    set_refinement_immediate_replacement(rep_state)
	    # what do we check for?!

	    
    def test17_0(self):
	    """Setting multiple atom attributes"""

	    global imol_rnase
	    self.failUnless(valid_model_molecule_qm(imol_rnase), "   Error invalid imol-rnase")

	    x_test_val = 2.1
	    y_test_val = 2.2
	    z_test_val = 2.3
	    o_test_val = 0.5
	    b_test_val = 44.4
	    ls = [[imol_rnase, "A", 2, "", " CA ", "", "x", x_test_val],
		  [imol_rnase, "A", 2, "", " CA ", "", "y", y_test_val],
		  [imol_rnase, "A", 2, "", " CA ", "", "z", z_test_val],
		  [imol_rnase, "A", 2, "", " CA ", "", "occ", o_test_val],
		  [imol_rnase, "A", 2, "", " CA ", "", "b", b_test_val]]

	    set_atom_attributes(ls)
	    atom_ls = residue_info(imol_rnase, "A", 2, "")
	    self.failIf(atom_ls == [])
	    for atom in atom_ls:
		    atom_name = atom[0][0]
		    if (atom_name == " CA "):
			    x = atom[2][0]
			    y = atom[2][1]
			    z = atom[2][2]
			    occ = atom[1][0]
			    b = atom[1][1]
			    self.failUnlessAlmostEqual(x, x_test_val, 1, "Error in setting multiple atom: These are not close %s %s" %(x, x_test_val))
			    self.failUnlessAlmostEqual(y, y_test_val, 1, "Error in setting multiple atom: These are not close %s %s" %(y, y_test_val))
			    self.failUnlessAlmostEqual(z, z_test_val, 1, "Error in setting multiple atom: These are not close %s %s" %(z, z_test_val))
			    self.failUnlessAlmostEqual(occ, o_test_val, 1, "Error in setting multiple atom: These are not close %s %s" %(occ, o_test_val))
			    self.failUnlessAlmostEqual(b, b_test_val, 1, "Error in setting multiple atom: These are not close %s %s" %(b, b_test_val))


    def test18_0(self):
	    """Tweak Alt Confs on Active Residue"""
	    
	    global imol_rnase
	    # did it reset?
	    def matches_alt_conf_qm(imol, chain_id, resno, inscode, atom_name_ref, alt_conf_ref):
		    atom_ls = residue_info(imol, chain_id, resno, inscode)
		    self.failIf(atom_ls ==[], "No atom list found - failing.")
		    for atom in atom_ls:
			    atom_name = atom[0][0]
			    alt_conf = atom[0][1]
			    if ((atom_name == atom_name_ref) and (alt_conf == alt_conf_ref)):
				    return True
		    return False

	    # main line
	    chain_id = "B"
	    resno = 58
	    inscode = ""
	    atom_name = " CB "
	    new_alt_conf_id = "B"
	    set_go_to_atom_molecule(imol_rnase)
	    set_go_to_atom_chain_residue_atom_name(chain_id, resno, " CA ")
	    set_atom_string_attribute(imol_rnase, chain_id, resno, inscode,
				      atom_name, "", "alt-conf", new_alt_conf_id)
	    self.failUnless(matches_alt_conf_qm(imol_rnase, chain_id, resno, inscode,
						atom_name, new_alt_conf_id),
			    "   No matching pre CB altconfed - failing.")
	    sanitise_alt_confs_in_residue(imol_rnase, chain_id, resno, inscode)
	    self.failUnless(matches_alt_conf_qm(imol_rnase, chain_id, resno, inscode, atom_name, ""),
			    "   No matching post CB (unaltconfed) - failing.")


    def test19_0(self):
	    """Libcif horne"""

	    if (have_test_skip):
		    self.skipIf(not os.path.isfile(horne_pdb), "file %s not found - skipping test" %horne_pdb)
		    self.skipIf(not os.path.isfile(horne_cif), "file %s not found - skipping test" %horne_cif)
		    self.skipIf(not os.path.isfile(horne_works_cif), "file %s not found - skipping test" %horne_works_cif)
	    else:
		    if (not os.path.isfile(horne_pdb)):
			    print "file %s not found - skipping test (actually passing!)" %horne_pdb
			    skipped_tests.append("Libcif horne")
			    return
		    if (not os.path.isfile(horne_cif)):
			    print "file %s not found - skipping test (actually passing!)" %horne_cif
			    skipped_tests.append("Libcif horne")
			    return
		    if (not os.path.isfile(horne_works_cif)):
			    print "file %s not found - skipping test (actually passing!)" %horne_works_cif
			    skipped_tests.append("Libcif horne")
			    return
		    
	    imol = read_pdb(horne_pdb)
	    self.failUnless(valid_model_molecule_qm(imol),"bad molecule number %i" %imol)

	    read_cif_dictionary(horne_works_cif)
	    with_auto_accept(
		    [regularize_zone, imol, "A", 1, 1, ""]
		    )
	    print "\n \n \n \n"
	    read_cif_dictionary(horne_cif)
	    with_auto_accept(
		    [regularize_zone, imol, "A", 1, 1, ""]
		    )
	    cent = molecule_centre(imol)
	    self.failUnless(cent[0] < 2000, "Position fail 2000 test: %s in %s" %(cent[0], cent))


    def test20_0(self):
	    """Refmac Parameters Storage"""

	    arg_list = [rnase_mtz, "/RNASE3GMP/COMPLEX/FWT", "/RNASE3GMP/COMPLEX/PHWT", "", 0, 0, 1, 
			"/RNASE3GMP/COMPLEX/FGMP18", "/RNASE3GMP/COMPLEX/SIGFGMP18",
			"/RNASE/NATIVE/FreeR_flag", 1]
	    imol = make_and_draw_map_with_refmac_params(*arg_list)
	    
	    self.failUnless(valid_map_molecule_qm(imol),"  Can't get valid refmac map molecule from %s" %rnase_mtz)

	    refmac_params = refmac_parameters(imol)
	    refmac_params[0] = os.path.normpath(refmac_params[0])

	    self.failUnlessEqual(refmac_params, arg_list, "        non matching refmac params")


    def test21_0(self):
	    """The position of the oxygen after a mutation"""

	    import operator
	    # Return the o-pos (can be False) and the number of atoms read.
	    #
	    def get_o_pos(pdb_file):
		    fin = open(pdb_file, 'r')
		    lines = fin.readlines()
		    fin.close
		    n_atoms = 0
		    o_pos = False
		    for line in lines:
			    atom_name = line[12:16]
			    if (line[0:4] == "ATOM"):
				    n_atoms +=1
			    if (atom_name == " O  "):
				    o_pos = n_atoms
			    
		    return o_pos, n_atoms

	    # main body
	    #
	    hist_pdb_name = "his.pdb"

	    imol = read_pdb(os.path.join(unittest_data_dir, "val.pdb"))
	    self.failUnless(valid_model_molecule_qm(imol), "   failed to read file val.pdb")
	    mutate_state = mutate(imol, "C", 3, "", "HIS")
	    self.failUnlessEqual(mutate_state, 1, "   failure in mutate function")
	    write_pdb_file(imol, hist_pdb_name)
	    self.failUnless(os.path.isfile(hist_pdb_name), "   file not found: %s" %hist_pdb_name)
	    
	    o_pos, n_atoms = get_o_pos(hist_pdb_name)
	    self.failUnlessEqual(n_atoms, 10, "   found %s atoms (not 10)" %n_atoms)
	    self.failUnless(operator.isNumberType(o_pos), "   Oxygen O position not found")
	    self.failUnlessEqual(o_pos, 4,"   found O atom at %s (not 4)" %o_pos) 

	    
    def test22_0(self):
	    """Deleting (non-existing) Alt conf and Go To Atom [JED]"""

	    global imol_rnase
	    # alt conf "A" does not exist in this residue:
	    delete_residue_with_altconf(imol_rnase, "A", 88, "", "A")
	    # to activate the bug, we need to search over all atoms
	    active_residue()
	    # test for what?? (no crash??)


    def test23_0(self):
	    """Mask and difference map"""

	    def get_ca_coords(imol_model, resno_1, resno_2):
		    coords = []
		    for resno in range(resno_1, resno_2+1):
			    atom_info = atom_specs(imol_model, "A", resno, "", " CA ", "")
			    co = atom_info[3:6]
			    coords.append(co)
		    return coords

	    d_1 = difference_map(-1, 2, -9)
	    d_2 = difference_map(2, -1, -9)

	    self.failUnlessEqual(d_1, -1, "failed on bad d_1")
	    self.failUnlessEqual(d_2, -1, "failed on bad d_1")

	    imol_map_nrml_res = make_and_draw_map(rnase_mtz, "FWT", "PHWT", "", 0, 0)
	    prev_sampling_rate = get_map_sampling_rate()
	    nov_1 = set_map_sampling_rate(2.2)
	    imol_map_high_res = make_and_draw_map(rnase_mtz, "FWT", "PHWT", "", 0, 0)
	    nov_2 = set_map_sampling_rate(prev_sampling_rate)
	    imol_model = handle_read_draw_molecule_with_recentre(rnase_pdb, 0)

	    imol_masked = mask_map_by_atom_selection(imol_map_nrml_res, imol_model,
						     "//A/1-10", 0)

	    self.failUnless(valid_map_molecule_qm(imol_map_nrml_res))

	    # check that imol-map-nrml-res is good here by selecting
	    # regions where the density should be positive. The masked
	    # region should be 0.0
	    #
	    high_pts = get_ca_coords(imol_model, 20, 30)
	    low_pts  = get_ca_coords(imol_model,  1, 10)

	    high_values = map(lambda pt: density_at_point(imol_masked, *pt), high_pts)
	    low_values = map(lambda pt: density_at_point(imol_masked, *pt), low_pts)

	    print "high-values: %s  low values: %s" %(high_values, low_values)

	    self.failUnless((sum(low_values) < 0.000001), "Bad low values")
	    
	    self.failUnless((sum(high_values) > 5), "Bad high values")

	    diff_map = difference_map(imol_masked, imol_map_high_res, 1.0)
	    self.failUnless(valid_map_molecule_qm(diff_map), "failure to make good difference map")

	    # now in diff-map low pt should have high values and high
	    # pts should have low values

	    diff_high_values = map(lambda pt: density_at_point(diff_map, *pt), high_pts)
	    diff_low_values = map(lambda pt: density_at_point(diff_map, *pt), low_pts)
	    
	    print "diff-high-values: %s  diff-low-values: %s" %(diff_high_values, diff_low_values)

	    self.failUnless((sum(diff_high_values) < 0.03), "Bad diff low values")
	    
	    self.failUnless((sum(diff_low_values) < -5), "Bad diff high values")
	    

    def test24_0(self):
	    """Make a glycosidic linkage"""

	    carbo = "multi-carbo-coot-2.pdb"
	    imol = unittest_pdb(carbo)

	    if (have_test_skip):
		    self.skipIf(not valid_model_molecule_qm(imol), "file %s not found, skipping test" %carbo)
	    else:
		    if (not valid_model_molecule_qm(imol)):
			print "file %s not found, skipping test (actually passing!)" %carbo
			skipped_tests.append("Make a glycosidic linkage")
			return
		    
	    atom_1 = get_atom(imol, "A", 1, " O4 ")
	    atom_2 = get_atom(imol, "A", 2, " C1 ")

	    print "bond-length: ", bond_length(atom_1[2], atom_2[2])

	    s = dragged_refinement_steps_per_frame()
	    set_dragged_refinement_steps_per_frame(300)
	    with_auto_accept([regularize_zone, imol, "A", 1, 2, ""])
	    set_dragged_refinement_steps_per_frame(s)

	    atom_1 = get_atom(imol, "A", 1, " O4 ")
	    atom_2 = get_atom(imol, "A", 2, " C1 ")

	    print "bond-length: ", bond_length(atom_1[2], atom_2[2])

    def test25_0(self):
	    """Test for flying hydrogens on undo"""

	    from types import ListType
	    
	    imol = unittest_pdb("monomer-VAL.pdb")

	    self.failUnless(valid_model_molecule_qm(imol), "   Failure to read monomer-VAL.pdb")

	    with_auto_accept([regularize_zone, imol, "A", 1, 1, ""])
	    set_undo_molecule(imol)
	    apply_undo()
	    with_auto_accept([regularize_zone, imol, "A", 1, 1, ""])

	    atom_1 = get_atom(imol, "A", 1, "HG11")
	    atom_2 = get_atom(imol, "A", 1, " CG1")

	    self.failUnless((type(atom_1) is ListType) and
			    (type(atom_2) is ListType) ,
			    "   flying hydrogen failure, atoms: %s %s" %(atom_1, atom_2))
	    self.failUnless(bond_length_within_tolerance_qm(atom_1, atom_2, 0.96, 0.02),
			    "   flying hydrogen failure, bond length %s, should be 0.96" %bond_length_from_atoms(atom_1, atom_2))


    def test25_1(self):
	    """Test for mangling of hydrogen names from a PDB v 3.0"""

	    imol = unittest_pdb("3ins-6B-3.0.pdb")
	    self.failUnless(valid_model_molecule_qm(imol), "Bad read of unittest test pdb: 3ins-6B-3.0.pdb")

	    with_auto_accept([regularize_zone, imol, "B", 6, 6, ""])
	    atom_pairs = [["HD11", " CD1"],
			  ["HD12", " CD1"],
			  ["HD13", " CD1"],
			  ["HD21", " CD2"],
			  ["HD22", " CD2"],
			  ["HD23", " CD2"]]
	    atoms_1 = map(lambda pair: get_atom(imol, "B", 6, pair[0]), atom_pairs)
	    atoms_2 = map(lambda pair: get_atom(imol, "B", 6, pair[1]), atom_pairs)
	    #all_true = map(lambda atom_1, atom_2: bond_length_within_tolerance_qm(atom_1, atom_2, 0.96, 0.02), atoms_1, atoms_2)
	    self.failUnless(all(map(lambda atom_1, atom_2: bond_length_within_tolerance_qm(atom_1, atom_2, 0.96, 0.02), atoms_1, atoms_2)), "Hydrogen names mangled from PDB")


    def test26_0(self):
	    """Update monomer restraints"""

	    atom_pair = [" CB ", " CG "]
	    m = monomer_restraints("TYR")

	    n = strip_bond_from_restraints(atom_pair, m)
	    set_monomer_restraints("TYR", n)

	    imol = new_molecule_by_atom_selection(imol_rnase, "//A/30")

	    atom_1 = get_atom(imol, "A", 30, " CB ")
	    atom_2 = get_atom(imol, "A", 30, " CG ")

	    with_auto_accept([refine_zone, imol, "A", 30, 30, ""])

	    atom_1 = get_atom(imol, "A", 30, " CB ")
	    atom_2 = get_atom(imol, "A", 30, " CG ")
	    print "  Bond-length: ", bond_length(atom_1[2], atom_2[2])

	    self.failUnless(bond_length_within_tolerance_qm(atom_1, atom_2, 2.8, 0.6),
			    "fail 2.8 tolerance test")
	    print "pass intermediate 2.8 tolerance test"
	    set_monomer_restraints("TYR", m)

	    with_auto_accept([refine_zone, imol, "A", 30, 30, ""])
	    
	    atom_1 = get_atom(imol, "A", 30, " CB ")
	    atom_2 = get_atom(imol, "A", 30, " CG ")

	    post_set_plane_restraints = monomer_restraints("TYR")["_chem_comp_plane_atom"]

	    atom = post_set_plane_restraints[0][1][0]
	    self.failUnless(atom == " CB ", "FAIL plane atom %s" %atom)
	    print "  OK plane atom ", atom

	    print "  Bond-length: ", bond_length(atom_1[2], atom_2[2])
	    self.failUnless(bond_length_within_tolerance_qm(atom_1, atom_2, 1.512, 0.04),
			    "fail 1.512 tolerance test")
	    

    def test27_0(self):
	    """Change Chain IDs and Chain Sorting"""

	    imol = copy_molecule(imol_rnase)
	    change_chain_id(imol, "A", "D", 0,  0,  0)
	    change_chain_id(imol, "B", "E", 1, 80, 90)
	    change_chain_id(imol, "B", "F", 1, 70, 80)
	    change_chain_id(imol, "B", "G", 1, 60, 70)
	    change_chain_id(imol, "B", "J", 1, 50, 59)
	    change_chain_id(imol, "B", "L", 1, 40, 49)
	    change_chain_id(imol, "B", "N", 1, 30, 38)
	    change_chain_id(imol, "B", "Z", 1, 20, 28)


    def test28_0(self):
	    """Replace Fragment"""

	    atom_sel_str = "//A70-80"
	    imol_rnase_copy = copy_molecule(imol_rnase)
	    imol = new_molecule_by_atom_selection(imol_rnase, atom_sel_str)
	    self.failUnless(valid_model_molecule_qm(imol))

	    translate_molecule_by(imol, 11, 12, 13)
	    reference_res = residue_info(imol_rnase, "A", 75, "")
	    moved_res     = residue_info(imol,       "A", 75, "")
	    test_res     = residue_info(imol,       "A", 74, "")

	    replace_fragment(imol_rnase_copy, imol, atom_sel_str)
	    replaced_res = residue_info(imol_rnase_copy, "A", 75, "")

	    # now replace-res should match reference-res.
	    # the atoms of moved-res should be 20+ A away from both.

	    self.failUnless(all(map(atoms_match_qm, moved_res, replaced_res)),
			    "   moved-res and replaced-res do not match")

	    self.failIf(all(map(atoms_match_qm, moved_res, reference_res)),
			"   fail - moved-res and replaced-res Match!")
	    
	    print "distances: ", map(atom_distance, reference_res, replaced_res)
	    self.failUnless(all(map(lambda d: d > 20, map(atom_distance, reference_res, replaced_res))))



    def test29_0(self):
	    """Residues in Region of Residue"""

	    for dist, n_neighbours in zip([4,0], [6,0]):
		    rs = residues_near_residue(imol_rnase, ["A", 40, ""], dist)
		    self.failUnless(len(rs) == n_neighbours, "wrong number of neighbours %s %s" %(len(rs), rs))
		    print "found %s neighbours %s" %(len(rs), rs)
	    

    def test30_0(self):
	    """Empty molecule on type selection"""

	    global imol_rnase
	    imol1 = new_molecule_by_residue_type_selection(imol_rnase, "TRP")
	    self.failUnlessEqual(imol1, -1, "failed on empty selection 1 gives not imol -1")
	    imol2 = new_molecule_by_residue_type_selection(imol_rnase, "TRP")
	    self.failUnlessEqual(imol2, -1, "failed on empty selection 2 gives not imol -1")


    def test31_0(self):
	    """Set Rotamer"""

	    chain_id = "A"
	    resno = 51

	    n_rot = n_rotamers(-1, "ZZ", 45, "")
	    self.failUnless(n_rot == -1)

	    n_rot = n_rotamers(imol_rnase, "Z", 45, "")
	    self.failUnless(n_rot == -1)

	    residue_pre = residue_info(imol_rnase, chain_id, resno, "")

	    # note that the rotamer number is 0-indexed (unlike the rotamer
	    # number dialog)
	    set_residue_to_rotamer_number(imol_rnase, chain_id, resno, "", 1)
	    
	    residue_post = residue_info(imol_rnase, chain_id, resno, "")

	    self.failUnless(len(residue_pre) == len(residue_post))

	    # average dist should be > 0.1 and < 0.3.
	    #
	    dists = map(atom_distance, residue_pre, residue_post)
	    #print "BL DEBUG:: dists", dists
	    self.failUnless(all(map(lambda d: d <= 0.6, dists)))
	    self.failUnless(all(map(lambda d: d >= 0.0, dists)))
	    

    def test32_0(self):
	    """Align and mutate a model with deletions"""

	    def residue_in_molecule_qm(imol, chain_id, resno, ins_code):
		    r = residue_info(imol, chain_id, resno, ins_code)
		    if (r):
			    return True
		    else:
			    return False

	    # in this PDB file 60 and 61 have been deleted. Relative to the
	    # where we want to be (tutorial-modern.pdb, say) 62 to 93 have
	    # been moved to 60 to 91
	    #
	    imol = unittest_pdb("rnase-A-needs-an-insertion.pdb")
	    if (have_test_skip):
		    self.skipIf(not os.path.isfile(rnase_seq), "file %s not found, skipping test" %rnase_seq)
	    else:
		    if (not os.path.isfile(rnase_seq)):
			print "file %s not found, skipping test (actually passing!)" %rnase_seq
			skipped_tests.append("Align and mutate a model with deletions")
			return
	    rnase_seq_string = file2string(rnase_seq)
	    self.failUnless(valid_model_molecule_qm(imol), "   Missing file rnase-A-needs-an-insertion.pdb")

	    align_and_mutate(imol, "A", rnase_seq_string)
	    write_pdb_file(imol, "mutated.pdb")

	    ls = [[False, imol, "A",  1, ""],
		  [True,  imol, "A",  4, ""],
		  [True,  imol, "A", 59, ""],
		  [False, imol, "A", 60, ""],
		  [False, imol, "A", 61, ""],
		  [True,  imol, "A", 93, ""],
		  [False, imol, "A", 94, ""]]

	    def compare_res_spec(res_info):
		    residue_spec = res_info[1:len(res_info)]
		    expected_status = res_info[0]
		    print ":::::", residue_spec, residue_in_molecule_qm(*residue_spec), expected_status
		    if (residue_in_molecule_qm(*residue_spec) == expected_status):
			    return True
		    else:
			    return False

	    self.failUnless(all(map(lambda res: compare_res_spec(res), ls)))


    def test33_0(self):
	    """Read/write gz coordinate files"""

	    # this is mainly an mmdb test for windows

	    # first write a gz file
	    gz_state = write_pdb_file(imol_rnase, "rnase_zip_test.pdb.gz")
	    self.failIf(gz_state == 1)
	    self.failUnless(os.path.isfile("rnase_zip_test.pdb.gz"))
	    
	    # now unzip and read the file
	    gz_imol = handle_read_draw_molecule("rnase_zip_test.pdb.gz")
	    self.failUnless(valid_model_molecule_qm(gz_imol))
		  

    def test34_0(self):
	    """Autofit Rotamer on Residue with Insertion codes"""

	    # we need to check that H52 LEU and H53 GLY do not move and H52A does move

	    def centre_atoms(mat):
		    tm = transpose_mat(map(lambda x: x[2], mat))
		    centre = map(lambda ls: sum(ls)/len(ls), tm)
		    return centre

	    imol = unittest_pdb("pdb3hfl.ent")
	    mtz_file_name = os.path.join(unittest_data_dir, "3hfl_sigmaa.mtz")
	    imol_map = make_and_draw_map(mtz_file_name,
					 "2FOFCWT", "PH2FOFCWT", "", 0, 0)

	    self.failUnless(valid_map_molecule_qm(imol_map))

	    leu_atoms_1 = residue_info(imol, "H", 52, "")
	    leu_resname = residue_name(imol, "H", 52, "")
	    gly_atoms_1 = residue_info(imol, "H", 53, "")
	    gly_resname = residue_name(imol, "H", 53, "")
	    pro_atoms_1 = residue_info(imol, "H", 52, "A")
	    pro_resname = residue_name(imol, "H", 52, "A")

	    # First check that the residue names are correct
	    self.failUnless((leu_resname == "LEU" and
			     gly_resname == "GLY" and
			     pro_resname == "PRO"),
			    "  failure of residues names: %s %s %s" \
			    %(leu_resname, gly_resname, pro_resname))

	    # OK, what are the centre points of these residues?
	    leu_centre_1 = centre_atoms(leu_atoms_1)
	    gly_centre_1 = centre_atoms(gly_atoms_1)
	    pro_centre_1 = centre_atoms(pro_atoms_1)

	    auto_fit_best_rotamer(52, "", "A", "H", imol, imol_map, 0, 0)

	    # OK, what are the centre points of these residues?
	    leu_atoms_2  = residue_info(imol, "H", 52, "")
	    gly_atoms_2  = residue_info(imol, "H", 53, "")
	    pro_atoms_2  = residue_info(imol, "H", 52, "A")
	    leu_centre_2 = centre_atoms(leu_atoms_2)
	    gly_centre_2 = centre_atoms(gly_atoms_2)
	    pro_centre_2 = centre_atoms(pro_atoms_2)

	    d_leu = pos_diff(leu_centre_1, leu_centre_2)
	    d_gly = pos_diff(gly_centre_1, gly_centre_2)
	    d_pro = pos_diff(pro_centre_1, pro_centre_2)

	    self.failUnlessAlmostEqual(d_leu, 0.0, 1,
				       "   Failure: LEU 52 moved")
	    
	    self.failUnlessAlmostEqual(d_gly, 0.0, 1,
				       "   Failure: GLY 53 moved")
	    
	    self.failIf(d_pro < 0.05, "   Failure: PRO 52A not moved enough")

	    # rotamer 4 is out of range.
	    set_residue_to_rotamer_number(imol, "H", 52, "A", 4) # crash!!?