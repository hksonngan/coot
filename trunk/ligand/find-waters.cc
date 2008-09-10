/* ligand/find-waters.cc
 * 
 * Copyright 2004, 2005 The University of York
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

// Portability gubbins
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

#include "mmdb-extras.h"
#include "mmdb.h"
#include "ligand.hh"
#include "coot-map-utils.hh"
#include "coot-trim.hh"
#include "clipper/core/map_utils.h"
#include "clipper/ccp4/ccp4_map_io.h"
#include "clipper/ccp4/ccp4_mtz_io.h"
#include <iostream>

int
main(int argc, char **argv) {
   
   if (argc < 6) { 
      std::cout << "Usage: " << argv[0] 
		<< " --pdbin pdb-in-filename" << " --hklin mtz-filename"
		<< " --f f_col_label"
		<< " --phi phi_col_label"
		<< " --pdbout waters-filename"
		<< " --sigma sigma-level"
		<< " --flood"
		<< " --chop"
		<< "\n"
		<< "        --mapin ccp4-map-name can be used"
		<< " instead of --hklin --f --phi\n";
      std::cout << "        where pdbin is the protein (typically)\n"
		<< "        and pdbout is file for the waters.\n"
		<< "        The default sigma level is 2.0\n"
		<< "        Use --chop to remove waters below given sigma-level\n"
		<< "            In this case, pdbout is the modified input coordinates\n"
		<< "        Use ---flood to fill everything with waters "
		<< "(not just water peaks)\n";
   } else { 

      std::string pdb_file_name;
      std::string  mtz_filename;
      std::string         f_col;
      std::string       phi_col;
      std::string    output_pdb;
      std::string     sigma_str;
      std::string map_file_name;
      short int do_flood_flag = 0;
      short int do_chop_flag = 0;

      const char *optstr = "i:h:f:p:o:s:e:c:m";
      struct option long_options[] = {
	 {"pdbin",  1, 0, 0},
	 {"hklin",  1, 0, 0},
	 {"f",      1, 0, 0},
	 {"phi",    1, 0, 0},
	 {"pdbout", 1, 0, 0},
	 {"sigma",  1, 0, 0},
	 {"mapin",  1, 0, 0},
	 {"flood",  0, 0, 0},
	 {"chop",   0, 0, 0},
	 {0, 0, 0, 0}
      };

      int ch;
      int option_index = 0;
      while ( -1 != 
	      (ch = getopt_long(argc, argv, optstr, long_options, &option_index))) { 

	 switch(ch) { 
	    
	 case 0:
	    if (optarg) { 
	       std::string arg_str = long_options[option_index].name;

	       if (arg_str == "pdbin") { 
		  pdb_file_name = optarg;
	       } 
	       if (arg_str == "pdbout") { 
		  output_pdb = optarg;
	       } 
	       if (arg_str == "hklin") { 
		  mtz_filename = optarg;
	       } 
	       if (arg_str == "f") { 
		  f_col = optarg;
	       } 
	       if (arg_str == "phi") {
		  phi_col = optarg;
	       } 
	       if (arg_str == "sigma") {
		  sigma_str = optarg;
	       }
	       if (arg_str == "mapin") {
		  map_file_name = optarg;
	       }
	       
	    } else { 
	       std::string arg_str = long_options[option_index].name;
	       if (arg_str == "flood") {
		  std::cout << "Flooding\n";
		  do_flood_flag = 1;
	       } else {
		  if (arg_str == "chop") {
		     std::cout << "Removing waters";
		     do_chop_flag = 1;
		  } else { 
		     std::cout << "Malformed option: "
			       << long_options[option_index].name << std::endl;
		  }
	       }
	    }
	    break;

	 case 'i':
	    pdb_file_name = optarg;
	    break;
	    
	 case 'o':
	    output_pdb = optarg;
	    break;
	    
	 case 'h':
	    mtz_filename = optarg;
	    break;
	    
	 case 'f':
	    f_col = optarg;
	    break;
	    
	 case 'p':
	    phi_col = optarg;
	    break;
	    
	 case 's':
	    sigma_str = optarg;
	    break;

	 case 'e':
	    do_flood_flag = 1;
	    break;

	 case 'c':
	    do_chop_flag = 1;
	    break;
	    
	 default:
	    break;
	 }
      }

      short int do_it = 0;  // vs do flood
      short int do_it_with_map = 0;
      short int have_map = 0;
      if (map_file_name.length() > 0)
	 have_map = 1;
      
      if ( (pdb_file_name.length() == 0) && !do_flood_flag) {
	 std::cout << "Missing input PDB file\n";
	 exit(1);
      } else { 
	 if (output_pdb.length() == 0) { 
	    std::cout << "Missing output PDB file\n";
	    exit(1);
	 } else {
	    if (have_map) {
	       do_it_with_map = 1;
	       do_it = 1;
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
			do_it = 1;
		     }
		  }
	       }
	    }
	 }
      }
      
      if (do_it) { 

	 float input_sigma_level = atof(sigma_str.c_str());
	 short int use_weights = 0;
	 short int is_diff_map = 0; 
	 coot::ligand lig;
	 if (have_map) {
	    clipper::CCP4MAPfile file;
	    clipper::Xmap<float> xmap;
	    file.open_read(map_file_name);
	    file.import_xmap(xmap);
	    file.close_read();
	    lig.import_map_from(xmap);
	 } else { 
	    short int stat = lig.map_fill_from_mtz(mtz_filename, f_col, phi_col, "", 
						   use_weights, is_diff_map);

	    if (! stat) { 
	       std::cout << "ERROR: in filling map from mtz file: " << mtz_filename
			 << std::endl;
	       exit(1);
	    } else { 

	       if (! do_flood_flag) { 
		  if (pdb_file_name.length() == 0) {
		     std::cout << "confused input: no pdb input name and no --flood specified...\n";
		     exit(1);
		  } else { 
		     if (do_chop_flag == 0) { 

			// mask_by_atoms() [which calls mask_map(flag)] here in
			// find-waters currently behaves differently to
			// c-interface-waters.cc's mask_map(mol, water_flag)
			// resulting in move_atom_to_peak() moving wats to the
			// wrong place
			//
			lig.set_map_atom_mask_radius(1.9);
			lig.mask_by_atoms(pdb_file_name);
			if (lig.masking_molecule_has_atoms()) { 
			   lig.output_map("find-waters-masked.map");
			   // 		  std::cout << "DEBUG:: in findwaters: using input_sigma_level: "
			   // 			    << input_sigma_level << std::endl;
			   lig.water_fit(input_sigma_level, 3); // e.g. 2.0 sigma for 3 cycles 
			   coot::minimol::molecule water_mol = lig.water_mol();
			   water_mol.write_file(output_pdb, 20.0);
			} else { 
			   std::cout << "No atoms found in masking molecule: " 
				     << pdb_file_name << std::endl;
			}

		     } else {

			// chop[py] waters!
			float input_sigma_level = atof(sigma_str.c_str());
			clipper::Xmap<float> xmap;
			coot::util::map_fill_from_mtz(&xmap, mtz_filename, f_col, phi_col, "", 
						      use_weights, is_diff_map);
			atom_selection_container_t asc = get_atom_selection(pdb_file_name);
			short int waters_only_flag = 1;
			short int remove_or_zero_occ_flag = coot::util::TRIM_BY_MAP_DELETE;
			clipper::Map_stats stats(xmap);
			float map_level = stats.mean() + input_sigma_level * stats.std_dev();
			int n_atoms = coot::util::trim_molecule_by_map(asc, xmap, map_level,
								       remove_or_zero_occ_flag,
								       waters_only_flag);
			std::cout << "INFO:: " << n_atoms << " waters removed" << std::endl;
			asc.mol->WritePDBASCII((char *)output_pdb.c_str());
		     }
		  }
	       } else {
		  std::cout << "===================== Flood mode ======================= "
			    << std::endl;
		  // if a pdb file was defined, let's mask it
		  if (pdb_file_name.length() > 0) {
		     std::cout << "INFO:: masking map by coords in " << pdb_file_name << std::endl;
		     lig.set_map_atom_mask_radius(1.9);
		     lig.mask_by_atoms(pdb_file_name);
		  } 
		  lig.set_cluster_size_check_off();
		  lig.set_chemically_sensible_check_off();
		  lig.set_sphericity_test_off();
		  lig.set_map_atom_mask_radius(1.2);
		  lig.set_water_to_protein_distance_limits(10.0, 1.5); // should not be 
		  // used in lig.
		  lig.flood(); // with atoms (waters initially)
		  coot::minimol::molecule water_mol = lig.water_mol();
		  water_mol.write_file(output_pdb, 20.0);
		  // lig.output_map("find-waters-masked-flooded.map");
	       }
	    }
	 }
      }
   }
   return 0;
}

