// -*-c++-*- ; emacs directive
/* src/molecule-class-info.h
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
 * Copyright 2008, 2009 by the University of Oxford
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


#ifndef MOLECULE_CLASS_INFO_T
#define MOLECULE_CLASS_INFO_T

#ifndef HAVE_STRING
#define HAVE_STRING
#include <string>
#endif // HAVE_STRING

enum {CONTOUR_UP, CONTOUR_DOWN}; 

// needs:

//#include "mmdb_manager.h"
//#include "mmdb-extras.h"
//#include "mmdb.h"

// display list GLuint
#include <GL/gl.h>

#include "Bond_lines.h"
#include "gtk-manual.h"

#include "mini-mol.hh"
#include "CalphaBuild.hh"
#include "coot-render.hh"
#include "coot-surface.hh"
#include "coot-align.hh"
#include "coot-fasta.hh"
#include "coot-shelx.hh"
#include "coot-utils.hh"

using namespace std; // Hmmm.. I don't approve, FIXME

#include "select-atom-info.hh"
#include "coot-coord-utils.hh"
#include "coot-coord-extras.hh"

#include "protein-geometry.hh"


#include "validation-graphs.hh"  // GTK things, now part of
				 // molecule_class_info_t, they used
				 // to be part of graphics_info_t before the 
                                 // array->vector change-over.

#include "dipole.hh"

namespace molecule_map_type {
   enum { TYPE_SIGMAA=0, TYPE_2FO_FC=1, TYPE_FO_FC=2, TYPE_FO_ALPHA_CALC=3,
	  TYPE_DIFF_SIGMAA=4 };
}

namespace coot {

   enum { UNSET_TYPE = -1, NORMAL_BONDS=1, CA_BONDS=2, COLOUR_BY_CHAIN_BONDS=3,
	  CA_BONDS_PLUS_LIGANDS=4, BONDS_NO_WATERS=5, BONDS_SEC_STRUCT_COLOUR=6,
	  CA_BONDS_PLUS_LIGANDS_SEC_STRUCT_COLOUR=7,
	  CA_BONDS_PLUS_LIGANDS_B_FACTOR_COLOUR=14,
	  COLOUR_BY_MOLECULE_BONDS=8,
	  COLOUR_BY_RAINBOW_BONDS=9, COLOUR_BY_B_FACTOR_BONDS=10,
	  COLOUR_BY_OCCUPANCY_BONDS=11};

   // representation_types
   enum { SIMPLE_LINES, STICKS, BALL_AND_STICK, SURFACE };

   enum { RESIDUE_NUMBER_UNSET = -1111}; 

   class model_view_residue_button_info_t {
   public:
      model_view_residue_button_info_t(){} // for new allocator

      model_view_residue_button_info_t(const std::string &lab,
				       CResidue *res) {
	 button_label = lab;
	 residue = res;
      } 
      std::string button_label;
      CResidue *residue;
   };

   class model_view_atom_tree_item_info_t {
   public:
      // model_view_atom_tree_item_info_t() {}  // not needed?
      model_view_atom_tree_item_info_t(const std::string &label,
				       CResidue *res) {
	 button_label = label;
	 residue = res;
      }
      std::string button_label;
      CResidue *residue;
   };

   class model_view_atom_tree_chain_t {
   public:
      model_view_atom_tree_chain_t() {} // for new allocator
      model_view_atom_tree_chain_t(const std::string &chain_id_in) {
	 chain_id = chain_id_in;
      }
      void add_residue(const model_view_atom_tree_item_info_t &res) {
	 tree_residue.push_back(res);
      } 
      std::vector<model_view_atom_tree_item_info_t> tree_residue;
      std::string chain_id;
   };

   // old 
   class model_view_atom_button_info_t {
   public:
      model_view_atom_button_info_t() {} // for new allocator
      model_view_atom_button_info_t(const std::string &label,
				    CAtom *atom_in) {
	 button_label = label;
	 atom = atom_in;
      }
      std::string button_label;
      CAtom *atom;
   };


   // a helper class - provide filenames and status for dialog widget
   // 
   class backup_file_info { 
   public:
      short int status;
      int imol; 
      std::string name;
      std::string backup_file_name;
      backup_file_info() { 
	 status = 0; // initially no backup reported
      }
   };

   // Maybe we want a mainchain/side chain split here...
   class ncs_residue_info_t {
   public:
     float mean_diff;
     float n_weighted_atoms;
     int resno; 
     bool filled;
     std::string inscode;
     int serial_number;
     int target_resno; 
     int target_serial_number;
     std::string target_inscode;
     ncs_residue_info_t() {
       mean_diff = -1;
       n_weighted_atoms = 0;
       filled = 0;
     }
     ncs_residue_info_t(int resno_in, const std::string &ins_code_in, int serial_number_in,
			int target_resno_in, const std::string &target_ins_code_in, int target_serial_number_in) {
       filled = 1;
       resno = resno_in;
       inscode = ins_code_in;
       serial_number = serial_number_in;
       target_resno = target_resno_in;
       target_inscode = target_ins_code_in;
       target_serial_number = target_serial_number_in;
     }
   };

   class ncs_chain_difference_t {
   public:
     std::string peer_chain_id;
     std::vector<ncs_residue_info_t> residue_info;
     ncs_chain_difference_t() {
     }
     ncs_chain_difference_t(const std::string &peer_chain_id_in,
			    const std::vector<ncs_residue_info_t> &residue_info_in) {
       peer_chain_id = peer_chain_id_in;
       residue_info = residue_info_in;
     }
   };

   class ncs_differences_t {
   public:
     std::string target_chain_id;
     std::vector<ncs_chain_difference_t> diffs;
     unsigned int size() const { return diffs.size(); }
     ncs_differences_t() {}
     ncs_differences_t(const std::string &target_chain_id_in, 
		       std::vector<ncs_chain_difference_t> diffs_in) {
       target_chain_id = target_chain_id_in;
       diffs = diffs_in;
     }
   };

   class ncs_matrix_info_t {
   public: 
     bool state; 
     clipper::RTop_orth rtop;
     std::vector<int> residue_matches;
     ncs_matrix_info_t() {
       state = 0;
     }
     ncs_matrix_info_t(bool state_in, clipper::RTop_orth rtop_in, 
		       std::vector<int> residue_matches_in) {
       state = state_in;
       rtop = rtop_in;
       residue_matches = residue_matches_in;
     }
   };

   class ghost_molecule_display_t {
   public:
      clipper::RTop_orth rtop;
      int SelectionHandle;
      graphical_bonds_container bonds_box;
      std::string name;
      std::string chain_id;
      std::string target_chain_id;  // this operator matches to this chain.
      short int display_it_flag;
      std::vector<int> residue_matches;
      ghost_molecule_display_t() { SelectionHandle = -1;
	 display_it_flag = 0; }
      ghost_molecule_display_t(const clipper::RTop_orth &rtop_in,
			       int SelHnd_in,
			       const std::string &name_in) {
	 rtop = rtop_in;
	 SelectionHandle = SelHnd_in;
	 name = name_in;
	 display_it_flag = 1;
      }
      void update_bonds(CMMDBManager *mol); // the parent's mol
      bool is_empty() { return (SelectionHandle == -1); }
      ncs_residue_info_t get_differences(CResidue *this_residue_p, 
					 CResidue *master_residue_p, 
					 float main_chain_weight) const;
      friend std::ostream& operator<<(std::ostream &s, const ghost_molecule_display_t &ghost);
   };

   std::ostream& operator<<(std::ostream &s, const ghost_molecule_display_t &ghost);

   
   class display_list_object_info {
   public:
      bool is_closed;
      GLuint tag_1;
      GLuint tag_2;
      int type;
      std::string atom_selection;
      int atom_selection_handle;
      bool display_it;
      bool operator==(const display_list_object_info &dloi) const {
	 return (dloi.tag_1 == tag_1);
      }
      display_list_object_info() { 
	 display_it = 1;
	 is_closed = 0;
	 tag_1 = 0;
	 tag_2 = 0;
      }
      void close_yourself() {
	 is_closed = 1;
      } 
   };

   class dots_representation_info_t {
      bool is_closed;
   public:
      std::vector<clipper::Coord_orth> points;
      dots_representation_info_t(const std::vector<clipper::Coord_orth> &points_in) {
	 points = points_in;
	 is_closed = 0;
      }
      void close_yourself() {
	 points.clear();
	 is_closed = 1;
      }
      bool is_open_p() const {
	 int r = 1 - is_closed;
	 return r;
      }
   };

   class at_dist_info_t {

   public:
      int imol;
      CAtom *atom;
      float dist;
      at_dist_info_t(int imol_in, CAtom *atom_in, float dist_in) {
	 imol = imol_in;
	 atom = atom_in;
	 dist = dist_in;
      }
   };

   class go_to_residue_string_info_t {
   public:
      bool resno_is_set;
      bool chain_id_is_set;
      int resno;
      std::string chain_id;
      go_to_residue_string_info_t(const std::string &go_to_residue_string, CMMDBManager *mol);
   }; 

   class atom_attribute_setting_help_t {
   public:
     enum { UNSET, IS_FLOAT, IS_STRING, IS_INT};
      short int type;
      int i;
      float val;
      std::string s;
      atom_attribute_setting_help_t(const std::string &s_in) {
	 s = s_in;
	 type = IS_STRING;
      }
      atom_attribute_setting_help_t(float v) {
	 val = v;
	 type = IS_FLOAT;
      }
      atom_attribute_setting_help_t(int iin) {
	i = iin;
	type = IS_INT;
      }
      atom_attribute_setting_help_t() {
	 type = UNSET;
      }
   };

   class atom_attribute_setting_t {
   public: 
     atom_spec_t atom_spec;
     std::string attribute_name;
     atom_attribute_setting_help_t attribute_value;
     atom_attribute_setting_t(const std::string &chain_id_in, 
			      int resno_in, 
			      const std::string &inscode_in, 
			      const std::string &atom_name_in, 
			      const std::string &alt_conf_in, 
			      const std::string &attribute_name_in, 
			      const atom_attribute_setting_help_t &att_val) {
       atom_spec = atom_spec_t(chain_id_in, resno_in, inscode_in, atom_name_in, alt_conf_in);
       attribute_name = attribute_name_in;
       attribute_value = att_val;
     } 
   };

   class atom_selection_info_t { 
   public:
      enum { UNSET, BY_STRING, BY_ATTRIBUTES }; 
      int type;
      std::string chain_id;
      int resno_start;
      int resno_end;
      std::string ins_code;
      std::string altconf;
      bool alt_conf_is_set;
      // or:
      std::string atom_selection_str;
      atom_selection_info_t(const std::string &s) { 
	 atom_selection_str = s;
	 type = BY_STRING;
	 alt_conf_is_set = 0;
      }
      atom_selection_info_t(const std::string &chain_id_in, 
			    int resno_start_in, 
			    int resno_end_in,
			    const std::string &ins_code_in) { 
	 chain_id = chain_id_in;
	 resno_start = resno_start_in;
	 resno_end = resno_end_in;
	 ins_code = ins_code_in;
	 type = BY_ATTRIBUTES;
	 alt_conf_is_set = 0;
      }
      atom_selection_info_t(const std::string &chain_id_in, 
			    int resno_start_in, 
			    int resno_end_in,
			    const std::string &ins_code_in,
			    const std::string &alt_conf_in) { 
	 chain_id = chain_id_in;
	 resno_start = resno_start_in;
	 resno_end = resno_end_in;
	 ins_code = ins_code_in;
	 type = BY_ATTRIBUTES;
	 altconf = alt_conf_in;
	 alt_conf_is_set = 1;
      }
      atom_selection_info_t() { 
	 type = UNSET;
	 alt_conf_is_set = 0;
      }

      // Return the selection handle.  It is up to the caller to
      // dispose of the atom selection, with a DeleteSelection().
      int select_atoms(CMMDBManager *mol) const;
      void using_altconf(const std::string &altconf_in) {
	 altconf = altconf_in;
	 alt_conf_is_set = 1;
      }
      std::string name() const;
      std::string mmdb_string() const;
   };

   class additional_representations_t { 
   public:
      bool show_it;
      int bonds_box_type;
      int representation_type;
      float bond_width;
      bool draw_hydrogens_flag;
      graphical_bonds_container bonds_box;
      atom_selection_info_t atom_sel_info;
      CMMDBManager *mol;
      int display_list_handle;
      void update_self() {
	 if (representation_type != BALL_AND_STICK) {
	    fill_bonds_box();
	 }
      }
      void update_self_display_list_entity(int handle_in) {
	 display_list_handle = handle_in;
      }
      void fill_bonds_box();
     additional_representations_t(CMMDBManager *mol_in,
				  int representation_type_in,
				  int bonds_box_type_in,
				  float bond_width_in,
				  bool draw_hydrogens_flag_in,
				  const atom_selection_info_t &atom_sel_info_in) { 
       show_it = 1;
       mol = mol_in;
       bond_width = bond_width_in;
       representation_type = representation_type_in;
       bonds_box_type = bonds_box_type_in;
       draw_hydrogens_flag = draw_hydrogens_flag_in;
       atom_sel_info = atom_sel_info_in;
       fill_bonds_box();
     } 
     // on changind the outside (molecule_class_info_t's mol) we need
     // to change that of the additional_representations too.
     void change_mol(CMMDBManager *mol_in) { 
       mol = mol_in;
     } 
     void clear() { 
       show_it = 0;
     } 
     std::string info_string() const;
     void add_display_list_handle(int handle) { 
       display_list_handle = handle;
     } 
   };
}


// Forward declaration
class graphics_info_t;

#include "gl-bits.hh"

// should be arrays so that we can store lots of molecule
// informations.
//
class molecule_class_info_t {

   // This is needed because we use side by side stereo with display
   // lists.  When the maps get updated we need to generate display
   // lists - and the display lists are specific to the glarea
   // (GLContext).  So, when we compile_density_map_display_list() we
   // need to know where to store the returned diplay list index in
   // theMapContours (in first or second).
   // 
   // Note SIDE_BY_SIDE_MAIN refers to the normal mono glarea.
   // 
   enum { SIDE_BY_SIDE_MAIN = 0, SIDE_BY_SIDE_SECONDARY = 1}; 

   // we will use a pointer to this int so that we can (potentially
   // amongst other things) use it to construct gtk widget labels.
   // We need it to not go away before the whole
   // (molecule_class_info_t) object goes away. 
   // 
   int imol_no;
   int *imol_no_ptr;  // use this not &imol_no, because this is safe
		      // on copy of mol, but &imol_no most definately
		      // is not.

   float data_resolution_;
   float map_sigma_;
   float map_mean_;
   float map_max_;
   float map_min_;
   short int is_dynamically_transformed_map_flag;
   coot::ghost_molecule_display_t map_ghost_info;

   void draw_density_map_internal(short int display_lists_for_maps_flag_local,
				  bool draw_map_local_flag,
				  short int main_or_secondary);

   // display flags:
   // 
   int bonds_box_type; // public accessable via Bonds_box_type();
   short int manual_bond_colour;  // bond colour was set by hand and
				  // shouldn't be overridden by
				  // color-by-molecule-number.

   float bond_width;
   std::vector<float> bond_colour_internal;
   // 
   int draw_hydrogens_flag;
   //
   short int molecule_is_all_c_alphas() const; 
   
   std::string make_symm_atom_label_string(PCAtom atom, symm_trans_t symm_trans) const;
   std::string make_atom_label_string(PCAtom atom, int brief_atom_labels_flag) const;

   // rebuild/save state command
   std::vector<std::string> save_state_command_strings_;
   std::string single_quote(const std::string &s) const;
   short int have_unsaved_changes_flag; 
   int coot_save_index; // initially 0, incremeneted on save. 

   // saving temporary files (undo)
   //
   std::string save_molecule_filename(const std::string &dir);
   int make_backup(); // changes history details
   int make_maybe_backup_dir(const std::string &filename) const;
   short int backup_this_molecule; 

   // history_index and max_history_index tell us where we are in the
   // history of modifications of this molecule.  When we "Undo"
   // history_index is decreased (by 1) (and we restore from backup).
   // 
   int history_index;
   int max_history_index;
   void save_history_file_name(const std::string &file);
   std::vector<std::string> history_filename_vec;
   std::string save_time_string;
   void restore_from_backup(int history_offset);

   void set_initial_contour_level(); // tinker with the class data.
				     // Must be called after sigma_
				     // and is_diff_map has been set

   //
   void update_molecule_after_additions(); // cleanup, new
					   // atom_selection, sets
					   // unsaved changes flag,
					   // makes bonds.

   // fourier
   std::string fourier_f_label;   
   std::string fourier_phi_label;
   std::string fourier_weight_label;   // tested vs "" (unset).
   
   // refmac 
   short int have_sensible_refmac_params; // has public interface;
   std::string refmac_mtz_filename;
   std::string refmac_file_mtz_filename;
   std::string refmac_fobs_col;
   std::string refmac_sigfobs_col;
   std::string refmac_r_free_col;
   int refmac_r_free_flag_sensible;
   int refmac_count;

   short int have_refmac_phase_params; // has public interface;
   std::string refmac_phi_col;
   std::string refmac_fom_col;
   std::string refmac_hla_col;
   std::string refmac_hlb_col;
   std::string refmac_hlc_col;
   std::string refmac_hld_col;

   // generic

   // change asc.
   atom_selection_container_t filter_atom_selection_container_CA_sidechain_only(atom_selection_container_t asc) const;
   
   // skeleton colouring flag
   short int colour_skeleton_by_random;

   // Display list map contours id.  For left and right in
   // side-by-side stereo.  For mono (or hardware stereo) use just theMapContours.first()
   // 
   std::pair<GLuint, GLuint> theMapContours;

   // Noble surface display list id
   GLuint theSurface;
   
   // difference map negative level colour relative to positive level:
   float rotate_colour_map_for_difference_map; // 120.0 default colour_map_rotation

   float get_clash_score(const coot::minimol::molecule &a_rotamer) const;
   std::pair<double, clipper::Coord_orth> get_minimol_pos(const coot::minimol::molecule &a_rotamer) const; 

   // backup is done in the wrappers.  (This factorization needed for
   // add_alt_conf/INSERT_CHANGE_ALTCONF supprt).
   void insert_coords_internal(const atom_selection_container_t &asc);

   // In this instance, we don't want to install a whole residue, we want
   // to install atoms in this residue (alt conf B) into a a atom_sel mol
   // residue that contains (say) "" and "A".
   //
   // pass shelx_occ_fvar_number -1 if we don't have it.
   void insert_coords_atoms_into_residue_internal(const atom_selection_container_t &asc,
						  int shelx_occ_fvar_number);

   short int is_mmcif(const std::string &filename) const; 
   void unalt_conf_residue_atoms(CResidue *residue_p); 

   // return status and a chain id [status = 0 when there are 26 chains...]
   // 
   std::pair<short int, std::string> unused_chain_id() const;

   int set_coot_save_index(const std::string &s); 

   // move this where they belong (from globjects)
   void set_symm_bond_colour_mol(int i); 
   void set_symm_bond_colour_mol_and_symop(int icol, int isymop); 
   void set_symm_bond_colour_mol_rotate_colour_map(int icol, int isymop); 
   void rotate_rgb_in_place(float *rgb, const float &amount) const;
   void convert_rgb_to_hsv_in_place(const float *rgb, float *hsv) const; 
   void convert_hsv_to_rgb_in_place(const float* hsv, float *rgb) const;

   float combine_colour (float v, int col_part_index);

   std::vector<coot::dipole> dipoles;

   // make fphidata lie within the resolution limits of reso.  Do we
   // need a cell to do this?
   // 
   void filter_by_resolution(clipper::HKL_data< clipper::datatypes::F_phi<float> > *fphidata,
			     const float &reso_low,
			     const float &reso_high) const;
   // Retard the phases for use with anomalous data.
   void fix_anomalous_phases(clipper::HKL_data< clipper::datatypes::F_phi<float> > *fphidata) const;


   // merge molecules helper function
   std::vector<std::string> map_chains_to_new_chains(const std::vector<std::string> &adding_model_chains,
						     const std::vector<std::string> &this_model_chains) const;
   void copy_and_add_residue_to_chain(CChain *this_model_chain, CResidue *add_model_residue);

   short int ligand_flip_number;

   // NCS ghost molecules:
   // 
   std::vector<coot::ghost_molecule_display_t> ncs_ghosts;
   void update_ghosts();
   short int show_ghosts_flag;
   float ghost_bond_width;
   bool ncs_ghost_chain_is_a_target_chain_p(const std::string &chain_id) const;
   // throw an exception when the matrix is not defined (e.g. no atoms).
   coot::ncs_matrix_info_t find_ncs_matrix(int SelHandle1, int SelHandle2) const;
   short int ncs_ghosts_have_rtops_flag;
   // have to take into account the potential built/non-built offsets:
   bool ncs_chains_match_p(const std::vector<std::pair<std::string, int> > &v1,
			   const std::vector<std::pair<std::string, int> > &v2,
			   float exact_homology_level, 
			   bool allow_offset_flag) const;
   bool ncs_chains_match_with_offset_p(const std::vector<std::pair<std::string, int> > &v1,
				       const std::vector<std::pair<std::string, int> > &v2,
				       float exact_homology_level) const;
   bool last_ghost_matching_target_chain_id_p(int i_match, 
					      const std::vector<coot::ghost_molecule_display_t> &ncs_ghosts) const;
   void delete_ghost_selections();

   std::vector<coot::ghost_molecule_display_t> strict_ncs_info;
   std::vector<coot::coot_mat44> strict_ncs_matrices;

   // void set_show_ghosts(short int istate); public
   // void set_ghost_bond_thickness(float f); public
   // float ghost_bond_thickness() const {return int(ghost_bond_width);} // public

   std::vector <coot::atom_spec_t>
   find_water_baddies_AND(float b_factor_lim, const clipper::Xmap<float> &xmap_in,
			  float map_sigma,
			  float outlier_sigma_level,
			  float min_dist, float max_dist,
			  short int part_occ_contact_flag,
			  short int zero_occ_flag);

   std::vector <coot::atom_spec_t>
   find_water_baddies_OR(float b_factor_lim, const clipper::Xmap<float> &xmap_in,
			 float map_sigma,
			 float outlier_sigma_level,
			 float min_dist, float max_dist,
			 short int part_occ_contact_flag,
			 short int zero_occ_flag);

   // alignment
   coot::chain_mutation_info_container_t
   align_on_chain(const std::string &chain_id,
		  PCResidue *SelResidues, int nSelResidues,
		  const std::string &target) const;

   std::string
   make_model_string_for_alignment(PCResidue *SelResidues,
				   int nSelResidues) const;
   // renumber_reidues starting at 1 and removing insertion codes
   // (no backup).  For use in alignment (maybe other places)
   void simplify_numbering_internal(CChain *chain_p);



   // String munging helper function (for reading mtz files).
   // Return a pair.first string of length 0 on error to construct dataname(s).
   std::pair<std::string, std::string> make_import_datanames(const std::string &fcol,
							     const std::string &phi_col,
							     const std::string &weight_col,
							     int use_weights) const;

   // change chain id internal function
   std::pair<int, std::string>
   change_chain_id_with_residue_range(const std::string &from_chain_id,
				      const std::string &to_chain_id,
				      int start_resno,
				      int end_resno);

   void change_chain_id_with_residue_range_helper_insert_or_add(CChain *to_chain_p, CResidue *new_residue);

   // private nomenclature fixing (maybe) function.
   int test_and_fix_PHE_TYR_nomenclature_errors(CResidue *res); 

   // shelx stuff
   bool is_from_shelx_ins_flag;
   coot::ShelxIns shelxins;

   // for NCS copying of A chain onto the other chains.
   int copy_chain(CChain *from_chain, CChain *to_chain,
		  clipper::RTop_orth a_to_b_transform);
   int copy_residue_range(CChain *from_chain, CChain *to_chain,
			  int residue_range_1,
			  int residue_range_2,
			  clipper::RTop_orth a_to_b_transform);

   std::vector<coot::dots_representation_info_t> dots;
   coot::at_dist_info_t closest_atom(const coot::Cartesian &pt,
				     bool ca_check_flag) const;
   coot::at_dist_info_t closest_atom(const coot::Cartesian &pt,
				     bool ca_check_flag,
				     const std::string &chain_id,
				     bool use_this_chain_id) const;
   
   // return -1 on not found
   int get_atom_index(CAtom *atom) { 
     int idx = -1;
     if (has_model()) { 
       int ic;
       if (atom->GetUDData(atom_sel.UDDAtomIndexHandle, ic) == UDDATA_Ok) {
	 idx = ic;
       }
     }
     return idx;
   } 

   // save the data used for the fourier, so that we can use it to
   // sharpen the map:
   // uncommenting the following line causes a crash in the multi-molecule
   // (expand molecule space) test.
   bool original_fphis_filled;
   clipper::HKL_data< clipper::datatypes::F_phi<float> > original_fphis;


   // remove TER record from residue
   bool residue_has_TER_atom(CResidue *res_p) const;
   void remove_TER_internal(CResidue *res_p);
   void remove_TER_on_last_residue(CChain *chain_p);

   // ------------------------------------------------------------------
 public:
   // ------------------------------------------------------------------

   // we should dump this constructor in the skip then.
   // it does not set imol_no; 
   molecule_class_info_t() { 

      setup_internal(); 
      // give it a pointer to something at least *vagely* sensible
      // (better than pointing at -129345453). 
      imol_no = -1;
      imol_no_ptr = new int;
      *imol_no_ptr = imol_no;
      //
   } 

   // See graphics_info_t::initialize_graphics_molecules()
   // 
   molecule_class_info_t(int i) { 

      setup_internal(); 
      imol_no = i;
      imol_no_ptr = new int;
      *imol_no_ptr = i;
   }

   // Why did I add this?  It causes a bug in "expanding molecule space"
//    ~molecule_class_info_t() {
//       delete imol_no_ptr;
//       bonds_box.clear_up();
//       symmetry_bonds_box.clear_up();

//       // also the atom labels
      
//    }

   ~molecule_class_info_t() {

      // using this destructor causes a redraw, it seems.
      //
      // Note that the constructor used in expand_molecule_space()
      // new_molecules[i] = molecules[i];
      // does a shallow copy of the pointers.  So we can't delete them here.
      // 

      // give back the memory from the map, so that we don't get
      // clipper leak message?
      drawit = 0;
      drawit_for_map = 0;  // don't display this thing on a redraw!

      // don't do these things when we have a shallow copy constructor.
      if (0) { 
	 if (has_map()) {
	    clipper::Xmap<float> x;
	    xmap_list[0] = x;
	    delete [] xmap_list;
	    xmap_list = 0;
	 }
      }
   }

   void setup_internal() { 
      atom_sel.atom_selection = NULL;
      atom_sel.n_selected_atoms = 0;
      atom_sel.mol = NULL;

      xmap_list = 0;
      xmap_is_filled = NULL;
      // while zero maps, don't need to intialise the arrays (xmap_is_filled)
      max_xmaps = 0;
      //
      xskel_is_filled = 0; // not filled.
      skeleton_treenodemap_is_filled = 0;

      draw_hydrogens_flag = 1;
      bond_width = 3.0; 
      ghost_bond_width = 2.0;

      // initial bonds type (checked and reset in handle_read_draw_molecule)
      bonds_box_type = coot::UNSET_TYPE;

      save_time_string = "";

      pickable_atom_selection = 1; 

      // refmac stuff
      // 
      refmac_count = 0;
      have_sensible_refmac_params = 0;  // initially no refmac params.
      have_refmac_phase_params    = 0;  // initially no refmac params.

      // history stuff
      // 
      history_index = 0;
      max_history_index = 0;

      have_unsaved_changes_flag = 0; // no unsaved changes initially.
      show_unit_cell_flag = 0;
      fc_skeleton_draw_on = 0;
      greer_skeleton_draw_on = 0;

      // Don't draw it initially.
      drawit = 0;
      drawit_for_map = 0;

      // backup on by default, turned off for dummy atoms (baton building)
      backup_this_molecule = 1;

      // Map sutff
      map_max_ = 100.0;
      map_min_ = -100.0;

      // fourier (for phase recombination (potentially) in refmac:
      fourier_weight_label = ""; // unset initially.

      // HL coeff and phi (for phase recombination (potentially) in refmac:
      // should be enough to unset the first one for testing for HL
      refmac_phi_col = ""; // unset initially.
      refmac_hla_col = ""; // unset initially.

      // 
      colour_skeleton_by_random = 0;

      // original Fs saved? (could be from map)
      original_fphis_filled = 0;

      //  bond width (now changeable).
      bond_width = 3.0;

      // 
      rotate_colour_map_for_difference_map = 240.0; // degrees

      // save index
      coot_save_index = 0;

      // ligand flipping. save the index
      ligand_flip_number = 0; // or 1 2 3 for the 4 different
			      // orientations round the eigen vectors.
			      // (For ligand molecules).  In future,
			      // this may need to be keyed on the
			      // resno and chain_id for multiple
			      // ligands in a molecule.

      show_symmetry = 1; // individually on by default.

      // Draw NCS ghosts?
      show_ghosts_flag = 0;
      is_dynamically_transformed_map_flag = 0;
      ncs_ghosts_have_rtops_flag = 0;

      // contour by sigma
      contour_by_sigma_flag = 1;
      contour_sigma_step = 0.1;

      //
      cootsurface = NULL; // no surface initial, updated by make_surface()
      theSurface = 0;

      //
      theMapContours.first = 0;
      theMapContours.second = 0;

      // don't show strict ncs unless it's turned on.
      show_strict_ncs_flag = 1;

      // shelx stuff
      is_from_shelx_ins_flag = 0;

      // symmetry, 20060202 valgrind found that these was not set.
      symmetry_rotate_colour_map_flag = 0;
      symmetry_as_calphas = 0;
      symmetry_whole_chain_flag = 0;
      symmetry_colour_by_symop_flag = 0;
   }

   int handle_read_draw_molecule(int imol_no_in,
				 std::string filename,
				 short int recentre_rotation_centre,
				 short int is_undo_or_redo, 
				 float bond_width_in);

   void      label_symm_atom(int i, symm_trans_t symm_trans);
   void test_label_symm_atom(int i);
   
   void label_atom(int i, int brief_atom_labels_flag);

   void debug_selection() const; 
   void debug() const;

   // Ugh.  Horrible.  I don't want outside access to setting of
   // imol_no - I want to do it in the constructor.  Must FIX. 
   // 
   void set_mol_number(int i) {
      imol_no = i;
      *imol_no_ptr = i;
   }; 
   void set_bond_colour_by_mol_no(int icolour);  // not const because
						 // we also set
						 // bond_colour_internal.

   void set_bond_colour_by_colour_wheel_position(int i, int bond_type);

   std::string name_; // otherwise get and set, so make it public.

   int MoleculeNumber() const { return imol_no; }

   std::string save_mtz_file_name;
   std::string save_f_col;
   std::string save_phi_col;
   std::string save_weight_col;
   int save_use_weights;
   int save_is_anomalous_map_flag;
   int save_is_diff_map_flag;
   float save_high_reso_limit;
   float save_low_reso_limit;
   int save_use_reso_limits;

   void map_fill_from_mtz(std::string mtz_file_name,
			  std::string f_col,
			  std::string phi_col,
			  std::string weight_col,
			  int use_weights,
			  int is_diff_map, 
			  float map_sampling_rate);

   void map_fill_from_mtz_with_reso_limits(std::string mtz_file_name,
					   std::string f_col,
					   std::string phi_col,
					   std::string weight_col,
					   int use_weights,
					   short int is_anomalous_flag,
					   int is_diff_map,
					   short int use_reso_flag,
					   float low_reso_limit,
					   float high_reso_limit, 
					   float map_sampling_rate);

   void map_fill_from_cns_hkl(std::string cns_file_name,
			      std::string f_col,
			      int is_diff_map, 
			      float map_sampling_rate);

   atom_selection_container_t atom_sel;
   int drawit; // used by Molecule Display control, toggled using
	       // toggle fuctions. 
   bool drawit_for_map; 
   int pickable_atom_selection;  // ditto (toggling).
   
   std::string show_spacegroup() const;

   void set_mol_is_displayed(int state) {
      if (atom_sel.n_selected_atoms > 0) {
	 drawit = state;
      }
   }

   void set_mol_is_active(int state) {
      if (atom_sel.n_selected_atoms > 0)
	 pickable_atom_selection = state;
      else 
	 pickable_atom_selection = 0;
   }

   void set_map_is_displayed(int state) {
      drawit_for_map = state;
   }

   void do_solid_surface_for_density(short int on_off_flag);

   int atom_selection_is_pickable() const { 
      return pickable_atom_selection && (atom_sel.n_selected_atoms > 0);
   } 

   short int show_symmetry; // now we have individual control of the
			    // symmetry display (as well as a master
			    // control).

   void set_show_symmetry(short int istate) {
      show_symmetry = istate;
   }

   // Unit Cell (should be one for each molecule)
   // 
   short int show_unit_cell_flag; 
   short int have_unit_cell;
   void set_have_unit_cell_flag_maybe();

   void draw_molecule(short int do_zero_occ_spots);
   void zero_occupancy_spots() const;
   void set_occupancy_residue_range(const std::string &chain_id, int ires1, int ires2, float occ_val);

   void set_b_factor_residue_range(const std::string &chain_id, int ires1, int ires2, float b_val);
   void set_b_factor_atom_selection(const atom_selection_container_t &asc, float b_val, bool moving_atoms);


   std::vector<coot::atom_spec_t> fixed_atom_specs;
   std::vector<coot::Cartesian>   fixed_atom_positions; // updated on make_bonds_type_checked()
   void update_fixed_atom_positions();
   void update_additional_representations(const gl_context_info_t &gl_info);
   void update_mols_in_additional_representations(); //uses atom_sel.mol
   void draw_fixed_atom_positions() const;
   void clear_all_fixed_atoms();
   std::vector<coot::atom_spec_t> get_fixed_atoms() const;

   // we could combine these 2, because a bonds_box contains symmetry
   // data members, but we do not, because update_symmetry is more 
   // elegant without it.
   // 
   // Note that display_bonds now used symmetry_bonds_box.
   // 
   // Tripping up here?  Need Bond_lines.h
   graphical_bonds_container bonds_box;
   std::vector<coot::additional_representations_t> add_reps;
   void remove_display_list_object_with_handle(int handle);

   std::vector<std::pair<graphical_bonds_container, std::pair<symm_trans_t, Cell_Translation> > > symmetry_bonds_box;
   std::vector<std::pair<graphical_bonds_container, symm_trans_t> > strict_ncs_bonds_box;
   void clear_up_all_symmetry() {
      for (unsigned int i=0; i<symmetry_bonds_box.size(); i++)
	 symmetry_bonds_box[i].first.clear_up();
      for (unsigned int i=0; i<strict_ncs_bonds_box.size(); i++)
	 strict_ncs_bonds_box[i].first.clear_up();
   }

   // Return a copy of the pointer (only).  Return NULL on residue not
   // found.
   // 
   CResidue *get_residue(int reso, const std::string &insertion_code,
			 const std::string &chain_id) const;

   // Return a copy of the pointer (only).  Return NULL on residue not
   // found.
   // 
   CResidue *get_residue(const coot::residue_spec_t &rs) const;

   // Return a copy of the pointer (only) of the residue following
   // that of the given spec.  Return NULL on residue not found.
   // 
   CResidue *get_following_residue(const coot::residue_spec_t &rs) const;

   // A function of molecule-class that returns the position
   // (if posible) of an atom given a string e.g. 
   // "A" -> nearest atom in the "A" chain
   // "a" -> nearest atom in the "a" chain, or failing that the "A" chain.
   // "43" residue 43 of the chain we are looking at.
   //"21b" -> residue 21 of the "b" chain, otherwise residue 21 of the "B" 
   //         chain 
   // 
   // For keyboarding go-to atom.
   //
   // Return NULL on not-able-to-find-atom.
   // 
   CAtom *get_atom(const std::string &go_to_residue_string,
		   const coot::atom_spec_t &active_atom_spec,
		   const coot::Cartesian &pt) const;

   CAtom *get_atom(const coot::atom_spec_t &atom_spec) const;

   void set_draw_hydrogens_state(int i) {
      if (draw_hydrogens_flag != i) { 
	 draw_hydrogens_flag = i;
	 make_bonds_type_checked();
	 update_symmetry();
      }
   }

   int draw_hydrogens() const {
      return draw_hydrogens_flag;
   }

   void makebonds();
   void makebonds(float max_dist); // maximum distance for bond (search)
   void makebonds(float min_dist, float max_dist); 
   void make_ca_bonds(float min_dist, float max_dist); 
   void make_ca_bonds();
   void make_ca_plus_ligands_bonds();
   void make_colour_by_chain_bonds(short int c_only_flag);
   void make_colour_by_molecule_bonds();
   void bonds_no_waters_representation();
   void bonds_sec_struct_representation();
   void ca_plus_ligands_sec_struct_representation();
   void ca_plus_ligands_rainbow_representation();
   void b_factor_representation();
   void b_factor_representation_as_cas();
   void occupancy_representation();

   void make_bonds_type_checked(); 


   void label_atoms(int brief_atom_labels_flag);
   
   void update_symmetry();
   void update_strict_ncs_symmetry(const coot::Cartesian &centre_point,
				   const molecule_extents_t &extents); // in m-c-i-ncs.cc
   void anisotropic_atoms(); 
   void draw_coord_unit_cell(const coot::colour_holder &cell_colour);
   void draw_map_unit_cell(const coot::colour_holder &cell_colour);
   void draw_unit_cell_internal(float rsc[8][3]);
   void draw_dots();
   // return the status of whether or not the dots were cleared.
   bool clear_dots(int dots_handle);
   int  make_dots(const std::string &atom_selection_str,
		  float dot_density, float atom_radius_scale);
   int n_dots_sets() const {  // return the number of sets of dots.
      return dots.size();
   } 

   void initialize_on_read_molecule(); 
   
   void initialize_map_things_on_read_molecule(std::string name, int is_diff_map, 
					       short int swap_difference_map_colours);
   void initialize_coordinate_things_on_read_molecule(std::string name);
   void initialize_coordinate_things_on_read_molecule_internal(std::string name,
							       short int is_undo_or_redo);
   void install_model(int imol_no_in, 
		      atom_selection_container_t asc, const std::string &mol_name,
		      short int display_in_display_control_widget_status,
		      bool is_from_shelx_ins=0);

   const coot::CartesianPair* draw_vectors;
   const coot::CartesianPair* diff_map_draw_vectors;
   int n_diff_map_draw_vectors;
   int n_draw_vectors;

   coot::Cartesian  centre_of_molecule() const;
   float size_of_molecule() const; // return the standard deviation of
				   // the length of the atoms from the
				   // centre of the molecule



   // return -1 if atom not found.
   int atom_index(const char *chain_id, int iresno, const char *atom_id); 

   // atom labels
   //

   // These are not const because set_bond_colour_by_mol_no() gets called.
   // maybe needs fixing.  Similarly  set_symm_bond_colour_mol_and_symop().
   void display_bonds();
   void display_symmetry_bonds();
   void display_bonds(const graphical_bonds_container &bonds_box, float bond_width_in);
   void display_ghost_bonds(int ighost);


   // old style
   // int* labelled_atom_index_list;
   // int  n_labelled_atoms; 
   // new style
   std::vector<int> labelled_atom_index_list;
   //
   // Symmetery atom labels.
   //
   //    int* labelled_symm_atom_index_list;
   std::vector<int> labelled_symm_atom_index_list; 
   //    int  n_labelled_symm_atoms;
   std::vector<std::pair<symm_trans_t, Cell_Translation> > labelled_symm_atom_symm_trans_;

   // Atom Labelling Interface Functions
   // 
   void add_to_labelled_atom_list(int atom_index);

   // Return the atom index of the i'th atom to be labelled 
   // (0 indexed). 
   // 
   int labelled_atom(int i);
   // 
   // how many atoms do we want to index?
   // int max_labelled_atom();  // old pointer stuff
   // 
   void unlabel_atom(int i); 
   // is the i'th atom in the list of atoms to be labelled?
   // 
   bool is_in_labelled_list(int i);
   //
   void unlabel_last_atom(); // remove the last atom from the list (if
			     // there *are* atoms in the list, else do
			     // nothing)/

   void trim_atom_label_table(); // when we delete a residue and have
				 // new atoms, we don't want to label
				 // atoms that are over the end of the
				 // (atom label) list.


   // 
   void add_atom_to_labelled_symm_atom_list(int atom_index, 
					    const symm_trans_t &symm_trans,
					    const Cell_Translation &pre_shift_cell_trans);

   // Return the atom index of the i'th symmetry atom to be labelled.
   //
   int labelled_symm_atom(int i);
   //
   // unlabell the i'th symm atom:
   //
   void unlabel_symm_atom(int i);
   // 
   // int max_labelled_symm_atom(); // old pointer stuff 
   bool is_in_labelled_symm_list(int i);
   std::pair<symm_trans_t, Cell_Translation> labelled_symm_atom_symm_trans(int i);
   //
   // Added from c-interface (the guile interface) simply named
   // functions, add a label to the atom with the characteristics
   // (using atom_index).
   //
   int    add_atom_label(char *chain_id, int iresno, char *atom_id);
   int remove_atom_label(char *chain_id, int iresno, char *atom_id);
   void remove_atom_labels(); // and symm labels

   // xmap information
   // 
   // We have problems using static vectors, so to Kevin's irritation,
   // we will use a pointer to xmaps that gets renewed, copied and
   // deleted (if necessary).
   // 
   // We use xmap_is_filled[0] to see if this molecule is a map. 
   // 
   clipper::Xmap<float>* xmap_list;
   int   *xmap_is_filled;
   int    max_xmaps;
   int   *xmap_is_diff_map;

   float *contour_level; 
   short int contour_by_sigma_flag;
   float contour_sigma_step;
   // 
   double  **map_colour;
   std::vector<double> previous_map_colour;
   void save_previous_map_colour();
   void restore_previous_map_colour();

   std::vector<coot::display_list_object_info> display_list_tags;
   void update_map_internal();
   void update_map();
   void compile_density_map_display_list(short int first_or_second);
   void draw_density_map(short int display_list_for_maps_flag,
			 short int main_or_secondary);
   void draw_surface();
   void draw_dipoles() const;
   bool has_display_list_objects();
   int draw_display_list_objects(int GL_context); // return number of display list objects drawn
   // return the display list tag
   int make_ball_and_stick(const std::string &atom_selection_str,
 			   float bond_thickness, float sphere_size,
 			   bool do_spheres_flag, gl_context_info_t gl_info);
   // return the display list tag
   coot::display_list_object_info
   make_ball_and_stick(const std::string &atom_selection_str,
		       float bond_thickness, float sphere_size,
		       bool do_spheres_flag, bool is_second_context,
		       coot::display_list_object_info dloi);
   void clear_display_list_object(GLuint tag);

   // the charges for the surface come from the dictionary.
   void make_surface(int on_off_flag, const coot::protein_geometry &geom);
   
   void dynamically_transform(coot::CartesianPairInfo v);
   void set_draw_vecs(const coot::CartesianPair* c, int n) { 
      delete [] draw_vectors;
      draw_vectors = c; n_draw_vectors = n; 
   }

   // for negative the other map.
   // 
   void set_diff_map_draw_vecs(const coot::CartesianPair* c, int n) { 
      delete [] diff_map_draw_vectors;
      diff_map_draw_vectors = c; n_diff_map_draw_vectors = n; 
   }

   void update_map_triangles(float radius, coot::Cartesian centre); 


   // skeleton
   // 
   int greer_skeleton_draw_on; 
   int fc_skeleton_draw_on; 
   void draw_skeleton();
   // void update_skeleton();  bye. use update_clipper_skeleton instead
   graphical_bonds_container greer_skel_box;
   graphical_bonds_container fc_skel_box;
   void draw_fc_skeleton();
   void unskeletonize_map();
   void set_skeleton_bond_colour(float f);
   void set_colour_skeleton_by_segment(); // use random colouring
   void set_colour_skeleton_by_level(); // use given colour (by shading it)
   void update_fc_skeleton_old(); 
   void update_clipper_skeleton(); 
   clipper::Xmap<int> xskel_cowtan;
   short int xskel_is_filled; 


   //
   void update_map_colour_menu_maybe(int imol); 
   void handle_map_colour_change(gdouble *map_col,
				 short int swap_difference_map_colours_flag,
				 short int main_or_secondary);

   int next_free_map(); 

   // 
   void check_static_vecs_extents(); 

   //
   int read_ccp4_map(std::string f, int is_diff_map_flag,
		     const std::vector<std::string> &map_glob_extensions); // return -1 on error

   // 
   int make_map_from_phs(std::string pdb_filename,
			 std::string phs_filename); 

   int make_map_from_phs_using_reso(std::string phs_filename,
				    const clipper::Spacegroup &sg,
				    const clipper::Cell &cell,
				    float reso_limit_low,
				    float reso_limit_high, 
				    float map_sampling_rate);

   int make_map_from_phs(const clipper::Spacegroup &sg,
			 const clipper::Cell &cell,
			 std::string phs_filename);

   int make_map_from_cns_data(const clipper::Spacegroup &sg,
			      const clipper::Cell &cell,
			      std::string cns_data_filename);

   int make_map_from_cif_sigmaa(int imol_no_in,
				std::string cif_filename, int sigma_map_type); // has phases
   int make_map_from_cif(int imol_no_in,
			 std::string cif_file_name); // uses above with type SIGMAA
   int make_map_from_cif_diff_sigmaa(int imol_no_in,
				     std::string cif_file_name); // uses above with TYPE_DIFF_SIGMAA

   int make_map_from_cif(int imol_no_in, 
			 std::string cif_filename,  // generate phases from mol
			  int imol_coords);
   int make_map_from_cif(int imol_no_in, 
			 std::string cif_file_name,
			 atom_selection_container_t SelAtom);
   int make_map_from_cif_2fofc(int imol_no_in, 
			       std::string cif_file_name,
			       atom_selection_container_t SelAtom);
   int make_map_from_cif_2fofc(int imol_no_in, 
			       std::string cif_file_name,
			       int imol_coords);
   int make_map_from_cif_fofc(int imol_no_in, 
			      std::string cif_file_name,
			      int imol_coords);
   int make_map_from_cif_generic(int imol_no_in,
				 std::string cif_file_name,
				 atom_selection_container_t SelAtom,
				 short int is_2fofc_type); 
   int make_map_from_cif_nfofc(int imol_map_in,
			       std::string cif_file_name,  // Virgina request
			       int map_type,
			       short int swap_difference_map_colours);
   int calculate_sfs_and_make_map(int imol_no_in,
				  const std::string &mol_name,
				  const clipper::HKL_data< clipper::datatypes::F_sigF<float> > &myfsigf,
				  atom_selection_container_t SelAtom,
				  short int is_2fofc_type);
   int make_map_from_mtz_by_calc_phases(int imol_no_in, 
					const std::string &mtz_file_name,
					const std::string &f_col,
					const std::string &sigf_col,
					atom_selection_container_t SelAtom,
					short int is_2fofc_type);

   
   void update_map_in_display_control_widget() const; 
   void new_coords_mol_in_display_control_widget() const;  // for a new molecule.
   void update_mol_in_display_control_widget() const; // changing the name of
                                                      // this mol (e.g on save).
   void update_mol_in_simple_display_control_menu(GtkWidget *model_menu, int map_coords_mol_flag);

   //
   float map_sigma() const { return map_sigma_; }

   // for debugging.
   int test_function();
   
   // output
   // return the mmdb exit status (as write_pdb_file)
   int export_coordinates(std::string filename) const; 

   // for labelling from guile
   int atom_spec_to_atom_index(std::string chain, int reso, 
			       std::string atom_name) const;
   
   int full_atom_spec_to_atom_index(const std::string &chain,
				    int reso,
				    const std::string &insersion_code,
				    const std::string &atom_name,
				    const std::string &alt_conf) const;

   int full_atom_spec_to_atom_index(const coot::atom_spec_t &spec) const;

   int atom_to_atom_index(CAtom *at) const;

   // Does atom at from moving atoms match atom_sel.atom_selection[this_mol_index_maybe]?
   // or has atom_sel changed in the mean time?
   bool moving_atom_matches(CAtom *at, int this_mol_index_maybe) const;

   int atom_index_first_atom_in_residue(const std::string &chain_id,
					int iresno,
					const std::string &ins_code) const;

   int atom_index_first_atom_in_residue(const std::string &chain_id,
					int iresno,
					const std::string &ins_code,
					const std::string &altconf) const;

   int atom_index_first_atom_in_residue_internal(const std::string &chain_id,
						 int iresno,
						 const std::string &ins_code,
						 const std::string &altconf,
						 bool test_alt_conf_flag) const;

   void install_ghost_map(const clipper::Xmap<float> &mapin, std::string name,
			  const coot::ghost_molecule_display_t &ghost_info,
			  int is_diff_map_flag,
			  int swap_difference_map_colours_flag,
			  float sigma_in);
   
   void new_map(const clipper::Xmap<float> &mapin, std::string name);
   void set_name(std::string name); // you are encouraged not to use
				    // this (only for use after having
				    // imported an xmap).


   // regularization results:
   void replace_coords(const atom_selection_container_t &asc,
		       bool change_altconf_occs_flag,
		       bool replace_coords_with_zero_occ_flag);
   // helper function for above function
   bool movable_atom(CAtom *mol_atom, bool replace_coords_with_zero_occ_flag) const;

   // extra modelling results, e.g. waters, terminal residues, etc
   void add_coords(const atom_selection_container_t &asc);
   //
   // Add a residue "in the middle" of some other residues
   // (uses CChain::InsResidue()).
   //
   // This relies on the mol of the asc being different to the mol of the
   // atom_sel.
   //
   // If it is the same, then error and do nothing.
   //
   // Typically, this is called by fit terminal residue, which has its
   // mol created from a pcmmdbmanager() from the molecule of the
   // residue, so this is fine in this case.
   // 
   void insert_coords(const atom_selection_container_t &asc);
   void insert_coords_change_altconf(const atom_selection_container_t &asc);
   // a utility function for insert_coords, return the residue (that
   // is currently at res_index) too.  Return a index of -1 (and a
   // CResidue of NULL) when no residue found.
   
   std::pair<int, CResidue *> find_serial_number_for_insert(int res_index, const std::string &chain_id) const;

   void update_molecule_to(std::vector<coot::scored_skel_coord> &pos_position); 

   //
   // Add this molecule (typically of waters to this
   // molecule... somehow).  All the atoms of water_mol need to be in
   // a chain that has a different chain id to all the chains in this
   // molecule.  Else fail (return status 0).
   //
   // But we should try to put the waters into (add/append to) a chain
   // of waters in this molecule, if it has one.
   // 
   int insert_waters_into_molecule(const coot::minimol::molecule &water_mol);
   int append_to_molecule(const coot::minimol::molecule &water_mol);
   CResidue *residue_from_external(int reso, const std::string &insertion_code,
				   const std::string &chain_id) const;


   // for the "Render As: " menu items:
   // 
   void bond_representation();
   //
   void ca_representation();
   void ca_plus_ligands_representation();

   float bonds_colour_map_rotation;
   short int bonds_rotate_colour_map_flag;
   int Bonds_box_type() const { return bonds_box_type; }

   //
   void pepflip(int atom_index);
   int pepflip_residue(const std::string &chain_id,
		       int ires_seqno,
		       const std::string &ins_code,
		       const std::string &alt_conf);

   int do_180_degree_side_chain_flip(const std::string &chain_id,
				     int resno,
				     const std::string &inscode,
				     const std::string &altconf,
				     coot::protein_geometry *geom_p);

   // return "N', "C" or "not-terminal-residue"
   std::string get_term_type_old(int atom_index); 
   std::string get_term_type(int atom_index); 
   // by alignment (against asigned pir seq file) return, "HIS", "ALA" etc, if we can.
   std::pair<bool, std::string> find_terminal_residue_type(const std::string &chain_id, int resno) const;


   // These create an object that is not specific to a molecule, there
   // is only one environment bonds box, no matter how many molecules. Hmm.
   // The symmetry version is now added 030624 - PE.
   //
   graphical_bonds_container make_environment_bonds_box(int atom_index,
							coot::protein_geometry *protein_geom_p) const;
   graphical_bonds_container make_symmetry_environment_bonds_box(int atom_index,
								 coot::protein_geometry *protein_geom_p) const;

   // instead of using xmap_is_filled[0] and atom_sel.n_selected_atoms
   // directly, lets provide these functions:

   bool has_map() const {

     if (!xmap_is_filled) {
       // If you see this, then a molecule is being displayed before
       // it is initialized, somehow.
       std::cout << "!!! Bogus bogus bogus bogus bogus bogus bogus!!!!!! " << std::endl;
       std::cout << "!!! molecule number apparently: " << imol_no << std::endl;
       return 0;
     } else {
       if (xmap_is_filled[0])
	 return 1;
       else
	 return 0;
     }
   }

   bool has_model() const {
      if (atom_sel.n_selected_atoms > 0)
	 return 1;
      else
	 return 0;
   }

   // for use when we want to tell the difference between a molecule
   // with no atoms that is open and one that is closed. I guess that
   // most has_model() usaged should be changed to open_molecule_p()
   // usage...
   bool open_molecule_p() const {
      if (atom_sel.mol)
	 return 1;
      else
	 return 0;
   } 

   bool is_displayed_p() const {
      bool i;
      if (has_model()) {
	 if (drawit) {
	    i = 1;
	 } else {
	    i = 0;
	 }
      } else {
	 if (has_map()) {
	    if (drawit_for_map) { 
	       i = 1;
	    } else {
	       i = 0;
	    }
	 } else {
	    i = 0;
	 }
      }
      return i;
   }


   // delete residue, typically for waters
   //
   // return the success status (0: failure to delete, 1 is deleted)
   // 
   short int delete_residue(const std::string &chain_id, int resno, 
                            const std::string &inscode);
   // Delete only the atoms of the residue that have the same altconf (as
   // the selected atom).  If the selected atom has altconf "", you
   // should call simply delete_residue().
   // 
   // Return 1 if at least one atom was deleted, else 0.
   //
   short int delete_residue_with_altconf(const std::string &chain_id,
					 int resno, 
					 const std::string &inscode,
					 const std::string &altconf);
   short int delete_residue_sidechain(const std::string &chain_id,
				      int resno,
				      const std::string &inscode);
   bool delete_atom(const std::string &chain_id, 
		    int resno,
		    const std::string &ins_code,
		    const std::string &atname,
		    const std::string &altconf);

   short int delete_residue_hydrogens(const std::string &chain_id, int resno,
				      const std::string &ins_code,
				      const std::string &altloc);

   int delete_atoms(const std::vector<coot::atom_spec_t> &atom_specs);


   // closing molecules, delete maps and atom sels as appropriate
   // and unset "filled" variables.  Set name_ to "".
   void close_yourself();

   // "Interactive" function that does make_backup().
   int mutate(int atom_index, const std::string &residue_type, short int do_stub_flag);
   // an alternate interface to the above:
   int mutate(int resno, const std::string &insertion_code,
	       const std::string &chain_id, const std::string &residue_type);
   // and another:
   int mutate(CResidue *res, const std::string &residue_type);

   // Here is something that does DNA/RNA
   int mutate_base(const coot::residue_spec_t &res_spec, std::string type);

   // and the biggie: lots of mutations/deletions/insertions from an
   // alignment:
   // 
   // We return a mutation container so that the calling function can
   // do a autofit rotamer on the mutated residues.
   //
   coot::chain_mutation_info_container_t align_and_mutate(const std::string chain_id,
							  const coot::fasta &seq,
							  bool renumber_residues_flag);
   
   void mutate_chain(const std::string &chain_id,
		     const coot::chain_mutation_info_container_t &mut_cont_info,
		     PCResidue *SelResidues,
		     int nSelResidues,
		     bool renumber_residues_flag);

   std::pair<bool, std::string>
   residue_type_next_residue_by_alignment(const coot::residue_spec_t &clicked_residue,
					  CChain *clicked_residue_chain_p, 
					  short int is_n_term_addition) const;

   // return a status flag (alignments done)
   std::pair<bool, std::vector<coot::chain_mutation_info_container_t> > 
   residue_mismatches() const;

   
   void make_backup_from_outside(); // when we have a multi mutate, we
				    // want the wrapper to make a
				    // backup when we start and set
				    // changes when when finish.
				    // Rather crap that this needs to
				    // be done externally, I think.
   int backups_state() const { return backup_this_molecule; }
   void set_have_unsaved_changes_from_outside();

   void mutate_internal(CResidue *residue, CResidue *std_residue);
   bool progressive_residues_in_chain_check_by_chain(const char *chain_id) const;
   void mutate_base_internal(CResidue *residue, CResidue *std_base);

   // multiple mutate: we don't want to put the results into an asc,
   // we simply want to make the replacement withouth user
   // intervention.  This is the externally visible function (via
   // c-interface imol selector wrapper).
   // 
   // Importantly, this function is called by some sort of wrapper
   // that is doing multiple mutations and therefore doesn't do a
   // backup.  However, backup should be done in the wrapping function
   //
   int mutate_single_multipart(int ires, const char *chain_id,
			       const std::string &target_res_type);
   //
   // and the functions that it uses:
   // (which returns success status - up from get_ori_to_this_res):
   //
   short int move_std_residue(CResidue* moving_residue, const CResidue *reference_residue) const;
   //
   // Get a deep copy:
// return NULL on failure
   CResidue *get_standard_residue_instance(const std::string &res_type);

   // Return the atom index of the "next" atom
   // -1 on failure.
   int intelligent_next_atom(const std::string &chain,
			     int resno,
			     const std::string &atom_name,
			     const std::string &ins_code);
   int intelligent_previous_atom(const std::string &chain,
				 int resno,
				 const std::string &atom_name,
				 const std::string &ins_code);
   CAtom *atom_intelligent(const std::string &chain_id, int resno,
			   const std::string &ins_code) const;

   // If there is a CA in this residue then return the index of that
   // atom, if not, then return the index of the first atom in the
   // residue.
   // 
   // Return -1 on no atoms in residue.
   //
   int intelligent_this_residue_atom(CResidue *res_p) const;
   CAtom *intelligent_this_residue_mmdb_atom(CResidue *res_p) const;

   // pointer atoms:
   void add_pointer_atom(coot::Cartesian pos);
   void add_typed_pointer_atom(coot::Cartesian pos, const std::string &type);

   // dummy atoms (not bonded)
   void add_dummy_atom(coot::Cartesian pos);
   void add_pointer_multiatom(CResidue *res_p, const coot::Cartesian &pos, const std::string &type);
   void add_multiple_dummies(CChain *chain_p,
			     const std::vector<coot::scored_skel_coord> &pos_position);
   void add_multiple_dummies(const std::vector<coot::scored_skel_coord> &pos_position); 
   void add_multiple_dummies(const std::vector<coot::Cartesian> &positions);




   // Return a vector of upto 3 positions of the most latestly added
   // atoms with the most lastest atom addition (that is the passed atom)
   // in the back() slot of the vector.
   //
   // Are we looking back along the chain (i.e. we are building forward (direction = 1))
   // or building backward (direction = 0)? 
   // 
   std::vector<clipper::Coord_orth> previous_baton_atom(const CAtom* latest_atom_addition,
							short int direction) const;

   clipper::Xmap<coot::SkeletonTreeNode> skeleton_treenodemap;
   short int skeleton_treenodemap_is_filled;
   void fill_skeleton_treenodemap();

   // baton atoms:
   CAtom *add_baton_atom(coot::Cartesian pos, 
			 int i_chain_start_resno, 
			 short int i_start_resno_active, // dont ignore it this time?
			 short int direction_flag); // return a pointer to
                                                    // the just added atom.

   // Return the position of the previous atom.
   // 
   std::pair<short int, CAtom *> baton_build_delete_last_residue();
   
   std::vector<coot::scored_skel_coord>
   next_ca_by_skel(const std::vector<clipper::Coord_orth> &previous_ca_positions,
		   const clipper::Coord_grid &coord_grid_start,
		   short int use_coord_grid_start_flag,
		   float ca_ca_bond_length,
		   float map_cut_off,
		   int max_skeleton_search_depth) const;
   std::pair<short int, clipper::Coord_grid> search_for_skeleton_near(const coot::Cartesian &pos) const;
   float density_at_point(const clipper::Coord_orth &pt) const; // return the values of map_list[0]
                                                                // at that point.

   
   //
   std::vector<std::string> save_state_command_strings() const {
      return save_state_command_strings_;
   }

   std::vector<std::string> set_map_colour_strings() const;
   std::vector<float> map_colours() const; // return an empty vector
					   // if closed or is a coords
					   // mol, 3 elements and 6
					   // elements for a
					   // difference map.

   // save yourself and update have_unsaved_changes_flag status
   // 
   int save_coordinates(const std::string filename); 
   std::string stripped_save_name_suggestion(); // sets coot_save_index maybe
   int Have_unsaved_changes_p() const;
   short int Have_modifications_p() const { return history_index > 0 ? 1 : 0;}
   short int Have_redoable_modifications_p() const ;
   void turn_off_backup() { backup_this_molecule = 0; }
   void turn_on_backup()  { backup_this_molecule = 1; }
   void apply_undo();
   void apply_redo();
   // Called from outside, if there is a backup file more recent
   // than the file name_ then restore from it.
   // 
   // Return 1 if the restore happened, 0 if not.
   // 
   short int execute_restore_from_recent_backup(std::string backup_file_name);
   coot::backup_file_info recent_backup_file_info() const;
   
   // For model view (go to atom)
   //
   std::vector<coot::model_view_residue_button_info_t>  // old style
   model_view_residue_button_labels() const;

   std::vector<coot::model_view_atom_tree_chain_t>
   model_view_residue_tree_labels() const;

   std::vector<coot::model_view_atom_button_info_t>
   model_view_atom_button_labels(char *chain_id, int seqno) const;

   // return the number of residues in chain with chain_id, return -1 on error
   // 
   int chain_n_residues(const char *chain_id) const;

   // Fourier stuff
   std::string Fourier_f_label()      const { return fourier_f_label; }
   std::string Fourier_phi_label()    const { return fourier_phi_label; }
   std::string Fourier_weight_label() const { return fourier_weight_label; }

   // refmac 
   //
   // sets private have_sensible_refmac_params
   // 
   void store_refmac_params(const std::string &mtz_filename,
			    const std::string &fobs_col,
			    const std::string &sigfobs_col,
			    const std::string &r_free_col,
			    int r_free_flag_sensible);
   std::vector<coot::atom_attribute_setting_help_t> get_refmac_params() const;

   void store_refmac_mtz_filename(const std::string &mtz_filename);
   void store_refmac_file_mtz_filename(const std::string &mtz_filename);

   void store_refmac_phase_params(const std::string &phi,
				  const std::string &fom,
				  const std::string &hla,
				  const std::string &hlb,
				  const std::string &hlc,
				  const std::string &hld);

   // more refmac stuff
   //
   int write_pdb_file(const std::string &filename); // not const because of shelx/name manip
   short int Have_sensible_refmac_params() const { return has_map() && have_sensible_refmac_params; }
   short int Have_refmac_phase_params()    const { return have_refmac_phase_params; }
   void increment_refmac_count() { refmac_count++; }
   int Refmac_count() const { return refmac_count; }
   std::string Refmac_mtz_filename() const { return refmac_mtz_filename; }
   std::string Refmac_file_mtz_filename() const { return refmac_file_mtz_filename; }
   std::string Refmac_fobs_col() const { return refmac_fobs_col; }
   std::string Refmac_sigfobs_col() const { return refmac_sigfobs_col; }
   std::string Refmac_r_free_col() const { return refmac_r_free_col; }
   std::string Refmac_phi_col() const { return refmac_phi_col; }
   std::string Refmac_fom_col() const { return refmac_fom_col; }
   std::string Refmac_hla_col() const { return refmac_hla_col; }
   std::string Refmac_hlb_col() const { return refmac_hlb_col; }
   std::string Refmac_hlc_col() const { return refmac_hlc_col; }
   std::string Refmac_hld_col() const { return refmac_hld_col; }
   short int Refmac_r_free_sensible() const { return refmac_r_free_flag_sensible; }
   void set_refmac_counter(int i) { refmac_count = i; } 
   void set_refmac_save_state_commands(std::string mtz_file_name,
				       std::string f_col,
				       std::string phi_col,
				       std::string weight_col,
				       bool use_weights,
				       bool is_diff_map,
				       std::string refmac_fobs_col,
				       std::string refmac_sigfobs_col,
				       std::string refmac_r_free_col,
				       bool refmac_r_free_flag_sensible);
   std::string Refmac_name_stub() const;
   std::string Refmac_in_name() const;
   std::string Refmac_out_name() const;
   std::string Refmac_mtz_out_name() const;
   std::string name_sans_extension(short int include_path_flag) const;
   CMMDBManager *get_residue_range_as_mol(const std::string &chain_id,
					  int resno_start,
					  int resno_end) const;


   // (best fit) rotamer stuff:
   //
   // This is the generic interface.  Inside auto_fit_best_rotamer, we
   // look at the rotamer_seach_mode and decide there if we want to
   // call the function that runs the backrub_rotamer mode.
   //
   float auto_fit_best_rotamer(int rotamer_seach_mode,
			       int resno,
			       const std::string &altloc,
			       const std::string &insertion_code,
			       const std::string &chain_id, int imol_map, int clash_flag,
			       float lowest_probability,
			       const coot::protein_geometry &pg);
   // (best fit) rotamer stuff:
   //
   float auto_fit_best_rotamer(int resno,
			       const std::string &altloc,
			       const std::string &insertion_code,
			       const std::string &chain_id, int imol_map, int clash_flag,
			       float lowest_probability,
			       const coot::protein_geometry &pg);
   // interface from atom picking (which simply gets the resno, altloc
   // etc form the atom_index and calls the above function.
   float auto_fit_best_rotamer(int rotamer_search_mode,
			       int atom_index, int imol_map, int clash_flag,
			       float lowest_probability,
			       const coot::protein_geometry &pg);

   // Internal.  Return succes status and score (we need status
   // because on failure, we should fall back to conventional rotamer
   // search).
   std::pair<bool,float> backrub_rotamer(const std::string &chain_id,
					 int res_no, 
					 const std::string &ins_code,
					 const std::string &alt_conf,
					 const coot::protein_geometry &pg);


   int set_residue_to_rotamer_number(coot::residue_spec_t res_spec, int rotamer_number,
				     const coot::protein_geometry &pg);

   // Add OXT atom:  Return status, 0 = fail, 1 = worked.
   // (use get_residue() to get the residue for this);
   // 
   short int add_OXT_to_residue(CResidue *residue);
   short int add_OXT_to_residue(int reso, const std::string &insertion_code,
				const std::string &chain_id); // external usage
   bool residue_has_oxt_p(CResidue *residue) const; // used by above.  Dont add if returns true.

   std::pair<short int, int>  last_residue_in_chain(const std::string &chain_id) const;
   std::pair<short int, int> first_residue_in_chain(const std::string &chain_id) const;

   // return NULL on no last residue.
   CResidue *last_residue_in_chain(CChain *chain_p) const;

   // 
   int residue_serial_number(int reso, const std::string &insertion_code,
			     const std::string &chain_id) const;  

   // cell and symmetry swapping
   // 
   // return an empty vector on failure, a vector of size 6 on success:
   std::pair<std::vector<float>, std::string> get_cell_and_symm() const;
   // 
   void set_mmdb_cell_and_symm(std::pair<std::vector<float>, std::string>);
   // return the success status of the set
   bool set_mmdb_symm(const std::string &s);

   //
   void set_bond_thickness(float t) { bond_width = t; }
   int bond_thickness() const { return int(bond_width); }

   // 
   void apply_atom_edit(const coot::select_atom_info &sai);
   void apply_atom_edits(const std::vector <coot::select_atom_info> &saiv);
   
   
   CChain *water_chain() const; // which chain contains just waters
				// (or is empty)?  return 0 if none.

   CChain *water_chain_from_shelx_ins() const; // the single chain

   std::pair<bool, std::string> chain_id_for_shelxl_residue_number(int resno) const;

   // return state, max_resno + 1, or 0, 1 of no residues in chain.
   // 
   std::pair<short int, int> next_residue_in_chain(CChain *w) const;


   // For environment distance application, we need to find the atom
   // nearest the centre of rotation
   std::pair<float, int> nearest_atom(const coot::Cartesian &pos) const; 

   // molecule-class-info-other functions:
   // 
   // For edit phi psi, tinker with the passed (moving atoms) asc
   // Return status, -1 on error (unable to change).
   short int residue_edit_phi_psi(atom_selection_container_t residue_asc, 
				  int atom_index, double phi, double psi); 

   // Return the residue thats for moving_atoms_asc as a molecule.
   //

   // whole_res_flag is for specifying if we want the whole residue
   // (1) or just the atoms of this residue with alt conf (and atoms
   // with altconf "") (0).
   atom_selection_container_t edit_residue_pull_residue(int atom_index,
							short int whole_res_flag);
   clipper::Coord_orth to_coord_orth(CAtom *atom) const;

   std::pair<double, double> get_phi_psi(int atom_index) const;

   // edit chi angles:
   // 
   // Return the number of chi angles for this residue:
   // Do not be mislead this is only a flag, *not* the number of chi
   // angles for this residue, i.e. does this residue have chi angles?
   //
   int N_chis(int atom_index); 

   // So that we can move around all the atoms of a ligand (typically)
   void translate_by(float x, float y, float z);
   void transform_by(mat44 mat); // can't make this const: mmdb probs.
   void transform_by(const clipper::RTop_orth &rtop);
   void transform_by(const clipper::RTop_orth &rtop, CResidue *res);

   //
   std::string name_for_display_manager() const; // stripped of path maybe
   std::string dotted_chopped_name() const;

   // external contour control (from saving parameters):
   void set_contour_level(float f);
   void set_contour_level_by_sigma(float f);
   short int change_contour(int direction); // return status 0 if it didn't happen.
   std::vector <std::string> get_map_contour_strings() const;
   std::vector <std::string> get_map_contour_sigma_step_strings() const;
   void set_contour_by_sigma_step(float v, short int state);
   // we ask of this molecule a question: contour_by_sigma?
   short int contoured_by_sigma_p() const;

   // a -other function (return the number of trimmed atoms):
   int trim_by_map(const clipper::Xmap<float> &xmap_in,
		   float map_level, short int delete_or_zero_occ_flag);

   // logical_operator_and_or_flag 0 for AND 1 for OR.
   // 
   std::vector <coot::atom_spec_t>
   find_water_baddies(float b_factor_lim, const clipper::Xmap<float> &xmap_in,
		      float map_sigma,
		      float outlier_sigma_level,
		      float min_dist, float max_dist,
		      short int part_occ_contact_flag,
		      short int zero_occ_flag,
		      short int logical_operator_and_or_flag);

   // return a vector of outliers
   std::vector <coot::atom_spec_t>
   check_waters_by_difference_map(const clipper::Xmap<float> &xmap_in,
				  float outlier_sigma_level) const;


   // 
   void set_map_is_difference_map();
   short int is_difference_map_p() const;


   // Scripting Refinement:
   int does_residue_exist_p(const std::string &chain_id, int resno, const std::string &inscode) const;


   // LMB surface
   coot::surface *cootsurface;

   // for widget label:
   std::string cell_text_with_embeded_newline() const;

   // for pointer distances
   std::vector<clipper::Coord_orth> distances_to_point(const clipper::Coord_orth &pt,
						       double min_dist,
						       double max_dist);

   // we may decide in future that sequence can be more sophisticated
   // than a simple string.
   // 
   // pair: chain_id sequence.  We need public access to these now
   // that they are going into the state script.
   std::vector<std::pair<std::string, std::string> > input_sequence;
   bool is_fasta_aa(const std::string &a) const;
   bool is_pir_aa  (const std::string &a) const;

   // sequence [a -other function]
   void assign_fasta_sequence(const std::string &chain_id, const std::string &seq);
   void assign_sequence(const clipper::Xmap<float> &xmap, const std::string &chain_id);
   std::vector<std::pair<std::string, std::string> > sequence_info() { return input_sequence; };

   void assign_pir_sequence(const std::string &chain_id, const std::string &seq);

   void assign_sequence_from_file(const std::string &filename);

   void assign_sequence_from_string(const std::string &chain_id, const std::string &seq);

   void delete_all_sequences_from_molecule();

   void delete_sequence_by_chain_id(const std::string &chain_id);


   // render option (other functions)
   coot::ray_trace_molecule_info fill_raster_model_info(); // messes with bond_colour_internal
   coot::ray_trace_molecule_info fill_raster_map_info(short int lev) const;

   // return a list of bad chiral volumes for this molecule:
   std::pair<std::vector<std::string>, std::vector<coot::atom_spec_t> > bad_chiral_volumes() const;

   // a other function
   float score_residue_range_fit_to_map(int res1, int res2, std::string altloc,
					std::string chain_id, int imol_for_map);
   // likewise:
   void fit_residue_range_to_map_by_simplex(int res1, int res2, std::string altloc,
					    std::string chain_id, int imol_for_map);

   // residue splitting (add alt conf)
   std::pair<bool,std::string> split_residue(int atom_index, int alt_conf_split_type);

   // internal (private) functions:
   std::pair<bool,std::string>
   split_residue_internal(CResidue *residue,
			  const std::string &altconf, 
			  const std::vector<std::string> &all_altconfs,
			  atom_selection_container_t residue_mol,
			  short int use_residue_mol_flag);
   void split_residue_then_rotamer(CResidue *residue, const std::string &altconf, 
				   const std::vector<std::string> &all_altconfs,
				   atom_selection_container_t residue_mol,
				   short int use_residue_mol_flag);
   void split_residue_internal(CResidue *residue);
   // We just added a new atom to a residue, now we need to adjust the
   // occupancy of the other atoms (so that we don't get residues with
   // atoms whose occupancy is greater than 1.0 (Care for SHELX molecule?)).
   void adjust_occupancy_other_residue_atoms(CAtom *at, CResidue *residue,
					     short int force_one_sum_flag);
   std::vector<std::string> get_residue_alt_confs(CResidue *res) const;
   std::string make_new_alt_conf(const std::vector<std::string> &residue_alt_confs, 
				 int alt_conf_split_type_in) const;
   short int have_atoms_for_rotamer(CResidue *res) const;
   CMMDBManager * create_mmdbmanager_from_res_selection(PCResidue *SelResidues, 
							int nSelResidues, 
							int have_flanking_residue_at_start,
							int have_flanking_residue_at_end, 
							const std::string &altconf,
							const std::string &chain_id_1,
							short int residue_from_alt_conf_split_flag);

   // merge molecules
   
   std::pair<int, std::vector<std::string> > merge_molecules(const std::vector<atom_selection_container_t> &add_molecules);
   int renumber_residue_range(const std::string &chain_id,
			      int start_resno, int last_resno, int offset);

   int change_residue_number(const std::string &chain_id_str,
			     int current_resno,
			     const std::string &current_inscode_str,
			     int new_resno,
			     const std::string &new_inscode_str);

   int renumber_waters(); // renumber all solvent changes so that
			  // their waters start at residue 1 and
			  // continue monotonically.

   void mark_atom_as_fixed(const coot::atom_spec_t &atom_spec, bool state);

   // validation
   void find_deviant_geometry(float strictness);


   // ===================== NCS ghosts ===============================

   void set_show_ghosts(short int istate); 
   void set_ghost_bond_thickness(float f);
   int update_ncs_ghosts();
   float ghost_bond_thickness() const {return int(ghost_bond_width);}
   int draw_ncs_ghosts_p() const { // needed for setting the Bond Parameters checkbutton
      return show_ghosts_flag;
   }

   std::vector<coot::ghost_molecule_display_t> NCS_ghosts() const;

   std::vector<std::vector<std::string> > ncs_ghost_chains() const;

   // Not const because we may modify ncs_ghosts by adding their ncs
   // operators: (and recall that this function is a coordinates
   // molecule function, so it uses its ncs operators on the given
   // map) to make new maps):
   std::vector<std::pair<clipper::Xmap<float>, std::string> > 
     ncs_averaged_maps(const clipper::Xmap<float> &xmap_in, float homology_lev);
   short int has_ncs_p() { return (ncs_ghosts.size() > 0) ? 1 : 0; }
   short int ncs_ghosts_have_rtops_p() const { return ncs_ghosts_have_rtops_flag;}
   int fill_ghost_info(short int do_rtops_flag,
		       float homology_lev);   // button callback fills ghosts
					      // if requested to display them.
   void add_ncs_ghost(const std::string &chain_id,
		      const std::string &target_chain_id,
		      const clipper::RTop_orth &rtop);

   void clear_ncs_ghost_matrices();

   // and the other way for CNS NCS users
   void add_strict_ncs_matrix(const std::string &chain_id,
			      const std::string &target_chain_id,
			      const coot::coot_mat44 &m);

   // the first value is if we should apply the matrix or not (we may not have ghosts)
   std::pair<bool, clipper::RTop_orth>
     apply_ncs_to_view_orientation(const clipper::Mat33<double> &current_view_mat,
				   const clipper::Coord_orth &current_position,
				   const std::string &current_chain,
				   const std::string &next_ncs_chain) const;
   
   short int show_strict_ncs_flag;

   // Not 'const' because we can do a fill_ghost_info if the NCS ghosts
   // do not have rtops.
   coot::ncs_differences_t ncs_chain_differences(std::string master_chain_id, 
						 float main_chain_weight);


   // ====================== SHELX stuff ======================================
   std::pair<int, std::string> write_shelx_ins_file(const std::string &filename);
   int read_shelx_ins_file(const std::string &filename);
   // return the success status, 0 for fail, 1 for good.
   int add_shelx_string_to_molecule(const std::string &str);
   bool is_from_shelx_ins() const { return is_from_shelx_ins_flag; }

   // data resolution, in A (or a negative number on error)
   float data_resolution() const { return data_resolution_; }

   // Change chain id
   // return -1 on a conflict
   // 
   std::pair<int, std::string> change_chain_id(const std::string &from_chain_id,
					       const std::string &to_chain_id,
					       short int use_resno_range,
					       int start_resno,
					       int end_resno);

   // return 1 on successful deletion, 0 on fail
   int delete_zone(const coot::residue_spec_t &res1, const coot::residue_spec_t &res2);

   // 
   void spin_search(clipper::Xmap<float> &xmap,
		    const std::string &chain_id,
		    int resno,
		    const std::string &ins_code,
		    const std::pair<std::string, std::string> &direction_atoms,
		    const std::vector<std::string> &moving_atoms_list); 


   // nomenclature errors
   // return a vector of the changed residues (used for updating the rotamer graph)
   std::vector<CResidue *> fix_nomenclature_errors(); // by looking for bad rotamers in
				                      // some residue types and alter ing
                            			      // the atom names to see if they get
				                      // more likely rotamers

   // ---- cis <-> trans conversion
   int cis_trans_conversion(const std::string &chain_id, int resno, const std::string &inscode);
   int cis_trans_conversion(CAtom *at, short int is_N_flag);
   int cis_trans_convert(PCResidue *mol_residues,   // internal function, make private
			 PCResidue *trans_residues, // or move into utils?
			 PCResidue *cis_residues);


   // ---- baton build redirection ----
   short int reverse_direction_of_fragment(const std::string &chain_id,
					   int resno);

   // ---- missing atoms ----
   //
   // Return a vector of residues that have missing atoms by dictionary
   // search.  missing_hydrogens_flag reflects if we want to count
   // residues that have missing hydrogens as residues with missing
   // atoms that should be part of the returned vector. Most of the
   // time, we don't care about hydrogens and the flag is 0.
   //
   // Pass and potentially add to geom_p
   coot::util::missing_atom_info
   missing_atoms(short int missing_hydrogens_flag, coot::protein_geometry *geom_p) const;

   // The function that uses missing atom info:
   coot::util::missing_atom_info
   fill_partial_residues(coot::protein_geometry *geom_p, int imol_refinement_map);
   // return 1 if the residue was filled, 0 if the residue was not found
   int fill_partial_residue(coot::residue_spec_t &residue_spec,
			     coot::protein_geometry *geom_p, int imol_refinement_map);

   // Ribosome People:
   int exchange_chain_ids_for_seg_ids(); 

   // public interface to chain copying
   void copy_chain(const std::string &from_chain, const std::string &to_chain);
   int copy_from_ncs_master_to_others(const std::string &master_chain_id);
   int copy_residue_range_from_ncs_master_to_other_using_ghost(std::string from_chain_id,
							       std::string to_chain_id,
							       int residue_range_1,
							       int residue_range_2);
   int copy_residue_range_from_ncs_master_to_others(const std::string &master_chain_id,
						    int resno_start, int resno_end);
   int set_ncs_master_chain(const std::string &new_master_chain_id, float homology_lev);
   // add ghosts
   void add_ncs_ghosts_no_explicit_master(const std::vector<std::string> &chain_ids,
					  const std::vector<std::vector<std::pair<std::string, int> > > &residue_types,
					  std::vector<short int> first_chain_of_this_type,
					  const std::vector<int> &chain_atom_selection_handles,
					  short int do_rtops_flag,
					  float homology_lev, 
					  bool allow_offset_flag);
   void add_ncs_ghosts_using_ncs_master(const std::string &master_chain_id,
					const std::vector<std::string> &chain_ids,
					const std::vector<std::vector<std::pair<std::string, int> > > &residue_types,
					const std::vector<int> &chain_atom_selection_handles,
					float homology_lev);

   // symmetry control
   // 
   // We fill the frame that's passed.  It is used to fill the
   // symmetry control widget (requested by Frank von Delft)
   void fill_symmetry_control_frame(GtkWidget *dialog) const;
   int   symmetry_whole_chain_flag;
   int   symmetry_as_calphas;
   int   symmetry_colour_by_symop_flag; 
   short int symmetry_rotate_colour_map_flag; // do we want symmetry of other
						     // molecules to have a different
						     // colour [MOL]?

   // ncs control
   void fill_ncs_control_frame(GtkWidget *dialog) const; // called for every coords mol
   void fill_ncs_control_frame_internal(GtkWidget *dialog) const; // called if needed.
   void set_display_ncs_ghost_chain(int ichain, int state);
   void ncs_control_change_ncs_master_to_chain_update_widget(GtkWidget *w, int ichain) const;

   std::vector<std::string> get_symop_strings() const;


   // Replace the atoms in this molecule by those in the given atom selection.
   int replace_fragment(atom_selection_container_t asc);

   int set_atom_attribute(std::string chain_id, int resno, std::string ins_code,
			  std::string atom_name, std::string alt_conf,
			  std::string attribute_name, float val);

   int set_atom_string_attribute(std::string chain_id, int resno, std::string ins_code,
				 std::string atom_name, std::string alt_conf,
				 std::string attribute_name, std::string val_str);

   int set_atom_attributes(const std::vector<coot::atom_attribute_setting_t> &v);


   coot::at_dist_info_t closest_atom(const coot::Cartesian &pt) const;


   // cleaner interface to molecule's attributes:
   std::pair<bool, clipper::Spacegroup> space_group() const;
   std::pair<bool, clipper::Cell> cell() const;


   // 
   clipper::Coord_orth find_peak_along_line(const clipper::Coord_orth &p1,
					    const clipper::Coord_orth &p2) const;
   //  Throw an exception if peak not found (about the contour level for this map)
   clipper::Coord_orth find_peak_along_line_favour_front(const clipper::Coord_orth &p1,
							 const clipper::Coord_orth &p2) const;

   coot::minimol::molecule eigen_flip_residue(const std::string &chain_id, int resno); 

   // replace molecule
   int replace_molecule(CMMDBManager *mol);


   // add a factor to scale the colours in b factor representation:.
   // It goes into the atom_sel.mol
   void set_b_factor_bonds_scale_factor(float f);

   int
   apply_sequence(int imol_map, CMMDBManager *mol,
		  std::vector<coot::residue_spec_t> mmdb_residues,
		  std::string best_seq, std::string chain_id,
		  int resno_offset,
		  const coot::protein_geometry &pg);

   int delete_all_except_res(CResidue *res);

   // EM map function
   int scale_cell(float fac_u, float fac_v, float fac_w);

   // 
   void sharpen(float b_factor);
   void sharpen_using_orig_fphis(float b_factor);

   // reorder the chains in the models
   void sort_chains();

   int add_additional_representation(int representation_type,
				     const int &bonds_box_type_in, 
				     float bonds_width,
				     bool draw_hydrogens_flag,
				     const coot::atom_selection_info_t &info,
				     GtkWidget *display_control_window,
				     const gl_context_info_t &glci); 

   int adjust_additional_representation(int represenation_number, 
					const int &bonds_box_type_in, 
					float bonds_width,
					bool draw_hydrogens_flag,
					const coot::atom_selection_info_t &info, 
					bool show_it_flag_in); 

   void clear_additional_representation(int representation_number);
   void set_show_additional_representation(int representation_number, bool on_off_flag);
   void set_show_all_additional_representations(bool on_off_flag);
   // 
   std::vector<coot::residue_spec_t> residues_near_residue(const coot::residue_spec_t &rspec, float radius) const; 

   void remove_ter_atoms(const coot::residue_spec_t &spec); // from all models

   // c.f. progressive_residues_in_chain_check_by_chain()
   // bool residues_in_order_p(std::string &chain_id) const;
   
   coot::validation_graphs_t validation_graphs;


   void apply_charges(const coot::protein_geometry &geom);

   // return the dipole (a copy) and its number
   // 
   std::pair<coot::dipole, int>
   add_dipole(const std::vector<coot::residue_spec_t> &res_specs,
	      const coot::protein_geometry &geom);

   void delete_dipole(int dipole_number);

   // return the number of new hetatoms
   int assign_hetatms();

   // move waters so that they are around H-bonders (non-C) in protein.
   // return the number of moved atoms
   int move_waters_to_around_protein();


   // Return the maximum minimum distance of waters to protein atoms.
   // return something negative when we can't do above (no protein
   // atoms or no water atoms).
   float max_water_distance();

   // jiggle residue (a specific, useful/typical interface to jiggling).
   float fit_to_map_by_random_jiggle(coot::residue_spec_t &spec,
				     const clipper::Xmap<float> &xmap,
				     float map_sigma,
				     int n_trials,
				     float jiggle_scale_factor);

   // Random rotation and translation (translations scaled by
   // jiggle_scale_factor).
   //
   // return the z-weighted fit to density score of the atom
   // selection.
   // 
   // called by above
   float fit_to_map_by_random_jiggle(PPCAtom atom_selection,
				     int n_atoms,
				     const clipper::Xmap<float> &xmap,
				     float map_sigma,
				     int n_trials,
				     float jiggle_scale_factor);


   // return a fitted molecule
   coot::minimol::molecule rigid_body_fit(const coot::minimol::molecule &mol_in,
					  const clipper::Xmap<float> &xmap,
					  float map_sigma) const;

   // ---- utility function --- (so that we know to delete hydrogens
   // from HETATM molecule before merging with this one
   //
   bool molecule_has_hydrogens() const;

   // -------- simply print it (at the moment) --------------
   void print_secondary_structure_info();

};

#endif // MOLECULE_CLASS_INFO_T
