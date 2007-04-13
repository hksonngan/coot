/* src/c-interface-waters.cc
 * 
 * Copyright 2004 2005 The University of York
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
#include <iostream>

#ifdef USE_GUILE
#include <guile/gh.h>
#endif // USE_GUILE

#ifdef USE_PYTHON
#include "Python.h"
#endif // USE_PYTHON

#if defined _MSC_VER
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


#include "graphics-info.h"

#include "c-interface.h"

#include "wligand.hh"


GtkWidget *wrapped_create_unmodelled_blobs_dialog() { 

   GtkWidget *dialog = create_unmodelled_blobs_dialog();
   int ifound; 
   short int diff_maps_only_flag = 0;
   ifound = fill_ligands_dialog_map_bits_by_dialog_name(dialog, "find_blobs_map", 
							diff_maps_only_flag);
   if (ifound == 0) {
      std::cout << "Error: you must have a map to search for blobs!"
		<< std::endl;
   } 
   ifound = fill_ligands_dialog_protein_bits_by_dialog_name(dialog, "find_blobs_protein");
   if (ifound == 0) {
      std::cout << "Error: you must have a protein to mask the map to search for blobs!"
		<< std::endl;
   }

   // fill sigma level

   GtkWidget *entry;
   entry = lookup_widget(dialog, "find_blobs_peak_level_entry");

   char *txt = get_text_for_find_waters_sigma_cut_off();
   gtk_entry_set_text(GTK_ENTRY(entry), txt);
   free(txt);

   return dialog;
}

void execute_find_blobs_from_widget(GtkWidget *dialog) { 

   int imol_model = -1;
   int imol_for_map = -1;
   float sigma_cut_off = -1; 

   GtkWidget *entry = lookup_widget(dialog, "find_blobs_peak_level_entry");
   const gchar *txt = gtk_entry_get_text(GTK_ENTRY(entry));
   if (txt) { 
      float f = atof(txt);
      if (f > 0.0 && f < 1000.0) { 
	 sigma_cut_off = f;
      }
   }

   if (sigma_cut_off > 0.0) {

      // Find the first active map radiobutton
      GtkWidget *map_button;
      short int found_active_button_for_map = 0;
      for (int imol=0; imol<graphics_info_t::n_molecules; imol++) {
	 if (graphics_info_t::molecules[imol].has_map()) { 
	    std::string map_str = "find_blobs_map_radiobutton_";
	    map_str += graphics_info_t::int_to_string(imol);
	    map_button = lookup_widget(dialog, map_str.c_str());
	    if (map_button) { 
	       if (GTK_TOGGLE_BUTTON(map_button)->active) {
		  imol_for_map = imol;
		  found_active_button_for_map = 1;
		  break;
	       }
	    } else {
	       std::cout << "WARNING:: (error) " << map_str << " widget not found in "
			 << "execute_get_mols_ligand_search" << std::endl;
	    }
	 }
      }

      // Find the first active protein radiobutton
      GtkWidget *protein_button;
      short int found_active_button_for_protein = 0;
      for (int imol=0; imol<graphics_info_t::n_molecules; imol++) {
	 if (graphics_info_t::molecules[imol].has_model()) { 
	    std::string protein_str = "find_blobs_protein_radiobutton_";
	    protein_str += graphics_info_t::int_to_string(imol);
	    protein_button = lookup_widget(dialog, protein_str.c_str());
	    if (protein_button) { 
	       if (GTK_TOGGLE_BUTTON(protein_button)->active) {
		  imol_model = imol;
		  found_active_button_for_protein = 1;
		  break;
	       }
	    } else {
	       std::cout << protein_str << " widget not found in "
			 << "execute_get_mols_ligand_search" << std::endl;
	    }
	 }
      }

      if (! found_active_button_for_map) { 
	 std::cout << "INFO:: You need to have define a map to search for map blobs!\n";
      } else { 
	 if (! found_active_button_for_protein) { 
	 std::cout << "INFO:: You need to have define coordinates for masking to search for map blobs!\n";
	 } else { 
	    short int interactive_flag = 1;
	    execute_find_blobs(imol_model, imol_for_map, sigma_cut_off, interactive_flag);
	 }
      }
   } else { 
      std::cout << "WARNING:: nonsense sigma level " << sigma_cut_off
		<< " not doing search\n";
   }
}

void execute_find_blobs(int imol_model, int imol_for_map,
			float sigma_cut_off, short int interactive_flag) { 

   if (imol_model >= 0) { 
      if (imol_model < graphics_info_t::n_molecules) { 
	 if (graphics_info_t::molecules[imol_model].has_model()) { 

	    // check imol_for_map here.. FIXME

	    coot::ligand lig;
	    graphics_info_t g;
	    int n_cycles = 1;

	    short int mask_waters_flag; // treat waters like other atoms?
	    mask_waters_flag = g.find_ligand_mask_waters_flag;
	    // short int do_flood_flag = 0;    // don't flood fill the map with waters for now.

	    lig.import_map_from(g.molecules[imol_for_map].xmap_list[0], 
				g.molecules[imol_for_map].map_sigma());
	    // lig.set_masked_map_value(-2.0); // sigma level of masked map gets distorted
	    lig.set_map_atom_mask_radius(1.9); // Angstroms
	    lig.mask_map(g.molecules[imol_model].atom_sel.mol, mask_waters_flag);
	    std::cout << "using sigma cut off " << sigma_cut_off << std::endl;

	    lig.water_fit(sigma_cut_off, n_cycles);

	    // water fit makes big blobs
	    // *g.ligand_big_blobs = lig.big_blobs(); old style indirect method
                                        	    // to move to point
	    int n_big_blobs = lig.big_blobs().size();

	    if (interactive_flag) { 
	       if ( n_big_blobs > 0 ) {

		  GtkWidget *dialog = create_ligand_big_blob_dialog();
		  GtkWidget *main_window = lookup_widget(graphics_info_t::glarea, "window1");
		  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
		  GtkWidget *vbox = lookup_widget(dialog, "ligand_big_blob_vbox");
		  if (vbox) { 
		     std::string label;
		     for(int i=0; i< n_big_blobs; i++) {
			label = "Blob ";
			label += graphics_info_t::int_to_string(i + 1);
			GtkWidget *button = gtk_button_new_with_label(label.c_str());
			//	 gtk_widget_ref(button);
			clipper::Coord_orth *c = new clipper::Coord_orth;
			*c = lig.big_blobs()[i];
			gtk_signal_connect (GTK_OBJECT(button), "clicked", 
					    GTK_SIGNAL_FUNC(on_big_blob_button_clicked),
					    c);
			gtk_widget_show(button);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
			gtk_container_set_border_width(GTK_CONTAINER(button), 2);
		     }
		  }
		  gtk_widget_show(dialog);
	       } else { 
		  std::cout << "Coot found no blobs" << std::endl;
		  GtkWidget *dialog = create_ligand_no_blobs_dialog();
		  gtk_widget_show(dialog);
	       } 
	    }
	 }
      }
   }
}


// We need to look up the vboxes and add items, like we did in code in
// gtk-manual.c
// 
void fill_find_waters_dialog(GtkWidget *find_ligand_dialog) {

   int ifound; 
   short int diff_maps_only_flag = 0;
   ifound = fill_ligands_dialog_map_bits_by_dialog_name    (find_ligand_dialog,
							    "find_waters_map",
							    diff_maps_only_flag);
   if (ifound == 0) {
      std::cout << "Error: you must have a map to search for ligands!"
		<< std::endl;
   } 
   ifound = fill_ligands_dialog_protein_bits_by_dialog_name(find_ligand_dialog,
							    "find_waters_protein");
   if (ifound == 0) {
      std::cout << "Error: you must have a protein to mask the map!"
		<< std::endl;
   }

   GtkWidget *entry;
   entry = lookup_widget(find_ligand_dialog, "find_waters_peak_level_entry");

   char *txt = get_text_for_find_waters_sigma_cut_off();
   gtk_entry_set_text(GTK_ENTRY(entry), txt);
   free(txt);
   
}

char *get_text_for_find_waters_sigma_cut_off() {

   graphics_info_t g;
   char *text = (char *) malloc (100);
   snprintf(text, 99, "%5.1f", g.find_waters_sigma_cut_off);
   return text;

}

void set_value_for_find_waters_sigma_cut_off(float f) {
   graphics_info_t::find_waters_sigma_cut_off = f;
}

void set_ligand_water_spherical_variance_limit(float f) { // default 0.07, I think.

   graphics_info_t g;
   g.ligand_water_variance_limit = f;

}

void set_ligand_water_to_protein_distance_limits(float f1, float f2) {
   graphics_info_t g;
   g.ligand_water_to_protein_distance_lim_min = f1;
   g.ligand_water_to_protein_distance_lim_max = f2;
}

void set_ligand_water_n_cycles(int i) {

   graphics_info_t::ligand_water_n_cycles = i;
}

/* 0 off, 1 on */
void set_ligand_verbose_reporting(int i) {
   graphics_info_t::ligand_verbose_reporting_flag = i;
}


void
execute_find_waters(GtkWidget *dialog_ok_button) {

   // GtkWidget *widget = lookup_widget(dialog_ok_button, "find_waters_dialog");

   short int found_active_button_for_map = 0;
   short int found_active_button_for_protein = 0;
   int find_waters_map_mol = -1; // gets assigned
   int find_waters_protein_mol = -1; // gets assigned? Check me.
   graphics_info_t g;
   GtkWidget *dialog_button;

   // Find the active map radiobutton:
   for (int imol=0; imol<g.n_molecules; imol++) {
      if (g.molecules[imol].has_map()) {
	 std::string map_str = "find_waters_map_radiobutton_";
	 map_str += g.int_to_string(imol);
	 dialog_button = lookup_widget(dialog_ok_button, map_str.c_str());
	 if (dialog_button) {
	    if (GTK_TOGGLE_BUTTON(dialog_button)->active) {
	       find_waters_map_mol = imol;
	       found_active_button_for_map = 1;
	       break;
	    }
	 } else {
	    std::cout << map_str << " widget not found in execute_find_waters "
		      << std::endl;
	 }
      }
   }

   // Find the active protein radiobutton:
   for (int imol=0; imol<g.n_molecules; imol++) {
      if (g.molecules[imol].has_model()) {
	 std::string protein_str = "find_waters_protein_radiobutton_";
	 protein_str += g.int_to_string(imol);
	 dialog_button = lookup_widget(dialog_ok_button, protein_str.c_str());
	 if (dialog_button) {
	    if (GTK_TOGGLE_BUTTON(dialog_button)->active) {
	       find_waters_protein_mol = imol;
	       found_active_button_for_protein = 1;
	       break;
	    }
	 } else {
	    std::cout << protein_str << " widget not found in execute_find_waters "
		      << std::endl;
	 }
      }
   }

   // now read the entry containing the cut-off
   GtkWidget *entry = lookup_widget(dialog_ok_button,
				    "find_waters_peak_level_entry");
   const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
   float f = atof(text);
   if (f > 0.0 && f < 100.0) {
      std::cout << "finding peaks above " << f << " sigma " << std::endl;
   } else {
      f= 2.2;
      std::cout << "nonsense value for cut-off: using " << f
		<< " instead." << std::endl;
   }

   set_value_for_find_waters_sigma_cut_off(f); // save it for later
					       // display of the
					       // dialog

   // Should the waters be added to a new molecule or the masking molecule?
   //
   short int new_waters_mol_flag = 1; // 1 mean a new molecule,
				      // 0 means the masking molecule.
   
   GtkWidget *waters_toggle_button = lookup_widget(dialog_ok_button,
						   "water_mol_protein_mask_radiobutton");
   if (GTK_TOGGLE_BUTTON(waters_toggle_button)->active)
      new_waters_mol_flag = 0;
   waters_toggle_button = lookup_widget(dialog_ok_button,
					"water_mol_new_mol_radiobutton");
   if (GTK_TOGGLE_BUTTON(waters_toggle_button)->active)
      new_waters_mol_flag = 1;
   
   if (found_active_button_for_map &&
       found_active_button_for_protein) {
      execute_find_waters_real(find_waters_map_mol,
			       find_waters_protein_mol,
			       new_waters_mol_flag,
			       f);

      graphics_draw();
   } else {
      std::cout << "Something wrong in the selection of map/molecule"
		<< std::endl;
   } 
}

//    short int new_waters_mol_flag = 1; // 1 mean a new molecule,
// 				         // 0 means the masking molecule.
// 
void
execute_find_waters_real(int imol_for_map,
			 int imol_for_protein,
			 short int new_waters_mol_flag, 
			 float sigma_cut_off) {


   coot::ligand lig;
   graphics_info_t g;
   int n_cycles = g.ligand_water_n_cycles; // 3 by default

   // n_cycles = 1; // for debugging.

   short int mask_waters_flag; // treat waters like other atoms?
   // mask_waters_flag = g.find_ligand_mask_waters_flag;
   mask_waters_flag = 1; // when looking for waters we should not
			 // ignore the waters that already exist.
   // short int do_flood_flag = 0;    // don't flood fill the map with waters for now.

   lig.import_map_from(g.molecules[imol_for_map].xmap_list[0], 
		       g.molecules[imol_for_map].map_sigma());
   // lig.set_masked_map_value(-2.0); // sigma level of masked map gets distorted
   lig.set_map_atom_mask_radius(1.9); // Angstroms
   lig.set_water_to_protein_distance_limits(g.ligand_water_to_protein_distance_lim_max,
					    g.ligand_water_to_protein_distance_lim_min);
   lig.set_variance_limit(g.ligand_water_variance_limit);
   lig.mask_map(g.molecules[imol_for_protein].atom_sel.mol, mask_waters_flag);
   // lig.output_map("masked-for-waters.map");
   std::cout << "using sigma cut off " << sigma_cut_off << std::endl;
   if (g.ligand_water_write_peaksearched_atoms == 1) {
      std::cout << "DEBUG set_write_peaksearched_waters " << std::endl;
      lig.set_write_raw_waters();
   }
   // surely there is no need for this, in the general case?
   // lig.make_pseudo_atoms(); // put anisotropic atoms at the ligand sites

   std::cout << "Calling lig.water_fit()" << std::endl;
   lig.water_fit(sigma_cut_off, n_cycles);
   std::cout << "Done - back from lig.water_fit()" << std::endl;

   // water fit makes big blobs
   // *(g.ligand_big_blobs) = lig.big_blobs(); old style

   // It's just too painful to make this a c-interface.h function:
   if (lig.big_blobs().size() > 0) {

      GtkWidget *dialog = create_ligand_big_blob_dialog();
      GtkWidget *main_window = lookup_widget(graphics_info_t::glarea, "window1");
      gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
      GtkWidget *vbox = lookup_widget(dialog, "ligand_big_blob_vbox");
      if (vbox) { 
	 std::string label;
	 for(unsigned int i=0; i< lig.big_blobs().size(); i++) { 
	    label = "Blob ";
	    label += graphics_info_t::int_to_string(i + 1);
	    GtkWidget *button = gtk_button_new_with_label(label.c_str());
	    //	 gtk_widget_ref(button);
	    clipper::Coord_orth *c = new clipper::Coord_orth;
	    *c = lig.big_blobs()[i];
	    gtk_signal_connect (GTK_OBJECT(button), "clicked", 
				GTK_SIGNAL_FUNC(on_big_blob_button_clicked),
				c);
	    gtk_widget_show(button);
	    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	    gtk_container_set_border_width(GTK_CONTAINER(button), 2);
	 }
      }
      gtk_widget_show(dialog);
   }

   coot::minimol::molecule water_mol = lig.water_mol();
   std::cout << "DEBUG::  new_waters_mol_flag: " << new_waters_mol_flag << std::endl;
   if (new_waters_mol_flag) { 
      if (! water_mol.is_empty()) {
	 float bf = graphics_info_t::default_new_atoms_b_factor;
	 atom_selection_container_t asc = make_asc(water_mol.pcmmdbmanager(bf));
	 int g_mol_for_waters = g.n_molecules;
	 g.molecules[g_mol_for_waters].install_model(asc, "waters", 1);
	 g.n_molecules++;
	 if (g.go_to_atom_window){
	    g.update_go_to_atom_window_on_new_mol();
	    g.update_go_to_atom_window_on_changed_mol(g_mol_for_waters);
	 }
      }
   } else {
      // waters added to masking molecule
      g.molecules[imol_for_protein].insert_waters_into_molecule(water_mol);
      g.update_go_to_atom_window_on_changed_mol(imol_for_protein);
   }
}


void  free_blob_dialog_memory(GtkWidget *w) {

   // what are the buttons in this dialog?

}

   
// We don't do a malloc for this one, so this is not needed.  I think
// that there are others we we do and it is, but the memory is not
// freed.
void free_ligand_search_user_data(GtkWidget *button) {

} 


// void wrapped_create_big_blobs_dialog(const std::vector<Cartesian> &blobs) { 

//    GtkWidget *dialog = create_ligand_big_blob_dialog();
//    GtkWidget *vbox = lookup_widget(dialog, "ligand_big_blob_vbox");
//    if (vbox) { 
//       std::string label;
//       for(int i=0; i< blobs.size(); i++) { 
// 	 label = "Blob ";
// 	 label += int_to_string(i);
// 	 GtkWidget *button = gtk_button_new_with_label(label.c_str());
// 	 //	 gtk_widget_ref(button);
// 	 gtk_signal_connect (GTK_OBJECT(button), "clicked", 
// 			     GTK_SIGNAL_FUNC(on_big_blob_button_clicked),
// 			     GPOINTER_TO_INT(i));
// 	 gtk_widget_show(button);
// 	 gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
// 	 gtk_container_set_border_width(GTK_CONTAINER(button), 2);
//       }
//    }

//    gtk_widget_show(dialog);
// }

void
on_big_blob_button_clicked(GtkButton *button,
			   gpointer user_data) {

   graphics_info_t g;
   clipper::Coord_orth *p = (clipper::Coord_orth *) user_data;
   set_rotation_centre(p->x(), p->y(), p->z());
} 

// -------------------------------------------------------------------
// -------------------------------------------------------------------
//     Check waters by (anomalous?) difference map
// -------------------------------------------------------------------
// -------------------------------------------------------------------

GtkWidget *wrapped_create_check_waters_diff_map_dialog() { 

   GtkWidget *dialog = create_check_waters_diff_map_dialog();
   int ifound; 
   short int diff_maps_only_flag = 1;
   ifound = fill_ligands_dialog_map_bits_by_dialog_name(dialog, "check_waters_diff_map_map", 
							diff_maps_only_flag);
   if (ifound == 0) {
      std::cout << "Error: you must have a difference map to search for strange waters!"
		<< std::endl;
      GtkWidget *none_frame = lookup_widget(dialog, "no_difference_maps_frame");
      gtk_widget_show(none_frame);
   } 
   ifound = fill_ligands_dialog_protein_bits_by_dialog_name(dialog, "check_waters_diff_map_model");
   if (ifound == 0) {
      std::cout << "Error: you must have waters coordinates to analyse the map!"
		<< std::endl;
      GtkWidget *none_frame = lookup_widget(dialog,
					    "no_models_with_waters_frame");
      gtk_widget_show(none_frame);
   }
   return dialog;
}

void check_waters_by_difference_map_by_widget(GtkWidget *dialog) { 

   // short int interactive_flag = 1;
   int imol_diff_map = -1;
   int imol_coords = 1;

   // Find the first active map radiobutton
   GtkWidget *map_button;
   short int found_active_button_for_map = 0;
   for (int imol=0; imol<graphics_info_t::n_molecules; imol++) {
      if (graphics_info_t::molecules[imol].has_map()) { 
	 if (graphics_info_t::molecules[imol].is_difference_map_p()) { 
	    std::string map_str = "check_waters_diff_map_map_radiobutton_";
	    map_str += graphics_info_t::int_to_string(imol);
	    map_button = lookup_widget(dialog, map_str.c_str());
	    if (map_button) { 
	       if (GTK_TOGGLE_BUTTON(map_button)->active) {
		  imol_diff_map = imol;
		  found_active_button_for_map = 1;
		  break;
	       }
	    } else {
	       std::cout << "WARNING:: (error) " << map_str << " widget not found in "
			 << "check_waters_by_difference_map_by_widget" << std::endl;
	    }
	 }
      }
   }
   // Find the first active water radiobutton
   GtkWidget *water_button;
   short int found_active_button_for_waters = 0;
   for (int imol=0; imol<graphics_info_t::n_molecules; imol++) {
      if (graphics_info_t::molecules[imol].has_model()) { 
	 std::string water_str = "check_waters_diff_map_model_radiobutton_";
	 water_str += graphics_info_t::int_to_string(imol);
	 water_button = lookup_widget(dialog, water_str.c_str());
	 if (water_button) { 
	    if (GTK_TOGGLE_BUTTON(water_button)->active) {
	       imol_coords = imol;
	       found_active_button_for_waters = 1;
	       break;
	    }
	 } else {
	    std::cout << water_str << " widget not found in "
		      << "check_waters_by_difference_map_by_widget" << std::endl;

	 }
      }
   }

   if (! found_active_button_for_map) { 
      std::cout << "INFO:: You need to have define a map to search for waters!\n";
   } else { 
      if (! found_active_button_for_waters) { 
	 std::cout << "INFO:: You need to have define coordinates for map analysis!\n";
      } else { 
	 short int interactive_flag = 1;
	 check_waters_by_difference_map(imol_coords, imol_diff_map, interactive_flag);
      }
   }
}


GtkWidget *wrapped_nothing_bad_dialog(const std::string &label) { 
   
   graphics_info_t g;
   return g.wrapped_nothing_bad_dialog(label);
} 


/* Let's give access to the sigma level (default 4) */
float check_waters_by_difference_map_sigma_level_state() { 
   return graphics_info_t::check_waters_by_difference_map_sigma_level;
} 

void set_check_waters_by_difference_map_sigma_level(float f) { 
   graphics_info_t::check_waters_by_difference_map_sigma_level = f;
}


void renumber_waters(int imol) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].renumber_waters();
      graphics_draw();
      if (graphics_info_t::go_to_atom_window) {
	 update_go_to_atom_window_on_changed_mol(imol);
      }
   }
} 
