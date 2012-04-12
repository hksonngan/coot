/* src/c-interface-build.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007, 2008 The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
 * Copyright 2007 by Bernhard Lohkamp
 * Copyright 2008 by Kevin Cowtan
 * Copyright 2007, 2008 The University of Oxford
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

#include <stdlib.h>
#include <iostream>

#define HAVE_CIF  // will become unnessary at some stage.

#include <sys/types.h> // for stating
#include <sys/stat.h>
#if !defined _MSC_VER
#include <unistd.h>
#else
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#include <windows.h>
#endif
 
#include "globjects.h" //includes gtk/gtk.h

#include "callbacks.h"
#include "interface.h" // now that we are moving callback
		       // functionality to the file, we need this
		       // header since some of the callbacks call
		       // fuctions built by glade.

#include <vector>
#include <string>

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "Cartesian.h"
#include "Bond_lines.h"

// #include "xmap-interface.h"
#include "graphics-info.h"

// #include "atom-utils.h" // asc_to_graphics
// #include "db-main.h" not yet

#include "coot-coord-utils.hh"
#include "coot-fasta.hh"

#include "BuildCas.h"
#include "helix-placement.hh"
#include "fast-ss-search.hh"

#include "trackball.h" // adding exportable rotate interface

#include "coot-utils.hh"  // for is_member_p
#include "coot-map-heavy.hh"  // for fffear

#ifdef USE_GUILE
#include <guile/gh.h>
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
  // no fix up needed 
#else    
  #define scm_to_int gh_scm2int
  #define scm_to_locale_string SCM_STRING_CHARS
  #define scm_to_double  gh_scm2double
  #define scm_is_true gh_scm2bool
#endif // SCM version
#endif // USE_GUILE

// Including python needs to come after graphics-info.h, because
// something in Python.h (2.4 - chihiro) is redefining FF1 (in
// ssm_superpose.h) to be 0x00004000 (Grrr).
//
#ifdef USE_PYTHON
#include "Python.h"
#endif // USE_PYTHON


#include "c-interface.h"
#include "cc-interface.hh"

#include "ligand.hh" // for rigid body fit by atom selection.

#include "cmtz-interface.hh" // for valid columns mtz_column_types_info_t
#include "c-interface-mmdb.hh"
#include "c-interface-scm.hh"
#include "c-interface-python.hh"

#ifdef USE_DUNBRACK_ROTAMERS
#include "dunbrack.hh"
#else 
#include "richardson-rotamer.hh"
#endif 
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

   graphics_info_t g;
   coot::mtz_column_types_info_t r = coot::get_f_phi_columns(mtz_file_name);
   if (r.f_cols.size() == 0) {
      std::cout << "No Fobs found in " << mtz_file_name << std::endl;
      std::string s =  "No Fobs found in ";
      s += mtz_file_name;
      g.statusbar_text(s);
   } else { 
      if (r.sigf_cols.size() == 0) {
	 std::cout << "No SigFobs found in " << mtz_file_name << std::endl;
	 std::string s =  "No SigFobs found in ";
	 s += mtz_file_name;
	 g.statusbar_text(s);
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
      int new_mol_number = graphics_info_t::create_molecule();
      CMMDBManager *m = graphics_info_t::molecules[imol].atom_sel.mol;
      CMMDBManager *n = new CMMDBManager;
      n->Copy(m, MMDBFCM_All);
      atom_selection_container_t asc = make_asc(n);
      std::string label = "Copy of ";
      label += graphics_info_t::molecules[imol].name_;
      graphics_info_t::molecules[new_mol_number].install_model(new_mol_number, asc, label, 1);
      update_go_to_atom_window_on_new_mol();
      iret = new_mol_number;
   }
   if (is_valid_map_molecule(imol)) {
      int new_mol_number = graphics_info_t::create_molecule();
      std::string label = "Copy of ";
      label += graphics_info_t::molecules[imol].name_;
      graphics_info_t::molecules[new_mol_number].new_map(graphics_info_t::molecules[imol].xmap_list[0], label);
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

/*! \brief replace the parts of molecule number imol that are
  duplicated in molecule number imol_frag */
int replace_fragment(int imol_target, int imol_fragment,
		     const char *mmdb_atom_selection_str) {

   int istate = 0;
   if (is_valid_model_molecule(imol_target)) {
      if (is_valid_model_molecule(imol_fragment)) {
	 CMMDBManager *mol = graphics_info_t::molecules[imol_fragment].atom_sel.mol;
	 int SelHnd = mol->NewSelection();
	 mol->Select(SelHnd, STYPE_ATOM, (char *) mmdb_atom_selection_str, SKEY_OR);
	 CMMDBManager *mol_new =
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

void apply_undo() {		/* "Undo" button callback */
   graphics_info_t g;
   g.apply_undo();
   add_to_history_simple("apply-undo");
}

void apply_redo() { 
   graphics_info_t g;
   g.apply_redo();
   add_to_history_simple("apply-redo");
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
			 char *chain_id, 
			 int residue_number,
			 char *residue_type,
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
	    CResidue *res_p =
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


/*  ----------------------------------------------------------------------- */
/*                  spin search                                             */
/*  ----------------------------------------------------------------------- */
void spin_search_by_atom_vectors(int imol_map, int imol, const std::string &chain_id,
				 int resno, const std::string &ins_code,
				 const std::pair<std::string, std::string> &direction_atoms,
				 const std::vector<std::string> &moving_atoms_list) {


   if (is_valid_map_molecule(imol_map)) {

      if (is_valid_model_molecule(imol)) {

	 graphics_info_t::molecules[imol].spin_search(graphics_info_t::molecules[imol_map].xmap_list[0],
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
      short int istat = g.molecules[imol].delete_residue(chain_id, resno, std::string(inscode));
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


void delete_residue_with_altconf(int imol,
				 const char *chain_id,
				 int resno,
				 const char *inscode,
				 const char *altloc) {
   std::string altconf(altloc);
   graphics_info_t g;
   short int istat =
      g.molecules[imol].delete_residue_with_altconf(chain_id, resno, inscode, altconf);
   
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
   std::vector<std::string> command_strings;
   command_strings.push_back("delete-residue-with-altconf");
   command_strings.push_back(g.int_to_string(imol));
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
      CResidue *residue_p =
	 graphics_info_t::molecules[imol].get_residue(resno, ins_code, chain_id);
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
      if (v[i].size() > 0){
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


#ifdef USE_GUILE
SCM secret_jed_refine_func(int imol, SCM r) {
   SCM rv = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> residue_specs;
      SCM r_length_scm = scm_length(r);
      int r_length = scm_to_int(r_length_scm);
      for (unsigned int i=0; i<r_length; i++) {
	 SCM res_spec_scm = scm_list_ref(r, SCM_MAKINUM(i));
	 std::pair<bool, coot::residue_spec_t> res_spec =
	    make_residue_spec(res_spec_scm);
	 if (res_spec.first) {
	    residue_specs.push_back(res_spec.second);
	 } 
      }

      if (residue_specs.size() > 0) {
	 std::vector<CResidue *> residues;
	 for (unsigned int i=0; i<residue_specs.size(); i++) {
	    coot::residue_spec_t rs = residue_specs[i];
	    CResidue *r = graphics_info_t::molecules[imol].get_residue(rs);
	    if (r) {
	       residues.push_back(r);
	    }
	 }

	 if (residues.size() > 0) {
	    graphics_info_t g;
	    int imol_map = g.Imol_Refinement_Map();
	    if (is_valid_map_molecule(imol_map)) { 
	       CMMDBManager *mol = g.molecules[imol].atom_sel.mol;
	       g.refine_residues_vec(imol, residues, mol);
	    }
	 } 
      } else {
	 std::cout << "No residue specs found" << std::endl;
      } 
   }
   return rv;
} 
#endif



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

   graphics_info_t g;
   short int istat = g.molecules[imol].delete_atom(chain_id, resno, ins_code, at_name, altLoc);
   if (istat) { 
      // now if the go to atom widget was being displayed, we need to
      // redraw the residue list and atom list (if the molecule of the
      // residue and atom list is the molecule that has just been
      // deleted)
      //

      g.update_go_to_atom_window_on_changed_mol(imol);
      if (g.go_to_atom_window) {
	 int go_to_atom_imol = g.go_to_atom_molecule();
	 if (go_to_atom_imol == imol) { 

	    // The go to atom molecule matched this molecule, so we
	    // need to regenerate the residue and atom lists.
	    GtkWidget *gtktree = lookup_widget(g.go_to_atom_window,
					       "go_to_atom_residue_tree");
#if (GTK_MAJOR_VERSION == 1)
	    g.fill_go_to_atom_residue_list_gtk1(gtktree);
#else 	    
	    g.fill_go_to_atom_residue_tree_gtk2(imol, gtktree);
#endif	    
	 } 
      }
      graphics_draw();
   } else { 
      std::cout << "failed to delete atom  chain_id: :" << chain_id 
		<< ": " << resno << " incode :" << ins_code
		<< ": atom-name :" <<  at_name << ": altloc :" <<  altLoc << ":" << "\n";
   }

   std::string cmd = "delete-atom";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(chain_id);
   args.push_back(resno);
   args.push_back(at_name);
   args.push_back(altLoc);
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

void delete_atom_by_atom_index(int imol, int index, short int do_delete_dialog) {
   graphics_info_t g;

   std::string atom_name = g.molecules[imol].atom_sel.atom_selection[index]->name;
   std::string chain_id  = g.molecules[imol].atom_sel.atom_selection[index]->GetChainID();
   int resno             = g.molecules[imol].atom_sel.atom_selection[index]->GetSeqNum();
   std::string altconf   = g.molecules[imol].atom_sel.atom_selection[index]->altLoc;
   char *ins_code        = g.molecules[imol].atom_sel.atom_selection[index]->GetInsCode();

   CResidue *residue_p =
      graphics_info_t::molecules[imol].get_residue(resno, ins_code, chain_id);
   if (residue_p) {
      graphics_info_t g;
      coot::residue_spec_t spec(residue_p);
      g.delete_residue_from_geometry_graphs(imol, spec);
   }

   delete_atom(imol, chain_id.c_str(), resno, ins_code, atom_name.c_str(), altconf.c_str());
   if (graphics_info_t::delete_item_widget != NULL) {
      if (do_delete_dialog) { // via ctrl

	 // another check is needed, is the check button active?
	 // 
	 // If not we can go ahead and delete the dialog
	 //
	 GtkWidget *checkbutton = lookup_widget(graphics_info_t::delete_item_widget,
						"delete_item_keep_active_checkbutton");
	 if (GTK_TOGGLE_BUTTON(checkbutton)->active) {
	    // don't kill the widget
	    pick_cursor_maybe(); // it was set to normal_cursor() in
                                 // graphics-info-define's delete_item().
	 } else {
	 
	    gint upositionx, upositiony;
	    gdk_window_get_root_origin (graphics_info_t::delete_item_widget->window,
					&upositionx, &upositiony);
	    graphics_info_t::delete_item_widget_x_position = upositionx;
	    graphics_info_t::delete_item_widget_y_position = upositiony;
	    gtk_widget_destroy(graphics_info_t::delete_item_widget);
	    graphics_info_t::delete_item_widget = NULL;
	    graphics_draw();
	 }
      }
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
   std::string chain_id  = g.molecules[imol].atom_sel.atom_selection[index]->GetChainID();
   int resno             = g.molecules[imol].atom_sel.atom_selection[index]->GetSeqNum();
   std::string altloc    = g.molecules[imol].atom_sel.atom_selection[index]->altLoc;
   std::string inscode   = g.molecules[imol].atom_sel.atom_selection[index]->GetInsCode();

   // I don't think that there is any need to call get_residue() here,
   // we can simply construct spec from chain_id, resno and inscode.
   // There are other places where we do this too (to delete a residue
   // from the geometry graphs).
   CResidue *residue_p =
      graphics_info_t::molecules[imol].get_residue(resno, inscode, chain_id);
   if (residue_p) {
      graphics_info_t g;
      coot::residue_spec_t spec(residue_p);
      g.delete_residue_from_geometry_graphs(imol, spec);
   }

   if (altloc == "") 
      delete_residue(imol, chain_id.c_str(), resno, inscode.c_str());
   else
      delete_residue_with_altconf(imol, chain_id.c_str(), resno, inscode.c_str(), altloc.c_str());
   
   if (graphics_info_t::delete_item_widget != NULL) {
      short int do_delete_dialog = do_delete_dialog_by_ctrl;
      GtkWidget *checkbutton = lookup_widget(graphics_info_t::delete_item_widget,
					     "delete_item_keep_active_checkbutton");
      if (GTK_TOGGLE_BUTTON(checkbutton)->active) { 
	 do_delete_dialog = 0;
	 pick_cursor_maybe(); // it was set to normal_cursor() in
			      // graphics-info-define's delete_item().
      }
      if (do_delete_dialog) { 
	 gint upositionx, upositiony;
	 gdk_window_get_root_origin (graphics_info_t::delete_item_widget->window,
				     &upositionx, &upositiony);
	 graphics_info_t::delete_item_widget_x_position = upositionx;
	 graphics_info_t::delete_item_widget_y_position = upositiony;
	 gtk_widget_destroy(graphics_info_t::delete_item_widget);
	 graphics_info_t::delete_item_widget = NULL;
      }
   }

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
   
   if (graphics_info_t::delete_item_widget != NULL) {
      if (do_delete_dialog) { 
	 GtkWidget *checkbutton = lookup_widget(graphics_info_t::delete_item_widget,
						"delete_item_keep_active_checkbutton");
	 if (GTK_TOGGLE_BUTTON(checkbutton)->active) {
	    // dont destroy it
	 } else {
	    // save the position of the window and kill it off.
	    gint upositionx, upositiony;
	    gdk_window_get_root_origin (graphics_info_t::delete_item_widget->window,
					&upositionx, &upositiony);
	    graphics_info_t::delete_item_widget_x_position = upositionx;
	    graphics_info_t::delete_item_widget_y_position = upositiony;
	    gtk_widget_destroy(graphics_info_t::delete_item_widget);
	    graphics_info_t::delete_item_widget = NULL;
	 }
      }
   }
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


/* We need to set the pending delete flag and that can't be done in
   callback, so this wrapper does it */
GtkWidget *wrapped_create_delete_item_dialog() {

   GtkWidget *widget = create_delete_item_dialog();
   GtkWidget *atom_toggle_button;

   if (delete_item_mode_is_atom_p()) { 
      atom_toggle_button = lookup_widget(GTK_WIDGET(widget),
					 "delete_item_atom_radiobutton");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(atom_toggle_button), TRUE);
      std::cout << "Click on the atom that you wish to delete\n";
   } else {
      if (delete_item_mode_is_water_p()) {
	 GtkWidget *water_toggle_button = lookup_widget(widget,
							"delete_item_water_radiobutton");
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(water_toggle_button), TRUE);
      } else { 
	 set_delete_residue_mode(); // The widget default radio button
	 std::cout << "Click on an atom in the residue that you wish to delete\n";
      }
   }
   graphics_info_t::pick_pending_flag = 1;
   pick_cursor_maybe();
   set_transient_and_position(COOT_DELETE_WINDOW, widget);
   store_delete_item_widget(widget);
   return widget; 
}

// -----------------------------------------------------
//  move molecule here widget
// -----------------------------------------------------
GtkWidget *wrapped_create_move_molecule_here_dialog() {

   GtkWidget *w = create_move_molecule_here_dialog();
   GtkWidget *option_menu = lookup_widget(w, "move_molecule_here_optionmenu"); 
   int imol = first_coords_imol();
   graphics_info_t::move_molecule_here_molecule_number = imol;
   GtkSignalFunc callback_func = GTK_SIGNAL_FUNC(graphics_info_t::move_molecule_here_item_select);
   
   graphics_info_t g;
   g.fill_option_menu_with_coordinates_options(option_menu, callback_func, imol);

   return w;
}


void move_molecule_here_by_widget(GtkWidget *w) {

   int imol = graphics_info_t::move_molecule_here_molecule_number;
   move_molecule_to_screen_centre_internal(imol);
   std::vector<std::string> command_strings;
   command_strings.push_back("move-molecule-here");
   command_strings.push_back(clipper::String(imol));
   add_to_history(command_strings);
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

   if (imol_coords < graphics_n_molecules()) {
      if (imol_map < graphics_n_molecules()) {
	 // if (graphics_info_t::molecules[imol_map].has_map()) {
	 std::string ins(insertion_code);
	 std::string chain(chain_id);
	 if (imol_map < 0 ) {
	    std::cout << "INFO:: fitting rotamers by clash score only " << std::endl;
	    f = graphics_info_t::molecules[imol_coords].auto_fit_best_rotamer(resno, altloc, ins,
									      chain, imol_map,
									      1,
									      lowest_probability);
	 } else {
	    if (graphics_info_t::molecules[imol_map].has_map()) {
	       f = graphics_info_t::molecules[imol_coords].auto_fit_best_rotamer(resno, altloc, ins,
										 chain, imol_map,
										 clash_flag,
										 lowest_probability);

	       // first do real space refine if requested
	       if (graphics_info_t::rotamer_auto_fit_do_post_refine_flag) {
		 // Run refine zone with autoaccept, autorange on
		 // the "clicked" atom:
		 // BL says:: dont think we do autoaccept!?
		 short int auto_range = 1;
		 refine_auto_range(imol_coords, chain_id, resno,
				   altloc);
	       }

	       // get the residue so that it can update the geometry graph
	       CResidue *residue_p =
		  graphics_info_t::molecules[imol_coords].get_residue(resno, ins, chain);
	       if (residue_p) {
		  graphics_info_t g;
		  g.update_geometry_graphs(&residue_p, 1, imol_coords, imol_map);
	       }
	       std::cout << "Fitting score for best rotamer: " << f << std::endl;
	    }
	 }
	 graphics_draw();
      }
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
rotamer_score(int resno, const char *insertion_code,
	      const char *chain_id, int imol_coords, int imol_map,
	      int rotamer_number) {
   float f = 0;
   std::string cmd = "rotamer-score";
   std::vector<coot::command_arg_t> args;
   args.push_back(resno);
   args.push_back(coot::util::single_quote(insertion_code));
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(imol_coords);
   args.push_back(imol_map);
   args.push_back(rotamer_number);
   add_to_history_typed(cmd, args);
   return f;
}


/*! \brief return the number of rotamers for this residue */
int n_rotamers(int imol, const char *chain_id, int resno, const char *ins_code) {

   int r = -1; 
   if (is_valid_model_molecule(imol)) { 
      CResidue *res = graphics_info_t::molecules[imol].get_residue(resno, ins_code, chain_id);
      if (res) {
	 graphics_info_t g;
#ifdef USE_DUNBRACK_ROTAMERS			
	 coot::dunbrack d(res, g.molecules[imol].atom_sel.mol, g.rotamer_lowest_probability, 0);
#else			
	 coot::richardson_rotamer d(res, g.molecules[imol].atom_sel.mol, g.rotamer_lowest_probability, 0);
#endif // USE_DUNBRACK_ROTAMERS
	 
	 std::vector<float> probabilities = d.probabilities();
	 r = probabilities.size();
      }
   }
   return r;
} 

/*! \brief set the residue specified to the rotamer number specifed. */
int set_residue_to_rotamer_number(int imol, const char *chain_id, int resno, const char *ins_code, int rotamer_number) {

   int i_done = 0;
   if (is_valid_model_molecule(imol)) {
      int n = rotamer_number;
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);
      i_done = graphics_info_t::molecules[imol].set_residue_to_rotamer_number(res_spec, n);
      graphics_draw();
   }
   return i_done; 
}

#ifdef USE_GUILE
SCM get_rotamer_name_scm(int imol, const char *chain_id, int resno, const char *ins_code) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      coot::residue_spec_t res_spec(chain_id, resno, ins_code);
      CResidue *res = graphics_info_t::molecules[imol].get_residue(res_spec);
      if (res) {
#ifdef USE_DUNBRACK_ROTAMERS
#else
	 coot::richardson_rotamer d(res, graphics_info_t::molecules[imol].atom_sel.mol, 0.0, 1);
	 coot::rotamer_probability_info_t prob = d.probability_of_this_rotamer();
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
      CResidue *res = graphics_info_t::molecules[imol].get_residue(res_spec);
      if (res) {
#ifdef USE_DUNBRACK_ROTAMERS
#else
	 coot::richardson_rotamer d(res, graphics_info_t::molecules[imol].atom_sel.mol, 0.0, 1);
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
      // put up a widget which has a OK callback button which does a 
      // g.place_typed_atom_at_pointer();
      GtkWidget *window = create_pointer_atom_type_dialog();
      
      GtkWidget *optionmenu = lookup_widget(window, "pointer_atom_molecule_optionmenu");
//       GtkSignalFunc callback_func =
// 	 GTK_SIGNAL_FUNC(graphics_info_t::pointer_atom_molecule_menu_item_activate);

      fill_place_atom_molecule_option_menu(optionmenu);

      gtk_widget_show(window);
   }
   add_to_history_simple("place-atom-at-pointer");
}

// This is a copy - more or less - of
// fill_option_menu_with_coordinates_options, except we also add at
// the top "New Molecule" if a molecule by the name of "Pointer Atoms"
// is not found.
// 
// Note that we can't use fill_option_menu_with_coordinates_options
// and add to it because gtk_menu_set_active fails/is ignored.
// 
// fill_pointer_atom_molecule_option_menu
// 
void fill_place_atom_molecule_option_menu(GtkWidget *optionmenu) { 

   GtkSignalFunc callback_func = 
      GTK_SIGNAL_FUNC(graphics_info_t::pointer_atom_molecule_menu_item_activate);
   GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(optionmenu));
   if (menu) 
      gtk_widget_destroy(menu);
   menu = gtk_menu_new();

   GtkWidget *menuitem;
   int pointer_atoms_mol = -1;

   for (int imol=0; imol<graphics_n_molecules(); imol++) {
      if (graphics_info_t::molecules[imol].has_model()) { 
	 if (graphics_info_t::molecules[imol].name_ == "Pointer Atoms") { 
	    pointer_atoms_mol = imol;
	 } 
      }
   }

   int menu_index = 0;

   if (pointer_atoms_mol == -1) { 
      // There were no pointer atoms so let's create "New Molecule" at
      // the top of the list.
      // 
      GtkWidget *menu_item = gtk_menu_item_new_with_label("New Molecule");
      int imol_new = -10;
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 callback_func,
			 GINT_TO_POINTER(imol_new));
      gtk_menu_append(GTK_MENU(menu), menu_item); 
      gtk_widget_show(menu_item); 
      gtk_menu_set_active(GTK_MENU(menu), 0);
      menu_index = 0;
   }

   for (int imol=0; imol<graphics_n_molecules(); imol++) {
      if (graphics_info_t::molecules[imol].has_model()) {
	 std::string ss = graphics_info_t::int_to_string(imol);
	 ss += " " ;
	 int ilen = graphics_info_t::molecules[imol].name_.length();
	 int left_size = ilen-graphics_info_t::go_to_atom_menu_label_n_chars_max;
	 if (left_size <= 0) {
	    // no chop
	    left_size = 0;
	 } else {
	    // chop
	    ss += "...";
	 }
	 ss += graphics_info_t::molecules[imol].name_.substr(left_size, ilen);
	 menuitem =  gtk_menu_item_new_with_label (ss.c_str());
	 menu_index++;
	 gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			     callback_func,
			     GINT_TO_POINTER(imol));
	 gtk_menu_append(GTK_MENU(menu), menuitem); 
	 gtk_widget_show(menuitem);

	 // set any previously saved active position:
	 if (graphics_info_t::user_pointer_atom_molecule == imol) {
	    std::cout << "setting active menu item to "
		      << menu_index << std::endl;
	    gtk_menu_set_active(GTK_MENU(menu), menu_index);
	 }
      }
   }
   
   /* Link the new menu to the optionmenu widget */
   gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu),
			    menu);
} 

int pointer_atom_molecule() {
   graphics_info_t g;
   return g.pointer_atom_molecule();
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
void set_draw_baton(short int i) {
   graphics_info_t g;
   g.draw_baton_flag = i;
   if (i == 1)
      g.start_baton_here();
   graphics_draw();
   std::string cmd = "set-draw-baton";
   std::vector<coot::command_arg_t> args;
   args.push_back(i);
   add_to_history_typed(cmd, args);
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

void baton_try_another() {
   graphics_info_t g;
   g.baton_try_another();
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

// User data has been placed in the window - we use it to get the
// molecule number.
void baton_mode_calculate_skeleton(GtkWidget *window) {

   int imol = -1;

   int *i;

   std::cout << "getting intermediate data in baton_mode_calculate_skeleton "
	     << std::endl;
   i = (int *) gtk_object_get_user_data(GTK_OBJECT(window));

   std::cout << "got intermediate int: " << i << " " << *i << std::endl;

   imol = *i;

   std::cout << "calculating map for molecule " << imol << std::endl;
   if (imol < graphics_info_t::n_molecules() && imol >= 0) { 
      skeletonize_map(0, imol);
   }
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
   args.push_back(ires);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(imol);
   args.push_back(coot::util::single_quote(target_res_type));
   add_to_history_typed(cmd, args);
   
   return istate;
} 


// Return success on residue type match
// success: 1, failure: 0.
//
// Does not cause a make_backup().
//
int
mutate_single_residue_by_serial_number(int ires, const char *chain_id, int imol,
			   char target_res_type) {

   std::string target_as_str = coot::util::single_letter_to_3_letter_code(target_res_type);
   std::cout << "INFO:: mutate target_res_type :" << target_as_str << ":" << std::endl;
      
   return mutate_internal(ires, chain_id, imol, target_as_str);

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
   
   if (imol < graphics_n_molecules()) {
      if (imol >= 0) { 
	 status = graphics_info_t::molecules[imol].mutate(ires,
							  std::string(inscode),
							  std::string(chain_id),
							  target_as_str);
      }
   }
   return status;
}


// return -1 on error:
// 
int chain_n_residues(const char *chain_id, int imol) {

   graphics_info_t g;
   if (imol < graphics_n_molecules()) {
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
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 CChain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    int nres = chain_p->GetNumberOfResidues();
	    if (serial_num < nres) {
	       int ch_n_res;
	       PCResidue *residues;
	       chain_p->GetResidueTable(residues, ch_n_res);
	       CResidue *this_res = residues[serial_num];
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

   int iseqnum = -10000;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 CChain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    int nres = chain_p->GetNumberOfResidues();
	    if (serial_num < nres) {
	       int ch_n_res;
	       PCResidue *residues;
	       chain_p->GetResidueTable(residues, ch_n_res);
	       CResidue *this_res = residues[serial_num];
	       iseqnum = this_res->GetSeqNum();
	    }
	 }
      }
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
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 CChain *chain_p = mol->GetChain(1,ichain);
	 std::string mol_chain_id(chain_p->GetChainID());
	 if (mol_chain_id == std::string(chain_id)) {
	    int nres = chain_p->GetNumberOfResidues();
	    if (serial_num < nres) {
	       int ch_n_res;
	       PCResidue *residues;
	       chain_p->GetResidueTable(residues, ch_n_res);
	       CResidue *this_res = residues[serial_num];
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


char *chain_id(int imol, int ichain) {

   char *r = NULL;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      CChain *chain_p = mol->GetChain(1,ichain);
      if (chain_p) 
	 r = chain_p->GetChainID();
   }
   std::string cmd = "chain_id";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(ichain);
   add_to_history_typed(cmd, args);
   return r;
}


int n_chains(int imol) {

   int nchains = -1;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      nchains = mol->GetNumberOfChains(1);
   }
   std::string cmd = "n-chains";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return nchains;
}

// return -1 on error (e.g. chain_id not found, or molecule is not a
// model), 0 for no, 1 for is.
int is_solvent_chain_p(int imol, const char *chain_id) {

   int r = -1;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int nchains = mol->GetNumberOfChains(1);
      for (int ichain=0; ichain<nchains; ichain++) {
	 CChain *chain_p = mol->GetChain(1,ichain);
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

/* withdrawn for now because it can cause coot to crash - need more investigation */
// 
// /*! \brief sort the chain ids of the imol-th molecule in lexographical order */
// void sort_chains(int imol) { 

//    if (is_valid_model_molecule(imol)) {
//       graphics_info_t::molecules[imol].sort_chains();
//    }
// }



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

GtkWidget *wrapped_create_renumber_residue_range_dialog() {

   GtkWidget *w = create_renumber_residue_range_dialog();
   int imol = first_coords_imol();
   graphics_info_t::renumber_residue_range_molecule = imol;
   if (graphics_info_t::renumber_residue_range_molecule >= 0) { 
      graphics_info_t::fill_renumber_residue_range_dialog(w);
      graphics_info_t g;
      g.fill_renumber_residue_range_internal(w, imol);
   }
   

   return w;
}

void renumber_residues_from_widget(GtkWidget *window) {

   int imol = graphics_info_t::renumber_residue_range_molecule;

   GtkWidget *e1 = lookup_widget(window, "renumber_residue_range_resno_1_entry");
   GtkWidget *e2 = lookup_widget(window, "renumber_residue_range_resno_2_entry");
   GtkWidget *offent = lookup_widget(window, "renumber_residue_range_offset_entry");
   

   std::pair<short int, int> r1  = int_from_entry(e1);
   std::pair<short int, int> r2  = int_from_entry(e2);
   std::pair<short int, int> off = int_from_entry(offent);

   if (r1.first && r2.first && off.first) {
      int start_res = r1.second;
      int last_res =  r2.second;
      int offset   = off.second;

      if (imol >= 0) {
	 if (imol < graphics_info_t::n_molecules()) {
	    if (graphics_info_t::molecules[imol].has_model()) {
	       std::string chain = graphics_info_t::renumber_residue_range_chain;
	       
	       renumber_residue_range(imol, chain.c_str(),
				      start_res, last_res, offset);
	       
	    }
	 }
      }
   } else {
      std::cout << "Sorry. Couldn't read residue or offset from entry widget\n";
   } 
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
/*  return 0 on success, 1 on error. */
int
write_residue_range_to_pdb_file(int imol, const char *chain_id, 
				int resno_start, int resno_end,
				const char *filename) {

   int istat = 0;
   if (is_valid_model_molecule(imol)) {
      std::string chain(chain_id);
      if (resno_end < resno_start) {
	 int tmp = resno_end;
	 resno_end = resno_start;
	 resno_start = tmp;
      } 
      CMMDBManager *mol =
	 graphics_info_t::molecules[imol].get_residue_range_as_mol(chain, resno_start, resno_end);
      if (mol) {
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
   add_to_history_typed(cmd, args);
   return istat;
} 


/*  ------------------------------------------------------------------------ */
/*                         refmac stuff                                      */
/*  ------------------------------------------------------------------------ */

void execute_refmac(GtkWidget *window) {  /* lookup stuff here. */

   // The passed window, is the refmac dialog, where one selects the
   // coords molecule and the map molecule.

   GtkWidget *option_menu = lookup_widget(window,
					  "run_refmac_coords_optionmenu");

   GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));

   GtkWidget *active_item = gtk_menu_get_active(GTK_MENU(menu));

   int imol_coords = graphics_info_t::refmac_molecule; 
   if (imol_coords < 0) {
      std::cout << "No coordinates selected for refmac\n";
   } else { 

//       std::cout << " Running refmac coords molecule number "
// 		<< imol_coords << std::endl;

      option_menu = lookup_widget(window, "run_refmac_map_optionmenu");
      GtkWidget *mtz_file_radiobutton;
#if (GTK_MAJOR_VERSION > 1)
      mtz_file_radiobutton = lookup_widget(window, "run_refmac_mtz_file_radiobutton");
#else
      mtz_file_radiobutton = NULL;
#endif
      menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));
      active_item = gtk_menu_get_active(GTK_MENU(menu));
      int imol_map_refmac = -1;
      int have_mtz_file = 0;
      if (mtz_file_radiobutton && GTK_TOGGLE_BUTTON(mtz_file_radiobutton)->active) {
	have_mtz_file = 1;
      }

      // active_item is set if there was at least one map with refmac params:
      // if none, it is null.
      
      if (active_item == 0 && have_mtz_file == 0) {
	 add_status_bar_text("No map has associated Refmac Parameters - no REFMAC!");
      } else {
	int imol_window = -1;
	std::string mtz_in_filename = "";
	if (!have_mtz_file) {
	  // we get imol from a map mtz file
	  imol_window = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(active_item)));
	} else {
	  // check the filename of the button
	  GtkWidget *button_mtz_label = lookup_widget(window, "run_refmac_mtz_file_label");
#if (GTK_MAJOR_VERSION > 1)
	  const gchar *mtz_filename = gtk_label_get_text(GTK_LABEL(button_mtz_label));
	  mtz_in_filename = mtz_filename;
#else
	  gchar **mtz_filename = 0;
	  gtk_label_get(GTK_LABEL(button_mtz_label), mtz_filename);
	  mtz_in_filename = (char *)mtz_filename;
#endif // GTK
	  if (mtz_in_filename == "(None)") {
	    have_mtz_file = 0;
	    std::cout << "WARNING:: no mtz file given" <<std::endl;
	  } else {
	    if (! coot::file_exists(mtz_in_filename)) {
	      have_mtz_file = 0;
	      std::cout << "WARNING:: mtz file " << mtz_in_filename << " does not exist" <<std::endl;
	    }
	  }

	}

	if (imol_window < 0 && have_mtz_file == 0) {
	  if (have_mtz_file == 0) {
	    std::cout << "No mtz file selected for refmac\n";
	  } else {
	    std::cout << "No map data selected for refmac\n";
	  }
	} else { 
	  imol_map_refmac = imol_window;
	  if (!is_valid_map_molecule(imol_map_refmac) && have_mtz_file == 0) {
	    std::string s = "Invalid molecule number: ";
	    s += graphics_info_t::int_to_string(imol_map_refmac);
	    std::cout << s << std::endl;
	    graphics_info_t g;
	    g.statusbar_text(s);
	  } else {
	    // normal path
	    //	       if (graphics_info_t::molecules[imol_map_refmac].Have_sensible_refmac_params()) { 
	    // just check for refmac mtz file now (either from map or direct
	    if (have_mtz_file == 1 ||
		graphics_info_t::molecules[imol_map_refmac].Refmac_mtz_filename().size() > 0) {
	      if (!have_mtz_file) {
		std::cout << " Running refmac refmac params molecule number "
			  << imol_map_refmac << std::endl;
	      } else {
		std::cout << " Running refmac from mtz file " << mtz_in_filename << "(not map)" << std::endl;
	      }

	      graphics_info_t g;

	      std::string refmac_dir("coot-refmac");
	      short int have_ccp4i_project = 0;
	      if (graphics_info_t::refmac_ccp4i_project_dir != "") { 
		refmac_dir = graphics_info_t::refmac_ccp4i_project_dir;
		have_ccp4i_project = 1;
	      }
	      int istat = make_directory_maybe(refmac_dir.c_str());
	      if (istat != 0) { // fails
		std::cout << "WARNING failed to make directory for refmac -"
			  << " run refmac fails\n" << std::endl;
	      } else {
		// now lookup the active state of the difference map and
		// the phase combine buttons:
		//
		int diff_map_flag;
		int phase_combine_flag;
		GtkWidget *checkbutton;
		// phase_combine_flag is set in refmac_phase_input now.
		// 0: no phase
		// 1: combine (with phase and FOM, as before)
		// 2: combine with HL
		phase_combine_flag = get_refmac_phase_input();
	       
		checkbutton =  lookup_widget(window,"run_refmac_diff_map_checkbutton");
		if (GTK_TOGGLE_BUTTON(checkbutton)->active) {
		  diff_map_flag = 1;
		} else {
		  diff_map_flag = 0;
		}

		// g.molecules[imol_coords].increment_refmac_count();
      
		std::string pdb_in_filename  = refmac_dir;
		std::string pdb_out_filename = refmac_dir;
		std::string mtz_out_filename = refmac_dir;
		std::cout << "DEBUG:: pdb_in_filename is now 1 " <<  pdb_in_filename << std::endl;
		if (! have_ccp4i_project) { 
		  pdb_in_filename  += "/";
		  pdb_out_filename += "/";
		  mtz_out_filename += "/";
		}
		std::cout << "DEBUG:: pdb_in_filename is now 2 " <<  pdb_in_filename << std::endl;
		pdb_in_filename += g.molecules[imol_coords].Refmac_in_name();
		std::cout << "DEBUG:: pdb_in_filename is now 3 " <<  pdb_in_filename << std::endl;

		// cleverness happens in Refmac_out_name:
		pdb_out_filename += g.molecules[imol_coords].Refmac_out_name();
		mtz_out_filename += g.molecules[imol_coords].Refmac_mtz_out_name();

		if (! have_mtz_file) {
		  mtz_in_filename = g.molecules[imol_map_refmac].Refmac_mtz_filename();
		}
		std::string refmac_count_string =
		  g.int_to_string(g.molecules[imol_coords].Refmac_count());

		std::cout << "DEBUG:: mtz_out_filename: " <<
		  mtz_out_filename << std::endl;
		std::cout << "DEBUG:: pdb_out_filename: " <<
		  pdb_out_filename << std::endl;

		// now get the column labels

		// before running refmac we may want to set refmac parameters from the GUI
		// this should overwrite whatever has been set as refmac parameters before
		// we do it before checking for phases, so that these can be included later
		coot::mtz_column_types_info_t *saved_f_phi_columns
		  = (coot::mtz_column_types_info_t *) gtk_object_get_user_data(GTK_OBJECT(window));
		     
		std::string phib_string = "";
		std::string fom_string  = "";
		std::string fobs_col;
		std::string sigfobs_col;

		int icol;
		int sensible_r_free_col = 0;
		std::string fiobs_col;
		std::string sigfiobs_col;
		std::string r_free_col;

		// different in GTK1/2
#if (GTK_MAJOR_VERSION > 1)

		// and we need to get the column labels if we have an input mtz (not map)
		if (have_mtz_file) {

		  if (refmac_use_twin_state() == 0) {
		    // for now we only use Is in twin not in 'normal' refinement
		      icol = saved_f_phi_columns->selected_refmac_fobs_col;
		      fobs_col = saved_f_phi_columns->f_cols[icol].column_label;
		  } else {
		    // for twin we check both Is and Fs
		    // first check the I
		    icol = saved_f_phi_columns->selected_refmac_iobs_col;
		    if (icol > -1) {
		      fiobs_col = saved_f_phi_columns->i_cols[icol].column_label;
		      set_refmac_use_intensities(1);
		    } else {
		      // we must have F/sigF then
		      icol = saved_f_phi_columns->selected_refmac_fobs_col;
		      fiobs_col = saved_f_phi_columns->f_cols[icol].column_label;
		      set_refmac_use_intensities(0);
		    }
		    icol = saved_f_phi_columns->selected_refmac_sigfobs_col;
		    sigfiobs_col = saved_f_phi_columns->sigf_cols[icol].column_label;
		  }
		  icol = saved_f_phi_columns->selected_refmac_r_free_col; /* magic -1 if not set */
		  if (icol >= 0) { 
		    // 
		    sensible_r_free_col = 1;
		    r_free_col = saved_f_phi_columns->r_free_cols[icol].column_label;
		  } else { 
		    sensible_r_free_col = 0;
		    r_free_col = "";
		  }
		}

		std::string phi_label = "";
		std::string fom_label = "";
		std::string hla_label = "";
		std::string hlb_label = "";
		std::string hlc_label = "";
		std::string hld_label = "";

		if (saved_f_phi_columns->selected_refmac_fobs_col > -1 &&
		    saved_f_phi_columns->selected_refmac_sigfobs_col > -1) {
		  // assign the labels if we have them (from the GUI)
		  //std::cout << "BL DEBUG:: selected fobs col " << saved_f_phi_columns->selected_refmac_fobs_col<<std::endl;
		  //std::cout << "BL DEBUG:: selected sigf col " << saved_f_phi_columns->selected_refmac_sigfobs_col<<std::endl;
		  //std::cout << "BL DEBUG:: selected rfree col " << saved_f_phi_columns->selected_refmac_r_free_col<<std::endl;
		  //std::cout << "BL DEBUG:: selected phi col " << saved_f_phi_columns->selected_refmac_phi_col<<std::endl;
		  //std::cout << "BL DEBUG:: selected fom col " << saved_f_phi_columns->selected_refmac_fom_col<<std::endl;
		  //std::cout << "BL DEBUG:: selected hla col " << saved_f_phi_columns->selected_refmac_hla_col<<std::endl;

		  if (refmac_use_sad_state()) {
		    // SAD F/sigF columns
		    // we make F=/F- and sigF+/f- a list to pass to scripting refmac
		    std::string fp_col;
		    std::string fm_col;
		    std::string sigfp_col;
		    std::string sigfm_col;
		    // F+
		    icol = saved_f_phi_columns->selected_refmac_fp_col;
		    fp_col = saved_f_phi_columns->fpm_cols[icol].column_label;
		    // F-
		    icol = saved_f_phi_columns->selected_refmac_fm_col;
		    fm_col = saved_f_phi_columns->fpm_cols[icol].column_label;
		    // sigF+
		    icol = saved_f_phi_columns->selected_refmac_sigfp_col;
		    sigfp_col = saved_f_phi_columns->sigfpm_cols[icol].column_label;
		    // sigF-
		    icol = saved_f_phi_columns->selected_refmac_sigfm_col;
		    sigfm_col = saved_f_phi_columns->sigfpm_cols[icol].column_label;
		    // make lists
#ifdef USE_GUILE
		    fobs_col  = "(cons ";
		    fobs_col += single_quote(fp_col);
		    fobs_col += " ";
		    fobs_col += single_quote(fm_col);
		    fobs_col += ")";
		    sigfobs_col  = "(cons ";
		    sigfobs_col += single_quote(sigfp_col);
		    sigfobs_col += " ";
		    sigfobs_col += single_quote(sigfm_col);
		    sigfobs_col += ")";
#else
#ifdef USE_PYTHON
		    fobs_col = "[";
		    fobs_col += single_quote(fp_col);
		    fobs_col += ", ";
		    fobs_col += single_quote(fm_col);
		    fobs_col += "]";
		    sigfobs_col = "[";
		    sigfobs_col += single_quote(sigfp_col);
		    sigfobs_col += ", ";
		    sigfobs_col += single_quote(sigfm_col);
		    sigfobs_col += "]";
#endif // USE_GUILE
#endif // USE_PYTHON
		    // now get the information about anomalous atom
		    GtkWidget *atom_entry    = lookup_widget(window, "run_refmac_sad_atom_entry");
		    GtkWidget *fp_entry      = lookup_widget(window, "run_refmac_sad_fp_entry");
		    GtkWidget *fpp_entry     = lookup_widget(window, "run_refmac_sad_fpp_entry");
		    GtkWidget *lambda_entry  = lookup_widget(window, "run_refmac_sad_lambda_entry");
		    const gchar *atom_str  = gtk_entry_get_text(GTK_ENTRY(atom_entry));
		    std::string fp_str     = gtk_entry_get_text(GTK_ENTRY(fp_entry));
		    std::string fpp_str    = gtk_entry_get_text(GTK_ENTRY(fpp_entry));
		    std::string lambda_str = gtk_entry_get_text(GTK_ENTRY(lambda_entry));
		    float fp, fpp, lambda;
		    if (fp_str != "") {
		      fp = atof(fp_str.c_str());
		    } else {
		      fp = -9999; // magic unset
		    }
		    if (fpp_str != "") {
		      fpp = atof(fpp_str.c_str());
		    } else {
		      fpp = -9999; // magic unset
		    }
		    if (lambda_str != "") {
		      lambda = atof(lambda_str.c_str());
		    } else {
		      lambda = -9999; // magic unset
		    }
		    add_refmac_sad_atom(atom_str, fp, fpp, lambda);

		  } else {
		    icol = saved_f_phi_columns->selected_refmac_fobs_col;
		    fobs_col = saved_f_phi_columns->f_cols[icol].column_label;

		    icol = saved_f_phi_columns->selected_refmac_sigfobs_col;
		    sigfobs_col = saved_f_phi_columns->sigf_cols[icol].column_label;
		  }

		  icol = saved_f_phi_columns->selected_refmac_r_free_col; /* magic -1 if not set */
		  if (icol >= 0) { 
		    // 
		    sensible_r_free_col = 1;
		    r_free_col = saved_f_phi_columns->r_free_cols[icol].column_label;
		  } else { 
		    sensible_r_free_col = 0;
		    r_free_col = "";
		  }

		  // We save the phase and FOM as 'fourier_*_labels' too, so that they are saved!?
		  if (phase_combine_flag == 1) {
		    icol = saved_f_phi_columns->selected_refmac_phi_col;
		    if (icol == -1) { 
		      printf("INFO:: no phase available (phi/fom)! \n");
		    } else { 
		      phi_label = saved_f_phi_columns->phi_cols[icol].column_label; 
		      icol = saved_f_phi_columns->selected_refmac_fom_col;
		      fom_label = saved_f_phi_columns->weight_cols[icol].column_label;
		      if (! have_mtz_file) {
			graphics_info_t::molecules[imol_map_refmac].store_refmac_phase_params(std::string(phi_label),
											      std::string(fom_label),
											      std::string(hla_label),
											      std::string(hlb_label),
											      std::string(hlc_label),
											      std::string(hld_label));
		      }
		    }
		  }

		  // check the HLs
		  if (phase_combine_flag == 2) {
		    icol = saved_f_phi_columns->selected_refmac_hla_col;
		    if (icol == -1) {
		      printf("INFO:: no phase available (HLs)! \n");
		    } else { 
		      hla_label = saved_f_phi_columns->hl_cols[icol].column_label;
		      icol = saved_f_phi_columns->selected_refmac_hlb_col;
		      hlb_label = saved_f_phi_columns->hl_cols[icol].column_label;
		      icol = saved_f_phi_columns->selected_refmac_hlc_col;
		      hlc_label = saved_f_phi_columns->hl_cols[icol].column_label;
		      icol = saved_f_phi_columns->selected_refmac_hld_col;
		      hld_label = saved_f_phi_columns->hl_cols[icol].column_label;
		      g_print("BL DEBUG:: have HLs \n");
		      if (! have_mtz_file) {
			graphics_info_t::molecules[imol_map_refmac].store_refmac_phase_params(std::string(phi_label),
											      std::string(fom_label),
											      std::string(hla_label),
											      std::string(hlb_label),
											      std::string(hlc_label),
											      std::string(hld_label));
		      }
		    }
		  }
			    
		  if (have_mtz_file){
		    graphics_info_t g;
		    g.store_refmac_params(std::string(mtz_in_filename),
					  std::string(fobs_col), 
					  std::string(sigfobs_col), 
					  std::string(r_free_col),
					  sensible_r_free_col);
		    set_refmac_used_mtz_file(1);
		  } else {
		    graphics_info_t::molecules[imol_map_refmac].store_refmac_params(std::string(mtz_in_filename),
										    std::string(fobs_col), 
										    std::string(sigfobs_col), 
										    std::string(r_free_col),
										    sensible_r_free_col);
		    set_refmac_used_mtz_file(0);
		  }
		}

		//if (g.molecules[imol_map_refmac].Fourier_weight_label() != "") {
		//  phib_string = g.molecules[imol_map_refmac].Fourier_phi_label();
		//  fom_string  = g.molecules[imol_map_refmac].Fourier_weight_label();
		//} else {
		if (phase_combine_flag == 1) {
		  if (! have_mtz_file) {
		    if (g.molecules[imol_map_refmac].Refmac_phi_col() != "") {
		      phib_string = g.molecules[imol_map_refmac].Refmac_phi_col();
		      fom_string  = g.molecules[imol_map_refmac].Refmac_fom_col();
		    } else {
		      std::cout << "WARNING:: Can't do phase combination if we don't use FOMs ";
		      std::cout << "to make the map" << std::endl;
		      std::cout << "WARNING:: Turning off phase combination." << std::endl;
		      phase_combine_flag = 0;
		    }
		  } else {
		    if (phi_label != "" && fom_label != "") {
		      phib_string = phi_label;
		      fom_string  = fom_label;
		    } else {
		      std::cout << "WARNING:: Can't do phase combination if we don't use FOMs ";
		      std::cout << "to make the map" << std::endl;
		      std::cout << "WARNING:: Turning off phase combination." << std::endl;
		      phase_combine_flag = 0;		      
		    }
		  }
		}
		// 	    std::cout << "DEBUG:: fom_string " << fom_string << " "
		// 		      << g.molecules[imol_map_refmac].Fourier_weight_label()
		// 		      << std::endl;

		// now check for HLs
		if (phase_combine_flag == 2) {
		  std::string hla_string = "";
		  std::string hlb_string;
		  std::string hlc_string;
		  std::string hld_string;
		  if (! have_mtz_file && g.molecules[imol_map_refmac].Refmac_hla_col() != "") {
		    hla_string = g.molecules[imol_map_refmac].Refmac_hla_col();
		    hlb_string = g.molecules[imol_map_refmac].Refmac_hlb_col();
		    hlc_string = g.molecules[imol_map_refmac].Refmac_hlc_col();
		    hld_string = g.molecules[imol_map_refmac].Refmac_hld_col();
		  } else {
		    if (have_mtz_file && hla_label != ""){
		      hla_string = hla_label;
		      hlb_string = hlb_label;
		      hlc_string = hlc_label;
		      hld_string = hld_label;
		    } else {
		      std::cout << "WARNING:: no valid HL columns found" <<std::endl;
		    }
		  }
		  if (hla_string != "") {
		    // now save the HLs in a list string (phib) for refmac, depending on scripting
		    std::vector<std::string> hl_list;
		    hl_list.push_back(hla_string);
		    hl_list.push_back(hlb_string);
		    hl_list.push_back(hlc_string);
		    hl_list.push_back(hld_string);
#ifdef USE_GUILE
		    //phib_string = "(list ";
		    // here we pass it as a string, scheme will later make a list
		    phib_string  = hla_string;
		    phib_string += " ";
		    phib_string += hlb_string;
		    phib_string += " ";
		    phib_string += hlc_string;
		    phib_string += " ";
		    phib_string += hld_string;
		    //phib_string += ")";
#else
#ifdef USE_PYTHON
		    phib_string = "[";
		    phib_string += single_quote(hla_string);
		    phib_string += ", ";
		    phib_string += single_quote(hlb_string);
		    phib_string += ", ";
		    phib_string += single_quote(hlc_string);
		    phib_string += ", ";
		    phib_string += single_quote(hld_string);
		    phib_string += "]";
#endif // USE_GUILE
#endif // USE_PYTHON
		    fom_string = "";
		  } else {
		    std::cout << "WARNING:: Can't do phase combination if we don't have HLs " << std::endl;
		    std::cout << "WARNING:: Turning off phase combination." << std::endl;
		    phase_combine_flag = 0;
		  }
		}
		// for TWIN we reset the flags as we dont have phase combination for twin yet
		if (refmac_use_twin_state() == 1) {
		  phase_combine_flag = 0;
		  phib_string = "";
		  fom_string = "";
		}

#else
		// now gtk1
		if (graphics_info_t::molecules[imol_map_refmac].Have_sensible_refmac_params()) {

		  std::cout << " Running refmac refmac params molecule number "
			    << imol_map_refmac << std::endl;
		  
		  fobs_col = g.molecules[imol_map_refmac].Refmac_fobs_col();
		  sigfobs_col = g.molecules[imol_map_refmac].Refmac_sigfobs_col();
		  r_free_col = g.molecules[imol_map_refmac].Refmac_r_free_col();
		  sensible_r_free_col = g.molecules[imol_map_refmac].Refmac_r_free_sensible();

		}
#endif //GTK1/2
		     
		std::string cif_lib_filename = ""; // default, none
		if (graphics_info_t::cif_dictionary_filename_vec->size() > 0) {
		  cif_lib_filename = (*graphics_info_t::cif_dictionary_filename_vec)[0];
		}


		// 	    std::cout << "DEBUG:: attempting to write pdb input file "
		// 		      << pdb_in_filename << std::endl;
		int ierr = g.molecules[imol_coords].write_pdb_file(pdb_in_filename);
		if (!ierr) { 
		  std::cout << "refmac ccp4i project dir " 
			    << graphics_info_t::refmac_ccp4i_project_dir 
			    << std::endl;
		  int run_refmac_with_no_labels = 0;
#if (GTK_MAJOR_VERSION > 1)
		  if (refmac_runs_with_nolabels()) {
		    GtkWidget *nolabels_checkbutton = lookup_widget(window,
								    "run_refmac_nolabels_checkbutton");
		    if (GTK_TOGGLE_BUTTON(nolabels_checkbutton)->active) {
		      run_refmac_with_no_labels = 1;
		      fobs_col    = "";
		      sigfobs_col = "";
		      r_free_col  = "";
		      sensible_r_free_col = 0;
		    }
		  }

#endif // GTK2

		  // And finally run refmac
		  if (run_refmac_with_no_labels == 1 || fobs_col != "") {
		    execute_refmac_real(pdb_in_filename, pdb_out_filename,
					mtz_in_filename, mtz_out_filename,
					cif_lib_filename,
					fobs_col, sigfobs_col, r_free_col, sensible_r_free_col,
					refmac_count_string,
					g.swap_pre_post_refmac_map_colours_flag,
					imol_map_refmac,
					diff_map_flag,
					phase_combine_flag, phib_string, fom_string,
					graphics_info_t::refmac_ccp4i_project_dir);
		  } else {

		    std::cout << "WARNING:: we cannot run Refmac without without valid labels" <<std::endl;
		  }
		} else {
		  std::cout << "WARNING:: fatal error in writing pdb input file"
			    << pdb_in_filename << " for refmac.  Can't run refmac"
			    << std::endl;
		}
	      }
	    }
	  }
	}
	
      }
   }
}


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
   if (phase_combine_flag) {
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

   //cmds.push_back(graphics_info_t::int_to_string(-1)); // don't use NCYCLES
   // oh yes, we do
   cmds.push_back(graphics_info_t::int_to_string(graphics_info_t::refmac_ncycles));
   // BL says:: again debackslash
   cmds.push_back(single_quote(coot::util::intelligent_debackslash(ccp4i_project_dir)));
   if (graphics_info_t::refmac_use_sad_flag && fobs_col_name != "") {
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


void fill_option_menu_with_refmac_options(GtkWidget *optionmenu) {

   graphics_info_t g;
   g.fill_option_menu_with_refmac_options(optionmenu);

} 

void fill_option_menu_with_refmac_methods_options(GtkWidget *optionmenu) {

   graphics_info_t g;
   g.fill_option_menu_with_refmac_methods_options(optionmenu);

} 

void fill_option_menu_with_refmac_phase_input_options(GtkWidget *optionmenu) {

   graphics_info_t g;
   g.fill_option_menu_with_refmac_phase_input_options(optionmenu);

} 

void fill_option_menu_with_refmac_labels_options(GtkWidget *optionmenu) {

   graphics_info_t g;
   g.fill_option_menu_with_refmac_labels_options(optionmenu);

}

void fill_option_menu_with_refmac_file_labels_options(GtkWidget *optionmenu) {

   graphics_info_t g;
   g.fill_option_menu_with_refmac_file_labels_options(optionmenu);

} 

void fill_option_menu_with_refmac_ncycle_options(GtkWidget *optionmenu) {

   graphics_info_t g;
   g.fill_option_menu_with_refmac_ncycle_options(optionmenu);

}

void update_refmac_column_labels_frame(GtkWidget *optionmenu, 
				       GtkWidget *fobs_menu, GtkWidget *fiobs_menu, GtkWidget *fpm_menu,
				       GtkWidget *r_free_menu,
				       GtkWidget *phases_menu, GtkWidget *fom_menu, GtkWidget *hl_menu) {
  graphics_info_t g;
  g.update_refmac_column_labels_frame(optionmenu,
				      fobs_menu, fiobs_menu, fpm_menu,
				      r_free_menu,
				      phases_menu, fom_menu, hl_menu);

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

void set_refmac_phase_input(int phase_flag) {

  graphics_info_t g;
#if (GTK_MAJOR_VERSION > 1)
  g.set_refmac_phase_input(phase_flag);
#else
  std::cout<<"BL INFO:: cannot use phase recombination in GTK1 (yet), so wont change flag!\n"<<std::endl;;
  g.set_refmac_phase_input(0);
#endif // GTK2
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
#if (GTK_MAJOR_VERSION > 1)
  g.set_refmac_use_twin(state);
#else
  std::cout<<"BL INFO:: cannot use TWIN refinement in GTK1 (yet), so wont change flag!\n"<<std::endl;;
  g.set_refmac_use_twin(0);
#endif // GTK2
}

int refmac_use_twin_state() {

  graphics_info_t g;
  return g.refmac_use_twin_flag;
}

void set_refmac_use_sad(int state) {

  graphics_info_t g;
#if (GTK_MAJOR_VERSION > 1)
  g.set_refmac_use_sad(state);
#else
  std::cout<<"BL INFO:: cannot use SAD refinement in GTK1 (yet), so wont change flag!\n"<<std::endl;;
  g.set_refmac_use_sad(0);
#endif // GTK2
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
  for (int i=0; i<sad_atoms.size(); i++) {
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
  for (int i=0; i<sad_atoms.size(); i++) {
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
  }
  return r;
}
#endif // USE_PYTHON

void
fill_refmac_sad_atom_entry(GtkWidget *w) {

  GtkWidget *atom_entry    = lookup_widget(w, "run_refmac_sad_atom_entry");
  GtkWidget *fp_entry      = lookup_widget(w, "run_refmac_sad_fp_entry");
  GtkWidget *fpp_entry     = lookup_widget(w, "run_refmac_sad_fpp_entry");
  GtkWidget *lambda_entry  = lookup_widget(w, "run_refmac_sad_lambda_entry");
  if (graphics_info_t::refmac_sad_atoms.size() > 0) {
    std::string atom_name  = graphics_info_t::refmac_sad_atoms[0].atom_name;
    float fp     = graphics_info_t::refmac_sad_atoms[0].fp;
    float fpp    = graphics_info_t::refmac_sad_atoms[0].fpp;
    float lambda = graphics_info_t::refmac_sad_atoms[0].lambda;
    std::string fp_str = "";
    std::string fpp_str = "";
    std::string lambda_str = "";
    if (fabs(fp + 9999) >= 0.1) {
      fp_str = graphics_info_t::float_to_string(fp);
    } 
    if (fabs(fpp + 9999) >= 0.1) {
      fpp_str = graphics_info_t::float_to_string(fpp);
    }
    if (fabs(lambda + 9999) >= 0.1) {
      lambda_str = graphics_info_t::float_to_string(lambda);
    }
    gtk_entry_set_text(GTK_ENTRY(atom_entry), atom_name.c_str());
    gtk_entry_set_text(GTK_ENTRY(fp_entry), fp_str.c_str());
    gtk_entry_set_text(GTK_ENTRY(fpp_entry), fpp_str.c_str());
    gtk_entry_set_text(GTK_ENTRY(lambda_entry), lambda_str.c_str());
  }

}

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

void apply_add_OXT_from_widget(GtkWidget *w) {

   int imol = graphics_info_t::add_OXT_molecule;
   int resno = -9999;
   std::string chain_id = graphics_info_t::add_OXT_chain;

   GtkWidget *terminal_checkbutton = lookup_widget(w, "add_OXT_c_terminus_radiobutton");
   GtkWidget *residue_number_entry = lookup_widget(w, "add_OXT_residue_entry");

   if (GTK_TOGGLE_BUTTON(terminal_checkbutton)->active) {
      std::cout << "DEBUG:: auto determine C terminus..." << std::endl;
      // we need to determine the last residue in this chain:
      if (is_valid_model_molecule(imol)) { 
	 if (graphics_info_t::molecules[imol].has_model()) {
	    std::pair<short int, int> p =
	       graphics_info_t::molecules[imol].last_residue_in_chain(chain_id);
	    if (p.first) {
	       resno = p.second;
	    } 
	 }
      }
   } else {
      // we get the resno from the widget
      std::pair<short int, int> p = int_from_entry(residue_number_entry);
      if (p.first) {
	 resno = p.second;
      }
   }

   if (resno > -9999) { 
      if (is_valid_model_molecule(imol)) { 
	 if (graphics_info_t::molecules[imol].has_model()) { 
	    std::cout << "DEBUG:: adding OXT to " << imol << " "
		      << chain_id << " " << resno << std::endl;
	    
	    add_OXT_to_residue(imol, resno, "", chain_id.c_str());
	 }
      }
   } else {
      std::cout << "WARNING:: Could not determine last residue - not adding OXT\n";
   } 
}


GtkWidget *wrapped_create_add_OXT_dialog() {

   GtkWidget *w = create_add_OXT_dialog();

   GtkWidget *option_menu = lookup_widget(w, "add_OXT_molecule_optionmenu");

   GtkSignalFunc callback_func = GTK_SIGNAL_FUNC(graphics_info_t::add_OXT_molecule_item_select);

   graphics_info_t g;
   int imol = first_coords_imol();
   graphics_info_t::add_OXT_molecule = imol;
   g.fill_option_menu_with_coordinates_options(option_menu, callback_func, imol);

   g.fill_add_OXT_dialog_internal(w, imol); // function needs object (not static)

   return w;
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

void setup_alt_conf_with_dialog(GtkWidget *dialog) {

   GtkWidget *widget_ca = lookup_widget(dialog, 
					"add_alt_conf_ca_radiobutton");
   GtkWidget *widget_whole = lookup_widget(dialog, 
					   "add_alt_conf_whole_single_residue_radiobutton");
   GtkWidget *widget_range = lookup_widget(dialog, 
					   "add_alt_conf_residue_range_radiobutton");

   if (graphics_info_t::alt_conf_split_type_number() == 0)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_ca), TRUE);
   if (graphics_info_t::alt_conf_split_type_number() == 1)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_whole), TRUE);
   if (graphics_info_t::alt_conf_split_type_number() == 2)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_range), TRUE);

   if (graphics_info_t::alt_conf_split_type_number() < 2) { 
      std::cout << "Click on the residue you want to split" << std::endl;
   } else { 
      std::cout << "Click on a residue range you want to split" << std::endl;
   }

   graphics_info_t::in_add_alt_conf_define = 1;
   graphics_info_t::pick_cursor_maybe();
   graphics_info_t::pick_pending_flag = 1;
   graphics_info_t::add_alt_conf_dialog = dialog;
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


void altconf() { 

   GtkWidget *widget = create_add_alt_conf_dialog();
   setup_alt_conf_with_dialog(widget);
   gtk_widget_show(widget);
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
      g.statusbar_text("Click on a atom. The clicked atom affects the torsion's wagging dog/tail...");
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
      g.statusbar_text("Click on a atom. The order of the clicked atoms affects the torsion's wagging dog/tail...");
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



// Set a flag: Should torsions that move hydrogens be
// considered/displayed in button box?
// 
void set_find_hydrogen_torsions(short int state) {
   graphics_info_t g;
   g.find_hydrogen_torsions = state;
   std::string cmd = "set-find-hydrogen-torsion";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);
}

void set_graphics_edit_current_chi(int ichi) { /* button callback */

   graphics_info_t::edit_chi_current_chi = 0;
   graphics_info_t::in_edit_chi_mode_flag = 0; // off
}

void unset_moving_atom_move_chis() { 
   graphics_info_t::moving_atoms_move_chis_flag = 0; // keyboard 1,2,3
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
	 g.execute_edit_chi_angles(atom_index, imol);
	 status = 1;
      }
   }
   return status;
}



/*  ------------------------------------------------------------------------ */
/*                         recover session:                                  */
/*  ------------------------------------------------------------------------ */
/* section Recover Session Function */
/* After a crash (shock horror!) we provide this convenient interface
   to restore the session.  It runs through all the molecules with
   models and looks at the coot backup directory looking for related
   backup files that are more recent that the read file. */
void recover_session() { 

   int i_rec = 0;
   for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) { 
      if (graphics_info_t::molecules[imol].has_model()) { 
	 coot::backup_file_info info = 
	    graphics_info_t::molecules[imol].recent_backup_file_info();
	 if (info.status) { 

	    coot::backup_file_info *info_copy = new coot::backup_file_info;
	    *info_copy = info;
	    info_copy->imol = imol;
	    
	    GtkWidget *widget = create_recover_coordinates_dialog();
	    gtk_object_set_user_data(GTK_OBJECT(widget), info_copy);
	    
	    GtkWidget *label1, *label2;
	    label1 = lookup_widget(widget, "recover_coordinates_read_coords_label");
	    label2 = lookup_widget(widget, "recover_coordinates_backup_coordinates_label");

	    gtk_label_set_text(GTK_LABEL(label1), info.name.c_str());
	    gtk_label_set_text(GTK_LABEL(label2), info.backup_file_name.c_str());

	    gtk_widget_show(widget);
	    i_rec++;
	 }
      }
   }
   if (i_rec == 0) {
      GtkWidget *w = create_nothing_to_recover_dialog();
      gtk_widget_show(w);
   }
}

// widget needed for lookup of user data:
// 
void execute_recover_session(GtkWidget *widget) { 

   coot::backup_file_info *info = (coot::backup_file_info *) gtk_object_get_user_data(GTK_OBJECT(widget));

   if (info) { 
      
      graphics_info_t g;
      if (info->imol >= 0 && info->imol < g.n_molecules()) { 
	 g.molecules[info->imol].execute_restore_from_recent_backup(info->backup_file_name);
	 graphics_draw();
      }
   } else { 
      std::cout << "ERROR:: couldn't find user data in execute_recover_session\n";
   } 
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
    graphics_info_t::molecules[imol].assign_sequence_from_string(chain_id, std::string(seq));
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
	 if (g.molecules[imol_map].has_map()) { 
	    int iv = g.molecules[imol_coords].trim_by_map(g.molecules[imol_map].xmap_list[0], 
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
	    if (graphics_info_t::molecules[imol_for_map].has_map()) { 
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
	    if (graphics_info_t::molecules[imol_for_map].has_map()) { 
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

   if (ipick == 0) {
      graphics_info_t::in_fixed_atom_define = coot::FIXED_ATOM_NO_PICK;
   } else {
      if (is_unpick) {
	 graphics_info_t::in_fixed_atom_define = coot::FIXED_ATOM_UNFIX;
      } else { 
	 graphics_info_t::in_fixed_atom_define = coot::FIXED_ATOM_FIX;
      }
   }
}


/*  ----------------------------------------------------------------------- */
/*                         Merge Molecules                                  */
/*  ----------------------------------------------------------------------- */

GtkWidget *wrapped_create_merge_molecules_dialog() {

   GtkWidget *w = create_merge_molecules_dialog();
   // fill the dialog here
   GtkWidget *molecule_option_menu = lookup_widget(w, "merge_molecules_optionmenu");
   GtkWidget *molecules_vbox       = lookup_widget(w, "merge_molecules_vbox");

   GtkSignalFunc callback_func = GTK_SIGNAL_FUNC(merge_molecules_menu_item_activate);
   GtkSignalFunc checkbox_callback_func = GTK_SIGNAL_FUNC(on_merge_molecules_check_button_toggled);


   fill_vbox_with_coordinates_options(molecules_vbox, checkbox_callback_func);

   int imol_master = graphics_info_t::merge_molecules_master_molecule;
   if (imol_master == -1) { 
      for (int i=0; i<graphics_info_t::n_molecules(); i++) {
	 if (graphics_info_t::molecules[i].has_model()) {
	    graphics_info_t::merge_molecules_master_molecule = i;
	    imol_master = i;
	    break;
	 }
      }
   }

   graphics_info_t g;
   g.fill_option_menu_with_coordinates_options(molecule_option_menu,
					       callback_func, imol_master);
   return w;
}

void merge_molecules_menu_item_activate(GtkWidget *item, 
					GtkPositionType pos) {

   graphics_info_t::merge_molecules_master_molecule = pos;
}

void fill_vbox_with_coordinates_options(GtkWidget *dialog,
					GtkSignalFunc checkbox_callback_func) {

   GtkWidget *checkbutton;
   std::string button_label;
   GtkWidget *molecules_vbox = lookup_widget(dialog, "merge_molecules_vbox");

   // Unset any preconcieved notion of merging molecules:
   // 
   graphics_info_t::merge_molecules_merging_molecules->resize(0);

   for (int i=0; i<graphics_info_t::n_molecules(); i++) {
      if (graphics_info_t::molecules[i].has_model()) {
	 button_label = graphics_info_t::int_to_string(i);
	 button_label += " ";
	 button_label += graphics_info_t::molecules[i].name_for_display_manager();
	 std::string button_name = "merge_molecules_checkbutton_";
	 button_name += graphics_info_t::int_to_string(i);

	 checkbutton = gtk_check_button_new_with_label(button_label.c_str());
  	 gtk_widget_ref (checkbutton);
  	 gtk_object_set_data_full (GTK_OBJECT (dialog),
  				   button_name.c_str(), checkbutton,
  				   (GtkDestroyNotify) gtk_widget_unref);
	 // The callback (if active) adds this molecule to the merging molecules list.
	 // If not active, it tries to remove it from the list.
	 //
	 // Why am I doing it like this instead of just looking at the
	 // state of the checkbutton when the OK button is pressed?
	 // Because (for the life of me) I can't seem to correctly
	 // lookup the checkbuttons from the button (or dialog for
	 // that matter).
	 // 
	 //  We look at the state when the
	 // "Merge" button is pressed - we don't need a callback to do
	 // that.
	 // 
  	 gtk_signal_connect (GTK_OBJECT (checkbutton), "toggled",
  			     GTK_SIGNAL_FUNC (checkbox_callback_func),
  			     GINT_TO_POINTER(i));
	 gtk_widget_show (checkbutton);
	 gtk_box_pack_start (GTK_BOX (molecules_vbox), checkbutton, FALSE, FALSE, 0);
	 gtk_container_set_border_width (GTK_CONTAINER (checkbutton), 2);
      }
   }
}

// The callback (if active) adds this molecule to the merging molecules list.
// If not active, it tries to remove it from the list.
// 
void on_merge_molecules_check_button_toggled (GtkToggleButton *togglebutton,
					      gpointer         user_data) {

   int imol = GPOINTER_TO_INT(user_data);
   if (togglebutton->active) {
      std::cout << "INFO:: adding molecule " << imol << " to merging list\n";
      graphics_info_t::merge_molecules_merging_molecules->push_back(imol);
   } else {
      std::cout << "INFO:: removing molecule " << imol << " from merging list\n";
      if (coot::is_member_p(*graphics_info_t::merge_molecules_merging_molecules, imol)) {
	 // passing a pointer
	 coot::remove_member(graphics_info_t::merge_molecules_merging_molecules, imol);
      }
   }
}


// Display the gui
void do_merge_molecules_gui() {

   GtkWidget *w = wrapped_create_merge_molecules_dialog();
   gtk_widget_show(w);
} 

// The action on Merge button press:
// 
void do_merge_molecules(GtkWidget *dialog) {

   std::vector<int> add_molecules = *graphics_info_t::merge_molecules_merging_molecules;
   if (add_molecules.size() > 0) { 
      std::pair<int, std::vector<std::string> > stat =
	 merge_molecules_by_vector(add_molecules, graphics_info_t::merge_molecules_master_molecule);
      if (stat.first)
	 graphics_draw();
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
   
   // clean up
   Py_XDECREF(vos);

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
      if (graphics_info_t::molecules[imol].has_model()) {
	 
	 for (unsigned int i=0; i<add_molecules.size(); i++) {
	    if (add_molecules[i]<graphics_info_t::n_molecules()) {
	       if (i >= 0) {
		  if (graphics_info_t::molecules[add_molecules[i]].has_model()) {
		     if (add_molecules[i] != imol) { 
			add_molecules_at_sels.push_back(graphics_info_t::molecules[add_molecules[i]].atom_sel);
		     }
		  }
	       }
	    }
	 }
	 if (add_molecules_at_sels.size() > 0) { 
	    merged_info = graphics_info_t::molecules[imol].merge_molecules(add_molecules_at_sels);
	 }
      }
   }

   if (graphics_info_t::use_graphics_interface_flag) {
      graphics_info_t g;
      g.update_go_to_atom_window_on_changed_mol(imol);
   } 
   return merged_info;
}



/*  ----------------------------------------------------------------------- */
/*                         Mutate Sequence GUI                              */
/*  ----------------------------------------------------------------------- */

GtkWidget *wrapped_create_mutate_sequence_dialog() {

   GtkWidget *w = create_mutate_sequence_dialog();

   set_transient_and_position(COOT_MUTATE_RESIDUE_RANGE_WINDOW, w);

   GtkWidget *molecule_option_menu = lookup_widget(w, "mutate_molecule_optionmenu");
   GtkWidget *chain_option_menu    = lookup_widget(w, "mutate_molecule_chain_optionmenu");
//    GtkWidget *entry1 = lookup_widget(w, "mutate_molecule_resno_1_entry");
//    GtkWidget *entry2 = lookup_widget(w, "mutate_molecule_resno_2_entry");
//    GtkWidget *textwindow = lookup_widget(w, "mutate_molecule_sequence_text");
   GtkSignalFunc callback_func = GTK_SIGNAL_FUNC(mutate_sequence_molecule_menu_item_activate);


   // Get the default molecule and fill chain optionmenu with the molecules chains:
   int imol = -1; 
   for (int i=0; i<graphics_info_t::n_molecules(); i++) {
      if (graphics_info_t::molecules[i].has_model()) {
	 imol = i;
	 break;
      }
   }
   if (imol >= 0) {
      graphics_info_t::mutate_sequence_imol = imol;
      GtkSignalFunc callback =
	 GTK_SIGNAL_FUNC(mutate_sequence_chain_option_menu_item_activate);
      std::string set_chain = graphics_info_t::fill_chain_option_menu(chain_option_menu, imol,
								      callback);
      graphics_info_t::mutate_sequence_chain_from_optionmenu = set_chain;
   } else {
      graphics_info_t::mutate_sequence_imol = -1; // flag for can't mutate
   }
   graphics_info_t g;
   // std::cout << "DEBUG:: filling option menu with default molecule " << imol << std::endl;
   g.fill_option_menu_with_coordinates_options(molecule_option_menu, callback_func, imol);
   return w;
}

void mutate_sequence_molecule_menu_item_activate(GtkWidget *item, 
						 GtkPositionType pos) {

   // change the chain id option menu here...
   std::cout << "DEBUG:: mutate_sequence_molecule_menu_item_activate got pos:"
	     << pos << std::endl;

   graphics_info_t::mutate_sequence_imol = pos;

   GtkWidget *chain_option_menu =
      lookup_widget(item, "mutate_molecule_chain_optionmenu");

   GtkSignalFunc callback_func =
      GTK_SIGNAL_FUNC(mutate_sequence_chain_option_menu_item_activate);
   
   std::string set_chain = graphics_info_t::fill_chain_option_menu(chain_option_menu,
								   pos, callback_func);

   graphics_info_t::mutate_sequence_chain_from_optionmenu = set_chain;
}


void mutate_sequence_chain_option_menu_item_activate (GtkWidget *item,
						      GtkPositionType pos) { 

   char *data = NULL;
   data = (char *)pos;
   std::cout << "INFO:: mutate_sequence_chain_option_menu_item_activate "
	     << " got data: " << data << std::endl;
   // this can fail when more than one sequence mutate is used at the same time:
   if (data) 
      graphics_info_t::mutate_sequence_chain_from_optionmenu = data;
}


// Don't use this function.  Use the one in graphics_info_t which you
// pass the callback function and get back a chain id.
// nvoid fill_chain_option_menu(GtkWidget *chain_option_menu, int imol) {

//   GtkSignalFunc callback_func =
//      GTK_SIGNAL_FUNC(mutate_sequence_chain_option_menu_item_activate);

   // fill_chain_option_menu_with_callback(chain_option_menu, imol, callback_func);
// }

// the generic form of the above
// void fill_chain_option_menu_with_callback(GtkWidget *chain_option_menu, int imol,
//  					  GtkSignalFunc callback_func) {

   // junk this function and use the one that returns a string.
// }




// The "Mutate" button action:
// 
void do_mutate_sequence(GtkWidget *dialog) {

#ifdef USE_PYTHON
#ifdef USE_GUILE
   short int state_lang = coot::STATE_SCM;
#else    
   short int state_lang = coot::STATE_PYTHON;
#endif
#else // python not used
#ifdef USE_GUILE
   short int state_lang = coot::STATE_SCM;
#else    
   short int state_lang = 0;
#endif
#endif   
   
   
   // decode the dialog here

   GtkWidget *entry1 = lookup_widget(dialog, "mutate_molecule_resno_1_entry");
   GtkWidget *entry2 = lookup_widget(dialog, "mutate_molecule_resno_2_entry");

   int t;
   int res1 = -9999, res2 = -99999;
   graphics_info_t g;
   
   const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(entry1));
   t = atoi(entry_text);
   if ((t > -999) && (t < 9999))
      res1 = t;
   entry_text = gtk_entry_get_text(GTK_ENTRY(entry2));
   t = atoi(entry_text);
   if ((t > -999) && (t < 9999))
      res2 = t;

// BL says: we should set a flag tha we swapped the direction and swap back
// before we call fit-gap to actually build backwards!!
   int swap_flag = 0;
   if (res2 < res1) {
      t = res1;
      res1 = res2;
      res2 = t;
      swap_flag = 1;
   }


   // set the imol and chain_id:
   // 
   int imol = graphics_info_t::mutate_sequence_imol;
   std::string chain_id = graphics_info_t::mutate_sequence_chain_from_optionmenu;

   // Auto fit?
   GtkWidget *checkbutton = lookup_widget(dialog, "mutate_sequence_do_autofit_checkbutton"); 
   short int autofit_flag = 0;

   if (GTK_TOGGLE_BUTTON(checkbutton)->active)
      autofit_flag = 1;
      

   if (imol>= 0) { // redundant
      if (is_valid_model_molecule(imol)) { 

	 // get the sequence:
	 GtkWidget *text = lookup_widget(dialog, "mutate_molecule_sequence_text");
	 char *txt = NULL;


#if (GTK_MAJOR_VERSION == 1) 
	 gint start_pos = 0;
	 gint end_pos = -1;
	 txt = gtk_editable_get_chars(GTK_EDITABLE(text), start_pos, end_pos);
#else
	 // std::cout << "Gtk2 text view code... " << std::endl;
	 // text is a GtkTextView in GTK2
	 GtkTextView *tv = GTK_TEXT_VIEW(text);
	 GtkTextBuffer* tb = gtk_text_view_get_buffer(tv);
	 GtkTextIter startiter;
	 GtkTextIter enditer;
	 gtk_text_buffer_get_iter_at_offset(tb, &startiter, 0);
	 gtk_text_buffer_get_iter_at_offset(tb, &enditer, -1);
	 txt = gtk_text_buffer_get_text(tb, &startiter, &enditer, 0);
#endif 	 

	 if (txt) {
	    std::string sequence(txt);
	    sequence = coot::util::plain_text_to_sequence(sequence);
	    std::cout << "we got the sequence: " << sequence << std::endl;

	    if (int(sequence.length()) == (res2 - res1 + 1)) {
	       std::vector<std::string> cmd_strings;
	       if (autofit_flag)
		  cmd_strings.push_back("mutate-and-autofit-residue-range");
	       else 
		  cmd_strings.push_back("mutate-residue-range");
	       cmd_strings.push_back(graphics_info_t::int_to_string(imol));
	       cmd_strings.push_back(single_quote(chain_id));
	       cmd_strings.push_back(graphics_info_t::int_to_string(res1));
	       cmd_strings.push_back(graphics_info_t::int_to_string(res2));
	       cmd_strings.push_back(single_quote(sequence));
	       std::string cmd = g.state_command(cmd_strings, state_lang);
// BL says: I believe we should distinguish between python and guile here!?
#ifdef USE_GUILE
	       if (state_lang == coot::STATE_SCM) {
		  safe_scheme_command(cmd);
	       }
#else
#ifdef USE_PYTHON
              if (state_lang == coot::STATE_PYTHON) {
                 safe_python_command(cmd);
              }
#endif // PYTHON
#endif // GUILE
	       update_go_to_atom_window_on_changed_mol(imol);
	    } else {
	       std::cout << "WARNING:: can't mutate.  Sequence of length: "
			 << sequence.length() << " but residue range size: "
			 << res2 - res1 + 1 << "\n";
	    } 
	 } else {
	    std::cout << "WARNING:: can't mutate.  No sequence\n";
	 } 
      } else {
	 std::cout << "WARNING:: Bad molecule number: " << imol << std::endl;
	 std::cout << "          Can't mutate." << std::endl;
      }
   } else {
      std::cout << "WARNING:: unassigned molecule number: " << imol << std::endl;
      std::cout << "          Can't mutate." << std::endl;
   }
}

GtkWidget *wrapped_fit_loop_dialog() {

   GtkWidget *w = wrapped_create_mutate_sequence_dialog();

   GtkWidget *label              = lookup_widget(w, "function_for_molecule_label");
   GtkWidget *method_frame       = lookup_widget(w, "loop_fit_method_frame");
   GtkWidget *mutate_ok_button   = lookup_widget(w, "mutate_sequence_ok_button");
   GtkWidget *fit_loop_ok_button = lookup_widget(w, "fit_loop_ok_button");
   GtkWidget *checkbutton        = lookup_widget(w, "mutate_sequence_do_autofit_checkbutton");
#if (GTK_MAJOR_VERSION > 1)
   GtkWidget *rama_checkbutton   = lookup_widget(w, "mutate_sequence_use_ramachandran_restraints_checkbutton");
#endif
   
   gtk_label_set_text(GTK_LABEL(label), "\nFit loop in Molecule:\n");
   gtk_widget_hide(mutate_ok_button);
   gtk_widget_hide(checkbutton);
   gtk_widget_show(fit_loop_ok_button);
#if (GTK_MAJOR_VERSION > 1)
   gtk_widget_show(rama_checkbutton);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rama_checkbutton), TRUE);
#endif

   gtk_widget_show(method_frame);

   return w;
}

// And the function called by the Fit Loop (OK) button.
// 
void fit_loop_from_widget(GtkWidget *dialog) {

#ifdef USE_PYTHON
#ifdef USE_GUILE
   short int state_lang = coot::STATE_SCM;
#else    
   short int state_lang = coot::STATE_PYTHON;
#endif
#else // python not used
#ifdef USE_GUILE
   short int state_lang = coot::STATE_SCM;
#else    
   short int state_lang = 0;
#endif
#endif

   // decode the dialog here

   GtkWidget *entry1 = lookup_widget(dialog, "mutate_molecule_resno_1_entry");
   GtkWidget *entry2 = lookup_widget(dialog, "mutate_molecule_resno_2_entry");

   int t;
   int res1 = -9999, res2 = -99999;
   graphics_info_t g;
   
   const gchar *entry_text = gtk_entry_get_text(GTK_ENTRY(entry1));
   t = atoi(entry_text);
   if ((t > -999) && (t < 9999))
      res1 = t;
   entry_text = gtk_entry_get_text(GTK_ENTRY(entry2));
   t = atoi(entry_text);
   if ((t > -999) && (t < 9999))
      res2 = t;

// BL says: we should set a flag that we swapped the direction and swap back
// before we call fit-gap to actually build backwards!!
   int swap_flag = 0;
   if (res2 < res1) {
      t = res1;
      res1 = res2;
      res2 = t;
      swap_flag = 1;
   }


   // set the imol and chain_id:
   // 
   int imol = graphics_info_t::mutate_sequence_imol;
   std::string chain_id = graphics_info_t::mutate_sequence_chain_from_optionmenu;

   // Auto fit?
   GtkWidget *checkbutton = lookup_widget(dialog, "mutate_sequence_do_autofit_checkbutton"); 
   short int autofit_flag = 0;

   if (GTK_TOGGLE_BUTTON(checkbutton)->active)
      autofit_flag = 1;

   // use Ramachandran restraints?
   GtkWidget *rama_checkbutton;
#if (GTK_MAJOR_VERSION > 1)
   rama_checkbutton   = lookup_widget(dialog, "mutate_sequence_use_ramachandran_restraints_checkbutton");
#else
   rama_checkbutton = NULL;
#endif
   int use_rama_restraints = 0;
   if (rama_checkbutton && GTK_TOGGLE_BUTTON(rama_checkbutton)->active) 
      use_rama_restraints = 1;

   if (imol>= 0) { // redundant
      if (is_valid_model_molecule(imol)) {

	 // get the sequence:
	 GtkWidget *text = lookup_widget(dialog, "mutate_molecule_sequence_text");
	 char *txt = NULL;


#if (GTK_MAJOR_VERSION == 1) 
	 gint start_pos = 0;
	 gint end_pos = -1;
	 txt = gtk_editable_get_chars(GTK_EDITABLE(text), start_pos, end_pos);
#else
	 // std::cout << "Gtk2 text view code... " << std::endl;
	 // text is a GtkTextView in GTK2
	 GtkTextView *tv = GTK_TEXT_VIEW(text);
	 GtkTextBuffer* tb = gtk_text_view_get_buffer(tv);
	 GtkTextIter startiter;
	 GtkTextIter enditer;
	 gtk_text_buffer_get_iter_at_offset(tb, &startiter, 0);
	 gtk_text_buffer_get_iter_at_offset(tb, &enditer, -1);
	 txt = gtk_text_buffer_get_text(tb, &startiter, &enditer, 0);
#endif
	 
	 if (txt) {
	    std::string sequence(txt);
	    sequence = coot::util::plain_text_to_sequence(sequence);
	    int text_widget_sequence_length = sequence.length();
	    std::cout << "INFO:: mutating to the sequence :" << sequence
		      << ":" << std::endl;

	    if (int(sequence.length()) == (res2 - res1 + 1) ||
	        sequence == "") {
	    } else {
	       // so set sequence to poly-ala and give us a message:
	       sequence = "";
	       for (int i=0; i<(res2 - res1 + 1); i++)
		  sequence += "A";

	       std::cout << "WARNING:: Sequence of length: "
			 << text_widget_sequence_length << " but residue range size: "
			 << res2 - res1 + 1 << ".  Using Poly-Ala\n";
	       std::string s("WARNING:: Mis-matched sequence length\nUsing Poly Ala");
	       GtkWidget *w = wrapped_nothing_bad_dialog(s);
	       gtk_widget_show(w);
	    }
            if (swap_flag == 1) {
               t = res1;
               res1 = res2;
               res2 = t;
            }

	    std::vector<std::string> cmd_strings;
	    cmd_strings.push_back("fit-gap");
	    cmd_strings.push_back(graphics_info_t::int_to_string(imol));
	    cmd_strings.push_back(single_quote(chain_id));
	    cmd_strings.push_back(graphics_info_t::int_to_string(res1));
	    cmd_strings.push_back(graphics_info_t::int_to_string(res2));
	    cmd_strings.push_back(single_quote(sequence));
	    cmd_strings.push_back(graphics_info_t::int_to_string(use_rama_restraints));
	    std::string cmd = g.state_command(cmd_strings, state_lang);

#ifdef USE_GUILE
	    if (state_lang == coot::STATE_SCM) {
	       safe_scheme_command(cmd);
	    }
#else
#ifdef USE_PYTHON
            if (state_lang == coot::STATE_PYTHON) {
               safe_python_command(cmd);
            }
#endif // PYTHON
#endif // GUILE
	 }
      }
   }
}




/*  ----------------------------------------------------------------------- */
/*                         construct a molecule and update                  */
/*  ----------------------------------------------------------------------- */

#ifdef USE_GUILE
int clear_and_update_molecule(int molecule_number, SCM molecule_expression) {

   int state = 0; 
   if (is_valid_model_molecule(molecule_number)) {

      CMMDBManager *mol =
	 mmdb_manager_from_scheme_expression(molecule_expression);

      if (mol) { 
	 state = 1;
	 graphics_info_t::molecules[molecule_number].replace_molecule(mol);
	 graphics_draw();
      }
   }
   return state;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
int clear_and_update_molecule_py(int molecule_number, PyObject *molecule_expression) {

   int state = 0;
   if (is_valid_model_molecule(molecule_number)) {

      CMMDBManager *mol =
         mmdb_manager_from_python_expression(molecule_expression);

      if (mol) {
         state = 1;
         graphics_info_t::molecules[molecule_number].replace_molecule(mol);
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
   CMMDBManager *mol =
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
   CMMDBManager *mol =
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

/*  ----------------------------------------------------------------------- */
/*                         Align and Mutate GUI                             */
/*  ----------------------------------------------------------------------- */
GtkWidget *wrapped_create_align_and_mutate_dialog() {

   graphics_info_t g;
   GtkWidget *w = create_align_and_mutate_dialog();
   // fill w
   GtkWidget *mol_optionmenu   = lookup_widget(w, "align_and_mutate_molecule_optionmenu");
   GtkWidget *chain_optionmenu = lookup_widget(w, "align_and_mutate_chain_optionmenu");
   GtkSignalFunc callback = GTK_SIGNAL_FUNC(align_and_mutate_molecule_menu_item_activate);
   GtkSignalFunc chain_callback = GTK_SIGNAL_FUNC(align_and_mutate_chain_option_menu_item_activate);

   int imol = graphics_info_t::align_and_mutate_imol;
   if (imol == -1 || (! g.molecules[imol].has_model())) { 
      for (int i=0; i<g.n_molecules(); i++) {
	 if (g.molecules[i].has_model()) {
	    imol = i;
	    break;
	 }
      }
   }

   if (imol >= 0) {
      g.fill_option_menu_with_coordinates_options(mol_optionmenu, callback, imol);
      std::string set_chain = graphics_info_t::fill_chain_option_menu(chain_optionmenu, imol,
								      chain_callback);
      graphics_info_t::align_and_mutate_chain_from_optionmenu = set_chain;
   }
   
   return w;
}




// --------------------------------------------------------------
//                 symmetry
// --------------------------------------------------------------

/* for shelx FA pdb files, there is no space group.  So allow the user
   to set it.  This can be initted with a HM symbol or a symm list for
   clipper */
void set_space_group(int imol, const char *spg) {

   // we should test that this is a clipper string here...
   // does it contain X Y and Z chars?
   // 
   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].set_mmdb_symm(spg);
   }
}


// 
void setup_save_symmetry_coords() {

   graphics_info_t::in_save_symmetry_define = 1;
   //    GtkWidget *w = wrapped_nothing_bad_dialog(std::string("Now click on a symmetry atom"));
   std::string s = "Now click on a symmetry atom";
   graphics_info_t g;
   g.statusbar_text(s);
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
	    CMMDBManager *mol2 = new CMMDBManager;
	    mol2->Copy(graphics_info_t::molecules[imol].atom_sel.mol, MMDBFCM_All);
	    
	    atom_selection_container_t asc = make_asc(mol2);
	    mat44 mat;
	    mat44 mat_origin_shift;

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
	    asc.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
	    asc.mol->FinishStructEdit();
	    
	    int ierr = mol2->WritePDBASCII(filename);
	    if (ierr) {
	       std::cout << "WARNING:: WritePDBASCII to " << filename << " failed." << std::endl;
	       std::string s = "WARNING:: WritePDBASCII to file ";
	       s += filename;
	       s += " failed.";
	       graphics_info_t g;
	       g.statusbar_text(s);
	    } else {
	       std::cout << "INFO:: Wrote symmetry atoms to " << filename << "." << std::endl;
	       std::string s = "INFO:: Wrote symmetry atoms to file ";
	       s += filename;
	       s += ".";
	       graphics_info_t g;
	       g.statusbar_text(s);
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




/*  ----------------------------------------------------------------------- */
/*                  sequence (assignment)                                   */
/*  ----------------------------------------------------------------------- */
/* section Sequence (Assignment) */

void assign_sequence(int imol_coords, int imol_map, const char *chain_id) {

   if (is_valid_model_molecule(imol_coords))
      if (is_valid_map_molecule(imol_map))
	 graphics_info_t::molecules[imol_coords].assign_sequence(graphics_info_t::molecules[imol_map].xmap_list[0], std::string(chain_id));

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
      CAtom *at = graphics_info_t::molecules[imol].atom_sel.atom_selection[idx];
      CResidue *r = at->residue;
      if (r) {
	 coot::residue_spec_t res_spec_t(r);
	 int istat = graphics_info_t::molecules[imol].mutate_base(res_spec_t, std::string(type));
	 if (istat)
	    graphics_draw();
      }
   }
}

/*  ----------------------------------------------------------------------- */
/*                  Change chain ID                                         */
/*  ----------------------------------------------------------------------- */

GtkWidget *wrapped_create_change_chain_id_dialog() {

   GtkWidget *w = create_change_chain_id_dialog();
   GtkWidget *mol_option_menu =  lookup_widget(w, "change_chain_id_molecule_optionmenu");
   GtkWidget *chain_option_menu =  lookup_widget(w, "change_chain_id_chain_optionmenu");
   GtkWidget *residue_range_no_radiobutton =  lookup_widget(w, "change_chain_residue_range_no_radiobutton");

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(residue_range_no_radiobutton), TRUE);

   GtkSignalFunc callback_func = GTK_SIGNAL_FUNC(change_chain_ids_mol_option_menu_item_activate);

   int imol = first_coords_imol();
   if (imol >= 0) {
      graphics_info_t::change_chain_id_molecule = imol;
      GtkSignalFunc chain_callback_func =
	 GTK_SIGNAL_FUNC(change_chain_ids_chain_menu_item_activate);
      std::string set_chain = graphics_info_t::fill_chain_option_menu(chain_option_menu,
								       imol,
								      chain_callback_func);
      graphics_info_t::change_chain_id_from_chain = set_chain;
   }
   graphics_info_t g; 
   g.fill_option_menu_with_coordinates_options(mol_option_menu, callback_func, imol);
   return w;
}

void
change_chain_ids_mol_option_menu_item_activate(GtkWidget *item,
					       GtkPositionType pos) {
   graphics_info_t::change_chain_id_molecule = pos;
   int imol = pos;
   GtkWidget *chain_option_menu =  lookup_widget(item, "change_chain_id_chain_optionmenu");
   GtkSignalFunc chain_callback_func =
      GTK_SIGNAL_FUNC(change_chain_ids_chain_menu_item_activate);
   std::string set_chain = graphics_info_t::fill_chain_option_menu(chain_option_menu,
								   imol,
								   chain_callback_func);
   graphics_info_t::change_chain_id_from_chain = set_chain;
}

void
change_chain_ids_chain_menu_item_activate(GtkWidget *item,
					  GtkPositionType pos) {
   char *data = NULL;
   data = (char *)pos;
   // this can fail when more than one sequence mutate is used at the same time:
   if (data) 
      graphics_info_t::change_chain_id_from_chain = data;
}


void
change_chain_id_by_widget(GtkWidget *w) {

   GtkWidget *residue_range_yes_radiobutton =  lookup_widget(w, "change_chain_residue_range_yes_radiobutton");

   GtkWidget *residue_range_from_entry  =  lookup_widget(w, "change_chain_residues_from_entry");
   GtkWidget *residue_range_to_entry  =  lookup_widget(w, "change_chains_residues_to_entry");
   GtkWidget *change_chains_new_chain_entry  =  lookup_widget(w, "change_chains_new_chain_id");

   int imol = graphics_info_t::change_chain_id_molecule;
   short int use_res_range_flag = 0;
   int from_resno = -9999;
   int to_resno = -9999;

   if (GTK_TOGGLE_BUTTON(residue_range_yes_radiobutton)->active) { 
      use_res_range_flag = 1;
      std::pair<short int, int> p1 = int_from_entry(residue_range_from_entry);
      std::pair<short int, int> p2 = int_from_entry(residue_range_to_entry);
      if (p1.first)
	 from_resno = p1.second;
      if (p2.first)
	 to_resno = p2.second;
   }

   const gchar *txt = gtk_entry_get_text(GTK_ENTRY(change_chains_new_chain_entry));

   if (txt) { 
   
      if (is_valid_model_molecule(imol)) {
	 std::string to_chain_id(txt);
	 std::string from_chain_id = graphics_info_t::change_chain_id_from_chain;
	 std::pair<int, std::string> r = 
	    graphics_info_t::molecules[imol].change_chain_id(from_chain_id,
							     to_chain_id,
							     use_res_range_flag,
							     from_resno,
							     to_resno);
	 if (r.first == 1) { // it went OK
	    update_go_to_atom_window_on_changed_mol(imol);
	    graphics_draw();
	 } else {
	    GtkWidget *ws = wrapped_nothing_bad_dialog(r.second);
	    gtk_widget_show(ws);
	 }
      }
   } else {
      std::cout << "ERROR: Couldn't get txt in change_chain_id_by_widget\n";
   }
}

void change_chain_id(int imol, const char *from_chain_id, const char *to_chain_id, 
		     short int use_res_range_flag, int from_resno, int to_resno) {

   std::pair<int, std::string> r = 
      graphics_info_t::molecules[imol].change_chain_id(from_chain_id,
						       to_chain_id,
						       use_res_range_flag,
						       from_resno,
						       to_resno);
} 

#ifdef USE_GUILE
SCM change_chain_id_with_result_scm(int imol, const char *from_chain_id, const char *to_chain_id,
				    short int use_res_range_flag, int from_resno, int to_resno){
   SCM r = SCM_EOL;
   std::pair<int, std::string> p = 
      graphics_info_t::molecules[imol].change_chain_id(from_chain_id,
						       to_chain_id,
						       use_res_range_flag,
						       from_resno,
						       to_resno);

   r = scm_cons(scm_makfrom0str(p.second.c_str()), r);
   r = scm_cons(SCM_MAKINUM(p.first), r);

   return r;
}
#endif // USE_GUILE


#ifdef USE_PYTHON
PyObject *change_chain_id_with_result_py(int imol, const char *from_chain_id, const char *to_chain_id, 
					 short int use_res_range_flag, int from_resno, int to_resno){

   std::pair<int, std::string> r = 
      graphics_info_t::molecules[imol].change_chain_id(from_chain_id,
						       to_chain_id,
						       use_res_range_flag,
						       from_resno,
						       to_resno);
   
   PyObject *v;
   v = PyList_New(2);
   PyList_SetItem(v, 0, PyInt_FromLong(r.first));
   PyList_SetItem(v, 1, PyString_FromString(r.second.c_str()));

   return v;

}
#endif // USE_PYTHON



/*  ----------------------------------------------------------------------- */
/*               Nomenclature Errors                                        */
/*  ----------------------------------------------------------------------- */
/* return the number of resides altered. */
int fix_nomenclature_errors(int imol) {

   int ifixed = 0;
   if (is_valid_model_molecule(imol)) {
      std::vector<CResidue *> vr =
	 graphics_info_t::molecules[imol].fix_nomenclature_errors();
      ifixed = vr.size();
      graphics_info_t g;
      g.update_geometry_graphs(graphics_info_t::molecules[imol].atom_sel,
			       imol);
      graphics_draw();
   }
   // update geometry graphs (not least rotamer graph).
   // but we have no intermediate atoms...
   
   return ifixed; 

} 

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
      g.statusbar_text(s);
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
      g.statusbar_text("Click on an atom in the residue that you want to flip");
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
      g.statusbar_text("Click on an atom in the fragment that you want to reverse");
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
	 int resno =  m_i_info.residues_with_missing_atoms[i]->GetSeqNum();
	 std::string chain_id = m_i_info.residues_with_missing_atoms[i]->GetChainID();
	 std::string residue_type = m_i_info.residues_with_missing_atoms[i]->GetResName();
	 std::string inscode = m_i_info.residues_with_missing_atoms[i]->GetInsCode();
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
      }
      //r = scm_reverse(r);
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

   if (is_valid_model_molecule(imol)) {
      std::string c(master_chain_id);
      graphics_info_t::molecules[imol].copy_residue_range_from_ncs_master_to_others(c,
										    res_range_start,
										    res_range_end);
      graphics_draw();
   }
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
      coot::helix_placement p(graphics_info_t::molecules[imol_map].xmap_list[0]);
      float min_density_limit = 1.0 * graphics_info_t::molecules[imol_map].map_sigma();
      // std::cout << "DEBUG:: choosing map_density limit: " << min_density_limit << std::endl;
      coot::helix_placement_info_t n =
	 p.place_alpha_helix_near_kc_version(pt, 20, min_density_limit);
       if (! n.success) {
	  n = p.place_alpha_helix_near_kc_version(pt, 9, min_density_limit);
       }

       if (n.success) {
	  float bf = graphics_info_t::default_new_atoms_b_factor;
	 atom_selection_container_t asc = make_asc(n.mol[0].pcmmdbmanager(bf));
	 imol = g.create_molecule();
	 graphics_info_t::molecules[imol].install_model(imol, asc, "Helix", 1);

	 if (n.mol.size() > 1) { 
	    atom_selection_container_t asc2 = make_asc(n.mol[1].pcmmdbmanager(bf));
	    imol = g.create_molecule();
	    graphics_info_t::molecules[imol].install_model(imol, asc2, "Reverse Helix", 1);
	 }
	 
	 if (g.go_to_atom_window) {
	    g.set_go_to_atom_molecule(imol);
	    g.update_go_to_atom_window_on_new_mol();
	 } else {
	    g.set_go_to_atom_molecule(imol);
	 }
	 g.statusbar_text("Helix added");
      } else {
	 std::cout << "Helix addition failure: message: " << n.failure_message << "\n";
	 g.statusbar_text(n.failure_message);
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
      g.statusbar_text("You need to set the map to fit against");
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
      coot::helix_placement p(graphics_info_t::molecules[imol_map].xmap_list[0]);
      coot::helix_placement_info_t si =
	 p.place_strand(pt, n_residues, n_sample_strands, s);
      if (si.success) {
	 // nice to refine the fragment here, but the interface
	 // doesn't work that way, so put the refinement after the
	 // molecule has been accepted.
	 float bf = graphics_info_t::default_new_atoms_b_factor;
	 atom_selection_container_t asc = make_asc(si.mol[0].pcmmdbmanager(bf));
	 imol = g.create_molecule();
	 graphics_info_t::molecules[imol].install_model(imol, asc, "Strand", 1);
	 g.statusbar_text("Strand added");

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
	 g.statusbar_text(si.failure_message);
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
      g.statusbar_text("You need to set the map to fit against");
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
   graphics_info_t g;
   clipper::Coord_orth pt(g.RotationCentre_x(),
			  g.RotationCentre_y(),
			  g.RotationCentre_z());
   int imol_map = g.Imol_Refinement_Map();
   if (imol_map != -1) {
      int imol = -1;
      coot::fast_secondary_structure_search ssfind;
      ssfind( graphics_info_t::molecules[imol_map].xmap_list[0], pt,
	      0.0, 7, coot::fast_secondary_structure_search::ALPHA3 );
      if (ssfind.success) {
	 float bf = graphics_info_t::default_new_atoms_b_factor;
	 atom_selection_container_t asc = make_asc(ssfind.mol.pcmmdbmanager(bf));
	 imol = g.create_molecule();
	 graphics_info_t::molecules[imol].install_model(imol,asc,"Helices",1);
	 g.molecules[imol].ca_representation();
	 if (g.go_to_atom_window) {
	    g.set_go_to_atom_molecule(imol);
	    g.update_go_to_atom_window_on_new_mol();
	 } else {
	    g.set_go_to_atom_molecule(imol);
	 }
	 g.statusbar_text("Helices added");
      } else {
	 std::cout << "No secondary structure found\n";
	 g.statusbar_text("No secondary structure found" );
      }
      std::vector<std::string> command_strings;
      command_strings.resize(0);
      command_strings.push_back("find-helices");
      add_to_history(command_strings);
      graphics_draw();
      return imol;
   } else {
      std::cout << " You need to set the map to fit against\n";
      g.statusbar_text("You need to set the map to fit against");
      g.show_select_map_dialog();
      return -1;
   }
} 


/*  ----------------------------------------------------------------------- */
/*                  Autofind Strands                                        */
/*  ----------------------------------------------------------------------- */
/* Find secondary structure in the current map.
   Add to a molecule called "Strands", create it if
   needed. */
int find_strands() {
   graphics_info_t g;
   clipper::Coord_orth pt(g.RotationCentre_x(),
			  g.RotationCentre_y(),
			  g.RotationCentre_z());
   int imol_map = g.Imol_Refinement_Map();
   if (imol_map != -1) {
      int imol = -1;
      coot::fast_secondary_structure_search ssfind;
      ssfind( graphics_info_t::molecules[imol_map].xmap_list[0], pt,
	      0.0, 7, coot::fast_secondary_structure_search::BETA3 );
      if (ssfind.success) {
	 float bf = graphics_info_t::default_new_atoms_b_factor;
	 atom_selection_container_t asc = make_asc(ssfind.mol.pcmmdbmanager(bf));
	 imol = g.create_molecule();
	 graphics_info_t::molecules[imol].install_model(imol,asc,"Strands",1);
	 g.molecules[imol].ca_representation();
	 if (g.go_to_atom_window) {
	    g.set_go_to_atom_molecule(imol);
	    g.update_go_to_atom_window_on_new_mol();
	 } else {
	    g.set_go_to_atom_molecule(imol);
	 }
	 g.statusbar_text("Strands added");
      } else {
	 std::cout << "No secondary structure found\n";
	 g.statusbar_text("No secondary structure found" );
      }
      std::vector<std::string> command_strings;
      command_strings.resize(0);
      command_strings.push_back("find-strands");
      add_to_history(command_strings);
      graphics_draw();
      return imol;
   } else {
      std::cout << " You need to set the map to fit against\n";
      g.statusbar_text("You need to set the map to fit against");
      g.show_select_map_dialog();
      return -1;
   }
}


/*  ----------------------------------------------------------------------- */
/*                  FFfearing                                               */
/*  ----------------------------------------------------------------------- */


// return the new model number
// 
int fffear_search(int imol_model, int imol_map) {

   float angular_resolution = graphics_info_t::fffear_angular_resolution;
   int imol_new = -1;
   if (is_valid_model_molecule(imol_model)) {
      if (is_valid_map_molecule(imol_map)) { 
	 coot::util::fffear_search f(graphics_info_t::molecules[imol_model].atom_sel.mol,
				     graphics_info_t::molecules[imol_model].atom_sel.SelectionHandle,
				     graphics_info_t::molecules[imol_map].xmap_list[0],
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
	 CMMDBManager *mol = g.molecules[imol].atom_sel.mol;
	 std::string atom_selection_str(atom_selection_string);
	 std::pair<coot::minimol::molecule, coot::minimol::molecule> p = 
	    coot::make_mols_from_atom_selection_string(mol, atom_selection_str, fill_mask);
   
	 g.rigid_body_fit(p.first,   // without selected region.
			  p.second,  // selected region.
			  imol_ref_map,
			  mask_waters_flag);
      }
   }
}


void fill_option_menu_with_refine_options(GtkWidget *option_menu) { 

   graphics_info_t g;

   g.fill_option_menu_with_map_options(option_menu, 
				       GTK_SIGNAL_FUNC(graphics_info_t::refinement_map_select));
}

void
set_rigid_body_fit_acceptable_fit_fraction(float f) {
   if (f >= 0.0 && f<= 1.0) { 
      graphics_info_t::rigid_body_fit_acceptable_fit_fraction = f;
   } else {
      std::cout << "ignoring set_rigid_body_fit_acceptable_fit_fraction"
		<< " of " << f << std::endl;
   } 
} 

/*  ----------------------------------------------------------------------- */
//                                 refinement
/*  ----------------------------------------------------------------------- */

void set_matrix(float f) {

   graphics_info_t::geometry_vs_map_weight = f;
}

float matrix_state() {
   return graphics_info_t::geometry_vs_map_weight;
}

void set_refine_auto_range_step(int i) { 
   graphics_info_t::refine_auto_range_step = i;
} 

void set_refine_max_residues(int n) { 
   graphics_info_t::refine_regularize_max_residues = n;
}

// (refine-zone-atom-index-define 0 688 688)
// 
void refine_zone_atom_index_define(int imol, int ind1, int ind2) {
   
   graphics_info_t g;

   if (is_valid_model_molecule(imol)) {
      if (g.molecules[imol].has_model()) {
	 if (g.molecules[imol].atom_sel.n_selected_atoms > ind1 &&
	     g.molecules[imol].atom_sel.n_selected_atoms > ind2) {
	    g.refine(imol, 0, ind1, ind2);
	 } else {
	    std::cout << "WARNING: atom index error in "
		      << "refine_zone_atom_index_define\n";
	 }
      } else {
	 std::cout << "WARNING: no model for molecule " << imol << " in "
		   << "refine_zone_atom_index_define\n";
      }
   } else {
      std::cout << "WARNING: no molecule " << imol << " in "
		<< "refine_zone_atom_index_define\n";
   }
}

void refine_zone(int imol, const char *chain_id,
		 int resno1,
		 int resno2,
		 const char *altconf) {

   refine_zone_with_full_residue_spec(imol, chain_id, resno1, "", resno2, "", altconf);
}


void refine_zone_with_full_residue_spec(int imol, const char *chain_id,
					int resno1,
					const char *inscode_1,
					int resno2,
					const char *inscode_2,
					const char *altconf) {

   if (imol >= 0) {
      if (is_valid_model_molecule(imol)) {
	 graphics_info_t g;
// 	 int index1 = atom_index(imol, chain_id, resno1, " CA ");
// 	 int index2 = atom_index(imol, chain_id, resno2, " CA ");
	 // the "" is the insertion code (not passed to this function (yet)

	 int index1 = graphics_info_t::molecules[imol].atom_index_first_atom_in_residue(chain_id, resno1, inscode_1, altconf); 
	 int index2 = graphics_info_t::molecules[imol].atom_index_first_atom_in_residue(chain_id, resno2, inscode_2, altconf);

	 short int auto_range = 0;
	 if (index1 >= 0) {
	    if (index2 >= 0) { 
	       g.refine(imol, auto_range, index1, index2);
	    } else {
	       std::cout << "WARNING:: refine_zone: Can't get index for resno2: "
			 << resno2 << std::endl;
	    } 
	 } else {
	    std::cout << "WARNING:: refine_zone: Can't get index for resno1: "
		      << resno1 << std::endl;
	 }
      } else {
	 std::cout << "Not a valid model molecule" << std::endl;
      }
   }
}

void refine_auto_range(int imol, const char *chain_id, int resno1, const char *altconf) {


   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      int index1 = atom_index(imol, chain_id, resno1, " CA ");
      short int auto_range = 1;
      if (index1 >= 0) { 
	 g.refine(imol, auto_range, index1, index1);
      } else {
	 std::cout << "WARNING:: refine_auto_range: Can't get index for resno1: "
		   << resno1 << std::endl;
      }
   }
}

/*! \brief regularize a zone
  */
void regularize_zone(int imol, const char *chain_id, int resno1, int resno2, const char *altconf) {
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      // the "" is the insertion code (not passed to this function (yet)
      int index1 = graphics_info_t::molecules[imol].atom_index_first_atom_in_residue(chain_id, resno1, ""); 
      int index2 = graphics_info_t::molecules[imol].atom_index_first_atom_in_residue(chain_id, resno2, "");
      short int auto_range = 0;
      if (index1 >= 0) {
	 if (index2 >= 0) { 
	    g.regularize(imol, auto_range, index1, index2);
	 } else {
	    std::cout << "WARNING:: regularize_zone: Can't get index for resno2: "
		      << resno2 << std::endl;
	 } 
      } else {
	 std::cout << "WARNING:: regularize_zone: Can't get index for resno1: "
		   << resno1 << std::endl;
      }
   } else {
      std::cout << "Not a valid model molecule" << std::endl;
   }
} 


// This does not control if the atoms are accepted immediately, just
// whether the Accept Refinemnt gui is shown.
// 
void set_refinement_immediate_replacement(int istate) {
   graphics_info_t::refinement_immediate_replacement_flag = istate;
}

int  refinement_immediate_replacement_state() {
   return graphics_info_t::refinement_immediate_replacement_flag; 
} 


int imol_refinement_map() {

   graphics_info_t g;
   return g.Imol_Refinement_Map();

}

int set_imol_refinement_map(int imol) {

   int r = -1; 
   if (is_valid_map_molecule(imol)) { 
      graphics_info_t g;
      r = g.set_imol_refinement_map(imol);
   }
   return r;
}

/*  ----------------------------------------------------------------------- */
/*                  regularize/refine                                       */
/*  ----------------------------------------------------------------------- */

void do_regularize(short int state) { 

   //
   graphics_info_t g; 

   g.set_in_range_define_for_regularize(state);  // TRUE or FALSE
   if (state) { 
      g.untoggle_model_fit_refine_buttons_except("model_refine_dialog_regularize_zone_togglebutton");
      // and kill the delete dialog if it is there
      if (g.delete_item_widget) { 
	 gtk_widget_destroy(g.delete_item_widget);
	 g.delete_item_widget = NULL;
	 // hopefully superfluous:
	 g.delete_item_atom = 0;
	 g.delete_item_residue = 0;
	 g.delete_item_residue_hydrogens = 0;
      }
      std::cout << "click on 2 atoms (in the same molecule)" << std::endl; 
      g.pick_cursor_maybe();
      g.pick_pending_flag = 1;
   } else { 
      g.normal_cursor();
   }
}

void do_refine(short int state) { 

   //
   graphics_info_t g; 

   g.set_in_range_define_for_refine(state);  // TRUE or Not...

   // std::cout << "DEBUG:: in do_refine" << std::endl;
   
   if (state) { 
      g.untoggle_model_fit_refine_buttons_except("model_refine_dialog_refine_togglebutton");
      // and kill the delete dialog if it is there
      if (g.delete_item_widget) { 
	 gtk_widget_destroy(g.delete_item_widget);
	 g.delete_item_widget = NULL;
	 // hopefully superfluous:
	 g.delete_item_atom = 0;
	 g.delete_item_residue = 0;
	 g.delete_item_residue_hydrogens = 0;
      }
      
      int imol_map = g.Imol_Refinement_Map();
      // std::cout << "DEBUG:: in do_refine, imol_map: " << imol_map << std::endl;
      if (imol_map >= 0) {
	 if (g.molecules[imol_map].has_map()) { 
	    std::cout << "click on 2 atoms (in the same molecule)" << std::endl; 
	    g.pick_cursor_maybe();
	    g.pick_pending_flag = 1;
	    std::string s = "Pick 2 atoms or Autozone (pick 1 atom the press the A key)";
	    s += " [Ctrl Left-mouse rotates the view]";
	    s += "...";
	    g.statusbar_text(s);
	 } else {
	    g.show_select_map_dialog();
	    g.in_range_define_for_refine = 0;
	    g.model_fit_refine_unactive_togglebutton("model_refine_dialog_refine_togglebutton");
	 }
      } else {
	 // map chooser dialog
	 g.show_select_map_dialog();
	 g.in_range_define_for_refine = 0;
	 g.model_fit_refine_unactive_togglebutton("model_refine_dialog_refine_togglebutton");
      }
   } else { 
      g.normal_cursor();
      g.in_range_define_for_refine = 0;
      // g.pick_pending_flag = 0;
   }

}

void set_residue_selection_flash_frames_number(int i) {

   graphics_info_t::residue_selection_flash_frames_number = i;
}

void set_refinement_refine_per_frame(int i) {

   graphics_info_t::dragged_refinement_refine_per_frame_flag = i; 
}

int  refinement_refine_per_frame_state() {

   return graphics_info_t::dragged_refinement_refine_per_frame_flag;
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
      CMMDBManager *mol_orig = graphics_info_t::molecules[imol_orig].atom_sel.mol;
      int SelectionHandle = mol_orig->NewSelection();
      mol_orig->SelectAtoms(SelectionHandle, 0, "*",
			    ANY_RES, "*",
			    ANY_RES, "*",
			    (char *) residue_type,
			    "*", "*", "*");
      CMMDBManager *mol =
	 coot::util::create_mmdbmanager_from_atom_selection(mol_orig, SelectionHandle);

      if (mol) {
	 std::string name = "residue type ";
	 name += residue_type;
	 name += " from ";
	 name += graphics_info_t::molecules[imol_orig].name_for_display_manager();
	 atom_selection_container_t asc = make_asc(mol);
	 if (asc.n_selected_atoms > 0) {
	   graphics_info_t::molecules[imol].install_model(imol, asc, name, 1);
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
      CMMDBManager *mol_orig = graphics_info_t::molecules[imol_orig].atom_sel.mol;
      int SelectionHandle = mol_orig->NewSelection();
      mol_orig->Select(SelectionHandle, STYPE_ATOM,
		       (char *) atom_selection_str, 
		       SKEY_OR);
      CMMDBManager *mol =
	 coot::util::create_mmdbmanager_from_atom_selection(mol_orig,
							    SelectionHandle);

      { // debug code 
	 int imod = 1;
	 
	 CModel *model_p = mol->GetModel(imod);
	 CChain *chain_p;
	 // run over chains of the existing mol
	 int nchains = model_p->GetNumberOfChains();
	 for (int ichain=0; ichain<nchains; ichain++) {
	    chain_p = model_p->GetChain(ichain);
	    std::cout << "In new_molecule_by_atom_selection has contains chain id "
		      << chain_p->GetChainID() << std::endl;
	 }
      }

      if (mol) {
	 std::string name = "atom selection from ";
	 name += graphics_info_t::molecules[imol_orig].name_for_display_manager();
	 atom_selection_container_t asc = make_asc(mol);
	 if (asc.n_selected_atoms > 0){ 
	    graphics_info_t::molecules[imol].install_model(imol, asc, name, 1);
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
	 // good CMMDBManager pointer.
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

int new_molecule_by_sphere_selection(int imol_orig, float x, float y, float z, float r) {

   int imol = -1;
   if (is_valid_model_molecule(imol_orig)) {
      imol = graphics_info_t::create_molecule();
      CMMDBManager *mol_orig = graphics_info_t::molecules[imol_orig].atom_sel.mol;
      int SelectionHandle = mol_orig->NewSelection();
      mol_orig->SelectSphere(SelectionHandle, STYPE_ATOM,
			     x, y, z, r, SKEY_OR);
      CMMDBManager *mol =
	 coot::util::create_mmdbmanager_from_atom_selection(mol_orig,
							    SelectionHandle);
      if (mol) {
	 std::string name = "sphere selection from ";
	 name += graphics_info_t::molecules[imol_orig].name_for_display_manager();
	 atom_selection_container_t asc = make_asc(mol);
	 if (asc.n_selected_atoms > 0){ 
	    graphics_info_t::molecules[imol].install_model(imol, asc, name, 1);
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
	 // good CMMDBManager pointer.
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
int read_shelx_ins_file(const char *filename) {

   int istat = -1;
   graphics_info_t g;
   if (filename) { 
      int imol = graphics_info_t::create_molecule();

      // 20080518 Why did I want to not centre on a new shelx
      // molecule? I forget now.  So comment out these lines of code
      // and the resetting of recentre_on_read_pdb flag later.
      
//       short int reset_centre_flag = g.recentre_on_read_pdb;
//       g.recentre_on_read_pdb = 0;
      istat = g.molecules[imol].read_shelx_ins_file(std::string(filename));
      if (istat != 1) {
	 graphics_info_t::erase_last_molecule();
	 std::cout << "WARNING:: " << istat << " on read_shelx_ins_file "
		   << filename << std::endl;
      } else {
	 std::cout << "Molecule " << imol << " read successfully\n";
	 istat = imol; // for return status 
	 if (g.go_to_atom_window) {
	    g.set_go_to_atom_molecule(imol);
	    g.update_go_to_atom_window_on_new_mol();
	 }
	 graphics_draw();
	 std::vector<std::string> command_strings;
	 command_strings.push_back("read-shelx-ins-file");
	 command_strings.push_back(single_quote(coot::util::intelligent_debackslash(filename)));
	 add_to_history(command_strings);
      }
      //       g.recentre_on_read_pdb = reset_centre_flag;
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
	 g.statusbar_text(stat.second);
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

#if defined USE_GUILE && !defined WINDOWS_MINGW
   safe_scheme_command("(smiles-gui)");
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

void pepflip(int imol, const char *chain_id, int resno, const char *inscode) { /* the residue with CO,
							   for scripting interface. */

   if (is_valid_model_molecule(imol)) { 
      graphics_info_t g;
      g.molecules[imol].pepflip_residue(resno, std::string(""),
					std::string(chain_id));
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
void handle_cif_dictionary(const char *filename) {

   graphics_info_t g;
   g.add_cif_dictionary(filename, 1); // show dialog if no bonds

}

void read_cif_dictionary(const char *filename) { 
   
   handle_cif_dictionary(filename);

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
my_delete_ramachandran_mol_option(GtkWidget *widget, void *data) {
   gtk_container_remove(GTK_CONTAINER(data), widget);
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

/*  ----------------------------------------------------------------------- */
/*                  get molecule by libcheck/refmac code                    */
/*  ----------------------------------------------------------------------- */

/* Libcheck monomer code */
void 
handle_get_libcheck_monomer_code(GtkWidget *widget) { 

   const gchar *text = gtk_entry_get_text(GTK_ENTRY(widget));
   std::cout << "Refmac monomer Code: " << text << std::endl;
   get_monomer(text);

   // and kill the libcheck code window
   GtkWidget *window = lookup_widget(GTK_WIDGET(widget), "libcheck_monomer_dialog");
   if (window)
      gtk_widget_destroy(window);
   else 
      std::cout << "failed to lookup window in handle_get_libcheck_monomer_code" 
		<< std::endl;
}

// Return the new molecule number, or else a negitive error code.
// 
int get_monomer(const char *three_letter_code) {

   int imol = -1;

#ifdef USE_GUILE
   string scheme_command;

   scheme_command = "(monomer-molecule-from-3-let-code \"";

   scheme_command += three_letter_code;
   scheme_command += "\"";

   // now add in the bespoke cif library if it was given.  It is
   // ignored in the libcheck script if cif_lib_filename is "".
   //
   // However, we only want to pass the bespoke cif library if the
   // monomer to be generated is in the cif file.
   std::string cif_lib_filename = "";
   if (graphics_info_t::cif_dictionary_filename_vec->size() > 0) {
      std::string dict_name = (*graphics_info_t::cif_dictionary_filename_vec)[0];
      coot::simple_cif_reader r(dict_name);
      if (r.has_restraints_for(three_letter_code))
	 cif_lib_filename = dict_name;
   }

   scheme_command += " ";
   std::string quoted_cif_lib_filename = single_quote(cif_lib_filename);
   scheme_command += quoted_cif_lib_filename;

   if (graphics_info_t::libcheck_ccp4i_project_dir != "") { 
      scheme_command += " ";
      scheme_command += single_quote(graphics_info_t::libcheck_ccp4i_project_dir);
   }

   scheme_command += ")";

   SCM v = safe_scheme_command(scheme_command);

   int was_int_flag = gh_scm2bool(scm_integer_p(v));

#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)      
   if (was_int_flag)
      imol = scm_to_int(v);
#else    
   if (was_int_flag)
      imol = gh_scm2int(v);
#endif // SCM version   
   

#else 
   
#ifdef USE_PYTHON
   string python_command;

   python_command = "monomer_molecule_from_3_let_code(\"";

   python_command += three_letter_code;
   python_command += "\"";

   // now add in the bespoke cif library if it was given
   // ignored in the libcheck script if cif_lib_filename is "".
   //
   // However, we only want to pass the bespoke cif library if the
   // monomer to be generated is in the cif file.
   std::string cif_lib_filename = "";
   if (graphics_info_t::cif_dictionary_filename_vec->size() > 0) {
      std::string dict_name = (*graphics_info_t::cif_dictionary_filename_vec)[0];
      coot::simple_cif_reader r(dict_name);
      if (r.has_restraints_for(three_letter_code))
         cif_lib_filename = dict_name;
   }

   python_command += ",";
   std::string quoted_cif_lib_filename = single_quote(coot::util::intelligent_debackslash(cif_lib_filename));
   python_command += quoted_cif_lib_filename;
   python_command += ",";

   std::string cif_lib_dirname = "";
   if (graphics_info_t::libcheck_ccp4i_project_dir != "") {
      cif_lib_dirname = (graphics_info_t::libcheck_ccp4i_project_dir);
   }
   std::string quoted_cif_lib_dirname = single_quote(coot::util::intelligent_debackslash(cif_lib_dirname));
   python_command += quoted_cif_lib_dirname;
   python_command += ")";

   PyObject *v = safe_python_command_with_return(python_command);

   int was_int_flag = PyInt_AsLong(v);

//   std::cout << "BL DEBUG:: was_int_flag is " << was_int_flag << std::endl;

   if (was_int_flag)
      imol = was_int_flag;
   
// BL says: I guess I've done it now..., at least sort of
//   std::cout << "not compiled with guile.  This won't work \n"
//          << "Need function to be coded in python..." << std::endl; 
#endif // USE_PYTHON

#endif // USE_GUILE

   std::vector<std::string> command_strings;
   command_strings.push_back("get-monomer");
   command_strings.push_back(coot::util::single_quote(three_letter_code));
   add_to_history(command_strings);

   return imol;
}

/* Use the protein geometry dictionary to retrieve a set of
   coordinates quickly.  There are no restraints from this method
   though. */
int get_monomer_from_dictionary(const char *three_letter_code,
				int idealised_flag) {

   int istat = 0;
   graphics_info_t g;
   CMMDBManager *mol = g.Geom_p()->mol_from_dictionary(three_letter_code, idealised_flag);
   if (mol) {
      int imol = graphics_info_t::create_molecule();
      atom_selection_container_t asc = make_asc(mol);
      std::string name = three_letter_code;
      name += "_from_dict";
      graphics_info_t::molecules[imol].install_model(imol, asc, name, 1);
      move_molecule_to_screen_centre_internal(imol);
      graphics_draw();
      istat = imol;
   }
   return istat;
}


// not the write place for this function.  c-interface-map.cc would be better.
int laplacian (int imol) {

   int iret = -1;
   if (is_valid_map_molecule(imol)) {
      clipper::Xmap<float> xmap = coot::util::laplacian_transform(graphics_info_t::molecules[imol].xmap_list[0]);
      int new_molecule_number = graphics_info_t::create_molecule();
      std::string label = "Laplacian of ";
      label += graphics_info_t::molecules[imol].name_;
      graphics_info_t::molecules[new_molecule_number].new_map(xmap, label);
      iret = new_molecule_number;
   }
   return iret;
}



/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/*                               skeleton                                   */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

GtkWidget *
create_skeleton_colour_selection_window() { 

   GtkWidget  *colorseldialog;
   GtkWidget *colorsel;

   colorseldialog = 
      gtk_color_selection_dialog_new("Skeleton Colour Selection"); 

/* How do we get to the buttons? */

   colorsel = GTK_COLOR_SELECTION_DIALOG(colorseldialog)->colorsel;

  /* Capture "color_changed" events in col_sel_window */

  gtk_signal_connect (GTK_OBJECT (colorsel), "color_changed",
                      (GtkSignalFunc)on_skeleton_color_changed, 
		      (gpointer)colorsel);
  
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(colorseldialog)->
				ok_button), "clicked",
		     GTK_SIGNAL_FUNC(on_skeleton_col_sel_cancel_button_clicked),
		     colorseldialog);

  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(colorseldialog)->
				cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(on_skeleton_col_sel_cancel_button_clicked), 
		     colorseldialog);

  gtk_color_selection_set_color(GTK_COLOR_SELECTION(colorsel),
				graphics_info_t::skeleton_colour);

  return GTK_WIDGET(colorseldialog);

}


void
show_partial_charge_info(int imol, const char *chain_id, int resno, const char *ins_code) {

   if (is_valid_model_molecule(imol)) {
      CResidue *residue =
	 graphics_info_t::molecules[imol].get_residue(resno, ins_code, chain_id);
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