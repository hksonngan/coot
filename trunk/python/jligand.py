# Coot and JLigand communicate via "secret" files.
# 
# Coot send to jligand the comp-ids it wants joined in *to-jligand-secret-file-name*.
#
# JLigand sends Coot a file that contains filenames for the link and
# new dictionary/restraints.
#
global to_jligand_secret_file_name
to_jligand_secret_file_name = ".coot-to-jligand-8lcs"
global from_jligand_secret_link_file_name
from_jligand_secret_link_file_name = ".jligand-to-coot-link"

# Define an environment variable for the place where jligand.jar
# resides in a canonical distribution
# Can be everywhere on windows? (maybe not in new ccp4!?) its in CCP4_BIN
#
jligand_home_env = os.getenv("JLIGAND_HOME")
jligand_home = jligand_home_env if jligand_home_env else "."

global java_command
java_command = "java"

jligand_jar = os.path.join(os.path.normpath(jligand_home), "jligand.jar")
# windows ccp4 installation calls it "JLigand.jar" though! Grrr. FIXME
# Not yet as the old version uses different files...
#if not os.path.isfile(jligand_jar):
#    # lets try CCP4_BIN and JLigand.jar
#    ccp4_bin = os.getenv("CCP4_BIN")
#    jligand_jar = os.path.join(os.path.normpath(ccp4_bin), "JLigand.jar")
jligand_args = ["-jar", jligand_jar, "-coot"] # for coot-enable jligand

# jligand internal parameter
global imol_jligand_link
imol_jligand_link = False


def write_file_for_jligand(res_spec_1, resname_1, res_spec_2, resname_2):

    fin = open(to_jligand_secret_file_name, 'w')
    int_time = time.time()
    print "res_spec_1:", res_spec_1
    print "res_spec_2:", res_spec_2
    chain_id_1 = res_spec2chain_id(res_spec_1)
    chain_id_2 = res_spec2chain_id(res_spec_2)
    
    fin.write("CODE " + resname_1 + " " + chain_id_1 + " " + \
              str(res_spec2res_no(res_spec_1)) + "\n")
    fin.write("CODE " + resname_2 + " " + chain_id_2 + " " + \
              str(res_spec2res_no(res_spec_2)) + "\n")
    fin.write("TIME " + str(int_time) + "\n")
    fin.close()

def click2res_spec(click):
    return [click[2], click[3], click[4]]

# every fraction of a second look to see if
# from_jligand_secret_link_file_name has been updated.  If so, then
# run the handle_read_from_jligand_file function.
#
def start_jligand_listener():

    global from_jligand_secret_link_file_name
    global startup_mtime
    
    # BL says: why do we use the link file and not the other one??
    startup_mtime = get_file_latest_time(from_jligand_secret_link_file_name)
    
    def jligand_timeout_func():
    
        import operator
        global startup_mtime
    
        now_time = get_file_latest_time(from_jligand_secret_link_file_name)
        if operator.isNumberType(now_time):
            if not operator.isNumberType(startup_mtime):
                startup_mtime = now_time
                # print "just set startup-mtime to (first)", startup_mtime
                handle_read_from_jligand_file()
            else:
                if (now_time > startup_mtime):
                    startup_mtime = now_time
                    # print "just set startup-mtime to", startup_mtime
                    handle_read_from_jligand_file()
        return True # we never expire...

    gobject.timeout_add(700, jligand_timeout_func)

# This happens when user clicks on the "Launch JLigand" button.
# It starts a jligand and puts it in the background.
#
def launch_jligand_function():

    global jligand_jar
    global jligand_home_env
    global java_command
    
    start_jligand_listener()
    if not os.path.isfile(jligand_jar):

        # Boo.  Give us a warning dialog
        #
        s = "jligand java jar file: " + jligand_jar + " not found"

        # make an extra message telling us that JLIGAND_HOME is
        # not set if it is not set.
        env_message = "Environment variable JLIGAND_HOME not set\n\n" \
                      if not jligand_home_env else ""
        info_dialog(env_message + s)

    else:
        # OK, it does exist - run it!
        #
        java_exe = find_exe(java_command)
        if not java_exe:
            print "BL INFO:: no java found"
        else:
            run_concurrently(java_exe, jligand_args)
            # beam in a new menu to the menu bar:
            if (have_coot_python):
                if coot_python.main_menubar():
                    jligand_menu = coot_menubar_menu("JLigand")
                    add_simple_coot_menu_menuitem(
                        jligand_menu, "Send Link to JLigand (click 2 monomers)",
                        lambda func: click_select_residues_for_jligand()
                        )

# This happens when user clicks on the "Select Residues for JLigand"
# (or some such) button.  It expects the user to click on atoms of
# the two residues involved in the link.
#
def click_select_residues_for_jligand():

    global imol_jligand_link
    
    def link_em(*args):
        print "we received these clicks", args
        if (len(args) == 2):
            click_1 = args[0]
            click_2 = args[1]
            print "click_1:", click_1
            print "click_2:", click_2
            if ((len(click_1) == 7)
                and (len(click_2) ==7)):
                resname_1 = residue_name(click_1[1],
                                         click_1[2],
                                         click_1[3],
                                         click_1[4])
                resname_2 = residue_name(click_2[1],
                                         click_2[2],
                                         click_2[3],
                                         click_2[4])
                imol_click_1 = click_1[1]
                imol_click_2 = click_2[1]
                if not (isinstance(resname_1, str) and
                        isinstance(resname_2, str)):
                    print "Bad resnames: %s and %s" %(resname_1, resname_2)
                else:
                    if not (imol_click_1 == imol_click_2):
                        imol_jligand_link = False
                    else:
                        # happy path
                        imol_jligand_link = imol_click_1
                        write_file_for_jligand(click2res_spec(click_1), resname_1,
                                               click2res_spec(click_2), resname_2)

    user_defined_click(2, link_em)

def handle_read_from_jligand_file():

    global imol_jligand_link
    global from_jligand_secret_link_file_name
    
    def bond_length(pos_1, pos_2):
        def square(x):
            return x*x
        def sub(x, y):
            return x-y

        import math
        ret = math.sqrt(sum(map(square, map(sub, pos_1, pos_2))))
        return ret

    def bond_length_from_atoms(atom_1, atom_2):
        if not isinstance(atom_1, list):
            print "   WARNING:: bond_length_from_atoms: atom_1 not a list:", atom_1
            return
        elif not isinstance(atom_2, list):
                  print "   WARNING:: bond_length_from_atoms: atom_2 not a list:", atom_2
                  return
        else:
            return bond_length(atom_1[2],
                               atom_2[2])

    def get_dist(atom_spec_1, atom_spec_2):
        atom_1 = get_atom(*([imol_jligand_link] + atom_spec_1))
        atom_2 = get_atom(*([imol_jligand_link] + atom_spec_2))
        if not (isinstance(atom_1, list) and
                isinstance(atom_2, list)):
            return False
        else:
            return bond_length_from_atoms(atom_1, atom_2)

    if os.path.isfile(from_jligand_secret_link_file_name):
        # this file consists of 2 lines:
        # the first is a file name that contains the cif dictionary for the cif
        # the second is a LINK link that should be added to the PDB file.
        #
        fin = open(from_jligand_secret_link_file_name, 'r')
        lines = fin.read().splitlines()  # remove the newlines
        fin.close()
        if not lines:
            print "BL WARNING:: empty file", from_jligand_secret_link_file_name
        else:
            cif_dictionary = lines[0]
            # was it the READY marker or a cif file name?
            if (cif_dictionary == "READY"):
                print "JLigand is ready"
                # maybe I need to set something here? (that
                # from now a modification of .jligand-to-coot
                # is means that we should read it?)
            if os.path.isfile(cif_dictionary):
                read_cif_dictionary(cif_dictionary)
                link_line = lines[1]
                print "Now handle this link line", link_line
                if (len(link_line) > 72):
                    atom_name_1 = link_line[12:16]
                    atom_name_2 = link_line[42:46]
                    chain_id_1  = link_line[21:22] # 1 char, FIXME in future
                    chain_id_2  = link_line[51:52]
                    res_no_1_str = link_line[22:26]
                    res_no_2_str = link_line[52:56]
                    res_no_1 = int(res_no_1_str)
                    res_no_2 = int(res_no_2_str)
                    ins_code_1 = ""  # too lazy to look these up for now.
                    ins_code_2 = ""
                    alt_conf_1 = ""  # ditto.
                    alt_conf_2 = ""
                    link_type = link_line[72]

                    print "we parsed  these: "
                    print "       atom_name_1", atom_name_1
                    print "       atom_name_2", atom_name_2
                    print "        chain_id_1", chain_id_1
                    print "        chain_id_2", chain_id_2
                    print "      res_no_1_str", res_no_1_str
                    print "      res_no_2_str", res_no_2_str
                    print "         link_type", link_type

                    # check res_no_1/2 as number? Shoudl be earlier as
                    # converted to int now... use try...
                    if (valid_model_molecule_qm(imol_jligand_link)):
                        res_spec_1 = [chain_id_1, res_no_1, ins_code_1]
                        res_spec_2 = [chain_id_2, res_no_2, ins_code_2]
                        atom_spec_1 = res_spec_1 + [atom_name_1, alt_conf_1]
                        atom_spec_2 = res_spec_2 + [atom_name_2, alt_conf_2]
                        dist = get_dist(atom_spec_1, atom_spec_2)
                        if not dist:
                            print "bad dist %s from %s %s" %(dist, atom_spec_1, atom_spec_2)
                        else:
                            make_link(imol_jligand_link, atom_spec_1,
                                      atom_spec_2, link_type, dist)
                        
                    
                    
            
        
        
        


