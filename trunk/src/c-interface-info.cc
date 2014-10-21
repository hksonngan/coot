/* src/c-interface.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006 The University of York
 * Author: Paul Emsley
 * Copyright 2007 by Paul Emsley
 * Copyright 2007, 2008, 2009 The University of Oxford
 * Copyright 2008 by The University of Oxford
 * Author: Paul Emsley
 * Copyright 2007, 2008 by Bernhard Lohkamp
 * Copyright 2007, 2008 The University of York
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

#ifdef USE_PYTHON
#include <Python.h>  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#include "compat/coot-sysdep.h"


#include <stdlib.h>
#include <string.h> // strncmp
#include <iostream>
#include <fstream>
#include <stdexcept>


#define HAVE_CIF  // will become unnessary at some stage.

#include <sys/types.h> // for stating
#include <sys/stat.h>

#if !defined _MSC_VER
#include <unistd.h>
#else
#include <windows.h>
#define snprintf _snprintf
#endif
 
#include "globjects.h" //includes gtk/gtk.h

#include "callbacks.h"
#include "interface.h" // now that we are moving callback
		       // functionality to the file, we need this
		       // header since some of the callbacks call
		       // fuctions built by glade.

#include <vector>
#include <string>

#include <mmdb2/mmdb_manager.h>
#include "coords/mmdb-extras.h"
#include "coords/mmdb.h"
#include "coords/mmdb-crystal.h"

#include "coot-utils/coot-map-utils.hh" // for make_rtop_orth_from()

#include "coords/Cartesian.h"
#include "coords/Bond_lines.h"

#include "graphics-info.h"

#include "skeleton/BuildCas.h"
#include "ligand/primitive-chi-angles.hh"

#include "trackball.h" // adding exportable rotate interface

#include "c-interface.h"
#include "coot-database.hh"

#include "guile-fixups.h"

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
#include "c-interface-python.hh"


/*  ----------------------------------------------------------------- */
/*                         Scripting:                                 */
/*  ----------------------------------------------------------------- */

#ifdef USE_GUILE
SCM coot_has_python_p() {

   SCM r = SCM_BOOL_F;
#ifdef USE_PYTHON
   r = SCM_BOOL_T;
#endif    
   return r;
}
#endif

#ifdef USE_PYTHON
PyObject *coot_has_guile() {
   PyObject *r = Py_False;
#ifdef USE_GUILE
   r = Py_True;
#endif
   Py_INCREF(r);
   return r;
}
#endif

bool coot_can_do_lidia_p() {
   
   bool r = false;

#ifdef HAVE_GOOCANVAS
#if ( ( (GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION > 11) ) || GTK_MAJOR_VERSION > 2)
   r = true;
#endif
#endif
   
   return r;
   
}


/*  ------------------------------------------------------------------------ */
/*                         Molecule Functions       :                        */
/*  ------------------------------------------------------------------------ */
/* section Molecule Functions */
// return status, less than -9999 is for failure (eg. bad imol);
float molecule_centre_internal(int imol, int iaxis) {

   float fstat = -10000;

   if (is_valid_model_molecule(imol)) {
      if (iaxis >=0 && iaxis <=2) { 
	 coot::Cartesian c =
	    centre_of_molecule(graphics_info_t::molecules[imol].atom_sel);
	 if (iaxis == 0)
	    return c.x();
	 if (iaxis == 1)
	    return c.y();
	 if (iaxis == 2)
	    return c.z();
      }
   } else {
      std::cout << "WARNING: molecule " << imol
		<< " is not a valid model molecule number " << std::endl;
   }
   return fstat;
}

int is_shelx_molecule(int imol) {

   int r=0;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].is_from_shelx_ins();
   }
   return r;

}


/*  ----------------------------------------------------------------------- */
/*               (Eleanor's) Residue info                                   */
/*  ----------------------------------------------------------------------- */
/* Similar to above, we need only one click though. */
void do_residue_info_dialog() {

   if (graphics_info_t::residue_info_edits->size() > 0) {

      std::string s =  "You have pending (un-Applied) residue edits\n";
      s += "Deal with them first.";
      GtkWidget *w = wrapped_nothing_bad_dialog(s);
      gtk_widget_show(w);
   } else { 
      std::cout << "Click on an atom..." << std::endl;
      graphics_info_t g;
      g.in_residue_info_define = 1;
      pick_cursor_maybe();
      graphics_info_t::pick_pending_flag = 1;
   }
} 

#ifdef USE_GUILE
SCM sequence_info(int imol) {

   SCM r = SCM_BOOL_F;

   if (is_valid_model_molecule(imol)) { 
      std::vector<std::pair<std::string, std::string> > seq =
	 graphics_info_t::molecules[imol].sequence_info();
      
      if (seq.size() > 0) {
	 r = SCM_EOL;
	 // unsigned int does't work here because then the termination
	 // condition never fails.
	 for (int iv=int(seq.size()-1); iv>=0; iv--) {
// 	    std::cout << "iv: " << iv << " seq.size: " << seq.size() << std::endl;
// 	    std::cout << "debug scming" << seq[iv].first.c_str()
// 		      << " and " << seq[iv].second.c_str() << std::endl;
	    SCM a = scm_makfrom0str(seq[iv].first.c_str());
	    SCM b = scm_makfrom0str(seq[iv].second.c_str());
	    SCM ls = scm_cons(a, b);
	    r = scm_cons(ls, r);
	 }
      }
   }
   return r;
} 
#endif // USE_GUILE


#ifdef USE_PYTHON
PyObject *sequence_info_py(int imol) {

   PyObject *r = Py_False;

   if (is_valid_model_molecule(imol)) { 
      
      std::vector<std::pair<std::string, std::string> > seq =
	 graphics_info_t::molecules[imol].sequence_info();


      if (seq.size() > 0) {
	 // unsigned int does't work here because then the termination
	 // condition never fails.
         r = PyList_New(seq.size());
	 PyObject *a;
	 PyObject *b;
	 PyObject *ls;
	 for (int iv=int(seq.size()-1); iv>=0; iv--) {
	    //std::cout << "iv: " << iv << " seq.size: " << seq.size() << std::endl;
	    //std::cout << "debug pythoning " << seq[iv].first.c_str()
	    //	      << " and " << seq[iv].second.c_str() << std::endl;
	    a = PyString_FromString(seq[iv].first.c_str());
	    b = PyString_FromString(seq[iv].second.c_str());
	    ls = PyList_New(2);
            PyList_SetItem(ls, 0, a);
            PyList_SetItem(ls, 1, b);
            PyList_SetItem(r, iv, ls);
	 }
      }
   }
   if (PyBool_Check(r)) {
      Py_INCREF(r);
   }
   return r;
} 
#endif // USE_PYTHON


// Called from a graphics-info-defines routine, would you believe? :)
//
// This should be a graphics_info_t function. 
//
// The reader is graphics_info_t::apply_residue_info_changes(GtkWidget *dialog);
// 
void output_residue_info_dialog(int imol, int atom_index) {

   if (graphics_info_t::residue_info_edits->size() > 0) {

      std::string s =  "You have pending (un-Applied) residue edits.\n";
      s += "Deal with them first.";
      GtkWidget *w = wrapped_nothing_bad_dialog(s);
      gtk_widget_show(w);

   } else { 

      if (imol <graphics_info_t::n_molecules()) {
	 if (graphics_info_t::molecules[imol].has_model()) {
	    if (atom_index < graphics_info_t::molecules[imol].atom_sel.n_selected_atoms) { 

	       graphics_info_t g;
	       output_residue_info_as_text(atom_index, imol);
	       mmdb::PAtom picked_atom = g.molecules[imol].atom_sel.atom_selection[atom_index];

	       std::string residue_name = picked_atom->GetResName();
   
	       mmdb::PPAtom atoms;
	       int n_atoms;

	       picked_atom->residue->GetAtomTable(atoms,n_atoms);
	       GtkWidget *widget = wrapped_create_residue_info_dialog();

	       mmdb::Residue *residue = picked_atom->residue; 
	       coot::residue_spec_t *res_spec_p =
		  new coot::residue_spec_t(residue->GetChainID(),
					   residue->GetSeqNum(),
					   residue->GetInsCode());
	       
	       // fill the master atom
	       GtkWidget *master_occ_entry =
		  lookup_widget(widget, "residue_info_master_atom_occ_entry"); 
	       GtkWidget *master_b_factor_entry =
		  lookup_widget(widget, "residue_info_master_atom_b_factor_entry");

	       gtk_signal_connect (GTK_OBJECT (master_occ_entry), "changed",
				   GTK_SIGNAL_FUNC (graphics_info_t::on_residue_info_master_atom_occ_changed),
				   NULL);
	       gtk_signal_connect (GTK_OBJECT (master_b_factor_entry),
				   "changed",
				   GTK_SIGNAL_FUNC (graphics_info_t::on_residue_info_master_atom_b_factor_changed),
				   NULL);


	       gtk_entry_set_text(GTK_ENTRY(master_occ_entry), "1.00");
	       gtk_entry_set_text(GTK_ENTRY(master_b_factor_entry),
				  graphics_info_t::float_to_string(graphics_info_t::default_new_atoms_b_factor).c_str());
					   
	       gtk_object_set_user_data(GTK_OBJECT(widget), res_spec_p);
	       g.fill_output_residue_info_widget(widget, imol, residue_name, atoms, n_atoms);
	       gtk_widget_show(widget);
	       g.reset_residue_info_edits();

	       try {
		  coot::primitive_chi_angles chi_angles(residue);
		  std::vector<coot::alt_confed_chi_angles> chis = chi_angles.get_chi_angles();
		  GtkWidget *chi_angles_frame = lookup_widget(widget, "chi_angles_frame");
		  gtk_widget_show(chi_angles_frame);
		  if (chis.size() > 0) {
		     unsigned int i_chi_set = 0;
		     for (unsigned int ich=0; ich<chis[i_chi_set].chi_angles.size(); ich++) {
		     
			int ic = chis[i_chi_set].chi_angles[ich].first;
			std::string label_name = "residue_info_chi_";
			label_name += coot::util::int_to_string(ic);
			label_name += "_label";
			GtkWidget *label = lookup_widget(widget, label_name.c_str());
			if (label) {
			   std::string text = "Chi ";
			   text += coot::util::int_to_string(ic);
			   text += ":  ";
			   if (chis[i_chi_set].alt_conf != "") {
			      text += " alt conf: ";
			      text += chis[i_chi_set].alt_conf;
			      text += " ";
			   } 
			   text += coot::util::float_to_string(chis[i_chi_set].chi_angles[ich].second);
			   text += " degrees";
			   gtk_label_set_text(GTK_LABEL(label), text.c_str());
			   gtk_widget_show(label);
			} else {
			   std::cout << "WARNING:: chi label not found " << label_name << std::endl;
			}
		     } 
		  }
	       }
	       catch (std::runtime_error mess) {
		  std::cout << mess.what() << std::endl;
	       }
	    }
	 }
      }
   }

   std::string cmd = "output-residue-info";
   std::vector<coot::command_arg_t> args;
   args.push_back(atom_index);
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

void
residue_info_dialog(int imol, const char *chain_id, int resno, const char *ins_code) {
   
   if (is_valid_model_molecule(imol)) {
      int atom_index = -1;
      mmdb::Residue *res = graphics_info_t::molecules[imol].residue_from_external(resno, ins_code, chain_id);
      mmdb::PPAtom residue_atoms;
      int n_residue_atoms;
      res->GetAtomTable(residue_atoms, n_residue_atoms);
      if (n_residue_atoms > 0) {
	 mmdb::Atom *at = residue_atoms[0];
	 int handle = graphics_info_t::molecules[imol].atom_sel.UDDAtomIndexHandle;
	 int ierr = at->GetUDData(handle, atom_index);
	 if (ierr == mmdb::UDDATA_Ok) { 
	    if (atom_index != -1) { 
	       output_residue_info_dialog(imol, atom_index);
	    }
	 }
      }
   }
}


// 23 Oct 2003: Why is this so difficult?  Because we want to attach
// atom info (what springs to mind is a pointer to the atom) for each
// entry, so that when the text in the entry is changed, we know to
// modify the atom.
//
// The problem with that is that behind our backs, that atom could
// disappear (e.g close molecule or delete residue, mutate or
// whatever), we are left with a valid looking (i.e. non-NULL)
// pointer, but the memory to which is points is invalid -> crash when
// we try to reference it.
//
// How shall we get round this?  refcounting?
//
// Instead, let's make a trivial class that contains the information
// we need to do a SelectAtoms to find the pointer to the atom, that
// class shall be called select_atom_info, it shall contain the
// molecule number, the chain id, the residue number, the insertion
// code, the atom name, the atom altconf.
// 


void output_residue_info_as_text(int atom_index, int imol) { 
   
   // It would be cool to flash the residue here.
   // (heh - it is).
   // 
   graphics_info_t g;
   mmdb::PAtom picked_atom = g.molecules[imol].atom_sel.atom_selection[atom_index];
   
   g.flash_selection(imol, 
		     picked_atom->residue->seqNum,
		     picked_atom->GetInsCode(),
		     picked_atom->residue->seqNum,
		     picked_atom->GetInsCode(),
		     picked_atom->altLoc,
		     picked_atom->residue->GetChainID());

   mmdb::PPAtom atoms;
   int n_atoms;

   picked_atom->residue->GetAtomTable(atoms,n_atoms);
   for (int i=0; i<n_atoms; i++) { 
      std::string segid = atoms[i]->segID;
      std::cout << "(" << imol << ") \"" 
		<< atoms[i]->name << "\"/"
		<< atoms[i]->GetModelNum()
		<< "/\""
		<< atoms[i]->GetChainID()  << "\"/"
		<< atoms[i]->GetSeqNum()   << "/\""
		<< atoms[i]->GetResName()
		<< "\", \""
		<< segid
		<< "\" occ: " 
		<< atoms[i]->occupancy 
		<< " with B-factor: "
		<< atoms[i]->tempFactor
		<< " element: \""
		<< atoms[i]->element
		<< "\""
		<< " at " << "("
		<< atoms[i]->x << "," << atoms[i]->y << "," 
		<< atoms[i]->z << ")" << std::endl;
   }

   // chi angles:
   coot::primitive_chi_angles chi_angles(picked_atom->residue);
   try { 
      std::vector<coot::alt_confed_chi_angles> chis = chi_angles.get_chi_angles();
      if (chis.size() > 0) {
	 unsigned int i_chi_set = 0;
	 std::cout << "   Chi Angles:" << std::endl;
	 for (unsigned int ich=0; ich<chis[i_chi_set].chi_angles.size(); ich++) {
	    std::cout << "     chi "<< chis[i_chi_set].chi_angles[ich].first << ": "
		      << chis[i_chi_set].chi_angles[ich].second
		      << " degrees" << std::endl;
	 }
      } else {
	 std::cout << "No Chi Angles for this residue" << std::endl;
      }
   }
   catch (std::runtime_error mess) {
      std::cout << mess.what() << std::endl;
   } 
   
   std::string cmd = "output-residue-info-as-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(atom_index);
   args.push_back(imol);
   add_to_history_typed(cmd, args);
}

// Actually I want to return a scheme object with occ, pos, b-factor info
// 
void
output_atom_info_as_text(int imol, const char *chain_id, int resno,
			 const char *inscode, const char *atname,
			 const char *altconf) {

   if (is_valid_model_molecule(imol)) {
      int index =
	 graphics_info_t::molecules[imol].full_atom_spec_to_atom_index(std::string(chain_id),
							resno,
							std::string(inscode),
							std::string(atname),
							std::string(altconf));
      
      mmdb::Atom *atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[index];
      std::cout << "(" << imol << ") " 
		<< atom->name << "/"
		<< atom->GetModelNum()
		<< "/"
		<< atom->GetChainID()  << "/"
		<< atom->GetSeqNum()   << "/"
		<< atom->GetResName()
		<< ", occ: " 
		<< atom->occupancy 
		<< " with B-factor: "
		<< atom->tempFactor
		<< " element: \""
		<< atom->element
		<< "\""
		<< " at " << "("
		<< atom->x << "," << atom->y << "," 
		<< atom->z << ")" << std::endl;
      try { 
	 // chi angles:
	 coot::primitive_chi_angles chi_angles(atom->residue);
	 std::vector<coot::alt_confed_chi_angles> chis = chi_angles.get_chi_angles();
	 if (chis.size() > 0) {
	    unsigned int i_chi_set = 0;
	    std::cout << "   Chi Angles:" << std::endl;
	    for (unsigned int ich=0; ich<chis[i_chi_set].chi_angles.size(); ich++) {
	       std::cout << "     chi "<< chis[i_chi_set].chi_angles[ich].first << ": "
			 << chis[i_chi_set].chi_angles[ich].second
			 << " degrees" << std::endl;
	    }
	 } else {
	    std::cout << "No Chi Angles for this residue" << std::endl;
	 }
      }
      catch (std::runtime_error mess) {
	 std::cout << mess.what() << std::endl;
      }
   }
   std::string cmd = "output-atom-info-as-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(chain_id);
   args.push_back(resno);
   args.push_back(inscode);
   args.push_back(atname);
   args.push_back(altconf);
   add_to_history_typed(cmd, args);

}

std::string atom_info_as_text_for_statusbar(int atom_index, int imol) {

  std::string ai;
  ai = "";
  if (is_valid_model_molecule(imol)) {      
    mmdb::Atom *at = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];
    std::string alt_conf_bit("");
    if (strncmp(at->altLoc, "", 1))
      alt_conf_bit=std::string(",") + std::string(at->altLoc);
    ai += "(mol. no: ";
    ai += graphics_info_t::int_to_string(imol);
    ai += ") ";
    ai += at->name;
    ai += alt_conf_bit;
    ai += "/";
    ai += graphics_info_t::int_to_string(at->GetModelNum());
    ai += "/";
    ai += at->GetChainID();
    ai += "/";
    ai += graphics_info_t::int_to_string(at->GetSeqNum());
    ai += at->GetInsCode();
    ai += " ";
    ai += at->GetResName();
    ai += " occ: ";
    ai += graphics_info_t::float_to_string(at->occupancy);
    ai += " bf: ";
    ai += graphics_info_t::float_to_string(at->tempFactor);
    ai += " ele: ";
    ai += at->element;
    ai += " pos: (";
    // using atom positions (ignoring symmetry etc)
    ai += graphics_info_t::float_to_string(at->x);
    ai += ",";
    ai += graphics_info_t::float_to_string(at->y);
    ai += ",";
    ai += graphics_info_t::float_to_string(at->z);
    ai += ")";
  }
  return ai;
}


std::string
atom_info_as_text_for_statusbar(int atom_index, int imol, 
                                const std::pair<symm_trans_t, Cell_Translation> &sts) {

  std::string ai;
  ai = "";
  if (is_valid_model_molecule(imol)) {      
    mmdb::Atom *at = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];
    std::string alt_conf_bit("");
    if (strncmp(at->altLoc, "", 1))
      alt_conf_bit=std::string(",") + std::string(at->altLoc);
    ai += "(mol. no: ";
    ai += graphics_info_t::int_to_string(imol);
    ai += ") ";
    ai += at->name;
    ai += alt_conf_bit;
    ai += "/";
    ai += graphics_info_t::int_to_string(at->GetModelNum());
    ai += "/";
    ai += at->GetChainID();
    ai += "/";
    ai += graphics_info_t::int_to_string(at->GetSeqNum());
    ai += at->GetInsCode();
    ai += " ";
    ai += at->GetResName();
    // ignoring symmetry?!, no
    ai += " ";
    ai += to_string(sts);
    ai += " occ: ";
    ai += graphics_info_t::float_to_string(at->occupancy);
    ai += " bf: ";
    ai += graphics_info_t::float_to_string(at->tempFactor);
    ai += " ele: ";
    ai += at->element;
    ai += " pos: (";
    // using atom positions (ignoring symmetry etc)
    ai += graphics_info_t::float_to_string(at->x);
    ai += ",";
    ai += graphics_info_t::float_to_string(at->y);
    ai += ",";
    ai += graphics_info_t::float_to_string(at->z);
    ai += ")";
  }

  return ai;

}


#ifdef USE_GUILE
// (list occ temp-factor element x y z) or #f 
SCM atom_info_string_scm(int imol, const char *chain_id, int resno,
			 const char *ins_code, const char *atname,
			 const char *altconf) {

   SCM r = SCM_BOOL_F;
   
   if (is_valid_model_molecule(imol)) {
      int index =
	 graphics_info_t::molecules[imol].full_atom_spec_to_atom_index(std::string(chain_id),
								       resno,
								       std::string(ins_code),
								       std::string(atname),
								       std::string(altconf));
      if (index > -1) { 
	 mmdb::Atom *atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[index];

	 r = SCM_EOL;

	 r = scm_cons(scm_double2num(atom->occupancy), r);
	 r = scm_cons(scm_double2num(atom->tempFactor), r);
	 r = scm_cons(scm_makfrom0str(atom->element), r);
	 r = scm_cons(scm_double2num(atom->x), r);
	 r = scm_cons(scm_double2num(atom->y), r);
	 r = scm_cons(scm_double2num(atom->z), r);
	 r = scm_reverse(r);
      }
   }
   std::string cmd = "atom-info-string";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(coot::util::single_quote(chain_id));
   args.push_back(resno);
   args.push_back(coot::util::single_quote(ins_code));
   args.push_back(coot::util::single_quote(atname));
   args.push_back(coot::util::single_quote(altconf));
   add_to_history_typed(cmd, args);

   return r;
}
#endif // USE_GUILE


// BL says:: we return a string in python list compatible format.
// to use it in python you need to eval the string!
#ifdef USE_PYTHON
// "[occ,temp_factor,element,x,y,z]" or 0
PyObject *atom_info_string_py(int imol, const char *chain_id, int resno,
			      const char *ins_code, const char *atname,
			      const char *altconf) {

   PyObject *r = Py_False;
   
   if (is_valid_model_molecule(imol)) {
      int index =
         graphics_info_t::molecules[imol].full_atom_spec_to_atom_index(std::string(chain_id),
                                                                       resno,
                                                                       std::string(ins_code),
                                                                       std::string(atname),
                                                                       std::string(altconf));
      if (index > -1) { 
         mmdb::Atom *atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[index];

	 r = PyList_New(6);
	 PyList_SetItem(r, 0, PyFloat_FromDouble(atom->occupancy));
	 PyList_SetItem(r, 1, PyFloat_FromDouble(atom->tempFactor));
	 PyList_SetItem(r, 2, PyString_FromString(atom->element));
	 PyList_SetItem(r, 3, PyFloat_FromDouble(atom->x));
	 PyList_SetItem(r, 4, PyFloat_FromDouble(atom->y));
	 PyList_SetItem(r, 5, PyFloat_FromDouble(atom->z));
      }
   }
   std::string cmd = "atom_info_string";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(chain_id);
   args.push_back(resno);
   args.push_back(ins_code);
   args.push_back(atname);
   args.push_back(altconf);
   add_to_history_typed(cmd, args);

   return r;
}
#endif // PYTHON



#ifdef USE_GUILE
#ifdef USE_PYTHON
PyObject *scm_to_py(SCM s) {

   PyObject *o = Py_None;
   if (scm_is_true(scm_list_p(s))) {
      SCM s_length_scm = scm_length(s);
      int s_length = scm_to_int(s_length_scm);
      o = PyList_New(s_length);
      for (int item=0; item<s_length; item++) {
	 SCM item_scm = scm_list_ref(s, SCM_MAKINUM(item));
	 PyList_SetItem(o, item, scm_to_py(item_scm));
      }
   } else {
      if (scm_is_true(scm_boolean_p(s))) {
	 if (scm_is_true(s)) {
	    o = Py_True;
	 } else {
	    o = Py_False;
	 }
      } else {
	 if (scm_is_true(scm_integer_p(s))) {
	    int iscm = scm_to_int(s);
	    o = PyInt_FromLong(iscm);
	 } else {
	    if (scm_is_true(scm_real_p(s))) {
	       float f = scm_to_double(s);
	       o = PyFloat_FromDouble(f);

	    } else {
	       if (scm_is_true(scm_string_p(s))) {
		  std::string str = scm_to_locale_string(s);
		  o = PyString_FromString(str.c_str());
	       }
	    }
	 }
      }
   }

   if (PyBool_Check(o) || o == Py_None) {
     Py_INCREF(o);
   }

   return o;
}
#endif // USE_GUILE
#endif // USE_PYTHON


#ifdef USE_GUILE
#ifdef USE_PYTHON
SCM py_to_scm(PyObject *o) {

   SCM s = SCM_BOOL_F;
   if (PyList_Check(o)) {
      int l = PyObject_Length(o);
      s = SCM_EOL;
      for (int item=0; item<l; item++) {
	 PyObject *py_item = PyList_GetItem(o, item);
	 if (py_item == NULL) {
	   PyErr_Print();
	 }
	 SCM t = py_to_scm(py_item);
	 s = scm_cons(t, s);
      }
      s = scm_reverse(s);
   } else {
      if (PyBool_Check(o)) {
	 s = SCM_BOOL_F;
	 int i = PyInt_AsLong(o);
	 if (i)
	    s = SCM_BOOL_T;
      } else {
	 if (PyInt_Check(o)) {
	    int i=PyInt_AsLong(o);
	    s = SCM_MAKINUM(i);
	 } else {
	    if (PyFloat_Check(o)) {
	       double f = PyFloat_AsDouble(o);
	       s = scm_float2num(f);
	    } else {
	       if (PyString_Check(o)) {
		  std::string str = PyString_AsString(o);
		  s = scm_makfrom0str(str.c_str());
	       } else {
		  if (o == Py_None) {
		     s = SCM_UNSPECIFIED;
		  }
	       } 
	    }
	 }
      }
   }
   return s;
}

#endif // USE_GUILE
#endif // USE_PYTHON

// Return residue specs for residues that have atoms that are
// closer than radius Angstroems to any atom in the residue
// specified by res_in.
// 
#ifdef USE_GUILE
SCM residues_near_residue(int imol, SCM residue_in, float radius) {

   SCM r = SCM_EOL;
   if (is_valid_model_molecule(imol)) {
      SCM chain_id_scm = scm_list_ref(residue_in, SCM_MAKINUM(0));
      SCM resno_scm    = scm_list_ref(residue_in, SCM_MAKINUM(1));
      SCM ins_code_scm = scm_list_ref(residue_in, SCM_MAKINUM(2));
      std::string chain_id = scm_to_locale_string(chain_id_scm);
      std::string ins_code = scm_to_locale_string(ins_code_scm);
      int resno            = scm_to_int(resno_scm);
      coot::residue_spec_t rspec(chain_id, resno, ins_code);
      std::vector<coot::residue_spec_t> v =
	 graphics_info_t::molecules[imol].residues_near_residue(rspec, radius);
      for (unsigned int i=0; i<v.size(); i++) {
	 SCM res_spec = SCM_EOL;
	 res_spec = scm_cons(scm_makfrom0str(v[i].insertion_code.c_str()), res_spec);
	 res_spec = scm_cons(scm_int2num(v[i].resno), res_spec);
	 res_spec = scm_cons(scm_makfrom0str(v[i].chain.c_str()), res_spec);
	 r = scm_cons(res_spec, r);
      }
   } 
   return r;
}
#endif // USE_GUILE

// Return residue specs for residues that have atoms that are
// closer than radius Angstroems to any atom in the residue
// specified by res_in.
// 
#ifdef USE_PYTHON
PyObject *residues_near_residue_py(int imol, PyObject *residue_in, float radius) {

   PyObject *r = PyList_New(0);
   if (is_valid_model_molecule(imol)) {
      PyObject *chain_id_py = PyList_GetItem(residue_in, 0);
      PyObject *resno_py    = PyList_GetItem(residue_in, 1);
      PyObject *ins_code_py = PyList_GetItem(residue_in, 2);
      std::string chain_id = PyString_AsString(chain_id_py);
      std::string ins_code = PyString_AsString(ins_code_py);
      int resno            = PyInt_AsLong(resno_py);
      coot::residue_spec_t rspec(chain_id, resno, ins_code);
      std::vector<coot::residue_spec_t> v =
	 graphics_info_t::molecules[imol].residues_near_residue(rspec, radius);
      for (unsigned int i=0; i<v.size(); i++) {
	 PyObject *res_spec = PyList_New(3);
	 PyList_SetItem(res_spec, 0, PyString_FromString(v[i].chain.c_str()));
	 PyList_SetItem(res_spec, 1, PyInt_FromLong(v[i].resno));
	 PyList_SetItem(res_spec, 2, PyString_FromString(v[i].insertion_code.c_str()));
	 PyList_Append(r, res_spec);
	 Py_XDECREF(res_spec);
      }
   } 
   return r;
}
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM residues_near_position_scm(int imol, SCM pt_in_scm, float radius) {

   SCM r = SCM_EOL;

   if (is_valid_model_molecule(imol)) {

      SCM pt_in_length_scm = scm_length(pt_in_scm);
      int pt_in_length = scm_to_int(pt_in_length_scm);
      if (pt_in_length != 3) {
	 std::cout << "WARNING:: input pt is not a list of 3 elements"
		   << std::endl;
      } else {

	 double x = scm_to_double(scm_list_ref(pt_in_scm, SCM_MAKINUM(0)));
	 double y = scm_to_double(scm_list_ref(pt_in_scm, SCM_MAKINUM(1)));
	 double z = scm_to_double(scm_list_ref(pt_in_scm, SCM_MAKINUM(2)));

	 clipper::Coord_orth pt(x,y,z);
	 
	 mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
	 std::vector<mmdb::Residue *> v = coot::residues_near_position(pt, mol, radius);
	 for (unsigned int i=0; i<v.size(); i++) {
	    SCM r_scm = scm_residue(coot::residue_spec_t(v[i]));
	    r = scm_cons(r_scm, r);
	 }
      }
   }
   return r;
} 
#endif 

#ifdef USE_PYTHON
PyObject *residues_near_position_py(int imol, PyObject *pt_in_py, float radius) {

   PyObject *r = PyList_New(0);

   if (is_valid_model_molecule(imol)) {

      int pt_in_length = PyObject_Length(pt_in_py);
      if (pt_in_length != 3) {
	 std::cout << "WARNING:: input pt is not a list of 3 elements"
		   << std::endl;
      } else {

	double x = PyFloat_AsDouble(PyList_GetItem(pt_in_py, 0));
	double y = PyFloat_AsDouble(PyList_GetItem(pt_in_py, 1));
	double z = PyFloat_AsDouble(PyList_GetItem(pt_in_py, 2));

	clipper::Coord_orth pt(x,y,z);
	 
	mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
	std::vector<mmdb::Residue *> v = coot::residues_near_position(pt, mol, radius);
	for (unsigned int i=0; i<v.size(); i++) {
	  PyObject *r_py = py_residue(coot::residue_spec_t(v[i]));
	  PyList_Append(r, r_py);
	  Py_XDECREF(r_py);
	}
      }
   }
   return r;
} 
#endif

//! find the active residue, find the near residues (within radius) 
//! create a new molecule, run reduce on that, import hydrogens from
//! the result and apply them to the molecule of the active residue.
void hydrogenate_region(float radius) {

   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();
   if (pp.first) {
      int imol = pp.second.first;
      coot::residue_spec_t central_residue(pp.second.second);
      std::cout << "----------- hydrogenating " << central_residue
		<< " in " << imol << std::endl;
      std::vector<coot::residue_spec_t> v =
	 graphics_info_t::molecules[imol].residues_near_residue(pp.second.second, radius);
      v.push_back(central_residue);
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      mmdb::Manager *new_mol = coot::util::create_mmdbmanager_from_residue_specs(v, mol);
      if (new_mol) {

	 coot::util::create_directory("coot-molprobity"); // exists already maybe? Handled.

	 std::string name_part = graphics_info_t::molecules[imol].Refmac_name_stub() + ".pdb";
	 
	 std::string pdb_in_file_name  = "hydrogenate-region-in-"  + name_part;
	 std::string pdb_out_file_name = "hydrogenate-region-out-" + name_part;
	 
	 std::string pdb_in =  coot::util::append_dir_file("coot-molprobity", pdb_in_file_name);
	 std::string pdb_out = coot::util::append_dir_file("coot-molprobity", pdb_out_file_name);
	 
	 new_mol->WritePDBASCII(pdb_in.c_str());
	 
	 if (graphics_info_t::prefer_python) {
#ifdef USE_PYTHON

	    std::string python_command = "reduce_on_pdb_file(";
	    python_command += coot::util::int_to_string(imol);
	    python_command += ", ";
	    python_command += single_quote(pdb_in);
	    python_command += ", ";
	    python_command += single_quote(pdb_out);
	    python_command += ")";

	    PyObject *r = safe_python_command_with_return(python_command);
	    std::cout << "::: safe_python_command_with_return() returned " << r << std::endl;
	    std::cout << "::: safe_python_command_with_return() returned "
		      << PyString_AsString(display_python(r)) << std::endl;
	    if (r == Py_True) {
	       std::cout << "........ calling add_hydrogens_from_file() with pdb_out "
			 << pdb_out << std::endl;
	       graphics_info_t::molecules[imol].add_hydrogens_from_file(pdb_out);
	    }
	    Py_XDECREF(r);

#endif // PYTHON
	 } else {
#ifdef USE_GUILE
	    // write a PDB file and run reduce, read it in
	    //
	    std::string scheme_command = "(reduce-on-pdb-file ";
	    scheme_command += coot::util::int_to_string(imol);
	    scheme_command += " ";
	    scheme_command += single_quote(pdb_in);
	    scheme_command += " ";
	    scheme_command += single_quote(pdb_out);
	    scheme_command += ")";
	    
	    SCM r = safe_scheme_command(scheme_command);
	    if (scm_is_true(r)) {
	       graphics_info_t::molecules[imol].add_hydrogens_from_file(pdb_out);
	    }
#endif 	 
	 }

	 graphics_draw();
	 delete new_mol;
	 
      } 
   } 
}



#ifdef USE_GUILE
coot::residue_spec_t residue_spec_from_scm(SCM residue_in) {
   SCM chain_id_scm = scm_list_ref(residue_in, SCM_MAKINUM(0));
   SCM resno_scm    = scm_list_ref(residue_in, SCM_MAKINUM(1));
   SCM ins_code_scm = scm_list_ref(residue_in, SCM_MAKINUM(2));
   std::string chain_id = scm_to_locale_string(chain_id_scm);
   std::string ins_code = scm_to_locale_string(ins_code_scm);
   int resno            = scm_to_int(resno_scm);
   coot::residue_spec_t rspec(chain_id, resno, ins_code);
   return rspec;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
coot::residue_spec_t residue_spec_from_py(PyObject *residue_in) {

   // What about make_residue_spec_py()?

   coot::residue_spec_t rspec; // default

   if (PyList_Check(residue_in)) { 

      if (PyList_Size(residue_in) == 3) { 
	 PyObject *chain_id_py = PyList_GetItem(residue_in, 0);
	 PyObject *resno_py    = PyList_GetItem(residue_in, 1);
	 PyObject *ins_code_py = PyList_GetItem(residue_in, 2);
	 std::string chain_id  = PyString_AsString(chain_id_py);
	 std::string ins_code  = PyString_AsString(ins_code_py);
	 long resno            = PyInt_AsLong(resno_py);
	 std::cout << "making spec from " << chain_id << " " << resno
		   << " :" << ins_code << ":" << std::endl;
	 rspec = coot::residue_spec_t(chain_id, resno, ins_code);
      }
   }
   return rspec;
}
#endif // USE_PYTHON

#ifdef USE_GUILE
// output is like this:
// (list
//    (list (list atom-name alt-conf)
//          (list occ temp-fact element)
//          (list x y z)))
// 
SCM residue_info(int imol, const char* chain_id, int resno, const char *ins_code) {

  SCM r = SCM_BOOL_F;
  if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      
      mmdb::Model *model_p = mol->GetModel(imod);
      mmdb::Chain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id_mol(chain_p->GetChainID());
	 if (chain_id_mol == std::string(chain_id)) { 
	    int nres = chain_p->GetNumberOfResidues();
	    mmdb::PResidue residue_p;
	    mmdb::Atom *at;

	    for (int ires=0; ires<nres; ires++) { 
	       residue_p = chain_p->GetResidue(ires);
	       std::string res_ins_code(residue_p->GetInsCode());
	       if (residue_p->GetSeqNum() == resno) { 
		  if (res_ins_code == std::string(ins_code)) {
		     int n_atoms = residue_p->GetNumberOfAtoms();
		     SCM at_info = SCM_BOOL(0);
		     SCM at_pos;
		     SCM at_occ, at_biso, at_ele, at_name, at_altconf;
		     SCM at_segid;
		     SCM at_x, at_y, at_z;
		     SCM all_atoms = SCM_EOL;
		     for (int iat=0; iat<n_atoms; iat++) {
			at = residue_p->GetAtom(iat);
                        if (at->Ter) continue; //ignore TER records
                        
			at_x  = scm_float2num(at->x);
			at_y  = scm_float2num(at->y);
			at_z  = scm_float2num(at->z);
			at_pos = scm_list_3(at_x, at_y, at_z);
			at_occ = scm_float2num(at->occupancy);
			at_biso= scm_float2num(at->tempFactor);
			at_ele = scm_makfrom0str(at->element);
			at_name = scm_makfrom0str(at->name);
			at_segid = scm_makfrom0str(at->segID);
			at_altconf = scm_makfrom0str(at->altLoc);
			SCM at_b = at_biso;
			if (at->WhatIsSet & mmdb::ASET_Anis_tFac) {
			   at_b = SCM_EOL;
			   at_b = scm_cons(at_biso, at_b);
			   at_b = scm_cons(scm_float2num(at->u11), at_b);
			   at_b = scm_cons(scm_float2num(at->u22), at_b);
			   at_b = scm_cons(scm_float2num(at->u33), at_b);
			   at_b = scm_cons(scm_float2num(at->u12), at_b);
			   at_b = scm_cons(scm_float2num(at->u13), at_b);
			   at_b = scm_cons(scm_float2num(at->u23), at_b);
			   at_b = scm_reverse(at_b);
			}
			SCM compound_name = scm_list_2(at_name, at_altconf);
			SCM compound_attrib = scm_list_4(at_occ, at_b, at_ele, at_segid);
			at_info = scm_list_3(compound_name, compound_attrib, at_pos);
			all_atoms = scm_cons(at_info, all_atoms);
		     }
		     r = scm_reverse(all_atoms);
		  }
	       }
	    }
	 }
      }
   }
   return r;
}
#endif // USE_GUILE
// BL says:: this is my attepmt to code it in python
#ifdef USE_PYTHON
PyObject *residue_info_py(int imol, const char* chain_id, int resno, const char *ins_code) {

   PyObject *r = Py_False;
   PyObject *all_atoms;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      
      mmdb::Model *model_p = mol->GetModel(imod);
      mmdb::Chain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
         chain_p = model_p->GetChain(ichain);
         std::string chain_id_mol(chain_p->GetChainID());
         if (chain_id_mol == std::string(chain_id)) { 
            int nres = chain_p->GetNumberOfResidues();
            mmdb::PResidue residue_p;
            mmdb::Atom *at;

            // why use this bizarre contrivance to get a null list for
            // starting? I must be missing something.
            for (int ires=0; ires<nres; ires++) { 
               residue_p = chain_p->GetResidue(ires);
               std::string res_ins_code(residue_p->GetInsCode());
               if (residue_p->GetSeqNum() == resno) { 
                  if (res_ins_code == std::string(ins_code)) {
                     int n_atoms = residue_p->GetNumberOfAtoms();
		     PyObject *at_info = Py_False;
		     PyObject *at_pos;
		     PyObject *at_occ, *at_b, *at_biso, *at_ele, *at_name, *at_altconf;
		     PyObject *at_segid;
		     PyObject *at_x, *at_y, *at_z;
		     PyObject *compound_name;
		     PyObject *compound_attrib;
		     all_atoms = PyList_New(0);
                     for (int iat=0; iat<n_atoms; iat++) {

                        at = residue_p->GetAtom(iat);
                        if (at->Ter) continue; //ignore TER records
                        
                        at_x  = PyFloat_FromDouble(at->x);
                        at_y  = PyFloat_FromDouble(at->y);
                        at_z  = PyFloat_FromDouble(at->z);
                        at_pos = PyList_New(3);
                        PyList_SetItem(at_pos, 0, at_x);
                        PyList_SetItem(at_pos, 1, at_y);
                        PyList_SetItem(at_pos, 2, at_z);

                        at_occ = PyFloat_FromDouble(at->occupancy);
                        at_biso= PyFloat_FromDouble(at->tempFactor);
                        at_ele = PyString_FromString(at->element);
                        at_name = PyString_FromString(at->name);
                        at_segid = PyString_FromString(at->segID);
                        at_altconf = PyString_FromString(at->altLoc);

			at_b = at_biso;
			if (at->WhatIsSet & mmdb::ASET_Anis_tFac) {
			   at_b = PyList_New(7);
			   PyList_SetItem(at_b, 0, at_biso);
			   PyList_SetItem(at_b, 1, PyFloat_FromDouble(at->u11));
			   PyList_SetItem(at_b, 2, PyFloat_FromDouble(at->u22));
			   PyList_SetItem(at_b, 3, PyFloat_FromDouble(at->u33));
			   PyList_SetItem(at_b, 4, PyFloat_FromDouble(at->u12));
			   PyList_SetItem(at_b, 5, PyFloat_FromDouble(at->u13));
			   PyList_SetItem(at_b, 6, PyFloat_FromDouble(at->u23));
			}

                        compound_name = PyList_New(2);
                        PyList_SetItem(compound_name, 0 ,at_name);
                        PyList_SetItem(compound_name, 1 ,at_altconf);

                        compound_attrib = PyList_New(4);
                        PyList_SetItem(compound_attrib, 0, at_occ);
                        PyList_SetItem(compound_attrib, 1, at_b);
                        PyList_SetItem(compound_attrib, 2, at_ele);
                        PyList_SetItem(compound_attrib, 3, at_segid);

                        at_info = PyList_New(3);
                        PyList_SetItem(at_info, 0, compound_name);
                        PyList_SetItem(at_info, 1, compound_attrib);
                        PyList_SetItem(at_info, 2, at_pos);

                        PyList_Append(all_atoms, at_info);
                     }
		     r = all_atoms;
                  }
               }
            }
         }
      }
   }

   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }

   return r;

}

#endif //PYTHON


#ifdef USE_GUILE
SCM residue_name(int imol, const char* chain_id, int resno, const char *ins_code) {

   SCM r = SCM_BOOL(0);
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      bool have_resname_flag = 0;
      
      mmdb::Model *model_p = mol->GetModel(imod);
      mmdb::Chain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id_mol(chain_p->GetChainID());
	 if (chain_id_mol == std::string(chain_id)) { 
	    int nres = chain_p->GetNumberOfResidues();
	    mmdb::PResidue residue_p;
	    for (int ires=0; ires<nres; ires++) { 
	       residue_p = chain_p->GetResidue(ires);
	       if (residue_p->GetSeqNum() == resno) { 
		  std::string ins = residue_p->GetInsCode();
		  if (ins == ins_code) {
		     r = scm_makfrom0str(residue_p->GetResName());
		     have_resname_flag = 1;
		     break;
		  }
	       }
	    }
	 }
	 if (have_resname_flag)
	    break;
      }
   }
   return r;
}
#endif // USE_GUILE
#ifdef USE_PYTHON
PyObject *residue_name_py(int imol, const char* chain_id, int resno, const char *ins_code) {

   PyObject *r;
   r = Py_False;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      int imod = 1;
      bool have_resname_flag = 0;
      
      mmdb::Model *model_p = mol->GetModel(imod);
      mmdb::Chain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
         chain_p = model_p->GetChain(ichain);
         std::string chain_id_mol(chain_p->GetChainID());
         if (chain_id_mol == std::string(chain_id)) { 
            int nres = chain_p->GetNumberOfResidues();
            mmdb::PResidue residue_p;
            for (int ires=0; ires<nres; ires++) { 
               residue_p = chain_p->GetResidue(ires);
               if (residue_p->GetSeqNum() == resno) { 
                  std::string ins = residue_p->GetInsCode();
                  if (ins == ins_code) {
                     r = PyString_FromString(residue_p->GetResName());
                     have_resname_flag = 1;
                     break;
                  }
               }
            }
         }
         if (have_resname_flag)
            break;
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // USE_PYTHON


#ifdef USE_GUILE
SCM chain_fragments_scm(int imol, short int screen_output_also) {

   SCM r = SCM_BOOL_F;

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::vector<coot::fragment_info_t> f = g.molecules[imol].get_fragment_info(screen_output_also);
   } 
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *chain_fragments_py(int imol, short int screen_output_also) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::vector<coot::fragment_info_t> f = g.molecules[imol].get_fragment_info(screen_output_also);
   } 

   if (PyBool_Check(r)) {
      Py_INCREF(r);
   }

   return r;
}
#endif // USE_PYTHON


void set_rotation_centre(const clipper::Coord_orth &pos) {
   graphics_info_t g;
   g.setRotationCentre(coot::Cartesian(pos));
}


#ifdef USE_GUILE
// Bernie, no need to pythonize this, it's just to test the return
// values on pressing "next residue" and "previous residue" (you can
// if you wish of course).
//
// Pass the current values, return new values
//
// Hmm maybe these function should pass the atom name too?  Yes they should
SCM goto_next_atom_maybe(const char *chain_id, int resno, const char *ins_code,
			 const char *atom_name) {

   SCM r = SCM_BOOL_F;

   int imol = go_to_atom_molecule_number();
   if (is_valid_model_molecule(imol)) { 

      graphics_info_t g;
      coot::Cartesian rc = g.RotationCentre();

      int atom_index =
	 graphics_info_t::molecules[imol].intelligent_next_atom(chain_id, resno,
								atom_name, ins_code, rc);

      if (atom_index != -1) {
	 mmdb::Atom *next_atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];

	 std::string next_chain_id  = next_atom->GetChainID();
	 std::string next_atom_name = next_atom->name;
	 int next_residue_number    = next_atom->GetSeqNum();
	 std::string next_ins_code  = next_atom->GetInsCode();

	 r = SCM_EOL;
	 r = scm_cons(scm_makfrom0str(next_atom_name.c_str()), r);
	 r = scm_cons(scm_makfrom0str(next_ins_code.c_str()), r);
	 r = scm_cons(scm_int2num(next_residue_number) ,r);
	 r = scm_cons(scm_makfrom0str(next_chain_id.c_str()), r);
      }
   }
   return r;
}
#endif

#ifdef USE_GUILE
SCM goto_prev_atom_maybe(const char *chain_id, int resno, const char *ins_code,
			 const char *atom_name) {

   SCM r = SCM_BOOL_F;
   int imol = go_to_atom_molecule_number();
   if (is_valid_model_molecule(imol)) { 

      graphics_info_t g;
      coot::Cartesian rc = g.RotationCentre();
      int atom_index =
	 graphics_info_t::molecules[imol].intelligent_previous_atom(chain_id, resno,
								    atom_name, ins_code, rc);

      if (atom_index != -1) {
	 mmdb::Atom *next_atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];

	 std::string next_chain_id  = next_atom->GetChainID();
	 std::string next_atom_name = next_atom->name;
	 int next_residue_number    = next_atom->GetSeqNum();
	 std::string next_ins_code  = next_atom->GetInsCode();

	 r = SCM_EOL;
	 r = scm_cons(scm_makfrom0str(next_atom_name.c_str()), r);
	 r = scm_cons(scm_makfrom0str(next_ins_code.c_str()), r);
	 r = scm_cons(scm_int2num(next_residue_number) ,r);
	 r = scm_cons(scm_makfrom0str(next_chain_id.c_str()), r);
      }
   }
   return r;
} 
#endif 

#ifdef USE_PYTHON
// Pass the current values, return new values
//
// Hmm maybe these function should pass the atom name too?  Yes they should
PyObject *goto_next_atom_maybe_py(const char *chain_id, int resno, const char *ins_code,
				  const char *atom_name) {

   PyObject *r = Py_False;

   int imol = go_to_atom_molecule_number();
   if (is_valid_model_molecule(imol)) { 

      graphics_info_t g;
      coot::Cartesian rc = g.RotationCentre();
      int atom_index =
	 graphics_info_t::molecules[imol].intelligent_next_atom(chain_id, resno,
								    atom_name, ins_code, rc);
      if (atom_index != -1) {
	 mmdb::Atom *next_atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];

	 std::string next_chain_id  = next_atom->GetChainID();
	 std::string next_atom_name = next_atom->name;
	 int next_residue_number    = next_atom->GetSeqNum();
	 std::string next_ins_code  = next_atom->GetInsCode();

	 r = PyList_New(4);
	 PyList_SetItem(r, 0, PyString_FromString(next_chain_id.c_str()));
	 PyList_SetItem(r, 1, PyInt_FromLong(next_residue_number));
	 PyList_SetItem(r, 2, PyString_FromString(next_ins_code.c_str()));
	 PyList_SetItem(r, 3, PyString_FromString(next_atom_name.c_str()));
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // USE_PYTHON

#ifdef USE_PYTHON
PyObject *goto_prev_atom_maybe_py(const char *chain_id, int resno, const char *ins_code,
				  const char *atom_name) {

   PyObject *r = Py_False;
   int imol = go_to_atom_molecule_number();
   if (is_valid_model_molecule(imol)) { 

      graphics_info_t g;
      coot::Cartesian rc = g.RotationCentre();
      int atom_index =
	 graphics_info_t::molecules[imol].intelligent_previous_atom(chain_id, resno,
								    atom_name, ins_code, rc);

      if (atom_index != -1) {
	 mmdb::Atom *next_atom = graphics_info_t::molecules[imol].atom_sel.atom_selection[atom_index];

	 std::string next_chain_id  = next_atom->GetChainID();
	 std::string next_atom_name = next_atom->name;
	 int next_residue_number    = next_atom->GetSeqNum();
	 std::string next_ins_code  = next_atom->GetInsCode();

	 r = PyList_New(4);
	 PyList_SetItem(r, 0, PyString_FromString(next_chain_id.c_str()));
	 PyList_SetItem(r, 1, PyInt_FromLong(next_residue_number));
	 PyList_SetItem(r, 2, PyString_FromString(next_ins_code.c_str()));
	 PyList_SetItem(r, 3, PyString_FromString(next_atom_name.c_str()));
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif // USE_PYTHON

// A C++ function interface:
// 
int set_go_to_atom_from_spec(const coot::atom_spec_t &atom_spec) {

   int success = 0;
   graphics_info_t g;

   if (! atom_spec.empty()) { 
      g.set_go_to_atom_chain_residue_atom_name(atom_spec.chain.c_str(), 
					       atom_spec.resno,
					       atom_spec.insertion_code.c_str(), 
					       atom_spec.atom_name.c_str(),
					       atom_spec.alt_conf.c_str());
      
      success = g.try_centre_from_new_go_to_atom(); 
      if (success)
	 update_things_on_move_and_redraw();
   }
   return success; 
}

int set_go_to_atom_from_res_spec(const coot::residue_spec_t &spec) {

   int success = 0;
   graphics_info_t g;
   int imol = g.go_to_atom_molecule();

   if (is_valid_model_molecule(imol)) { 
      coot::atom_spec_t atom_spec = g.molecules[imol].intelligent_this_residue_atom(spec);
      if (! atom_spec.empty()) {
	 success = set_go_to_atom_from_spec(atom_spec);
      }
   }
   return success;
}

#ifdef USE_GUILE
int set_go_to_atom_from_res_spec_scm(SCM residue_spec_scm) {

   coot::residue_spec_t spec = residue_spec_from_scm(residue_spec_scm);
   return set_go_to_atom_from_res_spec(spec);
}

#endif 

#ifdef USE_PYTHON
int set_go_to_atom_from_res_spec_py(PyObject *residue_spec_py) {

   std::pair<bool, coot::residue_spec_t> spec = make_residue_spec_py(residue_spec_py);
   if (spec.first) { 
      return set_go_to_atom_from_res_spec(spec.second);
   } else { 
      return -1;
   }
} 
#endif 



// (is-it-valid? (active-molecule-number spec))
std::pair<bool, std::pair<int, coot::atom_spec_t> > active_atom_spec() {

   return graphics_info_t::active_atom_spec();
}

#ifdef USE_PYTHON
// return a tuple of (Py_Bool (number, atom_spec))
PyObject *active_atom_spec_py() {

   PyObject *rv = PyTuple_New(2);
   
   std::pair<bool, std::pair<int, coot::atom_spec_t> > r = active_atom_spec();
      PyObject *state_py = Py_True;
      PyObject *mol_no = PyInt_FromLong(r.second.first);
      PyObject *spec = py_residue(r.second.second);
      PyObject *tuple_inner = PyTuple_New(2);
   if (! r.first) {
      state_py = Py_False;
   }
   Py_INCREF(state_py);
   
   PyTuple_SetItem(tuple_inner, 0, mol_no);
   PyTuple_SetItem(tuple_inner, 1, spec);
   PyTuple_SetItem(rv, 0, state_py);
   PyTuple_SetItem(rv, 1, tuple_inner);
   return rv;
}
#endif // USE_PYTHON


#ifdef USE_GUILE
//* \brief 
// Return a list of (list imol chain-id resno ins-code atom-name
// alt-conf) for atom that is closest to the screen centre.  If there
// are multiple models with the same coordinates at the screen centre,
// return the attributes of the atom in the highest number molecule
// number.
// 
SCM active_residue() {

   SCM s = SCM_BOOL(0);
   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();

   if (pp.first) {
      s = SCM_EOL;
      s = scm_cons(scm_makfrom0str(pp.second.second.alt_conf.c_str()) , s);
      s = scm_cons(scm_makfrom0str(pp.second.second.atom_name.c_str()) , s);
      s = scm_cons(scm_makfrom0str(pp.second.second.insertion_code.c_str()) , s);
      s = scm_cons(scm_int2num(pp.second.second.resno) , s);
      s = scm_cons(scm_makfrom0str(pp.second.second.chain.c_str()) , s);
      s = scm_cons(scm_int2num(pp.second.first) ,s);
   } 
   return s;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *active_residue_py() {

   PyObject *s;
   s = Py_False;
   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();

   if (pp.first) {
      s = PyList_New(6);
      PyList_SetItem(s, 0, PyInt_FromLong(pp.second.first));
      PyList_SetItem(s, 1, PyString_FromString(pp.second.second.chain.c_str()));
      PyList_SetItem(s, 2, PyInt_FromLong(pp.second.second.resno));
      PyList_SetItem(s, 3, PyString_FromString(pp.second.second.insertion_code.c_str()));
      PyList_SetItem(s, 4, PyString_FromString(pp.second.second.atom_name.c_str()));
      PyList_SetItem(s, 5, PyString_FromString(pp.second.second.alt_conf.c_str()));
   }
   if (PyBool_Check(s)) {
     Py_INCREF(s);
   }   
   return s;
}
#endif // PYTHON

#ifdef USE_GUILE
SCM closest_atom(int imol) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      coot::at_dist_info_t at_info =
	 graphics_info_t::molecules[imol].closest_atom(g.RotationCentre());
      if (at_info.atom) {
	 r = SCM_EOL;
	 r = scm_cons(scm_double2num(at_info.atom->z), r);
	 r = scm_cons(scm_double2num(at_info.atom->y), r);
	 r = scm_cons(scm_double2num(at_info.atom->x), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->altLoc), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->name), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->GetInsCode()), r);
	 r = scm_cons(scm_int2num(at_info.atom->GetSeqNum()), r);
	 r = scm_cons(scm_makfrom0str(at_info.atom->GetChainID()), r);
	 r = scm_cons(scm_int2num(imol), r);
      }
   }
   return r;
} 
#endif 

#ifdef USE_PYTHON
PyObject *closest_atom_py(int imol) {

   PyObject *r;
   r = Py_False;
   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      coot::at_dist_info_t at_info =
	 graphics_info_t::molecules[imol].closest_atom(g.RotationCentre());
      if (at_info.atom) {
         r = PyList_New(9);
	 PyList_SetItem(r, 0, PyInt_FromLong(imol));
	 PyList_SetItem(r, 1, PyString_FromString(at_info.atom->GetChainID()));
	 PyList_SetItem(r, 2, PyInt_FromLong(at_info.atom->GetSeqNum()));
	 PyList_SetItem(r, 3, PyString_FromString(at_info.atom->GetInsCode()));
	 PyList_SetItem(r, 4, PyString_FromString(at_info.atom->name));
	 PyList_SetItem(r, 5, PyString_FromString(at_info.atom->altLoc));
	 PyList_SetItem(r, 6, PyFloat_FromDouble(at_info.atom->x));
	 PyList_SetItem(r, 7, PyFloat_FromDouble(at_info.atom->y));
	 PyList_SetItem(r, 8, PyFloat_FromDouble(at_info.atom->z));
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif // USE_PYTHON

/*! \brief update the Go To Atom widget entries to atom closest to
  screen centre. */
void update_go_to_atom_from_current_position() {
   
   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();
   if (pp.first) {
      set_go_to_atom_molecule(pp.second.first);
      set_go_to_atom_chain_residue_atom_name(pp.second.second.chain.c_str(),
					     pp.second.second.resno,
					     pp.second.second.atom_name.c_str());
      update_go_to_atom_window_on_other_molecule_chosen(pp.second.first);
   }
}


#ifdef USE_GUILE
SCM generic_string_vector_to_list_internal(const std::vector<std::string> &v) {

   SCM r = SCM_CAR(scm_listofnull);
   for (int i=v.size()-1; i>=0; i--) {
      r = scm_cons(scm_makfrom0str(v[i].c_str()), r);
   }
   return r; 
}
#endif // USE_GUILE

// BL says:: python version 
#ifdef USE_PYTHON
PyObject *generic_string_vector_to_list_internal_py(const std::vector<std::string> &v) {

  PyObject *r;

   r = PyList_New(v.size());
   for (int i=v.size()-1; i>=0; i--) {
      PyList_SetItem(r, i, PyString_FromString(v[i].c_str()));
   }
   return r;
}
#endif // PYTHON

// and the reverse function:
#ifdef USE_GUILE
std::vector<std::string>
generic_list_to_string_vector_internal(SCM l) {
   std::vector<std::string> r;
   SCM l_length_scm = scm_length(l);

#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)

   int l_length = scm_to_int(l_length_scm);
   for (int i=0; i<l_length; i++) {
      SCM le = scm_list_ref(l, SCM_MAKINUM(i));
      std::string s = scm_to_locale_string(le);
      r.push_back(s);
   } 
   
#else
   
   int l_length = gh_scm2int(l_length_scm);
   for (int i=0; i<l_length; i++) {
      SCM le = scm_list_ref(l, SCM_MAKINUM(i));
      std::string s = SCM_STRING_CHARS(le);
      r.push_back(s);
   }

#endif

   return r;
}
#endif

#ifdef USE_PYTHON
std::vector<std::string>
generic_list_to_string_vector_internal_py(PyObject *l) {
   std::vector<std::string> r;

   int l_length = PyObject_Length(l);
   for (int i=0; i<l_length; i++) {
      PyObject *le = PyList_GetItem(l, i);
      std::string s = PyString_AsString(le);
      r.push_back(s);
   } 

   return r;
}
   
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM generic_int_vector_to_list_internal(const std::vector<int> &v) {

   SCM r = SCM_EOL;
   for (int i=v.size()-1; i>=0; i--) {
      r = scm_cons(scm_int2num(v[i]), r);
   }
   return r; 
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *generic_int_vector_to_list_internal_py(const std::vector<int> &v) {

   PyObject *r;
   r = PyList_New(v.size()); 
   for (int i=v.size()-1; i>=0; i--) {
      PyList_SetItem(r, i, PyInt_FromLong(v[i]));
   }
   return r; 
}
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM rtop_to_scm(const clipper::RTop_orth &rtop) {

   SCM r = SCM_EOL;

   SCM tr_list = SCM_EOL;
   SCM rot_list = SCM_EOL;

   clipper::Mat33<double>  mat = rtop.rot();
   clipper::Vec3<double> trans = rtop.trn();

   tr_list = scm_cons(scm_double2num(trans[2]), tr_list);
   tr_list = scm_cons(scm_double2num(trans[1]), tr_list);
   tr_list = scm_cons(scm_double2num(trans[0]), tr_list);

   rot_list = scm_cons(scm_double2num(mat(2,2)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(2,1)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(2,0)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(1,2)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(1,1)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(1,0)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(0,2)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(0,1)), rot_list);
   rot_list = scm_cons(scm_double2num(mat(0,0)), rot_list);

   r = scm_cons(tr_list, r);
   r = scm_cons(rot_list, r);
   return r;

}
#endif // USE_GUILE

#ifdef  USE_GUILE
SCM inverse_rtop_scm(SCM rtop_scm) {

   clipper::RTop_orth r;
   SCM rtop_len_scm = scm_length(rtop_scm);
   int rtop_len = scm_to_int(rtop_len_scm);
   if (rtop_len == 2) {
      SCM rot_scm = scm_list_ref(rtop_scm, SCM_MAKINUM(0));
      SCM rot_length_scm = scm_length(rot_scm);
      int rot_length = scm_to_int(rot_length_scm);
      if (rot_length == 9) {
	 SCM trn_scm = scm_list_ref(rtop_scm, SCM_MAKINUM(1));
	 SCM trn_length_scm = scm_length(trn_scm);
	 int trn_length = scm_to_int(trn_length_scm);
	 double rot_arr[9];
	 double trn_arr[3];
	 if (trn_length == 3) {
	    for (int i=0; i<9; i++) {
	       rot_arr[i] = scm_to_double(scm_list_ref(rot_scm, SCM_MAKINUM(i)));
	    }
	    for (int i=0; i<3; i++) {
	       trn_arr[i] = scm_to_double(scm_list_ref(trn_scm, SCM_MAKINUM(i)));
	    }
	    clipper::Mat33<double> rot(rot_arr[0], rot_arr[1], rot_arr[2],
				       rot_arr[3], rot_arr[4], rot_arr[5],
				       rot_arr[6], rot_arr[7], rot_arr[8]);
	    clipper::Coord_orth trn(trn_arr[0], trn_arr[1], trn_arr[2]);
	    clipper::RTop_orth rtop_in(rot, trn);
	    r = rtop_in.inverse();
	 }
      }
   }
   return rtop_to_scm(r);

} 
#endif // USE_GUILE


#ifdef USE_PYTHON
PyObject *rtop_to_python(const clipper::RTop_orth &rtop) {

   PyObject *r;
   PyObject *tr_list;
   PyObject *rot_list;

   r = PyList_New(2);
   tr_list = PyList_New(3);
   rot_list = PyList_New(9);

   clipper::Mat33<double>  mat = rtop.rot();
   clipper::Vec3<double> trans = rtop.trn();

   PyList_SetItem(tr_list, 0, PyFloat_FromDouble(trans[0]));
   PyList_SetItem(tr_list, 1, PyFloat_FromDouble(trans[1]));
   PyList_SetItem(tr_list, 2, PyFloat_FromDouble(trans[2]));

   PyList_SetItem(rot_list, 0, PyFloat_FromDouble(mat(0,0)));
   PyList_SetItem(rot_list, 1, PyFloat_FromDouble(mat(0,1)));
   PyList_SetItem(rot_list, 2, PyFloat_FromDouble(mat(0,2)));
   PyList_SetItem(rot_list, 3, PyFloat_FromDouble(mat(1,0)));
   PyList_SetItem(rot_list, 4, PyFloat_FromDouble(mat(1,1)));
   PyList_SetItem(rot_list, 5, PyFloat_FromDouble(mat(1,2)));
   PyList_SetItem(rot_list, 6, PyFloat_FromDouble(mat(2,0)));
   PyList_SetItem(rot_list, 7, PyFloat_FromDouble(mat(2,1)));
   PyList_SetItem(rot_list, 8, PyFloat_FromDouble(mat(2,2)));

// BL says:: or maybe the other way round
   PyList_SetItem(r, 0, rot_list);
   PyList_SetItem(r, 1, tr_list);
   return r;

}

PyObject *inverse_rtop_py(PyObject *rtop_py) {

   clipper::RTop_orth r;
   int rtop_len = PyList_Size(rtop_py);
   if (rtop_len == 2) {
      PyObject *rot_py = PyList_GetItem(rtop_py, 0);
      int rot_length = PyList_Size(rot_py);
      if (rot_length == 9) {
	 PyObject *trn_py = PyList_GetItem(rtop_py, 1);
	 int trn_length = PyList_Size(trn_py);
	 double rot_arr[9];
	 double trn_arr[3];
	 if (trn_length == 3) {
	    for (int i=0; i<9; i++) {
	      rot_arr[i] = PyFloat_AsDouble(PyList_GetItem(rot_py, i));
	    }
	    for (int i=0; i<3; i++) {
	      trn_arr[i] = PyFloat_AsDouble(PyList_GetItem(trn_py, i));
	    }
	    clipper::Mat33<double> rot(rot_arr[0], rot_arr[1], rot_arr[2],
				       rot_arr[3], rot_arr[4], rot_arr[5],
				       rot_arr[6], rot_arr[7], rot_arr[8]);
	    clipper::Coord_orth trn(trn_arr[0], trn_arr[1], trn_arr[2]);
	    clipper::RTop_orth rtop_in(rot, trn);
	    r = rtop_in.inverse();
	 }
      }
   }
   return rtop_to_python(r);

}
#endif // USE_PYTHON

#ifdef USE_GUILE
// get the symmetry operators strings for the given molecule
//
/*! \brief Return as a list of strings the symmetry operators of the
  given molecule. If imol is a not a valid molecule, return an empty
  list.*/
// 
SCM get_symmetry(int imol) {

   SCM r = SCM_CAR(scm_listofnull);
   if (is_valid_model_molecule(imol) ||
       is_valid_map_molecule(imol)) {
      std::vector<std::string> symop_list =
	 graphics_info_t::molecules[imol].get_symop_strings();
      r = generic_string_vector_to_list_internal(symop_list);
   }
   return r; 
} 
#endif 

// BL says:: python's get_symmetry:
#ifdef USE_PYTHON
PyObject *get_symmetry_py(int imol) {

   PyObject *r = PyList_New(0);
   if (is_valid_model_molecule(imol) ||
       is_valid_map_molecule(imol)) {
      std::vector<std::string> symop_list =
         graphics_info_t::molecules[imol].get_symop_strings();
      r = generic_string_vector_to_list_internal_py(symop_list);
   }
   return r;
}
#endif //PYTHON


/*! \brief Undo symmetry view. Translate back to main molecule from
  this symmetry position.  */
int undo_symmetry_view() {

   int r=0;

   int imol = first_molecule_with_symmetry_displayed();

   if (is_valid_model_molecule(imol)) {

      graphics_info_t g;
      atom_selection_container_t atom_sel = g.molecules[imol].atom_sel;
      mmdb::Manager *mol = atom_sel.mol;
      float symmetry_search_radius = 1;
      coot::Cartesian screen_centre = g.RotationCentre();
      molecule_extents_t mol_extents(atom_sel, symmetry_search_radius);
      std::vector<std::pair<symm_trans_t, Cell_Translation> > boxes =
	 mol_extents.which_boxes(screen_centre, atom_sel);
      if (boxes.size() > 0) {
	 std::vector<std::pair<clipper::RTop_orth, clipper::Coord_orth> > symm_mat_and_pre_shift_vec;
	 for (unsigned int ibox=0; ibox<boxes.size(); ibox++) {
	    symm_trans_t st = boxes[ibox].first;
	    Cell_Translation pre_shift = boxes[ibox].second;
	    mmdb::mat44 my_matt;
	    int err = atom_sel.mol->GetTMatrix(my_matt, st.isym(), st.x(), st.y(), st.z());
	    if (err == mmdb::SYMOP_Ok) {
	       clipper::RTop_orth rtop_symm = coot::util::make_rtop_orth_from(my_matt);
	       // and we also need an RTop for the pre-shift
	       clipper::Coord_frac pre_shift_cf(pre_shift.us, pre_shift.vs, pre_shift.ws);
	       std::pair<clipper::Cell, clipper::Spacegroup> cs = coot::util::get_cell_symm(mol);
	       clipper::Coord_orth pre_shift_co = pre_shift_cf.coord_orth(cs.first);
	       std::pair<const clipper::RTop_orth, clipper::Coord_orth> p(rtop_symm, pre_shift_co);
	       symm_mat_and_pre_shift_vec.push_back(p);
	    }
	 }
	 // so we have a set of matrices and origins shifts, find the
	 // one that brings us closest to an atom in imol
	 // 
	 g.unapply_symmetry_to_view(imol, symm_mat_and_pre_shift_vec);
      }
   } else {
      std::cout << "WARNING:: No molecule found that was displaying symmetry"
		<< std::endl;
   }
   return r;
}


int first_molecule_with_symmetry_displayed() {

   int imol = -1; // unset
   int n = graphics_n_molecules();
   graphics_info_t g;
   for (int i=0; i<n; i++) {
      if (is_valid_model_molecule(i)) {
	 std::pair<std::vector<float>, std::string> cv =
	    g.molecules[i].get_cell_and_symm();
	 if (cv.first.size() == 6) {
	    if (g.molecules[i].show_symmetry) {
	       imol = i;
	       break;
	    }
	 }
      }
   }
   return imol;
}


void residue_info_apply_all_checkbutton_toggled() {

} 


void apply_residue_info_changes(GtkWidget *widget) {
   graphics_info_t g;
   g.apply_residue_info_changes(widget);
   graphics_draw();
} 

void do_distance_define() {

   std::cout << "Click on 2 atoms: " << std::endl;
   graphics_info_t g;
   g.pick_cursor_maybe();
   g.in_distance_define = 1;
   g.pick_pending_flag = 1;

} 

void do_angle_define() {

   std::cout << "Click on 3 atoms: " << std::endl;
   graphics_info_t g;
   g.pick_cursor_maybe();
   g.in_angle_define = 1;
   g.pick_pending_flag = 1;

} 

void do_torsion_define() {

   std::cout << "Click on 4 atoms: " << std::endl;
   graphics_info_t g;
   g.pick_cursor_maybe();
   g.in_torsion_define = 1;
   g.pick_pending_flag = 1;

} 

void clear_simple_distances() {
   graphics_info_t g;
   g.clear_simple_distances();
   g.normal_cursor();
   std::string cmd = "clear-simple-distances";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
}

void clear_last_simple_distance() { 

   graphics_info_t g;
   g.clear_last_simple_distance();
   g.normal_cursor();
   std::string cmd = "clear-last-simple-distance";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
} 


void clear_residue_info_edit_list() {

   graphics_info_t::residue_info_edits->resize(0);
   std::string cmd = "clear-residue-info-edit-list";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
}


/*  ----------------------------------------------------------------------- */
/*                  residue environment                                      */
/*  ----------------------------------------------------------------------- */
void fill_environment_widget(GtkWidget *widget) {

   GtkWidget *entry;
   char *text = (char *) malloc(100);
   graphics_info_t g;

   entry = lookup_widget(widget, "environment_distance_min_entry");
   snprintf(text, 99, "%-5.1f", g.environment_min_distance);
   gtk_entry_set_text(GTK_ENTRY(entry), text);
   
   entry = lookup_widget(widget, "environment_distance_max_entry");
   snprintf(text, 99, "%-5.1f" ,g.environment_max_distance);
   gtk_entry_set_text(GTK_ENTRY(entry), text);
   free(text);

   GtkWidget *toggle_button;
   toggle_button = lookup_widget(widget, "environment_distance_checkbutton");
   
   if (g.environment_show_distances == 1) {
      // we have to (temporarily) set the flag to 0 because the
      // set_active creates an event which causes
      // toggle_environment_show_distances to run (and thus turn off
      // distances if they were allowed to remain here at 1 (on).
      // Strange but true.
      g.environment_show_distances = 0;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), 1);
      // std::cout << "filling: button is active" << std::endl;
   } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), 0);
      // std::cout << "filling: button is inactive" << std::endl;
   }
   // set the label button
   toggle_button = lookup_widget(widget, 
                                 "environment_distance_label_atom_checkbutton");
   if (g.environment_distance_label_atom) {
     gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), 1);
   } else {
     gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button), 0);
   }
}

// Called when the OK button of the environment distances dialog is clicked 
// (just before it is destroyed).
// 
void execute_environment_settings(GtkWidget *widget) {
   
   GtkWidget *entry;
   float val;
   graphics_info_t g;

   entry = lookup_widget(widget, "environment_distance_min_entry");
   const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
   val = atof(text);
   if (val < 0 || val > 1000) {
      g.environment_min_distance = 2.2;
      std::cout <<  "nonsense value for limit using "
		<< g.environment_min_distance << std::endl;
   } else {
      g.environment_min_distance = val;
   } 

   entry = lookup_widget(widget, "environment_distance_max_entry");
   text = gtk_entry_get_text(GTK_ENTRY(entry));
   val = atof(text);
   if (val < 0 || val > 1000) {
      g.environment_max_distance = 3.2;
      std::cout <<  "nonsense value for limit using "
		<< g.environment_max_distance << std::endl;
   } else {
      g.environment_max_distance = val;
   }

   if (g.environment_max_distance < g.environment_min_distance) {
      float tmp = g.environment_max_distance;
      g.environment_max_distance = g.environment_min_distance;
      g.environment_min_distance = tmp;
   }

   GtkWidget *label_check_button;
   label_check_button = lookup_widget(widget, "environment_distance_label_atom_checkbutton");
   if (GTK_TOGGLE_BUTTON(label_check_button)->active) {
      g.environment_distance_label_atom = 1;
   }

   // not sure that this is necessary now that the toggle function is
   // active
   std::pair<int, int> r =  g.get_closest_atom();
   if (r.first >= 0) { 
      g.mol_no_for_environment_distances = r.second;
      g.update_environment_distances_maybe(r.first, r.second);
   }
   graphics_draw();
}

void set_show_environment_distances(int state) {

   graphics_info_t g;
   g.environment_show_distances = state;
   if (state) {
      std::pair<int, int> r =  g.get_closest_atom();
      if (r.first >= 0) {
	 g.mol_no_for_environment_distances = r.second;
	 g.update_environment_distances_maybe(r.first, r.second);
      }
   }
   graphics_draw();
}

void set_show_environment_distances_bumps(int state) {
   graphics_info_t g;
   std::pair<int, int> r =  g.get_closest_atom();
   g.environment_distances_show_bumps = state;
   g.update_environment_distances_maybe(r.first, r.second);
   graphics_draw();
}

void set_show_environment_distances_h_bonds(int state) {
   graphics_info_t g;
   std::pair<int, int> r =  g.get_closest_atom();
   g.environment_distances_show_h_bonds = state;
   g.update_environment_distances_maybe(r.first, r.second);
   graphics_draw();
}


int show_environment_distances_state() {
   return graphics_info_t::environment_show_distances;
}

/*! \brief min and max distances for the environment distances */
void set_environment_distances_distance_limits(float min_dist, float max_dist) {

   graphics_info_t::environment_min_distance = min_dist;
   graphics_info_t::environment_max_distance = max_dist;
}

void set_show_environment_distances_as_solid(int state) {
   graphics_info_t::display_environment_graphics_object_as_solid_flag = state;
} 

void set_environment_distances_label_atom(int state) {
  graphics_info_t::environment_distance_label_atom = state;
}

double
add_geometry_distance(int imol_1, float x_1, float y_1, float z_1, int imol_2, float x_2, float y_2, float z_2) {

   graphics_info_t g;
   double d = g.display_geometry_distance_symm(imol_1, coot::Cartesian(x_1, y_1, z_1),
					       imol_2, coot::Cartesian(x_2, y_2, z_2));
   return d;
} 

#ifdef USE_GUILE
double
add_atom_geometry_distance_scm(int imol_1, SCM atom_spec_1, int imol_2, SCM atom_spec_2) {

   double d = -1; 
   if (is_valid_model_molecule(imol_1)) {
      if (is_valid_model_molecule(imol_2)) {
	 
	 graphics_info_t g;
	 coot::atom_spec_t spec_1 = atom_spec_from_scm_expression(atom_spec_1);
	 coot::atom_spec_t spec_2 = atom_spec_from_scm_expression(atom_spec_2);
	 mmdb::Atom *at_1 = g.molecules[imol_1].get_atom(spec_1);
	 mmdb::Atom *at_2 = g.molecules[imol_2].get_atom(spec_2);
	 if (! at_1) {
	    std::cout << "WARNING:: atom not found from spec " << spec_1 << std::endl;
	 } else { 
	    if (! at_2) {
	       std::cout << "WARNING:: atom not found from spec " << spec_2 << std::endl;
	    } else {
	       // happy path
	       coot::Cartesian pos_1(at_1->x, at_1->y, at_1->z);
	       coot::Cartesian pos_2(at_2->x, at_2->y, at_2->z);
	       d = g.display_geometry_distance_symm(imol_1, pos_1, imol_2, pos_2);
	       std::cout << "Distance: " << spec_1 << " to " << spec_2 << " is " << d << " A" << std::endl;
	    }
	 }
      }
   }
   return d; 
} 
#endif

#ifdef USE_PYTHON
double add_atom_geometry_distance_py(int imol_1, PyObject *atom_spec_1, int imol_2, PyObject *atom_spec_2) {

   double d = -1;
   if (is_valid_model_molecule(imol_1)) {
      if (is_valid_model_molecule(imol_2)) {
	 
	 graphics_info_t g;
	 coot::atom_spec_t spec_1 = atom_spec_from_python_expression(atom_spec_1);
	 coot::atom_spec_t spec_2 = atom_spec_from_python_expression(atom_spec_2);
	 mmdb::Atom *at_1 = g.molecules[imol_1].get_atom(spec_1);
	 mmdb::Atom *at_2 = g.molecules[imol_2].get_atom(spec_2);
	 if (! at_1) {
	    std::cout << "WARNING:: atom not found from spec " << spec_1 << std::endl;
	 } else { 
	    if (! at_2) {
	       std::cout << "WARNING:: atom not found from spec " << spec_2 << std::endl;
	    } else {
	       // happy path
	       coot::Cartesian pos_1(at_1->x, at_1->y, at_1->z);
	       coot::Cartesian pos_2(at_2->x, at_2->y, at_2->z);
	       d = g.display_geometry_distance_symm(imol_1, pos_1, imol_2, pos_2);
	       std::cout << "Distance: " << spec_1 << " to " << spec_2 << " is " << d << " A" << std::endl;
	    }
	 }
      }
   }
   return d; 
} 
#endif




void set_show_pointer_distances(int istate) {

   // Use the graphics_info_t's pointer min and max dist

   std::cout << "in set_show_pointer_distances: state: "
	     << istate << std::endl;
   
   if (istate == 0) { 
      graphics_info_t::show_pointer_distances_flag = 0;
      graphics_info_t g;
      g.clear_pointer_distances();
   } else {
      graphics_info_t::show_pointer_distances_flag = 1;
      graphics_info_t g;
      // std::cout << "in set_show_pointer_distances: making distances.." << std::endl;
      g.make_pointer_distance_objects();
      // std::cout << "in set_show_pointer_distances: done making distances.." << std::endl;
   }
   graphics_draw();
   graphics_info_t::residue_info_edits->resize(0);
   std::string cmd = "set-show-pointer-distances";
   std::vector<coot::command_arg_t> args;
   args.push_back(istate);
   add_to_history_typed(cmd, args);
}


/*! \brief show the state of display of the  pointer distances  */
int  show_pointer_distances_state() {

   return graphics_info_t::show_pointer_distances_flag;
}



void fill_single_map_properties_dialog(GtkWidget *window, int imol) { 

   GtkWidget *cell_text = lookup_widget(window, "single_map_properties_cell_text");
   GtkWidget *spgr_text = lookup_widget(window, "single_map_properties_sg_text");

   std::string cell_text_string;
   std::string spgr_text_string;

   cell_text_string = graphics_info_t::molecules[imol].cell_text_with_embeded_newline();
   spgr_text_string = "   ";
   spgr_text_string += graphics_info_t::molecules[imol].xmap.spacegroup().descr().symbol_hm();
   spgr_text_string += "  [";
   spgr_text_string += graphics_info_t::molecules[imol].xmap.spacegroup().descr().symbol_hall();
   spgr_text_string += "]";

   gtk_label_set_text(GTK_LABEL(cell_text), cell_text_string.c_str());
   gtk_label_set_text(GTK_LABEL(spgr_text), spgr_text_string.c_str());

   // And now the map rendering style: transparent surface or standard lines:
   GtkWidget *rb_1  = lookup_widget(window, "displayed_map_style_as_lines_radiobutton");
   GtkWidget *rb_2  = lookup_widget(window, "displayed_map_style_as_cut_glass_radiobutton");
   GtkWidget *rb_3  = lookup_widget(window, "displayed_map_style_as_transparent_radiobutton");
   GtkWidget *scale = lookup_widget(window, "map_opacity_hscale");
   

   graphics_info_t g;
   if (g.molecules[imol].draw_it_for_solid_density_surface) {
      if (g.do_flat_shading_for_solid_density_surface)
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_2), TRUE);
      else 
	 gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_3), TRUE);
   } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb_1), TRUE);
   }
   
   GtkAdjustment *adjustment = gtk_range_get_adjustment(GTK_RANGE(scale));
   float op = g.molecules[imol].density_surface_opacity;
   gtk_adjustment_set_value(adjustment, 100.0*op);
   
} 

/*  ----------------------------------------------------------------------- */
/*                  miguels orientation axes matrix                         */
/*  ----------------------------------------------------------------------- */

void
set_axis_orientation_matrix(float m11, float m12, float m13,
			    float m21, float m22, float m23,
			    float m31, float m32, float m33) {

   graphics_info_t::axes_orientation =
      GL_matrix(m11, m12, m13, m21, m22, m23, m31, m32, m33);

   std::string cmd = "set-axis-orientation-matrix";
   std::vector<coot::command_arg_t> args;
   args.push_back(m11);
   args.push_back(m12);
   args.push_back(m12);
   args.push_back(m21);
   args.push_back(m22);
   args.push_back(m23);
   args.push_back(m31);
   args.push_back(m32);
   args.push_back(m33);
   add_to_history_typed(cmd, args);
}

void
set_axis_orientation_matrix_usage(int state) {

   graphics_info_t::use_axes_orientation_matrix_flag = state;
   graphics_draw();
   std::string cmd = "set-axis-orientation-matrix-usage";
   std::vector<coot::command_arg_t> args;
   args.push_back(state);
   add_to_history_typed(cmd, args);

} 



/*  ----------------------------------------------------------------------- */
/*                  dynamic map                                             */
/*  ----------------------------------------------------------------------- */
void toggle_dynamic_map_sampling() {
   graphics_info_t g;
   // std::cout << "toggling from " << g.dynamic_map_resampling << std::endl;
   if (g.dynamic_map_resampling) {
      g.dynamic_map_resampling = 0;
   } else {
      g.dynamic_map_resampling = 1;
   }
   std::string cmd = "toggle-dynamic-map-sampling";
   std::vector<coot::command_arg_t> args;
   add_to_history_typed(cmd, args);
}


void toggle_dynamic_map_display_size() {
   graphics_info_t g;
   // std::cout << "toggling from " << g.dynamic_map_size_display << std::endl;
   if (g.dynamic_map_size_display) {
      g.dynamic_map_size_display = 0;
   } else {
      g.dynamic_map_size_display = 1;
   }
} 

void   set_map_dynamic_map_sampling_checkbutton(GtkWidget *checkbutton) {

   graphics_info_t g;
   if (g.dynamic_map_resampling) {
      g.dynamic_map_resampling = 0;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 1);
   }
}

void
set_map_dynamic_map_display_size_checkbutton(GtkWidget *checkbutton) {

   graphics_info_t g;
   if (g.dynamic_map_size_display) {
      g.dynamic_map_size_display = 0; 
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), 1);
   }
} 

void
set_dynamic_map_size_display_on() {
   graphics_info_t::dynamic_map_size_display = 1;
} 
void
set_dynamic_map_size_display_off() {
   graphics_info_t::dynamic_map_size_display = 0;
} 
int
get_dynamic_map_size_display(){
   int ret = graphics_info_t::dynamic_map_size_display;
   return ret;
}
void
set_dynamic_map_sampling_on() {
   graphics_info_t::dynamic_map_resampling = 1;
}
void
set_dynamic_map_sampling_off() {
   graphics_info_t::dynamic_map_resampling = 0;
}
int
get_dynamic_map_sampling(){
   int ret = graphics_info_t::dynamic_map_resampling;
   return ret;
}

void
set_dynamic_map_zoom_offset(int i) {
   graphics_info_t::dynamic_map_zoom_offset = i;
} 




/*  ------------------------------------------------------------------------ */
/*                         history                                           */
/*  ------------------------------------------------------------------------ */



void add_to_history(const std::vector<std::string> &command_strings) {
   // something
   graphics_info_t g;
   g.add_history_command(command_strings);

   if (g.console_display_commands.display_commands_flag) { 

      char esc = 27;
      // std::string esc = "esc";
      if (g.console_display_commands.hilight_flag) {
	// std::cout << esc << "[34m";
#ifdef WINDOWS_MINGW
	// use the console cursor infot to distinguish between DOS and MSYS
	// shell
	CONSOLE_CURSOR_INFO ConCurInfo;
	if (GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ConCurInfo)) {
	  // we have a DOS shell
	  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				  FOREGROUND_RED | 
				  FOREGROUND_GREEN | 
				  FOREGROUND_BLUE |
				  FOREGROUND_INTENSITY);
	} else {
	  // we have MSYS (or whatever else shell)
	  std::cout << esc << "[1m";
	}
#else
	 std::cout << esc << "[1m";
#endif // MINGW
      } else {
	 std::cout << "INFO:: Command: ";
      }

      // Make it colourful?
      if (g.console_display_commands.hilight_colour_flag) {
#ifdef WINDOWS_MINGW
	CONSOLE_CURSOR_INFO ConCurInfo;
	if (GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ConCurInfo)) {
	  // we have a DOS shell
	  switch (g.console_display_commands.colour_prefix) {
	  case(1):
	    // red
	    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_RED);
	    break;
	  case(2):
	    // green
	    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_GREEN);
	    break;
	  case(3):
	    // yellow
	    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_RED |
				    FOREGROUND_GREEN);
	    break;
	  case(4):
	    // blue
	    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_BLUE);
	    break;
	  case(5):
	    // magenta
	    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_RED |
				    FOREGROUND_BLUE);
	    break;
	  case(6):
	    // cyan
	    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_GREEN |
				    FOREGROUND_BLUE);
	    break;
	  default:
	    //white
	    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				    FOREGROUND_RED | 
				    FOREGROUND_GREEN | 
				    FOREGROUND_BLUE);
	  }
	} else {
	  // MSYS shell
	 std::cout << esc << "[3"
		   << g.console_display_commands.colour_prefix << "m";
	}
#else
	 std::cout << esc << "[3"
		   << g.console_display_commands.colour_prefix << "m";
#endif // MINGW
      }

#if defined USE_GUILE && !defined WINDOWS_MINGW
      std::cout << graphics_info_t::schemize_command_strings(command_strings);
#else
#ifdef USE_PYTHON
      std::cout << graphics_info_t::pythonize_command_strings(command_strings);
#endif // USE_PYTHON
#endif // USE_GUILE/MINGW
      
      if (g.console_display_commands.hilight_flag) {// hilight off
#ifdef WINDOWS_MINGW
	CONSOLE_CURSOR_INFO ConCurInfo;
	if (GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ConCurInfo)) {
	  // we have a DOS shell (reset to white)
	  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
				  FOREGROUND_RED | 
				  FOREGROUND_GREEN | 
				  FOREGROUND_BLUE);
	} else {
	  // MSYS shell
	 std::cout << esc << "[0m"; // reset
	}
#else
	 std::cout << esc << "[0m"; // reset
	 //std::cout << esc; // reset
#endif // MINGW
      }
      std::cout << std::endl;
   }

#ifdef USE_MYSQL_DATABASE

   add_to_database(command_strings);
#endif
}

void add_to_history_typed(const std::string &command,
			  const std::vector<coot::command_arg_t> &args) {

   std::vector<std::string> command_strings;

   command_strings.push_back(command);
   for (unsigned int i=0; i<args.size(); i++)
      command_strings.push_back(args[i].as_string());

   add_to_history(command_strings);
}

void add_to_history_simple(const std::string &s) {

   std::vector<std::string> command_strings;
   command_strings.push_back(s);
   add_to_history(command_strings);
} 

std::string
single_quote(const std::string &s) {
   std::string r("\"");
   r += s;
   r += "\"";
   return r;
} 

std::string pythonize_command_name(const std::string &s) {

   std::string ss;
   for (unsigned int i=0; i<s.length(); i++) {
      if (s[i] == '-') {
	 ss += '_';
      } else {
	 ss += s[i];	 
      }
   }
   return ss;
} 

std::string schemize_command_name(const std::string &s) {

   std::string ss;
   for (unsigned int i=0; i<s.length(); i++) {
      if (s[i] == '_') {
	 ss += '-';
      } else {
	 ss += s[i];	 
      }
   }
   return ss;
}

#ifdef USE_MYSQL_DATABASE
int db_query_insert(const std::string &insert_string) {

   int v = -1;
   if (graphics_info_t::mysql) {

      time_t timep = time(0);
      long int li = timep;
      
      clipper::String query("insert into session");
      query += " (userid, sessionid, command, commandnumber, timeasint)";
      query += " values ('";
      query += graphics_info_t::db_userid_username.first;
      query += "', '";
      query += graphics_info_t::sessionid;
      query += "', '";
      query += insert_string;
      query += "', ";
      query += graphics_info_t::int_to_string(graphics_info_t::query_number);
      query += ", ";
      query += coot::util::long_int_to_string(li);
      query += ") ; ";

//       query = "insert into session ";
//       query += "(userid, sessionid, command, commandnumber) values ";
//       query += "('pemsley', 'sesh', 'xxx', ";
//       query += graphics_info_t::int_to_string(graphics_info_t::query_number);
//       query += ") ;";
      
//       std::cout << "query: " << query << std::endl;
      unsigned long length = query.length();
      v = mysql_real_query(graphics_info_t::mysql, query.c_str(), length);
      if (v != 0) {
	 if (v == CR_COMMANDS_OUT_OF_SYNC)
	    std::cout << "WARNING:: MYSQL Commands executed in an"
		      << " improper order" << std::endl;
	 if (v == CR_SERVER_GONE_ERROR) 
	    std::cout << "WARNING:: MYSQL Server gone!"
		      << std::endl;
	 if (v == CR_SERVER_LOST) 
	    std::cout << "WARNING:: MYSQL Server lost during query!"
		      << std::endl;
	 if (v == CR_UNKNOWN_ERROR) 
	    std::cout << "WARNING:: MYSQL Server transaction had "
		      << "an uknown error!" << std::endl;
	 std::cout << "history: mysql_real_query returned " << v
		   << std::endl;
      }
      graphics_info_t::query_number++;
   }
   return v;
}
#endif // USE_MYSQL_DATABASE

void add_to_database(const std::vector<std::string> &command_strings) {

#ifdef USE_MYSQL_DATABASE
   std::string insert_string =
      graphics_info_t::schemize_command_strings(command_strings);
   db_query_insert(insert_string);
#endif // USE_MYSQL_DATABASE

}


#ifdef USE_MYSQL_DATABASE
// 
void db_finish_up() {

   db_query_insert(";#finish");

} 
#endif // USE_MYSQL_DATABASE


/*  ----------------------------------------------------------------------- */
/*                         History/scripting                                */
/*  ----------------------------------------------------------------------- */

void print_all_history_in_scheme() {

   graphics_info_t g;
   std::vector<std::vector<std::string> > ls = g.history_list.history_list();
   for (unsigned int i=0; i<ls.size(); i++)
      std::cout << i << "  " << graphics_info_t::schemize_command_strings(ls[i]) << "\n";

   add_to_history_simple("print-all-history-in-scheme");

}

void print_all_history_in_python() {

   graphics_info_t g;
   std::vector<std::vector<std::string> > ls = g.history_list.history_list();
   for (unsigned int i=0; i<ls.size(); i++)
      std::cout << i << "  " << graphics_info_t::pythonize_command_strings(ls[i]) << "\n";
   add_to_history_simple("print-all-history-in-python");
}

/*! \brief set a flag to show the text command equivalent of gui
  commands in the console as they happen. 

  1 for on, 0 for off. */
void set_console_display_commands_state(short int istate) {

   graphics_info_t::console_display_commands.display_commands_flag = istate;
}

void set_console_display_commands_hilights(short int bold_flag, short int colour_flag, int colour_index) {

   graphics_info_t g;
   g.console_display_commands.hilight_flag = bold_flag;
   g.console_display_commands.hilight_colour_flag = colour_flag;
   g.console_display_commands.colour_prefix = colour_index;
}



std::string languagize_command(const std::vector<std::string> &command_parts) {

   short int language = 0; 
#ifdef USE_PYTHON
#ifdef USE_GUILE
   language = coot::STATE_SCM;
#else   
   language = coot::STATE_PYTHON;
#endif
#endif

#ifdef USE_GUILE
   language = 1;
#endif

   std::string s;
   if (language) {
      if (language == coot::STATE_PYTHON)
	 s = graphics_info_t::pythonize_command_strings(command_parts);
      if (language == coot::STATE_SCM)
	 s = graphics_info_t::schemize_command_strings(command_parts);
   }
   return s; 
}


/*  ------------------------------------------------------------------------ */
/*                     state (a graphics_info thing)                         */
/*  ------------------------------------------------------------------------ */

void
save_state() {
   graphics_info_t g;
   g.save_state();
   add_to_history_simple("save-state");
}

void
save_state_file(const char *filename) {

   graphics_info_t g;
   g.save_state_file(std::string(filename));
   std::string cmd = "save-state-file";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(filename));
   add_to_history_typed(cmd, args);
}

/*! \brief save the current state to file filename */
void save_state_file_py(const char *filename) {
   graphics_info_t g;
   g.save_state_file(std::string(filename), coot::PYTHON_SCRIPT);
   std::string cmd = "save-state-file";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(filename));
   add_to_history_typed(cmd, args);
} 



#ifdef USE_GUILE
/* Return the default file name suggestion (that would come up in the
   save coordinates dialog) or scheme faalse if imol is not a valid
   model molecule. */
SCM save_coords_name_suggestion_scm(int imol) {

   SCM r = SCM_BOOL_F;

   if (is_valid_model_molecule(imol)) {
      std::string s = graphics_info_t::molecules[imol].stripped_save_name_suggestion();
      r = scm_from_locale_string(s.c_str());
   }
   return r;
} 
#endif /*  USE_GUILE */


#ifdef USE_PYTHON
PyObject *save_coords_name_suggestion_py(int imol) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      std::string s = graphics_info_t::molecules[imol].stripped_save_name_suggestion();
      r = PyString_FromString(s.c_str());
   }

   return r;
} 
#endif /*  USE_PYTHON */



/*  ------------------------------------------------------------------------ */
/*                     resolution                                            */
/*  ------------------------------------------------------------------------ */

/* Return negative number on error, otherwise resolution in A (eg. 2.0) */
float data_resolution(int imol) {

   float r = -1;
   if (is_valid_map_molecule(imol)) {
      r = graphics_info_t::molecules[imol].data_resolution();
   }
   std::string cmd = "data-resolution";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   add_to_history_typed(cmd, args);
   return r;
}

/*! \brief return the resolution set in the header of the
  model/coordinates file.  If this number is not available, return a
  number less than 0.  */
float model_resolution(int imol) {

   float r = -1;
   if (is_valid_model_molecule(imol)) {
      r = graphics_info_t::molecules[imol].atom_sel.mol->GetResolution();
   } 
   return r;
} 


/*  ------------------------------------------------------------------------ */
/*                     resolution                                            */
/*  ------------------------------------------------------------------------ */

void solid_surface(int imap, short int on_off_flag) {

   if (is_valid_map_molecule(imap)) {
      graphics_info_t::molecules[imap].do_solid_surface_for_density(on_off_flag);
   }
   graphics_draw();
} 



/*  ------------------------------------------------------------------------ */
/*                     residue exists?                                       */
/*  ------------------------------------------------------------------------ */

int does_residue_exist_p(int imol, char *chain_id, int resno, char *inscode) {

   int istate = 0;
   if (is_valid_model_molecule(imol)) {
      istate = graphics_info_t::molecules[imol].does_residue_exist_p(std::string(chain_id),
								     resno,
								     std::string(inscode));
   }
   std::string cmd = "does-residue-exist-p";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol);
   args.push_back(chain_id);
   args.push_back(resno);
   args.push_back(inscode);
   add_to_history_typed(cmd, args);
   return istate;
}

/*  ------------------------------------------------------------------------ */
/*                         Parameters from map:                              */
/*  ------------------------------------------------------------------------ */
/*! \brief return the mtz file that was use to generate the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
// Caller should dispose of returned pointer.
const char *mtz_hklin_for_map(int imol_map) {

   std::string mtz;

   if (is_valid_map_molecule(imol_map)) {
      mtz = graphics_info_t::molecules[imol_map].save_mtz_file_name;
   }
   std::string cmd = "mtz-hklin-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   const char *s = strdup(mtz.c_str());
   return s;
}

/*! \brief return the FP column in the file that was use to generate
  the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
// Caller should dispose of returned pointer.
const char *mtz_fp_for_map(int imol_map) {

   std::string fp;
   if (is_valid_map_molecule(imol_map)) {
      fp = graphics_info_t::molecules[imol_map].save_f_col;
   }
   std::string cmd = "mtz-fp-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   const char *s = strdup(fp.c_str());
   return s;
} 

/*! \brief return the phases column in mtz file that was use to generate
  the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say). */
// Caller should dispose of returned pointer.
const char *mtz_phi_for_map(int imol_map) {

   std::string phi;
   if (is_valid_map_molecule(imol_map)) {
      phi = graphics_info_t::molecules[imol_map].save_phi_col;
   }
   std::string cmd = "mtz-phi-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   const char *s = strdup(phi.c_str());
   return s;
   
}

/*! \brief return the weight column in the mtz file that was use to
  generate the map

  return 0 when there is no mtz file associated with that map (it was
  generated from a CCP4 map file say) or no weights were used. */
// Caller should dispose of returned pointer.
const char *mtz_weight_for_map(int imol_map) {

   std::string weight;
   if (is_valid_map_molecule(imol_map)) {
      weight = graphics_info_t::molecules[imol_map].save_weight_col;
   }
   std::string cmd = "mtz-weight-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   const char *s = strdup(weight.c_str());
   return s;
}

/*! \brief return flag for whether weights were used that was use to
  generate the map

  return 0 when no weights were used or there is no mtz file
  associated with that map. */
short int mtz_use_weight_for_map(int imol_map) {

   short int i = 0;
   if (is_valid_map_molecule(imol_map)) {
      i = graphics_info_t::molecules[imol_map].save_use_weights;
   }
   std::string cmd = "mtz-use-weight-for-map";
   std::vector<coot::command_arg_t> args;
   args.push_back(imol_map);
   add_to_history_typed(cmd, args);
   return i;
}

// return #f or ("xxx.mtz" "FPH" "PHWT" "" #f)
//
#ifdef USE_GUILE
SCM map_parameters_scm(int imol) {

   SCM r = SCM_BOOL_F;
   if (is_valid_map_molecule(imol)) {
      r = SCM_EOL;
      if (graphics_info_t::molecules[imol].save_use_weights)
	 r = scm_cons(SCM_BOOL_T, r);
      else 
	 r = scm_cons(SCM_BOOL_F, r);
      r = scm_cons(scm_makfrom0str(graphics_info_t::molecules[imol].save_weight_col.c_str()), r);
      r = scm_cons(scm_makfrom0str(graphics_info_t::molecules[imol].save_phi_col.c_str()), r);
      r = scm_cons(scm_makfrom0str(graphics_info_t::molecules[imol].save_f_col.c_str()), r);
      r = scm_cons(scm_makfrom0str(graphics_info_t::molecules[imol].save_mtz_file_name.c_str()), r);
   }
   return r;
}
#endif // USE_GUILE

// return False or ["xxx.mtz", "FPH", "PHWT", "", False]
//
#ifdef USE_PYTHON
PyObject *map_parameters_py(int imol) {

   PyObject *r = Py_False;
   if (is_valid_map_molecule(imol)) {
      r = PyList_New(5);
      PyList_SetItem(r, 0, PyString_FromString(graphics_info_t::molecules[imol].save_mtz_file_name.c_str()));
      PyList_SetItem(r, 1, PyString_FromString(graphics_info_t::molecules[imol].save_f_col.c_str()));
      PyList_SetItem(r, 2, PyString_FromString(graphics_info_t::molecules[imol].save_phi_col.c_str()));
      PyList_SetItem(r, 3, PyString_FromString(graphics_info_t::molecules[imol].save_weight_col.c_str()));
      if (graphics_info_t::molecules[imol].save_use_weights) {
        Py_INCREF(Py_True);
        PyList_SetItem(r, 4, Py_True);
      } else {
        Py_INCREF(Py_False);
        PyList_SetItem(r, 4, Py_False);
      }
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // USE_PYTHON

#ifdef USE_GUILE
// return #f or (45 46 47 90 90 120), angles in degrees.
SCM cell_scm(int imol) {
   SCM r = SCM_BOOL_F;
   if (is_valid_map_molecule(imol) || (is_valid_model_molecule(imol))) {
      std::pair<bool, clipper::Cell> cell = graphics_info_t::molecules[imol].cell();
      if (cell.first) { 
	 r = SCM_EOL;
	 r = scm_cons(scm_double2num(clipper::Util::rad2d(cell.second.descr().gamma())), r);
	 r = scm_cons(scm_double2num(clipper::Util::rad2d(cell.second.descr().beta() )), r);
	 r = scm_cons(scm_double2num(clipper::Util::rad2d(cell.second.descr().alpha())), r);
	 r = scm_cons(scm_double2num(cell.second.descr().c()), r);
	 r = scm_cons(scm_double2num(cell.second.descr().b()), r);
	 r = scm_cons(scm_double2num(cell.second.descr().a()), r);
      }
   } 
   return r;
} 
#endif // USE_GUILE



#ifdef USE_PYTHON
// return False or [45, 46, 47, 90, 90, 120), angles in degrees.
PyObject *cell_py(int imol) {
   PyObject *r = Py_False;
   if (is_valid_map_molecule(imol) || (is_valid_model_molecule(imol))) {
      std::pair<bool, clipper::Cell> cell = graphics_info_t::molecules[imol].cell();
      if (cell.first) { 
	 r = PyList_New(6);
	 PyList_SetItem(r, 0, PyFloat_FromDouble(cell.second.descr().a()));
	 PyList_SetItem(r, 1, PyFloat_FromDouble(cell.second.descr().b()));
	 PyList_SetItem(r, 2, PyFloat_FromDouble(cell.second.descr().c()));
	 PyList_SetItem(r, 3, PyFloat_FromDouble(clipper::Util::rad2d(cell.second.descr().alpha())));
	 PyList_SetItem(r, 4, PyFloat_FromDouble(clipper::Util::rad2d(cell.second.descr().beta())));
	 PyList_SetItem(r, 5, PyFloat_FromDouble(clipper::Util::rad2d(cell.second.descr().gamma())));
      }
   } 
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
} 
#endif // USE_PYTHON



/*! \brief Put text at x,y,z  */
// use should be given access to colour and size.
int place_text(const char *text, float x, float y, float z, int size) {

   int handle = graphics_info_t::generic_texts_p->size();
   std::string s(text); 
   coot::generic_text_object_t o(s, handle, x, y, z); 
   graphics_info_t::generic_texts_p->push_back(o);
   //   return graphics_info_t::generic_text->size() -1; // the index of the
	  					    // thing we just
						    // pushed.
   std::string cmd = "place-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(text);
   args.push_back(x);
   args.push_back(y);
   args.push_back(z);
   args.push_back(size);
   add_to_history_typed(cmd, args);
   graphics_draw();

   return handle; // same value as above.
} 

void remove_text(int text_handle) {

   std::vector<coot::generic_text_object_t>::iterator it;
   for (it = graphics_info_t::generic_texts_p->begin();
	it != graphics_info_t::generic_texts_p->end();
	it++) {
      if (it->handle == text_handle) {
	 graphics_info_t::generic_texts_p->erase(it);
	 break;
      }
   }
   std::string cmd = "remove-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(text_handle);
   add_to_history_typed(cmd, args);
   graphics_draw();
}


void edit_text(int text_handle, const char *str) {

   graphics_info_t g;
   if (str) { 
      if (text_handle >= 0) {
	 if (text_handle < g.generic_texts_p->size()) {
	    (*g.generic_texts_p)[text_handle].s = str;
	 }
      }
   }
   std::string cmd = "edit-text";
   std::vector<coot::command_arg_t> args;
   args.push_back(text_handle);
   args.push_back(str);
   add_to_history_typed(cmd, args);
   graphics_draw();
}

/*! \brief return the closest text that is with r A of the given
  position.  If no text item is close, then return -1 */
int text_index_near_position(float x, float y, float z, float rad) {

   int r = -1;
   graphics_info_t g;
   double best_dist = 999999999.9; // not (long) integer, conversion to double problems in GCC 4.1.2

   std::cout << "size: " << g.generic_texts_p->size() << std::endl;
   
   for (unsigned int i=0; i<g.generic_texts_p->size(); i++) {
      std::cout << "i " << i << std::endl;
      clipper::Coord_orth p1(x,y,z);
      clipper::Coord_orth p2((*g.generic_texts_p)[i].x,
			     (*g.generic_texts_p)[i].y,
			     (*g.generic_texts_p)[i].z);
      double d = (p1-p2).lengthsq();
      std::cout << "   d " << d  << std::endl;
      if (d < rad*rad) { 
	 if (d < best_dist) {
	    best_dist = d;
	    r = i;
	 }
      }
   }
   return r;
}


/*  ----------------------------------------------------------------------- */
/*                         Dictionaries                                     */
/*  ----------------------------------------------------------------------- */
/*! \brief return a list of all the dictionaries read */

#ifdef USE_GUILE
SCM dictionaries_read() {

   return generic_string_vector_to_list_internal(*graphics_info_t::cif_dictionary_filename_vec);
}
#endif

// BL says:: python's func.
#ifdef USE_PYTHON
PyObject *dictionaries_read_py() {

   return generic_string_vector_to_list_internal_py(*graphics_info_t::cif_dictionary_filename_vec);
}
#endif // PYTHON

std::vector<std::string>
dictionary_entries() {
   graphics_info_t g;
   return g.Geom_p()->monomer_restraints_comp_ids();
} 

#ifdef USE_GUILE
SCM dictionary_entries_scm() {
   std::vector<std::string> comp_ids = dictionary_entries();
   return generic_string_vector_to_list_internal(comp_ids);
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *dictionary_entries_py() {

   std::vector<std::string> comp_ids = dictionary_entries();
   return generic_string_vector_to_list_internal_py(comp_ids);
} 
#endif // USE_PYTHON





#ifdef USE_GUILE
SCM cif_file_for_comp_id_scm(const std::string &comp_id) {

   graphics_info_t g;
   std::string f = g.Geom_p()->get_cif_file_name(comp_id);
   return scm_makfrom0str(f.c_str());
} 
#endif // GUILE


#ifdef USE_PYTHON
PyObject *cif_file_for_comp_id_py(const std::string &comp_id) {

   graphics_info_t g;
   return PyString_FromString(g.Geom_p()->get_cif_file_name(comp_id).c_str());
} 
#endif // PYTHON

// can throw and std::runtime_error exception
std::string SMILES_for_comp_id(const std::string &comp_id) {

   graphics_info_t g;
   std::string s = g.Geom_p()->Get_SMILES_for_comp_id(comp_id); // can throw
   return s;
}

#ifdef USE_GUILE
SCM SMILES_for_comp_id_scm(const std::string &comp_id) {

   SCM r = SCM_BOOL_F;
   try {
      std::string s = SMILES_for_comp_id(comp_id);
      r = scm_makfrom0str(s.c_str());
   }
   catch (std::runtime_error rte) {
      std::cout << "WARNING:: " << rte.what() << std::endl;
   } 
   return r;
}
#endif


#ifdef USE_PYTHON
PyObject *SMILES_for_comp_id_py(const std::string &comp_id) {
   PyObject *r = Py_False;
   try {
      std::string s = SMILES_for_comp_id(comp_id);
      r = PyString_FromString(s.c_str());
   }
   catch (std::runtime_error rte) {
      std::cout << "WARNING:: " << rte.what() << std::endl;
   } 
   if (PyBool_Check(r))
      Py_INCREF(r);
   return r;
} 
#endif // USE_PYTHON


/*  ----------------------------------------------------------------------- */
/*                         Restraints                                       */
/*  ----------------------------------------------------------------------- */
#ifdef USE_GUILE
SCM monomer_restraints(const char *monomer_type) {

   SCM r = SCM_BOOL_F;

   graphics_info_t g;
   // this forces try_dynamic_add()
   g.Geom_p()->have_dictionary_for_residue_type(monomer_type, ++g.cif_dictionary_read_number);
   // this doesn't force try_dynamic_add().  Hmmm.. FIXME (or think about it).
   std::pair<short int, coot::dictionary_residue_restraints_t> p =
      g.Geom_p()->get_monomer_restraints(monomer_type);
   if (p.first) {

      coot::dictionary_residue_restraints_t restraints = p.second;
      r = SCM_EOL;

      // ------------------ chem_comp -------------------------
      coot::dict_chem_comp_t info = restraints.residue_info;
      SCM chem_comp_scm = SCM_EOL;
      chem_comp_scm = scm_cons(scm_makfrom0str(info.comp_id.c_str()),           chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.three_letter_code.c_str()), chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.name.c_str()),              chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.group.c_str()),             chem_comp_scm);
      chem_comp_scm = scm_cons(scm_int2num(info.number_atoms_all),              chem_comp_scm);
      chem_comp_scm = scm_cons(scm_int2num(info.number_atoms_nh),               chem_comp_scm);
      chem_comp_scm = scm_cons(scm_makfrom0str(info.description_level.c_str()), chem_comp_scm);
      chem_comp_scm = scm_reverse(chem_comp_scm);
      SCM chem_comp_container = SCM_EOL;
      // chem_comp_container = scm_cons(chem_comp_scm, chem_comp_container);
      chem_comp_container = scm_cons(scm_makfrom0str("_chem_comp"), chem_comp_scm);

      // ------------------ chem_comp_atom -------------------------
      std::vector<coot::dict_atom> atom_info = restraints.atom_info;
      int n_atoms = atom_info.size();
      SCM atom_info_list = SCM_EOL;
      for (int iat=0; iat<n_atoms; iat++) { 
	 SCM atom_attributes_list = SCM_EOL;
	 atom_attributes_list = scm_cons(scm_makfrom0str(atom_info[iat].atom_id_4c.c_str()),   atom_attributes_list);
	 atom_attributes_list = scm_cons(scm_makfrom0str(atom_info[iat].type_symbol.c_str()),  atom_attributes_list);
	 atom_attributes_list = scm_cons(scm_makfrom0str(atom_info[iat].type_energy.c_str()),  atom_attributes_list);
	 atom_attributes_list = scm_cons(scm_double2num(atom_info[iat].partial_charge.second), atom_attributes_list);
	 SCM partial_flag = SCM_BOOL_F;
	 if (atom_info[iat].partial_charge.first)
	    partial_flag = SCM_BOOL_T;
	 atom_attributes_list = scm_cons(partial_flag, atom_attributes_list);
	 atom_attributes_list = scm_reverse(atom_attributes_list);
	 atom_info_list = scm_cons(atom_attributes_list, atom_info_list);
      }
      atom_info_list = scm_reverse(atom_info_list);
      SCM atom_info_list_container = SCM_EOL;
      // atom_info_list_container = scm_cons(atom_info_list, atom_info_list_container);
      atom_info_list_container = scm_cons(scm_makfrom0str("_chem_comp_atom"), atom_info_list);


      // ------------------ Bonds -------------------------
      SCM bond_restraint_list = SCM_EOL;
      
      for (unsigned int ibond=0; ibond<restraints.bond_restraint.size(); ibond++) {
	 const coot::dict_bond_restraint_t &bond_restraint = restraints.bond_restraint[ibond];
	 std::string a1 = bond_restraint.atom_id_1_4c();
	 std::string a2 = bond_restraint.atom_id_2_4c();
	 std::string type = bond_restraint.type();
	 SCM bond_restraint_scm = SCM_EOL;
	 SCM esd_scm = SCM_BOOL_F;
	 SCM d_scm   = SCM_BOOL_F;
	 try { 
	    double esd = bond_restraint.value_esd();
	    double d   = bond_restraint.value_dist();
	    esd_scm = scm_double2num(esd);
	    d_scm   = scm_double2num(d);
	 }
	 catch (std::runtime_error rte) {
	    // we use the default values of #f, if the esd or dist is not set.
	 } 
	 bond_restraint_scm = scm_cons(esd_scm, bond_restraint_scm);
	 bond_restraint_scm = scm_cons(d_scm,   bond_restraint_scm);
	 bond_restraint_scm = scm_cons(scm_makfrom0str(type.c_str()), bond_restraint_scm);
	 bond_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()),   bond_restraint_scm);
	 bond_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()),   bond_restraint_scm);
	 bond_restraint_list = scm_cons(bond_restraint_scm, bond_restraint_list);
      }
      SCM bond_restraints_container = SCM_EOL;
      // bond_restraints_container = scm_cons(bond_restraint_list, bond_restraints_container);
      bond_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_bond"), bond_restraint_list);

      // ------------------ Angles -------------------------
      SCM angle_restraint_list = SCM_EOL;
      for (unsigned int iangle=0; iangle<restraints.angle_restraint.size(); iangle++) {
	 coot::dict_angle_restraint_t angle_restraint = restraints.angle_restraint[iangle];
	 std::string a1 = angle_restraint.atom_id_1_4c();
	 std::string a2 = angle_restraint.atom_id_2_4c();
	 std::string a3 = angle_restraint.atom_id_3_4c();
	 double d   = angle_restraint.angle();
	 double esd = angle_restraint.esd();
	 SCM angle_restraint_scm = SCM_EOL;
	 angle_restraint_scm = scm_cons(scm_double2num(esd), angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_double2num(d),   angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_makfrom0str(a3.c_str()),   angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()),   angle_restraint_scm);
	 angle_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()),   angle_restraint_scm);
	 angle_restraint_list = scm_cons(angle_restraint_scm, angle_restraint_list);
      }
      SCM angle_restraints_container = SCM_EOL;
      // angle_restraints_container = scm_cons(angle_restraint_list, angle_restraints_container);
      angle_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_angle"), angle_restraint_list);

      // ------------------ Torsions -------------------------
      SCM torsion_restraint_list = SCM_EOL;
      for (unsigned int itorsion=0; itorsion<restraints.torsion_restraint.size(); itorsion++) {
	 coot::dict_torsion_restraint_t torsion_restraint = restraints.torsion_restraint[itorsion];
	 std::string id = torsion_restraint.id();
	 std::string a1 = torsion_restraint.atom_id_1_4c();
	 std::string a2 = torsion_restraint.atom_id_2_4c();
	 std::string a3 = torsion_restraint.atom_id_3_4c();
	 std::string a4 = torsion_restraint.atom_id_4_4c();
	 double tor  = torsion_restraint.angle();
	 double esd = torsion_restraint.esd();
	 int period = torsion_restraint.periodicity();
	 SCM torsion_restraint_scm = SCM_EOL;
	 torsion_restraint_scm = scm_cons(SCM_MAKINUM(period), torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_double2num(esd), torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_double2num(tor), torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a4.c_str()),   torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a3.c_str()),   torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()),   torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()),   torsion_restraint_scm);
	 torsion_restraint_scm = scm_cons(scm_makfrom0str(id.c_str()),   torsion_restraint_scm);
	 torsion_restraint_list = scm_cons(torsion_restraint_scm, torsion_restraint_list);
      }
      SCM torsion_restraints_container = SCM_EOL;
      // torsion_restraints_container = scm_cons(torsion_restraint_list, torsion_restraints_container);
      torsion_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_tor"), torsion_restraint_list);


      // ------------------ Planes -------------------------
      SCM plane_restraint_list = SCM_EOL;
      for (unsigned int iplane=0; iplane<restraints.plane_restraint.size(); iplane++) {
	 coot::dict_plane_restraint_t plane_restraint = restraints.plane_restraint[iplane];
	 SCM atom_list = SCM_EOL;
	 for (int iat=0; iat<plane_restraint.n_atoms(); iat++) {

	    std::string at = plane_restraint[iat].first;
	    atom_list = scm_cons(scm_makfrom0str(at.c_str()), atom_list);
	 }
	 atom_list = scm_reverse(atom_list);

	 double esd = plane_restraint.dist_esd(0); // fixme
	 SCM plane_id_scm = scm_makfrom0str(plane_restraint.plane_id.c_str());

	 SCM plane_restraint_scm = SCM_EOL;
	 plane_restraint_scm = scm_cons(scm_double2num(esd), plane_restraint_scm);
	 plane_restraint_scm = scm_cons(atom_list, plane_restraint_scm);
	 plane_restraint_scm = scm_cons(plane_id_scm, plane_restraint_scm);
	 plane_restraint_list = scm_cons(plane_restraint_scm, plane_restraint_list);
      }
      SCM plane_restraints_container = SCM_EOL;
      // plane_restraints_container = scm_cons(plane_restraint_list, plane_restraints_container);
      plane_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_plane_atom"),
					    plane_restraint_list);


      // ------------------ Chirals -------------------------
      SCM chiral_restraint_list = SCM_EOL;
      for (unsigned int ichiral=0; ichiral<restraints.chiral_restraint.size(); ichiral++) {
	 coot::dict_chiral_restraint_t chiral_restraint = restraints.chiral_restraint[ichiral];

	 std::string a1 = chiral_restraint.atom_id_1_4c();
	 std::string a2 = chiral_restraint.atom_id_2_4c();
	 std::string a3 = chiral_restraint.atom_id_3_4c();
	 std::string ac = chiral_restraint.atom_id_c_4c();
	 std::string chiral_id = chiral_restraint.Chiral_Id();
	 int vol_sign = chiral_restraint.volume_sign;

	 double esd = chiral_restraint.volume_sigma();
	 // int volume_sign = chiral_restraint.volume_sign;
	 SCM chiral_restraint_scm = SCM_EOL;
	 chiral_restraint_scm = scm_cons(scm_double2num(esd), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_int2num(vol_sign), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(a3.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(a2.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(a1.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(ac.c_str()), chiral_restraint_scm);
	 chiral_restraint_scm = scm_cons(scm_makfrom0str(chiral_id.c_str()), chiral_restraint_scm);
	 chiral_restraint_list = scm_cons(chiral_restraint_scm, chiral_restraint_list);
      }
      SCM chiral_restraints_container = SCM_EOL;
      // chiral_restraints_container = scm_cons(chiral_restraint_list, chiral_restraints_container);
      chiral_restraints_container = scm_cons(scm_makfrom0str("_chem_comp_chir"), chiral_restraint_list);

      
      r = scm_cons( chiral_restraints_container, r);
      r = scm_cons(  plane_restraints_container, r);
      r = scm_cons(torsion_restraints_container, r);
      r = scm_cons(  angle_restraints_container, r);
      r = scm_cons(   bond_restraints_container, r);
      r = scm_cons(    atom_info_list_container, r);
      r = scm_cons(         chem_comp_container, r);

   }
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *monomer_restraints_py(const char *monomer_type) {

   PyObject *r;
   r = Py_False;

   graphics_info_t g;
   // this forces try_dynamic_add()
   g.Geom_p()->have_dictionary_for_residue_type(monomer_type, ++g.cif_dictionary_read_number);
   // this doesn't force try_dynamic_add().  Hmmm.. FIXME (or think about it).
   std::pair<short int, coot::dictionary_residue_restraints_t> p =
      g.Geom_p()->get_monomer_restraints(monomer_type);
   if (!p.first) {
      std::cout << "WARNING:: can't find " << monomer_type << " in monomer dictionary"
		<< std::endl;
   } else {

      r = PyDict_New();
	 
      coot::dictionary_residue_restraints_t restraints = p.second;

      // ------------------ chem_comp -------------------------
      coot::dict_chem_comp_t info = restraints.residue_info;
      
      PyObject *chem_comp_py = PyList_New(7);
      PyList_SetItem(chem_comp_py, 0, PyString_FromString(info.comp_id.c_str()));
      PyList_SetItem(chem_comp_py, 1, PyString_FromString(info.three_letter_code.c_str()));
      PyList_SetItem(chem_comp_py, 2, PyString_FromString(info.name.c_str()));
      PyList_SetItem(chem_comp_py, 3, PyString_FromString(info.group.c_str()));
      PyList_SetItem(chem_comp_py, 4, PyInt_FromLong(info.number_atoms_all));
      PyList_SetItem(chem_comp_py, 5, PyInt_FromLong(info.number_atoms_nh));
      PyList_SetItem(chem_comp_py, 6, PyString_FromString(info.description_level.c_str()));
      
      // Put chem_comp_py into a dictionary?
      PyDict_SetItem(r, PyString_FromString("_chem_comp"), chem_comp_py);

      
      // ------------------ chem_comp_atom -------------------------
      std::vector<coot::dict_atom> atom_info = restraints.atom_info;
      int n_atoms = atom_info.size();
      PyObject *atom_info_list = PyList_New(n_atoms);
      for (int iat=0; iat<n_atoms; iat++) { 
	 PyObject *atom_attributes_list = PyList_New(5);
	 PyList_SetItem(atom_attributes_list, 0, PyString_FromString(atom_info[iat].atom_id_4c.c_str()));
	 PyList_SetItem(atom_attributes_list, 1, PyString_FromString(atom_info[iat].type_symbol.c_str()));
	 PyList_SetItem(atom_attributes_list, 2, PyString_FromString(atom_info[iat].type_energy.c_str()));
	 PyList_SetItem(atom_attributes_list, 3, PyFloat_FromDouble(atom_info[iat].partial_charge.second));
	 PyObject *flag = Py_False;
	 if (atom_info[iat].partial_charge.first)
	    flag = Py_True;
     Py_INCREF(flag);
	 PyList_SetItem(atom_attributes_list, 4, flag);
	 PyList_SetItem(atom_info_list, iat, atom_attributes_list);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_atom"), atom_info_list);

      // ------------------ Bonds -------------------------
      PyObject *bond_restraint_list = PyList_New(restraints.bond_restraint.size());
      for (unsigned int ibond=0; ibond<restraints.bond_restraint.size(); ibond++) {
	 std::string a1   = restraints.bond_restraint[ibond].atom_id_1_4c();
	 std::string a2   = restraints.bond_restraint[ibond].atom_id_2_4c();
	 std::string type = restraints.bond_restraint[ibond].type();

	 PyObject *py_value_dist = Py_False;
	 PyObject *py_value_esd = Py_False;

	 try { 
	    double d   = restraints.bond_restraint[ibond].value_dist();
	    double esd = restraints.bond_restraint[ibond].value_esd();
	    py_value_dist = PyFloat_FromDouble(d);
	    py_value_esd  = PyFloat_FromDouble(esd);
	 }
	 catch (std::runtime_error rte) {

	    // Use default false values.
	    // So I suppose that I need to do this then:
	    if (PyBool_Check(py_value_dist))
	       Py_INCREF(py_value_dist);
	    if (PyBool_Check(py_value_esd))
	       Py_INCREF(py_value_esd);
	 }
	 
	 PyObject *bond_restraint = PyList_New(5);
	 PyList_SetItem(bond_restraint, 0, PyString_FromString(a1.c_str()));
	 PyList_SetItem(bond_restraint, 1, PyString_FromString(a2.c_str()));
	 PyList_SetItem(bond_restraint, 2, PyString_FromString(type.c_str()));
	 PyList_SetItem(bond_restraint, 3, py_value_dist);
	 PyList_SetItem(bond_restraint, 4, py_value_esd);
	 PyList_SetItem(bond_restraint_list, ibond, bond_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_bond"), bond_restraint_list);


      // ------------------ Angles -------------------------
      PyObject *angle_restraint_list = PyList_New(restraints.angle_restraint.size());
      for (unsigned int iangle=0; iangle<restraints.angle_restraint.size(); iangle++) {
	 std::string a1 = restraints.angle_restraint[iangle].atom_id_1_4c();
	 std::string a2 = restraints.angle_restraint[iangle].atom_id_2_4c();
	 std::string a3 = restraints.angle_restraint[iangle].atom_id_3_4c();
	 double d   = restraints.angle_restraint[iangle].angle();
	 double esd = restraints.angle_restraint[iangle].esd();
	 PyObject *angle_restraint = PyList_New(5);
	 PyList_SetItem(angle_restraint, 0, PyString_FromString(a1.c_str()));
	 PyList_SetItem(angle_restraint, 1, PyString_FromString(a2.c_str()));
	 PyList_SetItem(angle_restraint, 2, PyString_FromString(a3.c_str()));
	 PyList_SetItem(angle_restraint, 3, PyFloat_FromDouble(d));
	 PyList_SetItem(angle_restraint, 4, PyFloat_FromDouble(esd));
	 PyList_SetItem(angle_restraint_list, iangle, angle_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_angle"), angle_restraint_list);

      
      // ------------------ Torsions -------------------------
      PyObject *torsion_restraint_list = PyList_New(restraints.torsion_restraint.size());
      for (unsigned int itorsion=0; itorsion<restraints.torsion_restraint.size(); itorsion++) {
	 std::string id = restraints.torsion_restraint[itorsion].id();
	 std::string a1 = restraints.torsion_restraint[itorsion].atom_id_1_4c();
	 std::string a2 = restraints.torsion_restraint[itorsion].atom_id_2_4c();
	 std::string a3 = restraints.torsion_restraint[itorsion].atom_id_3_4c();
	 std::string a4 = restraints.torsion_restraint[itorsion].atom_id_4_4c();
	 double tor  = restraints.torsion_restraint[itorsion].angle();
	 double esd = restraints.torsion_restraint[itorsion].esd();
	 int period = restraints.torsion_restraint[itorsion].periodicity();
	 PyObject *torsion_restraint = PyList_New(8);
	 PyList_SetItem(torsion_restraint, 0, PyString_FromString(id.c_str()));
	 PyList_SetItem(torsion_restraint, 1, PyString_FromString(a1.c_str()));
	 PyList_SetItem(torsion_restraint, 2, PyString_FromString(a2.c_str()));
	 PyList_SetItem(torsion_restraint, 3, PyString_FromString(a3.c_str()));
	 PyList_SetItem(torsion_restraint, 4, PyString_FromString(a4.c_str()));
	 PyList_SetItem(torsion_restraint, 5, PyFloat_FromDouble(tor));
	 PyList_SetItem(torsion_restraint, 6, PyFloat_FromDouble(esd));
	 PyList_SetItem(torsion_restraint, 7, PyInt_FromLong(period));
	 PyList_SetItem(torsion_restraint_list, itorsion, torsion_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_tor"), torsion_restraint_list);

      // ------------------ Planes -------------------------
      PyObject *plane_restraints_list = PyList_New(restraints.plane_restraint.size());
      for (unsigned int iplane=0; iplane<restraints.plane_restraint.size(); iplane++) {
	 PyObject *atom_list = PyList_New(restraints.plane_restraint[iplane].n_atoms());
	 for (int iat=0; iat<restraints.plane_restraint[iplane].n_atoms(); iat++) { 
	    std::string at = restraints.plane_restraint[iplane][iat].first;
	    PyList_SetItem(atom_list, iat, PyString_FromString(at.c_str()));
	 }
	 double esd = restraints.plane_restraint[iplane].dist_esd(0);
	 PyObject *plane_restraint = PyList_New(3);
	 PyList_SetItem(plane_restraint, 0, PyString_FromString(restraints.plane_restraint[iplane].plane_id.c_str()));
	 PyList_SetItem(plane_restraint, 1, atom_list);
	 PyList_SetItem(plane_restraint, 2, PyFloat_FromDouble(esd));
	 PyList_SetItem(plane_restraints_list, iplane, plane_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_plane_atom"), plane_restraints_list);

      // ------------------ Chirals -------------------------
      PyObject *chiral_restraint_list = PyList_New(restraints.chiral_restraint.size());
      for (unsigned int ichiral=0; ichiral<restraints.chiral_restraint.size(); ichiral++) {
	 
	 std::string a1 = restraints.chiral_restraint[ichiral].atom_id_1_4c();
	 std::string a2 = restraints.chiral_restraint[ichiral].atom_id_2_4c();
	 std::string a3 = restraints.chiral_restraint[ichiral].atom_id_3_4c();
	 std::string ac = restraints.chiral_restraint[ichiral].atom_id_c_4c();
	 std::string chiral_id = restraints.chiral_restraint[ichiral].Chiral_Id();

	 double esd = restraints.chiral_restraint[ichiral].volume_sigma();
	 int volume_sign = restraints.chiral_restraint[ichiral].volume_sign;
	 PyObject *chiral_restraint = PyList_New(7);
	 PyList_SetItem(chiral_restraint, 0, PyString_FromString(chiral_id.c_str()));
	 PyList_SetItem(chiral_restraint, 1, PyString_FromString(ac.c_str()));
	 PyList_SetItem(chiral_restraint, 2, PyString_FromString(a1.c_str()));
	 PyList_SetItem(chiral_restraint, 3, PyString_FromString(a2.c_str()));
	 PyList_SetItem(chiral_restraint, 4, PyString_FromString(a3.c_str()));
	 PyList_SetItem(chiral_restraint, 5, PyInt_FromLong(volume_sign));
	 PyList_SetItem(chiral_restraint, 6, PyFloat_FromDouble(esd));
	 PyList_SetItem(chiral_restraint_list, ichiral, chiral_restraint);
      }

      PyDict_SetItem(r, PyString_FromString("_chem_comp_chir"), chiral_restraint_list);
   }
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM set_monomer_restraints(const char *monomer_type, SCM restraints) {

   SCM retval = SCM_BOOL_F;

   if (!scm_is_true(scm_list_p(restraints))) {
      std::cout << " Failed to read restraints - not a list" << std::endl;
   } else {

      std::vector<coot::dict_bond_restraint_t> bond_restraints;
      std::vector<coot::dict_angle_restraint_t> angle_restraints;
      std::vector<coot::dict_torsion_restraint_t> torsion_restraints;
      std::vector<coot::dict_chiral_restraint_t> chiral_restraints;
      std::vector<coot::dict_plane_restraint_t> plane_restraints;
      std::vector<coot::dict_atom> atoms;
      coot::dict_chem_comp_t residue_info;

      SCM restraints_length_scm = scm_length(restraints);
      int restraints_length = scm_to_int(restraints_length_scm);
      if (restraints_length > 0) {
	 for (int i_rest_type=0; i_rest_type<restraints_length; i_rest_type++) {
	    SCM rest_container = scm_list_ref(restraints, SCM_MAKINUM(i_rest_type));
	    if (scm_is_true(scm_list_p(rest_container))) {
	       SCM rest_container_length_scm = scm_length(rest_container);
	       int rest_container_length = scm_to_int(rest_container_length_scm);
	       if (rest_container_length > 1) {
		  SCM restraints_type_scm = SCM_CAR(rest_container);
		  if (scm_string_p(restraints_type_scm)) {
		     std::string restraints_type = scm_to_locale_string(restraints_type_scm);

		     if (restraints_type == "_chem_comp") {
			SCM chem_comp_info_scm = SCM_CDR(rest_container);
			SCM chem_comp_info_length_scm = scm_length(chem_comp_info_scm);
			int chem_comp_info_length = scm_to_int(chem_comp_info_length_scm);

			if (chem_comp_info_length != 7) {
			   std::cout << "WARNING:: chem_comp_info length " << chem_comp_info_length
				     << " should be " << 7 << std::endl;
			} else { 
			   SCM  comp_id_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(0));
			   SCM      tlc_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(1));
			   SCM     name_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(2));
			   SCM    group_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(3));
			   SCM      noa_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(4));
			   SCM    nonha_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(5));
			   SCM desc_lev_scm = scm_list_ref(chem_comp_info_scm, SCM_MAKINUM(6));
			   if (scm_is_true(scm_string_p(comp_id_scm)) &&
			       scm_is_true(scm_string_p(tlc_scm)) &&
			       scm_is_true(scm_string_p(name_scm)) &&
			       scm_is_true(scm_string_p(group_scm)) &&
			       scm_is_true(scm_number_p(noa_scm)) &&
			       scm_is_true(scm_number_p(nonha_scm)) &&
			       scm_is_true(scm_string_p(desc_lev_scm))) {
			      std::string comp_id = scm_to_locale_string(comp_id_scm);
			      std::string     tlc = scm_to_locale_string(tlc_scm);
			      std::string    name = scm_to_locale_string(name_scm);
			      std::string   group = scm_to_locale_string(group_scm);
			      std::string des_lev = scm_to_locale_string(desc_lev_scm);
			      int no_of_atoms = scm_to_int(noa_scm);
			      int no_of_non_H_atoms = scm_to_int(nonha_scm);
			      coot::dict_chem_comp_t n(comp_id, tlc, name, group,
						       no_of_atoms, no_of_non_H_atoms,
						       des_lev);
			      residue_info = n;
			   }
			} 
		     }

		     if (restraints_type == "_chem_comp_atom") {
			SCM chem_comp_atoms = SCM_CDR(rest_container);
			SCM chem_comp_atoms_length_scm = scm_length(chem_comp_atoms);
			int chem_comp_atoms_length = scm_to_int(chem_comp_atoms_length_scm);

			for (int iat=0; iat<chem_comp_atoms_length; iat++) {
			   SCM chem_comp_atom_scm = scm_list_ref(chem_comp_atoms, SCM_MAKINUM(iat));
			   SCM chem_comp_atom_length_scm = scm_length(chem_comp_atom_scm);
			   int chem_comp_atom_length = scm_to_int(chem_comp_atom_length_scm);
			   
			   if (chem_comp_atom_length != 5) {
			      std::cout << "WARNING:: chem_comp_atom length " << chem_comp_atom_length
					<< " should be " << 5 << std::endl;
			   } else { 
			      SCM atom_id_scm  = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(0));
			      SCM element_scm  = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(1));
			      SCM energy_scm   = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(2));
			      SCM partial_charge_scm = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(3));
			      SCM valid_pc_scm = scm_list_ref(chem_comp_atom_scm, SCM_MAKINUM(4));

			      if (scm_string_p(atom_id_scm) && scm_string_p(element_scm) &&
				  scm_number_p(partial_charge_scm)) {
				 std::string atom_id(scm_to_locale_string(atom_id_scm));
				 std::string element(scm_to_locale_string(element_scm));
				 std::string energy(scm_to_locale_string(energy_scm));
				 float partial_charge = scm_to_double(partial_charge_scm);
				 short int valid_partial_charge = 1;
				 if SCM_FALSEP(valid_pc_scm)
				    valid_partial_charge = 0;
				 coot::dict_atom at(atom_id, atom_id, element, energy, 
						    std::pair<bool, float>(valid_partial_charge,
									   partial_charge));

				 atoms.push_back(at);
			      }
			   }
			}
			
		     }
		     

		     if (restraints_type == "_chem_comp_bond") {
			SCM bond_restraints_list_scm = SCM_CDR(rest_container);
			SCM bond_restraints_list_length_scm = scm_length(bond_restraints_list_scm);
			int bond_restraints_list_length = scm_to_int(bond_restraints_list_length_scm);

			for (int ibr=0; ibr<bond_restraints_list_length; ibr++) {
			   SCM bond_restraint = scm_list_ref(bond_restraints_list_scm, SCM_MAKINUM(ibr));
			   SCM bond_restraint_length_scm = scm_length(bond_restraint);
			   int bond_restraint_length = scm_to_int(bond_restraint_length_scm);

			   if (bond_restraint_length != 5) {
			      std::cout << "WARNING:: bond_restraint_length " << bond_restraint_length
					<< " should be " << 5 << std::endl;
			   } else { 
			      SCM atom_1_scm = scm_list_ref(bond_restraint, SCM_MAKINUM(0));
			      SCM atom_2_scm = scm_list_ref(bond_restraint, SCM_MAKINUM(1));
			      SCM type_scm   = scm_list_ref(bond_restraint, SCM_MAKINUM(2));
			      SCM dist_scm   = scm_list_ref(bond_restraint, SCM_MAKINUM(3));
			      SCM esd_scm    = scm_list_ref(bond_restraint, SCM_MAKINUM(4));
			      if (scm_string_p(atom_1_scm) && scm_string_p(atom_2_scm) &&
				  scm_number_p(dist_scm) && scm_number_p(esd_scm)) {
				 std::string atom_1 = scm_to_locale_string(atom_1_scm);
				 std::string atom_2 = scm_to_locale_string(atom_2_scm);
				 std::string type   = scm_to_locale_string(type_scm);
				 double dist        = scm_to_double(dist_scm);
				 double esd         = scm_to_double(esd_scm);
				 coot::dict_bond_restraint_t rest(atom_1, atom_2, type, dist, esd);
				 bond_restraints.push_back(rest);
			      }
			   }
			}
		     }

		     if (restraints_type == "_chem_comp_angle") {
			SCM angle_restraints_list = SCM_CDR(rest_container);
			SCM angle_restraints_list_length_scm = scm_length(angle_restraints_list);
			int angle_restraints_list_length = scm_to_int(angle_restraints_list_length_scm);

			for (int iar=0; iar<angle_restraints_list_length; iar++) {
			   SCM angle_restraint = scm_list_ref(angle_restraints_list, SCM_MAKINUM(iar));
			   SCM angle_restraint_length_scm = scm_length(angle_restraint);
			   int angle_restraint_length = scm_to_int(angle_restraint_length_scm);

			   if (angle_restraint_length != 5) {
			      std::cout << "WARNING:: angle_restraint_length length "
					<< angle_restraint_length << " should be " << 5 << std::endl;
			   } else { 
			      SCM atom_1_scm = scm_list_ref(angle_restraint, SCM_MAKINUM(0));
			      SCM atom_2_scm = scm_list_ref(angle_restraint, SCM_MAKINUM(1));
			      SCM atom_3_scm = scm_list_ref(angle_restraint, SCM_MAKINUM(2));
			      SCM angle_scm  = scm_list_ref(angle_restraint, SCM_MAKINUM(3));
			      SCM esd_scm    = scm_list_ref(angle_restraint, SCM_MAKINUM(4));
			      if (scm_string_p(atom_1_scm) && scm_string_p(atom_2_scm) &&
				  scm_string_p(atom_3_scm) &&
				  scm_number_p(angle_scm) && scm_number_p(esd_scm)) {
				 std::string atom_1 = scm_to_locale_string(atom_1_scm);
				 std::string atom_2 = scm_to_locale_string(atom_2_scm);
				 std::string atom_3 = scm_to_locale_string(atom_3_scm);
				 double angle       = scm_to_double(angle_scm);
				 double esd         = scm_to_double(esd_scm);
				 coot::dict_angle_restraint_t rest(atom_1, atom_2, atom_3, angle, esd);
				 angle_restraints.push_back(rest);
			      }
			   }
			}
		     }


		     if (restraints_type == "_chem_comp_tor") {
			SCM torsion_restraints_list = SCM_CDR(rest_container);
			SCM torsion_restraints_list_length_scm = scm_length(torsion_restraints_list);
			int torsion_restraints_list_length = scm_to_int(torsion_restraints_list_length_scm);

			for (int itr=0; itr<torsion_restraints_list_length; itr++) {
			   SCM torsion_restraint = scm_list_ref(torsion_restraints_list, SCM_MAKINUM(itr));
			   SCM torsion_restraint_length_scm = scm_length(torsion_restraint);
			   int torsion_restraint_length = scm_to_int(torsion_restraint_length_scm);
			   
			   if (torsion_restraint_length == 8) {
			      SCM torsion_id_scm = scm_list_ref(torsion_restraint, SCM_MAKINUM(0));
			      SCM atom_1_scm     = scm_list_ref(torsion_restraint, SCM_MAKINUM(1));
			      SCM atom_2_scm     = scm_list_ref(torsion_restraint, SCM_MAKINUM(2));
			      SCM atom_3_scm     = scm_list_ref(torsion_restraint, SCM_MAKINUM(3));
			      SCM atom_4_scm     = scm_list_ref(torsion_restraint, SCM_MAKINUM(4));
			      SCM torsion_scm    = scm_list_ref(torsion_restraint, SCM_MAKINUM(5));
			      SCM esd_scm        = scm_list_ref(torsion_restraint, SCM_MAKINUM(6));
			      SCM period_scm     = scm_list_ref(torsion_restraint, SCM_MAKINUM(7));
			      if (scm_is_true(scm_string_p(atom_1_scm)) &&
				  scm_is_true(scm_string_p(atom_2_scm)) &&
				  scm_is_true(scm_string_p(atom_3_scm)) &&
				  scm_is_true(scm_string_p(atom_4_scm)) &&
				  scm_is_true(scm_number_p(torsion_scm)) &&
				  scm_is_true(scm_number_p(esd_scm)) && 
				  scm_is_true(scm_number_p(period_scm))) {
				 std::string torsion_id = scm_to_locale_string(torsion_id_scm);
				 std::string atom_1     = scm_to_locale_string(atom_1_scm);
				 std::string atom_2     = scm_to_locale_string(atom_2_scm);
				 std::string atom_3     = scm_to_locale_string(atom_3_scm);
				 std::string atom_4     = scm_to_locale_string(atom_4_scm);
				 double torsion         = scm_to_double(torsion_scm);
				 double esd             = scm_to_double(esd_scm);
				 int period             = scm_to_int(period_scm);
				 coot::dict_torsion_restraint_t rest(torsion_id,
								     atom_1, atom_2, atom_3, atom_4,
								     torsion, esd, period);
				 torsion_restraints.push_back(rest);
			      }
			   }
			}
		     }
		     
		     if (restraints_type == "_chem_comp_plane_atom") {
			SCM plane_restraints_list = SCM_CDR(rest_container);
			SCM plane_restraints_list_length_scm = scm_length(plane_restraints_list);
			int plane_restraints_list_length = scm_to_int(plane_restraints_list_length_scm);

			for (int ipr=0; ipr<plane_restraints_list_length; ipr++) {
			   SCM plane_restraint = scm_list_ref(plane_restraints_list, SCM_MAKINUM(ipr));
			   SCM plane_restraint_length_scm = scm_length(plane_restraint);
			   int plane_restraint_length = scm_to_int(plane_restraint_length_scm);

			   if (plane_restraint_length == 3) {

			      std::vector<SCM> atoms;
			      SCM plane_id_scm   = scm_list_ref(plane_restraint, SCM_MAKINUM(0));
			      SCM esd_scm        = scm_list_ref(plane_restraint, SCM_MAKINUM(2));
			      SCM atom_list_scm  = scm_list_ref(plane_restraint, SCM_MAKINUM(1));
			      SCM atom_list_length_scm = scm_length(atom_list_scm);
			      int atom_list_length = scm_to_int(atom_list_length_scm);
			      bool atoms_pass = 1;
			      for (int iat=0; iat<atom_list_length; iat++) { 
				 SCM atom_scm   = scm_list_ref(atom_list_scm, SCM_MAKINUM(iat));
				 atoms.push_back(atom_scm);
				 if (!scm_string_p(atom_scm))
				    atoms_pass = 0;
			      }
			   
			      if (atoms_pass && scm_string_p(plane_id_scm) &&  scm_number_p(esd_scm)) { 
				 std::vector<std::string> atom_names;
				 for (unsigned int i=0; i<atoms.size(); i++)
				    atom_names.push_back(std::string(scm_to_locale_string(atoms[i])));

				 std::string plane_id = scm_to_locale_string(plane_id_scm);
				 double esd           = scm_to_double(esd_scm);
				 if (atom_names.size() > 0) { 
				    coot::dict_plane_restraint_t rest(plane_id, atom_names[0], esd);

				    for (unsigned int i=1; i<atom_names.size(); i++) {
				       rest.push_back_atom(atom_names[i], esd);
				    } 
				    plane_restraints.push_back(rest);
				    // std::cout << "plane restraint: " << rest << std::endl;
				 }
			      }
			   }
			}
		     }
		     
		     
		     if (restraints_type == "_chem_comp_chir") {
			SCM chiral_restraints_list = SCM_CDR(rest_container);
			SCM chiral_restraints_list_length_scm = scm_length(chiral_restraints_list);
			int chiral_restraints_list_length = scm_to_int(chiral_restraints_list_length_scm);

			for (int icr=0; icr<chiral_restraints_list_length; icr++) {
			   SCM chiral_restraint = scm_list_ref(chiral_restraints_list, SCM_MAKINUM(icr));
			   SCM chiral_restraint_length_scm = scm_length(chiral_restraint);
			   int chiral_restraint_length = scm_to_int(chiral_restraint_length_scm);

			   if (chiral_restraint_length == 7) {
			      SCM chiral_id_scm= scm_list_ref(chiral_restraint, SCM_MAKINUM(0));
			      SCM atom_c_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(1));
			      SCM atom_1_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(2));
			      SCM atom_2_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(3));
			      SCM atom_3_scm   = scm_list_ref(chiral_restraint, SCM_MAKINUM(4));
			      SCM chiral_vol_sign_scm = scm_list_ref(chiral_restraint, SCM_MAKINUM(5));
			      // SCM esd_scm      = scm_list_ref(chiral_restraint, SCM_MAKINUM(6));
			      if (scm_string_p(atom_1_scm) && scm_string_p(atom_2_scm) &&
				  scm_string_p(atom_3_scm) && scm_string_p(atom_c_scm)) { 
				 std::string chiral_id = scm_to_locale_string(chiral_id_scm);
				 std::string atom_1 = scm_to_locale_string(atom_1_scm);
				 std::string atom_2 = scm_to_locale_string(atom_2_scm);
				 std::string atom_3 = scm_to_locale_string(atom_3_scm);
				 std::string atom_c = scm_to_locale_string(atom_c_scm);
				 // double esd         = scm_to_double(esd_scm);
				 int chiral_vol_sign= scm_to_int(chiral_vol_sign_scm);
				 coot::dict_chiral_restraint_t rest(chiral_id,
								    atom_c, atom_1, atom_2, atom_3,
								    chiral_vol_sign);
				 
				 chiral_restraints.push_back(rest);
			      }
			   }
			}
		     }
		  }
	       }
	    }
	 }
      }

//       std::cout << "Found " <<    bond_restraints.size() << "   bond  restraints" << std::endl;
//       std::cout << "Found " <<   angle_restraints.size() << "   angle restraints" << std::endl;
//       std::cout << "Found " << torsion_restraints.size() << " torsion restraints" << std::endl;
//       std::cout << "Found " <<   plane_restraints.size() << "   plane restraints" << std::endl;
//       std::cout << "Found " <<  chiral_restraints.size() << "  chiral restraints" << std::endl;

      graphics_info_t g;
      coot::dictionary_residue_restraints_t monomer_restraints(monomer_type,
							       g.cif_dictionary_read_number++);
      monomer_restraints.bond_restraint    = bond_restraints;
      monomer_restraints.angle_restraint   = angle_restraints;
      monomer_restraints.torsion_restraint = torsion_restraints;
      monomer_restraints.chiral_restraint  = chiral_restraints;
      monomer_restraints.plane_restraint   = plane_restraints;
      monomer_restraints.residue_info      = residue_info;
      monomer_restraints.atom_info         = atoms; 

      bool s = g.Geom_p()->replace_monomer_restraints(monomer_type, monomer_restraints);
      if (s)
	 retval = SCM_BOOL_T;
   }

   return retval;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *set_monomer_restraints_py(const char *monomer_type, PyObject *restraints) {

   PyObject *retval = Py_False;

   if (!PyDict_Check(restraints)) {
      std::cout << " Failed to read restraints - not a list" << std::endl;
   } else {

      std::vector<coot::dict_bond_restraint_t> bond_restraints;
      std::vector<coot::dict_angle_restraint_t> angle_restraints;
      std::vector<coot::dict_torsion_restraint_t> torsion_restraints;
      std::vector<coot::dict_chiral_restraint_t> chiral_restraints;
      std::vector<coot::dict_plane_restraint_t> plane_restraints;
      std::vector<coot::dict_atom> atoms;
      coot::dict_chem_comp_t residue_info;

      PyObject *key;
      PyObject *value;
      Py_ssize_t pos = 0;

      std::cout << "looping over restraint" << std::endl;
      while (PyDict_Next(restraints, &pos, &key, &value)) {
	 // std::cout << ":::::::key: " << PyString_AsString(key) << std::endl;

	 std::string key_string = PyString_AsString(key);
	 if (key_string == "_chem_comp") {
	    PyObject *chem_comp_list = value;
	    if (PyList_Check(chem_comp_list)) {
	       if (PyObject_Length(chem_comp_list) == 7) {
		  std::string comp_id  = PyString_AsString(PyList_GetItem(chem_comp_list, 0));
		  std::string tlc      = PyString_AsString(PyList_GetItem(chem_comp_list, 1));
		  std::string name     = PyString_AsString(PyList_GetItem(chem_comp_list, 2));
		  std::string group    = PyString_AsString(PyList_GetItem(chem_comp_list, 3));
		  int n_atoms_all      = PyInt_AsLong(PyList_GetItem(chem_comp_list, 4));
		  int n_atoms_nh       = PyInt_AsLong(PyList_GetItem(chem_comp_list, 5));
		  std::string desc_lev = PyString_AsString(PyList_GetItem(chem_comp_list, 6));

		  coot::dict_chem_comp_t n(comp_id, tlc, name, group,
					   n_atoms_all, n_atoms_nh, desc_lev);
		  residue_info = n;
	       }
	    }
	 }


	 if (key_string == "_chem_comp_atom") {
	    PyObject *chem_comp_atom_list = value;
	    if (PyList_Check(chem_comp_atom_list)) {
	       int n_atoms = PyObject_Length(chem_comp_atom_list);
	       for (int iat=0; iat<n_atoms; iat++) {
		  PyObject *chem_comp_atom = PyList_GetItem(chem_comp_atom_list, iat);
		  if (PyObject_Length(chem_comp_atom) == 5) {
		     std::string atom_id  = PyString_AsString(PyList_GetItem(chem_comp_atom, 0));
		     std::string element  = PyString_AsString(PyList_GetItem(chem_comp_atom, 1));
		     std::string energy_t = PyString_AsString(PyList_GetItem(chem_comp_atom, 2));
		     float part_chr        = PyFloat_AsDouble(PyList_GetItem(chem_comp_atom, 3));
		     bool flag = 0;
		     if (PyLong_AsLong(PyList_GetItem(chem_comp_atom, 4))) {
			flag = 1;
		     }
		     std::pair<bool, float> part_charge_info(flag, part_chr);
		     coot::dict_atom at(atom_id, atom_id, element, energy_t, part_charge_info);
		     atoms.push_back(at);
		  }
	       }
	    }
	 }

	 if (key_string == "_chem_comp_bond") {
	    PyObject *bond_restraint_list = value;
	    if (PyList_Check(bond_restraint_list)) {
	       int n_bonds = PyObject_Length(bond_restraint_list);
	       for (int i_bond=0; i_bond<n_bonds; i_bond++) {
		  PyObject *bond_restraint = PyList_GetItem(bond_restraint_list, i_bond);
		  if (PyObject_Length(bond_restraint) == 5) { 
		     PyObject *atom_1_py = PyList_GetItem(bond_restraint, 0);
		     PyObject *atom_2_py = PyList_GetItem(bond_restraint, 1);
		     PyObject *type_py   = PyList_GetItem(bond_restraint, 2);
		     PyObject *dist_py   = PyList_GetItem(bond_restraint, 3);
		     PyObject *esd_py    = PyList_GetItem(bond_restraint, 4);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(type_py) &&
			 PyFloat_Check(dist_py) && 
			 PyFloat_Check(esd_py)) {
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string type   = PyString_AsString(type_py);
			float  dist = PyFloat_AsDouble(dist_py);
			float  esd  = PyFloat_AsDouble(esd_py);
			coot::dict_bond_restraint_t rest(atom_1, atom_2, type, dist, esd);
			bond_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }


	 if (key_string == "_chem_comp_angle") {
	    PyObject *angle_restraint_list = value;
	    if (PyList_Check(angle_restraint_list)) {
	       int n_angles = PyObject_Length(angle_restraint_list);
	       for (int i_angle=0; i_angle<n_angles; i_angle++) {
		  PyObject *angle_restraint = PyList_GetItem(angle_restraint_list, i_angle);
		  if (PyObject_Length(angle_restraint) == 5) { 
		     PyObject *atom_1_py = PyList_GetItem(angle_restraint, 0);
		     PyObject *atom_2_py = PyList_GetItem(angle_restraint, 1);
		     PyObject *atom_3_py = PyList_GetItem(angle_restraint, 2);
		     PyObject *angle_py  = PyList_GetItem(angle_restraint, 3);
		     PyObject *esd_py    = PyList_GetItem(angle_restraint, 4);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyFloat_Check(angle_py) && 
			 PyFloat_Check(esd_py)) {
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string atom_3 = PyString_AsString(atom_3_py);
			float  angle = PyFloat_AsDouble(angle_py);
			float  esd   = PyFloat_AsDouble(esd_py);
			coot::dict_angle_restraint_t rest(atom_1, atom_2, atom_3, angle, esd);
			angle_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }

	 if (key_string == "_chem_comp_tor") {
	    PyObject *torsion_restraint_list = value;
	    if (PyList_Check(torsion_restraint_list)) {
	       int n_torsions = PyObject_Length(torsion_restraint_list);
	       for (int i_torsion=0; i_torsion<n_torsions; i_torsion++) {
		  PyObject *torsion_restraint = PyList_GetItem(torsion_restraint_list, i_torsion);
		  if (PyObject_Length(torsion_restraint) == 7) { // info for Nigel.
		     std::cout << "torsions now have 8 elements starting with the torsion id\n"; 
		  } 
		  if (PyObject_Length(torsion_restraint) == 8) { 
		     PyObject *id_py     = PyList_GetItem(torsion_restraint, 0);
		     PyObject *atom_1_py = PyList_GetItem(torsion_restraint, 1);
		     PyObject *atom_2_py = PyList_GetItem(torsion_restraint, 2);
		     PyObject *atom_3_py = PyList_GetItem(torsion_restraint, 3);
		     PyObject *atom_4_py = PyList_GetItem(torsion_restraint, 4);
		     PyObject *torsion_py= PyList_GetItem(torsion_restraint, 5);
		     PyObject *esd_py    = PyList_GetItem(torsion_restraint, 6);
		     PyObject *period_py = PyList_GetItem(torsion_restraint, 7);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyString_Check(atom_4_py) &&
			 PyFloat_Check(torsion_py) && 
			 PyFloat_Check(esd_py)    && 
			 PyInt_Check(period_py)) { 
			std::string id     = PyString_AsString(id_py);
			std::string atom_1 = PyString_AsString(atom_1_py);
			std::string atom_2 = PyString_AsString(atom_2_py);
			std::string atom_3 = PyString_AsString(atom_3_py);
			std::string atom_4 = PyString_AsString(atom_4_py);
			float  torsion = PyFloat_AsDouble(torsion_py);
			float  esd     = PyFloat_AsDouble(esd_py);
			int  period    = PyInt_AsLong(period_py);
			coot::dict_torsion_restraint_t rest(id, atom_1, atom_2, atom_3, atom_4,
							    torsion, esd, period);
			torsion_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }

	 if (key_string == "_chem_comp_chir") {
	    PyObject *chiral_restraint_list = value;
	    if (PyList_Check(chiral_restraint_list)) {
	       int n_chirals = PyObject_Length(chiral_restraint_list);
	       for (int i_chiral=0; i_chiral<n_chirals; i_chiral++) {
		  PyObject *chiral_restraint = PyList_GetItem(chiral_restraint_list, i_chiral);
		  if (PyObject_Length(chiral_restraint) == 7) { 
		     PyObject *chiral_id_py= PyList_GetItem(chiral_restraint, 0);
		     PyObject *atom_c_py   = PyList_GetItem(chiral_restraint, 1);
		     PyObject *atom_1_py   = PyList_GetItem(chiral_restraint, 2);
		     PyObject *atom_2_py   = PyList_GetItem(chiral_restraint, 3);
		     PyObject *atom_3_py   = PyList_GetItem(chiral_restraint, 4);
		     PyObject *vol_sign_py = PyList_GetItem(chiral_restraint, 5);
		     PyObject *esd_py      = PyList_GetItem(chiral_restraint, 6);

		     if (PyString_Check(atom_1_py) &&
			 PyString_Check(atom_2_py) &&
			 PyString_Check(atom_3_py) &&
			 PyString_Check(atom_c_py) &&
			 PyString_Check(chiral_id_py) && 
			 PyFloat_Check(esd_py)    && 
			 PyInt_Check(vol_sign_py)) {
			std::string chiral_id = PyString_AsString(chiral_id_py);
			std::string atom_c    = PyString_AsString(atom_c_py);
			std::string atom_1    = PyString_AsString(atom_1_py);
			std::string atom_2    = PyString_AsString(atom_2_py);
			std::string atom_3    = PyString_AsString(atom_3_py);
			float  esd            = PyFloat_AsDouble(esd_py);
			int  vol_sign         = PyInt_AsLong(vol_sign_py);
			coot::dict_chiral_restraint_t rest(chiral_id,
							   atom_c, atom_1, atom_2, atom_3, 
							   vol_sign);
			chiral_restraints.push_back(rest);
		     }
		  }
	       }
	    }
	 }


	 if (key_string == "_chem_comp_plane_atom") {
	    PyObject *plane_restraint_list = value;
	    if (PyList_Check(plane_restraint_list)) {
	       int n_planes = PyObject_Length(plane_restraint_list);
	       for (int i_plane=0; i_plane<n_planes; i_plane++) {
		  PyObject *plane_restraint = PyList_GetItem(plane_restraint_list, i_plane);
		  if (PyObject_Length(plane_restraint) == 3) {
		     std::vector<std::string> atoms;
		     PyObject *plane_id_py = PyList_GetItem(plane_restraint, 0);
		     PyObject *esd_py      = PyList_GetItem(plane_restraint, 2);
		     PyObject *py_atoms_py = PyList_GetItem(plane_restraint, 1);

		     bool atoms_pass = 1;
		     if (PyList_Check(py_atoms_py)) {
			int n_atoms = PyObject_Length(py_atoms_py);
			for (int iat=0; iat<n_atoms; iat++) {
			   PyObject *at_py = PyList_GetItem(py_atoms_py, iat);
			   if (PyString_Check(at_py)) {
			      atoms.push_back(PyString_AsString(at_py));
			   } else {
			      atoms_pass = 0;
			   }
			}
			if (atoms_pass) {
			   if (PyString_Check(plane_id_py)) {
			      if (PyFloat_Check(esd_py)) {
				 std::string plane_id = PyString_AsString(plane_id_py);
				 float esd = PyFloat_AsDouble(esd_py);
				 if (atoms.size() > 0) { 
				    coot::dict_plane_restraint_t rest(plane_id, atoms[0], esd);
				    for (int i=1; i<atoms.size(); i++) {
				       double esd = 0.02;
				       rest.push_back_atom(atoms[i], esd);
				    }
				    plane_restraints.push_back(rest);
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
	
      coot::dictionary_residue_restraints_t monomer_restraints(monomer_type, 1);
      monomer_restraints.bond_restraint    = bond_restraints;
      monomer_restraints.angle_restraint   = angle_restraints;
      monomer_restraints.torsion_restraint = torsion_restraints;
      monomer_restraints.chiral_restraint  = chiral_restraints;
      monomer_restraints.plane_restraint   = plane_restraints;
      monomer_restraints.residue_info      = residue_info;
      monomer_restraints.atom_info         = atoms; 
      
      graphics_info_t g;
      bool s = g.Geom_p()->replace_monomer_restraints(monomer_type, monomer_restraints);
      if (s)
	 retval = Py_True;
      
   }
   if (PyBool_Check(retval)) {
     Py_INCREF(retval);
   }
   return retval;
} 
#endif // USE_PYTHON



/*  ----------------------------------------------------------------------- */
/*                  CCP4MG Interface                                        */
/*  ----------------------------------------------------------------------- */
void write_ccp4mg_picture_description(const char *filename) {

   std::ofstream mg_stream(filename);

   if (!mg_stream) {
      std::cout << "WARNING:: can't open file " << filename << std::endl;
   } else {
      mg_stream << "# CCP4mg picture definition file\n";
      mg_stream << "# See http://www.ysbl.york.ac.uk/~ccp4mg/ccp4mg_help/picture_definition.html\n";
      mg_stream << "# or your local copy of CCP4mg documentation.\n";
      mg_stream << "# created by Coot " << VERSION << "\n";

      // View:
      graphics_info_t g;
      coot::Cartesian rc = g.RotationCentre();
      mg_stream << "view = View (\n";
      mg_stream << "    centre_xyz = [";
      mg_stream << -rc.x() << ", " << -rc.y() << ", " << -rc.z() << "],\n";
      mg_stream << "    radius = " << 0.75*graphics_info_t::zoom << ",\n";
      //       mg_stream << "    orientation = [ " << g.quat[0] << ", "
      // 		<< g.quat[1] << ", " << g.quat[2] << ", " << g.quat[3] << "]\n";
      // Stuart corrects the orientation specification:
      mg_stream << "    orientation = [ " << -g.quat[3] << ", "
		<< g.quat[0] << ", " << g.quat[1] << ", " << g.quat[2] << "]\n";
      mg_stream << ")\n";

      // Parameters (maybe further down?)
      // GUI Parameters (for bg colour e.g.)
      mg_stream << "\n";
      mg_stream << "ParamsManager (\n";
      mg_stream << "   name = 'gui_params',\n";
      mg_stream << "   background_colour = [";
      mg_stream << (int)graphics_info_t::background_colour[0] * 255 << ", ";
      mg_stream << (int)graphics_info_t::background_colour[1] * 255 << ", ";
      mg_stream << (int)graphics_info_t::background_colour[2] * 255 << "]\n";
      mg_stream << ")\n";
      mg_stream << "\n";
      // Display Params (not working I think; default bond width for all)
      mg_stream << "\n";
      mg_stream << "ParamsManager (\n";
      mg_stream << "   name = 'model_drawing_style',\n";
      mg_stream << "   bond_width = ";
      mg_stream << graphics_info_t::default_bond_width << "         )\n";

      // Molecules:
      std::ostringstream map_colour_stream;
      for (int imol=0; imol<graphics_info_t::n_molecules(); imol++) {
	 if (is_valid_model_molecule(imol)) {
	    mg_stream << "MolData (\n";
	    mg_stream << "   filename = [\n";
	    mg_stream << "   'FULLPATH',\n";
	    mg_stream << "   " << single_quote(coot::util::absolutise_file_name(graphics_info_t::molecules[imol].name_)) << ",\n";
	    mg_stream << "   " << single_quote(coot::util::absolutise_file_name(graphics_info_t::molecules[imol].name_)) << "])\n";
	    mg_stream << "\n";
	    mg_stream << "MolDisp (\n";
	    mg_stream << "    select    = 'all',\n";
	    mg_stream << "    colour    = 'atomtype',\n"; 
	    mg_stream << "    style     = 'BONDS',\n";
	    mg_stream << "    selection_parameters = {\n";
	    mg_stream << "                'neighb_rad' : '5.0',\n";
	    mg_stream << "                'select' : 'all',\n";
	    mg_stream << "                'neighb_excl' : 0  },\n";
	    mg_stream << "    colour_parameters =  {\n";
	    mg_stream << "                'colour_mode' : 'atomtype',\n";
	    mg_stream << "                'user_scheme' : [ ]  })\n";
	    mg_stream << "\n";
	 }
	 if (is_valid_map_molecule(imol)) {
	    std::string phi = g.molecules[imol].save_phi_col;
	    std::string f   = g.molecules[imol].save_f_col;
	    std::string w   = g.molecules[imol].save_weight_col;
	    float lev       = g.molecules[imol].contour_level;
	    float r         = g.box_radius;
	    // first create a colour
	    map_colour_stream << "   coot_mapcolour" << imol << " = ["<<
	       graphics_info_t::molecules[imol].map_colour[0][0] << ", " <<
	       graphics_info_t::molecules[imol].map_colour[0][1] << ", " <<
	       graphics_info_t::molecules[imol].map_colour[0][2] << ", " <<
	       "1.0],\n";
	    //colour_definitions.push_back(map_colour);
	    // int use_weights_flag = g.molecules[imol].save_use_weights;
	    std::string name = single_quote(coot::util::absolutise_file_name(graphics_info_t::molecules[imol].save_mtz_file_name));
	    mg_stream << "MapData (\n";
	    mg_stream << "   filename = [\n";
	    mg_stream << "   'FULLPATH',\n";
	    mg_stream << "   " << name << ",\n";
	    mg_stream << "   " << name << "],\n";
	    mg_stream << "   phi = " << single_quote(phi) << ",\n";
	    mg_stream << "   f   = " << single_quote(f) << ",\n";
	    mg_stream << "   rate = 0.75\n";
	    mg_stream << ")\n";
	    mg_stream << "MapDisp (\n";
	    mg_stream << "    contour_level = " << lev << ",\n";
	    mg_stream << "    surface_style = 'chickenwire',\n";
	    mg_stream << "    radius = " << r << ",\n";
	    mg_stream << "    colour = 'coot_mapcolour" << imol << "',\n";
	    mg_stream << "    clip_mode = 'OFF')\n";
	    mg_stream << "\n";
	    if (map_is_difference_map(imol)) {
		   // make a second contour level
		   mg_stream << "MapDisp (\n";
		   mg_stream << "    contour_level = -" << lev << ",\n";
		   mg_stream << "    surface_style = 'chickenwire',\n";
		   mg_stream << "    radius = " << r << ",\n";
		   mg_stream << "    colour = 'coot_mapcolour" << imol << "_2',\n";
		   mg_stream << "    clip_mode = 'OFF')\n";
		   mg_stream << "\n";
		   // and a color
		   map_colour_stream << "   coot_mapcolour" << imol << "_2 = ["<<
		      graphics_info_t::molecules[imol].map_colour[0][1] << ", " <<
		      graphics_info_t::molecules[imol].map_colour[0][0] << ", " <<
		      graphics_info_t::molecules[imol].map_colour[0][2] << ", " <<
		      "1.0],\n";
		}
	 }
      }
      // now define map colours
      mg_stream << "Colours (\n";
      mg_stream << map_colour_stream.str();
      mg_stream << ")\n";
      mg_stream << "\n";

   }
}

// function to get the atom colours to be displayed in CCP4MG?!
char *get_atom_colour_from_mol_no(int imol, const char *element) {

   char *r;
   std::vector<float> rgb(3);
   float rotation_size = graphics_info_t::molecules[imol].bonds_colour_map_rotation/360.0;
   while (rotation_size > 1.0) { // no more black bonds?
      rotation_size -= 1.0;
   }
   int i_element;
   i_element = atom_colour(element);
   switch (i_element) {
   case YELLOW_BOND: 
      rgb[0] = 0.8; rgb[1] =  0.8; rgb[2] =  0.3;
      break;
   case BLUE_BOND: 
      rgb[0] = 0.5; rgb[1] =  0.5; rgb[2] =  1.0;
      break;
   case RED_BOND: 
      rgb[0] = 1.0; rgb[1] =  0.3; rgb[2] =  0.3;
      break;
   case GREEN_BOND:
      rgb[0] = 0.1; rgb[1] =  0.99; rgb[2] =  0.1;
      break;
   case GREY_BOND: 
      rgb[0] = 0.7; rgb[1] =  0.7; rgb[2] =  0.7;
      break;
   case HYDROGEN_GREY_BOND: 
      rgb[0] = 0.6; rgb[1] =  0.6; rgb[2] =  0.6;
      break;
   case MAGENTA_BOND:
      rgb[0] = 0.99; rgb[1] =  0.2; rgb[2] = 0.99;
      break;
   case ORANGE_BOND:
      rgb[0] = 0.89; rgb[1] =  0.89; rgb[2] = 0.1;
      break;
   case CYAN_BOND:
      rgb[0] = 0.1; rgb[1] =  0.89; rgb[2] = 0.89;
      break;
      
   default:
      rgb[0] = 0.8; rgb[1] =  0.2; rgb[2] =  0.2;
      rgb = rotate_rgb(rgb, float(imol*26.0/360.0));

   }
   // "correct" for the +1 added in the calculation of the rotation
   // size.
   // 21. is the default colour map rotation
   rgb = rotate_rgb(rgb, float(1.0 - 21.0/360.0));
   if (graphics_info_t::rotate_colour_map_on_read_pdb_c_only_flag) {
      if (i_element == YELLOW_BOND) { 
	 rgb = rotate_rgb(rgb, rotation_size);
      } 
   } else {
//       std::cout << "DEBUG: rotating coordinates colour map by "
//                 << rotation_size * 360.0 << " degrees " << std::endl;
         rgb = rotate_rgb(rgb, rotation_size);
   }

   std::string rgb_list = "[";
   rgb_list += graphics_info_t::float_to_string(rgb[0]);
   rgb_list += ", ";
   rgb_list += graphics_info_t::float_to_string(rgb[1]);
   rgb_list += ", ";
   rgb_list += graphics_info_t::float_to_string(rgb[2]);
   rgb_list += "]";
   r = (char*)rgb_list.c_str();
   return r;
}


/*  ----------------------------------------------------------------------- */
/*                  Restratints                                             */
/*  ----------------------------------------------------------------------- */

void write_restraints_cif_dictionary(const char *monomer_type, const char *file_name) {

   graphics_info_t g;
   std::pair<short int, coot::dictionary_residue_restraints_t> r = 
      g.Geom_p()->get_monomer_restraints(monomer_type);
   if (!r.first)
      std::cout << "Failed to find " << monomer_type << " in dictionary" << std::endl;
   else
      r.second.write_cif(file_name);
} 

/*  ----------------------------------------------------------------------- */
/*                  PKGDATDDIR                                              */
/*  ----------------------------------------------------------------------- */
#ifdef USE_PYTHON
PyObject *get_pkgdatadir_py() {

  std::string pkgdatadir = PKGDATADIR;
  return PyString_FromString(pkgdatadir.c_str());
}
#endif

#ifdef USE_GUILE
SCM get_pkgdatadir_scm() {

   std::string pkgdatadir = PKGDATADIR;
   return scm_from_locale_string(pkgdatadir.c_str());
} 
#endif


/* refmac version */
/* returns:
   1 if 5.4 or newer
   2 if 5.5 or newer */

int refmac_runs_with_nolabels() {

  int ret = 0;

 
#ifdef USE_GUILE
  SCM refmac_version = safe_scheme_command("(get-refmac-version)");
  if (scm_is_true(scm_list_p(refmac_version))) {
     int major = scm_to_int(scm_list_ref(refmac_version, SCM_MAKINUM(0)));
     int minor = scm_to_int(scm_list_ref(refmac_version, SCM_MAKINUM(1)));
     if ((major == 5 && minor >= 4) || (major > 5)) {
	ret = 1;
	if (minor >= 5 || major > 5) {
	  // for SAD and TWIN
	  ret = 2;
	}
     }
  }
  
#else
#ifdef USE_PYTHON
  PyObject *refmac_version;
  refmac_version = safe_python_command_with_return("get_refmac_version()");
  if (refmac_version) {
     int major = PyInt_AsLong(PyList_GetItem(refmac_version, 0));
     int minor = PyInt_AsLong(PyList_GetItem(refmac_version, 1));
     if ((major == 5 && minor >= 4) || (major > 5)) {
	ret = 1;
	if (minor >= 5 || major > 5) {
	  // for SAD and TWIN
	  ret = 2;
	}
     }
  }
  Py_XDECREF(refmac_version);
#endif
#endif

  

  return ret;
}

#ifdef USE_GUILE
SCM ccp4i_projects_scm() {
   SCM r = SCM_EOL;
   std::string ccp4_defs_file_name = graphics_info_t::ccp4_defs_file_name();
   std::vector<std::pair<std::string, std::string> > project_pairs =
      parse_ccp4i_defs(ccp4_defs_file_name);
   for (unsigned int i=0; i<project_pairs.size(); i++) {
      SCM p = SCM_EOL;
      p = scm_cons(scm_makfrom0str(project_pairs[i].second.c_str()), p);
      p = scm_cons(scm_makfrom0str(project_pairs[i].first.c_str()),  p);
      r = scm_cons(p, r);
   }
   r = scm_reverse(r);
   return r;
} 
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *ccp4i_projects_py() {
   PyObject *r = PyList_New(0);
   std::string ccp4_defs_file_name = graphics_info_t::ccp4_defs_file_name();
   std::vector<std::pair<std::string, std::string> > project_pairs =
      parse_ccp4i_defs(ccp4_defs_file_name);
   for (unsigned int i=0; i<project_pairs.size(); i++) {
      PyObject *p = PyList_New(2);
      PyList_SetItem(p, 0, PyString_FromString(project_pairs[i].first.c_str()));
      PyList_SetItem(p, 1, PyString_FromString(project_pairs[i].second.c_str()));
      PyList_Append(r, p);
      Py_XDECREF(p);
   }
   return r;
} 
#endif // USE_PYTHON



#ifdef USE_GUILE
SCM remarks_scm(int imol) {

   SCM r = SCM_EOL;
   
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      mmdb::TitleContainer *tc_p = mol->GetRemarks();
      int l = tc_p->Length();
      for (unsigned int i=0; i<l; i++) { 
	 mmdb::Remark *cr = static_cast<mmdb::Remark *> (tc_p->GetContainerClass(i));
	 SCM a_scm = SCM_MAKINUM(cr->remarkNum);
	 SCM b_scm = scm_makfrom0str(cr->remark);
	 SCM l2 = SCM_LIST2(a_scm, b_scm);
	 r = scm_cons(l2, r);
      }
      r = scm_reverse(r); // undo schemey backwardsness
   }
   return r;
}

#endif


#ifdef USE_PYTHON
PyObject *remarks_py(int imol) {

   PyObject *o = Py_False;

   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      mmdb::TitleContainer *tc_p = mol->GetRemarks();
      int n_records = tc_p->Length();
      o = PyList_New(n_records);
      for (unsigned int i=0; i<n_records; i++) {
	 mmdb::Remark *cr = static_cast<mmdb::Remark *> (tc_p->GetContainerClass(i));
	 PyObject *l = PyList_New(2);
	 PyList_SetItem(l, 0, PyInt_FromLong(cr->remarkNum));
	 PyList_SetItem(l, 1, PyString_FromString(cr->remark));
	 PyList_SetItem(o, i, l);
      }
   }
   if (PyBool_Check(o))
      Py_INCREF(o);
   return o;
}

#endif


#ifdef USE_GUILE
SCM residue_centre_scm(int imol, const char *chain_id, int resno, const char *ins_code) {

   SCM r = SCM_BOOL_F;

   if (is_valid_model_molecule(imol)) {
      std::pair<bool, clipper::Coord_orth> rr =
	 graphics_info_t::molecules[imol].residue_centre(chain_id, resno, ins_code);
      if (rr.first) {
	 r = SCM_LIST3(scm_double2num(rr.second.x()),
		       scm_double2num(rr.second.y()),
		       scm_double2num(rr.second.z()));
      } 
   } 
   return r;
} 

#endif


#ifdef USE_PYTHON
PyObject *residue_centre_py(int imol, const char *chain_id, int resno, const char *ins_code) {

   PyObject *r = Py_False;

   if (is_valid_model_molecule(imol)) {
      std::pair<bool, clipper::Coord_orth> rr =
	 graphics_info_t::molecules[imol].residue_centre(chain_id, resno, ins_code);
      if (rr.first) {
	 r = PyList_New(3);
	 PyList_SetItem(r, 0, PyFloat_FromDouble(rr.second.x()));
	 PyList_SetItem(r, 1, PyFloat_FromDouble(rr.second.y()));
	 PyList_SetItem(r, 2, PyFloat_FromDouble(rr.second.z()));
      } 
   } 
   if (PyBool_Check(r)) {
      Py_INCREF(r);
   }
   return r;
} 

#endif


#ifdef USE_GUILE
SCM link_info_scm(int imol) {

   SCM r = SCM_EOL;
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      if (mol) {

	 for(int imod = 1; imod<=mol->GetNumberOfModels(); imod++) {

	    mmdb::Model *model_p = mol->GetModel(imod);
	    int n_links = model_p->GetNumberOfLinks();
	    if (n_links > 0) { 
	       for (int i_link=1; i_link<=n_links; i_link++) {
		  mmdb::PLink link = model_p->GetLink(i_link);

		  std::pair<coot::atom_spec_t, coot::atom_spec_t> atoms = coot::link_atoms(link);
		  SCM l = scm_list_3(SCM_MAKINUM(imod),
				     atom_spec_to_scm(atoms.first),
				     atom_spec_to_scm(atoms.second));
		  r = scm_cons(l,r);
	       }
	    }
	 }
	 r = scm_reverse(r);
      }
   }
   return r;
}

#endif 




#ifdef USE_PYTHON
PyObject *link_info_py(int imol) {

   PyObject *r = PyList_New(0);
   if (is_valid_model_molecule(imol)) {
      mmdb::Manager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      if (mol) {
	 for(int imod = 1; imod<=mol->GetNumberOfModels(); imod++) {
	    mmdb::Model *model_p = mol->GetModel(imod);
	    int n_links = model_p->GetNumberOfLinks();
	    if (n_links > 0) { 
	       for (int i_link=1; i_link<=n_links; i_link++) {
		  mmdb::PLink link = model_p->GetLink(i_link);
		  std::pair<coot::atom_spec_t, coot::atom_spec_t> atoms = coot::link_atoms(link);
		  PyObject *l = PyList_New(3);
		  PyList_SetItem(l, 0, PyInt_FromLong(imod));
		  PyList_SetItem(l, 1, atom_spec_to_py(atoms.first));
		  PyList_SetItem(l, 2, atom_spec_to_py(atoms.second));
		  PyList_Append(r, l);
	       }
	    }
	 }
      }
   }
   return r;
}

#endif 
