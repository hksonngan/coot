/* src/c-interface-waters-gui.cc
 * 
 * Copyright 2004, 2005 by The University of York
 * Author: Paul Emsley
 * Copyright 2008, 2010 by The University of Oxford
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
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#include "compat/coot-sysdep.h"

#include <stdlib.h>
#include <iostream>

#if defined _MSC_VER
#include <windows.h>
#define snprintf _snprintf
#endif
 
#include "globjects.h" //includes gtk/gtk.h

#include "callbacks.h"
#include "interface.h" // now that we are moving callback
		       // functionality to the file, we need this
		       // header since some of the callbacks call
		       // fuctions built by glade.

#include <vector>
#include <string>

#include <mmdb2/mmdb_manager.h>
#include "coords/mmdb-extras.h"
#include "coords/mmdb.h"
#include "coords/mmdb-crystal.h"


#include "graphics-info.h"

#ifdef USE_GUILE
#include <libguile.h>
#endif // USE_GUILE

// Including python needs to come after graphics-info.h, because
// something in Python.h (2.4 - chihiro) is redefining FF1 (in
// ssm_superpose.h) to be 0x00004000 (Grrr).
//
// 20100813: Python.h needs to come before to stop"_POSIX_C_SOURCE" redefined problems 
//
// #ifdef USE_PYTHON
// #include "Python.h"
// #endif // USE_PYTHON

#include "c-interface.h"

#include "ligand/wligand.hh"




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
      for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) {
	 if (graphics_info_t::molecules[imol].has_xmap()) { 
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
      for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) {
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

   if (is_valid_model_molecule(imol_model)) { 
      if (is_valid_map_molecule(imol_for_map)) { 

	 coot::ligand lig;
	 graphics_info_t g;
	 int n_cycles = 1;

	 short int mask_waters_flag; // treat waters like other atoms?
	 mask_waters_flag = g.find_ligand_mask_waters_flag;
	 // short int do_flood_flag = 0;    // don't flood fill the map with waters for now.

	 lig.import_map_from(g.molecules[imol_for_map].xmap, 
			     g.molecules[imol_for_map].map_sigma());
	 // lig.set_masked_map_value(-2.0); // sigma level of masked map gets distorted
	 lig.set_map_atom_mask_radius(1.9); // Angstrom
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
		     *c = lig.big_blobs()[i].first;
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


// function to show find waters (from wherever)
void
wrapped_create_find_waters_dialog() {

   GtkWidget *widget;
   widget = create_find_waters_dialog();
   fill_find_waters_dialog(widget);
   gtk_widget_show(widget);
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

   // Now deal with the (new) entries for the distances to the protein
   // (if they exist (not (yet?) in gtk1 version)).
   //
   GtkWidget *wd1 = lookup_widget(GTK_WIDGET(find_ligand_dialog),
				  "find_waters_max_dist_to_protein_entry");
   GtkWidget *wd2 = lookup_widget(GTK_WIDGET(find_ligand_dialog),
				  "find_waters_min_dist_to_protein_entry");

   if (wd1 && wd2) {
      float max = graphics_info_t::ligand_water_to_protein_distance_lim_max;
      float min = graphics_info_t::ligand_water_to_protein_distance_lim_min;
      gtk_entry_set_text(GTK_ENTRY(wd1), coot::util::float_to_string(max).c_str());
      gtk_entry_set_text(GTK_ENTRY(wd2), coot::util::float_to_string(min).c_str());
   }
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
   for (int imol=0; imol<g.n_molecules(); imol++) {
      if (g.molecules[imol].has_xmap()) {
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
   for (int imol=0; imol<g.n_molecules(); imol++) {
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


   // 20090201
   //
   // Now deal with the (new) entries for the distances to the protein
   // (if they exist (not (yet?) in gtk1 version)).
   //
   GtkWidget *wd1 = lookup_widget(GTK_WIDGET(dialog_ok_button),
				  "find_waters_max_dist_to_protein_entry");
   GtkWidget *wd2 = lookup_widget(GTK_WIDGET(dialog_ok_button),
				  "find_waters_min_dist_to_protein_entry");

   if (wd1 && wd2) {
      const gchar *t1 = gtk_entry_get_text(GTK_ENTRY(wd1));
      const gchar *t2 = gtk_entry_get_text(GTK_ENTRY(wd2));
      float f1 = atof(t1);
      float f2 = atof(t2);
      // slam in the distances to the static vars directly (not as
      // arguments to find_waters_real()).
      g.ligand_water_to_protein_distance_lim_max = f1;      
      g.ligand_water_to_protein_distance_lim_min = f2;
   } 



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


// fire up a waters dialog.  
// 
void find_waters(int imol_for_map,
		 int imol_for_protein,
		 short int new_waters_mol_flag, 
		 float sigma_cut_off,
		 short int show_blobs_dialog) {

   if (!is_valid_model_molecule(imol_for_protein)) {
      std::cout << "WARNING:: in find_waters " << imol_for_protein
		<< " is not a valid model" << std::endl;
   } else { 
      if (!is_valid_map_molecule(imol_for_map)) {
	 std::cout << "WARNING:: in find_waters " << imol_for_map
		   << " is not a valid map" << std::endl;
      } else { 
	 coot::ligand lig;
	 graphics_info_t g;
	 int n_cycles = g.ligand_water_n_cycles; // 3 by default

	 // n_cycles = 1; // for debugging.

	 short int mask_waters_flag; // treat waters like other atoms?
	 // mask_waters_flag = g.find_ligand_mask_waters_flag;
	 mask_waters_flag = 1; // when looking for waters we should not
	 // ignore the waters that already exist.
	 // short int do_flood_flag = 0;    // don't flood fill the map with waters for now.

	 lig.import_map_from(g.molecules[imol_for_map].xmap, 
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

	 if (graphics_info_t::use_graphics_interface_flag) { 
	    if (show_blobs_dialog) { 
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
			*c = lig.big_blobs()[i].first;
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
	    }
	 }

	 coot::minimol::molecule water_mol = lig.water_mol();
	 std::cout << "DEBUG::  new_waters_mol_flag: " << new_waters_mol_flag << std::endl;
	 if (new_waters_mol_flag) { 
	    if (! water_mol.is_empty()) {
	       float bf = graphics_info_t::default_new_atoms_b_factor;
	       atom_selection_container_t asc = make_asc(water_mol.pcmmdbmanager());
	       // We need to make the atoms in asc HETATMs
	       for (int iat=0; iat<asc.n_selected_atoms; iat++)
		  asc.atom_selection[iat]->Het = 1;
	       int g_mol_for_waters = graphics_info_t::create_molecule();
	       g.molecules[g_mol_for_waters].install_model(g_mol_for_waters, asc, g.Geom_p(), "waters", 1);
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

   clipper::Coord_orth *p = (clipper::Coord_orth *) user_data;
   set_rotation_centre(p->x(), p->y(), p->z());
} 

