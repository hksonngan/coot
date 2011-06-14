cprodrg = "cprodrg"
# we cannot use full path as cprodrg is spawning refmac...
#cprodrg = "c:/Programs/CCP4-Packages/ccp4-6.1.13/bin/cprodrg.exe"

# we need $CLIBD to run prodrg (for prodrg.param), so check for it:
if not os.getenv("CLIBD"):
    print "BL INFO:: potential problem to run prodrg (no CLIBD set)"
    print "          try to fix this"
    bin_dir = os.path.dirname(cprodrg)
    base_dir = os.path.dirname(bin_dir)
    clibd = os.path.join(base_dir, "lib", "data")
    prodrg_params = os.path.join(clibd, "prodrg.param")
    if os.path.isfile(prodrg_params):
        os.environ["CLIBD"] = clibd
        print "BL INFO:: found prodrg.param"
        print "          and set CLIBD to", clibd
    else:
        print "BL ERROR:: no prodrg.param available"
        print "searched in ", clidb
        print "Sorry, module not available"
        # should load rest?
        # i.e. stop here?
    

# these are the files that mdl-latest-time and sbase time-out functions
# look at.
# 
# if there is a prodrg-xyzin set the current-time to its mtime, else False
#
global prodrg_xyzin
global sbase_to_coot_tlc
prodrg_xyzin      = "prodrg-in.mdl"
sbase_to_coot_tlc = ".sbase-to-coot-comp-id"

# this is for BL win machine
home = os.getenv("HOME")
if (not home and is_windows()):
    home = os.getenv("COOT_HOME")
if home:
    prodrg_xyzin      = os.path.join(home, "Projects",
                                     "build-xp-python", "lbg", "prodrg-in.mdl")
else:
    print "BL DEBUG:: buggery home is", home  # FIXME
sbase_to_coot_tlc = "../../build-xp-python/lbg/.sbase-to-coot-comp-id"

# this is latest!!!!
prodrg_xyzin      = "prodrg-in.mdl"
sbase_to_coot_tlc = ".sbase-to-coot-comp-id"

print "new prodrg_import.py"

def import_from_prodrg(minimize_mode):

    import operator
    global prodrg_xyzin

    # return True or False
    #
    def have_restraints_for_qm(res_name):
        restraints = monomer_restraints(res_name)
        return not restraints == False   # twisted but short

    # overlap the imol_ligand residue if there are restraints for the
    # reference residue/ligand.
    #
    # Don't overlap of the reference residue/ligand is not a het-group.
    #
    def overlap_ligands_maybe(imol_ligand, imol_ref, chain_id_ref, res_no_ref):

        # we don't want to overlap-ligands if there is no dictionary
        # for the residue to be matched to.
        res_name = residue_name(imol_ref, chain_id_ref, res_no_ref, "")
        restraints = monomer_restraints(res_name)
        if (not restraints):
            return False
        else:
            if not residue_has_hetatms_qm(imol_ref, chain_id_ref, res_no_ref, ""):
                return False
            else:
                overlap_ligands(imol_ligand, imol_ref, chain_id_ref, res_no_ref)
                return True

    # return the new molecule number.
    #
    def read_and_regularize(prodrg_xyzout):
        imol = handle_read_draw_molecule_and_move_molecule_here(prodrg_xyzout)
        # speed up the minisation (and then restore setting).
        # No need to put the refinement step stuff in auto accept!?
        s = dragged_refinement_steps_per_frame()
        set_dragged_refinement_steps_per_frame(500)
        with_auto_accept([regularize_residues, imol, [["", 1, ""]]])
        set_dragged_refinement_steps_per_frame(s)
        return imol

    # return the new molecule number
    # (only works with aa_ins_code of "")
    #
    def read_regularize_and_match_torsions(prodrg_xyzout,
                                           aa_imol, aa_chain_id, aa_res_no):
        imol = handle_read_draw_molecule_and_move_molecule_here(prodrg_xyzout)

        if (not have_restraints_for_qm(residue_name(aa_imol, aa_chain_id,
                                                    aa_res_no, ""))):
            return False
        else:
            overlap_ligands_maybe(imol, aa_imol, aa_chain_id, aa_res_no)
            
            # speed up the minisation (and then restore setting).
            # No need to put refinement steps in auto accept!?
            s = dragged_refinement_steps_per_frame()
            set_dragged_refinement_steps_per_frame(500)
            with_auto_accept([regularize_residues, imol, [["", 1, ""]]])
            # BL says:: we already checked for restraints!?
            if (have_restraints_for_qm(residue_name(aa_imol, aa_chain_id,
                                                    aa_res_no, ""))):
                match_ligand_torsions(imol, aa_imol, aa_chain_id, aa_res_no)
        return imol

    # main line
    #
    prodrg_dir = "coot-ccp4"
    res_name = "DRG"

    make_directory_maybe(prodrg_dir)
    prodrg_xyzout = os.path.join(prodrg_dir, "prodrg-" + res_name + ".pdb")
    prodrg_cif    = os.path.join(prodrg_dir, "prodrg-out.cif")
    prodrg_log    = os.path.join(prodrg_dir, "prodrg.log")

    # requires python >= 2.5 (shall we test?)
    mini_mode = "NO" if (minimize_mode == 'mini-no') else "PREP"
    # see if we have cprodrg
    if not (os.path.isfile(cprodrg) or
            command_in_path_qm(cprodrg)):
        info_dialog("BL INFO:: No cprodrg found")
    else:
        status = popen_command(cprodrg,
                               ["XYZIN",  prodrg_xyzin,
                                "XYZOUT", prodrg_xyzout,
                                "LIBOUT", prodrg_cif],
                               ["MINI " + mini_mode, "END"],
                               prodrg_log, True)
        if isinstance(status, int):
            if (status == 0):
                
                # OK, so here we read the PRODRG files and
                # manipulate them.  We presume that the active
                # residue is quite like the input ligand from
                # prodrg.
                #
                # Read in the lib and coord output of PRODRG.  Then
                # overlay the new ligand onto the active residue
                # (just so that we can see it approximately
                # oriented). Then match the torsions from the new
                # ligand to the those of the active residue.  Then
                # overlay again so that we have the best match.
                #
                # We want to see just one molecule with the protein
                # and the new ligand.
                # add_ligand_delete_residue_copy_molecule provides
                # that for us.  We just colour it and undisplay the
                # other molecules.
            
                read_cif_dictionary(prodrg_cif)

                # we do different things depending on whether or
                # not there is an active residue.  We need to test
                # for having an active residue here (currently we
                # presume that there is).
                # 
                # Similarly, if the aa_ins_code is non-null, let's
                # presume that we do not have an active residue.

                active_atom = active_residue()
                if ((not active_atom) or
                    (not active_atom[3] == "")):  # aa_ins_code

                    # then there is no active residue to match to
                    read_and_regularize(prodrg_xyzout)
                    # BL says:: maybe we should merge the ligand into
                    # the protein molecule?! But what is the protein?
                    # merge_molecules([active_atom[0]], imol)
                else:
                    # we have an active residue to match to
                    # BL says: not using active_atom!? Too confusing
                    aa_imol      = active_atom[0]
                    aa_chain_id  = active_atom[1]
                    aa_res_no    = active_atom[2]
                    aa_ins_code  = active_atom[3]
                    aa_atom_name = active_atom[4]
                    aa_alt_conf  = active_atom[5]

                    if not residue_is_close_to_screen_centre_qm(
                        aa_imol, aa_chain_id, aa_res_no, ""):

                        # not close, no overlap
                        #
                        read_and_regularize(prodrg_xyzout)
                    else:
                        # try overlap
                        # BL says:: this overwrites 
                        imol = read_regularize_and_match_torsions(prodrg_xyzout,
                                                                  aa_imol, aa_chain_id, aa_res_no)

                        overlapped_flag = overlap_ligands_maybe(imol, aa_imol,
                                                                aa_chain_id, aa_res_no)
            
                        if overlapped_flag:
                            set_mol_displayed(aa_imol, 0)
                            set_mol_active(aa_imol, 0)
                            col = get_molecule_bonds_colour_map_rotation(aa_imol)
                            new_col = col + 5 # a tiny amount
                            # new ligand specs, then "reference" ligand (to be deleted)
                            imol_replaced = add_ligand_delete_residue_copy_molecule(
                                imol, "", 1,
                                aa_imol, aa_chain_id, aa_res_no)


                            set_molecule_bonds_colour_map_rotation(imol_replaced, new_col)
                            set_mol_displayed(imol, 0)
                            set_mol_active(imol, 0)
                            graphics_draw()
                    return True
        # return False otherwise? FIXME

if (have_coot_python):
    if coot_python.main_menubar():
        menu = coot_menubar_menu("_Lidia")
        add_simple_coot_menu_menuitem(
            menu,
            "Import (using MINI PREP)",
            lambda func:
            # run prodrg, read its output files, and run regularisation
            # on the imported PDB file
            import_from_prodrg('mini-prep')
            )

        add_simple_coot_menu_menuitem(
            menu,
            "Import (no pre-minimisation)",
            lambda func:
            # run prodrg, read its output files, (no regularisation
            # on the imported PDB file)
            import_from_prodrg('mini-no')
            )

#        add_simple_coot_menu_menuitem(
#            menu,
#            "Export to LIDIA",
#            lambda func:
#            using_active_atom(prodrg_flat, "aa_imol", "aa_chain_id", "aa_res_no")
#            )

        add_simple_coot_menu_menuitem(
            menu,
            "View in LIDIA",
            lambda func:
            using_active_atom(fle_view,
                              "aa_imol", "aa_chain_id",
                              "aa_res_no", "aa_ins_code")
            )
        
        add_simple_coot_menu_menuitem(
            menu,
            "Load SBase monomer...",
            lambda func:
            generic_single_entry("Load SBase Monomer from three-letter-code: ",
                                 "",
                                 " Load ",
                                 lambda tlc:
                                 get_sbase_monomer(tlc))
            )

        # should be in rdkit file FIXME!
        add_simple_coot_menu_menuitem(
            menu,
            "FLEV this residue [RDKIT]",
            lambda func:
            using_active_atom(fle_view_with_rdkit,
                              "aa_imol", "aa_chain_id",
                              "aa_res_no", "aa_ins_code", 4.2)
            )

        
def get_mdl_latest_time(file_name):
    if not os.path.isfile(file_name):
        return False
    else:
        return os.stat(file_name).st_mtime

# globals should be in the beginning! FIXME
global mdl_latest_time
global sbase_transfer_latest_time
mdl_latest_time = get_mdl_latest_time(prodrg_xyzin)
sbase_transfer_latest_time = get_mdl_latest_time(sbase_to_coot_tlc)
# FIXME: this is not a proper name
def mdl_update_timeout_func():

    import operator
    global mdl_latest_time
    global sbase_transfer_latest_time
    
    mdl_now_time   = get_mdl_latest_time(prodrg_xyzin)
    sbase_now_time = get_mdl_latest_time(sbase_to_coot_tlc)

    # print "sbase_now_time %s    sbase_latest_time %s" %(sbase_now_time, sbase_transfer_latest_time)

    if (operator.isNumberType(mdl_now_time)):
        if operator.isNumberType(mdl_latest_time):
            if (mdl_now_time > mdl_latest_time):
                mdl_latest_time = mdl_now_time
                import_from_prodrg('mini-prep')

    if operator.isNumberType(sbase_transfer_latest_time):
        if operator.isNumberType(sbase_now_time):
            if (sbase_now_time > sbase_transfer_latest_time):
                sbase_transfer_latest_time = sbase_now_time
                try: # sort of check if file exists?
                    fin = open(sbase_to_coot_tlc, 'r')
                    tlc_symbol = fin.readline()  # need to read more? FIXME
                    fin.close()
                    imol = get_sbase_monomer(tlc_symbol)
                    if not valid_model_molecule_qm(imol):
                        print "failed to get SBase molecule for", tlc_symbol
                    else:
                        # it was read OK, do an overlap
                        using_active_atom(overlap_ligands, imol,
                                          "aa_imol", "aa_chain_id", "aa_res_no")
                except:
                    print "BL ERROR:: reading sbase file", sbase_to_coot_tlc
                
    return True # return value, keep running; FIXME:: how to stop?
gobject.timeout_add(500, mdl_update_timeout_func)

# return False (if fail) or a list of: the molecule number of the
# selected residue, the prodrg output mol file_name, the prodrg
# output pdb file_name
#
def prodrg_flat(imol_in, chain_id_in, res_no_in):
    """return False (if fail) or a list of: the molecule number of the
    selected residue, the prodrg output mol file_name, the prodrg
    output pdb file_name

    Keyword arguments:
    imol_in -- the molecule number
    chain_id_in -- chain id of the molecule
    res_no_in -- residue number ot the molecule
    
    """

    import operator
    import random

    selection_string = "//" + chain_id_in + "/" + str(res_no_in)
    imol = new_molecule_by_atom_selection(imol_in, selection_string)
    prodrg_input_file_name = os.path.join("coot-ccp4", "tmp-residue-for-prodrg.pdb")
    prodrg_output_mol_file = os.path.join("coot-ccp4", ".coot-to-lbg-mol")
    prodrg_output_pdb_file = os.path.join("coot-ccp4", ".coot-to-lbg-pdb")
    prodrg_output_lib_file = os.path.join("coot-ccp4", ".coot-to-lbg-lib")
    prodrg_log             = os.path.join("coot-ccp4", "tmp-prodrg-flat.log")
    set_mol_displayed(imol, 0)
    set_mol_active   (imol, 0)
    write_pdb_file(imol, prodrg_input_file_name)
    arg_list = ["XYZIN",  prodrg_input_file_name,
                "MOLOUT", prodrg_output_mol_file,
                "XYZOUT", prodrg_output_pdb_file,
                "LIBOUT", prodrg_output_lib_file]
    print "arg_list", arg_list
    status = popen_command(cprodrg,
                           arg_list,
                           ["COORDS BOTH", "MINI FLAT", "END"],
                           prodrg_log, True)
    # Does this make sense in python? status being number that is.
    # Maybe rather check for cprodrg exe. FIXME
    if not operator.isNumberType(status):
        info_dialog("Ooops: cprodrg not found?")
        return False
    else:
        if not (status == 0):
            # only for python >=2.5
            mess = "Something went wrong running cprodrg\n" + \
                   ("(quelle surprise)" if random.randint(0,100) <10 else "")
            info_dialog(mess)
            return False
        else:
            # normal return value (hopefully)
            return [imol,
                    prodrg_output_mol_file,
                    prodrg_output_pdb_file,
                    prodrg_output_lib_file]


def prodrg_plain(mode, imol_in, chain_id_in, res_no_in):

    selection_string = "//" + chain_id_in + "/" + \
                       str(res_no_in)
    imol = new_molecule_by_atom_selection(imol_in, selection_string)
    stub = os.path.join("coot-ccp4", "prodrg-tmp-" + str(os.getpid()))
    prodrg_xyzin  = stub + "-xyzin.pdb"
    prodrg_xyzout = stub + "-xyzout.pdb"
    prodrg_cif    = stub + "-dict.cif"
    prodrg_log    = stub + ".log"

    write_pdb_file(imol, prodrg_xyzin)
    result = popen_command(cprodrg,
                           ["XYZIN",  prodrg_xyzin,
                            "XYZOUT", prodrg_xyzout,
                            "LIBOUT", prodrg_cif],
                           ["MINI PREP", "END"],
                           prodrg_log, True)
    close_molecule(imol)
    return [result, prodrg_xyzout, prodrg_cif]


# Why do we pass imol etc and then use active atom?
def fle_view(imol, chain_id, res_no, ins_code):

    import operator
    global have_mingw
    
    # not using active atom, but make property list
    r_flat  = prodrg_flat (imol, chain_id, res_no)
    r_plain = prodrg_plain('mini-no', imol, chain_id, res_no)
    if (r_flat and (r_plain[0] == 0 )):
        imol_ligand_fragment = r_flat[0]
        prodrg_output_flat_mol_file_name = r_flat[1]
        prodrg_output_flat_pdb_file_name = r_flat[2]
        prodrg_output_cif_file_name      = r_flat[3]
        prodrg_output_3d_pdb_file_name   = r_plain[1]
        # 'using_active_atom'
        active_atom = active_residue()
        aa_imol     = active_atom[0]
        aa_chain_id = active_atom[1]
        aa_res_no   = active_atom[2]
        fle_view_internal(aa_imol, aa_chain_id, aa_res_no, "",  # should be from active_atom!!     using_active_atom([[]])
                          imol_ligand_fragment,
                          prodrg_output_flat_mol_file_name,
                          prodrg_output_flat_pdb_file_name,
                          prodrg_output_3d_pdb_file_name,
                          prodrg_output_cif_file_name)
        # touch on Windows!?
        # either distribute touch.exe or use DOS:
        # for non existing file use:
        # copy NUL YourFile.txt
        # for existing:
        # copy /b filename.ext +,,
        #if (is_windows() and not have_mingw):
        if (is_windows()):
            import subprocess
            lbg_ready = os.path.abspath(os.path.join("coot-ccp4",
                                                      ".coot-to-lbg-mol-ready"))
            print "BL DEBUG:: lbg_ready is", lbg_ready
            if os.path.isfile(lbg_ready):
                subprocess.call("copy /b /y " + lbg_ready + " +,, " + lbg_ready
                                , shell=True)
            else:
                subprocess.call("copy NUL " + lbg_ready, shell=True)
        else:
            popen_command("touch",
                          [os.path.join("coot-ccp4",
                                        ".coot-to-lbg-mol-ready")],
                          [],
                          "/dev/null", False)


                
