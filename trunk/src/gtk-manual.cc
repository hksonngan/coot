/* src/c-interface.cc
 * 
 * Copyright 2002, 2003, 2004, 2005 by The University of York
 * Copyright 2008 by The University of Oxford
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


#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <string.h>

// NOTE:: The order of these 6 include files seems fragile
#include <gtk/gtk.h>
#include "c-interface.h"
#include "cc-interface.hh"

#include "gtk-manual.h"
#include "gtk-manual.hh"

#include "interface.h"  /* for create_single_map_properties_dialog() */
#include "coot-utils.hh"



/* This is the signal handler for a color change event created when
   the colorseldialog has had its colour changed. */

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/*                                 map                                      */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

void 
on_map_color_changed(GtkWidget *w,
/* 		 GtkColorSelection *colorsel)  */
		     gpointer *tmd)
{ 
 
   gdouble color[4];
   struct map_colour_data_type* t;

   t = (struct map_colour_data_type*) tmd;

   gtk_color_selection_get_color(t->colorsel, color);

   handle_map_colour_change(t->imol, color); 
}

/*  The colour selection dialog has had its OK button pressed */
void
on_map_col_sel_ok_button_clicked        (GtkButton       *button,
					 gpointer         user_data)
{
   GtkWidget *w = GTK_WIDGET(user_data);
   gtk_widget_destroy(w);
}

/*  The colour selection dialog has had its cancel button pressed */
void
on_map_col_sel_cancel_button_clicked (GtkButton       *button,
				      gpointer         *tmd)
{
   
  struct map_colour_data_type* t = (struct map_colour_data_type*) tmd;
  int imol = t->imol;
  GtkWidget *dialog = lookup_widget(GTK_WIDGET(button), "map_color_selection_dialog");

  /* we should put the colour back to
     how it used to be then. */
  restore_previous_map_colour(t->imol);
  gtk_widget_destroy(dialog);
}


GtkWidget *create_map_colour_selection_window(struct map_colour_data_type *mcdt) { 


/*    GtkColorSelectionDialog *colorseldialog;  */
   GtkWidget  *colorseldialog;

   GtkButton *ok_button;
   GtkButton *cancel_button;
   GtkButton *help_button;
   GtkWidget *colorsel;
   struct map_colour_data_type *t;

   t = (struct map_colour_data_type*) malloc(100);

   colorseldialog = gtk_color_selection_dialog_new("Map Colour Selection"); 
   gtk_object_set_data(GTK_OBJECT(colorseldialog), "map_color_selection_dialog",
		       colorseldialog);

   colorsel = GTK_COLOR_SELECTION_DIALOG(colorseldialog)->colorsel;

   t->imol = mcdt->imol; 
   t->colorsel = (GtkColorSelection*)colorsel;
   save_previous_map_colour(t->imol);

  /* Capture "color_changed" events in col_sel_window */

  gtk_signal_connect (GTK_OBJECT (colorsel), "color_changed",
                      (GtkSignalFunc)on_map_color_changed, 
/* 		      (gpointer)colorsel); */
		      (gpointer)t);
  
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(colorseldialog)->
				ok_button), "clicked",
		     GTK_SIGNAL_FUNC(on_map_col_sel_ok_button_clicked),
		     colorseldialog);

  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(colorseldialog)->
				cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(on_map_col_sel_cancel_button_clicked), 
		     (gpointer)t);

  gtk_object_set_data (GTK_OBJECT (colorseldialog), "map_colour_selection",
		       colorseldialog);
                      

  return GTK_WIDGET(colorseldialog);

}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/*                               symmetry                                   */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

GtkWidget *
create_symmetry_colour_selection_window() { 


/*    GtkColorSelectionDialog *colorseldialog;  */
   GtkWidget  *colorseldialog;

   GtkButton *ok_button;
   GtkButton *cancel_button;
   GtkButton *help_button;
   GtkWidget *colorsel;

   colorseldialog = 
      gtk_color_selection_dialog_new("Symmetry Colour Selection"); 


/* How do we get to the buttons? */

   colorsel = GTK_COLOR_SELECTION_DIALOG(colorseldialog)->colorsel;

  /* Capture "color_changed" events in col_sel_window */

  gtk_signal_connect (GTK_OBJECT (colorsel), "color_changed",
                      (GtkSignalFunc)on_symmetry_color_changed, 
		      (gpointer)colorsel);
  
  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(colorseldialog)->
				ok_button), "clicked",
		     GTK_SIGNAL_FUNC(on_symm_col_sel_cancel_button_clicked),
		     colorseldialog);

  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(colorseldialog)->
				cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(on_symm_col_sel_cancel_button_clicked), 
		     colorseldialog);

  /* give this widget a name so that we can look it up? */
  gtk_object_set_data (GTK_OBJECT (colorseldialog), "symmetry_bonds_colour_selection",
		       colorseldialog);

  return GTK_WIDGET(colorseldialog);

}


void 
on_symmetry_color_changed(GtkWidget *w,
			  GtkColorSelection *colorsel) { 
   gdouble color[4];

   gtk_color_selection_get_color(colorsel,color);

   /* we pass back the model number */
   handle_symmetry_colour_change(1,color);
}

/*  The colour selection dialog has had its OK button pressed */
void
on_symm_col_sel_ok_button_clicked (GtkButton       *button,
				   gpointer         user_data)
{
   GtkWidget *w = GTK_WIDGET(user_data);
   gtk_widget_destroy(w);
}

/*  The colour selection dialog has had its cancel button pressed */
void
on_symm_col_sel_cancel_button_clicked (GtkButton       *button,
				      gpointer         user_data)
{
   GtkWidget *w = GTK_WIDGET(user_data);
   gtk_widget_destroy(w);
				/* we should put the colour back to
				   how it used to be then. */
}

/* ----------------------------------------------------------------- */
/* dynamic map color menu */
/* ----------------------------------------------------------------- */

void 
create_initial_map_color_submenu(GtkWidget *widget) { 

   GtkWidget *map_colour1_menu; 
   GtkWidget *window1; 
   GtkWidget *map_colour1; 
  
 /* We need to get to window1 */
   window1 = widget; 

   map_colour1 = GTK_WIDGET(lookup_widget(window1, "map_colour1"));

   map_colour1_menu = gtk_menu_new ();
   gtk_widget_ref (map_colour1_menu);
   gtk_object_set_data_full (GTK_OBJECT (window1), "map_colour1_menu", 
			     map_colour1_menu,
			     (GtkDestroyNotify) gtk_widget_unref);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM (map_colour1), 
			      map_colour1_menu);
}



/* And similar code for the scroll whell - which need to know about the 
   list of maps too.  */
/* ----------------------------------------------------------------- */
/*  scroll wheel menu */
/* ----------------------------------------------------------------- */


void 
create_initial_map_scroll_wheel_submenu(GtkWidget *widget) { 

   GtkWidget *map_scroll_wheel_menu; 
   GtkWidget *window1; 
   GtkWidget *map_scroll_wheel; 
  
 /* We need to get to window1 */

   window1 = widget; 

   map_scroll_wheel = 
     GTK_WIDGET(lookup_widget(window1, "attach_scroll_wheel_to_which_map_1"));

   map_scroll_wheel_menu = gtk_menu_new ();
   gtk_widget_ref (map_scroll_wheel_menu);
   gtk_object_set_data_full (GTK_OBJECT (window1), 
			     "map_scroll_wheel_menu", 
			     map_scroll_wheel_menu,
			     (GtkDestroyNotify) gtk_widget_unref);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM (map_scroll_wheel), 
			      map_scroll_wheel_menu);
}



void
update_map_scroll_wheel_menu_manual(int imol, const char *name) { 

   GtkWidget *mapscroll_wheelmap1;
   GtkWidget *window1; 
   GtkWidget *map_scroll_wheel1_menu; 
   char *text; 
   struct map_colour_data_type *map_scroll_wheel_data; 

   map_scroll_wheel_data = (struct map_colour_data_type *) 
      malloc(sizeof(struct map_colour_data_type));

   map_scroll_wheel_data->imol = imol; 
   map_scroll_wheel_data->imap = 0; 

   // text = (char *) malloc(200); 
   // strncpy(text, name, 199);
   // text = name;
   
   
   window1 = GTK_WIDGET(lookup_widget(main_window(), "window1")); 

   mapscroll_wheelmap1 = gtk_menu_item_new_with_label (name);

   map_scroll_wheel1_menu = 
     GTK_WIDGET(lookup_widget(window1, "map_scroll_wheel_menu")); 

   gtk_widget_ref (mapscroll_wheelmap1);
   gtk_object_set_data_full (GTK_OBJECT (window1), "mapscroll_wheelmap1", 
			     mapscroll_wheelmap1,
			     (GtkDestroyNotify) gtk_widget_unref);

  gtk_widget_show (mapscroll_wheelmap1);
  gtk_container_add (GTK_CONTAINER (map_scroll_wheel1_menu), 
		     mapscroll_wheelmap1);

  gtk_signal_connect (GTK_OBJECT (mapscroll_wheelmap1), "activate",  
		      GTK_SIGNAL_FUNC (my_map_scroll_wheel_activate),  
		      (gpointer) map_scroll_wheel_data);   
}

void
my_map_scroll_wheel_activate (GtkMenuItem     *menuitem,
                        gpointer         user_data)
{
   struct map_colour_data_type *map_colour_data;
   printf("scroll map change\n");
   map_colour_data = (struct map_colour_data_type *) user_data; 
   set_scrollable_map(map_colour_data->imol); 
}


void 
create_initial_ramachandran_mol_submenu(GtkWidget *widget) { 

   GtkWidget *rama_draw_menu; 
   GtkWidget *window1; 
   GtkWidget *rama_draw; 
  
 /* We need to get to window1 */

   window1 = widget; 

   rama_draw = GTK_WIDGET(lookup_widget(window1, "ramachandran_plot1"));
			
   rama_draw_menu = gtk_menu_new ();
   gtk_widget_ref (rama_draw_menu);
   gtk_object_set_data_full (GTK_OBJECT (window1), "rama_plot_menu", 
			     rama_draw_menu,
			     (GtkDestroyNotify) gtk_widget_unref);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM (rama_draw), 
			      rama_draw_menu);
}


void
update_ramachandran_plot_menu_manual(int imol, const char *name) {

   GtkWidget *menu_item;
   GtkWidget *window1; 
   GtkWidget *rama_plot_menu; 
   char *text; 

   // text = (char *) malloc(200); 
   // strncpy(text, name, 199);
   // text = name;

   window1 = GTK_WIDGET(lookup_widget(main_window(), "window1")); 

   menu_item = gtk_menu_item_new_with_label (name);

   rama_plot_menu = GTK_WIDGET(lookup_widget(window1, "rama_plot_menu")); 

   gtk_widget_ref (menu_item);
   gtk_object_set_data_full (GTK_OBJECT (window1), "rama_plot_menu_item", 
			     menu_item,
			     (GtkDestroyNotify) gtk_widget_unref);

  gtk_widget_show (menu_item);
  gtk_container_add (GTK_CONTAINER (rama_plot_menu), menu_item);

  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",  
		      GTK_SIGNAL_FUNC (rama_plot_mol_selector_activate),  
		      GINT_TO_POINTER(imol));
}


void
rama_plot_mol_selector_activate (GtkMenuItem     *menuitem,
				 gpointer         user_data)
{
  int imol = GPOINTER_TO_INT(user_data);
  GtkWidget *rama_widget = 0; 
/*   printf("selector activate: do rama plot for molecule: %d\n", *imol); */

/* We should come here and be given imol.  New molecules should insert
   themselves into the Ramachandran Plot menu(item). */

#if defined(HAVE_GTK_CANVAS) || defined (HAVE_GNOME_CANVAS)

  rama_widget = dynarama_is_displayed_state(imol);
  if (rama_widget == NULL) { 
    do_ramachandran_plot(imol); 
  } else { 
    if (!GTK_WIDGET_MAPPED(rama_widget))
      gtk_widget_show(rama_widget);
    else 
      gdk_window_raise(rama_widget->window);
  }
#else 
  printf("not compiled with HAVE_GTK_CANVAS/GNOME_CANVAS - remake\n"); 
#endif /* HAVE_GTK_CANVAS */

}

/* And similar for sequence view: */
void create_initial_sequence_view_mol_submenu(GtkWidget *widget) { 

   GtkWidget *seq_view_draw = lookup_widget(widget, "sequence_view1");
   GtkWidget *seq_view_menu = gtk_menu_new();
   gtk_widget_ref(seq_view_menu);
   gtk_object_set_data_full(GTK_OBJECT(widget), "seq_view_menu",
			    seq_view_menu,
			    (GtkDestroyNotify) gtk_widget_unref);
   gtk_menu_item_set_submenu (GTK_MENU_ITEM(seq_view_draw),
			      seq_view_menu);
}

void update_sequence_view_menu_manual(int imol, const char *name) { 

   char *text; 
   GtkWidget *window1 = lookup_widget(main_window(), "window1");
   GtkWidget *seq_view_menu = lookup_widget(window1, "seq_view_menu");
   GtkWidget *menu_item;
   
   // text = (char *) malloc(200);
   // strncpy(text, name, 199);
   // text = name;

   menu_item = gtk_menu_item_new_with_label (name);
   gtk_widget_ref(menu_item);
   gtk_object_set_data_full (GTK_OBJECT(window1), "seq_view_menu_item",
			     menu_item, 
			     (GtkDestroyNotify) gtk_widget_unref);
   gtk_widget_show(menu_item);
   gtk_container_add(GTK_CONTAINER(seq_view_menu), menu_item);
   gtk_signal_connect (GTK_OBJECT(menu_item), "activate",
		       GTK_SIGNAL_FUNC(sequence_view_mol_selector_activate),
		       GINT_TO_POINTER(imol));
}

void sequence_view_mol_selector_activate (GtkMenuItem     *menuitem,
					  gpointer         user_data) { 

  int imol = GPOINTER_TO_INT(user_data);
#if defined(HAVE_GTK_CANVAS) || defined (HAVE_GNOME_CANVAS)
   do_sequence_view(imol);
#else    
  printf("not compiled with HAVE_GTK_CANVAS/GNOME_CANVAS - remake\n"); 
#endif /* HAVE_GTK_CANVAS */

}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/*                               skeleton                                   */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

void 
on_skeleton_color_changed(GtkWidget *w,
			  GtkColorSelection *colorsel) { 
   gdouble color[4];

   gtk_color_selection_get_color(colorsel,color);

   /* we pass back the model number */
   handle_skeleton_colour_change(1,color);
}



/*  The colour selection dialog has had its OK button pressed */
void
on_skeleton_col_sel_ok_button_clicked (GtkButton       *button,
				       gpointer         user_data)
{
   GtkWidget *w = GTK_WIDGET(user_data);
   gtk_widget_destroy(w);
}

/*  The colour selection dialog has had its cancel button pressed */
void
on_skeleton_col_sel_cancel_button_clicked (GtkButton       *button,
				      gpointer         user_data)
{
   GtkWidget *w = GTK_WIDGET(user_data);
   gtk_widget_destroy(w);
				/* we should put the colour back to
				   how it used to be then. */
}




/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */
/*              map and molecule display control                            */
/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */



/* Coordinates  */
/* n is the molecule number (not necessarily the nth element in the
   molecule display VBox) */
void display_control_molecule_combo_box(GtkWidget *display_control_window_glade, 
					const gchar *name, 
					int n, bool show_add_reps_frame_flag) {

  GtkWidget *my_combo_box; 

  GtkWidget *display_molecule_vbox; 
/*   GtkWidget *display_control_window_glade; passed parameter */

  GtkWidget *display_mol_frame_1; 
  GtkWidget *hbox31; 
  GtkWidget *hbox32; 
  GtkWidget *entry2; 
  GtkWidget *displayed_button_1; 
  GtkWidget *active_button_1; 
  GtkWidget *render_optionmenu_1; 
  GtkWidget *render_optionmenu_1_menu; 
  GtkWidget *glade_menuitem; 
  GtkWidget *menu;
  int bond_type; 
  GList  *tmp_list;
  GtkWidget *child;
  GtkWidget *active_item;
  GtkWidget *mol_label;

/* messing about with string variables for unique lookup values/name of the widgets */
  gchar *widget_name; 
  gchar *tmp_name; 

  /* we need to find references to objects that contain this widget */

  display_molecule_vbox = lookup_widget(display_control_window_glade, 
					"display_molecule_vbox"); 


  display_mol_frame_1 = gtk_frame_new (NULL);
  gtk_widget_ref (display_mol_frame_1);
  
  widget_name = (gchar *) malloc(100); /* should be enough */

  strcpy(widget_name, "display_mol_frame_"); 
  tmp_name = widget_name + strlen(widget_name); 
  
  snprintf(tmp_name, 4, "%-d", n); 

  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			    widget_name, 
			    display_mol_frame_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (display_mol_frame_1);
  gtk_box_pack_start (GTK_BOX (display_molecule_vbox), display_mol_frame_1, 
		      FALSE, FALSE, 0);

  hbox31 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox31);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), "hbox31", hbox31,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox31);

  GtkWidget *vbox_single_molecule_all_attribs = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox_single_molecule_all_attribs);
  gtk_container_add (GTK_CONTAINER (display_mol_frame_1), vbox_single_molecule_all_attribs);
  gtk_box_pack_start(GTK_BOX(vbox_single_molecule_all_attribs), hbox31, FALSE, FALSE, 2);


/* -- molecule number label */

  strcpy(widget_name, "display_mol_number_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n);

  mol_label = gtk_label_new (_(tmp_name));
  gtk_widget_ref (mol_label);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			    widget_name, 
			    mol_label,
                            (GtkDestroyNotify) gtk_widget_unref);

  gtk_widget_show (mol_label);
  gtk_box_pack_start (GTK_BOX (hbox31), mol_label, FALSE, FALSE, 3);

/* -- done molecule number label */

  strcpy(widget_name, "display_mol_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 

  entry2 = gtk_entry_new ();
  gtk_widget_ref (entry2);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), widget_name, entry2,
                            (GtkDestroyNotify) gtk_widget_unref);
  if (name) { 
    gtk_entry_set_text(GTK_ENTRY(entry2), name); 
    /* these 2 seem not to do what I want :-( */
    gtk_entry_set_position(GTK_ENTRY(entry2), strlen(name)-1);
    gtk_entry_append_text(GTK_ENTRY(entry2), "");
  } 
  gtk_entry_set_editable(GTK_ENTRY (entry2), FALSE);


  gtk_widget_show (entry2);
  gtk_box_pack_start (GTK_BOX (hbox31), entry2, TRUE, TRUE, 3);

  hbox32 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox32);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), "hbox32", hbox32,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox32);
  gtk_box_pack_start (GTK_BOX (hbox31), hbox32, TRUE, TRUE, 0);

  strcpy(widget_name, "displayed_button_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 

  displayed_button_1 = gtk_check_button_new_with_label (_("Display"));
  gtk_widget_ref (displayed_button_1);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(displayed_button_1), 
			       mol_is_displayed(n));

  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			    widget_name, 
			    displayed_button_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (displayed_button_1);
  gtk_box_pack_start (GTK_BOX (hbox32), displayed_button_1, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (displayed_button_1), 2);

  strcpy(widget_name, "active_button_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 

  active_button_1 = gtk_check_button_new_with_label (_("Active"));
  gtk_widget_ref (active_button_1);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_button_1), mol_is_active(n));
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			    widget_name, 
			    active_button_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (active_button_1);
  gtk_box_pack_start (GTK_BOX (hbox32), active_button_1, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (active_button_1), 2);



  strcpy(widget_name, "render_optionmenu_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 

  render_optionmenu_1 = gtk_option_menu_new ();
  gtk_widget_ref (render_optionmenu_1);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			    "render_optionmenu_1", render_optionmenu_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (render_optionmenu_1);
  gtk_box_pack_start (GTK_BOX (hbox32), render_optionmenu_1, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (render_optionmenu_1), 1);
  render_optionmenu_1_menu = gtk_menu_new ();
/*   glade_menuitem = gtk_menu_item_new_with_label (_("Render As: Bonds (Colour by Atom)")); */
  glade_menuitem = gtk_menu_item_new_with_label (_("Bonds (Colour by Atom)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer to the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_bonds_button_select),
		     GINT_TO_POINTER(n));


 /* Now a button for Colour by molecule bonds button: */

  glade_menuitem = gtk_menu_item_new_with_label (_("Bonds (Colour by Molecule)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_bonds_colored_by_molecule_button_select),
		     GINT_TO_POINTER(n));

 /* Now a button for Colour by segment bonds button: */

  glade_menuitem = gtk_menu_item_new_with_label (_("Bonds (Colour by Chain)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_bonds_colored_by_chain_button_select),
		     GINT_TO_POINTER(n));

 /* Now a button for Bonds with Sec. Str. Colour: */

  glade_menuitem = gtk_menu_item_new_with_label (_("Bonds (Colour by Sec. Str.)"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_sec_struct_bonds_button_select),
		     GINT_TO_POINTER(n));

 /* Now a button for Ca bonds: */

  glade_menuitem = gtk_menu_item_new_with_label (_("C-alphas"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_ca_bonds_button_select),
		     GINT_TO_POINTER(n));


 /* Now a button for Ca + ligands bonds: */

  glade_menuitem = gtk_menu_item_new_with_label (_("CAs + Ligs"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_ca_plus_ligands_bonds_button_select),
		     GINT_TO_POINTER(n));

 /* Now a button for Ca + ligands bonds, Sec. Str. Colour: */

  glade_menuitem = gtk_menu_item_new_with_label (_("CAs+Ligs SecStr Col"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_ca_plus_ligands_sec_str_bonds_button_select),
		     GINT_TO_POINTER(n));

 /* Now a button for Ca + ligands bonds, Jones' Rainbow: */

  glade_menuitem = gtk_menu_item_new_with_label (_("Jones' Rainbow"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_rainbow_representation_button_select),
		     GINT_TO_POINTER(n));


 /* Now a button for Normal - No Waters: */

  glade_menuitem = gtk_menu_item_new_with_label (_("Colour by Atom - No Waters"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_bonds_no_waters),
		     GINT_TO_POINTER(n));


 /* Now a button for B-factor colours: */

  glade_menuitem = gtk_menu_item_new_with_label (_("Colour by B-factors"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_b_factor_representation_button_select),
		     GINT_TO_POINTER(n));

  glade_menuitem = gtk_menu_item_new_with_label (_("Colour by Occupancy"));
  gtk_widget_show (glade_menuitem);
  gtk_menu_append (GTK_MENU (render_optionmenu_1_menu), glade_menuitem);
/* Notice how this time we attach a pointer to the molecule number -
   more usually, we attach a pointer tto the menu item position number */
  gtk_signal_connect(GTK_OBJECT(glade_menuitem), "activate",
		     GTK_SIGNAL_FUNC(render_as_occupancy_representation_button_select),
		     GINT_TO_POINTER(n));


/* Set User Data, the molecule which this button(s) is attached to 
   (casting (int *) to (char *)).
*/
  gtk_object_set_user_data(GTK_OBJECT(displayed_button_1), GINT_TO_POINTER(n)); 
  gtk_object_set_user_data(GTK_OBJECT(   active_button_1), GINT_TO_POINTER(n)); 

/* Add signals for the Active and Display toggle buttons */

  gtk_signal_connect(GTK_OBJECT (displayed_button_1),  "toggled",
		     GTK_SIGNAL_FUNC (on_display_control_mol_displayed_button_toggled),
		     NULL);

  gtk_signal_connect(GTK_OBJECT (active_button_1),  "toggled",
		     GTK_SIGNAL_FUNC (on_display_control_mol_active_button_toggled),
		     NULL);



/* Set User Data, the molecule which this button(s) is attached to 
   (casting (int *) to (char *)).
*/
  gtk_object_set_user_data(GTK_OBJECT(displayed_button_1), GINT_TO_POINTER(n)); 
  gtk_object_set_user_data(GTK_OBJECT(   active_button_1), GINT_TO_POINTER(n)); 



  /* Which menu item should be displayed in the selector?  If we
     turned to a C-alpha representation and the closed the display
     control window.  when it comes back, we want it to be C-alpha
     too, not the default (1) - "Bonds"  */

  bond_type = graphics_molecule_bond_type(n);

  if (bond_type != 1) {
/*     printf("setting menu to item to other bonds...\n"); */
     menu = render_optionmenu_1_menu;
     active_item = gtk_menu_get_active(GTK_MENU(menu));

     /*  The conversion between menu item order and bond type (enum) order */
     if (bond_type == 1) { /* atom type bonds */
       gtk_menu_set_active(GTK_MENU(menu), 0); 
     }
     if (bond_type == 2) { /* CA bonds */
       gtk_menu_set_active(GTK_MENU(menu), 4); 
     }
     if (bond_type == 3) { /* segid-coloured bonds */
       gtk_menu_set_active(GTK_MENU(menu), 2); 
     }
     if (bond_type == 4) { /* CA_BONDS_PLUS_LIGANDS */
       gtk_menu_set_active(GTK_MENU(menu), 5); 
     }
     if (bond_type == 5) { /* BONDS_NO_WATERS */
       gtk_menu_set_active(GTK_MENU(menu), 8); 
     }
     if (bond_type == 6) { /* BONDS_SEC_STRUCT_COLOUR */
       gtk_menu_set_active(GTK_MENU(menu), 3); 
     }
     if (bond_type == 7) { /* CA_BONDS_PLUS_LIGANDS_SEC_STRUCT_COLOUR */
       gtk_menu_set_active(GTK_MENU(menu), 6); 
     }
     if (bond_type == 8) { /* COULUR_BY_MOLECULE_BONDS */
       gtk_menu_set_active(GTK_MENU(menu), 1); 
     }
     if (bond_type == 9) { /* COLOUR_BY_RAINBOW_BONDS */
       gtk_menu_set_active(GTK_MENU(menu), 7); 
     }
     if (bond_type == 10) { /* COLOUR_BY_B_FACTOR_BONDS */
       gtk_menu_set_active(GTK_MENU(menu), 9); 
     }
     if (bond_type == 11) { /* COLOUR_BY_OCCUPANCY_BONDS */
       gtk_menu_set_active(GTK_MENU(menu), 10); 
     }
     /* c.f molecule-class-info.h:30 enum */
  }

/* And finally connect the menu to the optionmenu */
  gtk_option_menu_set_menu (GTK_OPTION_MENU (render_optionmenu_1), 
			    render_optionmenu_1_menu);

  // A delete molecule button
  display_control_add_delete_molecule_button(n, hbox32);

  free(widget_name);
  // Now add the additional representations frame and vbox
  add_add_reps_frame_and_vbox(display_control_window_glade,
			      vbox_single_molecule_all_attribs, n, show_add_reps_frame_flag);
  
}

void display_control_add_delete_molecule_button(int imol, GtkWidget *hbox32) {

   std::string delete_button_name = "delete_molecule_";
   delete_button_name += coot::util::int_to_string(imol);
   GtkWidget *delete_button = gtk_button_new_with_label(_("Delete Molecule"));
   gtk_widget_show(delete_button);
   gtk_box_pack_start (GTK_BOX (hbox32), delete_button, FALSE, FALSE, 0);
   gtk_container_set_border_width (GTK_CONTAINER (delete_button), 2);
   gtk_signal_connect(GTK_OBJECT(delete_button), "clicked",
		      GTK_SIGNAL_FUNC(on_display_control_delete_molecule_button_clicked),
		      GINT_TO_POINTER(imol));

} 

void add_add_reps_frame_and_vbox(GtkWidget *display_control_window_glade,
				 GtkWidget *hbox_for_single_molecule, int imol_no,
				 bool show_add_reps_frame_flag) {

   GtkWidget *frame = gtk_frame_new("Additional Representations");
   GtkWidget *v = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER(hbox_for_single_molecule), frame);
   gtk_container_add(GTK_CONTAINER(frame), v);
   // show the frame if there were additional representations
   if (show_add_reps_frame_flag)
      gtk_widget_show(frame);
   gtk_widget_ref (v);
   gtk_widget_ref (frame);

   // set the name so that it can be looked up.
   std::string widget_name = "add_rep_display_control_frame_vbox_";
   widget_name += coot::util::int_to_string(imol_no);
   gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			     widget_name.c_str(),
			     v,
			     (GtkDestroyNotify) gtk_widget_unref);
   
   widget_name = "add_rep_display_control_frame_";
   widget_name += coot::util::int_to_string(imol_no);
   gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			     widget_name.c_str(),
			     frame,
			     (GtkDestroyNotify) gtk_widget_unref);
   
//    std::cout <<"DEBUG:: added set_data_full for " << v << " on to " << display_control_window_glade
// 	     << " " << widget_name << std::endl;

} 



void 
update_name_in_display_control_molecule_combo_box(GtkWidget *display_control_window_glade, 
						  const gchar *display_name,
						  int n) {
  int i;
  char entry_name[1024];
  GtkWidget *entry;
  gchar *tmp_name; 
  int imol = n;

  for (i=0; i<1024; i++)
    entry_name[i]= 0;
  memcpy(entry_name, "display_mol_entry_", 18);
  tmp_name = entry_name + strlen(entry_name); 
  snprintf(tmp_name, 4, "%-d", imol); 

/*   printf("debug:: molecule number (dereferenced): %d\n", *n); */
/*   printf("debug:: pointer string: %s\n", tmp_name); */
/*   printf("debug:: searching for entry name %s\n", entry_name); */
  entry = lookup_widget(display_control_window_glade, entry_name);

  if (entry) 
    gtk_entry_set_text(GTK_ENTRY(entry), display_name);
  else 
    printf("oops no entry found with name %s\n", entry_name);
}




void
render_as_bonds_button_select(GtkWidget *item, GtkPositionType mol) {

   graphics_to_bonds_representation(mol);
}

void
render_as_bonds_colored_by_chain_button_select(GtkWidget *item, GtkPositionType mol) {

  set_colour_by_chain(mol);
}

void
render_as_bonds_colored_by_molecule_button_select(GtkWidget *item, GtkPositionType mol) {

  set_colour_by_molecule(mol);
}

void
render_as_bonds_no_waters(GtkWidget *item, GtkPositionType mol) {

  graphics_to_bonds_no_waters_representation(mol);
}


void
render_as_ca_bonds_button_select(GtkWidget *item, GtkPositionType mol) {

   graphics_to_ca_representation(mol);
}

void 
render_as_ca_plus_ligands_bonds_button_select(GtkWidget *item, GtkPositionType pos) { 
   graphics_to_ca_plus_ligands_representation(pos);
}

void 
render_as_ca_plus_ligands_sec_str_bonds_button_select(GtkWidget *item, GtkPositionType pos) { 
   graphics_to_ca_plus_ligands_sec_struct_representation(pos);
}

void 
render_as_sec_struct_bonds_button_select(GtkWidget *item, GtkPositionType pos) { 
   graphics_to_sec_struct_bonds_representation(pos);
}

void render_as_rainbow_representation_button_select(GtkWidget *item, GtkPositionType pos) { 
   graphics_to_rainbow_representation(pos);
}

void render_as_b_factor_representation_button_select(GtkWidget *item, GtkPositionType pos) { 
   graphics_to_b_factor_representation(pos);

}

void render_as_occupancy_representation_button_select(GtkWidget *item, GtkPositionType pos) {
   graphics_to_occupancy_representation(pos);
}



/* n is the nth element in the molecule display VBox */
GtkWidget *display_control_map_combo_box(GtkWidget *display_control_window_glade, 
					 const gchar *name, 
					 int n) {

  GtkWidget *my_combo_box = 0;

  GtkWidget *display_map_vbox; 
/*   GtkWidget *display_control_window_glade; passed parameter */

  GtkWidget *display_map_frame_1; 
  GtkWidget *hbox31; 
  GtkWidget *hbox32; 
  GtkWidget *entry2; 
  GtkWidget *displayed_button_1; 
  GtkWidget *mol_label;

/* messing about with string variables */
  gchar *widget_name; 
  gchar *tmp_name; 

  GSList **sgp = gslist_for_scroll_in_display_manager_p();
  GSList *scroll_group = *sgp;

  GtkWidget *scroll_radio_button_1;

  /* we need to find references to objects that contain this widget */

  display_map_vbox = lookup_widget(display_control_window_glade, 
				   "display_map_vbox"); 

  display_map_frame_1 = gtk_frame_new (NULL);
  gtk_widget_ref (display_map_frame_1);
  
  widget_name = (gchar *) malloc(100); /* should be enough */

  strcpy(widget_name, "display_map_frame_"); 
  tmp_name = widget_name + strlen(widget_name); 
  
  snprintf(tmp_name, 4, "%-d", n); 

/*   printf("display_map_frame_{thing} name constructed as: :%s:\n", widget_name);  */
  

  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), widget_name, display_map_frame_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (display_map_frame_1);
  /* setting to true means that the buttons etc in the box can expand
     vertically to fill the box  */
  gtk_box_pack_start (GTK_BOX (display_map_vbox), display_map_frame_1, FALSE, FALSE, 0);

  hbox31 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox31);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), "hbox31", hbox31,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox31);
  gtk_container_add (GTK_CONTAINER (display_map_frame_1), hbox31);

/* -- molecule number label */

  strcpy(widget_name, "display_map_number_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n);

  mol_label = gtk_label_new (_(tmp_name));
  gtk_widget_ref (mol_label);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), widget_name, mol_label,
                            (GtkDestroyNotify) gtk_widget_unref);

  gtk_widget_show (mol_label);
  gtk_box_pack_start (GTK_BOX (hbox31), mol_label, FALSE, FALSE, 3);

/* -- molecule number label */

/* -- */

  strcpy(widget_name, "display_map_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n);

  entry2 = gtk_entry_new ();
  gtk_widget_ref (entry2);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), widget_name, entry2,
                            (GtkDestroyNotify) gtk_widget_unref);
  if (name) { 
    gtk_entry_set_text(GTK_ENTRY(entry2), name); 
  } 
  gtk_entry_set_editable(GTK_ENTRY (entry2), FALSE);

  gtk_widget_show (entry2);
  gtk_box_pack_start (GTK_BOX (hbox31), entry2, TRUE, TRUE, 0);

  hbox32 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox32);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), "hbox32", hbox32,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox32);
  gtk_box_pack_start (GTK_BOX (hbox31), hbox32, TRUE, TRUE, 0);

/* -- */

  strcpy(widget_name, "displayed_button_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n);

  displayed_button_1 = gtk_check_button_new_with_label (_("Display"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(displayed_button_1), map_is_displayed(n));
  gtk_widget_ref (displayed_button_1);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			    widget_name, 
			    displayed_button_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (displayed_button_1);
  gtk_box_pack_start (GTK_BOX (hbox32), displayed_button_1, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (displayed_button_1), 2);

  gtk_signal_connect(GTK_OBJECT (displayed_button_1),  "toggled",
		     GTK_SIGNAL_FUNC (on_display_control_map_displayed_button_toggled),
		     GINT_TO_POINTER(n));

/*   // associate with the button a pointer to the variable which */
/*   // contains the passed variable int n */
/*   //  */
/*   // we cast as a (char *) to make the compiler happy. */
/*   //  */
   gtk_object_set_user_data (GTK_OBJECT (displayed_button_1), GINT_TO_POINTER(n)); 

/* -- */
  /* 20050316 Today I add scroll check-button, as Charlie asked for ages ago. */

  strcpy(widget_name, "map_scroll_button_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n);

  std::cout << "DEBUG:: gtk_radio_button_new_with_label() with scroll_group "
	    << scroll_group << std::endl;

  scroll_radio_button_1 = gtk_radio_button_new_with_label(scroll_group, _("Scroll"));
  scroll_group = gtk_radio_button_group (GTK_RADIO_BUTTON(scroll_radio_button_1));
  *gslist_for_scroll_in_display_manager_p() = scroll_group;

  gtk_widget_ref(scroll_radio_button_1);
  gtk_object_set_data_full(GTK_OBJECT(display_control_window_glade), widget_name,
			   scroll_radio_button_1, 
			   (GtkDestroyNotify) gtk_widget_unref);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scroll_radio_button_1), FALSE);

  gtk_widget_show(scroll_radio_button_1);
  gtk_box_pack_start(GTK_BOX(hbox32), scroll_radio_button_1, FALSE,FALSE, 2);
  gtk_signal_connect(GTK_OBJECT(scroll_radio_button_1), "toggled",
		     GTK_SIGNAL_FUNC (on_display_control_map_scroll_radio_button_toggled),
		     GINT_TO_POINTER(n));
  if (scroll_wheel_map() == n) {
     gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scroll_radio_button_1), TRUE);
  }
/*   gtk_object_set_user_data (GTK_OBJECT (scroll_radio_button_1), (char *) n);  */

/* -- */

  strcpy(widget_name, "properties_button_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n);

  displayed_button_1 = gtk_button_new_with_label (_("Properties"));
  gtk_widget_ref (displayed_button_1);
  gtk_object_set_data_full (GTK_OBJECT (display_control_window_glade), 
			    widget_name, 
			    displayed_button_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (displayed_button_1);
  gtk_box_pack_start (GTK_BOX (hbox32), displayed_button_1, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (displayed_button_1), 2);

  gtk_signal_connect(GTK_OBJECT (displayed_button_1),  "clicked",
		     GTK_SIGNAL_FUNC (on_display_control_map_properties_button_clicked),
		     GINT_TO_POINTER(n));

/*   // associate with the button a pointer to the variable which */
/*   // contains the passed variable int n */
/*   //  */
  /*   // we cast as a (char *) to make the compiler happy. */
/*   //  */
    gtk_object_set_user_data (GTK_OBJECT (displayed_button_1), GINT_TO_POINTER(n));

  // A delete molecule button
  display_control_add_delete_molecule_button(n, hbox32);

  free(widget_name); 
  return my_combo_box; 
}


void
on_display_control_map_displayed_button_toggled   (GtkToggleButton       *button,
						   gpointer         user_data)
{

  int imol = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(button)));

  if (button->active)
    set_map_displayed(imol, 1);
  else 
    set_map_displayed(imol, 0);
}

/* Added 20050316 (Bangalore) */
void
on_display_control_map_scroll_radio_button_toggled (GtkToggleButton *button,
						    gpointer         user_data) {
  int imol = GPOINTER_TO_INT(user_data);
  const char *state = "inactive";
  if (GTK_TOGGLE_BUTTON(button)->active) {
    state = "active";
    set_scrollable_map(imol);
  }
}

void
on_display_control_delete_molecule_button_clicked   (GtkButton       *button,
						   gpointer         user_data)
{

   int imol = GPOINTER_TO_INT(user_data);
   close_molecule(imol);
}

void
on_display_control_map_properties_button_clicked   (GtkButton       *button,
						   gpointer         user_data)
{

/* Remove (comment out) archaic use of casting int * for user data. */
  int imol = GPOINTER_TO_INT(user_data);
  GtkWidget *frame;
  GtkWidget *window = create_single_map_properties_dialog();
  GtkWidget *patch_frame = lookup_widget(window, 
					 "single_map_colour_button_frame");
  GtkWidget *single_map_properties_colour_button = 
    lookup_widget(window, "single_map_properties_colour_button");
  GtkWidget *label = lookup_widget(window, "label114");


  // FIXME? block commented.  Did it do anything?
//   // chunk from the glade FAQ:
//   GdkColor red = { 0, 65535, 0, 0 };
//   GtkRcStyle *rc_style = gtk_rc_style_new ();
//   rc_style->bg[GTK_STATE_NORMAL] = red;
//   rc_style->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BG;
//   // gtk_widget_modify_style (single_map_properties_colour_button, rc_style);
//   // can't set patch frame fg usefully.
//   gtk_widget_modify_style (label, rc_style);
//   gtk_rc_style_unref (rc_style);

  fill_single_map_properties_dialog(window, imol);
  gtk_object_set_user_data(GTK_OBJECT(window), GINT_TO_POINTER(imol));
  /*  and now the skeleton buttons */
  frame = lookup_widget(window, "single_map_skeleton_frame");
  set_on_off_single_map_skeleton_radio_buttons(frame, imol);
  /* contour by sigma step */
  set_contour_sigma_button_and_entry(window, imol);
  gtk_widget_show(window);
}

void
fill_map_colour_patch(GtkWidget *patch_frame, int imol){ 

   GdkColor *color;
  int width, height;
  GtkWidget *widget;
  GdkGC *gc;
  double *mol_colour = get_map_colour(imol);
  gushort red, green, blue;
  GtkWidget *widget_thing;

  red   =  gushort (mol_colour[0] * 254.0);
  green =  gushort (mol_colour[1] * 254.0);
  blue  =  gushort (mol_colour[2] * 254.0);

  widget = patch_frame; 


  widget = gtk_drawing_area_new();  
  widget_thing = lookup_widget(GTK_WIDGET(patch_frame), "single_map_colour_hbox");
  widget_thing = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  
  printf("adding widget to patch_frame\n");
  gtk_container_add(GTK_CONTAINER(widget_thing), widget);

  // printf("gdk_gc_new\n");
  gc = gdk_gc_new(widget->window);

  printf("get window size\n");
  /* find proper dimensions for rectangle */
  gdk_window_get_size(widget->window, &width, &height);

  /* the color we want to use */
  color = (GdkColor *)malloc(sizeof(GdkColor));
  
  /* red, green, and blue are passed values, indicating the RGB triple
   * of the color we want to draw. Note that the values of the RGB components
   * within the GdkColor are taken from 0 to 65535, not 0 to 255.
   */
  color->red = red * (65535/255);
  color->green = green * (65535/255);
  color->blue = blue * (65535/255);

  
  /* the pixel value indicates the index in the colormap of the color.
   * it is simply a combination of the RGB values we set earlier
   */
  color->pixel = (gulong)(red*65536 + green*256 + blue);

  /* However, the pixel valule is only truly valid on 24-bit (TrueColor)
   * displays. Therefore, this call is required so that GDK and X can
   * give us the closest color available in the colormap
   */
  printf("colour alloc\n");
  gdk_color_alloc(gtk_widget_get_colormap(widget), color);

  /* set the foreground to our color */
  printf("set background\n");
  gdk_gc_set_background(gc, color);
  
  /* draw the rectangle */
  printf("draw rectangle:\n");
  gdk_draw_rectangle(widget->window, gc, 1, 0, 0, width, height);
} 



void
on_display_control_mol_displayed_button_toggled   (GtkToggleButton       *button,
						   gpointer         user_data)
{
  int imol = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(button))); 
  int idisplay; 
  GtkWidget *active_toggle_button;
  char *widget_name = (char *) malloc(100);
  char *tmp_name;

/*   printf("DEBUG::  display toggle of molecule: %d\n", imol); */

  strcpy(widget_name, "active_button_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", imol);

/*   printf("mol display button clicked %d, active: %d\n", *imol, button->active); */

  if (imol >= 0 && imol < graphics_n_molecules()) {
    if (button->active)
      set_mol_displayed(imol, 1);
    else 
      set_mol_displayed(imol, 0);
      
/*     printf("looking up widget name %s\n", widget_name); */
    active_toggle_button = lookup_widget(GTK_WIDGET(button), widget_name);
    if (active_toggle_button) { 
      /*  printf("INFO:: Got active_toggle_button from name: %s\n", widget_name); */

      if (mol_is_displayed(imol)) {
	// activate the button
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_toggle_button), TRUE);
      } else {
	/* deactivate the button */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_toggle_button), FALSE);
      }



    } else {
      printf("ERROR:: Failed to find active_toggle_button from name: %s\n", widget_name);
    }
  } else { 
    printf("ERROR:: (ignoring) display toggle of bogus molecule: %d\n", imol);
  }
  free(widget_name);
}


void
on_display_control_mol_active_button_toggled   (GtkToggleButton  *toggle_button,
						gpointer         user_data)
{
  int imol = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(toggle_button))); 
  int iactive; 
  if (toggle_button->active) { 
    set_mol_active(imol, 1);
/*     iactive = toggle_active_mol(*imol);  */
  } else {
    set_mol_active(imol, 0);
  }
}


/* ------------------------------------------------------------------------------ */
/*              cell chooser for phs                                              */
/* ------------------------------------------------------------------------------ */

GSList *display_cell_chooser_box(GtkWidget *phs_cell_choice_window,
				 GSList *phs_cell_group, 
				 int n) {

  GtkWidget *vbox39;
  GtkWidget *phs_cell_chooser_vbox;
  GtkWidget *hbox33;
  GtkWidget *phs_cell_radiobutton_1;
  GtkWidget *label53;
  GtkWidget *phs_cell_symm_entry_1;
  GtkWidget *label54;
  GtkWidget *phs_cell_a_entry_1;
  GtkWidget *label55;
  GtkWidget *phs_cell_b_entry_1;
  GtkWidget *label56;
  GtkWidget *phs_cell_c_entry_1;
  GtkWidget *label57;
  GtkWidget *phs_cell_alpha_entry_1;
  GtkWidget *label58;
  GtkWidget *phs_cell_beta_entry_1;
  GtkWidget *label59;
  GtkWidget *phs_cell_gamma_entry_1;
  GtkWidget *hbox34;
  GtkWidget *phs_cell_none_radiobutton;
  GtkWidget *hbox32;
  GtkWidget *phs_cell_choice_ok_button;
  GtkWidget *phs_cell_choice_cancel_button;

/* messing about with string variables */
  gchar *widget_name; 
  gchar *tmp_name; 

  widget_name = (gchar *) malloc(100);

  phs_cell_chooser_vbox = lookup_widget(phs_cell_choice_window, "phs_cell_chooser_vbox"); 

  hbox33 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox33);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "hbox33", hbox33,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox33);
  gtk_box_pack_start (GTK_BOX (phs_cell_chooser_vbox), hbox33, TRUE, TRUE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox33), 6);

  strcpy(widget_name, "phs_cell_radiobutton_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_radiobutton_1 = gtk_radio_button_new_with_label (phs_cell_group, "");
  phs_cell_group = gtk_radio_button_group (GTK_RADIO_BUTTON (phs_cell_radiobutton_1));
  gtk_widget_ref (phs_cell_radiobutton_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name, 
			    phs_cell_radiobutton_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (phs_cell_radiobutton_1), FALSE); 

  gtk_widget_show (phs_cell_radiobutton_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_radiobutton_1, FALSE, FALSE, 4);

  label53 = gtk_label_new (_("Symm"));
  gtk_widget_ref (label53);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "label53", label53,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label53);
  gtk_box_pack_start (GTK_BOX (hbox33), label53, FALSE, FALSE, 2);


  strcpy(widget_name, "phs_cell_symm_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_symm_entry_1 = gtk_entry_new ();
  gtk_widget_ref (phs_cell_symm_entry_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name, 
			    phs_cell_symm_entry_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phs_cell_symm_entry_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_symm_entry_1, TRUE, TRUE, 0);
  gtk_widget_set_usize (phs_cell_symm_entry_1, 80, -2);

  label54 = gtk_label_new (_("a"));
  gtk_widget_ref (label54);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "label54", label54,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label54);
  gtk_box_pack_start (GTK_BOX (hbox33), label54, FALSE, FALSE, 2);


  strcpy(widget_name, "phs_cell_a_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_a_entry_1 = gtk_entry_new ();
  gtk_widget_ref (phs_cell_a_entry_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name, 
			    phs_cell_a_entry_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phs_cell_a_entry_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_a_entry_1, TRUE, TRUE, 0);
  gtk_widget_set_usize (phs_cell_a_entry_1, 65, -2);

  label55 = gtk_label_new (_("b"));
  gtk_widget_ref (label55);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "label55", label55,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label55);
  gtk_box_pack_start (GTK_BOX (hbox33), label55, FALSE, FALSE, 2);


  strcpy(widget_name, "phs_cell_b_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_b_entry_1 = gtk_entry_new ();
  gtk_widget_ref (phs_cell_b_entry_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name, 
			    phs_cell_b_entry_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phs_cell_b_entry_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_b_entry_1, TRUE, TRUE, 0);
  gtk_widget_set_usize (phs_cell_b_entry_1, 65, -2);

  label56 = gtk_label_new (_("c"));
  gtk_widget_ref (label56);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "label56", label56,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label56);
  gtk_box_pack_start (GTK_BOX (hbox33), label56, FALSE, FALSE, 2);


  strcpy(widget_name, "phs_cell_c_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_c_entry_1 = gtk_entry_new ();
  gtk_widget_ref (phs_cell_c_entry_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name, 
			    phs_cell_c_entry_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phs_cell_c_entry_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_c_entry_1, TRUE, TRUE, 0);
  gtk_widget_set_usize (phs_cell_c_entry_1, 65, -2);

  label57 = gtk_label_new (_("alpha"));
  gtk_widget_ref (label57);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "label57", label57,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label57);
  gtk_box_pack_start (GTK_BOX (hbox33), label57, FALSE, FALSE, 2);


  strcpy(widget_name, "phs_cell_alpha_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_alpha_entry_1 = gtk_entry_new ();
  gtk_widget_ref (phs_cell_alpha_entry_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name, 
			    phs_cell_alpha_entry_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phs_cell_alpha_entry_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_alpha_entry_1, TRUE, TRUE, 0);
  gtk_widget_set_usize (phs_cell_alpha_entry_1, 60, -2);

  label58 = gtk_label_new (_("beta"));
  gtk_widget_ref (label58);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "label58", label58,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label58);
  gtk_box_pack_start (GTK_BOX (hbox33), label58, FALSE, FALSE, 2);


  strcpy(widget_name, "phs_cell_beta_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_beta_entry_1 = gtk_entry_new ();
  gtk_widget_ref (phs_cell_beta_entry_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name,
			    phs_cell_beta_entry_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phs_cell_beta_entry_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_beta_entry_1, TRUE, TRUE, 0);
  gtk_widget_set_usize (phs_cell_beta_entry_1, 60, -2);

  label59 = gtk_label_new (_("gamma"));
  gtk_widget_ref (label59);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "label59", label59,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label59);
  gtk_box_pack_start (GTK_BOX (hbox33), label59, FALSE, FALSE, 2);


  strcpy(widget_name, "phs_cell_gamma_entry_"); 
  tmp_name = widget_name + strlen(widget_name); 
  snprintf(tmp_name, 4, "%-d", n); 


  phs_cell_gamma_entry_1 = gtk_entry_new ();
  gtk_widget_ref (phs_cell_gamma_entry_1);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), widget_name, 
			    phs_cell_gamma_entry_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (phs_cell_gamma_entry_1);
  gtk_box_pack_start (GTK_BOX (hbox33), phs_cell_gamma_entry_1, TRUE, TRUE, 0);
  gtk_widget_set_usize (phs_cell_gamma_entry_1, 60, -2);

  return phs_cell_group; 

}


/* ------------------------------------------------------------------------------ */
/*     None of the above cell chooser for phs                                     */
/* ------------------------------------------------------------------------------ */

void display_none_cell_chooser_box(GtkWidget *phs_cell_choice_window, 
				   GSList *phs_cell_group) {

  GtkWidget *hbox34;
  GtkWidget *phs_cell_chooser_vbox;
  GtkWidget *phs_cell_none_radiobutton;


  phs_cell_chooser_vbox = lookup_widget(phs_cell_choice_window, "phs_cell_chooser_vbox"); 

  hbox34 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox34);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "hbox34", hbox34,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox34);
  gtk_box_pack_start (GTK_BOX (phs_cell_chooser_vbox), hbox34, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox34), 6);

  phs_cell_none_radiobutton = gtk_radio_button_new_with_label (phs_cell_group, _("None of the Above"));
  phs_cell_group = gtk_radio_button_group (GTK_RADIO_BUTTON (phs_cell_none_radiobutton));
  gtk_widget_ref (phs_cell_none_radiobutton);
  gtk_object_set_data_full (GTK_OBJECT (phs_cell_choice_window), "phs_cell_none_radiobutton", 
			    phs_cell_none_radiobutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (phs_cell_none_radiobutton), TRUE); 

  gtk_widget_show (phs_cell_none_radiobutton);
  gtk_box_pack_start (GTK_BOX (hbox34), phs_cell_none_radiobutton, FALSE, FALSE, 4);

}



/* ------------------------------------------------------------------------------ */
/* Additional Representation Handling                                             */
/* ------------------------------------------------------------------------------ */

// Return the vbox for the Add Reps, (which will then be used in
// display_control_add_reps().
//
// It's a bit ugly to send back a widget to the molecule_info_class_t
// function.  If you want to remove that in future (make this function
// a void), then you'll have to set a name on the vbox that can be
// looked up by display_control_add_reps().
//
//
// Function no longer needed, frame is made by
// display_control_molecule_combo_box().
// 
GtkWidget *
display_control_add_reps_container(GtkWidget *display_control_window_glade,
				   int imol_no) {

   GtkWidget *w = NULL;

   if (display_control_window_glade) {
      std::string name = "add_rep_display_control_frame_vbox_";
      name += coot::util::int_to_string(imol_no);
      GtkWidget *t = lookup_widget(display_control_window_glade, name.c_str());
      if (t)
	 w = t;
      else
	 std::cout << "ERROR:: in display_control_add_reps_frame failed to lookup "
		   << name << " widget" << std::endl;
   }
   return w;
}

GtkWidget *
display_control_add_reps_frame(GtkWidget *display_control_window_glade,
			       int imol_no) {

   GtkWidget *w = NULL;

   if (display_control_window_glade) {
      std::string name = "add_rep_display_control_frame_";
      name += coot::util::int_to_string(imol_no);
      GtkWidget *t = lookup_widget(display_control_window_glade, name.c_str());
      if (t)
	 w = t;
      else
	 std::cout << "ERROR:: in display_control_add_reps_container failed to lookup "
		   << name << " widget" << std::endl;
   } 
   return w;
}



void display_control_add_reps(GtkWidget *display_control_window_glade,
			      int imol_no, int add_rep_no,
			      bool show_it,
			      int bonds_box_type, const std::string &name) {

   if (display_control_window_glade) { 
      GtkWidget *add_rep_vbox  = display_control_add_reps_container(display_control_window_glade, imol_no);
      GtkWidget *add_rep_frame = display_control_add_reps_frame(    display_control_window_glade, imol_no);
//       std::cout <<  "DEBUG::  add_rep_vbox is " << add_rep_vbox  << std::endl;
//       std::cout <<  "DEBUG:: add_rep_frame is " << add_rep_frame << std::endl;

      GtkWidget *hbox = gtk_hbox_new(FALSE, 2);
      gtk_widget_ref (hbox);
      gtk_box_pack_start(GTK_BOX(add_rep_vbox), hbox, FALSE, FALSE, 0);
      std::string label = name;
      GtkWidget *toggle_button_show_it = gtk_check_button_new_with_label(label.c_str());
      gtk_widget_ref (toggle_button_show_it);
      if (show_it)
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button_show_it), TRUE);
      int cc = encode_ints(imol_no, add_rep_no);
      gtk_signal_connect(GTK_OBJECT(toggle_button_show_it), "toggled",
			 GTK_SIGNAL_FUNC(add_rep_toggle_button_toggled),
			 GINT_TO_POINTER(cc));
      gtk_box_pack_start(GTK_BOX(hbox), toggle_button_show_it, FALSE, FALSE, 0);
      
      gtk_widget_show(toggle_button_show_it);
      gtk_widget_show(hbox);
      gtk_widget_show(add_rep_vbox);
      gtk_widget_show(add_rep_frame);
   }
} 

void add_rep_toggle_button_toggled(GtkToggleButton       *button,
				   gpointer         user_data) {
   
   std::pair<int, int> p = decode_ints(GPOINTER_TO_INT(user_data));
   int on_off_flag = 0;
   if (button->active)
      on_off_flag = 1;
   set_show_additional_representation(p.first, p.second, on_off_flag);
} 


#ifdef COOT_USE_GTK2_INTERFACE
/* Hack in a function, because it's missing somehow from Bernhard's
   commits */

/* I dont think this should be here at all!
   I think it should be in gtk2-interface.c !
   But somehow it disappeared from there, so I will put it back!!!
   Where did all my things go........ */ 

#endif	/* COOT_USE_GTK2_INTERFACE */
