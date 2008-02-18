/* src/graphics-info-preferences.cc
 * 
 * Copyright 2008 The University of York
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

#if defined _MSC_VER
#include <windows.h>
#endif

#include <fstream>

#include <gtk/gtk.h>
#include "interface.h"
#include "graphics-info.h"
#include "c-interface.h"
#include "cc-interface.hh"
#include "c-interface-scm.hh"
#include "c-inner-main.h"
#include "coot-preferences.h"


// Return succes status
// il - interface language
//  2 - python
//  1 - guile
short int
graphics_info_t::save_preference_file(const std::string &filename, short int il) {

   short int istat = 1;
   //std::cout << "DEBUG:: writing preferences" << std::endl;
   std::vector<std::string> commands;

   std::string comment_str;
   if (il == 1) { 
      comment_str = "; These commands are the preferences of coot.  You can evaluate them\n";
      comment_str += "; using \"Calculate->Run Script...\".";
   } else { 
      comment_str = "# These commands are the preferences of coot.  You can evaluate them\n";
      comment_str += "# using \"Calculate->Run Script...\".";
   }
   commands.push_back(comment_str);
   
   unsigned short int v = 4;
   int preference_type;
   float fval1;
   float fval2;
   float fval3;
   graphics_info_t g;
   for (int i=0; i<g.preferences_internal.size(); i++) {
     preference_type = g.preferences_internal[i].preference_type;
     switch (preference_type) {

     case PREFERENCES_FILE_CHOOSER:
       commands.push_back(state_command("set-file-chooser-selector",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_FILE_OVERWRITE:
       commands.push_back(state_command("set-file-chooser-overwrite",
					g.preferences_internal[i].ivalue, il));
       break;
       
     case PREFERENCES_FILE_FILTER:
       commands.push_back(state_command("set-filter-fileselection-filenames", 
					g.preferences_internal[i].ivalue, il));
       break;
       
     case PREFERENCES_FILE_SORT_DATE:
       if (g.preferences_internal[i].ivalue == 1) {
	 commands.push_back(state_command("set-sticky-sort-by-date", il));
       } else {
	 commands.push_back(state_command("unset-sticky-sort-by-date", il));
       }
       break;
       
     case PREFERENCES_ACCEPT_DIALOG_DOCKED:
       commands.push_back(state_command("set-accept-reject-dialog-docked",
					g.preferences_internal[i].ivalue, il));
       break;
       
     case PREFERENCES_IMMEDIATE_REPLACEMENT:
       commands.push_back(state_command("set-refinement-immediate-replacement",
					g.preferences_internal[i].ivalue, il));
      break;
      
     case PREFERENCES_VT_SURFACE:
       commands.push_back(state_command("vt-surface",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_RECENTRE_PDB:
       commands.push_back(state_command("set-recentre-on-read-pdb",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_BONDS_THICKNESS:
       commands.push_back(state_command("set-default-bond-thickness",
					g.preferences_internal[i].ivalue, il));
       break;
       
     case PREFERENCES_BOND_COLOURS_MAP_ROTATION:
       commands.push_back(state_command("set-colour-map-rotation-on-read-pdb",
					g.preferences_internal[i].fvalue1, il));
       break;
       
     case PREFERENCES_BOND_COLOUR_ROTATION_C_ONLY:
       commands.push_back(state_command("set-colour-map-rotation-on-read-pdb-c-only-flag",
					g.preferences_internal[i].ivalue, il));
       break;
       
     case PREFERENCES_MAP_RADIUS:
       commands.push_back(state_command("set-map-radius",
					 g.preferences_internal[i].fvalue1, il));
       break;
       
     case PREFERENCES_MAP_ISOLEVEL_INCREMENT:
       commands.push_back(state_command("set-iso-level-increment",
					 g.preferences_internal[i].fvalue1, il, v));
       break;

     case PREFERENCES_DIFF_MAP_ISOLEVEL_INCREMENT:
       commands.push_back(state_command("set-diff-map-iso-level-increment",
					 g.preferences_internal[i].fvalue1, il, v));
       break;

     case PREFERENCES_MAP_SAMPLING_RATE:
       commands.push_back(state_command("set-map-sampling-rate",
					 g.preferences_internal[i].fvalue1, il, v));
       break;

     case PREFERENCES_DYNAMIC_MAP_SAMPLING:
       if (g.preferences_internal[i].ivalue == 1) {
	 commands.push_back(state_command("set-dynamic-map-sampling-on", il));
       } else {
	 commands.push_back(state_command("set-dynamic-map-sampling-off", il));
       }
       break;
      
     case PREFERENCES_DYNAMIC_MAP_SIZE_DISPLAY:
       if (g.preferences_internal[i].ivalue == 1) {
	 commands.push_back(state_command("set-dynamic-map-size-display-on", il));
       } else {
	 commands.push_back(state_command("set-dynamic-map-size-display-off", il));
       }
       break;

     case PREFERENCES_SWAP_DIFF_MAP_COLOURS:
       commands.push_back(state_command("set-swap-difference-map-colours",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_SMOOTH_SCROLL:
       commands.push_back(state_command("set-smooth-scroll-flag",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_SMOOTH_SCROLL_STEPS:
       commands.push_back(state_command("set-smooth-scroll-steps",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_SMOOTH_SCROLL_LIMIT:
       commands.push_back(state_command("set-smooth-scroll-limit",
					g.preferences_internal[i].fvalue1, il));
       break;

     case PREFERENCES_MAP_DRAG:
       commands.push_back(state_command("set-active-map-drag-flag",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_MARK_CIS_BAD:
       commands.push_back(state_command("set-mark-cis-peptides-as-bad",
					g.preferences_internal[i].ivalue, il));     
       break;

     case PREFERENCES_BG_COLOUR:      
       fval1 = g.preferences_internal[i].fvalue1;  // red
       fval2 = g.preferences_internal[i].fvalue2;  // green
       fval3 = g.preferences_internal[i].fvalue3;  // blue
       if (fval1 < 0.01 && fval2 < 0.01 && fval3 < 0.01) {
	 // black
	 commands.push_back(state_command("set-background-colour", 0, 0, 0, il));
       } else if (fval1 > 0.99 && fval2 > 0.99 && fval3 > 0.99) {
	 // white
	 commands.push_back(state_command("set-background-colour", 1, 1, 1, il));
       } else {
	 // other colour
	 commands.push_back(state_command("set-background-colour", fval1, fval2, fval3, il));
       }
       break;

     case PREFERENCES_ANTIALIAS:
       commands.push_back(state_command("set-do-anti-aliasing",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_CONSOLE_COMMANDS:
       commands.push_back(state_command("set-console-display-commands-state",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_REFINEMENT_SPEED:
       commands.push_back(state_command("set-dragged-refinement-steps-per-frame",
					g.preferences_internal[i].ivalue, il));
       break;

     case PREFERENCES_TIPS:
       if (g.preferences_internal[i].ivalue == 0) {
	 commands.push_back(state_command("no-coot-tips", il));
       }
       break;
      
     case PREFERENCES_SPIN_SPEED:
       commands.push_back(state_command("set-idle-function-rotate-angle",
					g.preferences_internal[i].fvalue1, il));
       break;
      
     case PREFERENCES_FONT_SIZE:
       commands.push_back(state_command("set-font-size",
					g.preferences_internal[i].ivalue, il));
       break;
      
     case PREFERENCES_FONT_COLOUR:      
       fval1 = g.preferences_internal[i].fvalue1;  // red
       fval2 = g.preferences_internal[i].fvalue2;  // green
       fval3 = g.preferences_internal[i].fvalue3;  // blue
       commands.push_back(state_command("set-font-colour", fval1, fval2, fval3, il));
       break;

     case PREFERENCES_PINK_POINTER:
       commands.push_back(state_command("set-rotation-centre-size",
					g.preferences_internal[i].fvalue1, il));
       break;
      
     }
   }

   std::ofstream f(filename.c_str());

   if (f) {
      for (unsigned int i=0; i<commands.size(); i++) {
	//std::cout<<"DEBUG:: commands " << commands[i] <<std::endl;
	f << commands[i] << std::endl;
      }
      f.close();
      std::cout << "Preference file " << filename << " written." << std::endl;

   } else {
      std::cout << "WARNING: couldn't write to preference file " << filename
		<< std::endl;
      istat = 0;
   } 

   return istat;
}

// makes a vector of preference_info_t (graphics_info_t::preferences_internal)
// based on current status of coot
//
void
graphics_info_t::make_preferences_internal() {

  coot::preference_info_t p;
  std::vector<coot::preference_info_t> ret;
  
  int on;
  float fvalue;
  // General preference settings
  // file chooser
#if (GTK_MAJOR_VERSION > 1)
  on = file_chooser_selector_state();
#else
  on = 0;
#endif // GTK
  p.preference_type = PREFERENCES_FILE_CHOOSER;
  p.ivalue = on;
  ret.push_back(p);
  
#if (GTK_MAJOR_VERSION > 1)
  on = file_chooser_overwrite_state();
#else
  on = 0;
#endif // GTK
  p.preference_type = PREFERENCES_FILE_OVERWRITE;
  p.ivalue = on;
  ret.push_back(p);
 
  on = filter_fileselection_filenames_state();
  p.preference_type = PREFERENCES_FILE_FILTER;
  p.ivalue = on;
  ret.push_back(p);
 
  on = graphics_info_t::sticky_sort_by_date;
  p.preference_type = PREFERENCES_FILE_SORT_DATE;
  p.ivalue = on;
  ret.push_back(p);
 
  // dialogs
  on = accept_reject_dialog_docked_state();
  p.preference_type = PREFERENCES_ACCEPT_DIALOG_DOCKED;
  p.ivalue = on;
  ret.push_back(p);

  on = refinement_immediate_replacement_state();
  p.preference_type = PREFERENCES_IMMEDIATE_REPLACEMENT;
  p.ivalue = on;
  ret.push_back(p);

  // hid
  if (trackball_size > 5) {
    on = 1;
  } else {
    on = 2;
  }
  //  on = graphics_info_t::vt_surface_status();
  p.preference_type = PREFERENCES_VT_SURFACE;
  p.ivalue = on;
  ret.push_back(p);

  // recentre pdb
  on = recentre_on_read_pdb;
  p.preference_type = PREFERENCES_RECENTRE_PDB;
  p.ivalue = on;
  ret.push_back(p);

  // Bond preference settings
  // Bond parameters
  on = get_default_bond_thickness();
  p.preference_type = PREFERENCES_BONDS_THICKNESS;
  p.ivalue = on;
  ret.push_back(p);

  // Bond colours
  fvalue = graphics_info_t::rotate_colour_map_on_read_pdb;
  p.preference_type = PREFERENCES_BOND_COLOURS_MAP_ROTATION;
  p.fvalue1 = fvalue;
  ret.push_back(p);
   
  on = get_colour_map_rotation_on_read_pdb_c_only_flag();
  p.preference_type = PREFERENCES_BOND_COLOUR_ROTATION_C_ONLY;
  p.ivalue = on;
  ret.push_back(p);
   
  // Map preference settings
  // Map parameters
  fvalue = get_map_radius();
  p.preference_type = PREFERENCES_MAP_RADIUS;
  p.fvalue1 = fvalue;
  ret.push_back(p);

  fvalue = get_iso_level_increment();
  p.preference_type = PREFERENCES_MAP_ISOLEVEL_INCREMENT;
  p.fvalue1 = fvalue;
  ret.push_back(p);

  fvalue = get_diff_map_iso_level_increment();
  p.preference_type = PREFERENCES_DIFF_MAP_ISOLEVEL_INCREMENT;
  p.fvalue1 = fvalue;
  ret.push_back(p);

  fvalue = get_map_sampling_rate(); 
  p.preference_type = PREFERENCES_MAP_SAMPLING_RATE;
  p.fvalue1 = fvalue;
  ret.push_back(p);

  on = get_dynamic_map_sampling();
  p.preference_type = PREFERENCES_DYNAMIC_MAP_SAMPLING;
  p.ivalue = on;
  ret.push_back(p);
  
  on = get_dynamic_map_size_display();
  p.preference_type = PREFERENCES_DYNAMIC_MAP_SIZE_DISPLAY;
  p.ivalue = on;
  ret.push_back(p);

  // Map colours
  on = swap_difference_map_colours_state();
  p.preference_type = PREFERENCES_SWAP_DIFF_MAP_COLOURS;
  p.ivalue = on;
  ret.push_back(p);

  // Map smooth scroll
  on = get_smooth_scroll();
  p.preference_type = PREFERENCES_SMOOTH_SCROLL;
  p.ivalue = on;
  ret.push_back(p);

  on = graphics_info_t::smooth_scroll_steps;
  p.preference_type = PREFERENCES_SMOOTH_SCROLL_STEPS;
  p.ivalue = on;
  ret.push_back(p);

  fvalue = graphics_info_t::smooth_scroll_limit;
  p.preference_type = PREFERENCES_SMOOTH_SCROLL_LIMIT;
  p.fvalue1 = fvalue;
  ret.push_back(p);

  // Dragged map
  on = get_active_map_drag_flag();
  p.preference_type = PREFERENCES_MAP_DRAG;
  p.ivalue = on;
  ret.push_back(p);

  // Geometry preference settings
  // Cis peptides
  on = show_mark_cis_peptides_as_bad_state();
  p.preference_type = PREFERENCES_MARK_CIS_BAD;
  p.ivalue = on;
  ret.push_back(p);
  
  // Colour preference settings
  // Background colour
  p.preference_type = PREFERENCES_BG_COLOUR;
  p.fvalue1 = graphics_info_t::background_colour[0];
  p.fvalue2 = graphics_info_t::background_colour[1];
  p.fvalue3 = graphics_info_t::background_colour[2];
  ret.push_back(p);

  // Others preference settings
  // Antialias
  p.preference_type = PREFERENCES_ANTIALIAS;
  p.ivalue = do_anti_aliasing_state();
  ret.push_back(p);

  // Console
  p.preference_type = PREFERENCES_CONSOLE_COMMANDS;
  p.ivalue = graphics_info_t::console_display_commands.display_commands_flag;
  ret.push_back(p);

  // Tips
  p.preference_type = PREFERENCES_TIPS;
  p.ivalue = graphics_info_t::do_tip_of_the_day_flag;
  ret.push_back(p);

  // Speed
  on = graphics_info_t::dragged_refinement_steps_per_frame;
  p.preference_type = PREFERENCES_REFINEMENT_SPEED;
  p.ivalue = on;
  ret.push_back(p);

  fvalue = graphics_info_t::idle_function_rotate_angle;
  p.preference_type = PREFERENCES_SPIN_SPEED;
  p.fvalue1 = fvalue;
  ret.push_back(p);

  // Fonts
  // Font size
  on = graphics_info_t::atom_label_font_size;
  p.preference_type = PREFERENCES_FONT_SIZE;
  p.ivalue = on;
  ret.push_back(p);

  // Font colour
  p.preference_type = PREFERENCES_FONT_COLOUR;
  p.fvalue1 = graphics_info_t::font_colour.red;
  p.fvalue2 = graphics_info_t::font_colour.green;
  p.fvalue3 = graphics_info_t::font_colour.blue;
  ret.push_back(p);

  // Pink pointer size
  fvalue = graphics_info_t::rotation_centre_cube_size;
  p.preference_type = PREFERENCES_PINK_POINTER;
  p.fvalue1 = fvalue;
  ret.push_back(p);

  graphics_info_t::preferences_internal = ret;
  //graphics_info_t::preferences_internal->assign(ret.begin(), ret.end());
  //return ret;

}

void
graphics_info_t::preferences_internal_change_value(int preference_type, int ivalue){

  for (int i=0; i<preferences_internal.size(); i++) {
    if (preferences_internal[i].preference_type == preference_type) {
      preferences_internal[i].ivalue = ivalue;
      break;
    }
  }
}

void
graphics_info_t::preferences_internal_change_value(int preference_type, float fvalue){

  for (int i=0; i<preferences_internal.size(); i++) {
    if (preferences_internal[i].preference_type == preference_type) {
      preferences_internal[i].fvalue1 = fvalue;
      break;
    }
  }
}

void
graphics_info_t::preferences_internal_change_value(int preference_type, 
						   float fvalue1, float fvalue2, float fvalue3){

  for (int i=0; i<preferences_internal.size(); i++) {
    if (preferences_internal[i].preference_type == preference_type) {
      preferences_internal[i].fvalue1 = fvalue1;
      preferences_internal[i].fvalue2 = fvalue2;
      preferences_internal[i].fvalue3 = fvalue3;
      break;
    }
  }
}
