def do_code(id_code):
    pdb_code_str = id_code + ".pdb"
    cif_code_str = id_code + ".cif"
    imol = handle_read_draw_molecule(pdb_code_str)
    read_cif_data(cif_cod_str,imol)
