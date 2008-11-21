/* src/graphics-info.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 by The University of York
 * Copyright 2007 by Paul Emsley
 * Copyright 2007, 2008 by The University of Oxford
 * 
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

// -*-c++-*-
#ifndef GRAPHICS_INFO_H
#define GRAPHICS_INFO_H

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif // HAVE_VECTOR

// need gtk things
#include <gtk/gtk.h>
#include <GL/gl.h>

#if (GTK_MAJOR_VERSION == 1)
#include <gtkgl/gtkglarea.h>
#else  // GTK2 build then:
#include <gdk/gdkglconfig.h>
#include <gdk/gdkgldrawable.h>
#include <gtk/gtkgl.h>
#endif // (GTK_MAJOR_VERSION == 1)

#include "Cartesian.h"
#include "mgtree.h"
#include "pick.h"
#include "clipper/core/xmap.h"

#include "rotamer.hh"

#include "protein-geometry.hh"

#include "molecule-class-info.h"

#ifdef HAVE_SSMLIB
#include "ssm_align.h"
#endif

#include "db-main.hh"
#include "CalphaBuild.hh"
#include "simple-restraint.hh"

#include "history_list.hh"
#include "sequence-view.hh"
#include "rama_plot.hh" 
#include "geometry-graphs.hh"
#include "coot-utils.hh"
#include "coot-coord-utils.hh"
#include "coot-coord-extras.hh"

#include "positioned-widgets.h"
#include "geometry-graphs.hh"

#include "coot-database.hh"

#if (GTK_MAJOR_VERSION > 1) 
#include "restraints-editor.hh"
#endif 

#ifdef USE_GUILE
#include <libguile.h>
#endif 

#ifdef HAVE_GSL
#else
// fake in refinement_results_t
// BL says:: probably shouldnt be here as already defined in ../ideal/simple-restraints.hh
// FIXME
namespace coot { 
   enum geometry_graph_type {GEOMETRY_GRAPH_GEOMETRY,
                             GEOMETRY_GRAPH_B_FACTOR,
                             GEOMETRY_GRAPH_DENSITY_FIT,
                             GEOMETRY_GRAPH_OMEGA_DISTORTION,
                             GEOMETRY_GRAPH_ROTAMER,
                             GEOMETRY_GRAPH_NCS_DIFFS,
                             SEQUENCE_VIEW,
                             RAMACHANDRAN_PLOT
   };
//  class refinement_results_t {
//  public:
//      short int found_restraints_flag; // 0 or 1 (if we found restraints or not).
//      int progress; // GSL_ENOPROG, GSL_CONTINUE, GSL_SUCCESS, GSL_ENOPROG (no progress)
//      std::string info;
//      refinement_results_t(short int frf, int prog_in, std::string info_in) {
//         found_restraints_flag = frf;
//         info = info_in;
//         progress = prog_in;
//     }
 // };
}
#endif // HAVE_GSL

#include "generic-display-object.hh"


namespace coot { 
   enum {MAX_LABELLED_ATOMS = 200};
   enum {NEW_COORDS_ADD = 1, NEW_COORDS_REPLACE = 2, NEW_COORDS_REPLACE_CHANGE_ALTCONF = 3, 
	 NEW_COORDS_INSERT = 4,
         NEW_COORDS_INSERT_CHANGE_ALTCONF = 5};
   enum {STATE_SCM = 1, STATE_PYTHON = 2};
   enum undo_type { UNDO, REDO};
   enum display_mode_type {MONO_MODE=0, HARDWARE_STEREO_MODE=1, 
			   SIDE_BY_SIDE_STEREO=2,  // cross-eye
			   DTI_SIDE_BY_SIDE_STEREO=3,
                           SIDE_BY_SIDE_STEREO_WALL_EYE=4};
   enum accept_reject_text_type { CHI_SQUAREDS, CHIRAL_CENTRES};
   enum chooser_selector_type { OLD_STYLE, CHOOSER_STYLE };
   enum chooser_overwrite_type { CHOOSER_OVERWRITE, CHOOSER_OVERWRITE_PROTECT };
   enum accept_reject_dialog_type { DIALOG, DIALOG_DOCKED };
   enum accept_reject_dialog_docked_type { DIALOG_DOCKED_HIDE = 0,
                                           DIALOG_DOCKED_SHOW = 1};
   enum fixed_atom_pick_state_t { FIXED_ATOM_NO_PICK = 0, 
				  FIXED_ATOM_FIX = 1, 
				  FIXED_ATOM_UNFIX = 2 };
   namespace model_toolbar {
     enum toolbar_position_type { RIGHT  = 0,
                                  LEFT   = 1,
                                  TOP    = 2,
                                  BOTTOM = 3};
   }
   namespace refmac {
     enum refmac_refinement_method_type { RESTRAINED = 0,
					  RIGID_BODY = 1 };
     enum refmac_phase_input_type { NO_PHASES = 0,
				    PHASE_FOM = 1 ,
				    HL        = 2 };
     enum refmac_use_tls_type { TLS_OFF = 0,
				TLS_ON  = 1};
     enum refmac_use_twin_type { TWIN_OFF = 0,
				 TWIN_ON  = 1};
     enum refmac_use_sad_type { SAD_OFF = 0,
				SAD_ON = 1};
     enum refmac_use_ncs_type { NCS_OFF = 0,
				NCS_ON  = 1};
     enum refmac_use_intensities_type { AMPLITUDES   = 0,
					INTENSITIES  = 1};
     enum refmac_used_mtz_file_type { MAP = 0,
				      MTZ = 1};

     class sad_atom_info_t {
     public:
	std::string atom_name;
	float fp;
	float fpp;
	float lambda;
        sad_atom_info_t(std::string atom_name_in, float &fp_in,
			float &fpp_in, float &lambda_in) {
	  atom_name = atom_name_in;
	  fp = fp_in;
	  fpp = fpp_in;
	  lambda = lambda_in;
	}
     };
   }

#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)
   void set_validation_graph(int imol, coot::geometry_graph_type type, GtkWidget *dialog);
   GtkWidget *get_validation_graph(int imol, coot::geometry_graph_type type); 
#endif // defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)

   class coord_orth_triple {
   public:
      clipper::Coord_orth p1;
      clipper::Coord_orth p2;
      clipper::Coord_orth p3;
   };



   class intermediate_atom_distance_t {
     Cartesian static_position;
     CAtom *dynamic_atom;
     bool static_pos_filled_flag;
     
   public: 
     intermediate_atom_distance_t() { 
       dynamic_atom = 0;
       static_pos_filled_flag = 0;
     }
     intermediate_atom_distance_t(const coot::Cartesian &pt) { 
       dynamic_atom = 0;
       static_position = pt;
       static_pos_filled_flag = 1;
     }
     intermediate_atom_distance_t(CAtom *at) { 
       dynamic_atom = at;
       static_pos_filled_flag = 0;
     }
     void draw_dynamic_distance() const;
     bool static_position_is_filled() const { return static_pos_filled_flag; }
     bool atom_is_filled() const {
       return (dynamic_atom != 0);
     }
     void add_atom(CAtom *at) {
       dynamic_atom = at;
     }
     void add_static_point(Cartesian &pt) {
       static_position = pt;
       static_pos_filled_flag = 1;
     }
     bool filled() const {
       return (static_pos_filled_flag && dynamic_atom);
     }
   };

   class ramachandran_points_container_t {

      std::vector<std::pair<std::string, clipper::Coord_orth> > points;
      

   public: 
      ramachandran_points_container_t() {};
      void clear() {
	 points.resize(0);
      }
      std::pair<short int, clipper::Coord_orth> get(const std::string &atom_name) const { 
	 std::pair<short int, clipper::Coord_orth> v;
	 v.first = 0;
	 for (unsigned int i=0; i<points.size(); i++) { 
	    if (atom_name == points[i].first) { 
	       v.first = 1;
	       v.second = points[i].second;
	       break;
	    }
	 }
	 return v;
      }
      void add(const std::string &s, const clipper::Coord_orth &p) { 
	 points.push_back(std::pair<std::string, clipper::Coord_orth> (s,p));
      } 

   };

   class graph_rotamer_info_t {
   public:
     std::string chain_id;
     int resno;
     std::string inscode;
     float probability;
     std::string rotamer_name;
     graph_rotamer_info_t(const std::string &chain_id_in, int resno_in, const std::string &inscode_in, float prob_in, const std::string &rotamer_name_in) { 
       chain_id = chain_id_in;
       resno = resno_in;
       inscode = inscode_in;
       probability = prob_in;
       rotamer_name = rotamer_name_in;
     } 
   };

   // To pass rotamer info back to scripting layer, for testing. 
   // Hmmmm.. confusing names, perhaps.
   // 
   class rotamer_graphs_info_t { 
   public:
     std::vector<graph_rotamer_info_t> info;
   }; 

   class diff_map_peak_helper_data {
   public:
      int ipeak;
      clipper::Coord_orth pos;
   };

   // This is for a Pymol-like feature to switch between views:
   // 
   class view_info_t {
   public:
      float zoom;
      coot::Cartesian rotation_centre;
      std::string view_name;
      std::string description;
      bool is_simple_spin_view_flag;
      bool is_action_view_flag;
      int n_spin_steps;
      float degrees_per_step;
      float quat[4];
      std::string action;
      view_info_t(float *quat_in, const coot::Cartesian &rot_centre_in,
		  float zoom_in, const std::string &view_name_in) {
	is_simple_spin_view_flag = 0;
	is_action_view_flag = 0;
	 zoom = zoom_in;
	 rotation_centre = rot_centre_in;
	 view_name = view_name_in;
	 for (int i=0; i<4; i++) 
	    quat[i] = quat_in[i];
      }
      view_info_t() {
      	is_simple_spin_view_flag = 0;
	is_action_view_flag = 0;
      }
      // a spin view 
      view_info_t(const std::string &view_name_in, int nsteps, float degrees_total) {
	is_simple_spin_view_flag = 1;
	is_action_view_flag = 0;
	view_name = view_name_in;
	n_spin_steps = nsteps;
	if (n_spin_steps > 0) 
	  degrees_per_step = degrees_total/n_spin_steps;
	else 
	  degrees_per_step = 0.5;
      }
      // an action view
      view_info_t(const std::string &view_name_in, const std::string &funct) {
	is_action_view_flag = 1;
	view_name = view_name_in;
	action = funct;
      }
      bool matches_view (const coot::view_info_t &view) const;
      float quat_length() const;
      void add_description(const std::string &descr) { 
	description = descr;
      }
      static view_info_t interpolate(const view_info_t &view1,
				     const view_info_t &view2,
				     int n_steps);
      static float dot_product(const view_info_t &view1,
			       const view_info_t &view2);

      friend std::ofstream& operator<<(std::ofstream &stream, 
				       view_info_t &view);
   };

   class console_display_commands_t {
   public:
     bool display_commands_flag;
     bool hilight_flag;
     bool hilight_colour_flag; 
     int colour_prefix;
     console_display_commands_t() { 
       // hilighting
       display_commands_flag = 1;
       hilight_flag = 1;
       colour_prefix = 4;
       hilight_colour_flag = 0;
     }
   };

  // for preferences
  class preference_info_t {
    
  public:
    int preference_type;   // e.g. PREFERENCES_bla
    int ivalue1;
    int ivalue2;
    float fvalue1;
    float fvalue2;
    float fvalue3;
  };

  class preferences_icon_info_t {
  
  public:
    int icon_pos;
    std::string icon_filename;
    std::string icon_text;
    std::string icon_widget;
    int show_hide_flag;
    int default_show_flag;
    preferences_icon_info_t(int icon_pos_in,
			    std::string icon_filename_in,
			    std::string icon_text_in,
			    std::string icon_widget_in,
			    int show_hide_flag_in,
			    int default_show_flag_in) {
	icon_pos = icon_pos_in;
	icon_filename = icon_filename_in;
	icon_text = icon_text_in;
	icon_widget = icon_widget_in;
	show_hide_flag = show_hide_flag_in;
	default_show_flag = default_show_flag_in;
    }
    void hide() {
	show_hide_flag = 0;
    }
    void show() {
	show_hide_flag = 1;
    }
  };

  class command_line_commands_t { 
  public:
    std::vector<std::string> commands;
    bool is_python;
    command_line_commands_t() { 
      is_python = 0;
    } 
  }; 
  
} 

#include "select-atom-info.hh"


class graphics_info_t {

   static int n_molecules_max;

   static short int in_side_by_side_stereo_mode; 

   static double mouse_begin_x, mouse_begin_y; 

   static float rotation_centre_x;
   static float rotation_centre_y;
   static float rotation_centre_z;

   static float old_rotation_centre_x;
   static float old_rotation_centre_y;
   static float old_rotation_centre_z;

   static long int T0; 
   static long int Frames;
   // 
   static int show_fps_flag; 
   static short int active_map_drag_flag;


   void smooth_scroll_maybe(float x, float y, float z,
			    short int do_zoom_and_move_flag,
			    float target_zoom);

   static float trackball_size;

   // Go To Atom privates: 
   static std::string go_to_atom_chain_;
   static std::string go_to_atom_atom_name_; 
   static int         go_to_atom_residue_; 
   static int         go_to_atom_molecule_;
   static std::string go_to_atom_atom_altLoc_;
   static std::string go_to_atom_inscode_;

   // distance static
   static coot::Cartesian distance_pos_1;

   // angle/torsion static 
   static coot::Cartesian angle_tor_pos_1;
   static coot::Cartesian angle_tor_pos_2;
   static coot::Cartesian angle_tor_pos_3;
   static coot::Cartesian angle_tor_pos_4;

   // 
   static GtkWidget *display_control_window_;

   //
   static graphical_bonds_container regularize_object_bonds_box;
   static graphical_bonds_container environment_object_bonds_box;
   static graphical_bonds_container symmetry_environment_object_bonds_box;

   // 
   static GdkModifierType button_1_mask_;
   static GdkModifierType button_2_mask_;
   static GdkModifierType button_3_mask_;

   static int find_ligand_protein_mol_;
   static int find_ligand_map_mol_;
   static std::vector<std::pair<int, bool> > *find_ligand_ligand_mols_; // contain a molecule number 
                                                                        // and flag for is_wiggly?
   // static std::vector<short int> *find_ligand_wiggly_ligands_; // incorporporated above.

   static coot::protein_geometry* geom_p;

   static coot::rotamer_probability_tables rot_prob_tables;
   coot::rotamer_probability_info_t get_rotamer_probability(CResidue *res,
							    CMMDBManager *mol,
							    float lowest_probability,
							    short int add_extra_PHE_and_TYR_rotamers_flag);

   static atom_selection_container_t *moving_atoms_asc;
   CResidue *get_first_res_of_moving_atoms();
   static int imol_moving_atoms;
   static int imol_refinement_map;
   static int moving_atoms_n_cis_peptides; 

   //
   static int undo_molecule; // -1 initially

   // No, we don't want MMDBManager in the include files.
   // (or indeed mmdb-extras, etc...)
   // 
/*    //  */
/*    void create_regularized_graphical_object(const std::string chain_id_1, */
/* 						     int resno_1, */
/* 						     int resno_2, */
/* 						     MMDBManager *results);  */


   // db-main
   // 
   static coot::db_main main_chain;

   // flash picked intermediate atom - try to stop Erik going insane.
   static bool flash_intermediate_atom_pick_flag;
   static clipper::Coord_orth intermediate_flash_point;
   // function that uses this, picked_intermediate_atom_graphics_object() is public

   void environment_graphics_object_internal(const graphical_bonds_container &env_bonds_box) const;

   void read_standard_residues();   // for mutation, we have
				    // pre-prepared a pdb file with
				    // residues in Buccaneer "Standard
				    // Orientation", idealized and in
				    // the most likely rotamer.


   // state
   static short int state_language; // a bit-tested variable, 1 = scheme,
                                    // 2 = python, 3 = both.

   std::string state_command(const std::string &str,                 short int state_lang) const;
   std::string state_command(const std::string &str, int i,          short int state_lang) const;
   std::string state_command(const std::string &str, int i1, int i2, short int state_lang) const;
   std::string state_command(const std::string &str, float f,        short int state_lang) const;
   std::string state_command(const std::string &str, float f,        short int state_lang, short unsigned int v) const;
   std::string state_command(const std::string &str, float f1, float f2, float f3, short int state_lang) const;
   std::string state_command(const std::string &str, const std::string &str2, short int state_lang); 

   // baton stuff
   static coot::Cartesian baton_root;
   static coot::Cartesian baton_tip;
   static float baton_length;
   static std::vector<coot::scored_skel_coord> *baton_next_ca_options;
   // baton_previous_ca_positions->back() is the point closest to the new
   // baton tip (is the baton root)
   static std::vector<clipper::Coord_orth> *baton_previous_ca_positions; // up to 3.
   void baton_next_directions(int imol_for_skel, const CAtom *atom, const coot::Cartesian& pos,
			      const clipper::Coord_grid &cg_start,
			      short int use_cg_start);
   coot::Cartesian baton_tip_by_ca_option(int index) const;
   static int baton_next_ca_options_index;
   static int user_set_baton_atom_molecule; // -1 if not set (default).
   // imol_for_baton_atoms?  see baton_build_atoms_molecule(); 
   
   // starting point and direction parameters:
   static int baton_build_start_resno;
   static std::string baton_build_chain_id;
   static short int baton_build_direction_flag; // +1 for forwards, -1 for backwards
   static short int baton_build_params_active; // usually they are
					       // ignored because we
					       // get the residue
					       // number, etc from the
					       // previous atoms.

   
   int imol_for_skeleton() const;
   /* for all the GtkItem:: and GtkTreeItem:: signals */
#if (GTK_MAJOR_VERSION == 1) || defined (GTK_ENABLE_BROKEN)

   static void residue_tree_view_itemsignal( GtkWidget *item,
				      gchar     *signame);
#endif

   void create_molecule_and_display(std::vector<coot::scored_skel_coord> &pos_position,
				    const std::string &molname);
   // as above, except we update molecule with name molname to the
   // pos_positions (and delete everything else).
   // 
   void update_molecule_to(std::vector<coot::scored_skel_coord> &pos_position,
			   const std::string &molname);
   int create_empty_molecule(const std::string &molname);

   // symm_atom_pick (public) uses this (private) function:

   void 
   fill_hybrid_atoms(std::vector<coot::clip_hybrid_atom> *hybrid_atoms, 
		     const atom_selection_container_t &asc,
		     const clipper::Spacegroup &spg, 
		     const clipper::Cell &cell,
		     const std::pair<symm_trans_t, Cell_Translation> &symm_trans) const;
   
   // directory saving for fileselection
   //
   static std::string directory_for_fileselection;
   static std::string directory_for_saving_for_fileselection;
#if (GTK_MAJOR_VERSION > 1)
   static std::string directory_for_filechooser;
   static std::string directory_for_saving_for_filechooser;
#endif // GTK_MAJOR_VERSION

   // distance object vector, and angle
   static std::vector<std::pair<clipper::Coord_orth, clipper::Coord_orth> > *distance_object_vec;
   static std::vector<coot::coord_orth_triple> *angle_object_vec;

   static int moving_atoms_dragged_atom_index;
#ifdef  HAVE_GSL
   static coot::restraints_container_t last_restraints;
#endif // HAVE_GSL   
   // the mode flag is public:

   // edit ramachandran store:
   static coot::ramachandran_points_container_t rama_points;
   std::pair<std::pair<double, double>, std::pair<double, double> >
   phi_psi_pairs_from_moving_atoms();

   // end points of backbone torsion moving
   static clipper::Coord_orth backbone_torsion_end_ca_1;
   static clipper::Coord_orth backbone_torsion_end_ca_2;
   static int backbone_torsion_peptide_button_start_pos_x;
   static int backbone_torsion_peptide_button_start_pos_y;
   static int backbone_torsion_carbonyl_button_start_pos_x;
   static int backbone_torsion_carbonyl_button_start_pos_y;

   clipper::Coord_orth moving_atoms_centre() const;

   void set_edit_backbone_adjustments(GtkWidget *widget); 
   static void edit_backbone_peptide_changed_func (GtkAdjustment *adj, GtkWidget *window); // callback
   static void edit_backbone_carbonyl_changed_func(GtkAdjustment *adj, GtkWidget *window); // callback

   void check_and_warn_bad_chirals_and_cis_peptides() const;

#if defined(HAVE_GNOME_CANVAS) || defined(HAVE_GTK_CANVAS)
   void handle_rama_plot_update(coot::rama_plot *plot);
#endif

#ifdef HAVE_GSL   
#if defined(HAVE_GNOME_CANVAS) || defined(HAVE_GTK_CANVAS)
   // Geometry Graphs:
   coot::geometry_graphs * geometry_graph_dialog_to_object(GtkWidget *w) const {
      coot::geometry_graphs *gr = NULL;
      if (!w) {
	 std::cout << "ERROR:: null w in geometry_graph_dialog_to_object" << std::endl;
      } else {
	 GtkWidget *local_graph_dialog = lookup_widget(w, "geometry_graph_canvas");
	 if (local_graph_dialog)
	    gr = (coot::geometry_graphs *) (gtk_object_get_user_data(GTK_OBJECT(local_graph_dialog)));
      }
      return gr;
   }
   std::vector<coot::geometry_distortion_info_container_t>
   geometric_distortions_from_mol(const atom_selection_container_t &asc);
#endif // HAVE_GSL
#endif // defined(HAVE_GNOME_CANVAS) || defined(HAVE_GTK_CANVAS)

   int  check_if_in_regularize_define(GdkEventButton *event);
   int  check_if_in_refine_define(GdkEventButton *event);
   int  check_if_in_rigid_body_define(GdkEventButton *event);
   int  check_if_in_rot_trans_define(GdkEventButton *event);
   void check_if_in_residue_info_define(GdkEventButton *event);
   void check_if_in_geometry_range_defines(GdkEventButton *event);
   void check_if_in_pepflip_define(GdkEventButton *event);
   void check_if_in_terminal_residue_define(GdkEventButton *event);
   void check_if_in_db_main_define(GdkEventButton *event);
   void check_if_in_delete_item_define(GdkEventButton *event,
				       const GdkModifierType &state);
   void check_if_in_rotamer_define(GdkEventButton *event);
   void check_if_in_mutate_define(GdkEventButton *event);
   void check_if_in_mutate_auto_fit_define(GdkEventButton *event);
   void check_if_in_auto_fit_define(GdkEventButton *event);
   void check_if_in_add_alt_conf_define(GdkEventButton *event);
   void check_if_in_cis_trans_convertion_define(GdkEventButton *event);
   void check_if_in_edit_phi_psi_define(GdkEventButton *event);
   void check_if_in_edit_chi_angles_define(GdkEventButton *event);
   void check_if_in_180_degree_flip_define(GdkEventButton *event);
   void check_if_in_edit_backbone_torsion_define(GdkEventButton *event);
   void check_if_in_save_symmetry_define(GdkEventButton *event);
   void check_if_in_reverse_direction_define(GdkEventButton *event);
   void check_if_in_lsq_plane_define(GdkEventButton *event);
   void check_if_in_lsq_plane_deviant_atom_define(GdkEventButton *event);
   void check_if_in_torsion_general_define(GdkEventButton *event);
   void check_if_in_fixed_atom_define(GdkEventButton *event,
				      const GdkModifierType &state); // can use Ctrl key
   static std::vector<std::string> model_fit_refine_toggle_button_name_list();
   static std::vector<std::string> model_fit_refine_button_name_list();
   static std::vector<std::string> other_modelling_tools_toggle_button_name_list();
   static std::vector<std::string> other_modelling_tools_button_name_list();

   // ----------------------------------------------------------------
   //             public:
   // ----------------------------------------------------------------
   
public:

   enum { USE_PYTHON_STATE_COMMANDS = 2, USE_SCM_STATE_COMMANDS = 1 };

   // 
   void initialize_molecules() { 
     
      // I don't like the way that this is currently done. 
      // 
      // I would like to pass i, the molecule number to the constuctor.
      // How do I do that?      You can't.
      // 
      // Actually you can, just use it as you would a constructor.
      //
      // std::cout << "initializing molecules..."; 
      // molecules = new molecule_class_info_t[n_molecules_max];
     // std::cout << "done" << std::endl;
/*       for (int i=0; i<n_molecules_max; i++) { */
/*          molecules[i].set_mol_number(i);  */
/*       } */

/* Old style array of molecules code */
/*       for (int i=0; i<n_molecules_max; i++) { */
/* 	 dynarama_is_displayed[i]      = NULL; */
/* 	 sequence_view_is_displayed[i] = NULL; */
/* 	 geometry_graph[i]             = NULL; */
/* 	 b_factor_variance_graph[i]    = NULL; */
/* 	 residue_density_fit_graph[i]  = NULL; */
/* 	 omega_distortion_graph[i]     = NULL; */
/* 	 rotamer_graph[i]              = NULL; */
/* 	 ncs_diffs_graph[i]            = NULL; */
/*       } */

   }

   void init() { 

     for (int i=0; i<4; i++) 
       background_colour[i] = 0.0;

     find_ligand_ligand_mols_ = new std::vector<std::pair<int, bool> >;
     // find_ligand_wiggly_ligands_ = new std::vector<short int>; // now incorporated above
      geom_p = new coot::protein_geometry;
      cif_dictionary_read_number = geom_p->init_standard();
      geom_p->add_planar_peptide_restraint();

      // rotamer probabilitiles
      // guess we shall rather use COOT_DATA_DIR and only as fallback PKGDATADIR?!
      // maybe only for windows!?
      std::string tables_dir = PKGDATADIR;
#ifdef WINDOWS_MINGW
      char *data_dir = getenv("COOT_DATA_DIR");
      if (data_dir) {
	tables_dir = data_dir;
      }
#endif // MINGW
      tables_dir += "/rama-data";
      rot_prob_tables.set_tables_dir(tables_dir);

      moving_atoms_asc = new atom_selection_container_t;
      moving_atoms_asc->mol = NULL;
      moving_atoms_asc->atom_selection = NULL;
      moving_atoms_asc->n_selected_atoms = 0;

      standard_residues_asc.read_success = 0;
      standard_residues_asc.n_selected_atoms = 0;
      read_standard_residues(); // updates read_success

      symmetry_colour_merge_weight = 0.5; // 0.0 -> 1.0

      symmetry_colour = std::vector<double> (4, 0.5);
      symmetry_colour[0] = 0.1;
      symmetry_colour[1] = 0.2;
      symmetry_colour[2] = 0.8;

      // use_graphics_interface_flag = 1;  don't (re)set this here, 
      // it is set as a static and possibly modified by immediate 
      // handling of command line data in main.cc

      // moving_atoms_asc gets filled in copy_mol_and_regularize, not
      // here.

      // db_main = NULL;

      // command line scripts:
      command_line_scripts = new std::vector<std::string>;

      // LSQ matching info
      lsq_matchers = new std::vector<coot::lsq_range_match_info_t>;

      // LSQ Plane
      lsq_plane_atom_positions = new std::vector<clipper::Coord_orth>;

      directory_for_fileselection = "";
#if (GTK_MAJOR_VERSION > 1)
      directory_for_filechooser = "";
#endif // GTK_MAJOR_VERSION
      baton_next_ca_options = new std::vector<coot::scored_skel_coord>;
      baton_previous_ca_positions = new std::vector<clipper::Coord_orth>;

      // rotamer distortion graph scale
      rotamer_distortion_scale = 0.3;

      // cif dictionary
      cif_dictionary_filename_vec = new std::vector<std::string>;

/*       // ramachandran plots: */
/*       dynarama_is_displayed = new GtkWidget *[n_molecules_max]; */
/*       for (int i=0; i<n_molecules_max; i++) */
/* 	 dynarama_is_displayed[i] = NULL; // belt and braces */

/*       // sequence_view */
/*       sequence_view_is_displayed = new GtkWidget * [n_molecules_max]; */
/*       for (int i=0; i<n_molecules_max; i++) */
/* 	 sequence_view_is_displayed[i] = NULL; */


      // ---- Adding something here? Like another graph?  don't forget
      // to update expand_molecule_space()

      // residue edits
      residue_info_edits = new std::vector<coot::select_atom_info>;

      // display distances
      distance_object_vec = new std::vector<std::pair<clipper::Coord_orth, clipper::Coord_orth> >;

      // pointer distances
      pointer_distances_object_vec = new std::vector<std::pair<clipper::Coord_orth, clipper::Coord_orth> >;

      // ligand blobs:
      ligand_big_blobs = new std::vector<clipper::Coord_orth>;

      // rot_trans adjustments:
      for (int i=0; i<6; i++) 
	 previous_rot_trans_adjustment[i] = -10000;

      // merging molecules
      merge_molecules_merging_molecules = new std::vector<int>;

      // generic display objects
      generic_objects_p = new std::vector<coot::generic_display_object_t>;

      // generic text:
      generic_texts_p = new std::vector<coot::generic_text_object_t>;

      // views
      views = new std::vector<coot::view_info_t>;

      // glob extensions:
      coordinates_glob_extensions = new std::vector<std::string>;
      data_glob_extensions = new std::vector<std::string>;
      map_glob_extensions = new std::vector<std::string>;
      dictionary_glob_extensions  = new std::vector<std::string>;

      coordinates_glob_extensions->push_back(".pdb");
      coordinates_glob_extensions->push_back(".pdb.gz");
      coordinates_glob_extensions->push_back(".brk");
      coordinates_glob_extensions->push_back(".brk.gz");
      coordinates_glob_extensions->push_back(".ent");
      coordinates_glob_extensions->push_back(".ent.gz");
      coordinates_glob_extensions->push_back(".ent.Z");
      coordinates_glob_extensions->push_back(".cif");
      coordinates_glob_extensions->push_back(".mmcif");
      coordinates_glob_extensions->push_back(".mmCIF");
      coordinates_glob_extensions->push_back(".cif.gz");
      coordinates_glob_extensions->push_back(".mmcif.gz");
      coordinates_glob_extensions->push_back(".mmCIF.gz");
      coordinates_glob_extensions->push_back(".res");  // SHELX
      coordinates_glob_extensions->push_back(".ins");  // SHELX

      data_glob_extensions->push_back(".mtz");
      data_glob_extensions->push_back(".hkl");
      data_glob_extensions->push_back(".data");
      data_glob_extensions->push_back(".phs");
      data_glob_extensions->push_back(".cif");
      data_glob_extensions->push_back(".fcf"); // SHELXL
      data_glob_extensions->push_back(".mmcif");
      data_glob_extensions->push_back(".mmCIF");
      data_glob_extensions->push_back(".cif.gz");
      data_glob_extensions->push_back(".mmcif.gz");
      data_glob_extensions->push_back(".mmCIF.gz");

      map_glob_extensions->push_back(".map");
      map_glob_extensions->push_back(".ext");
      map_glob_extensions->push_back(".msk");
      map_glob_extensions->push_back(".ccp4");
      map_glob_extensions->push_back(".cns");

      dictionary_glob_extensions->push_back(".cif");
      dictionary_glob_extensions->push_back(".mmcif");
      dictionary_glob_extensions->push_back(".mmCIF");
      dictionary_glob_extensions->push_back(".cif.gz");
      dictionary_glob_extensions->push_back(".mmcif.gz");
      dictionary_glob_extensions->push_back(".mmCIF.gz");

      /* things for preferences */
      //preferences_internal = new std::vector<coot::preference_info_t>;
      preferences_general_tabs = new std::vector<std::string>;
      preferences_bond_tabs = new std::vector<std::string>;
      preferences_geometry_tabs = new std::vector<std::string>;
      preferences_colour_tabs = new std::vector<std::string>;
      preferences_map_tabs = new std::vector<std::string>;
      preferences_other_tabs = new std::vector<std::string>;
      
      preferences_general_tabs->push_back("preferences_file_selection");
      preferences_general_tabs->push_back("preferences_dock_accept_dialog");
      preferences_general_tabs->push_back("preferences_hid");
      preferences_general_tabs->push_back("preferences_recentre_pdb");
      preferences_general_tabs->push_back("preferences_model_toolbar_style");
      preferences_general_tabs->push_back("preferences_smooth_scroll");
      
      preferences_bond_tabs->push_back("preferences_bond_parameters");
      preferences_bond_tabs->push_back("preferences_bond_colours");
      
      preferences_map_tabs->push_back("preferences_map_parameters");
      preferences_map_tabs->push_back("preferences_map_colours");
      preferences_map_tabs->push_back("preferences_map_drag");      
      
      preferences_geometry_tabs->push_back("preferences_cis_peptides");

      preferences_colour_tabs->push_back("preferences_background_colour");
      preferences_colour_tabs->push_back("preferences_bond_colours");
      preferences_colour_tabs->push_back("preferences_map_colours");

      preferences_other_tabs->push_back("preferences_console");
      preferences_other_tabs->push_back("preferences_tips");
      preferences_other_tabs->push_back("preferences_speed");
      preferences_other_tabs->push_back("preferences_antialias");
      preferences_other_tabs->push_back("preferences_font");
      preferences_other_tabs->push_back("preferences_pink_pointer");
      // for toolbar icons in preferences
      model_toolbar_icons = new std::vector<coot::preferences_icon_info_t>;
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(0, "refine-1.svg",
								   "Real Space Refine",
								   "model_toolbar_refine_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(1, "regularize-1.svg",
								   "Regularize",
								   "model_toolbar_regularize_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(2, "anchor.svg",
								   "Fixed Atoms...",
								   "model_toolbar_fixed_atoms_button",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(3, "rigid-body.svg",
								   "Rigid Body Fit Zone",
								   "model_toolbar_rigid_body_fit_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(4, "rtz.svg",
								   "Rotate/Translate Zone",
								   "model_toolbar_rot_trans_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(5, "auto-fit-rotamer.svg",
								   "Auto Fit Rotamer",
								   "model_toolbar_auto_fit_rotamer_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(6, "rotamers.svg",
								   "Rotamers",
								   "model_toolbar_rotamers_togglebutton",
								   1,1 ));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(7, "edit-chi.svg",
								   "Edit Chi Angles",
								   "model_toolbar_edit_chi_angles_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(8, "torsion-general.svg",
								   "Torsion General",
								   "model_toolbar_torsion_general_toggletoolbutton",
								   1, 0));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(9, "flip-peptide.svg",
								   "Flip Peptide",
								   "model_toolbar_flip_peptide_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(10, "side-chain-180.svg",
								   "Side Chain 180 Degree Flip",
								   "model_toolbar_sidechain_180_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(11, "edit-backbone.svg",
								   "Edit Backbone Torsions",
								   "model_toolbar_edit_backbone_torsions_toggletoolbutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(12, "",
								   "---------------------",
								   "model_toolbar_hsep_toolitem",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(13, "mutate-auto-fit.svg",
								   "Mutate and Auto-Fit...",
								   "model_toolbar_mutate_and_autofit_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(14, "mutate.svg",
								   "Simple Mutate...",
								   "model_toolbar_simple_mutate_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(15, "add-peptide-1.svg",
								   "Add Terminal Residue...",
								   "model_toolbar_add_terminal_residue_togglebutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(16, "add-alt-conf.svg",
								   "Add Alt Conf...",
								   "model_toolbar_add_alt_conf_toolbutton",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(17, "atom-at-pointer.svg",
								   "Place Atom at Pointer",
								   "model_toolbar_add_atom_button",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(18, "gtk-clear",
								   "Clear Pending Picks",
								   "model_toolbar_clear_pending_picks_button",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(19, "gtk-delete",
								   "Delete...",
								   "model_toolbar_delete_button",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(20, "gtk-undo",
								   "Undo",
								   "model_toolbar_undo_button",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(21, "gtk-redo",
								   "Redo",
								   "model_toolbar_redo_button",
								   1, 1));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(22, "",
								   "---------------------",
								   "model_toolbar_hsep_toolitem2",
								   1, 0));
      model_toolbar_icons->push_back(coot::preferences_icon_info_t(23, "azerbaijan.svg",
								   "Run Refmac...",
								   "model_toolbar_refmac_button",
								   1, 0));

      do_expose_swap_buffers_flag = 1;

      refmac_dialog_mtz_file_label = NULL;
      /* set no of refmac cycles */
      preset_number_refmac_cycles = new std::vector<int>;
      preset_number_refmac_cycles->push_back(0);
      preset_number_refmac_cycles->push_back(1);
      preset_number_refmac_cycles->push_back(2);
      preset_number_refmac_cycles->push_back(3);
      preset_number_refmac_cycles->push_back(5);
      preset_number_refmac_cycles->push_back(7);
      preset_number_refmac_cycles->push_back(10);
      preset_number_refmac_cycles->push_back(15);
      preset_number_refmac_cycles->push_back(20);
      preset_number_refmac_cycles->push_back(50);
   }

   static bool do_expose_swap_buffers_flag;
   
   static void graphics_draw() {
     if (glarea) { 
       gtk_widget_draw(glarea, NULL);
       if (make_movie_flag)
	 dump_a_movie_image();
     }
     if (glarea_2)
       gtk_widget_draw(glarea_2, NULL);
   }


   static bool is_valid_model_molecule(int imol) {
     
     bool v = 0;
     if (imol >= 0) {
       if (imol < n_molecules()) {
	 if (molecules[imol].has_model()) {
	   v = 1;
	 }
       }
     }
     return v;
   }
   
   static bool is_valid_map_molecule(int imol) {
     
     bool v = 0;
     if (imol >= 0) {
       if (imol < n_molecules()) {
	 if (molecules[imol].has_map()) {
	   v = 1;
	 }
       }
     }
     return v;
   }

   static bool display_mode_use_secondary_p() {

     bool r = 0;
     if ((display_mode == coot::SIDE_BY_SIDE_STEREO) ||
	 (display_mode == coot::SIDE_BY_SIDE_STEREO_WALL_EYE) ||
	 (display_mode == coot::DTI_SIDE_BY_SIDE_STEREO)) {
       r = 1;
     }
     return r;
   }

   /* OpenGL functions can be called only if make_current returns true */
   static int make_current_gl_context(GtkWidget *widget) {
   
#if (GTK_MAJOR_VERSION == 1)
     return gtk_gl_area_make_current(GTK_GL_AREA(widget));
#else
     GdkGLContext *glcontext = gtk_widget_get_gl_context (widget);
     GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
     return gdk_gl_drawable_gl_begin (gldrawable, glcontext);
     
     // Something from Bernhard which I don't understand.
     // 
     // BL says:: dunno how to use gdk_gl_drawable_make_current here, currently
     //       glViewport(0,0, widget->allocation.width, widget->allocation.height);
     //       graphics_info_t g;
     //       g.graphics_x_size = widget->allocation.width;
     //     g.graphics_y_size = widget->allocation.height;
     //    graphics_info_t::graphics_draw();
#endif   
   }


   enum {GL_CONTEXT_MAIN = 0, GL_CONTEXT_SECONDARY = 1};

   static bool make_gl_context_current(short int gl_context_current_request) { 
     bool r = 0;
     if (display_mode_use_secondary_p()) { 
       if (gl_context_current_request == GL_CONTEXT_SECONDARY) {
	 if (glarea_2) {
	   make_current_gl_context(glarea_2);
	 } 
       } 
       if (gl_context_current_request == GL_CONTEXT_MAIN) { 
	 if (glarea) {
	   make_current_gl_context(glarea);
	 } 
       } 
     } else { 
       if (gl_context_current_request == GL_CONTEXT_MAIN) { 
	 if (glarea) {
	   make_current_gl_context(glarea);
	 } 
       } 
     } 
     return r;
   }



   // ------------- main window -----------------------
   static GtkWidget *glarea; // so that the molecule redraw function
                             // in c-interface.cc can find which window to redraw.
   static GtkWidget *glarea_2; 
   static GtkWidget *statusbar;
   static guint statusbar_context_id;
   static short int model_fit_refine_dialog_was_sucked;
   void statusbar_text(const std::string &text) const;

   static void statusbar_ctrl_key_info(); // Ctrl to rotate or pick?
   // -------------------------------------------------

   // accept/reject window, now controlled by keyboarding in main window.
   static GtkWidget *accept_reject_dialog;
   static GtkWidget *refine_params_dialog;

   // flag to display the accept/reject dialog in the toolbar
   static int accept_reject_dialog_docked_flag;

   // flag to show/hide/sensitise docked accept/reject dialog
   static int accept_reject_dialog_docked_show_flag;

   // flag for the refinement toolbar show/hide
   static short int model_toolbar_show_hide_state;

   // flag for the refinement toolbar position
   static short int model_toolbar_position_state;

   // flag for the refinement toolbar style
   static short int model_toolbar_style_state;

   static short int do_anti_aliasing_flag; // BL feature
   void set_do_anti_aliasing(int state);
   static int display_mode; // e.g. HARDWARE_STEREO_MODE, DTI_SIDE_BY_SIDE_STEREO
   static float hardware_stereo_angle_factor;
   static short int do_lighting_flag; 
   static short int in_wall_eyed_side_by_side_stereo_mode;

   // return the new molecule number
   static int create_molecule();

   static void erase_last_molecule() { 
     // std::vector<molecule_class_info_t>::iterator it = molecules.end();
     // std << "DEBUG:: Erasing molecule number " << it->MoleculeNumber() << std::endl;
/*      std::cout << "DEBUG:: Erasing the back molecule " << molecules.size() - 1  */
/* 	       << " which says that it has molecule number "  */
/* 	       << molecules[molecules.size() -1].MoleculeNumber() << std::endl; */
     molecules.pop_back();
   }

   static short int use_graphics_interface_flag; 

   // Display size
   static int graphics_x_size;
   static int graphics_y_size;
   static int graphics_x_position;
   static int graphics_y_position;

   static int model_fit_refine_x_position;
   static int model_fit_refine_y_position;

   static int display_manager_x_size;
   static int display_manager_y_size;
   static int display_manager_x_position;
   static int display_manager_y_position;

   static int display_manager_molecules_vbox_x_size;
   static int display_manager_molecules_vbox_y_size;
   static int display_manager_maps_vbox_x_size;
   static int display_manager_maps_vbox_y_size;

   static int display_manager_paned_position; 

   static int go_to_atom_window_x_position;
   static int go_to_atom_window_y_position;

   static int rotate_translate_x_position;
   static int rotate_translate_y_position;

   static int delete_item_widget_x_position;
   static int delete_item_widget_y_position;
   
   static int accept_reject_dialog_x_position;
   static int accept_reject_dialog_y_position;

   static int model_fit_refine_dialog_stays_on_top_flag;
   static std::string model_fit_refine_place_atom_at_pointer_string;
   static std::string model_fit_refine_rotate_translate_zone_string;

   static int edit_chi_angles_dialog_x_position;
   static int edit_chi_angles_dialog_y_position;

   static int rotamer_selection_dialog_x_position;
   static int rotamer_selection_dialog_y_position;

   static int ramachandran_plot_x_position;
   static int ramachandran_plot_y_position; 
   
   static int distances_and_angles_dialog_x_position; 
   static int distances_and_angles_dialog_y_position; 

   std::string get_directory_for_fileselection() const {
     return directory_for_fileselection;
   }
#if (GTK_MAJOR_VERSION > 1)
   std::string get_directory_for_filechooser() const {
     return directory_for_filechooser;
   }
#endif // GTK_MAJOR_VERSION
   
   static int map_line_width;

   static double mouse_current_x, mouse_current_y; 
   static float* quat;  // rotation quaternion, allocatated [4]
   static float* baton_quat;

   static int mouse_just_cliked; // delete this?
   static float zoom;
   static short int quanta_like_zoom_flag; 

   static float box_radius;

   static float iso_level_increment; 
   static float diff_map_iso_level_increment;
   static short int swap_difference_map_colours; // for Jan Dohnalek

   static float map_sampling_rate; // Shannon sampling rate multiplier (1.5 default)

   // 
   static int control_is_pressed;
   static int shift_is_pressed; 
   static int z_is_pressed;
   static int a_is_pressed;
   static short int control_key_for_rotate_flag;
   static short int pick_pending_flag;
   // 20051004 now use use this interface rather direct access to pick_pending_flag
   //          because a overal pick_pending_flag doesn't work when we consider turning
   //          off various different model/fit/refine toggle-buttons.  So
   //          static_graphics_pick_pending() checks each of the pending picks which
   //          would make the graphics not rotate (ctrl key issues).
   short int static_graphics_pick_pending() const; 

   static coot::residue_spec_t current_residue; // to be updated on set_go_to_atom and
		   			              // middle-mouse recentring.  check for
					              // unset_p() when used.
   static int current_residue_imol;

   // 
   static coot::colour_holder cell_colour;

   //
   // Frames per seconds control
   // (all we need to do is get and set). 
   // 

   // symm colour is a part of the molecule now
    static double  symmetry_colour_merge_weight;
    static std::vector<double> symmetry_colour;

   // Rotate colour map?
   static short int rotate_colour_map_on_read_pdb_flag;
   static float rotate_colour_map_on_read_pdb; // e.g. 5.0 (degrees)
   static short int rotate_colour_map_on_read_pdb_c_only_flag; 

   static float rotate_colour_map_for_map; // e.g. 31.0 (degrees)

   // 
   static float symmetry_search_radius;
   // static short int symmetry_as_calphas; // moved to per molecule basis
   // static short int symmetry_rotate_colour_map_flag; // do we want symmetry of other
						     // molecules to have a different
						     // colour [MOL]?
                                                     // moved to per molecule basis

   static int symmetry_shift_search_size; // the shift size for which_boxes. A hack.


   static float symmetry_operator_rotate_colour_map;
   // static int   symmetry_colour_by_symop_flag; // moved to per molecule basis
   // static int   symmetry_whole_chain_flag; moved to molecule_class_info_t
   static int   symmetry_atom_labels_expanded_flag; 

   // 
   static short int show_symmetry; 

   // Clipping Planes:
   static float clipping_front;
   static float clipping_back;  

   // This is for the display object 
   static short int display_lists_for_maps_flag;

   // expose this so that it can be seen in draw();
   static short int smooth_scroll_on;
   std::vector<int> displayed_map_imols() const;

   // expose so that they can be used in c-interface.cc
   static int smooth_scroll;
   static int smooth_scroll_steps; 
   static float smooth_scroll_limit; 
   static float smooth_scroll_zoom_limit; // above this value we zoom, if zoom is on.
   static int   smooth_scroll_do_zoom;

   // atom label font size
   static int atom_label_font_size; // range of 1->3. small, medium, large.
   static void *atom_label_font; 
   static int label_atom_on_recentre_flag; 
   static coot::colour_holder font_colour;
   void remove_all_atom_labels();

   static int n_molecules() { return molecules.size();}
   static std::vector<molecule_class_info_t> molecules; 

   // To which map is the mouse scroll wheel attached?
   // 
   static int scroll_wheel_map; 

   // a static utility function (excised for portability)
   static std::string add_dir_file(const std::string &dirname, const std::string &filename);

   // state_command (now public, it's called from c-interface-build (mutate sequence)
   // 
   std::string state_command(const std::vector<std::string> &str, short int state_lang) const;

   // esoteric depth cue on/off  (on by default)
   static int esoteric_depth_cue_flag;

   // external c interface get/set functions. 
   // 
   static void ShowFPS(); 
   static int  GetFPSFlag() {return show_fps_flag; }
   static void SetShowFPS(int t); 

   void SetActiveMapDrag(int t); 
   short int GetActiveMapDrag() { return active_map_drag_flag; }; 

   // return -1 on error
   // 
   int lookup_molecule_name(const std::string &molecule_name) const; 


   void SetMouseBegin(double x, double y);

   double  GetMouseBeginX() const ;
   double  GetMouseBeginY() const ;
   
   // We are given atom atom index, we will use this to look up 
   // the atom using molecule_class_info and find its coordinates.
   // Those coordinates will get used in draw() to centre on that
   // atom. 
   //
   void setRotationCentre(int atom_index, int imol); 

   void setRotationCentre(coot::Cartesian centre); 
   void setRotationCentreAndZoom(coot::Cartesian centre,
				 float target_zoom); 

   // old style: soon to be redundent
   void setRotationCentre(const symm_atom_info_t &symm_atom_info);
   void setRotationCentre(const coot::clip_hybrid_atom &hybrid_atom);

   void update_things_on_move();
   void update_things_on_move_and_redraw();

   float X(void) { return rotation_centre_x; };
   float Y(void) { return rotation_centre_y; };
   float Z(void) { return rotation_centre_z; };

   coot::Cartesian RotationCentre() const 
      { return coot::Cartesian(rotation_centre_x,
			       rotation_centre_y,
			       rotation_centre_z);}

   // we need static, so that we don't need to instance a
   // graphics_info_t for every frame draw.
   static float RotationCentre_x() { return rotation_centre_x; }
   static float RotationCentre_y() { return rotation_centre_y; }
   static float RotationCentre_z() { return rotation_centre_z; }
   

   // pointer: aka rotation centre:
   // 
   void display_where_is_pointer() const {
      std::cout << "Pointer at" << RotationCentre() << std::endl;
   } 

   // x_diff and y_diff are the scale factors to the x and y
   // drag vectors.
   // 
   void add_to_RotationCentre(coot::CartesianPair x_y, 
			      gdouble x_diff, gdouble y_diff) {

      // x_drag and y_drag are the model space vector due
      // to screen x and y differences respectively.
      // 
      coot::Cartesian x_drag = x_y.getStart(); 
      coot::Cartesian y_drag = x_y.getFinish(); 

      rotation_centre_x += x_drag.get_x()*x_diff + y_drag.get_x()*y_diff;
      rotation_centre_y += x_drag.get_y()*x_diff + y_drag.get_y()*y_diff;
      rotation_centre_z += x_drag.get_z()*x_diff + y_drag.get_z()*y_diff;

   };
   void add_vector_to_RotationCentre(const coot::Cartesian &vec); // do the updates and redraw too.


   // map colour by scripting
   void set_last_map_colour(double f1, double f2, double f3) const;

   // sigma contour stepping?
   // Calling this turns it on.
   void set_last_map_contour_level(float f);
   void set_last_map_contour_level_by_sigma(float f);

   // 
   static float rotation_centre_cube_size; 

   static void Increment_Frames() { 
      Frames++; 
   }

   // 
   void set_font_size(int size);


   // 
   void update_map_colour_menu(); 

   static short int do_scroll_by_wheel_mouse_flag; 
   //
   void set_Scrollable_Map(int imol, int imap) {
      scroll_wheel_map = imol;
   }

   // Transfered from molecule_class_info, because they are a parameter
   // of the *graphis* at the moment, not each molecule (there is no
   // molecule control in Anisotropic Atoms at the momemnt). 
   // 
   static short int show_aniso_atoms_flag;
   static float     show_aniso_atoms_radius;
   static short int show_aniso_atoms_radius_flag; // shall the atoms be
                                           // limited to a certain distance?
   static float     show_aniso_atoms_probability;


   // 
   void set_vt_surface(int v); // virtual trackball
   float get_trackball_size() const {return trackball_size; }; 
   int vt_surface_status() const;

   // skeleton colour
   static double* skeleton_colour; 


   // idle function token (holder)
   //
   static int i_fn_token;
   // drag refine idle function token:
   static int drag_refine_idle_function_token;
   static float idle_function_rotate_angle; // degrees
   static gint drag_refine_idle_function(GtkWidget *widget);
   static void add_drag_refine_idle_function();
   static gint drag_refine_refine_intermediate_atoms();
   static coot::refinement_results_t saved_dragged_refinement_results;

   // now that the refinement goes via the idle function callback
   // (which gets called several times) we need to external control of
   // when to print the chi_squareds... and we turn them on when we
   // set the idle function.  Then once the function has run, we turn
   // them off.  Internally (in the restraints minimize()), on
   // GSL_SUCCESS and GSL_ENOPROG we print the chi squareds.
   static short int print_initial_chi_squareds_flag;

   // phs filename (because we actually do things when we OK on the
   // *coordinates* filename).  So give store and retrieve functions.
   // 
   static std::string phs_filename; // make me private.
   std::string get_phs_filename() const; 
   void set_phs_filename( std::string filename); 
   // static int phs_cell_from_molecule; 

   // get rid of the actual molecule (as opposed to
   // clear_moving_atoms_object which removed the bonds).
   void clear_up_moving_atoms();

   // 0: never run it
   // 1: ask to run it
   // 2: alwasy run it
   static short int run_state_file_status; 

   // Go To Atom 
   // 

   const char *go_to_atom_chain(); 
   const char *go_to_atom_atom_name(); 
   const char *go_to_atom_ins_code();
   const char *go_to_atom_alt_conf();
   int go_to_atom_residue(); 
   int go_to_atom_molecule();
   static int go_to_atom_mol_menu_active_position;
   static int go_to_atom_menu_label_n_chars_max; // the last 30 (or so) chars
   std::string make_mmdb_atom_string_from_go_to_atom(); 

   void set_go_to_atom_chain_residue_atom_name(const gchar *t1, 
					       int it2, const gchar *t3);
   void set_go_to_atom_chain_residue_atom_name(const char *chain_id, 
					       int resno, const char *atom_name, const char *altLoc);
   void set_go_to_atom_chain_residue_atom_name(const char *chain_id, 
					       int resno, const char *ins_code,
					       const char *atom_name, const char *altLoc);
   void set_go_to_residue_intelligent(const std::string &chain_id, int resno, 
				      const std::string &ins_code);
   static std::pair<std::string, std::string> split_atom_name(const std::string &atom_name);
   static std::pair<std::string, std::string> split_resno_inscode(const std::string &atom_name);


   void set_go_to_atom_molecule(int pos); 

   int try_centre_from_new_go_to_atom();

   void update_widget_go_to_atom_values(GtkWidget *window, CAtom *atom);
   void make_synthetic_select_on_residue_list(GtkWidget *residue_list, CAtom *atom_p) const;

   void make_synthetic_select_on_residue_tree(GtkWidget *residue_list, CAtom *atom_p) const;
   void make_synthetic_select_on_residue_tree_gtk1(GtkWidget *residue_list, CAtom *atom_p) const;

   void update_go_to_atom_window_on_changed_mol(int imol); 
   void update_go_to_atom_window_on_new_mol(); 
   void update_go_to_atom_window_on_other_molecule_chosen(int imol);
   int go_to_atom_molecule_optionmenu_active_molecule(GtkWidget *widget);
#if (GTK_MAJOR_VERSION == 1) || defined (GTK_ENABLE_BROKEN)
   static void fill_go_to_atom_atom_list_gtk1(GtkWidget *atom_gtklist, int imol,
					      char *chain_id, int seqno);
#else
   static void fill_go_to_atom_window_gtk2(GtkWidget *go_to_atom_window,
					   GtkWidget *residue_tree_scrolled_window,
					   GtkWidget *atom_list_scrolled_window);
   static void fill_go_to_atom_atom_list_gtk2(GtkWidget *atom_tree, int imol,
					      char *chain_id, int seqno, char *ins_code);
#endif
   static void clear_atom_list(GtkWidget *atom_gtklist); 
   static void fill_go_to_atom_residue_list_gtk1(GtkWidget *gtklist);
   static void fill_go_to_atom_residue_tree_gtk2(GtkWidget *gtktree);
   void fill_go_to_atom_option_menu(GtkWidget *option_menu);
   void fill_option_menu_with_coordinates_options(GtkWidget *option_menu,
						  GtkSignalFunc callback_func);
   void fill_option_menu_with_coordinates_options(GtkWidget *option_menu, 
						  GtkSignalFunc signal_func,
						  int imol_active_position);
   void fill_option_menu_with_coordinates_options_internal(GtkWidget *option_menu,
							   GtkSignalFunc callback_func,
							   short int set_last_active_flag);
   void fill_option_menu_with_coordinates_options_internal_2(GtkWidget *option_menu,
							     GtkSignalFunc callback_func, 
							     short int set_last_active_flag,
							     int imol_active);
   void fill_option_menu_with_coordinates_options_internal_with_active_mol(GtkWidget *option_menu,
									   GtkSignalFunc callback_func, 
									   int imol_active);
   static void go_to_atom_mol_menu_item_select(GtkWidget *item, GtkPositionType pos); 
   static void on_go_to_atom_residue_list_selection_changed (GtkList *gtklist,
							     gpointer user_data);

   static void on_go_to_atom_residue_tree_selection_changed_gtk1(GtkList *gtklist,
								 gpointer user_data);
#if (GTK_MAJOR_VERSION == 1) || defined (GTK_ENABLE_BROKEN)

   static int go_to_atom_residue_tree_signal_handler_event_gtk1(GtkWidget *widget, 
								GdkEventButton *event, 
								gpointer func_data);
   static int cb_chain_tree_itemsignal( GtkWidget *item,
					GdkEventButton *event, 
					gpointer func_data);
   static int go_to_atom_atom_list_signal_handler_event_gtk1(GtkWidget *widget, 
							     GdkEventButton *event, 
							     gpointer func_data);

#else 
   // -------------------- Gtk2 code -----------------------------
   static void on_go_to_atom_residue_tree_selection_changed (GtkTreeView *gtklist,
							     gpointer user_data);
   static void
     residue_tree_residue_row_activated(GtkTreeView        *treeview,
					GtkTreePath        *path,
					GtkTreeViewColumn  *col,
					gpointer            userdata);
   static void
     atom_tree_atom_row_activated(GtkTreeView        *treeview,
				  GtkTreePath        *path,
				  GtkTreeViewColumn  *col,
				  gpointer            userdata);
   static gboolean
     residue_tree_selection_func(GtkTreeSelection *selection,
				 GtkTreeModel *model,
				 GtkTreePath *path,
				 gboolean path_currently_selected,
				 gpointer data);

   static gboolean
     atom_tree_selection_func(GtkTreeSelection *selection,
			      GtkTreeModel *model,
			      GtkTreePath *path,
			      gboolean path_currently_selected,
			      gpointer data);

// BL says:: put my gtk2 stuff in here too:
   static int gtk2_file_chooser_selector_flag;
   static int gtk2_chooser_overwrite_flag;
#endif // #if (GTK_MAJOR_VERSION == 1) || defined (GTK_ENABLE_BROKEN)

   void apply_go_to_atom_from_widget(GtkWidget *widget); 
   static void pointer_atom_molecule_menu_item_activate(GtkWidget *item, 
							GtkPositionType pos);


   // return success status
   int intelligent_next_atom_centring(GtkWidget *widget);
   int intelligent_previous_atom_centring(GtkWidget *widget);
   int intelligent_near_atom_centring(GtkWidget *widget, const std::string &direction);

   pick_info find_atom_index_from_goto_info(int imol);
   // int find_atom_index_in_moving_atoms(char *chain_id, int resno, char *atom_name) const;
   CAtom *find_atom_in_moving_atoms(const coot::atom_spec_t &at) const;

   coot::Symm_Atom_Pick_Info_t symmetry_atom_pick() const;

   // map skeletonization level (and (different widget) boxsize). 
   //
   static float skeleton_level; 
   static float skeleton_box_radius; 

   // autobuild
   static short int autobuild_flag; 

   // file selection should be sorted by date?
   static short int sticky_sort_by_date;
   
   // file filter should be on?
   static short int sticky_file_filter;
   
   // 
   void show_refine_params_dialog(); // not used for map selection now.
   void show_select_map_dialog();

   // Map and molecule display.  We need this so that we can look up
   // the names of the boxes so that we can add extra entries to them
   // when we create new maps and molecules
   // 
   void save_display_control_widget_in_graphics(GtkWidget *widget) { 
      display_control_window_ = widget;
      if (widget == NULL) {
	 // then the widget was destroyed and so we need to unset the
	 // group of the scroll radio buttons.
	 gslist_for_scroll_in_display_manager = NULL;
      }
   }

   static GSList *gslist_for_scroll_in_display_manager;
   
   GtkWidget *display_control_window() { 
     return display_control_window_; 
   }

   static void activate_scroll_radio_button_in_display_manager(int imol);
   static float find_waters_sigma_cut_off;


   // for defining a range (to, say, regularize), we click on atoms
   // and manipulate atom indexes
   // 
   // in_range_define has special values 0, 1, 2.  0 means not in a
   // range definition, 1 mean that we have just clicked the menu
   // item, 2 means that we are waiting for the last atom pick
   // (e.g. we have clicked on the first atom and are waiting for the
   // user to select the second).
   // 
   static short int in_range_define; // initially 0
   static short int in_range_define_for_refine; // initially 0
   static int refine_regularize_max_residues; 
   static int residue_range_mol_no; 
   static int residue_range_atom_index_1;
   static int residue_range_atom_index_2;
   static int geometry_atom_index_1;
   static int geometry_atom_index_2;
   static int geometry_atom_index_3;
   static int geometry_atom_index_4;
   static int geometry_atom_index_1_mol_no;
   static int geometry_atom_index_2_mol_no;
   static int geometry_atom_index_3_mol_no;
   static int geometry_atom_index_4_mol_no;
   static short int fix_chiral_volume_before_refinement_flag;
   static short int show_chiral_volume_errors_dialog_flag;

   // torsion general
   static int torsion_general_atom_index_1;
   static int torsion_general_atom_index_2;
   static int torsion_general_atom_index_3;
   static int torsion_general_atom_index_4;
   static int torsion_general_atom_index_1_mol_no;
   static int torsion_general_atom_index_2_mol_no;
   static int torsion_general_atom_index_3_mol_no;
   static int torsion_general_atom_index_4_mol_no;
   static std::vector<coot::atom_spec_t> torsion_general_atom_specs;
   static bool torsion_general_reverse_flag;
   static Tree torsion_general_tree;

   // 
   static int imol_pepflip;
   static int iresno_pepflip;
   static int atom_index_pepflip;
   static short int in_pepflip_define;
   //
   static int imol_rigid_body_refine;
   static short int in_rigid_body_define; 
   // uses imol_refinement_map;
   static short int in_terminal_residue_define;
   static std::string add_terminal_residue_type;

   // CIS <-> TRANS conversion
   static int in_cis_trans_convert_define;

   // rotate/translate object mode
   static short int in_rot_trans_object_define;
   static int rot_trans_atom_index_1;
   static int rot_trans_atom_index_2;
   static int imol_rot_trans_object;
   static short int rot_trans_zone_rotates_about_zone_centre;

   // additional representation
   static int add_reps_molecule_option_menu_item_select_molecule; // option menu 

   static coot::fixed_atom_pick_state_t in_fixed_atom_define;
   static GtkWidget *fixed_atom_dialog;

   static short int in_torsion_general_define;
   // static int rot_trans_atom_index_rotation_origin_atom; old naive way.
   static CAtom *rot_trans_rotation_origin_atom; // "Eugene's way"

   // save symmetry?
   static short int in_save_symmetry_define;

   // Where should we open up the save coords fileselection?
   static int save_coordinates_in_original_dir_flag;

   // Was private, but need to be used by auto_fit_best_rotamer() scripting function.
   void update_geometry_graphs(PCResidue *SelResidues, int nSelResidues, int imol_coords, int imol_map);
   void delete_residue_from_geometry_graphs(int imol, coot::residue_spec_t res_spec); 


   void execute_rotate_translate_ready(); // manual movement
   void setup_rotate_translate_buttons(GtkWidget *window);
   void unsetup_rotate_translate_buttons(GtkWidget *window); /* delete the user data */
   void do_rot_trans_adjustments(GtkWidget *dialog);
   static void rot_trans_adjustment_changed(GtkAdjustment *adj, gpointer user_data);
   static float *previous_rot_trans_adjustment;

   // rottrans_buttons class calls back this function on button pressed mouse motion
   // 
   // void rot_trans_obj(int xdiff, const std::string &button_label); old
   
   void set_in_range_define_for_regularize(short int state) { in_range_define = state; } // true
   void set_in_range_define_for_refine(short int state) { in_range_define_for_refine = state; } // true
   void set_in_pepflip_define(short int state) { in_pepflip_define = state; }
   void set_in_rigid_body_refine(short int state) { in_rigid_body_define = state; }
   static float rigid_body_fit_acceptable_fit_fraction; 
   static int refine_auto_range_step;  // +/- (1) about the clicked residue

   // called by scripting interface to rigid_body_refine_zone.  This
   // is not a great ame for the function (or for the residues that it
   // sets) since they do obviously refer to rigid body refinement.
   // 
   void set_residue_range_refine_atoms(const std::string &chain_id,
					int resno_start, int resno_end,
				       const std::string &alt_conf,
				       int imol);
   void execute_rigid_body_refine(short int auto_range_flag);
   // called by above:
   void rigid_body_fit(const coot::minimol::molecule &mol_without_moving_zone,
		       const coot::minimol::molecule &range_mol,
		       int imol_ref_map,
		       bool mask_water_flag);

   static short int in_residue_info_define; // initially 0
   static float geometry_vs_map_weight; 
   static float rama_plot_restraint_weight;

   // similarly for distance and angles:
   //
   static short int in_distance_define;
   static short int in_angle_define;
   static short int in_torsion_define;

   void set_in_distance_define() { in_distance_define = 1; } 
   void set_in_angle_define()    { in_angle_define = 1; } 

   static short int in_db_main_define; 
   static int db_main_imol;
   static int db_main_atom_index_1;
   static int db_main_atom_index_2;
   int load_db_main();

   static short int in_reverse_direction_define;

   // wrappers for regularization and refinement:
   //
   static short int do_torsion_restraints; // all, including side chains
   static short int do_peptide_omega_torsion_restraints;
   static bool do_rama_restraints; 
   static bool do_numerical_gradients; // for debugging

   void regularize(int imol, short int auto_range_flag, int i_atom_start, int i_atom_end); 
   void refine    (int imol, short int auto_range_flag, int i_atom_start, int i_atom_end);
   // a more modern interface to refine:
   void refine_residue_range(int imol,
			     const std::string &chain_id1,
			     const std::string &chain_id2,
			     int resno_1,
			     const std::string &ins_code_1,
			     int resno_2,
			     const std::string &ins_code_2,
			     const std::string &altconf,
			     short int is_water_flag);
   
   // used by above:
   void flash_selection(int imol, int resno_1, 
			std::string ins_code_1,
			int resno_2, 
			std::string ins_code_2,
			std::string altconf, // use this altconf or "" atoms.
			std::string chain_id_1); 
   static void flash_position(const clipper::Coord_orth &pos);

   std::pair<int, int> auto_range_residues(int atom_index, int imol) const; 


   // Idealize the geometry without considering the map.
   // 
   // return 1 if restraints were found, 0 if not.
   // 
   coot::refinement_results_t
     copy_mol_and_regularize(int imol,
			     int resno_1, 
			     std::string inscode_1,
			     int resno_2, 
			     std::string inscode_2,
			     std::string altconf, // use this altconf or "" atoms.
			     std::string chain_id_1); 

   // Regularize *and* fit to density.
   //
   // return 1 if restraints were found, 0 if not.
   // 
   coot::refinement_results_t
   copy_mol_and_refine(int imol_for_atoms,
		       int imol_for_map,
		       int resno_1, 
		       std::string inscode_1,
		       int resno_2, 
		       std::string inscode_2,
		       std::string altconf, // use this altconf or "" atoms.
		       std::string chain_id_1);

   // return 0 if any of the residues in selection don't have (at least) bond 
   // restraints.  Try to auto-load the dictionary cifs and try again.
   // The vector is a list of residues for which no restraints could be found.
   std::pair<int, std::vector<std::string> >
   check_dictionary_for_residues(PCResidue *SelResidues, int nSelResidues);

   // called by copy_mol_and_refine and copy_mol_and_regularize
   // 
   CMMDBManager *create_mmdbmanager_from_res_selection(PCResidue *SelResidues, 
						       int nSelResidues, 
						       int have_flanking_residue_at_start,
						       int have_flanking_residue_at_end, 
						       const std::string &altconf,
						       const std::string &chain_id_1,
						       short int residue_from_alt_conf_split_flag,
						       int imol); // imol is for uddatom index

   // on reading a pdb file, we get a list of residues, use these to
   // load monomers from the dictionary
   int load_needed_monomers(const std::vector<std::string> &pdb_residue_types);


   // geometry graphs
   void update_geometry_graphs(const atom_selection_container_t &asc, int imol_moving_atoms);


   // Display the graphical object of the regularization
   static void moving_atoms_graphics_object(); // filled by flash_selection

   static void environment_graphics_object();
   // void symmetry_environment_graphics_object() const;

   // for flashing the picked intermediate atom.
   static void picked_intermediate_atom_graphics_object();
   
   void update_environment_distances_maybe(int index, int imol);
   void update_environment_distances_by_rotation_centre_maybe(int imol); 

   // cif dictionary read number.  Update on reading (or attempting to
   // read) a cif dictionary file.  Public because reading cif file in
   // molecule-class-info-other.cc (bad chiral volumes) needs it.
   static int cif_dictionary_read_number;

   static int residue_selection_flash_frames_number;

   // scripting
   static short int guile_gui_loaded_flag;
   static short int python_gui_loaded_flag;
   static std::vector<std::string> *command_line_scripts;
   static coot::command_line_commands_t command_line_commands;
   static std::vector<std::string> command_line_accession_codes;

   // background colour
   static float *background_colour;

   // dynarama: a list of dynarama canvases, each of which has
   // attached a pointer to a rama_plot class object.
   // 
   static GtkWidget **dynarama_is_displayed;
   void set_dynarama_is_displayed(GtkWidget *dyna_canvas, int imol); 
   void destroy_edit_backbone_rama_plot(); // only one of these.

   // sequence view
   static GtkWidget **sequence_view_is_displayed;
   void set_sequence_view_is_displayed(GtkWidget *seq_view_canvas, int imol);

   // Geometry Graphs:

/* Old style array of molecules code */
/*    static GtkWidget **geometry_graph; */
/*    static GtkWidget **b_factor_variance_graph; */
/*    static GtkWidget **residue_density_fit_graph; */
/*    static GtkWidget **omega_distortion_graph; */
/*    static GtkWidget **rotamer_graph; */
/*    static GtkWidget **ncs_diffs_graph; */

#ifdef HAVE_GSL
#if defined(HAVE_GNOME_CANVAS) || defined(HAVE_GTK_CANVAS)
   std::vector<coot::geometry_graph_block_info_generic>
   density_fit_from_mol(const atom_selection_container_t &asc, int imol_moving_atoms,
			int imol_for_map);
   std::vector<coot::geometry_graph_block_info_generic>
   density_fit_from_residues(PCResidue *SelResidues, int nSelResidues,
			     int imol_moving_atoms,
			     int imol_for_map) const;

   coot::omega_distortion_info_container_t 
     omega_distortions_from_mol(const atom_selection_container_t &asc, const std::string &chain_id);

   std::vector<coot::geometry_graph_block_info_generic>
     rotamers_from_mol(const atom_selection_container_t &asc, int imol_moving_atoms);
   std::vector<coot::geometry_graph_block_info_generic>
     rotamers_from_residue_selection(PCResidue *SelResidues,
				   int nSelResidues, int imol); 

   std::vector<coot::geometry_graph_block_info_generic> ncs_diffs_from_mol(int imol);
   std::vector<coot::geometry_graph_block_info_generic> ncs_diffs(int imol, 
								  const coot::ncs_chain_difference_t &d);
#endif   
#endif // defined(HAVE_GNOME_CANVAS) || defined(HAVE_GTK_CANVAS)
   static float residue_density_fit_scale_factor; // 1.0 usually, adjustable for CNS/mapman map users.

   // shall we recentre when a new molecule is added (default no)
   static short int recentre_on_read_pdb; 

   // map radius slider maximum
   static float map_radius_slider_max; 

   // mouse buttons
   GdkModifierType gdk_button1_mask() { return button_1_mask_;}
   GdkModifierType gdk_button2_mask() { return button_2_mask_;}
   GdkModifierType gdk_button3_mask() { return button_3_mask_;}

   void quanta_buttons() { 
      button_1_mask_ = GDK_BUTTON2_MASK;
      button_2_mask_ = GDK_BUTTON1_MASK;
   }

   static int draw_axes_flag; 

   // 
   static short int display_density_level_on_screen;
   static short int display_density_level_this_image; 
   static std::string display_density_level_screen_string; 
   void set_density_level_string(int imol, float dlevel);

   static coot::Cartesian to_cartesian(const clipper::Coord_orth &co) {
      return coot::Cartesian(co.x(), co.y(), co.z());
   }
   static clipper::Coord_orth to_coord_orth(const coot::Cartesian &c) {
      return clipper::Coord_orth(c.x(), c.y(), c.z());
   }

   static std::string int_to_string(int i); 
   static std::string float_to_string(float f);
   static std::string float_to_string_using_dec_pl(float f, unsigned short n_dec_pl);
   static std::string backslash_filename(const std::string &s); // needed for windows?
   
   void set_find_ligands_mols(int map, int protein,
			      const std::vector<std::pair<int, bool> > &ligand_wiggly_info) {
      find_ligand_map_mol_ = map;
      find_ligand_protein_mol_ = protein; 
      // *find_ligand_ligand_mols_ = ligand_wiggly_info; // lets add some protection..
      find_ligand_ligand_mols_->clear();
      for (unsigned int ilig=0; ilig<ligand_wiggly_info.size(); ilig++) {
	int il=ligand_wiggly_info[ilig].first;
	if (il < n_molecules()) {
	  if (molecules[il].atom_sel.n_selected_atoms > 0) { 
	    find_ligand_ligand_mols_->push_back(ligand_wiggly_info[ilig]);
	  }
	}
      }
   }
   int find_ligand_map_mol() const { 
      return find_ligand_map_mol_;
   }
   int find_ligand_protein_mol() const { 
      return find_ligand_protein_mol_;
   }
   static void set_ligand_protein_mol(int imol) { 
     if (imol >=0) 
       if (imol < n_molecules()) 
	 find_ligand_protein_mol_ = imol;
   }
   static void set_ligand_map_mol(int imol) { 
     if (imol >=0) 
       if (imol < n_molecules())
       find_ligand_map_mol_ = imol;
   }
   static void find_ligand_add_rigid_ligand(int imol) { 
     if (imol >=0) {
       if (imol < n_molecules()) { 
	 if (molecules[imol].has_model()) { 
	   find_ligand_ligand_mols_->push_back(std::pair<int, bool>(imol, 0));
	 }
       }
     }
   }
   static void find_ligand_add_flexible_ligand(int imol) { 
     if (imol >=0) {
       if (imol < n_molecules()) { 
	 if (molecules[imol].has_model()) { 
	   find_ligand_ligand_mols_->push_back(std::pair<int, bool>(imol, 1));
	 }
       }
     }
   }
   std::vector<std::pair<int, bool> > find_ligand_ligand_mols() const { 
     return *find_ligand_ligand_mols_;
   }
   void find_ligand_clear_ligand_mols() {
     find_ligand_ligand_mols_->clear();
   }
   static std::vector<clipper::Coord_orth> *ligand_big_blobs;
   static int find_ligand_n_top_ligands;
   static short int find_ligand_mask_waters_flag;
   static int find_ligand_ligand_atom_limit;
   static short int ligand_expert_flag;
   static float map_mask_atom_radius;

   // Geometry issues:
   
   // debugging:
   const coot::protein_geometry *Geom_p() const { return geom_p; }
   coot::protein_geometry *Geom_p() { return geom_p; }
   //
   std::vector <coot::dict_torsion_restraint_t> get_monomer_torsions_from_geometry(const std::string &monomer_type) const;
   
   // make regularize_object_bonds_box be the empty box (and redisplay)
   // 
   void clear_moving_atoms_object(); 
   // copy the contents of moving_atoms_asc into the molecule being refined.
   // 
   void accept_moving_atoms();
   void set_refinement_map(int imol);

   // public access to the clear the in range defines
   void clear_pending_picks(); 

    
   // For updating the goto atom widget, let's store the window
   // pointer here, so that on recentering (e.g. on "space"), we check
   // to see if this variable is non-null, if it is, we use it to
   // update the vaules in the entries.
   //
   // Also, this is needed for deleting molecules (menuitems) from the
   // menu when molecules are closed.
   //
   // FIXME Needs to be implemented.  close window button callback and
   // destroy window need to set this to NULL.
   // 
   static GtkWidget *go_to_atom_window;



   // For updating the model/fit/refine widget (turning on/off
   // checkbuttons), let's store the Model/Fit/Refine window and
   // create only one of them.
   //
   static GtkWidget *model_fit_refine_dialog;
   static GtkWidget *other_modelling_tools_dialog;

   // And similarly for the rotamers, we now want to be able to change
   // them via keyboard arrow keys, so we need to register the rotamer
   // dialog here, the number of rotamers will be attached to the
   // dialog and the dialog will get sent :next rotamer:, :previous
   // rotamer: signals.

   static GtkWidget *rotamer_dialog;

   // And also for the difference map peaks dialog, which people want
   // to scroll through using . and ,

   static GtkWidget *difference_map_peaks_dialog;
   
   // 
   void model_fit_refine_unactive_togglebutton(const std::string &button_name) const;
   void other_modelling_tools_unactive_togglebutton(const std::string &button_name) const;


   // And also for the residue info button, we don't want to mess
   // around with having lots of these, life is too short to sort out
   // that mess.
   static GtkWidget *residue_info_dialog;
   

   // geometry:
   // (text message in the console, currently)
   void display_geometry_distance() ; // not const because we add to distance_object_vec
   void display_geometry_angle() const;
   void display_geometry_torsion() const;
   void display_geometry_distance_symm(const coot::Cartesian &p1, const coot::Cartesian &p2);
   //
   void pepflip();

   // we need this in c-interface.cc for the rigid body refinement
   // which refines against a map
   //
   // return -1 on no map found.  
   //
   // If there is exactly one map in all of the molecules, return the
   // molecule index of that map.
   // 
   int Imol_Refinement_Map() const;
   int set_imol_refinement_map(int imol);

   void make_moving_atoms_graphics_object(const atom_selection_container_t &asc);
   static short int moving_atoms_asc_type; 

   //
   static float environment_min_distance;
   static float environment_max_distance;
   static short int environment_show_distances;
   static short int environment_distance_label_atom;

   // private?
   void update_environment_graphics_object(int atom_index, int imol);
   void update_symmetry_environment_graphics_object(int atom_index, int imol); 

   //
   static short int dynamic_map_resampling;
   static short int dynamic_map_size_display;
   static int graphics_sample_step;
   static int dynamic_map_zoom_offset; 

   // uses cif_dictionary_filename_vec
   //
   void add_cif_dictionary(std::string cif_dictionary_filename,
			   short int show_no_bonds_dialog_maybe_flag);
   void import_all_refmac_cifs();
   static std::vector<std::string> *cif_dictionary_filename_vec;

   // make private?
   static short int python_history;
   static short int guile_history;
   void add_history_command(const std::vector<std::string> &command_strings);
   static coot::history_list_t history_list;
   static std::string pythonize_command_strings(const std::vector<std::string> &command_strings);
   static std::string schemize_command_strings(const std::vector<std::string> &command_strings);

   static coot::console_display_commands_t console_display_commands;

   // build one residue by phi psi
   static int add_terminal_residue_n_phi_psi_trials;
   static int add_terminal_residue_add_other_residue_flag; 
   static short int add_terminal_residue_do_post_refine; 
   static float terminal_residue_addition_direct_phi; 
   static float terminal_residue_addition_direct_psi; 

   void execute_add_terminal_residue(int imol, 
				     const std::string &terminus,
				     const CResidue *res_p,
				     const std::string &chain_id, 
				     const std::string &res_type,
				     short int immediate_addition_flag);
   void execute_simple_nucleotide_addition(int imol, const std::string &term_type, 
					   CResidue *res_p, const std::string &chain_id);

   static short int add_terminal_residue_immediate_addition_flag;
   static short int refinement_immediate_replacement_flag;  // don't dialog me please
   // called by above (private)
   atom_selection_container_t add_side_chain_to_terminal_res(atom_selection_container_t asc, 
							     const std::string res_type); 

   // we allow terminal fitting without rigid body refinement
   static short int terminal_residue_do_rigid_body_refine; 

   // Rotate position round direction, direction is not necessarily a
   // unit vector
   // 
   clipper::Coord_orth rotate_round_vector(const clipper::Coord_orth &direction,
					   const clipper::Coord_orth &position,
					   const clipper::Coord_orth &origin_shift,
					   double angle) const; 

   // public (from globjects);
   // 
   void execute_db_main();
   // direction string (flag): "forwards" or "backwards" (actually, if
   // not "backwards", "forwards" is presumed).
   void execute_db_main(int imol, std::string chain_id,
			int iresno_start, int iresno_end, std::string direction_string); 

   static float ligand_acceptable_fit_fraction;
   static float ligand_cluster_sigma_level; // was 2.2 default
   static int   ligand_wiggly_ligand_n_samples;
   static int   ligand_verbose_reporting_flag; 
   
   // Eleanor wants control over water parameters
   // 
   static float ligand_water_to_protein_distance_lim_max; 
   static float ligand_water_to_protein_distance_lim_min; 
   static float ligand_water_variance_limit; 
   static int   ligand_water_n_cycles;
   static short int ligand_water_write_peaksearched_atoms; 

   // Delete item mode:
   static short int delete_item_atom; 
   static short int delete_item_residue;
   static short int delete_item_residue_zone;
   static short int delete_item_residue_hydrogens;
   static short int delete_item_water;
   static short int delete_item_sidechain;
   // must save the widget so that it can be deleted when the item is selected.
   static GtkWidget *delete_item_widget;
   static int keep_delete_item_active_flag;
   // really, we should save pick data with atom or residue specs, so
   // let's start with delete zones' first click:
   static coot::residue_spec_t delete_item_residue_zone_1;
   void delete_residue_range(int imol, const coot::residue_spec_t &res1,
			     const coot::residue_spec_t &res2);

   // c-info functions really, but we cant have mmdb_manager there, so the are moved here.
   // 
   static void fill_output_residue_info_widget(GtkWidget *widget, int imol,
					       PPCAtom atoms, int n_atoms);
   static void fill_output_residue_info_widget_atom(GtkWidget *widget,
						    int imol, PCAtom atom, int iat);
   // and the keypres callbacks for the above
   static gboolean on_residue_info_occ_entry_key_release_event (GtkWidget       *widget,
								GdkEventKey     *event,
								gpointer         user_data);
   static gboolean on_residue_info_master_atom_occ_changed (GtkWidget       *widget,
							    GdkEventKey     *event,
							    gpointer         user_data);

   static gboolean on_residue_info_master_atom_b_factor_changed (GtkWidget       *widget,
								 GdkEventKey     *event,
								 gpointer         user_data);

   void fill_option_menu_with_map_options(GtkWidget *option_menu, GtkSignalFunc signal_func); 
   void fill_option_menu_with_map_options(GtkWidget *option_menu, GtkSignalFunc signal_func,
					  int imol_active_position); 
   void set_on_off_skeleton_radio_buttons(GtkWidget *skeleton_frame); 
   void set_on_off_single_map_skeleton_radio_buttons(GtkWidget *skeleton_frame, 
						     int i); 
   void set_contour_sigma_button_and_entry(GtkWidget *window, int imol);
      
   void fill_option_menu_with_refmac_options(GtkWidget *option_menu); 
   void fill_option_menu_with_refmac_methods_options(GtkWidget *option_menu); 
   void fill_option_menu_with_refmac_phase_input_options(GtkWidget *option_menu); 
   void fill_option_menu_with_refmac_labels_options(GtkWidget *option_menu); 
   void fill_option_menu_with_refmac_file_labels_options(GtkWidget *option_menu); 
   void fill_option_menu_with_refmac_ncycle_options(GtkWidget *option_menu); 
   void update_refmac_column_labels_frame(GtkWidget *optionmenu, 
					  GtkWidget *fobs_menu, GtkWidget *fiobs_menu, GtkWidget *fpm_menu,
					  GtkWidget *r_free_menu,
					  GtkWidget *phases_menu, GtkWidget *fom_menu, GtkWidget *hl_menu);
   static void refinement_map_select(GtkWidget *item, GtkPositionType pos);
   static void refinement_map_select_add_columns(GtkWidget *item, GtkPositionType pos);
   static void   skeleton_map_select(GtkWidget *item, GtkPositionType pos);
   static int map_for_skeletonize; // used by skeletonize_map; 
   static void   skeletonize_map(short int prune_it, int imol);
   static void unskeletonize_map(int imol);
   static void set_initial_map_for_skeletonize(); 
   static std::string refmac_ccp4i_project_dir;
   static std::string libcheck_ccp4i_project_dir;

   // refmac stuff
   static std::vector<int> *preset_number_refmac_cycles;
   static coot::refmac::refmac_refinement_method_type refmac_refinement_method;
   static coot::refmac::refmac_phase_input_type refmac_phase_input;
   static coot::refmac::refmac_use_tls_type     refmac_use_tls_flag;
   static coot::refmac::refmac_use_twin_type    refmac_use_twin_flag;
   static coot::refmac::refmac_use_sad_type     refmac_use_sad_flag;
   static coot::refmac::refmac_use_ncs_type     refmac_use_ncs_flag;
   static coot::refmac::refmac_use_intensities_type refmac_use_intensities_flag;
   static coot::refmac::refmac_used_mtz_file_type refmac_used_mtz_file_flag;
   static const gchar *saved_refmac_file_filename;
   static int refmac_ncycles;
   static void set_refmac_refinement_method(int method);
   static void refmac_change_refinement_method(GtkWidget *item, GtkPositionType pos);
   static void set_refmac_phase_input(int phase_flag);
   static void refmac_change_phase_input(GtkWidget *item, GtkPositionType pos);
   static void set_refmac_use_tls(int state);
   static void set_refmac_use_twin(int state);
   static void set_refmac_use_sad(int state);
   static void set_refmac_use_intensities(int state);
   static void set_refmac_used_mtz_file(int state);
   static void set_refmac_n_cycles(int no_cycles);
   static void refmac_change_ncycles(GtkWidget *item, GtkPositionType pos);
   static void set_refmac_use_ncs(int state);
   static std::vector<coot::refmac::sad_atom_info_t> refmac_sad_atoms;
   static GtkWidget *refmac_dialog_mtz_file_label;
   void add_refmac_sad_atom(const char *atom_name, float fp, float fpp, float lambda);
   void add_refmac_ncycle_no(int &cycle);
   static short int have_sensible_refmac_params;
   static std::string refmac_mtz_file_filename;
   static std::string refmac_fobs_col;
   static std::string refmac_sigfobs_col;
   static std::string refmac_r_free_col;
   static int refmac_r_free_flag_sensible;
	
   std::string Refmac_mtz_file_filename() const { return refmac_mtz_file_filename; }
   std::string Refmac_fobs_col() const { return refmac_fobs_col; }
   std::string Refmac_sigfobs_col() const { return refmac_sigfobs_col; }
   std::string Refmac_r_free_col() const { return refmac_r_free_col; }
   short int Refmac_r_free_sensible() const { return refmac_r_free_flag_sensible; }
   void store_refmac_params(const std::string &mtz_filename,
                            const std::string &fobs_col,
                            const std::string &sigfobs_col,
                            const std::string &r_free_col,
                            int r_free_flag_sensible);
   void store_refmac_phase_params(const std::string &phi,
                                  const std::string &fom,
                                  const std::string &hla,
                                  const std::string &hlb,
                                  const std::string &hlc,
                                  const std::string &hld);
   void store_refmac_file_mtz_filename(const std::string &mtz_filename);

   static int max_skeleton_search_depth;
   // Rotamer stuff
   static float rotamer_lowest_probability;  // Expressed as a percentage (2.0 default).
                                      // This is the P_r1_r2_r3_r4 probability, no bayesian 
                                      // stuff involved, currently.
   static float rotamer_distortion_scale; // for the validation graphs
   static int rotamer_fit_clash_flag;
   static short int in_rotamer_define;
   static int rotamer_residue_atom_index;
   static int rotamer_residue_imol;
   void do_rotamers(int atom_index, int imol) ; // display the rotamer option and display
                         			// the most likely in the graphics as a
			                        // moving_atoms_asc
   void fill_rotamer_selection_buttons(GtkWidget *window, int atom_index, int imol) const;

   short int generate_moving_atoms_from_rotamer(int irot);
   static void on_rotamer_selection_button_toggled (GtkButton       *button,
						    gpointer         user_data);
   void set_rotamer_fit_clash_flag(int i) { rotamer_fit_clash_flag = i; }
   // autofit rotamer:
   static short int in_auto_fit_define;


   // Mutation stuff
   static short int in_mutate_define;
   static int mutate_residue_atom_index;
   static int mutate_residue_imol;
   void do_mutation(const std::string &residue_type, short int do_stub_flag);
   static atom_selection_container_t standard_residues_asc;
   GtkWidget *wrapped_create_residue_type_chooser_window(bool show_stub_option_flag) const; /* set the stub checkbox */

   // mutate then auto fit:
   static short int in_mutate_auto_fit_define; 
   static int mutate_auto_fit_residue_atom_index;
   static int mutate_auto_fit_residue_imol;
   void do_mutation_auto_fit(const std::string &residue_type, short int do_stub_flag);
   static short int residue_type_chooser_auto_fit_flag;
   static short int residue_type_chooser_stub_flag;
   static short int mutate_auto_fit_do_post_refine_flag; 
   static short int rotamer_auto_fit_do_post_refine_flag; 

   // mutate sequence
   static std::string mutate_sequence_chain_from_optionmenu;
   static int         mutate_sequence_imol;

   // align and mutate
   static int         align_and_mutate_imol;
   static std::string align_and_mutate_chain_from_optionmenu;

   void mutate_chain(int imol, const std::string &chain_id,
		     const std::string &seq);

   // save molecule [option menu usage]
   static int save_imol;

   // Pointer Distances
   static float pointer_min_dist;
   static float pointer_max_dist;
   static int show_pointer_distances_flag;
   void clear_pointer_distances();
   static std::vector<std::pair<clipper::Coord_orth, clipper::Coord_orth> > *pointer_distances_object_vec;
   static void pointer_distances_objects(); // draw them
   void make_pointer_distance_objects(); // (re)generate them

   // Dynamic distances to intermediate atoms:
   static short int in_dynamic_distance_define;
   static coot::intermediate_atom_distance_t running_dynamic_distance;
   static std::vector<coot::intermediate_atom_distance_t> dynamic_distances;

   // Pointer atoms
   static short int pointer_atom_is_dummy; // force dummy atom, no atom type choice.
   void place_dummy_atom_at_pointer();
   void place_typed_atom_at_pointer(const std::string &type);
   int pointer_atom_molecule() const;
   static int user_pointer_atom_molecule; // set to -1;

   // save state, return success status of writing
   int save_state();
   int save_state_file(const std::string &filename);
   int save_history() const;
   std::vector<std::string> save_state_data_and_models(short int lang_flag) const;
   std::vector<std::string> save_state_data_and_models(const std::string &filename,
						       short int lang_flag) const;
   

   static std::string save_state_file_name;

   // show citation?
   static short int show_citation_notice; 

   static void info_dialog(const std::string &s);
   
   // Return success status.
   // 
   short int write_state(const std::vector<std::string> &commands,
			 const std::string &filename) const; 

   //
   int check_for_unsaved_changes() const; // in state
   void fill_unsaved_changes_dialog(GtkWidget *dialog) const; 

   // File selection directory saving:
   // (uses private directory_for_fileselection)
   // 
   void set_directory_for_fileselection(GtkWidget *fileselection) const;
   void save_directory_from_fileselection(const GtkWidget *fileselection);
   void save_directory_for_saving_from_fileselection(const GtkWidget *fileselection);
   void set_file_for_save_fileselection(GtkWidget *fileselection) const;

   // for file_chooser
#if (GTK_MAJOR_VERSION > 1)
   void set_directory_for_filechooser(GtkWidget *fileselection) const;
   void save_directory_from_filechooser(const GtkWidget *fileselection);
   void save_directory_for_saving_from_filechooser(const GtkWidget *fileselection);
   void set_file_for_save_filechooser(GtkWidget *fileselection) const;
#endif

   // saving temporary files (undo)
   //
   std::string save_molecule_dir(int imol) const;
   void apply_undo(); 
   void apply_redo();
   void activate_redo_button();
   // used by option menu item callback which sets the molecule for undoing
   void set_undo_molecule_number(int i) { undo_molecule = i; }
   // undo_molecule_select uses set_undo_molecule_number()
   static void undo_molecule_select(GtkWidget *item, GtkPositionType pos);
   void fill_option_menu_with_undo_options(GtkWidget *option_menu); // not const
   int Undo_molecule(coot::undo_type) const; // return -2 on ambiguity, -1 on unset
			      // and a molecule number >=0 for no
			      // ambiguity (or undo_molecule has been
			      // set already).


   // used in globjects:
   // 
   int check_if_in_range_defines(GdkEventButton *event,
				 const GdkModifierType &state);
   void check_if_moving_atom_pull(); // and setup moving atom-drag if we are.

   // baton stuff:
   // 
   static short int draw_baton_flag;
   static short int baton_mode; // if set, rotation moves the baton, not the view
   static short int baton_tmp_atoms_to_new_molecule; 
   static void baton_object();
   void start_baton_here(); // modify baton root and tip
   void accept_baton_position(); /* put an atom at the tip and move baton */
   int baton_build_atoms_molecule() const; // -1 on no such molecule
					   // with name "Baton Atoms"
   void rotate_baton(const double &x, const double &y);
   void toggle_baton_mode();  // on a "b" button press

   void baton_try_another();
   void shorten_baton();
   void lengthen_baton();
   void baton_build_delete_last_residue(); 
   void set_max_skeleton_search_depth(int v) { max_skeleton_search_depth = v; }
   void set_baton_build_params(int istart_resno,
			      const char *chain_id, 
			      const char *backwards);


   // save parameters for running refmac: not here.  Now in molecule class
//    void save_refmac_params(std::string fobs, 
// 			   std::string sig_fobs, std::string r_free, 
// 			   short int r_free_flag);
//    static std::string refmac_fobs_col;
//    static std::string refmac_sigfobs_col;
//    static std::string refmac_r_free_col;
//    short int refmac_r_free_flag; // is sensible r-free column?

   // bond thickness
   void set_bond_thickness(int imol, float thick);
   static int bond_thickness_intermediate_value; // not intermediate atoms
   static void bond_parameters_molecule_menu_item_select(GtkWidget *item, GtkPositionType pos); 
   static int bond_parameters_molecule;
   static void fill_bond_parameters_internals(GtkWidget *w, int imol);
   static void bond_width_item_select(GtkWidget *item, GtkPositionType pos);
   static float bond_thickness_intermediate_atoms; // white atoms
   void set_bond_thickness_intermediate_atoms(float f);
   // colour map rotation adjustment change:
   static void bond_parameters_colour_rotation_adjustment_changed(GtkAdjustment *adj,
								  GtkWidget *window);
   void fill_bond_colours_dialog_internal(GtkWidget *w);
   // new style: each molecule has its own bond rotation value: Here
   // is where the individual colour rotation steps are changed:
   static void bonds_colour_rotation_adjustment_changed(GtkAdjustment *adj,
							GtkWidget *window);



   // residue info
   // 
   // Why not use just the atom index, you might ask.  Because, from
   // scriting we will pass the atom attributes, not the atom index so
   // we don't have to pick an atom in the c-interface and then expand
   // it to the residue in graphics-info.
   // 
   static short int   residue_info_pending_edit_b_factor;
   static short int   residue_info_pending_edit_occ;
   static int         residue_info_n_atoms; // so that we can release the memory and
					    // propogate the changes to the other widgets
   static std::vector<coot::select_atom_info> *residue_info_edits;
   void reset_residue_info_edits() { // set residue_info_edits to zero elements
      residue_info_edits->resize(0);
   }
   void residue_info_release_memory(GtkWidget *dialog); 
   static void  residue_info_add_b_factor_edit(coot::select_atom_info sai, float val);
   static void  residue_info_add_occ_edit(     coot::select_atom_info sai, float val);
   void apply_residue_info_changes(GtkWidget *t);
   static void residue_info_edit_b_factor_apply_to_other_entries_maybe(GtkWidget *widget); 
   static void residue_info_edit_occ_apply_to_other_entries_maybe(GtkWidget *widget); 

   // crosshairs
   static short int draw_crosshairs_flag; 
   void crosshairs_text() const; 

   // environment_show_distances
   // Return imol = -1 if no (close) atoms found.
   // 
   // index, imol
   std::pair<int, int> get_closest_atom() const;

   // 
   static GdkCursorType pick_cursor_index; // user setable
   static void pick_cursor_maybe();
   static void pick_cursor_real();
   static void normal_cursor();
   static void watch_cursor();
   static void fleur_cursor();


   // Alternate Conformation
   static GtkWidget *add_alt_conf_dialog; 
   static short int alt_conf_split_type;
   static short int alt_conf_split_type_number(); 
   static short int in_add_alt_conf_define;
   static int add_alt_conf_atom_index;
   static int add_alt_conf_imol; 
   void split_residue(int imol, int atom_index); 
   void split_residue(int imol, std::string &chain_id, int resno, std::string &altconf);
   void split_residue_range(int imol, int index_1, int index2);
   // we add altconf because we want to triple-split a residue
   // sometimes (which we do by splitting one of the already existing
   // altconfs - we need to know which one).
   static float add_alt_conf_new_atoms_occupancy;
   static short int show_alt_conf_intermediate_atoms_flag; 
   static void new_alt_conf_occ_adjustment_changed(GtkAdjustment *adj, gpointer user_data);

   
   // Backbone torsion
   // 
   static short int in_backbone_torsion_define;
   void execute_setup_backbone_torsion_edit(int imol, int atom_index); 

   // Edit Phi/Psi
   static short int in_edit_phi_psi_define; // set by button callback: setup_edit_phi_psi()
   static int edit_phi_psi_atom_index;  
   static int edit_phi_psi_imol; // needed, yes (for the ramachandran widget)
   void edit_phi_psi();
   void execute_edit_phi_psi(int atom_index, int imol); // c.f. execute_rotate_translate_ready()
   void set_edit_phi_psi_to(double phi, double psi); // a callback
						     // from the
						     // ramachandran
						     // widget
   void rama_plot_for_single_phi_psi(int imol, int atom_index);
   void rama_plot_for_2_phi_psis(int imol, int atom_index);

   // Edit Chi
   static short int in_edit_chi_angles_define;
   static short int in_edit_chi_mode_flag; // c.f. in baton_mode
   static short int in_edit_chi_mode_view_rotate_mode;
   static coot::atom_spec_t chi_angles_clicked_atom_spec;
   void execute_edit_chi_angles(int atom_index, int imol);
   int wrapped_create_edit_chi_angles_dialog(const std::string &res_type);
   // used by above:
   int fill_chi_angles_vbox(GtkWidget *vbox, std::string res_type);
   void clear_out_container(GtkWidget *vbox);


   // real values start at 1:
   static int edit_chi_current_chi;
   static void on_change_current_chi_button_clicked(GtkButton *button,
						    gpointer user_data);
   static void on_change_current_chi_button_entered(GtkButton *button,
						    gpointer user_data);
   static short int moving_atoms_move_chis_flag;
   void setup_flash_bond_internal(int ibond);

   //! angle in degrees.
   void rotate_chi(double x, double y); // a 'callback' from the
					     // chi editting
   					     // function

   // Torsion General
   static short int in_edit_torsion_general_flag; 
   void rotate_chi_torsion_general(double x, double y);

   void execute_torsion_general();

   // which bond are we rotating about in the ligand?
   
   void add_flash_bond(const std::pair<clipper::Coord_orth, clipper::Coord_orth> &p); 
   static std::pair<clipper::Coord_orth, clipper::Coord_orth> flash_bond;
   static short int draw_chi_angle_flash_bond_flag; 
   static void draw_chi_angles_flash_bond();

   // Tinker with the atom positions of residue
   // (used by rotate_chi)
   // We need to pass the asc for the mol because we need it for seekcontacts()
   // 
   short int update_residue_by_chi_change(CResidue *residue,
					  atom_selection_container_t &asc,
					  int chi, double diff);

   std::vector<std::vector<int> > get_contact_indices_from_restraints(CResidue *residue, const atom_selection_container_t &asc, short int is_regular_residue_flag) const;

   // Do 180 degree sidechain flip stuff
   //
   static short int in_180_degree_flip_define;

   // More moving backbone stuff:
   //
   void set_moving_atoms(atom_selection_container_t asc, int imol, int new_coords_type);
   // 
   // changes moving atoms:
   // 
   void change_peptide_carbonyl_by(double angle); // in degrees.
   void change_peptide_peptide_by(double angle); // in degress

   // button call backs for moving backbone (this is the alternative
   // method to using rottrans buttons method (pointer to info class
   // attached to rot/trans window))
   // These change private data:
   void set_backbone_torsion_peptide_button_start_pos(int ix, int iy);
   void change_peptide_peptide_by_current_button_pos(int ix, int iy);
   void set_backbone_torsion_carbonyl_button_start_pos(int ix, int iy);
   void change_peptide_carbonyl_by_current_button_pos(int ix, int iy);
   
   // sequence view:
   // 
   // return NULL on failure (this molecule does not have a sequence
   // view)
   //
#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)
   coot::sequence_view *get_sequence_view(int imol);
   static coot::rama_plot *edit_phi_psi_plot;
#endif // HAVE_GTK_CANVAS/GNOME_CANVAS

   // distances and angles displayed on screen
   // uses distance_objects vector
   static void geometry_objects();
   static void draw_dynamic_distances();
   static void draw_generic_objects();
   static void draw_generic_text();
   void clear_simple_distances();
   void clear_last_simple_distance();
   static GtkWidget *geometry_dialog;
   void unset_geometry_dialog_distance_togglebutton();
   void unset_geometry_dialog_angle_togglebutton();
   void unset_geometry_dialog_torsion_togglebutton();
   void unset_geometry_dialog_dynamic_distance_togglebutton();
   

   static short int find_hydrogen_torsions;

   // pickable moving atoms molecule
   pick_info moving_atoms_atom_pick() const;
   static short int in_moving_atoms_drag_atom_mode_flag;
   // when shift is pressed move (more or less) just the "local"
   // moving atoms atoms, we do this by making the shift proportional
   // to the square of the distance ratio.
   void move_moving_atoms_by_shear(int screenx, int screeny, short int squared_flag);
   void move_moving_atoms_by_shear_internal(const coot::Cartesian &diff_std,
					    short int squared_flag);
   void move_moving_atoms_by_simple_translation(int screenx, int screeny); // for rot/trans
   void move_single_atom_of_moving_atoms(int screenx, int screeny);
   short int rotate_intermediate_atoms_maybe(short int axis, double angle); 
                                                 // do it if have intermediate atoms
                                                 // and ctrl is pressed.
   // axis: 0 for Z, 1 for X.
   void rotate_intermediate_atoms_round_screen_z(double angle);
   void rotate_intermediate_atoms_round_screen_x(double angle);

   static void drag_intermediate_atom(const coot::atom_spec_t &atom_spec, const clipper::Coord_orth &pt);
   static void mark_atom_as_fixed(int imol, const coot::atom_spec_t &atom_spec, bool state);
   // static std::vector<CAtom *> fixed_intermediate_atoms;
   static bool fixed_atom_for_refinement_p(CAtom *);  // examines the imol_moving_atoms molecule
                                                      // for correspondence

   // so that we know that fixed_points_sheared_drag_1 and
   // fixed_points_sheared_drag_2 are sensible:
   // 
   static short int have_fixed_points_sheared_drag_flag;
   void set_fixed_points_for_sheared_drag();
   void do_post_drag_refinement_maybe();
#ifdef  HAVE_GSL
   int last_restraints_size() const { return last_restraints.size(); };
#endif // HAVE_GSL   
   static int dragged_refinement_steps_per_frame;
   static short int dragged_refinement_refine_per_frame_flag;
   static bool refinement_move_atoms_with_zero_occupancy_flag;

   // 
   static short int draw_zero_occ_spots_flag;

   // static
   static std::string ccp4_defs_file_name();
   static int ccp4_projects_index_last;
   void set_directory_for_fileselection_string(std::string filename);
   void set_directory_for_saving_for_fileselection_string(std::string filename);
#if (GTK_MAJOR_VERSION > 1)
   void set_directory_for_filechooser_string(std::string filename);
   void set_directory_for_saving_for_filechooser_string(std::string filename);
#endif // GKT_MAJOR_VERSION
   static int file_selection_dialog_x_size;
   static int file_selection_dialog_y_size; 

   // Origin marker for Johan
   static int show_origin_marker_flag;
   

   // Check waters interface for Florence:
   static int   check_waters_molecule;
   static float check_waters_b_factor_limit;
   static float check_waters_map_sigma_limit;
   static float check_waters_min_dist_limit;
   static float check_waters_max_dist_limit;
   static float check_waters_by_difference_map_sigma_level;
   // save the dialog so that . and , can be used on it
   static GtkWidget *checked_waters_baddies_dialog;

   // zoom widget:
   // 
   static void zoom_adj_changed(GtkAdjustment *adj, GtkWidget *window);
   static void set_zoom_adjustment(GtkWidget *widget);

   //
   static int show_paths_in_display_manager_flag;

   // scrollin' scrollin' scrollin'... Shall we stop? When shall we stop?
   static short int stop_scroll_diff_map_flag;
   static short int stop_scroll_iso_map_flag;
   static float stop_scroll_iso_map_level;
   static float stop_scroll_diff_map_level;

   // globbing [we want a "Filter" button so we need a list of
   // extensions that are coordinates].
   // 
   // Same for "data".
   // 
   static std::vector<std::string> *coordinates_glob_extensions;
   static std::vector<std::string> *data_glob_extensions;
   static std::vector<std::string> *map_glob_extensions;
   static std::vector<std::string> *dictionary_glob_extensions;
   void add_coordinates_glob_extension(const std::string &extension);
   void add_data_glob_extension(const std::string &extension);
   void add_map_glob_extension(const std::string &extension);
   void add_dictionary_glob_extension(const std::string &extension);
   void remove_coordinates_glob_extension(const std::string &extension);
   void remove_data_glob_extension(const std::string &extension);
   void remove_map_glob_extension(const std::string &extension);
   void remove_dictionary_glob_extension(const std::string &extension);
   static int filter_fileselection_filenames_flag;

   // superposition
   static int superpose_imol1;
   static int superpose_imol2;
   static std::string superpose_imol1_chain;
   static std::string superpose_imol2_chain;
   static void superpose_optionmenu_activate_mol1(GtkWidget *item, GtkPositionType pos);
   static void superpose_optionmenu_activate_mol2(GtkWidget *item, GtkPositionType pos);
   static void superpose_moving_chain_option_menu_item_activate (GtkWidget *item,
								 GtkPositionType pos);
   static void superpose_reference_chain_option_menu_item_activate (GtkWidget *item,
								    GtkPositionType pos);
   static void fill_superpose_option_menu_with_chain_options(GtkWidget *chain_optionmenu, 
							     int is_reference_structure_flag);
   static int         ramachandran_plot_differences_imol1;
   static int         ramachandran_plot_differences_imol2;
   static std::string ramachandran_plot_differences_imol1_chain;
   static std::string ramachandran_plot_differences_imol2_chain;
   static float rama_level_prefered;
   static float rama_level_allowed;
   static float rama_plot_background_block_size; // divisible into 360 preferably.

   void superpose_with_atom_selection(atom_selection_container_t asc_ref,
				      atom_selection_container_t asc_mov,
				      int imol_mov,
				      std::string moving_mol_name,
				      std::string referennce_mol_name,
				      short int move_copy_of_imol2_flag);

   void print_ssm_sequence_alignment(CSSMAlign *SSMAlign,
				     atom_selection_container_t asc_ref,
				     atom_selection_container_t asc_mov,
				     PCAtom *atom_selection1, 
				     PCAtom *atom_selection2, 
				     int n_selected_atoms_1, int n_selected_atoms_2, 
				     short int move_copy_of_imol2_flag);

   // widget stuff:

   static std::pair<short int, float> float_from_entry(GtkWidget *entry);
   static std::pair<short int, int>   int_from_entry(GtkWidget *entry);

   // for widgets that are created by graphics_info_t functions (we
   // can't call c-interface.cc versions of these functions from here);
   static void      store_window_position(int window_type, GtkWidget *widget);
   static void set_transient_and_position(int widget_type, GtkWidget *window);

   // contour level saving
   void set_last_map_sigma_step(float level);

   // ----- Refmac state params: ----
   // 
   // Normally a new map gets its own colour depending on the molecule
   // number, but often when you run refmac you want the new map to
   // have the colour of the old map (blue say) and the old map gets
   // coloured with the new colour scheme (horrible red, often).
   // 
   static short int swap_pre_post_refmac_map_colours_flag;

   // ------ (More) water checking: -------
   void check_waters_by_difference_map(int imol_waters, int imol_diff_map,
				       int interactive_flag);
   // and a gui wrapper for that:
   GtkWidget *wrapped_create_checked_waters_by_variance_dialog(const std::vector <coot::atom_spec_t> &v, int imol);
   // and its buttons callbacks:
   static void on_generic_atom_spec_button_clicked (GtkButton *button,
						    gpointer user_data);


   void check_chiral_volumes(int imol);
   GtkWidget *wrapped_check_chiral_volumes_dialog(const std::vector <coot::atom_spec_t> &v,
						  int imol);
   static int chiral_volume_molecule_option_menu_item_select_molecule; // option menu 
   static void on_bad_chiral_volume_button_clicked(GtkButton *button,
						   gpointer user_data);
   // Tell us which residue types for chiral volumes restraints were missing:
   GtkWidget *wrapped_create_chiral_restraints_problem_dialog(const std::vector<std::string> &sv) const; 



   // unbonded star size - actually too messy to fix properly - so not used.
   static float unbonded_atom_star_size;

   // -------- povray/raster3d --------
   short int raster3d(std::string filename);
   short int povray(std::string filename);
   static float raster3d_bond_thickness; 
   static float raster3d_atom_radius; 
   static float raster3d_density_thickness;
   static int renderer_show_atoms_flag;
   static float raster3d_bone_thickness; 

   // ------- auto read mtz file: --------
   static int auto_read_do_difference_map_too_flag;

   // ------- refmac molecules option menu  -----
   static int refmac_molecule; 

   // ------ add OXT -------
   void fill_add_OXT_dialog_internal(GtkWidget *w);
   static int add_OXT_molecule;
   static void add_OXT_molecule_item_select(GtkWidget *item,
					    GtkPositionType pos);
   void fill_add_OXT_dialog_internal(GtkWidget *widget, int imol);
   // return the default chain string (top of the list).
   // (return "no-chain" if it was not assigned (nothing in the list)).
   static std::string fill_chain_option_menu(GtkWidget *option_menu,
					     int imol,
					     GtkSignalFunc signal_func);
   static std::string add_OXT_chain;
   static void add_OXT_chain_menu_item_activate (GtkWidget *item,
						 GtkPositionType pos);

   // 
   static GtkWidget *wrapped_nothing_bad_dialog(const std::string &label);

   // ----- merge molecules ------
   static int merge_molecules_master_molecule;
   static std::vector<int> *merge_molecules_merging_molecules;

   // ------ change chain ids:
   static int change_chain_id_molecule;
   static std::string change_chain_id_from_chain;
   

   // ----- renumber residue range -------
   static void fill_renumber_residue_range_dialog(GtkWidget *w);
   static void renumber_residue_range_molecule_menu_item_select(GtkWidget *item,
								GtkPositionType pos);
   static int renumber_residue_range_molecule;
   static std::string renumber_residue_range_chain;
   void fill_renumber_residue_range_internal(GtkWidget *w, int imol);
   static void renumber_residue_range_chain_menu_item_select(GtkWidget *item, GtkPositionType pos);

   // -------- public toggle button control: ---------
   void untoggle_model_fit_refine_buttons_except(const std::string &button_name);
   static void set_model_fit_refine_button_names(GtkWidget *w);

   
   // -------- keyboard rotamer control: ---------
   static void rotamer_dialog_next_rotamer();
   static void rotamer_dialog_previous_rotamer();
   static void rotamer_dialog_neighbour_rotamer(int istep); // could be private

   // -------- keyboard difference map peak control: ------------
   static void difference_map_peaks_next_peak();
   static void difference_map_peaks_previous_peak();
   static void difference_map_peaks_neighbour_peak(int istep); // could be private

   //  -------- keyboard baddie waters control: ------
   static void checked_waters_next_baddie(int dir);

   // ------ NCS ------
   static float ncs_min_hit_ratio;
   static short int ncs_maps_do_average_flag;
   /* At what level of homology should we say that we can't see homology
      for NCS calculation? (default 0.8) */
   static float ncs_homology_level;
   
   // ------------- validation: -------------------

   // pretty graphs on a canvas: (not const because geom_p may be added to)
   void geometric_distortion(int imol);
   void b_factor_graphs(int imol);
   void omega_graphs(int imol);
   coot::rotamer_graphs_info_t rotamer_graphs(int imol); // give results back to scripting layer
   void density_fit_graphs(int imol);
   static GtkWidget *wrapped_create_diff_map_peaks_dialog(const std::vector<std::pair<clipper::Coord_orth, float> > &centres, float map_sigma);
   // the buttons callback for above:
   static void on_diff_map_peak_button_selection_toggled (GtkButton       *button,
							  gpointer         user_data);
   static std::vector<clipper::Coord_orth> *diff_map_peaks;
   static int max_diff_map_peaks;
   void clear_diff_map_peaks();
   static float difference_map_peaks_sigma_level;

   // ---------------- backup filenames ----------------------
   static short int unpathed_backup_file_names_flag;

   // --------- Miguel's axis orientation matrix ---------------
   static GL_matrix axes_orientation;
   static short int use_axes_orientation_matrix_flag;
   
   // --------- preferences ---------------
   static GtkWidget *preferences_widget;
   static int mark_cis_peptides_as_bad_flag;

   static std::vector<std::string> *preferences_general_tabs;
   static std::vector<std::string> *preferences_bond_tabs;
   static std::vector<std::string> *preferences_geometry_tabs;
   static std::vector<std::string> *preferences_colour_tabs;
   static std::vector<std::string> *preferences_map_tabs;
   static std::vector<std::string> *preferences_other_tabs;

   static std::vector<coot::preferences_icon_info_t> *model_toolbar_icons;

   short int save_preference_file(const std::string &filename, short int il);
   static std::vector<coot::preference_info_t> preferences_internal;
   static std::vector<coot::preference_info_t> preferences_internal_default;
   void make_preferences_internal();
   void preferences_internal_change_value(int preference_type, int ivalue);
   void preferences_internal_change_value(int preference_type, float fvalue);
   void preferences_internal_change_value(int preference_type, 
					  float fvalue1, float fvalue2, float fvalue3);
   void preferences_internal_change_value(int preference_type, int ivalue1, int ivalue);
  
#if (GTK_MAJOR_VERSION > 1)
   static void preferences_model_toolbar_icon_toggled(GtkCellRendererToggle *button,
					      gchar *path,
		    			      gpointer data);
   static void update_model_toolbar_icons(GtkTreeModel *model);
   void fill_preferences_model_toolbar_icons(GtkWidget *preferences,
					     GtkWidget *scrolled_window);
#endif // GTK_MAJOR_VERSION
   void show_hide_model_toolbar_icon_pos(int pos, int show_hide_flag);
   std::vector<int> get_model_toolbar_icons_list();

   // --- remote controlled coot: ----
   static int try_port_listener;
   static int remote_control_port_number;
   static std::string remote_control_hostname;
   static int coot_socket_listener_idle_function_token; // -1 default (off)
   static std::string socket_string_waiting;
   static volatile bool have_socket_string_waiting_flag;
   static volatile bool socket_string_waiting_mutex_lock; 
#ifdef USE_GUILE
   static SCM safe_scheme_command(const std::string &scheme_command);
   static SCM process_socket_string_waiting();
#endif
   static gboolean process_socket_string_waiting_bool(gpointer user_data);

   // --------- Tip of the Day ---------------
   static short int do_tip_of_the_day_flag; 
   
   // --------- LSQing ---------------
   std::pair<int, clipper::RTop_orth> apply_lsq(int imol_ref, int imol_moving,
		 const std::vector<coot::lsq_range_match_info_t> &matches);
   static std::vector<coot::lsq_range_match_info_t> *lsq_matchers;
   // the simple widget for LSQing: (perhaps these should be vectors of strings 
   // in the general case (more complex widget)?)
   static std::string lsq_match_chain_id_ref;
   static std::string lsq_match_chain_id_mov;
   static int lsq_ref_imol;
   static int lsq_mov_imol;

   // -------- some people dont want restype and slashes in atom label:
   static short int brief_atom_labels_flag;

   // -------- undo move: suggested by Frank von Delft -----
   void undo_last_move(); 

   // ---- auto read MTZ file ---
   static std::string auto_read_MTZ_FWT_col;
   static std::string auto_read_MTZ_PHWT_col;
   static std::string auto_read_MTZ_DELFWT_col;
   static std::string auto_read_MTZ_PHDELWT_col;

   // ---- cis trans conversion ---
   void cis_trans_conversion(CAtom *at, int imol, short int is_N_flag);

   // symmetry control dialog:
   GtkWidget *wrapped_create_symmetry_controller_dialog() const;

   // ----- LSQ Plane -----
   static std::vector<clipper::Coord_orth> *lsq_plane_atom_positions;
   int add_lsq_plane_atom(int imol, int atom_index);
   int measure_lsq_plane_deviant_atom(int imol, int atom_index);
   int remove_last_lsq_plane_atom();
   void render_lsq_plane_atoms(); // put a blob at atoms in lsq_plane_atom_positions
   GtkWidget *wrapped_create_lsq_plane_dialog();
   static GtkWidget *lsq_plane_dialog;
   static short int in_lsq_plane_deviation;
   static short int in_lsq_plane_define;

   // -------- Fffearing -------------
   static float fffear_angular_resolution; 

   // ------- generic object interface ------
   static std::vector<coot::generic_display_object_t> *generic_objects_p;
   int new_generic_object_number(const std::string &name) { 
     coot::generic_display_object_t o(name);
     generic_objects_p->push_back(o);
     int r = generic_objects_p->size() -1;
     return r;
   } 

   // ---- open url
   // Hmm.. do we need a vector here?
   static std::string browser_open_command;

   // -- default bond width
   static int default_bond_width;

   // ---- default sigma level:
   static float default_sigma_level_for_map;
   static float default_sigma_level_for_fofc_map;

   // probe dots on intermediate atoms (we need to have hydrogens)
   static short int do_probe_dots_on_rotamers_and_chis_flag;
   static short int do_probe_dots_post_refine_flag;
   void do_probe_dots_on_rotamers_and_chis();
   void do_probe_dots_post_refine();
   void do_interactive_probe() const;

   // can be private?
   void setup_for_probe_dots_on_chis_molprobity(int imol);
   static coot::Cartesian probe_dots_on_chis_molprobity_centre;
   static float probe_dots_on_chis_molprobity_radius;

   // a text string and a handle (so that it can be removed)
   static std::vector<coot::generic_text_object_t> *generic_texts_p;

   // -- move molecule here
   static int move_molecule_here_molecule_number;
   static void move_molecule_here_item_select(GtkWidget *item,
					      GtkPositionType pos);

   // -- make the key user changable.  A template for other bound
   // functions:
   static int ncs_residue_skip_key;
   static int update_go_to_atom_from_current_residue_key;

   // -- PHENIX support
   static std::string external_refinement_program_button_label;

#ifdef HAVE_GSL
   // ---- pseudo bond for sec str restraints
   static coot::pseudo_restraint_bond_type pseudo_bonds_type;
#endif // HAVE_GSL


   // ---- a debugging thing for user to give me feedback
   static int debug_atom_picking;

   // ---- default new atoms b factor
   static float default_new_atoms_b_factor; 

   // reset the b factors after atoms were moved, default no
   static int reset_b_factor_moved_atoms_flag;

   // ---- disable state script writing on exiting (no-guano) 
   static bool disable_state_script_writing; 

   // ---- man this is tricky.  Store the listener socket state, so
   // that we can call coot-listener-idle-function-proc if there is a
   // good socket.  The actual socket is a scheme variable.
   // 
   static int listener_socket_have_good_socket_state; 

   void post_recentre_update_and_redraw(); 


#ifdef USE_MYSQL_DATABASE
   // MYSQL database
   static MYSQL *mysql;
   static int query_number;
   static std::string mysql_host;
   static std::string mysql_user;
   static std::string mysql_passwd;
string   static std::string sessionid; 
   static std::pair<std::string, std::string> db_userid_username;
#endif

   static std::vector<coot::view_info_t> *views;
   static float views_play_speed;

   static std::string movie_file_prefix;
   static int movie_frame_number;
   static int make_movie_flag;
   static void dump_a_movie_image(); // should be private
   static int screendump_image(const std::string &file_name); 

   // ------------------------- restraints editor ----------------------------
   // 
#if (GTK_MAJOR_VERSION > 1) 
   static std::vector<coot::restraints_editor> restraints_editors;
   coot::restraints_editor get_restraints_editor(GtkWidget *w) { 
     coot::restraints_editor r;
     int found_index = -1;
     for (unsigned int i=0; i<restraints_editors.size(); i++) { 
       if (restraints_editors[i].is_valid()) { 
	 std::cout << " debug:: testing restraints editor number "
		   << i << " of " << restraints_editors.size() << " "
		   << restraints_editors[i].get_dialog() << std::endl;
       }
     }
     for (unsigned int i=0; i<restraints_editors.size(); i++) { 
       if (restraints_editors[i].is_valid()) { 
	 std::cout << " debug:: testing restraints editor number "
		   << i << " of " << restraints_editors.size() << std::endl;
         if (restraints_editors[i].matches_dialog(w)) { 
           found_index = i;
	   std::cout << "restraints editor index " << i << " matches" << std::endl;
           break;
         }
       }
     }
     if (found_index != -1)
       r = restraints_editors[found_index];
     std::cout << "Returing restraints editor with dialog " << r.get_dialog() 
	       << " from get_restraints_editor() from dialog " 
	       << w  << std::endl;
     return r;
   } 
#endif //GTK_MAJOR_VERSION

};


class molecule_rot_t { 

 public:   
   static float x_axis_angle;
   static float y_axis_angle;
}; 
   
void initialize_graphics_molecules();


void do_accept_reject_dialog(std::string fit_type, const coot::refinement_results_t &ref_results);
void add_accept_reject_lights(GtkWidget *window, const coot::refinement_results_t &ref_results);
// return a pointer to a "new" object
GdkColor colour_by_distortion(float dist);
GdkColor colour_by_rama_plot_distortion(float plot_value);
void set_colour_accept_reject_event_box(GtkWidget *eventbox, GdkColor *col);
GtkWidget *wrapped_create_accept_reject_refinement_dialog();
void update_accept_reject_dialog_with_results(GtkWidget *accept_reject_dialog,
					      coot::accept_reject_text_type text_type,
					      const coot::refinement_results_t &rr);


// Some currently useless Perspective View definition
// 
#define VIEW_ASPECT 1.3


#endif // GRAPHICS_INFO_H
