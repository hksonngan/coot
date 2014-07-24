/* geometry/protein-geometry.cc
 * 
 * Copyright 2003, 2004, 2005, 2006 The University of York
 * Author: Paul Emsley
 * Copyright 2007, 2008, 2009, 2010, 2011, 2012 The University of Oxford
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

#include <string.h>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>  // needed for sort? Yes.
#include <stdexcept>  // Thow execption.

#include "mini-mol/atom-quads.hh"
#include "geometry/protein-geometry.hh"
#include "utils/coot-utils.hh"

#include <sys/types.h> // for stating
#include <sys/stat.h>

#if !defined _MSC_VER
#include <unistd.h>
#else
#define DATADIR "C:/coot/share"
#define PKGDATADIR DATADIR
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#endif

#include "clipper/core/clipper_util.h"

#include "compat/coot-sysdep.h"

#include "lbg-graph.hh"
// return the number of atoms read (not the number of bonds (because
// that is not a good measure of having read the file properly for
// (for example) CL)).
// 
int
coot::protein_geometry::init_refmac_mon_lib(std::string ciffilename, int read_number_in) {

   int ret_val = 0; 
   CMMCIFFile ciffile;

   // Here we would want to check through dict_res_restraints for the
   // existance of this restraint.  If it does exist, kill it by
   // changing its name to blank.  However, we don't know the name of
   // the restraint yet!  We only know that at the add_atom(),
   // add_bond() [etc] stage.

   // added 20120121, to fix Jack Sack and Kevin Madauss bug (so that
   // in mon_lib_add_chem_comp(), the read numbers between new file
   // and previous are different, hence trash the old restraints).
   // 
   read_number = read_number_in;

   struct stat buf;
   int istat = stat(ciffilename.c_str(), &buf);
   // Thanks Ezra Peisach for this this bug report

   if (istat != 0) {
      std::cout << "WARNING: in init_refmac_mon_lib file \"" << ciffilename
		<< "\" not found."
		<< "\n";
      return 0; // failure
   }

   if (! S_ISREG(buf.st_mode)) {
      std::cout << "WARNING: in init_refmac_mon_lib \"" << ciffilename
		<< "\" not read.  It is not a regular file.\n";
   } else { 

      int ierr = ciffile.ReadMMCIFFile(ciffilename.c_str());
      std::string comp_id_1; 
      std::string comp_id_2;  // initially unset
   
      if (ierr!=CIFRC_Ok) {
	 std::cout << "dirty mmCIF file? " << ciffilename.c_str() << std::endl;
	 std::cout << "    Bad CIFRC_Ok on ReadMMCIFFile" << std::endl;
	 std::cout << "    " << GetErrorDescription(ierr) << std::endl;
	 char        err_buff[1000];
	 std::cout <<  "CIF error rc=" << ierr << " reason:" << 
	    GetCIFMessage (err_buff,ierr) << std::endl;

      } else {
	 if (verbose_mode)
	    std::cout << "There are " << ciffile.GetNofData() << " data in "
		      << ciffilename << std::endl; 
      
	 for(int idata=0; idata<ciffile.GetNofData(); idata++) { 
         
	    PCMMCIFData data = ciffile.GetCIFData(idata);
	    
	    // note that chem_link goes here to:
	    // 
	    if (std::string(data->GetDataName()).substr(0,5) == "link_") {
	       ret_val += init_links(data);
	    }

	    if (std::string(data->GetDataName()).length() > 7) {
	       if (std::string(data->GetDataName()).substr(0,8) == "mod_list") {
		  // this handles all "list_chem_mod"s in the file (just one call)
		  ret_val += add_chem_mods(data);

		  if (0) // debug
		     for (unsigned int i=0; i<chem_mod_vec.size(); i++)
			std::cout << "     " << chem_mod_vec[i] << std::endl;
	       }
	    }
	    
	    if (std::string(data->GetDataName()).length() > 4) {
	       // e.g. mod_NH3 ? 
	       if (std::string(data->GetDataName()).substr(0,4) == "mod_") {
		  ret_val += add_mod(data);
	       }
	    }
	       

	    if (std::string(data->GetDataName()).length() > 16) { 
	       if (std::string(data->GetDataName()).substr(0,17) == "comp_synonym_list") {
		  add_synonyms(data);
	       }
	    }
	    
	    int n_chiral = 0;
	    std::vector<std::string> comp_ids_for_chirals;
	    for (int icat=0; icat<data->GetNumberOfCategories(); icat++) { 

	       PCMMCIFCategory cat = data->GetCategory(icat);
	       std::string cat_name(cat->GetCategoryName());
	       
	       // All catagories have loops (AFAICS). 
	       // std::cout << "DEBUG:: got catagory: " << cat_name << std::endl; 

	       PCMMCIFLoop mmCIFLoop = data->GetLoop(cat_name.c_str() );

	       int n_loop_time = 0;
	       if (mmCIFLoop == NULL) {

		  bool handled = false;
		  if (cat_name == "_chem_comp") {
		     // read the chemical component library which does
		     // not have a loop (the refmac files do) for the
		     // chem_comp info.
		     handled = 1;
		     PCMMCIFStruct structure = data->GetStructure(cat_name.c_str());
		     if (structure) {
			comp_id_1 = chem_comp_component(structure);
		     }
		  }

		  if (cat_name == "_chem_comp_chir") {
		     handled = 1;
		     PCMMCIFStruct structure = data->GetStructure(cat_name.c_str());
		     if (structure) {
			chem_comp_chir_structure(structure);
		     }
		  } 
		     
		  if (cat_name == "_chem_comp_tor") {
		     handled = 1;
		     PCMMCIFStruct structure = data->GetStructure(cat_name.c_str());
		     if (structure) {
			chem_comp_tor_structure(structure);
		     }
		  }
		  
		  if (! handled) 
		     std::cout << "in init_refmac_mon_lib() null loop for catagory "
			       << cat_name << std::endl; 
		  
	       } else {
               
		  n_loop_time++;

		  // We currently want to stop adding chem comp info
		  // if the chem_comp info comes from mon_lib_list.cif:
		  if (cat_name == "_chem_comp") { 
		     if (read_number_in != coot::protein_geometry::MON_LIB_LIST_CIF)
			comp_id_2 = chem_comp(mmCIFLoop);
		     else
			comp_id_2 = simple_mon_lib_chem_comp(mmCIFLoop);
		  }
		  
		  // monomer info, name, number of atoms etc.
		  if (cat_name == "_chem_comp_atom")
		     ret_val += comp_atom(mmCIFLoop); // and at the end pad up the atom names

		  // tree
		  if (cat_name == "_chem_comp_tree")
		     comp_tree(mmCIFLoop);

		  // bond
		  if (cat_name == "_chem_comp_bond")
		     comp_bond(mmCIFLoop);

		  // angle
		  if (cat_name == "_chem_comp_angle")
		     comp_angle(mmCIFLoop);

		  // tor
		  if (cat_name == "_chem_comp_tor")
		     comp_torsion(mmCIFLoop);

		  // chiral
		  if (cat_name == "_chem_comp_chir") {
		     std::pair<int, std::vector<std::string> > chirals = 
			comp_chiral(mmCIFLoop);
		     n_chiral += chirals.first;
		     for (unsigned int ichir=0; ichir<chirals.second.size(); ichir++)
			comp_ids_for_chirals.push_back(chirals.second[ichir]);
		  }
               
		  // plane
		  if (cat_name == "_chem_comp_plane_atom")
		     comp_plane(mmCIFLoop);

		  // PDBx stuff
		  if (cat_name == "_pdbx_chem_comp_descriptor")
		     pdbx_chem_comp_descriptor(mmCIFLoop);

	       }
	    }
	    if (n_chiral) {
	       assign_chiral_volume_targets();
	       filter_chiral_centres(comp_ids_for_chirals);
	    }
	 } // for idata
	 add_cif_file_name(ciffilename, comp_id_1, comp_id_2);
      } // cif file is OK test
   } // is regular file test

   // debug_mods();
   
   return ret_val; // the number of atoms read.
}


// 
void
coot::protein_geometry::add_cif_file_name(const std::string &cif_file_name,
					  const std::string &comp_id1,
					  const std::string &comp_id2) {

   std::string comp_id = comp_id1;
   if (comp_id == "")
      comp_id = comp_id2;
   if (comp_id != "") { 
      int idx = get_monomer_restraints_index(comp_id2, true);
      if (idx != -1) {
	 dict_res_restraints[idx].cif_file_name = cif_file_name;
      }
   }
}


std::string
coot::protein_geometry::get_cif_file_name(const std::string &comp_id) const {

   std::string r; 
   int idx = get_monomer_restraints_index(comp_id, true);
   if (idx != -1)
      r = dict_res_restraints[idx].cif_file_name;
   return r;
}
      


// return the comp id (so that later we can associate the file name with the comp_id).
// 
std::string 
coot::protein_geometry::chem_comp_component(PCMMCIFStruct structure) {

   int n_tags = structure->GetNofTags();
   std::string cat_name = structure->GetCategoryName();

   if (0) 
      std::cout << "DEBUG: ================= chem_comp_component() by structure: in category "
		<< cat_name << " there are "
		<< n_tags << " tags" << std::endl;
    
   std::pair<bool, std::string> comp_id(0, "");
   std::pair<bool, std::string> three_letter_code(0, "");
   std::pair<bool, std::string> name(0, "");
   std::pair<bool, std::string> type(0, ""); // aka group?
   int number_of_atoms_all = coot::protein_geometry::UNSET_NUMBER;
   int number_of_atoms_nh  = coot::protein_geometry::UNSET_NUMBER;
   std::pair<bool, std::string> description_level(0, "");

   for (int itag=0; itag<n_tags; itag++) {
      std::string tag = structure->GetTag(itag);
      std::string field = structure->GetField(itag);
      // std::cout << " by structure got tag " << itag << " "
      // << tag << " field: " << f << std::endl;
      if (tag == "id")
	 comp_id = std::pair<bool, std::string> (1,field);
      if (tag == "three_letter_code")
	 three_letter_code = std::pair<bool, std::string> (1,field);
      if (tag == "name")
	 name = std::pair<bool, std::string> (1,field);
      if (tag == "type")
	 type = std::pair<bool, std::string> (1,field);
      if (tag == "descr_level")
	 description_level = std::pair<bool, std::string> (1,field);
      if (tag == "description_level")
	 description_level = std::pair<bool, std::string> (1,field);
      // number of atoms here too.

      if (tag == "number_atoms_all") { 
	 try {
	    number_of_atoms_all = coot::util::string_to_int(field);
	 }
	 catch (std::runtime_error rte) {
	    std::cout << rte.what() << std::endl;
	 }
      }
      if (tag == "number_atoms_nh") { 
	 try {
	    number_of_atoms_nh = coot::util::string_to_int(field);
	 }
	 catch (std::runtime_error rte) {
	    std::cout << rte.what() << std::endl;
	 }
      }
   }

   if (0) 
      std::cout
	 << "chem_comp_component() comp_id :" << comp_id.first << " :" << comp_id.second << ": "
	 << "three_letter_code :" << three_letter_code.first << " :" << three_letter_code.second
	 << ": "
	 << "name :" << name.first << " :" << name.second << ": "
	 << "type :" << type.first << " :" << type.second << ": "
	 << "description_level :" << description_level.first << " :" << description_level.second
	 << ": "
	 << std::endl;

   if (comp_id.first && three_letter_code.first && name.first) {

      mon_lib_add_chem_comp(comp_id.second, three_letter_code.second,
			    name.second, type.second,
			    number_of_atoms_all, number_of_atoms_nh,
			    description_level.second);
   } else { 
      // std::cout << "oooppps - something missing, not adding that" << std::endl;
   }

   if (comp_id.first)
      return comp_id.second;
   else
      return "";
}


// non-looping (single) tor
void
coot::protein_geometry::chem_comp_tor_structure(PCMMCIFStruct structure) {
   
   int n_tags = structure->GetNofTags();
   std::string cat_name = structure->GetCategoryName();

   if (0)
      std::cout << "DEBUG: ================= by chem_comp_tor by structure: in category "
		<< cat_name << " there are "
		<< n_tags << " tags" << std::endl;

   std::pair<bool, std::string> comp_id(0, "");
   std::pair<bool, std::string> torsion_id(0, "");
   std::pair<bool, std::string> atom_id_1(0, "");
   std::pair<bool, std::string> atom_id_2(0, "");
   std::pair<bool, std::string> atom_id_3(0, "");
   std::pair<bool, std::string> atom_id_4(0, "");
   std::pair<bool, int> period(0, 0);
   std::pair<bool, realtype> value_angle(0, 0);
   std::pair<bool, realtype> value_angle_esd(0, 0);
   
   for (int itag=0; itag<n_tags; itag++) {
      std::string tag = structure->GetTag(itag);
      std::string field = structure->GetField(itag);
      // std::cout << " by structure got tag " << itag << " \""
      // << tag << "\" field: \"" << field << "\"" << std::endl;
      if (tag == "comp_id")
	 comp_id = std::pair<bool, std::string> (1,field);
      if (tag == "torsion_id")
	 torsion_id = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_1")
	 atom_id_1 = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_2")
	 atom_id_2 = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_3")
	 atom_id_3 = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_4")
	 atom_id_4 = std::pair<bool, std::string> (1,field);
      if (tag == "period") { 
	 try { 
	    period = std::pair<bool, int> (1,coot::util::string_to_int(field));
	 }
	 catch (std::runtime_error rte) {
	    std::cout << "WARNING:: not an integer: " << field << std::endl;
	 }
      }
      if (tag == "value_angle") { 
	 try { 
	    value_angle = std::pair<bool, float> (1,coot::util::string_to_float(field));
	 }
	 catch (std::runtime_error rte) {
	    std::cout << "WARNING:: value_angle not an number: " << field << std::endl;
	 }
      }
      if (tag == "value_angle_esd") { 
	 try { 
	    value_angle_esd = std::pair<bool, float> (1,coot::util::string_to_float(field));
	 }
	 catch (std::runtime_error rte) {
	    std::cout << "WARNING:: value_angle_esd not an number: " << field << std::endl;
	 }
      }
   }

   if (comp_id.first && 
       atom_id_1.first && atom_id_2.first && atom_id_3.first && atom_id_4.first &&
       value_angle.first && value_angle_esd.first && 
       period.first) {
      mon_lib_add_torsion(comp_id.second,
			  torsion_id.second,
			  atom_id_1.second,
			  atom_id_2.second,
			  atom_id_3.second,
			  atom_id_4.second,
			  value_angle.second, value_angle_esd.second,
			  period.second);
   } else {
      std::cout << "WARNING:: chem_comp_tor_structure() something bad" << std::endl;
   } 
}

// non-looping (single) chir
void
coot::protein_geometry::chem_comp_chir_structure(PCMMCIFStruct structure) {

   int n_tags = structure->GetNofTags();
   std::string cat_name = structure->GetCategoryName();

   if (0)
      std::cout << "DEBUG: ================= by chem_comp_dot by structure: in category "
		<< cat_name << " there are "
		<< n_tags << " tags" << std::endl;

   std::pair<bool, std::string> comp_id(0, "");
   std::pair<bool, std::string>      id(0, "");
   std::pair<bool, std::string> atom_id_centre(0, "");
   std::pair<bool, std::string> atom_id_1(0, "");
   std::pair<bool, std::string> atom_id_2(0, "");
   std::pair<bool, std::string> atom_id_3(0, "");
   std::pair<bool, std::string> volume_sign(0, "");
   
   for (int itag=0; itag<n_tags; itag++) {
      std::string tag = structure->GetTag(itag);
      std::string field = structure->GetField(itag);
      // std::cout << " by structure got tag " << itag << " \""
      // << tag << "\" field: \"" << field << "\"" << std::endl;
      if (tag == "comp_id")
	 comp_id = std::pair<bool, std::string> (1,field);
      if (tag == "id")
	 id = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_centre")
	 atom_id_centre = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_1")
	 atom_id_1 = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_2")
	 atom_id_2 = std::pair<bool, std::string> (1,field);
      if (tag == "atom_id_3")
	 atom_id_3 = std::pair<bool, std::string> (1,field);
      if (tag == "volume_sign")
	 volume_sign = std::pair<bool, std::string> (1,field);
   }

   if (comp_id.first && atom_id_centre.first &&
       atom_id_1.first && atom_id_2.first && atom_id_3.first &&
       volume_sign.first) {
      mon_lib_add_chiral(comp_id.second,
			 id.second,
			 atom_id_centre.second,
			 atom_id_1.second,
			 atom_id_2.second,
			 atom_id_3.second,
			 volume_sign.second);
   } else {
      std::cout << "WARNING:: chem_comp_chir_structure() something bad" << std::endl;
   } 
}

void
coot::protein_geometry::mon_lib_add_chem_comp(const std::string &comp_id,
					      const std::string &three_letter_code,
					      const std::string &name,
					      const std::string &group,
					      int number_atoms_all, int number_atoms_nh,
					      const std::string &description_level) {


   if (0) 
      std::cout << "DEBUG:: in mon_lib_add_chem_comp :"
		<< comp_id << ": :" << three_letter_code << ": :"
		<< name << ": :" << group << ": :"
		<< description_level << ": :" << number_atoms_all << ": :"
		<< number_atoms_nh << std::endl;
   
   coot::dict_chem_comp_t ri(comp_id, three_letter_code, name, group,
			     number_atoms_all, number_atoms_nh,
			     description_level);
   bool ifound = false;

   for (unsigned int i=0; i<dict_res_restraints.size(); i++) {
      if (dict_res_restraints[i].residue_info.comp_id == comp_id) {

	 if (0) 
	    std::cout << "DEBUG:: comparing read numbers: " << dict_res_restraints[i].read_number
		      << " " << read_number << std::endl;
	 
	 if (dict_res_restraints[i].read_number == read_number) { 
	    ifound = true;
	    dict_res_restraints[i].residue_info = ri;
	    break;
	 } else {
	    // trash the old one then
	    // Message for Kevin.
	    std::cout << "INFO:: clearing old restraints for  " << comp_id << std::endl;
	    dict_res_restraints[i].clear_dictionary_residue();
	 }
      }
   }

   if (! ifound) {
      // std::cout << "DEBUG:: residue not found in mon_lib_add_chem_comp" << std::endl;
      dict_res_restraints.push_back(dictionary_residue_restraints_t(comp_id, read_number));
      dict_res_restraints[dict_res_restraints.size()-1].residue_info = ri;
   }
}


// add to simple_monomer_descriptions not dict_res_restraints.
void
coot::protein_geometry::simple_mon_lib_add_chem_comp(const std::string &comp_id,
					      const std::string &three_letter_code,
					      const std::string &name,
					      const std::string &group,
					      int number_atoms_all, int number_atoms_nh,
					      const std::string &description_level) {


   if (0)
      std::cout << "------- DEBUG:: in simple_mon_lib_add_chem_comp comp_id :"
		<< comp_id << ": three-letter-code :" << three_letter_code << ": name :"
		<< name << ": :" << group << ": descr-lev :"
		<< description_level << ": :" << number_atoms_all << ": :"
		<< number_atoms_nh << std::endl;

   // notice that we also pass the comp_id here (a different constructor needed);
   coot::dict_chem_comp_t ri(comp_id, three_letter_code, name, group, number_atoms_all,
			     number_atoms_nh, description_level);


   std::map<std::string,coot::dictionary_residue_restraints_t>::const_iterator it =
      simple_monomer_descriptions.find(comp_id);

   coot::dictionary_residue_restraints_t blank_res_rest;
   blank_res_rest.residue_info = ri; // now mostly blank
   simple_monomer_descriptions[comp_id] = blank_res_rest;
   
//    std::cout << "Added [residue info :"
// 	     << ri.comp_id << ": :"
// 	     << ri.three_letter_code << ": " 
// 	     << ri.group << "] " 
// 	     << " for key :"
// 	     << comp_id << ":" << " simple_monomer_descriptions size() "
// 	     << simple_monomer_descriptions.size() 
// 	     << std::endl;

}


void
coot::protein_geometry::mon_lib_add_atom(const std::string &comp_id,
					 const std::string &atom_id,
					 const std::string &atom_id_4c,
					 const std::string &type_symbol,
					 const std::string &type_energy,
					 const std::pair<bool, realtype> &partial_charge,
					 const std::pair<bool, clipper::Coord_orth> &model_pos,
					 const std::pair<bool, clipper::Coord_orth> &model_pos_ideal) { 

   // debugging
   bool debug = false;
   
   if (debug) { 
      std::cout << "   mon_lib_add_atom  " << comp_id << " atom-id:" << atom_id << ": :"
		<< atom_id_4c << ": " << type_symbol << " " << type_energy << " ("
		<< partial_charge.first << "," << partial_charge.second << ")" << std::endl;
   } 

   coot::dict_atom at_info(atom_id, atom_id_4c, type_symbol, type_energy, partial_charge);
   if (debug) { 
      std::cout << "mon_lib_add_atom " << model_pos.first << " "
		<< model_pos.second.format() << std::endl;
      std::cout << "mon_lib_add_atom " << model_pos_ideal.first << " "
		<< model_pos_ideal.second.format() << std::endl;
   }
   
   if (model_pos.first)
      at_info.add_pos(coot::dict_atom::REAL_MODEL_POS, model_pos);
   if (model_pos_ideal.first)
      at_info.add_pos(coot::dict_atom::IDEAL_MODEL_POS, model_pos_ideal);

   bool ifound = 0;
   int this_index = -1; // unset

   for (unsigned int i=0; i<dict_res_restraints.size(); i++) {
//       std::cout << "comparing comp_ids: :" << dict_res_restraints[i].comp_id
// 		<< ":  :" << comp_id << ":" << std::endl;
      
      if (dict_res_restraints[i].residue_info.comp_id == comp_id) {

// 	 std::cout << "comparing read numbers: "
// 		   << dict_res_restraints[i].read_number << " and "
// 		   << read_number << std::endl;
	 if (dict_res_restraints[i].read_number == read_number) { 
	    ifound = true;
	    this_index = i;
	    dict_res_restraints[i].atom_info.push_back(at_info);
	    break;
	 } else {
	    // trash the old one then
	    dict_res_restraints[i].clear_dictionary_residue();
	 }
      }
   }

   if (! ifound) {
      // std::cout << "residue not found in mon_lib_add_atom" << std::endl;
      dict_res_restraints.push_back(dictionary_residue_restraints_t(comp_id, read_number));
      this_index = dict_res_restraints.size()-1;
      dict_res_restraints[this_index].atom_info.push_back(at_info);
   }

   if (debug) {
      std::cout << "   dictionary for " << dict_res_restraints[this_index].residue_info.comp_id
		<< " now contains " << dict_res_restraints[this_index].atom_info.size()
		<< " atoms" << std::endl;
      for (unsigned int i=0; i<dict_res_restraints[this_index].atom_info.size(); i++) { 
	 // 	  std::cout << "  " << i << "  " << dict_res_restraints[this_index].atom_info[i]
	 // 		    << std::endl;
      }
   }
}
void
coot::dict_atom::add_pos(int pos_type,
			 const std::pair<bool, clipper::Coord_orth> &model_pos) {

   if (pos_type == coot::dict_atom::IDEAL_MODEL_POS)
      pdbx_model_Cartn_ideal = model_pos;
   if (pos_type == coot::dict_atom::REAL_MODEL_POS)
      model_Cartn = model_pos;

}

void
coot::protein_geometry::mon_lib_add_tree(std::string comp_id,
					 std::string atom_id,
					 std::string atom_back,
					 std::string atom_forward,
					 std::string connect_type) {
   bool ifound = 0;
   coot::dict_chem_comp_tree_t ac(atom_id, atom_back, atom_forward, connect_type);
   // std::cout << "mon_lib_add_tree atom :" << atom_id << ":" << std::endl;
   for (unsigned int i=0; i<dict_res_restraints.size(); i++) {
      if (dict_res_restraints[i].residue_info.comp_id == comp_id) {
	 ifound = 1;
	 dict_res_restraints[i].tree.push_back(ac);
	 break;
      }
   }
}

void
coot::protein_geometry::mon_lib_add_bond(std::string comp_id,
				   std::string atom_id_1,
				   std::string atom_id_2,
				   std::string type,
				   realtype value_dist, realtype value_dist_esd) {

//     std::cout << "adding bond for " << comp_id << " " << atom_id_1
// 	      << " " << atom_id_2 << " " << type << " " << value_dist
// 	      << " " << value_dist_esd << std::endl;

   // add a bond restraint to the list for comp_id.
   // The list container for comp_id is a dictionary_residue_restraints_t

   add_restraint(comp_id, dict_bond_restraint_t(atom_id_1,
						atom_id_2,
						type,
						value_dist,
						value_dist_esd));
}

void
coot::protein_geometry::mon_lib_add_bond_no_target_geom(std::string comp_id,
							std::string atom_id_1,
							std::string atom_id_2,
							std::string type) { 

   add_restraint(comp_id, dict_bond_restraint_t(atom_id_1, atom_id_2, type));
}


void 
coot::protein_geometry::add_restraint(std::string comp_id, const dict_bond_restraint_t &restr) { 

   // if comp is in the list, simply push back restr, 
   // 
   // if not, then push back a dict_bond_restraint_t for it, passing
   // the comp_id. 

   bool ifound = 0;

   for (unsigned int i=0; i<dict_res_restraints.size(); i++) { 
      if (dict_res_restraints[i].residue_info.comp_id == comp_id) { 
	 ifound = 1;
	 dict_res_restraints[i].bond_restraint.push_back(restr); 
	 break;
      }
   } 

   // it was not there
   if (! ifound) { 
      dict_res_restraints.push_back(dictionary_residue_restraints_t(comp_id, read_number));
      // add the bond to the newly created dictionary_residue_restraints_t
      dict_res_restraints[dict_res_restraints.size()-1].bond_restraint.push_back(restr);
   }
}

void 
coot::protein_geometry::add_restraint(std::string comp_id, const dict_angle_restraint_t &restr) { 

   // if comp is in the list, simply push back restr, 
   // 
   // if not, then push back a dict_bond_restraint_t for it, passing
   // the comp_id. 

   short int ifound = 0;

   for (unsigned int i=0; i<dict_res_restraints.size(); i++) { 
      if (dict_res_restraints[i].residue_info.comp_id == comp_id) { 
	 ifound = 1;
	 dict_res_restraints[i].angle_restraint.push_back(restr); 
	 break;
      }
   } 

   // it was not there
   if (! ifound) { 
      dict_res_restraints.push_back(dictionary_residue_restraints_t(comp_id, read_number));
      // add the angle to the newly created dictionary_residue_restraints_t
      dict_res_restraints[dict_res_restraints.size()-1].angle_restraint.push_back(restr);
   }
}
void
coot::protein_geometry::mon_lib_add_angle(std::string comp_id,
				    std::string atom_id_1,
				    std::string atom_id_2,
				    std::string atom_id_3,
				    realtype value_angle,
				    realtype value_angle_esd) {

//    std::cout << "adding angle " << comp_id <<  " " << atom_id_1
// 	     << " " << atom_id_2 << " " << atom_id_3 << " "
// 	     << value_angle << std::endl;

   add_restraint(comp_id, dict_angle_restraint_t(atom_id_1,
						atom_id_2,
						atom_id_3,
						value_angle,
						value_angle_esd));
}

void
coot::protein_geometry::mon_lib_add_torsion(std::string comp_id,
					    std::string torsion_id,
					    std::string atom_id_1,
					    std::string atom_id_2,
					    std::string atom_id_3,
					    std::string atom_id_4,
					    realtype value_angle,
					    realtype value_angle_esd,
					    int period) {

   if (0)
      std::cout << "adding torsion " << comp_id <<  " " << atom_id_1
		<< " " << atom_id_2 << " " << atom_id_3 << " "
		<< atom_id_4 << " "
		<< "value_angle: " << value_angle
		<< ", value_angle_esd: " << value_angle_esd
		<< ", period: " << period << std::endl;
   
   add_restraint(comp_id, dict_torsion_restraint_t(torsion_id,
						   atom_id_1,
						   atom_id_2,
						   atom_id_3,
						   atom_id_4,
						   value_angle,
						   value_angle_esd,
						   period));

}



void
coot::protein_geometry::mon_lib_add_chiral(std::string comp_id,
				     std::string id,
				     std::string atom_id_centre,
				     std::string atom_id_1,
				     std::string atom_id_2,
				     std::string atom_id_3,
				     std::string volume_sign) {

    
    int volume_sign_int = 0;

    volume_sign_int = coot::protein_geometry::chiral_volume_string_to_chiral_sign(volume_sign);

    // std::cout << "DEBUG:: " << comp_id << " " << atom_id_centre << " " << volume_sign
    // << " " << volume_sign_int << std::endl;

//     std::cout << "adding chiral " << comp_id <<  " " << id
// 	      << " " << atom_id_centre << " " << volume_sign 
// 	      << " " << volume_sign.substr(0,3) << " " << volume_sign_int << endl; 
    
    // We only want to know about dictionary restraints that are
    // certainly one thing or the other, not "both".  The CB of VAL is
    // labelled as "both".  This is because it is the formal
    // definition of a chiral centre - the xGn atoms in a VAL are
    // equivalent so the Cb is not a chiral centre. Only if the atoms
    // are not equivalent can the atom be a chiral centre.  However,
    // in the equivalent atom case, we can use the "chiral" volume to
    // find residues with nomenclature errors (the CG1 and CG2 atom
    // names are swapped).
    // 
    if (volume_sign_int != 0)
       if (volume_sign_int != coot::dict_chiral_restraint_t::CHIRAL_VOLUME_RESTRAINT_VOLUME_SIGN_UNASSIGNED)
	  add_restraint(comp_id, dict_chiral_restraint_t(id, atom_id_centre,
							 atom_id_1,
						      atom_id_2,
							 atom_id_3,
							 volume_sign_int));
    

}

// Add a plane restraint atom
//
// Add a plane atom, we look through restraints trying to find a
// comp_id, and then a plane_id, if we find it, simply push back
// the atom name, if not, we create a restraint.
// 
void
coot::protein_geometry::mon_lib_add_plane(const std::string &comp_id,
					  const std::string &plane_id,
					  const std::string &atom_id,
					  const realtype &dist_esd) {

   if (0)
      std::cout << "adding plane " << comp_id <<  " " << plane_id
		<< " " << atom_id << std::endl;

   bool ifound = false;

   for (unsigned int i=0; i<dict_res_restraints.size(); i++) { 
      if (dict_res_restraints[i].residue_info.comp_id == comp_id) {
	 for (unsigned int ip=0; ip<dict_res_restraints[i].plane_restraint.size(); ip++) {
	    if (dict_res_restraints[i].plane_restraint[ip].plane_id == plane_id) { 
	       ifound = true;
	       dict_res_restraints[i].plane_restraint[ip].push_back_atom(atom_id, dist_esd);
	       break;
	    }
	 }
	 if (! ifound ) {
	    // we have comp_id, but no planes of that id.
	    coot::dict_plane_restraint_t res(plane_id, atom_id, dist_esd);
	    dict_res_restraints[i].plane_restraint.push_back(res);
	    ifound = true;
	 }
      }
   } 

   // It was not there.  This should only happen when plane restraints
   // are declared in the dictionary *before* the bonds (or angles).
   // Which is never, thanks to Alexei.
   //
   // So, there was no comp_id found 
   if (! ifound) { 
      // add the plane to the newly created dictionary_residue_restraints_t
      dict_res_restraints.push_back(dictionary_residue_restraints_t(comp_id, read_number));
      coot::dict_plane_restraint_t res(plane_id, atom_id, dist_esd);
      dict_res_restraints[dict_res_restraints.size()-1].plane_restraint.push_back(res);
   }
}
// We currently want to stop adding chem comp info
// if the chem_comp info comes from mon_lib_list.cif:
//
// This is the function that we use read things other than
// MON_LIB_LIST_CIF.  i.e. bespoke ligand dictionaries.
//
// return the chem_comp
// 
std::string
coot::protein_geometry::chem_comp(PCMMCIFLoop mmCIFLoop) {

   int ierr = 0;
   std::string returned_chem_comp; 

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {

      std::string id; // gets stored as comp_id, but is labelled "id"
		      // in the cif file chem_comp block.

      // modify a reference (ierr)
      // 
      std::string three_letter_code;
      std::string name;
      std::string group; // e.g. "L-peptide"
      int number_atoms_all;
      int number_atoms_nh;
      std::string description_level = "None";
      
      int ierr_tot = 0;
      char *s = mmCIFLoop->GetString("id", j, ierr);
      ierr_tot += ierr;
      if (s) 
	 id = s;
      
      s = mmCIFLoop->GetString("three_letter_code", j, ierr);
      ierr_tot += ierr;
      if (s)
	 three_letter_code = s;

      s = mmCIFLoop->GetString("name", j, ierr);
      ierr_tot += ierr;
      if (s)
	 name = s;

      s = mmCIFLoop->GetString("group", j, ierr);
      ierr_tot += ierr;
      if (s)
	 group = s; // e.g. "L-peptide"

      ierr = mmCIFLoop->GetInteger(number_atoms_all, "number_atoms_all", j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(number_atoms_nh, "number_atoms_nh", j);
      ierr_tot += ierr;

      // If desc_level is in the file, extract it, otherwise set it to "None"
      // 
      s = mmCIFLoop->GetString("desc_level", j, ierr);
      if (! ierr) {
	 if (s) { 
	    description_level = s;  // e.g. "." for full, I think
	 } else {
	    // if the description_level is "." in the cif file, then
	    // GetString() does not fail, but s is set to NULL
	    // (slightly surprising).
	    description_level = ".";
	 } 
      } else {
	 std::cout << "WARNING:: desc_level was not set " << j << std::endl;
      } 


      if (ierr_tot != 0) {
	 std::cout << "oops:: ierr_tot was " << ierr_tot << std::endl;
      } else { 

	 if (0) {
	    std::cout << "in chem_comp, adding chem_comp, description_level is :"
		      << description_level << ":" << std::endl;

	    std::cout << "Adding :" << id << ": :" << three_letter_code << ": :" << name << ": :"
		      << group << ": " << number_atoms_all << " "
		      << number_atoms_nh << " :" << description_level << ":" << std::endl;
	 }

	 delete_mon_lib(id); // delete it if it exists already.

	 mon_lib_add_chem_comp(id, three_letter_code, name,
			       group, number_atoms_all, number_atoms_nh,
			       description_level);
	 returned_chem_comp = id;
      }
   }
   return returned_chem_comp;
}

// We currently want to stop adding chem comp info
// if the chem_comp info comes from mon_lib_list.cif:
//
// This is the function that we use read MON_LIB_LIST_CIF
// 
// return the comp_id
std::string
coot::protein_geometry::simple_mon_lib_chem_comp(PCMMCIFLoop mmCIFLoop) {

   int ierr = 0;
   std::string comp_id;
   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 
      // modify a reference (ierr)
      // 
      char *s = mmCIFLoop->GetString("id", j, ierr);
      std::string three_letter_code;
      std::string name;
      std::string group; // e.g. "L-peptide"
      int number_atoms_all;
      int number_atoms_nh;
      std::string description_level = "None";

      if (ierr == 0) {
	 int ierr_tot = 0;
	 comp_id = s;
	 s = mmCIFLoop->GetString("three_letter_code", j, ierr);
	 ierr_tot += ierr;
	 if (s)
	    three_letter_code = s;
	 else {
	    three_letter_code = "";
// 	    std::cout << "WARNING:: failed to get 3-letter code for comp_id: "
// 		      << comp_id << " error: " << ierr << std::endl;
	 }

	 s = mmCIFLoop->GetString("name", j, ierr);
	 ierr_tot += ierr;
	 if (s)
	    name = s;

	 s = mmCIFLoop->GetString("group", j, ierr);
	 ierr_tot += ierr;
	 if (s)
	    group = s; // e.g. "L-peptide"

	 ierr = mmCIFLoop->GetInteger(number_atoms_all, "number_atoms_all", j);
	 ierr_tot += ierr;

	 ierr = mmCIFLoop->GetInteger(number_atoms_nh, "number_atoms_nh", j);
	 ierr_tot += ierr;

	 s = mmCIFLoop->GetString("desc_level", j, ierr);
	 if (! ierr)
	    if (s)
	       description_level = s;  // e.g. "." for full, I think

	 if (ierr_tot == 0) {
	    simple_mon_lib_add_chem_comp(comp_id, three_letter_code, name,
					 group, number_atoms_all, number_atoms_nh,
					 description_level);

	 }
      }
   }
   return comp_id;
}

// return the number of atoms.
int 
coot::protein_geometry::comp_atom(PCMMCIFLoop mmCIFLoop) {

   // If the number of atoms with partial charge matches the number of
   // atoms, then set a flag in the residue that this monomer has
   // partial charges.

   int ierr = 0;
   int n_atoms = 0;
   int n_atoms_with_partial_charge = 0;
   // count the following to see if we need to delete the model/ideal
   // atom coordinates because there were all at the origin.
   int n_origin_ideal_atoms = 0; 
   int n_origin_model_atoms = 0;
   std::string comp_id; // used to delete atoms (if needed).
   
   std::string comp_id_for_partial_charges = "unset"; // unassigned.

   

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      // modify a reference (ierr)
      // 
      char *s = mmCIFLoop->GetString("comp_id",j,ierr);
      std::string atom_id;
      std::string type_symbol; 
      std::string type_energy = "unset";
      std::pair<bool, realtype> partial_charge(0,0);

      std::pair<bool, int> pdbx_align(0, 0);
      int xalign;
      int ierr_optional;
      
      std::pair<bool, std::string> pdbx_aromatic_flag(0,"");
      std::pair<bool, std::string> pdbx_leaving_atom_flag(0,"");
      std::pair<bool, std::string> pdbx_stereo_config_flag(0,"");
      std::pair<bool, clipper::Coord_orth> pdbx_model_Cartn_ideal;
      std::pair<bool, clipper::Coord_orth> model_Cartn;

      if (ierr == 0) {
	 int ierr_tot = 0;
	 if (s)
	    comp_id = std::string(s); // e.g. "ALA"

	 s = mmCIFLoop->GetString("atom_id",j,ierr);
	 ierr_tot += ierr;
	 if (s) {
	    atom_id = s; 
	 }
			
	 s = mmCIFLoop->GetString("type_symbol",j,ierr);
	 ierr_tot += ierr;
	 if (s) {
	    type_symbol = s; 
	 }

	 int ierr_optional = 0;
	 s = mmCIFLoop->GetString("type_energy",j,ierr_optional);
	 if (s) {
	    type_energy = s; 
	 }

	 ierr_optional = mmCIFLoop->GetInteger(xalign, "pdbx_align", j);
	 if (! ierr_optional)
	    pdbx_align = std::pair<bool, int> (1, xalign);

	 s = mmCIFLoop->GetString("pdbx_aromatic_flag", j, ierr_optional);
	 if (s) {
	    if (! ierr_optional) 
	       pdbx_aromatic_flag = std::pair<bool, std::string> (1, s);
	 } 

	 s = mmCIFLoop->GetString("pdbx_leaving_atom_flag", j, ierr_optional);
	 if (s) {
	    if (! ierr_optional) 
	       pdbx_leaving_atom_flag = std::pair<bool, std::string> (1, s);
	 } 

	 s = mmCIFLoop->GetString("pdbx_stereo_config_flag", j, ierr_optional);
	 if (s) {
	    if (! ierr_optional) 
	       pdbx_stereo_config_flag = std::pair<bool, std::string> (1, s);
	 }

	 realtype x,y,z;
	 pdbx_model_Cartn_ideal.first = 0;
	 int ierr_optional_x = mmCIFLoop->GetReal(x, "pdbx_model_Cartn_x_ideal", j);
	 int ierr_optional_y = mmCIFLoop->GetReal(y, "pdbx_model_Cartn_y_ideal", j);
	 int ierr_optional_z = mmCIFLoop->GetReal(z, "pdbx_model_Cartn_z_ideal", j);
	 if (ierr_optional_x == 0)
	    if (ierr_optional_y == 0)
	       if (ierr_optional_z == 0) { 
		  if (close_float_p(x, 0.0))
		     if (close_float_p(z, 0.0))
			if (close_float_p(z, 0.0))
			   n_origin_ideal_atoms++;
		  pdbx_model_Cartn_ideal = std::pair<bool, clipper::Coord_orth>(1, clipper::Coord_orth(x,y,z));
	       }

	 model_Cartn.first = 0;
	 ierr_optional_x = mmCIFLoop->GetReal(x, "model_Cartn_x", j);
	 ierr_optional_y = mmCIFLoop->GetReal(y, "model_Cartn_y", j);
	 ierr_optional_z = mmCIFLoop->GetReal(z, "model_Cartn_z", j);
	 if (ierr_optional_x == 0)
	    if (ierr_optional_y == 0)
	       if (ierr_optional_z == 0) { 
		  model_Cartn = std::pair<bool, clipper::Coord_orth>(1, clipper::Coord_orth(x,y,z));
		  if (close_float_p(x, 0.0))
		     if (close_float_p(z, 0.0))
			if (close_float_p(z, 0.0))
			   n_origin_model_atoms++;
	       }

	 // Try simple x, y, z (like the refmac dictionary that Garib sent has)
	 // 
	 if (model_Cartn.first == 0) {
	    ierr_optional_x = mmCIFLoop->GetReal(x, "x", j);
	    ierr_optional_y = mmCIFLoop->GetReal(y, "y", j);
	    ierr_optional_z = mmCIFLoop->GetReal(z, "z", j);
	    if (ierr_optional_x == 0)
	       if (ierr_optional_y == 0)
		  if (ierr_optional_z == 0) {
		     model_Cartn = std::pair<bool, clipper::Coord_orth>(true, clipper::Coord_orth(x,y,z));
		     if (close_float_p(x, 0.0))
			if (close_float_p(z, 0.0))
			   if (close_float_p(z, 0.0))
			      n_origin_model_atoms++;
		  }
	 }

	 // It's possible that this data type is not in the cif file,
	 // so don't fail if we can't read it.

	 realtype tmp_var;
	 ierr = mmCIFLoop->GetReal(tmp_var, "partial_charge", j);
	 if (ierr == 0) {
	    partial_charge = std::pair<bool, float>(1, tmp_var);
	    n_atoms_with_partial_charge++;
	 }

	 if (ierr_tot == 0) {

	    std::string padded_name = comp_atom_pad_atom_name(atom_id, type_symbol);
//  	    std::cout << "comp_atom_pad_atom_name: in :" << atom_id << ": out :"
//  		      << padded_name << ":" << std::endl;
	    n_atoms++;
	    if (comp_id_for_partial_charges != "bad match") { 
	       if (comp_id_for_partial_charges == "unset") {
		  comp_id_for_partial_charges = comp_id;
	       } else {
		  if (comp_id != comp_id_for_partial_charges) {
		     comp_id_for_partial_charges = "bad match";
		  }
	       }
	    }

	    if (0) 
	       std::cout << "debug:: calling mon_lib_add_atom: "
			 << ":" << comp_id << ":  "
			 << ":" << atom_id << ":  "
			 << ":" << padded_name << ":  "
			 << ":" << type_symbol << ":  "
			 << model_Cartn.second.format() << " "
			 << pdbx_model_Cartn_ideal.second.format()
			 << std::endl;
	    mon_lib_add_atom(comp_id, atom_id, padded_name, type_symbol, type_energy,
			     partial_charge, model_Cartn, pdbx_model_Cartn_ideal);

	 } else {
	    std::cout << " error on read " << ierr_tot << std::endl;
	 } 
      }
   }

   if (n_atoms_with_partial_charge == n_atoms) {
      if (comp_id_for_partial_charges != "unset") {
	 if (comp_id_for_partial_charges != "bad match") {
	    for (unsigned int id=0; id<dict_res_restraints.size(); id++) {
	       if (dict_res_restraints[id].residue_info.comp_id == comp_id_for_partial_charges) {
		  dict_res_restraints[id].set_has_partial_charges(1);
	       } 
	    }
	 }
      }
   }

   // Now we need to check that the atom ideal or model coordinates were not at the origin.
   // 
   if (n_origin_ideal_atoms > 2) // trash all ideal atoms
      delete_atom_positions(comp_id, coot::dict_atom::IDEAL_MODEL_POS);
   if (n_origin_model_atoms > 2) // trash all model/real atoms
      delete_atom_positions(comp_id, coot::dict_atom::REAL_MODEL_POS);
   
   return n_atoms;
}


void
coot::protein_geometry::comp_tree(PCMMCIFLoop mmCIFLoop) {

   std::string comp_id;
   std::string atom_id;
   std::string atom_back;
   std::string atom_forward; 
   std::string connect_type; 
   char *s; 
   
   int ierr;
   int ierr_tot = 0; 

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {

      // reset them for next round, we don't want the to keep the old values if
      // they were not set.
      
      comp_id = "";
      atom_id = "";
      atom_back = "";
      atom_forward = "";
      connect_type = "";
      
      // modify a reference (ierr)
      // 
      s = mmCIFLoop->GetString("comp_id",j,ierr);
      ierr_tot += ierr;
      if (s)
	 comp_id = s; 

      s = mmCIFLoop->GetString("atom_id",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id = s; 

      s = mmCIFLoop->GetString("atom_back",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_back = s; 

      s = mmCIFLoop->GetString("atom_forward",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_forward = s; 

      s = mmCIFLoop->GetString("connect_type",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 connect_type = s;

      if (ierr == 0) {
	 std::string padded_name_atom_id = atom_name_for_tree_4c(comp_id, atom_id);
	 std::string padded_name_atom_back = atom_name_for_tree_4c(comp_id, atom_back);
	 std::string padded_name_atom_forward = atom_name_for_tree_4c(comp_id, atom_forward);
	 mon_lib_add_tree(comp_id, padded_name_atom_id, padded_name_atom_back,
			  padded_name_atom_forward, connect_type); 
      } 
   }
}

// look up atom_id in the atom atom_info (dict_atom vector) of the comp_id restraint
// 
std::string
coot::protein_geometry::atom_name_for_tree_4c(const std::string &comp_id, const std::string &atom_id) const {

   std::string r = atom_id;
   for (int id=(dict_res_restraints.size()-1); id >=0; id--) {
      if (dict_res_restraints[id].residue_info.comp_id == comp_id) {
	 r = dict_res_restraints[id].atom_name_for_tree_4c(atom_id);
	 break;
      }
   }
   return r;
}


int
coot::protein_geometry::comp_bond(PCMMCIFLoop mmCIFLoop) {

   bool verbose_output = 0; // can be passed, perhaps.
   std::string comp_id;
   std::string atom_id_1, atom_id_2;
   std::string type;
   realtype value_dist = -1.0, value_dist_esd = -1.0;

   char *s; 
   int nbond = 0;
   int comp_id_index = -1; // not found initially

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      int ierr;
      int ierr_tot = 0; 
      int ierr_tot_for_ccd = 0;  // CCD comp_bond entries don't have
			         // target geometry (dist and esd) but we
			         // want to be able to read them in
			         // anyway (to get the bond orders for
			         // drawing).

   
      // modify a reference (ierr)
      //

      s = mmCIFLoop->GetString("comp_id",j,ierr);
      ierr_tot += ierr;
      ierr_tot_for_ccd += ierr;
      if (s) { 
	 comp_id = s;
	 for (int id=(dict_res_restraints.size()-1); id >=0; id--) {
	    if (dict_res_restraints[id].residue_info.comp_id == comp_id) {
	       comp_id_index = id;
	       break;
	    }
	 }
      }

      s = mmCIFLoop->GetString("atom_id_1", j, ierr);
      ierr_tot += ierr;
      ierr_tot_for_ccd += ierr;
      if (s) 
	 atom_id_1 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("atom_id_2", j, ierr);
      ierr_tot += ierr;
      ierr_tot_for_ccd += ierr;
      if (s) 
	 atom_id_2 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("type", j, ierr);
      if (s) 
	 type = s;
      // perhaps it was in the dictionary as "value_order"?
      if (ierr) {
	 s = mmCIFLoop->GetString("value_order", j, ierr);
	 if (! ierr) {
	    if (s) { // just in case (should not be needed).
	       std::string ss(s);
	       // convert from Chemical Component Dictionary value_order to
	       // refmac monomer library chem_comp_bond types - 
	       // or FeiDrg output.
	       if (ss == "SING")
		  type = "single";
	       if (ss == "DOUB")
		  type = "double";
	       if (ss == "TRIP")
		  type = "triple";
	       if (ss == "TRIPLE")
		  type = "triple";

	       if (ss == "SINGLE")
		  type = "single";
	       if (ss == "DOUBLE")
		  type = "double";
	       if (ss == "DELOC")
		  type = "deloc";
	       
	       // Chemical Chemical Dictionary also has an aromatic flag, so
	       // we can have bonds that are (for example) "double"
	       // "aromatic" Hmm!  Food for thought.
	       
	       // Metal bonds are "SING" (i.e. CCD doesn't have metal
	       // bonds).
	    }
	 } 
      } 
      ierr_tot += ierr;
      ierr_tot_for_ccd += ierr;

      ierr = mmCIFLoop->GetReal(value_dist, "value_dist", j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetReal(value_dist_esd, "value_dist_esd", j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 mon_lib_add_bond(comp_id, atom_id_1, atom_id_2,
			  type, value_dist, value_dist_esd); 
	 nbond++;
      } else {

	 if (! ierr_tot_for_ccd) {

	    mon_lib_add_bond_no_target_geom(comp_id, atom_id_1, atom_id_2, type);
	    
	 } else {
	    // Hopeless - nothing worked...
	    
	    // std::cout << "DEBUG::  ierr_tot " << ierr_tot << std::endl;
	    if (verbose_output) { 
	       std::cout << "Fail on read " << atom_id_1 << ": :" << atom_id_2 << ": :"
			 << type << ": :" << value_dist << ": :" << value_dist_esd
			 << ":" << std::endl;
	    }
	 }
      } 
   }
   return nbond;
}
void
coot::protein_geometry::comp_angle(PCMMCIFLoop mmCIFLoop) {

   std::string comp_id;
   std::string atom_id_1, atom_id_2, atom_id_3;
   realtype value_angle, value_angle_esd;
   int comp_id_index = -1;

   char *s; 
   int ierr;
   int ierr_tot = 0;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      // modify a reference (ierr)
      //

      s = mmCIFLoop->GetString("comp_id",j,ierr);
      ierr_tot += ierr;
      if (s) { 
	 comp_id = s;
	 for (int id=(dict_res_restraints.size()-1); id >=0; id--) {
	    if (dict_res_restraints[id].residue_info.comp_id == comp_id) {
	       comp_id_index = id;
	       break;
	    }
	 }
      }

      s = mmCIFLoop->GetString("atom_id_1",j,ierr);
      ierr_tot += ierr;
      if (s) {
	 atom_id_1 = get_padded_name(std::string(s), comp_id_index);
      }

      s = mmCIFLoop->GetString("atom_id_2",j,ierr);
      ierr_tot += ierr;
      if (s) { 
	 atom_id_2 = get_padded_name(std::string(s), comp_id_index);
      }

      s = mmCIFLoop->GetString("atom_id_3",j,ierr);
      ierr_tot += ierr;
      if (s) {
	 atom_id_3 = get_padded_name(std::string(s), comp_id_index);
      }

      ierr = mmCIFLoop->GetReal(value_angle, "value_angle",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetReal(value_angle_esd, "value_angle_esd",j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 mon_lib_add_angle(comp_id, atom_id_1, atom_id_2, atom_id_3,
			   value_angle, value_angle_esd);
// 	 std::cout << "added angle " << comp_id << " "
// 		   << atom_id_1 << " " 
// 		   << atom_id_2 << " " 
// 		   << atom_id_3 << " " 
// 		   << value_angle << " " 
// 		   << value_angle_esd << std::endl;
      }
   }
} 

void
coot::protein_geometry::comp_torsion(PCMMCIFLoop mmCIFLoop) {

   std::string comp_id, id;
   std::string atom_id_1, atom_id_2, atom_id_3, atom_id_4;
   realtype value_angle, value_angle_esd;
   int period;

   char *s; 
   int ierr;
   int ierr_tot = 0;
   int comp_id_index = -1;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      // modify a reference (ierr)
      //

      s = mmCIFLoop->GetString("comp_id",j,ierr);
      ierr_tot += ierr;
      if (s) { 
	 comp_id = s;
	 for (int id=(dict_res_restraints.size()-1); id >=0; id--) {
	    if (dict_res_restraints[id].residue_info.comp_id == comp_id) {
	       comp_id_index = id;
	       break;
	    }
	 }
      }

      s = mmCIFLoop->GetString("id",j,ierr);
      ierr_tot += ierr;
      if (s)
	 id = s; 

      s = mmCIFLoop->GetString("atom_id_1",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_1 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("atom_id_2",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_2 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("atom_id_3",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_3 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("atom_id_4",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_4 = get_padded_name(s, comp_id_index);

      ierr = mmCIFLoop->GetReal(value_angle, "value_angle",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetReal(value_angle_esd, "value_angle_esd",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(period, "period",j);
      ierr_tot += ierr;

      if (ierr_tot == 0) { 
	 // we don't want to add restraints for non-variable (CONST) torsion
	 // angles (e.g. in Roberto's DAC).
	 // So, reject if comp_id starts with "CONST" or "const".

	 
// 	 short int add_it = 0;
// 	 if (id.length() > 5) {
// 	    std::string bit = id.substr(0, 5);
// 	    if ((bit != "CONST" && bit != "const")) {
// 	       add_it = 1;
// 	    }
// 	 } else {
// 	    add_it = 1;
// 	 }
	 

// 	 if (add_it == 0)
// 	    std::cout << "DEBUG:: rejecting torsion " << comp_id << " " << atom_id_1 << " " << atom_id_2
// 		      << " " << atom_id_3 << " " << atom_id_4 << " " << value_angle << std::endl;
// 	 else 
// 	    std::cout << "DEBUG:: adding torsion " << id << " " << comp_id << " " << atom_id_1
// 		      << " " << atom_id_2
// 		      << " " << atom_id_3 << " " << atom_id_4 << " " << value_angle << std::endl;
   
	 mon_lib_add_torsion(comp_id, id,
			     atom_id_1, atom_id_2,
			     atom_id_3, atom_id_4,
			     value_angle, value_angle_esd, period); 
      }
   }
} 


std::pair<int, std::vector<std::string> >
coot::protein_geometry::comp_chiral(PCMMCIFLoop mmCIFLoop) {

   std::string comp_id;
   std::string id, atom_id_centre;
   std::string atom_id_1, atom_id_2, atom_id_3;
   std::string volume_sign;
   int n_chiral = 0;
   std::vector<std::string> comp_id_vector;

   char *s; 
   int ierr;
   int ierr_tot = 0;
   int comp_id_index = -1;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      // modify a reference (ierr)
      //

      s = mmCIFLoop->GetString("comp_id",j,ierr);
      ierr_tot += ierr;
      if (s) { 
	 comp_id = s;
	 for (int id=(dict_res_restraints.size()-1); id >=0; id--) {
	    if (dict_res_restraints[id].residue_info.comp_id == comp_id) {
	       comp_id_index = id;
	       break;
	    }
	 }
      }

      s = mmCIFLoop->GetString("id",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 id = s; 

      s = mmCIFLoop->GetString("atom_id_centre",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_centre = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("atom_id_1",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_1 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("atom_id_2",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_2 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("atom_id_3",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_3 = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("volume_sign",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 volume_sign = s;

      if (ierr_tot == 0) { 
	 mon_lib_add_chiral(comp_id, id, atom_id_centre,
			    atom_id_1,
			    atom_id_2,
			    atom_id_3,
			    volume_sign);
	 // add comp_id to comp_id_vector if it is not there already.
	 if (std::find(comp_id_vector.begin(), comp_id_vector.end(), comp_id) ==
	     comp_id_vector.end())
	    comp_id_vector.push_back(comp_id);
	 n_chiral++;
      }
   }

   return std::pair<int, std::vector<std::string> > (n_chiral, comp_id_vector);
}

void
coot::protein_geometry::comp_plane(PCMMCIFLoop mmCIFLoop) {

   std::string comp_id;
   std::string atom_id, plane_id;
   realtype dist_esd; 

   char *s; 
   int ierr;
   int ierr_tot = 0;
   int comp_id_index = -1;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      // modify a reference (ierr)
      //
      s = mmCIFLoop->GetString("comp_id",j,ierr);
      ierr_tot += ierr;
      if (s) { 
	 comp_id = s;
	 for (int id=(dict_res_restraints.size()-1); id >=0; id--) {
	    if (dict_res_restraints[id].residue_info.comp_id == comp_id) {
	       comp_id_index = id;
	       break;
	    }
	 }
      }

      s = mmCIFLoop->GetString("atom_id",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id = get_padded_name(s, comp_id_index);

      s = mmCIFLoop->GetString("plane_id",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 plane_id = s;

      ierr = mmCIFLoop->GetReal(dist_esd, "dist_esd",j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 mon_lib_add_plane(comp_id, plane_id, atom_id, dist_esd);
      } else {
	 std::cout << "problem reading comp plane" << std::endl;
      } 
   }
} 
// 
int
coot::protein_geometry::add_chem_mods(PCMMCIFData data) {

   int n_mods = 0;
   for (int icat=0; icat<data->GetNumberOfCategories(); icat++) { 
      
      PCMMCIFCategory cat = data->GetCategory(icat);
      std::string cat_name(cat->GetCategoryName());
      PCMMCIFLoop mmCIFLoop = data->GetLoop(cat_name.c_str() );
            
      if (mmCIFLoop == NULL) { 
	 std::cout << "null loop" << std::endl; 
      } else {
	 int n_chiral = 0;
	 if (cat_name == "_chem_mod")
	    n_mods += add_chem_mod(mmCIFLoop);
      }
   }
   return n_mods;
}


// 
void
coot::protein_geometry::add_synonyms(PCMMCIFData data) {

   for (int icat=0; icat<data->GetNumberOfCategories(); icat++) { 
      
      PCMMCIFCategory cat = data->GetCategory(icat);
      std::string cat_name(cat->GetCategoryName());
      PCMMCIFLoop mmCIFLoop = data->GetLoop(cat_name.c_str() );
            
      if (mmCIFLoop == NULL) { 
	 std::cout << "null loop" << std::endl; 
      } else {
	 int n_chiral = 0;
	 if (cat_name == "_chem_comp_synonym") {
	    add_chem_comp_synonym(mmCIFLoop);
	 }
      }
   }
}

void 
coot::protein_geometry::add_chem_comp_synonym(PCMMCIFLoop mmCIFLoop) {

   int ierr = 0;
   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {

      int ierr_tot = 0;
      std::string comp_id;
      std::string comp_alternative_id;
      std::string mod_id;

      char *s = mmCIFLoop->GetString("comp_id", j, ierr);
      ierr_tot += ierr;
      if (s) comp_id = s;

      s = mmCIFLoop->GetString("comp_alternative_id", j, ierr);
      ierr_tot += ierr;
      if (s) comp_alternative_id = s;

      s = mmCIFLoop->GetString("mod_id", j, ierr);
      ierr_tot += ierr;
      if (s) mod_id = s;

      if (ierr_tot == 0) {
	 coot::protein_geometry::residue_name_synonym rns(comp_id,
							  comp_alternative_id,
							  mod_id);
	 residue_name_synonyms.push_back(rns);
      } 
   }
} 




int 
coot::protein_geometry::add_chem_mod(PCMMCIFLoop mmCIFLoop) {

   int n_chem_mods = 0;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {

      int ierr_tot = 0;
      int ierr;
      
      std::string id;
      std::string name;
      std::string comp_id; // often "." (default)
      std::string group_id;
      char *s;

      s = mmCIFLoop->GetString("id", j, ierr);
      ierr_tot += ierr;
      if (s) id = s;
      
      s = mmCIFLoop->GetString("name", j, ierr);
      ierr_tot += ierr;
      if (s) name = s;
      
      s = mmCIFLoop->GetString("comp_id", j, ierr);
      ierr_tot += ierr;
      if (s) comp_id = s;
      
      s = mmCIFLoop->GetString("group_id", j, ierr);
      ierr_tot += ierr;
      if (s) group_id = s;

      if (ierr_tot == 0) {
	 coot::list_chem_mod mod(id, name, comp_id, group_id);
	 chem_mod_vec.push_back(mod);
	 n_chem_mods++;
      } 
   }
   return n_chem_mods;
}


// Normally, we would use a const pointer (or a reference). But this
// is mmdb.
//
// return then number of links read (to pass on to
// init_refmac_mon_lib) so it doesn't return 0 (ie. fail) when reading
// links (no new atoms in a link, you see).
// 
int
coot::protein_geometry::init_links(PCMMCIFData data) {

   int r = 0; 
   for (int icat=0; icat<data->GetNumberOfCategories(); icat++) { 
      
      PCMMCIFCategory cat = data->GetCategory(icat);
      std::string cat_name(cat->GetCategoryName());

      // std::cout << "DEBUG:: init_link is handling " << cat_name << std::endl;

      PCMMCIFLoop mmCIFLoop = data->GetLoop(cat_name.c_str() );
            
      if (mmCIFLoop == NULL) { 
	 std::cout << "null loop" << std::endl; 
      } else {
	 int n_chiral = 0;
	 if (cat_name == "_chem_link")
	    add_chem_links(mmCIFLoop);
	 if (cat_name == "_chem_link_bond")
	    r += link_bond(mmCIFLoop);
	 if (cat_name == "_chem_link_angle")
	    link_angle(mmCIFLoop);
	 if (cat_name == "_chem_link_tor")
	    link_torsion(mmCIFLoop);
	 if (cat_name == "_chem_link_plane")
	    link_plane(mmCIFLoop);
	 if (cat_name == "_chem_link_chiral")
	    n_chiral = link_chiral(mmCIFLoop);
	 if (n_chiral) {
	    assign_link_chiral_volume_targets();
	 }
      }
   }
   return r;
} 


// References to the modifications
// to the link groups (the modifications
// themselves are in data_mod_list).
void
coot::protein_geometry::add_chem_links(PCMMCIFLoop mmCIFLoop) {


   char *s;
   int ierr;
   int ierr_tot = 0;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {
      std::string chem_link_id;
      std::string chem_link_comp_id_1;
      std::string chem_link_mod_id_1;
      std::string chem_link_group_comp_1;
      std::string chem_link_comp_id_2;
      std::string chem_link_mod_id_2;
      std::string chem_link_group_comp_2;
      std::string chem_link_name;
      
      s = mmCIFLoop->GetString("id", j, ierr);
      ierr_tot += ierr;
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"id\"" << std::endl;
      if (s) chem_link_id = s;

      s = mmCIFLoop->GetString("comp_id_1", j, ierr);
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"comp_id_1\"" << std::endl;
      ierr_tot += ierr;
      if (s) chem_link_comp_id_1 = s;

      s = mmCIFLoop->GetString("mod_id_1", j, ierr);
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"mod_id_1\"" << std::endl;
      ierr_tot += ierr;
      if (s) chem_link_mod_id_1 = s;

      s = mmCIFLoop->GetString("group_comp_1", j, ierr);
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"group_comp_1\"" << std::endl;
      ierr_tot += ierr;
      if (s) chem_link_group_comp_1 = s;

      s = mmCIFLoop->GetString("comp_id_2", j, ierr);
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"comp_id_2\"" << std::endl;
      ierr_tot += ierr;
      if (s) chem_link_comp_id_2 = s;

      s = mmCIFLoop->GetString("mod_id_2", j, ierr);
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"mod_id_2\"" << std::endl;
      ierr_tot += ierr;
      if (s) chem_link_mod_id_2 = s;

      s = mmCIFLoop->GetString("group_comp_2", j, ierr);
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"group_comp_2\"" << std::endl;
      ierr_tot += ierr;
      if (s) chem_link_group_comp_2 = s;

      s = mmCIFLoop->GetString("name", j, ierr);
      if (ierr)
	 std::cout << "WARNING add_chem_links error getting field \"name\"" << std::endl;
      ierr_tot += ierr;
      if (s) chem_link_name = s;

      if (ierr_tot == 0) {
	 coot::chem_link clink(chem_link_id,
			       chem_link_comp_id_1, chem_link_mod_id_1, chem_link_group_comp_1,
			       chem_link_comp_id_2, chem_link_mod_id_2, chem_link_group_comp_2,
			       chem_link_name);
	 // std::cout << "Adding to chem_link_vec: " << clink << std::endl;
	 chem_link_vec.push_back(clink);
      } else {
	 std::cout << "WARNING:: an error occurred when trying to add link: "
		   << "\"" << chem_link_id << "\" "
		   << "\"" << chem_link_comp_id_1 << "\" "
		   << "\"" << chem_link_mod_id_1 << "\" "
		   << "\"" << chem_link_group_comp_1 << "\" "
		   << "\"" << chem_link_comp_id_2 << "\" "
		   << "\"" << chem_link_mod_id_2 << "\" "
		   << "\"" << chem_link_group_comp_2 << "\" "
		   << "\"" << chem_link_name << "\" "
		   << std::endl;
      }
   }
}

int
coot::protein_geometry::link_bond(PCMMCIFLoop mmCIFLoop) {
   std::string link_id;
   std::string atom_id_1, atom_id_2;
   std::string type;
   realtype value_dist, value_dist_esd;
   int atom_1_comp_id, atom_2_comp_id;

   char *s;
   int ierr;
   int ierr_tot = 0;
   int n_link_bonds = 0;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 
      s = mmCIFLoop->GetString("link_id",j,ierr);
      ierr_tot = ierr;
      if (s) link_id = s;

      s = mmCIFLoop->GetString("atom_id_1",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_1 = s;

      s = mmCIFLoop->GetString("atom_id_2",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_2 = s;

      ierr = mmCIFLoop->GetInteger(atom_1_comp_id, "atom_1_comp_id",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(atom_2_comp_id, "atom_2_comp_id",j);
      ierr_tot += ierr;
   
      ierr = mmCIFLoop->GetReal(value_dist, "value_dist",j);
      ierr_tot += ierr; 
      
      ierr = mmCIFLoop->GetReal(value_dist_esd, "value_dist_esd",j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 link_add_bond(link_id,
		       atom_1_comp_id, atom_2_comp_id,
		       atom_id_1, atom_id_2,
		       value_dist, value_dist_esd);
	 n_link_bonds++;
      } else {
	 std::cout << "problem reading bond mmCIFLoop" << std::endl;
      } 
   }
   return n_link_bonds;
}

void
coot::protein_geometry::link_angle(PCMMCIFLoop mmCIFLoop) {

   std::string link_id;
   std::string atom_id_1, atom_id_2, atom_id_3;
   std::string type;
   realtype value_angle, value_angle_esd;
   int atom_1_comp_id, atom_2_comp_id, atom_3_comp_id;

   char *s;
   int ierr;
   int ierr_tot = 0;

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 
      s = mmCIFLoop->GetString("link_id",j,ierr);
      ierr_tot = ierr;
      if (s) link_id = s;

      s = mmCIFLoop->GetString("atom_id_1",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_1 = s;

      s = mmCIFLoop->GetString("atom_id_2",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_2 = s;

      s = mmCIFLoop->GetString("atom_id_3",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_3 = s;

      ierr = mmCIFLoop->GetInteger(atom_1_comp_id, "atom_1_comp_id",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(atom_2_comp_id, "atom_2_comp_id",j);
      ierr_tot += ierr;
   
      ierr = mmCIFLoop->GetInteger(atom_3_comp_id, "atom_3_comp_id",j);
      ierr_tot += ierr;
   
      ierr = mmCIFLoop->GetReal(value_angle, "value_angle",j);
      ierr_tot += ierr;
      
      ierr = mmCIFLoop->GetReal(value_angle_esd, "value_angle_esd",j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 link_add_angle(link_id,
		       atom_1_comp_id, atom_2_comp_id, atom_3_comp_id,
		       atom_id_1, atom_id_2, atom_id_3,
		       value_angle, value_angle_esd);
      } else {
	 std::cout << "problem reading link angle mmCIFLoop" << std::endl;
      } 
   }
}


void
coot::protein_geometry::link_torsion(PCMMCIFLoop mmCIFLoop) {

   std::string link_id;
   std::string  atom_id_1, atom_id_2, atom_id_3, atom_id_4;
   realtype value_angle, value_angle_esd;
   int atom_1_comp_id, atom_2_comp_id, atom_3_comp_id, atom_4_comp_id;
   char *s;
   int ierr;
   int ierr_tot = 0;
   int period;
   std::string id("unknown"); // gets get to phi psi, etc

   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {

      s = mmCIFLoop->GetString("link_id",j,ierr);
      ierr_tot = ierr;
      if (s) link_id = s;
      
      id = "unknown"; // no need to error if "id" was not given
      s = mmCIFLoop->GetString("id",j,ierr);
      if (s) id = s; 
      
      s = mmCIFLoop->GetString("atom_id_1",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_1 = s;

      s = mmCIFLoop->GetString("atom_id_2",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_2 = s;

      s = mmCIFLoop->GetString("atom_id_3",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_3 = s;

      s = mmCIFLoop->GetString("atom_id_4",j,ierr);
      ierr_tot += ierr;
      if (s) atom_id_4 = s;

      ierr = mmCIFLoop->GetInteger(atom_1_comp_id, "atom_1_comp_id",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(atom_2_comp_id, "atom_2_comp_id",j);
      ierr_tot += ierr;
   
      ierr = mmCIFLoop->GetInteger(atom_3_comp_id, "atom_3_comp_id",j);
      ierr_tot += ierr;
   
      ierr = mmCIFLoop->GetInteger(atom_4_comp_id, "atom_4_comp_id",j);
      ierr_tot += ierr;
   
      ierr = mmCIFLoop->GetReal(value_angle, "value_angle",j);
      ierr_tot += ierr;
      
      ierr = mmCIFLoop->GetReal(value_angle_esd, "value_angle_esd",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(period, "period",j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 link_add_torsion(link_id,
			  atom_1_comp_id, atom_2_comp_id, atom_3_comp_id, atom_4_comp_id,
			  atom_id_1, atom_id_2, atom_id_3, atom_id_4,
			  value_angle, value_angle_esd, period, id);
      } else {
	 std::cout << "problem reading link torsion mmCIFLoop" << std::endl;
      } 
   }
}

int 
coot::protein_geometry::link_chiral(PCMMCIFLoop mmCIFLoop) {

   int n_chiral = 0;
   std::string chiral_id;
   std::string atom_id_c, atom_id_1, atom_id_2, atom_id_3;
   int atom_c_comp_id, atom_1_comp_id, atom_2_comp_id, atom_3_comp_id;
   int volume_sign;

   char *s; 
   int ierr;
   int ierr_tot = 0;
   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      // modify a reference (ierr)
      //
      s = mmCIFLoop->GetString("chiral_id",j,ierr);
      ierr_tot += ierr;
      if (s)
	 chiral_id = s; 

      ierr = mmCIFLoop->GetInteger(volume_sign, "volume_sign", j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(atom_c_comp_id, "atom_centre_comp_id",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(atom_1_comp_id, "atom_1_comp_id",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(atom_2_comp_id, "atom_2_comp_id",j);
      ierr_tot += ierr;

      ierr = mmCIFLoop->GetInteger(atom_3_comp_id, "atom_3_comp_id",j);
      ierr_tot += ierr;

      s = mmCIFLoop->GetString("atom_id_centre",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_c = s;
      
      s = mmCIFLoop->GetString("atom_id_1",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id_1 = s;

      if (ierr_tot == 0) {
	 link_add_chiral(chiral_id,
			 atom_c_comp_id,
			 atom_1_comp_id, atom_2_comp_id, atom_3_comp_id,
			 atom_id_c, atom_id_1, atom_id_2, atom_id_3,
			 volume_sign);
	 n_chiral++;
      } else {
	 std::cout << "problem reading link torsion mmCIFLoop" << std::endl;
      } 
   }
   return n_chiral;
}

void
coot::protein_geometry::link_plane(PCMMCIFLoop mmCIFLoop) {

   std::string link_id;
   std::string atom_id, plane_id;
   realtype dist_esd;
   int atom_comp_id;

   char *s; 
   int ierr;
   int ierr_tot = 0;
   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) { 

      // modify a reference (ierr)
      //
      s = mmCIFLoop->GetString("link_id",j,ierr);
      ierr_tot += ierr;
      if (s)
	 link_id = s; 

      s = mmCIFLoop->GetString("atom_id",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 atom_id = s; 

      ierr = mmCIFLoop->GetInteger(atom_comp_id, "atom_comp_id",j);
      ierr_tot += ierr;

      s = mmCIFLoop->GetString("plane_id",j,ierr);
      ierr_tot += ierr;
      if (s) 
	 plane_id = s;

      ierr = mmCIFLoop->GetReal(dist_esd, "dist_esd",j);
      ierr_tot += ierr;

      if (ierr_tot == 0) {
	 link_add_plane(link_id, atom_id, plane_id, atom_comp_id, dist_esd); 
      } else {
	 std::cout << "problem reading link plane mmCIFLoop" << std::endl;
      }
   }
}

void
coot::protein_geometry::link_add_bond(const std::string &link_id,
				      int atom_1_comp_id,
				      int atom_2_comp_id,
				      const std::string &atom_id_1,
				      const std::string &atom_id_2,
				      realtype value_dist,
				      realtype value_dist_esd) {

   dict_link_bond_restraint_t lbr(atom_1_comp_id,
				  atom_2_comp_id,
				  atom_id_1,
				  atom_id_2,
				  value_dist,
				  value_dist_esd);

//    std::cout << "attempting to add link bond "
// 	     << link_id << " "
// 	     << atom_1_comp_id << " "
// 	     << atom_1_comp_id << " "
// 	     << value_dist << " "
// 	     << value_dist_esd << " dict_link_res_restraints size = "
// 	     << dict_link_res_restraints.size() << std::endl;
   
   short int ifound = 0;
   for (unsigned int i=0; i<dict_link_res_restraints.size(); i++) {
      if (dict_link_res_restraints[i].link_id == link_id) {
	 ifound = 1;
	 dict_link_res_restraints[i].link_bond_restraint.push_back(lbr);
      }
   }

   if (! ifound) {
      dict_link_res_restraints.push_back(dictionary_residue_link_restraints_t(link_id));
      dict_link_res_restraints[dict_link_res_restraints.size()-1].link_bond_restraint.push_back(lbr);
   }
}

void
coot::protein_geometry::link_add_angle(const std::string &link_id,
				       int atom_1_comp_id,
				       int atom_2_comp_id,
				       int atom_3_comp_id,
				       const std::string &atom_id_1,
				       const std::string &atom_id_2,
				       const std::string &atom_id_3,
				       realtype value_dist,
				       realtype value_dist_esd) {
   
   dict_link_angle_restraint_t lar(atom_1_comp_id,
				   atom_2_comp_id,
				   atom_3_comp_id,
				   atom_id_1,
				   atom_id_2,
				   atom_id_3,
				   value_dist,
				   value_dist_esd);

   short int ifound = 0;
   for (unsigned int i=0; i<dict_link_res_restraints.size(); i++) {
      if (dict_link_res_restraints[i].link_id == link_id) {
	 ifound = 1;
	 dict_link_res_restraints[i].link_angle_restraint.push_back(lar);
      }
   }

   if (! ifound) {
      dict_link_res_restraints.push_back(dictionary_residue_link_restraints_t(link_id));
      dict_link_res_restraints[dict_link_res_restraints.size()-1].link_angle_restraint.push_back(lar);
   }
}

void
coot::protein_geometry::link_add_torsion(const std::string &link_id,
					 int atom_1_comp_id,
					 int atom_2_comp_id,
					 int atom_3_comp_id,
					 int atom_4_comp_id,
					 const std::string &atom_id_1,
					 const std::string &atom_id_2,
					 const std::string &atom_id_3,
					 const std::string &atom_id_4,
					 realtype value_dist,
					 realtype value_dist_esd,
					 int period,
					 const std::string &id) {  // e.g. phi, psi, omega

   dict_link_torsion_restraint_t ltr(atom_1_comp_id,
				     atom_2_comp_id,
				     atom_3_comp_id,
				     atom_4_comp_id,
				     atom_id_1,
				     atom_id_2,
				     atom_id_3,
				     atom_id_4,
				     value_dist,
				     value_dist_esd,
				     period,
				     id);

   short int ifound = 0;
   for (unsigned int i=0; i<dict_link_res_restraints.size(); i++) {
      if (dict_link_res_restraints[i].link_id == link_id) {
	 ifound = 1;
	 dict_link_res_restraints[i].link_torsion_restraint.push_back(ltr);
      }
   }

   if (! ifound) {
      dict_link_res_restraints.push_back(dictionary_residue_link_restraints_t(link_id));
      dict_link_res_restraints[dict_link_res_restraints.size()-1].link_torsion_restraint.push_back(ltr);
   }
}

void
coot::protein_geometry::link_add_chiral(const std::string &link_id,
					int atom_c_comp_id,
					int atom_1_comp_id,
					int atom_2_comp_id,
					int atom_3_comp_id,
					const std::string &atom_id_c,
					const std::string &atom_id_1,
					const std::string &atom_id_2,
					const std::string &atom_id_3,
					int volume_sign) {

}

void
coot::protein_geometry::link_add_plane(const std::string &link_id,
				       const std::string &atom_id,
				       const std::string &plane_id,
				       int atom_comp_id,
				       double dist_esd) {


   dict_link_plane_restraint_t lpr(atom_id,plane_id, atom_comp_id, dist_esd);
   short int ifound = 0;
   for (unsigned int i=0; i<dict_link_res_restraints.size(); i++) {
      if (dict_link_res_restraints[i].link_id == link_id) { // e.g "TRANS"
	 for (unsigned int ip=0; ip<dict_link_res_restraints[i].link_plane_restraint.size(); ip++) {
	    if (dict_link_res_restraints[i].link_plane_restraint[ip].plane_id == plane_id) {
	       ifound = 1;

	       // now check the atom_id and comp_id of that atom are not already in this restraint.
	       // Only add this atom to the plane restraint if there is not already an atom there.
	       // If there is, then replace the atom in the restraint.
	       //
	       
	       int found_atom_index = coot::protein_geometry::UNSET_NUMBER;

	       for (int i_rest_at=0; i_rest_at<dict_link_res_restraints[i].link_plane_restraint[ip].n_atoms(); i_rest_at++) {
		  if (dict_link_res_restraints[i].link_plane_restraint[ip].atom_ids[i_rest_at] == atom_id) { 
		     if (dict_link_res_restraints[i].link_plane_restraint[ip].atom_comp_ids[i_rest_at] == atom_comp_id) {
			found_atom_index = i_rest_at;
			break;
		     }
		  } 
	       }

	       if (found_atom_index != coot::protein_geometry::UNSET_NUMBER) {
		  // replace it then
		  dict_link_res_restraints[i].link_plane_restraint[ip].atom_ids[found_atom_index] = atom_id;		  
		  dict_link_res_restraints[i].link_plane_restraint[ip].atom_comp_ids[found_atom_index] = atom_comp_id;
	       } else {

		  dict_link_res_restraints[i].link_plane_restraint[ip].atom_ids.push_back(atom_id);
		  dict_link_res_restraints[i].link_plane_restraint[ip].atom_comp_ids.push_back(atom_comp_id);
	       }
	       
	       break;
	    }
	 } 
	 if (! ifound) {
	    // we have link_id, but no planes of that id
	    coot::dict_link_plane_restraint_t res(atom_id, plane_id, atom_comp_id, dist_esd);
	    dict_link_res_restraints[i].link_plane_restraint.push_back(res);
	    ifound = 1;
	 }
      }
   }
   
   // It was not there.  This should only happen when plane restraints
   // are declared in the dictionary *before* the bonds (or angles).
   // Which is never, thanks to Alexei.
   //
   // So, there was no comp_id found 
   if (! ifound) {
      // add the plae to the newly created dictionary_residue_link_restraints_t
      dict_link_res_restraints.push_back(dictionary_residue_link_restraints_t(link_id));
      coot::dict_link_plane_restraint_t res(atom_id, plane_id, atom_comp_id, dist_esd);
      dict_link_res_restraints[dict_link_res_restraints.size()-1].link_plane_restraint.push_back(res);
   }
}

// throw an error on no such chem_link
// 
std::vector<std::pair<coot::chem_link, bool> >
coot::protein_geometry::matching_chem_link(const std::string &comp_id_1,
					   const std::string &group_1,
					   const std::string &comp_id_2,
					   const std::string &group_2) const {
   return matching_chem_link(comp_id_1, group_1, comp_id_2, group_2, 1);
}

// throw an error on no chem links at all. (20100420, not sure why an
// exception is thrown, why not just return an empty vector?)
//
// Perhaps the throwing of the exception is a hang-over from when this
// function returned a pair, not a vector of chem_link (and
// assocciated order_switch_flags).
// 
std::vector<std::pair<coot::chem_link, bool> > 
coot::protein_geometry::matching_chem_link(const std::string &comp_id_1,
					   const std::string &group_1,
					   const std::string &comp_id_2,
					   const std::string &group_2,
					   bool allow_peptide_link_flag) const {

   bool switch_order_flag = 0;
   bool found = false;

   bool debug = false;
   
//    if (debug) { 
//       std::cout << "---------------------- Here are the chem_links: -----------------"
// 		<< std::endl;
//       print_chem_links();
//    }
   
   // Is this link a TRANS peptide or a CIS?  Both have same group and
   // comp_ids.  Similarly, is is BETA-1-2 or BETA1-4 (etc).  We need
   // to decide later, don't just pick the first one that matches
   // (keep the order switch flag too).
   // 
   std::vector<std::pair<coot::chem_link, bool> > matching_chem_links;
   for (unsigned int i_chem_link=0; i_chem_link<chem_link_vec.size(); i_chem_link++) {
      std::pair<bool, bool> match_res =
	 chem_link_vec[i_chem_link].matches_comp_ids_and_groups(comp_id_1, group_1,
								comp_id_2, group_2);
      if (match_res.first) {

	 if (debug)
	    std::cout << "... matching link " << comp_id_1 << " " << comp_id_2 << " " 
		      << chem_link_vec[i_chem_link] << std::endl;
	 
	 // make sure that this link id is not a (currently) useless one.
	 if (chem_link_vec[i_chem_link].Id() != "gap" &&
	     chem_link_vec[i_chem_link].Id() != "symmetry") { 
	    coot::chem_link clt = chem_link_vec[i_chem_link];
	    if (!clt.is_peptide_link_p() || allow_peptide_link_flag) {
	       switch_order_flag = match_res.second;
	       found = 1;
	       std::pair<coot::chem_link, bool> p(clt, switch_order_flag);
	       matching_chem_links.push_back(p);

	       // no! We want all of them - not just the first glycosidic bond that matches
	       // i.e. don't return just BETA1-2 when we have a BETA1-4.
	       // break; // we only want to find one chem link for this comp_id pair.
	       
	    } else {
	       if (debug)
		  std::cout << "reject link on peptide/allow-peptide test " << std::endl;
	    } 
	 } else {
	    if (debug) { 
	       std::cout << "reject link \"" << chem_link_vec[i_chem_link].Id() << "\""
			 << std::endl;
	    } 
	 } 
      }
   }

   // When allow_peptide_link_flag is FALSE, we don't want to hear
   // about not making a link between ASP and VAL etc (otherwise we
   // do).
   // 
   if ( (!found) && (allow_peptide_link_flag)) {
      std::string rte = "INFO:: No chem link for groups \"";
      rte += group_1;
      rte += "\" \"";
      rte += group_2;
      rte += "\" and comp_ids \"";
      rte += comp_id_1;
      rte += "\" \"";
      rte += comp_id_2;
      rte += "\"";
      throw std::runtime_error(rte);
   }
   return matching_chem_links;
}

// throw an error on no such chem_link
// 
std::vector<std::pair<coot::chem_link, bool> >
coot::protein_geometry::matching_chem_link_non_peptide(const std::string &comp_id_1,
						       const std::string &group_1,
						       const std::string &comp_id_2,
						       const std::string &group_2) const {
   return matching_chem_link(comp_id_1, group_1, comp_id_2, group_2, 0);
}
int 
coot::protein_geometry::init_standard() {

   // So, first we check if COOT_REFMAC_LIB_DIR has been set.  If it
   // try to use it.  If the directory fails to exist, try next...
   //
   // Next, try CLIBD_MON (which is e.g." ~/ccp4/ccp4-6.0/lib/data/monomers")
   //
   // Next, try to use the CCP4 lib/data dir.
   //
   // Then we check if the files are where should be given a proper
   // install (PKGDATADIR).  If so, use that...  [This is the prefered
   // option].
   //
   
   std::string dir = DATADIR; 
   std::string hardwired_default_place;
   hardwired_default_place = util::append_dir_dir(dir, "coot");
   hardwired_default_place = util::append_dir_dir(hardwired_default_place, "lib");
   bool using_clibd_mon = 0; 

   std::string mon_lib_dir; 
   short int env_dir_fails = 0;
   int istat;
   struct stat buf;
   char *cmld = NULL;
   
   char *s = getenv("COOT_REFMAC_LIB_DIR");
   if (s) {
      istat = stat(s, &buf);
      if (istat) { 
	 env_dir_fails = 1;
	 std::cout << "WARNING:: Coot REFMAC dictionary override COOT_REFMAC_LIB_DIR"
		   << "failed to find a dictionary " << s << std::endl;
      } else {
	 mon_lib_dir = s;
      }
   }

   if (!s || env_dir_fails) {
      cmld = getenv("COOT_MONOMER_LIB_DIR"); // for phenix.
      // we find $COOT_MONOMER_LIB_DIR/a/ALA.cif
      if (cmld) {
	 mon_lib_dir = cmld;
      }
   } 
      
   if (!s || env_dir_fails) {

      // OK, so COOT_REFMAC_LIB_DIR didn't provide a library.

      // Next try CLIBD_MON:
      s = getenv("CLIBD_MON");
      if (s) {
	 istat = stat(s, &buf);
	 if (istat) { 
	    env_dir_fails = 1;
	 } else {
	    env_dir_fails = 0;
	    std::cout << "INFO:: Using Standard CCP4 Refmac dictionary from"
		      << " CLIBD_MON: " << s << std::endl;
	    mon_lib_dir = s;
	    using_clibd_mon = 1;
	    // strip any trailing / from mon_lib_dir
	    if (mon_lib_dir.length() > 0) {
	       if (mon_lib_dir.at(mon_lib_dir.length()-1) == '/')
		  mon_lib_dir = mon_lib_dir.substr(0,mon_lib_dir.length()-1);
	    }
	 }
      }

      
      if (!s || env_dir_fails) {
	 // Next, try CCP4_LIB

	 s = getenv("CCP4_LIB");
	 if (s) {
	    std::cout << "INFO:: Using Standard CCP4 Refmac dictionary: "
		      << s << std::endl;
	    mon_lib_dir = s;

	 } else {

	    // OK, CCP4 failed to give us a dictionary, now try the
	    // version that comes with Coot:

	    int istat = stat(hardwired_default_place.c_str(), &buf);
	    if (istat == 0) {
	       mon_lib_dir = hardwired_default_place;
	    } else {

	       // OK, let's look for $COOT_PREFIX/share/coot/lib (as you
	       // would with the binary distros)

	       s = getenv("COOT_PREFIX");
	       if (s) {
		  std::string lib_dir = util::append_dir_dir(s, "share");
		  lib_dir = util::append_dir_dir(lib_dir, "coot");
		  lib_dir = util::append_dir_dir(lib_dir, "lib");
		  istat = stat(lib_dir.c_str(), &buf);
		  if (istat == 0) {
		     mon_lib_dir = lib_dir;
		  } else {
		     std::cout << "WARNING:: COOT_PREFIX set, but no dictionary lib found\n";
		  }
	       } else {
		  std::cout << "WARNING:: COOT_PREFIX not set, all attemps to "
			    << "find dictionary lib failed\n";
	       }
	    }
	 }
      }
   }
   
   if (mon_lib_dir.length() > 0) {
      mon_lib_dir =  coot::util::intelligent_debackslash(mon_lib_dir);
      std::string filename = mon_lib_dir;
      // contains the linkages:
      filename += "/data/monomers/list/mon_lib_list.cif";
      if (using_clibd_mon) {
	 filename = mon_lib_dir;
	 filename += "list/mon_lib_list.cif";
      }
      // now check that that file is there:
      struct stat buf;
      int istat = stat(filename.c_str(), &buf);
      if ((istat == 0) && (! S_ISREG(buf.st_mode))) {
	 std::cout << "ERROR: dictionary " << filename
		   << " is not a regular file" << std::endl;
      } else {
	 // OK 
	 
      }
   } else { 
      std::cout << "WARNING: Failed to read restraints dictionary. "
		<< std::endl; 
   }

   // setting up CCP4 sets mon_lib_cif to
   // $CCP4_MASTER/lib/data/monomers (by using $CLIBD_MON).
   // 
   std::string mon_lib_cif = mon_lib_dir + "/data/monomers/list/mon_lib_list.cif";
   std::string energy_cif_file_name = mon_lib_dir + "/data/monomers/ener_lib.cif";
   if (using_clibd_mon) { 
      mon_lib_cif = mon_lib_dir + "/list/mon_lib_list.cif";
      energy_cif_file_name = mon_lib_dir + "/ener_lib.cif";
   }
   if (cmld) { 
      mon_lib_cif = cmld;
      mon_lib_cif += "/list/mon_lib_list.cif";
      energy_cif_file_name = std::string(cmld) + "/ener_lib.cif";
   }
   
   init_refmac_mon_lib(mon_lib_cif, protein_geometry::MON_LIB_LIST_CIF);
   // now the protein monomers:
   read_number = 1;
   std::vector <std::string> protein_mono = standard_protein_monomer_files();
   for (unsigned int i=0; i<protein_mono.size(); i++) { 
      std::string monomer_cif_file = protein_mono[i];
      if (!cmld && !using_clibd_mon) {
	 monomer_cif_file = "data/monomers/" + monomer_cif_file;
      }
      refmac_monomer(mon_lib_dir, monomer_cif_file); // update read_number too :)
   }


   read_energy_lib(energy_cif_file_name);

   return read_number;
}


int 
coot::protein_geometry::refmac_monomer(const std::string &s, // dir
				       const std::string &protein_mono) { // extra path to file
   
   std::string filename = s; 
   filename = util::append_dir_file(s, protein_mono);
   struct stat buf;
   int istat = stat(filename.c_str(), &buf);
   if (istat == 0) { 
      if (S_ISREG(buf.st_mode)) {
	 init_refmac_mon_lib(filename, read_number);
	 read_number++;
      } else {

#if defined(WINDOWS_MINGW) || defined(_MSC_VER)
	 
	 // indenting goes strange if you factor out the else code here.
	 
	 if (! S_ISDIR(buf.st_mode) ) {
	    std::cout << "WARNING: " << filename 
		      << ": no such file (or directory)\n";
	 } else { 
	    std::cout << "ERROR: file " << filename
		      << " is not a regular file" << std::endl;
	 }
#else
	 if (! S_ISDIR(buf.st_mode) && 
	     ! S_ISLNK(buf.st_mode) ) {
	    std::cout << "WARNING: " << filename 
		      << ": no such file (or directory)\n";
	 } else { 
	    std::cout << "ERROR: file " << filename
		      << " is not a regular file" << std::endl;
	 }
#endif
      }
   }
   return read_number;
}

// quote atom name as needed - i.e. CA -> CA, CA' -> "CA'"
std::string 
coot::dictionary_residue_restraints_t::quoted_atom_name(const std::string &an) const {

   std::string n = an;
   bool has_quotes = false;

   for (unsigned int i=0; i<an.size(); i++) {
      if (an[i] == '\'') {
	 has_quotes = true;
	 break;
      } 
   }
   if (has_quotes)
      n = "\"" + an + "\"";

   return n;
}

void
coot::dictionary_residue_restraints_t::write_cif(const std::string &filename) const {

   PCMMCIFFile mmCIFFile = new CMMCIFFile(); // d
      
   PCMMCIFData   mmCIFData = NULL;
   PCMMCIFStruct mmCIFStruct;
   char S[2000];
   
   //  2.1  Example 1: add a structure into mmCIF object

   int rc;

   rc = mmCIFFile->AddMMCIFData("comp_list");
   mmCIFData = mmCIFFile->GetCIFData("comp_list");
   rc = mmCIFData->AddStructure ("_chem_comp", mmCIFStruct);
   // std::cout << "rc on AddStructure returned " << rc << std::endl;
   if (rc!=CIFRC_Ok && rc!=CIFRC_Created)  {
      // badness!
      std::cout << "rc not CIFRC_Ok " << rc << std::endl;
      printf ( " **** error: attempt to retrieve Loop as a Structure.\n" );
      if (!mmCIFStruct)  {
	 printf ( " **** error: mmCIFStruct is NULL - report as a bug\n" );
      }
   } else {
      if (rc==CIFRC_Created) { 
	 // printf ( " -- new structure created\n" );
      } else { 
	 printf(" -- structure was already in mmCIF, it will be extended\n");
      }
      // std::cout << "SUMMARY:: rc CIFRC_Ok or newly created. " << std::endl;

      PCMMCIFLoop mmCIFLoop = new CMMCIFLoop; // 20100212
      // data_comp_list, id, three_letter_code, name group etc:

      rc = mmCIFData->AddLoop("_chem_comp", mmCIFLoop);
      int i=0;
      const char *s = residue_info.comp_id.c_str();
      mmCIFLoop->PutString(s, "id", i);
      s = residue_info.three_letter_code.c_str();
      mmCIFLoop->PutString(s, "three_letter_code", i);
      std::string raw_name = residue_info.name.c_str();
      std::string quoted_name = util::single_quote(raw_name, "'");
      mmCIFLoop->PutString(quoted_name.c_str(), "name", i);
      s =  residue_info.group.c_str();
      mmCIFLoop->PutString(s, "group", i);
      int nat = residue_info.number_atoms_all;
      mmCIFLoop->PutInteger(nat, "number_atoms_all", i);
      nat = residue_info.number_atoms_nh;
      mmCIFLoop->PutInteger(nat, "number_atoms_nh", i);
      s = residue_info.description_level.c_str();
      mmCIFLoop->PutString(s, "desc_level", i);
      
      std::string comp_record = "comp_list";
      mmCIFData->PutDataName(comp_record.c_str()); // 'data_' record

      // atom loop

      std::string comp_monomer_name = "comp_";
      comp_monomer_name += residue_info.comp_id.c_str(); 
      rc = mmCIFFile->AddMMCIFData(comp_monomer_name.c_str());
      mmCIFData = mmCIFFile->GetCIFData(comp_monomer_name.c_str());

      // shall we add coordinates too?
      //
      bool add_coordinates = false; // if all atoms have coords, add this
      int n_atoms_with_coords = 0;
      for (int i=0; i<atom_info.size(); i++) {
	 if (atom_info[i].model_Cartn.first)
	    n_atoms_with_coords++;
      }
      if (n_atoms_with_coords == atom_info.size())
	 add_coordinates = true;
      
      if (atom_info.size()) { 
	 rc = mmCIFData->AddLoop("_chem_comp_atom", mmCIFLoop);
	 if (rc == CIFRC_Ok || rc == CIFRC_Created) {
	    for (int i=0; i<atom_info.size(); i++) {
	       const dict_atom &ai = atom_info[i];
	       const char *ss =  residue_info.comp_id.c_str();
	       mmCIFLoop->PutString(ss, "comp_id", i);
	       ss = util::remove_whitespace(ai.atom_id).c_str();
	       std::string qan = quoted_atom_name(ss);
	       mmCIFLoop->PutString(qan.c_str(), "atom_id", i);
	       ss = atom_info[i].type_symbol.c_str();
	       mmCIFLoop->PutString(ss, "type_symbol", i);
	       ss = atom_info[i].type_energy.c_str();
	       mmCIFLoop->PutString(ss, "type_energy", i);
	       if (atom_info[i].partial_charge.first) {
		  float v = atom_info[i].partial_charge.second;
		  mmCIFLoop->PutReal(v, "partial_charge", i, 4);
	       }
	       if (add_coordinates) {
		  float x = atom_info[i].model_Cartn.second.x();
		  float y = atom_info[i].model_Cartn.second.y();
		  float z = atom_info[i].model_Cartn.second.z();
		  mmCIFLoop->PutReal(x, "x", i, 6);
		  mmCIFLoop->PutReal(y, "y", i, 6);
		  mmCIFLoop->PutReal(z, "z", i, 6);
	       } 
	    }
	 }
      }

      // bond loop

      if (bond_restraint.size()) { 
	 rc = mmCIFData->AddLoop("_chem_comp_bond", mmCIFLoop);
	 if (rc == CIFRC_Ok || rc == CIFRC_Created) {
	    // std::cout << " number of bonds: " << bond_restraint.size() << std::endl;
	    for (int i=0; i<bond_restraint.size(); i++) {
	       // std::cout << "ading bond number " << i << std::endl;
	       const char *ss = residue_info.comp_id.c_str();
	       mmCIFLoop->PutString(ss, "comp_id", i);
	       std::string id_1 = util::remove_whitespace(bond_restraint[i].atom_id_1_4c());
	       std::string id_2 = util::remove_whitespace(bond_restraint[i].atom_id_2_4c());
	       std::string qan_1 = quoted_atom_name(id_1);
	       std::string qan_2 = quoted_atom_name(id_2);
	       ss = qan_1.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_1", i);
	       ss = qan_2.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_2", i);
	       ss = bond_restraint[i].type().c_str();
	       mmCIFLoop->PutString(ss, "type", i);
	       float v = bond_restraint[i].value_dist();
	       try { 
		  mmCIFLoop->PutReal(v, "value_dist", i);
		  v = bond_restraint[i].value_esd();
		  mmCIFLoop->PutReal(v, "value_dist_esd", i);
	       }
	       catch (std::runtime_error rte) {
		  // do nothing, it's not really an error if the dictionary
		  // doesn't have target geometry (the bonding description came
		  // from a Chemical Component Dictionary entry for example).
	       } 
	    }
	 }
      }

      // angle loop

      if (angle_restraint.size()) { 
	 rc = mmCIFData->AddLoop("_chem_comp_angle", mmCIFLoop);
	 if (rc == CIFRC_Ok || rc == CIFRC_Created) {
	    // std::cout << " number of angles: " << angle_restraint.size() << std::endl;
	    for (int i=0; i<angle_restraint.size(); i++) {
	       // std::cout << "ading angle number " << i << std::endl;
	       std::string id_1 = util::remove_whitespace(angle_restraint[i].atom_id_1_4c());
	       std::string id_2 = util::remove_whitespace(angle_restraint[i].atom_id_2_4c());
	       std::string qan_1 = quoted_atom_name(id_1);
	       std::string qan_2 = quoted_atom_name(id_2);
	       const char *ss = residue_info.comp_id.c_str();
	       mmCIFLoop->PutString(ss, "comp_id", i);
	       ss = qan_1.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_1", i);
	       ss = qan_2.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_2", i);

	       // bug fix(!) intermediate value my_ss clears up casting
	       // problem (inheritance-related?) on writing.
	       std::string my_ss = util::remove_whitespace(angle_restraint[i].atom_id_3_4c());
	       std::string qan_3 = quoted_atom_name(my_ss);
	       ss = qan_3.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_3", i);
	       
	       float v = angle_restraint[i].angle();
	       mmCIFLoop->PutReal(v, "value_angle", i);
	       v = angle_restraint[i].esd();
	       mmCIFLoop->PutReal(v, "value_angle_esd", i);
	    }
	 }
      }

      // torsion loop

      if (torsion_restraint.size() > 0) { 
	 rc = mmCIFData->AddLoop("_chem_comp_tor", mmCIFLoop);
	 if (rc == CIFRC_Ok || rc == CIFRC_Created) {
	    // std::cout << " number of torsions: " << torsion_restraint.size() << std::endl;
	    for (int i=0; i<torsion_restraint.size(); i++) {
	       // std::cout << "ading torsion number " << i << std::endl;
	       std::string id_1 = util::remove_whitespace(torsion_restraint[i].atom_id_1_4c());
	       std::string id_2 = util::remove_whitespace(torsion_restraint[i].atom_id_2_4c());
	       std::string id_3 = util::remove_whitespace(torsion_restraint[i].atom_id_3_4c());
	       std::string id_4 = util::remove_whitespace(torsion_restraint[i].atom_id_4_4c());
	       std::string qan_1 = quoted_atom_name(id_1);
	       std::string qan_2 = quoted_atom_name(id_2);
	       std::string qan_3 = quoted_atom_name(id_3);
	       std::string qan_4 = quoted_atom_name(id_4);
	       const char *ss = residue_info.comp_id.c_str();
	       mmCIFLoop->PutString(ss, "comp_id", i);
	       ss = torsion_restraint[i].id().c_str();
	       mmCIFLoop->PutString(ss, "id", i);
	       ss = qan_1.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_1", i);
	       ss = qan_2.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_2", i);
	       ss = qan_3.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_3", i);
	       ss = qan_4.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_4", i);
	       float v = torsion_restraint[i].angle();
	       mmCIFLoop->PutReal(v, "value_angle", i);
	       v = torsion_restraint[i].esd();
	       mmCIFLoop->PutReal(v, "value_angle_esd", i);
	       int p = torsion_restraint[i].periodicity();
	       mmCIFLoop->PutInteger(p, "period", i);
	    }
	 }
      }

      // chiral loop
      // 
      if (chiral_restraint.size() > 0) { 
	 rc = mmCIFData->AddLoop("_chem_comp_chir", mmCIFLoop);
	 if (rc == CIFRC_Ok || rc == CIFRC_Created) {
	    // std::cout << " number of chirals: " << chiral_restraint.size() << std::endl;
	    for (int i=0; i<chiral_restraint.size(); i++) {
	       // std::cout << "ading chiral number " << i << std::endl;
	       const char *ss = residue_info.comp_id.c_str();
	       std::string id_c = util::remove_whitespace(chiral_restraint[i].atom_id_c_4c());
	       std::string id_1 = util::remove_whitespace(chiral_restraint[i].atom_id_1_4c());
	       std::string id_2 = util::remove_whitespace(chiral_restraint[i].atom_id_2_4c());
	       std::string id_3 = util::remove_whitespace(chiral_restraint[i].atom_id_3_4c());
	       std::string qan_c = quoted_atom_name(id_c);
	       std::string qan_1 = quoted_atom_name(id_1);
	       std::string qan_2 = quoted_atom_name(id_2);
	       std::string qan_3 = quoted_atom_name(id_3);
	       mmCIFLoop->PutString(ss, "comp_id", i);
	       ss = chiral_restraint[i].Chiral_Id().c_str();
	       mmCIFLoop->PutString(ss, "id", i);
	       ss = qan_c.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_centre", i);
	       ss = qan_1.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_1", i);
	       ss = qan_2.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_2", i);
	       ss = qan_3.c_str();
	       mmCIFLoop->PutString(ss, "atom_id_3", i);
	       int sign = chiral_restraint[i].volume_sign;
	       ss = "both";
	       if (sign == 1)
		  ss = "positiv";
	       if (sign == -1)
		  ss = "negativ";
	       mmCIFLoop->PutString(ss, "volume_sign", i);
	    }
	 }
      }

      // plane loop
      if (plane_restraint.size() > 0) { 
	 rc = mmCIFData->AddLoop("_chem_comp_plane_atom", mmCIFLoop);
	 if (rc == CIFRC_Ok || rc == CIFRC_Created) {
	    // std::cout << " number of planes: " << plane_restraint.size() << std::endl;
	    int icount = 0;
	    for (int i=0; i<plane_restraint.size(); i++) {
	       // std::cout << "DEBUG:: adding plane number " << i << std::endl;
	       for (int iat=0; iat<plane_restraint[i].n_atoms(); iat++) {
		  const char *ss = residue_info.comp_id.c_str();
		  mmCIFLoop->PutString(ss, "comp_id", icount);
		  ss = plane_restraint[i].plane_id.c_str();
		  mmCIFLoop->PutString(ss, "plane_id", icount);
		  
		  std::string id = util::remove_whitespace(plane_restraint[i].atom_id(iat));
		  std::string qan = quoted_atom_name(id);
		  ss = qan.c_str();
		  mmCIFLoop->PutString(ss, "atom_id", icount);
		  
		  float v = plane_restraint[i].dist_esd(iat);
		  mmCIFLoop->PutReal(v, "dist_esd", icount, 4);
		  icount++;
	       }
	    }
	 }
      }

      // delete mmCIFLoop; // crashed when enabled?
      
      int status = mmCIFFile->WriteMMCIFFile(filename.c_str());
      if (status == 0) 
	 std::cout << "INFO:: wrote mmCIF \"" << filename << "\"" << std::endl;
      else 
	 std::cout << "INFO:: on write mmCIF \"" << filename << "\" status: "
		   << status << std::endl;
   }
   delete mmCIFFile; // deletes all its attributes too.
}



// make a connect file specifying the bonds to Hydrogens
bool
coot::protein_geometry::hydrogens_connect_file(const std::string &resname,
					       const std::string &filename) const {

   bool r = 0;
   std::pair<short int, dictionary_residue_restraints_t> p =
      get_monomer_restraints(resname);

   if (p.first) {
      std::vector<dict_bond_restraint_t> bv = p.second.bond_restraint;
      if (bv.size() > 0) {
	 // try to open the file then:
	 std::ofstream connect_stream(filename.c_str());
	 if (connect_stream) {
	    int n_atoms = p.second.atom_info.size();
	    connect_stream << "# Generated by Coot" << std::endl;
	    connect_stream << "RESIDUE   " << resname << "   " << n_atoms << std::endl;
	    std::vector<std::pair<std::string, std::vector<std::string> > > assoc; 
	    for (unsigned int i=0; i<bv.size(); i++) {
	       std::string atom1 = bv[i].atom_id_1();
	       std::string atom2 = bv[i].atom_id_2();
	       // find atom1
	       bool found = 0;
	       int index_1 = -1;
	       int index_2 = -1;
	       for (unsigned int j=0; j<assoc.size(); j++) {
		  if (atom1 == assoc[j].first) {
		     found = 1;
		     index_1 = j;
		     break;
		  } 
	       }
	       if (found == 1) {
		  assoc[index_1].second.push_back(atom2);
	       } else {
		  // we need to add a new atom:
		  std::vector<std::string> vt;
		  vt.push_back(atom2);
		  std::pair<std::string, std::vector<std::string> > p(atom1, vt);
		  assoc.push_back(p);
	       }
	       // find atom2
	       found = 0;
	       for (unsigned int j=0; j<assoc.size(); j++) {
		  if (atom2 == assoc[j].first) {
		     found = 1;
		     index_2 = j;
		     break;
		  }
	       }
	       if (found == 1) {
		  assoc[index_2].second.push_back(atom1);
	       } else {
		  // we need to add a new atom:
		  std::vector<std::string> vt;
		  vt.push_back(atom1);
		  std::pair<std::string, std::vector<std::string> > p(atom2, vt);
		  assoc.push_back(p);
	       } 
	    }

	    r = 1;
	    // for each atom in assoc
	    for (unsigned int i=0; i<assoc.size(); i++) {
	       
	       connect_stream << "CONECT     " << assoc[i].first << "    "
			      << assoc[i].second.size();
	       for (unsigned int ii=0; ii<assoc[i].second.size(); ii++) {
		  connect_stream << assoc[i].second[ii] << " ";
	       }
	       connect_stream << std::endl;
	    }
	 }
      }
   }
   return r;
} 
      
// constructor
coot::simple_cif_reader::simple_cif_reader(const std::string &cif_dictionary_file_name) {
   
   CMMCIFFile ciffile;
   struct stat buf;
   int istat = stat(cif_dictionary_file_name.c_str(), &buf);
   if (istat != 0) {
      std::cout << "WARNIG:: cif dictionary " << cif_dictionary_file_name
		<< " not found" << std::endl;
   } else {
      int ierr = ciffile.ReadMMCIFFile((char *)cif_dictionary_file_name.c_str());
      if (ierr != CIFRC_Ok) {
	 std::cout << "Dirty mmCIF file? " << cif_dictionary_file_name
		   << std::endl;
      } else {
	 for(int idata=0; idata<ciffile.GetNofData(); idata++) { 
         
	    PCMMCIFData data = ciffile.GetCIFData(idata);
	    for (int icat=0; icat<data->GetNumberOfCategories(); icat++) { 
	       PCMMCIFCategory cat = data->GetCategory(icat);
	       std::string cat_name(cat->GetCategoryName());
	       PCMMCIFLoop mmCIFLoop =
		  data->GetLoop(cat_name.c_str() );
	       if (mmCIFLoop == NULL) { 
		  std::cout << "null loop" << std::endl; 
	       } else {
		  if (cat_name == "_chem_comp") {
		     int ierr = 0;
		     for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {
			char *n = mmCIFLoop->GetString("name", j, ierr);
			char *t = mmCIFLoop->GetString("three_letter_code", j, ierr);
			if (n && t) {
			   names.push_back(n);
			   three_letter_codes.push_back(t);
			}
		     }
		  }
	       }
	    }
	 }
      }
   }
}

// maybe these function need their own file.  For now they can go here.
// 
void
coot::protein_geometry::pdbx_chem_comp_descriptor(PCMMCIFLoop mmCIFLoop) {

   std::string comp_id;
   std::string type;
   std::string program;
   std::string program_version;
   std::string descriptor;
   
   for (int j=0; j<mmCIFLoop->GetLoopLength(); j++) {
      int ierr;
      int ierr_tot = 0;
      char *s;
      s = mmCIFLoop->GetString("comp_id",j,ierr);
      ierr_tot += ierr;
      if (s) comp_id = s;
      s = mmCIFLoop->GetString("program",j,ierr);
      if (s) program = s;
      s = mmCIFLoop->GetString("program_version",j,ierr);
      if (s) program_version = s;
      s = mmCIFLoop->GetString("descriptor",j,ierr);
      ierr_tot += ierr;
      if (s) descriptor = s;
      s = mmCIFLoop->GetString("type",j,ierr);
      ierr_tot += ierr;
      if (s) type = s;
      
      if (ierr_tot == 0) {
	 pdbx_chem_comp_descriptor_item descr(type, program, program_version, descriptor);
	 add_pdbx_descriptor(comp_id, descr);
      }
   }
}



void
coot::protein_geometry::add_pdbx_descriptor(const std::string &comp_id,
					    pdbx_chem_comp_descriptor_item &descr) {

   // like the others, not using iterators because we don't have a
   // comparitor using a string (the comp_id).
   // 
   bool found = false;
   for (unsigned int i=0; i<dict_res_restraints.size(); i++) {
      if (dict_res_restraints[i].residue_info.comp_id == comp_id) {
	 found = true;
	 dict_res_restraints[i].descriptors.descriptors.push_back(descr);
	 break;
      }
   }
   if (! found) {
      dictionary_residue_restraints_t rest(comp_id, read_number);
      rest.descriptors.descriptors.push_back(descr);
      dict_res_restraints.push_back(rest);
   } 
}
