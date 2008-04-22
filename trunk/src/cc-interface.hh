/* src/c-interface.h
 * 
 * Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
 * Copyright 2008 by The University of Oxford
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
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef CC_INTERFACE_HH
#define CC_INTERFACE_HH

#include "coot-coord-utils.hh"

namespace coot {

   class alias_path_t {
   public:
      int index;
      std::string s;
      bool flag;
      alias_path_t(int index_in, const std::string &s_in, bool flag_in) {
	 index = index_in;
	 s = s_in;
	 flag = flag_in;
      }
   };
}


std::vector<std::string> filtered_by_glob(const std::string &pre_directory, 
					  int data_type);
/*  Return 1 if search appears in list, 0 if not) */
short int 
string_member(const std::string &search, const std::vector<std::string> &list); 
bool compare_strings(const std::string &a, const std::string &b); 


std::string pre_directory_file_selection(GtkWidget *sort_button);
void filelist_into_fileselection_clist(GtkWidget *fileselection, const std::vector<std::string> &v);

GtkWidget *wrapped_nothing_bad_dialog(const std::string &label);

std::pair<short int, float> float_from_entry(GtkWidget *entry);
std::pair<short int, int>   int_from_entry(GtkWidget *entry);

void
add_validation_mol_menu_item(int imol, const std::string &name, GtkWidget *menu, GtkSignalFunc callback);
void create_initial_validation_graph_submenu_generic(GtkWidget *window1,
						     const std::string &menu_name,
						     const std::string &sub_menu_name);

std::string probe_dots_short_contact_name_to_expanded_name(const std::string &short_name);


/*  ---------------------------------------------------------------------- */
/*                       go to atom   :                                    */
/*  ---------------------------------------------------------------------- */

#ifdef USE_GUILE
// Bernie, no need to pythonize this, it's just to test the return
// values on pressing "next residue" and "previous residue" (you can
// if you wish of course).
//
// Pass the current values, return new values
SCM goto_next_atom_maybe(const char *chain_id, int resno, const char *ins_code, const char *atom_name);
SCM goto_prev_atom_maybe(const char *chain_id, int resno, const char *ins_code, const char *atom_name);
#endif 

#ifdef USE_PYTHON
// but I 'want' to! Needed for python unittest!
#include "Python.h"
PyObject *goto_next_atom_maybe_py(const char *chain_id, int resno, const char *ins_code, const char *atom_name);
PyObject *goto_prev_atom_maybe_py(const char *chain_id, int resno, const char *ins_code, const char *atom_name);
#endif

int set_go_to_atom_from_spec(const coot::atom_spec_t &atom_spec);

// This is to make porting the active atom more easy for Bernhard.
// Return a class rather than a list, and rewrite the active-residue
// function use this atom-spec.  The first value of the pair indicates
// if an atom spec was found.
//
std::pair<bool, std::pair<int, coot::atom_spec_t> > active_atom_spec();


/*  ---------------------------------------------------------------------- */
/*                       symmetry functions:                               */
/*  ---------------------------------------------------------------------- */
// get the symmetry operators strings for the given molecule
//
#ifdef USE_GUILE

/*! \brief Return as a list of strings the symmetry operators of the
  given molecule. If imol is a not a valid molecule, return an empty
  list.*/
SCM get_symmetry(int imol);
#endif // USE_GUILE

#ifdef USE_PYTHON
// return a python object as a list (or some other python container)
PyObject *get_symmetry_py(int imol);
#endif // USE_PYTHON

/*  ---------------------------------------------------------------------- */
/*                       map functions:                                    */
/*  ---------------------------------------------------------------------- */
void add_map_colour_mol_menu_item(int imol, const std::string &name,
				  GtkWidget *sub_menu, GtkSignalFunc callback);
/* Actually this function is generic and could be renamed so. */
void add_map_scroll_wheel_mol_menu_item(int imol, 
					const std::string &name,
					GtkWidget *menu, 
					GtkSignalFunc callback);

//! \brief return the colour of the imolth map (e.g.: (list 0.4 0.6
//0.8). If invalid imol return #f.
// 
#ifdef USE_GUILE
SCM map_colour_components(int imol);
#endif // GUILE

#ifdef USE_PYTHON
// \brief return the colour of the imolth map (e.g.: [0.4, 0.6,
// 0.8]. If invalid imol return Py_False.
//
PyObject *map_colour_components_py(int imol);
#endif // PYTHON

/*  ------------------------------------------------------------------------ */
/*                         refmac stuff                                      */
/*  ------------------------------------------------------------------------ */
// if swap_map_colours_post_refmac_flag is not 1 thenn imol_refmac_map is ignored.
// 
void
execute_refmac_real(std::string pdb_in_filename,
		    std::string pdb_out_filename,
		    std::string mtz_in_filename,
		    std::string mtz_out_filename,
		    std::string cif_lib_filename, /* use "" for none */
		    std::string fobs_col_name,
		    std::string sigfobs_col_name,
		    std::string r_free_col_name,
		    short int have_sensible_free_r_flag,
		    std::string refmac_count_string, 
		    int swap_map_colours_post_refmac_flag,
		    int imol_refmac_map,
		    int diff_map_flag,
		    int phase_combine_flag,
		    std::string phib_string,
		    std::string fom_string,
		    std::string ccp4i_project_dir);


/*  ------------------------------------------------------------------- */
/*                    file selection                                    */
/*  ------------------------------------------------------------------- */

namespace coot {
   class str_mtime {
   public:
      str_mtime(std::string file_in, time_t mtime_in) {
	 mtime = mtime_in;
	 file = file_in;
      }
      str_mtime() {}
      time_t mtime;
      std::string file;
   };
   
   // trivial helper function
   class file_attribs_info_t {
   public:
      std::string directory_prefix;
      std::vector<str_mtime> file_mtimes;
   };
}


bool compare_mtimes(coot::str_mtime a, coot::str_mtime b); 

std::vector<std::pair<std::string, std::string> > parse_ccp4i_defs(const std::string &filename);

std::string ccp4_project_directory(const std::string &ccp4_project_name);

/*  -------------------------------------------------------------------- */
/*                     history                                           */
/*  -------------------------------------------------------------------- */

namespace coot { 
   class command_arg_t {
   public:
      enum coot_script_arg_type{INT, FLOAT, STRING};
      command_arg_t(int iin) {
	 i = iin;
	 type = INT;
      }
      command_arg_t(float fin) {
	 f = fin;
	 type = FLOAT;
      }
      command_arg_t(const clipper::String &sin) {
	 s = sin;
	 type = STRING;
      }
      command_arg_t(const std::string &sin) {
	 s = sin;
	 type = STRING;
      }
      command_arg_t(const char *sin) {
	 s = sin;
	 type = STRING;
      }
      coot_script_arg_type type;
      float f;
      int i;
      clipper::String s;
      std::string as_string() const {
	 std::string os("unknown-arg-type");
	 if (type == INT)
	    os = clipper::String(i);
	 if (type == FLOAT)
	    os = clipper::String(f);
	 if (type == STRING)
	    os = s;
	 return os;
      }
   };
}
void add_to_history(const std::vector<std::string> &ls);
void add_to_history_simple(const std::string &cmd);
void add_to_history_typed(const std::string &command,
			  const std::vector<coot::command_arg_t> &args);
std::string single_quote(const std::string &s);
std::string pythonize_command_name(const std::string &s);
std::string schemize_command_name(const std::string &s);
std::string languagize_command(const std::vector<std::string> &command_parts);

void add_to_database(const std::vector<std::string> &command_strings);


/*  ----------------------------------------------------------------------- */
/*                         Merge Molecules                                  */
/*  ----------------------------------------------------------------------- */
// return the status and vector of chain-ids of the new chain ids.
// 
std::pair<int, std::vector<std::string> > merge_molecules_by_vector(const std::vector<int> &add_molecules, int imol);

/*  ----------------------------------------------------------------------- */
/*                         Dictionaries                                     */
/*  ----------------------------------------------------------------------- */
/*! \brief return a list of all the dictionaries read */
#ifdef USE_GUILE
SCM dictionaries_read();
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *dictionaries_read_py();
#endif // PYTHON

/*  ----------------------------------------------------------------------- */
/*                         Restraints                                       */
/*  ----------------------------------------------------------------------- */
#ifdef USE_GUILE
SCM monomer_restraints(const char *monomer_type);
SCM set_monomer_restraints(const char *monomer_type, SCM restraints);
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *monomer_restraints_py(const char *monomer_type);
PyObject *set_monomer_restraints_py(const char *monomer_type, PyObject *restraints);
#endif // USE_PYTHON


/*  ----------------------------------------------------------------------- */
/*                  scripting                                               */
/*  ----------------------------------------------------------------------- */

#ifdef USE_GUILE
SCM safe_scheme_command_test(const char *cmd);
SCM safe_scheme_command(const std::string &scheme_command);
#else 
void safe_scheme_command(const std::string &scheme_command); /* do nothing */
#endif // USE_GUILE

#ifdef USE_PYTHON
void safe_python_command(const std::string &python_command); 
void safe_python_command_by_char_star(const char *python_command);
PyObject *py_clean_internal(PyObject *obj);
PyObject *safe_python_command_with_return(const std::string &python_cmd);
PyObject *safe_python_command_test(const char *cmd);
#endif // PYTHON
/*  Is this a repeat of something?  I don't know. */
void run_generic_script(const std::vector<std::string> &cmd_strings);

/* commands to run python commands from guile and vice versa */
/* we ignore return values for now */
#ifdef USE_PYTHON
PyObject *run_scheme_command(const char *scheme_command);
#endif // USE_PYTHON
#ifdef USE_GUILE
SCM run_python_command(const char *python_command);
#endif // USE_GUILE

#ifdef USE_GUILE
// Return a list describing a residue like that returned by
// residues-matching-criteria (list return-val chain-id resno ins-code)
// This is a library function really.  There should be somewhere else to put it.
// It doesn't need expression at the scripting level.
// return a null list on problem
SCM scm_residue(const coot::residue_spec_t &res);
#endif

#ifdef USE_PYTHON
// Return a list describing a residue like that returned by
// residues-matching-criteria [return_val, chain_id, resno, ins_code]
// This is a library function really.  There should be somewhere else to put it.
// It doesn't need expression at the scripting level.
// return a null list on problem
PyObject *py_residue(const coot::residue_spec_t &res);
#endif

/*  ----------------------------------------------------------------------- */
/*               Atom info                                                  */
/*  ----------------------------------------------------------------------- */

/*! \brief output atom info in a scheme list for use in scripting:

in this format (list occ temp-factor element x y z).  Return empty
list if atom not found. */
#ifdef USE_GUILE
const char *atom_info_string(int imol, const char *chain_id, int resno,
			     const char *ins_code, const char *atname,
			     const char *altconf);

//! \brief
// Return a list of atom info for each atom in the specified residue:
// 
// output is like this:
// (list
//    (list (list atom-name alt-conf)
//          (list occ temp-fact element)
//          (list x y z)))
// 
SCM residue_info(int imol, const char* chain_id, int resno, const char *ins_code);
SCM residue_name(int imol, const char* chain_id, int resno, const char *ins_code);

// And going the other way, given an s-expression, update
// molecule_number by the given molecule.  Clear what's currently
// there first though.
//
int clear_and_update_molecule(int molecule_number, SCM molecule_expression);
// return a molecule number, -1 on error
int add_molecule(SCM molecule_expression, const char *name);

//! \brief 
// Return a list of (list imol chain-id resno ins-code atom-name
// alt-conf) for atom that is closest to the screen centre in any
// displayed molecule.  If there are multiple models with the same
// coordinates at the screen centre, return the attributes of the atom
// in the highest number molecule number.
//
// return #f if no active residue
// 
SCM active_residue();

//! \brief
// 
// Return a list of (list imol chain-id resno ins-code atom-name
// alt-conf (list x y z)) for atom that is closest to the screen
// centre in the given molecule (unlike active-residue, no account is
// taken of the displayed state of the molecule).  If there is no
// atom, or if imol is not a valid model molecule, return #f.
// 
SCM closest_atom(int imol);

#endif	/* USE_GUILE */

/* Here the Python code for ATOM INFO */

/*! \brief output atom info in a python list for use in scripting:

in this format [occ, temp_factor, element, x, y, z].  Return empty
list if atom not found. */
#ifdef USE_PYTHON
const char *atom_info_string_py(int imol, const char *chain_id, int resno,
			     const char *ins_code, const char *atname,
			     const char *altconf);
//! \brief
// Return a list of atom info for each atom in the specified residue:
//
// output is like this:
// [
//     [[atom-name,alt-conf]
//      [occ,temp_fact,element]
//      [x,y,z]]]
//
PyObject *residue_info_py(int imol, const char* chain_id, int resno, const char *ins_code);
PyObject *residue_name_py(int imol, const char* chain_id, int resno, const char *ins_code);

// And going the other way, given an python-expression, update
// molecule_number by the given molecule.  Clear what's currently
// there first though.
//
int clear_and_update_molecule_py(int molecule_number, PyObject *molecule_expression);
// return a molecule number, -1 on error
int add_molecule_py(PyObject *molecule_expression, const char *name);

//! \brief
// Return a list of [imol, chain-id, resno, ins-code, atom-name,
// alt-conf] for atom that is closest to the screen centre.  If there
// are multiple models with the same coordinates at the screen centre,
// return the attributes of the atom in the highest number molecule
// number.
//
// return #f if no active residue
//
PyObject *active_residue_py();

//! \brief
// 
// Return a list of [imol, chain-id, resno, ins-code, atom-name,
// alt-conf, [x, y, z]] for atom that is closest to the screen
// centre in the given molecule (unlike active-residue, no account is
// taken of the displayed state of the molecule).  If there is no
// atom, or if imol is not a valid model molecule, return #f.
// 
PyObject *closest_atom_py(int imol);
#endif // USE_PYTHON

/*  ----------------------------------------------------------------------- */
/*                  water chain                                             */
/*  ----------------------------------------------------------------------- */

#ifdef USE_GUILE
/*! return the chain id of the water chain from a shelx molecule.  Raw interface
  Return #f if no chain or bad imol*/
SCM water_chain_from_shelx_ins_scm(int imol); 
/*! return the chain id of the water chain. Raw interface */
SCM water_chain_scm(int imol);
#endif 

#ifdef USE_PYTHON
/*! return the chain id of the water chain from a shelx molecule.  Raw interface.

Return False if no chain or bad imol*/
PyObject *water_chain_from_shelx_ins_py(int imol); 
/*! return the chain id of the water chain. Raw interface */
PyObject *water_chain_py(int imol);
#endif 


/*  ----------------------------------------------------------------------- */
/*                  spin search                                             */
/*  ----------------------------------------------------------------------- */
void spin_search_by_atom_vectors(int imol_map, int imol, const std::string &chain_id, int resno, const std::string &ins_code, const std::pair<std::string, std::string> &direction_atoms_list, const std::vector<std::string> &moving_atoms_list);
#ifdef USE_GUILE
/*! \brief for the given residue, spin the atoms in moving_atom_list
  around the bond defined by direction_atoms_list looking for the best
  fit to density of imom_map map of the first atom in
  moving_atom_list.  Works (only) with atoms in altconf "" */
void spin_search(int imol_map, int imol, const char *chain_id, int resno, const char *ins_code, SCM direction_atoms_list, SCM moving_atoms_list);
#endif

#ifdef USE_PYTHON
/*! \brief for the given residue, spin the atoms in moving_atom_list
  around the bond defined by direction_atoms_list looking for the best
  fit to density of imom_map map of the first atom in
  moving_atom_list.  Works (only) with atoms in altconf "" */
void spin_search_py(int imol_map, int imol, const char *chain_id, int resno, const char *ins_code, PyObject *direction_atoms_list, PyObject *moving_atoms_list);
#endif 

/*  ----------------------------------------------------------------------- */
/*                  monomer lib                                             */
/*  ----------------------------------------------------------------------- */
std::vector<std::pair<std::string, std::string> > monomer_lib_3_letter_codes_matching(const std::string &search_string, short int allow_minimal_descriptions_flag);

void on_monomer_lib_search_results_button_press (GtkButton *button, gpointer user_data);

/*  ----------------------------------------------------------------------- */
/*                  mutate                                                  */
/*  ----------------------------------------------------------------------- */
int mutate_internal(int ires, const char *chain_id,
		    int imol, std::string &target_res_type);
/* a function for multimutate to make a backup and set
   have_unsaved_changes_flag themselves */

/*  ----------------------------------------------------------------------- */
/*                  ligands                                                 */
/*  ----------------------------------------------------------------------- */
std::vector<int> execute_ligand_search_internal();
coot::graph_match_info_t
overlap_ligands_internal(int imol_ligand, int imol_ref, const char *chain_id_ref,
			 int resno_ref, bool apply_rtop_flag);


/*  ----------------------------------------------------------------------- */
/*                  Cootaneer                                               */
/*  ----------------------------------------------------------------------- */
int cootaneer_internal(int imol_map, int imol_model, coot::atom_spec_t &atom_spec);

#ifdef USE_GUILE
int cootaneer(int imol_map, int imol_model, SCM atom_in_fragment_atom_spec);
#endif 

#ifdef USE_PYTHON
int cootaneer_py(int imol_map, int imol_model, PyObject *atom_in_fragment_atom_spec);
#endif


/*  ----------------------------------------------------------------------- */
/*                  Generic Objects                                         */
/*  ----------------------------------------------------------------------- */

// return a clean name and a flag to say that this was something that
// we were interested to make a graphics object from (rather than just
// header info)
std::pair<short int, std::string> is_interesting_dots_object_next_p(const std::vector<std::string> &vs);

/*  ----------------------------------------------------------------------- */
/*                  Generic Functions                                       */
/*  ----------------------------------------------------------------------- */
#ifdef USE_GUILE
SCM generic_string_vector_to_list_internal(const std::vector<std::string> &v);
SCM generic_int_vector_to_list_internal(const std::vector<int> &v);
std::vector<std::string> generic_list_to_string_vector_internal(SCM l);
SCM rtop_to_scm(const clipper::RTop_orth &rtop);
SCM inverse_rtop_scm(SCM rtop_scm);
coot::atom_spec_t atom_spec_from_scm_expression(SCM expr);
#endif	/* USE_GUILE */

#ifdef USE_PYTHON
/* Bernhard, I suppose that there should be python equivalents of the above. */
/* BL says:: here they are: */
PyObject *generic_string_vector_to_list_internal_py(const std::vector<std::string>&v);
PyObject *generic_int_vector_to_list_internal_py(const std::vector<int> &v);
std::vector<std::string> generic_list_to_string_vector_internal_py(PyObject *l);
PyObject *rtop_to_python(const clipper::RTop_orth &rtop);
PyObject *inverse_rtop_py(PyObject *rtop_py);
coot::atom_spec_t atom_spec_from_python_expression(PyObject *expr);
#endif // PYTHON


void set_display_control_button_state(int imol, const std::string &button_type, int state);


/*  ----------------------------------------------------------------------- */
/*                  Utility Functions                                       */
/*  ----------------------------------------------------------------------- */
// These functions are for storing the molecule number and (some other
// number) as an int and used with GPOINTER_TO_INT and GINT_TO_POINTER.
int encode_ints(int i1, int i2);
std::pair<int, int> decode_ints(int i);
#ifdef USE_GUILE
int key_sym_code_scm(SCM s_scm);
#endif // USE_GUILE
#ifdef USE_PYTHON
int key_sym_code_py(PyObject *po);
#endif // USE_PYTHON
#ifdef USE_GUILE
#ifdef USE_PYTHON
PyObject *scm_to_py(SCM s);
SCM py_to_scm(PyObject *o);
#endif // USE_GUILE
#endif // USE_PYTHON

#endif // CC_INTERFACE_HH
