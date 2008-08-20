
# "  53" -> "53", " " -> ""
def strip_leading_spaces(str):
    return str.lstrip()

# conider for coot_utils
def go_to_residue_by_spec(imol, spec):

    def atom_spec_qm(spec):
        return (len(spec) == 5)

    # main line
    set_go_to_atom_molecule(imol)
    if (atom_spec_qm(spec)):
        atom_name = spec[3]
    else:
        atom_name = " CA "

    set_go_to_atom_chain_residue_name(spec[0],
                                      spec[1],
                                      atom_name)

# consider for coot_utsil
def residue_spec2string(spec):
    return (spec[0] + int(spec[1]) + spec[2])

# make this internal 
# 
# where line is something like  "  53 HIS  (  53 ) A12"
#                                   ignore   xxxxI Cyy
# xxxx is 4char resno, I insertion code, C chain id, yy is model number
# 
# return a residue spec, or False
# 
def get_flip_residue(line):
    l = len(line)
    if (l < 19):
        return False
    else:
        resno_string = line[11:15]
        try:
            resno = int(resno_string)
        except:
            return False
        inscode_pre = line[15:16]
        if (inscode_pre == " "):
            inscode = ""
        else:
            inscode = inscode_pre
        chain_id = line[18:19]
        print "found", resno, inscode, chain_id
        return [chain_id, resno, inscode]

def convert_to_button_info(problemed_res_list):
    print "convert_to_button_info got", problemed_res_list
    button_label      = problemed_res_list[1]
    problem_string    = problemed_res_list[0]
    action_func       = problemed_res_list[2]
    residue_spec_list = problemed_res_list[3]

    o = map(lambda x: [problem_string, button_label, action_func, x],
            residue_spec_list)
    print "convert_to_button_info returns", o
    return o

# A problemed-res-list is improper pair where the car is a pair of
# string, one describing the problem, and the other being the "fix
# it" button label.  The cdr is a list of residue specs.
# 
# A problemed-flip-list-list is a list of those things.
# 
def problem_residues2dialog(imol, problemed_res_list_list):
    if (problemed_res_list_list):
        dialog_pair_of_buttons(imol,
                               "What-Check Report",
                               [220, 250],
                               # buttons is a list of: [[button_1_label, button_1_action,],
                               #                        [button_2_label button_2_action]]
                               # The button_1_action function takes as an argument the imol.
                               # The button_2_action function takes as an argument the imol.
                               # think function needs to be a string, no args?!?!

                               # FIXME

                               "  Close  ")
# action is either 'gui' (then we get a gui) or 'apply-actions', then the model
# modifications are automatically applied.
#
def parse_check_db(imol, file_name, action):

    #
    def my_flip(imol_local, res_spec):
        do_180_degree_side_chain_flip(imol_local,
                                      res_spec[0],
                                      res_spec[1],
                                      res_spec[2],
                                      "")

    def my_rsr(imol_local, res_spec):
        with_auto_accept([[refine_zone, imol_local, res_spec[0], res_spec[2],
                           res_spec[3], ""]])

    #
    def my_delete_atom(imol, atom_spec):
        ls = [imol] + atom_spec
        print "calling delte_atom with args", ls
        delete_atom(*ls)

    #
    def my_rotamer_search(imol, atom_spec):
        chain_id = res_spec[0]
        resno    = res_spec[1]
        ins_code = res_spec[2]
        altloc   = ""

        imol_map = imol_refinement_map()
        if (valid_map_molecule_qm(imol_map)):
            print "rotamer search with imol: %s chain: %s resno: %s inscode: %s" \
                  %(imol, chain_id, resno, ins_code)
            auto_fit_best_rotamer(resno, altloc, ins_code, chain_id, imol, imol_map, 1, 0.01)

    # return False or a residue spec:
    def line2residue_spec(line):
        if (len(line) >21):
            resno_string = line[26:30]
            try:
                resno = int(resno_string)
            except:
                print "BL WARNING:: no int resno -> return"
                return False
            inscode_pre = line[40:41]
            if (inscode_pre == "_"):
                inscode = ""
            else:
                inscode = inscode_pre
            chain_id_pre = line[19:20]
            chain_id = " " if (chain_id_pre == "_") else chain_id_pre
            return [chain_id, resno, inscode]

    # return False or an atom_spec
    # an atom_spec is [chain_id, resno, ins_code, atom_name, alt_conf]
    #
    def line2atom_spec(line):
        if (len(line) > 53):
            resno_string  = line[26:30]
            try:
                resno = int(resno_string)
            except:
                return False
            inscode_pre = line[40:41]
            inscode = "" if (inscode_pre == "_") else inscode_pre
            chain_id_pre = line[19:20]
            chain_id = "" if (chain_id_pre == "_")  else chain_id_pre
            atom_name_pre = line[47:51]
            atom_name = atom_name_pre
            alt_conf = ""
            print "found atom name %s resno %s (from %s) inscode %s (%s) chain_id %s" \
                  %(atom_name, resno, resno_string, inscode_pre, inscode, chain_id)
            print "line:", line
            return [chain_id, resno, inscode, atom_name, alt_conf]

    def apply_actions(imol, action_list):

        for action in action_list:
            func = action[0]
            baddie_list = action[1]
            imol_list =  # ??????
            print "applying %s to %s" %(func, baddie_list)
            map(func, imol_list, baddie_list)

    # main line
    print "parse_whatcheck_report: parsing output of what-if..."
    found_flip_table     = False
    found_h2ohbo_table   = False
    found_BMPCHK_table   = False
    found_NAMCHK_table   = False
    found_PLNCHK_table   = False
    h2o_h_bond_less_list = []
    plane_list           = []
    flip_list            = []
    namchk_list          = []
    bump_list            = []

    if (os.path.isfile(file_name)):
        fin = open(file_name, 'r')
        lines = fin.readlines()
        fin.close()
        for line in lines:
            if (line == "//"):
                found_flip_table     = False
                found_h2ohbo_table   = False
                found_BMPCHK_table   = False
                found_NAMCHK_table   = False
                found_PLNCHK_table   = False

            if (found_flip_table):
                if (len(line) > 12):
                    attr_type = line[0:11]
                    if (attr_type == "   Name   :"):
                        #print "found flip name", line
                        res = line2residue_spec(line)
                        if (res):
                            flip_list.append(res)
            
            if (line == "CheckID   : HNQCHK"):
                found_flip_table = True

            if (found_h2ohbo_table):
                if (len(line) > 12):
                    attr_type = line[0:11]
                    if (attr_type == "   Name   :"):
                        print "found h2obmo name", line
                        atom = line2atom_spec(line)
                        if (atom):
                            h2o_h_bond_less_list.append(atom)
            
            if (line == "CheckID   : H2OHBO"):
                found_h2ohbo_table = True

            if (found_BMPCHK_table):
                if (len(line) > 12):
                    attr_type = line[0:11]
                    if (attr_type == "   Name   :"):
                        res = line2residue_spec(line)
                        if (res):
                            bump_list.append(atom)
            
            if (line == "CheckID   : BMPCHK"):
                found_BMPCHK_table = True
            
            if (found_NAMCHK_table):
                if (len(line) > 12):
                    attr_type = line[0:11]
                    if (attr_type == "   Name   :"):
                        res = line2residue_spec(line)
                        if (res):
                            namchk_list.append(atom)
            
            if (line == "CheckID   : NAMCHK"):
                found_NAMCHK_table = True
            
            if (found_PLNCHK_table):
                if (len(line) > 12):
                    attr_type = line[0:11]
                    if (attr_type == "   Name   :"):
                        res = line2residue_spec(line)
                        if (res):
                            plane_list.append(atom)
            
            if (line == "CheckID   : PLNCHK"):
                found_PLNCHK_table = True
            
        # print "Here flip_list is", flip_list
        # print "Here h2o-h-bond-less-list is", h2o_h_bon_less_list
        # print "Here bump_list is", bump_list
        if (action == 'gui'):
            problem_residues2dialog(imol, [
                ["Needs flipping", "Flip it", my_flip, flip_list],
                ["Water with no H-bonds", "Delete it", my_delete_atom, h2o_h_bond_less_list],
                ["Has Bumps", "Rotamer Search", my_rotamer_seach, bump_list],
                ["Nomenclature error", "180 degree flip", my_flip, namchk_list]])
        elif (action == 'apply-actions'):
            apply_actions(imol, [
                # [my_flip, flip_list],
                # [my_delete_atom, h2o_h_bond_less_list],
                # [my_rotamer_search, bump_list],
                [my_rsr, plane_list]
                # [my_flip, namchk_list]
                ])
        else:
            print "INFO:: wrong action input"
                     
            
    else:
        print "file not found", file_name
