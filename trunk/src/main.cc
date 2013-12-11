/* src/main.cc
 * 
 * Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 by The University of York
 * Copyright 2007, 2009, 2011, 2012 by The University of Oxford
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


#if defined (USE_PYTHON)
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

/* 
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#include <sys/time.h>
#include <string.h> // strcmp

#include <iostream>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


// We are not using NLS yet.
// #ifndef WINDOWS_MINGW
// #define ENABLE_NLS
// #endif
// #ifdef DATADIR
// #endif // DATADIR

#include <gtk/gtk.h>

// Sorted out by configure.
// #define USE_LIBGLADE
// #undef USE_LIBGLADE // for now

#ifdef USE_LIBGLADE
#include <glade/glade.h>
#endif // USE_LIBGLADE

#include <GL/glut.h> // for glutInit()

// #include "lbg/lbg.hh"

#include "interface.h"
#ifndef HAVE_SUPPORT_H
#define HAVE_SUPPORT_H
#include "support.h"
#endif /* HAVE_SUPPORT_H */


#include <sys/types.h> // for stating
#include <sys/stat.h>

#ifndef _MSC_VER 
#include <unistd.h>
#else
#define PKGDATADIR "C:/coot/share"
#endif

#include "globjects.h"

#include <vector>
#include <string>

#include <mmdb/mmdb_manager.h>
#include "coords/mmdb-extras.h"
#include "coords/mmdb.h"
#include "coords/mmdb-crystal.h"

#include "clipper/core/test_core.h"
#include "clipper/contrib/test_contrib.h"

#include "coords/Cartesian.h"
#include "coords/Bond_lines.h"

#include "command-line.hh"

#include "graphics-info.h"
// Including python needs to come after graphics-info.h, because
// something in Python.h (2.4 - chihiro) is redefining FF1 (in
// ssm_superpose.h) to be 0x00004000 (Grrr).
// BL says:: and (2.3 - dewinter), i.e. is a Mac - Python issue
// since the follwing two include python graphics-info.h is moved up
//
#if defined (WINDOWS_MINGW)
#ifdef DATADIR
#undef DATADIR
#endif // DATADIR
#endif
#include "compat/sleep-fixups.h"

#include "c-interface.h"
#include "cc-interface.hh"

#include "coot-surface/rgbreps.h"

#include "coot-database.hh"

#include <glob.h>

#ifdef USE_GUILE
#include <libguile.h>
#endif 

#if defined (USE_PYTHON)
// #include "Python.h" // for Py_Initialize(); 20100813 included above now.
#define SWIG_init    init_coot
#if defined(_WIN32) || defined(__WIN32__)
#       if defined(_MSC_VER)
#               if defined(STATIC_LINKED)
#                       define SWIGEXPORT(a) a
#                       define SWIGIMPORT(a) extern a
#               else
#                       define SWIGEXPORT(a) __declspec(dllexport) a
#                       define SWIGIMPORT(a) extern a
#               endif
#       else
#               if defined(__BORLANDC__)
#                       define SWIGEXPORT(a) a _export
#                       define SWIGIMPORT(a) a _export
#               else
#                       define SWIGEXPORT(a) a
#                       define SWIGIMPORT(a) a
#               endif
#       endif
#else
#       define SWIGEXPORT(a) a
#       define SWIGIMPORT(a) a
#endif
extern "C" {
SWIGEXPORT(void) SWIG_init(void);
}
#endif

#include "c-inner-main.h"
#include "coot-glue.hh"

#include "rotate-translate-modes.hh"

void show_citation_request();
void load_gtk_resources();
void setup_splash_screen();
int setup_screen_size_settings();
void setup_application_icon(GtkWindow *window);
void setup_symm_lib();
void setup_rgb_reps();
void check_reference_structures_dir();
void create_rot_trans_menutoolbutton_menu(GtkWidget *window1);
void start_command_line_python_maybe(char **argv);
#ifdef USE_MYSQL_DATABASE
#include "mysql/mysql.h"
int setup_database();
#endif

// This happens on the command line now (20070504) -DENABLE_NLS=1
// #if (GTK_MAJOR_VERSION > 1) 
// #define ENABLE_NLS
// #endif


// This main is used for both python/guile useage and unscripted. 
int
main (int argc, char *argv[]) {
   
  GtkWidget *window1 = NULL;
  GtkWidget *glarea = NULL;

  graphics_info_t graphics_info;
  
#ifdef ENABLE_NLS // not used currently in Gtk1. Gkt2, yes.
  // 
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);
#endif

#ifdef USE_LIBCURL
  curl_global_init(CURL_GLOBAL_NOTHING); // nothing extra (e.g. ssl or WIN32)
#endif  

  // command line
  command_line_data cld = parse_command_line(argc, argv);

  cld.handle_immediate_settings();

  if (cld.run_internal_tests_and_exit) {
    // do self tests
    std::cout << "Running internal self tests" << std::endl;
    clipper::Test_core test_core;       bool result_core    = test_core();
    clipper::Test_contrib test_contrib; bool result_contrib = test_contrib();
    std::cout<<" Clipper core   : "<<(result_core   ?"OK":"FAIL")<<std::endl;
    std::cout<<" Clipper contrib: "<<(result_contrib?"OK":"FAIL")<<std::endl;
    return (result_core&&result_contrib) ? 1 : 0;
  }
  
  if (graphics_info_t::show_citation_notice == 1) { 
     show_citation_request();
  }

  
  if (graphics_info_t::use_graphics_interface_flag) {
     gtk_set_locale ();    // gtk stuff
     load_gtk_resources();
     gtk_init (&argc, &argv);
     // activate to force icons in menus; cannot get it to work with 
     // cootrc. Bug?
     //gtk_settings_set_long_property(gtk_settings_get_default (), "gtk-menu-images", TRUE, NULL);
     glutInit(&argc, argv);
  } else {
     g_type_init(); // for lbg command-line mode, so that
		    // goo_canvas_new() works cleanly.
#ifdef WINDOWS_MINGW
     // in Windows we dont want a crash dialog if no-graphics
     SetErrorMode(SetErrorMode(SEM_NOGPFAULTERRORBOX) | SEM_NOGPFAULTERRORBOX);
#endif // MINGW
  }
  
  // popup widget is only filled with graphics at the end of startup
  // which is not what we want.
  // 
  setup_splash_screen();

  
  GtkWidget *splash = NULL;

  if (graphics_info_t::use_graphics_interface_flag) {

     if (cld.use_splash_screen) {
	std::string f = cld.alternate_splash_screen_file_name;
	if (f == "") {
	   splash = create_splash_screen_window();
	} else {
	   splash = create_splash_screen_window_for_file(f.c_str());
	}
	gtk_widget_show(splash);
     }
  
     while(gtk_main_iteration() == FALSE);
     while (gtk_events_pending()) {
	usleep(3000);
	gtk_main_iteration();
     }
  }
  
  setup_symm_lib();
  setup_rgb_reps();
  check_reference_structures_dir();
#ifdef USE_MYSQL_DATABASE
  setup_database();
#endif  

  // static vector usage
  // and reading in refmac geometry restratints info:
  // 
  graphics_info.init();

  if (graphics_info_t::use_graphics_interface_flag) {

#ifdef USE_LIBGLADE
	
    /* load the interface */
    GladeXML *xml = glade_xml_new("../../coot/coot-gtk2-try2.glade", NULL, NULL);
    /* connect the signals in the interface */
    glade_xml_signal_autoconnect(xml);
    window1 = glade_xml_get_widget(xml, "window1");
    g_object_set_data_full (G_OBJECT(xml), "window1", 
	gtk_widget_ref (window1), (GDestroyNotify) gtk_widget_unref);
    std::cout << "DEBUG:: ..... window1: " << window1 << std::endl;

#else
     window1 = create_window1 ();
#endif // USE_LIBGLADE
     
     std::string version_string = VERSION;
     std::string main_title = "Coot " + version_string;
     // if this is a pre-release, stick in the revision number too
     if (version_string.find("-pre") != std::string::npos) {
	main_title += " (revision ";
	main_title += coot::util::int_to_string(svn_revision());
	main_title += ")";
     }
     
#ifdef WINDOWS_MINGW
     main_title = "Win" + main_title;
#endif

     gtk_window_set_title(GTK_WINDOW (window1), main_title.c_str());

     glarea = gl_extras(lookup_widget(window1, "vbox1"),
			cld.hardware_stereo_flag);
     if (glarea) {
	// application icon:
	setup_application_icon(GTK_WINDOW(window1));
	// adjust screen size settings
	int small_screen = setup_screen_size_settings();
	if (!cld.small_screen_display) {
	   cld.small_screen_display = small_screen;
	}

	graphics_info.glarea = glarea; // save it in the static
	
	gtk_widget_show(glarea);
	if (graphics_info_t::glarea_2) 
	   gtk_widget_show(graphics_info_t::glarea_2);
	// and setup (store) the status bar
	GtkWidget *sb = lookup_widget(window1, "main_window_statusbar");
	graphics_info_t::statusbar = sb;
	graphics_info_t::statusbar_context_id =
	   gtk_statusbar_get_context_id(GTK_STATUSBAR(sb), "picked atom info");
	
	gtk_widget_show (window1);
	create_rot_trans_menutoolbutton_menu(window1);
	
	// We need to somehow connect the submenu to the menu's (which are
	// accessible via window1)
	// 
	create_initial_map_color_submenu(window1);
	create_initial_map_scroll_wheel_submenu(window1);
	create_initial_ramachandran_mol_submenu(window1);
	create_initial_sequence_view_mol_submenu(window1);
	
	// old style non-generic functions     
	//      create_initial_validation_graph_b_factor_submenu(window1);
	//      create_initial_validation_graph_geometry_submenu(window1);
	//      create_initial_validation_graph_omega_submenu(window1);

	// OK, these things work thusly:
	//
	// probe_clashes1 is the name of the menu_item set/created in
	// by glade and is in mapview.glade.
	// 
	// probe_submenu is something I make up. It must be the same
	// here and in c-interface-validate.cc's
	// add_on_validation_graph_mol_options()
	//
	// attach a function to the menu item activate function
	// created by glade in callbacks.c
	// (e.g. on_probe_clashes1_activate).  The name that is used
	// there to look up the menu is as above (e.g. probe_clashes1).
	//
	// The type defined there is that checked in
	// c-interface-validate.cc's
	// add_on_validation_graph_mol_options() 
	
	
	create_initial_validation_graph_submenu_generic(window1 , "peptide_omega_analysis1", "omega_submenu");
	create_initial_validation_graph_submenu_generic(window1 , "geometry_analysis1", "geometry_submenu");
	create_initial_validation_graph_submenu_generic(window1 , "temp_fact_variance_analysis1",
							"temp_factor_variance_submenu");
	create_initial_validation_graph_submenu_generic(window1 , "rotamer_analysis1", "rotamer_submenu");
	create_initial_validation_graph_submenu_generic(window1 , "density_fit_analysis1", "density_fit_submenu");
	create_initial_validation_graph_submenu_generic(window1 , "probe_clashes1", "probe_submenu");
	create_initial_validation_graph_submenu_generic(window1 , "gln_and_asn_b_factor_outliers1",
							"gln_and_asn_b_factor_outliers_submenu");
	create_initial_validation_graph_submenu_generic(window1 , "ncs_differences1", "ncs_diffs_submenu");

     } else {
	std::cout << "CATASTROPHIC ERROR:: failed to create Gtk GL widget"
		  << "  (Check that your X11 server is working and has (at least)"
		  << "  \"Thousands of Colors\" and supports GLX.)" << std::endl;
     }
  }

  
  // allocate some memory for the molecules
  //
  std::cout << "initalize graphics molecules...";
  std::cout.flush();
  initialize_graphics_molecules();
  std::cout << "done." << std::endl;
	
#if !defined(USE_GUILE) && !defined(USE_PYTHON)
  handle_command_line_data(cld);  // and add a flag if listener
	   		          // should be started.
#endif
  
     // which gets looked at later in c_inner_main's make_port_listener_maybe()

     // For the graphics to be able to see the data set in the .coot
     // file, we must put the gl_extras() call after
     // c_wrapper_scm_boot_guile().  So all the graphics
     // e.g. create_window1, gl_extras, and the submenu stuff should go
     // into the c_inner_main (I think)
     //

  if (splash) 
     gtk_widget_destroy(splash);

     // This should not be here, I think.  You can never turn it off -
     // because scripting/guile is not booted until after here - which
     // means its controlling parameter
     // (graphics_info_t::run_state_file_status) can never be changed
     // from the default.  It should be in c-inner-main, after the
     // run_command_line_scripts() call.  
     // 
     // run_state_file_maybe();

  // before we run the scripting, let's make default preferences
  make_preferences_internal_default();

  //
  add_ligand_builder_menu_item_maybe();

#ifdef USE_PYTHON
     //  (on Mac OS, call PyMac_Initialize() instead)
     //http://www.python.org/doc/current/ext/embedding.html

#ifdef USE_PYMAC_INIT 
  PyMac_Initialize();
#else  
  Py_Initialize(); // otherwise it core dumps saying python
  // interpreter not initialized (or something).
  PySys_SetArgv(argc, argv);
#endif     

  // Now we want to import coot.py/_coot.so (coot load _coot)
  // std::cout << "::::::::::::::::: Python path init_coot" << std::endl;
  init_coot(); // i.e. SWIG_init for python, best we do this before
               // running .coot.py, eh?


     /* And now read the users own initialization code */

#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
     char *directory = getenv("COOT_HOME");
     std::string pkgdirectory = PKGDATADIR;
     if (!directory) {
       directory = getenv("HOME");
     }
     if (!directory) {
       directory = (char *)pkgdirectory.c_str();
     }
#else      
     char *directory = getenv("HOME");
#endif

// BL says: we do some more insert to accommodate use_graphics_flag and tips-gui!
     short int use_graphics_flag = use_graphics_interface_state();

     // First load coot.py, then load the standard startup files, 
     // then load 0-coot.state.py
     
     std::string pydirectory = PKGDATADIR;
     pydirectory += "/python";
     char *pydirectory_cs = getenv("COOT_PYTHON_DIR");
     
     if (pydirectory_cs) { 
	pydirectory = pydirectory_cs;
     }

     std::cout << "DEBUG:: stating pydirectory " << pydirectory << std::endl;
     
     struct stat buf;
     int status = stat(pydirectory.c_str(), &buf);
     if (status != 0) { 
       std::cout << "WARNING python directory " << pydirectory 
		 << " does not exist" << std::endl;;
     } else { 
       const char *coot_dot_py = "coot.py";
       const char *coot_dot_py_checked = does_file_exist(pydirectory.c_str(), coot_dot_py);
       if (coot_dot_py_checked) {
	 std::cout << "INFO:: importing coot.py from " << coot_dot_py_checked
		   << std::endl;
	 //run_python_script(coot_dot_py_checked);
	 // not a const argument?  Dear oh dear....
	 int err = import_python_module("coot", 0);
	 //PyRun_SimpleString((char *)simple.c_str());
	 if (err == -1) {
	   std::cout << "ERROR:: could not import coot.py" << std::endl;
	 } else {

	   std::cout << "INFO:: coot.py imported" << std::endl;

	   const char *coot_load_modules_dot_py;
	   if (use_graphics_flag) {
	     // we have gui
	     // BL says:: lets initialize glue too but only if we have pygtk 
	     // (and gtk2)
#ifdef USE_PYGTK
#ifdef COOT_USE_GTK2_INTERFACE
	      initcoot_python();
	      std::cout << "INFO:: coot_python initialized" << std::endl;
#ifdef USE_GUILE_GTK
	      safe_python_command("global use_gui_qm; use_gui_qm = 2");
#else
	      safe_python_command("global use_gui_qm; use_gui_qm = 1");
#endif
#else
	      safe_python_command("global use_gui_qm; use_gui_qm = False");
#endif // GKT2
#else
	      safe_python_command("global use_gui_qm; use_gui_qm = False");
#endif // PYTGK
	   } else {
	      // we dont have gui
	      safe_python_command("global use_gui_qm; use_gui_qm = False");
	   }
	   coot_load_modules_dot_py = "coot_load_modules.py";
	   char *coot_load_modules_dot_py_checked = 
	     does_file_exist(pydirectory.c_str(), coot_load_modules_dot_py);
	   if (coot_load_modules_dot_py_checked) { 
	     std::cout << "INFO loading coot python modules" << std::endl;
	     run_python_script(coot_load_modules_dot_py_checked);
	   } else {
	     std::cout << "WARNING:: No coot modules found! Python scripting crippled. " 
		       << std::endl;
	   }
	 }
       } else { 
	 std::cout << "WARNING:: No coot.py file found! Python scripting unavailable. " 
		   << std::endl;
       }
     }
     
     // try to load extra dir files (if exist) do before preferences (as for
     // scheme version
     try_load_python_extras_dir();

     // load preferences file .coot_preferences.py
     // only GTK2
#if COOT_USE_GTK2_INTERFACE
     std::string preferences_dir = graphics_info_t::add_dir_file(directory, ".coot-preferences");
     struct stat buff;
     int preferences_dir_status = stat(preferences_dir.c_str(), &buff);
     if (preferences_dir_status != 0) { 
		std::cout << "INFO:: preferences directory " << preferences_dir 
                  << " does not exist. Won't read preferences." << std::endl;;
     } else {
       // load all .py files
       glob_t myglob;
       int flags = 0;
       //std::string glob_patt = "/*.py";
       std::string glob_file = preferences_dir;
       glob_file += "/*.py";
       glob(glob_file.c_str(), flags, 0, &myglob);
       size_t count;
       
       char **p;
       for (p = myglob.gl_pathv, count = myglob.gl_pathc; count; p++, count--) { 
         char *preferences_file(*p);
	 // dont load the coot_toolbuttons.py if no graphics
	 char *found = strstr(preferences_file, "coot_toolbuttons.py");
	 if ((!found) || (use_graphics_flag)) {
	   std::cout << "INFO:: loading preferences file " << preferences_file
	 	     << std::endl;
	   run_python_script(preferences_file);
	 }
       }
       globfree(&myglob);

       //char *preferences_filename = ".coot_preferences.py";
       //if (directory) {
       //char *check_preferences_file = does_file_exist(directory, preferences_filename);
       //if (check_preferences_file) {
       //   std::cout << "Loading Preferences ~/.coot_preferences.py..." << std::endl;
       //   run_python_script(check_preferences_file);
     }
     // update the preferences
     make_preferences_internal();
#endif // GTK2

     // load personal coot file .coot.py
     const char *filename = ".coot.py";
     if (directory) { 
        char *check_file = does_file_exist(directory, filename);
        if (check_file) {
	   std::cout << "Loading ~/.coot.py..." << std::endl;
	   run_python_script(check_file);
        }
     }

     // we only want to run one state file if using both scripting
     // languages.  Let that be the guile one.
#ifndef USE_GUILE     
     handle_command_line_data(cld);

     run_state_file_maybe(); // run local 0-coot.state.py?

     run_update_self_maybe();
#endif // USE_GUILE - not both start-up scripts

#if defined USE_PYGTK && !defined USE_GUILE_GTK
     // BL says: we wanna run tips-gui now, but only if we dont use guile_gtk
     // otherwise we'll get 2
     if (use_graphics_flag) {
       PyRun_SimpleString("tips_gui()");
     }
#endif // USE_PYGTK
     
#endif // USE_PYTHON  

     
#ifdef USE_GUILE
     
     // Must be the last thing in this function, code after it does not get 
     // executed (if we are using guile)
     //
     command_line_data *pcld = &cld;
     c_wrapper_scm_boot_guile(argc, argv, pcld); 

     //  
#endif

     // to start the graphics, we need to init glut and gtk with the
     // command line args.

  // Finally desensitize the missing scripting menu
  if (graphics_info_t::use_graphics_interface_flag) {
    GtkWidget *w;
#ifndef USE_GUILE
    w = lookup_widget(window1, "scripting_scheme1");
    gtk_widget_set_sensitive(w, False);
#endif
#ifndef USE_PYTHON
    w = lookup_widget(window1, "scripting_python1");
    gtk_widget_set_sensitive(w, False);
#endif
  }

#if ! defined (USE_GUILE)
#ifdef USE_PYTHON
     run_command_line_scripts();
     if (graphics_info_t::use_graphics_interface_flag)
	gtk_main ();
     else 
	start_command_line_python_maybe(argv);

#else
     // not python or guile
     if (graphics_info_t::use_graphics_interface_flag)
	gtk_main();
#endif // USE_PYTHON
     
#endif // ! USE_GUILE

     return 0;
}


void load_gtk_resources() {

   std::string gtkrcfile = PKGDATADIR;
   gtkrcfile += "/cootrc";

   // over-ridden by user?
   char *s = getenv("COOT_RESOURCES_FILE");
   if (s) {
      gtkrcfile = s;
   }
  
  std::cout << "Acquiring application resources from " << gtkrcfile << std::endl;
  gtk_rc_add_default_file(gtkrcfile.c_str());

}


/*  ----------------------------------------------------------------------- */
/*            Amusing (possibly) little splash screen                       */
/*  ----------------------------------------------------------------------- */
void
setup_splash_screen() { 

   // default location:
   std::string splash_screen_pixmap_dir = PKGDATADIR;  
   splash_screen_pixmap_dir += "/";
   splash_screen_pixmap_dir += "pixmaps";

   // over-ridden by user?
   char *s = getenv("COOT_PIXMAPS_DIR");
   if (s) {
      splash_screen_pixmap_dir = s;
   }
   
   std::cout << "INFO:: splash_screen_pixmap_dir " 
	     << splash_screen_pixmap_dir << std::endl;
   // now add splash_screen_pixmap_dir to the pixmaps_directories CList
   //
   add_pixmap_directory(splash_screen_pixmap_dir.c_str());

}

// this returns the effective screen height if possible otherwise an estimate
int
get_max_effective_screen_height() {

    // using properties
    gboolean ok;
    guchar* raw_data = NULL;
    gint data_len = 0;
    gint width;
    gint height;
    int max_height;
    max_height = -1;
#if (GTK_MAJOR_VERSION >1)
// no gdk_property get on windows (at the moment)
#if !defined WINDOWS_MINGW && !defined _MSC_VER 
    ok = gdk_property_get(gdk_get_default_root_window(),  // a gdk window
                          gdk_atom_intern("_NET_WORKAREA", FALSE),  // property
                          gdk_atom_intern("CARDINAL", FALSE),  // property type
                          0,  // byte offset into property
                          0xff,  // property length to retrieve
                          false,  // delete property after retrieval?
                          NULL,  // returned property type
                          NULL,  // returned data format
                          &data_len,  // returned data len
                          &raw_data);  // returned data
     
    if (ok) {
        
        // We expect to get four longs back: x, y, width, height.
        if (data_len >= static_cast<gint>(4 * sizeof(glong))) {
            glong* data = reinterpret_cast<glong*>(raw_data);
            gint x = data[0];
            gint y = data[1];
            width = data[2];
            height = data[3];
            max_height = height;
        }
        g_free(raw_data);
    } 
#endif // MINGW
    if (max_height < 0) {
        GdkScreen *screen;
        screen = gdk_screen_get_default();
        if (screen) {
            width = gdk_screen_get_width(screen);
            height = gdk_screen_get_height(screen);

#ifdef WINDOWS_MINGW
            max_height = int(height * 0.95);
#else
            max_height = int(height * 0.9);
#endif // MINGW
        } else {
            g_print ("BL ERROR:: couldnt get gdk screen; should never happen\n");
        }
    }
#endif //GTK
    return max_height;
}

int
setup_screen_size_settings() {

   int ret = 0;
   int max_height;
   max_height = get_max_effective_screen_height();

   // adjust the icons size of the refinement toolbar icons
   if (max_height <= 620) {
     std::cout << "BL INFO:: screen has " << max_height << " height, will make small icons and small font" << std::endl;
       max_height = 620;
       gtk_rc_parse_string("gtk-icon-sizes=\"gtk-large-toolbar=10,10:gtk-button=10,10\"");
       gtk_rc_parse_string("class \"GtkLabel\" style \"small-font\"");
       ret = 1;
   } else if (max_height <= 720) {
       int icon_size = 12 + (max_height - 620) / 25;
       std::cout << "BL INFO:: screen has " << max_height << " height, will make icons to " <<  icon_size <<std::endl;
       std::string toolbar_txt = "gtk-icon-sizes = \"gtk-large-toolbar=";
       toolbar_txt += coot::util::int_to_string(icon_size);
       toolbar_txt += ",";
       toolbar_txt += coot::util::int_to_string(icon_size);
       toolbar_txt += ":gtk-button=";
       toolbar_txt += coot::util::int_to_string(icon_size);
       toolbar_txt += ",";
       toolbar_txt += coot::util::int_to_string(icon_size);
       toolbar_txt += "\"";
       gtk_rc_parse_string (toolbar_txt.c_str());
   }
   return ret;
}


void setup_application_icon(GtkWindow *window) { 
      
   std::string splash_screen_pixmap_dir = PKGDATADIR;  
   splash_screen_pixmap_dir += "/";
   splash_screen_pixmap_dir += "pixmaps";

   // over-ridden by user?
   char *s = getenv("COOT_PIXMAPS_DIR");
   if (s) {
      splash_screen_pixmap_dir = s;
   }

   // now add the application icon
   std::string app_icon_path =
      coot::util::append_dir_file(splash_screen_pixmap_dir,
				  "coot-icon.png");

   struct stat buf;
   int status = stat(app_icon_path.c_str(), &buf); 
   if (status == 0) { // icon file was found

#if (GTK_MAJOR_VERSION == 2)
      GdkPixbuf *icon_pixbuf =
		 gdk_pixbuf_new_from_file (app_icon_path.c_str(), NULL);
      if (icon_pixbuf) {
		 gtk_window_set_icon (GTK_WINDOW (window), icon_pixbuf);
		 gdk_pixbuf_unref (icon_pixbuf);
      }


#else
      // gtk 1, use:
      // 
      // void gdk_window_set_icon(GdkWindow	  *window, 
      // 				GdkWindow	  *icon_window,
      // 				GdkPixmap	  *pixmap,
      // 				GdkBitmap	  *mask);

      // Argh. How do I make a bitmap from a file?  This is just too
      // hard.  Give up.
      // GdkBitmap *icon = ...(app_icon_path.c_str());
      // gtk_window_set_icon(window, window, icon, icon);


//       /* load a pixmap from a file */
//       GdkBitmap *mask;
//       GtkStyle *style = gtk_widget_get_style( window->default_widget );

//       GdkPixmap *pixmap = gdk_pixmap_create_from_xpm( window, &mask,
// 						      &style->bg[GTK_STATE_NORMAL],
// 						      app_icon_path.c_str());
//       GtkWidget *pixmapwid = gtk_pixmap_new( pixmap, mask );
//       gtk_widget_show( pixmapwid );
//       gtk_container_add( GTK_CONTAINER(window), pixmapwid );

      
#endif
   }
#if (GTK_MAJOR_VERSION >= 2)

   // load svg/png files to antialias icons
   // maybe should go somewhere else?!
   GtkIconSet* iconset;
   GtkIconFactory *iconfactory = gtk_icon_factory_new();
   GSList* stock_ids = gtk_stock_list_ids();
   GError *error = NULL;
   const char *stock_id;
   GdkPixbuf* pixbuf;

   glob_t myglob;
   int flags = 0;
   std::string glob_dir = splash_screen_pixmap_dir;
   std::string glob_file = glob_dir;
   glob_file += "/*.svg";
   glob(glob_file.c_str(), flags, 0, &myglob);
   glob_file = glob_dir;
   glob_file += "/*.png";
   glob(glob_file.c_str(), GLOB_APPEND, 0, &myglob);
   size_t count;
   char **p;
   for (p = myglob.gl_pathv, count = myglob.gl_pathc; count; p++, count--) {
      char *filename(*p);
      pixbuf = gdk_pixbuf_new_from_file(filename, &error);
      if (error) {
	 g_print ("Error loading icon: %s\n", error->message);
	 g_error_free (error);
	 error = NULL;
      } else {
	 if (pixbuf) {
	    iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
	    g_object_unref(pixbuf);
	    // may have to be adjusted for Windows!!
	    //stock_id = (coot::util::file_name_non_directory(filename)).c_str();
	    std::string tmp = coot::util::file_name_non_directory(filename);
	    stock_id = tmp.c_str();
	    if (strcmp(stock_id, "") !=0) { // if they don't match..
	       gtk_icon_factory_add(iconfactory, stock_id, iconset);
	       gtk_icon_factory_add_default(iconfactory);
	    }
	 }
      }
   }
   globfree(&myglob);

	 
#endif // GTK

}

void add_ligand_builder_menu_item_maybe() {

   
   if (graphics_info_t::use_graphics_interface_flag) {

      GtkWidget *w;
      GtkWidget *p = main_window();
      w = lookup_widget(p, "ligand_builder1");
      if (! w) {
	 std::cout << "oops failed to look up ligand_builder menu item" << std::endl;
      } else { 
#ifdef HAVE_GOOCANVAS
	 // all is hunky dory, it's OK to see the menu item
#else 	 
	 gtk_widget_set_sensitive(w, False);  // or hide!?
#endif // HAVE_GOOCANVAS
      }
   }

}

void 
show_citation_request() { 

   std::cout << "\n   If you have found this software to be useful, you are requested to cite:\n"
	     << "   Coot: model-building tools for molecular graphics" << std::endl;
   std::cout << "   Emsley P, Cowtan K" << std::endl;
   std::cout << "   ACTA CRYSTALLOGRAPHICA SECTION D-BIOLOGICAL CRYSTALLOGRAPHY\n";
   std::cout << "   60: 2126-2132 Part 12 Sp. Iss. 1 DEC 2004\n\n";

   std::cout << "   The reference for the REFMAC5 Dictionary is:\n";
   std::cout << "   REFMAC5 dictionary: organization of prior chemical knowledge and\n"
	     << "   guidelines for its use" << std::endl;
   std::cout << "   Vagin AA, Steiner RA, Lebedev AA, Potterton L, McNicholas S,\n"
	     << "   Long F, Murshudov GN" << std::endl;
   std::cout << "   ACTA CRYSTALLOGRAPHICA SECTION D-BIOLOGICAL CRYSTALLOGRAPHY " << std::endl;
   std::cout << "   60: 2184-2195 Part 12 Sp. Iss. 1 DEC 2004" << std::endl;

#ifdef HAVE_SSMLIB
    std::cout << "\n   If using \"SSM Superposition\", please cite:\n";

    std::cout << "   Secondary-structure matching (SSM), a new tool for fast\n"
	      << "   protein structure alignment in three dimensions" << std::endl;
    std::cout << "   Krissinel E, Henrick K" << std::endl;
    std::cout << "   ACTA CRYSTALLOGRAPHICA SECTION D-BIOLOGICAL CRYSTALLOGRAPHY" << std::endl;
    std::cout << "   60: 2256-2268 Part 12 Sp. Iss. 1 DEC 2004\n" << std::endl;
#endif // HAVE_SSMLIB
    
}


void setup_symm_lib() {

   // How should the setting up of symmetry work?
   //
   // First we check the environment variable SYMINFO.
   // 
   // If that is not set, then we look in the standard (hardwired at
   // compile time) place
   //
   // If both these fail then give an error message. 

   char *syminfo = getenv("SYMINFO");
   if (!syminfo) { 

      std::string symstring("SYMINFO=");

      // using PKGDATADIR will work for those who compiler, not the
      // binary users:
      std::string standard_file_name = PKGDATADIR; // xxx/share/coot
      standard_file_name += "/"; 
      standard_file_name += "syminfo.lib";

      struct stat buf;
      int status = stat(standard_file_name.c_str(), &buf); 
      if (status != 0) { // standard-residues file was not found in default location 

	 // This warning is only sensible for those who compile (or
	 // fink).  So let's test if SYMINFO was set before we write it
	 //
	 std::cout << "WARNING:: Symmetry library not found at "
		   << standard_file_name
		   << " and environment variable SYMINFO is not set." << std::endl;
	 std::cout << "WARNING:: Symmetry will not be possible\n";
	 
      } else { 

	 symstring += standard_file_name;

	 // Mind bogglingly enough, the string is part of the environment
	 // and malleable, so const char * of a local variable is not
	 // what we want at all.  
	 // 
	 //  We fire and forget, we don't want to change s.
	 // 
	 char * s = new char[symstring.length() + 1];
	 strcpy(s, symstring.c_str());
	 putenv(s);
	 // std::cout << "DEBUG:: SYMINFO set/not set? s is " << s <<std::endl;
      }
   }
}

void setup_rgb_reps() {

   // c.f. coot::package_data_dir() which now does this wrapping for us:
   
   // For binary installers, they use the environment variable:
   char *env = getenv("COOT_DATA_DIR");

   // Fall-back:
   std::string standard_file_name = PKGDATADIR; // xxx/share/coot

   if (env)
      standard_file_name = env;

   std::string colours_file = standard_file_name + "/";
   std::string colours_def = "colours.def";
   colours_file += colours_def;

   struct stat buf;
   int status = stat(colours_file.c_str(), &buf); 
   if (status == 0) { // colours file was found in default location
      RGBReps r(colours_file);
      std::cout << "INFO:: Colours file: " << colours_file << " loaded"
		<< std::endl;

      // test:
//       std::vector<std::string> test_col;
//       test_col.push_back("blue");
//       test_col.push_back("orange");
//       test_col.push_back("magenta");
//       test_col.push_back("cyan");

//       std::vector<int> iv = r.GetColourNumbers(test_col);
//       for (int i=0; i<iv.size(); i++) {
// 	 std::cout << "Colour number: " << i << "  " << iv[i] << std::endl;
//       }
      
      
   } else {
      std::cout << "WARNING! Can't find file: colours.def at " << standard_file_name
		<< std::endl;
   }
}


void check_reference_structures_dir() {

   char *coot_reference_structures = getenv("COOT_REF_STRUCTS");
   if (coot_reference_structures) {
      struct stat buf;
      int status = stat(coot_reference_structures, &buf); 
      if (status != 0) { // file was not found in default location either
	 std::cout << "WARNING:: The reference structures directory "
		   << "(COOT_REF_STRUCTS): "
		   << coot_reference_structures << " was not found." << std::endl;
	 std::cout << "          Ca->Mainchain will not be possible." << std::endl;
      }
   } else {

      // check in the default place: pkgdatadir = $prefix/share/coot
      std::string pkgdatadir = PKGDATADIR;
      std::string ref_structs_dir = pkgdatadir;
      ref_structs_dir += "/";
      ref_structs_dir += "reference-structures";
      struct stat buf;
      int status = stat(ref_structs_dir.c_str(), &buf); 
      if (status != 0) { // file was not found in default location either
	 std::cout << "WARNING:: No reference-structures found (in default location)."
		   << "          and COOT_REF_STRUCTS was not defined." << std::endl;
	 std::cout << "          Ca->Mainchain will not be possible." << std::endl;
      }
   }

}

void
menutoolbutton_rot_trans_activated(GtkWidget *item, GtkPositionType pos) {

   // std::cout << "changing to zone type" << pos << std::endl;
   set_rot_trans_object_type(pos);
   do_rot_trans_setup(1);
   if (graphics_info_t::model_fit_refine_dialog) {
     update_model_fit_refine_dialog_menu(graphics_info_t::model_fit_refine_dialog);
   }
}

void create_rot_trans_menutoolbutton_menu(GtkWidget *window1) {

#if (GTK_MAJOR_VERSION > 1)
// RHEL 4 (gtk 2.4.13) does't have menutoolbuttons (not sure at which
// minor version it was introduced).  FC4 (GTK 2.6.7 *does* have it).
#if ( ( (GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION > 4) ) || GTK_MAJOR_VERSION > 2)
   GtkWidget *menu_tool_button = lookup_widget(window1, "model_toolbar_rot_trans_toolbutton");

   if (menu_tool_button) { 
      GtkWidget *menu = gtk_menu_new();
      GtkWidget *menu_item;
      GSList *group = NULL;

      menu_item = gtk_radio_menu_item_new_with_label(group, "By Residue Range...");
      group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 GTK_SIGNAL_FUNC(menutoolbutton_rot_trans_activated),
			 GINT_TO_POINTER(ROT_TRANS_TYPE_ZONE));
      /* activate the first item */
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), TRUE);

      menu_item = gtk_radio_menu_item_new_with_label(group, "By Chain...");
      group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 GTK_SIGNAL_FUNC(menutoolbutton_rot_trans_activated),
			 GINT_TO_POINTER(ROT_TRANS_TYPE_CHAIN));

      menu_item = gtk_radio_menu_item_new_with_label(group, "By Molecule...");
      group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menu_item));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 GTK_SIGNAL_FUNC(menutoolbutton_rot_trans_activated),
			 GINT_TO_POINTER(ROT_TRANS_TYPE_MOLECULE));
      
      gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(menu_tool_button), menu);
   }
#endif  // GTK_MINOR_VERSION
#endif  // GTK_MAJOR_VERSION
} 

