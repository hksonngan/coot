/* coot-utils/test-utils.cc
 * 
 * Copyright 2005, 2006 by Paul Emsley, The University of York
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
#include <algorithm>

#include "clipper/core/rotation.h"

#include "utils/coot-utils.hh"
#include "coot-coord-utils.hh"
#include "coot-coord-extras.hh"
#include "lsq-improve.hh"
#include "helix-analysis.hh"


class testing_data {
public:
   static coot::protein_geometry geom;
   testing_data() {
      if (geom.size() == 0)
	 geom.init_standard();
   }
};

coot::protein_geometry testing_data::geom;


namespace coot { 
   class SortableChainsmmdb::Manager : public mmdb::Manager {
   public:
      void SortChains();
   };
}

void
coot::SortableChainsmmdb::Manager::SortChains() {

   for (int imod=1; imod<=GetNumberOfModels(); imod++) { 
      mmdb::Model *model_p = GetModel(imod);
      mmdb::Chain *chain_p;
      // run over chains of the existing mol
      int nchains = model_p->GetNumberOfChains();
      std::vector<std::pair<mmdb::Chain *, std::string> > chain_ids(nchains);
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id = chain_p->GetChainID();
	 chain_ids[ichain] = std::pair<mmdb::Chain *, std::string> (chain_p, chain_id);
      }
      // now chain_ids is full
      std::sort(chain_ids.begin(), chain_ids.end(), sort_chains_util);
      for (int ichain=0; ichain<nchains; ichain++) {
	 // model_p->Chain[ichain] = chain_ids[ichain].first;
      }
   }
   PDBCleanup(mmdb::PDBCLEAN_SERIAL|mmdb::PDBCLEAN_INDEX);
   FinishStructEdit();
}

void test_euler_angles() {

   clipper::Euler_ccp4 e(M_PI/2.0, 0, 0);
   clipper::Rotation r(e);

   std::cout << "Rotation from euler angles: \n"
	     << r.matrix().format() << std::endl;
   
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
   mmdb::Manager *mol = new mmdb::Manager;
   mol->ReadCoorFile(file_name.c_str());
   coot::sort_chains(mol);
   mol->WritePDBASCII("test-sort-chains-sorted.pdb");
}

void test_lsq_improve() {

   std::cout << "========================= lsq-improve ==================" << std::endl;
   mmdb::Manager *mol_1 = new mmdb::Manager;
   mmdb::Manager *mol_2 = new mmdb::Manager;

   std::string ref_pdb = "tutorial-modern.pdb";
   std::string mov_pdb = "1py3-matched-A6-13.pdb";

   mmdb::ERROR_CODE err_1 = mol_1->ReadCoorFile(ref_pdb.c_str());
   mmdb::ERROR_CODE err_2 = mol_2->ReadCoorFile(mov_pdb.c_str());

   if (err_1) {
      std::cout << "There was an error reading " << ref_pdb << ".\n";
   }
   if (err_2) {
      std::cout << "There was an error reading " << mov_pdb << ".\n";
   }

   for (unsigned int i=0; i<1; i++) { 

      try { 
	 coot::lsq_improve lsq_imp(mol_1, "//A/1-50", mol_2, "//A/4-50");
	 lsq_imp.improve();
	 clipper::RTop_orth rtop = lsq_imp.rtop_of_moving();
	 std::cout << "rtop:\n" << rtop.format() << std::endl;
	 coot::util::transform_mol(mol_2, rtop);
	 // mol_2->WritePDBASCII("lsq-improved.pdb");
	 
      }
      catch (std::runtime_error rte) {
	 std::cout << "lsq_improve ERROR::" << rte.what() << std::endl;
      }
   }

}

int test_string_manipulation() {


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

   return 0;
   
}

int test_matrices() {


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

   return 0;
}

int test_glyco_tree() {

   testing_data t;
   int dynamic_add_status_1 = t.geom.try_dynamic_add("NAG", 1);
   int dynamic_add_status_2 = t.geom.try_dynamic_add("MAN", 1);
   int dynamic_add_status_3 = t.geom.try_dynamic_add("BMA", 1);
   int dynamic_add_status_4 = t.geom.try_dynamic_add("GAL", 1);
   // int dynamic_add_status_4 = t.geom.try_dynamic_add("GLB", 1); minimal
   
   mmdb::Manager *mol = new mmdb::Manager;
   // std::string file_name = "3u2s.pdb";
   // coot::residue_spec_t spec("G", 560, "");

   std::string file_name = "sweet2-test-1.pdb";
   coot::residue_spec_t spec("", 1, "");
   
   mol->ReadCoorFile(file_name.c_str());
   mmdb::Residue *r = coot::util::get_residue(spec, mol);
   if (! r) {
      std::cout << "No residue " << spec << std::endl;
   } else {
      coot::glyco_tree_t gt(r, mol, &t.geom);
   } 


   delete mol;
   return 0;
}

int test_helix_analysis() {

   mmdb::Manager *mol = new mmdb::Manager;
   // std::string file_name = "theor-helix-down-z.pdb";
   std::string file_name = "helix-just-off-z.pdb";
   // file_name = "../src/pdb2qc1-sans-sans-ASN.pdb";
   coot::residue_spec_t spec("A", 10, "");
   // coot::residue_spec_t spec("B", 201, "");
   
   mol->ReadCoorFile(file_name.c_str());
   mmdb::Residue *r = coot::util::get_residue(spec, mol);
   if (! r) {
      std::cout << "No residue " << spec << std::endl;
   } else {
      coot::helix_params_container_t h;
      // h.make(mol, "B", 201, 207);
      h.make(mol, "A", 10, 15);
   }
   delete mol;
   return 0;
}

#include <fstream>

int test_qq_plot() {

   int status = 0;

   std::string file_name = "random-300.tab";

   std::ifstream f(file_name.c_str());

   if (f) { 

      std::vector<double> data;
      std::string line;
      while (std::getline(f, line)) {
	 std::vector<std::string> bits = coot::util::split_string_no_blanks(line, " ");
	 for (unsigned int ibit=0; ibit<bits.size(); ibit++) { 
	    try {
	       double v = coot::util::string_to_float(bits[ibit]);
	       data.push_back(v);
	    }
	    catch (const std::runtime_error &rte) {
	       std::cout << "   " << rte.what() << std::endl;
	    } 
	 }
      }

      coot::util::qq_plot_t qq(data);
      std::vector<std::pair<double, double> > qqd = qq.qq_norm();

      for (unsigned int i=0; i<qqd.size(); i++) { 
	 std::cout << "plot " << i << " " << "   " << qqd[i].first << "   "
		   << qqd[i].second << std::endl;
      }
   }
   return status;
}


int main(int argv, char **argc) {


   if (0)
      test_string_manipulation();

   if (0) 
      test_sort_chains();

   if (0) 
      test_euler_angles();

   if (0) 
      test_lsq_improve();

   if (0)
      test_glyco_tree();

   if (0)
      test_helix_analysis();

   if (1)
      test_qq_plot();
   
   return 0;
}

