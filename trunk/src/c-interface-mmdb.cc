/* src/c-interface-mmdb.cc
 * 
 * Copyright 2007 The University of York
 * Author: Paul Emsley
 * Copyright 2007 Bernhard Lohkamp
 * Copyright 2007 University of York
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
#include <string>

#if defined(_MSC_VER) || defined(WINDOWS_MINGW)
#define AddAtomA AddAtom
#endif

#include "c-interface-mmdb.hh"

#ifdef USE_GUILE

   // Instead of cut and pasting here, perhaps I should have done a typedef?
   
#if (SCM_MAJOR_VERSION > 1) || (SCM_MINOR_VERSION > 7)
// no fix up needed 
#else    
#define scm_to_int gh_scm2int
#define scm_to_locale_string SCM_STRING_CHARS
#define scm_to_double  gh_scm2double
#define  scm_is_true gh_scm2bool
#endif // SCM version

CMMDBManager *
mmdb_manager_from_scheme_expression(SCM molecule_expression) {

   CMMDBManager *mol = 0;

   SCM nmodel = scm_length(molecule_expression);
   int inmodel = scm_to_int(nmodel);

   if (inmodel > 0) {
      mol = new CMMDBManager; 
      for(int imodel=0; imodel<inmodel; imodel++) {
	 CModel *model_p = new CModel;
	 SCM imodel_scm = SCM_MAKINUM(imodel);
	 SCM model_expression = scm_list_ref(molecule_expression, imodel_scm);
	 SCM model_expression_length = scm_length(model_expression);
	 int len_model_expression = scm_to_int(model_expression_length);
	 if (len_model_expression > 0) {
	    // SCM chain_list = model_expression; // interesting
	    int nchains = len_model_expression;

	    for (int ichain=0; ichain<nchains; ichain++) {
	       
	       SCM chain_expression = scm_list_ref(model_expression,
						   SCM_MAKINUM(ichain));
	       SCM chain_is_list_scm = scm_list_p(chain_expression);
	       if (scm_is_true(chain_is_list_scm)) {
		  // printf("chain_expression is a list\n");
	       } else {
		  printf("chain_expression is not a list\n");
	       }
	       // 	    display_scm(chain_expression);
	       SCM chain_expression_length = scm_length(chain_expression);
	       int len_chain_expr = scm_to_int(chain_expression_length);
	       if (len_chain_expr != 2) {
		  std::cout << "bad chain expression, length "
			    << len_chain_expr << std::endl;
	       } else {
		  // normal case
		  // std::cout << "good chain expression " << std::endl;
		  SCM chain_id_scm = scm_list_ref(chain_expression, SCM_MAKINUM(0));
		  SCM residues_list = scm_list_ref(chain_expression, SCM_MAKINUM(1));
		  if (scm_is_true(scm_list_p(residues_list))) {
		     // printf("residues_list is a list\n");
		  } else {
		     printf("residue_list is not a list\n");
		  }
		  SCM n_residues_scm = scm_length(residues_list);
		  int n_residues = scm_to_int(n_residues_scm);
		  // printf("there were %d residues\n", n_residues);
		  if (n_residues > 0) {
		     CChain *chain_p = new CChain;
		     std::string chain_id = scm_to_locale_string(chain_id_scm);
		     chain_p->SetChainID(chain_id.c_str());
		     for (int ires=0; ires<n_residues; ires++) {
			SCM ires_scm = SCM_MAKINUM(ires);
			SCM scm_residue = scm_list_ref(residues_list, ires_scm);
			SCM scm_len_residue_expr = scm_length(scm_residue);
			int len_residue_expr = scm_to_int(scm_len_residue_expr);
			if (len_residue_expr != 4) {
			   std::cout << "bad residue expression, length "
				     << len_residue_expr << std::endl;
			} else {
			   // normal case
			   // std::cout << "good residue expression" << std::endl;
			   SCM scm_residue_number = scm_list_ref(scm_residue, SCM_MAKINUM(0));
			   SCM scm_residue_inscode = scm_list_ref(scm_residue, SCM_MAKINUM(1));
			   SCM scm_residue_name = scm_list_ref(scm_residue, SCM_MAKINUM(2));
			   SCM atoms_list = scm_list_ref(scm_residue, SCM_MAKINUM(3));

			   if (scm_is_true(scm_list_p(atoms_list))) {
			      // printf("atoms_list is a list\n");
			   } else {
			      printf("atoms_list is not a list\n");
			   }
			   SCM n_atoms_scm = scm_length(atoms_list);
			   int n_atoms = scm_to_int(n_atoms_scm);
			   if (n_atoms > 0) {
			      CResidue *residue_p = new CResidue;
			      std::string resname = scm_to_locale_string(scm_residue_name);
			      int resno = scm_to_int(scm_residue_number);
			      // std::cout << "DEBUG:: Found resno:   " << resno << std::endl;
			      // std::cout << "DEBUG:: Found resname: " << resname << std::endl;
			      std::string inscode = scm_to_locale_string(scm_residue_inscode);
			      residue_p->SetResName(resname.c_str());
			      residue_p->seqNum = resno;
			      memcpy(residue_p->insCode, inscode.c_str(), sizeof(InsCode));
			      for (int iat=0; iat<n_atoms; iat++) {
				 SCM iat_scm = SCM_MAKINUM(iat);
				 SCM atom_expression = scm_list_ref(atoms_list, iat_scm);
				 SCM len_atom_expr_scm = scm_length(atom_expression);
				 int len_atom_expr = scm_to_int(len_atom_expr_scm);
				 if (len_atom_expr != 3) {
				    std::cout << "bad atom expression, length "
					      << len_residue_expr << std::endl;
				    SCM dest = SCM_BOOL_F;
				    SCM mess = scm_makfrom0str("object: ~S\n");
				    SCM bad_scm = scm_simple_format(dest, mess, scm_list_1(atom_expression));
				    std::string bad_str = scm_to_locale_string(bad_scm);
				    std::cout << bad_str << std::endl;
				 } else {
				    // normal case
				    // std::cout << "good atom expression " << std::endl;
				    SCM name_alt_conf_pair = scm_list_ref(atom_expression, SCM_MAKINUM(0));
				    SCM occ_b_ele = scm_list_ref(atom_expression, SCM_MAKINUM(1));
				    SCM pos_expr =  scm_list_ref(atom_expression, SCM_MAKINUM(2));
				    SCM len_name_alt_conf_scm = scm_length(name_alt_conf_pair);
				    int len_name_alt_conf = scm_to_int(len_name_alt_conf_scm);
				    SCM len_occ_b_ele_scm = scm_length(occ_b_ele);
				    int len_occ_b_ele = scm_to_int(len_occ_b_ele_scm);
				    SCM len_pos_expr_scm = scm_length(pos_expr);
				    int len_pos_expr = scm_to_int(len_pos_expr_scm);
				    if (len_name_alt_conf == 2) {
				       if (len_occ_b_ele == 3) {
					  if (len_pos_expr == 3) {
					     SCM atom_name_scm = SCM_CAR(name_alt_conf_pair);
					     std::string atom_name = scm_to_locale_string(atom_name_scm);
					     SCM alt_conf_scm = SCM_CAR(SCM_CDR(name_alt_conf_pair));
					     std::string alt_conf = scm_to_locale_string(alt_conf_scm);
					     SCM occ_scm = scm_list_ref(occ_b_ele, SCM_MAKINUM(0));
					     SCM b_scm   = scm_list_ref(occ_b_ele, SCM_MAKINUM(1));
					     SCM ele_scm = scm_list_ref(occ_b_ele, SCM_MAKINUM(2));
					     float b = scm_to_double(b_scm);
					     float occ = scm_to_double(occ_scm);
					     std::string ele = scm_to_locale_string(ele_scm);
					     float x = scm_to_double(scm_list_ref(pos_expr, SCM_MAKINUM(0)));
					     float y = scm_to_double(scm_list_ref(pos_expr, SCM_MAKINUM(1)));
					     float z = scm_to_double(scm_list_ref(pos_expr, SCM_MAKINUM(2)));
					     CAtom *atom = new CAtom;
					     atom->SetCoordinates(x, y, z, occ, b);
					     atom->SetAtomName(atom_name.c_str());
					     atom->SetElementName(ele.c_str());
					     strncpy(atom->altLoc, alt_conf.c_str(), 2);
					     residue_p->AddAtom(atom);
					     // std::cout << "DEBUG:: adding atom " << atom << std::endl;
					  } else {
					     std::cout << "bad atom (position expression) "
						       << std::endl;
					     SCM bad_scm = display_scm(pos_expr);
					     std::string bad_str = scm_to_locale_string(bad_scm);
					     std::cout << bad_str << std::endl;
					  }
				       } else {
					  std::cout << "bad atom (occ b element expression) "
						    << std::endl;
					  SCM bad_scm = display_scm(occ_b_ele);
					  std::string bad_str = scm_to_locale_string(bad_scm);
					  std::cout << bad_str << std::endl;
				       }
				    } else {
				       std::cout << "bad atom (name alt-conf expression) "
						 << std::endl;
				       SCM bad_scm = display_scm(name_alt_conf_pair);
				       std::string bad_str = scm_to_locale_string(bad_scm);
				       std::cout << bad_str << std::endl;
				    }
				 }
			      }
			      chain_p->AddResidue(residue_p);
			   }
			}
		     }
		     model_p->AddChain(chain_p);
		  }
	       }
	       mol->AddModel(model_p);
	    }
	 }
      }
   }
   return mol;
} 

#endif  // USE_GUILE
#ifdef USE_PYTHON

CMMDBManager *
mmdb_manager_from_python_expression(PyObject *molecule_expression) {

   CMMDBManager *mol = 0;

   int inmodel = PyObject_Length(molecule_expression);

   if (inmodel > 0) {
      mol = new CMMDBManager; 
      for(int imodel=0; imodel<inmodel; imodel++) {
         CModel *model_p = new CModel;
         PyObject *model_expression = PyList_GetItem(molecule_expression, imodel);
         int len_model_expression = PyObject_Length(model_expression);
         if (len_model_expression > 0) {
            PyObject *chain_list = model_expression; // interesting
            int nchains = len_model_expression;

            for (int ichain=0; ichain<nchains; ichain++) {
               
               PyObject *chain_expression = PyList_GetItem(model_expression,
                                                   ichain);
               int chain_is_list_python = PyList_Size(chain_expression);
               if (chain_is_list_python > 0) {
                  // printf("chain_expression is a list\n");
               } else {
                  printf("chain_expression is not a list\n");
               }
               //           display_scm(chain_expression);
               int len_chain_expr = PyObject_Length(chain_expression);
               if (len_chain_expr != 2) {
                  std::cout << "bad chain expression, length "
                            << len_chain_expr << std::endl;
               } else {
                  // normal case
                  // std::cout << "good chain expression " << std::endl;
                  std::cout << "good chain expression " << std::endl;
                  PyObject *chain_id_python = PyList_GetItem(chain_expression, 0);
                  PyObject *residues_list = PyList_GetItem(chain_expression, 1);
                  if (PyList_Size(residues_list) > 0) {
                     // printf("residues_list is a list\n");
                  } else {
                     printf("residue_list is not a list\n");
                  }
                  int n_residues = PyObject_Length(residues_list);
		  // printf("there were %d residues\n", n_residues);
                  printf("there were %d residues\n", n_residues);
                  if (n_residues > 0) {
                     CChain *chain_p = new CChain;
                     std::string chain_id = PyString_AsString(chain_id_python);
                     chain_p->SetChainID(chain_id.c_str());
                     for (int ires=0; ires<n_residues; ires++) {
                        PyObject *python_residue = PyList_GetItem(residues_list, ires);
                        int len_residue_expr = PyObject_Length(python_residue);
                        if (len_residue_expr != 4) {
                           std::cout << "bad residue expression, length "
                                     << len_residue_expr << std::endl;
                        } else {
                           // normal case
                           // std::cout << "good residue expression" << std::endl;
                           PyObject *python_residue_number = PyList_GetItem(python_residue, 0);
                           PyObject *python_residue_inscode = PyList_GetItem(python_residue, 1);
                           PyObject *python_residue_name = PyList_GetItem(python_residue, 2);
                           PyObject *atoms_list = PyList_GetItem(python_residue, 3);

                           if (PyList_Size(atoms_list) > 0) {
                              // printf("atoms_list is a list\n");
                           } else {
                              printf("atoms_list is not a list\n");
                           }
                           int n_atoms = PyObject_Length(atoms_list);
                           if (n_atoms > 0) {
                              CResidue *residue_p = new CResidue;
                              std::string resname = PyString_AsString(python_residue_name);
                              int resno = PyInt_AsLong(python_residue_number);
                              // std::cout << "DEBUG:: Found resno:   " << resno << std::endl;
                              // std::cout << "DEBUG:: Found resname: " << resname << std::endl;
                              std::string inscode = PyString_AsString(python_residue_inscode);
                              residue_p->SetResName(resname.c_str());
                              residue_p->seqNum = resno;
                              memcpy(residue_p->insCode, inscode.c_str(), sizeof(InsCode));
                              for (int iat=0; iat<n_atoms; iat++) {
                                 PyObject *atom_expression = PyList_GetItem(atoms_list, iat);
                                 int len_atom_expr = PyObject_Length(atom_expression);
                                 if (len_atom_expr != 3) {
                                    std::cout << "bad atom expression, length "
                                              << len_residue_expr << std::endl;
                                    char *mess =  "object: %S\n";
                                    PyObject *bad_python = PyString_FromFormat(mess, atom_expression);
                                    std::string bad_str = PyString_AsString(bad_python);
				    Py_DECREF(bad_python);
                                    std::cout << bad_str << std::endl;
                                 } else {
                                    // normal case
                                    // std::cout << "good atom expression " << std::endl;
                                    PyObject *name_alt_conf_pair = PyList_GetItem(atom_expression, 0);
                                    PyObject *occ_b_ele = PyList_GetItem(atom_expression, 1);
                                    PyObject *pos_expr = PyList_GetItem(atom_expression, 2);
                                    int len_name_alt_conf = PyObject_Length(name_alt_conf_pair);
                                    int len_occ_b_ele = PyObject_Length(occ_b_ele);
                                    int len_pos_expr = PyObject_Length(pos_expr);
                                    if (len_name_alt_conf == 2) {
                                       if (len_occ_b_ele == 3) {
                                          if (len_pos_expr == 3) {
                                             PyObject *atom_name_python = PyList_GetItem(name_alt_conf_pair, 0);
                                             std::string atom_name = PyString_AsString(atom_name_python);
                                             PyObject *alt_conf_python = PyList_GetItem(name_alt_conf_pair, 1);
                                             std::string alt_conf = PyString_AsString(alt_conf_python);
                                             PyObject *occ_python = PyList_GetItem(occ_b_ele, 0);
                                             PyObject *b_python = PyList_GetItem(occ_b_ele, 1);
                                             PyObject *ele_python = PyList_GetItem(occ_b_ele, 2);
                                             float b = PyFloat_AsDouble(b_python);
                                             float occ = PyFloat_AsDouble(occ_python);
                                             std::string ele = PyString_AsString(ele_python);
                                             float x = PyFloat_AsDouble(PyList_GetItem(pos_expr, 0));
                                             float y = PyFloat_AsDouble(PyList_GetItem(pos_expr, 1));
                                             float z = PyFloat_AsDouble(PyList_GetItem(pos_expr, 2));
                                             CAtom *atom = new CAtom;
                                             atom->SetCoordinates(x, y, z, occ, b);
                                             atom->SetAtomName(atom_name.c_str());
                                             atom->SetElementName(ele.c_str());
                                             strncpy(atom->altLoc, alt_conf.c_str(), 2);
                                             residue_p->AddAtom(atom);
					     Py_DECREF(atom_name_python);
					     Py_DECREF(alt_conf_python);
					     Py_DECREF(occ_python);
					     Py_DECREF(b_python);
					     Py_DECREF(ele_python);
                                             // std::cout << "DEBUG:: adding atom " << atom << std::endl;
                                          } else {
                                             std::cout << "bad atom (position expression) "
                                                       << std::endl;
                                             PyObject *bad_python = display_python(pos_expr);
                                             std::string bad_str = PyString_AsString(bad_python);
                                             std::cout << bad_str << std::endl;
					     Py_DECREF(bad_python);
                                          }
                                       } else {
                                          std::cout << "bad atom (occ b element expression) "
                                                    << std::endl;
                                          PyObject *bad_python = display_python(occ_b_ele);
                                          std::string bad_str = PyString_AsString(bad_python);
                                          std::cout << bad_str << std::endl;
					  Py_DECREF(bad_python);
                                       }
                                    } else {
                                       std::cout << "bad atom (name alt-conf expression) "
                                                 << std::endl;
                                       PyObject *bad_python = display_python(name_alt_conf_pair);
                                       std::string bad_str = PyString_AsString(bad_python);
                                       std::cout << bad_str << std::endl;
				       Py_DECREF(bad_python);
                                    }
				    Py_DECREF(name_alt_conf_pair);
				    Py_DECREF(occ_b_ele);
				    Py_DECREF(pos_expr);
                                 }
				 Py_DECREF(atom_expression);
                              }
                              chain_p->AddResidue(residue_p);
                           }
			   Py_DECREF(python_residue_number);
			   Py_DECREF(python_residue_inscode);
			   Py_DECREF(python_residue_name);
			   Py_DECREF(atoms_list);
                        }
			Py_DECREF(python_residue);
                     }
                     model_p->AddChain(chain_p);
                  }
		  Py_DECREF(chain_id_python);
		  Py_DECREF(residues_list);
               }
	       Py_DECREF(chain_expression);
               mol->AddModel(model_p);
            }
	    Py_DECREF(chain_list);
         }
	 Py_DECREF(model_expression);
      }
   }
   return mol;
} 

// This is a common denominator really.  It does not depend on mmdb,
// but it can't be declared in c-interface.h because then we'd have to
// include c-interface.h which would cause (resolvable, I think, not
// checked) problems.
// 
// return a python string, decode to c++ using PyString_FromFormat();
// keep it here for now. Maybe should go in something like c-interface-python.cc ....
PyObject * display_python(PyObject *o) {

   PyObject *dest;
   dest = Py_False;
   char *mess = "object: %s\n";
   Py_DECREF(dest);
   return PyString_FromFormat(mess, o);
}

#endif // USE_PYTHON

