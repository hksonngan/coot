/* coot-utils/coot-tree-extras.cc
 * 
 * Copyright 2009 by The University of Oxford
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

#include <queue>
#include <sstream>
#include <string.h>

#include "coot-coord-utils.hh"
#include "coot-coord-extras.hh"

// constructor can throw an exception
// 
// the constructor should not throw an exception if there are no
// tree in the restraints.  It should instead try the bonds in
// the restraints.  If there are no bonds then it throws an
// exception.
// 
coot::atom_tree_t::atom_tree_t(const coot::dictionary_residue_restraints_t &rest,
			       const coot::minimol::residue &res_in,
			       const std::string &altconf) {

   made_from_minimol_residue_flag = 1;
   CResidue *residue_p = coot::GetResidue(res_in);
   construct_internal(rest, residue_p, altconf);
}


coot::atom_tree_t::atom_tree_t(const dictionary_residue_restraints_t &rest,
			       const std::vector<std::vector<int> > &contact_indices,
			       int base_atom_index, 
			       const minimol::residue &res_in,
			       const std::string &altconf) { 

   made_from_minimol_residue_flag = 1;
   residue = coot::GetResidue(res_in);
   fill_name_map(altconf);
   fill_atom_vertex_vec_using_contacts(contact_indices, base_atom_index);
}

// can throw an exception.
coot::atom_tree_t::atom_tree_t(const coot::dictionary_residue_restraints_t &rest,
			       CResidue *res,
			       const std::string &altconf) {

   made_from_minimol_residue_flag = 0;
   construct_internal(rest, res, altconf);

}   

// the constructor, given a list of bonds and a base atom index.
// Used perhaps as the fallback when the above raises an
// exception.
coot::atom_tree_t::atom_tree_t(const std::vector<std::vector<int> > &contact_indices,
			       int base_atom_index, 
			       CResidue *res,
			       const std::string &altconf) {
   made_from_minimol_residue_flag = 0;
   if (! res) {
      std::string mess = "null residue in alternate atom_tree_t constructor";
      throw std::runtime_error(mess);
   } else { 
      residue = res;
      fill_name_map(altconf);
      fill_atom_vertex_vec_using_contacts(contact_indices, base_atom_index);
   } 

}



void 
coot::atom_tree_t::construct_internal(const coot::dictionary_residue_restraints_t &rest,
				       CResidue *res,
				       const std::string &altconf) {

   if (! res) {
      std::string mess = "Null residue in atom tree constructor";
      throw std::runtime_error(mess);
   }
   residue = res; // class data now, used in rotate_about() - so that
		  // we don't have to pass the CResidue * again.

   if (rest.tree.size() == 0) {
      std::string mess = "No tree in restraint";
      throw std::runtime_error(mess);
   }

   // Fill bonds
   PPCAtom residue_atoms = 0;
   int n_residue_atoms;
   res->GetAtomTable(residue_atoms, n_residue_atoms);
   // fill the bonds
   for (unsigned int i=0; i<rest.bond_restraint.size(); i++) {
      int idx1 = -1;
      int idx2 = -1;
      for (int iat=0; iat<n_residue_atoms; iat++) {
	 std::string atom_name = residue_atoms[iat]->name;
	 std::string atom_altl = residue_atoms[iat]->altLoc;
	 if (atom_name == rest.bond_restraint[i].atom_id_1())
	    if (atom_altl == "" || atom_altl == altconf)
	       idx1 = iat;
	 if (atom_name == rest.bond_restraint[i].atom_id_2())
	    if (atom_altl == "" || atom_altl == altconf)
	       idx2 = iat;
	 // OK, we have found them, no need to go on searching.
	 if ((idx1 != -1) && (idx2 != -1))
	    break;
      }

      if ((idx1 != -1) && (idx2 != -1))
	 bonds.push_back(std::pair<int,int>(idx1, idx2));
   }

   fill_name_map(altconf);

   bool success_vertex = fill_atom_vertex_vec(rest, res, altconf);
   if (! success_vertex) {
      std::string mess = "Failed to fill atom vector from cif atom tree - bad tree?";
      throw std::runtime_error(mess);
   }

   bool success_torsion = fill_torsions(rest, res, altconf);

}


void
coot::atom_tree_t::fill_name_map(const std::string &altconf) {
   
   PPCAtom residue_atoms = 0;
   int n_residue_atoms;
   residue->GetAtomTable(residue_atoms, n_residue_atoms);

   // atom-name -> index, now class variable
   // std::map<std::string, int, std::less<std::string> > name_to_index;
   for (int iat=0; iat<n_residue_atoms; iat++) {
      std::string atom_name(residue_atoms[iat]->name);
      std::string atom_altl = residue_atoms[iat]->altLoc;
      if (0)
	 std::cout << "debug:: comparing altconf of this atom :" << atom_altl
		   << ": to (passed arg) :" << altconf << ": or blank" << std::endl; 
      if (atom_altl == "" || atom_altl == altconf)
	 name_to_index[atom_name] = iat;
   }
}



bool
coot::atom_tree_t::fill_torsions(const coot::dictionary_residue_restraints_t &rest, CResidue *res,
				 const std::string &altconf) {


   bool r = 0;
   int n_torsions_inserted = 0;
   if (rest.torsion_restraint.size() > 0) {
      std::vector<coot::atom_index_quad> quads;
      for (unsigned int itr=0; itr<rest.torsion_restraint.size(); itr++) { 
	 coot::dict_torsion_restraint_t tr = rest.torsion_restraint[itr];
	 std::pair<bool,coot::atom_index_quad> quad_pair = get_atom_index_quad(tr, res, altconf);
	 if (quad_pair.first)
	    quads.push_back(quad_pair.second);
      }

      // Now we have a set of atom index quads, put them in the
      // atom_vertex_vec, at the position of the second atom in the
      // torsion (the position of the first atom in the rotation
      // vector).

      // std::cout << " debug:: " << quads.size() << " quads" << std::endl;
      for (unsigned int iquad=0; iquad<quads.size(); iquad++) {
	 bool inserted = 0;
	 for (unsigned int iv=0; iv<atom_vertex_vec.size(); iv++) {
	    if (iv == quads[iquad].index2) {
	       for (unsigned int ifo=0; ifo<atom_vertex_vec[iv].forward.size(); ifo++) { 
		  if (atom_vertex_vec[iv].forward[ifo] == quads[iquad].index3) {

		     // now check that the forward atom of this
		     // forward atom is index4
		     int this_forward = atom_vertex_vec[iv].forward[ifo];
		     for (unsigned int ifo2=0; ifo2<atom_vertex_vec[this_forward].forward.size(); ifo2++) {
			if (atom_vertex_vec[this_forward].forward[ifo2] == quads[iquad].index4) {
			   atom_vertex_vec[iv].torsion_quad.first = 1;
			   atom_vertex_vec[iv].torsion_quad.second = quads[iquad];
			   if (0) {
			      std::cout << "            DEBUG:: inserting torsion " << iquad
					<< " into vertex " << iv << std::endl;
			   }
			   r = 1;
			   inserted = 1;
			   n_torsions_inserted++;
			}
		     } 
		  }
	       }
	    }
	    if (inserted)
	       break;
	 }
      } 
   }
   // std::cout << "DEBUG:: inserted " << n_torsions_inserted << " torsions" << std::endl;
   return r;
}

bool
coot::atom_tree_t::fill_atom_vertex_vec_using_contacts(const std::vector<std::vector<int> > &contact_indices,
						       int base_atom_index) {

   bool r=0;

   PPCAtom residue_atoms;
   int n_residue_atoms;
   residue->GetAtomTable(residue_atoms, n_residue_atoms);
   atom_vertex_vec.resize(n_residue_atoms);

   coot::atom_vertex av;
   av.connection_type = coot::atom_vertex::START;
   atom_vertex_vec[base_atom_index] = av;

   // fail to set up
   if (contact_indices.size() == 0)
      return 0;

   if (0) { 
      std::cout << " debug:: =========== contact indices ======= " << std::endl;
      for (unsigned int ic1=0; ic1<contact_indices.size(); ic1++) {
	 std::cout << " index " << ic1 << " : ";
	 for (unsigned int ic2=0; ic2<contact_indices[ic1].size(); ic2++)
	    std::cout << contact_indices[ic1][ic2] << " ";
	 std::cout << std::endl;
      }
   }



   std::queue<int> q;
   q.push(base_atom_index);
   std::vector<int> done; // things that were in the queue that are
			  // not done (on poping). So that we don't
			  // loop endlessly doing atom indices that we
			  // have already considered.

   while (q.size()) {
      int this_base_atom = q.front();
      // now what are the forward atoms of av?
      std::vector<int> av_contacts = contact_indices[this_base_atom]; // size check above
      
      for (unsigned int iav=0; iav<av_contacts.size(); iav++) {

	 int i_forward = av_contacts[iav];

	 // Check that this forward atom is not already in the forward
	 // atoms of this_base_atom
	 bool ifound_forward = 0;
	 for (unsigned int ifo=0; ifo<atom_vertex_vec[this_base_atom].forward.size(); ifo++) {
	    if (atom_vertex_vec[this_base_atom].forward[ifo] == av_contacts[iav]) { 
	       ifound_forward = 1;
	       break;
	    }
	 }

	 if (! ifound_forward) { 
	    // Add the contact as a forward atom of this_base_atom but
	    // only if the forward atom does not have a forward atom
	    // which is this_base_atom (and this keeps the tree going in
	    // up and not back again).
	    bool ifound_forward_forward = 0;
	    for (unsigned int ifo=0; ifo<atom_vertex_vec[i_forward].forward.size(); ifo++) {
	       if (atom_vertex_vec[i_forward].forward[ifo] == this_base_atom) {
		  ifound_forward_forward = 1;
		  break;
	       }
	    }
	    if (!ifound_forward_forward) {
	       atom_vertex_vec[this_base_atom].forward.push_back(av_contacts[iav]);
	    }
	 }

	 // add the forward atoms to the queue if they are not already
	 // in the done list.
	 bool in_done = 0;
	 for (unsigned int idone=0; idone<done.size(); idone++) {
	    if (done[idone] == av_contacts[iav]) {
	       in_done = 1;
	       break;
	    } 
	 }
	 if (!in_done)
	    q.push(av_contacts[iav]);
      }
      
      // if the forward atoms of this_atom_index do not have a back
      // atom, add one (but not, of course, if forward atom is the
      // start point of the tree).
      for (unsigned int iav=0; iav<av_contacts.size(); iav++) {
	 if (atom_vertex_vec[av_contacts[iav]].backward.size() == 0) {
	    if (atom_vertex_vec[av_contacts[iav]].connection_type != coot::atom_vertex::START) 
	       atom_vertex_vec[av_contacts[iav]].backward.push_back(this_base_atom);
	 }
      }
      q.pop();
      done.push_back(this_base_atom);
      r = 1; // return success
   }
   
   // print out the name_to_index map
   if (0) {
      std::cout << "==== atom indexes ===\n";
      for(std::map<std::string, coot::atom_tree_t::atom_tree_index_t>::const_iterator it = name_to_index.begin();
	  it != name_to_index.end(); ++it)
	 std::cout << "Atom :" << it->first << ": Index: " << it->second.index() << '\n';
   }

   // print out the atom tree
   if (0) {
      std::cout << "debug:: ==== atom_vertex_vec === from fill_atom_vertex_vec_using_contacts "
		<< std::endl;
      for (unsigned int iv=0; iv<atom_vertex_vec.size(); iv++) {
	 std::cout << "   atom_vertex_vec[" << iv << "] forward atom ";
	 for (unsigned int ifo=0; ifo<atom_vertex_vec[iv].forward.size(); ifo++) 
	    std::cout << atom_vertex_vec[iv].forward[ifo] << " ";
	 std::cout << std::endl;
      }
   }

   return r;
} 

// This used to throw an exception.  But now it does not because the
// catcher was empty (we don't want to know about unfilled hydrogen
// torsions).  And an empty catch is bad.  So now we return a pair,
// the first of which defines whether the torsion is good or not.
// 
std::pair<bool,coot::atom_index_quad>
coot::atom_tree_t::get_atom_index_quad(const coot::dict_torsion_restraint_t &tr,
				       CResidue *res,
				       const std::string &altconf) const {
   bool success = 0;
   coot::atom_index_quad quad(-1,-1,-1,-1);
   PPCAtom residue_atoms = 0;
   int n_residue_atoms;
   res->GetAtomTable(residue_atoms, n_residue_atoms);
   for (int iat=0; iat<n_residue_atoms; iat++) {
      std::string atom_name(   residue_atoms[iat]->name);
      std::string atom_altconf(residue_atoms[iat]->altLoc);
      if (atom_name == tr.atom_id_1_4c())
	 if (atom_altconf == "" || atom_altconf == altconf)
	    quad.index1 = iat;
      if (atom_name == tr.atom_id_2_4c())
	 if (atom_altconf == "" || atom_altconf == altconf)
	    quad.index2 = iat;
      if (atom_name == tr.atom_id_3_4c())
	 if (atom_altconf == "" || atom_altconf == altconf)
	    quad.index3 = iat;
      if (atom_name == tr.atom_id_4_4c())
	 if (atom_altconf == "" || atom_altconf == altconf)
	    quad.index4 = iat;
   }
   if ((quad.index1 == -1) || (quad.index2 == -1) ||
       (quad.index3 == -1) || (quad.index4 == -1)) {
      success = 0;
   } else {
      success = 1;
   }
   return std::pair<bool,coot::atom_index_quad> (success,quad);
}


bool
coot::atom_tree_t::fill_atom_vertex_vec(const coot::dictionary_residue_restraints_t &rest,
					CResidue *res,
					const std::string &altconf) {

   bool debug = 0;

   // Note that we add an extra check on the forward and back atom
   // indices before adding them.  This is a sanity check - if we come
   // here with a big tree but a small residue, then the forward and
   // back atom indices could go beyond the limits of the size of the
   // atom_vertex_vec.  Which would be badness.  Hmm.. I am not sure
   // about this now. The indices are into the residue table, are they
   // not?  And the residue table corresponds here to residue_atoms
   // and n_residue_atoms. Hmm.. maybe that was not the problem.

   bool retval = 0; // fail initially.

   bool found_start = 0;
   int rest_tree_start_index = -1;
   for (unsigned int i=0; i<rest.tree.size(); i++) {
      if (rest.tree[i].connect_type == "START") { 
	 found_start = 1;
	 rest_tree_start_index = i;
	 break;
      }
   }

   // print out the name_to_index map
   if (debug) {
      std::cout << "==== atom indexes ===\n";
      for(std::map<std::string, coot::atom_tree_t::atom_tree_index_t>::const_iterator it = name_to_index.begin();
	  it != name_to_index.end(); ++it)
	 std::cout << "Atom :" << it->first << ": Index: " << it->second.index() << '\n';
   }
 

   if (found_start) {

      // atom vertex is based on residue atoms (not dict).
      int n_residue_atoms = res->GetNumberOfAtoms();
      if (debug)
	 std::cout << "in fill_atom_vertex_vec(), residue has " << n_residue_atoms
		   << " atoms " << std::endl;
      atom_vertex_vec.resize(n_residue_atoms);

      // fill atom_vertex_vec, converting dictionary atom names to
      // residue indices.
      // 
      for (unsigned int itree=0; itree<rest.tree.size(); itree++) {
	 coot::atom_tree_t::atom_tree_index_t atom_id_index = name_to_index[rest.tree[itree].atom_id];
	 if (atom_id_index.is_assigned()) {
	    retval = 1;
	    int idx = atom_id_index.index();
	    coot::atom_tree_t::atom_tree_index_t atom_back_index =
	       name_to_index[rest.tree[itree].atom_back];
	    if (rest.tree[itree].atom_back != "") { 
	       // if connect_type is START then the back atom is n/a -> UNASSIGNED
	       if (atom_back_index.is_assigned()) { 
		  if (atom_back_index.index() < n_residue_atoms) { 
		     // there should be only one back atom for each vertex
		     atom_vertex_vec[idx].backward.push_back(atom_back_index.index());
		     // 
		     // make a synthetic forward atom
		     add_unique_forward_atom(atom_back_index.index(), idx);
		  }
	       }
	    }
	       
	    coot::atom_tree_t::atom_tree_index_t atom_forward_index =
	       name_to_index[rest.tree[itree].atom_forward];

	    if (atom_forward_index.is_assigned()) {
	       if (atom_forward_index.index() < n_residue_atoms)
		  add_unique_forward_atom(idx, atom_forward_index.index());
	    }

	    if (0) { 
	       std::cout << "   itree " << itree << " :"
			 << rest.tree[itree].atom_id << ": :"
		      << rest.tree[itree].atom_back << ": :"
			 << rest.tree[itree].atom_forward << ": -> "
			 << atom_id_index.index() << " "
			 << atom_back_index.index() << " "
			 << atom_forward_index.index()
			 << std::endl;
	    }
	    
	    atom_vertex_vec[idx].connection_type = coot::atom_vertex::STANDARD;
	    if (rest.tree[itree].connect_type == "START")
	       atom_vertex_vec[idx].connection_type = coot::atom_vertex::START;
	    if (rest.tree[itree].connect_type == "END")
	       atom_vertex_vec[idx].connection_type = coot::atom_vertex::END;
	 } else {
	    if (debug)
	       std::cout << "DEBUG:: in fill_atom_vertex_vec() "
			 << " no index assignment for "
			 << rest.tree[itree].atom_id 
			 << std::endl;
	 } 
      }
   } else {
      if (debug)
	 std::cout << "DEBUG:: in fill_atom_vertex_vec() no found start"
		   << std::endl;
   } 

   // print out the atom tree
   if (debug) {
      std::cout << "debug:: ==== atom_vertex_vec (atom_tree) === from fill_atom_vertex_vec "
		<< "======== size " <<  atom_vertex_vec.size() << std::endl;
      for (unsigned int iv=0; iv<atom_vertex_vec.size(); iv++) {
	 std::cout << "   atom_vertex_vec[" << iv << "] forward atoms ("
		   << atom_vertex_vec[iv].forward.size() << ") ";
	 for (unsigned int ifo=0; ifo<atom_vertex_vec[iv].forward.size(); ifo++) 
	    std::cout << atom_vertex_vec[iv].forward[ifo] << " ";
	 std::cout << std::endl;
      }
   }
   if (debug)
      std::cout << "fill_atom_vertex_vec() returns " << retval << std::endl;
   return retval;
}




// Add forward_atom_index as a forward atom of this_index - but only
// if forward_atom_index is not already a forward atom of this_index.
void
coot::atom_tree_t::add_unique_forward_atom(int this_index, int forward_atom_index) { 

   bool ifound = 0;
   for (unsigned int ifo=0; ifo<atom_vertex_vec[this_index].forward.size(); ifo++) {
      if (atom_vertex_vec[this_index].forward[ifo] == forward_atom_index) {
	 ifound = 1;
	 break;
      }
   }

   std::pair<int, std::vector<coot::atom_tree_t::atom_tree_index_t> >
      forward_atoms_of_forward_atom_index_pair = get_forward_atoms(forward_atom_index);

   if (0) 
      std::cout << " in add_unique_forward_atom get_forward_atoms called "
		<< forward_atoms_of_forward_atom_index_pair.first 
		<< " times - indices size: "
		<< forward_atoms_of_forward_atom_index_pair.second.size() << std::endl;
   
   if (0) { 
      std::cout << "debug:: forward atoms of " << forward_atom_index << ":"; 
      for (unsigned int i=0; i<forward_atoms_of_forward_atom_index_pair.second.size(); i++)
	 std::cout << " " << forward_atoms_of_forward_atom_index_pair.second[i].index();
      std::cout << std::endl;
   }

   for (unsigned int i=0; i<forward_atoms_of_forward_atom_index_pair.second.size(); i++)
      if (this_index == forward_atoms_of_forward_atom_index_pair.second[i].index()) {
	 ifound = 1;
	 if (0) { 
	    std::cout << " reject this attempt to add synthetic forward atom because "
		      << this_index << " is a forward atom of " << forward_atom_index
		      << std::endl;
	 }
      }

   if (ifound == 0) {
//       std::cout << "So, new forward: to this_index " << this_index << " added forward "
// 		<< forward_atom_index << std::endl;
      atom_vertex_vec[this_index].forward.push_back(forward_atom_index);
   } 

} 


// Throw exception on unable to rotate atoms.
//
// Return the new torsion angle (use the embedded torsion on index2 if you can)
// Otherwise return -1.0;
// 
double
coot::atom_tree_t::rotate_about(const std::string &atom1, const std::string &atom2,
				double angle, bool reversed_flag) {

   double new_torsion = 0.0;
   // OK, so when the user clicks atom2 then atom1 (as the middle 2
   // atoms), then they implictly want the fragment revsersed,
   // relative to the atom order internally.  In that case, se
   // internal_reversed, and only if it is not set and the
   // reversed_flag flag *is* set, do the reversal (if both are set
   // they cancel each other out).  Now we do not pre-reverse the
   // indices in the calling function. The revere is done here.
   bool internal_reversed = 0;
   
   coot::atom_tree_t::atom_tree_index_t index2 = name_to_index[atom1];
   coot::atom_tree_t::atom_tree_index_t index3 = name_to_index[atom2];

   if ((atom_vertex_vec[index2.index()].forward.size() == 0) && 
       (atom_vertex_vec[index3.index()].forward.size() == 0)) {
      throw std::runtime_error("Neither index2 nor index3 has forward atoms!");
   } 
       
   if (index2.is_assigned()) { 
      if (index3.is_assigned()) {

	 // is index3 in the forward atoms of index2?
	 // if not, then swap and test again.
	 //
	 bool index3_is_forward = 0;

	 for (unsigned int ifo=0; ifo<atom_vertex_vec[index2.index()].forward.size(); ifo++) {
	    if (atom_vertex_vec[index2.index()].forward[ifo] == index3.index()) {
	       index3_is_forward = 1;
	       break;
	    }
	 }

	 if (! index3_is_forward) {
	    // perhaps index2 is the forward atom of index3?
	    bool index2_is_forward = 0;
	    for (unsigned int ifo=0; ifo<atom_vertex_vec[index3.index()].forward.size(); ifo++) {
	       if (atom_vertex_vec[index3.index()].forward[ifo] == index2.index()) {
		  index2_is_forward = 1;
		  break;
	       }
	    }

	    // if index2 *was* the forward atom of index3, then swap
	    // around index2 and 3 into "standard" order.
	    // 
	    if (index2_is_forward) {
	       std::swap(index2, index3);
	       index3_is_forward = 1;
	       internal_reversed = 1;
	    }
	 }

	 // OK, try again
	 if (index3_is_forward) {

	    std::pair<int, std::vector<coot::atom_tree_t::atom_tree_index_t> > moving_atom_indices_pair =
	       get_forward_atoms(index3);
	    std::vector<coot::atom_tree_t::atom_tree_index_t> moving_atom_indices =
	       moving_atom_indices_pair.second;
// 	    std::cout << " in rotate_about get_forward_atoms called " << moving_atom_indices_pair.first
// 		      << " times - indices size: " << moving_atom_indices_pair.second.size() << std::endl;

	    // Maybe a synthetic forward atom was made, and later on
	    // in the dictionary, it was a real (normal) forward atom
	    // of a different atom.  In that case there can be 2
	    // copies of the atom in moving_atom_indices.  So let's
	    // filter them one.  Only the first copy.
	    // 
	    std::vector<coot::atom_tree_t::atom_tree_index_t> unique_moving_atom_indices =
	       uniquify_atom_indices(moving_atom_indices);

	    if (0) {
	       std::cout << "number of moving atoms based on atom index " << index3.index()
			 << " is " << moving_atom_indices.size() << std::endl;
	       for (unsigned int imov=0; imov<unique_moving_atom_indices.size(); imov++) {
		  std::cout << "now  moving atom[" << imov << "] is "
			    << unique_moving_atom_indices[imov].index() << std::endl;
	       }
	    }

	    // internal_reversed reversed_flag   action
	    //      0                0             -
	    //      0                1          complementary_indices
	    //      1                0          complementary_indices
	    //      1                1             -

	    bool xorv = reversed_flag ^ internal_reversed;
	    if (xorv)
	       unique_moving_atom_indices = complementary_indices(unique_moving_atom_indices,
								  index2, index3);

	    // so now we have a set of moving atoms:
	    // set up the coordinates for the rotation, and rotate
	    PPCAtom residue_atoms = 0;
	    int n_residue_atoms;
	    residue->GetAtomTable(residue_atoms, n_residue_atoms);
	    CAtom *at2 = residue_atoms[index2.index()];
	    CAtom *at3 = residue_atoms[index3.index()];
	    clipper::Coord_orth base_atom_pos(at2->x, at2->y, at2->z);
	    clipper::Coord_orth    third_atom(at3->x, at3->y, at3->z);;
	    clipper::Coord_orth direction = third_atom - base_atom_pos;

	    rotate_internal(unique_moving_atom_indices, direction, base_atom_pos, angle);

	    // set the new_torsion (return value) if possible.
	    // 
	    if (atom_vertex_vec[index2.index()].torsion_quad.first) {
	       new_torsion = quad_to_torsion(index2);
	    } 
	 }
      } else {
	 throw std::runtime_error("ERROR:: rotate_about(): index3 not assigned");
      } 
   } else {
      throw std::runtime_error("ERROR:: rotate_about(): index2 not assigned");
   } 
   return new_torsion;
}


// return the torsion angle in degrees.
double
coot::atom_tree_t::quad_to_torsion(const coot::atom_tree_t::atom_tree_index_t &index2) const {

   // std::cout << " this bond has a chi angle assigned" << std::endl;
   coot::atom_index_quad quad = atom_vertex_vec[index2.index()].torsion_quad.second;
   clipper::Coord_orth co[4];
   PPCAtom residue_atoms;
   int n_residue_atoms;
   residue->GetAtomTable(residue_atoms, n_residue_atoms);
   co[0] = clipper::Coord_orth(residue_atoms[quad.index1]->x,
			       residue_atoms[quad.index1]->y,
			       residue_atoms[quad.index1]->z);
   co[1] = clipper::Coord_orth(residue_atoms[quad.index2]->x,
			       residue_atoms[quad.index2]->y,
			       residue_atoms[quad.index2]->z);
   co[2] = clipper::Coord_orth(residue_atoms[quad.index3]->x,
			       residue_atoms[quad.index3]->y,
			       residue_atoms[quad.index3]->z);
   co[3] = clipper::Coord_orth(residue_atoms[quad.index4]->x,
			       residue_atoms[quad.index4]->y,
			       residue_atoms[quad.index4]->z);
   double ar = clipper::Coord_orth::torsion(co[0], co[1], co[2], co[3]);
   double new_torsion = clipper::Util::rad2d(ar);
   return new_torsion;
} 

// Back atoms, not very useful - is is only a single path.
// 
// some sort of recursion here
std::vector<coot::atom_tree_t::atom_tree_index_t>
coot::atom_tree_t::get_back_atoms(const coot::atom_tree_t::atom_tree_index_t &index) const {

   std::vector<coot::atom_tree_t::atom_tree_index_t> v;

   if (atom_vertex_vec[index.index()].connection_type == coot::atom_vertex::END)
      return v;
   
   // all of these
   for (unsigned int ib=0; ib<atom_vertex_vec[index.index()].backward.size(); ib++) {
      v.push_back(atom_vertex_vec[index.index()].backward[ib]);
   }

   // and the back atoms of back atoms.
   for (unsigned int ib=0; ib<atom_vertex_vec[index.index()].backward.size(); ib++) {
      int index_back = atom_vertex_vec[index.index()].backward[ib];
      coot::atom_tree_t::atom_tree_index_t back_index(index_back); // ho ho
      std::vector<coot::atom_tree_t::atom_tree_index_t> nv = 
	 coot::atom_tree_t::get_back_atoms(index_back);
      for (unsigned int inv=0; inv<nv.size(); inv++)
	 v.push_back(nv[inv]);
   }
   return v;
}


// with forward recursion
// 
std::pair<int, std::vector<coot::atom_tree_t::atom_tree_index_t> > 
coot::atom_tree_t::get_forward_atoms(const coot::atom_tree_t::atom_tree_index_t &index) const {

   std::vector<coot::atom_tree_t::atom_tree_index_t> v;
   int n_forward_count = 1; // this time at least

   // If atom_vertex_vec was not filled, then we should not index into
   // it with index.index():  Stops a crash, at least.
   // 
   if (index.index() >= atom_vertex_vec.size())
      return std::pair<int, std::vector<coot::atom_tree_t::atom_tree_index_t> > (n_forward_count, v);


   // how can this happen?
   if (atom_vertex_vec[index.index()].connection_type == coot::atom_vertex::START) {
      // std::cout << " in get_forward_atoms index " <<  index.index() << " at start" << std::endl;
      return std::pair<int, std::vector<coot::atom_tree_t::atom_tree_index_t> > (n_forward_count, v);
   } 

   // Bizarre atom trees in the refmac dictionary - the END atom is
   // marks the C atom - not the O.  How can that be sane?
   // 
//    if (atom_vertex_vec[index.index()].connection_type == coot::atom_vertex::END) { 
//       // std::cout << " in get_forward_atoms index " <<  index.index() << " at end" << std::endl;
//       return v;
//    } 
   
   // all of these
   // 
   if (0) {
      std::cout << "DEBUG::get_forward_atoms of " << index.index() << " has "
		<< atom_vertex_vec[index.index()].forward.size()
		<< " forward atoms ";
      for (unsigned int ifo=0; ifo<atom_vertex_vec[index.index()].forward.size(); ifo++)
	 std::cout << " " << atom_vertex_vec[index.index()].forward[ifo];
      std::cout << std::endl;
   }
   
   for (unsigned int ifo=0; ifo<atom_vertex_vec[index.index()].forward.size(); ifo++) { 
      v.push_back(atom_vertex_vec[index.index()].forward[ifo]);
      if (0) 
	 std::cout << " adding to forward atoms " << atom_vertex_vec[index.index()].forward[ifo]
		   << " which is atom_vertex_vec[" << index.index() << "].forward[" << ifo << "]"
		   << std::endl;
   }

   // and the forward atoms of the forward atoms
   for (unsigned int ifo=0; ifo<atom_vertex_vec[index.index()].forward.size(); ifo++) {
      int index_forward = atom_vertex_vec[index.index()].forward[ifo];
      coot::atom_tree_t::atom_tree_index_t forward_index(index_forward);
      std::pair<int, std::vector<coot::atom_tree_t::atom_tree_index_t> > nv_pair = 
	 coot::atom_tree_t::get_forward_atoms(forward_index);
      std::vector<coot::atom_tree_t::atom_tree_index_t> nv = nv_pair.second;
      n_forward_count += nv_pair.first;
      for (unsigned int inv=0; inv<nv.size(); inv++) {
	 if (0) 
	    std::cout << " adding item  " << inv << " which has index " << nv[inv].index()
		      << " as a forward atom of " << index.index() << std::endl;
	 v.push_back(nv[inv]);
      }
   }

   return std::pair<int, std::vector<coot::atom_tree_t::atom_tree_index_t> > (n_forward_count, uniquify_atom_indices(v));
}



std::vector<coot::atom_tree_t::atom_tree_index_t>
coot::atom_tree_t::uniquify_atom_indices(const std::vector<coot::atom_tree_t::atom_tree_index_t> &vin) const {

   std::vector<coot::atom_tree_t::atom_tree_index_t> v;

   for (unsigned int iv=0; iv<vin.size(); iv++) {
      // vin[iv] is in v?
      bool found = 0;

      for (unsigned int ii=0; ii<v.size(); ii++) {
	 if (vin[iv] == v[ii]) {
	    found = 1;
	    break;
	 } 
      }

      if (found == 0)
	 v.push_back(vin[iv]);
   } 
   return v; 
} 



// Return the complementary indices c.f. the moving atom set, but do
// not include index2 or index3 in the returned set (they do not move
// even with the reverse flag (of course)).
std::vector<coot::atom_tree_t::atom_tree_index_t> 
coot::atom_tree_t::complementary_indices(const std::vector<coot::atom_tree_t::atom_tree_index_t> &moving_atom_indices,
					 const coot::atom_tree_t::atom_tree_index_t &index2,
					 const coot::atom_tree_t::atom_tree_index_t &index3) const {

   std::vector<coot::atom_tree_t::atom_tree_index_t> v;
   for (unsigned int ivert=0; ivert<atom_vertex_vec.size(); ivert++) {
      bool ifound = 0;
      for (unsigned int im=0; im<moving_atom_indices.size(); im++) {
	 if (moving_atom_indices[im].index() == ivert) {
	    ifound = 1;
	    break;
	 }
      }
      if (ifound == 0)
	 if (index2.index() != ivert)
	    if (index3.index() != ivert)
	       v.push_back(ivert);
   }
   
   return v;
} 




// so now we have a set of moving and non-moving atoms:
//
// the angle is in radians. 
void
coot::atom_tree_t::rotate_internal(std::vector<coot::atom_tree_t::atom_tree_index_t> moving_atom_indices,
				   const clipper::Coord_orth &dir,
				   const clipper::Coord_orth &base_atom_pos,
				   double angle) {

//    std::cout << "in rotate_internal with " << moving_atom_indices.size() << " moving atoms "
// 	     << std::endl;
   PPCAtom residue_atoms = 0;
   int n_residue_atoms;
   residue->GetAtomTable(residue_atoms, n_residue_atoms);
   
   for (unsigned int im=0; im<moving_atom_indices.size(); im++) {
      int idx = moving_atom_indices[im].index();
      CAtom *at = residue_atoms[idx];
      clipper::Coord_orth po(at->x, at->y, at->z);
      clipper::Coord_orth pt = coot::util::rotate_round_vector(dir, po, base_atom_pos, angle);
      if (0)
	 std::cout << " rotate_internal() moving atom number " << im << " " << at->name
		   << " from " << at->x << "," << at->y << "," << at->z << " "
		   << pt.format() << std::endl;
      at->x = pt.x(); 
      at->y = pt.y(); 
      at->z = pt.z(); 
   } 
}

// can throw an exception
double
coot::atom_tree_t::set_dihedral(const std::string &atom1, const std::string &atom2,
				const std::string &atom3, const std::string &atom4,
				double angle) {

   double dihedral_angle = 0.0;
   coot::atom_tree_t::atom_tree_index_t i1 = name_to_index[atom1];
   coot::atom_tree_t::atom_tree_index_t i2 = name_to_index[atom2];
   coot::atom_tree_t::atom_tree_index_t i3 = name_to_index[atom3];
   coot::atom_tree_t::atom_tree_index_t i4 = name_to_index[atom4];

   if (i1.is_assigned() && i2.is_assigned() && i3.is_assigned() && i4.is_assigned()) {
      try {
	 coot::atom_index_quad iq(i1.index(), i2.index(), i3.index(), i4.index());
	 double current_dihedral_angle = iq.torsion(residue);
	 double diff = angle - current_dihedral_angle;
	 if (diff > 360.0)
	    diff -= 360.0;
	 if (diff < -360.0)
	    diff += 360.0;
	 // take out this try/catch when done.
	 rotate_about(atom2, atom3, clipper::Util::d2rad(diff), 0);

	 dihedral_angle = iq.torsion(residue);
      }
      catch (std::runtime_error rte) {
	 std::cout << rte.what() << std::endl;
	 std::string mess = "Torsion failure for ";
	 mess += atom1;
	 mess += " to ";
	 mess += atom2;
	 mess += " to ";
	 mess += atom3;
	 mess += " to ";
	 mess += atom4;
	 mess += ". No unassigned atoms";
	 throw std::runtime_error(mess);
      }
   } else {
      std::string mess = "Atom name(s) not found in residue. ";
      std::vector<std::string> unassigned;
      if (! i1.is_assigned()) unassigned.push_back(atom1);
      if (! i2.is_assigned()) unassigned.push_back(atom2);
      if (! i3.is_assigned()) unassigned.push_back(atom3);
      if (! i4.is_assigned()) unassigned.push_back(atom4);
      if (unassigned.size() > 0) {
	 mess += "Unassigned atoms: ";
	 for (unsigned int i=0; i<unassigned.size(); i++) { 
	    mess += "\"";
	    mess += unassigned[i];
	    mess += "\"  ";
	 }
      }
      throw std::runtime_error(mess);
   }
   return dihedral_angle;
}


// this can throw an exception
// 
// return the set of angles - should be the same that they were
// set to (for validation).
std::vector<double>
coot::atom_tree_t::set_dihedral_multi(const std::vector<tree_dihedral_info_t> &di) {

   std::vector<double> v(di.size());
   for (unsigned int id=0; id<di.size(); id++) {
      v[id] = set_dihedral(di[id].quad.atom1, di[id].quad.atom2,
			   di[id].quad.atom3, di[id].quad.atom4, di[id].dihedral_angle);
   }
   return v;
}



// For use with above function, where the class constructor is made
// with a minimol::residue.
// 
coot::minimol::residue
coot::atom_tree_t::GetResidue() const {

   return coot::minimol::residue(residue);
}

std::ostream&
coot::operator<<(std::ostream &o, coot::atom_tree_t::tree_dihedral_info_t t) { 

   o << "[dihedral-info: " << t.quad << " " << t.dihedral_angle << "]";
   return o;
}


