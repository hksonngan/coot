
import sys
import os
from subprocess import call
from rdkit import Chem
from rdkit.Chem import AllChem

import coot_libs as coot
import restraints_boost as coot_boost

from optparse import OptionParser

import tautomer

from jay_util import *

global run_mogul
global smiles_dict
run_mogul = True
smiles_dict = False

def make_mogul_ins_file(mogul_ins_file_name, mogul_out_file_name, sdf_file_name):
   f = open(mogul_ins_file_name, 'w')
   if f:
     f.write('mogul molecule file ')
     f.write(sdf_file_name)
     f.write('\n')
     f.write('mogul output   file ')
     f.write(mogul_out_file_name)
     f.write('\n')
     f.write('mogul output distribution all on\n')
     f.write('bond all\n')
     f.write('angle all\n')
#      f.write('torsion all\n')
#      f.write('ring all\n')
     f.write('config output format CSV\n')
     f.write('config output items fragment_type atom_indices query_value nhits mean median sd z-score dmin\n')
     f.write('config search all filter exclude_solvents\n')
     f.write('config output invalid_fragments exclude\n')
     f.close()
   return f


# return True for good, False for bad/not-run
#
def execute_mogul(sdf_file_name, mogul_ins_file_name, mogul_out_file_name):
   f = make_mogul_ins_file(mogul_ins_file_name, mogul_out_file_name, sdf_file_name)
   if f: 
      # print 'now run mogul using ins file %s' % mogul_ins_file_name
      if run_mogul:
          call(['mogul', '-ins', mogul_ins_file_name])
          return True
      else:
          return False
   else:
      return False

def atom_name_from_atomic_number_and_count(element, count):
    name = element
    name += str(count)
    return name

def add_atom_names(mol):
    nz = {}
    atom_names = []
    for atom in mol.GetAtoms():
       try:
          n = atom.GetProp('name')
          atom_names.append(n)
       except KeyError:
          z = atom.GetAtomicNum()
          if z in nz:
             nz[z]  = nz[z] + 1
          else:
             nz[z] = 1;
          ele = atom.GetSymbol().upper()
          name = atom_name_from_atomic_number_and_count(ele, nz[z])
          p_name = pad_atom_name(name, ele)
          atom.SetProp("name", p_name)
          atom_names.append(p_name)
    return atom_names

def convert_to_coot_bond_type(rdkit_type):
    out_type = 'single'
    if (rdkit_type == Chem.rdchem.BondType.SINGLE):
        out_type = 'single'
    if (rdkit_type == Chem.rdchem.BondType.AROMATIC):
        out_type = 'aromatic'
    if (rdkit_type == Chem.rdchem.BondType.DOUBLE):
        out_type = 'double'
    if (rdkit_type == Chem.rdchem.BondType.TRIPLE):
        out_type = 'triple'
    if (rdkit_type == Chem.rdchem.BondType.ONEANDAHALF):
        out_type = 'deloc'
    return out_type

def pad_atom_name(name, element):

    padded = name
    if (len(element) == 1):
        if (len(name) == 2):
            padded = ' ' + name + ' '
        if (len(name) == 3):
            padded = ' ' + name
    if (len(element) == 2):
        if (len(name) == 2):
            padded = name + '  '
        if (len(name) == 3):
            padded = name + ' '
    return padded

def is_smiles_file(file_name):
    bits = file_name.rsplit(".")
    if len(bits) > 1:
        return bits[1] == 'smi'
    else:
        return False

def is_comp_id(comp_id):
   return len(comp_id) == 3

def is_mdl_file(file_name):
    bits = file_name.rsplit(".")
    if (len(bits) < 2):
       return False
    else:
       idx = len(bits) - 1
       if (bits[idx] == 'mol'):
          return True
       else:
          if (bits[idx] == 'mdl'):
             return True
          else:
             return False
        

# return the contents of file_name
def read_file(file_name):
    f = open(file_name)
    return f.read()

        
def make_restraints_for_bond_orders(mol):
    restraints = {}
    bond_list = []
    for bond in mol.GetBonds(): 
        type = bond.GetBondType()
        coot_bond_type = convert_to_coot_bond_type(type)
        at_1 = bond.GetBeginAtom()
        at_2 = bond.GetEndAtom()
        name_1 = at_1.GetProp('name')
        name_2 = at_2.GetProp('name')
        item = [name_1, name_2, coot_bond_type, 1.0, 1.0]
        bond_list.append(item)
    restraints['_chem_comp_bond'] = bond_list
    restraints['_chem_comp'] = [mol.GetProp('comp_id'),
                                mol.GetProp('comp_id'),
                                mol.GetProp('name'),
                                'non-polymer',
                                mol.GetNumAtoms(),
                                mol.GetNumAtoms(),
                                'Partial']
    return restraints

# match_atom_index can be of type int or a list - otherwise trouble.
#
def set_atom_type(match, match_atom_index, mol, atom_type):
    # print "   trying to set ", match_atom_index, " of ", match
    try:
        this_atom = match[match_atom_index]
        # print "   this_atom:", this_atom
        try:
            current_type = mol.GetAtomWithIdx(this_atom).GetProp("atom_type")
            # print "   oops - atom ", mol.GetAtomWithIdx(this_atom).GetProp("name"), " already has type ", current_type
        except KeyError:
            mol.GetAtomWithIdx(this_atom).SetProp("atom_type", atom_type)
            # print "    set atom number ", this_atom, " having name \"", mol.GetAtomWithIdx(this_atom).GetProp("name"), "\" as type ", atom_type
            name = mol.GetAtomWithIdx(this_atom).GetProp("name");
            print '    set atom number %s having name %s with type %s ' % (str(this_atom).rjust(2),
                                                                           repr(name), repr(atom_type))
    except TypeError:
        for match_atom in match_atom_index:
            set_atom_type(match, match_atom, mol, atom_type)

def ele_to_smart(v):
    return (v.upper(), '['+v+']', 0)

# those not handled by hand-coding
def smarts_by_element():
   eles = [
      "He", "Li", "Be", "B",  "Ne", "Na", "Mg", "Al", 
      "Ar", "K", "Ca",  "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co", "Ni", "Cu",
      "Zn", "Ga", "Ge", "As", "Se",      "Kr", "Rb", "Sr", "Y", "Zr", "Nb", "Mo", "Tc",
      "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I", "Xe", "Cs", "Ba", "La",
      "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu",
      "Hf", "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At",
      "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U"]
   return map(ele_to_smart, eles)
   

def set_atom_types(mol):
    smarts_list = [

        # Full coverage for C, H, O.

        # Oxygen
        ('O2',  "[OX2;H0]", 0), # ester, Os between P and C are O2, not OP
        ('OP',  'O~P',   0),
        ('OS',  'O~S',   0),
        ('OB',  'O~B',   0),
        ('OC',  '*C(=O)[OH]', (2,3)), # carboxylate
        ('OH1', '[OH1]', 0), # alcohol
        ('O2',  "[oX2;H0]", 0), # ring oxygen
        ('O',   'O=*',   0), # carbonyl oxygen

        # OH2 no examples
        # OHA no examples
        # OHB no examples
        # OHC no examples
        # OC2 no exmampes
        
        # Fallback oxygen
        ('O',   'O',   0),

        # Carbon SP
        ("CSP1", '[H][C]#*',  1), # e.g. in 2GT
        ("CSP",  '[C]#[C]',   (0,1)),
        ("CSP",  '[C]#*',     0),
        
        # Carbon SP2
        ('CR56', 'c12aaaac1aaa2',  (0,5)), # works on indole
        ('CR56', 'c12aaaan1aaa2',  0), # same pattern as (below) N56, but catching first 56 atom
        ('CR66', 'c12aaaac1aaaa2', (0,5)),
        ('CR6',  'c12ccccc1OCO2',  (0,5)),   # mouse, fused atoms in 6-ring not non-arom 5-ring
        ('CR66', 'c12aaaac1AAAA2', (0,5)),   # one 6-ring aromatic, other not. Needed for XXX?
                                             # but makes a fail on 113.
        ('CR6',  'c12caccc1***2',  (0,5)),  # aromatic 6, (non-)aromatic 5, maybe this should be CR56?

        # note CR1  missing - can't find example
        #      CR1H missing - can't find example

        ('CR16', '[cr6;H1]',  0),
        ('CR6',  '[cr6;H0]',  0),
        ('CR15', '[cr5;H1]',  0),
#        ('CR5',  'C1(=O)[C,c][C,c]C=N1', 0), # carbonyl C in a (non-percieved?) 5 ring, 0CE (a)
        ('CR5',  '[cr5;H0]',  0),
        ('CR5',  '[CR5;H0]',  0),
        ('C1',   '[CX3;H1]',    0),  # double bond, single bond and one H
        ('C2',   '[CX3;H2]=*',  0),  # double bond, and 2 H
        ('C',    '[CX3;H0;^2]', 0),
        ('C',    '[CX3]=[OX1]', 0),  # carbonyl carbon
        ('C',    '[$([CX2](=C)=C)]',   0), # bonded to 3 things not hydrogen

        # Carbon SP3
        ('CT',   '[CX4H0]', 0), # single bonded to 4 things not hydrogen
        ('CH3',  '[C;H3;^3]',   0), # bonded to something via single bond and 3 Hs
        ('CH2',  '[C;^3;H2]',   0), # standard aliphatic C.
        ('CH1',  '*[C;H1](*)*', 1), # bonded to H and 3 things 

        # sp??? needs sorting 
        ('CH2',  '[CH2]',   0), # bonded to 2 hydrogens
        
        # Carbon fallback
        ('C', '[C,c]', 0),

        # Hydrogen
        ('HCH1', '[H][CH1]',    0),
        ('HCH2', '[H][C;H2^3]', 0),
        ('HCH3', '[H][CH3]',    0),
        ('HNC1', '[H][NX2;H1;^2]', 0), # H of N of N=C ? 
        ('HNC2', '[H][NX3;H2;^2]', 0), # H on a NC2 (NH1 and NH2 of ARG)
        ('HNC3', '[H][NX3;H3;^2]', 0), # guess - no examples
        ('HNT1', '[H][NX4;H1;^3]', 0),
        ('HNT1', '[H][NX3;H1;^3]', 0),
        ('HNT2', '[H][NX3;H2;^3]', 0), # H connected to type NT2
        ('HNH2', '[H][NH2;^2]', 0), # NH2 is sp2
        ('HNH1', '[H][NX3;H1;^2]',    0),
        ('HCR6', '[H][cr6;H1]', 0),
        ('HCR5', '[H][cr5;H1]', 0), # connected to aromatic ring C with 1 H
        ('HNR5', '[H][nr5;H1]', 0), # connected to aromatic ring C with 1 H
        ('HNR5', '[H][Nr5;H1]', 0), # guess based on above, connected to aromatic N in a 5-ring
        ('HNR6', '[H][nr6;H1]', 0), # connected to aromatic 6-ring N with 1 H
        ('HNR6', '[H][NR6;H1]', 0), # guess based on above

        # HCR missing - no examples (and how is it different to HCR1?)
        ('HCR1', '[H]c',        0),
        ('HNH1', '[H][NH1]',    0),
        ('HOH1', '[H][OH1]',    0),
        ('HOH2', '[H][OH2][H]', (0,2)), # H of HOH - water
        ('H',    '[H]',         0),
        
        # Nitrogen, SP3

        ('NT1', '[NX4;H1;^3]',  0),
        ('NT1', '[NX3;H1;^3]',  0),
        ('NT2', '[NX3;H2;^3]',  0), # different to mon-lib!
        ('NT3', '[NX4;H3;^3]',  0),
        ('NT',  '[NX3;H0;^3]',  0),
        
        # Nitrogen, SP2
        ('NR66', 'c12aaaan1aaaa2', 5), # (second) 66 atom is an N.
        ('NR56', 'c12aaaan1aaa2',  5), # (second) 56 atom is an N.
        ('NR55', 'c12aaan1aaa2',   4), # (second) 55 atom is an N.
        ('NH2',  '[NX3^2][CX3^2]=[N^2;X3+]', (0,2)), # amidinium (charged)... 
        ('NR15', '[nr5;X3;H1]',    0),
        ('NR5',  '[nr5;X3;H0]',    0),
        ('NR5',  '[NR;X3;H0;^2]',  0), # [NR5;X3;H0;^2] fails on 14C (also at daylight)
        ('NRD5', '[nr5;X2;H0]',    0), # guess from 071
        ('NRD5', 'C1(=O)[C,c][C,c]C=N1', 5), # N bonded to carbonyl C in a (non-percieved?) 5 ring, 0CE (a)
        ('NR16', '[nr6;H1]',    0),
        ('NRD6', 'a:[nr6;X2;H0]:a',  1), # aromatic N with no H, i.e. one double one single
        ('NR6',  '[nr6]',    0),
        ('NC1',  '[NX2;H1;^2]', 0),  # N of N=C ? 
        ('NH1',  '[NX3;H1;^2]', 0),
        ('NH2',  '[NX3;H2;^2]', 0),  # sp2, e.g. ND2 of an ASP
        ('NT',   '*n1~[o]~[o]1', 1), # guess from 16X dioxaziridine (bleugh)
        # (NT needs checking?)
        # NC2 no examples
        # NC3 no examples
        # NPA no examples
        # NPB no examples
        

        # Nitrogen SP1
        ('NS',   '[N^1]', 0),
        # NS1 no examples
        

        # fall-back nitrogen
        ('N',    '[N,n]',      0),  

        # Phosphorus
        ('P',    'P', 0),
        # Cl
        ('CL',   '[Cl]', 0),
        # F
        ('F',    '[F]',  0),
        # Br
        ('BR',    '[Br]',  0),

        # Sulfur
        ('SH1',  '[SH1]', 0),  # SG of CYS
        ('ST',   '[SX4]', 0), # tetrahedral (2 single bonds, 2 double)
        ('S1',   '[S]=*', 0),
        ('S2',   '[SX2,sX2]', 0),
        ('S3',   '[SX3,sX3]', 0),
        ('S',    '[S,s]', 0),

        ('SI1',  '[Si;X4]', 0), # tetragonal Si
        ('SI',   '[Si]',    0)  # Si any other

        ]

    full_list = smarts_list

    for item in smarts_by_element():
       full_list.append(item)

    for smarts_info in full_list:
        atom_type, smarts, match_atom_index = smarts_info
        pattern = Chem.MolFromSmarts(smarts)
        if mol.HasSubstructMatch(pattern):
            matches = mol.GetSubstructMatches(pattern)
            print "SMARTS ", smarts
            print "  ", atom_type, ": ", matches
            for match in matches:
                set_atom_type(match, match_atom_index, mol, atom_type)
        else:
            # print "SMARTS ", smarts, " --- No hits  "
            pass

    # do we return success (everything has a type) or not?
    #
    for atom in mol.GetAtoms():
       try:
          atom_type = atom.GetProp('atom_type')
       except KeyError:
          is_aromatic = atom.GetIsAromatic()
          hybrid      = atom.GetHybridization()
          print "Error:: Missing type for atom \"", atom.GetProp('name'), "\" is_aromatic: ", is_aromatic, " hybridization: ", hybrid
          return False
    # we got to the end, good
    return True

# return True if mogul is not run or mogul exe is in place.
# return False if mogul is expected but not found.
def test_for_mogul():
   if (run_mogul):
      mogol_exe = which('mogul')
      if (mogol_exe == None):
         print "mogul not found in path"
         return False
      else:
         return True
   else:
      return True # OK, really

# this can throw a TypeError
#
def get_smiles_from_comp_id(comp_id):
   global smiles_dict
   if (not smiles_dict):
      read_smiles_tab('smiles.tab')
   return smiles_dict[comp_id]

# return a dictionary or False (if the file does not exist)
# (can this go inside get_smiles_from_comp_id?)
#
def read_smiles_tab(file_name):
    global smiles_dict
    try:
       smiles_dict = {}
       f = open(file_name)
       lines = f.readlines()
       for line in lines:
           bits = line.rstrip().rsplit()
           smiles_dict[bits[0]] = bits[2]
       return True
    except IOError as e:
       smiles_dict = True # we've tested for it
       return False

def get_smiles_from_file(file_name):
    f = open(file_name)
    return f.readline()
   

def make_restraints_from_smiles(smiles_string, comp_id, sdf_file_name, pdb_out_file_name, mmcif_dict_name, quartet_planes, quartet_hydrogen_planes):

   if (not (test_for_mogul())): return False
   m = Chem.MolFromSmiles(smiles_string)
   return make_restraints(m, comp_id, sdf_file_name, pdb_out_file_name, mmcif_dict_name, quartet_planes, quartet_hydrogen_planes)

def make_restraints_from_mdl(mol_file_name, comp_id, sdf_file_name, pdb_out_file_name, mmcif_dict_name):

   if (not (test_for_mogul())): return False

   compound_name = '.'
   m = Chem.MolFromMolFile(mol_file_name)
   return make_restraints(m, comp_id, sdf_file_name, pdb_out_file_name, mmcif_dict_name)

def make_restraints_from_pdbx(cif_file_name_in, comp_id, sdf_file_name, pdb_out_file_name,
                              mmcif_dict_name, quartet_planes, quartet_hydrogen_planes):

   # later: embed the compound_name name into m.
   m = coot_boost.rdkit_mol_chem_comp_pdbx(cif_file_name_in, comp_id)

   name = ''
   try:
      name = m.GetProp('_Name')
   except KeyError:
      print 'caught KeyError in make_restraints_from_pdbx_cif() trying GetProp _Name'

#    for atom in m.GetAtoms():
#       name = atom.GetProp('name')
#       print 'in p2.py make_restraints_from_pdbx_cif()', atom, ' of m has name ', name

   return make_restraints(m, comp_id, sdf_file_name, pdb_out_file_name, mmcif_dict_name,
                          quartet_planes, quartet_hydrogen_planes)


def n_hydrogens(mol):
    n_H = 0
    for atom in mol.GetAtoms():
	if atom.GetAtomicNum() == 1:
	    n_H += 1
    return n_H
	
   
def make_restraints(m, comp_id, sdf_file_name, pdb_out_file_name, mmcif_dict_name,
                    quartet_planes, quartet_hydrogen_planes):

   try:
      compound_name = m.GetProp('_Name');
   except KeyError:
      print 'caught key error in trying to get _Name in make_restraints() for m'
      compound_name = '.'
   except AttributeError:
      print 'problem with molecule m in make_restraints()'
      return
   

   m_H = m
   if n_hydrogens(m) == 0:
       m_H = AllChem.AddHs(m)
   
   AllChem.EmbedMolecule(m_H)
   AllChem.UFFOptimizeMolecule(m_H)

   atom_names = add_atom_names(m_H)

   m_H.SetProp('comp_id', comp_id)
   m_H.SetProp('name', compound_name)
   all_set = set_atom_types(m_H)

   if (all_set != True):
      return False
   else:
      Chem.AllChem.ComputeGasteigerCharges(m_H)

      mb = Chem.MolToMolBlock(m_H)
      print >> file(sdf_file_name,'w'), mb
      # we don't want to do this now - we will write out results
      # post-mogul regularizement usually.
      # coot.write_pdb_from_mol(m_H, comp_id, pdb_out_file_name)
      mogul_ins_file_name = 'mogul-' + comp_id + '.ins'
      mogul_out_file_name = 'mogul-' + comp_id + '.out'
      bor = make_restraints_for_bond_orders(m_H)
      # print "we got this bor: ", bor
      # coot.write_restraints(bor, comp_id, 'bond-orders.cif')

      # print out the set types:
      print '--- Atom Props ---'
      for atom in m_H.GetAtoms():
         charge = atom.GetProp('_GasteigerCharge') # string?
         name   = atom.GetProp('name')
         try:
            atom_type   = atom.GetProp('atom_type')
            is_aromatic = atom.GetIsAromatic()
            hybrid      = atom.GetHybridization()
            f_charge    = float(charge)
            print "  atom: %s %s type: %s arom: %s hybrid: %s charge: %6.3f" % (name, atom.GetSymbol(),
                                                                                atom_type.ljust(4),
                                                                                str(is_aromatic).ljust(5),
                                                                                str(hybrid).rjust(3),
                                                                                f_charge)
         except KeyError:
            print "miss", name, atom.GetSymbol(), charge

      mogul_state = execute_mogul(sdf_file_name, mogul_ins_file_name, mogul_out_file_name)
      if mogul_state:
         restraints = coot.mogul_out_to_mmcif_dict_by_mol(mogul_out_file_name, comp_id,
                                                          compound_name, m_H, bor, mmcif_dict_name,
                                                          quartet_planes, quartet_hydrogen_planes)
         coot.regularize_and_write_pdb(m_H, restraints, comp_id, pdb_out_file_name)
         # new_mol = coot_boost.regularize(m_H)
         new_mol = coot_boost.regularize_with_dict(m_H, restraints, comp_id)
         # coot.write_pdb_from_mol(new_mol, comp_id, "test-LIG.pdb")

      else:
         # we need ENERGY_LIB_CIF set to run mmcif_dict_from_mol() correctly
         restraints = coot.mmcif_dict_from_mol(comp_id, compound_name, m_H, mmcif_dict_name,
                                               quartet_planes, quartet_hydrogen_planes)
         if restraints == None:
            print "No restraints"
         coot.write_pdb_from_mol(m_H, comp_id, pdb_out_file_name)
         return True # hacked in value
      return mogul_state


def old_main():

    quartet_planes = True
    quartet_hydrogen_planes = True

    smiles_string = "CC"
    sdf_file_name = 'test.sdf'
    cif_restraints_file_name = "restraints.cif"
    pdb_out_file_name = "out.pdb"
    comp_id = "XXX"
    smiles_string = 'Cc1ccccc1'
    smiles_string = 'Oc1ccccc1'
    smiles_string = 'O=C(O)c1ccc(O)cc1'

    # python p2.py "O=C(C1=CC=CN(C1=O)Cc2ccccc2)N" LIG --no-mogul
    
    if len(sys.argv) < 3:
       print 'Usage: pyrogen SMILES-or-file comp-id [--no-mogul]'
    else:
        smiles_or_file = sys.argv[1]
        smiles_string = ''
        is_mdl_file_flag = False
        # if smiles_string ends in .smi, read it as if it was a file
        if is_mdl_file(smiles_or_file):
           is_mdl_file_flag = True
            
        file_stub = sys.argv[2] # e.g. "LIG"

        # mogul handling
        if len(sys.argv) == 4:
           if sys.argv[3] == '--no-mogul':
              run_mogul = False

        sdf_file_name = file_stub + ".sdf"
        cif_restraints_file_name = file_stub + '-pyrogen.cif'
        pdb_out_file_name        = file_stub + '-pyrogen.pdb'
        comp_id = file_stub

        if is_mdl_file(smiles_or_file):
           status = make_restraints_from_mdl(smiles_or_file, comp_id, sdf_file_name,
                                             pdb_out_file_name, cif_restraints_file_name)
        else:

           if is_comp_id(smiles_or_file):
              try:
                 smiles_string = get_smiles_from_comp_id(smiles_or_file)
                 status = make_restraints_from_smiles(smiles_string, comp_id, sdf_file_name,
                                                      pdb_out_file_name, cif_restraints_file_name)
              except TypeError, ex:
                 print "Type Error", ex
                 pass
           else:

              extension = os.path.splitext(smiles_or_file)[1]
              if (extension == '.cif'):

                 make_restraints_from_pdbx(smiles_or_file, comp_id, sdf_file_name,
                                           pdb_out_file_name, cif_restraints_file_name,
                                           quartet_planes, quartet_hydrogen_planes)

              else:
                 smiles_string = smiles_or_mdl_string
                 if is_smiles_file(smiles_or_file):
                    smiles_string = read_file(smiles_or_file)
                 status = make_restraints_from_smiles(smiles_string, comp_id, sdf_file_name,
                                                      pdb_out_file_name, cif_restraints_file_name)
   
   

if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option("-m", "--mmcif", dest="mmcif_file",
		      help="Read mmcif FILE", metavar="FILE")
    parser.add_option("-s", "--sdf", dest="sdf_file",
		      help="Read sdf/mol FILE", metavar="FILE")
    parser.add_option("-t", "--type", dest="comp_id", default='LIG',
		      help="Create restraints for this type")
    parser.add_option("-4", "--quartet-planes", dest="quartet_planes",
		      default=False,
		      help="Use 4-atom plane restraints",
                      action="store_true")
    parser.add_option("-c", "--consolidated-hydrogens", dest="quartet_hydrogen_planes",
		      default=True,
		      help="Use 4-atom hydrogen plane restraints",
                      action="store_false")
    parser.add_option("-n", "--no-mogul", dest="use_mogul",
		      default=True, action="store_false")
    parser.add_option("-T", "--tautomers", dest="show_tautomers",
		      default=False, action="store_true")
    parser.add_option("-q", "--quiet",
                  action="store_false", dest="verbose", default=True,
                  help="don't print status messages to stdout")

    
    (options, args) = parser.parse_args()


    # print 'DEBUG:: options:', options
    
    pdb_out_file_name        = options.comp_id + '-pyrogen.pdb'
    cif_restraints_file_name = options.comp_id + '-pyrogen.cif'
    sdf_file_name            = options.comp_id + "-pyrogen.sdf"

    # this is a bit ugly, perhaps.  this value is inspected inside
    # the following functions
    #
    if options.use_mogul == False:
       run_mogul = False

    if options.mmcif_file != None:
	make_restraints_from_pdbx(options.mmcif_file, options.comp_id, sdf_file_name,
                                  pdb_out_file_name, cif_restraints_file_name,
                                  options.quartet_planes, options.quartet_hydrogen_planes)
    else:
	if options.sdf_file != None:
           status = make_restraints_from_mdl(options.sdf_file, comp_id, sdf_file_name,
                                             pdb_out_file_name, cif_restraints_file_name)

        else:

	    if options.show_tautomers:
		if len(args) > 0:
		    smi_raw = args[0]
		    mol = Chem.MolFromSmiles(smi_raw)
		    results = tautomer.enumerate_tautomers(mol)
		    for m in results:
			print Chem.MolToSmiles(m)
		else:
		    print 'Need to provide SMILES molecule in tautomer mode'

	    else: 
		if len(args) > 0:
		    smi_raw = args[0]
		    print 'smi_raw', smi_raw
		    extension = os.path.splitext(smi_raw)[1]
		    print 'extension :' + extension + ':'
		    smiles_string = ''
		    if extension == '.smi':
			smiles_string = get_smiles_from_file(smi_raw)
		    else:
			smiles_string = smi_raw
		    status = make_restraints_from_smiles(smiles_string, options.comp_id, sdf_file_name,
							 pdb_out_file_name, cif_restraints_file_name,
							 options.quartet_planes, options.quartet_hydrogen_planes)
	    
