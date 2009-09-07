
def pisa_xml(imol, file_name):

    from xml.etree.ElementTree import ElementTree
    import os

    if (os.path.isfile(file_name)):
        print "opened", file_name
        sm = ElementTree()
        sm.parse(file_name)
        parse_pisa(imol, sm)

# pisa results
#    name
#    status
#    total_asm
#    asm_set
#       ser_no
#       assembly
#          id
#          size
#          mmsize
#          diss_energy
#          asa
#          bas
#          entropy
#          diss_area
#          int_energy
#          n_uc
#          n_diss
#          symNumber
#          molecule
#             chain_id
#             rxx
#             rxy
#             rxz
#             tx
#             ryx
#             ryy
#             ryz
#             ty
#             rzx
#             rzy
#             rzz
#             tz
#             rxx-f
#             rxy-f
#             rxz-f
#             tx-f
#             ryx-f
#             ryy-f
#             ryz-f
#             ty-f
#             rzx-f
#             rzy-f
#             rzz-f
#             tz-f

def parse_pisa(imol, entity):

    # was it a chain? (or residue selection disguised as
    # a chain?)
    #
    def make_atom_selection_string(chain_id_raw):

        #  first try to split the chain-id-raw on a "]".  If there was
        # no "]" then we had a simple chain-id.  If there was a "]"
        # then we have something like "[CL]D:32", from which we need to
        # extract a residue number and a chain id.  Split on ":" and
        # construct the left and right hand sides of s.  Then use those
        # together to make an mmdb selection string.
        #
        if ("]" in chain_id_raw):
            print "found a residue selection chain_id", chain_id_raw
            s = chain_id_raw[chain_id_raw.find("]") + 1:] # e.g. "D:32"
            if (":" in s):
                residue_string = s[s.find(":") + 1:]
                chain_string   = s[0:s.find(":")]
                return "//" + chain_string + "/" + residue_string
        else:
            print "found a simple chain_id", chain_id_raw
            return "//" + chain_id_raw

    # return a molecule number or False
    #
    def handle_molecule(molecule):
        # molecule is an element in the xml tree
        # lets make a dictionary out of it for easier handling
        mol_dic = dict((ele.tag, ele.text) for ele in molecule)
        symbols = ['rxx', 'rxy', 'rxz', 'ryx', 'ryy', 'ryz', 'rzx', 'rzy', 'rzz',
                   'tx', 'ty', 'tz'] # orthogonal matrix elements

        ass_symbols = []
        atom_selection_string = "" # default, everything
        symm_name_part = ""        # the atom selection info without
                                   # "/" because that would chop the
                                   # name in the display manager.

        chain_id_raw = mol_dic["chain_id"]
        atom_selection_string = make_atom_selection_string(chain_id_raw)
        print "BL DEBUG:: chain_id_raw and atom_selection strin", chain_id_raw, atom_selection_string
        if (chain_id_raw == atom_selection_string):
            symm_name_part = "chain " + chain_id_raw
        else:
            symm_name_part = chain_id_raw

        # the rotation or translation symbols?
        for symbol in symbols:
            ass_symbols.append([symbol, float(mol_dic[symbol])])

        if (len(ass_symbols) == 12):
            mat = map(lambda sym: sym[1], ass_symbols)
            print "mat::::", mat
            print "currently %s molecules" %(graphics_n_molecules())
            new_molecule_name = "Symmetry copy of " + \
                                str(imol) +\
                                " using " + symm_name_part
            new_mol_no = new_molecule_by_symmetry_with_atom_selection(
                imol,
                new_molecule_name,
                atom_selection_string,
                *(mat + [0,0,0]))
            return new_mol_no
        return False # ass_symbols were not all set

    # Return the model number of the new assembly molecule
    #
    def create_assembly_set_molecule(assembly_set_molecule_numbers,
                                     assembly_set_number = ""):
        if not assembly_set_molecule_numbers:
            return False
        else:
            first_copy = copy_molecule(assembly_set_molecule_numbers[0])
            if not valid_model_molecule_qm(first_copy):
                return False
            else:
                rest = assembly_set_molecule_numbers[1:]
                merge_molecules(rest, first_copy)
                set_molecule_name(first_copy, "Assembly Set " + assembly_set_number)
                return first_copy

    ( STRING,
      STDOUT) = range(2)
    # record in an element in xml element tree
    # port is STRING or STDOUT
    #
    def print_assembly(record, port):
        # first make a dictionary
        rec_dic = dict((ele.tag, ele.text) for ele in record)
        # now either print to stdout or write to string (which will
        # be returned - otherwise return None, False on error)
        ret = None
        if (port == STRING):
            import StringIO
            out = StringIO.StringIO()
        elif (port == STDOUT):
            import sys
            out = sys.stdout
        else:
            return False
        print "BL DEBUG:: rec_dic", rec_dic
        out.write("assembly id:   %s\n" %rec_dic['id'])
        out.write("assembly size: %s\n" %rec_dic['size'])
        out.write("assembly symm-number: %s\n" %rec_dic['symNumber'])
        out.write("assembly asa:  %s\n" %rec_dic['asa'])
        out.write("assembly bsa:  %s\n" %rec_dic['bsa'])
        out.write("assembly diss_energy: %s\n" %rec_dic['diss_energy'])
        out.write("assembly entropy:     %s\n" %rec_dic['entropy'])
        out.write("assembly diss_area:   %s\n" %rec_dic['diss_area'])
        out.write("assembly int_energy:  %s\n" %rec_dic['int_energy'])
        out.write("assembly components:  %s\n" %rec_dic['molecule'])
        if (port == STRING):
            ret = out.getvalue()
            out.close()
        return ret

    # Return a list of model numbers
    #
    def create_assembly_molecule(assembly_molecule_numbers):
        return assembly_molecule_numbers

    # return an assembly record:
    # this is now a dictionary with all assembly properties
    # molecule is replaced with a list of symmetry created molecules
    # arg assembly is assembly element from xml tree
    #
    def handle_assembly(assembly):
        assembly_molecule_numbers = []
        assembly_dic = dict((ele.tag, ele.text) for ele in assembly)
        molecules = assembly.getiterator("molecule")
        for molecule in molecules:
            mol_no = handle_molecule(molecule)
            assembly_molecule_numbers.append(mol_no)
        assembly_dic["molecule"] = assembly_molecule_numbers
        return assembly_dic


    # handle assembly-set (the xml tree element for asm_set.
    #)
    # return values [False or the molecule number (list?) of the
    # assembly-set] and the assembly-record-set (which is an xml
    # tree element for the assembly-set, from 'assembly')
    #
    def handle_assembly_set(assembly_set):
        assembly_set_molecules = []
        assembly_records = assembly_set.find("assembly")
        assembly_record = handle_assembly(assembly_records)
        assembly_set_molecules = assembly_record["molecule"]
        new_mol = create_assembly_set_molecule(assembly_set_molecules)

        return new_mol, assembly_records

    # main line
    #

    #
    first_assembly_set_is_displayed_already_qm = False
    top_assembly_set = False

    assemblies = entity.getiterator("asm_set")
    for ass in assemblies:
        molecule_number, assembly_record_set = handle_assembly_set(ass)
        if not top_assembly_set:
            # make a string out of the dictionary
            top_assembly_set = print_assembly(assembly_record_set, STRING)
        if first_assembly_set_is_displayed_already_qm:
            set_mol_displayed(molecule_number, 0)
        else:
            first_assembly_set_is_displayed_already_qm = True

    
    print "top assembly-set:\n", top_assembly_set
    s = "top assembly-set: \n\n" + top_assembly_set
    info_dialog(s)     
            
    
#
#

def pisa_assemblies(imol):

    import os
    #
    def make_stubbed_name(imol):
        return strip_extension(os.path.basename(molecule_name(imol)))

    #
    def make_pisa_config(pisa_coot_dir, config_file_name):
        s = os.getenv("CCP4")
        # may need a normpath somewhere too?! FIXME
        ls = [["DATA_ROOT", os.path.join(s, "share/pisa")],
              ["PISA_WORK_ROOT", pisa_coot_dir],
              ["SBASE_DIR", os.path.join(s, "share/sbase")],
              # according to Paule (and I agree):
              # that we need these next 3 is ridiculous
              # BL:: hope not really needed (as for Win exe is missing)
              ["MOLREF_DIR", os.path.join(s, "share/pisa/molref")],
              ["RASMOL_COM", os.path.join(s, "bin/rasmol")],
              ["CCP4MG_COM", os.path.join(s, "bin/ccp4mg")],
              ["SESSION_PREFIX", "pisa1_"],
              ]
        if (os.path.isdir(s)):
            fin = open(config_file_name, 'w')
            for item_pair in ls:
                fin.write(item_pair[0] + "\n")
                fin.write(item_pair[1] + "\n")
            fin.close()

    # main line
    if valid_model_molecule_qm(imol):
        pisa_coot_dir = "coot-pisa"
        stubbed_name = make_stubbed_name(imol)
        pdb_file_name      = os.path.join(pisa_coot_dir, stubbed_name + ".pdb")
        pisa_config        = os.path.join(pisa_coot_dir, stubbed_name + "-pisa.cfg")
        pisa_xml_file_name = os.path.join(pisa_coot_dir, stubbed_name + ".xml")
        pisa_project_name  = os.path.join(stubbed_name)

        make_directory_maybe(pisa_coot_dir)
        make_pisa_config(pisa_coot_dir, pisa_config)
        write_pdb_file(imol, pdb_file_name)
        if (os.path.isfile(pdb_file_name)):

            # "pisa" should be pisa_exe or somthign like this (need absolute path...)
            # FIXME
            print "pisa args:", [pisa_project_name, "-analyse", pdb_file_name, pisa_config]
            status = popen_command("pisa",
                                   [pisa_project_name, "-analyse", pdb_file_name, pisa_config],
                                   [], "pisa.log", True)
#                                   [], "pisa.log", False)
            if (status == 0):  #good
                status_2 = popen_command("pisa", [pisa_project_name, "-xml", pisa_config],
                                         [], pisa_xml_file_name, True)
#                                         [], pisa_xml_file_name, False)
                if (status_2 == 0):
                    pisa_xml(imol, pisa_xml_file_name)
                
                
