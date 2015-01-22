
#ifdef HAVE_GOOCANVAS

#ifdef USE_PYTHON
#include <Python.h>
#endif

#include <iostream>
#include "lbg-drag-and-drop.hh"

#include "lbg.hh"

extern "C" G_MODULE_EXPORT gboolean
on_lbg_drag_drop (GtkWidget *widget,
		  GdkDragContext *context,
		  gint x, gint y,
		  guint time,
		  gpointer user_data) {

   
   // gboolean is_valid_drop_site = TRUE;
   gboolean retval = FALSE;
   // Request the data from the source.
   if (context->targets) {
      GdkAtom target_type =
	 GDK_POINTER_TO_ATOM(g_list_nth_data(context->targets, TARGET_STRING));
      
      gtk_drag_get_data(widget, context,  
			target_type,    /* the target type we want (a string) */
			time);
      retval = TRUE;
   } else {
      std::cout << "ERROR:: null dnd context" << std::endl;
   }
   return retval;
}


extern "C" G_MODULE_EXPORT void
on_lbg_drag_data_received (GtkWidget *widget, 
			   GdkDragContext *context, 
			   gint x, gint y,
			   GtkSelectionData *selection_data, 
			   guint target_type, 
			   guint time,
			   gpointer user_data) {

   gboolean dnd_success = FALSE;
   gboolean delete_selection_data = FALSE;
   
   GtkWidget *canvas = GTK_WIDGET(user_data);
   lbg_info_t *l = static_cast<lbg_info_t *> (gtk_object_get_user_data(GTK_OBJECT(widget)));
   if (l) {
      // Deal with what the source sent over
      if((selection_data != NULL) && (selection_data-> length >= 0)) {
	 if (target_type == TARGET_STRING) {
	    std::string uri_string = (gchar*)selection_data-> data;
	    dnd_success = l->handle_lbg_drag_and_drop_string(uri_string);
	 } 
      }
   }
   gtk_drag_finish (context, dnd_success, delete_selection_data, time);
}



int
lbg_info_t::handle_lbg_drag_and_drop_string(const std::string &uri_in) {

   int handled = FALSE;

   std::string uri = uri_in;
   
   std::cout << "lbg:: handle this string :" << uri << ": " << std::endl;
   std::string::size_type pos = uri.find_first_of('\n');
   if (pos != std::string::npos) {
      // there was a carriage return, strip down the string
      uri = uri.substr(0, pos-1); // front part
   }
   handled = handle_lbg_drag_and_drop_single_item(uri);
   return handled;
}

int
lbg_info_t::handle_lbg_drag_and_drop_single_item(const std::string &uri) {

   int handled = FALSE;

   if (uri.length() > 42) {
      if (uri.substr(0,42) == "javascript:openWindow('/ImageView.aspx?id=") {
	 // chemspider structure image
	 handled = handle_lbg_drag_and_drop_chemspider_image(uri);
      }
   }

   if (! handled) { 
      if (uri.length() > 58) {
	 if (uri.substr(0,58) == "javascript: void window.open('/image/structurefly.cgi?cid=") {
	    // pubchem structure image
	    handled = handle_lbg_drag_and_drop_pubchem_image(uri);
	 }
	 
	 std::string::size_type ipos = uri.find("id=");
	 std::string::size_type i_w  = uri.find("window.open");
	 std::string::size_type i_j  = uri.find("javascript");
	 std::string::size_type i_o  = uri.find("open");
	 if (ipos != std::string::npos &&
	     i_w  != std::string::npos &&
	     i_j  != std::string::npos &&
	     i_o  != std::string::npos) {
	    std::cout << "------------------ trying pubchem_image " << std::endl;
	    handled = handle_lbg_drag_and_drop_pubchem_image(uri);
	 }
      }
   }
   
   if (uri.length() > 7) {
      if (uri.substr(0,7)== "file://") {
	 handled = handle_lbg_drag_and_drop_filesystem_file(uri);
      }

      if (! handled) {
      
	 if (uri.substr(0,7) == "http://") {
	 
	    if (get_url_func_ptr_flag) { // we have a function get get urls?
	       int l = uri.length();
	       std::string uri_clean = uri;
	       if (uri[l-1] == '\n') 
		  uri_clean = uri.substr(0, l-1);
	       int status = coot::util::create_directory("coot-download"); // like make_directory_maybe()
	       if (status == 0) { // OK, we made it (or had it)
		  std::string::size_type pos = uri_clean.find_last_of('/');
		  if (pos != std::string::npos) {
		     // normal path
		     std::string url_file_name_file = uri_clean.substr(pos+1);
		     std::string ext = coot::util::file_name_extension(uri_clean);

		     handled = handle_lbg_drag_and_drop_mol_file(uri_clean, url_file_name_file);

		     if (! handled) 
			handled = handle_lbg_drag_and_drop_drugbank(uri_clean, url_file_name_file);

		     if (! handled)
			handled = handle_lbg_drag_and_drop_chemspider_structure(uri_clean);

		  }
	       }
	    }
	 } else {
	    
	    // maybe it's a SMILES string?
	    std::string smi_clean = uri;
	    int l = uri.length();
	    if (uri[l-1] == '\n') 
	       smi_clean = uri.substr(0, l-1);

	    if (! handled)
	       handled = handle_lbg_drag_and_drop_smiles(smi_clean);

	 } 
      }
   }
   return handled;
}

// perhaps this should be in coot utils?
std::string
lbg_info_t::get_id_string(const std::string &s, int prefix_len, int max_len) const {

   std::string id_string;
   for (int idx=0; idx<max_len; idx++) {
      char c = s[prefix_len+idx];
      // std::cout << "considering char " << c << std::endl;
      if (c >= '0') { 
	 if (c <= '9') {
	    id_string += c;
	 } else {
	    break;
	 }
      } else {
	 break;
      }
   }
   return id_string;
}

int
lbg_info_t::get_chemspider_mol(const std::string &id_string) {

   int handled = FALSE;
   if (id_string.length() > 0) {
      std::string local_file = id_string + ".mol";
      std::string file_name = coot::util::append_dir_file("coot-download", local_file);
      std::string chemspider_mol_url = "http://www.chemspider.com/";
      chemspider_mol_url += "FilesHandler.ashx?type=str&striph=yes&id=";
      chemspider_mol_url += id_string;
      get_url_func_ptr(chemspider_mol_url.c_str(), file_name.c_str());
      import_mol_from_file(file_name);
      handled = TRUE;
   }
   return handled;
} 

int
lbg_info_t::get_pubchem_cid_mol(const std::string &id_string) {

   return get_pubchem_mol_generic(id_string, "cid");
}

int
lbg_info_t::get_pubchem_sid_mol(const std::string &id_string) {

   return get_pubchem_mol_generic(id_string, "sid");
}

int
lbg_info_t::get_pubchem_mol_generic(const std::string &id_string, const std::string &id_type) {

   std::cout << "INFO:: get_pubchem_mol:: called with " << id_string << " and type "
	     << id_type  << std::endl;
   int handled = FALSE;
   if (id_string.length() > 0) {
      std::string local_file = id_string + ".mol";
      std::string file_name = coot::util::append_dir_file("coot-download", local_file);
      std::string pc_mol_url = "http://pubchem.ncbi.nlm.nih.gov/summary/summary.cgi?";
      pc_mol_url += id_type;
      pc_mol_url += "=";
      pc_mol_url += id_string;
      pc_mol_url += "&disopt=DisplaySDF";
      std::cout << "getting url: " << pc_mol_url << std::endl;
      get_url_func_ptr(pc_mol_url.c_str(), file_name.c_str());
      import_mol_from_file(file_name);
      handled = TRUE;
   }
   return handled;
}

int
lbg_info_t::handle_lbg_drag_and_drop_chemspider_image(const std::string &uri) {

   int handled = FALSE;
   if (uri.length() > 55) {
      if (uri.substr(0,42) == "javascript:openWindow('/ImageView.aspx?id=") {
	 std::string id_string = get_id_string(uri, 58, 13);
	 handled = get_chemspider_mol(id_string);
      }
   }
   return handled;
}

int
lbg_info_t::handle_lbg_drag_and_drop_pubchem_image(const std::string &uri) {

   int handled = FALSE;
   if (uri.length() > 58) {

      // I think that this no longer matches
      if (uri.substr(0,58) == "javascript: void window.open('/image/structurefly.cgi?cid=") {
	 std::string id_string = get_id_string(uri, 58, 13);
	 handled = get_pubchem_cid_mol(id_string);
      }

      std::string types[] = { "sid", "cid" };

      for (unsigned int itype=0; itype<2; itype++) {
	 std::string test_string = types[itype] + std::string("=");
	 std::string::size_type ipos = uri.find(test_string);
	 if (ipos != std::string::npos) { 
	    if (uri.length() > (ipos + (4+2))) {
	       try {
		  std::pair<std::string, long> id = coot::util::extract_number_string(uri.substr(ipos+4));
		  handled = get_pubchem_mol_generic(id.first, types[itype]);
	       }
	       catch (const std::runtime_error &rte) {
		  std::cout << "failed to extract number from " << uri.substr(ipos+4)
			    << std::endl;
	       } 
	    }
	 }
      }
   }
   return handled;
}


int
lbg_info_t::handle_lbg_drag_and_drop_chemspider_structure(const std::string &uri) {

   int handled = FALSE;
   if (uri.length() > 7) {
      if (uri.substr(0,7) == "http://") {
	 if (uri.find("http://www.chemspider.com/Chemical-Structure.") != std::string::npos) { 
	    if (uri.find(".html") != std::string::npos) {
	       std::string id_string = get_id_string(uri, 45, 13);
	       handled = get_chemspider_mol(id_string);
	    } else {
	       std::cout << "fail 2" << std::endl;
	    }
	 } else {
	    std::cout << "fail 1 " << std::endl;
	 }
      } else {
	 std::cout << "fail 0" << std::endl;
      } 
   }
   return handled;
}


int
lbg_info_t::handle_lbg_drag_and_drop_filesystem_file(const std::string &uri) {

   int handled = FALSE;
   if (uri.length() > 7) {
      if (uri.substr(0,7)== "file://") {
	 std::cout << "---:" << uri << ": was a file:// string " << std::endl;
	 std::string file_name;

	 // Why this?
#ifdef WINDOWS_MINGW
	 file_name = uri.substr(8);
#else
	 file_name = uri.substr(7);
#endif

	 std::string ext = coot::util::file_name_extension(file_name);
	 if (ext == ".mdl" || ext == ".mol" || ext == ".mol2") { 
	    import_mol_from_file(file_name);
	 }
	 handled = TRUE;
      }
   }
   return handled;
}

int
lbg_info_t::handle_lbg_drag_and_drop_smiles(const std::string &smiles) {

   int handled = FALSE;

#ifdef MAKE_ENHANCED_LIGAND_TOOLS   
   RDKit::RWMol *rdk_mol = RDKit::SmilesToMol(smiles);
   if (rdk_mol) {
      RDDepict::compute2DCoords(*rdk_mol, NULL, true);
      rdkit_mol_post_read_handling(rdk_mol, "from-SMILES-string");
      handled = TRUE;
   }
#endif
   return handled;
} 


int
lbg_info_t::handle_lbg_drag_and_drop_drugbank(const std::string &uri_clean,
					      const std::string &url_file_name_file) {

   int handled = FALSE;
   
   if (url_file_name_file == "image.png") {
      if (uri_clean.find_last_of("/molecules/DB") != std::string::npos) {
	 std::pair<std::string, std::string> s =
	    coot::util::split_string_on_last_slash(uri_clean);
	 std::pair<std::string, std::string> ss =
	    coot::util::split_string_on_last_slash(s.first);
	 if (ss.second.find("DB") != std::string::npos) {
	    std::string local_file = ss.second;
	    local_file += ".mol";
	    std::string drugbank_mol_url = "http://drugbank.ca/drugs/";
	    drugbank_mol_url += local_file;
	    std::string file_name =
	       coot::util::append_dir_file("coot-download", local_file);
	    get_url_func_ptr(drugbank_mol_url.c_str(), file_name.c_str());
	    import_mol_from_file(file_name);
	    handled = TRUE;
	 } 
      }
   }
   return handled;
}

int
lbg_info_t::handle_lbg_drag_and_drop_mol_file(const std::string &uri_clean,
					      const std::string &url_file_name_file) {



   int handled = FALSE;
   std::string ext = coot::util::file_name_extension(uri_clean);
   if (ext == ".mol" || ext == ".sdf") {
      std::string file_name =
	 coot::util::append_dir_file("coot-download", url_file_name_file);
      get_url_func_ptr(uri_clean.c_str(), file_name.c_str());
      import_mol_from_file(file_name);
      handled = TRUE;
   }

   return handled;

}


#endif // HAVE_GOOCANVAS
