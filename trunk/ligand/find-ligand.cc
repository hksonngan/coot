/* ligand/find-ligands.cc
 * 
 * Copyright 2004, 2005, 2006, 2007 The University of York
 * Author: Paul Emsley
 * Copyright 2007 The University of Oxford
 * Author: Paul Emsley
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

// Portability (getopt) gubbins
#ifndef _MSC_VER
#include <unistd.h> // for getopt(3)
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef __GNU_LIBRARY__
#include "coot-getopt.h"
#else
#define __GNU_LIBRARY__
#include "coot-getopt.h"
#undef __GNU_LIBRARY__
#endif

#include <iostream>
#include <string>
#include <stdexcept>

#include "wligand.hh"

int
main(int argc, char **argv) {

   if (argc < 6) { 
      std::cout << "Usage: " << argv[0] << "\n     "
		<< " --pdbin pdb-in-filename" << " --hklin mtz-filename" << "\n     "
		<< " --f f_col_label" << " --phi phi_col_label" << "\n     "
		<< " --clusters nclust"
		<< " --sigma sigma-level" << "\n     "
		<< " --fit-fraction frac" << "\n     "
		<< " --flexible"
		<< " --samples nsamples"
		<< " --dictionary cif-dictionary-name" << "\n     "
		<< " --script script-file-name\n"
		<< "   ligand-pdb-file-name(s)\n";
      std::cout << "     where pdbin is the protein (typically)\n"
		<< "           nclust is the number of clusters to fit [default 10]\n"
		<< "           sigma is the search level of the map (default 2.0)\n"
		<< "           --flexible means use torsional conformation ligand search\n"
		<< "           --dictionary file containing the CIF ligand dictionary description\n"
		<< "           nsamples is the number of flexible conformation samples [default 30]\n"
		<< "           frac is the minimum fraction of atoms in density allowed after fit [default 0.75]"
		<< "           script-file-name is a file name of helper script suitable for use in Coot"
		<< "               (default: coot-ligands.scm)."
		<< std::endl;


   } else { 

      int n_used_args = 1;
      std::string pdb_file_name;
      std::string  mtz_filename;
      std::string         f_col;
      std::string       phi_col;
      std::string     sigma_str;
      std::string     n_cluster_string;
      std::vector<std::string> lig_files;
      short int use_wiggly_ligand = 0;
      int wiggly_ligand_n_samples = 30;
      std::string cif_file_name = "";
      std::string fit_frac_str = "";
      std::string coot_ligands_script_file_name = "coot-ligands.scm";
      float fit_frac = -1.0; // tested for positivity

      const char *optstr = "i:h:f:p:s:c:w:n:d";
      struct option long_options[] = {
	 {"pdbin",      1, 0, 0},
	 {"hklin",      1, 0, 0},
	 {"f",          1, 0, 0},
	 {"phi",        1, 0, 0},
	 {"sigma",      1, 0, 0},
	 {"clusters",   1, 0, 0},
	 {"samples",    1, 0, 0},
	 {"dictionary", 1, 0, 0},
	 {"fit-fraction", 1, 0, 0},
	 {"flexible",   0, 0, 0},
	 {"script",   0, 0, 0},
	 {0, 0, 0, 0}
      };

      int ch;
      int option_index = 0;
      while ( -1 != 
	      (ch = getopt_long(argc, argv, optstr, long_options, &option_index))) { 

	 switch(ch) {
	    
	 case 0:
	    if (optarg) { 

// 	       std::cout << "DEBUG:: " << option_index << " " << strlen(optarg) << std::endl;
// 	       std::cout << " " << optarg << std::endl;
// 	       std::cout << "   ch:: " << ch << std::endl;
	 
	       std::string arg_str = long_options[option_index].name;

	       if (arg_str == "pdbin") { 
		  pdb_file_name = optarg;
		  n_used_args += 2;
	       } 
	       if (arg_str == "pdb") { 
		  pdb_file_name = optarg;
		  n_used_args += 2;
	       } 
	       if (arg_str == "hklin") { 
		  mtz_filename = optarg;
		  n_used_args += 2;
	       } 
	       if (arg_str == "f") { 
		  f_col = optarg;
		  n_used_args += 2;
	       } 
	       if (arg_str == "phi") {
		  phi_col = optarg;
		  n_used_args += 2;
	       } 
	       if (arg_str == "sigma") {
		  sigma_str = optarg;
		  n_used_args += 2;
	       } 
	       if (arg_str == "clusters") {
		  n_cluster_string = optarg;
		  n_used_args += 2;
	       }

	       if (arg_str == "samples") { 
		  wiggly_ligand_n_samples = atoi(optarg);
		  n_used_args += 2;
	       }

	       if (arg_str == "dictionary") { 
		  cif_file_name = optarg;
		  n_used_args += 2;
	       }

	       if (arg_str == "fit-fraction") { 
		  fit_frac_str = optarg;
		  n_used_args += 2;
	       }
	       
	       if (arg_str == "script") { 
		  coot_ligands_script_file_name = optarg;
		  n_used_args += 2;
	       }
	       
	    } else {

	       // options without arguments:
	       
	       // long argument without parameter:
	       std::string arg_str(long_options[option_index].name);
	       
	       if (arg_str == "flexible") {
		  use_wiggly_ligand = 1;
		  n_used_args += 1;
	       }
	       
	    }
	    break;

	 case 'i':
	    pdb_file_name = optarg;
	    n_used_args += 2;
	    break;
	    
	 case 'h':
	    mtz_filename = optarg;
	    n_used_args += 2;
	    break;
	    
	 case 'f':
	    f_col = optarg;
	    n_used_args += 2;
	    break;
	    
	 case 'p':
	    phi_col = optarg;
	    n_used_args += 2;
	    break;
	    
	 case 's':
	    sigma_str = optarg;
	    n_used_args += 2;
	    break;
	    
	 case 'c':
	    n_cluster_string = optarg;
	    n_used_args += 2;
	    break;
	    
	 default:
	    std::cout << "default optarg: " << optarg << std::endl;
	    break;
	 }
      }

      short int do_it = 0;
      if (pdb_file_name.length() == 0) { 
	 std::cout << "Missing input PDB file\n";
	 exit(1);
      } else { 
	 if (mtz_filename.length() == 0) { 
	    std::cout << "Missing MTZ file\n";
	    exit(1);
	 } else { 
	    if (f_col.length() == 0) { 
	       std::cout << "Missing F column name\n";
	       exit(1);
	    } else {
	       if (phi_col.length() == 0) { 
		  std::cout << "Missing PHI column name\n";
		  exit(1);
	       } else { 
		  if (sigma_str.length() == 0) {
		     sigma_str = "2.0";
		  }
		  if (n_cluster_string.length() == 0) { 
		     n_cluster_string = "10";
		  }
// 		  std::cout << "argc: " << argc << " n_used_args: " 
// 			    << n_used_args << std::endl;
		  for (int i=n_used_args; i<argc; i++) 
		     lig_files.push_back(argv[i]);
		  do_it = 1;
	       }
	    }
	 }
      }
      
      if (do_it) { 

	 if (lig_files.size() == 0) { 
	    std::cout << "No ligand pdb files specified\n";
	    exit(1);
	 } else {

	    if (fit_frac_str != "") { // it was set
	       fit_frac = atof(fit_frac_str.c_str());
	    } else {
	       fit_frac = 0.75;
	    } 
	    std::cout << "INFO:: Using acceptable fit fraction of: " << fit_frac << std::endl;
	    float input_sigma_level = atof(sigma_str.c_str());
	    int n_cluster = atoi(n_cluster_string.c_str());
	    short int use_weights = 0;
	    short int is_diff_map = 0; 

	    if (! use_wiggly_ligand) { 
	       coot::ligand lig;
	       lig.set_verbose_reporting();
	       short int map_stat = lig.map_fill_from_mtz(mtz_filename, f_col, phi_col, "", 
							  use_weights, is_diff_map);

	       if (map_stat == 0) { 
		     std::cout << "Map making failure." << std::endl;
	       } else { 
		  lig.mask_by_atoms(pdb_file_name);
		  if (lig.masking_molecule_has_atoms()) {
		     lig.set_acceptable_fit_fraction(fit_frac);
		     lig.output_map("find-ligands-masked.map");
		     lig.find_clusters(input_sigma_level);
		     // install ligands:
		     for (unsigned int ilig=0; ilig<lig_files.size(); ilig++)
			lig.install_ligand(lig_files[ilig]);
		     lig.fit_ligands_to_clusters(10); 
		  } else { 
		     std::cout << "No atoms found in masking molecule: " 
			       << pdb_file_name << std::endl;
		  }
	       }

	    } else {

	       coot::wligand wlig;
	       coot::protein_geometry geom;
	       wlig.set_verbose_reporting();
	       
	       // this might be a pain if the flexible ligand is in the standard
	       // refmac dictionary...
	       if (cif_file_name.length() == 0) {
		  std::cout << "No cif dictionary file given\n";
	       } else { 
		  // geom.init_standard();
		  int geom_stat = geom.init_refmac_mon_lib(cif_file_name, 10);
		  if (geom_stat == 0) {
		     std::cout << "Critical cif dictionary reading failure." << std::endl;
		  } else { 
		     short int map_stat = wlig.map_fill_from_mtz(mtz_filename, f_col, phi_col, "", 
								 use_weights, is_diff_map);
		     
		     if (map_stat == 0) {
			std::cout << "Map making failure." << std::endl;
		     } else { 
			
			wlig.mask_by_atoms(pdb_file_name);
			if (wlig.masking_molecule_has_atoms()) { 
			   wlig.output_map("find-ligands-masked.map");
			   wlig.set_acceptable_fit_fraction(fit_frac);
			   wlig.find_clusters(input_sigma_level);

			   // install wiggly ligands...
			   // 
			   // wiggly ligands currently have to be minimols
			   for (unsigned int ilig=0; ilig<lig_files.size(); ilig++) { 
			      coot::minimol::molecule mmol;
			      mmol.read_file(lig_files[ilig]);

			      bool optim_geom = 1;
			      bool fill_vec = 0;
			      try { 
				 std::vector<coot::minimol::molecule> wiggled_ligands = 
				    wlig.install_simple_wiggly_ligands(&geom, mmol,
								       wiggly_ligand_n_samples,
								       optim_geom,
								       fill_vec);
			      }
			      catch (std::runtime_error mess) {
				 std::cout << "Failed to install flexible ligands\n" << mess.what()
					   << std::endl;
			      } 
			   }
			   wlig.fit_ligands_to_clusters(10); 
			}
		     }
		  }
	       }
	    } 
	 }
      }
   }
   return 0; 
}
