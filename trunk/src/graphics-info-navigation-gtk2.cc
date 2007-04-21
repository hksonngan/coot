/* src/c-interface.cc
 * 
 * Copyright 2007 The University of York
 * written by Paul Emsley
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
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <gtk/gtk.h>
#if (GTK_MAJOR_VERSION == 1) || defined (GTK_ENABLE_BROKEN)

# else 

#include <sys/types.h> // for stating
#include <sys/stat.h>
#if !defined _MSC_VER
#include <unistd.h>
#else
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE
#define S_IXUSR S_IEXEC
#define sleep Sleep
#include <windows.h>
#include <direct.h>
#endif // _MSC_VER

#include "graphics-info.h"

enum {
   CHAIN_COL,
   RESIDUE_COL
};

// static
void
graphics_info_t::fill_go_to_atom_window_gtk2(GtkWidget *go_to_atom_window,
					     GtkWidget *residue_tree_scrolled_window,
					     GtkWidget *atom_list_scrolled_window) {
     
   GtkWidget *residue_tree = gtk_tree_view_new(); 
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(residue_tree_scrolled_window),
					 residue_tree);
   gtk_widget_ref(residue_tree);
   gtk_object_set_data_full(GTK_OBJECT(go_to_atom_window),
			    "go_to_atom_residue_tree",
			    residue_tree, 
			    (GtkDestroyNotify) gtk_widget_unref);
   graphics_info_t::fill_go_to_atom_residue_tree_gtk2(residue_tree);
   gtk_widget_show(residue_tree);

   /* The atom list/tree */
   GtkWidget *scrolled_window = lookup_widget(GTK_WIDGET(go_to_atom_window),
					      "go_to_atom_atom_scrolledwindow");
   GtkTreeView *atom_tree = GTK_TREE_VIEW(gtk_tree_view_new());
   gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(scrolled_window),
					  GTK_WIDGET(atom_tree));
   /* attach the name to the widget (by hand (as interface.c does
      it) so that we can look it up in the callback of residue selection changed */
   gtk_object_set_data_full(GTK_OBJECT(go_to_atom_window), "go_to_atom_atom_list", 
			    atom_tree, 
			    (GtkDestroyNotify) gtk_widget_unref);
   
   gtk_widget_show(GTK_WIDGET(atom_tree));

}


// a static
//
// Recall that the tree is created in c-interface.cc's fill_go_to_atom_window().
void
graphics_info_t::fill_go_to_atom_residue_tree_gtk2(GtkWidget *gtktree) {

   std::cout << "filling residue tree!" << std::endl;
   
   std::string button_string;
   graphics_info_t g;

   g.go_to_atom_residue(); // sets values of unset (magic -1) go to
			   // atom residue number.

   std::vector<coot::model_view_atom_tree_chain_t> residue_chains = 
      molecules[g.go_to_atom_molecule()].model_view_residue_tree_labels();

   // so, clear the current tree:
   GtkTreeView *tv = NULL;
   if (gtktree) 
      tv = GTK_TREE_VIEW(gtktree);
   if (! tv)
      tv = GTK_TREE_VIEW(gtk_tree_view_new());
   gtk_tree_view_set_rules_hint (tv, TRUE);

   GtkTreeModel *model = gtk_tree_view_get_model(tv);
   std::cout << "model: " << model << std::endl;
   // potentially a bug here if an old model is left lying about in
   // the tree store?  That shouldn't happen though.., should it?
   bool need_renderer = 1;
   if (model) {
      std::cout << "clearing old tree store" << std::endl;
      gtk_tree_store_clear(GTK_TREE_STORE(model));
      need_renderer = 0;
   }

   // Here is the plan:
   //
   // outer loop:

   GtkTreeStore *tree_store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
   GtkTreeIter   toplevel, child;

   // what is the connection between tree_store and model?
   gtk_tree_view_set_model(GTK_TREE_VIEW(gtktree), GTK_TREE_MODEL(tree_store));
    
   for (unsigned int ichain=0; ichain<residue_chains.size(); ichain++) {
      // the chain label item e.g. "A"
      gtk_tree_store_append(GTK_TREE_STORE(tree_store), &toplevel, NULL);

      std::cout << "Adding tree item " << residue_chains[ichain].chain_id
		<< std::endl;
      gtk_tree_store_set (tree_store, &toplevel,
			  CHAIN_COL, residue_chains[ichain].chain_id.c_str(),
			  RESIDUE_COL, NULL,
			  -1);
      for (unsigned int ires=0; ires<residue_chains[ichain].tree_residue.size();
	   ires++) {
	 gtk_tree_store_append(GTK_TREE_STORE(tree_store),
			       &child, &toplevel);
	 gtk_tree_store_set(tree_store, &child,
			    CHAIN_COL,  residue_chains[ichain].tree_residue[ires].button_label.c_str(),
			    RESIDUE_COL, (gpointer)(residue_chains[ichain].tree_residue[ires].residue),
			    -1);
      }
   }

   if (need_renderer) { 
      GtkCellRenderer *cell = gtk_cell_renderer_text_new();
      GtkTreeViewColumn *column =
	 gtk_tree_view_column_new_with_attributes ("Chains", cell, "text", 0, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tv),
				   GTK_TREE_VIEW_COLUMN (column));

      GtkTreeSelection*   tree_sel = gtk_tree_view_get_selection (tv);
      gtk_tree_selection_set_mode(tree_sel, GTK_SELECTION_SINGLE);
      // double clicks
      g_signal_connect(tv, "row-activated",
		       (GCallback) residue_tree_residue_row_activated, NULL);

      gtk_tree_selection_set_select_function (tree_sel,
					      graphics_info_t::residue_tree_selection_func,
					      NULL, NULL);
   }
   
}

// static
gboolean
graphics_info_t::residue_tree_selection_func(GtkTreeSelection *selection,
					     GtkTreeModel *model,
					     GtkTreePath *path,
					     gboolean path_currently_selected,
					     gpointer data) {

   GtkTreeIter   iter;
   gboolean can_change_selected_status_flag = TRUE;
   
    if (gtk_tree_model_get_iter(model, &iter, path)) {
       gchar *name;
       gtk_tree_model_get(model, &iter, CHAIN_COL, &name, -1);
       if (!path_currently_selected) {
	  if (1) {  // if this was a residue, not a chain click
	     // update the go to atom residues from the
	     // characteristics of this row... bleurgh.. how do I do
	     // that!?  Check the level, is how I did it in gtk1...
	     graphics_info_t g;
	     int go_to_imol = g.go_to_atom_molecule();
	     if (go_to_imol<n_molecules) {
		gpointer residue_data;
		gtk_tree_model_get(model, &iter, RESIDUE_COL, &residue_data, -1);
		if (!residue_data) {
		   // This was a "Outer" chain row click (there was no residue)
		   // std::cout << "Null residue " << residue_data << std::endl;
		   std::cout << "should expand here, perhaps?" << std::endl;
		} else {
		   CResidue *res = (CResidue *) residue_data;
		   CAtom *at = molecules[go_to_imol].intelligent_this_residue_mmdb_atom(res);
		   // this does simple setting, nothing else
		   g.set_go_to_atom_chain_residue_atom_name(at->GetChainID(),
							    at->GetSeqNum(),
							    at->GetInsCode(),
							    at->name,
							    at->altLoc);
		   
		   g.update_widget_go_to_atom_values(g.go_to_atom_window, at);
		   
		   // now we want the atom list to contain the atoms of the
		   // newly selected residue:
		   
		   // Fill me...
		   fill_go_to_atom_atom_list_gtk2(g.go_to_atom_window,
						  g.go_to_atom_molecule(),
						  at->GetChainID(),
						  at->GetSeqNum(),
						  at->GetInsCode());
		}
	     }
	  }
       }
       g_free(name);
    }
    return can_change_selected_status_flag;
}


// static
void
graphics_info_t::residue_tree_residue_row_activated(GtkTreeView        *treeview,
						    GtkTreePath        *path,
						    GtkTreeViewColumn  *col,
						    gpointer            userdata) {

   // This gets called on double-clicking, and not on single clicking
   
   GtkTreeModel *model = gtk_tree_view_get_model(treeview);
   GtkTreeIter   iter;
   
    if (gtk_tree_model_get_iter(model, &iter, path)) {
       gchar *name;
       gtk_tree_model_get(model, &iter, CHAIN_COL, &name, -1);
       g_print ("Double-clicked row contains name %s\n", name);
       if (1) {
	  graphics_info_t g;
	  int go_to_imol = g.go_to_atom_molecule();
	  if (go_to_imol<n_molecules) {
	     gpointer residue_data;
	     gtk_tree_model_get(model, &iter, RESIDUE_COL, &residue_data, -1);
	     if (!residue_data) {
		// This was a "Outer" chain row click (there was no residue)
		// std::cout << "Null residue " << residue_data << std::endl;
		std::cout << "should expand here, perhaps?" << std::endl;
	     } else {
		CResidue *res = (CResidue *) residue_data;
		CAtom *at = molecules[go_to_imol].intelligent_this_residue_mmdb_atom(res);
		// this does simple setting, nothing else
		g.set_go_to_atom_chain_residue_atom_name(at->GetChainID(),
							 at->GetSeqNum(),
							 at->GetInsCode(),
							 at->name,
							 at->altLoc);
		
		g.update_widget_go_to_atom_values(g.go_to_atom_window, at);
		g.apply_go_to_atom_from_widget(go_to_atom_window);
		
	     }
	  }
       }
       g_free(name);
    }
}


// static
void
graphics_info_t::fill_go_to_atom_atom_list_gtk2(GtkWidget *go_to_atom_window, int imol,
						char *chain_id, int resno, char *ins_code) {
   
   GtkTreeView *atom_tree = GTK_TREE_VIEW(lookup_widget(go_to_atom_window, "go_to_atom_atom_list"));
   std::cout << "atom_tree:" << atom_tree << std::endl;
   bool need_renderer = 1; 
   if (!atom_tree) {
      std::cout << "making new atom_tree..." << std::endl;
      // add a new one
      atom_tree = GTK_TREE_VIEW(gtk_tree_view_new());
      GtkWidget *scrolled_window = lookup_widget(GTK_WIDGET(go_to_atom_window),
						 "go_to_atom_atom_scrolledwindow");
      gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
					    GTK_WIDGET(atom_tree));
   } else {
      std::cout << "using a pre-existing atom tree...\n";
   }

   std::vector<coot::model_view_atom_button_info_t> atoms =
      graphics_info_t::molecules[imol].model_view_atom_button_labels(chain_id, resno);

   GtkListStore *list_store = 0;
   GtkTreeModel *model = gtk_tree_view_get_model(atom_tree);
   if (!model) { 
      list_store = gtk_list_store_new (1, G_TYPE_STRING);
   } else {
      // clear (any) old atom tree/list
      list_store = GTK_LIST_STORE(model);
      gtk_list_store_clear(list_store);
      need_renderer = 0;
   }

   GtkTreeIter   toplevel;
   gtk_tree_view_set_model(GTK_TREE_VIEW(atom_tree), GTK_TREE_MODEL(list_store));

   for(unsigned int iatom=0; iatom<atoms.size(); iatom++) {
      gtk_list_store_append(GTK_LIST_STORE(list_store), &toplevel);
      gtk_list_store_set(GTK_LIST_STORE(list_store), &toplevel,
			 CHAIN_COL, atoms[iatom].button_label.c_str(),
			 RESIDUE_COL, (gpointer) atoms[iatom].atom,
			 -1);
   }

   if (need_renderer) { 
      GtkCellRenderer *cell = gtk_cell_renderer_text_new();
      GtkTreeViewColumn *column =
	 gtk_tree_view_column_new_with_attributes ("Atoms", cell, "text", 0, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (atom_tree),
				   GTK_TREE_VIEW_COLUMN (column));

      GtkTreeSelection*   tree_sel = gtk_tree_view_get_selection (atom_tree);
      gtk_tree_selection_set_mode(tree_sel, GTK_SELECTION_SINGLE);
      // double clicks
      g_signal_connect(atom_tree, "row-activated",
		       (GCallback) atom_tree_atom_row_activated, NULL);
      gtk_tree_selection_set_select_function (tree_sel,
					      graphics_info_t::atom_tree_selection_func,
					      NULL, NULL);
   }
      
}

// static
void
graphics_info_t::atom_tree_atom_row_activated(GtkTreeView        *treeview,
					      GtkTreePath        *path,
					      GtkTreeViewColumn  *col,
					      gpointer            userdata) {

   // This gets called on double-clicking, and not on single clicking
   GtkTreeModel *model = gtk_tree_view_get_model(treeview);
   GtkTreeIter   iter;
   graphics_info_t g;
   
   if (gtk_tree_model_get_iter(model, &iter, path)) {
      gchar *name;
      gpointer residue_data;
      gtk_tree_model_get(model, &iter, CHAIN_COL, &residue_data, -1);
      int go_to_imol = g.go_to_atom_molecule();
      if (go_to_imol<n_molecules) {
	 CResidue *res = (CResidue *) residue_data;
	 CAtom *at = molecules[go_to_imol].intelligent_this_residue_mmdb_atom(res);
	 // this does simple setting, nothing else
	 g.set_go_to_atom_chain_residue_atom_name(at->GetChainID(),
						  at->GetSeqNum(),
						  at->GetInsCode(),
						  at->name,
						  at->altLoc);
	 
	 g.update_widget_go_to_atom_values(g.go_to_atom_window, at);
	 g.apply_go_to_atom_from_widget(go_to_atom_window);
	 
      }
      g_free(name);
   }
}


// static
gboolean
graphics_info_t::atom_tree_selection_func(GtkTreeSelection *selection,
					  GtkTreeModel *model,
					  GtkTreePath *path,
					  gboolean path_currently_selected,
					  gpointer data) {
}

   
#endif // GTK_MAJOR_VERSION etc
