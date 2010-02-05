/* src/graphics-info.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 by the University of York
 * Copyright 2007, 2008, 2009 by the University of Oxford
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


#ifndef HAVE_STRING
#define HAVE_STRING
#include <string>
#endif

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif

#include <gtk/gtk.h>  // must come after mmdb_manager on MacOS X Darwin
#include <GL/glut.h>  // for some reason...  // Eh?

#include <iostream>
#include <dirent.h>   // for refmac dictionary files

#include <sys/types.h> // for stating
#include <sys/stat.h>

#if !defined _MSC_VER && !defined WINDOWS_MINGW
#include <unistd.h>
#else
#include "coot-sysdep.h"
#endif

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "Cartesian.h"
#include "Bond_lines.h"

#include "clipper/core/map_utils.h" // Map_stats
#include "graphical_skel.h"

#include "coot-sysdep.h"

#include "interface.h"

#include "molecule-class-info.h"
#include "BuildCas.h"

#include "gl-matrix.h" // for baton rotation
#include "trackball.h" // for baton rotation

#include "bfkurt.hh"

#include "globjects.h"
#include "ligand.hh"
#include "graphics-info.h"

#include "dunbrack.hh"

#include "coot-utils.hh"

//temp
#include "cmtz-interface.hh"

#include "manipulation-modes.hh"
#include "guile-fixups.h"



// e.g. fit type is "Rigid Body Fit" or "Regularization" etc.
//
// if fit_type is "Torsion General" show the Reverse button.
// 
void do_accept_reject_dialog(std::string fit_type, const coot::refinement_results_t &rr) {

   GtkWidget *window = wrapped_create_accept_reject_refinement_dialog();
   GtkWindow *main_window = GTK_WINDOW(lookup_widget(graphics_info_t::glarea, 
						     "window1"));
   GtkWidget *label;
   if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG_DOCKED){
      label = lookup_widget(GTK_WIDGET(window),
			    "accept_dialog_accept_docked_label_string");
   } else {
      label = lookup_widget(GTK_WIDGET(window),
			    "accept_dialog_accept_label_string");
      gtk_window_set_transient_for(GTK_WINDOW(window), main_window);
      
      // now set the position, if it was set:
      if ((graphics_info_t::accept_reject_dialog_x_position > -100) && 
	  (graphics_info_t::accept_reject_dialog_y_position > -100)) {
	 gtk_widget_set_uposition(window,
				  graphics_info_t::accept_reject_dialog_x_position,
				  graphics_info_t::accept_reject_dialog_y_position);
      }
   }

   update_accept_reject_dialog_with_results(window, coot::CHI_SQUAREDS, rr);
   if (rr.lights.size() > 0){
     if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG_DOCKED){
       add_accept_reject_lights(GTK_WIDGET(main_window), rr);
     } else {
       add_accept_reject_lights(window, rr);
     }
   }
   
   std::string txt = "";
   txt += "Accept ";
   txt += fit_type;
   txt += "?";

   gtk_label_set_text(GTK_LABEL(label), txt.c_str());

   // Was this a torsion general, in which we need to active the reverse button?
   GtkWidget *reverse_button;
   if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG_DOCKED) {
     reverse_button = lookup_widget(window, "accept_reject_docked_reverse_button");
     if (fit_type == "Torsion General") {
       gtk_widget_show(reverse_button);	
     } else {
       gtk_widget_hide(reverse_button);
     }
   } else {
     if (fit_type == "Torsion General") {
       reverse_button = lookup_widget(window, "accept_reject_reverse_button");
       gtk_widget_show(reverse_button);	
     }
   }
   
   if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG_DOCKED){
     // we need to show some individual widget to make sure we get the right amount
     // of light boxes
     GtkWidget *button_box = lookup_widget(GTK_WIDGET(main_window), "hbuttonbox1");
     gtk_widget_show_all(button_box);
     gtk_widget_show(label);
     if (graphics_info_t::accept_reject_dialog_docked_show_flag == coot::DIALOG_DOCKED_SHOW) {
       gtk_widget_set_sensitive(window, TRUE);
     }
   }
   gtk_widget_show(window);
}

void add_accept_reject_lights(GtkWidget *window, const coot::refinement_results_t &ref_results) {

   std::vector<std::pair<std::string, std::string> > boxes;
   boxes.push_back(std::pair<std::string, std::string>("Bonds",                    "bonds_"));
   boxes.push_back(std::pair<std::string, std::string>("Angles",                  "angles_"));
   boxes.push_back(std::pair<std::string, std::string>("Torsions",              "torsions_"));
   boxes.push_back(std::pair<std::string, std::string>("Planes",                  "planes_"));
   boxes.push_back(std::pair<std::string, std::string>("Chirals",                "chirals_"));
   boxes.push_back(std::pair<std::string, std::string>("Non-bonded", "non_bonded_contacts_"));
   boxes.push_back(std::pair<std::string, std::string>("Rama",                      "rama_"));
 
   GtkWidget *frame;
   if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG_DOCKED) {
     frame = lookup_widget(window, "accept_reject_lights_frame_docked");
   } else {
     frame = lookup_widget(window, "accept_reject_lights_frame");
   }
   gtk_widget_show(frame);

   for (unsigned int i_rest_type=0; i_rest_type<ref_results.lights.size(); i_rest_type++) {
      // std::cout << "Lights for " << ref_results.lights[i_rest_type].second << std::endl;
      for (unsigned int ibox=0; ibox<boxes.size(); ibox++) {
	 if (ref_results.lights[i_rest_type].name == boxes[ibox].first) {
	    std::string stub = boxes[ibox].second.c_str();
	    std::string event_box_name;
	    if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG_DOCKED) {
	      event_box_name = stub + "eventbox_docked";
		} else {
	      event_box_name = stub + "eventbox";
	    }
	    GtkWidget *w = lookup_widget(frame, event_box_name.c_str());
	    if (w) { 
	       GtkWidget *p = w->parent;
	       if (boxes[ibox].first != "Rama") { 
		  GdkColor color = colour_by_distortion(ref_results.lights[i_rest_type].value);
		  set_colour_accept_reject_event_box(w, &color);
	       } else {
		  GdkColor color = colour_by_rama_plot_distortion(ref_results.lights[i_rest_type].value);
		  set_colour_accept_reject_event_box(w, &color);
	       }
#if (GTK_MAJOR_VERSION > 1)	       
	       gtk_widget_show(p); // event boxes don't get coloured
				   // in GTK1 version - no need to
				   // show them then.
#endif	       
	    } else {
	       std::cout << "ERROR:: lookup of event_box_name: " << event_box_name
			 << " failed" << std::endl;
	    } 

	    // we do not add labels for the docked box
	    if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG) {
	    
	       std::string label_name = stub + "label";
	       GtkWidget *label = lookup_widget(frame, label_name.c_str());
	       gtk_label_set_text(GTK_LABEL(label), ref_results.lights[i_rest_type].label.c_str());
	       gtk_widget_show(label);
	    } else {
	       GtkTooltips *tooltips;
	       tooltips = GTK_TOOLTIPS(lookup_widget(window, "tooltips"));
	       std::string tips_info = ref_results.lights[i_rest_type].label;
	       gtk_tooltips_set_tip(tooltips, w, tips_info.c_str(), NULL);
	    }
	 }
      }
   }
}

// Actually, it seems that this does not do anything for GTK == 1. So
// the function that calls it is not compiled (for Gtk1).
// 
void set_colour_accept_reject_event_box(GtkWidget *eventbox, GdkColor *col) {

#if (GTK_MAJOR_VERSION == 1)    
  GtkRcStyle *rc_style = gtk_rc_style_new ();
  rc_style->fg[GTK_STATE_NORMAL] = *col;
  // rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_FG; // compiler failure, try...
  GtkRcFlags new_colour_flags = GtkRcFlags(GTK_RC_FG | rc_style->color_flags[GTK_STATE_NORMAL]);
  rc_style->color_flags[GTK_STATE_NORMAL] = new_colour_flags;
  gtk_widget_modify_style (eventbox, rc_style);
#else    
   gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL, col);
#endif
   
}

void
update_accept_reject_dialog_with_results(GtkWidget *accept_reject_dialog,
					 coot::accept_reject_text_type text_type,
					 const coot::refinement_results_t &rr) {

    std::string extra_text = rr.info;
    if (extra_text != "") {
      if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG){
      
	// now look up the label in window and change it.
	GtkWidget *extra_label = lookup_widget(GTK_WIDGET(accept_reject_dialog),
					       "extra_text_label");
      
	if (text_type == coot::CHIRAL_CENTRES)
	  extra_label = lookup_widget(GTK_WIDGET(accept_reject_dialog), "chiral_centre_text_label");
      
	gtk_label_set_text(GTK_LABEL(extra_label), extra_text.c_str());
      } else {

	// we have a docked accept/reject dialog
	GtkWidget *window = lookup_widget(accept_reject_dialog, "window1");
	GtkTooltips *tooltips;
	GtkTooltipsData *td;
	tooltips = GTK_TOOLTIPS(lookup_widget(GTK_WIDGET(window), "tooltips"));

	int cis_pep_warn = extra_text.find("CIS");
	int chirals_warn = extra_text.find("chiral");

	GtkWidget *cis_eventbox = lookup_widget(GTK_WIDGET(accept_reject_dialog),
						       "cis_peptides_eventbox_docked");
	GtkWidget *p = cis_eventbox->parent;
	GtkWidget *chirals_eventbox = lookup_widget(GTK_WIDGET(accept_reject_dialog),"chirals_eventbox_docked");

	td = gtk_tooltips_data_get(chirals_eventbox);
	std::string old_tip = td->tip_text;

	std::string tips_info_chirals;
        std::string tips_info_cis;
	
	if (cis_pep_warn > -1) {
	  // we have extra cis peptides

	  if (chirals_warn > -1) {
	    // remove the chirals warn text
	    string::size_type start_warn = extra_text.find("WARN");
	    string::size_type end_warn = extra_text.size();
	    tips_info_cis = extra_text.substr(start_warn, end_warn); // list the extra cis peptides here?
	  } else {
	    tips_info_cis = extra_text; // list the extra cis peptides?
	  }

	  gtk_tooltips_set_tip(tooltips, cis_eventbox, tips_info_cis.c_str(), NULL);
	  GdkColor red = colour_by_distortion(1000.0);	// red
	  set_colour_accept_reject_event_box(cis_eventbox, &red);
	  gtk_widget_show(p);

	}
	if (chirals_warn > -1) {
	  // we may have some extra chiral text

	  if (cis_pep_warn > -1) {
	    // remove the cis warn text
	    string::size_type start_warn = extra_text.find("WARN");
	    tips_info_chirals = extra_text.substr(0,start_warn) + old_tip;
	  } else {
	    tips_info_chirals = extra_text + old_tip;
	  }

	  gtk_tooltips_set_tip(tooltips, chirals_eventbox, tips_info_chirals.c_str(), NULL);

	}
	if (extra_text == " "){
	// reset the tips
	  gtk_widget_hide(p);
	  gtk_tooltips_set_tip(tooltips, chirals_eventbox, old_tip.c_str(), NULL);
	}
      }		   
    }
    if (rr.lights.size() > 0)
      add_accept_reject_lights(accept_reject_dialog, rr);
}


GtkWidget *
wrapped_create_accept_reject_refinement_dialog() {

  GtkWidget *w;
  if (graphics_info_t::accept_reject_dialog_docked_flag == coot::DIALOG_DOCKED){
    w = lookup_widget(GTK_WIDGET(graphics_info_t::glarea), "accept_reject_dialog_frame_docked");
  } else {
    w = create_accept_reject_refinement_dialog();
  }
  graphics_info_t::accept_reject_dialog = w;
  return w;
 }

// static
GtkWidget *
graphics_info_t::info_dialog(const std::string &s) {
   
   GtkWidget *w = NULL;
   if (graphics_info_t::use_graphics_interface_flag) { 
      w = wrapped_nothing_bad_dialog(s);
      gtk_widget_show(w);
   }
   return w;
}

// This uses an info_dialog but uses gtk_label_set_markup() to display
// in a monospace font.
void
graphics_info_t::info_dialog_alignment(coot::chain_mutation_info_container_t mutation_info) const {

   std::string s;

   s = "<tt>";
   s += ": ";
   s += mutation_info.alignedS_label;
   if (mutation_info.chain_id != "") { 
      s += " Chain ";
      s += mutation_info.chain_id;
   }
   s += "\n";
   s += mutation_info.alignedS;
   s += "\n";
   s += ": ";
   s += mutation_info.alignedT_label;
   s += "\n";
   s += mutation_info.alignedT;
   s += "\n";
   // s += "something_here";
   s += "</tt>";

   GtkWidget *dialog = info_dialog(s); // get trashed by markup text
   if (dialog) { 
      GtkWidget *label = lookup_widget(dialog, "nothing_bad_label");
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

      // guessing that we need > 6, could be more than 6.
#if ( ( (GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION > 6) ) || GTK_MAJOR_VERSION > 2)
      gtk_label_set_markup(GTK_LABEL(label), s.c_str());
#endif       
   }
}

void
graphics_info_t::info_dialog_refinement_non_matching_atoms(std::vector<std::pair<std::string, std::vector<std::string> > > nma) {

   std::string s = "   Failed to match (to the dictionary) the following model atom names:\n";
   for (unsigned int i=0; i<nma.size(); i++) {
      s += "   ";
      s += nma[i].first;
      s += "\n   ";
      for (unsigned int j=0; j<nma[i].second.size(); j++) {
	 s += "\"";
	 s += nma[i].second[j];
	 s += "\"   ";
      }
      s += "\n";
      s += "\n";
      s += "   That would cause exploding atoms, so the refinement didn't start\n";
   }

   GtkWidget *dialog = info_dialog(s); // get trashed by markup text
   if (dialog) { 
      GtkWidget *label = lookup_widget(dialog, "nothing_bad_label");
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
   }
}



// To be used to (typically) get the menu item text label from chain
// option menus (rather than the ugly/broken casting of
// GtkPositionType data.  
// static
std::string
graphics_info_t::menu_item_label(GtkWidget *menu_item) {
   
   gchar *text = 0;
   if (GTK_BIN (menu_item)->child) {
      GtkWidget *child = GTK_BIN (menu_item)->child;
      if (GTK_IS_LABEL (child)) {
	 gtk_label_get (GTK_LABEL (child), &text);
	 // g_print ("menu item text: %s\n", text);
      }
   }

   if (text) {
      std::string s(text);
      return s;
   } else { 
      return std::string("");
   }
}



// static 
void
graphics_info_t::store_window_position(int window_type, GtkWidget *widget) {

   // I am not sure that this function is worth anything now.  It will
   // only be useful for dialog destroys that happen from within
   // graphics-info class... i.e. not many.  Most destroys happen from
   // callbacks.c and thus will use the c-interface.cc's version of
   // this function.
   //
   // If you find yourself in future wanting to add stuff here,
   // instead just move the body of the c-interface.cc's function here
   // and make the c-interface.cc just a one line which calls this
   // function.
   
   gint upositionx, upositiony;
   gdk_window_get_root_origin (widget->window, &upositionx, &upositiony);

   if (window_type == COOT_EDIT_CHI_DIALOG) {
      graphics_info_t::edit_chi_angles_dialog_x_position = upositionx;
      graphics_info_t::edit_chi_angles_dialog_y_position = upositiony;
   }

   if (window_type == COOT_ROTAMER_SELECTION_DIALOG) {
      graphics_info_t::rotamer_selection_dialog_x_position = upositionx;
      graphics_info_t::rotamer_selection_dialog_y_position = upositiony;
   }
}

// static 
void
graphics_info_t::set_transient_and_position(int widget_type, GtkWidget *window) {

   GtkWindow *main_window =
      GTK_WINDOW(lookup_widget(graphics_info_t::glarea, "window1"));
   gtk_window_set_transient_for(GTK_WINDOW(window), main_window);
   
   if (widget_type == COOT_EDIT_CHI_DIALOG) {
//       std::cout << "set_transient_and_position 2" 
// 		<< graphics_info_t::edit_chi_angles_dialog_x_position << "\n";
	 
      if (graphics_info_t::edit_chi_angles_dialog_x_position > -100) {
	 if (graphics_info_t::edit_chi_angles_dialog_y_position > -100) {
	    gtk_widget_set_uposition(window,
				     graphics_info_t::edit_chi_angles_dialog_x_position,
				     graphics_info_t::edit_chi_angles_dialog_y_position);
	 }
      }
   }

   if (widget_type == COOT_ROTAMER_SELECTION_DIALOG) {
      if (graphics_info_t::rotamer_selection_dialog_x_position > -100) {
	 if (graphics_info_t::rotamer_selection_dialog_y_position > -100) {
	    gtk_widget_set_uposition(window,
				     graphics_info_t::rotamer_selection_dialog_x_position,
				     graphics_info_t::rotamer_selection_dialog_y_position);
	 }
      }
   }


}


void
graphics_info_t::statusbar_text(const std::string &text) const {

   if (use_graphics_interface_flag) { 
      if (statusbar) {
	 std::string sbt = text;
	 // If it is "too long" chop it down.
	 unsigned int max_width = 130;
	 GtkWidget *main_window = lookup_widget(glarea, "window1");
	 // some conversion between the window width and the max text length
	 max_width = main_window->allocation.width/4 -38;
	 if (sbt.length() > max_width) { // some number
	    // -------------------------
	    //        |                |  
	    //     200-130            200
	    int l = sbt.length();
	    std::string short_text = text.substr(l-max_width, max_width);
	    // std::cout << "short_text length: " << short_text.length() << std::endl;
	    sbt = "..." + short_text;
	 }
	 gtk_statusbar_push(GTK_STATUSBAR(statusbar),
			    statusbar_context_id,
			    sbt.c_str());
      }
   }
}


void
graphics_info_t::save_directory_from_fileselection(const GtkWidget *fileselection) {

   const gchar *filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileselection));
   directory_for_fileselection = coot::util::file_name_directory(filename);
   // std::cout << "saved directory name: " << directory_for_fileselection << std::endl;
}

void
graphics_info_t::save_directory_for_saving_from_fileselection(const GtkWidget *fileselection) {

   const gchar *filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileselection));
   directory_for_saving_for_fileselection = coot::util::file_name_directory(filename);
   // std::cout << "saved directory name: " << directory_for_fileselection << std::endl;
}


void
graphics_info_t::set_directory_for_fileselection_string(std::string filename) {

   directory_for_fileselection = filename;
}

void
graphics_info_t::set_directory_for_fileselection(GtkWidget *fileselection) const {

#if (GTK_MAJOR_VERSION > 1)
  if (graphics_info_t::gtk2_file_chooser_selector_flag == coot::CHOOSER_STYLE) {
    set_directory_for_filechooser(fileselection);
  } else {
    if (directory_for_fileselection != "") {
//       std::cout << "set directory_for_fileselection "
// 		<< directory_for_fileselection << std::endl;
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileselection),
				      directory_for_fileselection.c_str());
    } else {
      // std::cout << "not setting directory_for_fileselection" << std::endl;
    }
  }
#else
   if (directory_for_fileselection != "") {
//       std::cout << "set directory_for_fileselection "
// 		<< directory_for_fileselection << std::endl;
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileselection),
				      directory_for_fileselection.c_str());
   } else {
      // std::cout << "not setting directory_for_fileselection" << std::endl;
   } 
#endif // GTK_MAJOR_VERSION
}

void 
graphics_info_t::set_file_for_save_fileselection(GtkWidget *fileselection) const { 

   // just like set_directory_for_fileselection actually, but we give
   // it the full filename, not just the directory.

   int imol = save_imol;
   if (imol >= 0 && imol < graphics_info_t::n_molecules()) { 
      std::string stripped_name = 
	 graphics_info_t::molecules[imol].stripped_save_name_suggestion();
      std::string full_name = stripped_name;
      //       if (graphics_info_t::save_coordinates_in_original_dir_flag != 0) 
      if (graphics_info_t::directory_for_saving_for_fileselection != "")
	 full_name = directory_for_saving_for_fileselection + stripped_name;

      std::cout << "INFO:: Setting fileselection with file: " << full_name
		<< std::endl;
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(fileselection),
				      full_name.c_str());
   }
}

#if (GTK_MAJOR_VERSION > 1)
void
graphics_info_t::save_directory_from_filechooser(const GtkWidget *fileselection) {

   gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileselection));
   directory_for_filechooser = coot::util::file_name_directory(filename);
   // std::cout << "saved directory name: " << directory_for_fileselection << std::endl;
   std::cout << "saved directory name: " << directory_for_filechooser << std::endl;
   g_free(filename);
}

void
graphics_info_t::save_directory_for_saving_from_filechooser(const GtkWidget *fileselection) {

   gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fileselection));
   directory_for_saving_for_filechooser = coot::util::file_name_directory(filename);
   // std::cout << "saved directory name: " << directory_for_fileselection << std::endl;
   std::cout << "saved directory name and filename: " << directory_for_filechooser <<" "<<filename << std::endl;
   g_free(filename);
}

void
graphics_info_t::set_directory_for_filechooser_string(std::string filename) {

   directory_for_filechooser = filename;
}

void
graphics_info_t::set_directory_for_filechooser(GtkWidget *fileselection) const {

   if (directory_for_filechooser != "") {
       std::cout << "set directory_for_filechooser "
                 << directory_for_filechooser << std::endl;
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileselection),
                                      directory_for_filechooser.c_str());
   } else {
      // std::cout << "not setting directory_for_fileselection" << std::endl;
   }

}

void
graphics_info_t::set_file_for_save_filechooser(GtkWidget *fileselection) const {
   // just like set_directory_for_filechooser actually, but we give
   // it the full filename, not just the directory.

   int imol = save_imol;
   if (imol >= 0 && imol < graphics_info_t::n_molecules()) {
      std::string stripped_name =
         graphics_info_t::molecules[imol].stripped_save_name_suggestion();
      std::string full_name = stripped_name;

      if (graphics_info_t::directory_for_saving_for_filechooser != "") {
	full_name = directory_for_saving_for_filechooser + stripped_name;
      } else {
	// if we have a directory in the fileselection path we take this
	if (graphics_info_t::directory_for_saving_for_fileselection != "") {
	  directory_for_saving_for_filechooser = graphics_info_t::directory_for_saving_for_fileselection;
	  
	} else {
	  // otherwise we make one
	  gchar *current_dir = g_get_current_dir();
	  full_name = g_build_filename(current_dir, stripped_name.c_str(), NULL);
	  directory_for_saving_for_filechooser = current_dir;
	  g_free(current_dir);
	}
      }

      std::cout << "INFO:: Setting fileselection with file: " << full_name
                << std::endl;
      if (g_file_test(full_name.c_str(), G_FILE_TEST_EXISTS)) {
         gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fileselection),
                                      full_name.c_str());
	 // we shouldnt need to call set_current_name and the filename
	 // should be automatically set in the entry field, but this seems
	 // to be buggy at least on gtk+-2.0 v. 1.20.13
	 gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fileselection),
					   stripped_name.c_str());
      } else {
         gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileselection),
                                      directory_for_saving_for_filechooser.c_str());
         gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fileselection),
                                      stripped_name.c_str());
      }
   }
}
#endif // GTK_MAJOR_VERSION


// I find this somewhat asthetically pleasing (maybe because the
// display control widgets are uniquely named [which was a bit of a
// struggle in C, I seem to recall]).
// 
// static
void
graphics_info_t::activate_scroll_radio_button_in_display_manager(int imol) {

   graphics_info_t g;
   if (g.display_control_window()) {
      std::string wname = "map_scroll_button_";
      wname += graphics_info_t::int_to_string(imol);
      GtkWidget *w = lookup_widget(g.display_control_window(), wname.c_str());
      if (w) {
	 if (g.scroll_wheel_map == imol) {
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	 }
      }
   }
}



void
graphics_info_t::show_select_map_dialog() {

   if (use_graphics_interface_flag) {

      GtkWidget *widget = create_select_fitting_map_dialog();
      GtkWidget *optionmenu = lookup_widget(GTK_WIDGET(widget),
					    "select_map_for_fitting_optionmenu");
      
      int imol_map = Imol_Refinement_Map();
      
      // If no map has been set before, set the map to the top of the
      // list (if there are maps in the list)
      // 
      if (imol_map == -1) { 
	 for (int imol=0; imol<n_molecules(); imol++) { 
	    if (molecules[imol].has_map()) {
	       imol_refinement_map = imol;
	       break;
	    }
	 }
      }
      // note that this uses one of 2 similarly named function:
      fill_option_menu_with_map_options(optionmenu,
					GTK_SIGNAL_FUNC(graphics_info_t::refinement_map_select),
					imol_refinement_map);
      
      
      // Old notes:
      // now activate the first menu item, i.e. creating this menu is as
      // if the first item in the menu was activated:
      //    for (int i=0; i<n_molecules; i++) {
      //       if (molecules[i].xmap_is_filled[0] == 1) {
      // 	 set_refinement_map(i);
      // 	 break;
      //       }
      //    }
      //
      // 10/6/2004: Well, that is in fact totally crap.  What the *user*
      // expects is that when they open this dialog that the default map
      // (the one they have previously chosen) will be in the active
      // position. i.e. the mechanism described above was totaly wrong.
      //
      // What we really want to do is call
      // fill_option_menu_with_map_options and pass it the active item
      // as an argument.
      //
      gtk_widget_show(widget);
   } else {
      std::cout << "No graphics!  Can't make Map Selection dialog.\n";
   } 
   
}



GtkWidget *
graphics_info_t::wrapped_create_skeleton_dialog(bool show_ca_mode_needs_skel_label) { 

   GtkWidget *w = create_skeleton_dialog();
   GtkWidget *option_menu = lookup_widget(w, "skeleton_map_optionmenu");
   GtkWidget *frame = lookup_widget(w, "skeleton_dialog_on_off_frame");
   GtkWidget *label = lookup_widget(w, "ca_baton_mode_needs_skel_label");
   GtkWidget *ok_button = lookup_widget(w, "skeleton_ok_button");

   // add user data to the OK button, we use it to determine if we go
   // on to display the baton dialog
   int show_baton_dialog = 0;
   
   if (show_ca_mode_needs_skel_label) {
      show_baton_dialog = 1;
   }

   gtk_signal_connect(GTK_OBJECT(ok_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (on_skeleton_ok_button_dynamic_clicked),
		      GINT_TO_POINTER(show_baton_dialog));
		      
   
   if (show_ca_mode_needs_skel_label) {
      gtk_widget_show(label);
   } 
   set_initial_map_for_skeletonize();
   fill_option_menu_with_skeleton_options(option_menu);
   set_on_off_skeleton_radio_buttons(frame);
   return w;
}


// static
void
graphics_info_t::on_skeleton_ok_button_dynamic_clicked (GtkButton       *button,
							gpointer         user_data)
{
  GtkWidget *window = lookup_widget(GTK_WIDGET(button),
				    "skeleton_dialog");
  GtkWidget *optionmenu = lookup_widget(window, "skeleton_map_optionmenu");
  int do_baton_mode = GPOINTER_TO_INT(user_data);

  std::cout << "do_baton_mode:: " << do_baton_mode << std::endl;

  graphics_info_t g;
  g.skeletonize_map_by_optionmenu(optionmenu);
  gtk_widget_destroy(window);
  if (do_baton_mode) {
     int state = g.try_set_draw_baton(1);
     if (state) {
	GtkWidget *w = create_baton_dialog();
	gtk_widget_show(w);
     } 
  }

}

void
graphics_info_t::skeletonize_map_by_optionmenu(GtkWidget *optionmenu) { 

   GtkWidget *window = lookup_widget(GTK_WIDGET(optionmenu), "skeleton_dialog");

   GtkWidget *on_radio_button;
   GtkWidget *prune_check_button;

   on_radio_button = lookup_widget(window, "skeleton_on_radiobutton");

   short int do_it = 0; 
   short int prune_it = 0;
   if (! is_valid_map_molecule(graphics_info_t::map_for_skeletonize)) {
      std::cout << "ERROR:: Trapped a bad map for skeletoning!" << std::endl;
   } else {
      if (GTK_TOGGLE_BUTTON(on_radio_button)->active) { 
	 do_it = 1;
      }
      prune_check_button = lookup_widget(window,"skeleton_prune_and_colour_checkbutton");
      if (GTK_TOGGLE_BUTTON(prune_check_button)->active) { 
	 prune_it = 1;
      }

      if (do_it)
	 graphics_info_t::skeletonize_map(prune_it, graphics_info_t::map_for_skeletonize);
      else {
	 std::cout << "INFO:: unskeletonizing map number "
		   << graphics_info_t::map_for_skeletonize << std::endl;
	 graphics_info_t::unskeletonize_map(graphics_info_t::map_for_skeletonize);
      }
   }
} 


int
graphics_info_t::try_set_draw_baton(short int i) { 
   graphics_info_t g;
   if (i) { 
      bool have_skeled_map_state = g.start_baton_here();
      if (have_skeled_map_state)
	 g.draw_baton_flag = 1;
   } else {
      g.draw_baton_flag = 0;
   } 
   graphics_draw();
   return g.draw_baton_flag;
}



void
graphics_info_t::fill_option_menu_with_skeleton_options(GtkWidget *option_menu) {  /* a wrapper */

   graphics_info_t g;
   GtkSignalFunc signalfunc = GTK_SIGNAL_FUNC(graphics_info_t::skeleton_map_select);
   g.fill_option_menu_with_map_options(option_menu, signalfunc,
				       graphics_info_t::map_for_skeletonize);

}

// static
void
graphics_info_t::skeleton_map_select(GtkWidget *item, GtkPositionType pos) { 

   graphics_info_t g;
   g.map_for_skeletonize = pos;

   // So now we have changed the map to skeletonize (or that has been
   // skeletonized).  We need to look up the radio buttons and set
   // them according to whether this map has a skeleton or not.
   //
   // Recall that for radio buttons, you have to set them on, setting
   // them off doesn't work.
   //
   GtkWidget *on_button  = lookup_widget(item, "skeleton_on_radiobutton");
   GtkWidget *off_button = lookup_widget(item, "skeleton_off_radiobutton");
   if (graphics_info_t::molecules[g.map_for_skeletonize].has_map()) {
      if (graphics_info_t::molecules[g.map_for_skeletonize].fc_skeleton_draw_on) {
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(on_button), TRUE);
      } else {
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(off_button), TRUE);
      }
   }

}


void
graphics_info_t::fill_option_menu_with_coordinates_options(GtkWidget *option_menu, 
							   GtkSignalFunc signal_func,
							   int imol_active_position) {

   GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));
//    std::cout << "option_menu: " << option_menu << std::endl;
//    std::cout << "menu: " << menu << std::endl;
//    std::cout << "menu is widget: " << GTK_IS_WIDGET(menu) << std::endl;
//    std::cout << "menu is menu: " << GTK_IS_MENU(menu) << std::endl;

   // menu is not GTK_MENU on Gtk2 Ubuntu kalypso 64 bit
   if (GTK_IS_MENU(menu))
      gtk_widget_destroy(menu);
   
   menu = gtk_menu_new();

   GtkWidget *menuitem;
   int item_count = 0; 

   for (int imol=0; imol<n_molecules(); imol++) {

      if (molecules[imol].has_model() > 0) { 

	 std::string ss = int_to_string(imol);
	 ss += " " ;
	 int ilen = molecules[imol].name_.length();
	 int left_size = ilen-go_to_atom_menu_label_n_chars_max;
	 if (left_size <= 0) {
	    // no chop
	    left_size = 0;
	 } else {
	    // chop
	    ss += "...";
	 } 

	 ss += molecules[imol].name_.substr(left_size, ilen);

	 menuitem = gtk_menu_item_new_with_label (ss.c_str());

	 gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			     signal_func,
			     GINT_TO_POINTER(imol)); 

	 gtk_menu_append(GTK_MENU(menu), menuitem); 
	 gtk_widget_show(menuitem);
 	 if (imol == imol_active_position) {
 	    gtk_menu_set_active(GTK_MENU(menu), item_count);
	 }
	 item_count++;
      }
   }
   
//    gtk_menu_set_active(GTK_MENU(menu), imol_active_position); 

   /* Link the new menu to the optionmenu widget */
   gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),
			    menu);

}

void
graphics_info_t::fill_option_menu_with_coordinates_options_possibly_small(GtkWidget *option_menu, 
									  GtkSignalFunc callback_func, 
									  int imol_active,
									  bool fill_with_small_molecule_only_flag) {

   int n_atoms_means_big_molecule = 400; 
   short int set_last_active_flag = 0;
   std::vector<int> fill_with_these_molecules;
   for (int imol=0; imol<n_molecules(); imol++) {
      if (molecules[imol].has_model()) {
	 int n_atoms = molecules[imol].atom_sel.n_selected_atoms;
	 if ( (!fill_with_small_molecule_only_flag) || (n_atoms < n_atoms_means_big_molecule) )
	    fill_with_these_molecules.push_back(imol);
      }
   }
   fill_option_menu_with_coordinates_options_internal_3(option_menu, callback_func, fill_with_these_molecules,
							set_last_active_flag, imol_active);
   

} 



void
graphics_info_t::set_on_off_skeleton_radio_buttons(GtkWidget *skeleton_frame) { 
   GtkWidget *on_button = lookup_widget(skeleton_frame, 
					"skeleton_on_radiobutton");
   GtkWidget *off_button = lookup_widget(skeleton_frame, 
					 "skeleton_off_radiobutton");

   int imol = map_for_skeletonize;
   if (imol >= 0) { 
      if (molecules[imol].xskel_is_filled) { 
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(on_button),  TRUE);
      } else {
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(off_button), TRUE);
      }
   } 
} 

void 
graphics_info_t::set_on_off_single_map_skeleton_radio_buttons(GtkWidget *skeleton_frame, 
							      int imol) { 
   GtkWidget *on_button = lookup_widget(skeleton_frame, 
					"single_map_skeleton_on_radiobutton");
   GtkWidget *off_button = lookup_widget(skeleton_frame, 
					 "single_map_skeleton_off_radiobutton");

   if (imol >= 0) { 
      if (molecules[imol].xskel_is_filled) { 
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(on_button),  TRUE);
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(off_button), FALSE);
      } else {
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(on_button),  FALSE);
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(off_button), TRUE);
      }
   } 
}

void
graphics_info_t::set_contour_sigma_button_and_entry(GtkWidget *window, int imol) { 

   GtkWidget *entry = lookup_widget(window, "single_map_sigma_step_entry");
   GtkWidget *checkbutton = lookup_widget(window, "single_map_sigma_checkbutton");

   if (imol < n_molecules()) { 
      if (molecules[imol].has_map()) { 
	 float v = molecules[imol].contour_sigma_step;
	 gtk_entry_set_text(GTK_ENTRY(entry), float_to_string(v).c_str());
	 if (molecules[imol].contour_by_sigma_flag) { 
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), TRUE);
	 } else { 
	    gtk_widget_set_sensitive(entry, FALSE);
	 }

	 
	 GtkWidget *level_entry =
	    lookup_widget(window, "single_map_properties_contour_level_entry");
	 float lev = molecules[imol].contour_level[0];
	 gtk_entry_set_text(GTK_ENTRY(level_entry), float_to_string(lev).c_str());
      }
   }

}

#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)
// coot::rama_plot is an unknown type if we don't have canvas
void
graphics_info_t::handle_rama_plot_update(coot::rama_plot *plot) {

   if (plot) {
      // if it's a normal plot: update it
      if (plot->is_kleywegt_plot()) {
	 // are the molecule numbers from which the kleywegt plot
	 // was generated still valid?
	 std::pair<int, int> p = plot->molecule_numbers();
	 if (graphics_info_t::molecules[p.first].has_model() && 
	     graphics_info_t::molecules[p.second].has_model()) { 
	    std::pair<std::string, std::string> chain_ids = plot->chain_ids();
	    std::cout << "updating kleywegt plot with chain ids :" << chain_ids.first
		      << ": :" << chain_ids.second << ":" << std::endl;
	    if (plot->kleywegt_plot_uses_chain_ids_p())
	       plot->draw_it(p.first, p.second,
			     graphics_info_t::molecules[p.first].atom_sel.mol,
			     graphics_info_t::molecules[p.second].atom_sel.mol,
			     chain_ids.first, chain_ids.second);
	    else 
	       plot->draw_it(p.first, p.second,
			     graphics_info_t::molecules[p.first].atom_sel.mol,
			     graphics_info_t::molecules[p.second].atom_sel.mol);
	 } else {
	    // close down the plot
	    plot->destroy_yourself();
	 }
      } else {
	 plot->draw_it(molecules[imol_moving_atoms].atom_sel.mol);
      } 
   } else {
      std::cout << "ERROR:: (trapped) in handle_rama_plot_update() attempt to draw to null plot\n";
   } 
}
#endif // HAVE_GNOME_CANVAS or HAVE_GTK_CANVAS


// static
gint
graphics_info_t::drag_refine_idle_function(GtkWidget *widget) {

#ifdef HAVE_GSL

   int retprog = graphics_info_t::drag_refine_refine_intermediate_atoms();

   if (retprog != GSL_CONTINUE) {
      if (retprog == GSL_ENOPROG)
	 std::cout << " NOPROGRESS" << std::endl;
      if (retprog == GSL_SUCCESS)
	 std::cout << " SUCCESS" << std::endl;
      long t1 = glutGet(GLUT_ELAPSED_TIME);
      std::cout << " TIME:: (dragged refinement): " << float(t1-T0)/1000.0 << std::endl;

      graphics_info_t g;
      g.check_and_warn_bad_chirals_and_cis_peptides();

      gtk_idle_remove(graphics_info_t::drag_refine_idle_function_token);
      graphics_info_t::drag_refine_idle_function_token = -1; // magic "not in use" value
   } 


   // 
   graphics_draw();

#else
   int retprog = -1;
#endif // HAVE_GSL   
   return retprog;
}



// static
void
graphics_info_t::add_drag_refine_idle_function() {

   // add a idle function if there isn't one in operation already.
   graphics_info_t g;
   if (g.drag_refine_idle_function_token == -1) {
      g.drag_refine_idle_function_token =
	 gtk_idle_add((GtkFunction)graphics_info_t::drag_refine_idle_function, g.glarea);
      T0 = glutGet(GLUT_ELAPSED_TIME);
      print_initial_chi_squareds_flag = 1;
   }
} 


// --------------------------------------------------------------------------------
//                 residue info widget
// --------------------------------------------------------------------------------


void
graphics_info_t::fill_output_residue_info_widget(GtkWidget *widget, int imol, PPCAtom atoms, int n_atoms) {

   // first do the label of the dialog
   GtkWidget *label_widget = lookup_widget(widget, "residue_info_residue_label");

   std::string label = "Molecule: ";
   label += int_to_string(imol);
   label += " ";
   label += molecules[imol].name_;

   gtk_label_set_text(GTK_LABEL(label_widget), label.c_str());
   GtkWidget *table = lookup_widget(widget, "residue_info_atom_table");
   
   residue_info_n_atoms = n_atoms; 
   for (int i=0; i<n_atoms; i++)
      graphics_info_t::fill_output_residue_info_widget_atom(table, imol, atoms[i], i);
}

void
graphics_info_t::fill_output_residue_info_widget_atom(GtkWidget *table, int imol, PCAtom atom,
						      int iatom) {

   GtkWidget *residue_info_dialog = lookup_widget(table, "residue_info_dialog");
   gint left_attach = 0;
   gint right_attach = 1;
   gint top_attach = iatom;
   gint bottom_attach = top_attach + 1;
   GtkAttachOptions xopt = GTK_FILL, yopt=GTK_FILL;
   guint xpad=0, ypad=0;

   // The text label of the atom name:
   left_attach = 0;
   right_attach = left_attach + 1;
   std::string label_str = "  ";
   label_str += atom->GetChainID();
   label_str += "/";
   label_str += graphics_info_t::int_to_string(atom->GetSeqNum());
   if (std::string(atom->GetInsCode()) != "") {
      label_str += atom->GetInsCode();
   }
   label_str += " ";
   label_str += atom->GetResName();
   label_str += "/";
   label_str += atom->name;
   if (0) { // do alt locs in the atom label (20090914, not now that we have altloc entries)
      if (std::string(atom->altLoc) != std::string("")) {
	 label_str += ",";
	 label_str += atom->altLoc;
      }
   }
   if (std::string(atom->segID) != std::string("")) {
      label_str += " ";
      label_str += atom->segID;
   }
   label_str += "  ";

   GtkWidget *residue_info_atom_info_label = gtk_label_new (label_str.c_str());
   gtk_table_attach(GTK_TABLE(table), residue_info_atom_info_label,
		    left_attach, right_attach, top_attach, bottom_attach,
		    xopt, yopt, xpad, ypad);
   gtk_widget_ref (residue_info_atom_info_label);
   gtk_object_set_data_full (GTK_OBJECT (residue_info_dialog),
			     "residue_info_atom_info_label", residue_info_atom_info_label,
			     (GtkDestroyNotify) gtk_widget_unref);
   gtk_widget_show (residue_info_atom_info_label);


   // The Occupancy entry:
   left_attach = 1;
   right_attach = left_attach + 1;
   coot::select_atom_info *ai = new coot::select_atom_info;
   *ai = coot::select_atom_info(iatom, imol, 
				std::string(atom->GetChainID()), 
				atom->GetSeqNum(),
				std::string(atom->GetInsCode()),
				std::string(atom->name),
				std::string(atom->altLoc));

   std::string widget_name = "residue_info_occ_entry_";
   widget_name += int_to_string(iatom);
   // 
   GtkWidget *residue_info_occ_entry = gtk_entry_new ();
   gtk_widget_ref (residue_info_occ_entry);
   gtk_object_set_data_full (GTK_OBJECT (residue_info_dialog),
			     widget_name.c_str(), residue_info_occ_entry,
			     (GtkDestroyNotify) gtk_widget_unref);
#if (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 6))
   gtk_widget_set_size_request(residue_info_occ_entry, 90, -1);
#endif
   gtk_widget_show (residue_info_occ_entry);
   gtk_object_set_user_data(GTK_OBJECT(residue_info_occ_entry), ai);
   gtk_entry_set_text(GTK_ENTRY(residue_info_occ_entry),
		      graphics_info_t::float_to_string(atom->occupancy).c_str());
   gtk_table_attach(GTK_TABLE(table), residue_info_occ_entry,
		    left_attach, right_attach, top_attach, bottom_attach,
		    xopt, yopt, xpad, ypad);


      // Note that we have to use key_release_event because if we use
   // key_press_event, when we try to get the value from the widget
   // (gtk_entry_get_text) then that does not see the key/number that
   // was just pressed.

   // B-factor entry:
   left_attach = 2;
   right_attach = left_attach + 1;

   widget_name = "residue_info_b_factor_entry_";
   widget_name += int_to_string(iatom);

   GtkWidget *residue_info_b_factor_entry = gtk_entry_new ();
   gtk_widget_ref (residue_info_b_factor_entry);
   gtk_object_set_data_full (GTK_OBJECT (residue_info_dialog),
			     widget_name.c_str(), residue_info_b_factor_entry,
			     (GtkDestroyNotify) gtk_widget_unref);
#if (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 6))
   gtk_widget_set_size_request(residue_info_b_factor_entry, 90, -1);
#endif
   gtk_widget_show (residue_info_b_factor_entry);
   gtk_entry_set_text(GTK_ENTRY(residue_info_b_factor_entry),
		      graphics_info_t::float_to_string(atom->tempFactor).c_str());
   // gtk_widget_set_usize(residue_info_b_factor_entry, 40, -2);
   gtk_object_set_user_data(GTK_OBJECT(residue_info_b_factor_entry), ai);
   gtk_widget_set_events(residue_info_b_factor_entry, 
			 GDK_KEY_PRESS_MASK     |
			 GDK_KEY_RELEASE_MASK);
   gtk_table_attach(GTK_TABLE(table), residue_info_b_factor_entry,
		    left_attach, right_attach, top_attach, bottom_attach,
		    xopt, yopt, xpad, ypad);


   // Alt Conf label:
   GtkWidget *alt_conf_label = gtk_label_new("  Alt-conf: ");
   gtk_widget_show(alt_conf_label);
   left_attach = 3;
   right_attach = left_attach + 1;
   gtk_table_attach(GTK_TABLE(table), alt_conf_label,
		    left_attach, right_attach, top_attach, bottom_attach,
		    xopt, yopt, xpad, ypad);


   // The Alt Conf entry:
   left_attach = 4;
   right_attach = left_attach + 1;
   ai = new coot::select_atom_info;
   *ai = coot::select_atom_info(iatom, imol, 
				std::string(atom->GetChainID()), 
				atom->GetSeqNum(),
				std::string(atom->GetInsCode()),
				std::string(atom->name),
				std::string(atom->altLoc));
   widget_name = "residue_info_altloc_entry_";
   widget_name += int_to_string(iatom);
   // 
   GtkWidget *residue_info_altloc_entry = gtk_entry_new ();
   gtk_widget_ref (residue_info_altloc_entry);
   gtk_object_set_data_full (GTK_OBJECT (residue_info_dialog),
			     widget_name.c_str(), residue_info_altloc_entry,
			     (GtkDestroyNotify) gtk_widget_unref);
#if (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION > 6))
   gtk_widget_set_size_request(residue_info_altloc_entry, 60, -1);
#endif   
   gtk_widget_show (residue_info_altloc_entry);
   gtk_object_set_user_data(GTK_OBJECT(residue_info_altloc_entry), ai);
   gtk_entry_set_text(GTK_ENTRY(residue_info_altloc_entry), atom->altLoc);
   gtk_table_attach(GTK_TABLE(table), residue_info_altloc_entry,
		    left_attach, right_attach, top_attach, bottom_attach,
		    xopt, yopt, xpad, ypad);
   


}

// static 
gboolean
graphics_info_t::on_residue_info_master_atom_occ_changed (GtkWidget       *widget,
							  GdkEventKey     *event,
							  gpointer         user_data) {
   const gchar *s = gtk_entry_get_text(GTK_ENTRY(widget));
   if (s) { 
      // consider strtof:
      // 
      // double f = atof(s);
      graphics_info_t::residue_info_pending_edit_occ = 1;
      
      graphics_info_t g;
      g.residue_info_edit_occ_apply_to_other_entries_maybe(widget);
   }
   return TRUE; 
}

// static
gboolean
graphics_info_t::on_residue_info_master_atom_b_factor_changed (GtkWidget       *widget,
							       GdkEventKey     *event,
							       gpointer         user_data) {

   // Let's get the entry value:
   //
   const gchar *s = gtk_entry_get_text(GTK_ENTRY(widget));
   if (s) { 
      // consider strtof:
      // 
      // float f = atof(s);
      graphics_info_t::residue_info_pending_edit_b_factor = 1;
      
      graphics_info_t g;
      g.residue_info_edit_b_factor_apply_to_other_entries_maybe(widget);
   }
   return TRUE;
}




//static 
void
graphics_info_t::residue_info_edit_b_factor_apply_to_other_entries_maybe(GtkWidget *start_entry) {

   // first find the checkbox:
   GtkWidget *dialog = lookup_widget(start_entry, "residue_info_dialog"); 
   GtkWidget *checkbutton = lookup_widget(dialog, "residue_info_b_factor_apply_all_checkbutton");
   std::string widget_name;
   GtkWidget *entry;

   if (! checkbutton) { 
      std::cout << "ERROR:: could not find checkbutton" << std::endl; 
   } else { 
      if (GTK_TOGGLE_BUTTON(checkbutton)->active) { 
	 
	 // propogate the change to the other b-factor widgets
	 std::string entry_text(gtk_entry_get_text(GTK_ENTRY(start_entry)));
	 for (int i=0; i<graphics_info_t::residue_info_n_atoms; i++) {
	    widget_name = "residue_info_b_factor_entry_";
	    widget_name += int_to_string(i);
	    entry = lookup_widget(dialog, widget_name.c_str());
	    if (entry) { 
	       gtk_entry_set_text(GTK_ENTRY(entry), entry_text.c_str());
	    } else { 
	       std::cout << "ERROR: no entry\n";
	    } 
	 }
      } 
   }
}


//static 
void
graphics_info_t::residue_info_edit_occ_apply_to_other_entries_maybe(GtkWidget *start_entry) {

   // first find the checkbox:
   GtkWidget *dialog = lookup_widget(start_entry, "residue_info_dialog"); 
   GtkWidget *checkbutton = lookup_widget(dialog, "residue_info_occ_apply_all_checkbutton");
   std::string widget_name;
   GtkWidget *entry;

   if (! checkbutton) { 
      std::cout << "ERROR:: could not find checkbutton" << std::endl; 
   } else { 
      if (GTK_TOGGLE_BUTTON(checkbutton)->active) { 
	 
	 // propogate the change to the other b-factor widgets
	 std::string entry_text(gtk_entry_get_text(GTK_ENTRY(start_entry)));
	 for (int i=0; i<graphics_info_t::residue_info_n_atoms; i++) {
	    widget_name = "residue_info_occ_entry_";
	    widget_name += int_to_string(i);
	    entry = lookup_widget(dialog, widget_name.c_str());
	    if (entry) { 
	       gtk_entry_set_text(GTK_ENTRY(entry), entry_text.c_str());
	    } else { 
	       std::cout << "ERROR: no entry\n";
	    } 
	 }
      } 
   }
}


// static
void
graphics_info_t::residue_info_add_b_factor_edit(coot::select_atom_info sai, 
						float val) { 

   graphics_info_t g;
   short int made_substitution_flag = 0;
   for (unsigned int i=0; i<g.residue_info_edits->size(); i++) {
      if (sai.udd == (*g.residue_info_edits)[i].udd) {
	 (*g.residue_info_edits)[i].add_b_factor_edit(val);
	 made_substitution_flag = 1;
	 break;
      }
   }
   if (! made_substitution_flag) { 
      sai.add_b_factor_edit(val);
      g.residue_info_edits->push_back(sai);
   }
}

// static
void
graphics_info_t::residue_info_add_occ_edit(coot::select_atom_info sai, 
					   float val) { 

   graphics_info_t g;
   short int made_substitution_flag = 0;
   for (unsigned int i=0; i<g.residue_info_edits->size(); i++) {
      if (sai.udd == (*g.residue_info_edits)[i].udd) {
	 (*g.residue_info_edits)[i].add_occ_edit(val);
	 made_substitution_flag = 1;
	 break;
      }
   }
   if (! made_substitution_flag) { 
      sai.add_occ_edit(val);
      g.residue_info_edits->push_back(sai);
   }
}

// This is the callback when the OK button of the residue info was pressed.
//
// The new way with a table:
void
graphics_info_t::apply_residue_info_changes(GtkWidget *dialog) {


   int imol = -1;
   // This is where we accumulate the residue edits:
   std::vector<coot::select_atom_info> local_atom_edits;

   GtkWidget *table = lookup_widget(dialog, "residue_info_atom_table");

#if (GTK_MAJOR_VERSION > 1)
   GList *container_list = gtk_container_get_children(GTK_CONTAINER(table));
#else    
   GList *container_list = gtk_container_children(GTK_CONTAINER(table));
#endif
   
   // The children are a list, gone in "backward", just like we'd been
   // consing onto a list as we added widgets to the table.
   // 
   int len = g_list_length(container_list);
   std::cout << "=== The table has " << len << " elements" << std::endl;
   
   for(int i=0; i < len; i+=5) {
      if ((i+1) < len) { 
	 GtkWidget *widget_alt = (GtkWidget*) g_list_nth_data(container_list, i);
	 GtkWidget *widget_b   = (GtkWidget*) g_list_nth_data(container_list, i+2);
	 GtkWidget *widget_o   = (GtkWidget*) g_list_nth_data(container_list, i+3);
	 std::string b_text = gtk_entry_get_text(GTK_ENTRY(widget_b));
	 std::string o_text = gtk_entry_get_text(GTK_ENTRY(widget_o));
// 	 std::cout << "b_text :" <<b_text << std::endl;
// 	 std::cout << "o_text :" <<o_text << std::endl;

	 // Handle OCCUPANCY edits
	 // 
	 coot::select_atom_info *ai =
	    (coot::select_atom_info *) gtk_object_get_user_data(GTK_OBJECT(widget_o));
	 if (ai) {
	    imol = ai->molecule_number;  // hehe
	    CAtom *at = ai->get_atom(graphics_info_t::molecules[imol].atom_sel.mol);
	    // std::cout << "got atom at " << at << std::endl;
	    std::pair<short int, float>  occ_entry =
	       graphics_info_t::float_from_entry(GTK_WIDGET(widget_o));
	    if (occ_entry.first) {
	       if (at) { 
// 		  std::cout << "    occ comparison " << occ_entry.second << " "
// 		  << at->occupancy << std::endl;
		  if (abs(occ_entry.second - at->occupancy) > 0.009) {
		     coot::select_atom_info local_at = *ai;
		     local_at.add_occ_edit(occ_entry.second);
		     local_atom_edits.push_back(local_at);
		  } 
	       }
	    }
	 } else {
	    std::cout << "no user data found for widget_o" << std::endl;
	 }

	 // HANDLE B-FACTOR edits
	 ai = (coot::select_atom_info *) gtk_object_get_user_data(GTK_OBJECT(widget_b));
	 if (ai) {
	    imol = ai->molecule_number;  // hehe
	    CAtom *at = ai->get_atom(graphics_info_t::molecules[imol].atom_sel.mol);
	    std::pair<short int, float>  temp_entry =
	       graphics_info_t::float_from_entry(GTK_WIDGET(widget_b));
	    if (temp_entry.first) {
	       if (at) { 
		  // std::cout << "    temp comparison " << temp_entry.second
		  // << " " << at->tempFactor << std::endl;
		  if (abs(temp_entry.second - at->tempFactor) > 0.009) {
		     coot::select_atom_info local_at = *ai;
		     local_at.add_b_factor_edit(temp_entry.second);
		     local_atom_edits.push_back(local_at);
		  }
	       }
	    }
	 }

	 // HANDLE Alt-conf edits
	 ai = (coot::select_atom_info *) gtk_object_get_user_data(GTK_OBJECT(widget_alt));
	 if (ai) {
	    imol = ai->molecule_number;  // hehe
	    CAtom *at = ai->get_atom(graphics_info_t::molecules[imol].atom_sel.mol);
	    std::string entry_text = gtk_entry_get_text(GTK_ENTRY(widget_alt));
	    if (at) { 
	       coot::select_atom_info local_at = *ai;
	       local_at.add_altloc_edit(entry_text);
	       local_atom_edits.push_back(local_at);
	    }
	 }
      } else {
	 std::cout << "Programmer error in decoding table." << std::endl;
	 std::cout << "  Residue Edits not applied!" << std::endl;
      }
   }

   if (local_atom_edits.size() >0)
      if (imol >= 0)
	 graphics_info_t::molecules[imol].apply_atom_edits(local_atom_edits);

   residue_info_edits->clear();
   // delete res_spec; // can't do this: the user may press the button twice
}

// static
// (should be called by the destroy event and the close button)
void
graphics_info_t::residue_info_release_memory(GtkWidget *dialog) { 

   GtkWidget *entry;
   for (int i=0; i<residue_info_n_atoms; i++) { 
      std::string widget_name = "residue_info_b_factor_entry_";
      widget_name += int_to_string(i);
      entry = lookup_widget(dialog, widget_name.c_str());
      if (entry) { 
	 coot::select_atom_info *sai_p = (coot::select_atom_info *) gtk_object_get_user_data(GTK_OBJECT(entry));
	 if (sai_p) { 
	    // delete sai_p; // memory bug.  We cant do this
	    // std::cout << "hmmm.. not deleting sai_p" << std::endl;
	 } else { 
	    std::cout << "ERROR:: no user data in b-factor entry widget\n"; 
	 } 
      }
      // same for occ entry:
      widget_name = "residue_info_occ_entry_";
      widget_name += int_to_string(i);
      entry = lookup_widget(dialog, widget_name.c_str());
      if (entry) { 
	 coot::select_atom_info *sai_p = (coot::select_atom_info *) gtk_object_get_user_data(GTK_OBJECT(entry));
	 if (sai_p) { 
	    // std::cout << "not deleting sai_p" << std::endl;
	    // delete sai_p; // memory bug.  We cant do this
	 } else { 
	    std::cout << "ERROR:: no user data in occ entry widget\n"; 
	 }
      }
   } 
} 

// static 
void
graphics_info_t::pointer_atom_molecule_menu_item_activate(GtkWidget *item, 
							  GtkPositionType pos) {

   graphics_info_t g;
   //    std::cout << "DEBUG:: pointer_atom_molecule_menu_item_activate sets user_pointer_atom_molecule to " << pos << std::endl;
   g.user_pointer_atom_molecule = pos;

}


   
// We are passed an GtkOptionMenu *option_menu
//
void
graphics_info_t::fill_option_menu_with_coordinates_options(GtkWidget *option_menu,
							   GtkSignalFunc callback_func) { 

   fill_option_menu_with_coordinates_options_internal(option_menu, callback_func, 0);

}

// See Changelog 2004-05-05
// 
// We are passed an GtkOptionMenu *option_menu
//
void
graphics_info_t::fill_option_menu_with_coordinates_options_internal(GtkWidget *option_menu,
								    GtkSignalFunc callback_func, 
								    short int set_last_active_flag) {

   int imol_active = -1; // To allow the function to work as it used to.
   fill_option_menu_with_coordinates_options_internal_2(option_menu, callback_func,
							set_last_active_flag, imol_active);

}

void
graphics_info_t::fill_option_menu_with_coordinates_options_internal_2(GtkWidget *option_menu,
								      GtkSignalFunc callback_func, 
								      short int set_last_active_flag,
								      int imol_active) {

   std::vector<int> fill_with_these_molecules;
   for (int imol=0; imol<n_molecules(); imol++) {
      if (molecules[imol].has_model()) {
	 fill_with_these_molecules.push_back(imol);
      }
   }
   fill_option_menu_with_coordinates_options_internal_3(option_menu, callback_func, fill_with_these_molecules,
							set_last_active_flag, imol_active);
}

void
graphics_info_t::fill_option_menu_with_coordinates_options_internal_3(GtkWidget *option_menu,
								      GtkSignalFunc callback_func,
								      std::vector<int> fill_with_these_molecules,
								      short int set_last_active_flag,
								      int imol_active) { 

   // like the column labels from an mtz file, similarly fill this
   // option_menu with items that correspond to molecules that have
   // coordinates.
   //

   // Get the menu of the optionmenu (which was set in interface.c:
   // gtk_option_menu_set_menu (GTK_OPTION_MENU (go_to_atom_molecule_optionmenu),
   //                           go_to_atom_molecule_optionmenu_menu);
   //
   GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));
   

   // for the strangeness of destroying and re-adding the menu to the
   // option menu, set the comments in the
   // fill_close_option_menu_with_all_molecule_options function

   // menu is not GTK_MENU on Gtk2 Ubuntu kalypso 64 bit
   if (menu) 
      gtk_widget_destroy(menu);

   /* Create a menu for the optionmenu button.  The various molecule
    numbers will be added to this menu as menuitems*/
   menu = gtk_menu_new();

   // GtkWidget *optionmenu_menu = gtk_menu_new();
   GtkWidget *menuitem;
   // int last_imol = 0;
   int last_menu_item_index = 0;

   int menu_index = 0; // for setting of imol_active as active mol in go to atom
   for (int imol=0; imol<fill_with_these_molecules.size(); imol++) {

//       std::cout << "in fill_option_menu_with_coordinates_options, "
// 		<< "g.molecules[" << imol << "].atom_sel.n_selected_atoms is "
// 		<< g.molecules[imol].atom_sel.n_selected_atoms << std::endl;
      
      if (molecules[fill_with_these_molecules[imol]].has_model()) { 

	 std::string ss = int_to_string(imol);
	 ss += " " ;
	 int ilen = molecules[imol].name_.length();
	 int left_size = ilen-go_to_atom_menu_label_n_chars_max;
	 if (left_size <= 0) {
	    // no chop
	    left_size = 0;
	 } else {
	    // chop
	    ss += "...";
	 } 
	 ss += molecules[fill_with_these_molecules[imol]].name_.substr(left_size, ilen);
	 menuitem = gtk_menu_item_new_with_label (ss.c_str());

	 gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			     // GTK_SIGNAL_FUNC(go_to_atom_mol_button_select),
			     callback_func,
			     GINT_TO_POINTER(imol)); 

	 // Note that we probably don't need to do the following
	 // because we already pass a GINT_TO_POINTER(imol) in the
	 // signal connect.
	 //
	 // But on reflection.. perhaps we do because we do a
	 // menu_get_active in save_go_to_atom_mol_menu_active_position
	 // 
	 // we set user data on the menu item, so that when this goto
	 // Atom widget is cancelled, we can whatever was the molecule
	 // number corresponding to the active position of the menu
	 //
	 // Should be freed in on_go_to_atom_cancel_button_clicked
	 // (callbacks.c)
	 // 
	  
	 gtk_object_set_user_data(GTK_OBJECT(menuitem), GINT_TO_POINTER(imol));
	 gtk_menu_append(GTK_MENU(menu), menuitem); 

	 if (fill_with_these_molecules[imol] == imol_active)
	    gtk_menu_set_active(GTK_MENU(menu), menu_index);

	 // we do need this bit of course:
	 gtk_widget_show(menuitem); 
	 last_menu_item_index++;
	 menu_index++; 
      }
   }
   
   // set any previously saved active position:
   // but overridden by flag:
   if (set_last_active_flag) {
      // explanation of -1 offset: when there are 2 menu items,
      // last_menu_item_index is 2, but the item index of the last
      // item is 2 - 1.
      //
      gtk_menu_set_active(GTK_MENU(menu), (last_menu_item_index-1)); 
   } else {
      // the old way (ie. not ..._with_active_mol() mechanism)
      if (imol_active == -1) { 
	 if (go_to_atom_mol_menu_active_position >= 0) {
	    gtk_menu_set_active(GTK_MENU(menu), go_to_atom_mol_menu_active_position); 
	 }
      }
   }

   /* Link the new menu to the optionmenu widget */
   gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu),
			    menu);
}


void
graphics_info_t::fill_option_menu_with_coordinates_options_internal_with_active_mol(GtkWidget *option_menu,
										    GtkSignalFunc callback_func, 
										    int imol_active) {

   short int set_last_active_flag = 0;
   fill_option_menu_with_coordinates_options_internal_2(option_menu, callback_func,
							set_last_active_flag, imol_active);
} 

// not const
void
graphics_info_t::fill_option_menu_with_undo_options(GtkWidget *option_menu) {

   
   GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));
   if (menu) 
      gtk_widget_destroy(menu);
   menu = gtk_menu_new();

   GtkWidget *menuitem;
   int active_mol_no = -1;
   int undo_local = undo_molecule;  // defaults to -1; If it is unset (-1)
			       // then pick the first undoable
			       // molecule as the undo_molecule.
   int pos_count = 0;

   for (int i=0; i<n_molecules(); i++) {
      // if (molecules[i].has_model()) { 
      if (molecules[i].atom_sel.mol) { 
	 if (molecules[i].Have_modifications_p()) {
	    if (undo_local == -1)
	       undo_local = i;
	    char s[200];
	    snprintf(s, 199, "%d", i);
	    std::string ss(s);
	    ss += " ";
	    ss += molecules[i].name_;
	    menuitem = gtk_menu_item_new_with_label(ss.c_str());
	    gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			       GTK_SIGNAL_FUNC(graphics_info_t::undo_molecule_select),
			       GINT_TO_POINTER(i));
	    gtk_menu_append(GTK_MENU(menu), menuitem);
	    gtk_widget_show(menuitem);
	    if (i == undo_molecule)
	       gtk_menu_set_active(GTK_MENU(menu), pos_count);
	    pos_count++; 
	 }
      }
   }

   gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);

   // undo_local should have been set (this function should not be called
   // if there are not more than 1 molecule with modifications).
   if (undo_local > -1) {
      set_undo_molecule_number(undo_local);
   }

} 

// a static function
void
graphics_info_t::undo_molecule_select(GtkWidget *item, GtkPositionType pos) {
   graphics_info_t g;
   g.set_undo_molecule_number(pos);
   std::cout << "undo molecule number set to " << pos << std::endl;
}

void
graphics_info_t::set_baton_build_params(int istart_resno,
					const char *chain_id, 
					const char *backwards) { 

   baton_build_params_active = 1; // don't ignore baton_build_params
				  // in placing atom.
   baton_build_start_resno = istart_resno;
   std::string dir(backwards);
   if (dir == "backwards") { 
      baton_build_direction_flag = -1;
   } else { 
      if (dir == "forwards") { 
	 baton_build_direction_flag = 1;
      } else { 
	 baton_build_direction_flag = 0; // unknown.
      }
   }
   baton_build_chain_id = std::string(chain_id);
   // std::cout << "DEBUG:: baton_build_chain_id set to " << baton_build_chain_id << std::endl;
}

void
graphics_info_t::model_fit_refine_unactive_togglebutton(const std::string &button_name) const { 

   if (model_fit_refine_dialog) {
      GtkWidget *toggle_button = lookup_widget(model_fit_refine_dialog, button_name.c_str());
      if (toggle_button)
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), FALSE);
      else 
	 std::cout << "ERROR:: failed to find button: " << button_name << std::endl;
      
   } else {
      // std::cout << "DEBUG:: model_fit_refine_dialog not found" << std::endl;
   }

#if (GTK_MAJOR_VERSION > 1)

   std::string toolbar_button_name = "not-found";
   if (button_name == "model_refine_dialog_refine_togglebutton")
      toolbar_button_name = "model_toolbar_refine_togglebutton";
   if (button_name == "model_refine_dialog_regularize_zone_togglebutton")
      toolbar_button_name = "model_toolbar_regularize_togglebutton";
   if (button_name == "model_refine_dialog_rigid_body_togglebutton")
      toolbar_button_name = "model_toolbar_rigid_body_fit_togglebutton";
   if (button_name == "model_refine_dialog_rot_trans_toolbutton")
      toolbar_button_name = "model_toolbar_rot_trans_toolbutton";
   if (button_name == "model_refine_dialog_auto_fit_rotamer_togglebutton")
      toolbar_button_name = "model_toolbar_auto_fit_rotamer_togglebutton";
   if (button_name == "model_refine_dialog_rotamer_togglebutton")
      toolbar_button_name = "model_toolbar_rotamers_togglebutton";
   if (button_name == "model_refine_dialog_edit_chi_angles_togglebutton")
      toolbar_button_name = "model_toolbar_edit_chi_angles_togglebutton";
   if (button_name == "model_refine_dialog_torsion_general_togglebutton")
      toolbar_button_name = "model_toolbar_torsion_general_toggletoolbutton";
   if (button_name == "model_refine_dialog_pepflip_togglebutton")
      toolbar_button_name = "model_toolbar_flip_peptide_togglebutton";
   if (button_name == "model_refine_dialog_do_180_degree_sidechain_flip_togglebutton")
      toolbar_button_name = "model_toolbar_sidechain_180_togglebutton";
   if (button_name == "model_refine_dialog_edit_backbone_torsions_togglebutton")
      toolbar_button_name = "model_toolbar_edit_backbone_torsions_toggletoolbutton";
   if (button_name == "model_refine_dialog_mutate_auto_fit_togglebutton")
      toolbar_button_name = "model_toolbar_mutate_and_autofit_togglebutton";
   if (button_name == "model_refine_dialog_mutate_togglebutton")
      toolbar_button_name = "model_toolbar_simple_mutate_togglebutton";
   if (button_name == "model_refine_dialog_fit_terminal_residue_togglebutton")
      toolbar_button_name = "model_toolbar_add_terminal_residue_togglebutton";

   // now, button_name may have been
   // model_refine_dialog_edit_phi_psi_togglebutton or
   // model_refine_dialog_edit_backbone_torsions_togglebutton, we
   // don't have toolbar equivalents of those.
   // 
   if (toolbar_button_name != "not-found") { 
      GtkWidget *toggle_button = lookup_widget(graphics_info_t::glarea,
					       toolbar_button_name.c_str());
//       std::cout << "DEBUG:: toggle_button for gtk2 toolbar: " << button_name << "->"
// 		<< toolbar_button_name << " " << toggle_button << std::endl;

      if (toggle_button) {
	// somehow we cannot use ->active on the toggle_tool_buttons?!
	gboolean active = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toggle_button));
	if (active)
	  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toggle_button), FALSE);
      }
   }
#endif    // GTK version
   
} 


void
graphics_info_t::other_modelling_tools_unactive_togglebutton(const std::string &button_name) const { 

   if (other_modelling_tools_dialog) {
      GtkWidget *toggle_button = lookup_widget(other_modelling_tools_dialog,
					       button_name.c_str());
      if (toggle_button)
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), FALSE);
      else 
	 std::cout << "ERROR:: failed to find button: " << button_name
		   << std::endl;
   }
}

int
graphics_info_t::wrapped_create_edit_chi_angles_dialog(const std::string &res_type) {

   GtkWidget *dialog = create_edit_chi_angles_dialog();
   set_transient_and_position(COOT_EDIT_CHI_DIALOG, dialog);

   
   // Fill the vbox with buttons with atom labels about which there
   // are rotatable torsions:
   //
   GtkWidget *vbox = lookup_widget(dialog,"edit_chi_angles_vbox");
   int n_boxes = fill_chi_angles_vbox(vbox, res_type);

   // this needs to be deleted when dialog is destroyed, I think,
   // (currently it isn't).
   int NCHAR = 100;
   char *s = new char[NCHAR];
   for (int i=0; i<NCHAR; i++) s[i] = 0;
   strncpy(s, res_type.c_str(), NCHAR);
   gtk_object_set_user_data(GTK_OBJECT(vbox), s);
   // we get this in c-interface-gui.cc's fill_chi_angles_vbox();

   gtk_widget_show(dialog);

   // and now the hydrogen torsion checkbutton:
   GtkWidget *togglebutton =
      lookup_widget(dialog, "edit_chi_angles_add_hydrogen_torsions_checkbutton");

   if (find_hydrogen_torsions)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton), TRUE);

   // "Reverse Fragment" button
   edit_chi_angles_reverse_fragment = 0; // reset the static

   return n_boxes;
}

void
graphics_info_t::clear_out_container(GtkWidget *vbox) {

   GList *ls = gtk_container_children(GTK_CONTAINER(vbox));

   while (ls) {
      GtkWidget *w = GTK_WIDGET(ls->data);
      gtk_widget_destroy(w);
      ls = ls->next;
   }
} 


int 
graphics_info_t::fill_chi_angles_vbox(GtkWidget *vbox, std::string res_type) {

   std::vector <coot::dict_torsion_restraint_t> v =
      get_monomer_torsions_from_geometry(res_type); // uses find H torsions setting.

   clear_out_container(vbox);

   // We introduce here ichi (which gets incremented if the current
   // torsion is not const), we do that so that we have consistent
   // indexing in the torsions vector with chi_angles's change_by()
   // (see comments above execute_edit_chi_angles()).

   int ichi = 0;
   for (unsigned int i=0; i<v.size(); i++) {
      if (!v[i].is_const()) {
	 std::string label = "  ";
	 label += v[i].id(); 
	 label += "  ";
	 label += v[i].atom_id_2_4c();
	 label += " <--> ";
	 label += v[i].atom_id_3_4c();
	 label += "  ";
	 GtkWidget *button = gtk_button_new_with_label(label.c_str());
	 gtk_signal_connect(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC(on_change_current_chi_button_clicked),
			    GINT_TO_POINTER(ichi));
	 gtk_signal_connect(GTK_OBJECT(button), "enter",
			    GTK_SIGNAL_FUNC(on_change_current_chi_button_entered),
			    GINT_TO_POINTER(ichi));
	 gtk_widget_set_name(button, "edit_chi_angles_button");
	 gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	 gtk_container_set_border_width(GTK_CONTAINER(button), 2);
	 gtk_widget_show(button);
	 ichi++;
      }
   }

   return v.size();
} 

// static
void
graphics_info_t::on_change_current_chi_button_clicked(GtkButton *button,
						      gpointer user_data) {

   graphics_info_t g;
   int i = GPOINTER_TO_INT(user_data);
   g.edit_chi_current_chi = i + 1;
   g.in_edit_chi_mode_flag = 1;

} 

// static
void
graphics_info_t::on_change_current_chi_button_entered(GtkButton *button,
						      gpointer user_data) {

   graphics_info_t g;
   int ibond_user = GPOINTER_TO_INT(user_data);

   g.setup_flash_bond_internal(ibond_user+1);

}

// Create a moving atoms molecule, consisting of the Ca(n), Ca(n+1) of
// the peptide, N(n) C(n+1), O(n+1).  Note the alt conf should be the
// same (we can have altconfed mainchain).
// 
void
graphics_info_t::execute_setup_backbone_torsion_edit(int imol, int atom_index) { 
   
   // if not the same altconf for all atoms, give up (for now).
   // 
   // Do an atom selection of this residue and the previous one
   // (unless this was a N, then do this one and the next)
   // 
   // Run through the atom selection finding the atoms.  put the atoms
   // into new residues (appropriate) and construct a mol and and asc
   // and make that the moving atoms asc.  Be sure to put the residue
   // with the peptide N first so that mmdb finds it first when it
   // does the atom selection that's part of make_asc(mol) - which
   // means that get_first_atom_with_atom_name() will work like we
   // want it to.

   if (imol < n_molecules()) { 
      if (molecules[imol].has_model()) { 
	 if (atom_index < molecules[imol].atom_sel.n_selected_atoms) { 
	    CAtom *this_atom_p = molecules[imol].atom_sel.atom_selection[atom_index];
	    int offset = 0; // usually this and next residue
	    std::string this_atname(this_atom_p->name);
	    if (this_atname == " N  ") { 
	       offset = -1;
	    }
	    int this_res = this_atom_p->GetSeqNum();
	    char *chain_id = this_atom_p->GetChainID();
	    int SelectionHandle = molecules[imol].atom_sel.mol->NewSelection();
	    char *ins_code = this_atom_p->GetInsCode();
	    char *altconf  = this_atom_p->altLoc;
	    std::string a_tmp(altconf);
	    if (a_tmp != "") { 
	       a_tmp += ",";
	    }
	    char *search_altconf = (char *) a_tmp.c_str();
	    molecules[imol].atom_sel.mol->SelectAtoms (SelectionHandle, 0, chain_id,
					      this_res + offset , // starting resno
					      ins_code, // any insertion code
					      this_res + 1 + offset, // ending resno
					      ins_code, // ending insertion code
					      "*", // any residue name
					      "*", // atom name
					      "*", // elements
					      search_altconf  // alt loc.
					      );
	    int nSelAtoms;
	    PPCAtom SelectAtoms;
	    molecules[imol].atom_sel.mol->GetSelIndex(SelectionHandle, SelectAtoms, nSelAtoms);
	    if (nSelAtoms < 4) { // the very min
	       std::cout <<  "WARNING:: not enough atoms in atom selection in "
			 <<  "execute_setup_backbone_torsion_edit" << std::endl;
	    } else {


	       // We construct a moving atom asc atom by atom...
	       CAtom *next_ca = NULL, *next_n = NULL;
	       CAtom *this_c = NULL, *this_o = NULL, *this_ca = NULL;
	       
	       // and the extra atoms that we need (we extract the
	       // coordinates) to construct a pair of ramachandran
	       // points in the rama_plot:

	       CAtom *prev_c = NULL;
	       CAtom *this_n = NULL;
	       CAtom *next_c = NULL;
	       CAtom *next_plus_1_n = NULL;

	       // to get prev_c and next_plus_1_n we do atom selections:

	       // You can add (this) hydrogen to that list if you want.
	       CAtom *at;
	       // 
	       for (int iat=0; iat<nSelAtoms; iat++) { 
		  at = SelectAtoms[iat];
		  if (at->GetSeqNum() == (this_res + offset) ) { 
		     std::string n(at->name);
		     if (n == " CA ") { 
			this_ca = at;
		     }
		     if (n == " O  ") { 
			this_o = at;
		     }
		     if (n == " C  ") { 
			this_c = at;
		     }
		     if (n == " N  ") { 
			this_n = at;
		     }
		  }
		  if (at->GetSeqNum() == (this_res + 1 + offset) ) { 
		     std::string n(at->name);
		     if (n == " CA ") { 
			next_ca = at;
		     }
		     if (n == " N  ") { 
			next_n = at;
		     }
		     if (n == " C  ") { 
			next_c = at;
		     }
		  }
	       }

	       int SelHnd_prev_c = molecules[imol].atom_sel.mol->NewSelection();
	       // to get prev_c and next_plus_1_n we do atom selections:
	       molecules[imol].atom_sel.mol->SelectAtoms (SelHnd_prev_c, 0, chain_id,
							  this_res - 1 + offset , // starting resno
							  ins_code, // any insertion code
							  this_res - 1 + offset, // ending resno
							  ins_code, // ending insertion code
							  "*",    // any residue name
							  " C  ", // atom name
							  "*",    // elements
							  search_altconf  // alt loc.
							  );

	       int nSelAtoms_prev_c;
	       PPCAtom SelectAtoms_prev_c;

	       molecules[imol].atom_sel.mol->GetSelIndex(SelHnd_prev_c, 
							 SelectAtoms_prev_c, 
							 nSelAtoms_prev_c);
	       if (nSelAtoms_prev_c > 0) { 
		  prev_c = SelectAtoms_prev_c[0];
	       } else { 
		  std::cout << "Oops:: didn't find prev_c\n";
	       } 
	       molecules[imol].atom_sel.mol->DeleteSelection(SelHnd_prev_c);
	       
	       // And similarly for next_plus_1_n:
	       int SelHnd_next_plus_1_n = molecules[imol].atom_sel.mol->NewSelection();
	       molecules[imol].atom_sel.mol->SelectAtoms (SelHnd_next_plus_1_n, 0, chain_id,
							  this_res + 2 + offset , // starting resno
							  ins_code, // any insertion code
							  this_res + 2 + offset, // ending resno
							  ins_code, // ending insertion code
							  "*",    // any residue name
							  " N  ", // atom name
							  "*",    // elements
							  search_altconf  // alt loc.
							  );
	       int nSelAtoms_next_plus_1_n;
	       PPCAtom SelectAtoms_next_plus_1_n;

	       molecules[imol].atom_sel.mol->GetSelIndex(SelHnd_next_plus_1_n, 
							 SelectAtoms_next_plus_1_n, 
							 nSelAtoms_next_plus_1_n);
	       if (nSelAtoms_next_plus_1_n > 0) { 
		  next_plus_1_n = SelectAtoms_next_plus_1_n[0];
	       } else {
		  std::cout << "Oops:: didn't find next + 1 N\n";
	       } 
	       molecules[imol].atom_sel.mol->DeleteSelection(SelHnd_next_plus_1_n);

	       
	       if (next_ca && next_n && this_ca && this_o && this_c) { 

		  // new addition 25Feb2004
		  rama_plot_for_2_phi_psis(imol, atom_index);

		  CMMDBManager *mol = new CMMDBManager;
		  CModel *model = new CModel;
		  CChain *chain = new CChain;
		  CResidue *res1 = new CResidue;
		  CResidue *res2 = new CResidue;
		  res1->SetResName(this_ca->GetResName());
		  res2->SetResName(next_ca->GetResName());
		  res1->seqNum = this_ca->GetSeqNum();
		  res2->seqNum = next_ca->GetSeqNum();
		  chain->SetChainID(this_ca->GetChainID());

		  at = new CAtom; 
		  at->Copy(this_ca);
		  res1->AddAtom(at);

		  at = new CAtom; 
		  at->Copy(this_c);
		  res1->AddAtom(at);

		  at = new CAtom; 
		  at->Copy(this_o);
		  res1->AddAtom(at);

		  at = new CAtom; 
		  at->Copy(next_ca);
		  res2->AddAtom(at);

		  at = new CAtom; 
		  at->Copy(next_n);
		  res2->AddAtom(at);

		  chain->AddResidue(res1);
		  chain->AddResidue(res2);
		  model->AddChain(chain);
		  mol->AddModel(model);
		  mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
		  mol->FinishStructEdit();
		  imol_moving_atoms = imol;
		  moving_atoms_asc_type = coot::NEW_COORDS_REPLACE;
		  atom_selection_container_t asc = make_asc(mol);
		  regularize_object_bonds_box.clear_up();
		  make_moving_atoms_graphics_object(asc);

		  // save the fixed end points:
		  backbone_torsion_end_ca_1 = 
		     clipper::Coord_orth(this_ca->x, this_ca->y, this_ca->z);
		  backbone_torsion_end_ca_2 = 
		     clipper::Coord_orth(next_ca->x, next_ca->y, next_ca->z);
		  

		  // add to rama_points:
		  rama_points.clear();
		  rama_points.add("this_ca", backbone_torsion_end_ca_1);
		  rama_points.add("next_ca", backbone_torsion_end_ca_2);
		  // the moving atoms:
		  rama_points.add("this_c", clipper::Coord_orth(this_c->x,
								this_c->y,
								this_c->z));
		  rama_points.add("this_o", clipper::Coord_orth(this_o->x,
								this_o->y,
								this_o->z));
		  rama_points.add("next_n", clipper::Coord_orth(next_n->x,
								next_n->y,
								next_n->z));

		  if (this_n) 
		     rama_points.add("this_n",  clipper::Coord_orth(this_n->x,
								    this_n->y,
								    this_n->z));

		  if (next_c) 
		     rama_points.add("next_c",  clipper::Coord_orth(next_c->x,
								    next_c->y,
								    next_c->z));

		  // next_plus_1_n, prev_c;
		  if (next_plus_1_n)
		     rama_points.add("next+1_n", clipper::Coord_orth(next_plus_1_n->x,
								     next_plus_1_n->y,
								     next_plus_1_n->z));
		  if (prev_c)
		     rama_points.add("prev_c", clipper::Coord_orth(prev_c->x,
								   prev_c->y,
								   prev_c->z));

// 		  std::cout << "DEBUG:: backbone_torsion_end_ca_1: " 
// 			    << backbone_torsion_end_ca_1.format() << std::endl;
// 		  std::cout << "DEBUG:: backbone_torsion_end_ca_2: " 
// 			    << backbone_torsion_end_ca_2.format() << std::endl;

		  graphics_draw();
		  GtkWidget *widget = create_edit_backbone_torsions_dialog();
		  set_edit_backbone_adjustments(widget);
		  gtk_widget_show(widget);
		  
	       } else { 
		  std::cout << "WARNING:: not all atoms found in " 
			    << "execute_setup_backbone_torsion_edit" << std::endl;
		     
	       } 
	    }
	    // 
	    molecules[imol].atom_sel.mol->DeleteSelection(SelectionHandle);
	 } 
      }
   }
}

void
graphics_info_t::set_edit_backbone_adjustments(GtkWidget *widget) { 

   GtkWidget *hscale_peptide = lookup_widget(widget, 
					     "edit_backbone_torsions_rotate_peptide_hscale");

   GtkWidget *hscale_carbonyl = lookup_widget(widget, 
					     "edit_backbone_torsions_rotate_carbonyl_hscale");

//    gfloat value,
//    gfloat lower,
//    gfloat upper,
//    gfloat step_increment,
//    gfloat page_increment,
//    gfloat page_size

   GtkAdjustment *adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -180.0, 360.0, 0.1, 1.0, 180));
   gtk_range_set_adjustment(GTK_RANGE(hscale_peptide), adjustment);
   gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		      GTK_SIGNAL_FUNC(graphics_info_t::edit_backbone_peptide_changed_func), NULL);

   
   // and the carbonyl:
   adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -180.0, 360.0, 0.1, 1.0, 180));
   gtk_range_set_adjustment(GTK_RANGE(hscale_carbonyl), adjustment);
   gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		      GTK_SIGNAL_FUNC(graphics_info_t::edit_backbone_carbonyl_changed_func), NULL);

}

// static 
void
graphics_info_t::edit_backbone_peptide_changed_func(GtkAdjustment *adj, GtkWidget *window) { 
   
   graphics_info_t g;
   // std::cout << "change backbone peptide by: " << adj->value << std::endl;
   
   std::pair<short int, clipper::Coord_orth> this_c = rama_points.get("this_c");
   std::pair<short int, clipper::Coord_orth> this_o = rama_points.get("this_o");
   std::pair<short int, clipper::Coord_orth> next_n = rama_points.get("next_n");

   if (this_c.first && this_o.first && next_n.first) { 
      CAtom *n_atom_p = coot::get_first_atom_with_atom_name(" N  ", *moving_atoms_asc);
      CAtom *c_atom_p = coot::get_first_atom_with_atom_name(" C  ", *moving_atoms_asc);
      CAtom *o_atom_p = coot::get_first_atom_with_atom_name(" O  ", *moving_atoms_asc);
      
      double rad_angle = clipper::Util::d2rad(adj->value);
      clipper::Coord_orth new_c = 
	 g.rotate_round_vector(backbone_torsion_end_ca_2 - backbone_torsion_end_ca_1,
			       this_c.second,
			       backbone_torsion_end_ca_1, rad_angle);
      clipper::Coord_orth new_o = 
	 g.rotate_round_vector(backbone_torsion_end_ca_2 - backbone_torsion_end_ca_1,
			       this_o.second,
			       backbone_torsion_end_ca_1, rad_angle);
      clipper::Coord_orth new_n = 
	 g.rotate_round_vector(backbone_torsion_end_ca_2 - backbone_torsion_end_ca_1,
			       next_n.second,
			       backbone_torsion_end_ca_1, rad_angle);
      n_atom_p->x = new_n.x();
      n_atom_p->y = new_n.y();
      n_atom_p->z = new_n.z();
  
      c_atom_p->x = new_c.x();
      c_atom_p->y = new_c.y();
      c_atom_p->z = new_c.z();
      
      o_atom_p->x = new_o.x();
      o_atom_p->y = new_o.y();
      o_atom_p->z = new_o.z();

      std::pair<std::pair<double, double>, std::pair<double, double> > pp = 
	 g.phi_psi_pairs_from_moving_atoms();
      
//    std::cout << pp.first.first  << " " << pp.first.second << "      " 
// 	     << pp.second.first << " " << pp.second.second << std::endl;

#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)

      if (edit_phi_psi_plot) { 
	 std::vector <coot::util::phi_psi_t> vp;
	 std::string label = int_to_string(c_atom_p->GetSeqNum());
	 if (pp.first.first > -200) { 
	    label += c_atom_p->GetChainID();
	    coot::util::phi_psi_t phipsi1(clipper::Util::rad2d(pp.first.first), 
					  clipper::Util::rad2d(pp.first.second),
					  "resname", label, 1, "inscode", "chainid");
	    vp.push_back(phipsi1);
	 }
	 if (pp.second.first > -200) { 
	    label = int_to_string(n_atom_p->GetSeqNum());
	    label += n_atom_p->GetChainID();
	    coot::util::phi_psi_t phipsi2(clipper::Util::rad2d(pp.second.first), 
					  clipper::Util::rad2d(pp.second.second),
					  "resname", label, 1, "inscode", "chainid");

	    vp.push_back(phipsi2);
	 }
	 if (vp.size() > 0) 
	    edit_phi_psi_plot->draw_it(vp);
      } 
#endif // HAVE_GTK_CANVAS
      regularize_object_bonds_box.clear_up();
      g.make_moving_atoms_graphics_object(*moving_atoms_asc);
      graphics_draw();

   } else { 
      std::cout << "ERROR:: can't find rama points in edit_backbone_peptide_changed_func" 
		<< std::endl;
   } 
} 

// static 
void
graphics_info_t::edit_backbone_carbonyl_changed_func(GtkAdjustment *adj, GtkWidget *window) { 

   graphics_info_t g;
   // std::cout << "change backbone peptide by: " << adj->value << std::endl;
   
   std::pair<short int, clipper::Coord_orth> this_c = rama_points.get("this_c");
   std::pair<short int, clipper::Coord_orth> this_o = rama_points.get("this_o");

   if (this_c.first && this_o.first) { 
      CAtom *c_atom_p = coot::get_first_atom_with_atom_name(" C  ", *moving_atoms_asc);
      CAtom *o_atom_p = coot::get_first_atom_with_atom_name(" O  ", *moving_atoms_asc);
      CAtom *n_atom_p = coot::get_first_atom_with_atom_name(" N  ", *moving_atoms_asc);

      clipper::Coord_orth carbonyl_n_pos(n_atom_p->x, n_atom_p->y, n_atom_p->z);

      double rad_angle = clipper::Util::d2rad(adj->value);
      clipper::Coord_orth new_c = 
	 g.rotate_round_vector(carbonyl_n_pos - backbone_torsion_end_ca_1,
			       this_c.second,
			       backbone_torsion_end_ca_1, rad_angle);
      clipper::Coord_orth new_o = 
	 g.rotate_round_vector(carbonyl_n_pos - backbone_torsion_end_ca_1,
			       this_o.second,
			       backbone_torsion_end_ca_1, rad_angle);

      c_atom_p->x = new_c.x();
      c_atom_p->y = new_c.y();
      c_atom_p->z = new_c.z();
      
      o_atom_p->x = new_o.x();
      o_atom_p->y = new_o.y();
      o_atom_p->z = new_o.z();

      std::pair<std::pair<double, double>, std::pair<double, double> > pp = 
	 g.phi_psi_pairs_from_moving_atoms();
      
//    std::cout << pp.first.first  << " " << pp.first.second << "      " 
// 	     << pp.second.first << " " << pp.second.second << std::endl;

#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)

      if (edit_phi_psi_plot) { 
	 std::vector <coot::util::phi_psi_t> vp;
	 std::string label = int_to_string(c_atom_p->GetSeqNum());
	 if (pp.first.first > -200) { 
	    label += c_atom_p->GetChainID();
	    coot::util::phi_psi_t phipsi1(clipper::Util::rad2d(pp.first.first), 
					  clipper::Util::rad2d(pp.first.second),
					  "resname", label, 1, "inscode", "chainid");
	    vp.push_back(phipsi1);
	 }
	 if (pp.second.first > -200) { 
	    label = int_to_string(n_atom_p->GetSeqNum());
	    label += n_atom_p->GetChainID();
	    coot::util::phi_psi_t phipsi2(clipper::Util::rad2d(pp.second.first), 
					  clipper::Util::rad2d(pp.second.second),
					  "resname", label, 1, "inscode", "chainid");

	    vp.push_back(phipsi2);
	 }
	 if (vp.size() > 0) 
	    edit_phi_psi_plot->draw_it(vp);
      } 

#endif // HAVE_GTK_CANVAS
      regularize_object_bonds_box.clear_up();
      g.make_moving_atoms_graphics_object(*moving_atoms_asc);
      graphics_draw();

   } else { 
      std::cout << "ERROR:: can't find rama points in edit_backbone_peptide_changed_func" 
		<< std::endl;
   } 
}


// #include "mmdb-extras.h"

// Tinker with the moving atoms
void
graphics_info_t::change_peptide_carbonyl_by(double angle) { 

//    std::cout << "move carbonyl by " << angle << std::endl;
   CAtom *n_atom_p = coot::get_first_atom_with_atom_name(" N  ", *moving_atoms_asc);
   CAtom *c_atom_p = coot::get_first_atom_with_atom_name(" C  ", *moving_atoms_asc);
   CAtom *o_atom_p = coot::get_first_atom_with_atom_name(" O  ", *moving_atoms_asc);

   clipper::Coord_orth carbonyl_n_pos(n_atom_p->x, n_atom_p->y, n_atom_p->z);
   clipper::Coord_orth carbonyl_c_pos(c_atom_p->x, c_atom_p->y, c_atom_p->z);
   clipper::Coord_orth carbonyl_o_pos(o_atom_p->x, o_atom_p->y, o_atom_p->z);

   double rad_angle = clipper::Util::d2rad(angle);
   
   clipper::Coord_orth new_c = 
      rotate_round_vector(carbonyl_n_pos - backbone_torsion_end_ca_1,
			  carbonyl_c_pos,
			  carbonyl_n_pos, rad_angle);
   clipper::Coord_orth new_o = 
      rotate_round_vector(carbonyl_n_pos - backbone_torsion_end_ca_1,
			  carbonyl_o_pos,
			  carbonyl_n_pos, rad_angle);

   c_atom_p->x = new_c.x();
   c_atom_p->y = new_c.y();
   c_atom_p->z = new_c.z();

   o_atom_p->x = new_o.x();
   o_atom_p->y = new_o.y();
   o_atom_p->z = new_o.z();

   std::pair<std::pair<double, double>, std::pair<double, double> > pp = 
      phi_psi_pairs_from_moving_atoms();
   
//    std::cout << pp.first.first  << " " << pp.first.second << "      " 
// 	     << pp.second.first << " " << pp.second.second << std::endl;
   

#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)

   if (edit_phi_psi_plot) { 
      std::vector <coot::util::phi_psi_t> vp;
      std::string label = int_to_string(c_atom_p->GetSeqNum());
      label += c_atom_p->GetChainID();
      coot::util::phi_psi_t phipsi1(clipper::Util::rad2d(pp.first.first), 
				    clipper::Util::rad2d(pp.first.second),
				    "resname", label, 1, "inscode", "chainid");
      label = int_to_string(n_atom_p->GetSeqNum());
      label += n_atom_p->GetChainID();
      coot::util::phi_psi_t phipsi2(clipper::Util::rad2d(pp.second.first), 
				    clipper::Util::rad2d(pp.second.second),
				    "resname", label, 1, "inscode", "chainid");

      vp.push_back(phipsi1);
      vp.push_back(phipsi2);
      edit_phi_psi_plot->draw_it(vp);
   } 

#endif // HAVE_GTK_CANVAS

   regularize_object_bonds_box.clear_up();
   make_moving_atoms_graphics_object(*moving_atoms_asc);
   graphics_draw();
} 


// Return a pair of phi psi's for the residue of the carbonyl and the next
// residue.  
// 
// If for some reason the pair is incalculable, put phi (first) to -2000.
//
std::pair<std::pair<double, double>, std::pair<double, double> >
graphics_info_t::phi_psi_pairs_from_moving_atoms() {

   std::pair<std::pair<double, double>, std::pair<double, double> > p;

   // There are 2 atoms in the moving atoms that we need for this calculation,
   // this_C and next_N.  The rest we will pull out of a pre-stored vector of
   // pairs (made in execute_setup_backbone_torsion_edit): rama_points.

   CAtom *c_atom_p = coot::get_first_atom_with_atom_name(" C  ", *moving_atoms_asc);
   CAtom *n_atom_p = coot::get_first_atom_with_atom_name(" N  ", *moving_atoms_asc);

   clipper::Coord_orth this_c (c_atom_p->x, c_atom_p->y, c_atom_p->z);
   clipper::Coord_orth next_n (n_atom_p->x, n_atom_p->y, n_atom_p->z);

   std::pair<short int, clipper::Coord_orth> prev_c      = rama_points.get("prev_c");
   std::pair<short int, clipper::Coord_orth> this_ca     = rama_points.get("this_ca");
   std::pair<short int, clipper::Coord_orth> this_n      = rama_points.get("this_n");
   std::pair<short int, clipper::Coord_orth> next_ca     = rama_points.get("next_ca");
   std::pair<short int, clipper::Coord_orth> next_c      = rama_points.get("next_c");
   std::pair<short int, clipper::Coord_orth> next_plus_n = rama_points.get("next+1_n");
   
   if (prev_c.first && this_ca.first && this_n.first) { 

      // we can calculate the first ramachadran phi/psi pair
      double phi = clipper::Coord_orth::torsion(prev_c.second, this_n.second,  this_ca.second, this_c);
      double psi = clipper::Coord_orth::torsion(this_n.second, this_ca.second, this_c,         next_n);

      p.first.first  = phi;
      p.first.second = psi;

   } else { 

      // can't get the first ramachandran point
      p.first.first = -2000;

   }

   if (next_ca.first && next_c.first && next_plus_n.first) { 

      // we can calculate the first ramachadran phi/psi pair
      
      double phi = clipper::Coord_orth::torsion(this_c, next_n, next_ca.second, next_c.second);
      double psi = clipper::Coord_orth::torsion(next_n, next_ca.second, next_c.second, next_plus_n.second);

      p.second.first  = phi;
      p.second.second = psi;

   } else { 

      // can't get the second ramachandran point
      p.second.first = -2000;
   } 


   return p;
} 
// Tinker with the moving atoms
void
graphics_info_t::change_peptide_peptide_by(double angle) { 

//    std::cout << "move peptide by " << angle << std::endl;

   CAtom *n_atom_p = coot::get_first_atom_with_atom_name(" N  ", *moving_atoms_asc);
   CAtom *c_atom_p = coot::get_first_atom_with_atom_name(" C  ", *moving_atoms_asc);
   CAtom *o_atom_p = coot::get_first_atom_with_atom_name(" O  ", *moving_atoms_asc);

   clipper::Coord_orth carbonyl_n_pos(n_atom_p->x, n_atom_p->y, n_atom_p->z);
   clipper::Coord_orth carbonyl_c_pos(c_atom_p->x, c_atom_p->y, c_atom_p->z);
   clipper::Coord_orth carbonyl_o_pos(o_atom_p->x, o_atom_p->y, o_atom_p->z);

   double rad_angle = clipper::Util::d2rad(angle);

   clipper::Coord_orth new_c = 
      rotate_round_vector(backbone_torsion_end_ca_2 - backbone_torsion_end_ca_1,
			  carbonyl_c_pos,
			  backbone_torsion_end_ca_1, rad_angle);
   clipper::Coord_orth new_o = 
      rotate_round_vector(backbone_torsion_end_ca_2 - backbone_torsion_end_ca_1,
			  carbonyl_o_pos,
			  backbone_torsion_end_ca_1, rad_angle);

   clipper::Coord_orth new_n = 
      rotate_round_vector(backbone_torsion_end_ca_2 - backbone_torsion_end_ca_1,
			  carbonyl_n_pos,
			  backbone_torsion_end_ca_1, rad_angle);

   n_atom_p->x = new_n.x();
   n_atom_p->y = new_n.y();
   n_atom_p->z = new_n.z();

   c_atom_p->x = new_c.x();
   c_atom_p->y = new_c.y();
   c_atom_p->z = new_c.z();

   o_atom_p->x = new_o.x();
   o_atom_p->y = new_o.y();
   o_atom_p->z = new_o.z();

   std::pair<std::pair<double, double>, std::pair<double, double> > pp = 
      phi_psi_pairs_from_moving_atoms();
   
//    std::cout << pp.first.first  << " " << pp.first.second << "      " 
// 	     << pp.second.first << " " << pp.second.second << std::endl;

#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)

   if (edit_phi_psi_plot) { 
      std::vector <coot::util::phi_psi_t> vp;
      std::string label = int_to_string(c_atom_p->GetSeqNum());
      label += c_atom_p->GetChainID();
      coot::util::phi_psi_t phipsi1(clipper::Util::rad2d(pp.first.first), 
				    clipper::Util::rad2d(pp.first.second),
				    "resname", label, 1, "inscode", "chainid");
      label = int_to_string(n_atom_p->GetSeqNum());
      label += n_atom_p->GetChainID();
      coot::util::phi_psi_t phipsi2(clipper::Util::rad2d(pp.second.first), 
				    clipper::Util::rad2d(pp.second.second),
				    "resname", label, 1, "inscode", "chainid");

      vp.push_back(phipsi1);
      vp.push_back(phipsi2);
      edit_phi_psi_plot->draw_it(vp);
   } 

#endif // HAVE_GTK_CANVAS

   regularize_object_bonds_box.clear_up();
   make_moving_atoms_graphics_object(*moving_atoms_asc);
   graphics_draw();
} 


void 
graphics_info_t::set_backbone_torsion_peptide_button_start_pos(int ix, int iy) { 
   backbone_torsion_peptide_button_start_pos_x = ix;
   backbone_torsion_peptide_button_start_pos_y = iy;
} 

void 
graphics_info_t::set_backbone_torsion_carbonyl_button_start_pos(int ix, int iy) { 
   backbone_torsion_carbonyl_button_start_pos_x = ix;
   backbone_torsion_carbonyl_button_start_pos_y = iy;
} 


void
graphics_info_t::change_peptide_peptide_by_current_button_pos(int ix, int iy) { 

   double diff = 0.05* (ix - backbone_torsion_peptide_button_start_pos_x);
   change_peptide_peptide_by(diff);
} 

void
graphics_info_t::change_peptide_carbonyl_by_current_button_pos(int ix, int iy) { 

   double diff = 0.05 * (ix - backbone_torsion_carbonyl_button_start_pos_x);
   change_peptide_carbonyl_by(diff);
} 

//      ----------------- sequence view ----------------
#if defined (HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)
//
// return NULL on failure to find the class for this molecule
coot::sequence_view *
graphics_info_t::get_sequence_view(int imol) {

   GtkWidget *w = NULL;
   coot::sequence_view *r = NULL;

   if (imol < n_molecules()) {
      if (molecules[imol].has_model()) {
	 // w = sequence_view_is_displayed[imol];
	 w = coot::get_validation_graph(imol, coot::SEQUENCE_VIEW);
	 r = (coot::sequence_view *) gtk_object_get_user_data(GTK_OBJECT(w));
	 // std::cout << "DEBUG:: user data from " << w << " is " << r << std::endl;
      }
   } 
   return r;
}
#endif // HAVE_GTK_CANVAS

void
graphics_info_t::set_sequence_view_is_displayed(GtkWidget *widget, int imol) {

#if defined(HAVE_GTK_CANVAS) || defined(HAVE_GNOME_CANVAS)

   // first delete the old sequence view if it exists
   GtkWidget *w = coot::get_validation_graph(imol, coot::SEQUENCE_VIEW);
   if (w) {
      coot::sequence_view *sv = (coot::sequence_view *)
	 gtk_object_get_user_data(GTK_OBJECT(w));
      delete sv;
   }

   if (imol < n_molecules()) {
//       coot::sequence_view *sv = (coot::sequence_view *)
// 	 gtk_object_get_user_data(GTK_OBJECT(sequence_view_is_displayed[imol]));
//       std::cout << "DEBUG:: seting sequence_view_is_displayed[" << imol
// 		<< "] " << widget << std::endl;
      // sequence_view_is_displayed[imol] = widget; // ols style
      coot::set_validation_graph(imol, coot::SEQUENCE_VIEW, widget);
   }
#endif // HAVE_GTK_CANVAS   
} 
void 
graphics_info_t::unset_geometry_dialog_distance_togglebutton() { 

   if (geometry_dialog) { 
      GtkWidget *toggle_button = lookup_widget(geometry_dialog, 
					       "geometry_distance_togglebutton");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), FALSE);
   }
}

void
graphics_info_t::unset_geometry_dialog_dynamic_distance_togglebutton() {

   if (geometry_dialog) { 
      GtkWidget *toggle_button = lookup_widget(geometry_dialog, 
					       "geometry_dynamic_distance_togglebutton");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), FALSE);
   }
}


void
graphics_info_t::unset_geometry_dialog_angle_togglebutton() { 

   if (geometry_dialog) { 
      GtkWidget *toggle_button = lookup_widget(geometry_dialog, 
					       "geometry_angle_togglebutton");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), FALSE);
   }
}


void
graphics_info_t::unset_geometry_dialog_torsion_togglebutton() { 

   if (geometry_dialog) { 
      GtkWidget *toggle_button = lookup_widget(geometry_dialog, 
					       "geometry_torsion_togglebutton");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), FALSE);
   }
}


void
graphics_info_t::set_zoom_adjustment(GtkWidget *widget) { 

   GtkWidget *zoom_hscale = lookup_widget(widget, "zoom_hscale");

   //  GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -18.0, 36.0, 0.01, 1.0, 18));
   //  GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -2.0, 4.0, 0.01, 1.0, 2));

   GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(zoom, zoom*0.125, zoom*8, 0.01, 0.5, zoom));

   gtk_range_set_adjustment(GTK_RANGE(zoom_hscale), adj);
   gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
		      GTK_SIGNAL_FUNC(graphics_info_t::zoom_adj_changed),
		      NULL);
}


// static
void
graphics_info_t::zoom_adj_changed(GtkAdjustment *adj, GtkWidget *window) { 
   graphics_info_t g;

//    double scaled = pow(M_E, -adj->value);
//    std::cout <<  adj->value << " " << scaled << std::endl;

   // g.zoom *= scaled;
   g.zoom = adj->value;
   graphics_draw();

} 


// ----------------------------------------------------------------------------
//                check waters 
// ----------------------------------------------------------------------------
// 
void
graphics_info_t::check_waters_by_difference_map(int imol_waters, int imol_diff_map, 
						int interactive_flag) {

   if (is_valid_model_molecule(imol_waters)) {
      if (is_valid_map_molecule(imol_diff_map)) { 
	 if (molecules[imol_diff_map].is_difference_map_p()) {
	    std::vector <coot::atom_spec_t> v = molecules[imol_waters].check_waters_by_difference_map(molecules[imol_diff_map].xmap_list[0], check_waters_by_difference_map_sigma_level);
	    if (interactive_flag) { 
	       GtkWidget *w = wrapped_create_checked_waters_by_variance_dialog(v, imol_waters);
	       gtk_widget_show(w);
	    }
	 } else {
	    std::cout << "molecule " <<  imol_diff_map
		      << " is not a difference map\n";
	 }
      } else {
	 std::cout << "molecule " <<  imol_diff_map << "has no map\n";
      }
   } else {
      std::cout << "molecule " <<  imol_waters << "has no model\n";
   } 
}

// results widget:
// 
// 

GtkWidget *
graphics_info_t::wrapped_create_checked_waters_by_variance_dialog(const std::vector <coot::atom_spec_t> &v, int imol) {

   GtkWidget *w;

   if (v.size() > 0) {
      w = create_interesting_waters_by_difference_map_check_dialog();
      GtkWidget *vbox = lookup_widget(w, "interesting_waters_by_difference_map_check_vbox");
      GtkWidget *button;
      coot::atom_spec_t *atom_spec;
      GSList *gr_group = NULL;
      
      for (unsigned int i=0; i<v.size(); i++) {

	 std::cout << "Suspicious water: "
		   << v[i].atom_name
		   << v[i].alt_conf << " "
		   << v[i].resno << " "
		   << v[i].insertion_code << " "
		   << v[i].chain << "\n";

	 std::string button_label(" ");
	 button_label += v[i].chain;
	 button_label += " " ;
	 button_label += int_to_string(v[i].resno);
	 button_label += " " ;
	 button_label += v[i].atom_name;
	 button_label += " " ;
	 button_label += v[i].alt_conf;
	 button_label += " " ;

	 button = gtk_radio_button_new_with_label(gr_group, button_label.c_str());
	 gr_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
	 atom_spec = new coot::atom_spec_t(v[i]);
	 atom_spec->int_user_data = imol;
      
	 gtk_signal_connect(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC (on_generic_atom_spec_button_clicked),
			    atom_spec);

	 GtkWidget *frame = gtk_frame_new(NULL);
	 gtk_container_add(GTK_CONTAINER(frame), button);
	 gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	 gtk_container_set_border_width(GTK_CONTAINER(frame), 2);
	 gtk_widget_show(button);
	 gtk_widget_show(frame);
      } 
   } else {
      std::cout << "There are no unusual waters\n";
      std::string s = "There were no strange/anomalous waters\n";
      s += "(in relation to the difference map).";
      w = wrapped_nothing_bad_dialog(s);
   }
   return w;
}


// static 
void
graphics_info_t::on_generic_atom_spec_button_clicked (GtkButton *button,
						      gpointer user_data) {

   if (GTK_TOGGLE_BUTTON(button)->active) { 

      graphics_info_t g;
      coot::atom_spec_t *atom_spec = (coot::atom_spec_t *) user_data;
//       std::cout << "atom_spec: " 
// 		<< atom_spec->chain << " " << atom_spec->resno << " " << atom_spec->atom_name
// 		<< std::endl;
   
      g.set_go_to_atom_molecule(atom_spec->int_user_data);
      g.set_go_to_atom_chain_residue_atom_name(atom_spec->chain.c_str(),
					       atom_spec->resno,
					       atom_spec->atom_name.c_str(),
					       atom_spec->alt_conf.c_str());
      g.try_centre_from_new_go_to_atom();
      g.update_things_on_move_and_redraw();
   }
}

GtkWidget *
graphics_info_t::wrapped_create_chiral_restraints_problem_dialog(const std::vector<std::string> &sv) const {

   GtkWidget *w = create_chiral_restraints_problem_dialog();

   GtkWidget *label = lookup_widget(w, "chiral_volume_restraints_problem_label");
   std::string s = "\n   Problem finding restraints for the following residues:   \n\n";
   for (unsigned int i=0; i<sv.size(); i++) {
      s += sv[i];
      s += "  ";
      if (10*((i+1)/10) == (i+1))
	 s += "\n";
   }
   s += "\n";
   gtk_label_set_text(GTK_LABEL(label), s.c_str());
   return w;
}


GtkWidget *
graphics_info_t::wrapped_check_chiral_volumes_dialog(const std::vector <coot::atom_spec_t> &v,
						     int imol) {

   GtkWidget *w = NULL;
   
   std::cout  << "There were " << v.size() << " bad chiral volumes: " << std::endl;

   if (v.size() > 0) {
      GtkWidget *button;
      w = create_bad_chiral_volumes_dialog ();
      GtkWidget *bad_chiral_volume_atom_vbox = 
	 lookup_widget(w, "chiral_volume_baddies_vbox");
      coot::atom_spec_t *atom_spec;
      for (unsigned int i=0; i<v.size(); i++) { 
	 std::cout << "  "
		   << v[i].chain << " " 
		   << v[i].resno << " " 
		   << v[i].atom_name << " " 
		   << v[i].alt_conf << " " 
		   << "\n";

	 // c.f. how we add rotamers: (fill_rotamer_selection_buttons)
	 std::string button_label(" ");
	 button_label += v[i].chain;
	 button_label += " " ;
	 button_label += int_to_string(v[i].resno);
	 button_label += " " ;
	 button_label += v[i].atom_name;
	 button_label += " " ;
	 button_label += v[i].alt_conf;
	 button_label += " " ;

	 button = gtk_button_new_with_label(button_label.c_str());
	 atom_spec = new coot::atom_spec_t(v[i]);
	 atom_spec->int_user_data = imol;
      
	 gtk_signal_connect(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC (on_bad_chiral_volume_button_clicked),
			    atom_spec);

	 gtk_box_pack_start(GTK_BOX(bad_chiral_volume_atom_vbox),
			    button, FALSE, FALSE, 0);
	 gtk_container_set_border_width(GTK_CONTAINER(button), 2);
	 gtk_widget_show(button);
      }

   } else { 
      std::cout << "Congratulations: there are no bad chiral volumes in this molecule.\n";
      w = create_no_bad_chiral_volumes_dialog();
   } 
   return w;
}

// static 
void
graphics_info_t::on_bad_chiral_volume_button_clicked (GtkButton       *button,
						      gpointer         user_data) {


   // This function may get called for an "unclick" too.  It may need
   // an active? test like the generic atom spec button callback.  But
   // perhaps not.
   
   graphics_info_t g;
   coot::atom_spec_t *atom_spec = (coot::atom_spec_t *) user_data;
//    std::cout << "atom_spec: " 
// 	     << atom_spec->chain << " "
// 	     << atom_spec->resno << " "
// 	     << atom_spec->atom_name << " "
// 	     << atom_spec->alt_conf << " "
// 	     << std::endl;
   
   g.set_go_to_atom_molecule(atom_spec->int_user_data);
   g.set_go_to_atom_chain_residue_atom_name(atom_spec->chain.c_str(),
					    atom_spec->resno,
					    atom_spec->atom_name.c_str(),
					    atom_spec->alt_conf.c_str());
   g.try_centre_from_new_go_to_atom();
   g.update_things_on_move_and_redraw();


}

// static
//
// imol can be -1 (for no molecules available)
// 
void graphics_info_t::fill_bond_parameters_internals(GtkWidget *w,
						    int imol) {

   // We don't do these 2 any more because they have been moved to
   // bond colour dialog:
   // fill the colour map rotation entry
   // check the Carbons only check button

   // fill the molecule bond width option menu

   // check the draw hydrogens check button

   // and now also set the adjustment on the hscale

   graphics_info_t g;

   GtkWidget *bond_width_option_menu = lookup_widget(w, "bond_parameters_bond_width_optionmenu");
   GtkWidget *draw_hydrogens_yes_radiobutton  = lookup_widget(w, "draw_hydrogens_yes_radiobutton");
   GtkWidget *draw_hydrogens_no_radiobutton   = lookup_widget(w, "draw_hydrogens_no_radiobutton");
   GtkWidget *draw_ncs_ghosts_yes_radiobutton = lookup_widget(w, "draw_ncs_ghosts_yes_radiobutton");
   GtkWidget *draw_ncs_ghosts_no_radiobutton  = lookup_widget(w, "draw_ncs_ghosts_no_radiobutton");

   // bye bye entry
//    gtk_entry_set_text(GTK_ENTRY(entry),
// 		      float_to_string(rotate_colour_map_on_read_pdb).c_str());

   g.bond_thickness_intermediate_value = -1;
   
   // Fill the bond width option menu.
   // Put a redraw on the menu item activate callback.
   // We do the thing with the new menu for the option_menu
   // 
   GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(bond_width_option_menu));
   GtkSignalFunc signal_func = GTK_SIGNAL_FUNC(graphics_info_t::bond_width_item_select);
   if (menu) 
      gtk_widget_destroy(menu);
   menu = gtk_menu_new();
   GtkWidget *menu_item;
   int current_bond_width = 3;
   if (imol >= 0 ) {
      if (imol < n_molecules()) {
	 if (molecules[imol].has_model()) {
	    current_bond_width = molecules[imol].bond_thickness();
	 }
      }
   }

   for (int i=1; i<21; i++) {
      std::string s = int_to_string(i);
      menu_item = gtk_menu_item_new_with_label(s.c_str());
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 GTK_SIGNAL_FUNC(signal_func),
			 GINT_TO_POINTER(i));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_widget_show(menu_item);
      if (i == current_bond_width) {
	 gtk_menu_set_active(GTK_MENU(menu), i);
      }
   }
   gtk_menu_set_active(GTK_MENU(menu), current_bond_width-1); // 0 offset
   gtk_option_menu_set_menu(GTK_OPTION_MENU(bond_width_option_menu), menu);

   // Draw Hydrogens?
   if (imol >= 0 ) {
      if (imol < n_molecules()) {
	 if (molecules[imol].has_model()) {
	    if (molecules[imol].draw_hydrogens()) {
	       gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(draw_hydrogens_yes_radiobutton), TRUE);
	    } else {
	       gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(draw_hydrogens_no_radiobutton), TRUE);
	    }
	 }
      }
   }

   // Draw NCS ghosts?
   if (imol >= 0 ) {
      if (imol < n_molecules()) {
	 if (molecules[imol].has_model()) {
	    if (molecules[imol].draw_ncs_ghosts_p()) {
	       if (molecules[imol].ncs_ghosts_have_rtops_p()) { 
		  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(draw_ncs_ghosts_yes_radiobutton), TRUE);
	       } else {
		  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(draw_ncs_ghosts_no_radiobutton), TRUE);
	       }
	    } else {
	       gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(draw_ncs_ghosts_no_radiobutton), TRUE);
	    }
	 }
      }
   }
   // Make the frame be insensitive if there is no NCS.
   GtkWidget *frame = lookup_widget(w, "ncs_frame");
   short int make_insensitive = 1;
   if (imol >= 0 ) {
      if (imol < n_molecules()) {
	 if (molecules[imol].has_model()) {
	    if (molecules[imol].has_ncs_p()) {
	       make_insensitive = 0;
	    } else {
	       std::cout << "INFO:: in fill_bond_parameters_internals no NCS for  "
			 << imol << "\n"; 
	    }
	 } else {
	    std::cout << "ERROR:: bad imol in fill_bond_parameters_internals no model "
		      << imol << "\n"; 
	 }
      } else {
	 std::cout << "ERROR:: bad imol in fill_bond_parameters_internals i " << imol << "\n"; 
      }
   } else {
      std::cout << "ERROR:: bad imol in fill_bond_parameters_internals " << imol << "\n"; 
   }
   if (make_insensitive)
      gtk_widget_set_sensitive(frame, FALSE);
   else 
      gtk_widget_set_sensitive(frame, TRUE);
   
}


void
graphics_info_t::fill_bond_colours_dialog_internal(GtkWidget *w) {

   // First the (global) step adjustment:
   GtkScale *hscale = GTK_SCALE(lookup_widget(w, "bond_parameters_colour_rotation_hscale"));
   GtkAdjustment *adjustment = GTK_ADJUSTMENT
      (gtk_adjustment_new(rotate_colour_map_on_read_pdb, 0.0, 370.0, 1.0, 20.0, 10.1));
   gtk_range_set_adjustment(GTK_RANGE(hscale), adjustment);
   gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		      GTK_SIGNAL_FUNC(bond_parameters_colour_rotation_adjustment_changed), NULL);


   // Now the "C only" checkbutton:
   GtkWidget *checkbutton = lookup_widget(w, "bond_parameters_rotate_colour_map_c_only_checkbutton");
   if (rotate_colour_map_on_read_pdb_c_only_flag) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), TRUE);
   }


   // Now the tricky bit, fill the scrolled vbox of molecule colour rotation step sliders:

   GtkWidget *frame_molecule_N;
   GtkWidget *coords_colour_control_dialog = w;
   GtkWidget *coords_colours_vbox = lookup_widget(w, "coords_colours_vbox");
   GtkWidget *hbox136;
   GtkWidget *label269;
   GtkWidget *label270;
   GtkWidget *coords_colour_hscale_mol_N;
   
   for (int imol=0; imol<n_molecules(); imol++) {
      if (molecules[imol].has_model()) { 

	 std::string m = "Molecule ";
	 m += coot::util::int_to_string(imol);
	 m += " ";
	 m += molecules[imol].name_for_display_manager();
	 frame_molecule_N = gtk_frame_new (m.c_str());
	 gtk_widget_ref (frame_molecule_N);
	 gtk_object_set_data_full (GTK_OBJECT (coords_colour_control_dialog),
				   "frame_molecule_N", frame_molecule_N,
				   (GtkDestroyNotify) gtk_widget_unref);
	 gtk_box_pack_start (GTK_BOX (coords_colours_vbox), frame_molecule_N, TRUE, TRUE, 0);
	 gtk_widget_set_usize (frame_molecule_N, 171, -2);
	 gtk_container_set_border_width (GTK_CONTAINER (frame_molecule_N), 6);

	 hbox136 = gtk_hbox_new (FALSE, 0);
	 gtk_widget_ref (hbox136);
	 gtk_object_set_data_full (GTK_OBJECT (coords_colour_control_dialog), "hbox136", hbox136,
				   (GtkDestroyNotify) gtk_widget_unref);
	 gtk_widget_show (hbox136);
	 gtk_container_add (GTK_CONTAINER (frame_molecule_N), hbox136);

	 label269 = gtk_label_new (_("    "));
	 gtk_widget_ref (label269);
	 gtk_object_set_data_full (GTK_OBJECT (coords_colour_control_dialog), "label269", label269,
				   (GtkDestroyNotify) gtk_widget_unref);
	 gtk_widget_show (label269);
	 gtk_box_pack_start (GTK_BOX (hbox136), label269, FALSE, FALSE, 0);

	 GtkAdjustment *adjustment_mol = GTK_ADJUSTMENT
	    (gtk_adjustment_new(molecules[imol].bonds_colour_map_rotation,
				0.0, 370.0, 1.0, 20.0, 10.1));
	 coords_colour_hscale_mol_N = gtk_hscale_new (adjustment_mol);
	 gtk_range_set_adjustment(GTK_RANGE(coords_colour_hscale_mol_N), adjustment_mol);
	 gtk_signal_connect(GTK_OBJECT(adjustment_mol), "value_changed",
			    GTK_SIGNAL_FUNC(bonds_colour_rotation_adjustment_changed), NULL);
	 gtk_object_set_user_data(GTK_OBJECT(adjustment_mol), GINT_TO_POINTER(imol));
	 
	 gtk_widget_ref (coords_colour_hscale_mol_N);
	 gtk_object_set_data_full (GTK_OBJECT (coords_colour_control_dialog),
				   "coords_colour_hscale_mol_N",
				   coords_colour_hscale_mol_N,
				   (GtkDestroyNotify) gtk_widget_unref);
	 gtk_widget_show (coords_colour_hscale_mol_N);
	 gtk_box_pack_start (GTK_BOX (hbox136), coords_colour_hscale_mol_N, TRUE, TRUE, 0);

	 label270 = gtk_label_new (_("  degrees  "));
	 gtk_widget_ref (label270);
	 gtk_object_set_data_full (GTK_OBJECT (coords_colour_control_dialog), "label270", label270,
				   (GtkDestroyNotify) gtk_widget_unref);
	 gtk_widget_show (label270);
	 gtk_box_pack_start (GTK_BOX (hbox136), label270, FALSE, FALSE, 0);
	 gtk_misc_set_alignment (GTK_MISC (label270), 0.5, 0.56);

	 gtk_widget_show(frame_molecule_N);
      }
   }
   
}

// static 
void graphics_info_t::bond_parameters_colour_rotation_adjustment_changed(GtkAdjustment *adj,
									 GtkWidget *window) {

   graphics_info_t g;
   g.rotate_colour_map_on_read_pdb = adj->value;
   graphics_draw(); // unnecessary.
   
}


// static 
void graphics_info_t::bonds_colour_rotation_adjustment_changed(GtkAdjustment *adj,
							       GtkWidget *window) {
   int imol = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(adj)));

   if (molecules[imol].has_model()) {
      molecules[imol].bonds_colour_map_rotation = adj->value;
   }
   graphics_draw();
   
}

// static
void
graphics_info_t::bond_width_item_select(GtkWidget *item, GtkPositionType pos) {

   graphics_info_t g;
   g.bond_thickness_intermediate_value = pos;
   if (g.bond_thickness_intermediate_value > 0) {
      int imol = g.bond_parameters_molecule;
      if (is_valid_model_molecule(imol)) 
	 g.set_bond_thickness(imol, g.bond_thickness_intermediate_value);
   }
}


void
graphics_info_t::fill_add_OXT_dialog_internal(GtkWidget *widget, int imol) {

   GtkWidget *chain_optionmenu = lookup_widget(widget, "add_OXT_chain_optionmenu");
//    GtkWidget *at_c_terminus_radiobutton =
//       lookup_widget(widget, "add_OXT_c_terminus_radiobutton");
   GtkSignalFunc signal_func = GTK_SIGNAL_FUNC(graphics_info_t::add_OXT_chain_menu_item_activate);

   std::string a = fill_chain_option_menu(chain_optionmenu, imol, signal_func);
   if (a != "no-chain") {
      graphics_info_t::add_OXT_chain = a;
   }
}

// static (menu item ativate callback)
void
graphics_info_t::add_OXT_chain_menu_item_activate (GtkWidget *item,
						   GtkPositionType pos) {

   char *data = NULL;
   data = (char *)pos;
   if (data)
      add_OXT_chain = std::string(data);
}

// a copy of the c-interface function which does not pass the signal
// function.  We also return the string at the top of the list:
// (return "no-chain" if it was not assigned (nothing in the list)).
//
// static
// 
std::string 
graphics_info_t::fill_chain_option_menu(GtkWidget *chain_option_menu, int imol,
					GtkSignalFunc signal_func) {

   std::string r("no-chain");

   if (imol<graphics_info_t::n_molecules()) {
      if (imol >= 0) { 
	 if (graphics_info_t::molecules[imol].has_model()) {
	    std::vector<std::string> chains = coot::util::chains_in_molecule(graphics_info_t::molecules[imol].atom_sel.mol);
	    GtkWidget *menu_item;
	    GtkWidget *menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(chain_option_menu));
	    if (menu)
	       gtk_widget_destroy(menu);
	    menu = gtk_menu_new();
	    short int first_chain_set_flag = 0;
	    std::string first_chain;
	    for (unsigned int i=0; i<chains.size(); i++) {
	       if (first_chain_set_flag == 0) {
		  first_chain_set_flag = 1;
		  first_chain = chains[i];
	       }
	       menu_item = gtk_menu_item_new_with_label(chains[i].c_str());
	       int l = chains[i].length();
	       char *v = new char[ l + 1];
	       for (int ii=0; ii<=l; ii++)
		 v[ii] = 0;
	       strncpy(v, chains[i].c_str(), l+1);
	       gtk_signal_connect(GTK_OBJECT(menu_item), "activate", signal_func, v);
	       gtk_menu_append(GTK_MENU(menu), menu_item);
	       gtk_widget_show(menu_item);
	    }
	    /* Link the new menu to the optionmenu widget */
	    gtk_option_menu_set_menu(GTK_OPTION_MENU(chain_option_menu), menu);

	    if (first_chain_set_flag) {
	       r = first_chain;
	    }
	 }
      }
   }
   return r;
} 

// static
void
graphics_info_t::add_OXT_molecule_item_select(GtkWidget *item,
					      GtkPositionType pos) {

   graphics_info_t g;
   g.add_OXT_molecule = pos;
   GtkWidget *w = lookup_widget(item, "add_OXT_dialog");
   g.fill_add_OXT_dialog_internal(w,pos);

}


// static
void
graphics_info_t::fill_renumber_residue_range_dialog(GtkWidget *window) {

   graphics_info_t g;

   GtkWidget *molecule_option_menu =
      lookup_widget(window, "renumber_residue_range_molecule_optionmenu");
//    GtkWidget *chain_option_menu =
//       lookup_widget(window, "renumber_residue_range_chain_optionmenu");
   
   // renumber_residue_range_resno_1_entry
   // renumber_residue_range_resno_2_entry
   // renumber_residue_range_offset_entry

   // fill molecules option menu
   GtkSignalFunc callback_func = 
      GTK_SIGNAL_FUNC(graphics_info_t::renumber_residue_range_molecule_menu_item_select);
   g.fill_option_menu_with_coordinates_options(molecule_option_menu, callback_func);

}

void
graphics_info_t::fill_renumber_residue_range_internal(GtkWidget *w, int imol) {

   GtkWidget *chain_option_menu =
      lookup_widget(w, "renumber_residue_range_chain_optionmenu");
   GtkSignalFunc callback_func = 
      GTK_SIGNAL_FUNC(graphics_info_t::renumber_residue_range_chain_menu_item_select);
   std::string a = fill_chain_option_menu(chain_option_menu, imol, callback_func);
   if (a != "no-chain") {
      graphics_info_t::renumber_residue_range_chain = a;
   } 
}


void
graphics_info_t::renumber_residue_range_molecule_menu_item_select(GtkWidget *item,
								  GtkPositionType pos) {
   graphics_info_t::renumber_residue_range_molecule = pos;
   GtkWidget *window = lookup_widget(GTK_WIDGET(item),
				     "renumber_residue_range_dialog");
   graphics_info_t g;
   g.fill_renumber_residue_range_internal(window, pos);
}

void
graphics_info_t::renumber_residue_range_chain_menu_item_select(GtkWidget *item,
					        	       GtkPositionType pos) {

   graphics_info_t::renumber_residue_range_chain = menu_item_label(item);
}


// static
GtkWidget *
graphics_info_t::wrapped_create_diff_map_peaks_dialog(const std::vector<std::pair<clipper::Coord_orth, float> > &centres, float map_sigma) {

   GtkWidget *w = create_diff_map_peaks_dialog();
   difference_map_peaks_dialog = w; // save it for use with , and .
                                    // (globjects key press callback)
   GtkWidget *radio_button;
   GSList *diff_map_group = NULL;
   GtkWidget *button_vbox = lookup_widget(w, "diff_map_peaks_vbox");
   GtkWidget *frame;

   // for . and , synthetic clicking.
   gtk_object_set_user_data(GTK_OBJECT(w), GINT_TO_POINTER(centres.size()));

   // a cutn'paste jobby from fill_rotamer_selection_buttons().
   for (unsigned int i=0; i<centres.size(); i++) {
      std::string label = int_to_string(i+1);
      label += " ";
      label += float_to_string(centres[i].second);
      label += " (";
      label += float_to_string(centres[i].second/map_sigma);
      label += " sigma)";
      label += " at ";
      label += centres[i].first.format();
      radio_button = gtk_radio_button_new_with_label(diff_map_group,
						     label.c_str());
      std::string button_name = "difference_map_peaks_button_";
      button_name += int_to_string(i);
      
      diff_map_group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_widget_ref (radio_button);
      gtk_object_set_data_full (GTK_OBJECT (w),
				button_name.c_str(), radio_button,
				(GtkDestroyNotify) gtk_widget_unref);
      // int *iuser_data = new int;
      coot::diff_map_peak_helper_data *hd = new coot::diff_map_peak_helper_data;
      hd->ipeak = i;
      hd->pos = centres[i].first;
	 
      // *iuser_data = i;
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  GTK_SIGNAL_FUNC (on_diff_map_peak_button_selection_toggled),
			  hd);
             
       gtk_widget_show (radio_button);
       frame = gtk_frame_new(NULL);
       gtk_container_add(GTK_CONTAINER(frame), radio_button);
       gtk_box_pack_start (GTK_BOX (button_vbox),
			   frame, FALSE, FALSE, 0);
       gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
       gtk_widget_show(frame);
   }

   // not used in the callback now that the button contains a pointer
   // to this info:
   diff_map_peaks->resize(0);
   for (unsigned int i=0; i<centres.size(); i++) 
      diff_map_peaks->push_back(centres[i].first);
   max_diff_map_peaks = centres.size();

   if (centres.size() > 0) {
      graphics_info_t g;
      coot::Cartesian c(centres[0].first.x(), centres[0].first.y(), centres[0].first.z());
      g.setRotationCentre(c);
      for(int ii=0; ii<n_molecules(); ii++) {
	 molecules[ii].update_map();
	 molecules[ii].update_symmetry();
      }
      graphics_draw();
   }
   return w;
}


// static 
void
graphics_info_t::on_diff_map_peak_button_selection_toggled (GtkButton       *button,
							    gpointer         user_data) {

   coot::diff_map_peak_helper_data *hd = (coot::diff_map_peak_helper_data *) user_data;
   // int i = hd->ipeak;
   
   graphics_info_t g;
   // std::cout << "button number " << i << " pressed\n";
   if (GTK_TOGGLE_BUTTON(button)->active) { 
      // std::cout << "button number " << i << " was active\n";
      coot::Cartesian c(hd->pos.x(), hd->pos.y(), hd->pos.z());
      g.setRotationCentre(c);
      for(int ii=0; ii<n_molecules(); ii++) {
	 molecules[ii].update_map();
	 molecules[ii].update_symmetry();
      }
      g.make_pointer_distance_objects();
      graphics_draw();
      std::string s = "Difference map peak number ";
      s += int_to_string(hd->ipeak);
      g.statusbar_text(s);
   } 
}


// static 
std::pair<short int, float>
graphics_info_t::float_from_entry(GtkWidget *entry) {

   std::pair<short int, float> p(0,0);
   const gchar *txt = gtk_entry_get_text(GTK_ENTRY(entry));
   if (txt) {
      float f = atof(txt);
      p.second = f;
      p.first = 1;
   }
   return p;
}

std::pair<short int, int>
graphics_info_t::int_from_entry(GtkWidget *entry) {

   std::pair<short int, int> p(0,0);
   const gchar *txt = gtk_entry_get_text(GTK_ENTRY(entry));
   if (txt) {
      int i = atoi(txt);
      p.second = i;
      p.first = 1;
   }
   return p;
}


// symmetry control dialog:
GtkWidget *graphics_info_t::wrapped_create_symmetry_controller_dialog() const {

   GtkWidget *w = symmetry_controller_dialog;
   if (! w) { 
      w = create_symmetry_controller_dialog();
      symmetry_controller_dialog = w;
      for (int imol=0; imol<n_molecules(); imol++) {
	 if (molecules[imol].has_model())
	    molecules[imol].fill_symmetry_control_frame(w);
      }
   }
#if (GTK_MAJOR_VERSION > 1)
   gtk_window_deiconify(GTK_WINDOW(w));
#endif    
   return w;
}



GtkWidget *
graphics_info_t::wrapped_create_lsq_plane_dialog() {

   GtkWidget *w = create_lsq_plane_dialog();
   pick_cursor_maybe();
   lsq_plane_dialog = w;
   GtkWindow *main_window = GTK_WINDOW(lookup_widget(glarea, "window1"));
   gtk_window_set_transient_for(GTK_WINDOW(w), main_window);
   
   return w;
} 
