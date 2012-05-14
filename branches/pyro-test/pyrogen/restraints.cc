

#include <boost/python.hpp>
#include "restraints.hh"
#include <rdkit-interface.hh>
#include <coot-coord-utils.hh>



void
coot::mogul_out_to_mmcif_dict(const std::string &mogul_file_name,
			      const std::string &comp_id,
			      const std::string &compound_name,
			      const std::vector<std::string> &atom_names,
			      int n_atoms_all,
			      int n_atoms_non_hydrogen,
			      PyObject *bond_order_restraints_py,
			      const std::string &cif_file_name) {

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

   
void
coot::mogul_out_to_mmcif_dict_by_mol(const std::string &mogul_file_name,
				     const std::string &comp_id,
				     const std::string &compound_name,
				     PyObject *rdkit_mol_py,
				     PyObject *bond_order_restraints_py,
				     const std::string &mmcif_out_file_name) {

   RDKit::ROMol &mol = boost::python::extract<RDKit::ROMol&>(rdkit_mol_py);
   coot::dictionary_residue_restraints_t bond_order_restraints = 
      monomer_restraints_from_python(bond_order_restraints_py);

   mogul mogul(mogul_file_name);
   std::vector<std::string> atom_names;
   int n_atoms_all = mol.getNumAtoms();
   int n_atoms_non_hydrogen = 0;

   for (unsigned int iat=0; iat<n_atoms_all; iat++) { 
      RDKit::ATOM_SPTR at_p = mol[iat];
      if (at_p->getAtomicNum() != 1)
	 n_atoms_non_hydrogen++;
      try {
	 std::string name = "";
	 at_p->getProp("name", name);
	 atom_names.push_back(name);
      }
      catch (KeyErrorException kee) {
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


   dictionary_residue_restraints_t restraints = mmcif_dict_from_mol_inner(comp_id, compound_name, rdkit_mol_py);
   restraints.conservatively_replace_with(mogul_restraints);
   restraints.write_cif(mmcif_out_file_name);
      
}

void 
coot::mmcif_dict_from_mol(const std::string &comp_id,
			  const std::string &compound_name,
			  PyObject *rdkit_mol_py,
			  const std::string &mmcif_out_file_name) {

   coot::dictionary_residue_restraints_t restraints =
      mmcif_dict_from_mol_inner(comp_id, compound_name, rdkit_mol_py);
   restraints.write_cif(mmcif_out_file_name);

} 

coot::dictionary_residue_restraints_t
coot::mmcif_dict_from_mol_inner(const std::string &comp_id,
				const std::string &compound_name,
				PyObject *rdkit_mol_py) { 

   coot::dictionary_residue_restraints_t restraints (comp_id, 1);
   
   RDKit::ROMol &mol = boost::python::extract<RDKit::ROMol&>(rdkit_mol_py);

   const char *env = getenv("ENERGY_LIB_CIF");
   if (! env) {
      std::cout << "ERROR:: no ENERGY_LIB_CIF env var" << std::endl;
   } else {

      // number of atom first
      // 
      int n_atoms_all = mol.getNumAtoms();
      int n_atoms_non_hydrogen = 0;
      for (unsigned int iat=0; iat<n_atoms_all; iat++)
	 if (mol[iat]->getAtomicNum() != 1)
	    n_atoms_non_hydrogen++;
      
      coot::energy_lib_t energy_lib(env);

      // fill with ener_lib values and then add mogul updates.
      // 
      restraints.residue_info.comp_id = comp_id;
      restraints.residue_info.three_letter_code = comp_id;
      restraints.residue_info.name = compound_name;
      restraints.residue_info.number_atoms_all = n_atoms_all;
      restraints.residue_info.number_atoms_nh = n_atoms_non_hydrogen;
      restraints.residue_info.group = "non-polymer";
      restraints.residue_info.description_level = "Partial";

      
      coot::add_chem_comp_atoms(mol, &restraints); // alter restraints
      coot::fill_with_energy_lib_bonds(mol, energy_lib, &restraints); // alter restraints
      coot::fill_with_energy_lib_angles(mol, energy_lib, &restraints); // alter restraints
      coot::fill_with_energy_lib_torsions(mol, energy_lib, &restraints); // alter restraints
      // and angles and torsions

      int n_chirals = coot::assign_chirals(mol, &restraints); // alter restraints
      if (n_chirals) 
	 restraints.assign_chiral_volume_targets();

      coot::add_chem_comp_planes(mol, &restraints);
   }
   return restraints;
}

void
coot::fill_with_energy_lib_bonds(const RDKit::ROMol &mol,
				 const coot::energy_lib_t &energy_lib,
				 coot::dictionary_residue_restraints_t *restraints) {
   
   int n_bonds = mol.getNumBonds();
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
	    catch (std::runtime_error rte) {
	       std::cout << "WARNING:: error in adding bond restraint for bond number "
			 << ib << " " << rte.what() << std::endl;
	    } 
	 
	 }
	 catch (KeyErrorException kee) {
	    std::cout << "WARNING:: caugh KeyErrorException in add_bonds_to_hydrogens() "
		      << std::endl;
	 }
      }
   }
}

std::string
coot::convert_to_energy_lib_bond_type(RDKit::Bond::BondType bt) {

   std::string r = "unset";

   if (bt == RDKit::Bond::UNSPECIFIED) r = "unset";
   if (bt == RDKit::Bond::SINGLE) r = "single";
   if (bt == RDKit::Bond::DOUBLE) r = "double";
   if (bt == RDKit::Bond::TRIPLE) r = "triple";
   if (bt == RDKit::Bond::QUADRUPLE) r = "quadruple";
   if (bt == RDKit::Bond::QUINTUPLE) r = "quintuple";
   if (bt == RDKit::Bond::HEXTUPLE) r = "hextuple";
   if (bt == RDKit::Bond::ONEANDAHALF) r = "deloc";
   if (bt == RDKit::Bond::AROMATIC) r = "aromatic";
   
//       TWOANDAHALF,
//       THREEANDAHALF,
//       FOURANDAHALF,
//       FIVEANDAHALF,
//       AROMATIC,
//       IONIC,
//       HYDROGEN,

   return r;
} 


void
coot::fill_with_energy_lib_angles(const RDKit::ROMol &mol,
				  const coot::energy_lib_t &energy_lib,
				  coot::dictionary_residue_restraints_t *restraints) {
   
   int n_atoms = mol.getNumAtoms();
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
		  catch (std::runtime_error rte) {
		     std::cout << "WARNING:: error in adding angle restraint for atoms "
			       << at_1->getIdx() << " "
			       << at_2->getIdx() << " "
			       << at_3->getIdx() << " "
			       << rte.what() << std::endl;
		  } 
	       }
	       catch (KeyErrorException kee) {
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
   
   int n_atoms = mol.getNumAtoms();
   int tors_no = 1; // incremented on addition
   int const_no = 1; // incremented on addition.  When const_no is incremented, tors_no is not.
   std::map<std::string, bool> done_torsion;
   
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

			if (done_torsion.find(torsion_key_name_1) == done_torsion.end() &&
			    done_torsion.find(torsion_key_name_2) == done_torsion.end()) {

			   if (0) // debug
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
		     
			   energy_lib_torsion tors =
			      energy_lib.get_torsion(atom_type_2, atom_type_3);

			   bool is_const = is_const_torsion(mol, at_2.get(), at_3.get());

			   if (0)
			      std::cout << "       got torsion " << tors << "  is_const: "
					<< is_const << std::endl;

			   if (tors.period != 0) { 
			      double esd = 20.0;
			      std::string tors_id;
			      if (! is_const) { 
				 tors_id = "var_";
				 char s[100];
				 snprintf(s,99,"%d", tors_no);
				 tors_id += std::string(s);
				 tors_no++;
			      } else {
				 tors_id = "CONST_";
				 char s[100];
				 snprintf(s,99,"%d", const_no);
				 tors_id += std::string(s);
				 const_no++;
			      } 
			      dict_torsion_restraint_t torsionr(tors_id,
								atom_name_1, atom_name_2,
								atom_name_3, atom_name_4,
								tors.angle, esd, tors.period);
			      restraints->torsion_restraint.push_back(torsionr);
			      done_torsion[torsion_key_name_1] = true;
			      done_torsion[torsion_key_name_2] = true;
			   
			   }
			}
		     }
		     catch (KeyErrorException kee) {
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

   bool status = false;
   
   int n_bonds = mol.getNumBonds();
   for (unsigned int ib=0; ib<n_bonds; ib++) {
      const RDKit::Bond *bond_p = mol.getBondWithIdx(ib);
      RDKit::Atom *bond_at_1 = bond_p->getBeginAtom();
      RDKit::Atom *bond_at_2 = bond_p->getEndAtom();

      bool found_torsion_bond = false;
      if (torsion_at_2 == bond_at_1)
	 if (torsion_at_3 == bond_at_2)
	    found_torsion_bond = true;
      if (torsion_at_2 == bond_at_2)
	 if (torsion_at_3 == bond_at_2)
	    found_torsion_bond = true;

      if (found_torsion_bond) { 
	 if (bond_p->getBondType() == RDKit::Bond::AROMATIC) status = true;
	 if (bond_p->getBondType() == RDKit::Bond::DOUBLE)   status = true;
	 if (bond_p->getBondType() == RDKit::Bond::TRIPLE)   status = true;
      }
   }

   return status;

} 


void
coot::add_chem_comp_atoms(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   int iconf = 0;
   int n_atoms = mol.getNumAtoms();
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

	 dict_atom atom(name, name, at_p->getSymbol(), atom_type, charge_pair);

 	 RDKit::Conformer conf = mol.getConformer(iconf);
 	 RDGeom::Point3D &r_pos = conf.getAtomPos(iat);
 	 clipper::Coord_orth pos(r_pos.x, r_pos.y, r_pos.z);
	 atom.model_Cartn = std::pair<bool, clipper::Coord_orth> (true, pos);
	 
	 restraints->atom_info.push_back(atom);
      }
      catch (KeyErrorException kee) {
	 std::cout << "WARNING:: caught property exception in add_chem_comp_atoms()"
		   << iat << std::endl;
      }
   }
}

// what fun!
// C++ smarts
void
coot::add_chem_comp_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   add_chem_comp_aromatic_planes(mol, restraints);
   add_chem_comp_deloc_planes(mol, restraints);

}

// what fun!
// C++ smarts
void
coot::add_chem_comp_aromatic_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   std::vector<std::string> patterns;
   patterns.push_back("a12aaaaa1aaaa2");
   patterns.push_back("a12aaaaa1aaa2");
   patterns.push_back("a1aaaaa1");
   patterns.push_back("a1aaaa1");
   patterns.push_back("[*;^2]1[*;^2][*;^2][A;^2][*;^2]1"); // non-aromatic 5-ring

   int n_planes = 1; 
   for (unsigned int ipat=0; ipat<patterns.size(); ipat++) {
      RDKit::ROMol *query = RDKit::SmartsToMol(patterns[ipat]);
      std::vector<RDKit::MatchVectType>  matches;
      bool recursionPossible = true;
      bool useChirality = true;
      bool uniquify = true;
      int matched = RDKit::SubstructMatch(mol,*query,matches,uniquify,recursionPossible, useChirality);
      for (unsigned int imatch=0; imatch<matches.size(); imatch++) { 
	 if (matches[imatch].size() > 0) {
	    std::cout << "matched plane pattern: " << patterns[ipat] << std::endl;
	    std::string plane_id = "plane-arom-";
	    char s[100];
	    snprintf(s,99,"%d", n_planes);
	    plane_id += std::string(s);
	    std::vector<std::string> plane_restraint_atoms; 
	    try {
	       for (unsigned int ii=0; ii<matches[imatch].size(); ii++) {
		  RDKit::ATOM_SPTR at_p = mol[matches[imatch][ii].second];

		  // only add this atom to a plane restraint if it not
		  // already in a plane restraint.  Test by failing to
		  // get the plane_id property.

		  bool add_atom_to_plane = true;

		  // add the atom to the plane if the plane that it is
		  // already is not this plane.

		  try {
		     std::string atom_plane;
		     at_p->getProp("plane_id", atom_plane);
		     if (atom_plane == plane_id)
			add_atom_to_plane = false;
		  }
		  catch (KeyErrorException kee) {
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
			std::cout << "debug:: set atom " << name << " to plane_id " << plane_id_lookup_debug << std::endl;
		     }

		     // run through neighours, because neighours of
		     // aromatic system atoms are in the plane too.
		     // 
		     RDKit::ROMol::ADJ_ITER nbr_idx_1, end_nbrs_1;
		     boost::tie(nbr_idx_1, end_nbrs_1) = mol.getAtomNeighbors(at_p);
		     std::vector<RDKit::ATOM_SPTR> attached_atoms;
		     while(nbr_idx_1 != end_nbrs_1) {
			const RDKit::ATOM_SPTR at_2 = mol[*nbr_idx_1];
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
			      if (std::find(plane_restraint_atoms.begin(), plane_restraint_atoms.end(), attached_atom_name) ==
				  plane_restraint_atoms.end())
				 plane_restraint_atoms.push_back(attached_atom_name);
			   }
			   catch (KeyErrorException kee) {
			      // do nothing then (no name found)
			   }
			}
		     }
		  }
	       }
	       // make a plane restraint with those atoms in then
	       if (plane_restraint_atoms.size() > 3) {
		  realtype dist_esd = 0.02;
		  for (unsigned int iat=0; iat<plane_restraint_atoms.size(); iat++) { 
		     coot::dict_plane_restraint_t rest(plane_id, plane_restraint_atoms[iat], dist_esd);
		     restraints->plane_restraint.push_back(rest);
		  }
	       } 
	       // maybe this should be in above clause for aesthetic reasons?
	       n_planes++;
	    }
	    catch (KeyErrorException kee) {
	       // this should not happen
	       std::cout << "WARNING:: add_chem_comp_planes() failed to get atom name "
			 << std::endl;
	    } 
	 }
      }
   }
}

void
coot::add_chem_comp_deloc_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {

   std::vector<std::string> patterns;
   patterns.push_back("*C(=O)[O;H]");     // ASP
   patterns.push_back("AC(=O)[N^2;H2,H1]([H])[A,H]");  // ASN
   patterns.push_back("*C(=N)[N^2;H2]([H])[A,H]");  // amidine
   patterns.push_back("CNC(=[NH])N([H])[H]");  // guanidinium with H - testing
   patterns.push_back("CNC(=[NH])N");  // guanidinium sans Hs
   
   int n_planes = 1; 
   for (unsigned int ipat=0; ipat<patterns.size(); ipat++) {
      RDKit::ROMol *query = RDKit::SmartsToMol(patterns[ipat]);
      std::vector<RDKit::MatchVectType>  matches;
      bool recursionPossible = true;
      bool useChirality = true;
      bool uniquify = true;
      int matched = RDKit::SubstructMatch(mol,*query,matches,uniquify,recursionPossible, useChirality);
      for (unsigned int imatch=0; imatch<matches.size(); imatch++) { 
	 if (matches[imatch].size() > 0) {
	    std::cout << "matched deloc pattern: " << patterns[ipat] << std::endl;
	    std::vector<std::string> atom_names;
	    std::string plane_id = "plane-deloc-";
	    char s[100];
	    snprintf(s,99,"%d", n_planes);
	    plane_id += std::string(s);
	    try {
	       for (unsigned int ii=0; ii<matches[imatch].size(); ii++) {
		  RDKit::ATOM_SPTR at_p = mol[matches[imatch][ii].second];

		  // Unlike aromatics, the atoms of this type of plane
		  // can be in more than one plane.

		  std::string name = "";
		  at_p->getProp("name", name);
		  atom_names.push_back(name);
		  realtype dist_esd = 0.02;
		  coot::dict_plane_restraint_t res(plane_id, name, dist_esd);
		  restraints->plane_restraint.push_back(res);
		  at_p->setProp("plane_id", plane_id);
	       }
	    }

	    catch (KeyErrorException kee) {
	       std::cout << "ERROR:: in add_chem_comp_planes_deloc failed to get atom name"
			 << std::endl;
	    } 
	    n_planes++;
	 }
      }
   }
} 


   
int 
coot::assign_chirals(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints) {
   
   int vol_sign = coot::dict_chiral_restraint_t::CHIRAL_VOLUME_RESTRAINT_VOLUME_SIGN_UNASSIGNED;

   int n_chirals = 0;

   int n_atoms = mol.getNumAtoms();
   for (unsigned int iat=0; iat<n_atoms; iat++) { 
      RDKit::ATOM_SPTR at_p = mol[iat];
      RDKit::Atom::ChiralType chiral_tag = at_p->getChiralTag();
      // std::cout << "atom " << iat << " chiral tag: " << chiral_tag << std::endl;

      // do I need to check the atom order here, like I do in rdkit-interface.cc?
      if (chiral_tag == RDKit::Atom::CHI_TETRAHEDRAL_CCW)
	 vol_sign = 1;
      if (chiral_tag == RDKit::Atom::CHI_TETRAHEDRAL_CW)
	 vol_sign = -1;

      if (chiral_tag != RDKit::Atom::CHI_UNSPECIFIED) { 
	 try { 
	    std::string chiral_centre;
	    at_p->getProp("name", chiral_centre);
	    std::string n1, n2, n3; // these need setting, c.f. rdkit-interface.cc?
	    std::string chiral_id = "chiral_" + std::string("1");
	    coot::dict_chiral_restraint_t chiral(chiral_id,
						 chiral_centre,
						 n1, n2, n3, vol_sign);
	    restraints->chiral_restraint.push_back(chiral);
	    n_chirals++;
	 }
	 catch (KeyErrorException kee) {
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
   CResidue *res = coot::make_residue(mol, 0, res_name);
   if (! res) {
      std::cout << "in write_pdb_from_mol() failed to make residue" << std::endl;
   } else {
      CMMDBManager *mol = coot::util::create_mmdbmanager_from_residue(NULL, res);
      mol->WritePDBASCII(file_name.c_str());
      delete mol;
   }
}




coot::dictionary_residue_restraints_t
monomer_restraints_from_python(PyObject *restraints) {

   PyObject *retval = Py_False;

   if (!PyDict_Check(restraints)) {
      std::cout << " Failed to read restraints - not a dictionary" << std::endl;

   } else {

      std::vector<coot::dict_bond_restraint_t> bond_restraints;
      std::vector<coot::dict_angle_restraint_t> angle_restraints;
      std::vector<coot::dict_torsion_restraint_t> torsion_restraints;
      std::vector<coot::dict_chiral_restraint_t> chiral_restraints;
      std::vector<coot::dict_plane_restraint_t> plane_restraints;
      std::vector<coot::dict_atom> atoms;
      coot::dict_chem_comp_t residue_info;

      PyObject *key;
      PyObject *value;
      Py_ssize_t pos = 0;

      while (PyDict_Next(restraints, &pos, &key, &value)) {
	 // std::cout << ":::::::key: " << PyString_AsString(key) << std::endl;

	 std::string key_string = PyString_AsString(key);
	 if (key_string == "_chem_comp") {
	    PyObject *chem_comp_list = value;
	    if (PyList_Check(chem_comp_list)) {
	       if (PyObject_Length(chem_comp_list) == 7) {
		  std::string comp_id  = PyString_AsString(PyList_GetItem(chem_comp_list, 0));
		  std::string tlc      = PyString_AsString(PyList_GetItem(chem_comp_list, 1));
		  std::string name     = PyString_AsString(PyList_GetItem(chem_comp_list, 2));
		  std::string group    = PyString_AsString(PyList_GetItem(chem_comp_list, 3));
		  int n_atoms_all      = PyInt_AsLong(PyList_GetItem(chem_comp_list, 4));
		  int n_atoms_nh       = PyInt_AsLong(PyList_GetItem(chem_comp_list, 5));
		  std::string desc_lev = PyString_AsString(PyList_GetItem(chem_comp_list, 6));

		  coot::dict_chem_comp_t n(comp_id, tlc, name, group,
					   n_atoms_all, n_atoms_nh, desc_lev);
		  residue_info = n;
	       }
	    }
	 }


	 if (key_string == "_chem_comp_atom") {
	    PyObject *chem_comp_atom_list = value;
	    if (PyList_Check(chem_comp_atom_list)) {
	       int n_atoms = PyObject_Length(chem_comp_atom_list);
	       for (int iat=0; iat<n_atoms; iat++) {
		  PyObject *chem_comp_atom = PyList_GetItem(chem_comp_atom_list, iat);
		  if (PyObject_Length(chem_comp_atom) == 5) {
		     std::string atom_id  = PyString_AsString(PyList_GetItem(chem_comp_atom, 0));
		     std::string element  = PyString_AsString(PyList_GetItem(chem_comp_atom, 1));
		     std::string energy_t = PyString_AsString(PyList_GetItem(chem_comp_atom, 2));
		     float part_chr        = PyFloat_AsDouble(PyList_GetItem(chem_comp_atom, 3));
		     bool flag = 0;
		     if (PyLong_AsLong(PyList_GetItem(chem_comp_atom, 4))) {
			flag = 1;
		     }
		     std::pair<bool, float> part_charge_info(flag, part_chr);
		     coot::dict_atom at(atom_id, atom_id, element, energy_t, part_charge_info);
		     atoms.push_back(at);
		  }
	       }
	    }
	 }

	 if (key_string == "_chem_comp_bond") {
	    PyObject *bond_restraint_list = value;
	    if (PyList_Check(bond_restraint_list)) {
	       int n_bonds = PyObject_Length(bond_restraint_list);
	       for (int i_bond=0; i_bond<n_bonds; i_bond++) {
		  PyObject *bond_restraint = PyList_GetItem(bond_restraint_list, i_bond);
		  if (PyObject_Length(bond_restraint) == 5) {
		     PyObject *atom_1_py = PyList_GetItem(bond_restraint, 0);
		     PyObject *atom_2_py = PyList_GetItem(bond_restraint, 1);
		     PyObject *type_py   = PyList_GetItem(bond_restraint, 2);
		     PyObject *dist_py   = PyList_GetItem(bond_restraint, 3);
		     PyObject *esd_py    = PyList_GetItem(bond_restraint, 4);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(type_py) &&
			 PyFloat_Check(dist_py) && 
			 PyFloat_Check(esd_py)) {
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string type   = PyString_AsString(type_py);
			float  dist = PyFloat_AsDouble(dist_py);
			float  esd  = PyFloat_AsDouble(esd_py);
			coot::dict_bond_restraint_t rest(atom_1, atom_2, type, dist, esd);
			bond_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }


	 if (key_string == "_chem_comp_angle") {
	    PyObject *angle_restraint_list = value;
	    if (PyList_Check(angle_restraint_list)) {
	       int n_angles = PyObject_Length(angle_restraint_list);
	       for (int i_angle=0; i_angle<n_angles; i_angle++) {
		  PyObject *angle_restraint = PyList_GetItem(angle_restraint_list, i_angle);
		  if (PyObject_Length(angle_restraint) == 5) { 
		     PyObject *atom_1_py = PyList_GetItem(angle_restraint, 0);
		     PyObject *atom_2_py = PyList_GetItem(angle_restraint, 1);
		     PyObject *atom_3_py = PyList_GetItem(angle_restraint, 2);
		     PyObject *angle_py  = PyList_GetItem(angle_restraint, 3);
		     PyObject *esd_py    = PyList_GetItem(angle_restraint, 4);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyFloat_Check(angle_py) && 
			 PyFloat_Check(esd_py)) {
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string atom_3 = PyString_AsString(atom_3_py);
			float  angle = PyFloat_AsDouble(angle_py);
			float  esd   = PyFloat_AsDouble(esd_py);
			coot::dict_angle_restraint_t rest(atom_1, atom_2, atom_3, angle, esd);
			angle_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }

	 if (key_string == "_chem_comp_tor") {
	    PyObject *torsion_restraint_list = value;
	    if (PyList_Check(torsion_restraint_list)) {
	       int n_torsions = PyObject_Length(torsion_restraint_list);
	       for (int i_torsion=0; i_torsion<n_torsions; i_torsion++) {
		  PyObject *torsion_restraint = PyList_GetItem(torsion_restraint_list, i_torsion);
		  if (PyObject_Length(torsion_restraint) == 7) { // info for Nigel.
		     std::cout << "torsions now have 8 elements starting with the torsion id\n"; 
		  } 
		  if (PyObject_Length(torsion_restraint) == 8) { 
		     PyObject *id_py     = PyList_GetItem(torsion_restraint, 0);
		     PyObject *atom_1_py = PyList_GetItem(torsion_restraint, 1);
		     PyObject *atom_2_py = PyList_GetItem(torsion_restraint, 2);
		     PyObject *atom_3_py = PyList_GetItem(torsion_restraint, 3);
		     PyObject *atom_4_py = PyList_GetItem(torsion_restraint, 4);
		     PyObject *torsion_py= PyList_GetItem(torsion_restraint, 5);
		     PyObject *esd_py    = PyList_GetItem(torsion_restraint, 6);
		     PyObject *period_py = PyList_GetItem(torsion_restraint, 7);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyString_Check(atom_4_py) &&
			 PyFloat_Check(torsion_py) && 
			 PyFloat_Check(esd_py)    && 
			 PyInt_Check(period_py)) { 
			std::string id     = PyString_AsString(id_py);
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string atom_3 = PyString_AsString(atom_3_py);
			std::string atom_4 = PyString_AsString(atom_4_py);
			float  torsion = PyFloat_AsDouble(torsion_py);
			float  esd     = PyFloat_AsDouble(esd_py);
			int  period    = PyInt_AsLong(period_py);
			coot::dict_torsion_restraint_t rest(id, atom_1, atom_2, atom_3, atom_4,
							    torsion, esd, period);
			torsion_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }

	 if (key_string == "_chem_comp_chir") {
	    PyObject *chiral_restraint_list = value;
	    if (PyList_Check(chiral_restraint_list)) {
	       int n_chirals = PyObject_Length(chiral_restraint_list);
	       for (int i_chiral=0; i_chiral<n_chirals; i_chiral++) {
		  PyObject *chiral_restraint = PyList_GetItem(chiral_restraint_list, i_chiral);
		  if (PyObject_Length(chiral_restraint) == 7) { 
		     PyObject *chiral_id_py= PyList_GetItem(chiral_restraint, 0);
		     PyObject *atom_c_py   = PyList_GetItem(chiral_restraint, 1);
		     PyObject *atom_1_py   = PyList_GetItem(chiral_restraint, 2);
		     PyObject *atom_2_py   = PyList_GetItem(chiral_restraint, 3);
		     PyObject *atom_3_py   = PyList_GetItem(chiral_restraint, 4);
		     PyObject *vol_sign_py = PyList_GetItem(chiral_restraint, 5);
		     PyObject *esd_py      = PyList_GetItem(chiral_restraint, 6);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyString_Check(atom_c_py) &&
			 PyString_Check(chiral_id_py) && 
			 PyFloat_Check(esd_py)    && 
			 PyInt_Check(vol_sign_py)) {
			std::string chiral_id = PyString_AsString(chiral_id_py);
			std::string atom_c    = PyString_AsString(atom_c_py);
			std::string atom_1    = PyString_AsString(atom_1_py);
			std::string atom_2    = PyString_AsString(atom_2_py);
			std::string atom_3    = PyString_AsString(atom_3_py);
			float  esd            = PyFloat_AsDouble(esd_py);
			int  vol_sign         = PyInt_AsLong(vol_sign_py);
			coot::dict_chiral_restraint_t rest(chiral_id,
							   atom_c, atom_1, atom_2, atom_3, 
							   vol_sign);
			chiral_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }


	 if (key_string == "_chem_comp_plane_atom") {
	    PyObject *plane_restraint_list = value;
	    if (PyList_Check(plane_restraint_list)) {
	       int n_planes = PyObject_Length(plane_restraint_list);
	       for (int i_plane=0; i_plane<n_planes; i_plane++) {
		  PyObject *plane_restraint = PyList_GetItem(plane_restraint_list, i_plane);
		  if (PyObject_Length(plane_restraint) == 3) {
		     std::vector<std::string> atoms;
		     PyObject *plane_id_py = PyList_GetItem(plane_restraint, 0);
		     PyObject *esd_py      = PyList_GetItem(plane_restraint, 2);
		     PyObject *py_atoms_py = PyList_GetItem(plane_restraint, 1);

		     bool atoms_pass = 1;
		     if (PyList_Check(py_atoms_py)) {
			int n_atoms = PyObject_Length(py_atoms_py);
			for (int iat=0; iat<n_atoms; iat++) {
			   PyObject *at_py = PyList_GetItem(py_atoms_py, iat);
			   if (PyString_Check(at_py)) {
			      atoms.push_back(PyString_AsString(at_py));
			   } else {
			      atoms_pass = 0;
			   }
			}
			if (atoms_pass) {
			   if (PyString_Check(plane_id_py)) {
			      if (PyFloat_Check(esd_py)) {
				 std::string plane_id = PyString_AsString(plane_id_py);
				 float esd = PyFloat_AsDouble(esd_py);
				 if (atoms.size() > 0) { 
				    coot::dict_plane_restraint_t rest(plane_id, atoms[0], esd);
				    for (int i=1; i<atoms.size(); i++)
				       rest.push_back_atom(atoms[i]);
				    plane_restraints.push_back(rest);
				 }
			      }
			   }
			}
		     }
		  }
	       }
	    }
	 }
      }
	
      coot::dictionary_residue_restraints_t monomer_restraints;
      monomer_restraints.bond_restraint    = bond_restraints;
      monomer_restraints.angle_restraint   = angle_restraints;
      monomer_restraints.torsion_restraint = torsion_restraints;
      monomer_restraints.chiral_restraint  = chiral_restraints;
      monomer_restraints.plane_restraint   = plane_restraints;
      monomer_restraints.residue_info      = residue_info;
      monomer_restraints.atom_info         = atoms; 
      return monomer_restraints;
   }
} 
