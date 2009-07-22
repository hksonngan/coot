/* src/c-interface-scm.cc
 * 
 * Copyright 2007, 2008, 2009 by The University of Oxford
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

#include <string>
#include <vector>

#include "coot-utils.hh"
#include "c-interface-scm.hh"

#include "graphics-info.h"

#ifdef USE_GUILE

#include "guile-fixups.h"


// This is a common denominator really.  It does not depend on mmdb,
// but it can't be declared in c-interface.h because then we'd have to
// include c-interface.h which would cause (resolvable, I think, not
// checked) problems.
// 
// return a scm string, decode to c++ using scm_to_locale_string();
SCM display_scm(SCM o) {

   SCM dest = SCM_BOOL_F;
   SCM mess = scm_makfrom0str("object: ~s\n");
   return scm_simple_format(dest, mess, scm_list_1(o));
}

bool scm_is_undefined(SCM o) {

   // #t and #f don't have that bit set.  Not sure about others, but
   // undefined does.
   return (1024 & SCM_UNPACK(o)) > 0 ? 1 : 0;
}

// e.g. '("B" 41 "" " CA " "")
std::pair<bool, coot::atom_spec_t>
make_atom_spec(SCM spec) {

   bool good_spec = 0;
   coot::atom_spec_t as;
   SCM spec_length_scm = scm_length(spec);
   int spec_length = scm_to_int(spec_length_scm);

   if (spec_length == 5) {
      SCM  chain_id_scm = scm_list_ref(spec, SCM_MAKINUM(0));
      SCM     resno_scm = scm_list_ref(spec, SCM_MAKINUM(1));
      SCM  ins_code_scm = scm_list_ref(spec, SCM_MAKINUM(2));
      SCM atom_name_scm = scm_list_ref(spec, SCM_MAKINUM(3));
      SCM  alt_conf_scm = scm_list_ref(spec, SCM_MAKINUM(4));
      std::string chain_id = scm_to_locale_string(chain_id_scm);
      int resno = scm_to_int(resno_scm);
      std::string ins_code  = scm_to_locale_string(ins_code_scm);
      std::string atom_name = scm_to_locale_string(atom_name_scm);
      std::string alt_conf  = scm_to_locale_string(alt_conf_scm);
      as = coot::atom_spec_t(chain_id, resno, ins_code, atom_name, alt_conf);
      good_spec = 1;
   }
   return std::pair<bool, coot::atom_spec_t> (good_spec, as);
}


std::pair<bool, coot::residue_spec_t>
make_residue_spec(SCM spec) {
   bool good_spec = 0;
   coot::residue_spec_t rs("A", 1);
   SCM spec_length_scm = scm_length(spec);
   int spec_length = scm_to_int(spec_length_scm);
   if (spec_length == 3) {
      SCM chain_id_scm = scm_list_ref(spec, SCM_MAKINUM(0));
      SCM resno_scm    = scm_list_ref(spec, SCM_MAKINUM(1));
      SCM ins_code_scm = scm_list_ref(spec, SCM_MAKINUM(2));
      std::string chain_id = scm_to_locale_string(chain_id_scm);
      int resno = scm_to_int(resno_scm);
      std::string ins_code  = scm_to_locale_string(ins_code_scm);
      rs = coot::residue_spec_t(chain_id, resno, ins_code);
      good_spec = 1;
   }
   return std::pair<bool, coot::residue_spec_t>(good_spec, rs);
} 

int key_sym_code_scm(SCM s_scm) {

   int r = -1;
   SCM s_test = scm_string_p(s_scm);
   if (scm_is_true(s_test)) { 
      std::string s = scm_to_locale_string(s_scm);
      r = coot::util::decode_keysym(s);
   }
   return r;
}

#endif // USE_GUILE

