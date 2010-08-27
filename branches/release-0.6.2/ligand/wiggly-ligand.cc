/* ligand/wiggly-ligand.cc 
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006 by The University of York
 * Copyright 2009 by The University of Oxford.
 * Author: Paul Emsley
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#include <stdexcept>
#include "coot-utils.hh"  // needed, it seems for the case where GSL
			  // is not defined.

#include "coot-coord-extras.hh"

#include "wligand.hh"
#include "regularize-minimol.hh"

// Ligand installation from coordinates (an minimol), number of trials.
//
// In the constructor we check all atoms of the coordinates are
// in the restraints of that residue type.
//
// We need to modify restraints_container_t to read the atom list too
// (see notes).
// 
//  done  (it was half done anyway).
//

std::vector<coot::installed_wiggly_ligand_info_t>
coot::wligand::install_simple_wiggly_ligands(coot::protein_geometry *pg,
					     const coot::minimol::molecule &ligand_in,
					     int n_samples,
					     bool optimize_geometry_flag,
					     bool fill_returned_molecules_vector_flag) {

   std::string alt_conf = ""; // should this be passed?
   
   std::vector<coot::installed_wiggly_ligand_info_t> returned_tors_molecules_info;
   short int istat = 0;
   std::string m = ""; 
   
   // m_torsions: a set of torsions for this monomer
   // 
   std::string monomer_type = get_monomer_type_from_mol(ligand_in);
//    std::cout << "DEBUG:: in install_simple_wiggly_ligands: "
// 	     << "get_monomer_type_from_mol returns :"
// 	     << monomer_type << ":" << std::endl;
   short int do_hydrogen_torsions_flag = 0;
   std::vector <coot::dict_torsion_restraint_t> m_torsions =
      pg->get_monomer_torsions_from_geometry(monomer_type, do_hydrogen_torsions_flag);
   std::pair<short int, dictionary_residue_restraints_t> monomer_restraints = 
      pg->get_monomer_restraints(monomer_type);

   std::vector <coot::dict_torsion_restraint_t> non_const_torsions;
   int n_torsionable = 0;
   for (unsigned int i_tor=0; i_tor<m_torsions.size(); i_tor++) {
      if (! m_torsions[i_tor].is_const()) { 
	 non_const_torsions.push_back(m_torsions[i_tor]);
	 n_torsionable++;
      } 
   } 
   std::cout << "This residues has " << m_torsions.size() << " defined non-H torsions "
	     << "of which " << n_torsionable << " are rotatable " << std::endl;
   if (debug_wiggly_ligands) {
      for (unsigned int itor=0; itor<m_torsions.size(); itor++) { 
	 std::cout << "   " << itor << " " << m_torsions[itor] << "\n";
      }
   }

   
   if (non_const_torsions.size() == 0) {

      // " Did you forget to read the dictionary?";
      
      std::pair<short int, dictionary_residue_restraints_t> p = 
	 pg->get_monomer_restraints(monomer_type);

      m = "Requested flexible molecule for ligand ";
      m += monomer_type;
      m += "\n"; 
      m += " but no non-Hydrogen rotatable bonds found.\n";
      if (p.first == 0) {
	 std::string mess = "WARNING:: " + m;
	 mess += " Did you forget to read the dictionary?\n";
	 throw std::runtime_error(mess);
      } else {
	 // OK, there were restraints for such a thing, but no
	 // torsions.  It was a phosphate or some such.
	 // In that case, just install a simple static molecule
	 install_ligand(ligand_in);
	 istat = 1;
      }
      if (fill_returned_molecules_vector_flag) {
	 coot::installed_wiggly_ligand_info_t l;
	 l.mol = ligand_in;
	 returned_tors_molecules_info.push_back(l); // get what you give.
	 return returned_tors_molecules_info;
      } 

   } else {
      istat = 1; // OK.... so far.
   }

   // This should be inside the else (then we can remove the above
   // return) , it's just a mess to do.
   // 
   for (int isample=0; isample<n_samples; isample++) {

      coot::minimol::molecule ligand = ligand_in; // local changable copy
      // the coot::atom_tree_t is constructed from a residue, not a molecule.
      coot::minimol::residue ligand_residue = ligand[0][ligand[0].min_res_no()];
      std::string ligand_chain_id = ligand[0].fragment_id;

      if (0) {
	 std::cout << "DEBUG:: ligand input has " << ligand.fragments.size() << " fragments "
		   << std::endl;
	 for (unsigned int ifrag=0; ifrag<ligand.fragments.size(); ifrag++) {
	    for (unsigned int ires=ligand.fragments[ifrag].min_res_no();
		 ires<=ligand.fragments[ifrag].max_residue_number();
		 ires++) {
	       std::cout << "DEBUG:: fragment: " << ifrag << " " << ires << " "
			 << ligand.fragments[ifrag][ires] << std::endl;
	    }
	 }
      }

      std::vector<float> torsion_set = get_torsions_by_random(non_const_torsions);
      std::vector<coot::atom_name_quad> atom_name_quads =
	 get_torsion_bonds_atom_quads(monomer_type, non_const_torsions);
      // the vector of rotation torsions. 
      std::vector<coot::atom_tree_t::tree_dihedral_info_t> v;
      if (torsion_set.size() == atom_name_quads.size()) { 
	 for (unsigned int it=0; it<torsion_set.size(); it++) {
	    coot::atom_tree_t::tree_dihedral_info_t di(atom_name_quads[it], torsion_set[it]);
	    v.push_back(di);
	 }
      }


      try { 
	 coot::atom_tree_t tree(monomer_restraints.second, ligand_residue, alt_conf);
	 // angles in degrees.
	 tree.set_dihedral_multi(v);
	 coot::minimol::residue wiggled_ligand_residue = tree.GetResidue();
	 returned_tors_molecules_info.push_back(optimize_and_install(wiggled_ligand_residue,
								     pg, non_const_torsions,
								     torsion_set, ligand_chain_id,
								     isample, optimize_geometry_flag,
								     fill_returned_molecules_vector_flag));
      }
      catch (std::runtime_error rte) {
	 try { 
	    CResidue *r = coot::GetResidue(ligand_residue);
	    bool add_reverse_contacts = 1;
	    std::vector<std::vector<int> > contact_indices =
	       coot::util::get_contact_indices_from_restraints(r, pg, 1, add_reverse_contacts);

	    if (0) 
	       for (unsigned int i=0; i<contact_indices.size(); i++) { 
		  std::cout << "contacts " << i << " has " << contact_indices[i].size() << " contacts: ";
		  for (unsigned int j=0; j<contact_indices[i].size(); j++) { 
		     std::cout << contact_indices[i][j] << " ";
		  }
		  std::cout << std::endl;
	       }
	    
	    int base_atom_index = 0; // hopefully this will work
	    coot::atom_tree_t tree(monomer_restraints.second,
				   contact_indices, base_atom_index, ligand_residue, alt_conf);
	    tree.set_dihedral_multi(v);
	    coot::minimol::residue wiggled_ligand_residue = tree.GetResidue();
	    returned_tors_molecules_info.push_back(optimize_and_install(wiggled_ligand_residue,
									pg, non_const_torsions,
									torsion_set, ligand_chain_id,
									isample, optimize_geometry_flag,
									fill_returned_molecules_vector_flag));
	    delete r;
	 }
	 catch (std::runtime_error rte) {
	    std::cout << "ERROR: in install_simple_wiggly_ligands() " << rte.what() << std::endl;
	 } 
      } 
   } // samples for loop
   
   return returned_tors_molecules_info;
}

coot::installed_wiggly_ligand_info_t
coot::wligand::optimize_and_install(const coot::minimol::residue &wiggled_ligand_residue,
				    coot::protein_geometry *pg,
				    const std::vector <dict_torsion_restraint_t> &non_const_torsions,
				    const std::vector<float> &torsion_set,
				    const std::string &ligand_chain_id,
				    int isample,
				    bool optimize_geometry_flag,
				    bool fill_returned_molecules_vector_flag) {

   coot::installed_wiggly_ligand_info_t l;  // returned object
   
   // now make a molecule of that (my son)
   coot::minimol::fragment wiggled_ligand_frag(ligand_chain_id);
   wiggled_ligand_frag.addresidue(wiggled_ligand_residue, 0);
   coot::minimol::molecule wiggled_ligand(wiggled_ligand_frag);

   if (debug_wiggly_ligands) {  // debugging wiggly ligands
      std::string filename = "wligand-";
      filename += int_to_string(isample);
      filename += ".pdb";
      wiggled_ligand.write_file(filename, default_b_factor);
   }
      
#ifdef HAVE_GSL
   if (optimize_geometry_flag) { 
      coot::minimol::molecule reg_ligand = coot::regularize_minimol_molecule(wiggled_ligand, *pg);
      install_ligand(reg_ligand);
      if (fill_returned_molecules_vector_flag) {
	 l.mol = reg_ligand;
	 l.add_torsions(non_const_torsions, torsion_set);
      } 
   } else {
      install_ligand(wiggled_ligand);
      if (fill_returned_molecules_vector_flag) { 
	 l.mol = wiggled_ligand;
	 l.add_torsions(non_const_torsions, torsion_set);
      } 
   }
#else
   install_ligand(wiggled_ligand);
   if (fill_returned_molecules_vector_flag) { 
      l.mol = ligand_in;
      return l;
   }
#endif
   return l;
}



   

std::string
coot::wligand::get_monomer_type_from_mol(const coot::minimol::molecule &mol) const {

   std::string r;
   short int ifound = 0;
   
   for(unsigned int ifrag=0; ifrag<mol.fragments.size(); ifrag++) {
      for (int ires=mol[ifrag].min_res_no(); ires<=mol[ifrag].max_residue_number(); ires++) {
	 if (mol[ifrag][ires].n_atoms() > 0) {
	    r = mol[ifrag][ires].name;
	    ifound = 1;
	    break;
	 }
      }
      if (ifound)
	 break;
   }

   std::cout << "get_monomer type: " << r << std::endl;
   return r;
}


std::vector<std::vector<int> >
coot::wligand::getcontacts(const coot::minimol::molecule &mol) const {

   std::vector<coot::minimol::atom *> atoms = mol.select_atoms_serial();
   std::vector<std::vector<int> > contacts;

   for (unsigned int i=0; i<atoms.size(); i++) {
      std::vector<int> v;
      contacts.push_back(v);
      for (unsigned int j=0; j<atoms.size(); j++) {
	 if (j != i) {
	    if (clipper::Coord_orth::length(atoms[i]->pos, atoms[j]->pos) < 1.85) {
	       contacts[i].push_back(j);
	    }
	 }
      }
   }
   return contacts;
}

std::vector<std::vector<int> >
coot::wligand::getcontacts(const coot::minimol::molecule &mol,
			   const coot::dictionary_residue_restraints_t &dict) const {

   std::vector<coot::minimol::atom *> atoms = mol.select_atoms_serial();
   std::vector<std::vector<int> > contacts(atoms.size());

   // need atom index pairs from dict.bond_restraint[ibond]
   // and those make atom index pairs.
   //
   // need to convert from std::vector<std::pair<int, int> > to std::vector<std::vector<int> >

   std::vector<coot::atom_name_pair> atom_name_pairs;
   for (unsigned int ibond=0; ibond<dict.bond_restraint.size(); ibond++) {
      coot::atom_name_pair pair(dict.bond_restraint[ibond].atom_id_1(),
				dict.bond_restraint[ibond].atom_id_2());
      atom_name_pairs.push_back(pair);
   } 
   std::vector<coot::atom_index_pair> atom_index_pairs =
      get_atom_index_pairs(atom_name_pairs, mol);
   for (unsigned int ipi=0; ipi<atom_index_pairs.size(); ipi++) {
      contacts[atom_index_pairs[ipi].index1].push_back(atom_index_pairs[ipi].index2);
   }
   return contacts;
} 


std::vector <float>
coot::wligand::get_torsions_by_random(const std::vector <coot::dict_torsion_restraint_t> &m_torsions) const { 
   
   std::vector <float> sample_tors(m_torsions.size(), 0.0);

   float non_rotating_torsion_cut_off = 11.0;

   if (0) {
      for(unsigned int itor=0; itor<m_torsions.size(); itor++) {
	 std::cout << "DEBUG get_torsions_by_random: input torsion: " << itor << " "
		   << m_torsions[itor].atom_id_2_4c() << " "
		   << m_torsions[itor].atom_id_3_4c() << " "
		   << m_torsions[itor].angle() << " " << m_torsions[itor].esd()
		   << std::endl;
      }
   }
   
   for(unsigned int itor=0; itor<m_torsions.size(); itor++) {
      if (m_torsions[itor].is_const() == 1) {
	 sample_tors[itor] = m_torsions[itor].angle(); // don't sample it.
      } else { 
	 if (m_torsions[itor].periodicity() == 1) {
	    if (m_torsions[itor].esd() < non_rotating_torsion_cut_off) {
	       sample_tors[itor] = m_torsions[itor].angle(); // don't sample it.
	    } else {
	       sample_tors[itor] = m_torsions[itor].angle();
	       double v = get_random_normal_value();
	       sample_tors[itor] += v*m_torsions[itor].esd();
	    } 
	 } else {
	    // Get a random periodicity sample, e.g. 1 from {0,1,2}
	    if (m_torsions[itor].periodicity() > 0) { 
	       float frac = float (coot::util::random())/float (RAND_MAX);
	       float p_f = float(m_torsions[itor].periodicity());
	       // do floor()?
	       int irandom_periodicity_incidence = int(p_f * frac);
	       float random_periodicity_incidence = float (irandom_periodicity_incidence);
	       // e.g. random_periodicity_incidence is 1.0 from {0.0, 1.0, 2.0}
	       float periodicity_angle = 360.0 * (random_periodicity_incidence/p_f);
// 	       std::cout << "DEBUG:: " << itor << " periodicity_angle: "
// 			 << periodicity_angle << std::endl;
	       sample_tors[itor] = m_torsions[itor].angle();
	       sample_tors[itor] += periodicity_angle;
	    } else {
	       // periodicity is 0
	       sample_tors[itor] = m_torsions[itor].angle();
	    } 

	    if (m_torsions[itor].esd() < non_rotating_torsion_cut_off) {
	       // add nothing
	    } else {
	       // add random selection of distribution using angle esd
	       double v = get_random_normal_value();
	       sample_tors[itor] += v*m_torsions[itor].esd();
	    }
	 }
      }
   }

   if (0) { // debugging torsions
      for(unsigned int itor=0; itor<m_torsions.size(); itor++)
 	 std::cout << "DEBUG:: in get_torsions_by_random sampled torsion " << itor << "  "
 		   <<  sample_tors[itor] << std::endl;
   }

   return sample_tors; 
}

// should this be a coot util function?
//
// get random selection from a normal distribution, mean 0, std: 1
// 
double
coot::wligand::get_random_normal_value() const {

   // now index a random number into that cumlative vector
   //
   float x = -16.0; // some nonsense initial value
   float sum = cumulative.back();
   float r = sum*coot::util::random()/float(RAND_MAX);

   for (unsigned int i=0; i<cumulative.size(); i++) {
      if (r < cumulative[i]) {
	 x = (cumulative_step*float(i))-4.0;
	 if (i>0) { 
	    float cum_i_1 = cumulative[i-1];
	    float cum_i   = cumulative[i];
	    float i_interp = (float(i)-1) + (r-cum_i_1)/(cum_i - cum_i_1);
	    x = (cumulative_step*i_interp)-4.0;
	 }
	 break;
      }
   }
   return x;
}


#ifdef COMPILE_OLD_STUFF

std::vector <float>
coot::wligand::get_torsions_by_random_old(const std::vector <coot::dict_torsion_restraint_t> &m_torsions) const { 
   
   std::vector <float> sample_tors(m_torsions.size());
   float prob;
   float r;

   float max_prob = 1.0;
   for (int itor=0; itor<m_torsions.size(); itor++)
      if (m_torsions[itor].periodicity() > 0) 
         max_prob *= 1/(m_torsions[itor].esd() * sqrt(2.0 * M_PI));  // 1/[2s sqrt(2pi)], what is that?
   std::cout << "max_prob: " << max_prob << std::endl;
   
   for (;;) {
      // Only sample torsions with eds below
      // non_rotating_torsion_cut_off at exact positions,
      // corresponding to repeats due to periodicity
      // 
      float non_rotating_torsion_cut_off = 10.0; 
      for(int itor=0; itor<m_torsions.size(); itor++) {
         if (m_torsions[itor].periodicity() > 0) {
	    if (m_torsions[itor].periodicity() == 1) {
	       sample_tors[itor] = m_torsions[itor].angle(); // don't sample it.
	    } else { 
	       if (m_torsions[itor].esd() > non_rotating_torsion_cut_off) { 
		  sample_tors[itor] = m_torsions[itor].angle() + 
		     360.0 * float (coot::util::random())/float (RAND_MAX);
	       } else {
		  sample_tors[itor] = m_torsions[itor].angle();
		  float frac = float (coot::util::random())/float (RAND_MAX);
		  float p_f = float(m_torsions[itor].periodicity());
		  int n_random_periodicity_incidence = int(p_f * frac);
		  float fnr = float(n_random_periodicity_incidence);
		  sample_tors[itor] += fnr * 360/p_f;
	       }
	       if (sample_tors[itor] > 360.0) {
		  sample_tors[itor] -= 360.0;
	       }
	    }
         }
      }
      r = max_prob * coot::util::random()/ float (RAND_MAX);
      prob = probability_of_torsions(m_torsions, sample_tors);

      std::cout << "max_prob: " << max_prob << std::endl;
      std::cout << "comparing " << prob << " and " << r << std::endl;
      if (prob > r) break;
   }
   std::cout << "get_torsions by random returns : " << std::endl;
   for(int itor=0; itor<m_torsions.size(); itor++)
      std::cout << "    " << itor << "  " <<  sample_tors[itor] << std::endl;

    return sample_tors; 
}

#endif // 0000, don't compile

// Possibly not specific to wligand:
float
coot::wligand::probability_of_torsions(const std::vector <coot::dict_torsion_restraint_t> &m_torsions,
				       const std::vector <float> &r) const {
   double pr = 1.0;
   if (m_torsions.size() != r.size()) {
      std::cout << "ERROR: this should never happen in wligand::probability_of_torsions"
		<< std::endl;
      return -999.0;
   } else {

      //for(int i=0; i<m_torsions.size(); i++) {
         //std::cout << "torsion " << i << " " 
		 //<< m_torsions[i].atom_id_1_4c() << " " 
		 //<< m_torsions[i].atom_id_2_4c() << " " 
		 //<< m_torsions[i].atom_id_3_4c() << " " 
		 //<< m_torsions[i].atom_id_4_4c() << " " 
		 //<< m_torsions[i].angle() << " " 
		 //<< m_torsions[i].esd() << " " 
		 //<< m_torsions[i].periodicity() << " " 
		 //<<std::endl;
      //}

      double z;
      double s;
      for(unsigned int i=0; i<r.size(); i++) {
	 int per = m_torsions[i].periodicity();

	 if (per > 0 ) { 
		 // i.e. actually is a torsion - not some kludged up plane restraint...

	    double diff = 99999.9; 
	    double tdiff;
	    double trial_target;
	    for(int iper=0; iper<per; iper++) { 
	       trial_target =  m_torsions[i].angle()+ double(iper)*360.0/double(per);
	       if (trial_target > 360.0) trial_target -= 360.0; 
	       tdiff = r[i] - trial_target; 
	       if (abs(tdiff) < abs(diff)) { 
	          diff = tdiff;
	       }
	    }
	    if (diff == 99999.9) { 
	       std::cout << "Error in periodicity (" << per << ") check" << std::endl;
	       std::cout << "target_value: " << m_torsions[i].angle()
		         << ", theta: " << r[i] << std::endl;
	    }
   
	    z = diff/m_torsions[i].esd();
	    s = 1/(m_torsions[i].esd() * sqrt(2.0 * M_PI));
//  	    std::cout << "DEBUG:: torsion " << i << " " << diff << "/" << m_torsions[i].esd()
//  		      << " multiplying " << pr << " by " << s
//  		      << " and " << exp( -0.5 * z *z ) << " z is " << z << "\n";
	    pr *= s * exp( -0.5 * z *z );
	 }
      }
   }
   return pr;
} 
				       
 
std::vector<coot::atom_index_pair>
coot::wligand::get_atom_index_pairs(std::vector<coot::atom_name_pair>atom_name_pairs,
				    const coot::minimol::molecule &ligand) const {
   
   int i_store_index; 
   std::vector<coot::atom_index_pair> index_pairs;

   for(unsigned int ifrag=0; ifrag<ligand.fragments.size(); ifrag++) {
      for (int ires=ligand[ifrag].min_res_no(); ires<=ligand[ifrag].max_residue_number(); ires++) {
	 for (unsigned int ipair=0; ipair<atom_name_pairs.size(); ipair++) {
	    i_store_index = -1;
	    for (unsigned int iat=0; iat<ligand[ifrag][ires].atoms.size(); iat++) {
	       //
	       if (ligand[ifrag][ires][iat].name == atom_name_pairs[ipair].atom1) {
		  i_store_index = iat;
	       }
	    }
	    for (unsigned int iat=0; iat<ligand[ifrag][ires].atoms.size(); iat++) {
	       if (ligand[ifrag][ires][iat].name == atom_name_pairs[ipair].atom2) {
		  if (i_store_index > -1) { 
		     index_pairs.push_back(coot::atom_index_pair(i_store_index, iat));
		  }
	       }
	    }
	 }
      }
   }
   
   return index_pairs; 
}

std::vector<coot::atom_index_quad>
coot::wligand::get_atom_index_quads(std::vector<coot::atom_name_quad> atom_name_quads,
				    const coot::minimol::molecule &ligand) const {
   
   int i_store_index_1; 
   int i_store_index_2; 
   int i_store_index_3; 
   std::vector<coot::atom_index_quad> index_quads;

   for(unsigned int ifrag=0; ifrag<ligand.fragments.size(); ifrag++) {
      for (int ires=ligand[ifrag].min_res_no(); ires<=ligand[ifrag].max_residue_number(); ires++) {
	 for (unsigned int iquad=0; iquad<atom_name_quads.size(); iquad++) {
	    i_store_index_1 = -1;
	    i_store_index_2 = -2;
	    i_store_index_3 = -3;
	    for (unsigned int iat=0; iat<ligand[ifrag][ires].atoms.size(); iat++)
	       if (ligand[ifrag][ires][iat].name == atom_name_quads[iquad].atom1)
		  i_store_index_1 = iat;
	    for (unsigned int iat=0; iat<ligand[ifrag][ires].atoms.size(); iat++)
	       if (ligand[ifrag][ires][iat].name == atom_name_quads[iquad].atom2)
		  i_store_index_2 = iat;
	    for (unsigned int iat=0; iat<ligand[ifrag][ires].atoms.size(); iat++)
	       if (ligand[ifrag][ires][iat].name == atom_name_quads[iquad].atom3)
		  i_store_index_3 = iat;

	    for (unsigned int iat=0; iat<ligand[ifrag][ires].atoms.size(); iat++) {
	       if (ligand[ifrag][ires][iat].name == atom_name_quads[iquad].atom2) {
		  if (i_store_index_1 > -1) { 
		     if (i_store_index_2 > -1) { 
			if (i_store_index_3 > -1) { 
			   index_quads.push_back(coot::atom_index_quad(i_store_index_1,
								       i_store_index_2,
								       i_store_index_3,
								       iat));
			}
		     }
		  }
	       }
	    }
	 }
      }
   }
   return index_quads; 
}

std::vector<coot::atom_name_pair>
coot::wligand::get_torsion_bonds_atom_pairs(const std::string &monomer_type,
					    const std::vector <coot::dict_torsion_restraint_t> &monomer_torsions) const {

   std::vector<coot::atom_name_pair> atom_pairs;
   for(unsigned int i=0; i<monomer_torsions.size(); i++) {
      coot::atom_name_pair pair(monomer_torsions[i].atom_id_2_4c(),
				monomer_torsions[i].atom_id_3_4c());
      atom_pairs.push_back(pair);
   }
   return atom_pairs;
} 


std::vector<coot::atom_name_quad>
coot::wligand::get_torsion_bonds_atom_quads(const std::string &monomer_type,
					    const std::vector <coot::dict_torsion_restraint_t> &monomer_torsions) const {

   std::vector<coot::atom_name_quad> atom_quads;
   for(unsigned int i=0; i<monomer_torsions.size(); i++) {
      coot::atom_name_quad quad(monomer_torsions[i].atom_id_1_4c(),
				monomer_torsions[i].atom_id_2_4c(),
				monomer_torsions[i].atom_id_3_4c(),
				monomer_torsions[i].atom_id_4_4c());
      atom_quads.push_back(quad);
   }
   return atom_quads;
}

void
coot::installed_wiggly_ligand_info_t::add_torsion(const coot::dict_torsion_restraint_t &rest,
						  double torsion) {

   coot::torsioned_atoms_info_t t(rest, torsion);
   torsioned_atoms.push_back(t);
}

void
coot::installed_wiggly_ligand_info_t::add_torsions(const std::vector<coot::dict_torsion_restraint_t> &rests,
						   const std::vector<float> torsions) {

   if (torsions.size() == rests.size()) {
      for (unsigned int i=0; i<torsions.size(); i++)
	 add_torsion(rests[i], torsions[i]);
   } else {
      std::cout << "ERROR:: in installed_wiggly_ligand_info_t\n";
      std::cout << "    random_torsions size != non_const_torsions size() "
		<< torsions.size() << " " << rests.size() << std::endl;
   } 

} 

unsigned int
coot::installed_wiggly_ligand_info_t::n_torsions() const {
   return torsioned_atoms.size();
}

// throw an exception if not possible
std::pair<float, float>
coot::installed_wiggly_ligand_info_t::get_set_and_ideal_torsions(int i) const {

   std::pair<float, float> p;
   if (i >= 0) { 
      if (i < n_torsions()) {
	 p.first = torsioned_atoms[i].torsion;
	 p.second = torsioned_atoms[i].torsion_from_restraint;
      } else {
	 std::string mess = "bad torsion index ";
	 mess += coot::util::int_to_string(i);
	 throw std::runtime_error(mess);
      }
   } else {
      std::string mess = "bad torsion index ";
      mess += coot::util::int_to_string(i);
      throw std::runtime_error(mess);
   } 
   return p;
} 

// throw an exception if not possible
std::pair<float, float>
coot::installed_wiggly_ligand_info_t::get_set_and_real_torsions(int i) const {

   std::pair<float, float> p = get_set_and_ideal_torsions(i);
   if (i >= 0) { 
      if (i < n_torsions()) {
	 coot::atom_name_quad quad = torsioned_atoms[i].quad;
	 coot::minimol::residue res = mol[0][1];
	 p.second = res.get_torsion(quad);
	 
      } else {
	 std::string mess = "bad torsion index ";
	 mess += coot::util::int_to_string(i);
	 throw std::runtime_error(mess);
      }
   } else {
      std::string mess = "bad torsion index ";
      mess += coot::util::int_to_string(i);
      throw std::runtime_error(mess);
   }
   return p;
} 
