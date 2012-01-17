/* src/rotamer.hh
 * 
 * Copyright 2001, 2002, 2003, 2004, 2006 The University of York
 * Author: Paul Emsley
 * Copyright 2007 The University of Oxford
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

#ifndef ROTAMER_HH
#define ROTAMER_HH

#include <math.h>
#include "chi-angles.hh"

namespace coot {

   class rotamer_probability_info_t {
   public:
      enum {OK=1, // assigned
	    MISSING_ATOMS = 0,
	    ROTAMER_NOT_FOUND = -1,
	    RESIDUE_IS_GLY_OR_ALA = -2 };
      short int state;
      float probability;
      std::string alt_conf;

      std::string rotamer_name;
      rotamer_probability_info_t(short int state_in, float prob_in, const std::string &name) {
	 state = state_in;
	 probability = prob_in;
	 rotamer_name = name;
	 alt_conf = "";
      }
      rotamer_probability_info_t(short int state_in, float prob_in, const std::string &name,
				 const std::string alt_conf_in) {
	 state = state_in;
	 probability = prob_in;
	 rotamer_name = name;
	 alt_conf = alt_conf_in;
      }
   }; 

   class a_rotamer_table {
      void fill_chi_1(const std::string &file_name);
      void fill_chi_1_2(const std::string &file_name);
      void fill_chi_1_2_3(const std::string &file_name);
      void fill_chi_1_2_3_4(const std::string &file_name);
      int chi_angle_to_bin(const float &chi, const int &n_samples_per_360) const {
	 int t = lrint(n_samples_per_360 * (chi-0.4999)/360.0);
// 	 std::cout << "::: " << n_samples_per_360 << "*(" << chi << "-0.5)/360 = "
// 		   << t << std::endl;
	 return t;
      } 
   public:
      std::string residue_name;
      int n_chis;
      int n_chi1_samples_per_360;
      int n_chi2_samples_per_360;
      int n_chi3_samples_per_360;
      int n_chi4_samples_per_360;
      std::vector<float> pr_chi_1;
      std::vector<std::vector<float> > pr_chi_1_2;
      std::vector<std::vector<std::vector<float> > > pr_chi_1_2_3;
      std::vector<std::vector<std::vector<std::vector<float> > > > pr_chi_1_2_3_4;
      a_rotamer_table(const std::string &residue_name_in,
		      const std::string &file_name);
   };
   
   // filled with molprobity table data
   //
   // We want this to be a static, and we ask for the probability of a
   // rotamer of given a set of chi angles.
   //
   // We want to instantiate this class, fully, when it is needed, not
   // all the time.  At startup, we have a placeholder.  When rotamer
   // analysis is required we need to fill the class.  However, if
   // is_well_formatted_ is 0, we also need to know that we've tried
   // to fill the class before and somehow failed, and not to try to
   // fill the class again.
   // 
   class rotamer_probability_tables {

      std::vector<a_rotamer_table> tables;
      bool is_well_formatted_;
      bool tried_and_failed_;
      std::string tables_dir;
      // throw an exeption on failure
      // Note, not a const vector because we can locally manipulate chi_angles if an ASP, GLU, TYR/PHE.
      std::vector<int> chi_angles_to_bins(unsigned int table_index,
					  std::vector<std::pair<int,float> > chi_angles) const;

      rotamer_probability_info_t probability_this_rotamer(unsigned int i_table,
							  const std::vector<std::pair<int,float> > &chi_angles) const;
      bool test_yourself();
   public:
      rotamer_probability_tables() { is_well_formatted_ = 0; tried_and_failed_ = 0; }
      //
      void set_tables_dir(const std::string &tables_dir_in) { tables_dir = tables_dir_in; }
      void fill_tables() { fill_tables(tables_dir); } // for on the fly construction
      void fill_tables(const std::string &tables_dir); // set is_well_formatted.

      // the chi angles here (first) numbered 1, 2, 3...
      // throws an exception
      std::vector<rotamer_probability_info_t> probability_this_rotamer(CResidue *res) const;
      
      bool is_well_formatted() const { return is_well_formatted_; }
      bool tried_and_failed() const { return tried_and_failed_; }
   }; 


   class rotamer : public chi_angles {

      float probability_limit;
      rotamer_probability_info_t
      probability_of_this_rotamer(const std::vector<double> &chi_angles,
				  const std::vector<coot::simple_rotamer> &rots) const;
      
      std::vector<std::vector<std::string> >
      rotamer_atoms(const std::string &residue_name) const;
      std::vector<std::vector<int> > rotamer_atom_names_to_indices(const std::vector<std::vector<std::string> > &residue_rotamer_atoms, PCAtom *residue_atoms, int n_residue_atoms) const;
      double chi_torsion(const std::vector<int> &chi_angle_atom_indices,
			 PCAtom *residue_atoms);
      std::vector<coot::simple_rotamer>
      get_all_rotamers(const std::string &res_type) const;
      std::vector<CAtom *> ordered_residue_atoms(CResidue *residue_p) const;

      short int similar_rotamer_chi(const double &target, const double &model) const {
	 short int is = 0;
	 double diff = target - model;
	 while (diff > 180.0)
	    diff -= 360.0;
	 while (diff < -180.0)
	    diff += 360.0;

	 if (fabs(diff) < 40.0)
	    is = 1;

	 return is;
      }

   protected:
      CMMDBManager *stored_mol;
      float Probability_limit() const { return probability_limit; }
      void set_probability_limit(float lim) { probability_limit = lim;}
      std::string alt_conf;
      
   public:

      rotamer(CResidue *res,
	      const std::string &alt_conf_in,
	      short int add_extra_PHE_and_TYR_rotamers_flag ) :
	 chi_angles(res, add_extra_PHE_and_TYR_rotamers_flag) {
	 alt_conf = alt_conf_in;
      }

      // For use with Z-score (which is analysis only: we don't move anything)
      // 
      rotamer(CResidue *residue) : chi_angles(residue, 0) {}

      // LEU, VAL, THR have "nomenclature" (or real) chiral centres -
      // they are not dealt with here.
      // 
      // We deal with bifurcated symmetric non-chiral side chains (PHE, ASP,
      // GLU, THR)
      // 
      int optimize_rotamer_by_atom_names(bool apply_swap_if_found);

      rotamer_probability_info_t probability_of_this_rotamer(); // can't const - mmdb
                                                                // CResidue issues...

      // return a manipulated deep copy of input residue.
      CResidue *GetResidue(const dictionary_residue_restraints_t &rest,
			   int i_rot) const; // rotamer/button number
      CResidue *GetResidue_old(int i_rot) const; // for transitioning.  Delete later
      std::vector<coot::simple_rotamer> rotamers(const std::string &res_type, float prob_cut) const; 
      float Chi1(int i) const; // chi1 for the ith rotamer
      std::string rotamer_name(int irot);
   };
}

#endif // ROTAMER_HH

