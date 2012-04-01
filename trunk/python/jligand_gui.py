
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
            # first check if we can run it with coot, i.e. is '-version'
            # a valid command line arg
            jligand_version = ["-jar", jligand_jar, "-version"]
            cmd = java_exe + " " + \
                  string_append_with_spaces(jligand_version)
            res = shell_command_to_string(cmd)
            if (not res):
                message = "  Sorry, your JLigand is not new enough to work with Coot!\nPlease download a new one!"
                info_dialog(message)
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
