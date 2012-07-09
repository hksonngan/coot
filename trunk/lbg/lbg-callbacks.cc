/* lbg/lbg-callbacks.cc
 * 
 * Author: Paul Emsley
 * Copyright 2010 by The University of Oxford
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

#ifdef HAVE_GOOCANVAS

#ifdef USE_PYTHON
#include <Python.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h> // for system()

#include "lbg.hh"


extern "C" G_MODULE_EXPORT void
on_lbg_apply_button_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (!l) {
      std::cout << "failed to get lbg_info_t from " << canvas << std::endl;
   } else {
      // l->mol.debug();
      l->mol.write_mdl_molfile("prodrg-in.mdl");
      l->import_prodrg_output("prodrg-in.mdl");
   } 
}

extern "C" G_MODULE_EXPORT void
on_lbg_close_button_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l->is_stand_alone())
      gtk_exit(0);
   else
      gtk_widget_hide(l->lbg_window);
}

extern "C" G_MODULE_EXPORT void
on_lbg_search_button_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   l->search();
}

extern "C" G_MODULE_EXPORT void
on_carbon_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_C);
}

extern "C" G_MODULE_EXPORT void
on_other_element_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_X);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      l->show_atom_x_dialog();
   } 
}

extern "C" G_MODULE_EXPORT void
on_iodine_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_I);
}

extern "C" G_MODULE_EXPORT void
on_bromine_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_BR);
}

extern "C" G_MODULE_EXPORT void
on_chlorine_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_CL);
}

extern "C" G_MODULE_EXPORT void
on_fluorine_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_F);
}

extern "C" G_MODULE_EXPORT void
on_phos_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_P);
}

extern "C" G_MODULE_EXPORT void
on_sulfur_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_S);
}

extern "C" G_MODULE_EXPORT void
on_oxygen_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_O);
}

extern "C" G_MODULE_EXPORT void
on_nitrogen_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::ATOM_N);
}

extern "C" G_MODULE_EXPORT void
on_info_toolbutton_clicked(GtkToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   // std::cout << "lbg_info_t pointer is " << l << std::endl;
   if (l) {
      GtkWidget *w = l->about_dialog;
      // std::cout << "about_dialog widget is " << w << std::endl;
      gtk_widget_show(w);

   } else {
      std::cout << "failed to get lbg_info_t from canvas " << canvas << " " << user_data << std::endl;
   } 
}

extern "C" G_MODULE_EXPORT void
on_about_menuitem_activate(GtkMenuItem *button, gpointer user_data) {
   
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      GtkWidget *w = l->about_dialog;
      gtk_widget_show(w);
   } else {
      std::cout << "failed to get lbg_info_t from canvas " << canvas << " "
		<< user_data << std::endl;
   } 
}

extern "C" G_MODULE_EXPORT void
on_undo_toolbutton_clicked(GtkToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l)
      l->undo();
}

extern "C" G_MODULE_EXPORT void
on_delete_item_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::DELETE_MODE);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) { 
      if (gtk_toggle_tool_button_get_active(button)) {
	 l->set_in_delete_mode(1);
      } else {
	 l->set_in_delete_mode(0);
      }
   }
}

extern "C" G_MODULE_EXPORT void
on_clear_toolbutton_clicked(GtkToolButton *button, gpointer user_data) {

   GtkWidget *canvas = get_canvas_from_scrolled_win(GTK_WIDGET(user_data));
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l)
      l->clear();
}

extern "C" G_MODULE_EXPORT void
on_charge_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {

   // lbg_info_t::canvas_addition_mode = lbg_info_t::CHARGE;
   std::cout << "Somehow set canvas_addition_mode it CHARGE - Fixme "
	     << std::endl;
} 

extern "C" G_MODULE_EXPORT void
on_single_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::ADD_SINGLE_BOND);
} 

extern "C" G_MODULE_EXPORT void
on_double_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::ADD_DOUBLE_BOND);
} 

extern "C" G_MODULE_EXPORT void
on_triple_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::ADD_TRIPLE_BOND);
} 

extern "C" G_MODULE_EXPORT void
on_c3_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
				 gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::TRIANGLE);
}

extern "C" G_MODULE_EXPORT void
on_c4_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
				 gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::SQUARE);
} 

extern "C" G_MODULE_EXPORT void
on_c5_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
				 gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::PENTAGON);
} 

extern "C" G_MODULE_EXPORT void
on_c6_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::HEXAGON);
} 

extern "C" G_MODULE_EXPORT void
on_c6_arom_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::HEXAGON_AROMATIC);
} 

extern "C" G_MODULE_EXPORT void
on_c7_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::HEPTAGON);
} 
extern "C" G_MODULE_EXPORT void
on_c8_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
			       gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::OCTAGON);
} 

extern "C" G_MODULE_EXPORT void
on_stereo_out_toggle_toolbutton_toggled (GtkToggleToolButton *togglebutton,
					 gpointer         user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::ADD_STEREO_OUT_BOND);

}

extern "C" G_MODULE_EXPORT void
on_lbg_save_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
	 l->write_mdl_molfile_using_default_file_name();
   }
}


// Print?  No, this is saving as PDF button clicked.
// 
extern "C" G_MODULE_EXPORT void
on_print_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(l->lbg_export_as_pdf_dialog), "coot.pdf");
      gtk_widget_show(l->lbg_export_as_pdf_dialog);
   }
}

extern "C" G_MODULE_EXPORT void
on_export_as_png_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(l->lbg_export_as_png_dialog), "coot.png");
     gtk_widget_show(l->lbg_export_as_png_dialog);
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_aboutdialog_response (GtkDialog       *dialog,
			     gint             response_id,
			     gpointer         user_data) {

  GtkWidget *canvas = GTK_WIDGET(user_data);
  lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
  if (l) {
     GtkWidget *w = l->about_dialog;
     gtk_widget_hide(w);
  } else {
     std::cout << "failed to get lbg_info_t from canvas " << canvas << " " << user_data << std::endl;
  } 
}


extern "C" G_MODULE_EXPORT void
on_lbg_aboutdialog_close (GtkDialog       *dialog,
			  gpointer         user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      GtkWidget *w = l->about_dialog;
      gtk_widget_hide(w);
   } else {
      std::cout << "failed to get lbg_info_t from canvas " << canvas << " " << user_data << std::endl;
   } 
}


extern "C" G_MODULE_EXPORT void
on_lbg_open_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {

  GtkWidget *canvas = GTK_WIDGET(user_data);
  lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
  if (l) {
     gtk_widget_show(l->open_dialog);
  }
}

extern "C" G_MODULE_EXPORT void
on_lbg_open_filechooserdialog_response(GtkDialog       *dialog,
				       gint             response_id,
				       gpointer         user_data){

   if (response_id == GTK_RESPONSE_OK) { 
      GtkWidget *canvas = GTK_WIDGET(user_data);
      lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
      if (l) {
	 std::string file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l->open_dialog));
	 l->import_mol_from_file(file_name);
      }
   }
   gtk_widget_hide(GTK_WIDGET(dialog));
}

extern "C" G_MODULE_EXPORT void
on_lbg_open_filechooserdialog_close(GtkDialog       *dialog,
				    gpointer         user_data){
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_widget_hide(l->open_dialog);
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_sbase_search_results_dialog_response(GtkDialog       *dialog,
					    gint             response_id,
					    gpointer         user_data) {

   if (response_id == GTK_RESPONSE_CLOSE) { 
      GtkWidget *canvas = GTK_WIDGET(user_data);
      lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
      if (l) {
	 gtk_widget_hide(GTK_WIDGET(dialog));
      }
   }
}


extern "C" G_MODULE_EXPORT void
on_lbg_save_as_filechooserdialog_close(GtkDialog       *dialog,
				       gpointer         user_data) {
   std::cout << "on_lbg_save_as_filechooserdialog_close() " << std::endl;
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_widget_hide(GTK_WIDGET(dialog));
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_save_as_filechooserdialog_response(GtkDialog       *dialog,
					  gint             response_id,
					  gpointer         user_data){

   if (response_id == GTK_RESPONSE_OK) { 
      GtkWidget *canvas = GTK_WIDGET(user_data);
      lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
      if (l) {
	 std::string file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l->save_as_dialog));
	 l->set_default_mdl_file_name(file_name);
	 l->mol.write_mdl_molfile(file_name);
      }
   }
   gtk_widget_hide(GTK_WIDGET(dialog));
}


extern "C" G_MODULE_EXPORT void
on_lbg_save_as_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(l->save_as_dialog), "coot.mol");
      gtk_widget_show(l->save_as_dialog);
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_new_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      l->clear();
   }
}


extern "C" G_MODULE_EXPORT void
on_lbg_export_as_pdf_filechooserdialog_close(GtkDialog       *dialog,
				       gpointer         user_data){
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_widget_hide(GTK_WIDGET(dialog));
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_export_as_pdf_filechooserdialog_response(GtkDialog       *dialog,
					  gint             response_id,
					  gpointer         user_data){

   if (response_id == GTK_RESPONSE_OK) { 
      GtkWidget *canvas = GTK_WIDGET(user_data);
      lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
      if (l) {
	 std::string file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l->lbg_export_as_pdf_dialog));
	 l->write_pdf(file_name);
      }
   }
   gtk_widget_hide(GTK_WIDGET(dialog));
}

extern "C" G_MODULE_EXPORT void
on_lbg_close_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {

  GtkWidget *canvas = GTK_WIDGET(user_data);
  lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
  if (l) {
     if (l->is_stand_alone())
	gtk_exit(0);
     else
	gtk_widget_hide(l->lbg_window);
  }
}

extern "C" G_MODULE_EXPORT void
on_libcheckify_button_clicked(GtkButton *button, gpointer user_data) {
   
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      std::string fn = "coot-lbg-for-libcheck-minimal.cif";
      std::cout << "writing cif file " << fn << std::endl;
      l->mol.write_minimal_cif_file(fn);
   } else {
      std::cout << "failed to find lbg_info_t pointer" << std::endl;
   } 
}


extern "C" G_MODULE_EXPORT void
on_lbg_export_as_png_filechooserdialog_close(GtkDialog       *dialog,
				       gpointer         user_data){
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_widget_hide(GTK_WIDGET(dialog));
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_export_as_png_filechooserdialog_response(GtkDialog       *dialog,
					  gint             response_id,
					  gpointer         user_data){

   std::cout << "response " << response_id << std::endl;
   if (response_id == GTK_RESPONSE_OK) { 
      GtkWidget *canvas = GTK_WIDGET(user_data);
      lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
      if (l) {
	 std::string file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(l->lbg_export_as_png_dialog));
	 l->write_png(file_name);
      }
   } else {
      // std::cout << "not an OK response" << std::endl;
   } 
   gtk_widget_hide(GTK_WIDGET(dialog));
}

extern "C" G_MODULE_EXPORT void
on_residue_circles_toolbutton_clicked(GtkToolButton *button, gpointer user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {

      if (l->is_stand_alone()) { 
	 // l->read_draw_residues("../../build-coot-ubuntu-64bit/src/coot-tmp-fle-view-residue-info.txt");
	 std::string f = l->get_flev_analysis_files_dir();
	 f += "/coot-tmp-fle-view-residue-info.txt";
	 l->read_draw_residues(f);
      } else {
	 // normal enterprise, built-in case
	 l->draw_all_flev_annotations();
      } 
   } else {
	 std::cout << "Bad lbg_info_t lookup in on_residue_cirlces_toolbutton_clicked" << std::endl;
   }
}

extern "C" G_MODULE_EXPORT void
on_residue_circles_toggle_toolbutton_toggled(GtkToggleToolButton *toggle_button, gpointer user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {

      if (gtk_toggle_tool_button_get_active(toggle_button)) {
	 // on
	 l->set_draw_flev_annotations(true);
	 l->draw_all_flev_annotations();
      } else {
	 // off
	 l->set_draw_flev_annotations(false);
	 lig_build::pos_t zero_delta(0,0); // dummy, zero is now checked in clear_and_redraw()
	 l->clear_and_redraw(zero_delta);
      } 
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_smiles_toolbutton_clicked(GtkToolButton *button, gpointer user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {

      std::string smiles_text = l->get_smiles_string_from_mol();
      if (smiles_text.length()) { 
	 gtk_entry_set_text(GTK_ENTRY(l->lbg_smiles_entry), smiles_text.c_str());
	 gtk_widget_show(l->lbg_smiles_dialog);
      }
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_smiles_dialog_ok_button_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (!l) {
      std::cout << "failed to get lbg_info_t from " << canvas << std::endl;
   } else {
      gtk_widget_hide(l->lbg_smiles_dialog);
   } 
}


extern "C" G_MODULE_EXPORT void
on_lbg_smiles_dialog_close (GtkDialog *dialog,
			    gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      gtk_widget_hide(GTK_WIDGET(dialog));
   } else {
      std::cout << "failed to get lbg_info_t from canvas " << canvas << " " << user_data << std::endl;
   } 
}


extern "C" G_MODULE_EXPORT void
on_lbg_key_toggle_toolbutton_toggled(GtkToggleToolButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(button, canvas, lbg_info_t::DELETE_MODE);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) { 
      if (gtk_toggle_tool_button_get_active(button)) {
	 l->show_key();
      } else {
	 l->hide_key();
      }
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_charge_toolbutton_toggled(GtkToggleToolButton *togglebutton, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_handle_toggle_button(togglebutton, canvas, lbg_info_t::CHARGE);
}

extern "C" G_MODULE_EXPORT void
on_lbg_delete_hydrogens_toolbutton_clicked(GtkToolButton *button, gpointer user_data) {
   
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      l->delete_hydrogens();
   } 
}


extern "C" G_MODULE_EXPORT void
on_lbg_atom_x_dialog_close_button_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (!l) {
      std::cout << "failed to get lbg_info_t from " << canvas << std::endl;
   } else {
      gtk_widget_hide(l->lbg_atom_x_dialog);
   } 
}


extern "C" G_MODULE_EXPORT void
on_lbg_atom_x_entry_insert_at_cursor(GtkEntry *entry,
				     char *string,
				     gpointer  user_data) {
}

extern "C" G_MODULE_EXPORT void
on_lbg_atom_x_entry_changed (GtkEditable *editable,
			     gpointer     user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (!l) {
      std::cout << "failed to get lbg_info_t from " << canvas << std::endl;
   } else {
      l->set_atom_x_string(gtk_entry_get_text(GTK_ENTRY(editable)));
   }
} 


extern "C" G_MODULE_EXPORT void
on_lbg_get_drug_entry_changed (GtkEditable *editable,
			       gpointer     user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (!l) {
      std::cout << "failed to get lbg_info_t from " << canvas << std::endl;
   } else {
      // look for 
      // if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)...
      // what is event?
   }
}

extern "C" G_MODULE_EXPORT void
on_lbg_get_drug_ok_button_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (!l) {
      std::cout << "failed to get lbg_info_t from " << canvas << std::endl;
   } else {
      l->get_drug_using_entry_text();
      gtk_widget_hide(l->lbg_get_drug_dialog);
   } 
}

extern "C" G_MODULE_EXPORT void
on_lbg_get_drug_cancel_button_clicked(GtkButton *button, gpointer user_data) {
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (!l) {
      std::cout << "failed to get lbg_info_t from " << canvas << std::endl;
   } else {
      gtk_widget_hide(l->lbg_get_drug_dialog);
   }
}


extern "C" G_MODULE_EXPORT void
on_lbg_get_drug_menuitem_activate (GtkMenuItem *item, gpointer         user_data) {
   
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      GtkWidget *w = l->lbg_get_drug_dialog;
      gtk_widget_show(w);
   } else {
      std::cout << "failed to get lbg_info_t from canvas " << canvas << " "
		<< user_data << std::endl;
   } 
}

extern "C" G_MODULE_EXPORT void
on_lbg_show_alerts_checkbutton_toggled(GtkToggleButton *togglebutton,
				       gpointer         user_data) {

   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(canvas)));
   if (l) {
      if (togglebutton->active) {
	 gtk_widget_show(l->lbg_alert_hbox_outer);
	 l->show_alerts_user_control = true;
	 l->update_descriptor_attributes();
      } else {
	 gtk_widget_hide(l->lbg_alert_hbox_outer);
	 l->show_alerts_user_control = false;
	 l->clear_canvas_alerts();
      }
   }
}



#endif // HAVE_GOOCANVAS

