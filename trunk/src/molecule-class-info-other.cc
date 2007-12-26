/* src/molecule-class-info-other.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 by The University of York
 * Copyright 2007 by The University of Oxford
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
 * 02110-1301, USA
 */

#include <stdlib.h>

#if !defined WINDOWS_MINGW && !defined _MSC_VER
#  include <glob.h>
#  include <unistd.h>
#else
#   ifdef _MSC_VER
#     include <windows.h>
#     undef AddAtom
#     define AddAtomA AddAtom
#   else
#     include <unistd.h>
#   endif
#endif


#include <sys/types.h>  // for stating
#include <sys/stat.h>
#include <ctype.h>  // for toupper


#include <iostream>
#include <vector>
#include <queue>

#include "clipper/core/xmap.h"
#include "clipper/cns/cns_hkl_io.h"

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "graphics-info.h"
#include "xmap-utils.h"

#include "molecule-class-info.h"

#ifdef USE_DUNBRACK_ROTAMERS
#include "dunbrack.hh"
#else 
#include "richardson-rotamer.hh"
#endif 

#include "ligand.hh"
#include "coot-utils.hh"
#include "coot-trim.hh"
#include "coot-map-utils.hh"
#include "coot-coord-utils.hh" // check_dictionary_for_residue
#include "coot-map-heavy.hh"   // situation's heavy... [simplex]
#include "pepflip.hh"

#include "coot-nomenclature.hh"

#include "GL/glu.h"

void
molecule_class_info_t::debug_selection() const { 

   // sigh - debugging 
   // 
   
   int SelHnd = atom_sel.mol->NewSelection();
   PPCAtom atom;
   int n_atoms;

   atom_sel.mol->SelectAtoms(SelHnd,
		    0,
		    "A",
		    888, "*",
		    890, "*",
		    "*", // rnames
		    "*", // anames
		    "*", // elements
		    "*"  // altLocs 
		    );

   atom_sel.mol->GetSelIndex(SelHnd, atom, n_atoms);
   if (n_atoms == 0) { 
      std::cout << "debug_selection: no atoms selected" << std::endl;
   } else {       
      std::cout << "debug_selection: selected atoms" << std::endl;
      for(int i=0; i<n_atoms; i++) { 
	 std::cout << atom[i] << std::endl;
      }
      std::cout << "----------- " << std::endl;
   } 
}

short int
molecule_class_info_t::molecule_is_all_c_alphas() const {

   short int is_ca = 1;
   
   int n_atoms = atom_sel.n_selected_atoms;
   if (n_atoms == 0) {
      is_ca = 0;
   } else { 
      for (int i=0; i<n_atoms; i++) {
	 std::string name_string(atom_sel.atom_selection[i]->name);
	 if ( ! (name_string== " CA " )) {
	    is_ca = 0;
	    break;
	 }
      }
   }
   return is_ca;
} 

void
molecule_class_info_t::bond_representation() {
   makebonds();
}

void
molecule_class_info_t::ca_representation() {

   bonds_box.clear_up();
   make_ca_bonds(2.4, 4.7);
   bonds_box_type = coot::CA_BONDS;
}

void
molecule_class_info_t::ca_plus_ligands_representation() { 

   bonds_box.clear_up();
   make_ca_plus_ligands_bonds();
   bonds_box_type = coot::CA_BONDS_PLUS_LIGANDS;
}

void 
molecule_class_info_t::bonds_no_waters_representation() { 

   bonds_box.clear_up();
   Bond_lines_container bonds;
   bonds.do_normal_bonds_no_water(atom_sel, 0.01, 1.9);
   bonds_box = bonds.make_graphical_bonds();
   bonds_box_type = coot::BONDS_NO_WATERS;
} 

void
molecule_class_info_t::bonds_sec_struct_representation() { 

   // 
   Bond_lines_container bonds;
   bonds.do_colour_sec_struct_bonds(atom_sel, 0.01, 1.9);
   bonds_box = bonds.make_graphical_bonds();
   bonds_box_type = coot::BONDS_SEC_STRUCT_COLOUR;
} 
  

void 
molecule_class_info_t::ca_plus_ligands_sec_struct_representation() { 

   // 
   Bond_lines_container bonds;
   bonds.do_Ca_plus_ligands_colour_sec_struct_bonds(atom_sel, 2.4, 4.7);
   bonds_box = bonds.make_graphical_bonds();
   bonds_box_type = coot::CA_BONDS_PLUS_LIGANDS_SEC_STRUCT_COLOUR;
}

void
molecule_class_info_t::ca_plus_ligands_rainbow_representation() {

   // 
   Bond_lines_container bonds;
   bonds.do_Ca_plus_ligands_bonds(atom_sel, 2.4, 4.7,
				  coot::COLOUR_BY_RAINBOW); // not COLOUR_BY_RAINBOW_BONDS
   bonds_box = bonds.make_graphical_bonds();
   bonds_box_type = coot::COLOUR_BY_RAINBOW_BONDS;
}

void
molecule_class_info_t::b_factor_representation() { 

   Bond_lines_container::bond_representation_type bond_type =
      Bond_lines_container::COLOUR_BY_B_FACTOR;

   Bond_lines_container bonds(atom_sel, bond_type);
   bonds_box = bonds.make_graphical_bonds();
   bonds_box_type = coot::COLOUR_BY_B_FACTOR_BONDS;
} 

void
molecule_class_info_t::occupancy_representation() { 

   Bond_lines_container::bond_representation_type bond_type =
      Bond_lines_container::COLOUR_BY_OCCUPANCY;

   Bond_lines_container bonds(atom_sel, bond_type);
   bonds_box = bonds.make_graphical_bonds();
   bonds_box_type = coot::COLOUR_BY_OCCUPANCY_BONDS;
} 


int
molecule_class_info_t::set_atom_attribute(std::string chain_id, int resno, std::string ins_code,
					  std::string atom_name, std::string alt_conf,
					  std::string attribute_name, float val) {

   int istate = 0;
   if (atom_sel.n_selected_atoms > 0) {
      
      int SelectionHandle = atom_sel.mol->NewSelection(); 
      atom_sel.mol->SelectAtoms(SelectionHandle, 0,
				(char *) chain_id.c_str(),
				resno, (char *) ins_code.c_str(),
				resno, (char *) ins_code.c_str(),
				"*",  // res type
				(char *) atom_name.c_str(),
				"*",  // any element
				(char *) alt_conf.c_str(), SKEY_NEW);
      int nSelAtoms;
      PPCAtom SelAtoms = NULL;
      atom_sel.mol->GetSelIndex(SelectionHandle, SelAtoms, nSelAtoms);
      if (nSelAtoms > 0) {
	 CAtom *at = SelAtoms[0];
	 if (attribute_name == "x")
	    at->x = val;
	 if (attribute_name == "y")
	    at->y = val;
	 if (attribute_name == "z")
	    at->z = val;
	 if (attribute_name == "B")
	    at->tempFactor = val;
	 if (attribute_name == "b")
	    at->tempFactor = val;
	 if (attribute_name == "occ")
	    at->occupancy = val;
      }
      atom_sel.mol->DeleteSelection(SelectionHandle);
   }
   have_unsaved_changes_flag = 1;
   atom_sel.mol->FinishStructEdit();
   make_bonds_type_checked(); // calls update_ghosts()
   return istate;
}

int
molecule_class_info_t::set_atom_string_attribute(std::string chain_id, int resno, std::string ins_code,
						 std::string atom_name, std::string alt_conf,
						 std::string attribute_name, std::string val_str) { 

   int istate = 0;
   if (atom_sel.n_selected_atoms > 0) {
      int SelectionHandle = atom_sel.mol->NewSelection(); 
      atom_sel.mol->SelectAtoms(SelectionHandle, 0,
				(char *) chain_id.c_str(),
				resno, (char *) ins_code.c_str(),
				resno, (char *) ins_code.c_str(),
				"*",
				(char *) atom_name.c_str(),
				"*",
				(char *) alt_conf.c_str());
      int nSelAtoms;
      PPCAtom SelAtoms;
      atom_sel.mol->GetSelIndex(SelectionHandle, SelAtoms, nSelAtoms);
      if (nSelAtoms > 0) {
	 CAtom *at = SelAtoms[0];
	 if (attribute_name == "atom-name")
	    at->SetAtomName((char *)val_str.c_str());
	 if (attribute_name == "alt-conf") {
	    strncpy(at->altLoc, val_str.c_str(), 2);
	 }
	 if (attribute_name == "element") {
	    at->SetElementName(val_str.c_str());
	 }
      }
      have_unsaved_changes_flag = 1;
      atom_sel.mol->FinishStructEdit();
      make_bonds_type_checked(); // calls update_ghosts()
   }
   return istate;
}

int
molecule_class_info_t::set_atom_attributes(const std::vector<coot::atom_attribute_setting_t> &v) {

   int istate = 0;
   if (has_model()) {
      if (v.size() > 0) {
	 for (unsigned int iv=0; iv<v.size(); iv++) {
	    int SelectionHandle = atom_sel.mol->NewSelection();
	    atom_sel.mol->SelectAtoms(SelectionHandle, 0,
				      (char *) v[iv].atom_spec.chain.c_str(),
				      v[iv].atom_spec.resno, (char *) v[iv].atom_spec.insertion_code.c_str(),
				      v[iv].atom_spec.resno, (char *) v[iv].atom_spec.insertion_code.c_str(),
				      "*",
				      (char *) v[iv].atom_spec.atom_name.c_str(),
				      "*",
				      (char *) v[iv].atom_spec.alt_conf.c_str());
	    int nSelAtoms;
	    PPCAtom SelAtoms;
	    atom_sel.mol->GetSelIndex(SelectionHandle, SelAtoms, nSelAtoms);
	    // 	    std::cout << "DEBUG:: considering " << v[iv].atom_spec << " nSelAtoms: " << nSelAtoms << std::endl;
	    if (nSelAtoms > 0) {
	       CAtom *at = SelAtoms[0];
// 	       std::cout << "DEBUG:: "
// 			 << v[iv].attribute_value.type << " " << v[iv].attribute_name << " :"
// 			 << v[iv].attribute_value.s << ": " << v[iv].attribute_value.val
// 			 << std::endl;
	       if (v[iv].attribute_value.type == coot::atom_attribute_setting_help_t::IS_STRING) { 
		  if (v[iv].attribute_name == "atom-name")
		     at->SetAtomName((char *) v[iv].attribute_value.s.c_str());
		  if (v[iv].attribute_name == "alt-conf") {
		     strncpy(at->altLoc, v[iv].attribute_value.s.c_str(), 2);
		  }
		  if (v[iv].attribute_name == "element") {
		     at->SetElementName(v[iv].attribute_value.s.c_str());
		  }
	       }
	       if (v[iv].attribute_value.type == coot::atom_attribute_setting_help_t::IS_FLOAT) {
		  if (v[iv].attribute_name == "x")
		     at->x = v[iv].attribute_value.val;
		  if (v[iv].attribute_name == "y")
		     at->y = v[iv].attribute_value.val;
		  if (v[iv].attribute_name == "z")
		     at->z = v[iv].attribute_value.val;
		  if (v[iv].attribute_name == "b")
		     at->tempFactor = v[iv].attribute_value.val;
		  if (v[iv].attribute_name == "B")
		     at->tempFactor = v[iv].attribute_value.val;
		  if (v[iv].attribute_name == "occ")
		     at->occupancy = v[iv].attribute_value.val;
	       }
	    }
	 }
	 have_unsaved_changes_flag = 1;
	 atom_sel.mol->FinishStructEdit();
	 make_bonds_type_checked(); // calls update_ghosts()
      }
   } 
   return istate;
}


// -----------------------------------------------------------------------------
//                     pepflip
// -----------------------------------------------------------------------------
void
molecule_class_info_t::pepflip(int atom_index) {

   const char *chain_id = atom_sel.atom_selection[atom_index]->residue->GetChainID();
   int resno = atom_sel.atom_selection[atom_index]->residue->seqNum;
   std::string atom_name = atom_sel.atom_selection[atom_index]->name;
   std::string altconf =  atom_sel.atom_selection[atom_index]->altLoc;

   std::cout << "flipping " << resno << " " << altconf << " " << chain_id << std::endl;
   if (atom_name == " N  ")
      resno--;

   pepflip_residue(resno, altconf, chain_id);
   
}
// model_refine_dialog_pepflip_button

int
molecule_class_info_t::pepflip_residue(int resno, 
				       const std::string &alt_conf, 
				       const std::string &chain_id) {

   make_backup(); // must do it here, no intermediate.
   int iresult = coot::pepflip(atom_sel.mol, resno, alt_conf, chain_id);

   if (iresult) {
      std::cout << "flipped " << resno << " " << chain_id << std::endl;
      make_bonds_type_checked();
      have_unsaved_changes_flag = 1; 

   } else {
      std::cout << "pepflip failed " << std::endl;
   }

   return iresult;

} 


graphical_bonds_container
molecule_class_info_t::make_environment_bonds_box(int atom_index) const {

   graphical_bonds_container bonds_box;
   
   PCAtom point_atom_p = atom_sel.atom_selection[atom_index];
   graphics_info_t g;

   PPCResidue SelResidues;
   int nSelResdues;

   int ires = point_atom_p->GetSeqNum();
   char *chain_id = point_atom_p->GetChainID();

   int selHnd = atom_sel.mol->NewSelection();
   atom_sel.mol->Select (selHnd, STYPE_RESIDUE, 1,
		 chain_id, // chains
		 ires,"*", // starting res
		 ires,"*", // ending res
		 "*",  // residue name
		 "*",  // Residue must contain this atom name?
		 "*",  // Residue must contain this Element?
		 "*",  // altLocs
		 SKEY_NEW // selection key
		 );
   atom_sel.mol->GetSelIndex(selHnd, SelResidues, nSelResdues);

   if (nSelResdues != 1) {
      std::cout << " something broken in residue selection in ";
      std::cout << "make_environment_bonds_box: got " << nSelResdues
		<< " residues " << std::endl;
   } else {

      PPCAtom residue_atoms;
      int nResidueAtoms;
      SelResidues[0]->GetAtomTable(residue_atoms, nResidueAtoms);
      if (nResidueAtoms == 0) {
	 std::cout << " something broken in atom residue selection in ";
	 std::cout << "make_environment_bonds_box: got " << nResidueAtoms
		   << " atoms " << std::endl;
      } else {

	 short int residue_is_water_flag = 0;
	 std::string residue_name = point_atom_p->GetResName();
	 if (residue_name == "HOH" || residue_name == "WAT")
	    residue_is_water_flag = 1;
	 Bond_lines_container bonds(atom_sel,residue_atoms, nResidueAtoms,
				    residue_is_water_flag,
				    g.environment_min_distance,
				    g.environment_max_distance);
	 bonds_box = bonds.make_graphical_bonds();
      }
   }
   return bonds_box;
} 

graphical_bonds_container
molecule_class_info_t:: make_symmetry_environment_bonds_box(int atom_index) const {
   graphical_bonds_container bonds_box;

   // std::cout << ":: entering make_symmetry_environment_bonds_box" << std::endl;
   if (atom_sel.atom_selection != NULL) {

      graphics_info_t g;
      PPCResidue SelResidues;
      int nSelResdues;
      
      // First select all the atoms in this residue:
      //
      PCAtom point_atom_p = atom_sel.atom_selection[atom_index];
      int ires = point_atom_p->GetSeqNum();
      char *chain_id = point_atom_p->GetChainID();

      int selHnd = atom_sel.mol->NewSelection();
      atom_sel.mol->Select (selHnd, STYPE_RESIDUE, 1,
			    chain_id, // chains
			    ires,"*", // starting res
			    ires,"*", // ending res
			    "*",  // residue name
			    "*",  // Residue must contain this atom name?
			    "*",  // Residue must contain this Element?
			    "*",  // altLocs
			    SKEY_NEW // selection key
			    );
      atom_sel.mol->GetSelIndex(selHnd, SelResidues, nSelResdues);
      
      if (nSelResdues != 1) {
	 std::cout << " something broken in residue selection in ";
	 std::cout << "make_environment_bonds_box: got " << nSelResdues
		   << " residues " << std::endl;
      } else {
	 PPCAtom residue_atoms;
	 int nResidueAtoms;
	 SelResidues[0]->GetAtomTable(residue_atoms, nResidueAtoms);
	 if (nResidueAtoms == 0) {
	    std::cout << " something broken in atom residue selection in ";
	    std::cout << "make_environment_bonds_box: got " << nResidueAtoms
		      << " atoms " << std::endl;
	 } else {

	    short int do_symmetry = 1;
	    // std::cout << "... calling Bond_lines_container constructor" << std::endl;
	    Bond_lines_container bonds(atom_sel, residue_atoms, nResidueAtoms,
				       g.environment_max_distance,
				       g.environment_min_distance,
				       do_symmetry);
	    bonds_box = bonds.make_graphical_bonds();
	 }
      }
   }
   return bonds_box;
}

// return "N', "C" or "not-terminal-residue"
std::string 
molecule_class_info_t::get_term_type_old(int atom_index) { 

   std::string term_type;

   char *chainid = atom_sel.atom_selection[atom_index]->GetChainID();
   int ires_atom = atom_sel.atom_selection[atom_index]->GetSeqNum();
   PCChain chain = atom_sel.mol->GetChain(1,chainid);
   int nres = chain->GetNumberOfResidues();
   int lowest_res_no = 99999;
   int highest_res_no = -99999;
   for (int ires=0; ires<nres; ires++) { 
      PCResidue res = chain->GetResidue(ires);
      if (res) { // could have been deleted (NULL)
	 if (res->GetSeqNum() > highest_res_no) { 
	    highest_res_no = res->GetSeqNum();
	 }
	 if (res->GetSeqNum() < lowest_res_no) { 
	    lowest_res_no = res->GetSeqNum();
	 }
      }
   }
   if (ires_atom == lowest_res_no) { 
      term_type = "N";
   } else { 
      if (ires_atom == highest_res_no) {
	 term_type = "C";
      } else { 
	 term_type = "not-terminal-residue";
      }
   }
   return term_type;
}

// The atom_index is the atom index of the clicked atom.
// 
// Initially, this routine tested for real terminii.
// 
// The one day EJD asked me how I would build a few missing residues
// (in a loop or so) that arp-warp missed.  I said "add terminal
// residue".  Then I tried it and it failed, of course because that
// residue was not a real terminus.  On reflection, I don't think that
// it should fail in this situation.
//
// So, I don't want to test for a real terminus.
//
// Let's see if this residue has a residue on one side of it, but not
// the other.  If so, return N or C depending on whether the other
// residue is upstream or not.
// 
// Return not-terminal-residue if 0 neighbours, return "M" (for mid)
// for both neighbours present (used by
// graphics_info_t::execute_add_terminal_residue()).  Realise that
// this is a bit of a kludge, because usually, the terminal type
// refers to the residue that we clicked on not the missing residue.
// 
// In the "M" case, we refer to the missing residue.
//
// "singleton" is a possibile terminal type - for cases where this
// residue does not have neighbours.
//
// Note that this ignores altlocs and insertion codes. It should do
// altlocs at least.
// 
std::string
molecule_class_info_t::get_term_type(int atom_index) {

   std::string term_type = "not-terminal-residue"; // returned thing

   // These are the clicked atom's parameters:
   //
   // char *chainid = atom_sel.atom_selection[atom_index]->GetChainID();
   int ires_atom = atom_sel.atom_selection[atom_index]->GetSeqNum();
   PCChain chain = atom_sel.atom_selection[atom_index]->GetChain();
   int nres = chain->GetNumberOfResidues();

   // including tests needed for single missing residue:
   short int has_up_neighb = 0;
   short int has_down_neighb = 0;
   short int has_up_up_neighb = 0;
   short int has_down_down_neighb = 0;
   
   for (int ires=0; ires<nres; ires++) { 
      PCResidue res = chain->GetResidue(ires);
      if (res) { // could have been deleted (NULL)
	 if (res->GetSeqNum() == (ires_atom + 1))
	    has_up_neighb = 1;
	 if (res->GetSeqNum() == (ires_atom + 2))
	    has_up_up_neighb = 1;
	 if (res->GetSeqNum() == (ires_atom - 1))
	    has_down_neighb = 1;
	 if (res->GetSeqNum() == (ires_atom - 2))
	    has_down_down_neighb = 1;
      }
   }

   if ( (has_up_neighb + has_down_neighb) == 1 ) {
      if (has_up_neighb)
	 term_type = "N";
      if (has_down_neighb)
	 term_type = "C";
   }

   if ((has_up_neighb == 0) && (has_down_neighb == 0))
      term_type = "singleton";

   // Now test for missing single residue, "M" (mid):
   // 
   if ( (!has_up_neighb) && has_up_up_neighb)
      term_type = "MC"; // missing middle res, treat as C term

   if ( (!has_down_neighb) && has_down_down_neighb)
      term_type = "MN"; // missing middle res, treat as N term

   // std::cout << "DEBUG:: get_term_type Returning residue type " << term_type << std::endl;
   
   return term_type;
} 


// Replace the atoms in this molecule by those in the given atom selection.
int
molecule_class_info_t::replace_fragment(atom_selection_container_t asc) {

   replace_coords(asc, 0);  // or should that be 1?
   return 1;
}


// Delete residue: the whole residue.  We decide outside if this
// function or delete_residue_with_altconf should be used.
//
// Return the success status (0: failure to delete, 1 is deleted)
//
// regenerate the graphical bonds box if necessary.
// 
short int
molecule_class_info_t::delete_residue(const std::string &chain_id, int resno, const std::string &ins_code) {

   short int was_deleted = 0;
   CChain *chain;

   // run over chains of the existing mol
   int nchains = atom_sel.mol->GetNumberOfChains(1);
   for (int ichain=0; ichain<nchains; ichain++) {
	 
      chain = atom_sel.mol->GetChain(1,ichain);
      std::string mol_chain_id(chain->GetChainID());
      
      if (chain_id == mol_chain_id) {

	 int nres = chain->GetNumberOfResidues();
	 for (int ires=0; ires<nres; ires++) { 
	    PCResidue res = chain->GetResidue(ires);
	    if (res) {
	       if (res->GetSeqNum() == resno) {
		  
		  // so we have a matching residue:
		  int iseqno = res->GetSeqNum();
		  pstr inscode = res->GetInsCode();
                  std::string inscodestr(inscode);
		  if (ins_code == inscodestr) { 
		     make_backup();
		     atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
		     delete_ghost_selections();
		     chain->DeleteResidue(iseqno, inscode);
		     was_deleted = 1;
		     res = NULL;
		     break;
                  }
	       }
	    }
	 }
      }
      if (was_deleted)
	 break;
   }
   // potentially
   if (was_deleted) {

      // we can't do this after the modification: it has to be done before
      // atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
      
      atom_sel.atom_selection = NULL;
      atom_sel.mol->FinishStructEdit();
      atom_sel = make_asc(atom_sel.mol);
      have_unsaved_changes_flag = 1; 
      make_bonds_type_checked(); // calls update_ghosts()
      //
      trim_atom_label_table();
      update_symmetry();
   }
   return was_deleted;
}

short int
molecule_class_info_t::delete_residue_hydrogens(const std::string &chain_id, int resno,
                                                const std::string &ins_code,
						const std::string &altloc) {

   short int was_deleted = 0;
   CChain *chain;

   // run over chains of the existing mol
   int nchains = atom_sel.mol->GetNumberOfChains(1);
   for (int ichain=0; ichain<nchains; ichain++) {
	 
      chain = atom_sel.mol->GetChain(1,ichain);
      std::string mol_chain_id(chain->GetChainID());
      
      if (chain_id == mol_chain_id) {

	 int nres = chain->GetNumberOfResidues();
	 for (int ires=0; ires<nres; ires++) { 
	    PCResidue res = chain->GetResidue(ires);
	    if (res) {
	       if (res->GetSeqNum() == resno) {

		  // so we have a matching residue:
		  make_backup();
		  atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
		  delete_ghost_selections();
		  was_deleted = 1; // This is moved here because we
				   // don't want to DeleteSelection on
				   // multiple atom delete, so we set
				   // it before the delete actually
				   // happens - it does mean that in
				   // the pathological case
				   // was_deleted is set but no atoms
				   // get deleted - which is harmless
				   // (I think).

		  // getatom table here and check the elements in the lst

		  PPCAtom residue_atoms;
		  int nResidueAtoms;

		  res->GetAtomTable(residue_atoms, nResidueAtoms);
		  if (nResidueAtoms == 0) {
		     std::cout << "WARNING:: No atoms in residue (strange!)\n";
		  } else {
		     for (int i=0; i<nResidueAtoms; i++) {
			std::string element(residue_atoms[i]->element);
			if (element == " H" || element == " D") {
			   res->DeleteAtom(i);
			   // was_deleted = 1; // done upstairs
			}
		     }
		     if (was_deleted)
			res->TrimAtomTable();
		  } 
		  break;
	       }
	    }
	 }
      }
      if (was_deleted)
	 break;
   }

   // potentially
   if (was_deleted) {
      atom_sel.atom_selection = NULL;
      atom_sel.mol->FinishStructEdit();
      atom_sel = make_asc(atom_sel.mol);
      have_unsaved_changes_flag = 1; 
      make_bonds_type_checked();
      //
      trim_atom_label_table();
      update_symmetry();
   }
   return was_deleted;
} 


// Delete only the atoms of the residue that have the same altconf (as
// the selected atom).  If the selected atom has altconf "", you
// should call simply delete_residue().
// 
// Return 1 if at least one atom was deleted, else 0.
// 
short int
molecule_class_info_t::delete_residue_with_altconf(const std::string &chain_id,
						   int resno,
				                   const std::string &ins_code,
						   const std::string &altconf) {

   short int was_deleted = 0;
   CChain *chain;
   CResidue *residue_for_deletion = NULL;
   std::vector<std::pair<std::string, float> > deleted_atom;

   // run over chains of the existing mol
   int nchains = atom_sel.mol->GetNumberOfChains(1);
   for (int ichain=0; ichain<nchains; ichain++) {
	 
      chain = atom_sel.mol->GetChain(1,ichain);
      std::string mol_chain_id(chain->GetChainID());
      
      if (chain_id == mol_chain_id) {

	 int nres = chain->GetNumberOfResidues();
	 for (int ires=0; ires<nres; ires++) { 
	    PCResidue res = chain->GetResidue(ires);
	    if (res) { 
	       if (res->GetSeqNum() == resno) {

                  pstr inscode_res = res->GetInsCode();
                  std::string inscode_res_str(inscode_res);

		  if (inscode_res_str == ins_code) { 
		  
		     // so we have a matching residue:
		     residue_for_deletion = res;
		     make_backup();
		     atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
		     delete_ghost_selections();
   
		     // delete the specific atoms of the residue:
		     PPCAtom atoms;
		     int n_atoms;
		     res->GetAtomTable(atoms, n_atoms);
		     for (int i=0; i<n_atoms; i++) {
		        if (std::string(atoms[i]->altLoc) == altconf) {
			   std::pair<std::string, float> p(atoms[i]->name,
							   atoms[i]->occupancy);
			   deleted_atom.push_back(p);
			   res->DeleteAtom(i);
			   was_deleted = 1;
		        }
		     }
                  }
		  break;
	       }
	    }
	 }
      }
      if (was_deleted)
	 break;
   }
   // potentially (usually, I imagine)
   if (was_deleted) {
      atom_sel.atom_selection = NULL;
      atom_sel.mol->FinishStructEdit();
      trim_atom_label_table();
      unalt_conf_residue_atoms(residue_for_deletion);
      atom_sel.mol->FinishStructEdit();

      // Now, add the occpancy of the delete atom to the remaining
      // atoms of that residue with that atom name (if there are any)
      // 
      if (residue_for_deletion) {
	 if (deleted_atom.size() > 0) {
	    if (! is_from_shelx_ins_flag) {
	       PPCAtom atoms = NULL;
	       int n_atoms;
	       residue_for_deletion->GetAtomTable(atoms, n_atoms);
	       for (unsigned int idat=0; idat<deleted_atom.size(); idat++) {
		  std::vector<CAtom *> same_name;
		  for (int iresat=0; iresat<n_atoms; iresat++) {
		     if (deleted_atom[idat].first == std::string(atoms[iresat]->name)) {
			same_name.push_back(atoms[iresat]);
		     }
		  }
		  if (same_name.size() > 0) {
		     float extra_occ = deleted_atom[idat].second / float(same_name.size());
		     for (unsigned int isn=0; isn<same_name.size(); isn++) {
			// std::cout << "Adding " << extra_occ << " to "
			// << same_name[isn]->occupancy
			// << " for atom " << same_name[isn]->name << std::endl;
			same_name[isn]->occupancy += extra_occ;
		     }
		  }
	       }
	    } else {
	       // was a shelx molecule, if we are down to one alt conf
	       // now, then we set the occupancy to 11.0, otherwise
	       // leave it as it is.
	       PPCAtom atoms = NULL;
	       int n_atoms;
	       residue_for_deletion->GetAtomTable(atoms, n_atoms);
	       for (unsigned int idat=0; idat<deleted_atom.size(); idat++) {
		  std::vector<CAtom *> same_name;
		  for (int iresat=0; iresat<n_atoms; iresat++) {
		     if (deleted_atom[idat].first == std::string(atoms[iresat]->name)) {
			same_name.push_back(atoms[iresat]);
		     }
		  }
		  if (same_name.size() == 1) {
		     same_name[0]->occupancy = 11.0;
		  }
	       }
	    } 
	 }
      }
      
      atom_sel = make_asc(atom_sel.mol);
      have_unsaved_changes_flag = 1; 
      make_bonds_type_checked();
      update_symmetry();
   }
   return was_deleted;
}

short int
molecule_class_info_t::delete_residue_sidechain(const std::string &chain_id,
						int resno,
						const std::string &inscode) {


   short int was_deleted = 0;
   CChain *chain;
   CResidue *residue_for_deletion = NULL;
   
   // run over chains of the existing mol
   int nchains = atom_sel.mol->GetNumberOfChains(1);
   for (int ichain=0; ichain<nchains; ichain++) {
      
      chain = atom_sel.mol->GetChain(1,ichain);
      std::string mol_chain_id(chain->GetChainID());
      
      if (chain_id == mol_chain_id) {
	 
	 int nres = chain->GetNumberOfResidues();
	 for (int ires=0; ires<nres; ires++) { 
	    PCResidue res = chain->GetResidue(ires);
	    if (res) { 
	       if (res->GetSeqNum() == resno) {

                  pstr inscode_res = res->GetInsCode();
                  std::string inscode_res_str(inscode_res);

		  if (inscode_res_str == inscode) { 
		  
		     // so we have a matching residue:
		     residue_for_deletion = res;
		     make_backup();
		     atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
		     delete_ghost_selections();
		     was_deleted = 1; // need to set this here because
				      // we need to regenerate the
				      // atom selection.
		     // delete the specific atoms of the residue:
		     PPCAtom atoms;
		     int n_atoms;
		     res->GetAtomTable(atoms, n_atoms);
		     for (int i=0; i<n_atoms; i++) {
			if (! (coot::is_main_chain_or_cb_p(atoms[i]))) {
			   res->DeleteAtom(i);
			}
		     }
		     if (was_deleted)
			res->TrimAtomTable();
		  }
	       }
	    }
	 }
      }
   }
   // potentially (usually, I imagine)
   if (was_deleted) {
      atom_sel.atom_selection = NULL;
      atom_sel.mol->FinishStructEdit();
      atom_sel.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
      atom_sel = make_asc(atom_sel.mol);
      trim_atom_label_table();
      unalt_conf_residue_atoms(residue_for_deletion);
      atom_sel.mol->FinishStructEdit();
      have_unsaved_changes_flag = 1; 
      make_bonds_type_checked(); // calls update_ghosts()
   }
   return was_deleted;
} 



int
molecule_class_info_t::delete_zone(const coot::residue_spec_t &res1,
				   const coot::residue_spec_t &res2) {

   int first_res = res1.resno;
   int last_res  = res2.resno;
   // std::string alt_conf = res1.altconf;  // FIXME
   std::string alt_conf = "";
   std::string inscode = ""; // FIXME, more cleverness required.   
                             // A range of values?

   if (first_res > last_res) {
      int tmp = last_res;
      last_res = first_res;
      first_res = tmp;
   }

   make_backup();
   // temporarily turn off backups when we delete this range:
   // 
   int tmp_backup_this_molecule = backup_this_molecule;
   backup_this_molecule = 0;
   for (int i=first_res; i<=last_res; i++)
      // delete_residue_with_altconf(res1.chain, i, inscode, alt_conf);
      delete_residue(res1.chain, i, inscode);
   backup_this_molecule = tmp_backup_this_molecule; // restore state

   // bonds, have_unsaved_changes_flag etc dealt with by
   // delete_residue_with_altconf().
   return 0;
}


void
molecule_class_info_t::unalt_conf_residue_atoms(CResidue *residue_p) { 

   // We had a crash here (delete-residue-range), which is why there
   // is now more protection than you might imagine 20051231.
   
   if (residue_p) { 
      PPCAtom atoms;
      int n_atoms;
      residue_p->GetAtomTable(atoms, n_atoms);
      std::cout << "There are " << n_atoms << " atoms in "
		<< residue_p->GetChainID() << " " << residue_p->GetSeqNum()
		<< std::endl;
      for (int i=0; i<n_atoms; i++) {
	 std::string this_atom_name(atoms[i]->name);
	 int n_match = 0;
	 for (int j=0; j<n_atoms; j++) {
	    if (atoms[j]) {
	       if (atoms[j]->name) {
		  std::string inner_name(atoms[j]->name);
		  if (inner_name == this_atom_name) { 
		     n_match++;
		  }
	       } else {
		  std::cout << "ERROR:: null atom name in unalt_conf" << std::endl;
	       }
	    } else {
	       std::cout << "ERROR:: null atom in unalt_conf" << std::endl;
	    } 
	 }
	 if (n_match == 1) { 
	    if (std::string(atoms[i]->altLoc) != "") { 
	       std::string new_alt_conf("");
	       // force it down the atom's throat :) c.f. insert_coords_change_altconf
	       strncpy(atoms[i]->altLoc, new_alt_conf.c_str(), 2);
	    }
	 }
      }
   } 
} 


short int
molecule_class_info_t::delete_atom(const std::string &chain_id,
				   int resno,
				   const std::string &ins_code,
				   const std::string &atname,
				   const std::string &altconf) {

   short int was_deleted = 0;
   CChain *chain;
   CResidue *residue_of_deleted_atom = NULL;

   // run over chains of the existing mol
   int nchains = atom_sel.mol->GetNumberOfChains(1);
   for (int ichain=0; ichain<nchains; ichain++) {
	 
      chain = atom_sel.mol->GetChain(1,ichain);
      std::string mol_chain_id(chain->GetChainID());
      
      if (chain_id == mol_chain_id) {

	 int nres = chain->GetNumberOfResidues();
	 for (int ires=0; ires<nres; ires++) { 
	    PCResidue res = chain->GetResidue(ires);
	    if (res) { 
	       if (res->GetSeqNum() == resno) {

		  if (res->GetInsCode() == ins_code) { 
		  
		     // so we have a matching residue:

		     PPCAtom residue_atoms;
		     int nResidueAtoms;
		     std::string mol_atom_name;
		     res->GetAtomTable(residue_atoms, nResidueAtoms);
		     for (int iat=0; iat<nResidueAtoms; iat++) {
			
			mol_atom_name = residue_atoms[iat]->name;
			if (atname == mol_atom_name) { 
			
			   if (std::string(residue_atoms[iat]->altLoc) == altconf) {

			      make_backup();
			      atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
			      delete_ghost_selections();
			      res->DeleteAtom(iat);
			      was_deleted = 1;
			      residue_of_deleted_atom = res;
			      break;
			   }
			}
		     }
		  }
	       }
	    }
	    if (was_deleted) 
	       break;
	 }
      }
      if (was_deleted)
	 break;
   }

   // potentially
   if (was_deleted) { 
      atom_sel.mol->FinishStructEdit();

      //
      PPCAtom atoms = NULL;
      int n_atoms;
      CAtom *at = 0;
      int n_matching_name = 0;
      residue_of_deleted_atom->GetAtomTable(atoms, n_atoms);
      for (int iat=0; iat<n_atoms; iat++) {
	 std::string res_atom_name = atoms[iat]->name;
	 if (res_atom_name == atname) {
	    at = atoms[iat];
	    n_matching_name++;
	 }
      }
      if (n_matching_name == 1) { // one atom of this name left in the residue, so
          	                  // remove its altconf string
	 strncpy(at->altLoc, "", 2);
	 // set the occupancy to 1.0 of the remaining atom if it was not zero.
	 if (at) 
	    if (at->occupancy > 0.009)
	       at->occupancy = 1.0;
      }

      
      atom_sel = make_asc(atom_sel.mol);
      make_bonds_type_checked();
      have_unsaved_changes_flag = 1;
      // unlikely to be necessary:
      trim_atom_label_table();
      update_symmetry();
   }
   return was_deleted;

   
}


int
molecule_class_info_t::delete_atoms(const std::vector<coot::atom_spec_t> &atom_specs) {

   short int was_deleted = 0;
   int n_deleleted_atoms = 0;

   if (atom_sel.n_selected_atoms > 0) {
      if (atom_specs.size() > 0) 
	 make_backup();
      for (unsigned int i=0; i<atom_specs.size(); i++) {
	 int SelHnd = atom_sel.mol->NewSelection();
	 // how about a function that calls this:
	 // select_atomspec_atoms(atom_sel.mol, atom_specs[i])
	 // or  a member function of an atom_spec_t:
	 //    atom_specs[i].select_atoms(mol)
	 //
	 PCAtom *atoms = NULL;
	 int n_atoms;
	 atom_sel.mol->SelectAtoms(SelHnd, 0, (char *) atom_specs[i].chain.c_str(),
				   atom_specs[i].resno, (char *) atom_specs[i].insertion_code.c_str(),
				   atom_specs[i].resno, (char *) atom_specs[i].insertion_code.c_str(),
				   "*",
				   (char *) atom_specs[i].atom_name.c_str(),
				   "*",
				   (char *) atom_specs[i].alt_conf.c_str()
				   );
	 atom_sel.mol->GetSelIndex(SelHnd, atoms, n_atoms);
	 if (n_atoms) {
	    delete atoms[0];
	    atoms[0] = NULL;
	    n_deleleted_atoms++;
	    was_deleted = 1;
	 }
	 atom_sel.mol->DeleteSelection(SelHnd);
      }
   }

   // potentially
   if (was_deleted) { 
      atom_sel.mol->FinishStructEdit();
      atom_sel = make_asc(atom_sel.mol);
      make_bonds_type_checked();
      have_unsaved_changes_flag = 1;
      // unlikely to be necessary:
      trim_atom_label_table();
      update_symmetry();
   }

   return n_deleleted_atoms;
}


// best fit rotamer stuff
float
molecule_class_info_t::auto_fit_best_rotamer(int resno,
					     const std::string &altloc,
					     const std::string &insertion_code, 
					     const std::string &chain_id, int imol_map,
					     int clash_flag, float lowest_prob) {

   // First we will get the CResidue for the residue that we are
   // trying to fit.
   // 
   // For each rotamer of that residue: (probabilities.size() is the
   // number of rotamers for a residue of that type)
   //
   //    Rigid body refine rotamer
   //
   //    score by density fit to map imol_map
   //
   //

   // First check that imol_map has a map.
   short int have_map_flag = 1;
   if (imol_map < 0)
      have_map_flag = 0;

   float f = -99.9;
   float clash_score_limit = 20.0; // Rotamers must have a clash score
				   // less than this (if clashes are
				   // tested).
   

   CResidue *res = get_residue(resno, std::string(insertion_code), std::string(chain_id));

   if (res) {
      std::cout << " ==== fitting residue " << resno << " " << chain_id;
      if (have_map_flag)
	 std::cout << " to map number " << imol_map << " ======" << std::endl;
      else
	 std::cout << " without a map =====" << std::endl;

      if (coot::util::residue_has_hydrogens_p(res))
	 clash_score_limit = 500; // be more generous... lots of hydrogen contacts
      
      // std::cout << "found residue" << std::endl;
      std::string res_type(res->name);
      CResidue *copied_res = coot::deep_copy_this_residue(res, altloc, 0,
							  atom_sel.UDDAtomIndexHandle);
#ifdef USE_DUNBRACK_ROTAMERS			
      coot::dunbrack d(copied_res, atom_sel.mol, lowest_prob, 0);
#else			
      coot::richardson_rotamer d(copied_res, atom_sel.mol, lowest_prob, 0);
#endif // USE_DUNBRACK_ROTAMERS
      
      std::vector<float> probabilities = d.probabilities();
//       std::cout << "debug afbr probabilities.size() " << probabilities.size()
// 		<< " " << have_map_flag << std::endl;
      if (probabilities.size() == 0) {
	 std::cout << "WARNING:: no rotamers probabilityes for residue type "
		   << res_type << std::endl;
      } else { 
	 CResidue *rotamer_res;
	 double best_score = -99.9;
	 coot::minimol::molecule best_rotamer_mol;
	 if (have_map_flag) { 
	    for (unsigned int i=0; i<probabilities.size(); i++) {
	       // std::cout << "--- Rotamer number " << i << " ------"  << std::endl;
	       rotamer_res = d.GetResidue(i);
	       // 	 std::cout << " atom info: " << std::endl;
	       // 	 int nResidueAtoms;
	       // 	 PPCAtom residue_atoms;
	       // 	 rotamer_res->GetAtomTable(residue_atoms, nResidueAtoms);
	       // 	 for (int iat=0; iat<nResidueAtoms; iat++) {
	       // 	    std::cout << residue_atoms[iat] << std::endl;
	       // 	 }

	       // first make a minimol molecule for the residue so that we
	       // can install it into lig.
	       //
	       coot::minimol::residue  residue_res(rotamer_res);
	       coot::minimol::molecule residue_mol;
	       coot::minimol::fragment frag;
	       int ifrag = residue_mol.fragment_for_chain(chain_id);
	       residue_mol[ifrag].addresidue(residue_res, 0);

	       // now do "ligand" fitting setup and run:
	       coot::ligand lig;
	       lig.import_map_from(graphics_info_t::molecules[imol_map].xmap_list[0]);
	       // short int mask_water_flag = 0;
	       lig.set_acceptable_fit_fraction(0.5);  // at least half of the atoms
	       // have to be fitted into
	       // positive density, otherwise
	       // the fit failed, and we leave
	       // the atom positions as they
	       // are (presumably in an even
	       // worse position?)
	       lig.install_ligand(residue_mol);
	       lig.set_dont_write_solutions();
	       lig.find_centre_by_ligand(0);
	       lig.set_dont_test_rotations();
	       lig.fit_ligands_to_clusters(1);
	       // so we have the solution from lig, what was its score?
	       //
	       coot::ligand_score_card score_card = lig.get_solution_score(0);
	       coot::minimol::molecule moved_mol = lig.get_solution(0);
	       float clash_score = 0.0;
	       // std::cout << "debug INFO:: density score: " << score_card.score << "\n";
	       if (clash_flag) { 
		  clash_score = get_clash_score(moved_mol); // clash on atom_sel.mol
		  // std::cout << "INFO:: clash score: " << clash_score << "\n";
	       }
	       if (clash_score < clash_score_limit) { // This value may
						    // need to be
						    // exported to the
						    // user interface
		  if (score_card.score > best_score) {
		     best_score = score_card.score;
		     best_rotamer_mol = moved_mol;
		  }
	       }
	    }
	 } else {
	    // we don't have a map, like KD suggests:
	    float clash_score;
	    best_score = -99.9; // clash score are better when lower,
				// but we should stay consistent with
				// the map based scoring which is the
				// other way round.
	    for (unsigned int i=0; i<probabilities.size(); i++) {
	       // std::cout << "--- Rotamer number " << i << " ------"  << std::endl;
	       // std::cout << "Getting rotamered residue... " << std::endl;
	       rotamer_res = d.GetResidue(i);
	       // std::cout << "Got rotamered residue... " << std::endl;
	       coot::minimol::residue  residue_res(rotamer_res);
	       coot::minimol::molecule residue_mol;
	       int ifrag = residue_mol.fragment_for_chain(chain_id);
	       residue_mol[ifrag].addresidue(residue_res, 0);
	       coot::minimol::molecule moved_mol = residue_mol;
	       // std::cout << "Getting clash score... " << std::endl;
	       clash_score = -get_clash_score(moved_mol); // clash on atom_sel.mol
	       // std::cout << "INFO:: clash score: " << clash_score << "\n";
	       if (clash_score > best_score) {
		  best_score = clash_score;
		  best_rotamer_mol = moved_mol;
	       }
	    }
	 }

	 // now we have tested each rotamer:
	 if (best_score > -99.9) {
	    // replace current atoms by the atoms in best_rotamer_mol
	    //
	    // we create a mol and then an asc and then use
	    // replace_coords method:
	    //
	    float bf = graphics_info_t::default_new_atoms_b_factor;
	    PCMMDBManager mol = best_rotamer_mol.pcmmdbmanager(bf);
	    atom_selection_container_t asc = make_asc(mol);
	    replace_coords(asc, 1); // fix other alt conf occ
	    f = best_score;
	 }

	 // ALAs and GLY's get to here without entering the loops
	 if (probabilities.size() == 0) { // ALAs and GLYs
	    f = 0.0;
	 }
      }
      
   } else {
      std::cout << "WARNING:: residue not found in molecule" << std::endl;
   } 

   return f;
}

// interface from atom picking:
float 
molecule_class_info_t::auto_fit_best_rotamer(int atom_index, int imol_map, int clash_flag,
					     float lowest_probability) { 

   int resno = atom_sel.atom_selection[atom_index]->GetSeqNum();
   std::string insertion_code = atom_sel.atom_selection[atom_index]->GetInsCode();
   std::string chain_id = atom_sel.atom_selection[atom_index]->GetChainID();
   std::string altloc(atom_sel.atom_selection[atom_index]->altLoc);

   return auto_fit_best_rotamer(resno, altloc, insertion_code, chain_id, imol_map, clash_flag, lowest_probability);

} 


// Return NULL on residue not found in this molecule.
// 
CResidue *
molecule_class_info_t::get_residue(int reso, const std::string &insertion_code,
				   const std::string &chain_id) const {

   CResidue *res = NULL;
   short int found_res = 0;
      
   if (atom_sel.n_selected_atoms > 0) {
      CModel *model_p = atom_sel.mol->GetModel(1);
      CChain *chain_p;
      int n_chains = model_p->GetNumberOfChains(); 
      for (int i_chain=0; i_chain<n_chains; i_chain++) {
	 chain_p = model_p->GetChain(i_chain);
	 std::string mol_chain(chain_p->GetChainID());
	 if (mol_chain == chain_id) {
	    int nres = chain_p->GetNumberOfResidues();
	    CResidue *residue_p;
	    for (int ires=0; ires<nres; ires++) { // ires is a serial number
	       residue_p = chain_p->GetResidue(ires);
	       if (residue_p->GetSeqNum() == reso) {
		  std::string ins_code(residue_p->GetInsCode());
		  if (insertion_code == ins_code) {
		     res = residue_p;
		     found_res = 1;
		     break;
		  } else {
		     std::cout << "strange? Insertion codes non-match :"
			       << residue_p->GetInsCode() << ": :"
			       << insertion_code << ":" << std::endl;
		  }
	       }
	    }
	 }
	 if (found_res) break;
      }
   }
   return res;
}


CResidue *
molecule_class_info_t::residue_from_external(int resno, const std::string &insertion_code,
					     const std::string &chain_id) const {

   return get_residue(resno, insertion_code, chain_id);

} 


// pair.first is the status, 0 for bad, 1 for good.
// 
std::pair<short int, CAtom *>
molecule_class_info_t::baton_build_delete_last_residue() {

   // this is the baton molecule, we don't get here if we aren't.
   //
   // How do we find the last residue?  Simply it is the last chain in
   // the list of chains and the last residue in the list of residues.
   // 
   std::pair<short int, CAtom *> r;
   r.first = 0; // status 

   if (atom_sel.n_selected_atoms > 0) {
      CModel *model_p = atom_sel.mol->GetModel(1);
      CChain *chain_p;
      int n_chains = atom_sel.mol->GetNumberOfChains(1);
      chain_p = model_p->GetChain(n_chains-1);

      int nres = chain_p->GetNumberOfResidues();
      CResidue *residue_p = chain_p->GetResidue(nres-1); // is a serial number

      int iseqno = residue_p->GetSeqNum();
      pstr inscode = residue_p->GetInsCode();
      chain_p->DeleteResidue(iseqno, inscode);

      // Kevin's Baton Build crash: Argh.  I have no idea why.
      // atom_sel.SelectionHandle seems good and is generated by an
      // atom selection in make asc (when the latest Ca had been
      // placed).
      // 
//       if (atom_sel.SelectionHandle > 0) { 
// 	 atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle); // prefered.
//       }

      atom_sel.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
      atom_sel.mol->FinishStructEdit();
      atom_sel = make_asc(atom_sel.mol);
      have_unsaved_changes_flag = 1; 
      make_ca_bonds(2.4, 4.7);

      if (atom_sel.n_selected_atoms > 0 ) { // atom sel was modified.
	 residue_p = chain_p->GetResidue(nres-2);
	 CAtom *atom_p = residue_p->GetAtom(" CA ");
	 if (atom_p) {
	    r.first = 1; // OK
	    r.second = atom_p;
	 }
      } 
   }
   return r;
}

std::pair<short int, clipper::Coord_grid>
molecule_class_info_t::search_for_skeleton_near(const coot::Cartesian &pos) const {

   // std::pair<short int, clipper::Coord_grid> r;
   clipper::Coord_orth co(pos.x(), pos.y(), pos.z());

   coot::CalphaBuild cab;
   std::pair<short int, clipper::Coord_grid> r =
      cab.search_for_skeleton_near(co, xskel_cowtan, skeleton_treenodemap);

   return r;
} 

short int
molecule_class_info_t::add_OXT_to_residue(int reso, const std::string &insertion_code,
					  const std::string &chain_id) {

   CResidue *residue = get_residue(reso, insertion_code, chain_id);
   return add_OXT_to_residue(residue);  // check for null residue

} 

short int
molecule_class_info_t::add_OXT_to_residue(CResidue *residue) {

   PPCAtom residue_atoms;
   int nResidueAtoms;
   short int istatus = 0; // fail
   if (!residue) {
      std::cout << "WARNING: NULL residue, no atom added." << std::endl;
   } else { 
      residue->GetAtomTable(residue_atoms, nResidueAtoms);
      if (nResidueAtoms == 0) {
	 std::cout << "WARNING: no atoms in this residue" << std::endl;
      } else {

	 CAtom *n_atom = NULL;  
	 CAtom *c_atom = NULL; 
	 CAtom *ca_atom = NULL; 
	 CAtom *o_atom = NULL;

	 clipper::Coord_orth co; 
	 clipper::Coord_orth c_atom_co; 
	 clipper::Coord_orth o_atom_co; 
	 clipper::Coord_orth n_atom_co; 
	 clipper::Coord_orth ca_atom_co;
	 int n_found_atoms = 0;
      
	 CAtom *atom; 
	 atom = residue->GetAtom(" N  ");
	 if (atom) { 
	    n_found_atoms++;
	    n_atom = atom;
	    n_atom_co = clipper::Coord_orth(atom->x, atom->y, atom->z);
	 } 
	 atom = residue->GetAtom(" O  ");
	 if (atom) { 
	    n_found_atoms++;
	    o_atom = atom;
	    o_atom_co = clipper::Coord_orth(atom->x, atom->y, atom->z);
	 } 
	 atom = residue->GetAtom(" CA ");
	 if (atom) { 
	    n_found_atoms++;
	    ca_atom = atom;
	    ca_atom_co = clipper::Coord_orth(atom->x, atom->y, atom->z);
	 } 
	 atom = residue->GetAtom(" C  ");
	 if (atom) { 
	    n_found_atoms++;
	    c_atom = atom;
	    c_atom_co = clipper::Coord_orth(atom->x, atom->y, atom->z);
	 }

	 if (! (n_atom && c_atom && ca_atom && o_atom) ) {
	    std::cout << "WARNING:: Not all reference atoms found in residue."
		      << std::endl;
	    std::cout << "          No atom fitted." << std::endl;
	    std::string m("WARNING:: Not all reference atoms found in residue\n");
	    m += "          No OXT atom fitted.";
	    GtkWidget *w = graphics_info_t::wrapped_nothing_bad_dialog(m);
	    gtk_widget_show(w);
	 } else {
	    make_backup();
	    double torsion = clipper::Coord_orth::torsion(n_atom_co, ca_atom_co, c_atom_co, o_atom_co);
	    double angle = clipper::Util::d2rad(120.8);
	    clipper::Coord_orth new_oxt(n_atom_co, ca_atom_co, c_atom_co,
					1.231, angle, torsion + M_PI);
	    CAtom *new_oxt_atom = new CAtom;
	    new_oxt_atom->SetCoordinates(new_oxt.x(),
					 new_oxt.y(),
					 new_oxt.z(), 1.0,
					 graphics_info_t::default_new_atoms_b_factor);
	    new_oxt_atom->SetAtomName(" OXT");
	    new_oxt_atom->SetElementName(" O");
	    residue->AddAtom(new_oxt_atom);

	    atom_sel.mol->FinishStructEdit();
	    atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
	    atom_sel = make_asc(atom_sel.mol);
	    have_unsaved_changes_flag = 1;
	    makebonds(); // not type checked, so that we can see the atom.
	    istatus = 1;
	    std::cout << "Added OXT at " << new_oxt_atom << std::endl;
	 } 
      }
   }
   return istatus;
} 


int
molecule_class_info_t::residue_serial_number(int resno, const std::string &insertion_code,
					     const std::string &chain_id) const {

   int iserial = -1;
   CResidue *res = get_residue(resno, insertion_code, chain_id);
   if (res) {
      std::cout << "DEBUG:: residue_serial_number residue " << resno << " found " << std::endl;
      iserial = res->index;
   } else {
      std::cout << "WARNING:: residue" << resno << " " << insertion_code
		<< " " << chain_id << " not found" << std::endl;
   }
   return iserial;
}


void
molecule_class_info_t::apply_atom_edit(const coot::select_atom_info &sai) { 
   
   // first select the atom
   PPCAtom SelAtoms = NULL;
   int nSelAtoms;
   int SelHnd = atom_sel.mol->NewSelection();
   
   atom_sel.mol->SelectAtoms(SelHnd, 0, (char *) sai.chain_id.c_str(),
			     sai.residue_number, (char *) sai.insertion_code.c_str(),
			     sai.residue_number, (char *) sai.insertion_code.c_str(),
			     "*", // residue name
			     (char *) sai.atom_name.c_str(),
			     "*", // elements
			     (char *) sai.altconf.c_str()); // alt locs

   atom_sel.mol->GetSelIndex(SelHnd, SelAtoms, nSelAtoms);

   if (nSelAtoms == 0) { 
      std::cout << "Sorry. Could not find " << sai.atom_name << "," 
		<< sai.altconf << "/"
		<< sai.residue_number << sai.insertion_code << "/" << sai.chain_id 
		<< " in this molecule: ("
		<<  imol_no << ") " << name_ << std::endl; 
   } else {

      if (nSelAtoms > 1) {
	 std::cout << "Unexepected condition in apply_atom_edit: many atoms! "
		   << nSelAtoms << std::endl;
      } else { 
	 CAtom *atom_p = SelAtoms[0];
      
	 if (sai.has_b_factor_edit())
	    atom_p->tempFactor = sai.b_factor;
	 if (sai.has_occ_edit())
	    atom_p->occupancy = sai.occ;
      }
   }
}

void
molecule_class_info_t::apply_atom_edits(const std::vector<coot::select_atom_info> &saiv) {

   short int made_edit = 0;
   make_backup();

   for (unsigned int i=0; i<saiv.size(); i++) {
      CAtom *at = saiv[i].get_atom(atom_sel.mol);
      if (at) {
	 if (saiv[i].has_b_factor_edit()) {
	    at->tempFactor = saiv[i].b_factor;
	    made_edit = 1;
	 }
	 if (saiv[i].has_occ_edit()) {
	    at->occupancy = saiv[i].occ;
	    made_edit = 1;
	 }
      }
   }

   if (made_edit) {
      have_unsaved_changes_flag = 1;
      make_bonds_type_checked();
   }
}



// For edit phi psi, tinker with the passed (moving atoms) asc
// Return status, -1 on error (unable to change).
short int 
molecule_class_info_t::residue_edit_phi_psi(atom_selection_container_t residue_asc, 
					    int atom_index, double phi, double psi) { 

   // Don't forget we do this kind of setting positions by torsion value in 
   // ligand/residue_by_phi_psi (but recall that we use minimol there).
   // 

   CResidue *this_res = atom_sel.atom_selection[atom_index]->residue;

   // get the previous C and next N atoms.  If we can't find them then
   // we don't do any changes to the atom positions and return -1.
   
   // First get the residues:
   CResidue *next_res;
   CResidue *prev_res;

   int selHnd1 = atom_sel.mol->NewSelection();
   int selHnd2 = atom_sel.mol->NewSelection();
   PPCResidue SelResidues;
   int nSelResdues;

   atom_sel.mol->Select (selHnd1, STYPE_RESIDUE, 0, 
			 this_res->GetChainID(),
			 this_res->GetSeqNum() - 1, this_res->GetInsCode(),
			 this_res->GetSeqNum() - 1, this_res->GetInsCode(),
			 "*",
			 "*",
			 "*",
			 "*",
			 SKEY_NEW
			 );
   atom_sel.mol->GetSelIndex(selHnd1, SelResidues, nSelResdues); 
   if (nSelResdues < 1) { 
      std::cout << "Can't find previous residue" << std::endl;
      return -1; 
   } else { 
      prev_res = SelResidues[0];
   }
   atom_sel.mol->Select (selHnd2, STYPE_RESIDUE, 1,
			 this_res->GetChainID(),
			 this_res->GetSeqNum() + 1, this_res->GetInsCode(),
			 this_res->GetSeqNum() + 1, this_res->GetInsCode(),
			 "*",
			 "*",
			 "*",
			 "*",
			 SKEY_NEW
			 );
   atom_sel.mol->GetSelIndex(selHnd2, SelResidues, nSelResdues); 
   if (nSelResdues < 1) { 
      std::cout << "Can't find next residue" << std::endl;
      return -1; 
   } else { 
      next_res = SelResidues[0];
   }

   // So we have both residues if we have got here: 
   // 
   // Now to get the relavent atoms (recall that we need Next Ca
   // because the O of this residue lies in the peptide plane of the
   // next residue (and that plane is defined by (amongst other
   // things) the Ca position of the next residue).

   CAtom *next_N  = next_res->GetAtom(" N  ");
   CAtom *next_Ca = next_res->GetAtom(" CA ");
   CAtom *prev_C  = prev_res->GetAtom(" C  ");

   if (next_N == NULL) { 
      std::cout << "Can't find N in previous residue\n";
      return -1;
   } 
   if (next_Ca == NULL) { 
      std::cout << "Can't find CA in next residue\n";
      return -1;
   }
   if (prev_C == NULL) { 
      std::cout << "Can't find C in next residue\n";
      return -1;
   }

   // So we have next_N and prev_C if we have got here.

   // delete the selections
   // 
   atom_sel.mol->DeleteSelection(selHnd1);
   atom_sel.mol->DeleteSelection(selHnd2);


   // 
   CAtom *this_C  = this_res->GetAtom(" C  ");
   // CAtom *this_O  = this_res->GetAtom(" O  ");
   CAtom *this_N  = this_res->GetAtom(" N  ");
   CAtom *this_Ca = this_res->GetAtom(" CA ");

   if (this_C == NULL) { 
      std::cout << "Can't find C in this residue\n";
      return -1;
   } 
   if (this_C == NULL) { 
      std::cout << "Can't find O in this residue\n";
      return -1;
   } 
   if (this_N == NULL) { 
      std::cout << "Can't find N in this residue\n";
      return -1;
   } 
   if (this_Ca == NULL) { 
      std::cout << "Can't find Ca in this residue\n";
      return -1;
   }
   
   // OK, we we have all the atoms needed to calculate phi and psi if
   // we have got here.
   // 
   // Except, we don't want to calculate them, we want to set our
   // atoms.

   // We want to set C (then O) by phi and N by psi.
   // 
   // Convert to Coord_orth's

   clipper::Coord_orth cthis_C_orig  = to_coord_orth(this_C );
   // clipper::Coord_orth cthis_N  = to_coord_orth(this_N );
   clipper::Coord_orth cthis_Ca = to_coord_orth(this_Ca);
   clipper::Coord_orth cprev_C  = to_coord_orth(prev_C );
   clipper::Coord_orth cnext_N  = to_coord_orth(next_N );
   clipper::Coord_orth cnext_Ca = to_coord_orth(next_Ca);


   double angle, torsion;
   
   // we build up from the C terminal end (note that the new positions
   // for C and N are intertependent (because phi and psi depend on
   // the positions of the backbone atoms in this residue).
   // 
   // 

   // N
   angle   = clipper::Util::d2rad(111.200); // N-Ca-C
   torsion = clipper::Util::d2rad(psi);
   clipper::Coord_orth cthis_N(cnext_N, cthis_C_orig, cthis_Ca, 
			       1.458, angle,torsion);

   angle   = clipper::Util::d2rad(111.200);  // N-CA-C
   torsion = clipper::Util::d2rad(phi);
   clipper::Coord_orth cthis_C(cprev_C, cthis_N, cthis_Ca,
			       1.525, angle, torsion);  // CA-C bond

   // O
   angle   = clipper::Util::d2rad(123.0); // N-C-O
   torsion = clipper::Util::d2rad(0.0);
   clipper::Coord_orth cthis_O(cnext_Ca, cnext_N, cthis_C,
			       1.231, angle, torsion);
   
   
   // Now apply those changes to the residue_asc (the (white) moving atoms):
   // 
   int selHnd_moving = residue_asc.mol->NewSelection();

   residue_asc.mol->Select (selHnd_moving, STYPE_RESIDUE, 0, 
			    this_res->GetChainID(),
			    this_res->GetSeqNum(), this_res->GetInsCode(),
			    this_res->GetSeqNum(), this_res->GetInsCode(),
			    "*",
			    "*",
			    "*",
			    "*",
			    SKEY_NEW
			    );
   residue_asc.mol->GetSelIndex(selHnd_moving, SelResidues, nSelResdues); 

   if (nSelResdues != 1) { 
      std::cout<< "Can't find this moving residue (bizarrely enough)" << std::endl;
      return -1; 
   } else { 

      CAtom *moving_N = SelResidues[0]->GetAtom(" N  ");
      CAtom *moving_C = SelResidues[0]->GetAtom(" C  ");
      CAtom *moving_O = SelResidues[0]->GetAtom(" O  ");
      

      moving_N->x = cthis_N.x(); 
      moving_N->y = cthis_N.y(); 
      moving_N->z = cthis_N.z(); 
      
      moving_C->x = cthis_C.x(); 
      moving_C->y = cthis_C.y(); 
      moving_C->z = cthis_C.z(); 
      
      moving_O->x = cthis_O.x(); 
      moving_O->y = cthis_O.y(); 
      moving_O->z = cthis_O.z(); 

      residue_asc.mol->DeleteSelection(selHnd_moving);
   } 
   return 1;
} 

clipper::Coord_orth
molecule_class_info_t::to_coord_orth(CAtom *atom) const { 

   return clipper::Coord_orth(atom->x, atom->y, atom->z);
} 

// Return the residue thats for moving_atoms_asc as a molecule.
// 
atom_selection_container_t
molecule_class_info_t::edit_residue_pull_residue(int atom_index,
						 short int whole_residue_flag) { 
   
   CResidue *res = NULL;
   // short int found_res = 0;
   atom_selection_container_t r;
   r.n_selected_atoms = 0; // added 20050402 for Wall compilation
   r.atom_selection = 0;
   r.mol = 0;
   r.read_success = 0;

   res = atom_sel.atom_selection[atom_index]->residue;
   std::string altconf(atom_sel.atom_selection[atom_index]->altLoc);

   if (res) { 

      CResidue *ret_res = coot::deep_copy_this_residue(res, altconf, 
						       whole_residue_flag,
						       atom_sel.UDDAtomIndexHandle);

      MyCMMDBManager *MMDBManager = new MyCMMDBManager;
      CModel *model_p = new CModel;
      CChain *chain_p = new CChain;

      chain_p->SetChainID(res->GetChainID());
   
      model_p->AddChain(chain_p);
      MMDBManager->AddModel(model_p);
      chain_p->AddResidue(ret_res);
   
      atom_selection_container_t r = make_asc(MMDBManager);

      return r;
   }
   return r;
} 


// Return pair.first < -200 for error.
// 
std::pair<double, double> 
molecule_class_info_t::get_phi_psi(int atom_index) const { 

   std::pair<double, double> pp;
   pp.first = -999.9; // magic number for bad return values
   

   if (atom_sel.n_selected_atoms > 0) { 
      if (atom_index < atom_sel.n_selected_atoms) { 
	 CResidue *res = atom_sel.atom_selection[atom_index]->residue;
	 int this_resno = res->GetSeqNum();

	 PPCResidue SelResidues;
	 int nSelResdues;

	 int selHnd = atom_sel.mol->NewSelection();
	 atom_sel.mol->Select ( selHnd,STYPE_RESIDUE, 1, // .. TYPE, iModel
				res->GetChainID(), // Chain(s)
				res->GetSeqNum() - 1, "*",  // starting res
				res->GetSeqNum() + 1, "*",  // ending res
				"*",  // residue name
				"*",  // Residue must contain this atom name?
				"*",  // Residue must contain this Element?
				"*",  // altLocs
				SKEY_NEW // selection key
				);
	 atom_sel.mol->GetSelIndex(selHnd, SelResidues, nSelResdues); 
	 
	 if (nSelResdues == 3) { 

	    clipper::Coord_orth prev_C;
	    clipper::Coord_orth this_N;
	    clipper::Coord_orth this_Ca;
	    clipper::Coord_orth this_C;
	    clipper::Coord_orth next_N;
	    int n_atoms = 0;

	    for (int ires=0; ires<3; ires++) { 
	       if (SelResidues[ires]->GetSeqNum() == (this_resno-1)) { 
		  // previous residue
		  CAtom *atom = SelResidues[ires]->GetAtom(" C  ");
		  if (atom) { 
		     n_atoms++; 
		     prev_C = clipper::Coord_orth(atom->x, atom->y, atom->z);
		  }
	       } 
	       if (SelResidues[ires]->GetSeqNum() == this_resno) { 
		  // this residue
		  CAtom *atom;

		  atom = SelResidues[ires]->GetAtom(" N  ");
		  if (atom) { 
		     n_atoms++; 
		     this_N = clipper::Coord_orth(atom->x, atom->y, atom->z);
		  }
		  atom = SelResidues[ires]->GetAtom(" CA ");
		  if (atom) { 
		     n_atoms++; 
		     this_Ca = clipper::Coord_orth(atom->x, atom->y, atom->z);
		  }
		  atom = SelResidues[ires]->GetAtom(" C  ");
		  if (atom) { 
		     n_atoms++; 
		     this_C = clipper::Coord_orth(atom->x, atom->y, atom->z);
		  }
	       } 
	       if (SelResidues[ires]->GetSeqNum() == this_resno +1) { 
		  // next residue
		  CAtom *atom = SelResidues[ires]->GetAtom(" N  ");
		  if (atom) { 
		     n_atoms++; 
		     next_N = clipper::Coord_orth(atom->x, atom->y, atom->z);
		  }
	       } 
	    }

	    if (n_atoms == 5) { 
	       double phi = clipper::Util::rad2d(clipper::Coord_orth::torsion(prev_C, this_N, this_Ca, this_C));
	       double psi = clipper::Util::rad2d(clipper::Coord_orth::torsion(this_N, this_Ca, this_C, next_N));
	       pp.first = phi;
	       pp.second = psi;
	    } else { 
	       std::cout << "WARNING found " << n_atoms << " atoms (not 5) "
			 << "Can't get phi psi" << std::endl;
	    } 
	 } else { 
	    std::cout << "WARNING: found " << nSelResdues << " residues (not 3) "
		      << "Can't get phi psi" << std::endl;
	 } 
      }
   }

   return pp;
} 


// short int Have_modifications_p() const { return history_index > 0 ? 1 : 0;}

short int
molecule_class_info_t::Have_redoable_modifications_p() const {

   short int r = 0;
//     std::cout << "DEBUG:: redoable? history_index: " << history_index
//  	     << " max_history_index: "
//  	     << max_history_index << " " << name_ << std::endl;
   
   if (history_index < max_history_index) {
      // When there are 3 backups made and we are viewing molecule 2,
      // we don't want to restore from history_filename_vec[3]:
      // 
      if ( int(history_filename_vec.size()) > (history_index + 1)) { 
	 r = 1;
      }
   } else {
      r = 0;
   }
   return r; 
} 

// Do not be mislead this is only a flag, *not* the number of chi
// angles for this residue
int
molecule_class_info_t::N_chis(int atom_index) {

   CResidue *res_p = atom_sel.atom_selection[atom_index]->residue;
   int r;

   std::string resname(res_p->GetResName());
   graphics_info_t g;
   // we want to ask g->Geom_p() if it has torsions for residues of
   // this type.

   if ( (resname == "GLY") || (resname == "ALA") ) {
      r = 0;
   } else {
      if (g.Geom_p()->have_dictionary_for_residue_type(resname,
						       graphics_info_t::cif_dictionary_read_number)) {
	 std::vector <coot::dict_torsion_restraint_t> v =
	    g.Geom_p()->get_monomer_torsions_from_geometry(resname, 0);

	 if (v.size() > 0) 
	    r = v.size();
	 else
	    r = 0;
      } else {
	 r = 0;
      }

   }

   return r;

} 

// How are clashes scored?  Hmmm... well, I think no clashes at all should have a score
// of 0.0 (c.f. auto_best_fit_rotamer()).  I think a badly crashing residue should have a
// score of around 1000.  A single 2.0A crash will have a score of 16.7 and a 1.0A crash
// 66.7.
// 
float
molecule_class_info_t::get_clash_score(const coot::minimol::molecule &a_rotamer) const {

   float score = 0;
   float dist_crit = 2.7;

   // First, where is the middle of the rotamer residue atoms and what
   // is the mean and maximum distance of coordinates from that point?

   // double std_dev_residue_pos;
      
   std::pair<double, clipper::Coord_orth> rotamer_info = get_minimol_pos(a_rotamer);
   double max_dev_residue_pos = rotamer_info.first;
   clipper::Coord_orth mean_residue_pos = rotamer_info.second;
   if (rotamer_info.first < 0.0) {
      // there were no atoms then
      std::cout << "ERROR: clash score: there are no atoms in the residue" << std::endl;
   } else {

      // So now we know the centre of the residue and the maximum distance of one of its
      // atoms from the centre.  Now let's run over the atoms of the atom_sel.
      // 
      // When we find a distance between the middle of the residue and an atom_sel atom
      // that is less than (max_dev_residue_pos + distance), then we have found
      // potentially clashing atoms, so check for a clash of that atom_sel atom with all
      // the atoms of the residue.
      double d;
      double d_atom;
      float badness;
      for (int i=0; i<atom_sel.n_selected_atoms; i++) {
	 clipper::Coord_orth atom_sel_atom(atom_sel.atom_selection[i]->x,
					   atom_sel.atom_selection[i]->y,
					   atom_sel.atom_selection[i]->z);
	 d = clipper::Coord_orth::length(atom_sel_atom, mean_residue_pos);
	 if (d < (max_dev_residue_pos + dist_crit)) {
	    for (unsigned int ifrag=0; ifrag<a_rotamer.fragments.size(); ifrag++) {
	       for (int ires=a_rotamer[ifrag].min_res_no(); ires<=a_rotamer[ifrag].max_residue_number(); ires++) {
		  for (int iat=0; iat<a_rotamer[ifrag][ires].n_atoms(); iat++) {
		     d_atom = clipper::Coord_orth::length(a_rotamer[ifrag][ires][iat].pos,atom_sel_atom);
		     if (d_atom < dist_crit) {
			int atom_sel_atom_resno = atom_sel.atom_selection[i]->GetSeqNum();
			std::string atom_sel_atom_chain(atom_sel.atom_selection[i]->GetChainID());

			if (! ((ires == atom_sel_atom_resno) &&
			       (a_rotamer[ifrag].fragment_id == atom_sel_atom_chain)) ) {
			   if ( (a_rotamer[ifrag][ires][iat].name != " N  ") &&
				(a_rotamer[ifrag][ires][iat].name != " C  ") &&
				(a_rotamer[ifrag][ires][iat].name != " CA ") &&
				(a_rotamer[ifrag][ires][iat].name != " O  ") &&
				(a_rotamer[ifrag][ires][iat].name != " H  ") ) { 
			      badness = 100.0 * (1.0/d_atom - 1.0/dist_crit);
			      if (badness > 100.0)
				 badness = 100.0;
//  			      std::cout << "DEBUG:: adding clash badness " << badness
// 					<< " for atom "
//  					<< a_rotamer[ifrag][ires][iat].name << " with d_atom = "
//  					<< d_atom << " to sel atom "
// 					<< atom_sel.atom_selection[i] << std::endl;
			      score += badness;
			   }
			}
		     }
		  }
	       }
	    }
	 }
      }
   }
   return score;
} 


// Return a negative value in the pair.first if there were no atoms in a_rotamer.
// Also, print an error message because (AFAICS) it should never happen.
// 
std::pair<double, clipper::Coord_orth>
molecule_class_info_t::get_minimol_pos(const coot::minimol::molecule &a_rotamer) const {

   std::pair<double, clipper::Coord_orth> r;
   double max_dev_residue_pos = -9999999.9;
   clipper::Coord_orth running_pos(0.0, 0.0, 0.0);
   float n_atom = 0.0;
   
   for (unsigned int ifrag=0; ifrag<a_rotamer.fragments.size(); ifrag++) {
      for (int ires=a_rotamer[ifrag].min_res_no(); ires<=a_rotamer[ifrag].max_residue_number(); ires++) {
	 for (int iat=0; iat<a_rotamer[ifrag][ires].n_atoms(); iat++) {
	    
	    running_pos += a_rotamer[ifrag][ires][iat].pos;
	    n_atom += 1.0;
	    
	 }
      }
   }

   if (n_atom > 0.0) { 
      clipper::Coord_orth residue_centre = clipper::Coord_orth(running_pos.x()/n_atom,
							       running_pos.y()/n_atom,
							       running_pos.z()/n_atom);
      double devi;
      for (unsigned int ifrag=0; ifrag<a_rotamer.fragments.size(); ifrag++) {
	 for (int ires=a_rotamer[ifrag].min_res_no(); ires<=a_rotamer[ifrag].max_residue_number(); ires++) {
	    for (int iat=0; iat<a_rotamer[ifrag][ires].n_atoms(); iat++) {
	       devi = clipper::Coord_orth::length(a_rotamer[ifrag][ires][iat].pos, residue_centre);
	       if (devi > max_dev_residue_pos) {
		  max_dev_residue_pos = devi;
	       } 
	    }
	 }
      }
      r.first =  max_dev_residue_pos;
      r.second = residue_centre;
   } else {
      std::cout << "ERROR: minimol pos: there are no atoms in the residue" << std::endl;
   }
   return r;
} 

// We have name_.  We use that to find backup files in coot-backup
// dir.  If there is one and it is more recent that the file name_
// then restore for it and return 1.  Else return 0.
coot::backup_file_info 
molecule_class_info_t::recent_backup_file_info() const {

   coot::backup_file_info info;

#if !defined(WINDOWS_MINGW) && !defined(_MSC_VER)
   if (has_model()) { 
      std::string t_name_glob = name_;
      // convert "/" to "_"
      int slen = t_name_glob.length();
      for (int i=0; i<slen; i++)
	 if (t_name_glob[i] == '/')
	    t_name_glob[i] = '_';

      // Let's make a string that we can glob:
      // "coot-backup/thing.pdb*.pdb.gz"

      // c.f. make_backup():
      char *es = getenv("COOT_BACKUP_DIR");
      std::string backup_name_glob = "coot-backup/";
      if (es)
	 backup_name_glob = es;
      backup_name_glob += t_name_glob;
      backup_name_glob += "*.pdb.gz";

      glob_t myglob;
      int flags = 0;
      glob(backup_name_glob.c_str(), flags, 0, &myglob);
      size_t count;

      char **p;
      std::vector<std::string> v;
      for (p = myglob.gl_pathv, count = myglob.gl_pathc; count; p++, count--) { 
	 std::string f(*p);
	 v.push_back(f);
      }
      globfree(&myglob);

      if (v.size() > 0) {

	 struct stat buf;

	 // get the time of name_ file
	 int status = stat(name_.c_str(), &buf);
	 if (status == 0) { 

	    time_t name_mtime;
	    name_mtime = buf.st_mtime;
	 
	    // now stat for dated back files:
	    time_t mtime;
	    time_t mtime_youngest = 1000000000;
	    short int set_something = 0;
	    std::string backup_filename; 

	    for (unsigned int i=0; i<v.size(); i++) { 
	       status = stat(v[i].c_str(),&buf);
	       if (status == 0) { 
		  mtime = buf.st_mtime;
		  if (mtime > mtime_youngest) { 
		     set_something = 1;
		     mtime_youngest = mtime;
		     backup_filename = v[i];
		  }
	       }
	    }
	    if (set_something) { 
	       if (mtime_youngest > name_mtime) { 
// 		  std::cout << "Restoring from a recent backup " 
// 			    << backup_filename << std::endl;
		  info.name = name_;
		  info.backup_file_name = backup_filename;
		  info.status = 1; // There is a file.
	       }
	    }
	 } 
      }
   }
#endif
   return info;
} 


short int
molecule_class_info_t::execute_restore_from_recent_backup(std::string backup_file_name) {

   // std::cout << "Recovering from file: " << backup_file_name << std::endl;

   std::string save_name = name_;
   int save_imol = imol_no;
   // similarly, it messes with the save_state_command_strings_, we
   // don't want that either:
   std::vector<std::string> save_save_state = save_state_command_strings_;
   short int is_undo_or_redo = 1;
   short int reset_rotation_centre = 0;
   handle_read_draw_molecule(backup_file_name, reset_rotation_centre, is_undo_or_redo);
   save_state_command_strings_ = save_save_state;
   imol_no = save_imol; 
   name_ = save_name;
   have_unsaved_changes_flag = 1;

   return 0;
}


void
molecule_class_info_t::convert_rgb_to_hsv_in_place(const float *rgb, float *hsv) const {
   
   // convert to hsv
   float maxc = -1.0;
   float minc = 9.0;

   for (int i=0; i<3; i++) {
      if (maxc < rgb[i]) maxc = rgb[i];
      if (minc > rgb[i]) minc = rgb[i];
   }
   hsv[2] = maxc;

   if (minc == maxc) {
      hsv[0] = 0.0;
      hsv[1] = 0.0;
      hsv[2] = maxc;
   } else { 

      float range = maxc - minc; 
      hsv[1] = range/maxc;
      float rc = (maxc - rgb[0]) / range;
      float gc = (maxc - rgb[1]) / range;
      float bc = (maxc - rgb[2]) / range;
      if (rgb[0] == maxc){ 
	 hsv[0] = bc-gc;
      } else {
	 if (rgb[1]==maxc) {
	    hsv[0] = 2.0+rc-bc;
	 } else {
	    hsv[0] = 4.0 + gc-rc;
	 }
      }
      hsv[0] = hsv[0]/6.0- floorf(hsv[0]/6.0);
   }
}

void
molecule_class_info_t::convert_hsv_to_rgb_in_place(const float* hsv, float *rgb) const {


   if (hsv[1] == 0.0) {
      rgb[0] = hsv[2]; 
      rgb[1] = hsv[2]; 
      rgb[2] = hsv[2];
   } else {
      float fi = floorf(hsv[0]*6.0);
      float f  = (hsv[0]*6.0) - fi;
      float p = hsv[2]*(1.0 - hsv[1]);
      float q = hsv[2]*(1.0 - hsv[1]*f);
      float t = hsv[2]*(1.0 - hsv[1]*(1.0-f));

      int i = int(fi);
      switch (i) {

      case 0:
      case 6:
	 rgb[0] = hsv[2]; 
	 rgb[1] = t; 
	 rgb[2] = p;
	 break;

      case 1:
	 rgb[0] = q;
	 rgb[1] = hsv[2]; 
	 rgb[2] = p;
	 break;

      case 2:
	 rgb[0] = p;
	 rgb[1] = hsv[2]; 
	 rgb[2] = t;
	 break;

      case 3:
	 rgb[0] = p;
	 rgb[1] = q; 
	 rgb[2] = hsv[2];
	 break;

      case 4:
	 rgb[0] = t;
	 rgb[1] = p; 
	 rgb[2] = hsv[2];
	 break;

      case 5:
	 rgb[0] = hsv[2];
	 rgb[1] = p; 
	 rgb[2] = q;
	 break;
      }
   }
   
}


// for widget label:
std::string
molecule_class_info_t::cell_text_with_embeded_newline() const { 

   std::string s;

   if (has_map()) { 

      s = "   ";
      s += graphics_info_t::float_to_string(xmap_list[0].cell().descr().a());
      s += " ";
      s += graphics_info_t::float_to_string(xmap_list[0].cell().descr().b());
      s += " ";
      s += graphics_info_t::float_to_string(xmap_list[0].cell().descr().c());
      s += "\n   ";
      s += graphics_info_t::float_to_string(clipper::Util::rad2d(xmap_list[0].cell().descr().alpha()));
      s += " ";
      s += graphics_info_t::float_to_string(clipper::Util::rad2d(xmap_list[0].cell().descr().beta()));
      s += " ";
      s += graphics_info_t::float_to_string(clipper::Util::rad2d(xmap_list[0].cell().descr().gamma()));
   }
   return s;
} 


// http://ngfnblast.gbf.de/docs/fasta.html
// 
// For those programs that use amino acid query sequences (BLASTP and
// TBLASTN), the accepted amino acid codes are:
// 
//     A  alanine                         P  proline
//     B  aspartate or asparagine         Q  glutamine
//     C  cystine                         R  arginine
//     D  aspartate                       S  serine
//     E  glutamate                       T  threonine
//     F  phenylalanine                   U  selenocysteine
//     G  glycine                         V  valine
//     H  histidine                       W  tryptophane
//     I  isoleucine                      Y  tyrosine
//     K  lysine                          Z  glutamate or glutamine
//     L  leucine                         X  any
//     M  methionine                      *  translation stop
//     N  asparagine                      -  gap of indeterminate length

// sequence
void
molecule_class_info_t::assign_fasta_sequence(const std::string &chain_id, const std::string &seq_in) { 

   // format "> name\n <sequence>", we ignore everything that is not a
   // letter after the newline.

   // sequence is member data.  Let's fill it.
   std::cout << "in assign_fasta_sequence\n";

   std::string seq;

   int nchars = seq_in.length();
   short int found_greater = 0;
   short int found_newline = 0;
   std::string t;

   for (int i=0; i<nchars; i++) {

      // std::cout << "checking character: " << seq_in[i] << std::endl;

      if (found_newline && found_greater) {
	 t = toupper(seq_in[i]);
	 if (is_fasta_aa(t)) {
	    // 	    std::cout << "adding character: " << seq_in[i] << std::endl;
	    seq += t;
	 }
      }
      if (seq_in[i] == '>') {
	 // 	 std::cout << "DEBUG:: " << seq_in[i] << " is > (greater than)\n";
	 found_greater = 1;
      }
      if (seq_in[i] == '\n') { 
	 if (found_greater) {
	    // 	    std::cout << "DEBUG:: " << seq_in[i] << " is carriage return\n";
	    found_newline = 1;
	 }
      }
   }
   
   if (seq.length() > 0) { 
      std::cout << "storing sequence: " << seq << " for chain id: " << chain_id
		<< std::endl;
      input_sequence.push_back(std::pair<std::string, std::string> (chain_id,seq));
   } else { 
      std::cout << "WARNING:: no sequence found or improper fasta sequence format\n";
   }
}

void
molecule_class_info_t::assign_pir_sequence(const std::string &chain_id, const std::string &seq_in) {

   // format "> ID;database-id\ntext-descr\n <sequence>", we ignore everything that is not a
   // letter after the newline.

   // sequence is member data.  Let's fill it.
   std::cout << "in assign_fasta_sequence\n";

   std::string seq;

   int nchars = seq_in.length();
   short int found_greater = 0;
   short int found_newline = 0;
   short int found_textdescr = 0;
   std::string t;

   for (int i=0; i<nchars; i++) {

      // std::cout << "checking character: " << seq_in[i] << std::endl;

      if (found_newline && found_greater && found_textdescr) {
	 t = toupper(seq_in[i]);
	 if (is_pir_aa(t)) {
	    // 	    std::cout << "adding character: " << seq_in[i] << std::endl;
	    seq += t;
	    if (t == "*") // end of sequence
	       break; // the for loop
	 }
      }
      if (seq_in[i] == '>') {
	 // 	 std::cout << "DEBUG:: " << seq_in[i] << " is > (greater than)\n";
	 found_greater = 1;
      }
      if (seq_in[i] == '\n') { 
	 if (found_newline) {
	    // 	    std::cout << "DEBUG:: " << seq_in[i] << " is carriage return\n";
	    found_textdescr = 1;
	 }
	 if (found_greater) {
	    // 	    std::cout << "DEBUG:: " << seq_in[i] << " is carriage return\n";
	    found_newline = 1;
	 }
      }
   }
   
   if (seq.length() > 0) { 
      std::cout << "storing sequence: " << seq << " for chain id: " << chain_id
		<< std::endl;
      input_sequence.push_back(std::pair<std::string, std::string> (chain_id,seq));
   } else { 
      std::cout << "WARNING:: no sequence found or improper fasta sequence format\n";
   }
   
}


// Return a flag that tells us if we did indeed find a proper next
// residue.  Return the 3-letter-code in second.
// 
std::pair<bool, std::string>
molecule_class_info_t::residue_type_next_residue_by_alignment(const coot::residue_spec_t &clicked_residue,
							      CChain *clicked_residue_chain_p, 
							      short int is_n_term_addition) const {

   std::pair<bool, std::string> p(0, "");

   if (input_sequence.size() > 0) {
      std::string chain_id = clicked_residue.chain;
      for (unsigned int ich=0; ich<input_sequence.size(); ich++) {

	 if (input_sequence[ich].first == chain_id) {

	    if (input_sequence[ich].second.length() > 0) {
	       std::vector<PCResidue> frag_residues =
		  coot::util::get_residues_in_fragment(clicked_residue_chain_p, clicked_residue);
	       // copy from vector to array
	       PCResidue *SelResidues = new PCResidue[frag_residues.size()];
	       for (unsigned int ires=0; ires<frag_residues.size(); ires++)
		  SelResidues[ires] = frag_residues[ires];
	       coot::chain_mutation_info_container_t a = 
		  align_on_chain(chain_id, SelResidues, frag_residues.size(),
				 input_sequence[ich].second);

	       if ((a.insertions.size() +
		    a.mutations.size() +
		    a.deletions.size()) > (input_sequence[ich].second.length()/5)) {
		  std::cout << "WARNING:: Too many mutations, "
			    << "can't make sense of aligment "
			    << a.insertions.size() << " "
			    << a.mutations.size() << " "
			    << a.deletions.size() << " "
			    << input_sequence[ich].second.length()
			    << std::endl;
	       } else {
		  // proceed
		  std::cout << a.alignedS << std::endl;
		  std::cout << a.alignedT << std::endl;

		  // where is clicked_residue in SelResidues?
		  bool found = 0;
		  int frag_seqnum;
		  for (int ires=0; ires<input_sequence[ich].second.length(); ires++) {
		     if (SelResidues[ires]->GetSeqNum() == clicked_residue.resno) {
			if (clicked_residue.chain == SelResidues[ires]->GetChainID()) {
			   // found clicked_residue
			   found = 1;
			   frag_seqnum = ires;
// 			   std::cout << "DEBUG:: found frag_seqnum: " << frag_seqnum
// 				     << " " << SelResidues[ires]->GetSeqNum() << " "
// 				     << std::endl;
			   break;
			}
		     }
		  }
		  if (found) {
		     int added_res_resno; 
		     if (is_n_term_addition) {
			added_res_resno = frag_seqnum - 1; 
		     } else {
			added_res_resno = frag_seqnum + 1;
		     }

		     if (a.alignedT.length() > added_res_resno) {
			if (added_res_resno >= 0) {
			   char code = a.alignedT[added_res_resno];
			   std::cout << " code: " << code << std::endl;
			   std::string res =
			      coot::util::single_letter_to_3_letter_code(code);
			   p = std::pair<bool, std::string>(1, res);
			   for (int off=5; off>=0; off--) { 
			       char c = a.alignedT[added_res_resno-off];
			      std::cout << c;
			   }
			   std::cout << std::endl;
			}
		     }
		  }
	       }
	       delete [] SelResidues;
	    }
	    break;
	 } 
      }
   }
   return p;
}


bool
molecule_class_info_t::is_fasta_aa(const std::string &a) const { 

   short int r = 0;
   
   if (a == "A" || a == "G" ) { 
      r = 1;
   } else { 
      if (a == "B" 
	  || a == "C" || a == "D" || a == "E" || a == "F" || a == "H" || a == "I"
	  || a == "K" || a == "L" || a == "M" || a == "N" || a == "P" || a == "Q" 
	  || a == "R" || a == "S" || a == "T" || a == "U" || a == "V" || a == "W" 
	  || a == "Y" || a == "Z" || a == "X" || a == "*" || a == "-") {
	 r = 1;
      }
   }
   return r;
}

bool
molecule_class_info_t::is_pir_aa(const std::string &a) const { 

   short int r = 0;
   
   if (a == "A" || a == "G" ) { 
      r = 1;
   } else { 
      if (   a == "C" || a == "D" || a == "E" || a == "F" || a == "H" || a == "I"
	  || a == "K" || a == "L" || a == "M" || a == "N" || a == "P" || a == "Q" 
	  || a == "R" || a == "S" || a == "T" ||             a == "V" || a == "W" 
	  || a == "Y" || a == "Z" || a == "X" ) {
	 r = 1;
      }
   }
   return r;
}

// render option (other functions)
coot::ray_trace_molecule_info
molecule_class_info_t::fill_raster_model_info() {

   coot::ray_trace_molecule_info rtmi;
   if (has_model()) {
      for (int i=0; i<bonds_box.num_colours; i++) {
	 set_bond_colour_by_mol_no(i); //sets bond_colour_internal
	 for (int j=0; j<bonds_box.bonds_[i].num_lines; j++) {
	    rtmi.bond_lines.push_back(std::pair<coot::Cartesian, coot::Cartesian>(bonds_box.bonds_[i].pair_list[j].getStart(), bonds_box.bonds_[i].pair_list[j].getFinish()));
	    coot::colour_t c;
	    c.col.resize(3);
	    c.col[0] = bond_colour_internal[0];
	    c.col[1] = bond_colour_internal[1];
	    c.col[2] = bond_colour_internal[2];
	    rtmi.bond_colour.push_back(c);
	 }
      }

      std::cout << " There are " << bonds_box.n_atom_centres_
		<< " atom centres in this bonds_box\n";
      
      for (int i=0; i<bonds_box.n_atom_centres_; i++) {
	 coot::colour_t c;
	 c.col.resize(3);
	 set_bond_colour_by_mol_no(bonds_box.atom_centres_colour_[i]); //sets bond_colour_internal
	 c.col[0] = bond_colour_internal[0];
	 c.col[1] = bond_colour_internal[1];
	 c.col[2] = bond_colour_internal[2];
	 rtmi.atom.push_back(std::pair<coot::Cartesian, coot::colour_t> (bonds_box.atom_centres_[i], c));
      }
   }
   return rtmi;
}


// Pass lev as +1, -1
// 
coot::ray_trace_molecule_info
molecule_class_info_t::fill_raster_map_info(short int lev) const {

   coot::ray_trace_molecule_info rtmi;
   if (has_map()) {

      rtmi.bond_colour.resize(3);
      if (drawit_for_map) { 
	 if (lev == 1) {
	    if (n_draw_vectors>0) { 

	       rtmi.density_colour.col.resize(3);
	       rtmi.density_colour.col[0] = map_colour[0][0];
	       rtmi.density_colour.col[1] = map_colour[0][1];
	       rtmi.density_colour.col[2] = map_colour[0][2];

	       for(int i=0; i<n_draw_vectors; i++) {
		  rtmi.density_lines.push_back(std::pair<coot::Cartesian, coot::Cartesian>(draw_vectors[i].getStart(), draw_vectors[i].getFinish()));
	       }
	    }
	 } else {
	    if (n_diff_map_draw_vectors > 0) {

	       rtmi.density_colour.col.resize(3);
	       rtmi.density_colour.col[0] = map_colour[1][0];
	       rtmi.density_colour.col[1] = map_colour[1][1];
	       rtmi.density_colour.col[2] = map_colour[1][2];

	       for(int i=0; i<n_diff_map_draw_vectors; i++) {
		  rtmi.density_lines.push_back(std::pair<coot::Cartesian, coot::Cartesian>(diff_map_draw_vectors[i].getStart(), diff_map_draw_vectors[i].getFinish()));
	       }
	    }
	 }
      }

      if (fc_skeleton_draw_on == 1) {

	 rtmi.bones_colour.col.resize(3);
	 for (int i=0; i<3; i++) 
	    rtmi.bones_colour.col[i] = graphics_info_t::skeleton_colour[i];
	 for (int l=0; l<fc_skel_box.num_colours; l++) {
	    for (int j=0; j<fc_skel_box.bonds_[l].num_lines; j++) {
	       std::pair<coot::Cartesian, coot::Cartesian> p(fc_skel_box.bonds_[l].pair_list[j].getStart(),
							     fc_skel_box.bonds_[l].pair_list[j].getFinish());
	       rtmi.bone_lines.push_back(p);
	    }
	 }
      }
   } 
   return rtmi;
}


int
molecule_class_info_t::trim_by_map(const clipper::Xmap<float> &xmap_in,
				   float map_level, short int delete_or_zero_occ_flag) { 

   
   // "Trim of the bits we don't need":
   // 20050610: I laugh at that.
   
   short int waters_only_flag = 0;
   int i = coot::util::trim_molecule_by_map(atom_sel, xmap_in, map_level, 
					    delete_or_zero_occ_flag,
					    waters_only_flag);

   std::cout << "INFO:: " << i << " atoms were trimmed\n";
   if (i > 0) { 
      make_backup();
      update_molecule_after_additions(); // sets have_unsaved_changes_flag
   }
   return i;
} 



std::vector<coot::atom_spec_t>
molecule_class_info_t::check_waters_by_difference_map(const clipper::Xmap<float> &xmap,
						      float sigma_level) const {

   std::vector<coot::atom_spec_t> v;

   std::vector<std::pair<coot::util::density_stats_info_t, coot::atom_spec_t> > dsi;

   std::pair<coot::util::density_stats_info_t, coot::atom_spec_t> pair;
   coot::atom_spec_t at_spec;

   for (int i=0; i<atom_sel.n_selected_atoms; i++) {
      std::string resname = atom_sel.atom_selection[i]->residue->name;
      if (resname == "WAT" || resname == "HOH") {
	 clipper::Coord_orth p(atom_sel.atom_selection[i]->x,
			       atom_sel.atom_selection[i]->y,
			       atom_sel.atom_selection[i]->z);
	 coot::atom_spec_t at_spec(atom_sel.atom_selection[i]->GetChainID(),
				   atom_sel.atom_selection[i]->GetSeqNum(),
				   atom_sel.atom_selection[i]->GetInsCode(),
				   atom_sel.atom_selection[i]->GetAtomName(),
				   atom_sel.atom_selection[i]->altLoc);
	 pair = std::pair<coot::util::density_stats_info_t, coot::atom_spec_t>(coot::util::density_around_point(p, xmap, 1.5), at_spec);
      
      dsi.push_back(pair);
      }
   }

   float sum_v = 0.0;
   float sum_v_sq = 0.0;
   // int n_atoms = 0;
   int n_v = 0;
   for (unsigned int id=0; id<dsi.size(); id++) {
      sum_v    += dsi[id].first.sum_sq;
      sum_v_sq += dsi[id].first.sum_sq * dsi[id].first.sum_sq;
      n_v++;
   }

   float v_mean = sum_v/float(n_v);
   float v_variance = sum_v_sq/float(n_v) - v_mean*v_mean;
   for (unsigned int id=0; id<dsi.size(); id++) {
      if ( (dsi[id].first.sum_sq - v_mean)/sqrt(v_variance) > sigma_level) {
	 v.push_back(dsi[id].second);
      } 
   }   
   
   return v;
}


// Return a list of bad chiral volumes for this molecule:
// 
// Return also a flag for the status of this test, were there any
// residues for we we didn't find restraints?  The flag is the number
// of residue names in first part of the returned pair.
// 
// 
std::pair<std::vector<std::string> , std::vector <coot::atom_spec_t> >
molecule_class_info_t::bad_chiral_volumes() const {

   std::vector <coot::atom_spec_t> v;
   graphics_info_t g;
   int restraints_status = 1;
   std::vector<std::string> unknown_types_vec; 
   std::pair<std::vector<std::string>, std::vector<coot::atom_spec_t> > pair(unknown_types_vec, v);

#ifdef HAVE_GSL
   if (atom_sel.n_selected_atoms > 0) {
      // grr Geom_p() is not static
      graphics_info_t g;
      pair = coot::bad_chiral_volumes(atom_sel.mol, g.Geom_p(),
				      graphics_info_t::cif_dictionary_read_number);
   }
#endif //  HAVE_GSL

   return pair;
}



float
molecule_class_info_t::score_residue_range_fit_to_map(int resno1, int resno2,
						      std::string altloc,
						      std::string chain_id,
						      int imol_for_map) {

   float f = 0;

   // Make an atom selection:
   // Convert atoms to coord_orths,
   // Use a coot util function to return the score.
   // 

   int selHnd = atom_sel.mol->NewSelection();
   atom_sel.mol->SelectAtoms(selHnd, 0,  (char *) chain_id.c_str(),
			     resno1, "*",
			     resno2, "*",
			     "*", // residue name
			     "*", // atom name
			     "*", // element name
			     (char *) altloc.c_str());
   PPCAtom local_SelAtom = NULL;
   int nSelAtoms;
   atom_sel.mol->GetSelIndex(selHnd, local_SelAtom, nSelAtoms);

   if (nSelAtoms == 0) {
      std::cout << "WARNING:: No atoms selected in "
		<< "score_residue_range_fit_to_map\n";
   } else {
      f = coot::util::map_score(local_SelAtom, nSelAtoms,
				graphics_info_t::molecules[imol_for_map].xmap_list[0],
				0 // not score by atom type
				);
      std::cout << "score for residue range " << resno1 << " " << resno2
		<< " chain " <<  chain_id
		<< ": " << f << std::endl;
   }
   atom_sel.mol->DeleteSelection(selHnd);
   return f;
} 


void
molecule_class_info_t::fit_residue_range_to_map_by_simplex(int resno1, int resno2,
							   std::string altloc,
							   std::string chain_id,
							   int imol_for_map) {

#ifdef HAVE_GSL   
   int selHnd = atom_sel.mol->NewSelection();
   atom_sel.mol->SelectAtoms(selHnd, 0,  (char *) chain_id.c_str(),
			     resno1, "*",
			     resno2, "*",
			     "*", // residue name
			     "*", // atom name
			     "*", // ?
			     (char *) altloc.c_str());
   PPCAtom local_SelAtom = NULL;
   int nSelAtoms;
   atom_sel.mol->GetSelIndex(selHnd, local_SelAtom, nSelAtoms);

   if (nSelAtoms == 0) {
      std::cout << "WARNING:: No atoms selected in "
		<< "score_residue_range_fit_to_map\n";
   } else {
      make_backup();
      coot::util::fit_to_map_by_simplex(local_SelAtom, nSelAtoms,
					graphics_info_t::molecules[imol_for_map].xmap_list[0]);
      have_unsaved_changes_flag = 1;
      make_bonds_type_checked();
   }

   atom_sel.mol->DeleteSelection(selHnd);
#endif // HAVE_GSL   
}





void
molecule_class_info_t::split_residue(int atom_index) { 

   if (atom_index < atom_sel.n_selected_atoms) {
      int do_intermediate_atoms = 0;
      CResidue *res = atom_sel.atom_selection[atom_index]->residue;
      std::vector<std::string> residue_alt_confs =
	 get_residue_alt_confs(res);
      std::string altconf(atom_sel.atom_selection[atom_index]->altLoc);
      int atom_index_udd = atom_sel.UDDAtomIndexHandle;

      int udd_afix_handle = -1; // don't use value
      if (is_from_shelx_ins_flag)
	 udd_afix_handle = atom_sel.mol->GetUDDHandle(UDR_ATOM, "shelx afix");

//       std::cout << " ==================== in split_residue ===================== " << std::endl;
//       std::cout << " ============== udd_afix_handle " << udd_afix_handle << " ======== " << std::endl;

      std::pair<CResidue *, atom_selection_container_t> p =
	 coot::deep_copy_this_residue_and_make_asc(atom_sel.mol, res, altconf, 1,
						   atom_index_udd, udd_afix_handle);
      CResidue *res_copy = p.first;
      atom_selection_container_t residue_mol = p.second;

      // if is from SHELXL
      if (is_from_shelx_ins_flag)
	 // if splitting after/with CA
	 if (graphics_info_t::alt_conf_split_type == 0)
	    // a member function?
	    residue_mol = filter_atom_selection_container_CA_sidechain_only(residue_mol);

//       std::cout << "----------- state of residue mol: " << std::endl;
//       for (int i=0; i<residue_mol.n_selected_atoms; i++) {
// 	 std::cout << "residue mol " << residue_mol.atom_selection[i] << std::endl;
//       } 

      int udd_afix_handle_inter = residue_mol.mol->GetUDDHandle(UDR_ATOM, "shelx afix");
//       std::cout << "DEBUG:: split_residue got udd_afix_handle_inter : "
// 		<< udd_afix_handle_inter << std::endl;
//       for (int i=0; i<residue_mol.n_selected_atoms; i++) {
// 	 int afix_number = -1;
// 	 if (residue_mol.atom_selection[i]->GetUDData(udd_afix_handle_inter, afix_number) == UDDATA_Ok)
// 	    std::cout << residue_mol.atom_selection[i] << " has afix number " << afix_number
// 		      << std::endl;
// 	 else
// 	    std::cout << residue_mol.atom_selection[i]
// 		      << " Failed get udd afix number in split_residue"
// 		      << std::endl;
//       }
      
	 
      // if we are splitting a water, then testing for alt_conf_split_type == 0
      // doen't make sense, so let's also skip that if we have a water.
      // 
      short int is_water_flag = 0;
      std::string residue_type = res->GetResName();
      if (residue_type == "WAT" || residue_type == "HOH" || residue_type == "DUM")
	 is_water_flag = 1;
      
      // We can't do a rotamer fit if we don't have C and N atoms:
      // 
      // Also, we don't want to do a rotamer fit of the user has
      // turned this option off:
      // 
      if (!graphics_info_t::show_alt_conf_intermediate_atoms_flag &&
	  have_atoms_for_rotamer(res_copy)) {

	 // We want to delete atoms if we chose alt_conf_split_type.. thing
	 if (graphics_info_t::alt_conf_split_type == 0) { // partial split (not whole residue)
	    PPCAtom residue_atoms = NULL;
	    int nResidueAtoms;
	    res_copy->GetAtomTable(residue_atoms, nResidueAtoms);
	    std::string mol_atom_name;
	    for (int iat=0; iat<nResidueAtoms; iat++) {
	       mol_atom_name = std::string(residue_atoms[iat]->name);
	       if (mol_atom_name == " N  " ||
		   mol_atom_name == " C  " ||
		   mol_atom_name == " H  " ||
		   mol_atom_name == " HA " || // CA hydrogen
		   mol_atom_name == " O  ") { 
// 		  std::cout << "DEBUG:: split residue deleting  " 
// 			    << mol_atom_name << std::endl;
		  res_copy->DeleteAtom(iat);
	       } else {
		  // std::cout << "split residue accepting " << mol_atom_name
		  // << std::endl;
	       }
	    }
	    res_copy->TrimAtomTable();
	    // delete [] residue_atoms; // not with GetAtomTable(), fool.
	 }
	 
	 // just go ahead and stuff in the atoms to the
	 // molecule, no user intervention required.
	 // 
	 // std::cout << "Calling  -------------- split_residue_then_rotamer path ----------------\n";
	 short int use_residue_mol_flag = 0;
	 if (is_from_shelx_ins_flag)
	    use_residue_mol_flag = 1;
	 split_residue_then_rotamer(res_copy, altconf, residue_alt_confs, residue_mol,
				    use_residue_mol_flag);
	 
      } else {
	 // we don't have all the atoms in the residue to do a
	 // rotamer, therefore we must fall back to showing the
	 // intermediate atoms.
	 do_intermediate_atoms = 1;
	 if (graphics_info_t::alt_conf_split_type == 0) { // partial split (not whole residue)

	    if (! is_water_flag) { 
	       // so we need to delete some atoms from this residue:
	       //
	       // but we only want to do this if we are representing
	       // intermediate atoms.  If we are going on to rotamers,
	       // we need all the atoms (so that we can do a fit using
	       // the main chain atoms of the rotamer).
	       // 
	       //
	       PPCAtom residue_atoms;
	       int nResidueAtoms;
	       res_copy->GetAtomTable(residue_atoms, nResidueAtoms);
	       std::string mol_atom_name;
	       for (int iat=0; iat<nResidueAtoms; iat++) {
		  mol_atom_name = std::string(residue_atoms[iat]->name);
		  if (mol_atom_name == " N  " ||
		      mol_atom_name == " C  " ||
		      mol_atom_name == " H  " ||
		      mol_atom_name == " HA " ||
		      mol_atom_name == " O  ") { 
		     // std::cout << "split residue deleting  " << mol_atom_name
		     // << std::endl;
		     res_copy->DeleteAtom(iat);
		  } else {
		     // std::cout << "split residue accepting " << mol_atom_name
		     // << std::endl;
		  }
	       }
	    }
	 } else { 
	    std::cout << "split_residue split type " << graphics_info_t::alt_conf_split_type
		      << " no deleting atoms  of this residue\n";
	 }
	 
	 res_copy->TrimAtomTable();
	 atom_selection_container_t asc_dummy;
	 std::cout << "Calling  -------------- split_residue_internal ---------------------------\n";
	 split_residue_internal(res_copy, altconf, residue_alt_confs, p.second, 1);
      }
   } else {
      std::cout << "WARNING:: split_residue: bad atom index.\n";
   }
}

// change asc.
atom_selection_container_t
molecule_class_info_t::filter_atom_selection_container_CA_sidechain_only(atom_selection_container_t asc) const {

   std::string mol_atom_name;
   for (int iat=0; iat<asc.n_selected_atoms; iat++) {
      mol_atom_name = std::string(asc.atom_selection[iat]->name);
      if (mol_atom_name == " N  " ||
	  mol_atom_name == " C  " ||
	  mol_atom_name == " H  " ||
	  // mol_atom_name == " HA " || // CA hydrogen we don't want
                                        // to delete it if we don't delete the CA.
	  mol_atom_name == " H0 " || // shelx N hydrogen, 
	  mol_atom_name == " O  ") {
	 CResidue *r = asc.atom_selection[iat]->residue;
	 r->DeleteAtom(iat);
	 // std::cout << "Filter out atom " << asc.atom_selection[iat] << std::endl;
      } else {
	 // std::cout << "side chain atom " << asc.atom_selection[iat] << std::endl;
      } 
   }
   asc.mol->FinishStructEdit();

   atom_selection_container_t ret_asc = make_asc(asc.mol);
//    std::cout << "----------- immediate state of residue mol: " << std::endl;
//    for (int i=0; i<ret_asc.n_selected_atoms; i++) {
//       std::cout << "rest asc mol " << ret_asc.atom_selection[i] << std::endl;
//    } 
   
   return ret_asc;
} 
   


std::vector<std::string> 
molecule_class_info_t::get_residue_alt_confs(CResidue *res) const { 

   std::vector<std::string> v;

   PPCAtom residue_atoms;
   int nResidueAtoms;
   res->GetAtomTable(residue_atoms, nResidueAtoms);
   short int ifound = 0;
   for (int iat=0; iat<nResidueAtoms; iat++) {
      ifound = 0;
      for(unsigned int i=0; i<v.size(); i++) { 
	 if (std::string(residue_atoms[iat]->altLoc) == v[i]) {
	    ifound = 1;
	    break;
	 }
      }
      if (! ifound) 
	 v.push_back(std::string(residue_atoms[iat]->altLoc));
   }
   
   return v;
} 

// What's in the residue     What we clicked   Old Coordinates   New Coordinates
//      ""                        ""                "" -> "A"       "B"
//    "A" "B"                     "A"              no change        "C"
//    "A" "B"                     "B"              no change        "C"
//    "" "A" "B"                  ""               [1]              "C"
//    "" "A" "B"                  "A"              [1]              "C"
//    "" "A" "B"                  "B"              [1]              "C"
//
// [1] depends on the split:
//     whole residue split: "" -> "A" , "A" and "B" remain the same
//     partial split:       no change
// 
std::string 
molecule_class_info_t::make_new_alt_conf(const std::vector<std::string> &residue_alt_confs, 
					 int alt_conf_split_type_in) const { 
   
   std::string v("");
   std::vector<std::string> m;
   m.push_back("B");
   m.push_back("C");
   m.push_back("D");

   short int got;
   for (unsigned int im=0; im<m.size(); im++) { 
      got = 0;
      for (unsigned int ir=0; ir<residue_alt_confs.size(); ir++) {
	 if (m[im] == residue_alt_confs[ir]) {
	    got = 1;
	    break;
	 }
      }
      if (got == 0) {
	 v = m[im];
	 break;
      }
   }

   return v;
}


// This is a molecule-class-info function.  
void 
molecule_class_info_t::split_residue_internal(CResidue *residue, const std::string &altconf, 
					      const std::vector<std::string> &all_residue_altconfs,
					      atom_selection_container_t residue_mol,
					      short int use_residue_mol_flag) {

   PCResidue *SelResidues = new PCResidue;
   std::string ch(residue->GetChainID());
   
   SelResidues = &residue; // just one

   std::cout << "==================== in split_residue_internal ============= " << std::endl;

   atom_selection_container_t asc;
   if (!use_residue_mol_flag) { 
      CMMDBManager *mov_mol = create_mmdbmanager_from_res_selection(SelResidues,
								    1, 0, 0, altconf, 
								    ch, 1);
      
      asc = make_asc(mov_mol);
   } else {
      asc = residue_mol;
      int udd_afix_handle = atom_sel.mol->GetUDDHandle(UDR_ATOM, "shelx afix");
//       std::cout << "DEBUG:: split_residue_internal got udd_afix_handle : "
// 		<< udd_afix_handle << std::endl;
      for (int i=0; i<asc.n_selected_atoms; i++) {
	 int afix_number = -1;
	 if (asc.atom_selection[i]->GetUDData(udd_afix_handle, afix_number) == UDDATA_Ok)
	    std::cout << asc.atom_selection[i] << " has afix number " << afix_number << std::endl;
// 	 else
// 	    std::cout << asc.atom_selection[i]
// 		      << " Failed get udd afix number in split residue_internal"
// 		      << std::endl;
      } 
   }

   std::string new_alt_conf = make_new_alt_conf(all_residue_altconfs, 
						graphics_info_t::alt_conf_split_type);
   //std::cout << "DEBUG:: new_alt_conf " << new_alt_conf << " from ";
   // for (int ii=0; ii<all_residue_altconfs.size(); ii++) { 
   // std::cout << "  :" << all_residue_altconfs[ii] << ":  ";
   // }
   std::cout << std::endl;

   CAtom *at;
   for (int i=0; i<asc.n_selected_atoms; i++) { 
      at = asc.atom_selection[i];
      at->x += 0.02; 
      at->y += 0.2;
      at->z += 0.03;
      // apply the new_alt_conf:
      // std::cout << "applying alt_conf: " << new_alt_conf << std::endl;
      strncpy(at->altLoc, new_alt_conf.c_str(), 2);

      // set the occupancy:
      at->occupancy = graphics_info_t::add_alt_conf_new_atoms_occupancy;
      adjust_occupancy_other_residue_atoms(at, at->residue, 0);

//       at->SetAtomName(at->GetIndex(),
// 		      at->serNum,
// 		      at->name,
// 		      new_alt_conf.c_str(),
// 		      at->segID,
// 		      at->element,
// 		      at->charge);
   }

   // Rotamer?
   //
   // do_rotamers(0, imol);  no.  what is imol?
   //
   graphics_info_t g;

//    g.make_moving_atoms_graphics_object(asc);
//    g.imol_moving_atoms = imol_no;
//    g.moving_atoms_asc_type = coot::NEW_COORDS_INSERT_CHANGE_ALTCONF;

   g.set_moving_atoms(asc, imol_no, coot::NEW_COORDS_INSERT_CHANGE_ALTCONF);

// debugging
//    int nbonds = 0;
//    for (int i=0; i<regularize_object_bonds_box.num_colours; i++)
//       nbonds += regularize_object_bonds_box.bonds_[i].num_lines;
//    std::cout << "Post new bonds we have " << nbonds << " bonds lines\n";

   do_accept_reject_dialog("Alt Conf Split", "");
}

// We don't create an intermediate atom.
// We create a molecule and do an accept moving atoms equivalent on them.
//
// What is residue here?  residue is a pure copy of the clicked on
// residue, including all altconfs.
//
// altconf is the altconf of the clicked atom
// all_altconfs are all the altconfs in that residue (used so that we
// can find a new altconf for the new atoms).
//
void
molecule_class_info_t::split_residue_then_rotamer(CResidue *residue, const std::string &altconf, 
						  const std::vector<std::string> &all_residue_altconfs,
						  atom_selection_container_t residue_mol_asc,
						  short int use_residue_mol_flag) {


   PCResidue *SelResidues;
   std::string ch(residue->GetChainID());

   // alt_conf_split_type is a graphics_info_t static data member
   // 
   std::string new_altconf = make_new_alt_conf(all_residue_altconfs,
					       graphics_info_t::alt_conf_split_type);
   
   // Move the atoms of residue a bit here?
   atom_selection_container_t mov_mol_asc;

//    std::cout << "DEBUG:: in split_residue_then_rotamer use_residue_mol_flag: " << use_residue_mol_flag
// 	     << std::endl;
   
   if (use_residue_mol_flag) {
      //       std::cout << "DEBUG:: in split_residue_then_rotamer shelxl path " << std::endl;
      mov_mol_asc = residue_mol_asc;
      int udd_afix_handle = residue_mol_asc.mol->GetUDDHandle(UDR_ATOM, "shelx afix");
      for (int i=0; i<residue_mol_asc.n_selected_atoms; i++) {
	 int afix_number = -1;
	 if (residue_mol_asc.atom_selection[i]->GetUDData(udd_afix_handle, afix_number) == UDDATA_Ok)
	    std::cout << residue_mol_asc.atom_selection[i] << " has afix number " << afix_number
		      << std::endl;
// 	 else
// 	    std::cout << residue_mol_asc.atom_selection[i]
// 		      << " Failed get udd afix number in split residue_internal"
// 		      << std::endl;
      }
      
   } else { 
      // std::cout << "DEBUG:: in split_residue_then_rotamer normal path " << std::endl;
      SelResidues = &residue; // just one
      CMMDBManager *mov_mol = create_mmdbmanager_from_res_selection(SelResidues,
								    1, 0, 0, altconf, 
								    ch, 1);
      mov_mol_asc = make_asc(mov_mol);
   }

   CAtom *at;
   for (int i=0; i<mov_mol_asc.n_selected_atoms; i++) { 
      at = mov_mol_asc.atom_selection[i];
      at->x += 0.1;
      strncpy(at->altLoc, new_altconf.c_str(), 2);
      // set the occupancy:
      at->occupancy = graphics_info_t::add_alt_conf_new_atoms_occupancy;
   }

   std::string at_name;
   if (mov_mol_asc.n_selected_atoms > 0) {
      at_name = mov_mol_asc.atom_selection[0]->name;
   }
      
   insert_coords_change_altconf(mov_mol_asc);

   // now find the atom index of an atom with the new alt conf:

   int resno = residue->GetSeqNum();
   std::string chain_id = residue->GetChainID();
   std::string ins_code = residue->GetInsCode();

   int atom_index = full_atom_spec_to_atom_index(chain_id,
						 resno,
						 ins_code,
						 at_name,
						 new_altconf);

   if (atom_index >= 0) {
      graphics_info_t g;
      g.do_rotamers(atom_index, imol_no); // this imol, obviously.
   } else {
      std::cout << "ERROR bad atom index in split_residue_then_rotamer: "
		<< atom_index << std::endl;
   }
}

// We just added a new atom to a residue, now we need to adjust the
// occupancy of the other atoms (so that we don't get residues with
// atoms whose occupancy is greater than 1.0 (Care for SHELX molecule?)).
// at doesn't have to be in residue.
//
// Perhaps this can be a utility function?
// 
void
molecule_class_info_t::adjust_occupancy_other_residue_atoms(CAtom *at,
							    CResidue *residue,
							    short int force_sum_1_flag) {

   if (!is_from_shelx_ins_flag) { 
      int nResidueAtoms;
      PPCAtom ResidueAtoms = 0;
      residue->GetAtomTable(ResidueAtoms, nResidueAtoms);
      float new_atom_occ = at->occupancy;
      std::string new_atom_name(at->name);
      std::string new_atom_altconf(at->altLoc);
      std::vector<CAtom *> same_name_atoms;
      float sum_occ = 0;
      for (int i=0; i<nResidueAtoms; i++) {
	 std::string this_atom_name(ResidueAtoms[i]->name);
	 std::string this_atom_altloc(ResidueAtoms[i]->altLoc);
	 if (this_atom_name == new_atom_name) {
	    if (this_atom_altloc != new_atom_altconf) { 
	       same_name_atoms.push_back(ResidueAtoms[i]);
	       sum_occ += ResidueAtoms[i]->occupancy;
	    }
	 }
      }

      //
      if (sum_occ > 0.01) {
	 if (same_name_atoms.size() > 0) {
	    float other_atom_occ_sum = 0.0;
	    for (unsigned int i=0; i<same_name_atoms.size(); i++)
	       other_atom_occ_sum += same_name_atoms[i]->occupancy;
	    
	    float remainder = 1.0 - new_atom_occ;
	    float f = remainder/other_atom_occ_sum;
	    for (unsigned int i=0; i<same_name_atoms.size(); i++) {
// 	       std::cout << "debug " << same_name_atoms[i]
// 			 << " mulitplying occ " << same_name_atoms[i]->occupancy
// 			 << " by " << remainder << "/" << other_atom_occ_sum << "\n";
	       same_name_atoms[i]->occupancy *= f;
	    }
	 }
      }
   }
} 


short int
molecule_class_info_t::have_atoms_for_rotamer(CResidue *res) const {

   short int ihave = 0;  // initially not
   PPCAtom residue_atoms;
   int nResidueAtoms;
   int n_mainchain = 0;
   res->GetAtomTable(residue_atoms, nResidueAtoms);
   short int have_c = 0;
   short int have_ca = 0;
   short int have_n = 0;
   for (int iat=0; iat<nResidueAtoms; iat++) {
      std::string at_name(residue_atoms[iat]->name);
      if (at_name == " C  ") { 
	 n_mainchain++;
	 have_c = 1;
      }
      if (at_name == " CA ") { 
	 n_mainchain++;
	 have_ca = 1;
      }
      if (at_name == " N  ") { 
	 n_mainchain++;
	 have_n = 1;
      }
      
   }
   if ((n_mainchain > 2) && have_c && have_ca && have_n)
      ihave = 1;
   
   return ihave;
}


// The flanking residues (if any) are in the residue selection (SelResidues).
// The flags are not needed now we have made adjustments in the calling
// function.
// 
// create_mmdbmanager_from_res_selection must make adjustments
// 
CMMDBManager *
molecule_class_info_t::create_mmdbmanager_from_res_selection(PCResidue *SelResidues, 
							     int nSelResidues, 
							     int have_flanking_residue_at_start,
							     int have_flanking_residue_at_end, 
							     const std::string &altconf,
							     const std::string &chain_id_1,
							     short int residue_from_alt_conf_split_flag) { 

   int start_offset = 0;
   int end_offset = 0;
   
//    if (have_flanking_residue_at_start)
//       start_offset = -1;
//    if (have_flanking_residue_at_end)
//       end_offset = +1; 

   CMMDBManager *residues_mol = new CMMDBManager;
   CModel *model = new CModel;
   CChain *chain = new CChain;
   short int whole_res_flag = 0; // not all alt confs, only this one ("A") and "".

   // For the active residue range (i.e. not the flanking residues) we only want
   // to refine the atoms that have the alt conf the same as the picked atom
   // (and that is altconf, passed here).
   // 
   // However, for *flanking residues* it's different.  Say we are refining a
   // non-split residue with alt conf "": Say that residue has a flanking
   // residue that is completely split, into A and B.  In that case we want
   // either "" or "A" for the flanking atoms.
   // 
   // And say we want to refine the A alt conf of a completely split residue
   // that has a flanking neighbour that is completely unsplit (""), we want
   // atoms that are either "A" or "".
   // 
   // So let's try setting whole_res_flag to 1 for flanking residues.

   CResidue *r;
   for (int ires=start_offset; ires<(nSelResidues + end_offset); ires++) { 

      if ( (ires == 0) || (ires == nSelResidues -1) ) { 
	 if (! residue_from_alt_conf_split_flag)
	    whole_res_flag = 1;
      } else { 
	 whole_res_flag = 0;
      }
      
      r = coot::util::deep_copy_this_residue(SelResidues[ires], altconf,
					     whole_res_flag);
      chain->AddResidue(r);
      r->seqNum = SelResidues[ires]->GetSeqNum();
      r->SetResName(SelResidues[ires]->GetResName());
   }
   chain->SetChainID(chain_id_1.c_str());
   model->AddChain(chain);
   residues_mol->AddModel(model);
   residues_mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
   residues_mol->FinishStructEdit();

   return residues_mol;
}


// merge molecules
// Return +1 as status of pair if we did indeed do a merge
//
// Note, very often add_molecules will only be of size 1.
//
// Recall that we will often be merging ligands into (otherwise quite
// complete) proteins.  In that case, in which we add a single residue
// it would be The Right Thing to Do if we could find a chain that
// consisted only of the same residue type as is the new ligand and
// add it to that chain.
//
// If we can't find a chain like that - or the new molecule contains
// more than one residue, we simply add new chains (with new chain
// ids) to this molecule.
//
// Question to self: how do I deal with different models?
//

std::pair<int, std::vector<std::string> > 
molecule_class_info_t::merge_molecules(const std::vector<atom_selection_container_t> &add_molecules) {

   int istat = 0;
   std::vector<std::string> resulting_chain_ids;

   std::vector<std::string> this_model_chains = coot::util::chains_in_molecule(atom_sel.mol);

   for (unsigned int imol=0; imol<add_molecules.size(); imol++) {
      if (add_molecules[imol].n_selected_atoms > 0) {
	 int nresidues = coot::util::number_of_residues_in_molecule(add_molecules[imol].mol);

	 // we need to set add_by_chain_flag appropriately.

	 short int add_by_chain_flag = 1;
	 std::vector<std::string> adding_model_chains
	    = coot::util::chains_in_molecule(add_molecules[imol].mol);
	 if (nresidues == 1) {

	    std::cout << "Doing single residue merge..." << std::endl;
	    // If there is a chain that has only residues of the same
	    // type as is the (single) residue in the new adding
	    // molecule then we add by residue addition to chain
	    // rather than add by chain (to molecule).

	    // Are there chains in this model that only consist of
	    // residues of type adding_model_chains[0]?
	    short int has_single_residue_type_chain_flag = 0;

	    int i_this_model = 1;

	    CModel *this_model_p;
	    CChain *this_chain_p;
	    CChain *add_residue_to_this_chain = NULL;

	    this_model_p = atom_sel.mol->GetModel(i_this_model);
	    
	    int n_this_mol_chains = this_model_p->GetNumberOfChains();
	    
	    for (int ithischain=0; ithischain<n_this_mol_chains; ithischain++) { 
	       this_chain_p = this_model_p->GetChain(ithischain);
	       std::vector<std::string> r = coot::util::residue_types_in_chain(this_chain_p);
	       if (r.size() == 1) {
		  if (r[0] == adding_model_chains[0]) {
		     add_residue_to_this_chain = this_chain_p;
		     has_single_residue_type_chain_flag = 1;
		     break;
		  }
	       } 
	    }

	    if (has_single_residue_type_chain_flag) {
	       if (add_molecules[imol].n_selected_atoms > 0) {
		  CResidue *add_model_residue = add_molecules[imol].atom_selection[0]->residue;
		  copy_and_add_residue_to_chain(add_residue_to_this_chain, add_model_residue);
		  add_by_chain_flag = 0;
	       }
	    } else {
	       add_by_chain_flag = 1;
	    } 
	    
	 } else {
	    add_by_chain_flag = 1;
	 }

	 // Now that add_by_chain_flag has been set properly, we use it...
	 
	 if (add_by_chain_flag) { 

	    std::vector<std::string> mapped_chains =
	       map_chains_to_new_chains(adding_model_chains, this_model_chains);

	    std::cout << "INFO:: Merge From chains: " << std::endl;
	    for (unsigned int ich=0; ich<adding_model_chains.size(); ich++)
	       std::cout << " :" << adding_model_chains[ich] << ":";
	    std::cout << std::endl;
	    std::cout << "INFO:: Merge To chains: " << std::endl;
	    for (unsigned int ich=0; ich<mapped_chains.size(); ich++)
	       std::cout << " :" << mapped_chains[ich] << ":";
	    std::cout << std::endl;

	    if (mapped_chains.size() != adding_model_chains.size()) {
	       // can't continue with merging - no chains available.
	       std::cout << "can't continue with merging - no chains available." << std::endl;
	    } else {
	       // fine, continue

	       // Add the chains of the new molecule to this atom_sel, chain by chain.
	       CModel *model_p;
	       CModel *this_model_p;
	       CChain *chain_p;
	       CChain *copy_chain_p;
	       int i_add_model = 1;
	       int i_this_model = 1;

	       model_p = add_molecules[imol].mol->GetModel(i_add_model);
	       this_model_p = atom_sel.mol->GetModel(i_this_model);
	       
	       int n_add_chains = model_p->GetNumberOfChains();

	       for (int iaddchain=0; iaddchain<n_add_chains; iaddchain++) {
		  chain_p = model_p->GetChain(iaddchain);
		  copy_chain_p = new CChain;
		  copy_chain_p->Copy(chain_p);
		  copy_chain_p->SetChainID(mapped_chains[iaddchain].c_str());
		  this_model_p->AddChain(copy_chain_p);
		  this_model_chains.push_back(mapped_chains[iaddchain].c_str());
		  resulting_chain_ids.push_back(mapped_chains[iaddchain]);
	       }

	       if (n_add_chains > 0) {
		  atom_sel.mol->FinishStructEdit();
		  update_molecule_after_additions();
	       } 
	       istat = 1;
	    }
	 }
      } 
   }
   return std::pair<int, std::vector<std::string> > (istat, resulting_chain_ids);
} 


// Merge molecules helper function.
//
// What we want to do is map chains ids in the new (adding) molecules
// to chain ids that are unused in thid model:
//
// e.g. adding:  B,C,D
//      this:    A,B,C
//
//      return:  D,E,F
// 
std::vector<std::string>
molecule_class_info_t::map_chains_to_new_chains(const std::vector<std::string> &adding_model_chains,
						const std::vector<std::string> &this_model_chains) const {

   std::vector<std::string> rv;
   // we dont want ! [] or *, they are mmdb specials.
   std::string r("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%^&@?/~|-+=(){}:;.,'");

   // first remove from r, all the chain that already exist in this molecule.
   for (unsigned int i=0; i<this_model_chains.size(); i++) {
      // but only do that if the chain id is of length 1 (single character)
      if (this_model_chains[i].length() == 1) {
	 std::string::size_type found_this_model_chain = r.find(this_model_chains[i]);
	 if (found_this_model_chain != std::string::npos) {
	    // there was a match
	    r = coot::util::remove_string(r, this_model_chains[i]);
	 } else {
	    // else there was not a match, this chain id does not
	    // exist in r (surprisingly).
	 } 
      }
   }
   
   for (unsigned int i=0; i<adding_model_chains.size(); i++) {

      std::string t = "A";
      std::cout << "finding new chain id for chain id :" << adding_model_chains[i] << ": "
		<< i << "/" << adding_model_chains.size() << std::endl;

      if (r.length() > 0) { 
	 t = r[0];
	 r = r.substr(1); // return r starting at position 1;
      } else {
	 t = "A";
      }
      rv.push_back(t);
   }
   return rv;
}

// This doesn't do a backup or finalise model.
void
molecule_class_info_t::copy_and_add_residue_to_chain(CChain *this_model_chain,
						     CResidue *add_model_residue) {

   if (add_model_residue) {
      short int whole_res_flag = 1;
      int udd_atom_index_handle = 1; // does this matter?
      CResidue *residue_copy = coot::deep_copy_this_residue(add_model_residue,
							    "",
							    whole_res_flag,
							    udd_atom_index_handle);

      std::pair<short int, int> max_res_info = next_residue_in_chain(this_model_chain);
      int new_res_resno = 9999;
      if (max_res_info.first)
	 new_res_resno = max_res_info.second;
      this_model_chain->AddResidue(residue_copy);
      residue_copy->seqNum = new_res_resno;
   }
}


int
molecule_class_info_t::renumber_residue_range(const std::string &chain_id,
					      int start_resno, int last_resno, int offset) {

   int i = 0;

   if (start_resno > last_resno) {
      int tmp = start_resno;
      start_resno = last_resno;
      last_resno = tmp;
   }

   if (atom_sel.n_selected_atoms > 0) {
      CModel *model_p = atom_sel.mol->GetModel(1);
      CChain *chain_p;
      int n_chains = model_p->GetNumberOfChains(); 
      for (int i_chain=0; i_chain<n_chains; i_chain++) {
	 chain_p = model_p->GetChain(i_chain);
	 std::string mol_chain(chain_p->GetChainID());
	 if (mol_chain == chain_id) {
	    // std::cout << "DEBUG:: Found chain_id " << chain_id << std::endl;
	    int nres = chain_p->GetNumberOfResidues();
	    CResidue *residue_p;
	    for (int ires=0; ires<nres; ires++) { // ires is a serial number
	       residue_p = chain_p->GetResidue(ires);
	       if (residue_p->seqNum >= start_resno) {
		  if (residue_p->seqNum <= last_resno) {
		     residue_p->seqNum += offset;
		     i = 1; // found one residue at least.
		  }
	       }
	    }
	 }
      }
   }
   if (i) {
      have_unsaved_changes_flag = 1;
      // need to redraw the bonds:
      make_bonds_type_checked();
   } 
   return i;
}

int
molecule_class_info_t::change_residue_number(const std::string &chain_id,
					     int current_resno,
					     const std::string &current_inscode_str,
					     int new_resno,
					     const std::string &new_inscode_str) {

   int done_it = 0;
   if (atom_sel.n_selected_atoms > 0) {
      CModel *model_p = atom_sel.mol->GetModel(1);
      CChain *chain_p;
      int n_chains = model_p->GetNumberOfChains(); 
      for (int i_chain=0; i_chain<n_chains; i_chain++) {
	 chain_p = model_p->GetChain(i_chain);
	 std::string mol_chain(chain_p->GetChainID());
	 if (mol_chain == chain_id) {
	    std::cout << "DEBUG:: Found chain_id " << chain_id << std::endl;
	    int nres = chain_p->GetNumberOfResidues();
	    CResidue *residue_p;
	    for (int ires=0; ires<nres; ires++) { // ires is a serial number
	       residue_p = chain_p->GetResidue(ires);
	       if (residue_p->GetSeqNum() == current_resno) {
		  std::string inscode(residue_p->GetInsCode());
		  if (inscode == current_inscode_str) {
		     residue_p->seqNum = new_resno;
		     strncpy(residue_p->insCode, new_inscode_str.c_str(), 2);
		     done_it = 1;
		  }
	       }
	    }
	 }
      }
   }
   return done_it; 
} 



// add OXT
std::pair<short int, int>
molecule_class_info_t::last_residue_in_chain(const std::string &chain_id) const {

   std::pair<short int, int> p(0,0);
   int biggest_resno = -99999;

   if (atom_sel.n_selected_atoms > 0) {
      CModel *model_p = atom_sel.mol->GetModel(1);
      CChain *chain_p;
      int n_chains = model_p->GetNumberOfChains(); 
      for (int i_chain=0; i_chain<n_chains; i_chain++) {
	 chain_p = model_p->GetChain(i_chain);
	 std::string mol_chain(chain_p->GetChainID());
	 if (mol_chain == chain_id) {
	    int nres = chain_p->GetNumberOfResidues();
	    CResidue *residue_p;
	    for (int ires=0; ires<nres; ires++) { // ires is a serial number
	       residue_p = chain_p->GetResidue(ires);
	       if (residue_p->GetSeqNum() > biggest_resno) {
		  biggest_resno = residue_p->GetSeqNum();
		  p.first = 1;
	       }
	    }
	 }
      }
   }
   p.second = biggest_resno;
   return p;
}

std::pair<short int, int> 
molecule_class_info_t::first_residue_in_chain(const std::string &chain_id) const { 

   std::pair<short int, int> p(0,0);
   int smallest_resno = 999999;

   if (atom_sel.n_selected_atoms > 0) {
      CModel *model_p = atom_sel.mol->GetModel(1);
      CChain *chain_p;
      int n_chains = model_p->GetNumberOfChains(); 
      for (int i_chain=0; i_chain<n_chains; i_chain++) {
	 chain_p = model_p->GetChain(i_chain);
	 std::string mol_chain(chain_p->GetChainID());
	 if (mol_chain == chain_id) {
	    int nres = chain_p->GetNumberOfResidues();
	    CResidue *residue_p;
	    for (int ires=0; ires<nres; ires++) { // ires is a serial number
	       residue_p = chain_p->GetResidue(ires);
	       if (residue_p->GetSeqNum() < smallest_resno) {
		  smallest_resno = residue_p->GetSeqNum();
		  p.first = 1;
	       }
	    }
	 }
      }
   }
   p.second = smallest_resno;
   return p;
}




// validation

void 
molecule_class_info_t::find_deviant_geometry(float strictness) { 

#ifdef HAVE_GSL   
   if (atom_sel.n_selected_atoms > 0) {
      std::vector<CAtom *> fixed_atoms;
      short int have_flanking_residue_at_end = 0;
      short int have_flanking_residue_at_start = 0;
      // int resno_1, resno_2;
      
      CModel *model_p = atom_sel.mol->GetModel(1);
      CChain *chain_p;
      int n_chains = model_p->GetNumberOfChains(); 
      for (int i_chain=0; i_chain<n_chains; i_chain++) {
	 chain_p = model_p->GetChain(i_chain);
	 std::string mol_chain(chain_p->GetChainID());

	 std::pair<short int, int> resno_1 = first_residue_in_chain(mol_chain);
	 std::pair<short int, int> resno_2 =  last_residue_in_chain(mol_chain);

	 if (! (resno_1.first && resno_2.first)) { 
	    std::cout << "WARNING: Error getting residue ends in find_deviant_geometry\n";
	 } else { 
	    
	    short int have_disulfide_residues = 0;
	    std::string altconf = "";

	    int selHnd = atom_sel.mol->NewSelection();
	    int nSelResidues;
	    PCResidue *SelResidues = NULL;

	    atom_sel.mol->Select(selHnd, STYPE_RESIDUE, 0,
				 (char *) mol_chain.c_str(),
				 resno_1.second, "*",
				 resno_2.second, "*",
				 "*",  // residue name
				 "*",  // Residue must contain this atom name?
				 "*",  // Residue must contain this Element?
				 "*",  // altLocs
				 SKEY_NEW // selection key
				 );
	    atom_sel.mol->GetSelIndex(selHnd, SelResidues, nSelResidues);

	    // kludge in a value for icheck.
	    std::vector<std::string> kludge;
	    std::pair<int, std::vector<std::string> > icheck(1, kludge);

	    // coot::util::check_dictionary_for_residues(SelResidues, nSelResidues,
	    //  					 graphics_info_t::Geom_p());

	    if (icheck.first == 0) { 
	       for (unsigned int icheck_res=0; icheck_res<icheck.second.size(); icheck_res++) { 
		  std::cout << "WARNING:: Failed to find restraints for " 
			    << icheck.second[icheck_res] << std::endl;
	       }
	    }

	    std::cout << "INFO:: " << nSelResidues
		      << " residues selected for deviant object" << std::endl;

	    if (nSelResidues > 0) {

	       CMMDBManager *residues_mol = 
	       create_mmdbmanager_from_res_selection(SelResidues, nSelResidues, 
						     have_flanking_residue_at_start,
						     have_flanking_residue_at_end,
						     altconf, 
						     (char *) mol_chain.c_str(), 
						     0 // 0 because we are not in alt conf split
						     );
	       coot::restraints_container_t 
		  restraints(resno_1.second,
			     resno_2.second,
			     have_flanking_residue_at_start,
			     have_flanking_residue_at_end,
			     have_disulfide_residues,
			     altconf,
			     (char *) mol_chain.c_str(),
			     residues_mol,
			     fixed_atoms);
	    }
	 }
      }
   }
#endif // HAVE_GSL   
}


// ------------------------------------------------------------------
//                       sequence assignment
// ------------------------------------------------------------------

#include "sequence-assignment.hh"

void
molecule_class_info_t::assign_sequence(const clipper::Xmap<float> &xmap,
				       const std::string &chain_id) {

   coot::sequence_assignment::side_chain_score_t scs;

   std::string sequence_chain_id("A");

   std::string fasta_seq;
   for (unsigned int i=0; i<input_sequence.size(); i++) {
      if (input_sequence[i].first == sequence_chain_id){
	 scs.add_fasta_sequence(sequence_chain_id, input_sequence[i].second);
      }
   }
}


// Distances 
std::vector<clipper::Coord_orth>
molecule_class_info_t::distances_to_point(const clipper::Coord_orth &pt,
					  double min_dist,
					  double max_dist) {

   std::vector<clipper::Coord_orth> v;
   if (atom_sel.n_selected_atoms > 0) {
      for (int iat=0; iat<atom_sel.n_selected_atoms; iat++) {
	 clipper::Coord_orth atp(atom_sel.atom_selection[iat]->x,
				 atom_sel.atom_selection[iat]->y,
				 atom_sel.atom_selection[iat]->z);
	 if (clipper::Coord_orth::length(pt, atp) <= max_dist) {
	    if (clipper::Coord_orth::length(pt, atp) >= min_dist) {
	       v.push_back(atp);
	    }
	 }
      }
   }
   return v;

}

// logical_operator_and_or_flag 0 for AND 1 for OR.
// 
//
// If outlier_sigma_level is less than -50 then don't test for sigma level
// If min_dist < 0, don't test for min dist
// if max_dist < 0, don't test for max dist
// if b_factor_lim < 0, don't test for b_factor 
// 
std::vector <coot::atom_spec_t>
molecule_class_info_t::find_water_baddies(float b_factor_lim, const clipper::Xmap<float> &xmap_in,
					  float map_sigma,
					  float outlier_sigma_level,
					  float min_dist, float max_dist,
					  short int ignore_part_occ_contact_flag,
					  short int ignore_zero_occ_flag,
					  short int logical_operator_and_or_flag) {

   if (logical_operator_and_or_flag == 0)
      return find_water_baddies_AND(b_factor_lim, xmap_in, map_sigma, outlier_sigma_level,
				    min_dist, max_dist, ignore_part_occ_contact_flag,
				    ignore_zero_occ_flag);
   else
      return find_water_baddies_OR(b_factor_lim, xmap_in, map_sigma, outlier_sigma_level,
				   min_dist, max_dist, ignore_part_occ_contact_flag,
				   ignore_zero_occ_flag);

}


// This is logical opertator AND on the search criteria.
// 
std::vector <coot::atom_spec_t>
molecule_class_info_t::find_water_baddies_AND(float b_factor_lim, const clipper::Xmap<float> &xmap_in,
					      float map_sigma,
					      float outlier_sigma_level,
					      float min_dist, float max_dist,
					      short int ignore_part_occ_contact_flag,
					      short int ignore_zero_occ_flag) {

   std::vector <coot::atom_spec_t> v;
   std::vector <int> idx;
   double den;

   for (int i=0; i<atom_sel.n_selected_atoms; i++) {
      if (atom_sel.atom_selection[i]->tempFactor > b_factor_lim) {
	 std::string resname = atom_sel.atom_selection[i]->GetResName();
	 if (resname == "WAT" || resname == "HOH") { 
	    clipper::Coord_orth a(atom_sel.atom_selection[i]->x,
				  atom_sel.atom_selection[i]->y,
				  atom_sel.atom_selection[i]->z);
	    den = coot::util::density_at_point(xmap_in, a);

	    if (den > outlier_sigma_level*map_sigma) {
	       idx.push_back(i);
	    }
	 }
      }
   }

   // min_dist is the closest contact limit, i.e. atoms with distances 

   // max_dist is the maximum allowed distance to the nearest atom.

   double d;
   for (unsigned int i=0; i<idx.size(); i++) {
      clipper::Coord_orth p(atom_sel.atom_selection[idx[i]]->x,
			    atom_sel.atom_selection[idx[i]]->y,
			    atom_sel.atom_selection[idx[i]]->z);
      double dist_to_atoms_min = 99999;
      double dist_to_atoms_max = -99999;
      for (int iat=0; iat<atom_sel.n_selected_atoms; iat++) {
	 clipper::Coord_orth a(atom_sel.atom_selection[iat]->x,
			       atom_sel.atom_selection[iat]->y,
			       atom_sel.atom_selection[iat]->z);
	 d = clipper::Coord_orth::length(p,a);
	 if (d < dist_to_atoms_min)
	    dist_to_atoms_min = d;
	 if (d > dist_to_atoms_max)
	    dist_to_atoms_max = d;
      }
      if (dist_to_atoms_min < min_dist ||
	  dist_to_atoms_min > max_dist) {
	 coot::atom_spec_t atom_spec(atom_sel.atom_selection[idx[i]]);
	 v.push_back(atom_spec);
      }
   }
   return v;
}



// This is logical opertator AND on the search criteria.
// 
// If outlier_sigma_level is less than -50 then don't test for sigma level
// If min_dist < 0, don't test for min dist
// if max_dist < 0, don't test for max dist
// if b_factor_lim < 0, don't test for b_factor 
// 
std::vector <coot::atom_spec_t>
molecule_class_info_t::find_water_baddies_OR(float b_factor_lim, const clipper::Xmap<float> &xmap_in,
					     float map_in_sigma,
					     float outlier_sigma_level,
					     float min_dist, float max_dist,
					     short int ignore_part_occ_contact_flag,
					     short int ignore_zero_occ_flag) {
   
   std::vector <coot::atom_spec_t> v;

   std::vector<std::pair<CAtom *, float> > marked_for_display;

   short int this_is_marked;
   float den;
   short int use_b_factor_limit_test = 1;
   short int use_map_sigma_limit_test = 1;
   short int use_min_dist_test = 1;
   short int use_max_dist_test = 1;

   if (b_factor_lim < 0.0)
      use_b_factor_limit_test = 0;
   if (outlier_sigma_level < -50.0)
      use_map_sigma_limit_test = 0;
   if (min_dist < 0.0)
      use_min_dist_test = 0;
   if ( max_dist < 0.0 )
      use_max_dist_test = 0;

//    std::cout << "DEBUG:: passed: b_factor_lim "
// 	     << b_factor_lim << " outlier_sigma_level "
//  	     << outlier_sigma_level << " min_dist "
// 	     << min_dist << " max_dist " << max_dist
//  	     << std::endl;

//     std::cout << "DEBUG:: Usage flags b-factor: " << use_b_factor_limit_test
// 	     << " map sigma: " << use_map_sigma_limit_test
//  	     << " min dist: " << use_min_dist_test
//  	     << " max_dist: " << use_max_dist_test << std::endl;
       
   if (atom_sel.n_selected_atoms > 0) { 
      
      int n_models = atom_sel.mol->GetNumberOfModels();
      for (int imod=1; imod<=n_models; imod++) { 
	 
	 CModel *model_p = atom_sel.mol->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol
	 int nchains = model_p->GetNumberOfChains();
	 if (nchains <= 0) { 
	    std::cout << "bad nchains in molecule " << nchains
		      << std::endl;
	 } else { 
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);
	       if (chain_p == NULL) {  
		  // This should not be necessary. It seem to be a
		  // result of mmdb corruption elsewhere - possibly
		  // DeleteChain in update_molecule_to().
		  std::cout << "NULL chain in ... " << std::endl;
	       } else { 
		  int nres = chain_p->GetNumberOfResidues();
		  PCResidue residue_p;
		  CAtom *at;
		  for (int ires=0; ires<nres; ires++) { 
		     residue_p = chain_p->GetResidue(ires);
		     int n_atoms = residue_p->GetNumberOfAtoms();
		     
		     std::string resname = residue_p->GetResName();
		     if (resname == "WAT" || resname == "HOH") { 
			
			for (int iat=0; iat<n_atoms; iat++) {
			   at = residue_p->GetAtom(iat);
			   this_is_marked = 0;
			   
			   // density check:
			   if (map_in_sigma > 0.0) { // it *should* be!
			      clipper::Coord_orth a(at->x, at->y, at->z);
			      den = coot::util::density_at_point(xmap_in, a);
			      
			      den /= map_in_sigma;
			      if (den < outlier_sigma_level && use_map_sigma_limit_test) {
				 this_is_marked = 1;
				 marked_for_display.push_back(std::pair<CAtom *, float>(at, den));
			      }
			   } else {
			      std::cout << "Ooops! Map sigma is " << map_in_sigma << std::endl;
			   }
			   
			   // B factor check:
			   if (this_is_marked == 0) {
			      if (at->tempFactor > b_factor_lim && use_b_factor_limit_test) {
				 marked_for_display.push_back(std::pair<CAtom *, float>(at, den));
			      }
			   }


			   // distance check
			   if (this_is_marked == 0) {

			      // (ignoring things means less marked atoms)
			      if (ignore_part_occ_contact_flag==0) { 

				 // we want mark as a baddie if ignore Zero Occ is off (0)
				 //
				 if (ignore_zero_occ_flag==0 || at->occupancy < 0.01) { 
				    double dist_to_atoms_min = 99999;
				    double d;
				    clipper::Coord_orth a(at->x, at->y, at->z);
				    for (int j=0; j<atom_sel.n_selected_atoms; j++) {
				       if (at != atom_sel.atom_selection[j]) {
					  clipper::Coord_orth p(atom_sel.atom_selection[j]->x,
								atom_sel.atom_selection[j]->y,
								atom_sel.atom_selection[j]->z);
					  d = clipper::Coord_orth::length(p,a);
					  if (d < dist_to_atoms_min)
					     dist_to_atoms_min = d;
				       }
				    }
				    short int failed_min_dist_test = 0;
				    short int failed_max_dist_test = 0;

				    if ((dist_to_atoms_min < min_dist) && use_min_dist_test)
				       failed_min_dist_test = 1;

				    if ((dist_to_atoms_min > max_dist) && use_max_dist_test)
				       failed_max_dist_test = 1;
				       
				    if (failed_min_dist_test || failed_max_dist_test) {

				       marked_for_display.push_back(std::pair<CAtom *, float>(at, den));
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
      }
   }
   
   for (unsigned int i=0; i<marked_for_display.size(); i++) {
      std::string s = "B fac: ";
      s += coot::util::float_to_string(marked_for_display[i].first->tempFactor);
      if (map_in_sigma > 0.0) { 
	 s += " ED: ";
	 s += coot::util::float_to_string(marked_for_display[i].second);
	 s += " sigma";
      }
      coot::atom_spec_t as(marked_for_display[i].first, s);
      as.float_user_data = marked_for_display[i].first->occupancy;
      v.push_back(as);
   }
   return v;
}


// shelx stuff
// 
std::pair<int, std::string>
molecule_class_info_t::write_shelx_ins_file(const std::string &filename) {

   // std::cout << "DEBUG:: Pre write shelx: "<< std::endl;
   shelxins.debug();
   std::pair<int, std::string> p(1, "");
   
   if (atom_sel.n_selected_atoms > 0) { 
      p = shelxins.write_ins_file(atom_sel.mol, filename);
   } else {
      p.second = "WARNING:: No atoms to write!";
   } 
   return p;
}


// Return a variable like reading a pdb file (1 on success, -1 on failure)
// 
// This function doesn't get called by the normal handle_read_draw_molecule()
//
int
molecule_class_info_t::read_shelx_ins_file(const std::string &filename) {

   // returned a pair: status (0: bad), udd_afix_handle (-1 bad)

   int istat = 1;
   coot::shelx_read_file_info_t p = shelxins.read_file(filename);
   if (p.status == 0) { 
      std::cout << "WARNING:: bad status in read_shelx_ins_file" << std::endl;
      istat = -1;
   } else {


      int udd_afix_handle = p.mol->GetUDDHandle(UDR_ATOM, "shelx afix");
      // std::cout << "DEBUG:: in  get_atom_selection udd_afix_handle is "
      // << udd_afix_handle << " and srf.udd_afix_handle was "
      // << udd_afix_handle << std::endl;

      if (p.udd_afix_handle == -1) {
	 std::cout << "ERROR:: bad udd_afix_handle in read_shelx_ins_file"
		   << std::endl;
	 istat = -1;
      } 

      if (istat == 1) { 
	 // initialize some things.
	 //
	 atom_sel = make_asc(p.mol);
	 // FIXME? 20070721, shelx presentation day
	 // 	 fix_hydrogen_names(atom_sel); // including change " H0 " to " H  "
	 short int is_undo_or_redo = 0;
	 graphics_info_t g;

	 mat44 my_matt;
	 int err = atom_sel.mol->GetTMatrix(my_matt, 0, 0, 0, 0);
	 if (err != SYMOP_Ok) {
	    cout << "!! Warning:: No symmetry available for this molecule"
		 << endl;
	 } else { 
	    cout << "Symmetry available for this molecule" << endl;
	 }
	 is_from_shelx_ins_flag = 1;
      
	 set_have_unit_cell_flag_maybe();

	 if (molecule_is_all_c_alphas()) {
	    ca_representation();
	 } else {

	    short int do_rtops_flag = 0;

	    // Hmmm... why is this commented?  Possibly from ncs ghost
	    // toubles from many months ago?
	    
	    // 0.7 is not used (I think) if do_rtops_flag is 0
	    // int nghosts = fill_ghost_info(do_rtops_flag, 0.7);
	    // std::cout << "INFO:: found " << nghosts << " ghosts\n";

	    // I'll reinstate it.
	    int nmodels = atom_sel.mol->GetNumberOfModels();
	    if (nmodels == 1) { 
	       int nghosts = fill_ghost_info(do_rtops_flag, 0.7);
	    }

	    // Turn off hydrogen display if this is a protein
	    // (currently the hydrogen names are different from a
	    // shelx molecule, leading to a mess when refining.  So
	    // let's just undisplay those hydrogens :) 20070721
	    
	    if (p.is_protein_flag)
	       set_draw_hydrogens_state(0);

	    if (! is_undo_or_redo) 
	       bond_width = g.default_bond_width; // bleugh, perhaps this should
	                                          // be a passed parameter?
	    
	    // Generate bonds and save them in the graphical_bonds_container
	    // which has static data members.
	    //
	    if (bonds_box_type == coot::UNSET_TYPE)
	       bonds_box_type = coot::NORMAL_BONDS;
	    make_bonds_type_checked();

	 }
	 // debug();

	 short int reset_rotation_centre = 1;
	 //
	 if (g.recentre_on_read_pdb || g.n_molecules == 0)  // n_molecules
	    // is updated
	    // in
	    // c-interface.cc
	    if (reset_rotation_centre) 
	       g.setRotationCentre(::centre_of_molecule(atom_sel)); 

	 drawit = 1;
	 if (graphics_info_t::show_symmetry == 1) {
	    update_symmetry();
	 }
	 initialize_coordinate_things_on_read_molecule_internal(filename,
								is_undo_or_redo);
      }

      //    std::cout << "DEBUG:: Post read shelx: "<< std::endl;
      //    shelxins.debug();

      // save state strings
      save_state_command_strings_.push_back("read-shelx-ins-file");
      save_state_command_strings_.push_back(single_quote(filename));
   }
   return istat;
}

int
molecule_class_info_t::add_shelx_string_to_molecule(const std::string &str) {

   int istat = 0;
   if (is_from_shelx_ins_flag) {
      shelxins.add_pre_atoms_line(str);
      istat = 1;
   }
   return istat;
}


// -----------------------------------------------------------------
//              display list lovelies
// -----------------------------------------------------------------


int 
molecule_class_info_t::draw_display_list_objects() {

   int n_objects = 0;
   if (drawit) { 
      if (display_list_tags.size() > 0) { 
	 glEnable(GL_LIGHTING);
	 std::list<coot::display_list_object_info>::const_iterator it;
	 for (it=display_list_tags.begin(); it!=display_list_tags.end(); it++) {
	    n_objects++;
	    glCallList(it->tag);
	 }
	 glDisable(GL_LIGHTING);
      }
   }
   return n_objects;
}

// return the display list tag
int
molecule_class_info_t::make_ball_and_stick(const std::string &atom_selection_str,
					   float bond_thickness, float sphere_size,
					   short int do_spheres_flag) {

   int i= -1;
   if (has_model()) {
      int SelHnd = atom_sel.mol->NewSelection();
      atom_sel.mol->Select(SelHnd, STYPE_ATOM,
			   (char *) atom_selection_str.c_str(), // sigh...
			   SKEY_OR);
      int n_selected_atoms;
      PPCAtom atom_selection = NULL;
      atom_sel.mol->GetSelIndex(SelHnd, atom_selection, n_selected_atoms);
      atom_selection_container_t asc = atom_sel;
      asc.atom_selection = atom_selection;
      asc.n_selected_atoms = n_selected_atoms;
      asc.SelectionHandle = SelHnd;
      std::cout << "INFO:: " << n_selected_atoms
		<< " atoms selected for ball & stick" << std::endl;
      float min_dist = 0.1;
      float max_dist = 1.9; // prevent ambiguity in constructor def.
      Bond_lines_container bonds(asc, min_dist, max_dist);
      graphical_bonds_container bonds_box_local = bonds.make_graphical_bonds();
      Lines_list ll;
      // start display list object
      GLuint bonds_tag = glGenLists(1);
      glNewList(bonds_tag, GL_COMPILE);
      coot::display_list_object_info dloi;
      dloi.tag = bonds_tag;
      display_list_tags.push_back(dloi);

      GLfloat bgcolor[4]={1.0, 1.0, 0.3, 1.0};
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      // glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, bgcolor);
      glMaterialfv(GL_FRONT, GL_SPECULAR, bgcolor);
      // glMaterialfv(GL_FRONT, GL_EMISSION, bgcolor);
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 128); 
      //Let the returned colour dictate: note obligatory order of these calls
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);

      GLfloat  mat_specular[]  = {1.0, 0.3, 0.2, 1.0};
      GLfloat  mat_ambient[]   = {0.8, 0.1, 0.1, 1.0};
      GLfloat  mat_diffuse[]   = {0.2, 0.2, 0.2, 0.5};
      GLfloat  mat_shininess[] = {50.0};
      // GLfloat  light_position[] = {1.0, 1.0, 1.0, 1.0};
      
      glClearColor(0.0, 0.0, 0.0, 0.0);
      glShadeModel(GL_SMOOTH);

      glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
      glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
      glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
      glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
      // glLightfv(GL_LIGHT0,  GL_POSITION, light_position);

      //       glEnable(GL_LIGHTING);
      //       glEnable(GL_LIGHT0);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_NORMALIZE);
      
      for (int ii=0; ii<bonds_box_local.num_colours; ii++) {
	 ll = bonds_box_local.bonds_[ii];
	 set_bond_colour_by_mol_no(ii);
	 //	 GLfloat bgcolor[4]={1.0,1.0,0.3,1.0};
	 GLfloat bgcolor[4]={bond_colour_internal[0],
			     bond_colour_internal[1],
			     bond_colour_internal[2],
			     1.0};
	 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	 // glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, bgcolor);
	 glMaterialfv(GL_FRONT, GL_SPECULAR, bgcolor);
	 // glMaterialfv(GL_FRONT, GL_EMISSION, bgcolor);
	 for (int j=0; j< bonds_box_local.bonds_[ii].num_lines; j++) {
	    glPushMatrix();
	    glTranslatef(ll.pair_list[j].getFinish().get_x(),
			 ll.pair_list[j].getFinish().get_y(),
			 ll.pair_list[j].getFinish().get_z());
	    double base = bond_thickness;
	    double top = bond_thickness;
	    coot::Cartesian bond_height =
	       ll.pair_list[j].getFinish() - ll.pair_list[j].getStart();
	    double height = bond_height.amplitude();
	    int slices = 10;
	    int stacks = 2;

	    // 	    This code from cc4mg's cprimitive.cc (but modified)
	    //  	    ----- 
	    double ax;
	    double rx = 0; 
	    double ry = 0;
	    double length = bond_height.length();
	    double vz = bond_height.get_z();

	    bool rot_x = false;
	    if(fabs(vz)>1e-7){
	       ax = 180.0/M_PI*acos(vz/length);
	       if(vz<0.0) ax = -ax;
	       rx = -bond_height.get_y()*vz;
	       ry = bond_height.get_x()*vz;
	    }else{
	       double vx = bond_height.get_x();
	       double vy = bond_height.get_y();
	       ax = 180.0/M_PI*acos(vx/length);
	       if(vy<0) ax = -ax;
	       rot_x = true;
	    }

	    if (rot_x) { 
	       glRotated(90.0, 0.0, 1.0, 0.0);
	       glRotated(ax,  -1.0, 0.0, 0.0);
	    } else {
	       glRotated(ax, rx, ry, 0.0);
	    }
	    // 	    --------

	    GLUquadric* quad = gluNewQuadric();
	    glScalef(1.0, 1.0, -1.0); // account for mg maths :-)
	    gluCylinder(quad, base, top, height, slices, stacks);
	    // gluQuadricNormals(quad, GL_SMOOTH);
	    gluDeleteQuadric(quad);
	    glPopMatrix();
	 }
      }

      if (do_spheres_flag) { 
	 int slices = 10;
	 int stacks = 20;
	 for (int i=0; i<bonds_box_local.n_atom_centres_; i++) {
	    set_bond_colour_by_mol_no(bonds_box.atom_centres_colour_[i]);
	    glPushMatrix();
	    glTranslatef(bonds_box_local.atom_centres_[i].get_x(),
			 bonds_box_local.atom_centres_[i].get_y(),
			 bonds_box_local.atom_centres_[i].get_z());

	    GLUquadric* quad = gluNewQuadric();
	    gluSphere(quad, sphere_size, slices, stacks);
	    // gluQuadricNormals(quad, GL_SMOOTH);
	    gluDeleteQuadric(quad);
	    glPopMatrix();
	 }
      }
      
      glEndList();
      bonds_box_local.clear_up();
      atom_sel.mol->DeleteSelection(SelHnd);
   }
   return i;
}

void
molecule_class_info_t::clear_display_list_object(GLuint tag) {

   // actually, clear them all, not just those (or that one) with tag:

//    std::list<coot::display_list_object_info>::const_iterator it;
//    for (it=display_list_tags.begin(); it!=display_list_tags.end(); it++) {
//    }

   display_list_tags.resize(0);
}


void
molecule_class_info_t::set_occupancy_residue_range(const std::string &chain_id, int ires1, int ires2, float occ_val) {

   if (ires2 < ires1) {
      int tmp = ires1;
      ires1 = ires2;
      ires2 = tmp;
   }

   PPCAtom SelAtoms = NULL;
   int nSelAtoms;
   int SelHnd = atom_sel.mol->NewSelection();
   
   atom_sel.mol->SelectAtoms(SelHnd, 0, (char *) chain_id.c_str(),
			     ires1, "*",
			     ires2, "*",
			     "*", // residue name
			     "*",
			     "*", // elements
			     "*");

   atom_sel.mol->GetSelIndex(SelHnd, SelAtoms, nSelAtoms);

   if (nSelAtoms == 0) { 
      std::cout << "Sorry. Could not find residue range " << ires1
		<< " to " << ires2 << " in this molecule: ("
		<<  imol_no << ") " << name_ << std::endl; 
   } else {
      for (int i=0; i<nSelAtoms; i++)
	 SelAtoms[i]->occupancy = occ_val;
      atom_sel.mol->DeleteSelection(SelHnd);
      have_unsaved_changes_flag = 1;
      make_bonds_type_checked();
   }
}


// Change chain id
// return -1 on a conflict
// 1 on good.
// 0 on did nothing
// 
std::pair<int, std::string>
molecule_class_info_t::change_chain_id(const std::string &from_chain_id,
				       const std::string &to_chain_id,
				       short int use_resno_range,
				       int start_resno,
				       int end_resno) {
   int istat = 0;
   std::string message("Nothing to say");

   //    std::cout << "DEBUG:: use_resno_range: " << use_resno_range << std::endl;

   if (atom_sel.n_selected_atoms > 0) { 
	 
      if (use_resno_range == 1) {

	 std::pair<int, std::string> r =
	    change_chain_id_with_residue_range(from_chain_id, to_chain_id, start_resno, end_resno);
	 istat = r.first;
	 message = r.second;

      } else { 
      // The usual case, I imagine

	 short int target_chain_id_exists = 0;
      
	 int n_models = atom_sel.mol->GetNumberOfModels();
	 for (int imod=1; imod<=n_models; imod++) { 
	    
	    CModel *model_p = atom_sel.mol->GetModel(imod);
	    CChain *chain_p;
	    // run over chains of the existing mol
	    int nchains = model_p->GetNumberOfChains();
	    if (nchains <= 0) { 
	       std::cout << "bad nchains in molecule " << nchains
			 << std::endl;
	    } else { 
	       for (int ichain=0; ichain<nchains; ichain++) {
		  chain_p = model_p->GetChain(ichain);
		  if (chain_p == NULL) {  
		     // This should not be necessary. It seem to be a
		     // result of mmdb corruption elsewhere - possibly
		     // DeleteChain in update_molecule_to().
		     std::cout << "NULL chain in change chain id" << std::endl;
		  } else {
		     std::string chain_id = chain_p->GetChainID();
		     if (to_chain_id == chain_id) { 
			target_chain_id_exists = 1;
			break;
		     }
		  }
	       }
	    }
	 }

	 if (target_chain_id_exists == 0) {

	    int n_models = atom_sel.mol->GetNumberOfModels();
	    for (int imod=1; imod<=n_models; imod++) { 
	    
	       CModel *model_p = atom_sel.mol->GetModel(imod);
	       CChain *chain_p;
	       // run over chains of the existing mol
	       int nchains = model_p->GetNumberOfChains();
	       if (nchains <= 0) { 
		  std::cout << "bad nchains in molecule " << nchains
			    << std::endl;
	       } else { 
		  for (int ichain=0; ichain<nchains; ichain++) {
		     chain_p = model_p->GetChain(ichain);
		     if (chain_p) {  
			std::string chain_id = chain_p->GetChainID();
			if (from_chain_id == chain_id) {
			   make_backup();
			   chain_p->SetChainID(to_chain_id.c_str());
			   istat = 1;
			   have_unsaved_changes_flag = 1;
			   atom_sel.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
			   atom_sel.mol->FinishStructEdit();
			   atom_sel = make_asc(atom_sel.mol);
			   make_bonds_type_checked();
			}
		     }
		  }
	       }
	    }

	    std::cout << "istat: ; " << istat << std::endl;

	 } else {
	    std::cout << "WARNING:: CONFLICT: target chain id already exists "
		      << "in this molecule" << std::endl;
	    message = "CONFLICT: target chain id (";
	    message += to_chain_id;
	    message += ") already \nexists in this molecule!";
	 }
      } // residue range
   } // no atoms

   return std::pair<int, std::string> (istat, message);
}


std::pair<int, std::string>
molecule_class_info_t::change_chain_id_with_residue_range(const std::string &from_chain_id,
							  const std::string &to_chain_id,
							  int start_resno,
							  int end_resno) {

   std::string message;
   int istat = 0;
   
   short int target_chain_id_exists = 0;
   int n_models = atom_sel.mol->GetNumberOfModels();
   for (int imod=1; imod<=n_models; imod++) { 
	    
      CModel *model_p = atom_sel.mol->GetModel(imod);
      CChain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      if (nchains <= 0) { 
	 std::cout << "bad nchains in molecule " << nchains
		   << std::endl;
      } else { 
	 for (int ichain=0; ichain<nchains; ichain++) {
	    chain_p = model_p->GetChain(ichain);
	    if (chain_p == NULL) {  
	       // This should not be necessary. It seem to be a
	       // result of mmdb corruption elsewhere - possibly
	       // DeleteChain in update_molecule_to().
	       std::cout << "NULL chain in change chain id" << std::endl;
	    } else {
	       std::string chain_id = chain_p->GetChainID();
	       if (to_chain_id == chain_id) { 
		  target_chain_id_exists = 1;
		  break;
	       }
	    }
	 }
      }
   }

   if (target_chain_id_exists == 0) {

      // So we are moving residues 12->24 of Chain A to (new) Chain
      // C.  Not very frequent, I suspect.

      // make sure start and end are a sensible way round
      if (end_resno < start_resno) {
	 int tmp = end_resno;
	 end_resno = start_resno;
	 start_resno = tmp;
      }

      int n_models = atom_sel.mol->GetNumberOfModels();
      for (int imod=1; imod<=n_models; imod++) { 
	    
	 CModel *model_p = atom_sel.mol->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol
	 int nchains = model_p->GetNumberOfChains();
	 if (nchains <= 0) { 
	    std::cout << "bad nchains in molecule " << nchains
		      << std::endl;
	 } else { 
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);
	       if (chain_p) {  
		  std::string chain_id = chain_p->GetChainID();
		  if (from_chain_id == chain_id) {

		     // So we have the chain from which we wish to move residues
		     make_backup();
		     atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
		     // Create a new chain and add it to the molecule
		     CChain *new_chain_p = new CChain;
		     new_chain_p->SetChainID(to_chain_id.c_str());
		     model_p->AddChain(new_chain_p);
		     
		     int nresidues = chain_p->GetNumberOfResidues();
		     for (int ires=0; ires<nresidues; ires++) {
			CResidue *residue_p = chain_p->GetResidue(ires);
			if (residue_p->GetSeqNum() >= start_resno &&
			    residue_p->GetSeqNum() <= end_resno) {
			   int iseqnum  = residue_p->GetSeqNum();
			   pstr inscode = residue_p->GetInsCode();
			   CResidue *residue_copy = coot::util::deep_copy_this_residue(residue_p, "", 1);
			   chain_p->DeleteResidue(iseqnum, inscode);
			   new_chain_p->AddResidue(residue_copy);
			}
		     }

		     istat = 1;
		     have_unsaved_changes_flag = 1;
		     atom_sel.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
		     atom_sel.mol->FinishStructEdit();
		     atom_sel = make_asc(atom_sel.mol);
		     make_bonds_type_checked();
		  }
	       }
	    }
	 }
      }
   } else {

      // target chain alread exists.   Here is where we merge...

      // We need to check that we are not reproducing residues that
      // already exist in the chain.  If we are doing that, we stop
      // and give an error message back telling user that that
      // residues exists already.
      //
      int n_models = atom_sel.mol->GetNumberOfModels();
      for (int imod=1; imod<=n_models; imod++) { 
	    
	 CModel *model_p = atom_sel.mol->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol
	 int nchains = model_p->GetNumberOfChains();
	 short int residue_already_exists_flag = 0;
	 if (nchains <= 0) { 
	    std::cout << "bad nchains in molecule " << nchains
		      << std::endl;
	 } else { 
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);
	       if (chain_p) {  
		  std::string chain_id = chain_p->GetChainID();
		  if (to_chain_id == chain_id) {
		     
		     int nresidues = chain_p->GetNumberOfResidues();
		     int existing_residue_number = 0; // set later
		     for (int ires=0; ires<nresidues; ires++) {
			CResidue *residue_p = chain_p->GetResidue(ires);
			if (residue_p->GetSeqNum() >= start_resno &&
			    residue_p->GetSeqNum() <= end_resno) {
			   residue_already_exists_flag = 1;
			   existing_residue_number = residue_p->GetSeqNum();
			   break;
			}

		     }
		     if (residue_already_exists_flag) {
			message += "CONFLICT!  Residue ";
			message += coot::util::int_to_string(existing_residue_number);
			message += " already exists in chain ";
			message += chain_id;
			message += "\nChange chain ID of residue range failed.\n";
		     } else {

			// We are OK to move the residue into the existing chain
			// (move is done by copy and delete)

			CChain *to_chain = NULL;
			for (int ichain=0; ichain<nchains; ichain++) {
			   chain_p = model_p->GetChain(ichain);
			   if (chain_p) {  
			      std::string chain_id = chain_p->GetChainID();
			      if (to_chain_id == chain_id) {
				 to_chain = chain_p;
			      }
			   }
			}

			
			if (to_chain) {
			   make_backup();
			   for (int ichain=0; ichain<nchains; ichain++) {
			      chain_p = model_p->GetChain(ichain);
			      int to_chain_nresidues = chain_p->GetNumberOfResidues();
			      if (chain_p) {  
				 std::string chain_id = chain_p->GetChainID();
				 if (from_chain_id == chain_id) {
				    for (int ires=0; ires<to_chain_nresidues; ires++) {
				       CResidue *residue_p = chain_p->GetResidue(ires);
				       if (residue_p->GetSeqNum() >= start_resno &&
					   residue_p->GetSeqNum() <= end_resno) {

					  int iseqnum  = residue_p->GetSeqNum();
					  pstr inscode = residue_p->GetInsCode();
					  CResidue *residue_copy = coot::util::deep_copy_this_residue(residue_p, "", 1);
					  // delete the residue in the "from" chain:
					  chain_p->DeleteResidue(iseqnum, inscode);

					  //
					  change_chain_id_with_residue_range_helper_insert_or_add(to_chain, residue_copy);
					  // to_chain->AddResidue(residue_copy);
				       }
				    }
				 }
			      }
			   }
			   istat = 1;
			   have_unsaved_changes_flag = 1;
			   atom_sel.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
			   atom_sel.mol->FinishStructEdit();
			   atom_sel = make_asc(atom_sel.mol);
			   make_bonds_type_checked();
			}
		     }
		  }
	       }
	       if (residue_already_exists_flag)
		  break;
	    }
	 }
	 if (residue_already_exists_flag)
	    break;
      }
   }
   
   return std::pair<int, std::string> (istat, message);

}

void
molecule_class_info_t::change_chain_id_with_residue_range_helper_insert_or_add(CChain *to_chain_p, CResidue *new_residue) {

   // OK, if we can, let's try to *insert* the new_residue into the
   // right place in the to_chain_p.  If we don't manage to insert it,
   // let's fall back and simply add it.  The new residue is inserted
   // *before* the residue specified by the given serial number.

   // Let's use the serial number interface to InsResidue()

   int resno_new_residue = new_residue->GetSeqNum();
   std::string ins_code = new_residue->GetInsCode();
   int target_res_serial_number = coot::RESIDUE_NUMBER_UNSET;
   int target_res_seq_num = resno_new_residue; // simply ignore ins codes :)
   std::string target_res_ins_code = ""; // ignore this.  Fix later.
   int best_seq_num_diff = 99999999;

   int n_chain_residues;
   PCResidue *chain_residues;
   to_chain_p->GetResidueTable(chain_residues, n_chain_residues);
   for (int iserial=0; iserial<n_chain_residues; iserial++) {
      int chain_residue_seq_num = chain_residues[iserial]->GetSeqNum();
      int this_seq_num_diff = chain_residue_seq_num - resno_new_residue;
      if (this_seq_num_diff > 0) {
	 if (this_seq_num_diff < best_seq_num_diff) {
	    best_seq_num_diff = this_seq_num_diff;
	    target_res_serial_number = iserial;
	 }
      }
   }
      
   if (target_res_serial_number != coot::RESIDUE_NUMBER_UNSET) {
      // Good stuff
      std::cout << "Debugging, inserting residue here...." << std::endl;
      to_chain_p->InsResidue(new_residue, target_res_serial_number);
   } else {
      std::cout << "Debugging, adding residue here...." << std::endl;
      to_chain_p->AddResidue(new_residue);
   } 
}




// nomenclature errors
// return a vector of the changed residues (used for updating the rotamer graph)
std::vector<CResidue *>
molecule_class_info_t::fix_nomenclature_errors() {    // by looking for bad rotamers in
				                      // some residue types and alter ing
                            			      // the atom names to see if they get
				                      // more likely rotamers

   std::vector<CResidue *> r;
   if (atom_sel.n_selected_atoms > 0) {
      graphics_info_t g;
      make_backup();
      coot::nomenclature n(atom_sel.mol);
      r = n.fix(g.Geom_p());
      have_unsaved_changes_flag = 1;
   }
   return r;
}

int
molecule_class_info_t::cis_trans_conversion(const std::string &chain_id, int resno, const std::string &inscode) {

   int imod = 1;

   bool found = 0;
   int r = 0; // returned value
   CModel *model_p = atom_sel.mol->GetModel(imod);
   CChain *chain_p;
   // run over chains of the existing mol
   int nchains = model_p->GetNumberOfChains();
   for (int ichain=0; ichain<nchains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      if (chain_id == chain_p->GetChainID()) { 
	 int nres = chain_p->GetNumberOfResidues();
	 PCResidue residue_p;
	 CAtom *at;
	 for (int ires=0; ires<nres; ires++) {
	    residue_p = chain_p->GetResidue(ires);
	    if (residue_p->GetSeqNum() == resno) {
	       if (inscode == residue_p->GetInsCode()) { 
		  int n_atoms = residue_p->GetNumberOfAtoms();
		  
		  for (int iat=0; iat<n_atoms; iat++) {
		     at = residue_p->GetAtom(iat);

		     if (std::string(at->name) != " N  ") {

			found = 1;
			r = cis_trans_conversion(at, 0);
		     }
		     if (found)
			break;
		  }
		  if (found)
		     break;
	       }
	    }
	    if (found)
	       break;
	 }
      }
      if (found)
	 break;
   }
   return r;

} 


// ---- cis <-> trans conversion
int
molecule_class_info_t::cis_trans_conversion(CAtom *at, short int is_N_flag) {

   // These 3 are pointers, each of which are of size 2
   PCResidue *trans_residues = NULL;
   PCResidue *cis_residues = NULL;
   PCResidue *mol_residues = NULL;

   int offset = 0;
   if (is_N_flag)
      offset = -1;

   int resno_1 = at->GetSeqNum() + offset;
   int resno_2 = resno_1 + 1; // i.e. *this* residue, if N clicked.
   char *chain_id = at->GetChainID();

   int selHnd = atom_sel.mol->NewSelection();
   int nSelResidues;
   
   atom_sel.mol->Select(selHnd, STYPE_RESIDUE, 0,
			chain_id,
			resno_1, "*",
			resno_2, "*",
			"*",  // residue name
			"*",  // Residue must contain this atom name?
			"*",  // Residue must contain this Element?
			"*",  // altLocs
			SKEY_NEW // selection key
			);
   atom_sel.mol->GetSelIndex(selHnd, mol_residues, nSelResidues);

   graphics_info_t g;
   int istat = 0;
   if (nSelResidues >= 2) {

      int selHnd_trans = g.standard_residues_asc.mol->NewSelection();
      int ntrans_residues; 
      g.standard_residues_asc.mol->Select(selHnd_trans, STYPE_RESIDUE, 0,
					  "*",
					  ANY_RES, "*",
					  ANY_RES, "*",
					  "TNS", // residue name
					  "*",   // Residue must contain this atom name?
					  "*",   // Residue must contain this Element?
					  "*",   // altLocs
					  SKEY_NEW // selection key
					  );
      g.standard_residues_asc.mol->GetSelIndex(selHnd_trans,
					       trans_residues, ntrans_residues);
      if (ntrans_residues >= 2) {

	 int selHnd_cis = g.standard_residues_asc.mol->NewSelection();
	 int ncis_residues; 
	 g.standard_residues_asc.mol->Select(selHnd_cis, STYPE_RESIDUE, 0,
					     "*",
					     ANY_RES, "*",
					     ANY_RES, "*",
					     "CIS", // residue name
					     "*",   // Residue must contain this atom name?
					     "*",   // Residue must contain this Element?
					     "*",   // altLocs
					     SKEY_NEW // selection key
					     );
	 g.standard_residues_asc.mol->GetSelIndex(selHnd_cis, cis_residues, ncis_residues);

	 if (ncis_residues >= 2) { 

	    PPCAtom residue_atoms = NULL;
	    int n_residue_atoms;
	    trans_residues[0]->GetAtomTable(residue_atoms, n_residue_atoms);
	    trans_residues[1]->GetAtomTable(residue_atoms, n_residue_atoms);
	    
	    istat = cis_trans_convert(mol_residues, trans_residues, cis_residues);
	    if (istat) {
	       // backup made in cis_trans_convert()
	       have_unsaved_changes_flag = 1;
	       make_bonds_type_checked();
	    }
	 } else {
	    std::cout << "ERROR:: failed to get cis residues in cis_trans_convert "
		      << ncis_residues << std::endl;
	 }
	 g.standard_residues_asc.mol->DeleteSelection(selHnd_cis);
      } else {
	 std::cout << "ERROR:: failed to get trans residues in cis_trans_convert "
		   << ntrans_residues << std::endl;
      }
      g.standard_residues_asc.mol->DeleteSelection(selHnd_trans);
   } else {
      std::cout << "ERROR:: failed to get mol residues in cis_trans_convert" << std::endl;
   }
   atom_sel.mol->DeleteSelection(selHnd);


   return istat;
}


// mol_residues, trans_residues, cis_residues must be at least of length 2.
int
molecule_class_info_t::cis_trans_convert(PCResidue *mol_residues,
					 PCResidue *trans_residues,
					 PCResidue *cis_residues) {

   // First of all, are we cis or trans?
   // 
   // mol_residues is guaranteed to have at least 2 residues.
   //
   int istatus = 0;
   std::string altconf("");
   std::pair<short int, double> omega =
      coot::util::omega_torsion(mol_residues[0], mol_residues[1], altconf);

   std::cout << "omega: " << omega.first << " " << omega.second*180.0/3.14159
	     << " degrees " << std::endl;
   if (omega.first) {
      short int is_cis_flag = 0;
      PCResidue *cis_trans_init_match = trans_residues;
      PCResidue *converted_residues   = cis_residues;
      if ((omega.second < 1.57) && (omega.second > -1.57)) {
	 std::cout << "INFO This is a CIS peptide - making it TRANS" << std::endl;
	 is_cis_flag = 1;
	 cis_trans_init_match = cis_residues;
	 converted_residues = trans_residues;
      } else { 
	 std::cout << "INFO This is a TRANS peptide - making it CIS" << std::endl;
      }
	 
      // Now match cis_trans_init_match petide atoms onto the peptide
      // atoms of mol_residues and give us a transformation matrix.
      // That matrix will be applied to the converted_residues to give
      // new position of peptide atoms of the mol_residues.

      // we need to set these:
      CAtom *mol_residue_CA_1 = NULL;
      CAtom *mol_residue_C_1  = NULL;
      CAtom *mol_residue_O_1  = NULL;
      CAtom *mol_residue_CA_2 = NULL;
      CAtom *mol_residue_N_2  = NULL;
      
      PPCAtom mol_residue_atoms = NULL;
      int n_residue_atoms;
      mol_residues[0]->GetAtomTable(mol_residue_atoms, n_residue_atoms);
      for (int i=0; i<n_residue_atoms; i++) {
	 std::string atom_name = mol_residue_atoms[i]->name;
	 if (atom_name == " CA ") {
	    mol_residue_CA_1 = mol_residue_atoms[i];
	 }
	 if (atom_name == " C  ") {
	    mol_residue_C_1 = mol_residue_atoms[i];
	 }
	 if (atom_name == " O  ") {
	    mol_residue_O_1 = mol_residue_atoms[i];
	 }
      }
      mol_residue_atoms = NULL;
      mol_residues[1]->GetAtomTable(mol_residue_atoms, n_residue_atoms);
      for (int i=0; i<n_residue_atoms; i++) {
	 std::string atom_name = mol_residue_atoms[i]->name;
	 if (atom_name == " CA ") {
	    mol_residue_CA_2 = mol_residue_atoms[i];
	 }
	 if (atom_name == " N  ") {
	    mol_residue_N_2 = mol_residue_atoms[i];
	 }
      }

      if (mol_residue_CA_1 && mol_residue_C_1 && mol_residue_O_1 &&
	  mol_residue_CA_2 && mol_residue_N_2) {

	 // So we have this molecules atoms.
	 // Now do something similar to get the atoms of cis_trans_init_match

	 // we need to set these:
	 CAtom *cis_trans_init_match_residue_CA_1 = NULL;
	 CAtom *cis_trans_init_match_residue_C_1  = NULL;
	 CAtom *cis_trans_init_match_residue_O_1  = NULL;
	 CAtom *cis_trans_init_match_residue_CA_2 = NULL;
	 CAtom *cis_trans_init_match_residue_N_2  = NULL;
      
	 PPCAtom cis_trans_init_match_residue_atoms = NULL;
	 cis_trans_init_match[0]->GetAtomTable(cis_trans_init_match_residue_atoms,
					       n_residue_atoms);
	 for (int i=0; i<n_residue_atoms; i++) {
	    std::string atom_name = cis_trans_init_match_residue_atoms[i]->name;
	    if (atom_name == " CA ") {
	       cis_trans_init_match_residue_CA_1 = cis_trans_init_match_residue_atoms[i];
	    }
	    if (atom_name == " C  ") {
	       cis_trans_init_match_residue_C_1 = cis_trans_init_match_residue_atoms[i];
	    }
	    if (atom_name == " O  ") {
	       cis_trans_init_match_residue_O_1 = cis_trans_init_match_residue_atoms[i];
	    }
	 }
	 cis_trans_init_match_residue_atoms = NULL;
	 cis_trans_init_match[1]->GetAtomTable(cis_trans_init_match_residue_atoms,
					       n_residue_atoms);
	 for (int i=0; i<n_residue_atoms; i++) {
	    std::string atom_name = cis_trans_init_match_residue_atoms[i]->name;
	    if (atom_name == " CA ") {
	       cis_trans_init_match_residue_CA_2 = cis_trans_init_match_residue_atoms[i];
	    }
	    if (atom_name == " N  ") {
	       cis_trans_init_match_residue_N_2 = cis_trans_init_match_residue_atoms[i];
	    }
	 }

	 if (cis_trans_init_match_residue_CA_1 &&
	     cis_trans_init_match_residue_C_1 &&
	     cis_trans_init_match_residue_O_1 &&
	     cis_trans_init_match_residue_CA_2 &&
	     cis_trans_init_match_residue_N_2) {

	    // Now do the same other:

	    // we need to set these:
	    CAtom *converted_residues_residue_CA_1 = NULL;
	    CAtom *converted_residues_residue_C_1  = NULL;
	    CAtom *converted_residues_residue_O_1  = NULL;
	    CAtom *converted_residues_residue_CA_2 = NULL;
	    CAtom *converted_residues_residue_N_2  = NULL;
      
	    PPCAtom converted_residues_residue_atoms = NULL;
	    converted_residues[0]->GetAtomTable(converted_residues_residue_atoms,
						n_residue_atoms);
	    for (int i=0; i<n_residue_atoms; i++) {
	       std::string atom_name = converted_residues_residue_atoms[i]->name;
	       if (atom_name == " CA ") {
		  converted_residues_residue_CA_1 = converted_residues_residue_atoms[i];
	       }
	       if (atom_name == " C  ") {
		  converted_residues_residue_C_1 = converted_residues_residue_atoms[i];
	       }
	       if (atom_name == " O  ") {
		  converted_residues_residue_O_1 = converted_residues_residue_atoms[i];
	       }
	    }

	    converted_residues_residue_atoms = NULL;
	    int n_residue_atoms;
	    converted_residues[1]->GetAtomTable(converted_residues_residue_atoms,
						n_residue_atoms);
	    for (int i=0; i<n_residue_atoms; i++) {
	       std::string atom_name = converted_residues_residue_atoms[i]->name;
	       if (atom_name == " CA ") {
		  converted_residues_residue_CA_2 = converted_residues_residue_atoms[i];
	       }
	       if (atom_name == " N  ") {
		  converted_residues_residue_N_2 = converted_residues_residue_atoms[i];
	       }
	    }

	    if (converted_residues_residue_CA_1 &&
		converted_residues_residue_C_1 &&
		converted_residues_residue_O_1 &&
		converted_residues_residue_CA_2 &&
		converted_residues_residue_N_2) {

	       std::vector<clipper::Coord_orth> current;
	       std::vector<clipper::Coord_orth> cis_trans_init;
	       std::vector<clipper::Coord_orth> converted;

	       current.push_back(clipper::Coord_orth(mol_residue_CA_1->x,
						     mol_residue_CA_1->y,
						     mol_residue_CA_1->z));
	       current.push_back(clipper::Coord_orth(mol_residue_C_1->x,
						     mol_residue_C_1->y,
						     mol_residue_C_1->z));
	       current.push_back(clipper::Coord_orth(mol_residue_O_1->x,
						     mol_residue_O_1->y,
						     mol_residue_O_1->z));
	       current.push_back(clipper::Coord_orth(mol_residue_CA_2->x,
						     mol_residue_CA_2->y,
						     mol_residue_CA_2->z));
	       current.push_back(clipper::Coord_orth(mol_residue_N_2->x,
						     mol_residue_N_2->y,
						     mol_residue_N_2->z));
	    
	       cis_trans_init.push_back(clipper::Coord_orth(cis_trans_init_match_residue_CA_1->x,
							    cis_trans_init_match_residue_CA_1->y,
							    cis_trans_init_match_residue_CA_1->z));
	    
	       cis_trans_init.push_back(clipper::Coord_orth(cis_trans_init_match_residue_C_1->x,
							    cis_trans_init_match_residue_C_1->y,
							    cis_trans_init_match_residue_C_1->z));
	    
	       cis_trans_init.push_back(clipper::Coord_orth(cis_trans_init_match_residue_O_1->x,
							    cis_trans_init_match_residue_O_1->y,
							    cis_trans_init_match_residue_O_1->z));
	    
	       cis_trans_init.push_back(clipper::Coord_orth(cis_trans_init_match_residue_CA_2->x,
							    cis_trans_init_match_residue_CA_2->y,
							    cis_trans_init_match_residue_CA_2->z));
	    
	       cis_trans_init.push_back(clipper::Coord_orth(cis_trans_init_match_residue_N_2->x,
							    cis_trans_init_match_residue_N_2->y,
							    cis_trans_init_match_residue_N_2->z));
	       
	       converted.push_back(clipper::Coord_orth(converted_residues_residue_CA_1->x,
						       converted_residues_residue_CA_1->y,
						       converted_residues_residue_CA_1->z));
	    
	       converted.push_back(clipper::Coord_orth(converted_residues_residue_C_1->x,
						       converted_residues_residue_C_1->y,
						       converted_residues_residue_C_1->z));
	    
	       converted.push_back(clipper::Coord_orth(converted_residues_residue_O_1->x,
						       converted_residues_residue_O_1->y,
						       converted_residues_residue_O_1->z));
	    
	       converted.push_back(clipper::Coord_orth(converted_residues_residue_CA_2->x,
						       converted_residues_residue_CA_2->y,
						       converted_residues_residue_CA_2->z));
	    
	       converted.push_back(clipper::Coord_orth(converted_residues_residue_N_2->x,
						       converted_residues_residue_N_2->y,
						       converted_residues_residue_N_2->z));

	       make_backup();
	       clipper::RTop_orth lsq_mat(cis_trans_init, current);

	       // now move the current atoms in mol_residues to the
	       // positions of converted (after converted atoms have
	       // been shifted by lsq_mat)

	       clipper::Coord_orth newpos;

	       newpos = converted[0].transform(lsq_mat);
	       mol_residue_CA_1->x = newpos.x();
	       mol_residue_CA_1->y = newpos.y();
	       mol_residue_CA_1->z = newpos.z();
	       
	       newpos = converted[1].transform(lsq_mat);
	       mol_residue_C_1->x = newpos.x();
	       mol_residue_C_1->y = newpos.y();
	       mol_residue_C_1->z = newpos.z();

	       newpos = converted[2].transform(lsq_mat);
	       mol_residue_O_1->x = newpos.x();
	       mol_residue_O_1->y = newpos.y();
	       mol_residue_O_1->z = newpos.z();

	       newpos = converted[3].transform(lsq_mat);
	       mol_residue_CA_2->x = newpos.x();
	       mol_residue_CA_2->y = newpos.y();
	       mol_residue_CA_2->z = newpos.z();

	       newpos = converted[4].transform(lsq_mat);
	       mol_residue_N_2->x = newpos.x();
	       mol_residue_N_2->y = newpos.y();
	       mol_residue_N_2->z = newpos.z();

	       istatus = 1;
	    }
	 }
      }
   }
   return istatus;
}



/* Reverse the direction of a the fragment of the clicked on
   atom/residue.  A fragment is a consequitive range of residues -
   where there is a gap in the numbering, that marks breaks between
   fragments in a chain.  There also needs to be a distance break - if
   the CA of the next/previous residue is more than 5A away, that also
   marks a break. Thow away all atoms in fragment other than CAs.

   Do it "in place" not return a new molecule.
*/
// 
// This is a cheesy implementation that does the fragmentization based
// only on chain id and residue number, not distance.
// 
short int
molecule_class_info_t::reverse_direction_of_fragment(const std::string &chain_id,
						     int resno) {


   // First find the fragment
   short int istat=0;

   if (atom_sel.n_selected_atoms > 0) { 
      // Let's use a minimol:
      coot::minimol::molecule m_initial(atom_sel.mol);
      coot::minimol::molecule fragmented_mol(m_initial.fragmentize());
      short int found_fragment_flag = 0;

      for (unsigned int ifrag=0; ifrag<fragmented_mol.fragments.size(); ifrag++) {
	 if (fragmented_mol[ifrag].fragment_id == chain_id) {
	    for (int ires=fragmented_mol[ifrag].min_res_no();
		 ires<=fragmented_mol[ifrag].max_residue_number();
		 ires++) {
	       if (fragmented_mol[ifrag][ires].atoms.size() > 0) {
		  if (ires == resno) {
		     // OK, we have found our fragment
		     found_fragment_flag = 1;
		     make_backup();
		     istat = 1; // we are doing something.

		     // Lets make a new fragment and replace the
		     // current fragment in fragmented_mol

		     coot::minimol::fragment f = fragmented_mol[ifrag];
		     
		     // find the range of residues that have atoms -
		     // the is a kluge because currently (20050815)
		     // min_res_no() returns low [0,1?] for fragments
		     // that actually start (say) 1005.
		     int fragment_low_resno = fragmented_mol[ifrag].max_residue_number();
		     int fragment_high_resno = -9999;
		     for (int ires_in_frag=fragmented_mol[ifrag].min_res_no();
			  ires_in_frag<=fragmented_mol[ifrag].max_residue_number();
			  ires_in_frag++) {
			if (fragmented_mol[ifrag][ires_in_frag].atoms.size() > 0) {
			   if (fragmented_mol[ifrag][ires_in_frag].seqnum > fragment_high_resno) {
			      fragment_high_resno = fragmented_mol[ifrag][ires_in_frag].seqnum;
			   }
			   if (fragmented_mol[ifrag][ires_in_frag].seqnum < fragment_low_resno) {
			      fragment_low_resno = fragmented_mol[ifrag][ires_in_frag].seqnum;
			   }
			}
		     }
		     // OK between fragment_low_resno and
		     // fragment_high_resno we want to reverse the
		     // numbering
		     for (int ires_in_frag=fragment_low_resno;
			  ires_in_frag<=fragment_high_resno;
			  ires_in_frag++) {
			// pencil and paper job:
			int new_resno = fragment_high_resno - ires_in_frag + fragment_low_resno;
			coot::minimol::residue r(new_resno, fragmented_mol[ifrag][ires_in_frag].name);
			// add the CA
			for (unsigned int iat=0;
			     iat<fragmented_mol[ifrag][ires_in_frag].atoms.size();
			     iat++) {
			   if (fragmented_mol[ifrag][ires_in_frag].atoms[iat].name == " CA ") {
			      r.addatom(fragmented_mol[ifrag][ires_in_frag].atoms[iat]);
			   }
			}
			f.addresidue(r, 0);
		     }
		     // now replace the fragment in fragmented_mol
		     fragmented_mol[ifrag] = f;
		  }
	       }
	       if (found_fragment_flag)
		  break;
	    }
	 }
	 if (found_fragment_flag)
	    break;
      }
      if (found_fragment_flag) {

	 float bf = graphics_info_t::default_new_atoms_b_factor;
	 CMMDBManager *mol = fragmented_mol.pcmmdbmanager(bf);
	 // before we get rid of the old atom_sel lets save the cell, symm.
	 realtype a[6];
	 realtype vol;
	 int orthcode;
	 atom_sel.mol->GetCell(a[0], a[1], a[2], a[3], a[4], a[5], vol, orthcode);
	 char *sg = atom_sel.mol->GetSpaceGroup();
	 mol->SetCell(a[0], a[1], a[2], a[3], a[4], a[5]);

	 if (sg)
	    mol->SetSpaceGroup(sg);

	 // Now, we can convert that minimol back to a CMMDBManager
	 delete atom_sel.mol;
	 atom_sel = make_asc(mol);

	 have_unsaved_changes_flag = 1;
	 make_bonds_type_checked();
      }
   }
   return istat;
}


// Return 1 on a successful flip.  Flip the last chi angle.
// 
int
molecule_class_info_t::do_180_degree_side_chain_flip(const std::string &chain_id,
						     int resno,
						     const std::string &inscode,
						     const std::string &altconf,
						     coot::protein_geometry *geom_p) {

   // Notice that chi_angles has no concept of alt conf.
   //
   // chi_angles works on the atoms of a residue, with no alt conf
   // checking.
   // 
   // So we need to create a synthetic residue (copy) with atoms of
   // the alt conf (and "" if needed).  We flip the residue copy and
   // then feed back the atoms with (altLoc == altconf) into the
   // original residue.

   int istatus=0;
   double diff = 180.0;

   int nth_chi = -1; // unset

   if (atom_sel.n_selected_atoms > 0) {

      int nSelResidues;
      PCResidue *SelResidues = NULL;
      int selnd = atom_sel.mol->NewSelection();
      atom_sel.mol->Select(selnd, STYPE_RESIDUE, 0,
			   (char *) chain_id.c_str(),
			   resno, (char *) inscode.c_str(),
			   resno, (char *) inscode.c_str(),
			   "*", "*", "*", "*", SKEY_NEW);
      atom_sel.mol->GetSelIndex(selnd, SelResidues, nSelResidues);
      if (nSelResidues > 0 ) { 
	 CResidue *residue = SelResidues[0];
	 std::string resname = residue->GetResName();

	 if (resname == "ARG") nth_chi = 4; 
	 if (resname == "ASP") nth_chi = 2; 
	 if (resname == "ASN") nth_chi = 2;
	 if (resname == "CYS") nth_chi = 1;
	 if (resname == "GLN") nth_chi = 3;
	 if (resname == "GLU") nth_chi = 3;
	 if (resname == "PHE") nth_chi = 2;
	 if (resname == "HIS") nth_chi = 2;
	 if (resname == "SER") nth_chi = 1;
	 if (resname == "THR") nth_chi = 1;
	 if (resname == "VAL") nth_chi = 1;
	 if (resname == "TRP") nth_chi = 2;
	 if (resname == "TYR") nth_chi = 2;

	 PPCAtom residue_atoms = NULL;
	 int nResidueAtoms;
	 residue->GetAtomTable(residue_atoms, nResidueAtoms);

	 if (nth_chi != -1) {
	    make_backup();
	    CResidue *residue_copy =
	       coot::util::deep_copy_this_residue(residue, altconf, 0);

	    // Which atoms have we got in residue_copy?
 	    int n_atom_residue_copy;
 	    PCAtom *residue_atoms_copy = 0;
 	    residue_copy->GetAtomTable(residue_atoms_copy, n_atom_residue_copy);
// 	    for (int iat=0; iat<n_atom_residue_copy; iat++)
// 	       std::cout << residue_atoms_copy[iat] << std::endl;

	    
	    
	    coot::chi_angles chi_ang(residue_copy, 0);
	    std::vector<std::vector<int> > contact_indices(n_atom_residue_copy);
	    contact_indices = coot::util::get_contact_indices_from_restraints(residue_copy, geom_p, 1);
	    std::pair<short int, float> istat = chi_ang.change_by(nth_chi, diff, contact_indices);
      
	    if (istat.first) { // failure
	       std::cout << "Failure to flip" << std::endl;
	    } else {
	       istatus = 1;
	       // OK, we need transfer the coordinates of the
	       // altconfed atoms of residue_copy to residue:
	       // 
	       for (int iatc=0; iatc<n_atom_residue_copy; iatc++) { 
		  // std::cout << residue_atoms_copy[iat] << std::endl;
		  std::string atom_copy_altconf = residue_atoms_copy[iatc]->altLoc;
		  if (atom_copy_altconf == altconf) {
		     // we need to find this atom in residue
		     std::string atom_copy_name = residue_atoms_copy[iatc]->name;
		     for (int iato=0; iato<nResidueAtoms; iato++) {
			std::string orig_atom_altconf = residue_atoms[iato]->altLoc;
			std::string orig_atom_name    = residue_atoms[iato]->name;
			if (orig_atom_name == atom_copy_name) {
			   if (atom_copy_altconf == orig_atom_altconf) {
// 			      std::cout << "DEBUG:: copying coords from "
// 					<< residue_atoms_copy[iatc] << std::endl;
			      residue_atoms[iato]->x = residue_atoms_copy[iatc]->x;
			      residue_atoms[iato]->y = residue_atoms_copy[iatc]->y;
			      residue_atoms[iato]->z = residue_atoms_copy[iatc]->z;
			   }
			}
		     }
		  }
	       }
	       // Now let's get rid of residue_copy:
	       delete residue_copy;
	       residue_copy = 0; 
	    }

	    have_unsaved_changes_flag = 1;
	    make_bonds_type_checked();
      
	 } else {
	    std::cout << "No flipping allowed for residue type " << resname
		      << std::endl;
	 }
      }
      atom_sel.mol->DeleteSelection(selnd);
   }
   return istatus;
}



   // Return a vector of residues that have missing atoms by dictionary
   // search.  missing_hydrogens_flag reflects if we want to count
   // residues that have missing hydrogens as residues with missing
   // atoms that should be part of the returned vector. Most of the
   // time, we don't care about hydrogens and the flag is 0.
// We also return a vector of residue names for which we couldn't get
// a geometry dictionary entry.
coot::util::missing_atom_info 
molecule_class_info_t::missing_atoms(short int missing_hydrogens_flag,
				     coot::protein_geometry *geom_p) const {

   std::vector<CResidue *> residues_with_missing_atoms;
   std::vector<std::string> residues_no_dictionary;
   // and these atoms will need to be deleted when we auto-complete the residue.
   std::vector<std::pair<CResidue *, std::vector<CAtom *> > > atoms_in_coords_but_not_in_dict;
   
   if (atom_sel.n_selected_atoms > 0) {
      
      // residue_atoms is a vector of residues names (monomer comp_id)
      // together with a vector of atoms.  On each check in this list
      // for the residue type, associated with each atom name is a
      // flag which says if this is a hydrogen or not.
      std::vector<coot::util::dict_residue_atom_info_t> residue_atoms;
      
      int imod = 1;
      CModel *model_p = atom_sel.mol->GetModel(imod);
      CChain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 int nres = chain_p->GetNumberOfResidues();
	 PCResidue residue_p;
	 CAtom *at;
	 for (int ires=0; ires<nres; ires++) { 
	    residue_p = chain_p->GetResidue(ires);
	    std::string residue_name(residue_p->GetResName());
	    short int found_dict = 0;
	    std::vector<coot::util::dict_atom_info_t> residue_dict_atoms;
	    for (unsigned int idict=0; idict<residue_atoms.size(); idict++) {
	       std::string tmp_str = residue_atoms[idict].residue_name;
	       if (residue_name == tmp_str) {
		  residue_dict_atoms = residue_atoms[idict].atom_info;
		  found_dict = 1;
	       }
	    }
	    if (!found_dict) {
	       // we need to try to make it from the dict_info restraint info.
	       // (dymamic_add will happen automatically).
	       // 
	       // set found_dict if we get it and make it.
	       coot::util::dict_residue_atom_info_t residue_atoms_for_a_residue(residue_name,
										geom_p);
	       if (!residue_atoms_for_a_residue.is_empty_p()) {
		  residue_atoms.push_back(residue_atoms_for_a_residue);
		  residue_dict_atoms = residue_atoms_for_a_residue.atom_info;
		  found_dict = 1;
	       }
	    }
	    
	    if (!found_dict) {

	       // No dictionary available, even after we try to make
	       // it (dynamic add failed, presumably).  So add this
	       // residue type to the list of residues for which we
	       // can't find the restraint info:
	       residues_no_dictionary.push_back(residue_name);

	    } else {

	       // OK, we have a dictionary. Deal with hydrogens, by
	       // assigning them as present if we don't care if they
	       // don't exist.
	       //
	       // Not this kludges the isHydrogen? flag.  We are
	       // testing if it was set (existed) or not!
	       // 
	       std::vector<coot::util::dict_atom_info_t> dict_atom_names_pairs;
	       for (unsigned int iat=0; iat<residue_dict_atoms.size(); iat++) {
		  // Let's not add hydrogens to the dictionary atoms
		  // if we don't care if the don't exist. 
		  if ((missing_hydrogens_flag == 0) &&
		      (residue_dict_atoms[iat].is_Hydrogen_flag == 1)) {
		     // do nothing 
		  } else {
		     // put it in the list and initially mark it as not found.
		     coot::util::dict_atom_info_t p(residue_dict_atoms[iat].name, 0);
		     dict_atom_names_pairs.push_back(p);
		  }
	       }
	       
	       // OK for every atom in the PDB residue, was it in the dictionary?
	       // 
	       int n_atoms = residue_p->GetNumberOfAtoms();
	       for (int iat=0; iat<n_atoms; iat++) {
		  at = residue_p->GetAtom(iat);
		  std::string atom_name(at->name);
		  // check against each atom in the dictionary:
		  for (unsigned int idictat=0; idictat<dict_atom_names_pairs.size(); idictat++) {
		     if (atom_name == dict_atom_names_pairs[idictat].name) {
			// kludge the is_Hydrogen_flag! Use as a marker
			dict_atom_names_pairs[idictat].is_Hydrogen_flag = 1; // mark as found
			break;
		     }
		  }
	       }

	       // OK, so we have run through all atoms in the PDB
	       // residue.  How many atoms in the dictionary list for
	       // this residue were not found?
	       int n_atoms_unfound = 0;
	       for (unsigned int idictat=0; idictat<dict_atom_names_pairs.size(); idictat++) {
		  if (dict_atom_names_pairs[idictat].is_Hydrogen_flag == 0)
		     n_atoms_unfound++;
	       }
		  
	       if (n_atoms_unfound> 0) {
		  residues_with_missing_atoms.push_back(residue_p);
	       }
	    }
	 }
      }
   }
   return coot::util::missing_atom_info(residues_no_dictionary,
					residues_with_missing_atoms,
					atoms_in_coords_but_not_in_dict);
}


// The function that uses missing atom info:
coot::util::missing_atom_info
molecule_class_info_t::fill_partial_residues(coot::protein_geometry *geom_p,
					     int refinement_map_number) {
   
   coot::util::missing_atom_info info;
   if (atom_sel.n_selected_atoms > 0) {

      make_backup();
      int backup_state = backup_this_molecule;
      backup_this_molecule = 0;  // temporarily
      info = missing_atoms(0, geom_p);

      if (info.residues_with_missing_atoms.size() > 0) {
	 std::cout << " Residues with missing atoms:" << "\n";
	 for (unsigned int i=0; i<info.residues_with_missing_atoms.size(); i++)
	    std::cout << info.residues_with_missing_atoms[i]->GetResName() << " "
		      << info.residues_with_missing_atoms[i]->GetSeqNum()  << " "
		      << info.residues_with_missing_atoms[i]->GetChainID() << " "
		      << "\n";

	 for (unsigned int i=0; i<info.residues_with_missing_atoms.size(); i++) {
	    int resno =  info.residues_with_missing_atoms[i]->GetSeqNum();
	    std::string chain_id = info.residues_with_missing_atoms[i]->GetChainID();
	    std::string residue_type = info.residues_with_missing_atoms[i]->GetResName();
	    std::string inscode = info.residues_with_missing_atoms[i]->GetInsCode();
	    std::string altloc("");
	    float lowest_probability = 0.8;
	    int clash_flag = 1;
	    
	    mutate(resno, inscode, chain_id, residue_type); // fill missing atoms
	    if (refinement_map_number >= 0)
	       auto_fit_best_rotamer(resno, altloc, inscode, chain_id,
				     refinement_map_number, clash_flag,
				     lowest_probability);

	    // we could do refinement here, possibly, but instead,
	    // return the missing atom info after this function is
	    // finished so that the refinement can be done by the
	    // calling function, were we can see some visual feedback.
	 }
      } else {
	 std::cout << " No Residues with missing atoms (that have dictionary entries)\n";
      }

      // restore backup state;
      backup_this_molecule = backup_state;
      have_unsaved_changes_flag = 1;
   }
   return info;
}


int 
molecule_class_info_t::fill_partial_residue(coot::residue_spec_t &residue_spec,
					    coot::protein_geometry *geom_p,
					    int refinement_map_number) {

   int resno = residue_spec.resno;
   std::string chain_id = residue_spec.chain;
   std::string inscode = residue_spec.insertion_code;
   std::string altloc = "";
   float lowest_probability = 0.8;
   int clash_flag = 1;
      
   CResidue *residue_p = get_residue(resno, inscode, chain_id);
   if (residue_p) { 
      std::string residue_type = residue_p->GetResName();
      mutate(resno, inscode, chain_id, residue_type); // fill missing atoms
      if (refinement_map_number >= 0)
	 auto_fit_best_rotamer(resno, altloc, inscode, chain_id,
			       refinement_map_number, clash_flag,
			       lowest_probability);
   }
}


// ------------------------------------------------------------------------
//                       dots
// ------------------------------------------------------------------------
void
molecule_class_info_t::draw_dots() {

   for (int iset=0; iset<dots.size(); iset++) {
      if (dots[iset].is_open_p() == 1) {
	 glPointSize(2);
	 glColor3f(0.3, 0.4, 0.5);
	 glBegin(GL_POINTS);
	 unsigned int n_points = dots[iset].points.size();
	 for (unsigned int i=0; i<n_points; i++) {
	    glVertex3f(dots[iset].points[i].x(),
		       dots[iset].points[i].y(),
		       dots[iset].points[i].z());
	 }
	 glEnd();
      }
   }
}

void
molecule_class_info_t::clear_dots(int dots_handle) {

   if ((dots_handle >= 0) && (dots_handle < dots.size())) { 
      dots[dots_handle].close_yourself();
   } else {
      std::cout << "WARNING:: bad dots_handle in clear_dots: "
		<< dots_handle << " " << dots.size() << std::endl;
   } 

}


int
molecule_class_info_t::make_dots(const std::string &atom_selection_str,
				 float dot_density, float atom_radius_scale) {

   int dots_handle = -1;

   if (has_model()) {
      int SelHnd = atom_sel.mol->NewSelection();
      atom_sel.mol->Select(SelHnd, STYPE_ATOM,
			   (char *) atom_selection_str.c_str(), // sigh...
			   SKEY_OR);
      int n_selected_atoms;
      PPCAtom atom_selection = NULL;
      atom_sel.mol->GetSelIndex(SelHnd, atom_selection, n_selected_atoms);
      float phi_step = 5.0 * (M_PI/180.0);
      float theta_step = 5.0 * (M_PI/180.0);
      if (dot_density > 0.0) { 
	 phi_step   /= dot_density;
	 theta_step /= dot_density;
      }
      float f = 0.1;
      float r = 1.0;
      
      std::vector<clipper::Coord_orth> points;
      for (int iatom=0; iatom<n_selected_atoms; iatom++) {
	 
	 clipper::Coord_orth centre(atom_selection[iatom]->x,
				    atom_selection[iatom]->y,
				    atom_selection[iatom]->z);

	 short int even = 1;
// 	    for (float theta=0; theta<M_PI;
// 		 theta+=theta_step + f*((theta-M_PI/2.0)*(theta-M_PI/2.0))) {
	 for (float theta=0; theta<M_PI; theta+=theta_step) {
	    float phi_step_inner = phi_step + 0.1 * pow(theta-0.5*M_PI, 2);
	    for (float phi=0; phi<2*M_PI; phi+=phi_step_inner) {
	       if (even) {

		  // Is there another atom in the same residue as this
		  // atom, that is closer to pt than the centre atom?
		  // If so, don't draw this point.
		  
		  clipper::Coord_orth pt(r*atom_radius_scale*cos(phi)*sin(theta),
					 r*atom_radius_scale*sin(phi)*sin(theta),
					 r*atom_radius_scale*cos(theta));
		  pt += centre;
		  
		  PPCAtom residue_atoms = NULL;
		  int n_residue_atoms;

		  atom_selection[iatom]->residue->GetAtomTable(residue_atoms, n_residue_atoms);
		  short int draw_it = 1;
		  for (int i=0; i<n_residue_atoms; i++) {
		     if (residue_atoms[i] != atom_selection[iatom]) { 
			clipper::Coord_orth residue_atom_pt(residue_atoms[i]->x,
							    residue_atoms[i]->y,
							    residue_atoms[i]->z);
			if (clipper::Coord_orth::length(pt, residue_atom_pt) < r*atom_radius_scale) {
			   draw_it = 0;
			   break;
			} 
		     }
		  }
		  if (draw_it) { 
		     points.push_back(pt);
		  }
	       }
	       even = 1 - even;
	    }
	 }
      } 
      
      coot::dots_representation_info_t dots_info(points);
      dots.push_back(dots_info);
      dots_handle = dots.size() -1;
   }
   return dots_handle;
} 


int
molecule_class_info_t::renumber_waters() {

   int renumbered_waters = 0;
   if (atom_sel.n_selected_atoms > 0) {

      short int changes_made = 0; 
      int n_models = atom_sel.mol->GetNumberOfModels();
      make_backup();
      for (int imod=1; imod<=n_models; imod++) { 
	 
	 CModel *model_p = atom_sel.mol->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol
	 int nchains = model_p->GetNumberOfChains();
	 if (nchains <= 0) { 
	    std::cout << "bad nchains in molecule " << nchains
		      << std::endl;
	 } else { 
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);
	       if (chain_p == NULL) {  
		  // This should not be necessary. It seem to be a
		  // result of mmdb corruption elsewhere - possibly
		  // DeleteChain in update_molecule_to().
		  std::cout << "NULL chain in renumber_waters" << std::endl;
	       } else {

		  if (chain_p->isSolventChain()) {

		     int resno = 1;
		     int nres = chain_p->GetNumberOfResidues();
		     PCResidue residue_p;
		     for (int ires=0; ires<nres; ires++) { 
			residue_p = chain_p->GetResidue(ires);
			residue_p->seqNum = resno; // stuff it in
			changes_made = 1;
			resno++;  // for next residue
		     }
		  }
	       }
	    }
	 }
      }
      if (changes_made) {
	 atom_sel.mol->FinishStructEdit();
	 have_unsaved_changes_flag = 1; 
	 renumbered_waters = 1;
      } 
   }
   return renumbered_waters;
} 

std::vector<std::string>
molecule_class_info_t::get_symop_strings() const {

   std::vector<std::string> r;
   if (atom_sel.mol) {
      // coords
      mat44 my_matt;
      int ierr = atom_sel.mol->GetTMatrix(my_matt, 0, 0, 0, 0);
      if (ierr == 0) {
	 // Good, we have symm info
	 for (int i = 0; i < atom_sel.mol->GetNumberOfSymOps(); i++) {
	    char* symop_str = atom_sel.mol->GetSymOp(i);
	    r.push_back(symop_str);
	 }
      }
   } else {
      // map
      int n = xmap_list[0].spacegroup().num_symops();
      for (int i=0; i<n; i++) {
	 r.push_back(xmap_list[0].spacegroup().symop(i).format());
      }
   } 
   return r;
} 


int
molecule_class_info_t::make_map_from_cns_data(const clipper::Spacegroup &sg,
					      const clipper::Cell &cell,
					      std::string cns_data_filename) {

   
   clipper::CNS_HKLfile cns;
   cns.open_read(cns_data_filename);

   clipper::Resolution resolution(cns.resolution(cell));
   
   clipper::HKL_info mydata(sg, cell, resolution);
   clipper::HKL_data<clipper::datatypes::F_phi<float>   >  fphidata(mydata); 

   std::cout << "importing info" << std::endl;
   cns.import_hkl_info(mydata);
   std::cout << "importing data" << std::endl;
   cns.import_hkl_data(fphidata); 
   cns.close_read(); 

   if (max_xmaps == 0) {
      xmap_list = new clipper::Xmap<float>[10];
      xmap_is_filled   = new int[10];
      xmap_is_diff_map = new int[10];
      contour_level    = new float[10];
   }

   max_xmaps++; 

   std::string mol_name = cns_data_filename;

   initialize_map_things_on_read_molecule(mol_name, 0, 0); // not diff map

   cout << "initializing map..."; 
   xmap_list[0].init(mydata.spacegroup(), 
		     mydata.cell(), 
		     clipper::Grid_sampling(mydata.spacegroup(),
					    mydata.cell(), 
					    mydata.resolution()));
   cout << "done."<< endl; 
   cout << "doing fft..." ; 
   xmap_list[0].fft_from( fphidata );                  // generate map
   cout << "done." << endl;

   mean_and_variance<float> mv = map_density_distribution(xmap_list[0],0);

   cout << "Mean and sigma of map from CNS file: " << mv.mean 
	<< " and " << sqrt(mv.variance) << endl;

   // fill class variables
   map_mean_ = mv.mean;
   map_sigma_ = sqrt(mv.variance);

   xmap_is_diff_map[0] = 0; 
   xmap_is_filled[0] = 1; 
   contour_level[0] = nearest_step(mv.mean + 1.5*sqrt(mv.variance), 0.05);

   graphics_info_t g; 
   g.scroll_wheel_map = g.n_molecules; // change the current scrollable map.
   update_map();
   return g.n_molecules;
}

// This seems to pick the front edge of the blob of density.  Why is
// that?
// 
clipper::Coord_orth
molecule_class_info_t::find_peak_along_line(const clipper::Coord_orth &p1,
					    const clipper::Coord_orth &p2) const {

   float high_point_1 = -9999999.9;
   float high_point_2 = -9999999.9;
   clipper::Coord_orth pbest;
   int istep_max = 500;

   for (unsigned int istep=0; istep<=istep_max; istep++) {
      float fr = float(istep)/float(istep_max);
      clipper::Coord_orth pc = p1 + fr*(p2-p1);
      float d = density_at_point(pc);
      // std::cout << ": " << istep << " " << fr << " " << d  << std::endl;
      if (d > high_point_1) {
	 high_point_1 = d;
	 pbest = pc;
      }
   }
   return pbest;
}


// replace molecule
int
molecule_class_info_t::replace_molecule(CMMDBManager *mol) {

   int was_changed = 0;
   if (mol) {
      atom_sel.mol->DeleteSelection(atom_sel.SelectionHandle);
      delete atom_sel.mol;
      atom_sel = make_asc(mol);
      have_unsaved_changes_flag = 1; 
      make_bonds_type_checked(); // calls update_ghosts()
      trim_atom_label_table();
      update_symmetry();
      was_changed = 1;
   }
   return was_changed;
}

// EM map function
int
molecule_class_info_t::scale_cell(float fac_u, float fac_v, float fac_w) {

   int retval = 0;

   if (has_map()) { 
      clipper::Cell cell_orig = xmap_list[0].cell();
      clipper::Cell_descr cell_d(cell_orig.a() * fac_u, 
				 cell_orig.b() * fac_v,
				 cell_orig.c() * fac_w,
				 cell_orig.alpha(), 
				 cell_orig.beta(), 
				 cell_orig.gamma());

      clipper::Cell new_cell(cell_d);
      clipper::Spacegroup new_spg(xmap_list[0].spacegroup());
      clipper::Xmap_base::Map_reference_index ix;
      clipper::Grid_sampling gs = xmap_list[0].grid_sampling();
      clipper::Xmap<float> new_map(new_spg, new_cell, gs);
      for (ix = xmap_list[0].first(); !ix.last(); ix.next() ) {
 	 new_map[ix] = xmap_list[0][ix];
      }
      xmap_list[0] = new_map;
      update_map();
   }
   return retval; 
} 
