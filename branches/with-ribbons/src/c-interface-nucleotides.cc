/* src/c-interface-nucleotides.cc
 * 
 * Copyright 2008 The University of Oxford
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

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include "mmdb_manager.h"
#include "mmdb-extras.h"

#include "graphics-info.h"

#include "c-interface.h"

#ifdef USE_GUILE
#include <guile/gh.h>
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
// no fix up needed 
#else    
#define scm_to_int gh_scm2int
#define scm_to_locale_string SCM_STRING_CHARS
#define scm_to_double  gh_scm2double
#define scm_is_true gh_scm2bool
#define scm_car SCM_CAR
#endif // SCM version
#endif // USE_GUILE

// Including python needs to come after graphics-info.h, because
// something in Python.h (2.4 - chihiro) is redefining FF1 (in
// ssm_superpose.h) to be 0x00004000 (Grrr).
//
#ifdef USE_PYTHON
#include "Python.h"
#if (PY_MINOR_VERSION > 4) 
// no fixup needed 
#else
#define Py_ssize_t int
#endif
#endif // USE_PYTHON

#include "cc-interface.hh"
#include "ideal-rna.hh"

#ifdef USE_GUILE
SCM pucker_info_scm(int imol, SCM residue_spec_scm, int do_pukka_pucker_check) {

   std::string altconf = "";
   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      coot::residue_spec_t residue_spec = residue_spec_from_scm(residue_spec_scm);
      CResidue *res_p = graphics_info_t::molecules[imol].get_residue(residue_spec);
      if (res_p) {
	 try {
	    coot::pucker_analysis_info_t pi(res_p, altconf);
	    if (do_pukka_pucker_check) {
	       CResidue *following_res =
		  graphics_info_t::molecules[imol].get_following_residue(residue_spec);
	       if (following_res) {
// 		  std::cout << "   DEBUG:: " << coot::residue_spec_t(following_res)
// 			    << " follows " << residue_spec << std::endl;
		  try {
		     double phosphate_d = pi.phosphate_distance(following_res);
		     r = SCM_EOL;
		     r = scm_cons(scm_double2num(pi.plane_distortion), r);
		     r = scm_cons(scm_double2num(pi.out_of_plane_distance), r);
		     r = scm_cons(scm_makfrom0str(pi.puckered_atom().c_str()), r);
		     r = scm_cons(scm_double2num(phosphate_d), r);
		     double dist_crit = 1.2;
		     // If C2', phosphate oop dist should be > dist_crit
		     // If C3', phosphate oop dist should be < dist_crit
		     
		  }
		  catch (std::runtime_error phos_mess) {
		     std::cout << " Failed to find Phosphate for "
			       << coot::residue_spec_t(following_res) << " " 
			       << phos_mess.what() << std::endl;
		  } 
		  
	       } else {
		  r = SCM_EOL;
		  // r = scm_cons(scm_double2num(pi.plane_distortion), r);
		  // r = scm_cons(scm_double2num(pi.out_of_plane_distance), r);
	       } 
	    } else { 
	       r = SCM_EOL;
	       r = scm_cons(scm_double2num(pi.plane_distortion), r);
	       r = scm_cons(scm_double2num(pi.out_of_plane_distance), r);
	       r = scm_cons(scm_makfrom0str(pi.puckered_atom().c_str()), r);
	    }
	 }
	 catch (std::runtime_error mess) {
	    std::cout << " failed to find pucker for " << residue_spec << " " 
		      << mess.what() << std::endl;
	 } 
      } 
   }
   return r;
}
#endif /* USE_GUILE */

#ifdef USE_PYTHON
PyObject *pucker_info_py(int imol, PyObject *residue_spec_py, int do_pukka_pucker_check) {

   std::string altconf = "";
   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      coot::residue_spec_t residue_spec = residue_spec_from_py(residue_spec_py);
      CResidue *res_p = graphics_info_t::molecules[imol].get_residue(residue_spec);
      if (res_p) {
	 try {
	    coot::pucker_analysis_info_t pi(res_p, altconf);
	    if (do_pukka_pucker_check) {
	       CResidue *following_res =
		  graphics_info_t::molecules[imol].get_following_residue(residue_spec);
	       if (following_res) {
// 		  std::cout << "   DEBUG:: " << coot::residue_spec_t(following_res)
// 			    << " follows " << residue_spec << std::endl;
		  try {
		     double phosphate_d = pi.phosphate_distance(following_res);
		     r = PyList_New(0);
		     PyList_Append(r, PyFloat_FromDouble(phosphate_d));
		     PyList_Append(r, PyString_FromString(pi.puckered_atom().c_str()));
		     PyList_Append(r, PyFloat_FromDouble(pi.out_of_plane_distance));
		     PyList_Append(r, PyFloat_FromDouble(pi.plane_distortion));
		     double dist_crit = 1.2;
		     // If C2', phosphate oop dist should be > dist_crit
		     // If C3', phosphate oop dist should be < dist_crit
		     
		  }
		  catch (std::runtime_error phos_mess) {
		     std::cout << " Failed to find Phosphate for "
			       << coot::residue_spec_t(following_res) << " " 
			       << phos_mess.what() << std::endl;
		  } 
		  
	       } else {
		 r = PyList_New(0);
		  // r = scm_cons(scm_double2num(pi.plane_distortion), r);
		  // r = scm_cons(scm_double2num(pi.out_of_plane_distance), r);
	       } 
	    } else { 
	       r = PyList_New(0);
	       PyList_Append(r, PyString_FromString(pi.puckered_atom().c_str()));
	       PyList_Append(r, PyFloat_FromDouble(pi.out_of_plane_distance));
	       PyList_Append(r, PyFloat_FromDouble(pi.plane_distortion));
	    }
	 }
	 catch (std::runtime_error mess) {
	    std::cout << " failed to find pucker for " << residue_spec << " " 
		      << mess.what() << std::endl;
	 } 
      } 
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif /* USE_PYTHON */



/*  \brief create a molecule of idea nucleotides 

use the given sequence (single letter code)

RNA_or_DNA is either "RNA" or "DNA"

form is either "A" or "B"

@return the new molecule number or -1 if a problem */
int ideal_nucleic_acid(const char *RNA_or_DNA, const char *form,
		       short int single_stranded_flag,
		       const char *sequence) {

   int istat = -1; 
   short int do_rna_flag = -1;
   short int form_flag = -1;

   float here_x = graphics_info_t::RotationCentre_x();
   float here_y = graphics_info_t::RotationCentre_y();
   float here_z = graphics_info_t::RotationCentre_z();

   std::string RNA_or_DNA_str(RNA_or_DNA);
   std::string form_str(form);

   if (RNA_or_DNA_str == "RNA")
      do_rna_flag = 1;
   if (RNA_or_DNA_str == "DNA")
      do_rna_flag = 0;

   if (form_str == "A")
      form_flag = 1;
   
   if (form_str == "B")
      form_flag = 1;

   if (! (form_flag > 0)) {
      std::cout << "Problem in nucleic acid form, use only either \"A\" or \"B\"."
		<< std::endl;
   } else {
      if (! (do_rna_flag >= 0)) {
	 std::cout << "Problem in nucleic acid type, use only either \"RNA\" or \"DNA\"."
		   << "You said: \"" << RNA_or_DNA << "\"" << std::endl;
      } else {
	 // proceed, input is good

	 std::string down_sequence(coot::util::downcase(sequence));
	 if (graphics_info_t::standard_residues_asc.read_success) {
	    coot::ideal_rna ir(RNA_or_DNA_str, form_str, single_stranded_flag,
			       down_sequence,
			       graphics_info_t::standard_residues_asc.mol);
	    CMMDBManager *mol = ir.make_molecule();

	    if (mol) { 
	       int imol = graphics_info_t::create_molecule();
	       istat = imol;
	       std::string label = "Ideal-" + form_str;
	       label += "-form-";
	       label += RNA_or_DNA_str;
	       atom_selection_container_t asc = make_asc(mol);
	       graphics_info_t::molecules[imol].install_model(imol, asc, label, 1);
	       graphics_info_t::molecules[imol].translate_by(here_x, here_y, here_z);
	       graphics_draw();
	       if (graphics_info_t::go_to_atom_window) {
		  graphics_info_t g;
		  g.update_go_to_atom_window_on_new_mol();
		  g.update_go_to_atom_window_on_changed_mol(imol);
	       }
	    }
	 } else {
	    std::string s("WARNING:: Can't proceed with Idea RNA - no standard residues!");
	    std::cout << s << std::endl;
	    graphics_info_t g;
	    g.statusbar_text(s);
	 } 
      }
   }
   std::vector<std::string> command_strings;
   command_strings.push_back("ideal-nucleic-acid");
   command_strings.push_back(single_quote(RNA_or_DNA_str));
   command_strings.push_back(single_quote(form_str));
   command_strings.push_back(coot::util::int_to_string(single_stranded_flag));
   command_strings.push_back(single_quote(sequence));
   add_to_history(command_strings);

   return istat;
}