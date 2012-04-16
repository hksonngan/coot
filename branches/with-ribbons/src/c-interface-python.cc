/* src/c-interface-python.cc
 * 
 * Copyright 2008 by The University of York
 * Author: Bernhard Lohkamp
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

#include <string>
#include <vector>

#include "coot-utils.hh"
#include "c-interface-python.hh"

#ifdef USE_PYTHON

// This is a common denominator really.  It does not depend on mmdb,
// but it can't be declared in c-interface.h because then we'd have to
// include c-interface.h which would cause (resolvable, I think, not
// checked) problems.
// 
// return a scm string, decode to c++ using scm_to_locale_string();
//SCM display_scm(SCM o) {

//   SCM dest = SCM_BOOL_F;
//   SCM mess = scm_makfrom0str("object: ~s\n");
//   return scm_simple_format(dest, mess, scm_list_1(o));
//}


// e.g. ["B", 41, "", " CA ", ""]
std::pair<bool, coot::atom_spec_t>
make_atom_spec_py(PyObject *spec) {

   bool good_spec = 0;
   coot::atom_spec_t as;
   int spec_length = PyObject_Length(spec);

   if (spec_length == 5) {
      PyObject  *chain_id_py = PyList_GetItem(spec, 0);
      PyObject     *resno_py = PyList_GetItem(spec, 1);
      PyObject  *ins_code_py = PyList_GetItem(spec, 2);
      PyObject *atom_name_py = PyList_GetItem(spec, 3);
      PyObject  *alt_conf_py = PyList_GetItem(spec, 4);
      std::string chain_id = PyString_AsString(chain_id_py);
      int resno = PyInt_AsLong(resno_py);
      std::string ins_code  = PyString_AsString(ins_code_py);
      std::string atom_name = PyString_AsString(atom_name_py);
      std::string alt_conf  = PyString_AsString(alt_conf_py);
      as = coot::atom_spec_t(chain_id, resno, ins_code, atom_name, alt_conf);
      good_spec = 1;
   }
   return std::pair<bool, coot::atom_spec_t> (good_spec, as);
}


std::pair<bool, coot::residue_spec_t>
make_residue_spec_py(PyObject *spec) {

   bool good_spec = 0;
   coot::residue_spec_t rs("A", 1);
   int spec_length = PyObject_Length(spec);

   if (spec_length == 3) {
      PyObject  *chain_id_py = PyList_GetItem(spec, 0);
      PyObject     *resno_py = PyList_GetItem(spec, 1);
      PyObject  *ins_code_py = PyList_GetItem(spec, 2);
      std::string chain_id = PyString_AsString(chain_id_py);
      int resno = PyInt_AsLong(resno_py);
      std::string ins_code  = PyString_AsString(ins_code_py);
      rs = coot::residue_spec_t(chain_id, resno, ins_code);
      good_spec = 1;
   }
   return std::pair<bool, coot::residue_spec_t> (good_spec, rs);
}

// return -1 on string/symbol not found
int key_sym_code_py(PyObject *po) {

   int r = -1;
   if (PyString_Check(po)) { 
      std::string s = PyString_AsString(po);
      r = coot::util::decode_keysym(s);
   }
   return r;
}


#endif // USE_PYTHON