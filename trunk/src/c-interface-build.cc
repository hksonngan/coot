/* src/c-interface-build.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007, 2008 The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
 * Copyright 2007 by Bernhard Lohkamp
 * Copyright 2008 by Kevin Cowtan
 * Copyright 2007, 2008, 2009, 2010, 2011 The University of Oxford
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

#ifdef USE_PYTHON
#include <Python.h>  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#include "compat/coot-sysdep.h"

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#define HAVE_CIF  // will become unnessary at some stage.

#include <sys/types.h> // for stating
#include <sys/stat.h>
#include <string.h> // strncmp
#if !defined _MSC_VER
#include <unistd.h>
#else
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#include <windows.h>
#endif
 

#include <mmdb2/mmdb_manager.h>
#include "coords/mmdb-extras.h"
#include "coords/mmdb.h"

#include "globjects.h" //includes gtk/gtk.h

#include "callbacks.h"
#include "interface.h" // now that we are moving callback
		       // functionality to the file, we need this
		       // header since some of the callbacks call
		       // fuctions built by glade.

#include "coords/mmdb-crystal.h"

#include "coords/Cartesian.h"
#include "coords/Bond_lines.h"

#include "graphics-info.h"

#include "coot-utils/coot-coord-utils.hh"
#include "utils/coot-fasta.hh"

#include "skeleton/BuildCas.h"
#include "ligand/helix-placement.hh"
#include "ligand/fast-ss-search.hh"

#include "trackball.h" // adding exportable rotate interface

#include "utils/coot-utils.hh"  // for is_member_p
#include "coot-utils/coot-map-heavy.hh"  // for fffear

#include "guile-fixups.h"


#include "c-interface.h"
#include "cc-interface.hh"
#include "cc-interface-scripting.hh"

#include "ligand/ligand.hh" // for rigid body fit by atom selection.

#include "cmtz-interface.hh" // for valid columns mtz_column_types_info_t
#include "c-interface-mmdb.hh"
#include "c-interface-scm.hh"
#include "c-interface-python.hh"

#ifdef USE_DUNBRACK_ROTAMERS
#include "ligand/dunbrack.hh"
#else 
#include "ligand/richardson-rotamer.hh"
#endif

#include "ligand/backrub-rotamer.hh"
#include "rotamer-search-modes.hh"

#include "protein_db/protein_db_utils.h"
#include "protein_db-interface.hh"

#include "cootilus/cootilus-build.h"

#include "c-interface-refine.hh"


/*  ------------------------------------------------------------------------ */
/*                   Maps - (somewhere else?):                               */
/*  ------------------------------------------------------------------------ */
/*! \brief Calculate SFs from an MTZ file and generate a map. 
 @return the new molecule number. */
int map_from_mtz_by_calc_phases(const char *mtz_file_name, 
				const char *f_col, 
				const char *sigf_col,
				int imol_coords) {

   int ir = -1; // return value
   graphics_info_t g;
   if (is_valid_model_molecule(imol_coords)) { 
      int imol_map = g.create_molecule();
      std::string m(mtz_file_name);
      std::string f(f_col);
      std::string s(sigf_col);
      atom_selection_container_t a = g.molecules[imol_coords].atom_sel;
      short int t = molecule_map_type::TYPE_2FO_FC;
      int istat = g.molecules[imol_map].make_map_from_mtz_by_calc_phases(imol_map,m,f,s,a,t);
      if (istat != -1) {
	 graphics_draw();
	 ir = imol_map;
      } else {
	 ir = -1; // error
	 graphics_info_t::erase_last_molecule();
      }
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("map-from-mtz-by-calc-phases");
   command_strings.push_back(mtz_file_name);
   command_strings.push_back(f_col);
   command_strings.push_back(sigf_col);
   command_strings.push_back(graphics_info_t::int_to_string(imol_coords));
   add_to_history(command_strings);
   return ir;
} 


/*! \brief fire up a GUI, which asks us which model molecule we want
  to calc phases from.  On "OK" button there, we call
  map_from_mtz_by_refmac_calc_phases() */
void calc_phases_generic(const char *mtz_file_name) {

   if (coot::file_exists(mtz_file_name)) { 
      graphics_info_t g;
      coot::mtz_column_types_info_t r = coot::get_mtz_columns(mtz_file_name);
      if (r.f_cols.size() == 0) {
	 std::cout << "No Fobs found in " << mtz_file_name << std::endl;
	 std::string s =  "No Fobs found in ";
	 s += mtz_file_name;
	 g.add_status_bar_text(s);
      } else { 
	 if (r.sigf_cols.size() == 0) {
	    std::cout << "No SigFobs found in " << mtz_file_name << std::endl;
	    std::string s =  "No SigFobs found in ";
	    s += mtz_file_name;
	    g.add_status_bar_text(s);
	 } else {
	    // normal path:
	    std::string f_obs_col = r.f_cols[0].column_label;
	    std::string sigfobs_col = r.sigf_cols[0].column_label;
	    std::vector<std::string> v;
	    v.push_back("refmac-for-phases-and-make-map");
	    // BL says:: dunno if we need the backslashing here, but just do it in case
	    v.push_back(coot::util::single_quote(coot::util::intelligent_debackslash(mtz_file_name)));
	    v.push_back(coot::util::single_quote(f_obs_col));
	    v.push_back(coot::util::single_quote(sigfobs_col));
	    std::string c = languagize_command(v);
	    std::cout << "command: " << c << std::endl;
#ifdef USE_GUILE
	    safe_scheme_command(c);
#else
#ifdef USE_PYTHON
	    safe_python_command(c);
#endif
#endif
	 }
      }
      std::vector<std::string> command_strings;
      command_strings.push_back("calc-phases-generic");
      command_strings.push_back(mtz_file_name);
      add_to_history(command_strings);
   }
}

/*! \brief Calculate SFs (using refmac optionally) from an MTZ file
  and generate a map. Get F and SIGF automatically (first of their
  type) from the mtz file.

@return the new molecule number, -1 on a problem. */
int map_from_mtz_by_refmac_calc_phases(const char *mtz_file_name, 
				       const char *f_col, 
				       const char *sigf_col, 
				       int imol_coords) {

   int istat = -1;
   std::vector<std::string> command_strings;
   command_strings.push_back("map-from-mtz-by-refmac-calc-phases");
   command_strings.push_back(mtz_file_name);
   command_strings.push_back(f_col);
   command_strings.push_back(sigf_col);
   command_strings.push_back(graphics_info_t::int_to_string(imol_coords));
   add_to_history(command_strings);
   return istat;
} 


/*  ------------------------------------------------------------------------ */
/*                   model/fit/refine functions:                             */
/*  ------------------------------------------------------------------------ */
void set_model_fit_refine_rotate_translate_zone_label(const char *txt) {
   graphics_info_t::model_fit_refine_rotate_translate_zone_string = txt;
   // if we have the dialog open we shall change the label
   if (graphics_info_t::model_fit_refine_dialog) {
     update_model_fit_refine_dialog_buttons(graphics_info_t::model_fit_refine_dialog);
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("set-model-fit-refine-rotate-translate-zone-label");
   command_strings.push_back(txt);
   add_to_history(command_strings);
}

void set_model_fit_refine_place_atom_at_pointer_label(const char *txt) {
   graphics_info_t::model_fit_refine_place_atom_at_pointer_string = txt;
   std::vector<std::string> command_strings;
   command_strings.push_back("set-model-fit-refine-atom-at-pointer-label");
   command_strings.push_back(txt);
   add_to_history(command_strings);
}

int copy_molecule(int imol) {
   int iret = -1;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      iret = g.copy_model_molecule(imol);
   }
   if (is_valid_map_molecule(imol)) {
      int new_mol_number = graphics_info_t::create_molecule();
      std::string label = "Copy_of_";
      label += graphics_info_t::molecules[imol].name_;
      graphics_info_t::molecules[new_mol_number].new_map(graphics_info_t::molecules[imol].xmap, label);
      if (graphics_info_t::molecules[imol].is_difference_map_p()) {
	 graphics_info_t::molecules[new_mol_number].set_map_is_difference_map();
      }
      iret = new_mol_number;
   }
   if (iret != -1) 
      graphics_draw();
   std::vector<std::string> command_strings;
   command_strings.push_back("copy-molecule");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   add_to_history(command_strings);
   return iret;
}

int
add_ligand_delete_residue_copy_molecule(int imol_ligand_new, 
					const char *chain_id_ligand_new,
					int res_no_ligand_new, 
					int imol_current,
					const char *chain_id_ligand_current,
					int res_no_ligand_current) {

   int r = -1; 
   bool created_flag = 0; // we only want to do this once

//    std::cout << "debug:: searching for residue :"
// 	     << chain_id_ligand_current << ": " << res_no_ligand_current
// 	     << " in molecule " << imol_current
// 	     << " replacing it with atom of :" << chain_id_ligand_new << ": "
// 	     << res_no_ligand_new << " of molecule " << imol_ligand_new
// 	     << std::endl;
   
   if (! is_valid_model_molecule(imol_ligand_new)) {
      std::cout << "WARNING:: ligand molecule " << imol_ligand_new << " is not a valid molecule"
		<< std::endl;
   } else { 
      if (! is_valid_model_molecule(imol_current)) {
	 std::cout << "WARNING:: (surrounding) molecule " << imol_current
		   << " is not a valid molecule" << std::endl;
      } else { 
	 graphics_info_t g;
	 mmdb::Residue *res_ligand_new =
	    g.molecules[imol_ligand_new].get_residue(chain_id_ligand_new,
						     res_no_ligand_new, "");
	 mmdb::Residue *res_ligand_current =
	    g.molecules[imol_current].get_residue(chain_id_ligand_current,
						  res_no_ligand_current, "");
	 if (!res_ligand_current || !res_ligand_new) {

	    // so which was it then?
	    if (! res_ligand_current)
	       std::cout << "WARNING:: Oops, reference residue (being replaced) not found"
			 << std::endl;
	    if (! res_ligand_new)
	       std::cout << "WARNING:: Oops, new residue (replacing other) not found"
			 << std::endl;
	       
	 } else { 
	    mmdb::Manager *n = new mmdb::Manager;
	    n->Copy(g.molecules[imol_current].atom_sel.mol, mmdb::MMDBFCM_All);

	    // now find the residue in imol_ligand_current.
	    // and replace its atoms.
	    int imodel = 1;
	    mmdb::Model *model_p = n->GetModel(imodel);
	    mmdb::Chain *chain_p;
	    int nchains = model_p->GetNumberOfChains();
	    for (int ichain=0; ichain<nchains; ichain++) {
	       chain_p = model_p->GetChain(ichain);
	       if (! strncmp(chain_id_ligand_current, chain_p->GetChainID(), 4)) {
		  int nres = chain_p->GetNumberOfResidues();
		  mmdb::Residue *residue_p;
		  mmdb::Atom *at;
		  for (int ires=0; ires<nres; ires++) {
		     residue_p = chain_p->GetResidue(ires);
		     if (residue_p->GetSeqNum() == res_no_ligand_current) {

			// delete the current atoms (backwards so that
			// we don't have reindexing problems)
			// 
			int n_atoms = residue_p->GetNumberOfAtoms();
			for (int iat=n_atoms-1; iat>=0; iat--) {
			   residue_p->DeleteAtom(iat);
			}

			n_atoms = res_ligand_new->GetNumberOfAtoms();
			for (int iat=0; iat<n_atoms; iat++) {
			   mmdb::Atom *at_copy = new mmdb::Atom;
			   at_copy->Copy(res_ligand_new->GetAtom(iat));
			   residue_p->AddAtom(at_copy);
			}
			residue_p->SetResName(res_ligand_new->GetResName());
			n->FinishStructEdit();

			r = graphics_info_t::create_molecule();
			atom_selection_container_t asc = make_asc(n);
			std::string label = "Copy_of_";
			label += coot::util::int_to_string(imol_current);
			label += "_with_";
			label += chain_id_ligand_current;
			label += coot::util::int_to_string(res_no_ligand_current);
			label += "_replaced";
			g.molecules[r].install_model(r, asc, label, 1);
			created_flag = 1;
			break;
		     }

		     if (created_flag)
			break;
		  }
	       }
	       if (created_flag)
		  break;
	    }
	 }
      }
   }

   if (created_flag) {
      graphics_draw();
   }
   std::cout << "add_ligand_delete_residue_copy_molecule() returns " << r << std::endl;
   return r;
} 


/*! \brief replace the parts of molecule number imol that are
  duplicated in molecule number imol_frag */
int replace_fragment(int imol_target, int imol_fragment,
		     const char *mmdb_atom_selection_str) {

   int istate = 0;
   if (is_valid_model_molecule(imol_target)) {
      if (is_valid_model_molecule(imol_fragment)) {
	 mmdb::Manager *mol = graphics_info_t::molecules[imol_fragment].atom_sel.mol;
	 int SelHnd = mol->NewSelection();
	 mol->Select(SelHnd, mmdb::STYPE_ATOM, mmdb_atom_selection_str, mmdb::SKEY_OR);
	 mmdb::Manager *mol_new =
	    coot::util::create_mmdbmanager_from_atom_selection(mol, SelHnd);
	 atom_selection_container_t asc = make_asc(mol_new);
	 istate = graphics_info_t::molecules[imol_target].replace_fragment(asc);
	 mol->DeleteSelection(SelHnd);
	 graphics_draw();
      }
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("replace-fragement");
   command_strings.push_back(graphics_info_t::int_to_string(imol_target));
   command_strings.push_back(single_quote(mmdb_atom_selection_str));
   add_to_history(command_strings);
   return istate;
}

/*! \brief copy the given residue range from the reference chain to the target chain 

resno_range_start and resno_range_end are inclusive. */
int copy_residue_range(int imol_target,    const char *chain_id_target, 
		       int imol_reference, const char *chain_id_reference, 
		       int resno_range_start, int resno_range_end) { 
   
   int status = 0; 
   if (! (is_valid_model_molecule(imol_target))) { 
      std::cout << "WARNING:: not a valid model molecule " 
		<< imol_target << std::endl;
   } else { 
      if (! (is_valid_model_molecule(imol_reference))) { 
	 std::cout << "WARNING:: not a valid model molecule " 
		   << imol_reference << std::endl;
      } else { 
	 mmdb::Chain *chain_p = graphics_info_t::molecules[imol_reference].get_chain(chain_id_reference);
	 if (! chain_p) { 
	    std::cout << "WARNING:: not chain " << chain_id_reference << " in molecule " 
		      << imol_reference << std::endl;
	 } else { 
	    mmdb::Chain *chain_pt = graphics_info_t::molecules[imol_target].get_chain(chain_id_target);
	    if (! chain_pt) { 
	       std::cout << "WARNING:: not chain " << chain_id_target << " in molecule " 
			 << imol_target << std::endl;
	    } else { 
	       clipper::RTop_orth rtop = clipper::RTop_orth::identity();
	       status = graphics_info_t::molecules[imol_target].copy_residue_range(chain_p, chain_pt, 
								  resno_range_start, resno_range_end,
								  rtop);
	       graphics_draw();
	    }
	 }
      }
   }
   return status;
} 


void set_refinement_move_atoms_with_zero_occupancy(int state) {
   // convert a int to a bool.
   graphics_info_t::refinement_move_atoms_with_zero_occupancy_flag = state;
}

int refinement_move_atoms_with_zero_occupancy_state() {
   // convert a bool to an int.
   return graphics_info_t::refinement_move_atoms_with_zero_occupancy_flag;
} 


/*  ------------------------------------------------------------------------ */
/*                         backup/undo functions:                            */
/*  ------------------------------------------------------------------------ */

void turn_off_backup(int imol) {
   
   if (is_valid_model_molecule(imol)) 
      graphics_info_t::molecules[imol].turn_off_backup();
   std::vector<std::string> command_strings;
   command_strings.push_back("turn-off-backup");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   add_to_history(command_strings);
} 

void turn_on_backup(int imol) {
   if (is_valid_model_molecule(imol))
      graphics_info_t::molecules[imol].turn_on_backup();
   std::vector<std::string> command_strings;
   command_strings.push_back("turn-on-backup");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   add_to_history(command_strings);
} 

int apply_undo() {		/* "Undo" button callback */
   graphics_info_t g;
   int r = g.apply_undo();
   add_to_history_simple("apply-undo");
   return r;
}

int  apply_redo() { 
   graphics_info_t g;
   int r = g.apply_redo();
   add_to_history_simple("apply-redo");
   return r;
}

void set_undo_molecule(int imol) {

   // Potentially a problem here.  What if we set the undo molecule to
   // a molecule that has accidentally just deleted the only ligand
   // residue.
   //
   // Too bad.
   //
//    if (is_valid_model_molecule(imol)) {
//       graphics_info_t g;
//       g.set_undo_molecule_number(imol);
//    }
   
   // 20060522 so how about I check that the index is within limits?
   //          and then ask if if the mol is valid (rather than the
   //          number of atoms selected):

   if ((imol >= 0) && (imol < graphics_info_t::n_molecules())) {
      graphics_info_t g;
      if (g.molecules[imol].atom_sel.mol) {
	 std::cout << "INFO:: undo molecule number set to: " << imol << std::endl;
	 g.set_undo_molecule_number(imol);
      }
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("set-undo-molecule");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   add_to_history(command_strings);
}

void set_unpathed_backup_file_names(int state) {
   graphics_info_t::unpathed_backup_file_names_flag = state;
   std::vector<std::string> command_strings;
   command_strings.push_back("set-unpathed-backup-file-names");
   command_strings.push_back(graphics_info_t::int_to_string(state));
   add_to_history(command_strings);
}

int  unpathed_backup_file_names_state() {
   add_to_history_simple("unpathed-backup-file-names-state");
   return graphics_info_t::unpathed_backup_file_names_flag;
}

void set_backup_compress_files(int state) {
   
  graphics_info_t::backup_compress_files_flag = state;
  std::vector<std::string> command_strings;
  command_strings.push_back("set-backup-compress-files");
  command_strings.push_back(graphics_info_t::int_to_string(state));
  add_to_history(command_strings);
} 

int backup_compress_files_state() {
   
  int state = graphics_info_t::backup_compress_files_flag;
  return state;
} 


/*  ------------------------------------------------------------------------ */
/*                    terminal residue functions:                            */
/*  ------------------------------------------------------------------------ */

void do_add_terminal_residue(short int state) { 

   graphics_info_t g;
   g.in_terminal_residue_define = state;
   if (state) {
      int imol_map = g.Imol_Refinement_Map();
      if (imol_map >= 0) { 
	 std::cout << "click on an atom of a terminal residue" << std::endl;
	 g.pick_cursor_maybe();
	 g.pick_pending_flag = 1;
      } else {
	 g.show_select_map_dialog();
	 g.in_terminal_residue_define = 0;
	 g.model_fit_refine_unactive_togglebutton("model_refine_dialog_fit_terminal_residue_togglebutton");
	 g.normal_cursor();
      }
   } else {
      g.normal_cursor();
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("do-add-terminal-residue");
   command_strings.push_back(graphics_info_t::int_to_string(state));
   add_to_history(command_strings);
}

void 
set_add_terminal_residue_n_phi_psi_trials(int n) { 
   graphics_info_t g;
   g.add_terminal_residue_n_phi_psi_trials = n;
   std::vector<std::string> command_strings;
   command_strings.push_back("set-add-terminal-residue-n-phi-psi-trials");
   command_strings.push_back(graphics_info_t::int_to_string(n));
   add_to_history(command_strings);
}

void
set_add_terminal_residue_add_other_residue_flag(int i) {
   graphics_info_t::add_terminal_residue_add_other_residue_flag = i;
   std::vector<std::string> command_strings;
   command_strings.push_back("set-add-terminal-residue-add-other-residue-flag");
   command_strings.push_back(graphics_info_t::int_to_string(i));
   add_to_history(command_strings);
}

void set_terminal_residue_do_rigid_body_refine(short int v) { 

   graphics_info_t g;
   g.terminal_residue_do_rigid_body_refine = v;
   std::vector<std::string> command_strings;
   command_strings.push_back("set-terminal-residue-do-rigid-body-refine");
   command_strings.push_back(graphics_info_t::int_to_string(v));
   add_to_history(command_strings);

}

void set_add_terminal_residue_do_post_refine(short int istat) {
   graphics_info_t::add_terminal_residue_do_post_refine = istat;
   std::vector<std::string> command_strings;
   command_strings.push_back("set-add-terminal-residue-do-post-refine");
   command_strings.push_back(graphics_info_t::int_to_string(istat));
   add_to_history(command_strings);
}

int add_terminal_residue_do_post_refine_state() {
  int i = graphics_info_t::add_terminal_residue_do_post_refine;
  return i;
}


int add_terminal_residue_immediate_addition_state() {
   int i = graphics_info_t::add_terminal_residue_immediate_addition_flag;
   return i;
} 

void set_add_terminal_residue_immediate_addition(int i) {
   graphics_info_t::add_terminal_residue_immediate_addition_flag = i;
}

// return 0 on failure, 1 on success
//
int add_terminal_residue(int imol, 
			 const char *chain_id, 
			 int residue_number,
			 const char *residue_type,
			 int immediate_add) {

   int istate = 0;
   graphics_info_t g;
   std::string residue_type_string = residue_type;
   
   int imol_map = g.Imol_Refinement_Map();
   if (imol_map == -1) {
      std::cout << "WARNING:: Refinement/Fitting map is not set." << std::endl;
      std::cout << "          addition of terminal residue terminated." << std::endl;
   } else { 
      if (is_valid_model_molecule(imol)) { 
	 // We don't do this as a member function of
	 // molecule_class_info_t because we are using
	 // graphics_info_t::execute_add_terminal_residue, which
	 // does the molecule manipulations outside of the
	 // molecule_class_info_t class.
	 //

	 graphics_info_t g;
	 int atom_indx = atom_index(imol, chain_id, residue_number, " CA ");
	 if (atom_indx >= 0) {
	    std::string term_type = g.molecules[imol].get_term_type(atom_indx);
	    std::string inscode = "";
	    mmdb::Residue *res_p =
	       g.molecules[imol].residue_from_external(residue_number, inscode,
						       std::string(chain_id));

	    
	    g.execute_add_terminal_residue(imol, term_type, res_p, chain_id,
					   residue_type_string, immediate_add);
	    
	    istate = 1;
	 } else {
	    std::cout << "WARNING:: in add_terminal_residue: "
		      << " Can't find atom index for CA in residue "
		      << residue_number << " " << chain_id << std::endl;
	 }
      }
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("add-terminal-residue");
   command_strings.push_back(graphics_info_t::int_to_string(imol));
   command_strings.push_back(single_quote(chain_id));
   command_strings.push_back(graphics_info_t::int_to_string(residue_number));
   command_strings.push_back(graphics_info_t::int_to_string(immediate_add));
   add_to_history(command_strings);
   return istate;
}

/*! \brief Add a terminal residue using given phi and psi angles
 */
int add_terminal_residue_using_phi_psi(int imol, const char *chain_id, int res_no, 
				       const char *residue_type, float phi, float psi) {

   int status = 0;
   if (is_valid_model_molecule(imol)) {
      status = graphics_info_t::molecules[imol].add_terminal_residue_using_phi_psi(chain_id, res_no, residue_type, phi, psi);
      if (status)
	 graphics_draw();
   } 
   return status;

} 


void set_add_terminal_residue_default_residue_type(const char *type) {

   if (type) 
      graphics_info_t::add_terminal_residue_type = type;
   std::string cmd = "set-add-terminal-residue-default-residue-type";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(type));
   add_to_history_typed(cmd, args);
}



/*  ----------------------------------------------------------------------- */
/*                  rotate/translate buttons                                */
/*  ----------------------------------------------------------------------- */

void do_rot_trans_setup(short int state) { 
   graphics_info_t g;
   g.in_rot_trans_object_define = state;
   if (state){ 
      g.pick_cursor_maybe();
      std::cout << "click on 2 atoms to define a zone" << std::endl;
      g.pick_pending_flag = 1;
   } else {
      g.pick_pending_flag = 0;
      g.normal_cursor();
   }
   std::string cmd = "do-rot-trans-setup";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}


void rot_trans_reset_previous() { 
   graphics_info_t g;
   // rot_trans adjustments:
   for (int i=0; i<6; i++) 
      g.previous_rot_trans_adjustment[i] = -10000;
   add_to_history_simple("rot-trans-reset-previous");
}

void set_rotate_translate_zone_rotates_about_zone_centre(int istate) {
   graphics_info_t::rot_trans_zone_rotates_about_zone_centre = istate;
   std::string cmd = "set-rotate-translate-zone-rotates-about-zone-centre";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
} 

void set_rot_trans_object_type(short int rt_type) { /* zone, chain, mol */

   graphics_info_t::rot_trans_object_type = rt_type;
}

int
get_rot_trans_object_type() {

   return graphics_info_t::rot_trans_object_type;
}

/*  ----------------------------------------------------------------------- */
/*                  spin search                                             */
/*  ----------------------------------------------------------------------- */
void spin_search_by_atom_vectors(int imol_map, int imol, const std::string &chain_id,
				 int resno, const std::string &ins_code,
				 const std::pair<std::string, std::string> &direction_atoms,
				 const std::vector<std::string> &moving_atoms_list) {


   if (is_valid_map_molecule(imol_map)) {

      if (is_valid_model_molecule(imol)) {

	 graphics_info_t::molecules[imol].spin_search(graphics_info_t::molecules[imol_map].xmap,
						      chain_id, resno, ins_code,
						      direction_atoms, moving_atoms_list);
	 graphics_draw();
	 
      } else {
	 std::cout << "Molecule number " << imol << " is not a valid model" << std::endl;
      }
   } else {
      std::cout << "Molecule number " << imol_map << " is not a valid map" << std::endl;
   }
} 

#ifdef USE_GUILE
/*! \brief for the given residue, spin the atoms in moving_atom_list
  around the bond defined by direction_atoms_list looking for the best
  fit to density of imom_map map of the first atom in
  moving_atom_list.  Works (only) with atoms in altconf "" */
void spin_search(int imol_map, int imol, const char *chain_id, int resno,
		 const char *ins_code, SCM direction_atoms_list, SCM moving_atoms_list) {

   std::vector<std::string> s = generic_list_to_string_vector_internal(direction_atoms_list);

   if (s.size() == 2) { 
      std::pair<std::string, std::string> p(s[0], s[1]);
      
      spin_search_by_atom_vectors(imol_map, imol, chain_id, resno, ins_code, p,
				  generic_list_to_string_vector_internal(moving_atoms_list));
   } else {
      std::cout << "bad direction atom pair" << std::endl;
   } 
} 
#endif
#ifdef USE_PYTHON
void spin_search_py(int imol_map, int imol, const char *chain_id, int resno,
                 const char *ins_code, PyObject *direction_atoms_list, PyObject *moving_atoms_list) {

   std::vector<std::string> s = generic_list_to_string_vector_internal_py(direction_atoms_list);

   if (s.size() == 2) {
      std::pair<std::string, std::string> p(s[0], s[1]);

      spin_search_by_atom_vectors(imol_map, imol, chain_id, resno, ins_code, p,
                                  generic_list_to_string_vector_internal_py(moving_atoms_list));
   } else {
      std::cout << "bad direction atom pair" << std::endl;
   }
}
#endif // PYTHON


/*  ----------------------------------------------------------------------- */
/*                  delete residue                                          */
/*  ----------------------------------------------------------------------- */
void delete_residue(int imol, const char *chain_id, int resno, const char *inscode) {

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      int model_number_ANY = mmdb::MinInt4;
      short int istat =
	 g.molecules[imol].delete_residue(model_number_ANY, chain_id, resno,
					  std::string(inscode));
      if (istat) { 
	 // now if the go to atom widget was being displayed, we need to
	 // redraw the residue list and atom list (if the molecule of the
	 // residue and atom list is the molecule that has just been
	 // deleted)

	 g.update_go_to_atom_window_on_changed_mol(imol);

	 graphics_draw();
      } else { 
	 std::cout << "failed to delete residue " << chain_id 
		   << " " << resno << "\n";
      }
      std::vector<std::string> command_strings;
      command_strings.push_back("delete-residue");
      command_strings.push_back(g.int_to_string(imol));
      command_strings.push_back(single_quote(chain_id));
      command_strings.push_back(g.int_to_string(resno));
      command_strings.push_back(single_quote(std::string(inscode)));
      add_to_history(command_strings);
   } else {
      add_status_bar_text("Oops bad molecule from whcih to delete a residue");
   }
}

void delete_residue_hydrogens(int imol,
			      const char *chain_id,
			      int resno,
			      const char *inscode,
			      const char *altloc) {

   graphics_info_t g;
   if (is_valid_model_molecule(imol)) { 
      short int istat = g.molecules[imol].delete_residue_hydrogens(chain_id, resno, inscode, altloc);
      if (istat) { 
	 // now if the go to atom widget was being displayed, we need to
	 // redraw the residue list and atom list (if the molecule of the
	 // residue and atom list is the molecule that has just been
	 // deleted)

	 g.update_go_to_atom_window_on_changed_mol(imol);
	 graphics_draw();

      } else { 
	 std::cout << "failed to delete residue hydrogens " << chain_id 
		   << " " << resno << "\n";
      }
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("delete-residue-hydrogens");
   command_strings.push_back(g.int_to_string(imol));
   command_strings.push_back(single_quote(chain_id));
   command_strings.push_back(g.int_to_string(resno));
   command_strings.push_back(single_quote(inscode));
   command_strings.push_back(single_quote(altloc));
   add_to_history(command_strings);
} 


void
delete_residue_with_full_spec(int imol,
			      int imodel,
			      const char *chain_id,
			      int resno,
			      const char *inscode,
			      const char *altloc) {
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) {
      std::string altconf(altloc);
      short int istat =
	 g.molecules[imol].delete_residue_with_full_spec(imodel, chain_id, resno, inscode, altconf);
   
      if (istat) { 
	 // now if the go to atom widget was being displayed, we need to
	 // redraw the residue list and atom list (if the molecule of the
	 // residue and atom list is the molecule that has just been
	 // deleted)
	 // 
	 g.update_go_to_atom_window_on_changed_mol(imol);

	 graphics_draw();
      } else { 
	 std::cout << "failed to delete residue atoms " << chain_id 
		   << " " << resno << " :" << altconf << ":\n";
      }
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("delete-residue-with-full_spec");
   command_strings.push_back(g.int_to_string(imol));
   command_strings.push_back(g.int_to_string(imodel));
   command_strings.push_back(single_quote(chain_id));
   command_strings.push_back(g.int_to_string(resno));
   command_strings.push_back(single_quote(inscode));
   command_strings.push_back(single_quote(altloc));
   add_to_history(command_strings);
}

void delete_residue_sidechain(int imol, const char *chain_id, int resno, const char *ins_code,
			      short int do_delete_dialog) {

   std::string inscode(ins_code);
   graphics_info_t g;

   if (is_valid_model_molecule(imol)) { 
      mmdb::Residue *residue_p =
	 graphics_info_t::molecules[imol].get_residue(chain_id, resno, ins_code);
      if (residue_p) {
	 graphics_info_t g;
	 coot::residue_spec_t spec(residue_p);
	 g.delete_residue_from_geometry_graphs(imol, spec);
      }
      short int istat =
	 g.molecules[imol].delete_residue_sidechain(std::string(chain_id), resno,
						    inscode);
      
      if (istat) {
	 g.update_go_to_atom_window_on_changed_mol(imol);
	 graphics_draw();
      }

      if (delete_item_widget_is_being_shown()) {
	 if (delete_item_widget_keep_active_on()) { 
	    // dont destroy it
	 } else {
	    store_delete_item_widget_position(); // and destroy it.
	 }
      }
   }

   std::string cmd = "delete-residue-sidechain";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno);
   args.push_back(ins_code);
   add_to_history_typed(cmd, args);
}

/*! \brief delete all hydrogens in molecule */
int delete_hydrogens(int imol) {

   int n_deleted = 0;
   if (is_valid_model_molecule(imol)) {
      n_deleted = graphics_info_t::molecules[imol].delete_hydrogens();
      if (n_deleted)
	 graphics_draw();
   }
   return n_deleted;
} 



void set_add_alt_conf_new_atoms_occupancy(float f) {  /* default 0.5 */

   graphics_info_t g;
   g.add_alt_conf_new_atoms_occupancy = f;
   std::string cmd = "set-add-alt-conf-new-atoms-occupancy";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
}

void set_numerical_gradients(int istate) {

   graphics_info_t::do_numerical_gradients = istate;
} 



int set_atom_attribute(int imol, const char *chain_id, int resno, const char *ins_code, const char *atom_name, const char*alt_conf, const char *attribute_name, float val) {
   int istat = 0;
   if (is_valid_model_molecule(imol)) {
      istat = graphics_info_t::molecules[imol].set_atom_attribute(chain_id, resno, ins_code, atom_name, alt_conf, attribute_name, val);
   }
   graphics_draw();
   std::string cmd = "set-atom-attribute";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno);
   args.push_back(coot::util::single_quote(ins_code));
   args.push_back(coot::util::single_quote(atom_name));
   args.push_back(coot::util::single_quote(alt_conf));
   args.push_back(coot::util::single_quote(attribute_name));
   args.push_back(val);
   add_to_history_typed(cmd, args);
   return istat;
} 

int set_atom_string_attribute(int imol, const char *chain_id, int resno, const char *ins_code, const char *atom_name, const char*alt_conf, const char *attribute_name, const char *attribute_value) {
   int istat = 0; 
   if (is_valid_model_molecule(imol)) {
      istat = graphics_info_t::molecules[imol].set_atom_string_attribute(chain_id, resno, ins_code, atom_name, alt_conf, attribute_name, attribute_value);
      graphics_draw();
   }
   std::string cmd = "set-atom-string-attribute";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno);
   args.push_back(coot::util::single_quote(ins_code));
   args.push_back(coot::util::single_quote(atom_name));
   args.push_back(coot::util::single_quote(alt_conf));
   args.push_back(coot::util::single_quote(attribute_name));
   args.push_back(coot::util::single_quote(attribute_value));
   add_to_history_typed(cmd, args);
   return istat;
}

#ifdef USE_GUILE
int set_atom_attributes(SCM attribute_expression_list) {

   int r= 0;
   SCM list_length_scm = scm_length(attribute_expression_list);
   int list_length = scm_to_int(list_length_scm);
   int n = graphics_info_t::n_molecules(); 
   std::vector<std::vector<coot::atom_attribute_setting_t> > v(n);

   if (list_length > 0) {
      for (int iattr=0; iattr<list_length; iattr++) { 
	 SCM iattr_scm = SCM_MAKINUM(iattr);
	 SCM attribute_expression = scm_list_ref(attribute_expression_list, iattr_scm);
	 if (scm_is_true(scm_list_p(attribute_expression))) { 
	    SCM attr_expression_length_scm = scm_length(attribute_expression);
	    int attr_expression_length = scm_to_int(attr_expression_length_scm);
	    if (attr_expression_length != 8) {
	       std::cout << "Incomplete attribute expression: "
			 << scm_to_locale_string(display_scm(attribute_expression))
			 << std::endl;		  
	    } else {
	       SCM imol_scm            = scm_list_ref(attribute_expression, SCM_MAKINUM(0));
	       SCM chain_id_scm        = scm_list_ref(attribute_expression, SCM_MAKINUM(1));
	       SCM resno_scm           = scm_list_ref(attribute_expression, SCM_MAKINUM(2));
	       SCM ins_code_scm        = scm_list_ref(attribute_expression, SCM_MAKINUM(3));
	       SCM atom_name_scm       = scm_list_ref(attribute_expression, SCM_MAKINUM(4));
	       SCM alt_conf_scm        = scm_list_ref(attribute_expression, SCM_MAKINUM(5));
	       SCM attribute_name_scm  = scm_list_ref(attribute_expression, SCM_MAKINUM(6));
	       SCM attribute_value_scm = scm_list_ref(attribute_expression, SCM_MAKINUM(7));
	       int imol = scm_to_int(imol_scm);
	       if (is_valid_model_molecule(imol)) {
		  std::string chain_id = scm_to_locale_string(chain_id_scm);
		  int resno = scm_to_int(resno_scm);
		  
		  std::string inscode        = "-*-unset-*-:";
		  std::string atom_name      = "-*-unset-*-:";
		  std::string alt_conf       = "-*-unset-*-:";
		  std::string attribute_name = "-*-unset-*-:";

		  if (scm_is_true(scm_string_p(ins_code_scm))) 
		      inscode        = scm_to_locale_string(ins_code_scm);
		  if (scm_is_true(scm_string_p(atom_name_scm))) 
		     atom_name      = scm_to_locale_string(atom_name_scm);
		  if (scm_is_true(scm_string_p(alt_conf_scm))) 
		     alt_conf       = scm_to_locale_string(alt_conf_scm); 
		  if (scm_is_true(scm_string_p(attribute_name_scm))) 
		     attribute_name = scm_to_locale_string(attribute_name_scm);

		  if ((inscode        == "-*-unset-*-:") ||
		      (atom_name      == "-*-unset-*-:") ||
		      (alt_conf       == "-*-unset-*-:") ||
		      (attribute_name == "-*-unset-*-:")) {

		     std::cout << "WARNING:: bad attribute expression: "
			       << scm_to_locale_string(display_scm(attribute_expression))
			       << std::endl;

		  } else { 
		      
		     coot::atom_attribute_setting_help_t att_val;
		     if (scm_is_true(scm_string_p(attribute_value_scm))) {
			// std::cout << "a string value :" << att_val.s << ":" << std::endl;
			att_val = coot::atom_attribute_setting_help_t(scm_to_locale_string(attribute_value_scm));
		     } else {
			att_val = coot::atom_attribute_setting_help_t(float(scm_to_double(attribute_value_scm)));
			// std::cout << "a float value :" << att_val.val << ":" << std::endl;
		     } 
		     v[imol].push_back(coot::atom_attribute_setting_t(chain_id, resno, inscode, atom_name, alt_conf, attribute_name, att_val));
		     //		     std::cout << "DEBUG:: Added attribute: "
		     //                        << scm_to_locale_string(display_scm(attribute_expression))
		     //        << std::endl;
		  }
	       }
	    }
	 }
      }
   }

   for (int i=0; i<n; i++) {
      if (v[i].size() > 0) {
	 // std::cout << "DEBUG:: setting atom attributes for molecule " << i << " " << v[i].size() 
	 //           << " attributes to set " << std::endl;
	 graphics_info_t::molecules[i].set_atom_attributes(v[i]);
      } 
   }
   if (v.size() > 0)
      graphics_draw();
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
int set_atom_attributes_py(PyObject *attribute_expression_list) {

   int r= 0;
   int list_length = PyObject_Length(attribute_expression_list);
   int n = graphics_info_t::n_molecules(); 
   std::vector<std::vector<coot::atom_attribute_setting_t> > v(n);
   PyObject *attribute_expression;
   PyObject *imol_py;
   PyObject *chain_id_py;
   PyObject *resno_py;
   PyObject *ins_code_py;
   PyObject *atom_name_py;
   PyObject *alt_conf_py;
   PyObject *attribute_name_py;
   PyObject *attribute_value_py;

   if (list_length > 0) {
      for (int iattr=0; iattr<list_length; iattr++) { 
	 attribute_expression = PyList_GetItem(attribute_expression_list, iattr);
	 if (PyList_Check(attribute_expression)) { 
	    int attr_expression_length = PyObject_Length(attribute_expression);
	    if (attr_expression_length != 8) {
	       std::cout << "Incomplete attribute expression: "
			 << PyString_AsString(attribute_expression)
			 << std::endl;		  
	    } else {
	       imol_py            = PyList_GetItem(attribute_expression, 0);
	       chain_id_py        = PyList_GetItem(attribute_expression, 1);
	       resno_py           = PyList_GetItem(attribute_expression, 2);
	       ins_code_py        = PyList_GetItem(attribute_expression, 3);
	       atom_name_py       = PyList_GetItem(attribute_expression, 4);
	       alt_conf_py        = PyList_GetItem(attribute_expression, 5);
	       attribute_name_py  = PyList_GetItem(attribute_expression, 6);
	       attribute_value_py = PyList_GetItem(attribute_expression, 7);
	       int imol = PyInt_AsLong(imol_py);
	       if (is_valid_model_molecule(imol)) {
		  std::string chain_id = PyString_AsString(chain_id_py);
		  int resno = PyInt_AsLong(resno_py);
		  
		  std::string inscode        = "-*-unset-*-:";
		  std::string atom_name      = "-*-unset-*-:";
		  std::string alt_conf       = "-*-unset-*-:";
		  std::string attribute_name = "-*-unset-*-:";

		  if (PyString_Check(ins_code_py)) 
		    inscode        = PyString_AsString(ins_code_py);
		  if (PyString_Check(atom_name_py))
		    atom_name      = PyString_AsString(atom_name_py);
		  if (PyString_Check(alt_conf_py))
		    alt_conf       = PyString_AsString(alt_conf_py); 
		  if (PyString_Check(attribute_name_py)) 
		    attribute_name = PyString_AsString(attribute_name_py);

		  if ((inscode        == "-*-unset-*-:") ||
		      (atom_name      == "-*-unset-*-:") ||
		      (alt_conf       == "-*-unset-*-:") ||
		      (attribute_name == "-*-unset-*-:")) {

		     std::cout << "WARNING:: bad attribute expression: "
			       << PyString_AsString(attribute_expression)
			       << std::endl;

		  } else { 
		      
		     coot::atom_attribute_setting_help_t att_val;
		     if (PyString_Check(attribute_value_py)) {
			// std::cout << "a string value :" << att_val.s << ":" << std::endl;
			att_val = coot::atom_attribute_setting_help_t(PyString_AsString(attribute_value_py));
		     } else {
			att_val = coot::atom_attribute_setting_help_t(float(PyFloat_AsDouble(attribute_value_py)));
			// std::cout << "a float value :" << att_val.val << ":" << std::endl;
		     } 
		     v[imol].push_back(coot::atom_attribute_setting_t(chain_id, resno, inscode, atom_name, alt_conf, attribute_name, att_val));
		     //		     std::cout << "DEBUG:: Added attribute: "
		     //                        << scm_to_locale_string(display_scm(attribute_expression))
		     //        << std::endl;
		  }
	       }
	    }
	 }
      }
   }

   for (int i=0; i<n; i++) {
      if (v[i].size() > 0){
	 graphics_info_t::molecules[i].set_atom_attributes(v[i]);
      } 
   }
   if (v.size() > 0)
      graphics_draw();
   return r;
}
#endif // USE_PYTHON



void set_residue_name(int imol, const char *chain_id, int res_no, const char *ins_code, const char *new_residue_name) {

   if (chain_id && ins_code && new_residue_name) { 
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t::molecules[imol].set_residue_name(chain_id, res_no, ins_code, new_residue_name);
	 graphics_draw();
      }
      std::string cmd = "set-residue-name";
      std::vector<coot::command_arg_t> args;
      args.push_back(imol);
      args.push_back(coot::util::single_quote(chain_id));
      args.push_back(res_no);
      args.push_back(coot::util::single_quote(ins_code));
      args.push_back(coot::util::single_quote(new_residue_name));
      add_to_history_typed(cmd, args);
   }
} 



#ifdef USE_GUILE
// return a list of refmac parameters.  Used so that we can test that
// the save state of a refmac map works correctly.
SCM refmac_parameters_scm(int imol) {

   SCM r = SCM_EOL;
   if (is_valid_map_molecule(imol)) { 
      std::vector<coot::atom_attribute_setting_help_t>
	 refmac_params = graphics_info_t::molecules[imol].get_refmac_params();
      if (refmac_params.size() > 0) {
	 // values have to go in in reverse order, as usual.
	 for (int i=(int(refmac_params.size())-1); i>=0; i--) {
	    if (refmac_params[i].type == coot::atom_attribute_setting_help_t::IS_STRING)
	       r = scm_cons(scm_makfrom0str(refmac_params[i].s.c_str()) ,r);
	    if (refmac_params[i].type == coot::atom_attribute_setting_help_t::IS_FLOAT)
	       r = scm_cons(scm_double2num(refmac_params[i].val) ,r);
	    if (refmac_params[i].type == coot::atom_attribute_setting_help_t::IS_INT)
	       r = scm_cons(SCM_MAKINUM(refmac_params[i].i) ,r);
	 }
      }
   }
   return r;
}

#endif	/* USE_GUILE */

#ifdef USE_PYTHON
PyObject *refmac_parameters_py(int imol) {

   PyObject *r = PyList_New(0);
   if (is_valid_map_molecule(imol)) { 
      std::vector<coot::atom_attribute_setting_help_t>
	 refmac_params = graphics_info_t::molecules[imol].get_refmac_params();
      if (refmac_params.size() > 0) {
	 // values have dont have to go in in reverse order.
	for (unsigned int i=0; i<refmac_params.size(); i++) {
	    if (refmac_params[i].type == coot::atom_attribute_setting_help_t::IS_INT)
	      PyList_Append(r, PyInt_FromLong(refmac_params[i].i));
	    if (refmac_params[i].type == coot::atom_attribute_setting_help_t::IS_FLOAT)
	      PyList_Append(r, PyFloat_FromDouble(refmac_params[i].val));
	    if (refmac_params[i].type == coot::atom_attribute_setting_help_t::IS_STRING)
	      PyList_Append(r, PyString_FromString(refmac_params[i].s.c_str()));
	 }
      }
   }
   return r;
}
#endif	/* USE_PYTHON */

void
regularize_residues(int imol, const std::vector<coot::residue_spec_t> &residue_specs) {

   std::string alt_conf;
   if (is_valid_model_molecule(imol)) {
      if (residue_specs.size() > 0) {
	 std::vector<mmdb::Residue *> residues;
	 for (unsigned int i=0; i<residue_specs.size(); i++) {
	    coot::residue_spec_t rs = residue_specs[i];
	    mmdb::Residue *r = graphics_info_t::molecules[imol].get_residue(rs);
	    if (r) {
	       residues.push_back(r);
	    }
	 }

	 if (residues.size() > 0) {
	    graphics_info_t g;
	    // normal
	    mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
	    g.regularize_residues_vec(imol, residues, alt_conf.c_str(), mol);
	 }
      } else {
	 std::cout << "No residue specs found" << std::endl;
      }
   }
}



#ifdef USE_GUILE
SCM refine_residues_scm(int imol, SCM r) {
   return refine_residues_with_alt_conf_scm(imol, r, "");
}
#endif // USE_GUILE

#ifdef USE_GUILE
SCM regularize_residues_scm(int imol, SCM r) { /* presumes the alt_conf is "". */
   return regularize_residues_with_alt_conf_scm(imol, r, "");
}
#endif // USE_GUILE

coot::refinement_results_t
refine_residues_with_alt_conf(int imol, const std::vector<coot::residue_spec_t> &residue_specs,
			      const std::string &alt_conf) {

   coot::refinement_results_t rr;
   if (is_valid_model_molecule(imol)) {
      if (residue_specs.size() > 0) {
	 std::vector<mmdb::Residue *> residues;
	 for (unsigned int i=0; i<residue_specs.size(); i++) {
	    coot::residue_spec_t rs = residue_specs[i];
	    mmdb::Residue *r = graphics_info_t::molecules[imol].get_residue(rs);
	    if (r) {
	       residues.push_back(r);
	    }
	 }

	 if (residues.size() > 0) {
	    graphics_info_t g;
	    int imol_map = g.Imol_Refinement_Map();
	    if (! is_valid_map_molecule(imol_map)) {
	       add_status_bar_text("Refinement map not set");
	    } else {
	       // normal
	       mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
	       rr = g.refine_residues_vec(imol, residues, alt_conf.c_str(), mol);
	    }
	 } 
      } else {
	 std::cout << "No residue specs found" << std::endl;
      }
   }
   return rr;
} 





#ifdef USE_GUILE
SCM refine_residues_with_alt_conf_scm(int imol, SCM r, const char *alt_conf) { /* to be renamed later. */

   SCM rv = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
      std::vector<coot::residue_spec_t> residue_specs = scm_to_residue_specs(r);
      coot::refinement_results_t rr =
	 refine_residues_with_alt_conf(imol, residue_specs, alt_conf);
      rv = g.refinement_results_to_scm(rr);
   }
   return rv;
} 
#endif // USE_GUILE

#ifdef USE_GUILE
SCM regularize_residues_with_alt_conf_scm(int imol, SCM r, const char *alt_conf) {
   
   SCM rv = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> residue_specs = scm_to_residue_specs(r);

      if (residue_specs.size() > 0) {
	 std::vector<mmdb::Residue *> residues;
	 for (unsigned int i=0; i<residue_specs.size(); i++) {
	    coot::residue_spec_t rs = residue_specs[i];
	    mmdb::Residue *r = graphics_info_t::molecules[imol].get_residue(rs);
	    if (r) {
	       residues.push_back(r);
	    }
	 }

	 if (residues.size() > 0) {
	    graphics_info_t g;
	    mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
	    coot::refinement_results_t rr =
	       g.regularize_residues_vec(imol, residues, alt_conf, mol);
	    rv = g.refinement_results_to_scm(rr);
	 }
      }
   }
   return rv;
}
#endif // USE_GUILE

#ifdef USE_GUILE
std::vector<coot::residue_spec_t> scm_to_residue_specs(SCM r) { 
   std::vector<coot::residue_spec_t> residue_specs;
   SCM r_length_scm = scm_length(r);
   int r_length = scm_to_int(r_length_scm);
   for (int i=0; i<r_length; i++) {
      SCM res_spec_scm = scm_list_ref(r, SCM_MAKINUM(i));
      std::pair<bool, coot::residue_spec_t> res_spec =
	 make_residue_spec(res_spec_scm);
      if (res_spec.first) {
	 residue_specs.push_back(res_spec.second);
      } 
   }
   return residue_specs;
}
#endif // USE_GUILE



#ifdef USE_PYTHON
PyObject *refine_residues_py(int imol, PyObject *r) {
   return refine_residues_with_alt_conf_py(imol, r, "");
}
#endif // USE_PYTHON

#ifdef USE_PYTHON
PyObject *regularize_residues_py(int imol, PyObject *r) { /* presumes the alt_conf is "". */
   return regularize_residues_with_alt_conf_py(imol, r, "");
}
#endif // USE_PYTHON



#ifdef USE_PYTHON
PyObject *refine_residues_with_alt_conf_py(int imol, PyObject *r, const char *alt_conf) {
      
   PyObject *rv = Py_False;
   if (is_valid_model_molecule(imol)) {
     std::vector<coot::residue_spec_t> residue_specs = py_to_residue_specs(r);

      if (residue_specs.size() > 0) {
        std::vector<mmdb::Residue *> residues;
        for (unsigned int i=0; i<residue_specs.size(); i++) {
          coot::residue_spec_t rs = residue_specs[i];
          mmdb::Residue *r = graphics_info_t::molecules[imol].get_residue(rs);
          if (r) {
            residues.push_back(r);
          }
        }

        if (residues.size() > 0) {
          graphics_info_t g;
          int imol_map = g.Imol_Refinement_Map();
          if (! is_valid_map_molecule(imol_map)) { 
            add_status_bar_text("Refinement map not set");
          } else {
            // normal
            mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
            coot::refinement_results_t rr =
              g.refine_residues_vec(imol, residues, alt_conf, mol);
            rv = g.refinement_results_to_py(rr);
          }
        } 
      } else {
        std::cout << "No residue specs found" << std::endl;
      } 
   }

   if (PyBool_Check(rv)) {
     Py_INCREF(rv);
   }

   return rv;
} 
#endif // USE_PYTHON

#ifdef USE_PYTHON
PyObject *regularize_residues_with_alt_conf_py(int imol, PyObject *r, const char *alt_conf) {
   
   PyObject *rv = Py_False;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> residue_specs = py_to_residue_specs(r);

      if (residue_specs.size() > 0) {
	 std::vector<mmdb::Residue *> residues;
	 for (unsigned int i=0; i<residue_specs.size(); i++) {
	    coot::residue_spec_t rs = residue_specs[i];
	    mmdb::Residue *r = graphics_info_t::molecules[imol].get_residue(rs);
	    if (r) {
	       residues.push_back(r);
	    }
	 }

	 if (residues.size() > 0) {
	    graphics_info_t g;
        mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
        coot::refinement_results_t rr =
		  g.regularize_residues_vec(imol, residues, alt_conf, mol);
	       rv = g.refinement_results_to_py(rr);
	    }
	 } 
      } else {
	 std::cout << "No residue specs found" << std::endl;
      } 

   if (PyBool_Check(rv)) {
     Py_INCREF(rv);
   }
   return rv;
}
#endif // USE_PYTHON

#ifdef USE_PYTHON
std::vector<coot::residue_spec_t> py_to_residue_specs(PyObject *r) {
  std::vector<coot::residue_spec_t> residue_specs;
  int r_length = PyObject_Length(r);
  for (int i=0; i<r_length; i++) {
    PyObject *res_spec_py = PyList_GetItem(r, i);
    std::pair<bool, coot::residue_spec_t> res_spec =
      make_residue_spec_py(res_spec_py);
    if (res_spec.first) {
      residue_specs.push_back(res_spec.second);
    } 
  }
  return residue_specs;
}
#endif // USE_PYTHON

// imol has changed.
// Now fix up the Go_To_Atom window to match:
// 
void update_go_to_atom_window_on_changed_mol(int imol) {

   // now if the go to atom widget was being displayed, we need to
   // redraw the residue list and atom list (if the molecule of the
   // residue and atom list is the molecule that has just been
   // deleted)
   graphics_info_t g;
   g.update_go_to_atom_window_on_changed_mol(imol);
   std::string cmd = "update-go-to-atom-window-on-changed-mol";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

// a new molecule has has been read in.
// 
// Now fix up the Go_To_Atom window to match by changing the option menu
// 
void update_go_to_atom_window_on_new_mol() {

   graphics_info_t g;
   g.update_go_to_atom_window_on_new_mol();
   add_to_history_simple("update-go-to-atom-window-on-new-mol");
}


void update_go_to_atom_window_on_other_molecule_chosen(int imol) {
   graphics_info_t g;
   g.update_go_to_atom_window_on_other_molecule_chosen(imol);
   add_to_history_simple("update-go-to-atom-window-on-other-molecule-chosen");

} 

void delete_atom(int imol, const char *chain_id, int resno, const char *ins_code,
		 const char *at_name, const char *altLoc) {


   // after the atom is deleted from the molecule then the calling
   // char * pointers are out of date!  We can't use them.  So, save
   // them as strings before writing the history.
   graphics_info_t g;

   if (! chain_id) {
      std::cout << "ERROR:: in delete_atom() trapped null chain_id\n";
      return; 
   } 
   if (! ins_code) {
      std::cout << "ERROR:: in delete_atom() trapped null ins_code\n";
      return; 
   } 
   if (! at_name) {
      std::cout << "ERROR:: in delete_atom() trapped null at_name\n";
      return; 
   } 
   if (! altLoc) {
      std::cout << "ERROR:: in delete_atom() trapped null altLoc\n";
      return; 
   }

   //
   std::string chain_id_string = chain_id;
   std::string ins_code_string = ins_code;
   std::string atom_name_string = at_name;
   std::string altloc_string = altLoc;
   

   mmdb::Residue *residue_p =
      graphics_info_t::molecules[imol].get_residue(chain_id, resno, ins_code);
   if (residue_p) {
     if (residue_p->GetNumberOfAtoms() > 1) {
       coot::residue_spec_t spec(residue_p);
       g.delete_residue_from_geometry_graphs(imol, spec);
     } else {
       // only one atom left, so better delete whole residue.
       delete_residue(imol, chain_id, resno, ins_code);
       return;
     }
   }

   short int istat = g.molecules[imol].delete_atom(chain_id, resno, ins_code, at_name, altLoc);
   if (istat) { 
      // now if the go to atom widget was being displayed, we need to
      // redraw the residue list and atom list (if the molecule of the
      // residue and atom list is the molecule that has just been
      // deleted)
      //

      g.update_go_to_atom_window_on_changed_mol(imol);
      update_go_to_atom_residue_list(imol);
      graphics_draw();
   } else { 
      std::cout << "failed to delete atom  chain_id: :" << chain_id 
		<< ": " << resno << " incode :" << ins_code
		<< ": atom-name :" <<  at_name << ": altloc :" <<  altLoc << ":" << "\n";
   }

   std::string cmd = "delete-atom";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id_string));
   args.push_back(resno);
   args.push_back(coot::util::single_quote(ins_code_string));
   args.push_back(coot::util::single_quote(atom_name_string));
   args.push_back(coot::util::single_quote(altloc_string));
   add_to_history_typed(cmd, args);

} 

void set_delete_atom_mode() {
   graphics_info_t g;
   g.delete_item_atom = 1;
   g.delete_item_residue_zone = 0;
   g.delete_item_residue_hydrogens = 0;
   g.delete_item_residue = 0;
   g.delete_item_sidechain = 0;
   add_to_history_simple("set-delete-atom-mode");
}

void set_delete_residue_mode() {
   graphics_info_t g;
   g.delete_item_atom = 0;
   g.delete_item_residue_zone = 0;
   g.delete_item_residue_hydrogens = 0;
   g.delete_item_water = 0;
   g.delete_item_residue = 1;
   g.delete_item_sidechain = 0; 
   add_to_history_simple("set-delete-residue-mode");
}

void set_delete_residue_hydrogens_mode() {

   graphics_info_t g;
   g.delete_item_residue = 0;
   g.delete_item_residue_zone = 0;
   g.delete_item_atom = 0;
   g.delete_item_water = 0;
   g.delete_item_residue_hydrogens = 1;
   g.delete_item_sidechain = 0; 
   add_to_history_simple("set-delete-residue-hydrogens-mode");

}

void set_delete_residue_zone_mode() {

   graphics_info_t g;
   g.delete_item_residue = 0;
   g.delete_item_residue_zone = 1;
   g.delete_item_atom = 0;
   g.delete_item_water = 0;
   g.delete_item_residue_hydrogens = 0;
   g.delete_item_sidechain = 0; 
   add_to_history_simple("set-delete-residue-zone-mode");
} 

void set_delete_water_mode() {

   graphics_info_t g;
   g.delete_item_residue = 0;
   g.delete_item_residue_zone = 0;
   g.delete_item_water = 1;
   g.delete_item_atom = 0;
   g.delete_item_residue_hydrogens = 0;
   g.delete_item_sidechain = 0; 
   add_to_history_simple("set-delete-residue-water-mode");

} 

void set_delete_sidechain_mode() {

   graphics_info_t g;
   g.delete_item_residue = 0;
   g.delete_item_residue_zone = 0;
   g.delete_item_water = 0;
   g.delete_item_atom = 0;
   g.delete_item_residue_hydrogens = 0;
   g.delete_item_sidechain = 1; 
   add_to_history_simple("set-delete-sidechain-mode");

}

// (predicate) a boolean (or else it's residue)
//
// Used by on_model_refine_dialog_delete_button_clicked callback to
// determine if the Atom checkbutton should be active when the new
// dialog is displayed.
// 
short int delete_item_mode_is_atom_p() {
   short int v=0;
   if (graphics_info_t::delete_item_atom == 1)
      v = 1;
   if (graphics_info_t::delete_item_residue == 1)
      v = 0;
   if (graphics_info_t::delete_item_water == 1)
      v = 0;
   return v;
}

short int delete_item_mode_is_residue_p() {
   short int v=0;
   if (graphics_info_t::delete_item_residue == 1)
      v = 1;
   return v;
}

short int delete_item_mode_is_water_p() {
   short int v=0;
   if (graphics_info_t::delete_item_water == 1)
      v = 1;
   return v;
}

short int delete_item_mode_is_sidechain_p() {
   short int v=0;
   if (graphics_info_t::delete_item_sidechain == 1)
      v = 1;
   return v;
}

// 20090911 Dangerous?  This used to be called by
// check_if_in_delete_item_define water mode.  But it caused a crash.
// It seemed to be accessing bad memory for the atom name.  That was
// because we were doing a delete_residue_hydrogens() between getting
// the atom index from the pick and calling this function.  Therefore
// the atom index/selection was out of date when we got here.  That
// has been fixed now, but it is a concern for other function that
// might want to do something similar.
//
// Delete this function when doing a cleanup.
//
void delete_atom_by_atom_index(int imol, int index, short int do_delete_dialog) {
   graphics_info_t g;

   if (index < g.molecules[imol].atom_sel.n_selected_atoms) { 
      const char *atom_name = g.molecules[imol].atom_sel.atom_selection[index]->name;
      const char *chain_id  = g.molecules[imol].atom_sel.atom_selection[index]->GetChainID();
      const char *altconf   = g.molecules[imol].atom_sel.atom_selection[index]->altLoc;
      const char *ins_code  = g.molecules[imol].atom_sel.atom_selection[index]->GetInsCode();
      int resno             = g.molecules[imol].atom_sel.atom_selection[index]->GetSeqNum();

      mmdb::Residue *residue_p =
	 graphics_info_t::molecules[imol].get_residue(chain_id, resno, ins_code);
      if (residue_p) {
	 coot::residue_spec_t spec(residue_p);
	 g.delete_residue_from_geometry_graphs(imol, spec);
      }

      std::cout << "calling delete_atom() with args chain_id :"
		<< chain_id << ": resno " << resno << " inscode :"
		<< ins_code << ": atom-name " << atom_name << ": altconf :"
		<< altconf << ":" << std::endl;
      delete_atom(imol, chain_id, resno, ins_code, atom_name, altconf);
      delete_object_handle_delete_dialog(do_delete_dialog);
   }

   // no need for this, the called delete_atom() does it.
//    std::string cmd = "delete-atom-by-atom-index";
//    std::vector<coot::command_arg_t> args;
//    args.push_back(imol);
//    args.push_back(index);
//    args.push_back(do_delete_dialog);
//    add_to_history_typed(cmd, args);

}

void delete_residue_by_atom_index(int imol, int index, short int do_delete_dialog_by_ctrl) {

   graphics_info_t g;
   int imodel            = g.molecules[imol].atom_sel.atom_selection[index]->GetModelNum();
   std::string chain_id  = g.molecules[imol].atom_sel.atom_selection[index]->GetChainID();
   int resno             = g.molecules[imol].atom_sel.atom_selection[index]->GetSeqNum();
   std::string altloc    = g.molecules[imol].atom_sel.atom_selection[index]->altLoc;
   std::string inscode   = g.molecules[imol].atom_sel.atom_selection[index]->GetInsCode();

   // I don't think that there is any need to call get_residue() here,
   // we can simply construct spec from chain_id, resno and inscode.
   // There are other places where we do this too (to delete a residue
   // from the geometry graphs).
   mmdb::Residue *residue_p =
      graphics_info_t::molecules[imol].get_residue(chain_id, resno, inscode);
   if (residue_p) {
      graphics_info_t g;
      coot::residue_spec_t spec(residue_p);
      g.delete_residue_from_geometry_graphs(imol, spec);
   }

   if ((altloc == "") && (imodel == 1))
      delete_residue(imol, chain_id.c_str(), resno, inscode.c_str());
   else
      delete_residue_with_full_spec(imol, imodel, chain_id.c_str(), resno,
				    inscode.c_str(), altloc.c_str());

   short int do_delete_dialog = do_delete_dialog_by_ctrl;
   delete_object_handle_delete_dialog(do_delete_dialog);

   graphics_draw();
   std::string cmd = "delete-residue-by-atom-index";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(index);
   args.push_back(do_delete_dialog_by_ctrl);
   add_to_history_typed(cmd, args);
}

void delete_residue_hydrogens_by_atom_index(int imol, int index, short int do_delete_dialog) {

   graphics_info_t g;
   std::string chain_id  = g.molecules[imol].atom_sel.atom_selection[index]->GetChainID();
   int resno             = g.molecules[imol].atom_sel.atom_selection[index]->GetSeqNum();
   std::string altloc    = g.molecules[imol].atom_sel.atom_selection[index]->altLoc;
   std::string inscode   = g.molecules[imol].atom_sel.atom_selection[index]->GetInsCode();


   delete_residue_hydrogens(imol, chain_id.c_str(), resno, inscode.c_str(), altloc.c_str());

   delete_object_handle_delete_dialog(do_delete_dialog);
   graphics_draw();
   std::string cmd = "delete-residue-hydrogens-by-atom-index";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(index);
   args.push_back(do_delete_dialog);
   add_to_history_typed(cmd, args);
}

// Deletes all altconfs, the whole residue goes.
// 
void delete_residue_range(int imol, const char *chain_id, int resno_start, int resno_end) {

   // altconf is ignored currently.
   // 
   coot::residue_spec_t res1(chain_id, resno_start);
   coot::residue_spec_t res2(chain_id, resno_end);

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      g.delete_residue_range(imol, res1, res2);
      if (graphics_info_t::go_to_atom_window) {
	 update_go_to_atom_window_on_changed_mol(imol);
      }
   }
   graphics_draw();
   std::string cmd = "delete-residue-range";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno_start);
   args.push_back(resno_end);
   add_to_history_typed(cmd, args);
}




void clear_pending_delete_item() { 

   graphics_info_t g;
   g.delete_item_atom = 0;
   g.delete_item_residue = 0;
   g.delete_item_residue_zone = 0;
   g.delete_item_residue_hydrogens = 0;
   add_to_history_simple("clear-pending-delete-item");
}




int move_molecule_to_screen_centre_internal(int imol) {

   int imoved_stat = 0;
   // std::cout << "move_molecule_here imol: " << imol << std::endl;
   if (is_valid_model_molecule(imol)) {
      
      // (move-molecule-here imol)
      coot::Cartesian cen =
	 centre_of_molecule(graphics_info_t::molecules[imol].atom_sel);

      graphics_info_t g;
      coot::Cartesian rc = g.RotationCentre();

      float x = rc.x() - cen.x();
      float y = rc.y() - cen.y();
      float z = rc.z() - cen.z();

      translate_molecule_by(imol, x, y, z);
      imoved_stat = 1;
      // finally show and activate
      set_mol_displayed(imol, 1);
      set_mol_active(imol, 1);

      if (0) { 
	 std::cout << "-------------------- move_molecule_to_screen_centre_internal() "
		   << imol << std::endl;
	 std::cout << "           calling g.setup_graphics_ligand_view_aa() "
		   << std::endl;
      }
      g.setup_graphics_ligand_view_aa(imol);
   }
   return imoved_stat;
}


// ---------------------------------------------------------------------
//                 rotamer
// ---------------------------------------------------------------------
// 

void set_write_peaksearched_waters() {
   graphics_info_t g;
   g.ligand_water_write_peaksearched_atoms = 1;
   add_to_history_simple("set-write-peaksearched-waters");
} 


// Called by the Model/Fit/Refine Rotamers button callback.
void setup_rotamers(short int state) {
   graphics_info_t g;
   g.in_rotamer_define = state;
   if (state) { 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;
      std::cout << "Click on an atom in a residue for which you wish to see rotamers"
		<< std::endl;
   } else {
      g.normal_cursor();
   }
   std::string cmd = "setup-rotamers";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
} 


void do_rotamers(int atom_index, int imol) {

//    std::cout << "     Rotamer library:" << std::endl;
//    std::cout << "     R. L. Dunbrack, Jr. and F. E. Cohen." << std::endl;
//    std::cout << "     Bayesian statistical analysis of ";
//    std::cout << "protein sidechain rotamer preferences" << std::endl;
//    std::cout << "     Protein Science, 6, 1661-1681 (1997)." << std::endl;
//    std::cout << "" << std::endl;

   graphics_info_t g;
   g.do_rotamers(atom_index, imol); 
   std::string cmd = "do-rotamers";
   std::vector<coot::command_arg_t> args;
   args.push_back(atom_index);
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

// same as do_rotamers, except, a better name and we give residue
// specs, so that we can use the active residue.
void show_rotamers_dialog(int imol, const char *chain_id, int resno, const char *ins_code, const char *altconf) {

   int atom_index = -1;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      atom_index = g.molecules[imol].atom_index_first_atom_in_residue(chain_id, resno, ins_code, altconf);
      if (atom_index != -1) {
	 g.do_rotamers(atom_index, imol); 
      } else {
	 std::cout << "No atom index found in molecule " << imol << std::endl;
      }
   }
} 


void
set_rotamer_lowest_probability(float f) {
#ifdef USE_DUNBRACK_ROTAMERS
   graphics_info_t g;
   g.rotamer_lowest_probability = f;
   std::string cmd = "set-rotamer-lowest-probability";
   std::vector<coot::command_arg_t> args;
   args.push_back(f);
   add_to_history_typed(cmd, args);
#endif    
}

void
set_rotamer_check_clashes(int i) {
   graphics_info_t::rotamer_fit_clash_flag = i;
   std::string cmd = "set-rotamer-check-clashes";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
}

// Return -999 on imol indexing error
// -99.9 in class function error
// 
float
auto_fit_best_rotamer(int resno,
		      const char *altloc,
		      const char *insertion_code, 
		      const char *chain_id, int imol_coords, int imol_map,
		      int clash_flag, float lowest_probability) {

   float f = -999.9;

   if (is_valid_model_molecule(imol_coords)) {

      std::string ins(insertion_code);
      std::string chain(chain_id);
      int mode = graphics_info_t::rotamer_search_mode;
      if (! is_valid_map_molecule(imol_map)) {
	 std::cout << "INFO:: fitting rotamers by clash score only " << std::endl;
	 graphics_info_t g;
	 f = graphics_info_t::molecules[imol_coords].auto_fit_best_rotamer(mode,
									   resno, altloc, ins,
									   chain, imol_map,
									   1,
									   lowest_probability,
									   *g.Geom_p());
      } else {
	 graphics_info_t g;
	 f = g.molecules[imol_coords].auto_fit_best_rotamer(mode,
							    resno, altloc, ins,
							    chain, imol_map,
							    clash_flag,
							    lowest_probability,
							    *g.Geom_p());

	 // first do real space refine if requested
	 if (g.rotamer_auto_fit_do_post_refine_flag) {
	    // Run refine zone with autoaccept, autorange on
	    // the "clicked" atom:
	    // BL says:: dont think we do autoaccept!?
	    short int auto_range = 1;
	    refine_auto_range(imol_coords, chain_id, resno, altloc);
	 }

	 // get the residue so that it can update the geometry graph
	 mmdb::Residue *residue_p = g.molecules[imol_coords].get_residue(chain, resno, ins);
	 if (residue_p) {
	    g.update_geometry_graphs(&residue_p, 1, imol_coords, imol_map);
	 }
	 std::cout << "Fitting score for best rotamer: " << f << std::endl;
      }
      graphics_draw();
   }
   std::string cmd = "auto-fit-best-rotamer";
   std::vector<coot::command_arg_t> args;
   args.push_back(resno);
   args.push_back(coot::util::single_quote(altloc));
   args.push_back(coot::util::single_quote(insertion_code));
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(imol_coords);
   args.push_back(imol_map);
   args.push_back(clash_flag);
   args.push_back(lowest_probability);
   add_to_history_typed(cmd, args);
   return f;
}


void
set_auto_fit_best_rotamer_clash_flag(int i) { /* 0 off, 1 on */
   graphics_info_t::rotamer_fit_clash_flag = i;
   std::string cmd = "set-auto-fit-best-rotamer-clash-flag";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
} 

void
setup_auto_fit_rotamer(short int state) {
   graphics_info_t::in_auto_fit_define = state;
   if (state) { 
      graphics_info_t::pick_cursor_maybe();
      graphics_info_t::pick_pending_flag = 1;
      std::cout << "Click on an atom in the residue that you wish to fit\n";
   } else {
      graphics_info_t::normal_cursor();
   }
   std::string cmd = "setup-auto-fit-rotamer";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}


// FIXME  (autofit rotamer seems to return a score OK).
float
rotamer_score(int imol, const char *chain_id, int res_no, const char *insertion_code,
	      const char *alt_conf) {

   float f = 0;

   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      mmdb::Residue *residue_p =
	 graphics_info_t::molecules[imol].get_residue(chain_id, res_no, insertion_code);
      if (residue_p) {
	 float lp = graphics_info_t::rotamer_lowest_probability;
	 graphics_info_t g;
	 coot::rotamer_probability_info_t d_score = 
	    g.get_rotamer_probability(residue_p, alt_conf, mol, lp, 1);
	 if (d_score.state == coot::rotamer_probability_info_t::OK) 
	    f = d_score.probability;
      }
   }
   
   
   std::string cmd = "rotamer-score";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(res_no);
   args.push_back(coot::util::single_quote(insertion_code));
   add_to_history_typed(cmd, args);
   return f;
}


/*! \brief return the number of rotamers for this residue */
int n_rotamers(int imol, const char *chain_id, int resno, const char *ins_code) {

   int r = -1; 
   if (is_valid_model_molecule(imol)) { 
      mmdb::Residue *res = graphics_info_t::molecules[imol].get_residue(chain_id, resno, ins_code);
      if (res) {
	 graphics_info_t g;
#ifdef USE_DUNBRACK_ROTAMERS
	 coot::dunbrack d(res, g.molecules[imol].atom_sel.mol, g.rotamer_lowest_probability, 0);
#else			
	 std::string alt_conf = "";
	 coot::richardson_rotamer d(res, alt_conf, g.molecules[imol].atom_sel.mol,
				    g.rotamer_lowest_probability, 0);
#endif // USE_DUNBRACK_ROTAMERS
	 
	 std::vector<float> probabilities = d.probabilities();
	 r = probabilities.size();
      }
   }
   return r;
} 

/*! \brief set the residue specified to the rotamer number specifed. */
int set_residue_to_rotamer_number(int imol, const char *chain_id, int resno, const char *ins_code,
				  const char *alt_conf, int rotamer_number) {

   int i_done = 0;
   if (is_valid_model_molecule(imol)) {
      int n = rotamer_number;
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);
      graphics_info_t g;
      i_done = g.molecules[imol].set_residue_to_rotamer_number(res_spec, alt_conf, n, *g.Geom_p());
      graphics_draw();
   }
   return i_done; 
}

int set_residue_to_rotamer_name(int imol, const char *chain_id, int resno, const char *ins_code,
				const char *alt_conf,
				const char *rotamer_name) {

   int status = 0;
   if (is_valid_model_molecule(imol)) {
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);
      graphics_info_t g;
      status = g.molecules[imol].set_residue_to_rotamer_name(res_spec, alt_conf, rotamer_name, *g.Geom_p());
      graphics_draw();
   } 
   return status;
} 


#ifdef USE_GUILE
SCM get_rotamer_name_scm(int imol, const char *chain_id, int resno, const char *ins_code) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);
      mmdb::Residue *res = graphics_info_t::molecules[imol].get_residue(res_spec);
      if (res) {
#ifdef USE_DUNBRACK_ROTAMERS
#else
	 // we are not passed an alt conf.  We should be, shouldn't we?
	 std::string alt_conf = "";
	 mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
	 coot::richardson_rotamer d(res, alt_conf, mol, 0.0, 1);
	 coot::rotamer_probability_info_t prob = d.probability_of_this_rotamer();
	 std::cout << "INFO:: " << coot::residue_spec_t(res) << " " << prob << std::endl;
	 r = scm_makfrom0str(prob.rotamer_name.c_str());
#endif      
      }
   }
   return r;
} 
#endif 

#ifdef USE_PYTHON
PyObject *get_rotamer_name_py(int imol, const char *chain_id, int resno, const char *ins_code) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);
      mmdb::Residue *res = graphics_info_t::molecules[imol].get_residue(res_spec);
      if (res) {
#ifdef USE_DUNBRACK_ROTAMERS
#else
	 // we are not passed an alt conf.  We should be, shouldn't we?
	 std::string alt_conf = "";
	 coot::richardson_rotamer d(res, alt_conf, graphics_info_t::molecules[imol].atom_sel.mol,
				    0.0, 1);
	 coot::rotamer_probability_info_t prob = d.probability_of_this_rotamer();
	 r = PyString_FromString(prob.rotamer_name.c_str());
#endif      
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif 




// ---------------------------------------------------------------------
//                 mutation 
// ---------------------------------------------------------------------
// 
void
setup_mutate(short int state) {

   graphics_info_t g;
   g.in_mutate_define = state;
   if (state) { 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;
      std::cout << "Click on an atom in a residue which you wish to mutate"
		<< std::endl;
   } else {
      g.normal_cursor();
   }
   std::string cmd = "setup-mutate";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void
setup_mutate_auto_fit(short int state) { 

   graphics_info_t g;

   if (state) { 
      int imol_map = g.Imol_Refinement_Map(); 
      if (imol_map >= 0) { 
	 std::cout << "Click on an atom in a residue which you wish to mutate"
		   << std::endl;
	 g.in_mutate_auto_fit_define = state;
	 g.pick_cursor_maybe();
	 g.pick_pending_flag = 1;
      } else { 
	 // map chooser dialog
	 g.show_select_map_dialog();
	 g.in_mutate_auto_fit_define = 0;
	 normal_cursor();
	 g.model_fit_refine_unactive_togglebutton("model_refine_dialog_mutate_auto_fit_togglebutton");
      }
   } else {
      g.in_mutate_auto_fit_define = state;
      g.normal_cursor();
   }
   std::string cmd = "setup-mutate-auto-fit";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}


/* 1 for yes, 0 for no. */
void set_mutate_auto_fit_do_post_refine(short int istate) {

   graphics_info_t::mutate_auto_fit_do_post_refine_flag = istate;
   std::string cmd = "set-mutate-auto-fit-do-post-refine";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
} 

/*! \brief what is the value of the previous flag? */
int mutate_auto_fit_do_post_refine_state() {
   add_to_history_simple("mutate-auto-fit-do-post-refine-state");
   return graphics_info_t::mutate_auto_fit_do_post_refine_flag;
} 

/* 1 for yes, 0 for no. */
void set_rotamer_auto_fit_do_post_refine(short int istate) {

   graphics_info_t::rotamer_auto_fit_do_post_refine_flag = istate;
   std::string cmd = "set-rotamer-auto-fit-do-post-refine";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
} 

/*! \brief what is the value of the previous flag? */
int rotamer_auto_fit_do_post_refine_state() {
   add_to_history_simple("rotamer-auto-fit-do-post-refine-state");
   return graphics_info_t::rotamer_auto_fit_do_post_refine_flag;
} 

/*! \brief set a flag saying that the chosen residue should only be
  added as a stub (mainchain + CB) */
void set_residue_type_chooser_stub_state(short int istat) {
   graphics_info_t::residue_type_chooser_stub_flag = istat;
   std::string cmd = "set-residue-type-chooser-stub-state";
   std::vector<coot::command_arg_t> args;
   args.push_back(istat);
   add_to_history_typed(cmd, args);
}


void
do_mutation(const char *type, short int stub_button_state_flag) {
   graphics_info_t g;
   // use g.mutate_residue_atom_index and g.mutate_residue_imol
   g.do_mutation(type, stub_button_state_flag);
   std::string cmd = "do-mutatation";
   std::vector<coot::command_arg_t> args;
   args.push_back(coot::util::single_quote(type));
   args.push_back(stub_button_state_flag);
   add_to_history_typed(cmd, args);
}

void
place_atom_at_pointer() {
   graphics_info_t g;
   if (g.pointer_atom_is_dummy)
      g.place_dummy_atom_at_pointer();
   else {
      place_atom_at_pointer_by_window();
   }
   add_to_history_simple("place-atom-at-pointer");
}


int pointer_atom_molecule() {
   return graphics_info_t::user_pointer_atom_molecule;
}

int create_pointer_atom_molecule_maybe() {
   graphics_info_t g;
   return g.create_pointer_atom_molecule_maybe();
}

void
set_pointer_atom_molecule(int imol) {
   if (is_valid_model_molecule(imol)) {
      graphics_info_t::user_pointer_atom_molecule = imol;
   }
}

void
place_typed_atom_at_pointer(const char *type) {
   graphics_info_t g;
   g.place_typed_atom_at_pointer(std::string(type));
   std::string cmd = "place-typed-atom-at-pointer";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(type));
   add_to_history_typed(cmd, args);
}

void set_pointer_atom_is_dummy(int i) { 
   graphics_info_t::pointer_atom_is_dummy = i;
   std::string cmd = "set-pointer-atom-is-dummy";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
} 

      

void display_where_is_pointer() {
   graphics_info_t g;
   g.display_where_is_pointer();
   add_to_history_simple("display-where-is-pointer");
}

// draw the baton?
int try_set_draw_baton(short int i) {

   graphics_info_t g;
   g.try_set_draw_baton(i);
   std::string cmd = "set-draw-baton";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
   return g.draw_baton_flag;
}

// Mouse movement moves the baton not the view?
void set_baton_mode(short int i) {
   graphics_info_t::baton_mode = i;
   std::string cmd = "set-baton-mode";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
}

void accept_baton_position() { 	/* put an atom at the tip and move baton */

   graphics_info_t g;
   g.accept_baton_position();
   add_to_history_simple("accept-baton-position");
}

void baton_tip_try_another() {
   graphics_info_t g;
   g.baton_tip_try_another();
   add_to_history_simple("baton-try-another");
}

void baton_tip_previous() {
   graphics_info_t g;
   g.baton_tip_previous();
   add_to_history_simple("baton-try-another");
}

void shorten_baton() {
   graphics_info_t g;
   g.shorten_baton();
   add_to_history_simple("shorten-baton");
}

void lengthen_baton() {
   graphics_info_t g;
   g.lengthen_baton();
   add_to_history_simple("lengthen-baton");
}

void baton_build_delete_last_residue() {

   graphics_info_t g;
   g.baton_build_delete_last_residue();
   add_to_history_simple("baton-build-delete-last-residue");
} 

void set_baton_build_params(int istart_resno, 
			   const char *chain_id, 
			   const char *backwards) { 

   graphics_info_t g;
   g.set_baton_build_params(istart_resno, chain_id, backwards); 
   std::string cmd = "set-baton-build-params";
   std::vector<coot::command_arg_t> args;
   args.push_back(istart_resno);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(coot::util::single_quote(backwards));
   add_to_history_typed(cmd, args);
}



/* Reverse the direction of a the fragment of the clicked on
   atom/residue.  A fragment is a consequitive range of residues -
   where there is a gap in the numbering, that marks breaks between
   fragments in a chain.  There also needs to be a distance break - if
   the CA of the next/previous residue is more than 5A away, that also
   marks a break. Thow away all atoms in fragment other than CAs */
void reverse_direction_of_fragment(int imol, const char *chain_id, int resno) {

   if (is_valid_model_molecule(imol)) {
      // return 1 if we did it.
      int istatus = graphics_info_t::molecules[imol].reverse_direction_of_fragment(std::string(chain_id), resno);
      if (istatus)
	 graphics_draw();
   }
   std::string cmd = "reverse-direction-of-fragment";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno);
   add_to_history_typed(cmd, args);
}



// -----------------------------------------------------------------------------
//                               Automutation stuff 
// -----------------------------------------------------------------------------
// 
short int progressive_residues_in_chain_check(const char *chain_id, int imol) {
   
   std::string cmd = "progressive-residues-in-chain-check";
   std::vector<coot::command_arg_t> args;
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   
   graphics_info_t g;
   if (imol < graphics_n_molecules()) {
      return g.molecules[imol].progressive_residues_in_chain_check_by_chain(chain_id);
   } else {
      std::cout << "no such molecule number in progressive_residues_in_chain_check\n";
      return 0;
   }
} 

// return success on residue type match
// success: 1, failure: 0.
int
mutate_internal(int ires_serial, const char *chain_id, int imol, std::string &target_res_type) {

   graphics_info_t g;
   int istate = 0;
   if (imol < graphics_n_molecules()) {
      istate = g.molecules[imol].mutate_single_multipart(ires_serial, chain_id, target_res_type);
      if (istate == 0) {
	 std::cout << "ERROR: got bad state in mutate_internal" << std::endl;
      }
      graphics_draw();
   }
   return istate;
}

// causes a make_backup()
int
mutate(int imol, const char *chain_id, int ires, const char *inscode,  const char *target_res_type) { 

   int istate = 0;
   std::string target_type(target_res_type);

   if (is_valid_model_molecule(imol)) { 
      istate = graphics_info_t::molecules[imol].mutate(ires, inscode, std::string(chain_id), std::string(target_res_type));
      graphics_draw();
   }
   std::string cmd = "mutate";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(ires);
   args.push_back(coot::util::single_quote(inscode));
   args.push_back(coot::util::single_quote(target_res_type));
   add_to_history_typed(cmd, args);
   
   return istate;
}

// return success status.  
int mutate_base(int imol, const char *chain_id, int res_no, const char *ins_code, const char *res_type) {
   int istate = 0;
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) {
      coot::residue_spec_t r(chain_id, res_no, ins_code);
      istate = graphics_info_t::molecules[imol].mutate_base(r, res_type,
							    g.convert_to_v2_atom_names_flag);
      graphics_draw();
   } 
   std::string cmd = "mutate-base";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(res_no);
   args.push_back(coot::util::single_quote(ins_code));
   args.push_back(coot::util::single_quote(res_type));
   add_to_history_typed(cmd, args);
   
   return istate; 
} 



// Return success on residue type match
// success: 1, failure: 0.
//
// Does not cause a make_backup().
//
int
mutate_single_residue_by_serial_number(int ires_serial, const char *chain_id, int imol,
			   char target_res_type) {

   std::string target_as_str = coot::util::single_letter_to_3_letter_code(target_res_type);
   std::cout << "INFO:: mutate target_res_type :" << target_as_str << ":" << std::endl;
      
   return mutate_internal(ires_serial, chain_id, imol, target_as_str);

}

// Previously, I was using mutate_single_residue_by_seqno to be a
// wrapper for mutate_single_residue_by_serial_number.
//
// But that fails when the residue is perfectly reasonably except that
// the serial number is -1 (I don't know wny this happens but it does
// in terminal residue addition).  So I will need new functionally
// that does the residue at root by seqnum not serial_number.
// 
int mutate_single_residue_by_seqno(int ires, const char *inscode,
				   const char *chain_id, 
				   int imol, char target_res_type) { 

   int status = -1; 
   std::string target_as_str = coot::util::single_letter_to_3_letter_code(target_res_type);

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      status = g.molecules[imol].mutate(ires, std::string(inscode),
					std::string(chain_id), target_as_str);
   }
   return status;
}

/* push the residues along a bit

e.g. if nudge_by is 1, then the sidechain of residue 20 is moved up
onto what is currently residue 21.  The mainchain numbering and atoms is not changed. */
int nudge_residue_sequence(int imol, char *chain_id, int res_no_range_start,
			   int res_no_range_end,
			   int nudge_by,
			   short int nudge_residue_numbers_also) {

   int status = 0;
   if (is_valid_model_molecule(imol)) {
      status = graphics_info_t::molecules[imol].nudge_residue_sequence(chain_id,
								       res_no_range_start,
								       res_no_range_end,
								       nudge_by,
								       nudge_residue_numbers_also);
   }
   return status;
} 



// return -1 on error:
// 
int chain_n_residues(const char *chain_id, int imol) {

   graphics_info_t g;
   if (is_valid_model_molecule(imol)) {
      return g.molecules[imol].chain_n_residues(chain_id);
   } else { 
      return -1;
   }
   std::string cmd = "chain-n-residues";
   std::vector<coot::command_arg_t> args;
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   
}

// Return NULL (#f) on failure.
// 
char *resname_from_serial_number(int imol, const char *chain_id, int serial_num) {

   char *r = NULL;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 mmdb::Chain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    int nres = chain_p->GetNumberOfResidues();
	    if (serial_num < nres) {
	       int ch_n_res;
	       mmdb::PResidue *residues;
	       chain_p->GetResidueTable(residues, ch_n_res);
	       mmdb::Residue *this_res = residues[serial_num];
	       r = this_res->GetResName();
	    }
	 }
      }
   }
   std::string cmd = "resname-from-serial-number";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(serial_num);
   add_to_history_typed(cmd, args);
   return r;
}

// Return < -9999 on failure
int  seqnum_from_serial_number(int imol, const char *chain_id, int serial_num) {

   int UNSET_SERIAL_NUMBER = -10000;
   int iseqnum = UNSET_SERIAL_NUMBER;
   
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 mmdb::Chain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    int nres = chain_p->GetNumberOfResidues();
	    if (serial_num < nres) {
	       int ch_n_res;
	       mmdb::PResidue *residues;
	       chain_p->GetResidueTable(residues, ch_n_res);
	       mmdb::Residue *this_res = residues[serial_num];
	       iseqnum = this_res->GetSeqNum();
	    } else {
	       std::cout << "WARNING:: seqnum_from_serial_number: requested residue with serial_num "
			 << serial_num << " but only " << nres << " residues in chain "
			 << mol_chain_id << std::endl;
	    }
	 }
      }
      if (iseqnum == UNSET_SERIAL_NUMBER) {
	 std::cout << "WARNING: seqnum_from_serial_number: returning UNSET serial number "
		   << std::endl;
      } 
   } else {
      std::cout << "WARNING molecule number " << imol << " is not a valid model molecule "
		<< std::endl;
   } 
   std::string cmd = "setnum-from-serial-number";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(serial_num);
   add_to_history_typed(cmd, args);
   return iseqnum;
}

char *insertion_code_from_serial_number(int imol, const char *chain_id, int serial_num) {

   char *r = NULL;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 mmdb::Chain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    int nres = chain_p->GetNumberOfResidues();
	    if (serial_num < nres) {
	       int ch_n_res;
	       mmdb::PResidue *residues;
	       chain_p->GetResidueTable(residues, ch_n_res);
	       mmdb::Residue *this_res = residues[serial_num];
	       r = this_res->GetInsCode();
	    }
	 }
      }
   }
   std::string cmd = "insertion-code-from-serial-number";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(serial_num);
   add_to_history_typed(cmd, args);
   return r;
}

#ifdef USE_GUILE
SCM
chain_id_scm(int imol, int ichain) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      mmdb::Chain *chain_p = mol->GetChain(1,ichain);
      if (chain_p) 
	 r = scm_from_locale_string(chain_p->GetChainID());
   }
   std::string cmd = "chain_id";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(ichain);
   add_to_history_typed(cmd, args);
   return r;
}
#endif // USE_GUILE


#ifdef USE_PYTHON
PyObject *
chain_id_py(int imol, int ichain) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      mmdb::Chain *chain_p = mol->GetChain(1,ichain);
      if (chain_p) 
	 r = PyString_FromString(chain_p->GetChainID());
   }
   std::string cmd = "chain_id";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(ichain);
   add_to_history_typed(cmd, args);

   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // USE_GUILE

int n_chains(int imol) {

   int nchains = -1;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      nchains = mol->GetNumberOfChains(1);
   }
   std::string cmd = "n-chains";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return nchains;
}


/*! \brief return the number of models in molecule number imol 

useful for NMR or other such multi-model molecules.

return the number of models or -1 if there was a problem with the
given molecule.
*/
int n_models(int imol) {

   int r = -1; // fail;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].n_models();
   }
   std::string cmd = "n-models";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return r;
}

/*!\brief return the number of residues in the molecule, 

return -1 if this is a map or closed.
 */
int n_residues(int imol) {

   int r = -1; 
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      r = g.molecules[imol].n_residues();
   } 
   return r;
} 



// return -1 on error (e.g. chain_id not found, or molecule is not a
// model), 0 for no, 1 for is.
int is_solvent_chain_p(int imol, const char *chain_id) {

   int r = -1;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 mmdb::Chain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    r = chain_p->isSolventChain();
	 }
      }
   }
   std::string cmd = "is-solvent-chain-p";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   add_to_history_typed(cmd, args);
   return r;
}

// return -1 on error (e.g. chain_id not found, or molecule is not a
// model), 0 for no, 1 for is.
int is_protein_chain_p(int imol, const char *chain_id) {

   int r = -1;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 mmdb::Chain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    r = chain_p->isAminoacidChain();
	 }
      }
   }
   std::string cmd = "is-protein-chain-p";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   add_to_history_typed(cmd, args);
   return r;
}

// return -1 on error (e.g. chain_id not found, or molecule is not a
// model), 0 for no, 1 for is.
int is_nucleotide_chain_p(int imol, const char *chain_id) {

   int r = -1;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 mmdb::Chain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    r = chain_p->isNucleotideChain();
	 }
      }
   }
   std::string cmd = "is-nucleotide-chain-p";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   add_to_history_typed(cmd, args);
   return r;
}

// 
// /*! \brief sort the chain ids of the imol-th molecule in lexographical order */
void sort_chains(int imol) { 

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].sort_chains();
      if (graphics_info_t::use_graphics_interface_flag) {
	 graphics_info_t g;
	 if (g.go_to_atom_window) {
	    g.update_go_to_atom_window_on_changed_mol(imol);
	 }
      }
   }
}

// /*! \brief sort the residues of the imol-th molecule */
void sort_residues(int imol) { 

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].sort_residues();
      if (graphics_info_t::use_graphics_interface_flag) {
        graphics_info_t g;
        if (g.go_to_atom_window) {
          g.update_go_to_atom_window_on_changed_mol(imol);
        }
      }
   }
}


/*! \brief simply print secondardy structure info to the
  terminal/console.  In future, this could/should return the info.  */
void print_header_secondary_structure_info(int imol) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].print_secondary_structure_info();
   } 
}

// Placeholder only.  Not documented, it doesn't work yet, because
// CalcSecStructure() creates SS type on the residues, it does not
// build and store CHelix, CStrand, CSheet records.
// 
void write_header_secondary_structure_info(int imol, const char *file_name) {

   if (is_valid_model_molecule(imol)) { 
      mmdb::io::File f;
      bool Text = true;
      f.assign(file_name, Text);
      if (f.rewrite()) { 
	 mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
	 int n_models = mol->GetNumberOfModels();
	 int imod = 1;
	 mmdb::Model *model_p = mol->GetModel(imod);
	 int ss_status = model_p->CalcSecStructure(1);
	 
	 // now do something clever with the residues of model_p to
	 // build mmdb sheet and strand objects and attach them to the
	 // model (should be part of mmdb).
	 
	 if (ss_status == mmdb::SSERC_Ok) {
	    std::cout << "INFO:: SSE status was OK\n";
	    model_p->PDBASCIIDumpPS(f); // dump CHelix and CStrand records.
	 }
      }
   } 
} 




/*  ----------------------------------------------------------------------- */
/*                         Renumber residue range                           */
/*  ----------------------------------------------------------------------- */

int renumber_residue_range(int imol, const char *chain_id,
			   int start_res, int last_res, int offset) {

   int i=0;
   if (imol >= 0) {
      if (imol <= graphics_info_t::n_molecules()) {
	 if (graphics_info_t::molecules[imol].has_model()) {
	    i = graphics_info_t::molecules[imol].renumber_residue_range(chain_id,
									start_res,
									last_res,
									offset);

	    if (i) {
	       graphics_info_t g;
	       graphics_draw();
	       g.update_go_to_atom_window_on_changed_mol(imol);
	    }
	 }
      }
   }
   std::string cmd = "renumber-residue-range";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(start_res);
   args.push_back(last_res);
   args.push_back(offset);
   add_to_history_typed(cmd, args);
   return i;
}

int change_residue_number(int imol, const char *chain_id, int current_resno, const char *current_inscode, int new_resno, const char *new_inscode) {

   int idone = -1;
   if (is_valid_model_molecule(imol)) {
      std::string chain_id_str(chain_id);
      std::string current_inscode_str(current_inscode);
      std::string new_inscode_str(new_inscode);
      graphics_info_t::molecules[imol].change_residue_number(chain_id_str, current_resno, current_inscode_str, new_resno, new_inscode_str);
      graphics_draw();
      idone = 1;
   } 
   std::string cmd = "change-residue-number";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(current_resno);
   args.push_back(coot::util::single_quote(current_inscode));
   args.push_back(new_resno);
   args.push_back(coot::util::single_quote(new_inscode));
   add_to_history_typed(cmd, args);
   return idone;
} 


/*  ----------------------------------------------------------------------- */
/*                         backup                                           */
/*  ----------------------------------------------------------------------- */

void make_backup(int imol) {

   if (is_valid_model_molecule(imol)) { 
      if (graphics_info_t::molecules[imol].has_model()) {
	 graphics_info_t::molecules[imol].make_backup_from_outside();
      } else {
	 std::cout << "No model for this molecule" << std::endl;
      } 
   } else {
      std::cout << "No model :" << imol << std::endl;
   }
   std::string cmd = "make-backup";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

int backup_state(int imol) {

   int istate = -1;

   if (is_valid_model_molecule(imol)) {
      if (graphics_info_t::molecules[imol].has_model()) {
	 istate = graphics_info_t::molecules[imol].backups_state();
      } else {
	 std::cout << "No model for this molecule" << std::endl;
      } 
      } else {
      std::cout << "No model :" << imol << std::endl;
   }
   std::string cmd = "backup-state";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return istate;
} 

void set_have_unsaved_changes(int imol) {

   if (is_valid_model_molecule(imol)) {
      if (graphics_info_t::molecules[imol].has_model()) {
	 graphics_info_t::molecules[imol].set_have_unsaved_changes_from_outside();
      }
   }
   std::string cmd = "set-have-unsaved-changes";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

int have_unsaved_changes_p(int imol) {

   int r = -1; 
   if (is_valid_model_molecule(imol)) { 
      if (graphics_info_t::molecules[imol].has_model()) {
	 r = graphics_info_t::molecules[imol].Have_unsaved_changes_p();
      }
   }
   return r; 

} 

/*  ------------------------------------------------------------------------ */
/*                         Write PDB file:                                   */
/*  ------------------------------------------------------------------------ */

// return 1 on error, 0 on success
int
write_pdb_file(int imol, const char *file_name) {

   graphics_info_t g;
   int istat = 0;
   if (is_valid_model_molecule(imol)) {
      istat = g.molecules[imol].write_pdb_file(std::string(file_name));
   }
   std::string cmd = "write-pdb-file";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(file_name));
   add_to_history_typed(cmd, args);
   return istat;
}

/*! \brief write molecule number imol's residue range as a PDB to file
  file_name */
/*  return 0 on success, -1 on error. */
int
write_residue_range_to_pdb_file(int imol, const char *chain_id, 
				int resno_start, int resno_end,
				const char *filename) {

   int istat = 1;
   if (is_valid_model_molecule(imol)) {
      std::string chain(chain_id);
      if (resno_end < resno_start) {
	 int tmp = resno_end;
	 resno_end = resno_start;
	 resno_start = tmp;
      } 
      mmdb::Manager *mol =
	 graphics_info_t::molecules[imol].get_residue_range_as_mol(chain, resno_start, resno_end);
      if (mol) {
        mmdb_manager_delete_conect(mol);
	 istat = mol->WritePDBASCII(filename);
	 delete mol; // give back the memory.
      }
   }
   std::string cmd = "write-residue-range-to-pdb-file";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno_start);
   args.push_back(resno_end);
   args.push_back(coot::util::single_quote(filename));
   add_to_history_typed(cmd, args);
   return istat;
}

int write_chain_to_pdb_file(int imol, const char *chain_id, const char *filename) {

   int istat = 0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
      int SelHnd = mol->NewSelection(); // d
      mol->SelectAtoms(SelHnd, 1,
		       chain_id, 
		       mmdb::ANY_RES, "*",
		       mmdb::ANY_RES, "*",
		       "*", // any residue name
		       "*", // atom name
		       "*", // elements
		       "*",  // alt loc.
		       mmdb::SKEY_NEW);
      mmdb::Manager *new_mol = coot::util::create_mmdbmanager_from_atom_selection(mol, SelHnd);
      if (new_mol) {
	 istat = new_mol->WritePDBASCII(filename);
	 delete new_mol;
      }
      mol->DeleteSelection(SelHnd);
   } 
   std::string cmd = "write-chain-to-pdb-file";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(coot::util::single_quote(filename));
   add_to_history_typed(cmd, args);
   return istat;
} 


/*! \brief save all modified coordinates molecules to the default
  names and save the state too. */
int quick_save() {

   // std::cout << "Quick save..." << std::endl;
   graphics_info_t g;
   for (int imol=0; imol<graphics_n_molecules(); imol++) {
      g.molecules[imol].quick_save();
   }

   
   short int il = coot::SCRIPT_UNSET;

#ifdef USE_GUILE
   il = coot::SCHEME_SCRIPT;
   g.save_state_file(g.save_state_file_name.c_str(), il);
#endif    
#ifdef USE_PYTHON
   il = coot::PYTHON_SCRIPT;
   g.save_state_file("0-coot.state.py", il);
#endif    
   return 0;
}


/*! \brief return the state of the write_conect_records_flag.
  */
int get_write_conect_record_state() {
  graphics_info_t g;
  return g.write_conect_records_flag;
}
/*! \} */

/*! \brief set the flag to write (or not) conect records to the PDB file.
  */
void set_write_conect_record_state(int state) {
  graphics_info_t g;
  if (state == 1) {
    g.write_conect_records_flag = 1;
  } else {
    g.write_conect_records_flag = 0;
  }
}

/*! \} */




//       int slen = mtz_in_filename.length(); c
//       if (slen > 4) {
// 	 mtz_out_filename = mtz_in_filename.substr(0,slen - 4) + "-refmac-";
// 	 mtz_out_filename += g.int_to_string(g.molecules[imol_coords].Refmac_count());
// 	 mtz_out_filename += ".mtz";
//       } else {
// 	 mtz_out_filename = "post-refmac";
// 	 mtz_out_filename += g.int_to_string(g.molecules[imol_coords].Refmac_count());
// 	 mtz_out_filename += ".mtz";
//       } 

// If ccp4i_project_dir is "", then carry on and put the log file in
// this directory.  If not, put it in the appropriate project dir. The
// pdb_in etc filename are manipulated in the calling routine.
//
// if swap_map_colours_post_refmac_flag is not 1 then imol_refmac_map is ignored.
//
// make_molecules_flag is 0 or 1: defining if run-refmac-by-filename
// function should create molecules (it should *not* create molecules
// if this is called in a sub-thread (because that will try to update
// the graphics from the subthread and a crash will result).
// 
void
execute_refmac_real(std::string pdb_in_filename,
		    std::string pdb_out_filename,
		    std::string mtz_in_filename,
		    std::string mtz_out_filename,
		    std::string cif_lib_filename,
		    std::string fobs_col_name,
		    std::string sigfobs_col_name,
		    std::string r_free_col_name,
		    short int have_sensible_free_r_flag,
		    short int make_molecules_flag,
		    std::string refmac_count_str,
		    int swap_map_colours_post_refmac_flag,
		    int imol_refmac_map,
		    int diff_map_flag,
		    int phase_combine_flag,
		    std::string phib_string,
		    std::string fom_string, 
		    std::string ccp4i_project_dir) {


   std::vector<std::string> cmds;

   cmds.push_back(std::string("run-refmac-by-filename"));
// BL says:: again debackslashing
   cmds.push_back(single_quote(coot::util::intelligent_debackslash(pdb_in_filename)));
   cmds.push_back(single_quote(coot::util::intelligent_debackslash(pdb_out_filename)));
   cmds.push_back(single_quote(coot::util::intelligent_debackslash(mtz_in_filename)));
   cmds.push_back(single_quote(coot::util::intelligent_debackslash(mtz_out_filename)));
   cmds.push_back(single_quote(coot::util::intelligent_debackslash(cif_lib_filename)));
   cmds.push_back(refmac_count_str);
   cmds.push_back(graphics_info_t::int_to_string(swap_map_colours_post_refmac_flag));
   cmds.push_back(graphics_info_t::int_to_string(imol_refmac_map));
   cmds.push_back(graphics_info_t::int_to_string(diff_map_flag));
   cmds.push_back(graphics_info_t::int_to_string(phase_combine_flag));

   std::string phase_combine_cmd;
   if (phase_combine_flag > 0 && phase_combine_flag < 3) {
#ifdef USE_GUILE
      phase_combine_cmd += "(cons ";
      phase_combine_cmd += single_quote(phib_string);
      phase_combine_cmd += " ";
      phase_combine_cmd += single_quote(fom_string);
      phase_combine_cmd += ")";
#else
#ifdef USE_PYTHON
      phase_combine_cmd += "[\'";
      phase_combine_cmd += phib_string;
      phase_combine_cmd += "\', ";
      phase_combine_cmd += single_quote(fom_string);
      phase_combine_cmd += "]";
#endif // USE_PYTHON
#endif // USE_GUILE
   } else {
      phase_combine_cmd += single_quote("dummy");
   }
   cmds.push_back(phase_combine_cmd);

   cmds.push_back(graphics_info_t::int_to_string(graphics_info_t::refmac_ncycles));
   cmds.push_back(graphics_info_t::int_to_string(make_molecules_flag));
   cmds.push_back(single_quote(coot::util::intelligent_debackslash(ccp4i_project_dir)));
   if (phase_combine_flag == 3 && fobs_col_name != "") {
     cmds.push_back(fobs_col_name);
     cmds.push_back(sigfobs_col_name);
   } else {
     cmds.push_back(single_quote(fobs_col_name));
     cmds.push_back(single_quote(sigfobs_col_name));
   }
   std::cout << "DEBUG in execute_refmac_real ccp4i_project_dir :"
	     << single_quote(coot::util::intelligent_debackslash(ccp4i_project_dir))
	     << ":" << std::endl;
		
   if (have_sensible_free_r_flag) { 
      cmds.push_back(single_quote(r_free_col_name));
   }

   graphics_info_t g;
   short int ilang = coot::STATE_SCM;
   std::string cmd;

#ifdef USE_PYTHON
#ifndef USE_GUILE
   ilang = coot::STATE_PYTHON;
#endif
#endif
   if (ilang == coot::STATE_PYTHON) { 
      cmd = g.state_command(cmds, ilang);
#ifdef USE_PYTHON
      safe_python_command(cmd);
#endif
   } else {
      cmd = g.state_command(cmds, ilang);
      safe_scheme_command(cmd);
   } 
} 

int set_refmac_molecule(int imol) {
   std::string cmd = "set-refmac-molecule";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   graphics_info_t::refmac_molecule = imol;
   return imol;
}


void set_refmac_counter(int imol, int refmac_count) {

   graphics_info_t g;
   if (imol< g.n_molecules()) {
      g.molecules[imol].set_refmac_counter(refmac_count);
      std::cout << "INFO:: refmac counter of molecule number " << imol
		<< " incremented to " << refmac_count << std::endl;
   } else {
      std::cout << "WARNING:: refmac counter of molecule number " << imol
		<< " not incremented to " << refmac_count << std::endl;
   } 
   std::string cmd = "set-refmac-counter";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(refmac_count);
   add_to_history_typed(cmd, args);
} 


const char *refmac_name(int imol) {

   std::string cmd = "refmac-name";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   graphics_info_t g;
   return g.molecules[imol].Refmac_in_name().c_str();
} 

int get_refmac_refinement_method() {
  
  graphics_info_t g;
  return g.refmac_refinement_method;
}

void set_refmac_refinement_method(int method) {

  graphics_info_t g;
  g.set_refmac_refinement_method(method);
}

int get_refmac_phase_input() {
  
  graphics_info_t g;
  return g.refmac_phase_input;
}

void set_refmac_use_tls(int state) {

  graphics_info_t g;
  g.set_refmac_use_tls(state);
}

int refmac_use_tls_state() {

  graphics_info_t g;
  return g.refmac_use_tls_flag;
}

void set_refmac_use_twin(int state) {

  graphics_info_t g;
  g.set_refmac_use_twin(state);
}

int refmac_use_twin_state() {

  graphics_info_t g;
  return g.refmac_use_twin_flag;
}

int refmac_use_sad_state() {

  graphics_info_t g;
  return g.refmac_use_sad_flag;
}

int get_refmac_ncycles() {
  
  graphics_info_t g;
  return g.refmac_ncycles;
}

void set_refmac_ncycles(int no_cycles) {

  graphics_info_t g;
  g.set_refmac_n_cycles(no_cycles);
}

void add_refmac_ncycle_no(int cycle) {

  graphics_info_t g;
  g.add_refmac_ncycle_no(cycle);
}

void set_refmac_use_ncs(int state) {

  graphics_info_t g;
  g.set_refmac_use_ncs(state);
}

int refmac_use_ncs_state() {

  graphics_info_t g;
  return g.refmac_use_ncs_flag;
}

void set_refmac_use_intensities(int state) {

  graphics_info_t g;
  g.set_refmac_use_intensities(state);
}

int refmac_use_intensities_state() {

  graphics_info_t g;
  return g.refmac_use_intensities_flag;
}
  

int refmac_imol_coords() {

  graphics_info_t g;
  return g.refmac_molecule;
}

/*! \brief add an atom to refmac_sad_atoms (used in refmac with SAD option)
  list with atom_name and  fp, and fpp (and/or wavelength),
  -9999 to not use fp/fpp or wavelength 
  adds a new atom or overwrites existing ones with new parameters */
void
add_refmac_sad_atom(const char *atom_name, float fp, float fpp, float lambda) {

  graphics_info_t g;
  g.add_refmac_sad_atom(atom_name, fp, fpp, lambda);

}

/* !brief add an atom to refmac_sad_atoms (used in refmac with SAD option)
  list with atom_name and  fp, and fpp 
  adds a new atom or overwrites existing ones with new parameters */
void
add_refmac_sad_atom_fp(const char *atom_name, float fp, float fpp) {

  graphics_info_t g;
  g.add_refmac_sad_atom(atom_name, fp, fpp, -9999);

}

/* !brief add an atom to refmac_sad_atoms (used in refmac with SAD option)
  list with atom_name and wavlength, fp and fpp will be calculated 
  adds a new atom or overwrites existing ones with new parameters */
void
add_refmac_sad_atom_lambda(const char *atom_name, float lambda) {

  graphics_info_t g;
  g.add_refmac_sad_atom(atom_name, -9999, -9999, lambda);

}

/*! \brief clear the refmac_sad_atoms list */
void
clear_refmac_sad_atoms() {

  graphics_info_t g;
  g.refmac_sad_atoms.clear();
}

#ifdef USE_GUILE
/*! \brief retrive the stored refmac_sad_atoms to be used in refmac with SAD option */
/*  return list of e.g. (list (list "SE" -8.0 -4.0 #f) ...)  */
SCM get_refmac_sad_atom_info_scm() {

  SCM r = SCM_EOL;
  std::vector<coot::refmac::sad_atom_info_t> sad_atoms = graphics_info_t::refmac_sad_atoms;
  for (unsigned int i=0; i<sad_atoms.size(); i++) {
    SCM ls = SCM_EOL;
    std::string atom_name = sad_atoms[i].atom_name;
    float fp = sad_atoms[i].fp;
    float fpp = sad_atoms[i].fpp;
    float lambda = sad_atoms[i].lambda;
    ls = scm_cons(scm_makfrom0str(atom_name.c_str()) ,ls);
    if (fabs(fp + 9999) <= 0.1) {
      ls = scm_cons(SCM_BOOL_F, ls);
    } else {
      ls = scm_cons(scm_double2num(fp), ls);
    }
    if (fabs(fpp + 9999) <= 0.1) {
      ls = scm_cons(SCM_BOOL_F, ls);
    } else {
      ls = scm_cons(scm_double2num(fpp), ls);
    }
    if (fabs(lambda + 9999) <= 0.1) {
      ls = scm_cons(SCM_BOOL_F, ls);
    } else {
      ls = scm_cons(scm_double2num(lambda), ls);
    }
    r = scm_cons(scm_reverse(ls), r);
  }
  r = scm_reverse(r);
  return r;
}
#endif // GUILE

#ifdef USE_PYTHON
/*! \brief retrive the stored refmac_sad_atoms to be used in refmac with SAD option */
/*  return list of e.g. [["SE", -8.0, -4.0, None], ...]  */
PyObject *get_refmac_sad_atom_info_py() {

  PyObject *r = PyList_New(0);

  std::vector<coot::refmac::sad_atom_info_t> sad_atoms = graphics_info_t::refmac_sad_atoms;
  for (unsigned int i=0; i<sad_atoms.size(); i++) {
    PyObject *ls = PyList_New(0);
    std::string atom_name = sad_atoms[i].atom_name;
    float fp = sad_atoms[i].fp;
    float fpp = sad_atoms[i].fpp;
    float lambda = sad_atoms[i].lambda;
    PyList_Append(ls, PyString_FromString(atom_name.c_str()));
    if (fabs(fp + 9999) <= 0.1) {
      Py_INCREF(Py_None);
      PyList_Append(ls, Py_None);
    } else {
      PyList_Append(ls, PyFloat_FromDouble(fp));
    }
    if (fabs(fpp + 9999) <= 0.1) {
      Py_INCREF(Py_None);
      PyList_Append(ls, Py_None);
    } else {
      PyList_Append(ls, PyFloat_FromDouble(fpp));
    }
    if (fabs(lambda + 9999) <= 0.1) {
      Py_INCREF(Py_None);
      PyList_Append(ls, Py_None);
    } else {
      PyList_Append(ls, PyFloat_FromDouble(lambda));
    }
    PyList_Append(r, ls);
    Py_XDECREF(ls);
  }
  return r;
}
#endif // USE_PYTHON


short int
get_refmac_used_mtz_file_state() {

  graphics_info_t g;
  return g.refmac_used_mtz_file_flag;
}

void
set_refmac_used_mtz_file(int state) {

  graphics_info_t g;
  return g.set_refmac_used_mtz_file(state);
}

const gchar *get_saved_refmac_file_filename() {

  graphics_info_t g;
  return g.saved_refmac_file_filename;
}

void
set_stored_refmac_file_mtz_filename(int imol, const char *mtz_filename) {

   if (imol < graphics_n_molecules()) { 
     graphics_info_t::molecules[imol].store_refmac_file_mtz_filename(std::string(mtz_filename));
   }
}

short int 
add_OXT_to_residue(int imol, int resno, const char *insertion_code, const char *chain_id) {

   short int istat = -1; 
   if (imol < graphics_n_molecules()) { 
      istat = graphics_info_t::molecules[imol].add_OXT_to_residue(resno, std::string(insertion_code),
								  std::string(chain_id));
      graphics_info_t::molecules[imol].update_symmetry();
      graphics_draw();
   }
   std::string cmd = "add-OXT-to-residue";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(resno);
   args.push_back(coot::util::single_quote(insertion_code));
   args.push_back(coot::util::single_quote(chain_id));
   add_to_history_typed(cmd, args);
   return istat;
}



/*  ------------------------------------------------------------------------ */
/*                         db_mainchain:                                     */
/*  ------------------------------------------------------------------------ */

// The button callback:
void 
do_db_main(short int state) {
   graphics_info_t g;
   g.in_db_main_define = state;
   if (state) { 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;
      std::cout << "click on 2 atoms to define a the range of residues to"
		<< " convert to mainchain" << std::endl;
   } else {
      g.pick_pending_flag = 0;
      g.normal_cursor();
   }
   std::string cmd = "do-db-main";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}


void
db_mainchain(int imol,
	     const char *chain_id,
	     int iresno_start,
	     int iresno_end,
	     const char *direction_string) {
   
   if (imol < graphics_n_molecules()) {
      graphics_info_t g;
      g.execute_db_main(imol, std::string(chain_id), iresno_start, iresno_end,
			std::string(direction_string));
   } else {
      std::cout << "WARNING molecule index error" << std::endl;
   } 
   std::string cmd = "db-mainchain";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(iresno_start);
   args.push_back(iresno_end);
   args.push_back(coot::util::single_quote(direction_string));
   add_to_history_typed(cmd, args);
}

/*  ----------------------------------------------------------------------- */
/*                  alternate conformation                                  */
/*  ----------------------------------------------------------------------- */

// so shall we define a nomenclature for the split type:
// 0: split at Ca
// 1: split whole residue
// 2: split residue range

/* c-interface-build function */
short int alt_conf_split_type_number() {
   add_to_history_simple("alt-conf-split-type-number");
   return graphics_info_t::alt_conf_split_type_number();
}


void set_add_alt_conf_split_type_number(short int i) { 
   graphics_info_t::alt_conf_split_type = i;
   std::string cmd = "set-add-alt-conf-split-type-number";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
   
} 

void unset_add_alt_conf_dialog()  { /* set the static dialog holder in
				     graphics info to NULL */
   graphics_info_t::add_alt_conf_dialog = NULL;
}

void unset_add_alt_conf_define() {  /* turn off pending atom pick */

   graphics_info_t g;
   graphics_info_t::in_add_alt_conf_define = 0;
   g.normal_cursor();
}


void set_show_alt_conf_intermediate_atoms(int i) {
   graphics_info_t::show_alt_conf_intermediate_atoms_flag = i;
   std::string cmd = "set-show-alt-conf-intermediate-atoms";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
}

int show_alt_conf_intermediate_atoms_state() {
   add_to_history_simple("show-alt-conf-intermediate-atoms-state");
   return graphics_info_t::show_alt_conf_intermediate_atoms_flag;
}

void zero_occupancy_residue_range(int imol, const char *chain_id, int ires1, int ires2) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_occupancy_residue_range(std::string(chain_id), ires1, ires2, 0.0);
   } else {
      std::cout << "WARNING:: invalid model molecule number in zero_occupancy_residue_range "
		<< imol << std::endl;
   }
   std::string cmd = "zero-occupancy-residue-range";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(ires1);
   args.push_back(ires2);
   add_to_history_typed(cmd, args);
   graphics_draw();
}

void fill_occupancy_residue_range(int imol, const char *chain_id, int ires1, int ires2) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_occupancy_residue_range(std::string(chain_id), ires1, ires2, 1.0);
   } else {
      std::cout << "WARNING:: invalid model molecule number in fill_occupancy_residue_range "
		<< imol << std::endl;
   }
   graphics_draw();
   std::string cmd = "fill-occupancy-residue-range";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(ires1);
   args.push_back(ires2);
   add_to_history_typed(cmd, args);
}

void
set_b_factor_residue_range(int imol, const char *chain_id, int ires1, int ires2, float bval) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_b_factor_residue_range(std::string(chain_id), ires1, ires2, bval);
   } else {
      std::cout << "WARNING:: invalid model molecule number in set_b_factor_residue_range "
		<< imol << std::endl;
   }
   graphics_draw();
   std::string cmd = "set-b-factor-residue-range";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(ires1);
   args.push_back(ires2);
   args.push_back(bval);
   add_to_history_typed(cmd, args);

}

void
reset_b_factor_residue_range(int imol, const char *chain_id, int ires1, int ires2) {

   if (is_valid_model_molecule(imol)) {
     graphics_info_t::molecules[imol].set_b_factor_residue_range(std::string(chain_id), ires1, ires2, graphics_info_t::default_new_atoms_b_factor);
   } else {
      std::cout << "WARNING:: invalid model molecule number in reset_b_factor_residue_range "
		<< imol << std::endl;
   }
   graphics_draw();
   std::string cmd = "reset-b-factor-residue-range";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(ires1);
   args.push_back(ires2);
   add_to_history_typed(cmd, args);

}

/*  ----------------------------------------------------------------------- */
/*                  Edit Chi Angles                                         */
/*  ----------------------------------------------------------------------- */
void setup_edit_chi_angles(short int state) {

   graphics_info_t g;
   if (state) { 
      g.in_edit_chi_angles_define = 1;
      std::cout << "Click on an atom in the residue that you want to edit" << std::endl;
      g.pick_cursor_maybe();
      g.add_status_bar_text("Click on a atom. The clicked atom affects the torsion's wagging dog/tail...");
      g.pick_pending_flag = 1;
   } else {
      g.in_edit_chi_angles_define = 0;
   }
   std::string cmd = "setup-edit-chi-angles";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void rotate_chi(float am) {

   graphics_info_t g;
   if (g.in_edit_chi_mode_flag || g.in_edit_torsion_general_flag) {
      g.rotate_chi(am, am);
   }
} 


void setup_torsion_general(short int state) {

   graphics_info_t g;
   if (state) {
      g.in_torsion_general_define = 1;
      g.pick_cursor_maybe();
      g.add_status_bar_text("Click on a atom. The order of the clicked atoms affects the torsion's wagging dog/tail...");
      g.pick_pending_flag = 1;
   } else {
      g.in_torsion_general_define = 0;
   }
   std::string cmd = "setup-torsion-general";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void toggle_torsion_general_reverse()  { /* a bool really */
   if (graphics_info_t::torsion_general_reverse_flag)
      graphics_info_t::torsion_general_reverse_flag = 0;
   else 
      graphics_info_t::torsion_general_reverse_flag = 1;
}


int set_show_chi_angle_bond(int imode) {

   graphics_info_t::draw_chi_angle_flash_bond_flag = imode;
   graphics_draw();
   std::string cmd = "set-show-chi-angle-bond";
   std::vector<coot::command_arg_t> args;
   args.push_back(imode);
   add_to_history_typed(cmd, args);
   return 0; // should be a void function, I imagine.
} 


/* a callback from the callbacks.c, setting the state of
   graphics_info_t::edit_chi_angles_reverse_fragment flag */
void set_edit_chi_angles_reverse_fragment_state(short int istate) {

   graphics_info_t::edit_chi_angles_reverse_fragment = istate;

} 


// Set a flag: Should torsions that move hydrogens be
// considered/displayed in button box?
// 
void set_find_hydrogen_torsions(short int state) {
   graphics_info_t g;
   g.find_hydrogen_torsions_flag = static_cast<bool>(state);
   std::string cmd = "set-find-hydrogen-torsion";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void set_graphics_edit_current_chi(int ichi) { /* button callback */

   graphics_info_t g;
   graphics_info_t::edit_chi_current_chi = ichi;
   if (ichi == 0) {
      graphics_info_t::in_edit_chi_mode_flag = 0; // off
   } else {
      graphics_info_t::in_edit_chi_mode_flag = 1; // on
      g.setup_flash_bond_internal(ichi-1);
   }

}

void unset_moving_atom_move_chis() { 
   graphics_info_t::moving_atoms_move_chis_flag = 0; // keyboard 1,2,3
						     // etc cant put
						     // graphics/mouse
						     // into rotate
						     // chi mode.
} 

// BL says:: maybe this should rather go via a set state function!?
void set_moving_atom_move_chis() { 
   graphics_info_t::moving_atoms_move_chis_flag = 1; // keyboard 1,2,3
						     // etc cant put
						     // graphics/mouse
						     // into rotate
						     // chi mode.
} 


// altconf is ignored here
int edit_chi_angles(int imol, const char *chain_id, int resno, 
		    const char *ins_code, const char *altconf) {

   int status = 0;

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      // type void
      int atom_index = atom_index_first_atom_in_residue(imol, chain_id, resno, ins_code);
      if (atom_index > -1) {
	 // 20090815
	 // Do I want this?
	 // g.chi_angle_alt_conf = molecules[imol].atom_sel.atom_selection[atom_index]->altLoc;
	 // or this?
	 g.chi_angle_alt_conf = altconf;
	 g.execute_edit_chi_angles(atom_index, imol);
	 status = 1;
      }
   }
   return status;
}





void translate_molecule_by(int imol, float x, float y, float z) { 

   if (is_valid_model_molecule(imol)) {
      if (graphics_info_t::molecules[imol].has_model()) { 
	 graphics_info_t::molecules[imol].translate_by(x, y, z);
      }
   }
   graphics_draw();
} 

/*! \brief transform molecule number imol by the given rotation
  matrix, then translate by (x,y,z) in Angstroms  */
void transform_molecule_by(int imol, 
			   float m11, float m12, float m13,
			   float m21, float m22, float m23,
			   float m31, float m32, float m33,
			   float x, float y, float z) {

   if (is_valid_model_molecule(imol)) {
      clipper::Mat33<double> clipper_mat(m11, m12, m13,
					 m21, m22, m23,
					 m31, m32, m33);
      clipper::Coord_orth cco(x,y,z);
      clipper::RTop_orth rtop(clipper_mat, cco);
      graphics_info_t::molecules[imol].transform_by(rtop);
   }
   graphics_draw();

}

void transform_zone(int imol, const char *chain_id, int resno_start, int resno_end, const char *ins_code,
		    float m11, float m12, float m13,
		    float m21, float m22, float m23,
		    float m31, float m32, float m33,
		    float x, float y, float z) {

   if (is_valid_model_molecule(imol)) {

      clipper::Mat33<double> clipper_mat(m11, m12, m13,
					 m21, m22, m23,
					 m31, m32, m33);
      clipper::Coord_orth cco(x,y,z);
      clipper::RTop_orth rtop(clipper_mat, cco);
      bool do_backup = 1;
      graphics_info_t::molecules[imol].transform_zone_by(chain_id, resno_start, resno_end, ins_code, rtop,
							 do_backup);
      
      std::string cmd = "transform-zone";
      std::vector<coot::command_arg_t> args;
      args.push_back(imol);
      args.push_back(chain_id);
      args.push_back(resno_start);
      args.push_back(resno_end);
      args.push_back(ins_code);
      args.push_back(m11);
      args.push_back(m12);
      args.push_back(m13);
      args.push_back(m21);
      args.push_back(m22);
      args.push_back(m23);
      args.push_back(m31);
      args.push_back(m32);
      args.push_back(m33);
      args.push_back(x);
      args.push_back(y);
      args.push_back(z);
      add_to_history_typed(cmd, args);
   }
} 



// Sequenc utils

void assign_fasta_sequence(int imol, const char *chain_id_in, const char *seq) { 

   // format "> name \n <sequence>"
   if (is_valid_model_molecule(imol)) {
      const std::string chain_id = chain_id_in;
      graphics_info_t::molecules[imol].assign_fasta_sequence(chain_id, std::string(seq));
   }
}

void assign_pir_sequence(int imol, const char *chain_id_in, const char *seq) { 

   if (is_valid_model_molecule(imol)) {
      const std::string chain_id = chain_id_in;
      graphics_info_t::molecules[imol].assign_pir_sequence(chain_id, std::string(seq));
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("assign-pir-sequence");
   command_strings.push_back(coot::util::int_to_string(imol));
   command_strings.push_back(single_quote(chain_id_in));
   command_strings.push_back(single_quote(seq));
   add_to_history(command_strings);
}

void assign_sequence_from_file(int imol, const char *file) {
   if (is_valid_model_molecule(imol)) {
    graphics_info_t::molecules[imol].assign_sequence_from_file(std::string(file));
  }
  std::string cmd = "assign-sequence-from-file";
  std::vector<coot::command_arg_t> args;
  args.push_back(imol);
  args.push_back(single_quote(file));
  add_to_history_typed(cmd, args);
}

void assign_sequence_from_string(int imol, const char *chain_id_in, const char *seq) {
   if (is_valid_model_molecule(imol)) {
    const std::string chain_id = chain_id_in;
    graphics_info_t::molecules[imol].assign_sequence_to_NCS_related_chains_from_string(chain_id, std::string(seq));
  }
  std::string cmd = "assign-sequence-from-string";
  std::vector<coot::command_arg_t> args;
  args.push_back(imol);
  args.push_back(single_quote(chain_id_in));
  args.push_back(single_quote(seq));
  add_to_history_typed(cmd, args);
}

void delete_all_sequences_from_molecule(int imol) {

   if (is_valid_model_molecule(imol)) {
      if ((graphics_info_t::molecules[imol].sequence_info()).size() > 0) {
	 std::cout <<"BL DEBUG:: we have sequence info"<<std::endl;
	 graphics_info_t::molecules[imol].delete_all_sequences_from_molecule();
      } else {
	 std::cout <<"BL DEBUG:: no sequence info"<<std::endl;
      }
   }
}

void delete_sequence_by_chain_id(int imol, const char *chain_id_in) {
   if (is_valid_model_molecule(imol)) { 
      if ((graphics_info_t::molecules[imol].sequence_info()).size() > 0) {
	 std::cout <<"BL DEBUG:: we have sequence info"<<std::endl;
	 const std::string chain_id = chain_id_in;
	 graphics_info_t::molecules[imol].delete_sequence_by_chain_id(chain_id);
      } else {
	 std::cout <<"BL DEBUG:: no sequence info"<<std::endl;
      }  
   }
}


/*  ----------------------------------------------------------------------- */
/*                  trim                                                    */
/*  ----------------------------------------------------------------------- */
void 
trim_molecule_by_map(int imol_coords, int imol_map, 
		     float map_level, int delete_or_zero_occ_flag) {

   graphics_info_t g;
   if (is_valid_model_molecule(imol_coords)) { 
      if (is_valid_map_molecule(imol_map)) {
	 if (g.molecules[imol_map].has_xmap()) { 
	    int iv = g.molecules[imol_coords].trim_by_map(g.molecules[imol_map].xmap, 
							  map_level,
							  delete_or_zero_occ_flag);
	    if (iv) 
	       graphics_draw();
	 } else { 
	    std::cout << "molecule " << imol_map << " has no map" << std::endl;
	 } 
      } else { 
	 std::cout << "No such molecule for map as " << imol_map << std::endl;
      } 
   } else { 
      std::cout << "No such molecule for model as " << imol_coords << std::endl;
   } 
}



/*  ----------------------------------------------------------------------- */
/*               Simplex Refinement                                         */
/*  ----------------------------------------------------------------------- */
//
// Perhaps this should be a just a call to a graphics_info_t function?
// 
void
fit_residue_range_to_map_by_simplex(int res1, int res2, char *altloc,
				    char *chain_id, int imol, int imol_for_map) {


   // The molecule_class_info_t updates its bonds.
   // 
   if (is_valid_model_molecule(imol)) {
      if (graphics_info_t::molecules[imol].has_model()) {
	 if (is_valid_map_molecule(imol_for_map)) { 
	    if (graphics_info_t::molecules[imol_for_map].has_xmap()) { 
	       graphics_info_t::molecules[imol].fit_residue_range_to_map_by_simplex(res1, res2, altloc, chain_id, imol_for_map);
	    } else {
	       std::cout << "No map for molecule " << imol_for_map << std::endl;
	    }
	 } else {
	    std::cout << "No molecule " << imol_for_map << std::endl;
	 }
      } else {
	 std::cout << "No coordinates for molecule " << imol << std::endl;
      } 
   } else {
      std::cout << "No molecule " << imol << std::endl;
   }

   graphics_draw();

} 

// Return a score of the fit to the map.
// 
float
score_residue_range_fit_to_map(int res1, int res2, char *altloc,
			       char *chain_id, int imol, int imol_for_map) {

   float f = 0.0;

   if (is_valid_model_molecule(imol)) {
      if (graphics_info_t::molecules[imol].has_model()) {
	 if (is_valid_map_molecule(imol_for_map)) {
	    if (graphics_info_t::molecules[imol_for_map].has_xmap()) { 
	       f = graphics_info_t::molecules[imol].score_residue_range_fit_to_map(res1, res2, altloc, chain_id, imol_for_map);
	    } else {
	       std::cout << "No map for molecule " << imol_for_map << std::endl;
	    }
	 } else {
	    std::cout << "No molecule " << imol_for_map << std::endl;
	 }
      } else {
	 std::cout << "No coordinates for molecule " << imol << std::endl;
      } 
   } else {
      std::cout << "No molecule " << imol << std::endl;
   }
   return f;
}
									 
/*  ----------------------------------------------------------------------- */
/*                         Planar Peptide Restraints                        */
/*  ----------------------------------------------------------------------- */

void add_planar_peptide_restraints() {

   graphics_info_t g;
   g.Geom_p()->add_planar_peptide_restraint();
} 

void remove_planar_peptide_restraints() {

   graphics_info_t g;
   g.Geom_p()->remove_planar_peptide_restraint();
}

/* return 1 if planar peptide restraints are on, 0 if off */
int planar_peptide_restraints_state() {

   graphics_info_t g;
   bool r = g.Geom_p()->planar_peptide_restraint_state();
   int rr = r;
   return rr;
} 


void add_omega_torsion_restriants() {

   graphics_info_t g;
   g.Geom_p()->add_omega_peptide_restraints();
} 

/*! \brief remove omega restraints on CIS and TRANS linked residues. */
void remove_omega_torsion_restriants() {

   graphics_info_t g;
   g.Geom_p()->remove_omega_peptide_restraints();
}

#ifdef USE_GUILE
// e.g. atom_spec: '("A" 81 "" " CA " "")
//      position   '(2.3 3.4 5.6)
SCM drag_intermediate_atom_scm(SCM atom_spec, SCM position) {
   SCM retval = SCM_BOOL_F;
   std::pair<bool, coot::atom_spec_t> p = make_atom_spec(atom_spec);
   if (p.first) {
      SCM pos_length_scm = scm_length(position);
      int pos_length = scm_to_int(pos_length_scm);
      if (pos_length == 3) {
	 SCM x_scm = scm_list_ref(position, SCM_MAKINUM(0));
	 SCM y_scm = scm_list_ref(position, SCM_MAKINUM(1));
	 SCM z_scm = scm_list_ref(position, SCM_MAKINUM(2));
	 double x = scm_to_double(x_scm);
	 double y = scm_to_double(y_scm);
	 double z = scm_to_double(z_scm);
	 clipper::Coord_orth pt(x,y,z);
	 graphics_info_t::drag_intermediate_atom(p.second, pt);
      }
   }
   return retval;
}
#endif

#ifdef USE_GUILE
SCM mark_atom_as_fixed_scm(int imol, SCM atom_spec, int state) {
   SCM retval = SCM_BOOL_F;
   std::pair<bool, coot::atom_spec_t> p = make_atom_spec(atom_spec);
   if (p.first) {
      graphics_info_t::mark_atom_as_fixed(imol, p.second, state);
      graphics_draw();
   }
   return retval;
}
#endif


#ifdef USE_PYTHON
PyObject *drag_intermediate_atom_py(PyObject *atom_spec, PyObject *position) {
// e.g. atom_spec: ["A", 81, "", " CA ", ""]
//      position   [2.3, 3.4, 5.6]
   PyObject *retval = Py_False;
   PyObject *x_py;
   PyObject *y_py;
   PyObject *z_py;
   std::pair<bool, coot::atom_spec_t> p = make_atom_spec_py(atom_spec);
   if (p.first) {
      int pos_length = PyObject_Length(position);
      if (pos_length == 3) {
	 x_py = PyList_GetItem(position, 0);
	 y_py = PyList_GetItem(position, 1);
	 z_py = PyList_GetItem(position, 2);
	 double x = PyFloat_AsDouble(x_py);
	 double y = PyFloat_AsDouble(y_py);
	 double z = PyFloat_AsDouble(z_py);
	 clipper::Coord_orth pt(x,y,z);
	 graphics_info_t::drag_intermediate_atom(p.second, pt);
	 retval = Py_True; // Shall we return True if atom is dragged?
      }
   }

   Py_INCREF(retval);
   return retval;
}
#endif // USE_PYTHON


#ifdef USE_PYTHON
PyObject *mark_atom_as_fixed_py(int imol, PyObject *atom_spec, int state) {
   PyObject *retval = Py_False;
   std::pair<bool, coot::atom_spec_t> p = make_atom_spec_py(atom_spec);
   if (p.first) {
      graphics_info_t::mark_atom_as_fixed(imol, p.second, state);
      graphics_draw();
      retval = Py_True; // Shall we return True if atom got marked?
   }
   Py_INCREF(retval);
   return retval;
}
#endif // USE_PYTHON 



/*! \brief clear all fixed atoms */
void clear_all_fixed_atoms(int imol) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].clear_all_fixed_atoms();
      graphics_draw();
   }
}


void clear_fixed_atoms_all() {

   for (int i=0; i<graphics_info_t::n_molecules(); i++) {
      if (is_valid_model_molecule(i)) {
	 clear_all_fixed_atoms(i);
      }
   }
   graphics_draw();
}


// ipick is on/off, is_unpick is when we are picking a fixed atom to
// be unfixed.
// 
void setup_fixed_atom_pick(short int ipick, short int is_unpick) {

   graphics_info_t g;
   if (ipick == 0) {
      graphics_info_t::in_fixed_atom_define = coot::FIXED_ATOM_NO_PICK;
   } else {
      g.pick_cursor_maybe();
      if (is_unpick) {
	 graphics_info_t::in_fixed_atom_define = coot::FIXED_ATOM_UNFIX;
      } else { 
	 graphics_info_t::in_fixed_atom_define = coot::FIXED_ATOM_FIX;
      }
   }
}



#ifdef USE_GUILE
// return e.g (list 1 "C" "D")
// 
SCM merge_molecules(SCM add_molecules, int imol) {
   SCM r = SCM_BOOL_F;

   std::vector<int> vam;
   SCM l_length_scm = scm_length(add_molecules);

#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)

   int l_length = scm_to_int(l_length_scm);
   for (int i=0; i<l_length; i++) {
      SCM le = scm_list_ref(add_molecules, SCM_MAKINUM(i));
      int ii = scm_to_int(le);
      vam.push_back(ii);
   } 
   
#else
   
   int l_length = gh_scm2int(l_length_scm);
   for (int i=0; i<l_length; i++) {
      SCM le = scm_list_ref(add_molecules, SCM_MAKINUM(i));
      int ii =  gh_scm2int(le);
      vam.push_back(ii);
   }
#endif // SCM_VERSION   
   
   std::pair<int, std::vector<std::string> > v = merge_molecules_by_vector(vam, imol);

   SCM vos = generic_string_vector_to_list_internal(v.second);
   r = SCM_EOL;
   r = scm_cons(vos, r);
   r = scm_cons(SCM_MAKINUM(v.first), r);
   
   return r;
}
#endif

#ifdef USE_PYTHON
// some python version of the merge_molecules()
// return e.g [1,"C","D"]
// 
PyObject *merge_molecules_py(PyObject *add_molecules, int imol) {
   PyObject *r;
   PyObject *le;
   PyObject *vos;
   r = Py_False;

   std::vector<int> vam;

   int l_length = PyObject_Length(add_molecules);
   for (int i=0; i<l_length; i++) {
      le = PyList_GetItem(add_molecules, i);
//      int ii = (int)le;
      int ii = PyInt_AsLong(le);
      vam.push_back(ii);
   } 
   
   std::pair<int, std::vector<std::string> > v = merge_molecules_by_vector(vam, imol);

   vos = generic_string_vector_to_list_internal_py(v.second);
   int vos_length = PyObject_Length(vos);
   r = PyList_New(vos_length + 1);
   PyList_SetItem(r, 0, PyInt_FromLong(v.first));
   for (int i=0; i<vos_length; i++) {
      PyList_SetItem(r, i+1, PyList_GetItem(vos,i));
   }
   
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif

std::pair<int, std::vector<std::string> > 
merge_molecules_by_vector(const std::vector<int> &add_molecules, int imol) {

   std::pair<int, std::vector<std::string> >  merged_info;
   
   std::vector<atom_selection_container_t> add_molecules_at_sels;
   if (is_valid_model_molecule(imol)) { 
      for (unsigned int i=0; i<add_molecules.size(); i++) {
	 if (is_valid_model_molecule(add_molecules[i])) { 
	    if (add_molecules[i] != imol) { 
	       add_molecules_at_sels.push_back(graphics_info_t::molecules[add_molecules[i]].atom_sel);
	       set_mol_displayed(add_molecules[i], 0);
	       set_mol_active(add_molecules[i], 0);
	    }
	 }
      }
   }
   if (add_molecules_at_sels.size() > 0) { 
      merged_info = graphics_info_t::molecules[imol].merge_molecules(add_molecules_at_sels);
   }

   if (graphics_info_t::use_graphics_interface_flag) {
      graphics_info_t g;
      g.update_go_to_atom_window_on_changed_mol(imol);
   } 
   return merged_info;
}






/*  ----------------------------------------------------------------------- */
/*                         construct a molecule and update                  */
/*  ----------------------------------------------------------------------- */

#ifdef USE_GUILE
int clear_and_update_molecule(int molecule_number, SCM molecule_expression) {

   int state = 0; 
   if (is_valid_model_molecule(molecule_number)) {

      mmdb::Manager *mol =
	 mmdb_manager_from_scheme_expression(molecule_expression);

      if (mol) { 
	 state = 1;
	 graphics_info_t::molecules[molecule_number].replace_molecule(mol);
	 graphics_draw();
      }
   } else {
      std::cout << "WARNING:: " << molecule_number << " is not a valid model molecule"
		<< std::endl;
   } 
   return state;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
int clear_and_update_molecule_py(int molecule_number, PyObject *molecule_expression) {

   int state = 0;
   if (is_valid_model_molecule(molecule_number)) {
      
      std::deque<mmdb::Model *> model_list =
         mmdb_models_from_python_expression(molecule_expression);

      if (!model_list.empty()) {
         state = 1;
         graphics_info_t::molecules[molecule_number].replace_models(model_list);
         graphics_draw();
      }
   }
   return state;
}
#endif // USE_GUILE

#ifdef USE_GUILE
// Return a molecule number, -1 on error.
int add_molecule(SCM molecule_expression, const char *name) {

   int imol = -1; 
   mmdb::Manager *mol =
      mmdb_manager_from_scheme_expression(molecule_expression);
   if (mol) {
      imol = graphics_info_t::create_molecule();
      atom_selection_container_t asc = make_asc(mol);
      graphics_info_t::molecules[imol].install_model(imol, asc, name, 1);
      graphics_draw();
   } else {
      std::cout << "WARNING:: bad format, no molecule created"
		<< std::endl;
   } 
   return imol;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
// Return a molecule number, -1 on error.
int add_molecule_py(PyObject *molecule_expression, const char *name) {

   int imol = -1;
   mmdb::Manager *mol =
      mmdb_manager_from_python_expression(molecule_expression);
   if (mol) {
      imol = graphics_info_t::create_molecule();
      atom_selection_container_t asc = make_asc(mol);
      graphics_info_t::molecules[imol].install_model(imol, asc, name, 1);
      graphics_draw();
   } else {
      std::cout << "WARNING:: bad format, no molecule created"
                << std::endl;
   }
   return imol;
}
#endif // USE_PYTHON



// --------------------------------------------------------------
//                 symmetry
// --------------------------------------------------------------

/* for shelx FA pdb files, there is no space group.  So allow the user
   to set it.  This can be initted with a HM symbol or a symm list for
   clipper */
short int set_space_group(int imol, const char *spg) {

   short int r = 0;

   // we should test that this is a clipper string here...
   // does it contain X Y and Z chars?
   // 
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].set_mmdb_symm(spg);
   }

   return r; 
}


// 
void setup_save_symmetry_coords() {

   graphics_info_t::in_save_symmetry_define = 1;
   std::string s = "Now click on a symmetry atom";
   graphics_info_t g;
   g.add_status_bar_text(s);
   pick_cursor_maybe();

}


void save_symmetry_coords(int imol, 
			  const char *filename,
			  int symop_no, 
			  int shift_a, 
			  int shift_b, 
			  int shift_c,
			  int pre_shift_to_origin_na,
			  int pre_shift_to_origin_nb,
			  int pre_shift_to_origin_nc) {

   // Copy the coordinates molecule manager
   // Transform them
   // write them out

   if (imol >= 0) { 
      if (imol < graphics_info_t::n_molecules()) { 
	 if (graphics_info_t::molecules[imol].has_model()) { 
	    mmdb::Manager *mol2 = new mmdb::Manager;
	    mol2->Copy(graphics_info_t::molecules[imol].atom_sel.mol, mmdb::MMDBFCM_All);
	    
	    atom_selection_container_t asc = make_asc(mol2);
	    mmdb::mat44 mat;
	    mmdb::mat44 mat_origin_shift;

	    mol2->GetTMatrix(mat_origin_shift, 0,
			     -pre_shift_to_origin_na,
			     -pre_shift_to_origin_nb,
			     -pre_shift_to_origin_nc);
			     
	    mol2->GetTMatrix(mat, symop_no, shift_a, shift_b, shift_c);

	    clipper::RTop_orth to_origin_rtop(clipper::Mat33<double>(1,0,0,0,1,0,0,0,1),
					      clipper::Coord_orth(mat_origin_shift[0][3],
								  mat_origin_shift[1][3],
								  mat_origin_shift[2][3]));
	    
	    for (int i=0; i<asc.n_selected_atoms; i++) {
	       clipper::Coord_orth co;
	       clipper::Coord_orth trans_pos; 

	       clipper::Mat33<double> clipper_mat(mat[0][0], mat[0][1], mat[0][2],
						  mat[1][0], mat[1][1], mat[1][2],
						  mat[2][0], mat[2][1], mat[2][2]);
	       clipper::Coord_orth  cco(mat[0][3], mat[1][3], mat[2][3]);
	       clipper::RTop_orth rtop(clipper_mat, cco);
	       co = clipper::Coord_orth(asc.atom_selection[i]->x, 
					asc.atom_selection[i]->y, 
					asc.atom_selection[i]->z);
	       trans_pos = co.transform(to_origin_rtop);
	       trans_pos = trans_pos.transform(rtop);
	       asc.atom_selection[i]->x = trans_pos.x();
	       asc.atom_selection[i]->y = trans_pos.y();
	       asc.atom_selection[i]->z = trans_pos.z();
	    } 
	    asc.mol->PDBCleanup(mmdb::PDBCLEAN_SERIAL|mmdb::PDBCLEAN_INDEX);
	    asc.mol->FinishStructEdit();
	    
        mmdb_manager_delete_conect(mol2);
	    int ierr = mol2->WritePDBASCII(filename);
	    if (ierr) {
	       std::cout << "WARNING:: WritePDBASCII to " << filename << " failed." << std::endl;
	       std::string s = "WARNING:: WritePDBASCII to file ";
	       s += filename;
	       s += " failed.";
	       graphics_info_t g;
	       g.add_status_bar_text(s);
	    } else {
	       std::cout << "INFO:: Wrote symmetry atoms to " << filename << "." << std::endl;
	       std::string s = "INFO:: Wrote symmetry atoms to file ";
	       s += filename;
	       s += ".";
	       graphics_info_t g;
	       g.add_status_bar_text(s);
	    }
	    
	    std::vector<std::string> command_strings;
	    command_strings.push_back("save-symmetry-coords");
	    command_strings.push_back(coot::util::int_to_string(imol));
	    command_strings.push_back(single_quote(filename));
	    command_strings.push_back(coot::util::int_to_string(symop_no));
	    command_strings.push_back(coot::util::int_to_string(shift_a));
	    command_strings.push_back(coot::util::int_to_string(shift_b));
	    command_strings.push_back(coot::util::int_to_string(shift_c));
	    command_strings.push_back(coot::util::int_to_string(pre_shift_to_origin_na));
	    command_strings.push_back(coot::util::int_to_string(pre_shift_to_origin_nb));
	    command_strings.push_back(coot::util::int_to_string(pre_shift_to_origin_nc));
	    add_to_history(command_strings);
	 }
      }
   }
}

/*! \brief create a new molecule (molecule number is the return value)
  from imol. 

The rotation/translation matrix components are given in *orthogonal*
coordinates.

Allow a shift of the coordinates to the origin before symmetry
expansion is apllied.

Return -1 on failure. */ 
int new_molecule_by_symmetry(int imol,
			     const char *name_in,
			     double m11, double m12, double m13, 
			     double m21, double m22, double m23, 
			     double m31, double m32, double m33, 
			     double tx, double ty, double tz,
			     int pre_shift_to_origin_na,
			     int pre_shift_to_origin_nb,
			     int pre_shift_to_origin_nc) {
   int istate = -1;
   if (is_valid_model_molecule(imol)) { 
      std::pair<bool, clipper::Cell> cell_info = graphics_info_t::molecules[imol].cell();

      mmdb::Manager *mol_orig = graphics_info_t::molecules[imol].atom_sel.mol;
      // test if returned molecule is non-null
      std::string name = "Symmetry copy of ";
      name += coot::util::int_to_string(imol);
      if (std::string(name_in) != "")
	 name = name_in;
      mmdb::Manager *mol_symm = new_molecule_by_symmetry_matrix_from_molecule(mol_orig,
									     m11, m12, m13,
									     m21, m22, m23,
									     m31, m32, m33,
									     tx, ty, tz,
									     pre_shift_to_origin_na,
									     pre_shift_to_origin_nb,
									     pre_shift_to_origin_nc);

      if (mol_symm) { 
	 int imol_new = graphics_info_t::create_molecule(); 
	 atom_selection_container_t asc = make_asc(mol_symm);
	 graphics_info_t::molecules[imol_new].install_model(imol_new, asc, name, 1);
	 update_go_to_atom_window_on_new_mol();
	 graphics_draw();
	 istate = imol_new;
      } else { 
	 std::cout << "WARNING:: molecule " << imol << " does not have a proper cell " 
		   << std::endl;
      }
   } else { 
	 std::cout << "WARNING:: molecule " << imol << " is not a valid model molecule " 
		   << std::endl;
   }
   return istate; 
} 



int new_molecule_by_symop(int imol, const char *symop_string, 
			  int pre_shift_to_origin_na,
			  int pre_shift_to_origin_nb,
			  int pre_shift_to_origin_nc) {

   int imol_new = -1;
   if (is_valid_model_molecule(imol)) { 
      std::pair<bool, clipper::Cell> cell_info = graphics_info_t::molecules[imol].cell();
      if (cell_info.first) { 
	 coot::symm_card_composition_t sc(symop_string);
	 std::cout << symop_string << " ->\n" 
		   << sc.x_element[0] << " " << sc.y_element[0] << " " << sc.z_element[0] << "\n"
		   << sc.x_element[1] << " " << sc.y_element[1] << " " << sc.z_element[1] << "\n"
		   << sc.x_element[2] << " " << sc.y_element[2] << " " << sc.z_element[2] << "\n"
		   << "translations: "
		   << sc.trans_frac(0) << " "
		   << sc.trans_frac(1) << " "
		   << sc.trans_frac(2) << std::endl;
	 std::cout << "pre-trans: "
		   << pre_shift_to_origin_na << " "
		   << pre_shift_to_origin_nb << " " 
		   << pre_shift_to_origin_nc << std::endl;

	 // those matrix elements are in fractional coordinates.  We want to
	 // pass a components for an rtop_orth.

	 clipper::Mat33<double> mat(sc.x_element[0], sc.y_element[0], sc.z_element[0], 
				    sc.x_element[1], sc.y_element[1], sc.z_element[1], 
				    sc.x_element[2], sc.y_element[2], sc.z_element[2]);

	 clipper::Vec3<double> vec(sc.trans_frac(0),
				   sc.trans_frac(1),
				   sc.trans_frac(2));

	 clipper::RTop_frac rtop_frac(mat, vec);
	 clipper::RTop_orth rtop_orth = rtop_frac.rtop_orth(cell_info.second);
	 clipper::Mat33<double> orth_mat = rtop_orth.rot();
	 clipper::Coord_orth    orth_trn(rtop_orth.trn());

	 std::string new_mol_name = "SymOp_";
	 new_mol_name += symop_string;
	 new_mol_name += "_Copy_of_";
	 new_mol_name += coot::util::int_to_string(imol);

	 imol_new =  new_molecule_by_symmetry(imol,
					      new_mol_name.c_str(),
					      orth_mat(0,0), orth_mat(0,1), orth_mat(0,2), 
					      orth_mat(1,0), orth_mat(1,1), orth_mat(1,2), 
					      orth_mat(2,0), orth_mat(2,1), orth_mat(2,2), 
					      orth_trn.x(), orth_trn.y(), orth_trn.z(),
					      pre_shift_to_origin_na,
					      pre_shift_to_origin_nb,
					      pre_shift_to_origin_nc);
      }
   }
   return imol_new;
}


int
new_molecule_by_symmetry_with_atom_selection(int imol, 
					     const char *name,
					     const char *mmdb_atom_selection_string,
					     double m11, double m12, double m13, 
					     double m21, double m22, double m23, 
					     double m31, double m32, double m33, 
					     double tx, double ty, double tz,
					     int pre_shift_to_origin_na,
					     int pre_shift_to_origin_nb,
					     int pre_shift_to_origin_nc) {

   int imol_new = -1;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol_orig = graphics_info_t::molecules[imol].atom_sel.mol;
      int SelectionHandle = mol_orig->NewSelection();
      mol_orig->Select(SelectionHandle, mmdb::STYPE_ATOM,
		       mmdb_atom_selection_string,
		       mmdb::SKEY_OR);

      mmdb::Manager *mol =
	 coot::util::create_mmdbmanager_from_atom_selection(mol_orig, SelectionHandle);

      mmdb::Manager *new_mol = new_molecule_by_symmetry_matrix_from_molecule(mol,
									    m11, m12, m13,
									    m21, m22, m23,
									    m31, m32, m33,
									    tx, ty, tz,
									    pre_shift_to_origin_na,
									    pre_shift_to_origin_nb,
									    pre_shift_to_origin_nc);

      delete mol; // done with it
      if (new_mol) {
	 imol_new = graphics_info_t::create_molecule();
	 atom_selection_container_t asc = make_asc(new_mol);
	 graphics_info_t::molecules[imol_new].install_model(imol_new, asc, name, 1);
	 update_go_to_atom_window_on_new_mol();
	 graphics_draw();
      }
      mol_orig->DeleteSelection(SelectionHandle);
   }
   return imol_new;
}

mmdb::Manager *new_molecule_by_symmetry_matrix_from_molecule(mmdb::Manager *mol,
							    double m11, double m12, double m13, 
							    double m21, double m22, double m23, 
							    double m31, double m32, double m33, 
							    double tx, double ty, double tz,
							    int pre_shift_to_origin_na,
							    int pre_shift_to_origin_nb,
							    int pre_shift_to_origin_nc) {
   mmdb::Manager *new_mol = 0;

   try {
      std::pair<clipper::Cell, clipper::Spacegroup> cell_info = coot::util::get_cell_symm(mol);
      std::vector<int> pre_shift(3);
      pre_shift[0] = pre_shift_to_origin_na;
      pre_shift[1] = pre_shift_to_origin_nb;
      pre_shift[2] = pre_shift_to_origin_nc;
      clipper::Mat33<double> mat(m11, m12, m13, m21, m22, m23, m31, m32, m33);
      clipper::Vec3<double> vec(tx, ty, tz);
      clipper::RTop_orth rtop_orth(mat, vec);
      clipper::RTop_frac rtop_frac = rtop_orth.rtop_frac(cell_info.first);
      if (0) { 
	 std::cout << "DEBUG:: tx,ty,tz:  " << tx << " " << ty << " " << tz << std::endl;
	 std::cout << "DEBUG:: mol_by_symmetry() passed args:\n" << cell_info.first.format()
		   << std::endl << rtop_frac.format() << "     "
		   << pre_shift_to_origin_na << " "
		   << pre_shift_to_origin_nb << " "
		   << pre_shift_to_origin_nc << " "
		   << std::endl;
      }
      new_mol = coot::mol_by_symmetry(mol, cell_info.first, rtop_frac, pre_shift);
   }
   catch (const std::runtime_error &rte) {
      std::cout << rte.what() << std::endl;
   } 
   return new_mol;
}

/*! \brief return the number of symmetry operators for the given molecule

return -1 on no-symmetry for molecule or inappropriate imol molecule number */
int n_symops(int imol) {

   int r = -1;

   if (is_valid_model_molecule(imol)) {
      std::pair<std::vector<float>, std::string> cs =
	 graphics_info_t::molecules[imol].get_cell_and_symm();
      if (cs.second.length() >0) { 
	 r = graphics_info_t::molecules[imol].atom_sel.mol->GetNumberOfSymOps();
      }
   };

   if (is_valid_map_molecule(imol)) {
      r = graphics_info_t::molecules[imol].xmap.spacegroup().num_symops();
   } 
   return r;
}

/* This function works by active symm atom. */
int move_reference_chain_to_symm_chain_position() {

   graphics_info_t g;
   return g.move_reference_chain_to_symm_chain_position();
} 





#ifdef __cplusplus
#ifdef USE_GUILE
/*! \brief return the pre-shift as a list of fraction or scheme false
  on failure  */
SCM origin_pre_shift_scm(int imol) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      try { 
	 clipper::Coord_frac cf = coot::util::shift_to_origin(mol);
	 r = SCM_EOL;
	 r = scm_cons(SCM_MAKINUM(int(round(cf.w()))), r);
	 r = scm_cons(SCM_MAKINUM(int(round(cf.v()))), r);
	 r = scm_cons(SCM_MAKINUM(int(round(cf.u()))), r);
      }
      catch (std::runtime_error rte) {
	 std::cout << rte.what() << std::endl;
      } 
   } 
   return r;
} 
#endif  /* USE_GUILE */

#ifdef USE_PYTHON
/*! \brief return the pre-shift as a list of fraction or python false
  on failure  */
PyObject *origin_pre_shift_py(int imol) {
   
   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      try { 
	 clipper::Coord_frac cf = coot::util::shift_to_origin(mol);
	 r = PyList_New(0);
	 PyList_Append(r, PyInt_FromLong(int(round(cf.u()))));
	 PyList_Append(r, PyInt_FromLong(int(round(cf.v()))));
	 PyList_Append(r, PyInt_FromLong(int(round(cf.w()))));
      }
      catch (std::runtime_error rte) {
	 std::cout << rte.what() << std::endl;
      } 
   } 
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif  /* USE_PYTHON */

#endif 



/*  ----------------------------------------------------------------------- */
/*                  sequence (assignment)                                   */
/*  ----------------------------------------------------------------------- */
/* section Sequence (Assignment) */

void assign_sequence(int imol_coords, int imol_map, const char *chain_id) {

   if (is_valid_model_molecule(imol_coords))
      if (is_valid_map_molecule(imol_map))
	 graphics_info_t::molecules[imol_coords].assign_sequence(graphics_info_t::molecules[imol_map].xmap, std::string(chain_id));

  std::string cmd = "assign-sequence";
  std::vector<coot::command_arg_t> args;
  args.push_back(imol_coords);
  args.push_back(imol_map);
  args.push_back(single_quote(chain_id));
  add_to_history_typed(cmd, args);
}


/*  ----------------------------------------------------------------------- */
/*                  base mutation                                           */
/*  ----------------------------------------------------------------------- */

void do_base_mutation(const char *type) {

   graphics_info_t g;
   int imol = g.mutate_residue_imol;

   if (is_valid_model_molecule(imol)) {

      // This is dangerous (in a pathological case). Really we should
      // save a residue spec in graphics-info-defines.cc not generate it here.
      // 
      int idx = g.mutate_residue_atom_index;
      mmdb::Atom *at = graphics_info_t::molecules[imol].atom_sel.atom_selection[idx];
      mmdb::Residue *r = at->residue;
      if (r) {
	 std::string cbn = "";
	 if (coot::util::nucleotide_is_DNA(r)) {
	    cbn = coot::util::canonical_base_name(type, coot::DNA);
	 } else {
	    cbn = coot::util::canonical_base_name(type, coot::RNA);
	 } 
	 if (cbn != "") {
	    bool old = g.convert_to_v2_atom_names_flag;
	    coot::residue_spec_t res_spec(r);
	    int istat = graphics_info_t::molecules[imol].mutate_base(res_spec, cbn, old);
	    if (istat)
	       graphics_draw();
	    // Is this the right function?
	    update_go_to_atom_window_on_changed_mol(imol);
	 } else {
	    std::string s = "No canonical base name found";
	    std::cout << "WARNING:: " << s << std::endl;
	    add_status_bar_text(s.c_str());
	 } 
      }
   }
}



void change_chain_id(int imol, const char *from_chain_id, const char *to_chain_id, 
		     short int use_res_range_flag, int from_resno, int to_resno) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::pair<int, std::string> r = 
	 graphics_info_t::molecules[imol].change_chain_id(from_chain_id,
							  to_chain_id,
							  use_res_range_flag,
							  from_resno,
							  to_resno);
      graphics_draw();
      g.update_go_to_atom_window_on_changed_mol(imol);
   }
} 

#ifdef USE_GUILE
SCM change_chain_id_with_result_scm(int imol, const char *from_chain_id, const char *to_chain_id,
				    short int use_res_range_flag, int from_resno, int to_resno){

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      std::pair<int, std::string> p = 
	 g.molecules[imol].change_chain_id(from_chain_id,
					   to_chain_id,
					   use_res_range_flag,
					   from_resno,
					   to_resno);
      graphics_draw();
      g.update_go_to_atom_window_on_changed_mol(imol);
      r = SCM_EOL;
      r = scm_cons(scm_makfrom0str(p.second.c_str()), r);
      r = scm_cons(SCM_MAKINUM(p.first), r);
   }
   return r;
}
#endif // USE_GUILE


#ifdef USE_PYTHON
PyObject *change_chain_id_with_result_py(int imol, const char *from_chain_id, const char *to_chain_id, 
					 short int use_res_range_flag, int from_resno, int to_resno){

   PyObject *v = Py_False;
   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      std::pair<int, std::string> r = 
	 g.molecules[imol].change_chain_id(from_chain_id,
					   to_chain_id,
					   use_res_range_flag,
					   from_resno,
					   to_resno);
   
      graphics_draw();
      g.update_go_to_atom_window_on_changed_mol(imol);
      v = PyList_New(2);
      PyList_SetItem(v, 0, PyInt_FromLong(r.first));
      PyList_SetItem(v, 1, PyString_FromString(r.second.c_str()));
   }
   return v;
}
#endif // USE_PYTHON




/*! \brief set way nomenclature errors should be handled on reading
  coordinates.

  mode should be "auto-correct", "ignore", "prompt".  The
  default is "prompt" */
void set_nomenclature_errors_on_read(const char *mode) {

   std::string m(mode);
   if (m == "auto-correct")
      graphics_info_t::nomenclature_errors_mode = coot::AUTO_CORRECT;
   if (m == "ignore")
      graphics_info_t::nomenclature_errors_mode = coot::IGNORE;
   if (m == "prompt")
      graphics_info_t::nomenclature_errors_mode = coot::PROMPT;

}


/*  ----------------------------------------------------------------------- */
/*               Nomenclature Errors                                        */
/*  ----------------------------------------------------------------------- */
/* return the number of resides altered. */
int fix_nomenclature_errors(int imol) {

   int ifixed = 0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::vector<mmdb::Residue *> vr =
	 graphics_info_t::molecules[imol].fix_nomenclature_errors(g.Geom_p());
      ifixed = vr.size();
      g.update_geometry_graphs(graphics_info_t::molecules[imol].atom_sel,
			       imol);
      graphics_draw();
   }
   // update geometry graphs (not least rotamer graph).
   // but we have no intermediate atoms...
   
   return ifixed; 

}

// the residue type and the spec.
//
std::vector<std::pair<std::string, coot::residue_spec_t> > 
list_nomenclature_errors(int imol) {

   std::vector<std::pair<std::string, coot::residue_spec_t> > r;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      r = g.molecules[imol].list_nomenclature_errors(g.Geom_p());
   }
   return r;
}

#ifdef USE_GUILE
SCM list_nomenclature_errors_scm(int imol) {

   std::vector<std::pair<std::string, coot::residue_spec_t> > v = list_nomenclature_errors(imol);
   SCM r = SCM_EOL;
   if (v.size()) { 
      for(int i=v.size()-1; i>=0; i--) {
	 r = scm_cons(scm_residue(v[i].second), r);
      }
   }
   return r;

} 
#endif // USE_GUILE


#ifdef USE_PYTHON
PyObject *list_nomenclature_errors_py(int imol) {

   PyObject *r = PyList_New(0);
   std::vector<std::pair<std::string, coot::residue_spec_t> > v = list_nomenclature_errors(imol);
   if (v.size()) {
      r = PyList_New(v.size());
      for (unsigned int i=0; i<v.size(); i++) { 
	 PyList_SetItem(r, i, py_residue(v[i].second));
      }
   } 
   return r;
} 
#endif // USE_PYTHON



/*  ----------------------------------------------------------------------- */
/*                  cis <-> trans conversion                                */
/*  ----------------------------------------------------------------------- */
void do_cis_trans_conversion_setup(int istate) {

   if (istate == 1) { 
      graphics_info_t::in_cis_trans_convert_define = 1;
      pick_cursor_maybe(); // depends on ctrl key for rotate
   } else {
      graphics_info_t::in_cis_trans_convert_define = 0;
      normal_cursor(); // depends on ctrl key for rotate
   }
} 

// scriptable interface:
// 
void
cis_trans_convert(int imol, const char *chain_id, int resno, const char *inscode) {

   
   graphics_info_t g;
   if (is_valid_model_molecule(imol)) { 
      g.molecules[imol].cis_trans_conversion(chain_id, resno, inscode);
   }
}


/*  ----------------------------------------------------------------------- */
/*                  180 degree flip                                         */
/*  ----------------------------------------------------------------------- */
/* rotate 180 degrees round the last chi angle */
void do_180_degree_side_chain_flip(int imol, const char* chain_id, int resno, 
			const char *inscode, const char *altconf) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      int istatus =
	 g.molecules[imol].do_180_degree_side_chain_flip(std::string(chain_id),
							 resno,
							 std::string(inscode),
							 std::string(altconf),
							 g.Geom_p());
      std::string s;
      if (istatus == 0) {
	 s = "Problem flipping chi angle on residue ";
	 s += chain_id;
	 s += graphics_info_t::int_to_string(resno);
	 s += ". Not done.";
      } else {
	 s = "Chi angle on residue ";
	 s += chain_id;
	 s += graphics_info_t::int_to_string(resno);
	 s += " successfully flipped.";
	 graphics_draw();
      }
      g.add_status_bar_text(s);
   }
}

// graphics click stuff
void setup_180_degree_flip(short int state) {

   graphics_info_t g;
   graphics_info_t::in_180_degree_flip_define = state;
   if (state) {
      g.in_180_degree_flip_define = 1;
      std::cout << "Click on a residue that you want to flip" << std::endl;
      g.pick_cursor_maybe();
      g.add_status_bar_text("Click on an atom in the residue that you want to flip");
      g.pick_pending_flag = 1;
   } else {
      g.normal_cursor();
      g.pick_pending_flag = 0;
   }
}

/*  ----------------------------------------------------------------------- */
/*                  reverse direction                                       */
/*  ----------------------------------------------------------------------- */
void
setup_reverse_direction(short int istate) {

   graphics_info_t::in_reverse_direction_define = istate;
   graphics_info_t g;
   if (istate == 1) {
      g.pick_cursor_maybe();
      g.add_status_bar_text("Click on an atom in the fragment that you want to reverse");
      g.pick_pending_flag = 1;
   } else {
      g.normal_cursor();
   }

}



/*  ----------------------------------------------------------------------- */
/*                  De-chainsaw                                             */
/*  ----------------------------------------------------------------------- */
// Fill amino acid residues
void fill_partial_residues(int imol) {

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      int imol_map = g.Imol_Refinement_Map();
      coot::util::missing_atom_info m_i_info =
	 g.molecules[imol].fill_partial_residues(g.Geom_p(), imol_map);
      graphics_draw();

      if (imol_map > -1) { 
	 int refinement_replacement_state = refinement_immediate_replacement_state();
	 set_refinement_immediate_replacement(1);
      	 for (unsigned int i=0; i<m_i_info.residues_with_missing_atoms.size(); i++) {
      	    int resno =  m_i_info.residues_with_missing_atoms[i]->GetSeqNum();
      	    std::string chain_id = m_i_info.residues_with_missing_atoms[i]->GetChainID();
      	    std::string residue_type = m_i_info.residues_with_missing_atoms[i]->GetResName();
      	    std::string inscode = m_i_info.residues_with_missing_atoms[i]->GetInsCode();
      	    std::string altconf("");
	    short int is_water = 0;
	    // hmmm backups are being done....
      	    g.refine_residue_range(imol, chain_id, chain_id, resno, inscode, resno, inscode,
				   altconf, is_water);
	    accept_regularizement();
      	 }
	 set_refinement_immediate_replacement(refinement_replacement_state);
      } else {
	 g.show_select_map_dialog();
      } 
   }
}

void fill_partial_residue(int imol, const char *chain_id, int resno, const char* inscode) {

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      int imol_map = g.Imol_Refinement_Map();
      if (imol_map > -1) { 
	 coot::residue_spec_t rs(chain_id, resno, inscode);
	 g.molecules[imol].fill_partial_residue(rs, g.Geom_p(), imol_map);
	 // post process...
	 int refinement_replacement_state = refinement_immediate_replacement_state();
	 set_refinement_immediate_replacement(1);
	 std::string altconf("");
	 short int is_water = 0;
	 // hmmm backups are being done....
	 g.refine_residue_range(imol, chain_id, chain_id, resno, inscode, resno, inscode,
				altconf, is_water);
	 accept_regularizement();
	 set_refinement_immediate_replacement(refinement_replacement_state);

      } else {
	 g.show_select_map_dialog();
      }
   }
}

#ifdef USE_GUILE

SCM missing_atom_info_scm(int imol) { 

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      r = SCM_EOL;
      graphics_info_t g;
      short int missing_hydrogens_flag = 0;
      coot::util::missing_atom_info m_i_info =
	 g.molecules[imol].missing_atoms(missing_hydrogens_flag, g.Geom_p());
      for (unsigned int i=0; i<m_i_info.residues_with_missing_atoms.size(); i++) {
	 mmdb::Residue *residue_p = m_i_info.residues_with_missing_atoms[i];
	 int resno                = residue_p->GetSeqNum();
	 std::string chain_id     = residue_p->GetChainID();
	 std::string residue_type = residue_p->GetResName();
	 std::string inscode      = residue_p->GetInsCode();
	 std::string altconf("");
	 SCM l = SCM_EOL;
	 l = scm_cons(scm_makfrom0str(inscode.c_str()), l);
	 l = scm_cons(SCM_MAKINUM(resno), l);
	 l = scm_cons(scm_makfrom0str(chain_id.c_str()), l);
	 r = scm_cons(l, r);
      }
      r = scm_reverse(r);
   }
   return r;
}
#endif // USE_GUILE

#ifdef USE_PYTHON

PyObject *missing_atom_info_py(int imol) { 

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      r = PyList_New(0);
      graphics_info_t g;
      short int missing_hydrogens_flag = 0;
      coot::util::missing_atom_info m_i_info =
	 g.molecules[imol].missing_atoms(missing_hydrogens_flag, g.Geom_p());
      for (unsigned int i=0; i<m_i_info.residues_with_missing_atoms.size(); i++) {
	 int resno =  m_i_info.residues_with_missing_atoms[i]->GetSeqNum();
	 std::string chain_id = m_i_info.residues_with_missing_atoms[i]->GetChainID();
	 std::string residue_type = m_i_info.residues_with_missing_atoms[i]->GetResName();
	 std::string inscode = m_i_info.residues_with_missing_atoms[i]->GetInsCode();
	 std::string altconf("");
	 PyObject *l = PyList_New(0);
	 PyList_Append(l, PyString_FromString(chain_id.c_str()));
	 PyList_Append(l, PyInt_FromLong(resno));
	 PyList_Append(l, PyString_FromString(inscode.c_str()));
	 PyList_Append(r, l);
	 Py_XDECREF(l);
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // USE_PYTHON

void
copy_chain(int imol, const char *from_chain, const char *to_chain) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].copy_chain(std::string(from_chain),
						  std::string(to_chain));
      graphics_draw();
   }
} 


void copy_from_ncs_master_to_others(int imol, const char *chain_id) {

   if (is_valid_model_molecule(imol)) {
      std::string c(chain_id);
      graphics_info_t::molecules[imol].copy_from_ncs_master_to_others(c);
      graphics_draw();
   }
}



void copy_residue_range_from_ncs_master_to_others(int imol, 
			const char *master_chain_id,
			int res_range_start,
			int res_range_end) {

   if (!is_valid_model_molecule(imol)) {
      std::cout << " molecule " << imol << " is not a valid model molecule"
		<< std::endl;
   } else { 
      std::string c(master_chain_id);
      graphics_info_t::molecules[imol].copy_residue_range_from_ncs_master_to_others(c,
										    res_range_start,
										    res_range_end);
      graphics_draw();
   }
}

#ifdef USE_GUILE
SCM ncs_master_chains_scm(int imol) {
   SCM r = SCM_BOOL_F;

   if (is_valid_model_molecule(imol)) {
      std::vector<std::string> v = graphics_info_t::molecules[imol].ncs_master_chains();
      if (v.size()) {
	 r = generic_string_vector_to_list_internal(v);
      } 
   } 
   return r;
}
#endif

#ifdef USE_PYTHON
PyObject *ncs_master_chains_py(int imol) {

   PyObject *r = Py_False;

   if (is_valid_model_molecule(imol)) {
      std::vector<std::string> v = graphics_info_t::molecules[imol].ncs_master_chains();
      if (v.size()) {
	 r = generic_string_vector_to_list_internal_py(v);
      }
   } 

   if (PyBool_Check(r)) {
      Py_INCREF(r);
   }
   return r;

}
#endif


#ifdef USE_GUILE
void copy_residue_range_from_ncs_master_to_chains_scm(int imol, const char *master_chain_id, 
						      int residue_range_start, int residue_range_end, 
						      SCM chain_id_list_in) {

   if (is_valid_model_molecule(imol)) {
      std::string c(master_chain_id);
      std::vector<std::string> chain_id_list = generic_list_to_string_vector_internal(chain_id_list_in);
      graphics_info_t::molecules[imol].copy_residue_range_from_ncs_master_to_chains(c,
										    residue_range_start,
										    residue_range_end,
										    chain_id_list);
      graphics_draw();
   }
}

void copy_from_ncs_master_to_chains_scm(int imol, const char *master_chain_id, 
                                        SCM chain_id_list_in) {

   if (is_valid_model_molecule(imol)) {
      std::string c(master_chain_id);
      std::vector<std::string> chain_id_list = generic_list_to_string_vector_internal(chain_id_list_in);
      graphics_info_t::molecules[imol].copy_from_ncs_master_to_chains(c,
										    chain_id_list);
      graphics_draw();
   }
} 
#endif 
#ifdef USE_PYTHON
void copy_residue_range_from_ncs_master_to_chains_py(int imol, const char *master_chain_id, 
						     int residue_range_start, int residue_range_end,
						     PyObject *chain_id_list_in) {
   
   if (is_valid_model_molecule(imol)) {
      std::string c(master_chain_id);
      std::vector<std::string> chain_id_list = generic_list_to_string_vector_internal_py(chain_id_list_in);
      graphics_info_t::molecules[imol].copy_residue_range_from_ncs_master_to_chains(c,
										    residue_range_start,
										    residue_range_end,
										    chain_id_list);
      graphics_draw();
   }
}

void copy_from_ncs_master_to_chains_py(int imol, const char *master_chain_id, 
                                       PyObject *chain_id_list_in) {
   
   if (is_valid_model_molecule(imol)) {
      std::string c(master_chain_id);
      std::vector<std::string> chain_id_list = generic_list_to_string_vector_internal_py(chain_id_list_in);
      graphics_info_t::molecules[imol].copy_from_ncs_master_to_chains(c,
										    chain_id_list);
      graphics_draw();
   }
} 
#endif 

void set_place_helix_here_fudge_factor(float ff) {
   graphics_info_t::place_helix_here_fudge_factor = ff;
} 



/*  ----------------------------------------------------------------------- */
/*                  Helices                                                 */
/*  ----------------------------------------------------------------------- */
/* Add a helix somewhere close to this point in the map, try to fit
   the orientation. Add to a molecule called "Helices", create it if needed.*/
int place_helix_here() {

   graphics_info_t g;
   clipper::Coord_orth pt(g.RotationCentre_x(),
			  g.RotationCentre_y(),
			  g.RotationCentre_z());
   int imol_map = g.Imol_Refinement_Map();
   if (imol_map != -1) {
      // perhaps we should add to the constructor the user-setable
      // g.helix_placement_cb_density_descrimination_ratio
      // defaults to 1.0.
      int imol = -1;
      const clipper::Xmap<float> &xmap = graphics_info_t::molecules[imol_map].xmap;
      coot::helix_placement p(xmap);

      // test
      std::vector<float> dv_test(100);
      for (unsigned int i=0; i<100; i++) { 
	 dv_test[i] = 100-i;
      }

      std::vector<float> dv = coot::util::density_map_points_in_sphere(pt, 20, xmap);
      float f_iqr = coot::util::interquartile_range(dv);
      float iqr_2 = 0.5*f_iqr;
	 
      float min_density_limit = iqr_2;
      float high_density_turning_point = f_iqr * 4 * g.place_helix_here_fudge_factor;
      std::cout << "DEBUG:: choosing map min_density limit: " << min_density_limit << std::endl;
      std::cout << "DEBUG:: choosing map high_density_turning_point limit: " << high_density_turning_point
		<< std::endl;
      
      float bf = g.default_new_atoms_b_factor;
      coot::helix_placement_info_t n =
	 p.place_alpha_helix_near_kc_version(pt, 20, min_density_limit, high_density_turning_point, bf);
       if (! n.success) {
	  n = p.place_alpha_helix_near_kc_version(pt, 9, min_density_limit, high_density_turning_point, bf);
       }

       if (n.success) {
	  atom_selection_container_t asc = make_asc(n.mol[0].pcmmdbmanager());
	  imol = g.create_molecule();
	  graphics_info_t::molecules[imol].install_model(imol, asc, "Helix", 1);
	  
	  if (n.mol.size() > 1) { 
	     atom_selection_container_t asc2 = make_asc(n.mol[1].pcmmdbmanager());
	     imol = g.create_molecule();
	     graphics_info_t::molecules[imol].install_model(imol, asc2, "Reverse Helix", 1);
	  }
	  
	  if (g.go_to_atom_window) {
	     g.set_go_to_atom_molecule(imol);
	     g.update_go_to_atom_window_on_new_mol();
	  } else {
	     g.set_go_to_atom_molecule(imol);
	  }
	  g.add_status_bar_text("Helix added");
       } else {
	  std::cout << "Helix addition failure: message: " << n.failure_message << "\n";
	  g.add_status_bar_text(n.failure_message);
       }
       std::vector<std::string> command_strings;
       command_strings.push_back("set-rotation-centre");
       command_strings.push_back(coot::util::float_to_string(g.RotationCentre_x()));
       command_strings.push_back(coot::util::float_to_string(g.RotationCentre_y()));
       command_strings.push_back(coot::util::float_to_string(g.RotationCentre_z()));
       add_to_history(command_strings);
       
       command_strings.resize(0);
       command_strings.push_back("place-helix-here");
       add_to_history(command_strings);
       graphics_draw();
       return imol;
   } else {
      std::cout << " You need to set the map to fit against\n";
      g.add_status_bar_text("You need to set the map to fit against");
      g.show_select_map_dialog();
      return -1;
   }
} 


/*  ----------------------------------------------------------------------- */
/*                  Place a strand                                          */
/*  ----------------------------------------------------------------------- */
int place_strand_here(int n_residues, int n_sample_strands) {

   int imol = -1; // failure status 
   graphics_info_t g;
   clipper::Coord_orth pt(g.RotationCentre_x(),
			  g.RotationCentre_y(),
			  g.RotationCentre_z());
   int imol_map = g.Imol_Refinement_Map();
   if (imol_map != -1) {

      float s = graphics_info_t::molecules[imol_map].map_sigma();
      coot::helix_placement p(graphics_info_t::molecules[imol_map].xmap);
      coot::helix_placement_info_t si =
	 p.place_strand(pt, n_residues, n_sample_strands, s * graphics_info_t::place_helix_here_fudge_factor);
      if (si.success) {
	 // nice to refine the fragment here, but the interface
	 // doesn't work that way, so put the refinement after the
	 // molecule has been accepted.
	 atom_selection_container_t asc = make_asc(si.mol[0].pcmmdbmanager());
	 imol = g.create_molecule();
	 graphics_info_t::molecules[imol].install_model(imol, asc, "Strand", 1);
	 g.add_status_bar_text("Strand added");

	 // Now refine.
	 coot::minimol::zone_info_t zi = si.mol[0].zone_info();
	 if (zi.is_simple_zone) {
	    graphics_info_t g;
	    int save_rirf = g.refinement_immediate_replacement_flag;
#ifdef HAVE_GSL	    
	    coot::pseudo_restraint_bond_type save_pseudos = g.pseudo_bonds_type;
	    g.pseudo_bonds_type = coot::STRAND_PSEUDO_BONDS;
	    g.refinement_immediate_replacement_flag = 1;
	    g.refine_residue_range(imol, zi.chain_id, zi.chain_id, zi.resno_1, "",
				   zi.resno_2, "", "", 0);
	    accept_regularizement();
	    g.pseudo_bonds_type = save_pseudos;
#endif // HAVE_GSL	    
	    g.refinement_immediate_replacement_flag = save_rirf;
	 } 
      } else {
	 std::cout << "Strand addition failure: message: " << si.failure_message << "\n";
	 g.add_status_bar_text(si.failure_message);
      }
      if (g.go_to_atom_window) {
	 g.set_go_to_atom_molecule(imol);
	 g.update_go_to_atom_window_on_new_mol();
      }
      
      std::vector<std::string> command_strings;
      command_strings.push_back("set-rotation-centre");
      command_strings.push_back(coot::util::float_to_string(g.RotationCentre_x()));
      command_strings.push_back(coot::util::float_to_string(g.RotationCentre_y()));
      command_strings.push_back(coot::util::float_to_string(g.RotationCentre_z()));
      add_to_history(command_strings);
      
      command_strings.resize(0);
      command_strings.push_back("place-strand-here");
      command_strings.push_back(coot::util::int_to_string(n_residues));
      command_strings.push_back(coot::util::int_to_string(n_sample_strands));
      add_to_history(command_strings);
      graphics_draw();
      return imol;
   } else {
      std::cout << " You need to set the map to fit against\n";
      g.add_status_bar_text("You need to set the map to fit against");
      g.show_select_map_dialog();
      return -1;
   }
} 


/*  ----------------------------------------------------------------------- */
/*                  Autofind Helices                                        */
/*  ----------------------------------------------------------------------- */
/* Find secondary structure in the current map.
   Add to a molecule called "Helices", create it if
   needed. */
int find_helices() {
  return find_secondary_structure_local( 1, 7, FIND_SECSTRUC_NORMAL,
					 0, 0, FIND_SECSTRUC_NORMAL, 0.0 );
} 


/*  ----------------------------------------------------------------------- */
/*                  Autofind Strands                                        */
/*  ----------------------------------------------------------------------- */
/* Find secondary structure in the current map.
   Add to a molecule called "Strands", create it if
   needed. */
int find_strands() {
  return find_secondary_structure_local( 0, 0, FIND_SECSTRUC_STRICT,
					 1, 5, FIND_SECSTRUC_STRICT, 0.0 );
}


/*  ----------------------------------------------------------------------- */
/*                  Autofind Secondary Structure                            */
/*  ----------------------------------------------------------------------- */
/* Find secondary structure in the current map.
   Add to a molecule called "SecStruc", create it if
   needed. */
int find_secondary_structure(
    short int use_helix,  int helix_length,  int helix_target,
    short int use_strand, int strand_length, int strand_target )
{
  return find_secondary_structure_local(
      use_helix,  helix_length,  helix_target,
      use_strand, strand_length, strand_target,
      0.0 );
}

/*  ----------------------------------------------------------------------- */
/*                  Autofind Secondary Structure                            */
/*  ----------------------------------------------------------------------- */
/* Find secondary structure in the current map.
   Add to a molecule called "SecStruc", create it if
   needed. */
int find_secondary_structure_local(
    short int use_helix,  int helix_length,  int helix_target,
    short int use_strand, int strand_length, int strand_target,
    float radius )
{
   graphics_info_t g;
   clipper::Coord_orth pt(g.RotationCentre_x(),
			  g.RotationCentre_y(),
			  g.RotationCentre_z());
   int imol_map = g.Imol_Refinement_Map();
   if (imol_map != -1) {
      using coot::SSfind;
      coot::SSfind::SSTYPE ta[] = { SSfind::ALPHA3, SSfind::ALPHA3S,
				    SSfind::ALPHA2, SSfind::ALPHA4 };
      coot::SSfind::SSTYPE tb[] = { SSfind::BETA3,  SSfind::BETA3S,
				    SSfind::BETA2,  SSfind::BETA4  };
      int imol = -1;
      std::vector<SSfind::Target> tgtvec;
      if ( use_helix )
	tgtvec.push_back( SSfind::Target(ta[helix_target %4],helix_length ) );
      if ( use_strand )
	tgtvec.push_back( SSfind::Target(tb[strand_target%4],strand_length) );
      coot::fast_secondary_structure_search ssfind;
      ssfind( graphics_info_t::molecules[imol_map].xmap, pt, 0.0, tgtvec );
      if (ssfind.success) {
	 atom_selection_container_t asc = make_asc(ssfind.mol.pcmmdbmanager());
	 imol = g.create_molecule();
	 graphics_info_t::molecules[imol].install_model(imol,asc,"SecStruc",1);
	 g.molecules[imol].ca_representation();
	 if (g.go_to_atom_window) {
	    g.set_go_to_atom_molecule(imol);
	    g.update_go_to_atom_window_on_new_mol();
	 } else {
	    g.set_go_to_atom_molecule(imol);
	 }
	 g.add_status_bar_text("Secondary structure added");
      } else {
	 std::cout << "No secondary structure found\n";
	 g.add_status_bar_text("No secondary structure found" );
      }
      std::vector<std::string> command_strings;
      command_strings.resize(0);
      command_strings.push_back("find-secondary-structure");
      add_to_history(command_strings);
      graphics_draw();
      return imol;
   } else {
      std::cout << " You need to set the map to fit against\n";
      g.add_status_bar_text("You need to set the map to fit against");
      g.show_select_map_dialog();
      return -1;
   }
}


/*  ----------------------------------------------------------------------- */
/*                  Autofind Nucleic acid chains                            */
/*  ----------------------------------------------------------------------- */
/* Find secondary structure local to current view in the current map.
   Add to a molecule called "NuclAcid", create it if needed. */
int find_nucleic_acids_local( float radius )
{
   // check for data file
   std::string nafile;
   const char *cp = getenv("COOT_PREFIX");
   if (cp) nafile = std::string(cp) + "/share/coot/nautilus_lib.pdb";
   else    nafile = std::string(PKGDATADIR) + "/nautilus_lib.pdb";
   if (!coot::file_exists(nafile)) {
      std::cout << "Ooops! Can't find nautilus data! - fail" << std::endl;
      return -1;
   }

   // check for map
   graphics_info_t g;
   clipper::Coord_orth pt(g.RotationCentre_x(),
			  g.RotationCentre_y(),
			  g.RotationCentre_z());
   int imol_map = g.Imol_Refinement_Map();
   if (imol_map == -1) {
      std::cout << " You need to set the map to fit against\n";
      g.add_status_bar_text("You need to set the map to fit against");
      g.show_select_map_dialog();
      return -1;
   }

   // FIND OR CREATE A MODEL FOR THE NAs
   int imol = -1;
   mmdb::Manager *mol;
   for ( int i = 0 ; i < graphics_n_molecules(); i++)
     if ( graphics_info_t::molecules[i].has_model() &&
	  graphics_info_t::molecules[i].name_ == "NuclAcid" ) {
       imol = i;
       mol = graphics_info_t::molecules[i].atom_sel.mol;
       break;
     }
   if ( imol < 0 ) {
     imol = g.create_molecule();
     mol = new mmdb::Manager;
     graphics_info_t::molecules[imol].install_model( imol, mol, "NuclAcid", 1 );
   }

   // build the model
   Coot_nucleic_acid_build nafind( nafile );
   bool success = nafind.build( mol, graphics_info_t::molecules[imol_map].xmap, pt, radius );
   graphics_info_t::molecules[imol].update_molecule_after_additions();

   if (success) {
   	 if (g.go_to_atom_window) {
   	    g.set_go_to_atom_molecule(imol);
   	    g.update_go_to_atom_window_on_new_mol();
   	 } else {
   	    g.set_go_to_atom_molecule(imol);
   	 }
   	 std::cout << "Nucleic acids found\n";
   	 g.add_status_bar_text("Nucleic acids added");
   } else {
   	 std::cout << "No nucleic acids found\n";
   	 g.add_status_bar_text("No nucleic acids found" );
   }
   std::vector<std::string> command_strings;
   command_strings.resize(0);
   command_strings.push_back("find-nucleic-acids-local");
   add_to_history(command_strings);
   graphics_draw();
   return imol;
}


/*  ----------------------------------------------------------------------- */
/*                  FFfearing                                               */
/*  ----------------------------------------------------------------------- */


// return the new model number
// 
int fffear_search(int imol_model, int imol_map) {

   float angular_resolution = graphics_info_t::fffear_angular_resolution;
   int imol_new = -1;
   if (!is_valid_model_molecule(imol_model)) {
      std::cout << "WARNING:: this is not a valid model: " << imol_model << std::endl;
   } else { 
      if (is_valid_map_molecule(imol_map)) {
	 std::cout << "WARNING:: this is not a valid map: " << imol_model << std::endl;
      } else {
	 coot::util::fffear_search f(graphics_info_t::molecules[imol_model].atom_sel.mol,
				     graphics_info_t::molecules[imol_model].atom_sel.SelectionHandle,
				     graphics_info_t::molecules[imol_map].xmap,
				     angular_resolution);

	 imol_new = graphics_info_t::create_molecule();
	 std::string name("FFFear search results");
	 graphics_info_t::molecules[imol_new].new_map(f.get_results_map(), name);

	 std::vector<std::pair<float, clipper::RTop_orth> > p = f.scored_orientations();
	 if (p.size() > 0) {
	    // install new molecule(s) that has been rotated.
	 }
      }
   }
   return imol_new;
}


void set_fffear_angular_resolution(float f) {

   graphics_info_t::fffear_angular_resolution = f; 
}

float fffear_angular_resolution() {

   return graphics_info_t::fffear_angular_resolution; 
}




/*  ----------------------------------------------------------------------- */
/*                  rigid body refinement                                   */
/*  ----------------------------------------------------------------------- */

void do_rigid_body_refine(short int state){

   graphics_info_t g;
   
   g.set_in_rigid_body_refine(state);
   if (state) { 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;
      std::cout << "click on 2 atoms to define a range of residue "
		<< "to rigid body refine: " << std::endl;
   } else {
      g.normal_cursor();
   }
}

void execute_rigid_body_refine(short int auto_range_flag){
   graphics_info_t g;
   g.execute_rigid_body_refine(auto_range_flag);
}

void rigid_body_refine_zone(int resno_start, int resno_end, 
			    const char *chain_id, int imol) {

   graphics_info_t g;
   std::string altconf = ""; // should be passed?
   
   // need to set graphics_info's residue_range_atom_index_1,
   // residue_range_atom_index_2, imol_rigid_body_refine

   if (imol < g.n_molecules()) {
      if (g.molecules[imol].has_model()) { 
	 g.imol_rigid_body_refine = imol;

	 g.set_residue_range_refine_atoms(std::string(chain_id),
					  resno_start, resno_end,
					  altconf,
					  imol);
	 g.execute_rigid_body_refine(0);
      }
   }
}


void
rigid_body_refine_by_atom_selection(int imol, 
				    const char *atom_selection_string) {


   graphics_info_t g;
   int imol_ref_map = g.Imol_Refinement_Map();
   if (is_valid_map_molecule(imol_ref_map)) {
      if (is_valid_model_molecule(imol)) { 
	 bool mask_waters_flag = 0;

	 // so the bulk of this function is to generate
	 // mol_without_moving_zone and range_mol from
	 // atom_selection_string.
	 //
	 // Let's make a (ligand) utility function for this.
	 //
	 bool fill_mask = 1;
	 mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
	 std::string atom_selection_str(atom_selection_string);
	 std::pair<coot::minimol::molecule, coot::minimol::molecule> p = 
	    coot::make_mols_from_atom_selection_string(mol, atom_selection_str, fill_mask);

	 g.imol_rigid_body_refine = imol;
	 g.rigid_body_fit(p.first,   // without selected region.
			  p.second,  // selected region.
			  imol_ref_map,
			  mask_waters_flag);
      } else {
	 std::cout << "WARNING:: model molecule " << imol << " is not valid " << std::endl;
      } 
   } else {
      std::cout << "WARNING:: refinement map not defined. " << std::endl;
   } 
}


#ifdef USE_GUILE
/* ! \brief rigid body refine using residue ranges.  residue_ranges is
   a list of residue ranges.  A residue range is (list chain-id
   resno-start resno-end). */
SCM
rigid_body_refine_by_residue_ranges_scm(int imol, SCM residue_ranges) {

   SCM ret_val = SCM_BOOL_F;
   std::vector<coot::residue_range_t> res_ranges;
   if (scm_is_true(scm_list_p(residue_ranges))) { 
      SCM rr_length_scm = scm_length(residue_ranges);
      int rr_length = scm_to_int(rr_length_scm);
      if (rr_length > 0) {
	 for (int irange=0; irange<rr_length; irange++) {
	    SCM range_scm = scm_list_ref(residue_ranges, SCM_MAKINUM(irange));
	    if (scm_is_true(scm_list_p(range_scm))) {
	       SCM range_length_scm = scm_length(range_scm);
	       int range_length = scm_to_int(range_length_scm);
	       if (range_length == 3) {
		  SCM chain_id_scm    = scm_list_ref(range_scm, SCM_MAKINUM(0));
		  SCM resno_start_scm = scm_list_ref(range_scm, SCM_MAKINUM(1));
		  SCM resno_end_scm   = scm_list_ref(range_scm, SCM_MAKINUM(2));
		  if (scm_is_string(chain_id_scm)) {
		     std::string chain_id = scm_to_locale_string(chain_id_scm);
		     if (scm_is_true(scm_number_p(resno_start_scm))) {
			int resno_start = scm_to_int(resno_start_scm);
			if (scm_is_true(scm_number_p(resno_end_scm))) {
			   int resno_end = scm_to_int(resno_end_scm);
			   // recall that mmdb does crazy things with
			   // the residue selection if the second
			   // residue is before the first residue in
			   // the chain.  So, check and swap if needed.
			   if (resno_end < resno_start)
			      std::swap<int>(resno_start, resno_end);
			   coot::residue_range_t rr(chain_id, resno_start, resno_end);
			   res_ranges.push_back(rr);
			}
		     }
		  } 
	       } 
	    } 
	 }
	 int status = rigid_body_fit_with_residue_ranges(imol, res_ranges); // test for res_ranges
	                                                                    // length in here.
	 if (status == 1) // good
	    ret_val = SCM_BOOL_T;
      } else {
	 std::cout << "incomprehensible input to rigid_body_refine_by_residue_ranges_scm"
		   << " null list" << std::endl;
      } 
   } else {
      std::cout << "incomprehensible input to rigid_body_refine_by_residue_ranges_scm"
		<< " not a list" << std::endl;
   }
   return ret_val;
}
#endif /* USE_GUILE */

#ifdef USE_PYTHON
/* ! \brief rigid body refine using residue ranges.  residue_ranges is
   a list of residue ranges.  A residue range is [chain-id,
   resno-start, resno-end]. */
PyObject *
rigid_body_refine_by_residue_ranges_py(int imol, PyObject *residue_ranges) {

   PyObject *ret_val = Py_False;
   std::vector<coot::residue_range_t> res_ranges;
   if (PyList_Check(residue_ranges)) { 
      int rr_length = PyObject_Length(residue_ranges);
      if (rr_length > 0) {
	 for (int irange=0; irange<rr_length; irange++) {
	   PyObject *range_py = PyList_GetItem(residue_ranges, irange);
	   if (PyList_Check(range_py)) {
	     int range_length = PyObject_Length(range_py);
	     if (range_length == 3) {
	       PyObject *chain_id_py    = PyList_GetItem(range_py, 0);
	       PyObject *resno_start_py = PyList_GetItem(range_py, 1);
	       PyObject *resno_end_py   = PyList_GetItem(range_py, 2);
	       if (PyString_Check(chain_id_py)) {
		 std::string chain_id = PyString_AsString(chain_id_py);
		 if (PyInt_Check(resno_start_py)) {
		   int resno_start = PyInt_AsLong(resno_start_py);
		   if (PyInt_Check(resno_end_py)) {
		     int resno_end = PyInt_AsLong(resno_end_py);
		     // recall that mmdb does crazy things with
		     // the residue selection if the second
		     // residue is before the first residue in
		     // the chain.  So, check and swap if needed.
		     if (resno_end < resno_start)
		       std::swap<int>(resno_start, resno_end);
		     coot::residue_range_t rr(chain_id, resno_start, resno_end);
		     res_ranges.push_back(rr);
		   }
		 }
	       } 
	     } 
	   } 
	 }
	 int status = rigid_body_fit_with_residue_ranges(imol, res_ranges); // test for res_ranges
	                                                                    // length in here.
	 if (status == 1) // good
	   ret_val = Py_True;
      } else {
	std::cout << "incomprehensible input to rigid_body_refine_by_residue_ranges_scm"
		  << " null list" << std::endl;
      } 
   } else {
     std::cout << "incomprehensible input to rigid_body_refine_by_residue_ranges_scm"
	       << " not a list" << std::endl;
   }
   if (PyBool_Check(ret_val)) {
     Py_INCREF(ret_val);
   }
   return ret_val;
}
#endif /* USE_PYTHON */


/*  ----------------------------------------------------------------------- */
/*                  rigid body fitting (multiple residue ranges)            */
/*  ----------------------------------------------------------------------- */
int rigid_body_fit_with_residue_ranges(int imol,
					const std::vector<coot::residue_range_t> &residue_ranges) {

   int success = 0;
   graphics_info_t g;
   int imol_ref_map = g.Imol_Refinement_Map();
   if (is_valid_map_molecule(imol_ref_map)) {
      if (is_valid_model_molecule(imol)) {

	 if (residue_ranges.size()) { 
	    bool mask_waters_flag = 0;

	    mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
	    int SelHnd = mol->NewSelection();

	    for (unsigned int ir=0; ir<residue_ranges.size(); ir++) {
	       mol->SelectAtoms(SelHnd, 0,
				residue_ranges[ir].chain_id.c_str(),
				residue_ranges[ir].start_resno, "*",
				residue_ranges[ir].end_resno, "*",
				"*","*","*","*", mmdb::SKEY_OR);
	    }

	    mmdb::Manager *mol_from_selected =
	       coot::util::create_mmdbmanager_from_atom_selection(mol, SelHnd);
	    // atom selection in mol gets inverted by this function:
	    mmdb::Manager *mol_from_non_selected =
	       coot::util::create_mmdbmanager_from_atom_selection(mol, SelHnd, 1);
	 
	    coot::minimol::molecule range_mol  = coot::minimol::molecule(mol_from_selected);
	    coot::minimol::molecule masked_mol = coot::minimol::molecule(mol_from_non_selected);
	    delete mol_from_selected;
	    delete mol_from_non_selected;
	    mol->DeleteSelection(SelHnd);
	    g.imol_rigid_body_refine = imol;
	    success = g.rigid_body_fit(masked_mol,   // without selected region.
				       range_mol,  // selected region.
				       imol_ref_map,
				       mask_waters_flag);
	 }
      }
   }
   return success;
}

int morph_fit_all(int imol, float transformation_averaging_radius) {

   int success = 0;
   graphics_info_t g;
   int imol_ref_map = g.Imol_Refinement_Map();
   if (is_valid_map_molecule(imol_ref_map)) {
      if (is_valid_model_molecule(imol)) {
	 success = g.molecules[imol].morph_fit_all(g.molecules[imol_ref_map].xmap,
						   transformation_averaging_radius);
	 graphics_draw();
      }
   }
   return success;
}

int morph_fit_chain(int imol, std::string chain_id, float transformation_averaging_radius) {

   int success = 0;
   graphics_info_t g;
   int imol_ref_map = g.Imol_Refinement_Map();
   if (is_valid_map_molecule(imol_ref_map)) {
      if (is_valid_model_molecule(imol)) {
	 success = g.molecules[imol].morph_fit_chain(chain_id,
						     g.molecules[imol_ref_map].xmap,
						     transformation_averaging_radius);
	 graphics_draw();
      }
   }
   return success;
} 


#ifdef USE_GUILE
int morph_fit_residues_scm(int imol, SCM residue_specs_scm, float transformation_averaging_radius) {

   std::vector<coot::residue_spec_t> residue_specs = scm_to_residue_specs(residue_specs_scm);
   return morph_fit_residues(imol, residue_specs, transformation_averaging_radius);
}
#endif // USE_GUILE

#ifdef USE_PYTHON
int morph_fit_residues_py( int imol, PyObject *residue_specs_py, float transformation_averaging_radius) {

   std::vector<coot::residue_spec_t> residue_specs = py_to_residue_specs(residue_specs_py);
   return morph_fit_residues(imol, residue_specs, transformation_averaging_radius);
} 
#endif // USE_PYTHON

int morph_fit_residues(int imol, const std::vector<coot::residue_spec_t> &residue_specs,
		       float transformation_averaging_radius) {
   
   graphics_info_t g;
   int success = 0;
   int imol_ref_map = g.Imol_Refinement_Map();
   if (is_valid_map_molecule(imol_ref_map)) {
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t g;
	 const clipper::Xmap<float> &xmap = g.molecules[imol_ref_map].xmap;
	 success = g.molecules[imol].morph_fit_residues(residue_specs, xmap,
							transformation_averaging_radius);
	 graphics_draw();
      } else {
	 std::cout << "WARNING:: no valid map. Stopping now" << std::endl;
      }
   } else {
      std::cout << "WARNING:: " << imol << " is not a valid model molecule " << std::endl;
   }
   return success;
}

int fit_by_secondary_structure_elements(int imol, const std::string &chain_id) {

   graphics_info_t g;
   int success = 0;
   int imol_ref_map = g.Imol_Refinement_Map();
   if (is_valid_map_molecule(imol_ref_map)) {
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t g;
	 const clipper::Xmap<float> &xmap = g.molecules[imol_ref_map].xmap;
	 success = g.molecules[imol].fit_by_secondary_structure_elements(chain_id, xmap);
	 graphics_draw();
      } else {
	 std::cout << "WARNING:: no valid map. Stopping now" << std::endl;
      }
   } else {
      std::cout << "WARNING:: " << imol << " is not a valid model molecule " << std::endl;
   } 
   return success;
} 




void set_secondary_structure_restraints_type(int itype) {

#ifdef HAVE_GSL   

   if (itype == 0)
      graphics_info_t::pseudo_bonds_type = coot::NO_PSEUDO_BONDS;
   if (itype == 1)
      graphics_info_t::pseudo_bonds_type = coot::HELIX_PSEUDO_BONDS;
   if (itype == 2)
      graphics_info_t::pseudo_bonds_type = coot::STRAND_PSEUDO_BONDS;
#endif // HAVE_GSL   
} 

/*! \brief return the secondary structure restraints type */
int secondary_structure_restraints_type() {

   // cast a pseudo_restraint_bond_type to an int
#ifdef HAVE_GSL   
   return graphics_info_t::pseudo_bonds_type;
#else
   return 0;
#endif // HAVE_GSL   
} 


void accept_regularizement() {

   graphics_info_t g;
   g.accept_moving_atoms();	// does a g.clear_up_moving_atoms();
   g.clear_moving_atoms_object();
}


/* \brief Experimental interface for Ribosome People. 

Ribosome People have many chains in their pdb file, they prefer segids
to chainids (chainids are only 1 character).  But coot uses the
concept of chain ids and not seg-ids.  mmdb allow us to use more than
one char in the chainid, so after we read in a pdb, let's replace the
chain ids with the segids. Will that help? */
int exchange_chain_ids_for_seg_ids(int imol) {

   int istat = 0;

   if (is_valid_model_molecule(imol)) {
      istat = graphics_info_t::molecules[imol].exchange_chain_ids_for_seg_ids();
      graphics_draw();
      graphics_info_t g;
      g.update_go_to_atom_window_on_changed_mol(imol);
   }
   return istat;
}

/*  ----------------------------------------------------------------------- */
/*             New Molecule by Various Selection                            */
/*  ----------------------------------------------------------------------- */
/*! \brief create a new molecule that consists of only the residue of
  type residue_type in molecule number imol

@return the noew molecule number, -1 means an error. */
int new_molecule_by_residue_type_selection(int imol_orig, const char *residue_type) {

   int imol = -1;

   if (is_valid_model_molecule(imol_orig)) {

      imol = graphics_info_t::create_molecule();
      mmdb::Manager *mol_orig = graphics_info_t::molecules[imol_orig].atom_sel.mol;
      int SelectionHandle = mol_orig->NewSelection();
      mol_orig->SelectAtoms(SelectionHandle, 0, "*",
			    mmdb::ANY_RES, "*",
			    mmdb::ANY_RES, "*",
			    (char *) residue_type,
			    "*", "*", "*");
      mmdb::Manager *mol =
	 coot::util::create_mmdbmanager_from_atom_selection(mol_orig, SelectionHandle);

      if (mol) {
	 std::string name = "residue type ";
	 name += residue_type;
	 name += " from ";
	 name += graphics_info_t::molecules[imol_orig].name_for_display_manager();
	 atom_selection_container_t asc = make_asc(mol);
	 if (asc.n_selected_atoms > 0) {
	    bool shelx_flag = 0;
	    if (graphics_info_t::molecules[imol_orig].is_from_shelx_ins())
	       shelx_flag = 1;
	    graphics_info_t::molecules[imol].install_model(imol, asc, name, 1, shelx_flag);
	 } else {
            std::cout << "in new_molecule_by_residue_type_selection "
                      << "Something bad happened - No residues selected"
                      << std::endl;
            std::string s = "Oops, failed to select residue type. ";
            s += "No residues selected\n";
            s += "Residue ";
            s += "\"";
            s += residue_type;
            s += "\" does not exist in molecule ";
	    s += coot::util::int_to_string(imol_orig);
	    s += "!?";
            info_dialog(s.c_str());
            imol = -1;
            graphics_info_t::erase_last_molecule();
	 }
      } else {
	 std::cout << "in new_molecule_by_residue_type_selection "
		   << "Something bad happened - null molecule" << std::endl;
	 graphics_info_t::erase_last_molecule();
      } 
      mol_orig->DeleteSelection(SelectionHandle);
      graphics_draw();
   } else {
      std::cout << "Molecule number " << imol_orig << " is not a valid "
		<< "model molecule" << std::endl;
   } 
   return imol;
}

int new_molecule_by_atom_selection(int imol_orig, const char* atom_selection_str) {

   int imol = -1;
   if (is_valid_model_molecule(imol_orig)) {
      imol = graphics_info_t::create_molecule();
      mmdb::Manager *mol_orig = graphics_info_t::molecules[imol_orig].atom_sel.mol;
      int SelectionHandle = mol_orig->NewSelection();
      mol_orig->Select(SelectionHandle, mmdb::STYPE_ATOM,
		       atom_selection_str, 
		       mmdb::SKEY_OR);
      mmdb::Manager *mol =
	 coot::util::create_mmdbmanager_from_atom_selection(mol_orig,
							    SelectionHandle);

      if (mol) {
	 std::string name = "atom selection from ";
	 name += graphics_info_t::molecules[imol_orig].name_for_display_manager();
	 atom_selection_container_t asc = make_asc(mol);
	 if (asc.n_selected_atoms > 0){
	    bool shelx_flag = 0;
	    if (graphics_info_t::molecules[imol_orig].is_from_shelx_ins())
	       shelx_flag = 1;
	    graphics_info_t::molecules[imol].install_model(imol, asc, name, 1, shelx_flag);
	    update_go_to_atom_window_on_new_mol();
	 } else {
	    std::cout << "in new_molecule_by_atom_selection "
		      << "Something bad happened - No atoms selected"
		      << std::endl;
	    std::string s = "Oops, failed to create fragment.  ";
	    s += "No atoms selected\n";
	    s += "Incorrect atom specifier? ";
	    s += "\"";
	    s += atom_selection_str;
	    s += "\"";
	    info_dialog(s.c_str());
	    imol = -1;
	    graphics_info_t::erase_last_molecule();
	 }
      } else {
	 // mol will (currently) never be null,
	 // create_mmdbmanager_from_atom_selection() always returns a
	 // good mmdb::Manager pointer.
	 std::cout << "in new_molecule_by_atom_selection "
		   << "Something bad happened - null molecule" << std::endl;
	 std::string s = "Oops, failed to create fragment.  ";
	 s += "Incorrect atom specifier?\n";
	 s += "\"";
	 s += atom_selection_str;
	 s += "\"";
	 info_dialog(s.c_str());
	 imol = -1;
	 graphics_info_t::erase_last_molecule();
      } 
      mol_orig->DeleteSelection(SelectionHandle);
      graphics_draw();
   } else {
      std::cout << "Molecule number " << imol_orig << " is not a valid "
		<< "model molecule" << std::endl;
   }
   return imol;
} 

int new_molecule_by_sphere_selection(int imol_orig, float x, float y, float z, float r,
				     short int allow_partial_residues_flag) {

   int imol = -1;
   if (is_valid_model_molecule(imol_orig)) {
      imol = graphics_info_t::create_molecule();
      mmdb::Manager *mol_orig = graphics_info_t::molecules[imol_orig].atom_sel.mol;
      int SelectionHandle = mol_orig->NewSelection();

      mmdb::Manager *mol = NULL;
      if (allow_partial_residues_flag) { 
	 mol_orig->SelectSphere(SelectionHandle, mmdb::STYPE_ATOM,
				x, y, z, r, mmdb::SKEY_OR);
	 mol = coot::util::create_mmdbmanager_from_atom_selection(mol_orig,
								  SelectionHandle);
      } else {
	 graphics_info_t g;
	 mol_orig->SelectSphere(SelectionHandle, mmdb::STYPE_RESIDUE,
				x, y, z, r, mmdb::SKEY_OR);
	 std::string alt_conf = "";

	 // convert for mmdb residue list to std::vector or residues.
	 std::vector<mmdb::Residue *> residues;
	 mmdb::PPResidue SelResidues = 0;
	 int nSelResidues;
	 mol_orig->GetSelIndex(SelectionHandle, SelResidues, nSelResidues);
	 for (int i=0; i<nSelResidues; i++)
	    residues.push_back(SelResidues[i]);
	 
	 std::pair<mmdb::Manager *, std::vector<mmdb::Residue *> > mp = 
	    g.create_mmdbmanager_from_res_vector(residues,
						 imol_orig, // for uddatom index.
						 mol_orig, alt_conf);
	 mol = mp.first;
      }
	 
      if (mol) {
	 std::string name = "sphere selection from ";
	 name += graphics_info_t::molecules[imol_orig].name_for_display_manager();
	 atom_selection_container_t asc = make_asc(mol);
	 if (asc.n_selected_atoms > 0){
	    bool shelx_flag = 0;
	    if (graphics_info_t::molecules[imol_orig].is_from_shelx_ins())
	       shelx_flag = 1;
	    graphics_info_t::molecules[imol].install_model(imol, asc, name, 1, shelx_flag);
	 } else {
	    graphics_info_t::erase_last_molecule();
	    std::cout << "in new_molecule_by_atom_selection "
		      << "Something bad happened - No atoms selected"
		      << std::endl;
	    std::string s = "Oops, failed to create fragment.  ";
	    s += "No atoms selected\n";
	    s += "Incorrect position or radius? ";
	    s += "Radius ";
	    s += coot::util::float_to_string(r);
	    s += " at (";
	    s += coot::util::float_to_string(x);
	    s += ", ";
	    s += coot::util::float_to_string(y);
	    s += ", ";
	    s += coot::util::float_to_string(z);
	    s += ")";
	    info_dialog(s.c_str());
	    imol = -1;
	 }
      } else {
	 // mol will (currently) never be null,
	 // create_mmdbmanager_from_atom_selection() always returns a
	 // good mmdb::Manager pointer.
	 graphics_info_t::erase_last_molecule();
	 std::cout << "in new_molecule_by_atom_selection "
		   << "Something bad happened - null molecule" << std::endl;
	 std::string s = "Oops, failed to create fragment.  ";
	 s += "No atoms selected\n";
	 s += "Incorrect position or radius? ";
	 s += "Radius ";
	 s += coot::util::float_to_string(r);
	 s += " at (";
	 s += coot::util::float_to_string(x);
	 s += ", ";
	 s += coot::util::float_to_string(y);
	 s += ", ";
	 s += coot::util::float_to_string(z);
	 s += ")";
	 info_dialog(s.c_str());
	 imol = -1;
      }
      mol_orig->DeleteSelection(SelectionHandle);
      graphics_draw();
   } else {
      std::cout << "Molecule number " << imol_orig << " is not a valid "
		<< "model molecule" << std::endl;
   }
   return imol;
}

// ---------------------------------------------------------------------
// b-factor
// ---------------------------------------------------------------------


void set_default_temperature_factor_for_new_atoms(float new_b) {

   graphics_info_t::default_new_atoms_b_factor = new_b;
} 

float default_new_atoms_b_factor() {
   return graphics_info_t::default_new_atoms_b_factor;
} 

void set_reset_b_factor_moved_atoms(int state) {
  
    graphics_info_t::reset_b_factor_moved_atoms_flag = state;
}

int get_reset_b_factor_moved_atoms_state() {
  
    return graphics_info_t::reset_b_factor_moved_atoms_flag;
}

/*  ----------------------------------------------------------------------- */
/*                  SHELX stuff                                             */
/*  ----------------------------------------------------------------------- */

/* section SHELXL Functions */
// return 
int read_shelx_ins_file(const char *filename, short int recentre_flag) {

   int istat = -1;
   graphics_info_t g;
   if (filename) { 
      int imol = graphics_info_t::create_molecule();

      // ugly method to recente the molecule on read
      // (save, set and reset state).
      // 
      short int reset_centre_flag = g.recentre_on_read_pdb;
      g.recentre_on_read_pdb = recentre_flag;
      
      istat = g.molecules[imol].read_shelx_ins_file(std::string(filename));
      if (istat != 1) {
	 graphics_info_t::erase_last_molecule();
	 std::cout << "WARNING:: " << istat << " on read_shelx_ins_file "
		   << filename << std::endl;
      } else {
	 std::cout << "Molecule " << imol << " read successfully\n";
	 istat = imol; // for return status 
	 if (g.go_to_atom_window) {

	    // See comments in
	    // handle_read_draw_molecule_with_recentre() about this.
	    // g.set_go_to_atom_molecule(imol); // No. 20090620
	    
	    g.update_go_to_atom_window_on_new_mol();
	 }
	 graphics_draw();
	 std::vector<std::string> command_strings;
	 command_strings.push_back("read-shelx-ins-file");
	 command_strings.push_back(single_quote(coot::util::intelligent_debackslash(filename)));
	 add_to_history(command_strings);
      }
      g.recentre_on_read_pdb = reset_centre_flag;
   } else {
      std::cout << "ERROR:: null filename in read_shelx_ins_file" << std::endl;
   }
   return istat;
}

int write_shelx_ins_file(int imol, const char *filename) {

   int istat = 0;
   if (filename) {

      if (is_valid_model_molecule(imol)) {
	 std::pair<int, std::string> stat =
	    graphics_info_t::molecules[imol].write_shelx_ins_file(std::string(filename));
	 istat = stat.first;
	 graphics_info_t g;
	 g.add_status_bar_text(stat.second);
	 std::cout << stat.second << std::endl;
      } else {
	 std::cout << "WARNING:: invalid molecule (" << imol
		   << ") for write_shelx_ins_file" << std::endl;
      }
   }
   return istat;
}

void add_shelx_string_to_molecule(int imol, const char *str) {

   if (is_valid_model_molecule(imol))
      graphics_info_t::molecules[imol].add_shelx_string_to_molecule(str);
}



#ifdef USE_GUILE
SCM chain_id_for_shelxl_residue_number(int imol, int resno) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      std::pair<bool, std::string> ch =
	 graphics_info_t::molecules[imol].chain_id_for_shelxl_residue_number(resno);
      if (ch.first)
	 r = scm_makfrom0str(ch.second.c_str());
   } 
   return r;
} 
#endif

#ifdef USE_PYTHON
PyObject *chain_id_for_shelxl_residue_number_py(int imol, int resno) {

   PyObject *r;
   r = Py_False;
   if (is_valid_model_molecule(imol)) {
      std::pair<bool, std::string> ch =
	 graphics_info_t::molecules[imol].chain_id_for_shelxl_residue_number(resno);
      if (ch.first)
	 r = PyString_FromString(ch.second.c_str());
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif // USE_PYTHON








/*  ----------------------------------------------------------------------- */
/*                  SMILES                                                  */
/*  ----------------------------------------------------------------------- */
void do_smiles_gui() {

#if defined USE_GUILE
#if defined USE_GUILE_GTK
   safe_scheme_command("(smiles-gui)");
#else   
#ifdef USE_PYGTK
   safe_python_command("smiles_gui()");
#endif // USE_PYGTK
#endif // USE_GUILE_GTK
#else 
#ifdef USE_PYGTK
   safe_python_command("smiles_gui()");
#endif // USE_PYGTK
#endif // USE_GUILE

} 

/*  ----------------------------------------------------------------------- */
/*                  pepflip                                                 */
/*  ----------------------------------------------------------------------- */
// use the values that are in graphics_info
void do_pepflip(short int state) {

   graphics_info_t g;

   g.set_in_pepflip_define(state);
   if (state) { 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;
      std::cout << "click on a atom in the peptide you wish to flip: "
		<< std::endl;
   } else {
      g.normal_cursor();
   } 
      
} 

void pepflip(int imol, const char *chain_id, int resno,
	     const char *inscode, const char *alt_conf) {
   /* the residue with CO,
      for scripting interface. */

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      g.molecules[imol].pepflip_residue(chain_id, resno, inscode, alt_conf);
      graphics_draw();
   } 
} 

void set_residue_density_fit_scale_factor(float f) {

   graphics_info_t::residue_density_fit_scale_factor = f;
}

float residue_density_fit_scale_factor() {
   return graphics_info_t::residue_density_fit_scale_factor; 
}


// dictionary
int handle_cif_dictionary(const char *filename) {

   graphics_info_t g;
   short int show_dialog_flag = 0;
   if (graphics_info_t::use_graphics_interface_flag)
      show_dialog_flag = 1;
   int r = g.add_cif_dictionary(coot::util::intelligent_debackslash(filename),
                                show_dialog_flag);
   graphics_draw();
   return r;
}

int read_cif_dictionary(const char *filename) {
   
   return handle_cif_dictionary(filename);

}

/*! \brief some programs produce PDB files with ATOMs where there
  should be HETATMs.  This is a function to assign HETATMs as per the
  PDB definition. */
int assign_hetatms(int imol) {

   int r = 0;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].assign_hetatms();
   }
   return r;
}

/*! \brief if this is not a standard group, then turn the atoms to HETATMs. 

Return 1 on atoms changes, 0 on not. Return -1 if residue not found.
*/
int hetify_residue(int imol, const char * chain_id, int resno, const char *ins_code) {

   int r = -1; 
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].hetify_residue_atoms(chain_id, resno, ins_code);
      graphics_draw();
   } 
   return r;
}

/*! \brief residue has HETATMs?
return 1 if all atoms of the specified residue are HETATMs, else,
return 0.  If residue not found, return -1. */
int residue_has_hetatms(int imol, const char * chain_id, int resno, const char *ins_code) {

   int r = -1; 
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].residue_has_hetatms(chain_id, resno, ins_code);
   } 
   return r;
   
}


//! Add hydrogens to imol from the given pdb file
void add_hydrogens_from_file(int imol, std::string pdb_with_Hs_file_name) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      g.molecules[imol].add_hydrogens_from_file(pdb_with_Hs_file_name);
      graphics_draw();
   }
}



/*  ----------------------------------------------------------------------- */
/*                  CNS data stuff                                          */
/*  ----------------------------------------------------------------------- */
int handle_cns_data_file_with_cell(const char *filename, int imol, float a, float b, float c, float alpha, float beta, float gamma, const char *spg_info) {

   clipper::Spacegroup sg;
   clipper::Cell cell;
   clipper::Cell_descr cell_d(a, b, c, 
			      clipper::Util::d2rad(alpha),
			      clipper::Util::d2rad(beta),
			      clipper::Util::d2rad(alpha));
   clipper::Spgr_descr sg_d(spg_info);
   cell.init(cell_d);
   sg.init(sg_d);
   int imol_new = graphics_info_t::create_molecule();
   int istat = graphics_info_t::molecules[imol_new].make_map_from_cns_data(sg, cell, filename);
   if (istat != -1) { 
      graphics_draw();
   }
   return istat;
}


int handle_cns_data_file(const char *filename, int imol_coords) {

   int istat = -1; // returned int
   // first, does the file exist?
   struct stat s; 
   int status = stat(filename, &s);
   // stat check the link targets not the link itself, lstat stats the
   // link itself.
   // 
   if (status != 0 || !S_ISREG (s.st_mode)) {
      std::cout << "Error reading " << filename << std::endl;
      return -1; // which is status in an error
   } else {
      if (S_ISDIR(s.st_mode)) {
	 std::cout << filename << " is a directory." << std::endl;
      } else {
	 if (is_valid_model_molecule(imol_coords)) { 
	    int imol = graphics_info_t::create_molecule();
	    std::pair<bool, clipper::Spacegroup> sg =
	       graphics_info_t::molecules[imol_coords].space_group();
	    std::pair<bool,clipper::Cell> cell =  graphics_info_t::molecules[imol_coords].cell();
	    if (sg.first && cell.first) { 
	       istat = graphics_info_t::molecules[imol].make_map_from_cns_data(sg.second,
									       cell.second,
									       filename);
	       if (istat != -1) { 
		  graphics_draw();
	       } else {
		  graphics_info_t::erase_last_molecule();
	       } 
	    } else {
	       graphics_info_t::erase_last_molecule();
	    }
	 } 
      }
   }
   return istat;
}


void
set_moving_atoms(double phi, double psi) { 

   graphics_info_t g;
   g.set_edit_phi_psi_to(phi, psi);
}

void
accept_phi_psi_moving_atoms() { 

   graphics_info_t g;
   g.accept_moving_atoms();
   clear_moving_atoms_object();

}

void
setup_edit_phi_psi(short int state) {

   graphics_info_t g;
   g.in_edit_phi_psi_define = state;
   if (state) { 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;

      std::cout << "click on an atom in the residue for phi/psi editting"
		<< std::endl;
   } else {
      g.normal_cursor();
   } 
}




// not the write place for this function.  c-interface-map.cc would be better.
int laplacian (int imol) {

   int iret = -1;
   if (is_valid_map_molecule(imol)) {
      clipper::Xmap<float> xmap = coot::util::laplacian_transform(graphics_info_t::molecules[imol].xmap);
      int new_molecule_number = graphics_info_t::create_molecule();
      std::string label = "Laplacian of ";
      label += graphics_info_t::molecules[imol].name_;
      graphics_info_t::molecules[new_molecule_number].new_map(xmap, label);
      iret = new_molecule_number;
   }
   return iret;
}





void
show_partial_charge_info(int imol, const char *chain_id, int resno, const char *ins_code) {

   if (is_valid_model_molecule(imol)) {
      mmdb::Residue *residue =
	 graphics_info_t::molecules[imol].get_residue(chain_id, resno, ins_code);
      if (residue) {
	 std::string resname = residue->GetResName();
	 int read_number = graphics_info_t::cif_dictionary_read_number;
	 graphics_info_t g; 
	 if (g.Geom_p()->have_dictionary_for_residue_type(resname, read_number)) {
	    
	 }
	 graphics_info_t::cif_dictionary_read_number++;
      }
   }
}

// -----------------------------------------
//
// -----------------------------------------
/*! \brief add an alternative conformer to a residue.  Add it in
  conformation rotamer number rotamer_number.  

 Return #f on fail, the altconf string on success */
#ifdef USE_GUILE  
SCM add_alt_conf_scm(int imol, const char*chain_id, int res_no, const char *ins_code,
		     const char *alt_conf, int rotamer_number) {

   SCM r=SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      // returns a bool,string pair (done-correct-flag, new atoms alt conf string)
      std::pair<bool, std::string> p =
	 g.split_residue(imol, std::string(chain_id), res_no,
			 std::string(ins_code), std::string(alt_conf));
      std::cout << "debug:: split_residue() returned " << p.first << " \"" << p.second << "\"" << std::endl;
      if (p.first) {
	 r = scm_makfrom0str(p.second.c_str());
      }
   }
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON  
PyObject *add_alt_conf_py(int imol, const char*chain_id, int res_no, const char *ins_code,
			  const char *alt_conf, int rotamer_number) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      // returns a bool,string pair (done-correct-flag, new atoms alt conf string)
      std::pair<bool, std::string> p =
	 g.split_residue(imol, std::string(chain_id), res_no,
			 std::string(ins_code), std::string(alt_conf));
      if (p.first) {
	 r = PyString_FromString(p.second.c_str());
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif // USE_PYTHON


/*  ----------------------------------------------------------------------- */
/*                  Backrub                                                 */
/*  ----------------------------------------------------------------------- */

/*! \brief set the mode of rotamer search, options are (ROTAMERSEARCHAUTOMATIC),  
  (ROTAMERSEARCHLOWRES) (aka. "backrub rotamers), 
  (ROTAMERSEARCHHIGHRES) (with rigid body fitting) */
void set_rotamer_search_mode(int mode) {

   if ((mode == ROTAMERSEARCHAUTOMATIC) || 
       (mode == ROTAMERSEARCHLOWRES) ||
       (mode == ROTAMERSEARCHHIGHRES)) { 
      graphics_info_t::rotamer_search_mode = mode;
   } else {
      std::string m = "Rotamer Mode ";
      m += coot::util::int_to_string(mode);
      m += " not found";
      add_status_bar_text(m.c_str());
      std::cout << m << std::endl;
   }
}

int rotamer_search_mode_state() {
   
   return graphics_info_t::rotamer_search_mode;
}


/*! \name Backrubbing function */
/*! \{ */
/* \brief do a back-rub rotamer search (with autoaccept) */
int backrub_rotamer(int imol, const char *chain_id, int res_no, 
		    const char *ins_code, const char *alt_conf) {

  int status = 0;

  if (is_valid_model_molecule(imol)) {
     graphics_info_t g;
     int imol_map = g.Imol_Refinement_Map();
     if (is_valid_map_molecule(imol_map)) {

	graphics_info_t g;
	std::pair<bool,float> brs  =
	   g.molecules[imol].backrub_rotamer(chain_id, res_no, ins_code, alt_conf,
					     *g.Geom_p());
	status = brs.first;
	graphics_draw();
	
     } else {
	std::cout << "   WARNING:: " << imol_map << " is not a valid map molecule"
		  << std::endl;
     } 
  } else {
     std::cout << "   WARNING:: " << imol << " is not a valid model molecule"
	       << std::endl;
  } 
  return status;
} 
/*! \} */

// --------------------------------------------------------------------------
//                   jiggle fit
// --------------------------------------------------------------------------
// 
float fit_to_map_by_random_jiggle(int imol, const char *chain_id, int resno, const char *ins_code,
				  int n_trials,
				  float jiggle_scale_factor) {
   float val = -999.0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      int imol_map = g.Imol_Refinement_Map();
      if (is_valid_map_molecule(imol_map)) { 
	 coot::residue_spec_t rs(chain_id, resno, ins_code);
	 float map_sigma = g.molecules[imol_map].map_sigma();
	 val = g.molecules[imol].fit_to_map_by_random_jiggle(rs,
							     g.molecules[imol_map].xmap,
							     map_sigma,
							     n_trials, jiggle_scale_factor);
	 graphics_draw();
      } else {
	 std::cout << "WARNING:: Refinement map not set" << std::endl;
	 add_status_bar_text("Refinement map not set.");
      } 
   }
   return val;
}

float fit_molecule_to_map_by_random_jiggle(int imol, int n_trials, float jiggle_scale_factor) {

   float val = -999.0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      int imol_map = g.Imol_Refinement_Map();
      if (is_valid_map_molecule(imol_map)) { 
	 float map_sigma = g.molecules[imol_map].map_sigma();
	 mmdb::PPAtom atom_selection = g.molecules[imol].atom_sel.atom_selection;
	 int n_atoms = g.molecules[imol].atom_sel.n_selected_atoms;
	 bool use_biased_density_scoring = false; // not for all-molecule
	 val = g.molecules[imol].fit_to_map_by_random_jiggle(atom_selection, n_atoms,
							     g.molecules[imol_map].xmap,
							     map_sigma,
							     n_trials, jiggle_scale_factor,
							     use_biased_density_scoring);
	 graphics_draw();
      }
   }
   return val;
}

float fit_chain_to_map_by_random_jiggle(int imol, const char *chain_id, int n_trials, float jiggle_scale_factor) {

   float val = -999.0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      int imol_map = g.Imol_Refinement_Map();
      mmdb::Manager *mol = g.molecules[imol].atom_sel.mol;
      if (is_valid_map_molecule(imol_map)) { 
	 float map_sigma = g.molecules[imol_map].map_sigma();
	 
	 mmdb::PPAtom atom_selection = 0;
	 int n_atoms;

	 int SelHnd = mol->NewSelection(); // d

	 mol->SelectAtoms(SelHnd, 0,
			  chain_id,
			  mmdb::ANY_RES, "*",
			  mmdb::ANY_RES, "*",
			  "*","*","*","*",mmdb::SKEY_NEW);
	 
	 mol->GetSelIndex(SelHnd, atom_selection, n_atoms);
	 if (n_atoms) { 
	    bool use_biased_density_scoring = false; // not for all-molecule
	    val = g.molecules[imol].fit_to_map_by_random_jiggle(atom_selection, n_atoms,
								g.molecules[imol_map].xmap,
								map_sigma,
								n_trials, jiggle_scale_factor,
								use_biased_density_scoring);
	    mol->DeleteSelection(SelHnd);
	    graphics_draw();
	 } else {
	    add_status_bar_text("Jiggle Fit: No atoms selected.");
	 } 
      }
   }
   return val;
}



/* add a linked residue based purely on dictionary templete. 
   For addition of NAG to ASNs typically.

   This doesn't work with residues with alt confs.
   
   return success status (0 = fail).
*/
int add_linked_residue(int imol, const char *chain_id, int resno, const char *ins_code, 
		       const char *new_residue_comp_id, const char *link_type, int n_trials) {

   int status = 0;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      g.Geom_p()->try_dynamic_add(new_residue_comp_id, g.cif_dictionary_read_number);
      g.cif_dictionary_read_number++;
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);
// 	 g.molecules[imol].add_linked_residue(res_spec, new_residue_comp_id,
// 					      link_type, g.Geom_p());
      // 20140429
      coot::residue_spec_t new_res_spec =
	 g.molecules[imol].add_linked_residue_by_atom_torsions(res_spec, new_residue_comp_id,
							       link_type, g.Geom_p());

      if (! new_res_spec.unset_p()) { 
	 if (is_valid_map_molecule(imol_refinement_map())) {
	    const clipper::Xmap<float> &xmap =
	       g.molecules[imol_refinement_map()].xmap;
	    std::vector<coot::residue_spec_t> residue_specs;
	    residue_specs.push_back(res_spec);
	    residue_specs.push_back(new_res_spec);
	    g.molecules[imol].multi_residue_torsion_fit(residue_specs, xmap, n_trials, g.Geom_p());
	 }
      }
      graphics_draw();
   }
   return status;
}

/* add a linked residue based purely on dictionary template. 
   For addition of NAG to ASNs typically.

   This doesn't work with residues with alt confs.
   
   return status is #f for fail and the spec of the added residue on success.
*/
#ifdef USE_GUILE
SCM add_linked_residue_scm(int imol, const char *chain_id, int resno, const char *ins_code, 
			   const char *new_residue_comp_id, const char *link_type) {

   int n_trials = 10000;
   SCM r = SCM_BOOL_F;
   bool do_fit_and_refine = graphics_info_t::linked_residue_fit_and_refine_state;

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      g.Geom_p()->try_dynamic_add(new_residue_comp_id, g.cif_dictionary_read_number);
      g.cif_dictionary_read_number++;
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);

      // 20140429
      coot::residue_spec_t new_res_spec =
	 g.molecules[imol].add_linked_residue_by_atom_torsions(res_spec, new_residue_comp_id,
							       link_type, g.Geom_p());

      if (do_fit_and_refine) { 
	 if (! new_res_spec.unset_p()) {
	    r = scm_residue(new_res_spec);
	    if (is_valid_map_molecule(imol_refinement_map())) {
	       const clipper::Xmap<float> &xmap =
		  g.molecules[imol_refinement_map()].xmap;
	       std::vector<coot::residue_spec_t> residue_specs;
	       residue_specs.push_back(res_spec);
	       residue_specs.push_back(new_res_spec);
	       int n_trials = 1000;

	       // 2 rounds of fit then refine
	       for (int ii=0; ii<2; ii++) { 
		  g.molecules[imol].multi_residue_torsion_fit(residue_specs, xmap, n_trials, g.Geom_p());

		  // refine and re-torsion-fit
		  int mode = graphics_info_t::refinement_immediate_replacement_flag;
		  std::string alt_conf;
		  graphics_info_t::refinement_immediate_replacement_flag = 1;
		  refine_residues_with_alt_conf(imol, residue_specs, alt_conf);
		  accept_regularizement();
		  remove_initial_position_restraints(imol, residue_specs);
		  graphics_info_t::refinement_immediate_replacement_flag = mode;
	       }
	    }
	 }
      }
      graphics_draw();
   }
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *add_linked_residue_py(int imol, const char *chain_id, int resno, const char *ins_code, 
				const char *new_residue_comp_id, const char *link_type) {

   int n_trials = 1000;
   PyObject *r = Py_False;
   bool do_fit_and_refine = graphics_info_t::linked_residue_fit_and_refine_state;

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      g.Geom_p()->try_dynamic_add(new_residue_comp_id, g.cif_dictionary_read_number);
      g.cif_dictionary_read_number++;
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);

      // 20140429
      coot::residue_spec_t new_res_spec =
	 g.molecules[imol].add_linked_residue_by_atom_torsions(res_spec, new_residue_comp_id,
							       link_type, g.Geom_p());

      if (do_fit_and_refine) {
         if (! new_res_spec.unset_p()) {
            r = py_residue(new_res_spec);
            if (is_valid_map_molecule(imol_refinement_map())) {
               const clipper::Xmap<float> &xmap =
                  g.molecules[imol_refinement_map()].xmap;
               std::vector<coot::residue_spec_t> residue_specs;
               residue_specs.push_back(res_spec);
               residue_specs.push_back(new_res_spec);

               // 2 rounds of fit then refine
               for (int ii=0; ii<2; ii++) {
                  g.molecules[imol].multi_residue_torsion_fit(residue_specs, xmap, n_trials, g.Geom_p());

                  // refine and re-torsion-fit
                  int mode = graphics_info_t::refinement_immediate_replacement_flag;
                  std::string alt_conf;
                  graphics_info_t::refinement_immediate_replacement_flag = 1;
                  refine_residues_with_alt_conf(imol, residue_specs, alt_conf);
                  accept_regularizement();
                  remove_initial_position_restraints(imol, residue_specs);
                  graphics_info_t::refinement_immediate_replacement_flag = mode;
               }
            }
         }
      }
      graphics_draw();
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif 

void set_add_linked_residue_do_fit_and_refine(int state) {

   graphics_info_t::linked_residue_fit_and_refine_state = state;
} 




/*  ----------------------------------------------------------------------- */
/*               Use Cowtan's protein_db to discover loops                  */
/*  ----------------------------------------------------------------------- */

#ifdef USE_GUILE
SCM
protein_db_loops_scm(int imol_coords, SCM residue_specs_scm, int imol_map, int nfrags,
		     bool preserve_residue_names) {

   SCM r = SCM_BOOL_F;
   std::vector<coot::residue_spec_t> specs = scm_to_residue_specs(residue_specs_scm);
   if (!specs.size()) {
      std::cout << "WARNING:: Ooops - no specs in " 
		<< scm_to_locale_string(display_scm(residue_specs_scm))
		<< std::endl;
   } else {
      std::pair<std::pair<int, int>, std::vector<int> > p = 
	 protein_db_loops(imol_coords, specs, imol_map, nfrags, preserve_residue_names);
      SCM mol_list_scm = SCM_EOL;
      // add backwards (for scheme)
      for (int i=(p.second.size()-1); i>=0; i--)
	 mol_list_scm = scm_cons(SCM_MAKINUM(p.second[i]), mol_list_scm);
      SCM first_pair_scm = scm_list_2(SCM_MAKINUM(p.first.first), SCM_MAKINUM(p.first.second));
      r = scm_list_2(first_pair_scm, mol_list_scm);
   } 
   return r;
} 
#endif 

#ifdef USE_PYTHON
PyObject *
protein_db_loops_py(int imol_coords, PyObject *residue_specs_py, int imol_map, int nfrags,
		    bool preserve_residue_names) {

   PyObject *r = Py_False;
   std::vector<coot::residue_spec_t> specs = py_to_residue_specs(residue_specs_py);
   if (!specs.size()) {
      std::cout << "WARNING:: Ooops - no specs in " 
		<< PyString_AsString(display_python(residue_specs_py))
		<< std::endl;
   } else {
      std::pair<std::pair<int, int>, std::vector<int> > p = 
	 protein_db_loops(imol_coords, specs, imol_map, nfrags, preserve_residue_names);
      PyObject *mol_list_py = PyList_New(p.second.size());
      for (unsigned int i=0; i< p.second.size(); i++)
        PyList_SetItem(mol_list_py, i, PyInt_FromLong(p.second[i]));
      r = PyList_New(2);
      PyObject *loop_molecules = PyList_New(2);
      PyList_SetItem(loop_molecules, 0, PyInt_FromLong(p.first.first));
      PyList_SetItem(loop_molecules, 1, PyInt_FromLong(p.first.second));

      PyList_SetItem(r, 0, loop_molecules);
      PyList_SetItem(r, 1, mol_list_py);
   } 
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif //PYTHON 

// return in the first pair, the imol of the new molecule generated
// from an atom selection of the imol_coords for the residue selection
// of the loop and the molecule number of the consolidated solutions
// (displayed in purple).  and the second of the outer pair, there is
// vector of molecule indices for each of the candidate loops.
// 
// return -1 in the first of the pair on failure
// 
std::pair<std::pair<int, int> , std::vector<int> > 
protein_db_loops(int imol_coords, const std::vector<coot::residue_spec_t> &residue_specs, int imol_map, int nfrags,
		 bool preserve_residue_names) {
   
   int imol_consolodated = -1;
   int imol_loop_orig = -1; // set later hopefully
   std::vector<int> vec_chain_mols;

   if (! is_valid_model_molecule(imol_coords)) {
      std::cout << "WARNING:: molecule number " << imol_coords 
		<< " is not a valid molecule " << std::endl;
   } else { 
      if (! is_valid_map_molecule(imol_map)) {
	 std::cout << "WARNING:: molecule number " << imol_coords << " is not a valid map " 
		   << std::endl;
      } else { 
	 
	 if (residue_specs.size()) { 
	    std::string chain_id = residue_specs[0].chain;
	    // what is the first resno?
	    std::vector<coot::residue_spec_t> rs = residue_specs;
	    std::sort(rs.begin(), rs.end());
	    int first_res_no = rs[0].resno;
	    
	    clipper::Xmap<float> &xmap = graphics_info_t::molecules[imol_map].xmap;

	    std::vector<ProteinDB::Chain> chains =
	       graphics_info_t::molecules[imol_coords].protein_db_loops(residue_specs, nfrags, xmap);

	    if (chains.size()) {

	       // a molecule for each chain
	       for(unsigned int ich=0; ich<chains.size(); ich++) { 
		  mmdb::Manager *mol = make_mol(chains[ich], chain_id, first_res_no, preserve_residue_names);
		  int imol = graphics_info_t::create_molecule();
		  std::string name = "Loop candidate #"; 
		  name += coot::util::int_to_string(ich+1);
		  graphics_info_t::molecules[imol].install_model(imol, mol, name, 1);
		  vec_chain_mols.push_back(imol);
		  set_mol_displayed(imol, 0);
		  set_mol_active(imol, 0);
	       }

	       // The consolodated molecule
	       imol_consolodated = graphics_info_t::create_molecule();
	       mmdb::Manager *mol = make_mol(chains, chain_id, first_res_no, preserve_residue_names);
	       std::string name = "All Loop candidates "; 
	       graphics_info_t::molecules[imol_consolodated].install_model(imol_consolodated, 
									   mol, name, 1);
	       graphics_info_t::molecules[imol_consolodated].set_bond_thickness(2);
	       graphics_info_t::molecules[imol_consolodated].bonds_colour_map_rotation = 260; // purple

	       // now create a new molecule that is the loop with a
	       // copy of the original coordinates
	       // 
	       std::string ass = protein_db_loop_specs_to_atom_selection_string(residue_specs);
	       imol_loop_orig = new_molecule_by_atom_selection(imol_coords, ass.c_str());
	       set_mol_active(imol_loop_orig, 0);
	       set_mol_displayed(imol_loop_orig, 0);

	       graphics_draw();
	    }
	 }
      }
   }
   std::pair<int, int> p(imol_loop_orig, imol_consolodated);
   return std::pair<std::pair<int, int>, std::vector<int> > (p, vec_chain_mols); 
} 

// so that we can create a "original loop" molecule from the atom
// specs picked (i.e. the atom selection string should extend over the
// range from the smallest residue number to the largest (in the same
// chain)).
std::string
protein_db_loop_specs_to_atom_selection_string(const std::vector<coot::residue_spec_t> &specs) { 

   std::string r = "////"; // fail

   // check that we have the same chain id in all the specs, if yes,
   // then continue.
   std::map<std::string, int> chain_ids;
   for (unsigned int i=0; i<specs.size(); i++)
      chain_ids[specs[i].chain]++;
   if (chain_ids.size() == 1) { 
      std::map<std::string, int>::const_iterator it = chain_ids.begin();
      std::string chain_id = it->first;
      int lowest_resno = 9999;
      int highest_resno = -999;
      for (unsigned int i=0; i<specs.size(); i++) { 
	 if (specs[i].resno < lowest_resno)
	    lowest_resno = specs[i].resno;
	 if (specs[i].resno > highest_resno)
	    highest_resno = specs[i].resno;
      }
      r = "//";
      r += chain_id;
      r += "/";
      r += coot::util::int_to_string(lowest_resno);
      r += "-";
      r += coot::util::int_to_string(highest_resno);
   }
   return r;
}


/* ------------------------------------------------------------------------- */
/*                      LINKs                                                */
/* ------------------------------------------------------------------------- */
void
make_link(int imol, coot::atom_spec_t &spec_1, coot::atom_spec_t &spec_2,
	  const std::string &link_name, float length) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      g.molecules[imol].make_link(spec_1, spec_2, link_name, length, *g.Geom_p());
      graphics_draw();
   } 
}

#ifdef USE_GUILE
void make_link_scm(int imol, SCM spec_1, SCM spec_2,
		   const std::string &link_name, float length) {

   // the link name and length are currently dummy values - not used.

   coot::atom_spec_t s1 = atom_spec_from_scm_expression(spec_1);
   coot::atom_spec_t s2 = atom_spec_from_scm_expression(spec_2);
   if (s1.string_user_data != "OK")
      std::cout << "WARNING:: problem with atom spec "
		<< scm_to_locale_string(display_scm(spec_1)) << std::endl;
   else 
      if (s2.string_user_data != "OK")
	 std::cout << "WARNING:: problem with atom spec "
		   << scm_to_locale_string(display_scm(spec_2)) << std::endl;
      else 
	 make_link(imol, s1, s2, link_name, length);
}
#endif

#ifdef USE_PYTHON
void make_link_py(int imol, PyObject *spec_1, PyObject *spec_2,
		  const std::string &link_name, float length) {
   coot::atom_spec_t s1 = atom_spec_from_python_expression(spec_1);
   coot::atom_spec_t s2 = atom_spec_from_python_expression(spec_2);
   if (s1.string_user_data != "OK")
     std::cout << "WARNING:: problem with atom spec "
               << PyString_AsString(display_python(spec_1)) << std::endl;
   else 
     if (s2.string_user_data != "OK")
       std::cout << "WARNING:: problem with atom spec "
                 << PyString_AsString(display_python(spec_2)) << std::endl;
     else 
       make_link(imol, s1, s2, link_name, length);
}
#endif
