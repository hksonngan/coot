/* src/c-interface-preferences.cc
 * 
 * Copyright 2005, 2006 The University of York
 * Author: Paul Emsley
 * Copyright 2007 The University of Oxford
 * Author: Paul Emsley, Bernhard Lohkamp
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifdef USE_PYTHON
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#include "compat/coot-sysdep.h"


#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif // HAVE_VECTOR

#ifndef HAVE_STRING
#define HAVE_STRING
#include <string>
#endif // HAVE_STRING

#include <algorithm>

#include <string.h> // strlen, strcpy
#include <sys/types.h> // for stating
#include <sys/stat.h>
#if !defined _MSC_VER
#include <unistd.h>
#else
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IXUSR S_IEXEC
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#include <windows.h>
#include <direct.h>
#endif // _MSC_VER

#include <dirent.h>   // for extra scheme dir


#include "guile-fixups.h"

#include "graphics-info.h"
#include "interface.h"
#include "c-interface.h"
#include "cc-interface.hh"
#include "coot-preferences.h"

void preferences() {

  GtkWidget *w;
  show_preferences();

  update_preference_gui();

}

void show_preferences(){

  GtkWidget *w = create_preferences();
  GtkWidget *scrolled_win_model_toolbar = lookup_widget(w, "preferences_model_toolbar_icons_scrolledwindow");
  fill_preferences_model_toolbar_icons(w, scrolled_win_model_toolbar);
  GtkWidget *scrolled_win_main_toolbar = lookup_widget(w, "preferences_main_toolbar_icons_scrolledwindow");
  fill_preferences_main_toolbar_icons(w, scrolled_win_main_toolbar);
  gtk_widget_show(w);
  graphics_info_t::preferences_widget = w;

#if (GTK_MAJOR_VERSION > 1)
  GtkComboBox *combobox;
  // fill the bond combobox
  combobox = GTK_COMBO_BOX(lookup_widget(w, "preferences_bond_width_combobox"));
  for (int j=1; j<21; j++) {
    std::string s = graphics_info_t::int_to_string(j);
    gtk_combo_box_append_text(combobox, s.c_str());
  }
  // fill the font combobox
  combobox = GTK_COMBO_BOX(lookup_widget(w, "preferences_font_size_combobox"));
  std::vector<std::string> fonts;  
  fonts.push_back("Times Roman 10");
  fonts.push_back("Times Roman 24");
  fonts.push_back("Fixed 8/13");
  fonts.push_back("Fixed 9/15");
  for (int j=0; j<fonts.size(); j++) {
    gtk_combo_box_append_text(combobox, fonts[j].c_str());
  }
#endif

}

void clear_preferences() {

   graphics_info_t::preferences_widget = NULL;

}

void set_mark_cis_peptides_as_bad(int istate) {

   graphics_info_t::mark_cis_peptides_as_bad_flag = istate;
}

int show_mark_cis_peptides_as_bad_state() {

   return graphics_info_t::mark_cis_peptides_as_bad_flag;

}

#if (GTK_MAJOR_VERSION > 1)
void show_hide_preferences_tabs(GtkToggleToolButton *toggletoolbutton, int preference_type) {

  GtkWidget *frame;

  std::vector<std::string> preferences_tabs;
  
  if (preference_type == COOT_GENERAL_PREFERENCES) {
    preferences_tabs = *graphics_info_t::preferences_general_tabs;
  }
  if (preference_type == COOT_BOND_PREFERENCES) {
    preferences_tabs = *graphics_info_t::preferences_bond_tabs;
  }
  if (preference_type == COOT_GEOMETRY_PREFERENCES) {
    preferences_tabs = *graphics_info_t::preferences_geometry_tabs;
  }
  if (preference_type == COOT_COLOUR_PREFERENCES) {
    preferences_tabs = *graphics_info_t::preferences_colour_tabs;
  }
  if (preference_type == COOT_MAP_PREFERENCES) {
    preferences_tabs = *graphics_info_t::preferences_map_tabs;
  }
  if (preference_type == COOT_OTHER_PREFERENCES) {
    preferences_tabs = *graphics_info_t::preferences_other_tabs;
  }

  for (int i=0; i<preferences_tabs.size(); i++) {
    frame = lookup_widget(GTK_WIDGET(toggletoolbutton), preferences_tabs[i].c_str());
    if (gtk_toggle_tool_button_get_active(toggletoolbutton)){
	  gtk_widget_show(frame);
	} else {
	  gtk_widget_hide(frame);
	}
  }

}
#endif // GTK_MAJOR_VERSION

void make_preferences_internal() {
  
  graphics_info_t g;
  g.make_preferences_internal();
}

void make_preferences_internal_default() {

  make_preferences_internal();
  graphics_info_t g;
  g.preferences_internal_default = g.preferences_internal;

}

void reset_preferences() {

  graphics_info_t g;
  //std::vector<coot::preference_info_t> *ret = g.preferences_internal_default;
  g.preferences_internal = g.preferences_internal_default;
  update_preference_gui();

}


// update and populate Preferences GUI according to (preference file) setting
void update_preference_gui() {

  GtkWidget *dialog;
  GtkWidget *w;
  GtkWidget *colour_button;
  GtkAdjustment *adjustment;
  GtkWidget *entry;
  const gchar *gtext;
  std::string text;
  int preference_type;
  int ivalue;
  int ivalue2;
  float fval1;
  float fval2;
  float fval3;
  std::vector<int> ivector;
  unsigned short int v = 4;
  graphics_info_t g;
  
  dialog = g.preferences_widget;

  for (int i=0; i<g.preferences_internal.size(); i++) {
    preference_type = g.preferences_internal[i].preference_type;
    //std::cout <<"BL DEBUG:: type "<< preference_type<<" int " <<g.preferences_internal[i].ivalue << " int default " << g.preferences_internal[i].ivalue<<std::endl;

    switch (preference_type) {
      
    case PREFERENCES_FILE_CHOOSER:
      w = lookup_widget(dialog, "preferences_filechooser_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_filechooser_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_FILE_OVERWRITE:
      w = lookup_widget(dialog, "preferences_file_overwrite_yes_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_file_overwrite_no_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_FILE_FILTER:
      w = lookup_widget(dialog, "preferences_file_filter_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_file_filter_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_FILE_SORT_DATE:
      w = lookup_widget(dialog, "preferences_file_sort_by_date_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_file_sort_by_date_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_ACCEPT_DIALOG_DOCKED:
      w = lookup_widget(dialog, "preferences_dialog_accept_docked_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_dialog_accept_detouched_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_ACCEPT_DIALOG_DOCKED_SHOW:
      w = lookup_widget(dialog, "preferences_dialog_accept_docked_show_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_dialog_accept_docked_hide_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_IMMEDIATE_REPLACEMENT:
      w = lookup_widget(dialog, "preferences_dialog_accept_off_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_dialog_accept_on_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_VT_SURFACE:
      w = lookup_widget(dialog, "preferences_hid_spherical_radiobutton");
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue == 2) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_hid_flat_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_RECENTRE_PDB:
      w = lookup_widget(dialog, "preferences_recentre_pdb_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_recentre_pdb_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;      

    case PREFERENCES_BONDS_THICKNESS:
      w = lookup_widget(dialog, "preferences_bond_width_combobox");
      ivalue = g.preferences_internal[i].ivalue1;
      ivalue -= 1;      // offset
#if (GTK_MAJOR_VERSION > 1)
      gtk_combo_box_set_active(GTK_COMBO_BOX(w), ivalue);
#endif
      break;

    case PREFERENCES_BOND_COLOURS_MAP_ROTATION:
      w = lookup_widget(dialog, "preferences_bond_colours_hscale");
      fval1 = g.preferences_internal[i].fvalue1;
      adjustment = gtk_range_get_adjustment(GTK_RANGE(w));
      gtk_adjustment_set_value(adjustment, fval1);
      break;

    case PREFERENCES_BOND_COLOUR_ROTATION_C_ONLY:
      w = lookup_widget(dialog, "preferences_bond_colours_checkbutton");
      if (g.preferences_internal[i].ivalue1 == 1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
      }
      break;

    case PREFERENCES_MAP_RADIUS:
      w = lookup_widget(dialog, "preferences_map_radius_entry");
      text = graphics_info_t::float_to_string(g.preferences_internal[i].fvalue1);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_MAP_ISOLEVEL_INCREMENT:
      w = lookup_widget(dialog, "preferences_map_increment_size_entry");
      text = graphics_info_t::float_to_string_using_dec_pl(g.preferences_internal[i].fvalue1, v);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_DIFF_MAP_ISOLEVEL_INCREMENT:
      w = lookup_widget(dialog, "preferences_map_diff_increment_entry");
      text = graphics_info_t::float_to_string_using_dec_pl(g.preferences_internal[i].fvalue1, v);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_MAP_SAMPLING_RATE:
      w = lookup_widget(dialog, "preferences_map_sampling_entry");
      text = graphics_info_t::float_to_string_using_dec_pl(g.preferences_internal[i].fvalue1, v);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_DYNAMIC_MAP_SAMPLING:
      w = lookup_widget(dialog, "preferences_map_dynamic_sampling_checkbutton");
      if (g.preferences_internal[i].ivalue1 == 1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
      }
      break;

    case PREFERENCES_DYNAMIC_MAP_SIZE_DISPLAY:
      w = lookup_widget(dialog, "preferences_map_dynamic_size_checkbutton");
      if (g.preferences_internal[i].ivalue1 == 1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
      }
      break;

    case PREFERENCES_SWAP_DIFF_MAP_COLOURS:
      w = lookup_widget(dialog, "preferences_diff_map_colours_o_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_diff_map_colours_coot_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_MAP_COLOURS_MAP_ROTATION:
      w = lookup_widget(dialog, "preferences_map_colours_hscale");
      fval1 = g.preferences_internal[i].fvalue1;
      adjustment = gtk_range_get_adjustment(GTK_RANGE(w));
      gtk_adjustment_set_value(adjustment, fval1);
      break;

    case PREFERENCES_SMOOTH_SCROLL:
      w = lookup_widget(dialog, "preferences_smooth_scroll_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_smooth_scroll_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_SMOOTH_SCROLL_STEPS:
      w = lookup_widget(dialog, "preferences_smooth_scroll_steps_entry");
      text = graphics_info_t::int_to_string(g.preferences_internal[i].ivalue1);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_SMOOTH_SCROLL_LIMIT:
      w = lookup_widget(dialog, "preferences_smooth_scroll_limit_entry");
      text = graphics_info_t::float_to_string(g.preferences_internal[i].fvalue1);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_MAP_DRAG:
      w = lookup_widget(dialog, "preferences_map_drag_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_map_drag_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_MARK_CIS_BAD:
      w = lookup_widget(dialog, "preferences_geometry_cis_peptide_bad_yes_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_geometry_cis_peptide_bad_no_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_BG_COLOUR:
      fval1 = g.preferences_internal[i].fvalue1;  // red
      fval2 = g.preferences_internal[i].fvalue2;  // green
      fval3 = g.preferences_internal[i].fvalue3;  // blue
      colour_button = lookup_widget(dialog, "preferences_bg_colour_colorbutton");
      GdkColor bg_colour;
      if (fval1 < 0.01 && fval2 < 0.01 && fval3 < 0.01) {
	// black
	w = lookup_widget(dialog, "preferences_bg_colour_black_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	bg_colour.red = 0;
	bg_colour.green = 0;
	bg_colour.blue = 0;
      } else if (fval1 > 0.99 && fval2 > 0.99 && fval3 > 0.99) {
	// white
	w = lookup_widget(dialog, "preferences_bg_colour_white_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	bg_colour.red = 65535;
	bg_colour.green = 65535;
	bg_colour.blue = 65535;
      } else {
	// other colour
#if (GTK_MAJOR_VERSION > 1)
	w = lookup_widget(dialog, "preferences_bg_colour_own_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	bg_colour.red = (guint)(fval1 * 65535);
	bg_colour.green = (guint)(fval2 * 65535);
	bg_colour.blue = (guint)(fval3 * 65535);
#endif
      }
#if (GTK_MAJOR_VERSION > 1)
      gtk_color_button_set_color(GTK_COLOR_BUTTON(colour_button), &bg_colour);
#endif
      break;

    case PREFERENCES_ANTIALIAS:
      w = lookup_widget(dialog, "preferences_antialias_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_antialias_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_CONSOLE_COMMANDS:
      w = lookup_widget(dialog, "preferences_console_info_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_console_info_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_TIPS:
      w = lookup_widget(dialog, "preferences_tips_on_radiobutton");
      if (g.preferences_internal[i].ivalue1) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_tips_off_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_REFINEMENT_SPEED:
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue == 4) {
	 w = lookup_widget(dialog, "preferences_refinement_speed_molasses_radiobutton");
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 120) {
	 w = lookup_widget(dialog, "preferences_refinement_speed_crock_radiobutton");
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 80) {
	 w = lookup_widget(dialog, "preferences_refinement_speed_default_radiobutton");
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	 w = lookup_widget(dialog, "preferences_refinement_speed_own_radiobutton");
	 entry = lookup_widget(dialog, "preferences_refinement_speed_entry");
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	 if (ivalue <= 0) {
	   std::cout<<"WARNING:: your refinement speed setting is 0 or below \nReset to default 80"<< std::endl;
	   ivalue = 80;
	 }
	 text = graphics_info_t::int_to_string(ivalue);
	 gtk_entry_set_text(GTK_ENTRY(entry), text.c_str());
      }
      break;

    case PREFERENCES_SPIN_SPEED:
      w = lookup_widget(dialog, "preferences_spin_speed_entry");
      text = graphics_info_t::float_to_string(g.preferences_internal[i].fvalue1);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_FONT_SIZE:
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue < 2) {
	w = lookup_widget(dialog, "preferences_font_size_small_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 2) {
	w = lookup_widget(dialog, "preferences_font_size_medium_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 3) {
	w = lookup_widget(dialog, "preferences_font_size_large_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_font_size_others_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	ivalue -= 4;      // offset
#if (GTK_MAJOR_VERSION > 1)
	GtkComboBox *combobox = GTK_COMBO_BOX(lookup_widget(w, "preferences_font_size_combobox"));
	gtk_combo_box_set_active(combobox, ivalue);
#endif
      }
      break;

    case PREFERENCES_FONT_COLOUR:
      fval1 = g.preferences_internal[i].fvalue1;  // red
      fval2 = g.preferences_internal[i].fvalue2;  // green
      fval3 = g.preferences_internal[i].fvalue3;  // blue
      colour_button = lookup_widget(dialog, "preferences_font_colorbutton");
      GdkColor font_colour;
      if (fval1 >= 0.999 && 
	  fval2 >= 0.799 && fval2 <= 0.801 &&
	  fval3 >= 0.799 && fval3 <= 0.801) {
	// default
	w = lookup_widget(dialog, "preferences_font_colour_default_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	font_colour.red   = (guint)(1.0 * 65535);
	font_colour.green = (guint)(0.8 * 65535);
	font_colour.blue  = (guint)(0.8 * 65535);
      } else {
	// other colour
#if (GTK_MAJOR_VERSION > 1)
	w = lookup_widget(dialog, "preferences_font_colour_own_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	font_colour.red   = (guint)(fval1 * 65535);
	font_colour.green = (guint)(fval2 * 65535);
	font_colour.blue  = (guint)(fval3 * 65535);
#endif
      }
#if (GTK_MAJOR_VERSION > 1)
      gtk_color_button_set_color(GTK_COLOR_BUTTON(colour_button), &font_colour);
#endif
      break;

    case PREFERENCES_PINK_POINTER:
      w = lookup_widget(dialog, "preferences_pink_pointer_entry");
      text = graphics_info_t::float_to_string(g.preferences_internal[i].fvalue1);
      gtk_entry_set_text(GTK_ENTRY(w), text.c_str());
      break;

    case PREFERENCES_MODEL_TOOLBAR_SHOW:
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue == 0) {
	w = lookup_widget(dialog, "preferences_model_toolbar_hide_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_model_toolbar_show_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_MODEL_TOOLBAR_POSITION:
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue == 0) {
	w = lookup_widget(dialog, "preferences_model_toolbar_right_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 1){
	w = lookup_widget(dialog, "preferences_model_toolbar_left_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 2){
	w = lookup_widget(dialog, "preferences_model_toolbar_top_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 3){
	w = lookup_widget(dialog, "preferences_model_toolbar_bottom_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_MODEL_TOOLBAR_STYLE:
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue <= 1) {
	w = lookup_widget(dialog, "preferences_model_toolbar_style_icons_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 2) {
	w = lookup_widget(dialog, "preferences_model_toolbar_style_both_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_model_toolbar_style_text_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_MODEL_TOOLBAR_ICONS:
      ivalue  = g.preferences_internal[i].ivalue1;
      ivalue2 = g.preferences_internal[i].ivalue2;
      if (ivalue2 == 1) {
	// show
	show_model_toolbar_icon(ivalue);
      } else {
	// hide
	hide_model_toolbar_icon(ivalue);
      }
      break;

    case PREFERENCES_MAIN_TOOLBAR_SHOW:
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue == 0) {
	w = lookup_widget(dialog, "preferences_main_toolbar_hide_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_main_toolbar_show_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    //case PREFERENCES_MAIN_TOOLBAR_POSITION:
    //  ivalue = g.preferences_internal[i].ivalue1;
    //  if (ivalue == 0) {
//	w = lookup_widget(dialog, "preferences_model_toolbar_right_radiobutton");
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      //} else if (ivalue == 1){
//	w = lookup_widget(dialog, "preferences_model_toolbar_left_radiobutton");
//	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
 //     } else if (ivalue == 2){
//	w = lookup_widget(dialog, "preferences_model_toolbar_top_radiobutton");
//	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
 //     } else if (ivalue == 3){
//	w = lookup_widget(dialog, "preferences_model_toolbar_bottom_radiobutton");
//	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
 //     }
  //    break;

    case PREFERENCES_MAIN_TOOLBAR_STYLE:
      ivalue = g.preferences_internal[i].ivalue1;
      if (ivalue <= 1) {
	w = lookup_widget(dialog, "preferences_main_toolbar_style_icons_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else if (ivalue == 2) {
	w = lookup_widget(dialog, "preferences_main_toolbar_style_both_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      } else {
	w = lookup_widget(dialog, "preferences_main_toolbar_style_text_radiobutton");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
      }
      break;

    case PREFERENCES_MAIN_TOOLBAR_ICONS:
      ivalue  = g.preferences_internal[i].ivalue1;
      ivalue2 = g.preferences_internal[i].ivalue2;
      if (ivalue2 == 1) {
	// show
	show_main_toolbar_icon(ivalue);
      } else {
	// hide
	hide_main_toolbar_icon(ivalue);
      }
      break;
    }
  }
}

void save_preferences() {

  graphics_info_t g;
  short int istat = 1;
  short int il;
  std::string preferences_name;
  std::string file_name;
  std::string directory = PKGDATADIR;
  
#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
  char *tmp_directory = getenv("COOT_HOME");
  if (!tmp_directory) {
    tmp_directory = getenv("HOME");
  } 
#else      
  char *tmp_directory = getenv("HOME");
#endif
  if (tmp_directory) {
    directory = tmp_directory;
  }

  directory += "/.coot-preferences/";

  int status = make_directory_maybe(directory.c_str());
  if (status != 0) {
    std::cout<<"ERROR:: Cannot find directory "<< directory <<
      "        Will not be able to save preferences"<<std::endl;
  } else {

#ifdef USE_GUILE
    preferences_name = "coot-preferences.scm";
    file_name = directory + preferences_name;
    il = 1;
    istat = g.save_preference_file(file_name, il);
#endif // USE_GUILE
#ifdef USE_PYTHON
    preferences_name = "coot_preferences.py";
    file_name = directory + preferences_name;
    il = 2;
    istat = g.save_preference_file(file_name, il);
#endif // USE_PYTHON
  }
}
 

void preferences_internal_change_value_int(int preference_type, int ivalue) {
  graphics_info_t g;
  g.preferences_internal_change_value(preference_type, ivalue);
}

void preferences_internal_change_value_int2(int preference_type, int ivalue1, int ivalue2) {
  graphics_info_t g;
  g.preferences_internal_change_value(preference_type, ivalue1, ivalue2);
}

void preferences_internal_change_value_float(int preference_type, float fvalue) {
  graphics_info_t g;
  g.preferences_internal_change_value(preference_type, fvalue);
}

void preferences_internal_change_value_float3(int preference_type,
				       float fvalue1, float fvalue2, float fvalue3) {
  graphics_info_t g;
  g.preferences_internal_change_value(preference_type, fvalue1, fvalue2, fvalue3);
}

// FIXME:: make this generic with TOOLBAR enums
void
show_model_toolbar_icon(int pos) {
  graphics_info_t g;
  g.show_hide_toolbar_icon_pos(pos, 1, MODEL_TOOLBAR);
}

void
hide_model_toolbar_icon(int pos) {
  graphics_info_t g;
  g.show_hide_toolbar_icon_pos(pos, 0, MODEL_TOOLBAR);
}

void
show_main_toolbar_icon(int pos) {
  graphics_info_t g;
  g.show_hide_toolbar_icon_pos(pos, 1, MAIN_TOOLBAR);
}

void
hide_main_toolbar_icon(int pos) {
  graphics_info_t g;
  g.show_hide_toolbar_icon_pos(pos, 0, MAIN_TOOLBAR);
}

void
fill_preferences_model_toolbar_icons(GtkWidget *preferences,
				     GtkWidget *scrolled_window) {

  graphics_info_t g;
#if (GTK_MAJOR_VERSION >1)
  g.fill_preferences_model_toolbar_icons(preferences, scrolled_window);
#endif // GTK_MAJOR_VERSION
}

void
fill_preferences_main_toolbar_icons(GtkWidget *preferences,
				     GtkWidget *scrolled_window) {

  graphics_info_t g;
#if (GTK_MAJOR_VERSION >1)
  g.fill_preferences_main_toolbar_icons(preferences, scrolled_window);
#endif // GTK_MAJOR_VERSION
}
// end FIXME


GtkWidget *popup_window(const char *str) {

   GtkWidget *w = create_popup_info_window();
   GtkWidget *label = lookup_widget(w, "info_label");
   gtk_label_set_text(GTK_LABEL(label), str);
   return w;
}

void add_status_bar_text(const char *s) {

   graphics_info_t g;
   g.add_status_bar_text(std::string(s));
} 



/*  ----------------------------------------------------------------------- */
/*                  Other interface preferences                            */
/*  ----------------------------------------------------------------------- */

void set_model_fit_refine_dialog_stays_on_top(int istate) { 
   graphics_info_t::model_fit_refine_dialog_stays_on_top_flag = istate;
}

int model_fit_refine_dialog_stays_on_top_state() {

   return graphics_info_t::model_fit_refine_dialog_stays_on_top_flag;
} 


void save_accept_reject_dialog_window_position(GtkWidget *acc_rej_dialog) {

   // 20070801 crash reported by "Gajiwala, Ketan"

   // OK, we can reproduce a problem
   // Refine something
   // Close the window using WM delete window
   // Press return in Graphics window (globjects:key_press_event() GDK_Return case)
   // 
   // So, we need to set graphics_info_t::accept_reject_dialog to NULL
   // when we get a WM delete event on the Accept/Reject box
   
   if (acc_rej_dialog) { 
      gint upositionx, upositiony;
      if (acc_rej_dialog->window) {
	 gdk_window_get_root_origin (acc_rej_dialog->window, &upositionx, &upositiony);
	 graphics_info_t::accept_reject_dialog_x_position = upositionx;
	 graphics_info_t::accept_reject_dialog_y_position = upositiony;
      } else {
	 std::cout << "ERROR:: Trapped an error in save_accept_reject_dialog_window_position\n"
		   << "        Report to Central Control!\n"
		   << "        (What did you do to make this happen?)\n";
      }
   }
}

void set_accept_reject_dialog(GtkWidget *w) { /* used by callbacks to unset the widget.
						 (errr... it wasn't but it is now (as it
						 should be)). */

   graphics_info_t::accept_reject_dialog = w;
}

/* \brief set position of Model/Fit/Refine dialog */
void set_model_fit_refine_dialog_position(int x_pos, int y_pos) {

   graphics_info_t::model_fit_refine_x_position = x_pos;
   graphics_info_t::model_fit_refine_y_position = y_pos;
}

/* \brief set position of Display Control dialog */
void set_display_control_dialog_position(int x_pos, int y_pos) {

   graphics_info_t::display_manager_x_position = x_pos;
   graphics_info_t::display_manager_y_position = y_pos;
}

/* \brief set position of Go To Atom dialog */
void set_go_to_atom_window_position(int x_pos, int y_pos) {

   graphics_info_t::go_to_atom_window_x_position = x_pos;
   graphics_info_t::go_to_atom_window_y_position = y_pos;
}

/* \brief set position of Delete dialog */
void set_delete_dialog_position(int x_pos, int y_pos) {

   graphics_info_t::delete_item_widget_x_position = x_pos;
   graphics_info_t::delete_item_widget_y_position = y_pos;
}

void set_rotate_translate_dialog_position(int x_pos, int y_pos) {

   graphics_info_t::rotate_translate_x_position = x_pos;
   graphics_info_t::rotate_translate_y_position = y_pos;
}

/*! \brief set position of the Accept/Reject dialog */
void set_accept_reject_dialog_position(int x_pos, int y_pos) {
   graphics_info_t::accept_reject_dialog_x_position = x_pos;
   graphics_info_t::accept_reject_dialog_y_position = y_pos;
}

/*! \brief set position of the Ramachadran Plot dialog */
void set_ramachandran_plot_dialog_position(int x_pos, int y_pos) {
   graphics_info_t::ramachandran_plot_x_position = x_pos;
   graphics_info_t::ramachandran_plot_y_position = y_pos;
}

/*! \brief set edit chi angles dialog position */
void set_edit_chi_angles_dialog_position(int x_pos, int y_pos) {

   graphics_info_t::edit_chi_angles_dialog_x_position = x_pos;
   graphics_info_t::edit_chi_angles_dialog_y_position = y_pos;
}


/*! \brief set rotamer selection dialog position */
void set_rotamer_selection_dialog_position(int x_pos, int y_pos) {

   graphics_info_t::rotamer_selection_dialog_x_position = x_pos;
   graphics_info_t::rotamer_selection_dialog_y_position = y_pos;
} 



/*  ------------------------------------------------------------------------ */
/*                     user define clicks                                    */
/*  ------------------------------------------------------------------------ */
#ifdef USE_GUILE
void user_defined_click_scm(int n_clicks, SCM func) {
  if (n_clicks > 0) {
    graphics_info_t g;
    g.user_defined_atom_pick_specs.clear();
    g.in_user_defined_define = n_clicks;
    SCM dest = SCM_BOOL_F;
    SCM mess = scm_makfrom0str("~s");
    SCM v = scm_simple_format(dest, mess, scm_list_1(func));
    std::string func_string = scm_to_locale_string(v);
    g.user_defined_click_scm_func = func;
    g.pick_cursor_maybe();
  } else {
    std::cout<<"INFO:: number of clicks less than 1, cannot define user click"<<std::endl;
  } 
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
void user_defined_click_py(int n_clicks, PyObject *func) {
  if (n_clicks > 0) {
    graphics_info_t g;
    g.user_defined_atom_pick_specs.clear();
    g.in_user_defined_define = n_clicks;
    g.user_defined_click_py_func = func;
    Py_XINCREF(g.user_defined_click_py_func);
    g.pick_cursor_maybe();
  } else {
    std::cout<<"INFO:: number of clicks less than 1, cannot define user click"<<std::endl;
  } 
} 
#endif // USE_PYTHON
/*  ------------------------------------------------------------------------ */
/*                     state (a graphics_info thing)                         */
/*  ------------------------------------------------------------------------ */
void set_save_state_file_name(const char *filename) {
   graphics_info_t::save_state_file_name = filename; 
}

const char *save_state_file_name_raw() {
   return graphics_info_t::save_state_file_name.c_str();
}


#ifdef USE_GUILE
SCM save_state_file_name_scm() {

//    char *f = (char *) malloc(graphics_info_t::save_state_file_name.length() +1);
//    strcpy(f, graphics_info_t::save_state_file_name.c_str());
//    return f;

   std::string f = graphics_info_t::save_state_file_name;
   return scm_makfrom0str(f.c_str());
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *save_state_file_name_py() {
   std::string f = graphics_info_t::save_state_file_name;
   return PyString_FromString(f.c_str());
}
#endif // USE_PYTHON




char *unmangle_hydrogen_name(const char *pdb_hydrogen_name) {

   std::string atom_name(pdb_hydrogen_name);
   std::string new_atom_name = atom_name;

   if (atom_name.length() == 4) { 
      if (atom_name[0] == '1' ||
	  atom_name[0] == '2' ||
	  atom_name[0] == '3' ||
	  atom_name[0] == '4' ||
	  atom_name[0] == '*') {
	 // switch it.
	 if (atom_name[3] == ' ') {
	    new_atom_name = " "; // lead with a space, testing.
	    new_atom_name += atom_name.substr(1,2) + atom_name[0];
	 } else { 
	    new_atom_name = atom_name.substr(1,3) + atom_name[0];
	 }
      }
   } else { 
      if (atom_name[3] != ' ') { 
	 if (atom_name[3] == ' ') {
	    new_atom_name = atom_name.substr(1,2) + atom_name[0];
	    new_atom_name += ' ';
	 }
	 if (atom_name[2] == ' ') {
	    new_atom_name = atom_name.substr(1,1) + atom_name[0];
	 new_atom_name += ' ';
	 new_atom_name += ' ';
	 }
      } else {
	 // atom_name length is 3 presumably
	 new_atom_name = ' ';
	 new_atom_name += atom_name.substr(1,2) + atom_name[0];
      }
   }

   int new_length = strlen(pdb_hydrogen_name) + 1;
   char *s = new char[new_length];
   for (int i=0; i<new_length; i++) s[i] = 0;
   strncpy(s, new_atom_name.c_str(), new_length);

//    std::cout << "mangle debug:: :" << pdb_hydrogen_name << ": to :" << s << ":" << std::endl;
   return s;
}



short int do_probe_dots_on_rotamers_and_chis_state() {
   return graphics_info_t::do_probe_dots_on_rotamers_and_chis_flag;
} 

void set_do_probe_dots_on_rotamers_and_chis(short int state) {
   graphics_info_t::do_probe_dots_on_rotamers_and_chis_flag = state;
}

void set_do_probe_dots_post_refine(short int state) {
   graphics_info_t::do_probe_dots_post_refine_flag = state;
} 

short int do_probe_dots_post_refine_state() {
   return graphics_info_t::do_probe_dots_post_refine_flag;
}




/*! \brief return the number of generic display objects */
int number_of_generic_objects() {

   graphics_info_t g;
   return g.generic_objects_p->size();
}


void generic_objects_gui_wrapper() {

   std::vector<std::string> cmd;
   cmd.push_back("generic-objects-gui");
   graphics_info_t g;

#if defined USE_GUILE && !defined WINDOWS_MINGW

   std::string s = g.state_command(cmd, coot::STATE_SCM);
   safe_scheme_command(s);


#else
#ifdef USE_PYGTK

   std::string s = g.state_command(cmd, coot::STATE_PYTHON);
   safe_python_command(s);

#endif // USE_PYGTK

#endif // USE_GUILE

} 

/*! \brief print to the console the name and display status of the
  generic display objects */
void generic_object_info() {

   graphics_info_t g;
   unsigned int n_obs = g.generic_objects_p->size();
   std::cout << "There are " << n_obs << " generic objects\n";
   if (n_obs) {
      for (int i=0; i<n_obs; i++) {
	 std::string display_str(":Displayed:");
	 if ((*g.generic_objects_p)[i].is_displayed_flag == 0)
	    display_str = ":Not Displayed:";
	 std::string closed_str(":Closed:");
	 if ((*g.generic_objects_p)[i].is_closed_flag == 0)
	    closed_str = ":Not Closed:";
	 std::cout << " # " << i << " \"" << (*g.generic_objects_p)[i].name << "\" "
		   << display_str << " " << closed_str << std::endl;
      }
   } else {
      std::cout << "No Generic Display Objects" << std::endl;
   } 
} 

// generic object obj_no has things to display?
// Return 0 or 1.
// 
short int generic_object_has_objects_p(int object_number) {

   short int r = 0;
   graphics_info_t g;
   if ((object_number >=0) && (object_number < g.generic_objects_p->size())) {
      if ((*g.generic_objects_p)[object_number].lines_set.size() > 0)
	 r = 1;
      if ((*g.generic_objects_p)[object_number].points_set.size() > 0)
	 r = 1;
   } else {
      std::cout << "WARNING:: object_number in generic_objects_p "
		<< object_number << std::endl;
   } 

   return r;

} 

std::pair<short int, std::string>
is_interesting_dots_object_next_p(const std::vector<std::string> &vs) {

   std::pair<short int, std::string> r(0, "");

   if (vs.size() == 3) {
//       std::cout << "Looking at bits:  \n  "; 
//       for (unsigned int i=0; i<3; i++) { 
// 	 std::cout << ":" << vs[i] << ": ";
//       }
//       std::cout << "\n"; 
      if ((vs[1] == "wide") && (vs[2] == "contact)")) {
	 r.first = 1;
	 r.second = "wide contact";
      }
      if ((vs[1] == "close") && (vs[2] == "contact)")) {
	 r.first = 1;
	 r.second = "close contact";
      }
      if ((vs[1] == "small") && (vs[2] == "overlap)")) {
	 r.first = 1;
	 r.second = "small overlap";
      }
      if ((vs[1] == "bad") && (vs[2] == "overlap)")) {
	 r.first = 1;
	 r.second = "bad overlap";
      }
      if (vs[1] == "H-bonds)") { 
	 r.first = 1;
	 r.second = "H-bonds";
      }
   }
   return r;
}

std::string probe_dots_short_contact_name_to_expanded_name(const std::string &short_name) {

   std::vector<std::pair<std::string, std::string> > names;
   names.push_back(std::pair<std::string, std::string>("wc", "wide contact"));
   names.push_back(std::pair<std::string, std::string>("cc", "close contact"));
   names.push_back(std::pair<std::string, std::string>("so", "small overlap"));
   names.push_back(std::pair<std::string, std::string>("bo", "bad overlap"));
   names.push_back(std::pair<std::string, std::string>("hb", "H-bonds"));

   std::string r = "unknown";
   for (int i=0; i<5; i++) {
      if (names[i].first == short_name) {
	 r = names[i].second;
	 break;
      }
   } 
   return r;
} 



/*! \brief close generic object, clear the lines/points etc, not
  available for buttons/displaying etc */
void close_generic_object(int object_number) {

   graphics_info_t g;
   if (object_number >=0) {
      if (object_number < int(g.generic_objects_p->size())) {
	 (*g.generic_objects_p)[object_number].close_yourself();
      }
   }
} 

/*! \brief has the generic object been closed? 

   @return 1 for yes, 0 othersize
*/
short int is_closed_generic_object_p(int object_number) {

   short int state = 0;
   graphics_info_t g;
   if (object_number >=0) { 
      if (object_number < int(g.generic_objects_p->size())) {
	 state = (*g.generic_objects_p)[object_number].is_closed_flag;
      }
   }
   return state;
}

void set_display_generic_objects_as_solid(int state) {
   graphics_info_t::display_generic_objects_as_solid_flag = state;
} 



// This is tedious and irritating to parse in C++.
// 
// a const when a member function
// 
// Note that the filenames have a trailing "/".
// 
std::vector<std::pair<std::string, std::string> > 
parse_ccp4i_defs(const std::string &filename) {

   std::vector<std::pair<std::string, std::string> > v;

   // put the current directory in, whether or not we can find the
   // ccp4 project dir
   // on Windows (without mingw or cygwin) there is no PWD,
   // so we set it to "", should work
   char *pwd = getenv("PWD");
   if (pwd) {
      v.push_back(std::pair<std::string, std::string> (std::string(" - Current Dir - "),
						       std::string(pwd) + "/"));
   } else {
#ifdef WINDOWS_MINGW
      v.push_back(std::pair<std::string, std::string> (std::string(" - Current Dir - "),
						       std::string("")));  
#endif // MINGW
   }
   
   struct stat buf;
   int stat_status = stat(filename.c_str(), &buf);
   if (stat_status != 0) {
     // silently return nothing if we can't find the file.
     return v;
   } 

   std::ifstream cin(filename.c_str());

   // Let's also add ccp4_scratch to the list if the environment
   // variable is declared and if directory exists
   char *scratch = getenv("CCP4_SCR");
   if (scratch) {
      struct stat buf;
      // in Windows stat needs to have a last / or \ removed, if existent
#ifdef WINDOWS_MINGW
      if (scratch[strlen(scratch) - 1] == '/') {
	scratch[strlen(scratch) - 1] = '\0';
      }
      if (scratch[strlen(scratch) - 1] == '\\') {
	scratch[strlen(scratch) - 1] = '\0';
      }
#endif // MINGW
      int istat_scratch = stat(scratch, &buf);
      if (istat_scratch == 0) {
	 if (S_ISDIR(buf.st_mode)) {
	    v.push_back(std::pair<std::string, std::string>(std::string("CCP4_SCR"),
							    std::string(scratch) + "/"));
	 }
      }
   }

   if (! cin) {
      std::cout << "WARNING:: failed to open " << filename << std::endl;
   } else {
      // std::string s;
      char s[1000];
      std::vector <coot::alias_path_t> alias;
      std::vector <coot::alias_path_t> path;
      std::string::size_type ipath;
      std::string::size_type ialias;
      int index = -1;
      int icomma;
      short int path_coming = 0;
      short int alias_coming = 0;
      bool alias_flag = 0;
      while (! cin.eof()) {
	 cin >> s;
	 std::string ss(s);
	 // std::cout << "parsing:" << ss << std::endl;
	 if (path_coming == 2) {
	    path.push_back(coot::alias_path_t(index, ss, alias_flag));
	    path_coming = 0;
	 }
	 if (alias_coming == 2) {
	    alias.push_back(coot::alias_path_t(index, ss, alias_flag));
	    alias_coming = 0;
	 }
	 if ( path_coming == 1)  path_coming++;
	 if (alias_coming == 1) alias_coming++;
	 ipath  = ss.find("PROJECT_PATH,");
	 ialias = ss.find("PROJECT_ALIAS,");
	 if (ipath != std::string::npos) {
	    // std::cout << "DEBUG::  found a project path..." << std::endl;
	    path_coming = 1;
	    alias_flag = 0;
	    icomma = ss.find_last_of(",");
	    // std::cout << icomma << " " << ss.length() << std::endl;
	    if ( (icomma+1) < int(ss.length())) {
	       index = atoi(ss.substr(icomma+1, ss.length()).c_str());
	       // std::cout << "index: " << index << std::endl;
	    }
	 }
	 if (ialias != std::string::npos) {
	    alias_coming = 1;
	    alias_flag = 0;
	    icomma = ss.find_last_of(",");
	    if ( (icomma+1) < int(ss.length())) {
	       index = atoi(ss.substr(icomma+1, ss.length()).c_str());
	    }
	 }

	 // Things called ALIASES at at the CCP4 top level are
	 // actually speciified by DEF_DIR_PATH and DEF_DIR_ALIAS 
	 // in the same way that PROJECT_ALIAS and PROJECT_PATH work.
	 //

	 ipath  = ss.find("DEF_DIR_PATH,");
	 ialias = ss.find("DEF_DIR_ALIAS,");
	 if (ipath != std::string::npos) {
	    // std::cout << "DEBUG::  found an ALIAS path..." << ss << std::endl;
	    path_coming = 1;
	    alias_flag = 1;
	    icomma = ss.find_last_of(",");
	    // std::cout << icomma << " " << ss.length() << std::endl;
	    if ( (icomma+1) < int(ss.length())) {
	       index = atoi(ss.substr(icomma+1, ss.length()).c_str());
	       // std::cout << "index: " << index << std::endl;
	    }
	 }
	 if (ialias != std::string::npos) {
	    alias_coming = 1;
	    // std::cout << "DEBUG::  found an ALIAS name..." << ss << std::endl;
	    icomma = ss.find_last_of(",");
	    if ( (icomma+1) < int(ss.length())) {
	       index = atoi(ss.substr(icomma+1, ss.length()).c_str());
	    }
	 }
	 
      }

//       std::cout << "----------- path pairs: ------------" << std::endl;
//       for (int i=0; i<path.size(); i++) {
//   	 std::cout << path[i].index << "  " << path[i].s << " " << path[i].flag << std::endl;
//       }
//       std::cout << "----------- alias pairs: ------------" << std::endl;
//       for (int i=0; i<alias.size(); i++)
//   	 std::cout << alias[i].index << "  " << alias[i].s << " " << alias[i].flag << std::endl;
//       std::cout << "-------------------------------------" << std::endl;
      

      std::string alias_str;
      std::string path_str;
      for (unsigned int j=0; j<alias.size(); j++) {
	 for (unsigned int i=0; i<path.size(); i++) {
	    if (path[i].index == alias[j].index) {
	       if (path[i].flag == alias[j].flag) {
		  // check for "" "" pair here.
		  alias_str = alias[j].s;
		  path_str  = path[i].s;
		  // if the file is a directory, we need to put a "/" at the
		  // end so that went we set that filename in the fileselection
		  // widget, we go into the directory, rather than being in the
		  // directory above with the tail as the selected file.
		  //
		  struct stat buf;
		  int status = stat(path_str.c_str(), &buf);
	       
		  // valgrind says that buf.st_mode is uninitialised here
		  // strangely.  Perhaps we should first test for status?
		  // Yes - that was it.  I was using S_ISDIR() on a file
		  // that didn't exist.  Now we skip if the file does not
		  // exist or is not a directory.

		  // std::cout << "stating "<< path_str << std::endl;

		  if (status == 0) { 
		     if (S_ISDIR(buf.st_mode)) {
			path_str += "/";

			if (alias_str == "\"\"") {
			   alias_str = "";
			   path_str  = "";
			}
			v.push_back(std::pair<std::string, std::string> (alias_str, path_str));
		     }
		     // } else { 
		     // // This is too boring to see every time we open a file selection
		     // std::cout << "INFO:: directory for a CCP4i project: " 
		     // << path_str << " was not found\n";
		  }
	       }
	    }
	 }
      }
   }
   return v;
}

std::string
ccp4_project_directory(const std::string &ccp4_project_name) {

   std::string ccp4_defs_file_name = graphics_info_t::ccp4_defs_file_name();
   std::vector<std::pair<std::string, std::string> > v = 
      parse_ccp4i_defs(ccp4_defs_file_name);
   std::string r = "";
   for (unsigned int i=0; i<v.size(); i++) {
      if (v[i].first == ccp4_project_name) {
	 r = v[i].second;
	 break;
      }
   }
   return r;
}


/* movies */
void set_movie_file_name_prefix(const char *file_name) {
   graphics_info_t::movie_file_prefix = file_name;
}

void set_movie_frame_number(int frame_number) {
   graphics_info_t::movie_frame_number = frame_number;
}

#ifdef USE_GUILE
SCM movie_file_name_prefix() {
   SCM r = scm_makfrom0str(graphics_info_t::movie_file_prefix.c_str());
   return r;
}
#endif
#ifdef USE_PYTHON
PyObject * movie_file_name_prefix_py() {
   PyObject *r;
   r = PyString_FromString(graphics_info_t::movie_file_prefix.c_str());
   return r;
}
#endif // PYTHON

int movie_frame_number() {
   return graphics_info_t::movie_frame_number;
}

void set_make_movie_mode(int make_movie_flag) {
   graphics_info_t::make_movie_flag = make_movie_flag;
}


#ifdef USE_GUILE
void try_load_scheme_extras_dir() {

   char *s = getenv("COOT_SCHEME_EXTRAS_DIR");
   if (s) {

#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
      std::vector<std::string> dirs = coot::util::split_string(s, ";");
#else
      std::vector<std::string> dirs = coot::util::split_string(s, ":");
#endif
      for (unsigned int i=0; i<dirs.size(); i++) { 
	 struct stat buf;
	 int status = stat(dirs[i].c_str(), &buf);
	 if (status != 0) {
	    std::cout << "WARNING:: no directory \"" << dirs[i] << "\""
		      << " in COOT_SCHEME_EXTRAS_DIR " << s
		      << std::endl;
	 } else {
	    if (S_ISDIR(buf.st_mode)) {

	       DIR *lib_dir = opendir(dirs[i].c_str());
	       if (lib_dir == NULL) {
		  std::cout << "An ERROR occured on opening the directory "
			    << dirs[i] << std::endl;
	       } else {

		  struct dirent *dir_ent;

		  // loop until the end of the filelist (readdir returns NULL)
		  // 
		  while (1) {
		     dir_ent = readdir(lib_dir);
		     if (dir_ent == NULL) {
			break;
		     } else {
			std::string sub_part(std::string(dir_ent->d_name));
			struct stat buf2;
			std::string fp = s;
			fp += "/";
			fp += sub_part;
			int status2 = stat(fp.c_str(), &buf2);
			if (status2 != 0) {
			   // std::cout << "WARNING:: no file " << sub_part << " in directory "
			   // << dirs[i] << std::endl;
			} else {
			   if (S_ISREG(buf2.st_mode)) {
			      if (coot::util::file_name_extension(sub_part) == ".scm") {
				 std::cout << "loading extra: " << fp << std::endl;
				 scm_c_primitive_load(fp.c_str()); 
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
#endif // USE_GUILE


#ifdef USE_PYTHON
void try_load_python_extras_dir() {

   char *s = getenv("COOT_PYTHON_EXTRAS_DIR");
   if (s) {
#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
      std::vector<std::string> dirs = coot::util::split_string(s, ";");
#else
      std::vector<std::string> dirs = coot::util::split_string(s, ":");
#endif
      for (unsigned int i=0; i<dirs.size(); i++) { 
	 struct stat buf;
	 int status = stat(dirs[i].c_str(), &buf);
	 if (status != 0) {
	    std::cout << "WARNING:: no directory \"" << dirs[i] << "\""
		      << " in COOT_PYTHON_EXTRAS_DIR " << s
		      << std::endl;
	 } else {
	    if (S_ISDIR(buf.st_mode)) {

	       DIR *lib_dir = opendir(dirs[i].c_str());
	       if (lib_dir == NULL) {
		  std::cout << "An ERROR occured on opening the directory "
			    << dirs[i] << std::endl;
	       } else {

		  struct dirent *dir_ent;

		  // loop until the end of the filelist (readdir returns NULL)
		  // 
		  while (1) {
		     dir_ent = readdir(lib_dir);
		     if (dir_ent == NULL) {
			break;
		     } else {
			std::string sub_part(std::string(dir_ent->d_name));
			struct stat buf2;
			std::string fp = s;
			fp += "/";
			fp += sub_part;
			int status2 = stat(fp.c_str(), &buf2);
			if (status2 != 0) {
			   // std::cout << "WARNING:: no file " << sub_part << std::endl;
			} else {
			   if (S_ISREG(buf2.st_mode)) {
			      if (coot::util::file_name_extension(sub_part) == ".py") {
				 std::cout << "loading python extra: " << fp << std::endl;
				 run_python_script(fp.c_str()); 
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
#endif // USE_PYTHON


void set_button_label_for_external_refinement(const char *button_label) {
   graphics_info_t::external_refinement_program_button_label = button_label;
}
