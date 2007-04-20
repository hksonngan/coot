/* ligand/monomer-utils.cc
 * 
 * Copyright 2002, 2003, 2004, 2005 by The University of York
 * Author: Paul Emsley
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
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
 * 02110-1301, USA.
 */

#include "monomer-utils.hh"

CResidue *
coot::deep_copy_residue(const CResidue *residue) {

   // Horrible casting to CResidue because GetSeqNum and GetAtomTable
   // are not const functions.
   // 
   CResidue *rres = new CResidue;
   CChain   *chain_p = new CChain;
   chain_p->SetChainID(((CResidue *)residue)->GetChainID());
   rres->seqNum = ((CResidue *)residue)->GetSeqNum();
   strcpy(rres->name, residue->name);

   PPCAtom residue_atoms;
   int nResidueAtoms;
   ((CResidue *)residue)->GetAtomTable(residue_atoms, nResidueAtoms);
   CAtom *atom_p;
   
   for(int iat=0; iat<nResidueAtoms; iat++) { 
      atom_p = new CAtom;
      atom_p->Copy(residue_atoms[iat]);
      // std::cout << "DEBUG:: " << atom_p << std::endl;
      rres->AddAtom(atom_p);
   }
   chain_p->AddResidue(rres);
   return rres;
}

			     
void
coot::monomer_utils::add_torsion_bond_by_name(const std::string &atom_name_1,
					      const std::string &atom_name_2) {

   atom_name_pair_list.push_back(coot::atom_name_pair(atom_name_1,
						      atom_name_2)); 

}



coot::contact_info
coot::monomer_utils::getcontacts(const atom_selection_container_t &asc) const {

   PSContact pscontact = NULL;
   int n_contacts;
   float min_dist = 0.1;
   float max_dist = 1.9; // CB->SG CYS 1.8A
   long i_contact_group = 1;
   mat44 my_matt;
   CSymOps symm;
   for (int i=0; i<4; i++) 
      for (int j=0; j<4; j++) 
	 my_matt[i][j] = 0.0;      
   for (int i=0; i<4; i++) my_matt[i][i] = 1.0;

   asc.mol->SeekContacts(asc.atom_selection, asc.n_selected_atoms,
			 asc.atom_selection, asc.n_selected_atoms,
			 min_dist, max_dist, // min, max distances
			 0,        // seqDist 0 -> in same res also
			 pscontact, n_contacts,
			 0, &my_matt, i_contact_group);

   return contact_info(pscontact, n_contacts);

} 


std::vector<coot::atom_index_pair> 
coot::monomer_utils::get_atom_index_pairs(const std::vector<coot::atom_name_pair> &atom_name_pairs_in,
				     const PPCAtom atoms, int nresatoms) const {

   int i_store_index;
   std::vector<coot::atom_index_pair> index_pairs;

   for (int ipair=0; ipair<atom_name_pairs_in.size(); ipair++) {
      int ifound = 0;
      i_store_index = -1;
      for(int i=0; i<nresatoms; i++) {
	 std::string atomname = atoms[i]->name;
	 if (atomname == atom_name_pairs_in[ipair].atom1) {
	    i_store_index = i;
	 }
      }
      if (i_store_index > -1) { // i.e. we found the first atom
	 for(int i2=0; i2<nresatoms; i2++) {
	    std::string atomname = atoms[i2]->name;
	    if (atomname == atom_name_pairs_in[ipair].atom2) {
	       index_pairs.push_back(coot::atom_index_pair(i_store_index, i2));
	    }
	 }
      } else {
	 std::cout << "first atom " << atom_name_pairs_in[ipair].atom1
		   << " not found in residue\n";
      }
   }

   if (index_pairs.size() != atom_name_pairs_in.size()) {
      std::cout << "failure to find all atom pair in residue atoms\n" ;
   } else {
      std::cout << "found all pairs in residue atoms\n" ;
   } 
   return index_pairs;
} 

coot::Cartesian
coot::monomer_utils::coord_orth_to_cartesian(const clipper::Coord_orth &c) {
   return coot::Cartesian(c.x(), c.y(), c.z());
}

clipper::Coord_orth coot::monomer_utils::coord_orth_to_cart(const Cartesian &c) {
   return clipper::Coord_orth(c.x(), c.y(), c.z());
} 
