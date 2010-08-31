/* coot-utils/coot-coord-extras.cc
 * 
 * Copyright 2006, by The University of York
 * Copyright 2009 by The University of Oxford
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

#include "coot-coord-utils.hh"

// LSQing
//
std::pair<short int, clipper::RTop_orth>
coot::util::get_lsq_matrix(CMMDBManager *mol1,
			   CMMDBManager *mol2,
			   const std::vector<coot::lsq_range_match_info_t> &matches,
			   int every_nth) {

   short int istat = 0;
   clipper::RTop_orth rtop(clipper::Mat33<double>(0,0,0,0,0,0,0,0,0),
			   clipper::Coord_orth(0,0,0));
   int SelHnd1 = mol1->NewSelection();
   int SelHnd2 = mol2->NewSelection();
   std::vector<int> v1;
   std::vector<int> v2;
   PPCAtom r_atoms;
   PPCAtom w_atoms;
   int r_selected_atoms;
   int w_selected_atoms;

   mol1->GetSelIndex(SelHnd1, r_atoms, r_selected_atoms);
   mol2->GetSelIndex(SelHnd2, w_atoms, w_selected_atoms);

   mol1->SelectAtoms(SelHnd1, 0, "*", ANY_RES, "*", ANY_RES, "*", "*", "*", "*", "*");
   mol2->SelectAtoms(SelHnd2, 0, "*", ANY_RES, "*", ANY_RES, "*", "*", "*", "*", "*");

   std::vector<clipper::Coord_orth> co1v;
   std::vector<clipper::Coord_orth> co2v;
   for (unsigned int i=0; i<matches.size(); i++) {
      std::pair<std::vector<clipper::Coord_orth>, std::vector<clipper::Coord_orth> > p =
	get_matching_indices(mol1, mol2, SelHnd1, SelHnd2, matches[i], every_nth);
      if ((p.first.size() > 0) && (p.first.size() == p.second.size())) {
	 for (unsigned int j=0; j<p.first.size(); j++) {
	    co1v.push_back(p.first[j]);
	    co2v.push_back(p.second[j]);
	 }
      }
   }

   // Now convert v1 and v2 to Coord_orth vectors and call Clipper's
   // LSQ function.
   //
   if (co1v.size() > 0) {
      if (co1v.size() > 2) {
	 if (co2v.size() > 2) {
	    std::cout << "INFO:: LSQ matched " << co1v.size() << " atoms" << std::endl;
	    rtop = clipper::RTop_orth(co2v, co1v);
	    double sum_dist = 0.0;
	    double sum_dist2 = 0.0;
	    double mind =  999999999.9;
	    double maxd = -999999999.9;
	    double d;
	    for (unsigned int i=0; i<co2v.size(); i++) {
	       d = clipper::Coord_orth::length(co1v[i],
					       clipper::Coord_orth(co2v[i].transform(rtop)));
	       sum_dist  += d;
	       sum_dist2 += d*d;
	       if (d>maxd)
		  maxd = d;
	       if (d<mind)
		  mind = d;
	    }
	    double mean = sum_dist/double(co2v.size());
	    // not variance about mean, variance from 0.
	    //double var  = sum_dist2/double(co2v.size()) - mean*mean;
	    double v    = sum_dist2/double(co2v.size());
	    std::cout << "INFO:: " << co1v.size() << " matched atoms had: \n"
		      << "   mean devi: " << mean << "\n"
		      << "    rms devi: " << sqrt(v) << "\n"
		      << "    max devi: " << maxd << "\n"
		      << "    min devi: " << mind << std::endl;
	    istat = 1;
	 } else {
	    std::cout << "WARNING:: not enough points to do matching (matching)"
		      << std::endl;
	 }
      } else { 
	 std::cout << "WARNING:: not enough points to do matching (reference)"
		   << std::endl;
      }
   } else {
      std::cout << "WARNING:: no points to do matching" << std::endl;
   }
   mol1->DeleteSelection(SelHnd1);
   mol2->DeleteSelection(SelHnd2);
   return std::pair<short int, clipper::RTop_orth> (istat, rtop);
}

// On useful return, first.size() == second.size() and first.size() > 0.
// 
std::pair<std::vector<clipper::Coord_orth>, std::vector<clipper::Coord_orth> > 
coot::util::get_matching_indices(CMMDBManager *mol1,
				 CMMDBManager *mol2,
				 int SelHnd1,
				 int SelHnd2,
				 const coot::lsq_range_match_info_t &match,
				 int every_nth) {

   std::vector<clipper::Coord_orth> v1;
   std::vector<clipper::Coord_orth> v2;

   PPCAtom r_atoms;
   PPCAtom w_atoms;
   int r_selected_atoms;
   int w_selected_atoms;

   mol1->GetSelIndex(SelHnd1, r_atoms, r_selected_atoms);
   mol2->GetSelIndex(SelHnd2, w_atoms, w_selected_atoms);

   // general main chain atom names
   std::vector<std::string> mc_at_names;
   // Set up the amino acid main chain atom names
   std::vector<std::string> amc_at_names;
   amc_at_names.push_back(" CA ");
   amc_at_names.push_back(" N  ");
   amc_at_names.push_back(" O  ");
   amc_at_names.push_back(" C  ");
   
   // Set up the nucleotide main chain atom names
   std::vector<std::string> nmc_at_names;
   nmc_at_names.push_back(" P  ");
   nmc_at_names.push_back(" O5*");
   nmc_at_names.push_back(" O5'");
   nmc_at_names.push_back(" C5*");
   nmc_at_names.push_back(" C5'");
   nmc_at_names.push_back(" C4*");
   nmc_at_names.push_back(" C4'");
   nmc_at_names.push_back(" O4*");
   nmc_at_names.push_back(" O4'");
   nmc_at_names.push_back(" C1*");
   nmc_at_names.push_back(" C1'");
   nmc_at_names.push_back(" C2*");
   nmc_at_names.push_back(" C2'");
   nmc_at_names.push_back(" O2*");
   nmc_at_names.push_back(" O2'");
   nmc_at_names.push_back(" C3*");
   nmc_at_names.push_back(" C3'");
   nmc_at_names.push_back(" O3*");
   nmc_at_names.push_back(" O3'");
   nmc_at_names.push_back(" O3T");


//   for (int ires=match.to_reference_start_resno; ires<=match.to_reference_end_resno; ires++) {
   if (every_nth < 1 || every_nth > 10) 
     every_nth = 1;   // reset for nonsense values
   for (int ires=match.to_reference_start_resno; ires<=match.to_reference_end_resno; ires+=every_nth) {
      int ires_matcher = ires - match.to_reference_start_resno + match.from_matcher_start_resno;
      int SelHnd_res1 = mol1->NewSelection();
      int SelHnd_res2 = mol2->NewSelection();
      PCResidue *SelResidue_1 = NULL;
      PCResidue *SelResidue_2 = NULL;
      int nSelResidues_1, nSelResidues_2;

//      std::cout << "Searching for residue number " << ires << " "
//		<< match.reference_chain_id << " in reference molecule" << std::endl;
//      std::cout << "Searching for residue number " << ires_matcher << " "
//		<< match.matcher_chain_id << " in matcher molecule" << std::endl;
      
      mol1->Select (SelHnd_res1, STYPE_RESIDUE, 0, // .. TYPE, iModel
		    match.reference_chain_id.c_str(), // Chain(s)
		    ires, "*",  // starting res
		    ires, "*",  // ending res
		    "*",  // residue name
		    "*",  // Residue must contain this atom name?
		    "*",  // Residue must contain this Element?
		    "*",  // altLocs
		    SKEY_NEW // selection key
		    );
      mol2->Select (SelHnd_res2, STYPE_RESIDUE, 0, // .. TYPE, iModel
		    match.matcher_chain_id.c_str(), // Chain(s)
		    ires_matcher, "*",  // starting res
		    ires_matcher, "*",  // ending res
		    "*",  // residue name
		    "*",  // Residue must contain this atom name?
		    "*",  // Residue must contain this Element?
		    "*",  // altLocs
		    SKEY_NEW // selection key
		    );

      mol1->GetSelIndex(SelHnd_res1, SelResidue_1, nSelResidues_1);
      mol2->GetSelIndex(SelHnd_res2, SelResidue_2, nSelResidues_2);

      if (nSelResidues_1 == 0 || nSelResidues_2 == 0) {
	 
	 if (nSelResidues_1 == 0) { 
	    std::cout << "WARNING - no residue for reference residue number "
		      << ires << " " << match.reference_chain_id << std::endl;
	 }
	 if (nSelResidues_2 == 0) { 
	    std::cout << "WARNING - no residue for moving molecule residue number "
		      << ires_matcher << " " << match.matcher_chain_id << std::endl;
	 }
      } else {

	 // So we have 2 good residues
	 
	 // Let's get their residue type:
	 std::string res_type_1(SelResidue_1[0]->GetResName());
	 std::string res_type_2(SelResidue_2[0]->GetResName());

	 // ---------------- CA ----------------------------

	 if (match.match_type_flag == COOT_LSQ_CA) {

	    // CA/P names
	    std::string ca_name;

	    CAtom *at1 = NULL;
	    CAtom *at2 = NULL;
	    if (SelResidue_1[0]->isAminoacid()) {
	      ca_name = " CA ";
	    } else {
	      if (SelResidue_1[0]->isNucleotide()) {
		ca_name = " P  ";
	      } else {
		ca_name = " P  ";
		std::cout << "WARNING:: residue is not amino acid or nucleotide! "
			  << "Assuming non-standard nucleotide." << std::endl;
	      }
	    }
	    at1 = SelResidue_1[0]->GetAtom(ca_name.c_str());
	    at2 = SelResidue_2[0]->GetAtom(ca_name.c_str());

	    if (at1 == NULL) {
	      std::cout << "WARNING:: no " << ca_name << " in this reference residue " << ires
			<< std::endl;
	    }
	    if (at2 == NULL) {
	      std::cout << "WARNING:: no " << ca_name << " in this reference residue " << ires
			<< std::endl;
	    }
	    if (at1 && at2) {
	       v1.push_back(clipper::Coord_orth(at1->x, at1->y, at1->z));
	       v2.push_back(clipper::Coord_orth(at2->x, at2->y, at2->z));
	    }
	 }


	 // ----------------- Mainchain ---------------------
	 //
	 if ((match.match_type_flag == COOT_LSQ_MAIN) ||
	     (match.match_type_flag == COOT_LSQ_ALL && res_type_1 != res_type_2)) {

            if (SelResidue_1[0]->isNucleotide()) {
               mc_at_names = nmc_at_names;
            } else {
               mc_at_names = amc_at_names;
            }
	    for (unsigned int iat=0; iat<mc_at_names.size(); iat++) {
	       CAtom *at1 = SelResidue_1[0]->GetAtom(mc_at_names[iat].c_str());
	       CAtom *at2 = SelResidue_2[0]->GetAtom(mc_at_names[iat].c_str());
	       if (at1) {
		  if (!at2) {
		     std::cout << "WARNING:: no " << mc_at_names[iat]
			       << " in this moving residue " << ires_matcher
			       << std::endl;
		  } else {
		     v1.push_back(clipper::Coord_orth(at1->x, at1->y, at1->z));
		     v2.push_back(clipper::Coord_orth(at2->x, at2->y, at2->z));
		  }
	       }
	    }
	 }


	 // ----------------- All Atom ---------------------
	 //
	 if (match.match_type_flag == COOT_LSQ_ALL) {

	    if (! match.is_single_atom_match) {
	       PCAtom *residue_atoms1 = NULL;
	       PCAtom *residue_atoms2 = NULL;
	       int n_residue_atoms1;
	       int n_residue_atoms2;
	       SelResidue_1[0]->GetAtomTable(residue_atoms1, n_residue_atoms1);
	       SelResidue_2[0]->GetAtomTable(residue_atoms2, n_residue_atoms2);
	       for (int iat=0; iat<n_residue_atoms1; iat++) {
		  CAtom *at1 = residue_atoms1[iat];
		  std::string at1_name(at1->name);
		  std::string at1_altconf(at1->altLoc);
		  short int found_atom2 = 0;
		  for (int jat=0; jat<n_residue_atoms2; jat++) {
		     CAtom *at2 = residue_atoms2[jat];
		     std::string at2_name(at2->name);
		     std::string at2_altconf(at2->altLoc);

		     if (at1_name == at2_name) {
			if (at1_altconf == at2_altconf) {
			   v1.push_back(clipper::Coord_orth(at1->x, at1->y, at1->z));
			   v2.push_back(clipper::Coord_orth(at2->x, at2->y, at2->z));
			   found_atom2 = 1;
			   break;
			}
		     }
		  }
	       }
	    } else {
	       // is single atom match
	       PCAtom *residue_atoms1 = NULL;
	       PCAtom *residue_atoms2 = NULL;
	       int n_residue_atoms1;
	       int n_residue_atoms2;
	       SelResidue_1[0]->GetAtomTable(residue_atoms1, n_residue_atoms1);
	       SelResidue_2[0]->GetAtomTable(residue_atoms2, n_residue_atoms2);
// 	       std::cout << "single atom match: residue: ref: "
// 			 << coot::residue_spec_t(SelResidue_1[0])
// 			 << " to " 
// 			 << coot::residue_spec_t(SelResidue_2[0])
// 			 << std::endl;
	       for (int iat=0; iat<n_residue_atoms1; iat++) {
		  CAtom *at1 = residue_atoms1[iat];
		  std::string at1_name(at1->name);
		  std::string at1_altconf(at1->altLoc);
		  short int found_atom2 = 0;
		  if (at1_name == match.reference_atom_name) { 
		     if (at1_altconf == match.reference_alt_conf) { 
			for (int jat=0; jat<n_residue_atoms2; jat++) {
			   CAtom *at2 = residue_atoms2[jat];
			   std::string at2_name(at2->name);
			   std::string at2_altconf(at2->altLoc);
			   if (at2_name == match.matcher_atom_name) { 
			      if (at2_altconf == match.matcher_alt_conf) {
				 v1.push_back(clipper::Coord_orth(at1->x, at1->y, at1->z));
				 v2.push_back(clipper::Coord_orth(at2->x, at2->y, at2->z));
			      }
			   }
			}
		     }
		  }
	       }
	    } 
	 }
      }

      // tidy up
      mol1->DeleteSelection(SelHnd_res1);
      mol2->DeleteSelection(SelHnd_res2);
   }
      
   return std::pair<std::vector<clipper::Coord_orth>, std::vector<clipper::Coord_orth> > (v1, v2);
}
