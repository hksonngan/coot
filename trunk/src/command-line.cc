/* src/command-line.cc
 * 
 * Copyright 2001, 2002, 2003, 2004, 2005, 2006 by The University of York
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#if defined USE_PYTHON && !defined WINDOWS_MINGW
#include "Python.h"  // before system includes to stop "POSIX_C_SOURCE" redefined problems
// BL says:: need to exclude in Windows as conflicting getopt definitions.
//           do we need python here at all?!
#endif

#include "compat/coot-sysdep.h"


#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h> // for getopt(3)
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __GNU_LIBRARY__
#include "compat/coot-getopt.h"
#else
#define __GNU_LIBRARY__
#include "compat/coot-getopt.h"
#undef __GNU_LIBRARY__
#endif

#ifdef WINDOWS_MINGW
// for whatever reason (getopt) include python here for windows
#include "Python.h"  
#endif //WINDOWS_MINGW

#include <iostream>
#include <string>
#include <vector>
#include <gtk/gtk.h>

// #include "mtz-bits.h" use cmtz-interface
#include "cmtz-interface.hh"

#include "graphics-info.h"
// Including python needs to come after graphics-info.h, because
// something in Python.h (2.4 - chihiro) is redefining FF1 (in
// ssm_superpose.h) to be 0x00004000 (Grrr).
// BL says:: and (2.3 - dewinter), i.e. is a Mac - Python issue
// since the follwing two include python graphics-info.h is moved up
#include "c-interface.h"
#include "cc-interface.hh"
#include "coot-version.hh"
#include "command-line.hh"


command_line_data
parse_command_line(int argc, char ** argv ) { 

   command_line_data cld; 

#ifdef _GETOPT_H    
   int ch; 

 /* 
   * the option string will be passed to getopt(3), the format
   * of our string "sf:v:" will allow us to accept -s as a flag,
   * and -f or -v with an argument, the colon suffix tells getopt(3)
   * that we're expecting an argument.  Eg:  optest -s -f this -v8
   *
   */
 
   const char *optstr = "p:m:d:s:c:"; 

     /* 
   * getopt(3) takes our argc, and argv, it also takes
   * the option string we set up earlier.  It will assign
   * the switch character to ch, and -1 when there are no more
   * command line options to parse.
   *
   */

   static struct option long_options[] = {
      {"pdb" ,   1, 0, 0},
      {"coords", 1, 0, 0},
      {"xyzin",  1, 0, 0},
      {"map" ,   1, 0, 0},
      {"data",   1, 0, 0},
      {"hklin",  1, 0, 0},
      {"auto",   1, 0, 0},
      {"script", 1, 0, 0},
      {"ccp4-project", 1, 0, 0},
      {"dictionary", 1, 0, 0},
      {"code",       1, 0, 0},
      {"comp_id",    1, 0, 0},
      {"comp-id",    1, 0, 0},
      {"title",      1, 0, 0},
      {"port",       1, 0, 0},
      {"host",       1, 0, 0},
      {"hostname",   1, 0, 0}, // alternate for host
      {"help",       0, 0, 0},
      {"python",     0, 0, 0},
      {"splash-screen", 1, 0, 0}, // alternate splash screen
      {"self-test",     0, 0, 0},
      {"no-state-script",    0, 0, 0},
      {"no-graphics",        0, 0, 0},
      {"no-splash-screen",        0, 0, 0},
      {"stereo",     0, 0, 0},       // no arguments 
      {"side-by-side",     0, 0, 0}, // no arguments 
      {"zalman-stereo",     0, 0, 0}, // no arguments 
      {"version",    0, 0, 0},       // no arguments 
      {"version-full",  0, 0, 0},       // no arguments 
      {"no-guano",   0, 0, 0},       // no arguments
      {"small-screen", 0, 0, 0},       // no arguments (setting for EEE PC and other small screens)
      {"update-self", 0, 0, 0},  
      {0, 0, 0, 0}	       // must have blanks at end
   };

   int option_index = 0; 

   while( -1 != 
	  (ch = getopt_long(argc, argv, optstr, long_options, &option_index) )) {

      switch(ch) {
	 
	 // a long option
	 
      case 0: 
	 if (optarg) {

	    // options that need an argument:
	    
	    std::string arg_str = long_options[option_index].name;

	    if (arg_str == "pdb") { 
	       cld.coords.push_back(optarg);
	    }
	    if (arg_str == "coords") { 
	       cld.coords.push_back(optarg);
	    }
	    if (arg_str == "xyzin") {
	       cld.coords.push_back(optarg);
	    }
	    if (arg_str == "map") { 
	       cld.maps.push_back(optarg);
	    }
	    if (arg_str == "data") { 
	       cld.datasets.push_back(optarg);
	    }
	    if (arg_str == "hklin") { 
	       cld.datasets.push_back(optarg);
	    }
	    if (arg_str == "script") {
	       cld.script.push_back(optarg);
	    } 
	    if (arg_str == "port") {
	       cld.port = atoi(optarg);
	    } 
	    if (arg_str == "host") {
	       cld.hostname = optarg;
	    } 
	    if (arg_str == "hostname") {
	       cld.hostname = optarg;
	    }
	    if (arg_str == "auto") {
	       cld.auto_datasets.push_back(optarg);
	    }
	    if (arg_str == "dictionary") {
	       cld.dictionaries.push_back(optarg);
	    }
	    if (arg_str == "ccp4-project") {
	       cld.ccp4_project = optarg;
	    }
	    if (arg_str == "code") {
	       cld.accession_codes.push_back(optarg);
	    }
	    if (arg_str == "comp_id") {
	       cld.comp_ids.push_back(optarg);
	    }
	    if (arg_str == "comp-id") {
	       cld.comp_ids.push_back(optarg);
	    }
	    if (arg_str == "title") {
	       cld.title = optarg;
	    }
	    if (arg_str == "splash-screen") {
	       cld.alternate_splash_screen_file_name = optarg;
	    }
	    
	 } else { 

	    // long argument without parameter:
	    std::string arg_str(long_options[option_index].name);
	    
	    if (arg_str == "stereo") {
	       cld.hardware_stereo_flag = 1;
	    } else {
	       if (arg_str == "zalman-stereo") {
		  cld.hardware_stereo_flag = 5;
	       } else {
		  
		  // Thanks for suggesting this Ezra.
		  // 
		  if (arg_str == "help") {
		     std::cout << std::endl
			       << "Usage: coot [--pdb pdb-file-name]\n"
			       << "            [--coords pdb/cif/shelx-filename]\n"
			       << "            [--map ccp4-map-file-name]\n"
			       << "            [--data mtz-file-name]\n"
			       << "            [--hklin mtz-file-name]\n"
			       << "            [--auto mtz-file-name]\n"
			       << "            [--dictionary cif-dictionary-file-name]\n"
			       << "            [--script script-file-name]\n"
			       << "            [--title some-title]\n"
			       << "            [--small-screen]\n"
			       << "            [--splash-screen]\n"
			       << "            [--stereo]\n"
			       << "            [--zalman-stereo]\n"
			       << "            [--side-by-side]\n"
			       << "            [--version]\n"
			       << "            [--version-full]\n"
// 			       << "            [--update-self]\n"
			       << "            [--self-test]\n"
			       << "            [--no-state-script]\n"
			       << "            [--no-splash-screen]\n"
			       << "            [--no-graphics]\n"
			       << "            [--no-guano]\n"
			       << std::endl;
		     exit(0);
		  } else {
		     
		     if (arg_str == "version") {
			std::cout << VERSION << " " << coot_version_extra_info() << std::endl;
			exit(0);
		     } else {
			
			if (arg_str == "version-full") {
			   std::cout  << VERSION << " " << coot_version_extra_info() << std::endl;
			   std::cout << "Binary type: " << COOT_SYS_BUILD_TYPE << std::endl;
			   std::string s = COOT_BUILD_INFO_STRING;
// 			   if (s.length() == 0)
// 			      s = "Unknown";
// 			   std::cout << "Builder_info: " << s << std::endl;
			   if (s.length())
			      std::cout << "Builder_info: " << s << std::endl;
			   exit(0);
			   
			} else {
			   if (arg_str == "python") {
			      cld.script_is_python_flag = 1;
			   } else { 

			      if (arg_str == "no-state-script") {
				 graphics_info_t::run_state_file_status = 0;
			      } else { 

				 if (arg_str == "no-graphics") {
				    cld.do_graphics = 0;
				 } else { 
				    if (arg_str == "side-by-side") {
				       cld.hardware_stereo_flag = 2;
				    } else { 
				       if (arg_str == "no-guano") {
					  cld.disable_state_script_writing = 1;
				       } else {
					  if (arg_str == "small-screen") {
					     cld.small_screen_display = 1;
					  } else {
					     if (arg_str == "no-splash-screen") {
						cld.use_splash_screen = 0;
					     } else {
					        if (arg_str == "self-test") {
						   cld.run_internal_tests_and_exit = 1;
						} else {
						   if (arg_str == "update-self") {
						      cld.update_self = 1;
						      cld.do_graphics = 0;
						   } else {
						      std::cout << "WARNING! Malformed option - needs an argument: " 
								<< long_options[option_index].name
								<< std::endl << std::endl;
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
		  }
	       }
	    }
	 }
	 break; 
	 
	 // try short options then...
	 
      case 'p':
	 cld.coords.push_back(optarg);
	 break; 
	 
      case 's':
	 cld.script.push_back(optarg);
	 break; 
	 
      case 'd':
	 cld.datasets.push_back(optarg);
	 break; 
	 
      case 'a':
	 cld.auto_datasets.push_back(optarg);
	 break; 
	 
      case 'm':
	 cld.maps.push_back(optarg);
	 break; 
	 
      case 'c':
         if (optarg) { 
            std::cout << "command optarg: " << optarg << std::endl;
	    cld.command.push_back(optarg);
         } else { 
            std::cout << "command optarg is NULL " << std::endl;
         } 
	 break; 
	 
      case '?':
	 std::cout << "Unrecognised option: " << optopt << std::endl;
	 break;
	 
      default:
	 std::cout << "Unaccounted for optarg condition " << std::endl; 
	 break; 
      }
   }

   if (cld.hostname != "" && cld.port != 0)
      cld.try_listener = 1;

   cld.roberto_pdbs(argc, argv);
   
#endif // _GETOPT_H    

   return cld; 

} 

void
command_line_data::handle_immediate_settings() {

   // start the graphics?
   if (do_graphics == 1) 
      graphics_info_t::use_graphics_interface_flag = 1;
   else {
      graphics_info_t::use_graphics_interface_flag = 0; 
   }

   if (script_is_python_flag)
      graphics_info_t::python_at_prompt_flag = 1;

   if (update_self)
      graphics_info_t::update_self = 1;

   // small screen
   if (small_screen_display && graphics_info_t::use_graphics_interface_flag) {
     std::cout <<"INFO:: set labels and icons for small screens" <<std::endl;
     gtk_rc_parse_string("gtk-icon-sizes=\"gtk-large-toolbar=10,10:gtk-button=10,10\"");
     gtk_rc_parse_string("class \"GtkLabel\" style \"small-font\"");
     graphics_info_t::graphics_x_size = 400;
     graphics_info_t::graphics_y_size = 400;
   }
}


// add any pdb files not alread added with --pdb/coords/xyzin
// BL:: extend to mtzs (auto) and scripts (the easier ones)
// maybe the name should be change then...
void
command_line_data::roberto_pdbs(int argc, char **argv) {

   for (int i=1; i<argc; i++) {
     std::string file = argv[i];
     if (coot::util::extension_is_for_coords(coot::util::file_name_extension(file)) ||
         coot::util::extension_is_for_shelx_coords(coot::util::file_name_extension(file)))
       if (std::find(coords.begin(), coords.end(), file) == coords.end())
         coords.push_back(file);

     if (coot::util::extension_is_for_auto_datasets(coot::util::file_name_extension(file)))
       if (std::find(auto_datasets.begin(), auto_datasets.end(), file) == auto_datasets.end())
         auto_datasets.push_back(file);

     if (coot::util::extension_is_for_scripts(coot::util::file_name_extension(file)))
       if (std::find(script.begin(), script.end(), file) == script.end())
         script.push_back(file);
     
   }
}

