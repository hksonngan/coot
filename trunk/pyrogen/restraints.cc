
#include <boost/python.hpp>
#include "restraints.hh"
#include "py-restraints.hh"
#include <lidia-core/rdkit-interface.hh>
#include <utils/coot-utils.hh>
#include <coot-utils/coot-coord-utils.hh> 

// for minimization
#define HAVE_GSL
#include <ideal/simple-restraint.hh>


void
coot::mogul_out_to_mmcif_dict(const std::string &mogul_file_name,
			      const std::string &comp_id,
			      const std::string &compound_name,
			      const std::vector<std::string> &atom_names,
			      int n_atoms_all,
			      int n_atoms_non_hydrogen,
			      PyObject *bond_order_restraints_py,
			      const std::string &cif_file_name,
			      bool quartet_planes, bool quartet_hydrogen_planes) {

   coot::mogul mogul(mogul_file_name);
   coot::dictionary_residue_restraints_t bond_order_restraints = 
      monomer_restraints_from_python(bond_order_restraints_py);
   coot::dictionary_residue_restraints_t restraints = mogul.make_restraints(comp_id,
									    compound_name,
									    atom_names,
									    n_atoms_all,
									    n_atoms_non_hydrogen,
									    bond_order_restraints);
   restraints.write_cif(cif_file_name);

}

PyObject *
coot::mogul_out_to_mmcif_dict_by_mol(const std::string &mogul_file_name,
				     const std::string &comp_id,
				     const std::string &compound_name,
				     PyObject *rdkit_mol_py,
				     PyObject *bond_order_restraints_py,
				     const std::string &mmcif_out_file_name,
				     bool quartet_planes, bool quartet_hydrogen_planes) {
   
   // Thanks Uwe H.
   RDKit::ROMol &mol = boost::python::extract<RDKit::ROMol&>(rdkit_mol_py);
   coot::dictionary_residue_restraints_t bond_order_restraints = 
      monomer_restraints_from_python(bond_order_restraints_py);

   mogul mogul(mogul_file_name);
   std::vector<std::string> atom_names;
   unsigned int n_atoms_all = mol.getNumAtoms();
   unsigned int n_atoms_non_hydrogen = 0;

   for (unsigned int iat=0; iat<n_atoms_all; iat++) { 
      RDKit::ATOM_SPTR at_p = mol[iat];
      if (at_p->getAtomicNum() != 1)
	 n_atoms_non_hydrogen++;
      try {
	 std::string name = "";
	 at_p->getProp("name", name);
	 atom_names.push_back(name);
      }
      catch (const KeyErrorException &kee) {
	 std::cout << "caught no-name for atom exception in mogul_out_to_mmcif_dict_by_mol(): "
		   <<  kee.what() << std::endl;
      } 
   }

   dictionary_residue_restraints_t mogul_restraints =
      mogul.make_restraints(comp_id,
			    compound_name,
			    atom_names,
			    n_atoms_all,
			    n_atoms_non_hydrogen,
			    bond_order_restraints);


   dictionary_residue_restraints_t restraints =
      mmcif_dict_from_mol_using_energy_lib(comp_id, compound_name, rdkit_mol_py,
					   quartet_planes, quartet_hydrogen_planes);
   restraints.conservatively_replace_with(mogul_restraints);
   restraints.write_cif(mmcif_out_file_name);

   if (0) { // testing.
      std::pair<mmdb::Residue *, dictionary_residue_restraints_t> matched_restraints =
	 match_restraints_to_amino_acids(restraints, NULL);

      if (matched_restraints.first) {
	 std::cout << "......... about to write matched.cif" << std::endl;
	 matched_restraints.second.write_cif("matched.cif");
      }
   }

   return monomer_restraints_to_python(restraints);
      
}

std::pair<mmdb::Residue *, coot::dictionary_residue_restraints_t>
coot::match_restraints_to_amino_acids(const coot::dictionary_residue_restraints_t &restraints,
				      mmdb::Residue *residue_p) {

   mmdb::Residue *returned_res = NULL;
   dictionary_residue_restraints_t returned_dict;

   unsigned int n_comp_ids = 19;
   std::string comp_ids[] = { "CYS", "ASP", "GLU",        "HIS", "ILE", "LYS", "LEU", "MET",
			      "ASN", "PRO", "GLN", "ARG", "SER", "THR", "VAL", "TRP", "TYR",
			      "G", "C"};
   protein_geometry pg;
   int read_number = 0;
   for (unsigned int i=0; i<n_comp_ids; i++) { 
      pg.try_dynamic_add(comp_ids[i], i);
   }

   std::string out_comp_id = restraints.residue_info.comp_id;
   int best_n_matches = 0;
   std::string best_comp_id;
   
   for (unsigned int i=0; i<n_comp_ids; i++) {
      std::pair<bool, dictionary_residue_restraints_t> rest = pg.get_monomer_restraints(comp_ids[i]);
      if (rest.first) {
	 std::pair<unsigned int, dictionary_residue_restraints_t> r_new =
	    restraints.match_to_reference(rest.second, NULL, out_comp_id);
	 std::cout << "Using " << comp_ids[i] << " found " << r_new.first << " matches" << std::endl;
	 if (r_new.first > best_n_matches) {
	    best_n_matches = r_new.first;
	    best_comp_id = comp_ids[i];
	 }
      }
   }

   if (! best_comp_id.empty()) {
      returned_res = util::deep_copy_this_residue(residue_p);
      std::pair<bool, dictionary_residue_restraints_t> rest = pg.get_monomer_restraints(best_comp_id);
      std::pair<unsigned int, dictionary_residue_restraints_t> r_new =
	 restraints.match_to_reference(rest.second, returned_res, out_comp_id);
      returned_dict = r_new.second;
   } 
   return std::pair<mmdb::Residue *, coot::dictionary_residue_restraints_t> (returned_res, returned_dict);

}


PyObject *
coot::types_from_mmcif_dictionary(const std::string &file_name) {

   coot::protein_geometry geom;
   geom.set_verbose(false);
   int read_number = 0;
   geom.init_refmac_mon_lib(file_name, read_number);

   std::vector<std::string> types = geom.monomer_types();

   PyObject *l = PyList_New(types.size());
   for (unsigned int i=0; i<types.size(); i++) { 
      PyObject *o = PyString_FromString(types[i].c_str());
      PyList_SetItem(l, i, o);
   }
   return l;
} 


// write restraints and return restraints
// 
PyObject *
coot::mmcif_dict_from_mol(const std::string &comp_id,
			  const std::string &compound_name,
			  PyObject *rdkit_mol_py,
			  const std::string &mmcif_out_file_name,
			  bool quartet_planes, bool quartet_hydrogen_planes) {
   
   coot::dictionary_residue_restraints_t restraints =
      mmcif_dict_from_mol_using_energy_lib(comp_id, compound_name, rdkit_mol_py,
					   quartet_planes, quartet_hydrogen_planes);
   if (restraints.is_filled()) { 
      restraints.write_cif(mmcif_out_file_name);
      return monomer_restraints_to_python(restraints);
   } else {
      PyObject *o = new PyObject;
      o = Py_None;
      Py_INCREF(o);
      return o;
   } 
} 

coot::dictionary_residue_restraints_t
coot::mmcif_dict_from_mol_using_energy_lib(const std::string &comp_id,
					   const std::string &compound_name,
					   PyObject *rdkit_mol_py,
					   bool quartet_planes, bool quartet_hydrogen_planes) {

   coot::dictionary_residue_restraints_t restraints (comp_id, 1);
   
   RDKit::ROMol &mol = boost::python::extract<RDKit::ROMol&>(rdkit_mol_py);

   // Was there a user over-ride?
   std::string env_as_string;
   const char *env = getenv("ENERGY_LIB_CIF");

   // To CCP4 standard place then:
   if (! env) {
      const char *env_1 = getenv("CLIBD");
      if (env_1) {
	 env_as_string = std::string(env_1) + "/monomers/ener_lib.cif";
      } else {
	 // Coot standard place then
	 env_as_string = std::string(PKGDATADIR) + "/lib/data/monomers/ener_lib.cif";
      }
   } else {
      env_as_string = env;
   }

   if (env_as_string.empty()) {
      // restraints.is_fillled() is false
      std::cout << "ERROR:: no ENERGY_LIB_CIF env var" << std::endl;
   } else {

      // number of atom first
      // 
      unsigned int n_atoms_all = mol.getNumAtoms();
      unsigned int n_atoms_non_hydrogen = 0;
      for (unsigned int iat=0; iat<n_atoms_all; iat++)
	 if (mol[iat]->getAtomicNum() != 1)
	    n_atoms_non_hydrogen++;
      
      coot::energy_lib_t energy_lib(env_as_string);

      // fill with ener_lib values and then add mogul updates.
      // 
      restraints.residue_info.comp_id = comp_id;
      restraints.residue_info.three_letter_code = comp_id;
      restraints.residue_info.name = compound_name;
      restraints.residue_info.number_atoms_all = n_atoms_all;
      restraints.residue_info.number_atoms_nh = n_atoms_non_hydrogen;
      restraints.residue_info.group = "non-polymer";
      restraints.residue_info.description_level = "."; // default is full <smiley>
      
      coot::add_chem_comp_atoms(mol, &restraints); // alter restraints
      coot::fill_with_energy_lib_bonds(mol, energy_lib, &restraints); // alter restraints
      coot::fill_with_energy_lib_angles(mol, energy_lib, &restraints); // alter restraints
      coot::fill_with_energy_lib_torsions(mol, energy_lib, &restraints); // alter restraints

      int n_chirals = coot::assign_chirals(mol, &restraints); // alter restraints
      if (n_chirals) 
	 restraints.assign_chiral_volume_targets();

      coot::add_chem_comp_planes(mol, &restraints, quartet_planes, quartet_hydrogen_planes);
   }
   return restraints;
}

void
coot::fill_with_energy_lib_bonds(const RDKit::ROMol &mol,
				 const coot::energy_lib_t &energy_lib,
				 coot::dictionary_residue_restraints_t *restraints) {
   
   unsigned int n_bonds = mol.getNumBonds();
   for (unsigned int ib=0; ib<n_bonds; ib++) {
      const RDKit::Bond *bond_p = mol.getBondWithIdx(ib);
      int idx_1 = bond_p->getBeginAtomIdx();
      int idx_2 = bond_p->getEndAtomIdx();
      RDKit::ATOM_SPTR at_1 = mol[idx_1];
      RDKit::ATOM_SPTR at_2 = mol[idx_2];
      {
	 // put the lighter atom first (so that we find "Hxx ."  rather than "N .")
	 if (at_1->getAtomicNum() > at_2->getAtomicNum())
	    std::swap(at_1, at_2);
	 try {
	    std::string atom_type_1;
	    std::string atom_type_2;
	    std::string atom_name_1;
	    std::string atom_name_2;
	    at_1->getProp("atom_type", atom_type_1);
	    at_2->getProp("atom_type", atom_type_2);
	    at_1->getProp("name", atom_name_1);
	    at_2->getProp("name", atom_name_2);
	    try {
	       std::string bt = convert_to_energy_lib_bond_type(bond_p->getBondType());
	       energy_lib_bond bond =
		  energy_lib.get_bond(atom_type_1, atom_type_2, bt); // add bond type as arg
	       if (0)
		  std::cout << "....... " << atom_name_1 << " " << atom_name_2 << " types \""
			    << atom_type_1 << "\" \"" << atom_type_2
			    << "\" got bond " << bond << std::endl;
	       std::string bond_type = bond.type;
	       dict_bond_restraint_t bondr(atom_name_1, atom_name_2, bond_type, bond.length, bond.esd);
	       restraints->bond_restraint.push_back(bondr);
	    }
	    catch (const std::runtime_error &rte) {
	       std::cout << "WARNING:: error in adding bond restraint for bond number "
			 << ib << " " << rte.what() << std::endl;
	    } 
	 
	 }
	 catch (const KeyErrorException &kee) {
	    std::cout << "WARNING:: caugh KeyErrorException in fill_with_energy_lib_bonds() "
		      << std::endl;
	 }
      }
   }
}

void
coot::fill_with_energy_lib_angles(const RDKit::ROMol &mol,
				  const coot::energy_lib_t &energy_lib,
				  coot::dictionary_residue_restraints_t *restraints) {
   
   unsigned int n_atoms = mol.getNumAtoms();
   std::map<std::string, bool> done_angle;
   for (unsigned int iat_1=0; iat_1<n_atoms; iat_1++) { 
      RDKit::ATOM_SPTR at_1 = mol[iat_1];
      RDKit::ROMol::ADJ_ITER nbr_idx_1, end_nbrs_1;
      boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_1);
      while(nbr_idx_1 != end_nbrs_1){
	 const RDKit::ATOM_SPTR at_2 = mol[*nbr_idx_1];

	 RDKit::ROMol::ADJ_ITER nbr_idx_2, end_nbrs_2;
	 boost::tie(nbr_idx_2, end_nbrs_2) = mol.getAtomNeighbors(at_2);
	 while(nbr_idx_2 != end_nbrs_2){
	    const RDKit::ATOM_SPTR at_3 = mol[*nbr_idx_2];
	    if (at_3 != at_1) { 

	       try {
		  std::string atom_type_1;
		  std::string atom_type_2;
		  std::string atom_type_3;
		  std::string atom_name_1;
		  std::string atom_name_2;
		  std::string atom_name_3;
		  at_1->getProp("atom_type", atom_type_1);
		  at_2->getProp("atom_type", atom_type_2);
		  at_3->getProp("atom_type", atom_type_3);
		  at_1->getProp("name", atom_name_1);
		  at_2->getProp("name", atom_name_2);
		  at_3->getProp("name", atom_name_3);

		  try {

		     std::string dash("-");
		     std::string angle_key_name_1 = atom_name_1 + dash + atom_name_2 + dash + atom_name_3;
		     std::string angle_key_name_2 = atom_name_3 + dash + atom_name_2 + dash + atom_name_1;

		     if (done_angle.find(angle_key_name_1) == done_angle.end() &&
			 done_angle.find(angle_key_name_2) == done_angle.end()) { 
		     
			energy_lib_angle angle =
			   energy_lib.get_angle(atom_type_1, atom_type_2, atom_type_3);
			
			dict_angle_restraint_t angler(atom_name_1, atom_name_2, atom_name_3,
						      angle.angle, angle.angle_esd);

			restraints->angle_restraint.push_back(angler);
			done_angle[angle_key_name_1] = true;
			done_angle[angle_key_name_2] = true;
		     }
		  }
		  catch (const std::runtime_error &rte) {
		     std::cout << "WARNING:: error in adding angle restraint for atoms "
			       << at_1->getIdx() << " "
			       << at_2->getIdx() << " "
			       << at_3->getIdx() << " "
			       << rte.what() << std::endl;
		  } 
	       }
	       catch (const KeyErrorException &kee) {
		  std::cout << "WARNING:: caugh KeyErrorException in fill_with_energy_lib_angles() "
			    << std::endl;
	       }
	       
	       
	    }
	    ++nbr_idx_2;
	 }
	 
	 ++nbr_idx_1;
      }
   }
}

void
coot::fill_with_energy_lib_torsions(const RDKit::ROMol &mol,
				    const coot::energy_lib_t &energy_lib,
				    coot::dictionary_residue_restraints_t *restraints) {
   
   unsigned int n_atoms = mol.getNumAtoms();
   unsigned int tors_no = 1; // incremented on addition
   unsigned int const_no = 1; // incremented on addition.  When const_no is incremented, tors_no is not.
   std::map<std::string, bool> done_torsion;
   bool debug = false;
   
   for (unsigned int iat_1=0; iat_1<n_atoms; iat_1++) { 
      RDKit::ATOM_SPTR at_1 = mol[iat_1];

      RDKit::ROMol::ADJ_ITER nbr_idx_1, end_nbrs_1;
      boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_1);
      while(nbr_idx_1 != end_nbrs_1){
	 const RDKit::ATOM_SPTR at_2 = mol[*nbr_idx_1];

	 RDKit::ROMol::ADJ_ITER nbr_idx_2, end_nbrs_2;
	 boost::tie(nbr_idx_2, end_nbrs_2) = mol.getAtomNeighbors(at_2);
	 while(nbr_idx_2 != end_nbrs_2){
	    const RDKit::ATOM_SPTR at_3 = mol[*nbr_idx_2];
	    if (at_3 != at_1) {

	       RDKit::ROMol::ADJ_ITER nbr_idx_3, end_nbrs_3;
	       boost::tie(nbr_idx_3, end_nbrs_3) = mol.getAtomNeighbors(at_3);
	       while(nbr_idx_3 != end_nbrs_3){
		  const RDKit::ATOM_SPTR at_4 = mol[*nbr_idx_3];
		  if (at_4 != at_2 && at_4 != at_1) {

		     // now do something with those indices

		     try {
			std::string atom_type_1;
			std::string atom_type_2;
			std::string atom_type_3;
			std::string atom_type_4;
			std::string atom_name_1;
			std::string atom_name_2;
			std::string atom_name_3;
			std::string atom_name_4;
			at_1->getProp("atom_type", atom_type_1);
			at_2->getProp("atom_type", atom_type_2);
			at_3->getProp("atom_type", atom_type_3);
			at_4->getProp("atom_type", atom_type_4);
			at_1->getProp("name", atom_name_1);
			at_2->getProp("name", atom_name_2);
			at_3->getProp("name", atom_name_3);
			at_4->getProp("name", atom_name_4);

			// if we have not done this atom-2 <-> atom-3 torsion before...
			//
			std::string torsion_key_name_1;
			std::string torsion_key_name_2;
			torsion_key_name_1  = atom_name_2;
			torsion_key_name_1 += "-";
			torsion_key_name_1 += atom_name_3;
			
			torsion_key_name_2  = atom_name_3;
			torsion_key_name_2 += "-";
			torsion_key_name_2 += atom_name_2;

			bool done_this_already = true;
			if (done_torsion.find(torsion_key_name_1) == done_torsion.end() &&
			    done_torsion.find(torsion_key_name_2) == done_torsion.end())
			   done_this_already = false;

			if (debug) { 
			   std::cout << "considering torsion keys \"" << torsion_key_name_1 << "\" and \""
				     << torsion_key_name_2 << "\"";
			   if (done_this_already)
			      std::cout << " done already" << std::endl;
			   else
			      std::cout << std::endl;
			}

			if (! done_this_already) {

			   if (debug) 
			      std::cout << "torsion-atoms..... "
					<< at_1->getIdx() << " "
					<< at_2->getIdx() << " "
					<< at_3->getIdx() << " "
					<< at_4->getIdx() << " "
					<< atom_name_1 << " " 
					<< atom_name_2 << " " 
					<< atom_name_3 << " " 
					<< atom_name_4 << " " 
					<< std::endl;

			   // some of the time we may try to get a torsion that does not
			   // correspond to anything in the dictionary (with the given
			   // atom types).  In that case, we will catch a runtime_error.
			   // What to do then is not clear to me yet.
			   // 
			   try {
			      if (debug)
				 std::cout << "trying to get torsion-from-lib for " << atom_type_2
					   << "---" << atom_type_3 << std::endl;
			      energy_lib_torsion tors =
				 energy_lib.get_torsion(atom_type_2, atom_type_3);
			      
			      bool is_const = is_const_torsion(mol, at_2.get(), at_3.get());

			      if (debug)
				 std::cout << "    torsion between a " << atom_type_2 << " and a "
					   << atom_type_3 << " gave torsion " << tors << "  is_const: "
					   << is_const << std::endl;

			      if (tors.period != 0) { 
				 double esd = 20.0;
				 std::string tors_id;
				 if (! is_const) { 
				    tors_id = "var_";
				    tors_id += util::int_to_string(tors_no);
				    tors_no++;
				 } else {
				    tors_id = "CONST_";
				    tors_id += util::int_to_string(const_no);
				    const_no++;
				 }
				 dict_torsion_restraint_t torsionr(tors_id,
								   atom_name_1, atom_name_2,
								   atom_name_3, atom_name_4,
								   tors.angle, esd, tors.period);
				 restraints->torsion_restraint.push_back(torsionr);
			      }
			   }
			   catch (const std::runtime_error &rte) {

			      if (debug)
				 std::cout << "failed  to get torsion-from-lib for " << atom_type_2
					   << " to " << atom_type_3 << std::endl;
			      
			      // default
			      double angle = 180;
			      double esd = 20;
			      int period = 1;
			      
			      bool is_const = is_const_torsion(mol, at_2.get(), at_3.get());
			      RDKit::Atom::HybridizationType ht_2 = at_2->getHybridization();
			      RDKit::Atom::HybridizationType ht_3 = at_3->getHybridization();

			      if (ht_2 == RDKit::Atom::SP3 || ht_3 == RDKit::Atom::SP3) {
				 period = 3;
				 angle = 60;
				 if (is_const) esd = 2;
			      }

			      if ((ht_2 == RDKit::Atom::SP2 && ht_3 == RDKit::Atom::SP3) ||
				  (ht_2 == RDKit::Atom::SP3 && ht_3 == RDKit::Atom::SP2)) {
				 period = 2;
				 angle = 90;
				 if (is_const) esd = 2;
			      }


			      if (ht_2 == RDKit::Atom::SP2 && ht_3 == RDKit::Atom::SP2) {
				 period = 2;
				 if (is_const) esd = 2;
			      } 

			      if (ht_2 == RDKit::Atom::SP || ht_3 == RDKit::Atom::SP) {
				 is_const = 1;
				 esd = 2;
			      }

			      std::string tors_id("var_");
			      if (is_const) {
				 tors_id="CONST_";
				 tors_id+=util::int_to_string(const_no);
				 const_no++;
			      } else {
				 tors_id+=util::int_to_string(tors_no);
				 tors_no++;
			      }

			      dict_torsion_restraint_t torsionr(tors_id,
								atom_name_1, atom_name_2,
								atom_name_3, atom_name_4,
								angle, esd, period);
			      if (debug)
				 std::cout << "fallback pushback::" << torsionr << std::endl;
			      restraints->torsion_restraint.push_back(torsionr);
			      
			   } 
			   done_torsion[torsion_key_name_1] = true;
			   done_torsion[torsion_key_name_2] = true;
			}
		     }
		     catch (const KeyErrorException &kee) {
			std::cout << "WARNING:: caugh KeyErrorException in fill_with_energy_lib_angles() "
				  << std::endl;
		     }
		  }
		  ++nbr_idx_3;
	       }
	    }
	    ++nbr_idx_2;
	 }
	 ++nbr_idx_1;
      }
   }
}


bool
coot::is_const_torsion(const RDKit::ROMol &mol,
		       const RDKit::Atom *torsion_at_2,
		       const RDKit::Atom *torsion_at_3) {

   // is the bond between the 2 central atoms of the torsion a double bond?
   // (or triple or atromatic).  If so, then this is a const torsion

   bool status = false;
   
   unsigned int n_bonds = mol.getNumBonds();
   for (unsigned int ib=0; ib<n_bonds; ib++) {
      const RDKit::Bond *bond_p = mol.getBondWithIdx(ib);
      RDKit::Atom *bond_at_1 = bond_p->getBeginAtom();
      RDKit::Atom *bond_at_2 = bond_p->getEndAtom();

      bool found_torsion_bond = false;
      if (torsion_at_2 == bond_at_1)
	 if (torsion_at_3 == bond_at_2)
	    found_torsion_bond = true;
      if (torsion_at_2 == bond_at_2)
	 if (torsion_at_3 == bond_at_1)
	    found_torsion_bond = true;

      if (found_torsion_bond) { 
	 if (bond_p->getBondType() == RDKit::Bond::AROMATIC)    status = true;
	 if (bond_p->getBondType() == RDKit::Bond::DOUBLE)      status = true;
	 if (bond_p->getBondType() == RDKit::Bond::TRIPLE)      status = true;
	 if (bond_p->getBondType() == RDKit::Bond::QUADRUPLE)   status = true;
	 if (bond_p->getBondType() == RDKit::Bond::ONEANDAHALF) status = true;
	 if (bond_p->getBondType() == RDKit::Bond::TWOANDAHALF) status = true;
	 break;
      }
   }

   return status;

} 


void
coot::add_chem_comp_atoms(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   int iconf = 0;
   unsigned int n_atoms = mol.getNumAtoms();
   for (unsigned int iat=0; iat<n_atoms; iat++) { 
      RDKit::ATOM_SPTR at_p = mol[iat];
      try {
	 std::string name;
	 std::string atom_type;
	 double charge;
	 bool have_charge = true; // can be clever with GetProp() KeyErrorException
	                          // if you like
	 at_p->getProp("name", name);
	 at_p->getProp("atom_type", atom_type);
	 at_p->getProp("_GasteigerCharge", charge);
	 std::pair<bool, float> charge_pair(have_charge, charge);

	 // std::cout << "in add_chem_comp_atoms() charge of " << iat << " " << charge << std::endl;

	 dict_atom atom(name, name, at_p->getSymbol(), atom_type, charge_pair);

 	 RDKit::Conformer conf = mol.getConformer(iconf);
 	 RDGeom::Point3D &r_pos = conf.getAtomPos(iat);
 	 clipper::Coord_orth pos(r_pos.x, r_pos.y, r_pos.z);
	 atom.model_Cartn = std::pair<bool, clipper::Coord_orth> (true, pos);
	 
	 restraints->atom_info.push_back(atom);
      }
      catch (const KeyErrorException &kee) {
	 std::cout << "WARNING:: caught property exception in add_chem_comp_atoms()"
		   << iat << std::endl;
      }
   }
}

// what fun!
// C++ smarts
void
coot::add_chem_comp_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints,
			   bool quartet_planes, bool quartet_hydrogen_planes) {

   add_chem_comp_aromatic_planes(mol, restraints, quartet_planes, quartet_hydrogen_planes);
   add_chem_comp_deloc_planes(mol, restraints);
   restraints->remove_redundant_plane_restraints();
   restraints->reweight_subplanes();
   add_chem_comp_sp2_N_planes(mol, restraints);
}

// what fun!
// C++ smarts
void
coot::add_chem_comp_aromatic_planes(const RDKit::ROMol &mol,
				    coot::dictionary_residue_restraints_t *restraints,
				    bool quartet_planes, bool quartet_hydrogen_planes) {

   std::vector<std::string> patterns;

   // I am not sure that fused ring systems are a good idea.
   // 
   //    patterns.push_back("a12aaaaa1aaaa2");

   patterns.push_back("a12aaaaa1aaa2"); // 6-5 is OK, I think
   
   patterns.push_back("a1aaaaa1");
   patterns.push_back("a1aaaa1");
   patterns.push_back("[*;^2]1[*;^2][*;^2][A;^2][*;^2]1"); // non-aromatic 5-ring

   int plane_id_idx = 1; 
   for (unsigned int ipat=0; ipat<patterns.size(); ipat++) {
      RDKit::ROMol *query = RDKit::SmartsToMol(patterns[ipat]);
      std::vector<RDKit::MatchVectType>  matches;
      bool recursionPossible = true;
      bool useChirality = true;
      bool uniquify = true;
      int matched = RDKit::SubstructMatch(mol,*query,matches,uniquify,recursionPossible, useChirality);
      for (unsigned int imatch=0; imatch<matches.size(); imatch++) { 
	 if (matches[imatch].size() > 0) {

	    std::cout << "INFO:: matched aromatic plane: " << std::setw(14) << std::right
		      << patterns[ipat];
	    std::cout << " ("; 
	    for (unsigned int iat=0; iat<matches[imatch].size(); iat++) { 
	       unsigned int atom_idx = matches[imatch][iat].second;
	       try {
		  RDKit::ATOM_SPTR at_p = mol[atom_idx];
		  std::string atom_name;
		  at_p->getProp("name", atom_name);
		  std::cout << " " << atom_name;
	       }
	       catch (const KeyErrorException &kee) {
		  std::cout << " " << atom_idx;
	       } 
	    }
	    std::cout << " )";
	    std::cout << std::endl; 

	    if (! quartet_planes) { 
	       dict_plane_restraint_t plr =
		  add_chem_comp_aromatic_plane_all_plane(matches[imatch], mol,
							 plane_id_idx,
							 quartet_hydrogen_planes);
	       if (! plr.empty()) {
		  restraints->plane_restraint.push_back(plr);
		  plane_id_idx++;
	       }
	    } else {
	       // Don't add hydrogen quartets (that's done later)
	       int n_added =
		  add_chem_comp_aromatic_plane_quartet_planes(matches[imatch], mol, restraints, plane_id_idx);
	       plane_id_idx += n_added;
	    } 
	 }
      }
   }

   if (quartet_hydrogen_planes || quartet_planes) {
      add_quartet_hydrogen_planes(mol, restraints);
   }
}

// modify restraints
void
coot::add_quartet_hydrogen_planes(const RDKit::ROMol &mol,
				  coot::dictionary_residue_restraints_t *restraints) { 

   int h_plane_quartet_id_idx = 1; // for the first
      
   // Find hydrogens that are connected to an sp2 atom and make a
   // plane of the sp2 atom and its neighbours (including this
   // hydrogen of course).
   unsigned int n_atoms = mol.getNumAtoms();
   for (unsigned int iat_1=0; iat_1<n_atoms; iat_1++) { 
      RDKit::ATOM_SPTR at_1 = mol[iat_1];
      if (at_1->getAtomicNum() == 1) {
	 std::vector<unsigned int> quartet_indices;

	 RDKit::ROMol::ADJ_ITER nbr_idx_1, end_nbrs_1;
	 boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_1);
	 while(nbr_idx_1 != end_nbrs_1){
	    const RDKit::ATOM_SPTR at_centre = mol[*nbr_idx_1];
	       
	    if (at_centre->getHybridization() == RDKit::Atom::SP2) {

	       quartet_indices.push_back(*nbr_idx_1); // the idx of atom to which the H is connected
	       RDKit::ROMol::ADJ_ITER nbr_idx_2, end_nbrs_2;
	       boost::tie(nbr_idx_2, end_nbrs_2) = mol.getAtomNeighbors(at_centre);
	       while(nbr_idx_2 != end_nbrs_2){
		  quartet_indices.push_back(*nbr_idx_2);
		  ++nbr_idx_2;
	       }
	    }
	    ++nbr_idx_1;
	 }

	 // OK! We found a H-quartet. Add it.

	 if (quartet_indices.size() == 4) {
	    try {
	       std::vector<std::string> quartet_names;
	       for (unsigned int jj=0; jj<quartet_indices.size(); jj++) {
		  std::string name;
		  mol[quartet_indices[jj]]->getProp("name", name);
		  if (0)
		     std::cout << "Quartet " << h_plane_quartet_id_idx << ": "
			       << quartet_indices[jj] << " " << name << std::endl;
		  quartet_names.push_back(name);
	       }
	       std::string quartet_plane_id = "H-quartet-" + util::int_to_string(h_plane_quartet_id_idx);

	       if (0) { // debug
		  std::cout << "Adding plane " << quartet_plane_id << " with atoms ";
		  for (unsigned int iat=0; iat<quartet_names.size(); iat++) { 
		     std::cout << quartet_names[iat] << " ";
		  }
		  std::cout << std::endl;
	       }

	       
	       double dist_esd = 0.02;
	       coot::dict_plane_restraint_t rest(quartet_plane_id, quartet_names, dist_esd);
	       restraints->plane_restraint.push_back(rest);
	       h_plane_quartet_id_idx++;
	    }
	    catch (const KeyErrorException &kee) {
	       std::cout << "Badness missing atom name in H-quartet" << std::endl;
	    }
	 }
      } 
   } 
}

coot::dict_plane_restraint_t
coot::add_chem_comp_aromatic_plane_all_plane(const RDKit::MatchVectType &match,
					     const RDKit::ROMol &mol,
					     int plane_id_idx,
					     bool quartet_hydrogen_planes) {

   coot::dict_plane_restraint_t plane_restraint; // returend value, empty initially

   std::string plane_id = "plane-arom-" + util::int_to_string(plane_id_idx);
   std::vector<std::string> plane_restraint_atoms; 
   try {
      for (unsigned int ii=0; ii<match.size(); ii++) {
	 RDKit::ATOM_SPTR at_p = mol[match[ii].second];

	 // only add this atom to a plane restraint if it not
	 // already in a plane restraint.  Test by failing to
	 // get the plane_id property.

	 bool add_atom_to_plane = true;

	 if ((at_p->getAtomicNum() != 1) || !quartet_hydrogen_planes) {

	    // add the atom to the plane if the plane that it is
	    // already in is not this plane.
	    // 
	    try {
	       std::string atom_plane;
	       at_p->getProp("plane_id", atom_plane);
	       if (atom_plane == plane_id)
		  add_atom_to_plane = false;
	    }
	    catch (const KeyErrorException &kee) {
	       add_atom_to_plane = true;
	    }
	    // the following exception is needed for my Ubuntu 10.04 machine, 
	    // don't know why: fixes:
	    // terminate called after throwing an instance of 'KeyErrorException'
	    //   what():  std::exception
	    // 
	    catch (const std::exception &stde) {
	       add_atom_to_plane = true;
	    }

	    if (add_atom_to_plane) {
		     
	       std::string name = "";
	       at_p->getProp("name", name);
	       // add name if it is not already in the vector
	       if (std::find(plane_restraint_atoms.begin(), plane_restraint_atoms.end(), name) ==
		   plane_restraint_atoms.end())
		  plane_restraint_atoms.push_back(name);
	       at_p->setProp("plane_id", plane_id);

	       // debug
	       if (0) { 
		  std::string plane_id_lookup_debug; 
		  at_p->getProp("plane_id", plane_id_lookup_debug);
		  std::cout << "debug:: set atom " << name << " to plane_id "
			    << plane_id_lookup_debug << std::endl;
	       }

	       // run through neighours, because neighours of
	       // aromatic system atoms are in the plane too.
	       // 
	       RDKit::ROMol::ADJ_ITER nbr_idx_1, end_nbrs_1;
	       boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_p);
	       std::vector<RDKit::ATOM_SPTR> attached_atoms;
	       while(nbr_idx_1 != end_nbrs_1) {
		  const RDKit::ATOM_SPTR at_2 = mol[*nbr_idx_1];
		  // add if not a hydrogen or we are not doing quartet hydrogen planes
		  if (at_2->getAtomicNum() != 1 || !quartet_hydrogen_planes)
		     attached_atoms.push_back(at_2);
		  ++nbr_idx_1;
	       }
	       if (attached_atoms.size() == 3) {
			
		  // Yes, there was something
		  for (unsigned int iattached=0; iattached<attached_atoms.size(); iattached++) { 
		     try {
			std::string attached_atom_name;
			attached_atoms[iattached]->getProp("name", attached_atom_name);
			// add it if it is not already in a plane,
			// 
			if (std::find(plane_restraint_atoms.begin(),
				      plane_restraint_atoms.end(),
				      attached_atom_name) ==
			    plane_restraint_atoms.end())
			   plane_restraint_atoms.push_back(attached_atom_name);
		     }
		     catch (const KeyErrorException &kee) {
			// do nothing then (no name found)
		     }
		  }
	       }
	    }
	 }
      }

      // make a plane restraint with those atoms in then
      if (plane_restraint_atoms.size() > 3) {
	 mmdb::realtype dist_esd = 0.02;
	 coot::dict_plane_restraint_t rest(plane_id, plane_restraint_atoms, dist_esd);
	 plane_restraint = rest;
      } 
   }
   
   catch (const KeyErrorException &kee) {
      // this should not happen
      std::cout << "WARNING:: add_chem_comp_planes() failed to get atom name "
		<< std::endl;
   } 

   // std::cout << "returning plane_restraint with " << plane_restraint.n_atoms() << " atoms" << std::endl;
   return plane_restraint;
}


// Return the number of added planes.
// 
// Don't add hydrogen quartets (that's done later).
// 
int
coot::add_chem_comp_aromatic_plane_quartet_planes(const RDKit::MatchVectType &match,
						  const RDKit::ROMol &mol,
						  coot::dictionary_residue_restraints_t *restraints,
						  int plane_id_idx_in) {

   std::vector<quartet_set> quartet_sets_vec;
   
   int n_planes = 0;
   try {
      for (unsigned int ii=0; ii<match.size(); ii++) {
	 RDKit::ATOM_SPTR at_p = mol[match[ii].second];
	 if (at_p->getAtomicNum() != 1) {

	    if (0) {
	       std::string name;
	       at_p->getProp("name", name);
	       std::cout << "--------- considering core atom " << match[ii].second
			 << " " << name << std::endl;
	    }
	    
	    // What are the neighbour of this atom? Are there more
	    // than 2 of them?  If so, let's make a plane restraint.

	    std::vector<unsigned int> quartet_indices;
	    quartet_indices.push_back(match[ii].second);
	    RDKit::ROMol::ADJ_ITER nbr_idx_1, end_nbrs_1;
	    boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_p);
	    while(nbr_idx_1 != end_nbrs_1){
	       const RDKit::ATOM_SPTR at_neighb = mol[*nbr_idx_1];
	       if (at_neighb->getAtomicNum() != 1) {
		  quartet_indices.push_back(*nbr_idx_1);
	       }
	       ++nbr_idx_1;
	    }

	    
	    if (quartet_indices.size() > 3) {

	       if (0) {  // debug
		  std::cout << "debug quartet_indices.size() 3 legs path: "
			    << quartet_indices.size() << std::endl;
		  for (unsigned int jj=0; jj<quartet_indices.size(); jj++) { 
		     std::string name;
		     mol[quartet_indices[jj]]->getProp("name", name);
		     std::cout << "   " << name;
		  }
		  std::cout << std::endl;
	       }


	       quartet_set q(quartet_indices);
	       quartet_sets_vec.push_back(q);

	    } else {

	       // We need neighbours of neighbours then:
	       //
	       std::vector<unsigned int> quartet_indices;
	       quartet_indices.push_back(match[ii].second);
	       
	       RDKit::ROMol::ADJ_ITER nbr_idx_1, end_nbrs_1;
	       boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_p);
	       while(nbr_idx_1 != end_nbrs_1){
		  if (mol[*nbr_idx_1]->getAtomicNum() != 1) {
		     quartet_indices.push_back(*nbr_idx_1);
		  }
		  ++nbr_idx_1;
	       }


	       // OK quartet_indices should be 3 now.  Root atom and
	       // its two neighbours.
	       //
	       if (0) {  // debug
		  std::cout << "debug quartet_indices.size() (should be 3): "
			    << quartet_indices.size() << std::endl;
		  for (unsigned int jj=0; jj<quartet_indices.size(); jj++) { 
		     std::string name;
		     mol[quartet_indices[jj]]->getProp("name", name);
		     std::cout << "   " << name;
		  }
		  std::cout << std::endl;
	       }

	       // Now neighbours, then neighbours of neighbours
	       //
	       boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_p);
	       while(nbr_idx_1 != end_nbrs_1){
		  const RDKit::ATOM_SPTR at_1 = mol[*nbr_idx_1];
		  if (at_1->getAtomicNum() != 1) {

		     RDKit::ROMol::ADJ_ITER nbr_idx_2, end_nbrs_2;
		     boost::tie(nbr_idx_2, end_nbrs_2) = mol.getAtomNeighbors(at_1);
		     while(nbr_idx_2 != end_nbrs_2){

			if (mol[*nbr_idx_2]->getAtomicNum() != 1) {
			   std::vector<unsigned int> local_quartet = quartet_indices;

			   // Add this atom if it's not already in the quartet
			   if (std::find(local_quartet.begin(),
					 local_quartet.end(),
					 *nbr_idx_2) == local_quartet.end()) {
			      local_quartet.push_back(*nbr_idx_2);
			      quartet_sets_vec.push_back(local_quartet);
			   }
			}
			++nbr_idx_2;
		     }
		  }
		  ++nbr_idx_1;
	       }
	    } 
	 } 
      }

      // std::cout << "got quartet_sets_vec.size(): " << quartet_sets_vec.size() << std::endl;
      n_planes = quartet_sets_vec.size();

      for (unsigned int i=0; i<quartet_sets_vec.size(); i++) { 
	 const quartet_set &q = quartet_sets_vec[i];
	 std::vector<std::string> atom_names;
	 for (unsigned int iat=0; iat<4; iat++) { 
	    const RDKit::ATOM_SPTR at = mol[q[iat]];
	    std::string name;
	    at->getProp("name", name);
	    atom_names.push_back(name);
	 }
	 if (atom_names.size() > 3) {
	    double esd = 0.014;
	    std::string plane_id = "quartet-plane-" + util::int_to_string(plane_id_idx_in+i);

	    if (0) { // debug
	       std::cout << "Adding plane " << plane_id << " with atoms ";
	       for (unsigned int iat=0; iat<atom_names.size(); iat++) { 
		  std::cout << atom_names[iat] << " ";
	       }
	       std::cout << std::endl;
	    } 
	    
	    dict_plane_restraint_t pr(plane_id, atom_names, esd);
	    restraints->plane_restraint.push_back(pr);
	    n_planes++;
	 }
      }
   }
   catch (const KeyErrorException &kee) {
      // this should not happen
      std::cout << "WARNING:: add_chem_comp_aromatic_plane_quartet_planes() failed to get atom name "
		<< std::endl;
   }

   return n_planes;
} 



void
coot::add_chem_comp_deloc_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   typedef std::pair<std::string, double> d_pat;

   std::vector<d_pat> patterns;
   patterns.push_back(d_pat("*C(=O)[O;H]",                 0.02));  // ASP
   patterns.push_back(d_pat("AC(=O)[N^2;H2,H1]([H])[A,H]", 0.02));  // ASN
   patterns.push_back(d_pat("*C(=N)[N^2;H2]([H])[A,H]",    0.02));  // amidine
   patterns.push_back(d_pat("CNC(=[NH])N([H])[H]",         0.02));  // guanidinium with H - testing
   patterns.push_back(d_pat("CNC(=[NH])N",                 0.02));  // guanidinium sans Hs

   // Martin's pattern, these should be weaker though, I think
   patterns.push_back(d_pat("[*^2]=[*^2]-[*^2]=[*;X1;^2]", 0.04));
   patterns.push_back(d_pat("[a^2]:[a^2]-[*^2]=[*;X1;^2]", 0.04));
   
   int n_planes = 1; 
   for (unsigned int ipat=0; ipat<patterns.size(); ipat++) {
      RDKit::ROMol *query = RDKit::SmartsToMol(patterns[ipat].first);
      std::vector<RDKit::MatchVectType>  matches;
      bool recursionPossible = true;
      bool useChirality = true;
      bool uniquify = true;
      int matched = RDKit::SubstructMatch(mol,*query,matches,uniquify,recursionPossible, useChirality);
      for (unsigned int imatch=0; imatch<matches.size(); imatch++) { 
	 if (matches[imatch].size() > 0) {
	    if (0) // debug
	       std::cout << "matched deloc plane pattern: " << patterns[ipat].first << std::endl;
	    std::vector<std::string> atom_names;
	    std::string plane_id = "plane-deloc-";
	    char s[100];
	    snprintf(s,99,"%d", n_planes);
	    plane_id += std::string(s);
	    try {
	       std::vector<std::string> atom_names;
	       for (unsigned int ii=0; ii<matches[imatch].size(); ii++) {
		  RDKit::ATOM_SPTR at_p = mol[matches[imatch][ii].second];

		  // Unlike aromatics, the atoms of this type of plane
		  // can be in more than one plane.

		  std::string name = "";
		  at_p->getProp("name", name);
		  at_p->setProp("plane_id", plane_id);
		  // std::cout << "... marking " << name << " as in " << plane_id << std::endl;
		  atom_names.push_back(name);
	       }
	       if (atom_names.size() > 3) { 
		  mmdb::realtype dist_esd = patterns[ipat].second;
		  coot::dict_plane_restraint_t res(plane_id, atom_names, dist_esd);
		  restraints->plane_restraint.push_back(res);
	       }
	    }

	    catch (const KeyErrorException &kee) {
	       std::cout << "ERROR:: in add_chem_comp_planes_deloc failed to get atom name"
			 << std::endl;
	    } 
	    n_planes++;
	 }
      }
   }
} 


void
coot::add_chem_comp_sp2_N_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   typedef std::pair<std::string, double> d_pat;
   std::vector<d_pat> patterns;
   patterns.push_back(d_pat("[c,C][N^2;H2]([H])[H]", 0.02));  // N6 on Adenosine.
                                                              // Should the Hs be replaced by *s?
   int n_planes = 1; // counter for output text
   for (unsigned int ipat=0; ipat<patterns.size(); ipat++) {
      RDKit::ROMol *query = RDKit::SmartsToMol(patterns[ipat].first);
      std::vector<RDKit::MatchVectType>  matches;
      bool recursionPossible = true;
      bool useChirality = true;
      bool uniquify = true;
      int matched = RDKit::SubstructMatch(mol,*query,matches,uniquify,recursionPossible, useChirality);
      if (0) // debug
	 std::cout << "Matched " << matched << " sp2 N planes" << std::endl;
      for (unsigned int imatch=0; imatch<matches.size(); imatch++) { 
	 if (matches[imatch].size() > 0) {
	    std::cout << "matched sp2 N plane pattern: " << patterns[ipat].first << std::endl;
	    std::string plane_id = "plane-sp2-N-";
	    char s[100];
	    snprintf(s,99,"%d", n_planes);
	    plane_id += std::string(s);
	    try {
	       std::vector<std::string> atom_names;
	       for (unsigned int ii=0; ii<matches[imatch].size(); ii++) {
		  RDKit::ATOM_SPTR at_p = mol[matches[imatch][ii].second];

		  // Unlike aromatics, the atoms of this type of plane
		  // can be in more than one plane.

		  std::string name = "";
		  at_p->getProp("name", name);
		  at_p->setProp("plane_id", plane_id);
		  atom_names.push_back(name);
	       }
	       if (atom_names.size() > 3) { 
		  mmdb::realtype dist_esd = patterns[ipat].second;
		  coot::dict_plane_restraint_t res(plane_id, atom_names, dist_esd);
		  restraints->plane_restraint.push_back(res);
	       }
	    }
	    catch (const KeyErrorException &kee) {
		  
	    }
	    n_planes++;
	 }
      }
   }
}

// alter restraints.
int 
coot::assign_chirals(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   int n_chirals = assign_chirals_mmcif_tags(mol, restraints);
   if (! n_chirals) {
      n_chirals = assign_chirals_rdkit_tags(mol, restraints);
   }
   return n_chirals;
}

#include "rdkit/RDBoost/Exceptions.h"
   
// alter restraints.
int 
coot::assign_chirals_mmcif_tags(const RDKit::ROMol &mol,
				coot::dictionary_residue_restraints_t *restraints) {

   int n_chirals = 0;

   unsigned int n_atoms = mol.getNumAtoms();
   for (unsigned int iat=0; iat<n_atoms; iat++) { 
      RDKit::ATOM_SPTR at_p = mol[iat];
      try {
	 std::string ch;
	 std::string chiral_centre, n1, n2, n3;
	 at_p->getProp("name", chiral_centre);
	 at_p->getProp("mmcif_chiral_volume_sign", ch);
	 at_p->getProp("mmcif_chiral_N1", n1);
	 at_p->getProp("mmcif_chiral_N2", n2);
	 at_p->getProp("mmcif_chiral_N3", n3);
	 int cv = dict_chiral_restraint_t::CHIRAL_VOLUME_RESTRAINT_VOLUME_SIGN_UNASSIGNED;
	 if (ch == "positive")
	    cv = dict_chiral_restraint_t::CHIRAL_RESTRAINT_POSITIVE;
	 if (ch == "negative")
	    cv = dict_chiral_restraint_t::CHIRAL_RESTRAINT_NEGATIVE;
	 if (ch == "both")
	    cv = dict_chiral_restraint_t::CHIRAL_RESTRAINT_BOTH;
	 
	 std::string chiral_id = "chiral_" + util::int_to_string(n_chirals+1);
	 dict_chiral_restraint_t cr(chiral_id, chiral_centre, n1, n2, n3, cv);
	 restraints->chiral_restraint.push_back(cr);
	 n_chirals++;
      }
      catch (const KeyErrorException &kee) {
	 // it's OK for this to happen.  Most atoms are not chiral.
	 // Most input molecules are not from mmcif files.
	 // 
	 // std::cout << "oops caught exception " << kee.key() << " "
	 // << kee.what() << std::endl;
      }
      // This shouldn't be needed because the exception should only be
      // KeyErrorException, and that should be caught by the block above.
      catch (const std::runtime_error &rte) {

	 if (0)
	    std::cout << "assign_chirals_mmcif_tags() strange - caught runtime_error "
		      << rte.what() << std::endl;
      } 
   }
   return n_chirals;
}
 
// alter restraints.
int 
coot::assign_chirals_rdkit_tags(const RDKit::ROMol &mol,
				coot::dictionary_residue_restraints_t *restraints) {
   
   int vol_sign = coot::dict_chiral_restraint_t::CHIRAL_VOLUME_RESTRAINT_VOLUME_SIGN_UNASSIGNED;

   int n_chirals = 0;

   unsigned int n_atoms = mol.getNumAtoms();
   for (unsigned int iat=0; iat<n_atoms; iat++) { 
      RDKit::ATOM_SPTR at_p = mol[iat];
      RDKit::Atom::ChiralType chiral_tag = at_p->getChiralTag();
      // std::cout << "atom " << iat << " chiral tag: " << chiral_tag << std::endl;

      if (chiral_tag == RDKit::Atom::CHI_TETRAHEDRAL_CCW)
	 vol_sign = dict_chiral_restraint_t::CHIRAL_RESTRAINT_NEGATIVE;
      if (chiral_tag == RDKit::Atom::CHI_TETRAHEDRAL_CW)
	 vol_sign = dict_chiral_restraint_t::CHIRAL_RESTRAINT_POSITIVE;

      if (chiral_tag != RDKit::Atom::CHI_UNSPECIFIED) {
	 try { 
	    std::string chiral_centre;
	    at_p->getProp("name", chiral_centre);
	    std::string n1, n2, n3; // these need setting, c.f. get_chiral_tag() in rdkit-interface.cc?

	    // The refmac monomer library and the rdkit (SMILES-based)
	    // representation of chirality is quite different.

	    // in SMILES the chiral centre has 4 substituents A[B@C](D)E
	    // Looking down the AB bond, C, D, and E are ordered clockwise (@).
	    //
	    // So we need to find the 4 neighbours of B: B should have
	    // an index below A, (similar reason for the others).
	    //
	    std::vector<std::pair<int, string> > neighbours;

	    unsigned int n_bonds = mol.getNumBonds();
	    for (unsigned int ib=0; ib<n_bonds; ib++) {
	       const RDKit::Bond *bond_p = mol.getBondWithIdx(ib);
	       unsigned int idx_1 = bond_p->getBeginAtomIdx();
	       unsigned int idx_2 = bond_p->getEndAtomIdx();

	       if (idx_1 == iat)
		  neighbours.push_back(std::pair<int, string> (idx_2, ""));
	       if (idx_2 == iat)
		  neighbours.push_back(std::pair<int, string> (idx_1, ""));
	    }

	    // std::cout << "centre idx " << iat << " neighbours size: " << neighbours.size() << std::endl;

	    std::sort(neighbours.begin(), neighbours.end()); // how does this work? :-)

	    // it sorts of the first index first, and if that is a
	    // match, sorts on the second, I think, neighbours is now
	    // sorted so that the low indices are first.

	    if (neighbours.size() == 4) {

	       for (unsigned int in=0; in<neighbours.size(); in++) {
		  std::string name;
		  mol[neighbours[in].first]->getProp("name", name); // already inside a try
		  // std::cout << "got name " << name << " for atom index "
		  // << neighbours[in].first << std::endl;
		  neighbours[in].second = name;
	       }

	       std::string chiral_id = "chiral_" + util::int_to_string(n_chirals+1);
	       // Neighbour[1] is the hydrogen, we presume. 
	       if (!chiral_centre.empty() &&
		   !neighbours[0].second.empty() &&
		   !neighbours[2].second.empty() &&
		   !neighbours[3].second.empty()) {
		  if (1) {
		     coot::dict_chiral_restraint_t chiral(chiral_id,
							  chiral_centre,
							  neighbours[0].second,
							  neighbours[2].second,
							  neighbours[3].second, vol_sign);
		     restraints->chiral_restraint.push_back(chiral);
		     n_chirals++;
		  }
	       }

	    } else {
	       std::cout << "oops - found " << neighbours.size() << " neighbours" << std::endl;
	    } 
	 }
	 catch (const KeyErrorException &kee) {
	    std::cout << "caught no-name for atom exception in chiral assignment(): "
		      <<  kee.what() << std::endl;
	 }
      } 
   }
   return n_chirals;
}




void
coot::write_restraints(PyObject *restraints_py,
		       const std::string &monomer_type,
		       const std::string &file_name) {

   coot::dictionary_residue_restraints_t rest = monomer_restraints_from_python(restraints_py);
   rest.write_cif(file_name);
}


void
coot::write_pdb_from_mol(PyObject *rdkit_mol_py,
			 const std::string &res_name,
			 const std::string &file_name) {

   RDKit::ROMol &mol = boost::python::extract<RDKit::ROMol&>(rdkit_mol_py);
   mmdb::Residue *res = coot::make_residue(mol, 0, res_name);
   if (! res) {
      std::cout << "in write_pdb_from_mol() failed to make residue" << std::endl;
   } else {
      mmdb::Manager *mol = coot::util::create_mmdbmanager_from_residue(res);
      int status = mol->WritePDBASCII(file_name.c_str());
      if (status == 0)
	 std::cout << "INFO:: wrote PDB   \"" << file_name << "\"" << std::endl;
      delete mol;
   }
}



void
coot::regularize_and_write_pdb(PyObject *rdkit_mol, PyObject *restraints_py,
			       const std::string &res_name,
			       const std::string &pdb_file_name) {

   std::pair<mmdb::Manager *, mmdb::Residue *> mol_res = regularize_inner(rdkit_mol, restraints_py, res_name);
   int status = mol_res.first->WritePDBASCII(pdb_file_name.c_str());
   if (status == 0)
      std::cout << "INFO:: wrote PDB   \"" << pdb_file_name << "\"" << std::endl;
   
}


// update the passed rdkit molecule
void
coot::regularize(PyObject *rdkit_mol_py, PyObject *restraints_py,
			   const std::string &res_name) {

   
   RDKit::ROMol &mol = boost::python::extract<RDKit::ROMol&>(rdkit_mol_py);
   
   std::pair<mmdb::Manager *, mmdb::Residue *> regular =
      regularize_inner(rdkit_mol_py, restraints_py, res_name);

   if (regular.second) { 

      // now create a new molecule, because the one we are given is a ROMol.
      RDKit::RWMol *rw_mol = new RDKit::RWMol(mol);
      int iconf = 0; 
      update_coords(rw_mol, iconf, regular.second);
   }
} 

std::pair<mmdb::Manager *, mmdb::Residue *>
coot::regularize_inner(PyObject *rdkit_mol_py,
		       PyObject *restraints_py,
		       const std::string &res_name) {

   RDKit::ROMol &mol = boost::python::extract<RDKit::ROMol&>(rdkit_mol_py);
   return regularize_inner(mol, restraints_py, res_name);
}


std::pair<mmdb::Manager *, mmdb::Residue *>
coot::regularize_inner(RDKit::ROMol &mol,
		       PyObject *restraints_py,
		       const std::string &res_name) {
   
   coot::dictionary_residue_restraints_t dict_restraints = 
      monomer_restraints_from_python(restraints_py);
   mmdb::Residue *residue_p = coot::make_residue(mol, 0, res_name);
   // remove this NULL at some stage (soon)
   mmdb::Manager *cmmdbmanager = coot::util::create_mmdbmanager_from_residue(residue_p);
   simple_refine(residue_p, cmmdbmanager, dict_restraints);
   return std::pair<mmdb::Manager *, mmdb::Residue *> (cmmdbmanager, residue_p);
} 

