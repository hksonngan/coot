/* src/testing.cc
 * 
 * Copyright 2008, 2009 by the University of Oxford
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

#include "testing.hh"

#ifdef BUILT_IN_TESTING

#include <iostream>
#include <sstream>

#include <GL/glut.h> // needed for GLUT_ELAPSED_TIME

#include "coot-sysdep.h"

#include "clipper/core/ramachandran.h"

#include "coot-coord-utils.hh"
#include "coot-rama.hh"
#include "primitive-chi-angles.hh"

#include "wligand.hh"
#include "simple-restraint.hh"
#include "ligand.hh"
#include "chi-angles.hh"

#ifdef HAVE_GSL
#else
#include "coot-utils.hh" // usually include from simple-restraint.hh
#endif

#include "coot-shelx.hh"
#include "coot-coord-utils.hh"

#include "coot-map-utils.hh"
#include "dipole.hh"

bool close_float_p(float f1, float f2) {

   return (fabs(f1-f2) < 0.0001);

} 

class testing_data {
public:
   static coot::protein_geometry geom;
   testing_data() {
      if (geom.size() == 0)
	 geom.init_standard();
   }
};

coot::protein_geometry testing_data::geom;


std::string greg_test(const std::string &file_name) {

   std::string d = getenv("HOME");
   d += "/data/greg-data/";
   d += file_name;
   return d;
} 

std::string stringify(double x) {
   std::ostringstream o;
   if (!(o << x))
      throw std::runtime_error("stringify(double)");
   return o.str();
}


#ifdef STRINGIFY_SIZE_T
std::string stringify(size_t x) {
   std::ostringstream o;
   if (!(o << x))
      throw std::runtime_error("stringify(size_t)");
   return o.str();
}
#endif

std::string stringify(int i) {
   std::ostringstream o;
   if (!(o << i))
      throw std::runtime_error("stringify(int)");
   return o.str();
}

std::string stringify(unsigned int i) {
   std::ostringstream o;
   if (!(o << i))
      throw std::runtime_error("stringify(unsigned int)");
   return o.str();
}

bool close_double(double a, double b) {
   double small_diff = 0.001;
   return (fabs(a-b) < small_diff);
}

bool close_pair_test(std::pair<double, double> a, std::pair<double, double> b) {
   return (close_double(a.first, b.first) && close_double(a.second, b.second));
}

std::ostream& operator<< (std::ostream &s, std::pair<double, double> b) {
   s << "[" << b.first << "," << b.second << "]";
   return s;
}

void add_test(int (*)(), const std::string &test_name, std::vector<named_func> *functions) {
   //    named_func p(t, test_name);
   //    functions->push_back(named_func(p));
} 


int test_internal() {

   int status = 1;
   std::vector<named_func> functions;
   functions.push_back(named_func(kdc_torsion_test, "kevin's torsion test"));
   functions.push_back(named_func(test_alt_conf_rotamers, "test_alt_conf_rotamers"));
   functions.push_back(named_func(test_wiggly_ligands, "test_wiggly_ligands"));
   // file not found.
   // functions.push_back(named_func(test_ramachandran_probabilities, "test_ramachandran_probabilities"));
   functions.push_back(named_func(test_fragmemt_atom_selection, "test_fragmemt_atom_selection"));
   functions.push_back(named_func(test_add_atom, "test_add_atom"));

   // restore me at some stage
   // functions.push_back(named_func(restr_res_vector, "restraints_for_residue_vec"));

   // restore me at some stage.
   // functions.push_back(named_func(test_peptide_link, "test_peptide_link"));
   functions.push_back(named_func(test_dictionary_partial_charges,
				  "test dictionary partial charges"));
   // restore me at some stage
   // functions.push_back(named_func(test_dipole, "test_dipole"));

   functions.push_back(named_func(test_segid_exchange, "test segid exchange"));

   functions.push_back(named_func(test_ligand_fit_from_given_point, "test ligand from point"));

   functions.push_back(named_func(test_peaksearch_non_close_peaks,
				  "test peak search non-close"));

   functions.push_back(named_func(test_symop_card, "test symop card"));
   functions.push_back(named_func(test_rotate_round_vector, "test rotate round vector"));

   for (unsigned int i_func=0; i_func<functions.size(); i_func++) {
      std::cout << "Entering test: " << functions[i_func].second << std::endl;
      try { 
	 status = functions[i_func].first();
	 if (status) {
	    std::cout << "PASS: " << functions[i_func].second << std::endl;
	 } else { 
	    std::cout << "FAIL: " << functions[i_func].second << std::endl;
	    break;
	 }
      }
      catch (std::runtime_error mess) {
	 std::cout << "FAIL: " << functions[i_func].second << " " << mess.what() << std::endl;
	 status = 0;
	 break;
      }
   } 
   return status; 
}

int test_internal_single() {
   int status = 0;
   try { 
      // status = test_symop_card();
      // status = test_rotate_round_vector();
      // status = test_coot_atom_tree();
      // status = test_coot_atom_tree_2();
      status = test_coot_atom_tree_proline();
   }
   catch (std::runtime_error mess) {
      std::cout << "FAIL: " << " " << mess.what() << std::endl;
   }
   return status;
}
      

      
int test_alt_conf_rotamers() {

   int status = 1;

   std::string filename = greg_test("tutorial-modern.pdb");
   atom_selection_container_t atom_sel = get_atom_selection(filename);
   bool ifound = 0;

   int imod = 1;
   if (atom_sel.read_success > 0) { 
      CModel *model_p = atom_sel.mol->GetModel(imod);
      CChain *chain_p;
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id = chain_p->GetChainID();
	 if (chain_id == "B") {
	    int nres = chain_p->GetNumberOfResidues();
	    PCResidue residue_p;
	    for (int ires=0; ires<nres; ires++) { 
	       residue_p = chain_p->GetResidue(ires);
	       int resno = residue_p->GetSeqNum();
	       if (resno == 72) {

		  ifound = 1;
		  coot::primitive_chi_angles prim_chis(residue_p);
		  std::vector<coot::alt_confed_chi_angles> chis = prim_chis.get_chi_angles();
		  if (chis.size() != 2) {
		     std::string mess = "chis.size() is ";
		     mess += stringify(int(chis.size()));
		     throw std::runtime_error(mess);
		  }

		  int n_rots_found = 0;
		  for (unsigned int i_rot=0; i_rot<2; i_rot++) {
// 		     std::cout << "DEBUG:: chi: " << chis[i_rot].alt_conf << " " 
// 			       << chis[i_rot].chi_angles[0].first  << " "
// 			       << chis[i_rot].chi_angles[0].second << std::endl;
		     
		     if (chis[i_rot].alt_conf == "A") {
			float chi = chis[i_rot].chi_angles[0].second;
			if (chi > 60.0)
			   if (chi < 61.0)
			      n_rots_found++;
		     }
		     if (chis[i_rot].alt_conf == "B") {
			float chi = chis[i_rot].chi_angles[0].second;
			if (chi > -75.0)
			   if (chi < -74.0)
			      n_rots_found++;
		     }
		  }
		  if (n_rots_found != 2) {
		     std::string mess = " found only ";
		     mess += stringify(n_rots_found);
		     mess += " rotamers ";
		     throw std::runtime_error(mess);
		  }
	       }

	       if (resno == 80) {
		  coot::primitive_chi_angles prim_chis(residue_p);
		  std::vector<coot::alt_confed_chi_angles> chis = prim_chis.get_chi_angles();
		  if (chis.size() != 1) {
		     std::string mess = "chis.size() is ";
		     mess += stringify(int(chis.size()));
		     mess += " for ";
		     mess += stringify(79);
		     throw std::runtime_error(mess);
		  }
		  int n_rots_found = 0;
		  if (chis[0].alt_conf == "") {
		     float chi_1 = chis[0].chi_angles[0].second;
		     float chi_2 = chis[0].chi_angles[1].second;
		     if (chi_1 > -58.0)
			if (chi_1 < -57.0)
			   if (chi_2 > 95.0)
			      if (chi_2 < 96.0)
			   n_rots_found++;
		  }
		  if (n_rots_found != 1) {
		     std::string mess = " Oops found ";
		     mess += stringify(n_rots_found);
		     mess += " rotamers ";
		     mess += " for ";
		     mess += stringify(79);
		     throw std::runtime_error(mess);
		  }
	       } 
	    }
	 }
      }
   }

   if (! ifound) {
      std::string mess = "file not found ";
      mess += filename;
      throw std::runtime_error(mess);
   }
   atom_sel.clear_up();

   return status; 
}

int test_wiggly_ligands () {

   // This is not a proper test.  It just exercises some functions.  I
   // had set wiggly_ligand_n_samples to 100 and checked that the
   // distribution of chi angles looked reasonable (it did).
   // Analysing the chi angles can be done another day.
   //
   // BUA looks great.  So why does 3GP's wiggly ligand look terrible?
   // Not sure, but I think it is because there are torsions going on
   // in the ribose (torsionable bonds (not const - they are filtered
   // out) that shouldn't be - and the ring is breaking).
   
   int r = 1;
   std::string cif_file_name = greg_test("libcheck_BUA.cif");
   coot::protein_geometry geom;
   int geom_stat = geom.init_refmac_mon_lib(cif_file_name, 0);
   if (geom_stat == 0) {
      std::string m = "Critical cif dictionary reading failure.";
      std::cout << m << std::endl;
      throw std::runtime_error(m);
   }
   coot::wligand wlig;
   coot::minimol::molecule mmol;
   mmol.read_file(greg_test("monomer-BUA.pdb"));
   int wiggly_ligand_n_samples = 10;
   try { 
      bool optim_geom = 0;
      bool fill_vec = 1;
      std::vector<coot::installed_wiggly_ligand_info_t> ms = 
	 wlig.install_simple_wiggly_ligands(&geom, mmol, wiggly_ligand_n_samples,
					    optim_geom, fill_vec);

      // jump out if no returned molecules
      if (ms.size() != wiggly_ligand_n_samples) {
	 std::cout << "FAIL: ms.size() != wiggly_ligand_n_samples " << ms.size() << " "
		   << wiggly_ligand_n_samples << std::endl;
	 return 0;
      }

      //
      for (unsigned int imol=0; imol<ms.size(); imol++) {
	 std::string file_name = "test-wiggly-ligand-";
	 file_name += stringify(imol);
	 file_name += ".pdb";
	 ms[imol].mol.write_file(file_name, 10.0);
      }
   }
   catch (std::runtime_error mess) {
      std::cout << mess.what() << std::endl;
   } 
   return r;
}

// Return a new mol, and a residue selection.  Delete the residue
// selection and mol when you are done with it.
// 
residue_selection_t
testing_func_probabilities_refine_fragment(atom_selection_container_t atom_sel,
					   PCResidue *SelResidues,
					   int nSelResidues,
					   const std::string &chain_id,
					   int resno_mid,
					   coot::protein_geometry geom,
					   bool enable_rama_refinement,
					   int side_step,
					   bool use_flanking_residues,
					   bool output_numerical_gradients) {

#ifdef HAVE_GSL
   long t0 = glutGet(GLUT_ELAPSED_TIME);
   
   // now refine a bit of structure:
   std::vector<coot::atom_spec_t> fixed_atom_specs;
   short int have_flanking_residue_at_start = 0;
   short int have_flanking_residue_at_end = 0;
   if (use_flanking_residues) {
       have_flanking_residue_at_start = 1;
       have_flanking_residue_at_end = 1;
   }
   short int have_disulfide_residues = 0;  // other residues are included in the
   std::string altconf = "";
   short int in_alt_conf_split_flag = 0;
   char *chn = (char *) chain_id.c_str(); // mmdb thing.  Needs updating on new mmdb?
	       
   std::pair<CMMDBManager *, int> residues_mol_pair = 
      coot::util::create_mmdbmanager_from_res_selection(atom_sel.mol,
							SelResidues, nSelResidues, 
							have_flanking_residue_at_start,
							have_flanking_residue_at_end,
							altconf,
							chain_id,
							in_alt_conf_split_flag);
   
   coot::restraints_container_t restraints(resno_mid-side_step,
					   resno_mid+side_step,
					   have_flanking_residue_at_start,
					   have_flanking_residue_at_end,
					   have_disulfide_residues,
					   altconf,
					   chn,
					   residues_mol_pair.first,
					   fixed_atom_specs);

   short int do_rama_restraints = 0;
   short int do_residue_internal_torsions = 1;
   short int do_link_torsions = 0;
   float rama_plot_restraint_weight = 1.0;

   // coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_PLANES_NON_BONDED_AND_CHIRALS;
   coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_TORSIONS_PLANES_NON_BONDED_AND_CHIRALS;
   flags = coot::BONDS_ANGLES_AND_PLANES;
      
   // coot::restraint_usage_Flags flags = coot::TORSIONS;
   if (enable_rama_refinement) { 
      do_rama_restraints = 1;
      flags = coot::BONDS_ANGLES_TORSIONS_PLANES_NON_BONDED_CHIRALS_AND_RAMA;
      //      flags = coot::BONDS_ANGLES_TORSIONS_PLANES_NON_BONDED_AND_CHIRALS;
      //flags = coot::BONDS_ANGLES_TORSIONS_AND_PLANES;
      //flags = coot::BONDS_ANGLES_TORSIONS_PLANES_AND_NON_BONDED;
      //flags = coot::BONDS_ANGLES_TORSIONS_PLANES_AND_CHIRALS;
      //flags = coot::BONDS_AND_NON_BONDED;
      flags = coot::RAMA;
   } 

   std::cout << " ===================== enable_rama_refinement: " 
	     << " " << do_rama_restraints << " ========================" << std::endl;
   
   coot::pseudo_restraint_bond_type pseudos = coot::NO_PSEUDO_BONDS;
   int nrestraints = 
      restraints.make_restraints(geom, flags,
				 do_residue_internal_torsions,
				 rama_plot_restraint_weight,
				 do_rama_restraints,
				 pseudos);

   if (output_numerical_gradients)
      restraints.set_do_numerical_gradients();
   restraints.minimize(flags);

   int post_refine_selHnd = residues_mol_pair.first->NewSelection();
   int post_refine_nSelResidues; 
   PCResidue *post_refine_SelResidues = NULL;
   residues_mol_pair.first->Select(post_refine_selHnd, STYPE_RESIDUE, 0,
				   chn,
				   resno_mid-side_step, "",
				   resno_mid+side_step, "",
				   "*",  // residue name
				   "*",  // Residue must contain this atom name?
				   "*",  // Residue must contain this Element?
				   "",   // altLocs
				   SKEY_NEW // selection key
				   );
   residues_mol_pair.first->GetSelIndex(post_refine_selHnd,
					post_refine_SelResidues,
					post_refine_nSelResidues);

   residue_selection_t res_sel;
   res_sel.mol = residues_mol_pair.first;
   res_sel.SelectionHandle = post_refine_selHnd;
   res_sel.nSelResidues = post_refine_nSelResidues;
   res_sel.SelResidues = post_refine_SelResidues;

   long t1 = glutGet(GLUT_ELAPSED_TIME);
   float seconds = float(t1-t0)/1000.0;
   std::cout << "refinement_took " << seconds << " seconds" << std::endl;
   return res_sel;
#endif // HAVE_GSL
} 

int test_ramachandran_probabilities() {

   int r = 0;

   std::string file_name = greg_test("crashes_on_cootaneering.pdb");
   file_name = "37-41.pdb";
   atom_selection_container_t atom_sel = get_atom_selection(file_name);

   if (! atom_sel.read_success)
      throw std::runtime_error(file_name + ": file not found.");


   std::string chain_id = "A";
   char *chn = (char *) chain_id.c_str(); // mmdb thing.  Needs updating on new mmdb?
   std::vector<int> resnos;
   resnos.push_back(39);

   coot::protein_geometry geom;
   geom.init_standard();
   int n_correct = 0; 
   for (int i=0; i<resnos.size(); i++) { 
      int selHnd = atom_sel.mol->NewSelection();
      int nSelResidues; 
      PCResidue *SelResidues = NULL;
      atom_sel.mol->Select(selHnd, STYPE_RESIDUE, 0,
			   chn,
			   resnos[i]-2, "",
			   resnos[i]+2, "",
			   "*",  // residue name
			   "*",  // Residue must contain this atom name?
			   "*",  // Residue must contain this Element?
			   "",   // altLocs
			   SKEY_NEW // selection key
			   );
      atom_sel.mol->GetSelIndex(selHnd, SelResidues, nSelResidues);

      // get the probability
      std::string residue_type = SelResidues[1]->GetResName();

      clipper::Ramachandran::TYPE rama_type = clipper::Ramachandran::NonGlyPro;
      if (residue_type == "GLY")
	 rama_type = clipper::Ramachandran::Gly;
      if (residue_type == "PRO")
	 rama_type = clipper::Ramachandran::Pro;
      clipper::Ramachandran rama(rama_type);

      double prob = 0;

//       coot::util::phi_psi_t angles = coot::util::ramachandran_angles(SelResidues, nSelResidues);
//       rama.probability(clipper::Util::d2rad(angles.phi()),
// 		       clipper::Util::d2rad(angles.psi()));
      
      double post_refine_prob = 0.0; // set later

      // --------------------------------------------------
      //     non-Ramachandran refinement:
      // --------------------------------------------------
      if (0) { 
	 int enable_rama_refinement = 0;
	 residue_selection_t refined_res_sel =
	    testing_func_probabilities_refine_fragment(atom_sel, SelResidues,
						       nSelResidues,
						       chain_id, resnos[i], geom,
						       enable_rama_refinement,
						       1, 0, 1);

	 if (0) { 
	    // Let's look at the refined structure. Write them out as pdb files ;-/
	    std::string tmp_file_name = "rama-test-";
	    tmp_file_name += coot::util::int_to_string(i);
	    tmp_file_name += ".pdb";
	    refined_res_sel.mol->WritePDBASCII((char *)tmp_file_name.c_str());
	 }

	 coot::util::phi_psi_t post_refine_angles =
	    coot::util::ramachandran_angles(refined_res_sel.SelResidues,
					    refined_res_sel.nSelResidues);
	 refined_res_sel.clear_up();
	       
	 post_refine_prob =
	    rama.probability(clipper::Util::d2rad(post_refine_angles.phi()),
			     clipper::Util::d2rad(post_refine_angles.psi()));
      }

	       
      // --------------------------------------------------
      //     now with Ramachandran refinement:
      // --------------------------------------------------


      int enable_rama_refinement = 1;
      bool flank = 1;
      residue_selection_t rama_refined_res_sel =
	 testing_func_probabilities_refine_fragment(atom_sel, SelResidues,
						    nSelResidues,
						    chain_id, resnos[i], geom,
						    enable_rama_refinement, 1, flank, 1);
      coot::util::phi_psi_t rama_refine_angles =
	 coot::util::ramachandran_angles(rama_refined_res_sel.SelResidues,
					 rama_refined_res_sel.nSelResidues);
      rama_refined_res_sel.clear_up();
	       
      double rama_refine_prob =
	 rama.probability(clipper::Util::d2rad(rama_refine_angles.phi()),
			  clipper::Util::d2rad(rama_refine_angles.psi()));
      std::cout << "--------------------------------------\n";
      std::cout << "Pre-refine         Rama probability residue " << resnos[i] << ": "
		<< prob << std::endl;
      std::cout << "Post-simple refine Rama probability residue " << resnos[i] << ": "
		<< post_refine_prob << std::endl;
      std::cout << "Post-Rama   refine Rama probability residue " << resnos[i] << ": "
		<< rama_refine_prob << std::endl;
      std::cout << "--------------------------------------\n";

   }
   return r;
} 


int kdc_torsion_test() { 
  int r = 1;

#ifdef HAVE_GSL
  clipper::Coord_orth co1, co2, co3, co4;
  std::vector<double> params(12);
  const int n1(5), n2(7);
  const double dx = 1.0e-3;
  for ( int t1 = 0; t1 < n1; t1++ )
     for ( int t2 = 0; t2 < n2; t2++ ) {

	// std::cout << " ===== t1 = " << t1 << "   t2 = " << t2 << " ======" << std::endl;
	
	// set up angles at either end of torsion
	double p1 = 6.283 * double(t1) / double(n1);
	double p2 = 6.283 * double(t2) / double(n2);
	// set up atomic coords up z axis
	params[0]  = cos(p1); params[1]  = sin(p1); params[2]  = 0.0;
	params[3]  =     0.0; params[4]  =     0.0; params[5]  = 1.0;
	params[6]  =     0.0; params[7]  =     0.0; params[8]  = 2.0;
	params[9]  = cos(p2); params[10] = sin(p2); params[11] = 3.0;

	// now pertub the coords (params[12] is unperturbed)
	std::vector<std::vector<double> > dparams(13,params);
	std::vector<double> dresult(13), ngrad(12), agrad(12);
	for ( int i = 0; i < 12; i++ )
	   dparams[i][i] += dx;

	// calculate torsion and derivs numerically
	std::vector<clipper::Coord_orth> co(4);
	for ( int i = 0; i < dparams.size(); i++ ) {
	   const std::vector<double>& param = dparams[i];
	   // convert parameter list to coord orths
	   for (int j=0;j<4;j++)
	      for(int k=0;k<3;k++)
		 co[j][k]=param[3*j+k];
	   // calculate torsion using clipper
	   dresult[i] = clipper::Coord_orth::torsion( co[0], co[1], co[2], co[3] );
	}
	for ( int i = 0; i < 12; i++ ) {
// 	   std::cout << "i = " << i << " (" << dresult[i] << " - " << dresult[12]
// 		     << ")/" << dx << " = " << (dresult[i]-dresult[12])/dx << std::endl;
	   ngrad[i] = (dresult[i]-dresult[12])/dx;
	}
	// push the perturbation the other way
	for ( int i = 0; i < 12; i++ )
	   dparams[i][i] -= 2*dx; 
	for ( int i = 0; i < dparams.size(); i++ ) {
	   const std::vector<double>& param = dparams[i];
	   // convert parameter list to coord orths
	   for (int j=0;j<4;j++)
	      for(int k=0;k<3;k++)
		 co[j][k]=param[3*j+k];
	   // calculate torsion using clipper
	   dresult[i] = clipper::Coord_orth::torsion( co[0], co[1], co[2], co[3] );
	}
	for ( int i = 0; i < 12; i++ ) {
	   ngrad[i] -= (dresult[i]-dresult[12])/dx;
	   ngrad[i] *= 0.5; // average + and - shift
	}

      
	// first check clipper torsion calc
	double tor1 = dresult[12];
	double tor2 = p2-p1;
	if ( cos( tor1 - tor2 ) < 0.999999 ) {
	   std::cout <<"TORSION ERROR (CLIPPER)"<<tor1<<" != "<<tor2<<std::endl;
	   r = 0;
	}

      
	// now check coot torsion calc
	for (int j=0;j<4;j++) for(int k=0;k<3;k++)
	   co[j][k]=params[3*j+k];
	coot::distortion_torsion_gradients_t dtg =
	   coot::fill_distortion_torsion_gradients( co[0], co[1], co[2], co[3] );
	double tor3 = clipper::Util::d2rad(dtg.theta);
	if ( cos( tor1 - tor3 ) < 0.999999 ) {
	   std::cerr <<"TORSION ERROR (COOT)  "<<tor1<<" != "<<tor3<<std::endl;
	   r = 0;
	}

	// ts = torsion_scale
	double ts = (1.0/(1+pow(tan(clipper::Util::d2rad(dtg.theta)),2)));

	// now fetch coot torsion gradients
	agrad[0] = ts*dtg.dD_dxP1; agrad[1] = ts*dtg.dD_dyP1; agrad[2] = ts*dtg.dD_dzP1;
	agrad[3] = ts*dtg.dD_dxP2; agrad[4] = ts*dtg.dD_dyP2; agrad[5] = ts*dtg.dD_dzP2;
	agrad[6] = ts*dtg.dD_dxP3; agrad[7] = ts*dtg.dD_dyP3; agrad[8] = ts*dtg.dD_dzP3;
	agrad[9] = ts*dtg.dD_dxP4; agrad[10]= ts*dtg.dD_dyP4; agrad[11]= ts*dtg.dD_dzP4;
	for ( int i = 0; i < 12; i++ )
	   if ( fabs( ngrad[i] - agrad[i] ) > 1.0e-4 ) {
	      char xyz[] = "xyz";
	      std::cerr << "TORSIONS " << i/3 << xyz[i%3] << " "
			<< ngrad[i] << " vs " << agrad[i] << std::endl;
	      r = 0;
	   }
     }
#endif // HAVE_GSL
  return r;
}


int
test_fragmemt_atom_selection() {

   int status = 0;

   bool fill_masking_molecule_flag = 1;
   std::string atom_selection_string = "//A,B/1-5";
   int n_atoms_in_frag = 64;
   // from wc, there are 64   atoms in that atom selection in tutorial-modern.pdb
   //          there are 1465 atoms in tutorial-modern.pdb
   
   std::string f = greg_test("tutorial-modern.pdb");
   atom_selection_container_t asc = get_atom_selection(f);
   
   std::pair<coot::minimol::molecule, coot::minimol::molecule> p = 
      coot::make_mols_from_atom_selection_string(asc.mol, atom_selection_string,
						 fill_masking_molecule_flag);

   // now test the number of atoms in first and second
   int n_initial = asc.n_selected_atoms;
   int n_1 = p.first.count_atoms();
   int n_2 = p.second.count_atoms();
   

   std::cout << "   n_initial: " << n_initial << "   n_1: " << n_1 << "   n_2: "
	     << n_2 << std::endl;
   if (n_1 == (n_initial - n_atoms_in_frag))
      if (n_2 == n_atoms_in_frag)
	 status = 1;
      
   return status;
}

int test_peptide_link() {
   
   std::string f = "1h4p.pdb";
   atom_selection_container_t asc = get_atom_selection(f);
   if (! asc.read_success)
      return 0;

   std::vector<std::pair<bool,CResidue *> > residues;
   CMMDBManager *mol = asc.mol;
   int imod = 1;
   CModel *model_p = mol->GetModel(imod);
   CChain *chain_p;
   // run over chains of the existing mol
   int nchains = model_p->GetNumberOfChains();
   for (int ichain=0; ichain<nchains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      int nres = chain_p->GetNumberOfResidues();
      std::string chain_id = chain_p->GetChainID();
      if (chain_id == "B") { 
	 PCResidue residue_p;
	 for (int ires=0; ires<nres; ires++) { 
	    residue_p = chain_p->GetResidue(ires);
	    int resno = residue_p->GetSeqNum();
	    if ((resno == 1455) || (resno == 1456))
	       residues.push_back(std::pair<bool, CResidue *>(0, residue_p));
	 }
      }
   }

   if (residues.size() != 2) {
      return 0; 
   } 

   // ======== Now start the test for real ===============
   //
   coot::protein_geometry geom;
   geom.init_standard();

   try {
      std::string comp_id_1 = "MAN";
      std::string comp_id_2 = "MAN";
      std::string group_1 = "D-pyranose";
      std::string group_2 = "D-pyranose";

      clipper::Xmap<float> xmap;
      float weight = 1.0;
      std::vector<coot::atom_spec_t> fixed_atom_specs;
      coot::restraints_container_t
	 restraints(residues, geom, mol, fixed_atom_specs, xmap, weight);
      std::string link_type = "";
      // restraints.find_link_type(residues[0].second,
      // 		residues[1].second,
      // 		geom);

      std::cout << "   link_type: " << link_type << ":" << std::endl;
      
      std::vector<std::pair<coot::chem_link, bool> > link_infos =
	 geom.matching_chem_link(comp_id_1, group_1, comp_id_2, group_2);
      unsigned int ilink = 0;
      std::cout << "Found link :" << link_infos[ilink].first.Id() << ":" << std::endl;
      if (link_infos[ilink].first.Id() != "BETA1-3") {
	 return 0;
      } 
   }

   catch (std::runtime_error mess) {
      std::cout << "in test_peptide_link() matching_chem_link fails" << std::endl;
   }

   return 1;

} 

 
// Restraints are correctly generated for vector of residues (new
// restraints constructor needed) 20081106
int
restr_res_vector() {

   std::string f = greg_test("tutorial-modern.pdb");
   //    f = "7_and_96_B-a-result.pdb";
//    f = "6_7_and_96_B.pdb";
//    f = "6_7.pdb";
   atom_selection_container_t asc = get_atom_selection(f);

   std::vector<std::pair<bool,CResidue *> > residues;
   CMMDBManager *mol = asc.mol;
   std::cout << "restr_res_vector: mol: " << mol << std::endl;
   std::vector<coot::atom_spec_t> fixed_atom_specs;

   if (!asc.read_success)
      return 0;
   
   int imod = 1;
   CModel *model_p = mol->GetModel(imod);
   CChain *chain_p;
   // run over chains of the existing mol
   int nchains = model_p->GetNumberOfChains();
   for (int ichain=0; ichain<nchains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      int nres = chain_p->GetNumberOfResidues();
      std::string chain_id = chain_p->GetChainID();
      if (chain_id == "B") { 
	 PCResidue residue_p;
	 for (int ires=0; ires<nres; ires++) { 
	    residue_p = chain_p->GetResidue(ires);
	    int resno = residue_p->GetSeqNum();
	    if ((resno == 7) || (resno == 96))
	       residues.push_back(std::pair<bool, CResidue *>(0, residue_p));
	 }
      }
   }

   if (residues.size() != 2) {
      std::cout << "  Fail to find residues - found " << residues.size() << std::endl;
      return 0;
   } else {
      clipper::Xmap<float> xmap;
      coot::util::map_fill_from_mtz(&xmap, "rnasa-1.8-all_refmac1.mtz", "FWT", "PHWT", "WT", 0, 0);
      float weight = 1;
      coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_PLANES_NON_BONDED_AND_CHIRALS;
      coot::protein_geometry geom;
      geom.init_standard();
      coot::restraints_container_t
	 restraints(residues, geom, mol, fixed_atom_specs, xmap, weight);
      restraints.make_restraints(geom, flags, 0, 0.0, 0, coot::NO_PSEUDO_BONDS);
      restraints.minimize(flags);
      restraints.write_new_atoms("ss-test.pdb");
   }
   return 0;
} 

 
int
test_add_atom() {

   int status = 0;

   std::string f = greg_test("tutorial-modern.pdb");
   atom_selection_container_t asc = get_atom_selection(f);

   int n_test_residues = 20;
   int pass_count = 0;
   int ires_count = 0;
   
   int imod = 1;
   CModel *model_p = asc.mol->GetModel(imod);
   CChain *chain_p;
   // run over chains of the existing mol
   int nchains = model_p->GetNumberOfChains();
   for (int ichain=0; ichain<nchains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      int nres = chain_p->GetNumberOfResidues();
      PCResidue residue_p;
      CAtom *at;
      while (ires_count<n_test_residues) {
	 residue_p = chain_p->GetResidue(ires_count);
	 ires_count++;
	 std::string res_name(residue_p->GetResName());
	 if (res_name == "GLY" ||
	     res_name == "ALA" ) {
	    pass_count++; // automatic pass
	 } else {
	    // normal case
	    coot::chi_angles chi(residue_p, 0);
	    std::vector<std::pair<int, float> > chi_angles = chi.get_chi_angles();
	    if (!chi_angles.size() > 0) {
	       std::cout << "   Failed to find chi angles in residue "
			 << coot::residue_spec_t(residue_p) << std::endl;
	    } else {
	       float bond_length = 1.53;
	       float angle = 113.8;
	       std::string ref_atom_name(" CG ");
	       if (res_name == "CYS") { 
		  ref_atom_name = " SG ";
		  bond_length = 1.808;
	       } 
	       if (res_name == "THR") { 
		  ref_atom_name = " OG1";
		  bond_length = 1.433;
		  angle = 109.600;
	       } 
	       if (res_name == "VAL")
		  ref_atom_name = " CG1";
	       if (res_name == "SER") { 
		  ref_atom_name = " OG ";
		  bond_length = 1.417;
	       }
	       if (res_name == "PRO") { 
		  bond_length = 1.492;
		  angle = 104.500;
	       }
	       CAtom *ref_atom = residue_p->GetAtom(ref_atom_name.c_str());
	       if (!ref_atom) {
		  std::cout << "   Failed to find reference CG in residue "
			    << coot::residue_spec_t(residue_p) << std::endl;
	       } else { 
		  double tors = chi_angles[0].second;
		  bool status = coot::util::add_atom(residue_p, " N  ", " CA ", " CB ", "",
						     bond_length,
						     angle, 
						     tors,
						     " XX ", " C", 1.0, 20.0);
		  if (status) {
		     clipper::Coord_orth ref_pt(ref_atom->x, ref_atom->y, ref_atom->z);
		     CAtom *new_atom = residue_p->GetAtom(" XX ");
		     if (! new_atom) {
			std::cout << "   Failed to find reference CG in residue "
				  << coot::residue_spec_t(residue_p) << std::endl;
		     } else {
			clipper::Coord_orth new_pos(new_atom->x, new_atom->y, new_atom->z);
			double d = clipper::Coord_orth::length(ref_pt, new_pos);
			if (d < 0.12) {
			   // std::cout << "   Pass make atom " << std::endl;
			   pass_count++; 
			} else {
			   std::cout << "   Failed closeness test, d = " << d << " "
				     << new_atom << " vs " << ref_atom << std::endl;
			}
		     }
		  } else {
		     std::cout << "   Failed to add atom to residue "
			       << coot::residue_spec_t(residue_p) << std::endl;
		  }
	       }
	    } 
	 } 
      }
   }
   if (pass_count == n_test_residues)
      status = 1;
   
   return status;
}

int
test_dipole() {

   int result = 0; // fail initially.
   testing_data t;
   
   std::string res_type = "TYR";
   
   std::string filename = greg_test("tutorial-modern.pdb");
   atom_selection_container_t atom_sel = get_atom_selection(filename);

   std::pair<short int, coot::dictionary_residue_restraints_t> rp = 
      t.geom.get_monomer_restraints(res_type);

   if (rp.first) { 
   
      int imod = 1;
      CModel *model_p = atom_sel.mol->GetModel(imod);
      CChain *chain_p;
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 int nres = chain_p->GetNumberOfResidues();
	 PCResidue residue_p;
	 for (int ires=0; ires<nres; ires++) { 
	    residue_p = chain_p->GetResidue(ires);
	    if (std::string(residue_p->GetResName()) == res_type) { 
	       coot::dipole d(rp.second, residue_p);
	       std::cout << "residue " << coot::residue_spec_t(residue_p)
			 << "   dipole: " << d << " at " << d.position().format()
			 << std::endl;
	       break;
	    }
	 }
      }
   }
   return result;
}


int
test_dictionary_partial_charges() {

   std::vector<std::string> v;
   v.push_back("ALA");
   v.push_back("ARG");
   v.push_back("TYR");
   v.push_back("TRP");
   v.push_back("VAL");
   v.push_back("SER");

   testing_data t;
   for (unsigned int iv=0; iv<v.size(); iv++) {
      std::pair<short int, coot::dictionary_residue_restraints_t> rp = 
	 t.geom.get_monomer_restraints(v[iv]);
      if (! rp.first) {
	 std::cout << " Fail - no restraints for " << v[iv] << std::endl;
	 return 0;
      } else {
	 for (unsigned int iat=0; iat<rp.second.atom_info.size(); iat++) {
	    if (! rp.second.atom_info[iat].partial_charge.first) {
	       std::cout << " Fail - no partial charge for "
			 << rp.second.atom_info[iat].atom_id << " in "
			 << v[iv] << std::endl;
	       return 0;
	    }
	 }
      } 
   }
   return 1;
}

int test_segid_exchange() {

   int status = 0;
   
   std::string filename = greg_test("tutorial-modern.pdb");
   atom_selection_container_t atom_sel = get_atom_selection(filename);
   bool ifound = 0;

   std::vector<CResidue *> residues;

   int imod = 1;
   if (atom_sel.read_success > 0) { 
      CModel *model_p = atom_sel.mol->GetModel(imod);
      CChain *chain_p;
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 std::string chain_id = chain_p->GetChainID();
	 int nres = chain_p->GetNumberOfResidues();
	 PCResidue residue_p;
	 for (int ires=0; ires<nres; ires++) { 
	    residue_p = chain_p->GetResidue(ires);
	    residues.push_back(residue_p);
	    if (residues.size() == 3)
	       break;
	 }
	 if (residues.size() == 3)
	    break;
      }

      if (residues.size() == 3) {
	 PPCAtom residue_atoms_1;
	 PPCAtom residue_atoms_2;
	 PPCAtom residue_atoms_3;
	 int n_residue_atoms_1;
	 int n_residue_atoms_2;
	 int n_residue_atoms_3;
	 
	 std::string new_seg_id  = "N";
	 
	 residues[0]->GetAtomTable(residue_atoms_1, n_residue_atoms_1);
	 for (int iat=0; iat<n_residue_atoms_1; iat++) {
	    CAtom *at = residue_atoms_1[iat];
	    at->SetAtomName(at->GetIndex(),
			    at->serNum,
			    at->GetAtomName(),
			    at->altLoc,
			    new_seg_id.c_str(),
			    at->GetElementName());
	 }

	 // testing this function:
	 coot::copy_segid(residues[0], residues[1]);
	 
	 residues[1]->GetAtomTable(residue_atoms_2, n_residue_atoms_2);
	 for (int iat=0; iat<n_residue_atoms_2; iat++) {
	    CAtom *at = residue_atoms_2[iat];
	    std::string this_seg_id = at->segID;
	    if (this_seg_id != new_seg_id) {
	       std::cout << "   Failed to copy seg id.  Was :"
			 << this_seg_id << ": should be :" << new_seg_id
			 << ":\n for atom " << at << std::endl;
	       return 0; // fail
	    }
	 }


	 // Now something harder, test that the segids are unchanged
	 // when the segids of provider are not consistent.
	 //

	 std::cout << "   Test with a rogue segid " << std::endl;
	 // Add a rogue:
	 // 
	 CAtom *at = residue_atoms_1[2];
	 at->SetAtomName(at->GetIndex(),
			 at->serNum,
			 at->GetAtomName(),
			 at->altLoc,
			 "C",
			 at->GetElementName());

	 residues[2]->GetAtomTable(residue_atoms_3, n_residue_atoms_3);

	 std::vector<std::string> orig_seg_ids;
	 for (int iat=0; iat<n_residue_atoms_2; iat++) {
	    CAtom *at = residue_atoms_2[iat];
	    std::string this_seg_id = at->segID;
	    orig_seg_ids.push_back(this_seg_id);
	 }

	 coot::copy_segid(residues[0], residues[2]);
	 bool fail_this = 0;
	 for (int iat=0; iat<n_residue_atoms_2; iat++) {
	    CAtom *at = residue_atoms_2[iat];
	    std::string this_seg_id = at->segID;
	    if (this_seg_id != orig_seg_ids[iat]) {
	       std::cout << "  Failed: segid changed when it shouldn't"
			 << " have, for " << at << std::endl;
	       fail_this = 1;
	       break;
	    }
	 }

	 // if we go to here it should be ok if fail_this didn't fail
	 if (! fail_this)
	    status = 1;
      } 
   }
   return status;
}

int test_ligand_fit_from_given_point() {

   int status = 0;
   int n_conformers = 5;
   testing_data t;

   std::string cif_file_name = "libcheck_3GP-torsion-filtered.cif";
   int geom_stat = t.geom.init_refmac_mon_lib(cif_file_name, 0);
   if (geom_stat == 0) {
      std::string m = "Critical cif dictionary reading failure.";
      std::cout << m << std::endl;
      throw std::runtime_error(m);
   }
   
   std::string f = greg_test("tutorial-modern.pdb");
   atom_selection_container_t asc = get_atom_selection(f);
   if (!asc.read_success)
      return 0;
   std::string l = greg_test("monomer-3GP.pdb");
   atom_selection_container_t l_asc = get_atom_selection(l);
   if (!l_asc.read_success)
      return 0;
   
   
   clipper::Xmap<float> xmap;
   std::string mtz_file_name;
   mtz_file_name = getenv("HOME");
   mtz_file_name += "/data/greg-data/rnasa-1.8-all_refmac1.mtz";
   
   bool stat = coot::util::map_fill_from_mtz(&xmap, mtz_file_name,
					     "FWT", "PHWT", "WT", 0, 0);

   if (!stat) {
      std::cout << "   ERROR:: Bad map fill from " << mtz_file_name << "\n";
      return 0; 
   }

   
   coot::wligand wlig;
   wlig.set_verbose_reporting();
   wlig.set_debug_wiggly_ligands();
   wlig.import_map_from(xmap);
   bool optim_geom = 1;
   bool fill_vec = 0;
   coot::minimol::molecule mmol(l_asc.mol);
   wlig.install_simple_wiggly_ligands(&t.geom, mmol, n_conformers, optim_geom, fill_vec);
   short int mask_waters_flag = 1;
   wlig.mask_map(asc.mol, mask_waters_flag);
   clipper::Coord_orth pt(55.06, 10.16, 21.73); // close to 3GP peak (not in it).
   float n_sigma = 1.0; // cluster points must be more than this.
   wlig.cluster_from_point(pt, n_sigma);
   wlig.fit_ligands_to_clusters(1);
   int n_final_ligands = wlig.n_final_ligands();
   if (n_final_ligands == 0) {
      return 0;
   } else { 
      coot::minimol::molecule m = wlig.get_solution(0);
      clipper::Coord_orth centre = m.centre();
      clipper::Coord_orth ref_pt(55.5, 9.36, 20.7); // coords of centred
                                                    // correct solution
      double d = clipper::Coord_orth::length(centre, ref_pt);
      if (d < 1.0) {
	 std::cout << " found distance from reference centre "
		   << d << std::endl;
	 status = 1;
      }
   }

   return status;
}


int test_ligand_conformer_torsion_angles() { 

   int status = 0;
   testing_data t;

   std::string cif_file_name = "libcheck_3GP-torsion-filtered.cif";
   int geom_stat = t.geom.init_refmac_mon_lib(cif_file_name, 0);
   if (geom_stat == 0) {
      std::string m = "Critical cif dictionary reading failure.";
      std::cout << m << std::endl;
      throw std::runtime_error(m);
   }
   
   std::string l = greg_test("monomer-3GP.pdb");
   atom_selection_container_t l_asc = get_atom_selection(l);
   if (!l_asc.read_success)
      return 0;
   
   coot::wligand wlig;
   wlig.set_verbose_reporting();
   wlig.set_debug_wiggly_ligands();
   bool optim_geom = 0;
   bool fill_vec = 1;
   int n_conformers = 200;
   coot::minimol::molecule mmol(l_asc.mol);
   std::vector<coot::installed_wiggly_ligand_info_t> conformer_info = 
      wlig.install_simple_wiggly_ligands(&t.geom, mmol, n_conformers,
					 optim_geom, fill_vec);
   std::cout << "INFO:: there were " << conformer_info.size()
	     << " returned conformers" << std::endl;
   for (unsigned int i=0; i<conformer_info.size(); i++) {
      unsigned int itor=0;
      std::pair<float, float> p = conformer_info[i].get_set_and_real_torsions(itor);
      std::cout << "   " << i << " " << itor << "  set: " << p.first << " real: "
		<< p.second << std::endl;
   }

   return 1;
}

#include "peak-search.hh"
int test_peaksearch_non_close_peaks() {

   clipper::Xmap<float> xmap;
   std::string mtz_file_name;
   mtz_file_name = getenv("HOME");
   mtz_file_name += "/data/greg-data/rnasa-1.8-all_refmac1.mtz";
   
   bool stat = coot::util::map_fill_from_mtz(&xmap, mtz_file_name,
					     "FWT", "PHWT", "WT", 0, 0);
   if (!stat) {
      std::cout << "   ERROR:: Bad map fill from " << mtz_file_name << "\n";
      return 0; 
   }

   double d_crit = 2.0; // don't return peaks that have a higher
		       // (absolute) peak less than 4.0 A away.


   coot::peak_search ps(xmap);
   ps.set_max_closeness(d_crit);
   std::vector<std::pair<clipper::Coord_orth, float> > peaks = 
      ps.get_peaks(xmap, 0.5, 1, 1);

   if (peaks.size() < 20) {
      std::cout << "   Not enough peaks! " << peaks.size() << std::endl;
      return 0;
   } 

   std::vector<std::pair<clipper::Coord_orth, float> > problems;


   for (unsigned int ipeak=0; ipeak<(peaks.size()-1); ipeak++) {
      for (unsigned int jpeak=(ipeak+1); jpeak<peaks.size(); jpeak++) {
	 double d = clipper::Coord_orth::length(peaks[ipeak].first, peaks[jpeak].first);
	 if (d < d_crit) { 
	    problems.push_back(peaks[jpeak]);
	    break;
	 }
      }
   }

   std::cout << "   There are " << peaks.size() << " peaks and "
	     << problems.size() << " problem peaks" << std::endl;
   if (problems.size() == 0) {
      return 1;
   } else {
      return 0;
   } 
   
} 

int test_symop_card() {

   int r = 0;

   std::string symop_string = "X-1,Y,Z";
   coot::symm_card_composition_t sc(symop_string);
   std::cout << sc;

   float rev[9] = {1,0,0,0,1,0,0,0,1};
   float tev[3] = {-1,0,0};

   if ((close_float_p(sc.x_element[0], rev[0])) && 
       (close_float_p(sc.x_element[1], rev[1])) && 
       (close_float_p(sc.x_element[2], rev[2]))) {
      if ((close_float_p(sc.y_element[0], rev[3])) && 
	  (close_float_p(sc.y_element[1], rev[4])) && 
	  (close_float_p(sc.y_element[2], rev[5]))) {
	 if ((close_float_p(sc.z_element[0], rev[6])) && 
	     (close_float_p(sc.z_element[1], rev[7])) && 
	     (close_float_p(sc.z_element[2], rev[8]))) {
	    if ((close_float_p(sc.trans_frac(0), tev[0])) && 
		(close_float_p(sc.trans_frac(1), tev[1])) && 
		(close_float_p(sc.trans_frac(2), tev[2]))) {
	       r = 1;
	    }
	 }
      }
   }

   return r;
}

int test_coot_atom_tree() {

   std::cout << "Atom tree test" << std::endl;
   int r = 0;
   coot::dictionary_residue_restraints_t rest("XXX",0);
   CResidue *res = 0;

   // test that the exception is thrown by setting b = 1 there, don't continue if not
   bool b = 0;
   try { 
      coot::atom_tree_t tree(rest, res, "");
   }
   catch (std::runtime_error rte) {
      std::cout << rte.what() << std::endl;
      b = 1;
   } 
   if (b == 0) { 
      std::cout << "No throw on null res" << std::endl;
      return 0;
   } 

   // Now test that the exception is thrown when there is no atom tree
   // (this test might not be necessary later).
   //
   b = 0;
   res = new CResidue;
   try { 
      coot::atom_tree_t tree(rest, res, "");
   }
   catch (std::runtime_error rte) {
      std::cout << rte.what() << std::endl;
      b = 1;
   } 
   if (b == 0) { 
      std::cout << "No throw on no tree" << std::endl;
      return 0;
   } 

   delete res; // clear up first
   
   // OK give it something correct then

   // setup the restraints (p.second). Reading from a file in this
   // directory.  That should be fixed at some stage.
   
   std::string cif_file_name = "libcheck_ASP.cif";
   coot::protein_geometry geom;
   int geom_stat = geom.init_refmac_mon_lib(cif_file_name, 0);
   std::pair<short int, coot::dictionary_residue_restraints_t> p = 
      geom.get_monomer_restraints("ASP");

   if (!p.first) {
      std::cout << "No ASP in dictionary" << std::endl;
      return 0;
   } 

   // Now get a residue
   std::string filename = greg_test("tutorial-modern.pdb");
   atom_selection_container_t atom_sel = get_atom_selection(filename);
   bool ifound = 0;

   int imod = 1;
   res = test_get_residue(atom_sel.mol, "B", 1);

   if (0) {
      try {
	 r = test_tree_rotation(p.second, res, " CB ", " CG ", 0);
	 if (r) 
	    r = test_tree_rotation(p.second, res, " CB ", " CG ", 1);
      }
      catch (std::runtime_error rte) {
	 std::cout << rte.what() << std::endl;
      }
   }

   if (1) {
      try {
	 filename = "monomer-3GP.pdb";
	 atom_selection_container_t atom_sel = get_atom_selection(filename);
	 if (!atom_sel.read_success) {
	    std::cout << "monomer-3GP.pdb not read successfully." << std::endl;
	 } else { 
	    CResidue *res = test_get_residue(atom_sel.mol, "A", 1);
	    if (res) {
	       geom_stat = geom.init_refmac_mon_lib("libcheck_3GP.cif", 0);
	       std::pair<short int, coot::dictionary_residue_restraints_t> p = 
		  geom.get_monomer_restraints("3GP");
	       if (p.first) { 
		  bool test1 = test_tree_rotation(p.second, res, " N9 ", " C1*", 0);
		  atom_sel.mol->WritePDBASCII("3GP-test1.pdb");
		  bool test2 = test_tree_rotation(p.second, res, " N9 ", " C1*", 1);
		  atom_sel.mol->WritePDBASCII("3GP-test2.pdb");
		  if (test1 && test2)
		     r = 1;
	       } else { 
		  std::cout << "Getting restraints for 3GP failed" << std::endl;
	       } 
	    }
	 }
      }
      catch (std::runtime_error rte) {
	 std::cout << rte.what() << std::endl;
	 r = 0; 
      }
   }

   return r;
}



bool
test_atom_tree_t::test_atom_vec(const std::vector<std::vector<int> > &contact_indices) const {

   bool r = 0;

   for (unsigned int iav=0; iav<atom_vertex_vec.size(); iav++) {
      std::cout << " vertex number: " << iav << " back [";
      for (unsigned int ib=0; ib<atom_vertex_vec[iav].backward.size(); ib++) {
	 std::cout << atom_vertex_vec[iav].backward[ib] << ","; 
      }
      std::cout << "] "; // end of backatom list

      std::cout << "forward ["; 
      for (unsigned int ifo=0; ifo<atom_vertex_vec[iav].forward.size(); ifo++) {
	 std::cout << atom_vertex_vec[iav].forward[ifo] << ","; 
      }
      std::cout << "] " << std::endl; // end of forward atom list
   }
   return r;
}

int
test_coot_atom_tree_2() {

   //                    o 4 
   //                   /
   //     0          1 /
   //     o --------- o                 0: NE1 (at origin)
   //     |           |                 1: CD
   //     |           |                 2: CE2
   //     |           |                 3: CZ
   //     o --------- o                 4: CG
   //     3           2
   // 
   int r = 0;

   std::vector<std::pair<std::string, clipper::Coord_orth> > names;
   names.push_back(std::pair<std::string, clipper::Coord_orth> (" NE1", clipper::Coord_orth(0,0,0)));
   names.push_back(std::pair<std::string, clipper::Coord_orth> (" CD ", clipper::Coord_orth(1,0,0)));
   names.push_back(std::pair<std::string, clipper::Coord_orth> (" CE2", clipper::Coord_orth(1,-1,0)));
   names.push_back(std::pair<std::string, clipper::Coord_orth> (" CZ ", clipper::Coord_orth(0,-1,0)));
   names.push_back(std::pair<std::string, clipper::Coord_orth> (" CG ", clipper::Coord_orth(0.5,1.5,0)));
		   
   CResidue *residue_p = new CResidue;
   for (int i=0; i<5; i++) {
      CAtom *at = new CAtom();
      at->SetAtomName(names[i].first.c_str());
      at->SetCoordinates(names[i].second.x(), names[i].second.y(), names[i].second.z(), 1.0, 20.0);
      residue_p->AddAtom(at);
   } 
   
   std::vector<std::vector<int> > contact_indices(5);
   contact_indices[0].push_back(1);
   contact_indices[0].push_back(3);
   contact_indices[1].push_back(0);
   contact_indices[1].push_back(2);
   contact_indices[1].push_back(4);
   contact_indices[2].push_back(1);
   contact_indices[2].push_back(3);
   contact_indices[3].push_back(0);
   contact_indices[3].push_back(2);
   contact_indices[4].push_back(1);

   test_atom_tree_t tat(contact_indices, 2, residue_p, "");
   bool tat_result = tat.test_atom_vec(contact_indices);

   double test_angle = 30.0; // degress
   double tar = (M_PI/180.0)* test_angle;
   bool reverse_flag = 0;
   tat.rotate_about(" CZ ", " CE2", tar, reverse_flag);
   reverse_flag = 1;
   tat.rotate_about(" CZ ", " CE2", tar, reverse_flag);
   
   delete residue_p;
   return r;
}

int
test_coot_atom_tree_proline() {

   int r = 0; 
   std::string filename = greg_test("tutorial-modern.pdb");
   atom_selection_container_t atom_sel = get_atom_selection(filename);
   CResidue *res_pro = test_get_residue(atom_sel.mol, "A", 12);
   if (res_pro) {
      coot::protein_geometry geom;
      geom.init_standard();
      PPCAtom residue_atoms;
      int n_residue_atoms;
      res_pro->GetAtomTable(residue_atoms, n_residue_atoms);
      std::vector<clipper::Coord_orth> before_pos(n_residue_atoms);
      std::vector<clipper::Coord_orth>  after_pos(n_residue_atoms);
      for (int iat=0; iat<n_residue_atoms; iat++)
	 before_pos[iat]=clipper::Coord_orth(residue_atoms[iat]->x,
					     residue_atoms[iat]->y,
					     residue_atoms[iat]->z);
      for (int iat=0; iat<n_residue_atoms; iat++) 
	 std::cout << "Atom Table " << iat << " " << residue_atoms[iat]->name << std::endl;
      std::vector<std::vector<int> > contact_indices =
	 coot::util::get_contact_indices_for_PRO_residue(residue_atoms,
							 n_residue_atoms,
							 &geom);

      int base_atom_index = 0;
      test_atom_tree_t tat(contact_indices, base_atom_index, res_pro, "");
      bool reverse_flag = 0;
      double test_angle = 30.0; // degress
      double tar = (M_PI/180.0)* test_angle;
      //      tat.rotate_about(" N  ", " CA ", tar, reverse_flag);
      tat.rotate_about(" CA ", " CB ", tar, reverse_flag);
      //      tat.rotate_about(" CB ", " CG ", tar, reverse_flag);

      for (int iat=0; iat<n_residue_atoms; iat++)
	 after_pos[iat]=clipper::Coord_orth(residue_atoms[iat]->x,
					    residue_atoms[iat]->y,
					    residue_atoms[iat]->z);
      for (int i=0; i<n_residue_atoms; i++) { 
	 double d = clipper::Coord_orth::length(before_pos[i],after_pos[i]);
	 if (d > 0.0001)
	    std::cout << "test: atom " << i << " " << residue_atoms[i]->name << " moved" << std::endl;
	 else 
	    std::cout << "test: atom " << i << " " << residue_atoms[i]->name << " static" << std::endl;
      }
   }
   return r;
}


// can return null;
CResidue *test_get_residue(CMMDBManager *mol, const std::string &chain_id_ref, int resno_ref) {

   CResidue *residue_p = 0;
   
   int imod = 1;
   CResidue *res = 0;

   CModel *model_p = mol->GetModel(imod);
   CChain *chain_p;
   int nchains = model_p->GetNumberOfChains();
   for (int ichain=0; ichain<nchains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      std::string chain_id = chain_p->GetChainID();
      if (chain_id == chain_id_ref) {
	 int nres = chain_p->GetNumberOfResidues();
	 PCResidue res;
	 for (int ires=0; ires<nres; ires++) { 
	    res = chain_p->GetResidue(ires);
	    int resno = res->GetSeqNum();
	    if (resno == resno_ref) {
	       residue_p = res;
	       break;
	    }
	 }
      }
      if (residue_p)
	 break;
   }
   return residue_p;
} 


bool test_tree_rotation(const coot::dictionary_residue_restraints_t &rest,
			CResidue *res,
			const std::string &rotate_atom_1,
			const std::string &rotate_atom_2,
			bool reverse_flag) {


   
   bool r = 0;
   coot::atom_tree_t tree(rest, res, "");
   PPCAtom residue_atoms;
   int n_residue_atoms;
   res->GetAtomTable(residue_atoms, n_residue_atoms);
   std::vector<clipper::Coord_orth> before_pos(n_residue_atoms);
   std::vector<clipper::Coord_orth>  after_pos(n_residue_atoms);
   for (int iat=0; iat<n_residue_atoms; iat++)
      before_pos[iat]=clipper::Coord_orth(residue_atoms[iat]->x,
					  residue_atoms[iat]->y,
					  residue_atoms[iat]->z);
   if (0) 
      for (int i=0; i<n_residue_atoms; i++)
	 std::cout << "   Test Before atom " << residue_atoms[i] << std::endl;

   double test_angle = 3.0; // degress
   double tar = (M_PI/180.0)* test_angle;
   
   tree.rotate_about(rotate_atom_1, rotate_atom_2, tar, reverse_flag);
   std::cout << std::endl; // separator
   for (int iat=0; iat<n_residue_atoms; iat++)
      after_pos[iat]=clipper::Coord_orth(residue_atoms[iat]->x,
					 residue_atoms[iat]->y,
					 residue_atoms[iat]->z);
   if (0) 
      for (int i=0; i<n_residue_atoms; i++)
	 std::cout << "    Test After atom " << residue_atoms[i] << std::endl;
      
   double dist = 0.0;
   for (int i=0; i<n_residue_atoms; i++) { 
      double d = clipper::Coord_orth::length(before_pos[i],after_pos[i]);
      if (d > 0.0001)
	 std::cout << "test: atom " << i << " " << residue_atoms[i]->name << " moved" << std::endl;
      else 
	 std::cout << "test: atom " << i << " " << residue_atoms[i]->name << " static" << std::endl;
      dist += d;
   }

   // Test that the moving atoms rotated the correct amount.
   //
   // First we need to find the rotate positions for the give atoms names.
   //

   clipper::Coord_orth r_pt_1;
   clipper::Coord_orth r_pt_2;
   for (int iat=0; iat<n_residue_atoms; iat++) {
      std::string at_name(residue_atoms[iat]->name);
      if (at_name == rotate_atom_1)
	 r_pt_1 = clipper::Coord_orth(residue_atoms[iat]->x,
				      residue_atoms[iat]->y,
				      residue_atoms[iat]->z);
      if (at_name == rotate_atom_2)
	 r_pt_2 = clipper::Coord_orth(residue_atoms[iat]->x,
				      residue_atoms[iat]->y,
				      residue_atoms[iat]->z);
   }

   r = 1; // intially success
   for (int i=0; i<n_residue_atoms; i++) { 
      double d = clipper::Coord_orth::length(before_pos[i], after_pos[i]);
      if (d > 0.0001) {
	 std::string at_name(residue_atoms[i]->name);
	 bool v = test_rotate_atom_angle(at_name,
					 r_pt_1, r_pt_2, before_pos[i], after_pos[i], test_angle);
	 if (v == 0) {
	    std::cout << " fail in test_rotate_atom_angle " << i << " "
		      << residue_atoms[i]->name << std::endl;
	    r = 0;
	    break;
	 }
      }
   }

   return r;

}


int 
test_rotate_round_vector() {

   int r = 0;
   
   std::string filename = "monomer-3GP.pdb";
   atom_selection_container_t atom_sel = get_atom_selection(filename);

   std::string rotate_atom_1 = " N9 ";
   std::string rotate_atom_2 = " C1*";

   CResidue *residue_p = test_get_residue(atom_sel.mol, "A", 1);

   if (! residue_p) {
      std::cout << "residue not found for test_rotate_round_vector()" << std::endl;
   } else {
      int found_n_rotate_pts = 0;
      std::vector<int> exclude_atoms;
      clipper::Coord_orth rotate_pt_1;
      clipper::Coord_orth rotate_pt_2;
      PPCAtom residue_atoms;
      int n_residue_atoms;
      residue_p->GetAtomTable(residue_atoms, n_residue_atoms);
      for (int iat=0; iat<n_residue_atoms; iat++) {
	 std::string at_name(residue_atoms[iat]->name);
	 if (at_name == rotate_atom_1) {
	    found_n_rotate_pts++;
	    rotate_pt_1 = clipper::Coord_orth(residue_atoms[iat]->x,
					      residue_atoms[iat]->y,
					      residue_atoms[iat]->z);
	    exclude_atoms.push_back(iat);
	 } 
	 if (at_name == rotate_atom_2) {
	    found_n_rotate_pts++;
	    rotate_pt_2 = clipper::Coord_orth(residue_atoms[iat]->x,
					      residue_atoms[iat]->y,
					      residue_atoms[iat]->z);
	    exclude_atoms.push_back(iat);
	 } 
      }

      if (found_n_rotate_pts != 2) {
	 std::cout << "rotate atoms not found for test_rotate_round_vector()" << std::endl;
      } else {
	 bool correct_rotation = 1; // success initially.
	 for (int iat=0; iat<n_residue_atoms; iat++) {
	    bool exclude = 0;
	    for (unsigned int iex=0; iex<exclude_atoms.size(); iex++) {
	       if (iat == exclude_atoms[iex]) {
		  exclude = 1;
		  break;
	       }
	    }

	    // We want point D.  Then we can measure CDC' for all
	    // rotated atoms. (C' is where C moves to after rotation).
	    //
	    //  \A
	    //   \\                                  ~
	    //    \  \                               ~
	    //     \   \                             ~
	    //      \    \                           ~
	    //       \     \                         ~
	    //        \      \                       ~
	    //         \ b     \                     ~
	    //        B \-------- C
	    //           .      /
	    //            .   /
	    //              /
	    //            D .         Angle BDC is 90 degrees.
	    //               .        BC is the hypotenuse of the BCD triangle.
	    //                .

	    if (0) {
	       rotate_pt_1 = clipper::Coord_orth(-1.0, 0.0, 0.0);
	       rotate_pt_2 = clipper::Coord_orth( 0.0, 0.0, 0.0);
	    }
	    
	    if (! exclude) {
	       clipper::Coord_orth C_pt(residue_atoms[iat]->x,
					residue_atoms[iat]->y,
					residue_atoms[iat]->z);

	       clipper::Coord_orth ab(rotate_pt_2-rotate_pt_1);
	       clipper::Coord_orth ba(rotate_pt_1-rotate_pt_2);
	       clipper::Coord_orth bc(C_pt-rotate_pt_2);
	       clipper::Coord_orth ab_unit(ab.unit());
	       double bclen = clipper::Coord_orth::length(rotate_pt_2, C_pt);
	       double ablen = clipper::Coord_orth::length(rotate_pt_1, rotate_pt_2);
	       double cos_b = clipper::Coord_orth::dot(ba, bc)/(ablen*bclen);
	       double bdlen = bclen * cos(M_PI-acos(cos_b));
	       clipper::Coord_orth D_pt = rotate_pt_2 + bdlen * ab_unit;

	       // Print angle at BDC:
	       clipper::Coord_orth cd(D_pt - C_pt);
	       clipper::Coord_orth bd(D_pt - rotate_pt_2);
	       double bdlen2 = clipper::Coord_orth::length(D_pt, rotate_pt_2);
	       double cdlen = clipper::Coord_orth::length(D_pt, C_pt);

	       if (0) { 
		  std::cout << "   D: " << D_pt.format() << " BD "<< bd.format() << " bdlen: "
			    << bdlen << " " << bdlen2 << " " << bc.format() << " " << cdlen
			    << " " << clipper::Coord_orth::dot(cd, bd) << " -> "
			    << (180.0/M_PI)*acos(clipper::Coord_orth::dot(cd, bd)/(bdlen*cdlen))
			    << " degrees " << std::endl;
		  std::cout << "   ab " << ab.format() << std::endl;
		  std::cout << "   ab_unit " << ab_unit.format() << std::endl;
		  std::cout << "   bclen " << bclen << std::endl;
		  std::cout << "   ablen " << ablen << std::endl;
		  std::cout << "   dot prod " << clipper::Coord_orth::dot(ab, bc) << std::endl;
		  std::cout << "   cos_b " << cos_b << std::endl;
		  std::cout << "   b     " << acos(cos_b) << std::endl;
		  std::cout << "   bdlen " << bdlen << std::endl;
		  std::cout << "   D_pt " << D_pt.format() << std::endl;
		  // Make sure that D_pt does not move when rotated:
		  for (double a=0; a<7.0; a+=1.0) {
		     clipper::Coord_orth D_pt_r =  coot::util::rotate_round_vector(ab, D_pt, rotate_pt_2, a);
		     std::cout << "   " << a << " " << D_pt_r.format() << std::endl;
		  }
	       }

	       double test_angle = 20.0; // degrees

	       clipper::Coord_orth C_prime_pt =
		  coot::util::rotate_round_vector(ab, C_pt, rotate_pt_2, (M_PI/180.0)*test_angle);

	       clipper::Coord_orth dc(C_pt-D_pt);
	       clipper::Coord_orth dc_prime(C_prime_pt-D_pt);
	       double dc_len = clipper::Coord_orth::length(C_pt,D_pt);
	       double dc_prime_len = clipper::Coord_orth::length(C_prime_pt,D_pt);

	       if (0) { 
		  std::cout << "   dc       " << dc.format() << std::endl;
		  std::cout << "   dc_prime " << dc_prime.format() << std::endl;
		  std::cout << "   dc_len   " << dc_len << std::endl;
		  std::cout << "   dc_prime_len " << dc_prime_len << std::endl;
		  std::cout << "   dot_prod " << clipper::Coord_orth::dot(dc, dc_prime) << std::endl;
	       }
	       

	       double cos_theta = clipper::Coord_orth::dot(dc, dc_prime)/(dc_len * dc_prime_len);

	       if (0) { 

		  std::cout << "   AB.DC " << clipper::Coord_orth::dot(ab,dc);
		  std::cout << "   AB.DC'" << clipper::Coord_orth::dot(ab,dc_prime) << std::endl;
		  
		  std::cout << "   AB.DC  angle " << (180.0/M_PI)*clipper::Coord_orth::dot(ab,dc)/(ablen * dc_len);
		  std::cout << "   AB.DC' angle " << (180.0/M_PI)*clipper::Coord_orth::dot(ab,dc_prime)/(ablen * dc_prime_len);
		  std::cout << std::endl;
	       }
	       
	       std::cout << "   " << iat << " " << residue_atoms[iat]->name << " "
			 << cos_theta << " -> " << acos(cos_theta)*180.0/M_PI << " degrees" << std::endl;

	       double real_rotation_angle = acos(cos_theta)*180.0/M_PI;
	       if (! close_float_p(test_angle, real_rotation_angle))
		  correct_rotation = 0;
	       
	       residue_atoms[iat]->x = C_prime_pt.x();
	       residue_atoms[iat]->y = C_prime_pt.y();
	       residue_atoms[iat]->z = C_prime_pt.z();
	    }
	 }
	 atom_sel.mol->WritePDBASCII("3gp-rotated.pdb");
	 r = correct_rotation;
      }
   }

   return r;
}

// Is the angle that after_pos is rotated round the r_pt_1 and r_pt_2
// vector the same as test_angle?
// 
bool
test_rotate_atom_angle(const std::string &atom_name,
		       const clipper::Coord_orth &rotate_pt_1,
		       const clipper::Coord_orth &rotate_pt_2,
		       const clipper::Coord_orth &C_pt,
		       const clipper::Coord_orth &C_prime_pt,
		       double test_angle) {
   bool r = 0;

   clipper::Coord_orth ab(rotate_pt_2-rotate_pt_1);
   clipper::Coord_orth bc(C_pt-rotate_pt_2);
   clipper::Coord_orth ab_unit(ab.unit());
   double bclen = clipper::Coord_orth::length(rotate_pt_2, C_pt);
   double ablen = clipper::Coord_orth::length(rotate_pt_1, rotate_pt_2);
   double cos_b = clipper::Coord_orth::dot(ab, bc)/(ablen*bclen);
   double bdlen = bclen * cos(M_PI-acos(cos_b));
   clipper::Coord_orth D_pt = rotate_pt_2 - bdlen * ab_unit;

   // Print angle at BDC:
   clipper::Coord_orth cd(D_pt - C_pt);
   clipper::Coord_orth bd(D_pt - rotate_pt_2);
   double bdlen2 = clipper::Coord_orth::length(D_pt, rotate_pt_2);
   double cdlen = clipper::Coord_orth::length(D_pt, C_pt);
   clipper::Coord_orth dc(C_pt-D_pt);
   clipper::Coord_orth dc_prime(C_prime_pt-D_pt);
   double dc_len = clipper::Coord_orth::length(C_pt,D_pt);
   double dc_prime_len = clipper::Coord_orth::length(C_prime_pt,D_pt);
   double cos_theta = clipper::Coord_orth::dot(dc, dc_prime)/(dc_len * dc_prime_len);
   double real_angle = acos(cos_theta)*180.0/M_PI;
   std::cout << "  " << atom_name << " " << cos_theta << " -> "
	     << real_angle << " degrees" << std::endl;
   if (close_float_p(real_angle, test_angle)) { 
      r = 1;
   } else {
      std::cout << "   Ooops " << real_angle << " not close to " << test_angle << std::endl;
   } 
   return r;
} 



#endif // BUILT_IN_TESTING

