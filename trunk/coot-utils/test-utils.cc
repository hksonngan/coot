/* coot-utils/test-utils.cc
 * 
 * Copyright 2005, 2006 by Paul Emsley, The University of York
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
 * 02110-1301, USA
 */

#include <iostream>
#include <algorithm>
#include "coot-utils.hh"
#include "coot-coord-utils.hh"

namespace coot { 
   class SortableChainsCMMDBManager : public CMMDBManager {
   public:
      void SortChains();
   };
}

void
coot::SortableChainsCMMDBManager::SortChains() {

   for (int imod=1; imod<=GetNumberOfModels(); imod++) { 
      CModel *model_p = GetModel(imod);
      CChain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      std::vector<std::pair<CChain *, std::string> > chain_ids(nchains);
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id = chain_p->GetChainID();
	 chain_ids[ichain] = std::pair<CChain *, std::string> (chain_p, chain_id);
      }
      // now chain_ids is full
      std::sort(chain_ids.begin(), chain_ids.end(), sort_chains_util);
      for (int ichain=0; ichain<nchains; ichain++) {
	 // model_p->Chain[ichain] = chain_ids[ichain].first;
      }
   }
   PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
   FinishStructEdit();
}


void split_test(const std::string &r) {

   std::vector<std::string> v = coot::util::split_string(r, " ");
   std::cout << ":" << r << ": -> ";
   for (unsigned int i=0; i<v.size(); i++) {
      std::cout << ":" << v[i] << ": ";
   }
   std::cout << "\n";
}

bool
test_quaternion_matrix(clipper::Mat33<double> m){

   coot::util::quaternion q1(m);
   clipper::Mat33<double> m2 = q1.matrix();
   coot::util::quaternion q2(m2);
   bool match = q1.is_similar_p(q2);

   std::cout << "   " << q1 << "\n" << "   " << q2 << "   ";
   std::cout << "match: " << match << std::endl;
   return match;
}

bool test_quaternion_quaternion(coot::util::quaternion q) {

   clipper::Mat33<double> m2 = q.matrix();
   coot::util::quaternion q2(m2);
   bool match = q.is_similar_p(q2);


   std::cout << std::endl;
   std::cout << m2.format() << std::endl;
   std::cout << "   " << q << "\n" << "   " << q2 << "   ";
   std::cout << "match: " << match << std::endl;
   return match;

}

void test_sort_chains() {

   std::string file_name = "test-sort-chains.pdb";
   CMMDBManager *mol = new CMMDBManager;
   mol->ReadCoorFile((const pstr) file_name.c_str());
   coot::sort_chains(mol);
   mol->WritePDBASCII("test-sort-chains-sorted.pdb");
} 


int main(int argv, char **argc) {


   /*
   std::string s;
   s = "AVasdfasdfC";
   std::cout << s << " cuts to :" << coot::util::remove_leading_spaces(s) << ":"
	     << std::endl;
   s = "   AVC";
   std::cout << s << " cuts to :" << coot::util::remove_leading_spaces(s) << ":"
	     << std::endl;
   s = " AVC ";
   std::cout << s << " cuts to :" << coot::util::remove_leading_spaces(s) << ":"
	     << std::endl;
   s = "C";
   std::cout << s << " cuts to :" << coot::util::remove_leading_spaces(s) << ":"
	     << std::endl;
   s = "";
   std::cout << s << " cuts to :" << coot::util::remove_leading_spaces(s) << ":"
	     << std::endl;
   */

   std::string a("ABCDefgh");
   std::cout << a << " downcased: " << coot::util::downcase(a) << std::endl;
   std::cout << a << "   upcased: " << coot::util::upcase(a) << std::endl;

   std::string s("Cottage");
   std::string r("tag"); 
   std::cout  << "removing :" << r << ": from :" << s << ": gives :"
	      << coot::util::remove_string(s, r) <<  ":" << std::endl;
   r = "tage";
   std::cout << "removing :" << r << ": from :" << s << ": gives :"
	     << coot::util::remove_string(s, r) <<  ":" << std::endl;
   r = "e";
   std::cout << "removing :" << r << ": from :" << s << ": gives :"
	     << coot::util::remove_string(s, r) <<  ":" << std::endl;
   r = "";
   std::cout << "removing :" << r << ": from :" << s << ": gives :"
	     << coot::util::remove_string(s, r) <<  ":" << std::endl;
   r = "ball";
   std::cout << "removing :" << r << ": from :" << s << ": gives :"
	     << coot::util::remove_string(s, r) <<  ":" << std::endl;
   r = "Cottage";
   std::cout << "removing :" << r << ": from :" << s << ": gives :"
	     << coot::util::remove_string(s, r) <<  ":" << std::endl;

   r = "Columns";
   split_test(r);
   r = "Columns ";
   split_test(r);
   r = " Columns ";
   split_test(r);
   r = "Columns     of   letters  ";
   split_test(r);
   r = " Columns     of   letters  ";
   split_test(r);

   clipper::Mat33<double> m1 (1,0,0, 0,1,0, 0,0,1);
   test_quaternion_matrix(m1);
   clipper::Mat33<double> m2 (0,1,0, 1,0,0, 0,0,-1);
   test_quaternion_matrix(m2);
   // this one from quat-convert.scm:
   clipper::Mat33<double> m3( 0.0347695872187614, 0.773433089256287,   0.632923781871796,
			      0.774806916713715,  0.379149734973907,  -0.505885183811188,
			     -0.631241261959076,  0.507983148097992,  -0.586078405380249);
        // -> (-0.557 -0.694704 -0.0007537 0.454928)
   test_quaternion_matrix(m3);

   coot::util::quaternion q1(1,0,0,0);
   coot::util::quaternion q2(0,1,0,0);
   coot::util::quaternion q3(0,0,1,0);
   coot::util::quaternion q4(0,0,0,1);
   coot::util::quaternion q5(-0.557, -0.694704, -0.0007537, 0.454928);
   test_quaternion_quaternion(q1);
   test_quaternion_quaternion(q2);
   test_quaternion_quaternion(q3);
   test_quaternion_quaternion(q4);
   test_quaternion_quaternion(q5);


   test_sort_chains();
   
   return 0;
}

