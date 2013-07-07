/* src/c-interface-ligands.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006, 2007 The University of York
 * Copyright 2008, 2009, 2010, 2011, 2012 by The University of Oxford
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifdef USE_PYTHON
#include <Python.h>  // before system includes to stop "POSIX_C_SOURCE" redefined problems
#endif

#include <stdlib.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#if defined _MSC_VER
#include <windows.h>
#endif
 
#include "globjects.h" //includes gtk/gtk.h

#include "callbacks.h"
#include "interface.h" // now that we are moving callback
		       // functionality to the file, we need this
		       // header since some of the callbacks call
		       // fuctions built by glade.
#include <vector>
#include <string>

#include <mmdb/mmdb_manager.h>
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "read-sm-cif.hh"

#include "graphics-info.h"
#include "c-interface.h"
#include "cc-interface.hh"
#include "coot-coord-utils.hh"
#include "peak-search.hh"

// #include "coot-compare-residues.hh"

#include "wligand.hh"

#include "guile-fixups.h"

#ifdef HAVE_GOOCANVAS
#include "goocanvas.h"
#include "wmolecule.hh"
#endif // HAVE_GOOCANVAS


/*! \brief centre on the ligand of the "active molecule", if we are
  already there, centre on the next hetgroup (etc) */
void go_to_ligand() {

   std::pair<bool, std::pair<int, coot::atom_spec_t> > pp = active_atom_spec();
   if (pp.first) {
      if (is_valid_model_molecule(pp.second.first)) {
	 clipper::Coord_orth rc(graphics_info_t::RotationCentre_x(),
				graphics_info_t::RotationCentre_y(),
				graphics_info_t::RotationCentre_z());
	 coot::new_centre_info_t new_centre =
	    graphics_info_t::molecules[pp.second.first].new_ligand_centre(rc, graphics_info_t::go_to_ligand_n_atoms_limit);
	 if (new_centre.type == coot::NORMAL_CASE) {
	    graphics_info_t g;
	    g.setRotationCentre(new_centre.position);
	    g.update_things_on_move_and_redraw();
	    std::string s = "Centred on residue ";
	    // s += new_centre.residue_spec;
	    s += coot::util::int_to_string(new_centre.residue_spec.resno);
	    s += new_centre.residue_spec.insertion_code;
	    s+= " ";
	    s += new_centre.residue_spec.chain;
	    s += " in molecule #";
	    s += coot::util::int_to_string(pp.second.first);
	    s += ".";
	    add_status_bar_text(s.c_str());
	 } else {
	    if (new_centre.type == coot::NO_LIGANDS) { 
	       std::string s = "No ligand (hetgroup) found in this molecule (#";
	       s += coot::util::int_to_string(pp.second.first);
	       s += ").";
	       add_status_bar_text(s.c_str());
	    }
	    if (new_centre.type == coot::SINGLE_LIGAND_NO_MOVEMENT) { 
	       std::string s = "This ligand (";
	       s += coot::util::int_to_string(new_centre.residue_spec.resno);
	       s += new_centre.residue_spec.insertion_code;
	       s += new_centre.residue_spec.chain;
	       s += ") ";
	       s += " is the only ligand in this molecule (#";
	       s += coot::util::int_to_string(pp.second.first);
	       s += ").";
	       add_status_bar_text(s.c_str());
	    }
	 } 
      } 
   } 
}

void set_go_to_ligand_n_atoms_limit(int n_atoms_min) {
   graphics_info_t::go_to_ligand_n_atoms_limit = n_atoms_min;
} 




/*  ----------------------------------------------------------------------- */
/*                  ligand overlay                                          */
/*  ----------------------------------------------------------------------- */
/*! \brief "Template"-based matching.  Overlap the first residue in
  imol_ligand onto the residue specified by the reference parameters.
  Use graph matching, not atom names.  */

#ifdef USE_GUILE
SCM
overlap_ligands(int imol_ligand, int imol_ref, const char *chain_id_ref,
		int resno_ref) {

   SCM scm_status = SCM_BOOL_F;
   coot::graph_match_info_t rtop_info =
      overlap_ligands_internal(imol_ligand, imol_ref, chain_id_ref, resno_ref, 1);

   if (rtop_info.success) { 
      SCM match_info = scm_cons(scm_int2num(rtop_info.n_match), SCM_EOL);
      match_info = scm_cons(scm_double2num(rtop_info.dist_score), match_info);
      SCM s = scm_cons(match_info, SCM_EOL);
      scm_status = scm_cons(rtop_to_scm(rtop_info.rtop), s);
   }
   return scm_status;
}
#endif

// should be in utils?
void
match_ligand_torsions(int imol_ligand, int imol_ref, const char *chain_id_ref, int resno_ref) {

   // 20110608 I don't think that this function should make
   // restraints.  Let's get restraints outside and test for their
   // existance before we run this function.

   if (!is_valid_model_molecule(imol_ligand)) {
      std::cout << "WARNING molecule number " << imol_ligand << " is not a valid model molecule"
		<< std::endl;
   } else {
      if (! is_valid_model_molecule(imol_ref)) {
	 std::cout << "WARNING molecule number " << imol_ref << " is not a valid model molecule"
		   << std::endl;
      } else { 

	 if (is_valid_model_molecule(imol_ref)) { 
	    graphics_info_t g;
	    CMMDBManager *mol_ref = g.molecules[imol_ref].atom_sel.mol;
	    CResidue *res_ref = coot::util::get_residue(chain_id_ref,
							resno_ref, "",
							mol_ref);
	    if (res_ref) {
	    
	       std::string res_name_ref_res(res_ref->GetResName());
	       std::pair<bool, coot::dictionary_residue_restraints_t> restraints_info = 
		  g.Geom_p()->get_monomer_restraints(res_name_ref_res);
	       if (restraints_info.first) {
		  std::vector <coot::dict_torsion_restraint_t> tr = 
		     g.Geom_p()->get_monomer_torsions_from_geometry(res_name_ref_res, 0);

		  if (tr.size() == 0) {
		     std::cout << "WARNING:: No torsion restraints from PRODRG"
			       << std::endl;
		  } else {
		     // normal case
		     int n_rotated = g.molecules[imol_ligand].match_torsions(res_ref, tr, *g.Geom_p());
		     std::cout << "INFO:: rotated " << n_rotated << " torsions in matching torsions"
			       << std::endl;
		  }
	       }
	       graphics_draw();
	    }
	 }
      }
   }

}


#ifdef USE_PYTHON
PyObject *overlap_ligands_py(int imol_ligand, int imol_ref, const char *chain_id_ref,
		int resno_ref) {

   PyObject *python_status = Py_False;
   coot::graph_match_info_t rtop_info =
      overlap_ligands_internal(imol_ligand, imol_ref, chain_id_ref, resno_ref, 1);

   if (rtop_info.success) { 
      PyObject *match_info = PyList_New(2);
      PyList_SetItem(match_info, 0, PyFloat_FromDouble(rtop_info.dist_score));
      PyList_SetItem(match_info, 1, PyInt_FromLong(rtop_info.n_match));
      python_status = PyList_New(2);
      PyList_SetItem(python_status, 0, rtop_to_python(rtop_info.rtop)); 
      PyList_SetItem(python_status, 1, match_info);
   }
   if (PyBool_Check(python_status)) {
     Py_INCREF(python_status);
   }
   return python_status;
}
#endif // USE_PYTHON

#ifdef USE_GUILE
SCM
analyse_ligand_differences(int imol_ligand, int imol_ref, const char *chain_id_ref,
			   int resno_ref) {

   SCM scm_status = SCM_BOOL_F;
   coot::graph_match_info_t rtop_info =
      overlap_ligands_internal(imol_ligand, imol_ref, chain_id_ref, resno_ref, 0);

   std::cout << "analyse_ligand_differences: success       " << rtop_info.success << std::endl;
   std::cout << "analyse_ligand_differences: n_match       " << rtop_info.n_match << std::endl;
   std::cout << "analyse_ligand_differences: dist_score    " << rtop_info.dist_score << std::endl;
   std::cout << "analyse_ligand_differences: atoms matched " << rtop_info.matching_atom_names.size() << std::endl;
   std::cout << "analyse_ligand_differences: rtop: \n" << rtop_info.rtop.format() << std::endl;
   
   if (rtop_info.success) {
      SCM match_info = scm_cons(scm_int2num(rtop_info.n_match), SCM_EOL);
      match_info = scm_cons(scm_double2num(rtop_info.dist_score), match_info);
      SCM s = scm_cons(match_info, SCM_EOL);
      scm_status = scm_cons(rtop_to_scm(rtop_info.rtop), s);
   }
   return scm_status;
}
#endif   

#ifdef USE_PYTHON
PyObject *analyse_ligand_differences_py(int imol_ligand, int imol_ref, const char *chain_id_ref,
			   int resno_ref) {

   PyObject *python_status;
   python_status = Py_False;
   coot::graph_match_info_t rtop_info =
      overlap_ligands_internal(imol_ligand, imol_ref, chain_id_ref, resno_ref, 0);

   std::cout << "analyse_ligand_differences: success       " << rtop_info.success << std::endl;
   std::cout << "analyse_ligand_differences: n_match       " << rtop_info.n_match << std::endl;
   std::cout << "analyse_ligand_differences: dist_score    " << rtop_info.dist_score << std::endl;
   std::cout << "analyse_ligand_differences: atoms matched " << rtop_info.matching_atom_names.size() << std::endl;
   std::cout << "analyse_ligand_differences: rtop: \n" << rtop_info.rtop.format() << std::endl;
   
   if (rtop_info.success) {
      PyObject *match_info;
      match_info = PyList_New(2);
      PyList_SetItem(match_info, 0, PyFloat_FromDouble(rtop_info.dist_score));
      PyList_SetItem(match_info, 1, PyInt_FromLong(rtop_info.n_match));
      python_status = PyList_New(2);
      PyList_SetItem(python_status, 0, rtop_to_python(rtop_info.rtop));
      PyList_SetItem(python_status, 1, match_info);
   }
   if (PyBool_Check(python_status)) {
     Py_INCREF(python_status);
   }
   return python_status;
}
#endif //USE_PYTHON



#ifdef USE_GUILE
// For testing that pyrogen generates consistent atom types.  Not
// useful for general public.
// 
SCM
compare_ligand_atom_types_scm(int imol_ligand, int imol_ref, const char *chain_id_ref,
			      int resno_ref) {

   SCM scm_status = SCM_BOOL_F;
   bool match_hydrogens_also = false;
   bool apply_rtop_flag = true;

   if (! is_valid_model_molecule(imol_ligand)) {
      std::cout << "WARNING:: not a valid model molecule (ligand) " << imol_ligand << std::endl;
   } else { 
      if (! is_valid_model_molecule(imol_ref)) {
	 std::cout << "WARNING:: not a valid model molecule (ref) " << imol_ligand << std::endl;
      } else { 

	 graphics_info_t g;
	 CResidue *res_ref = g.molecules[imol_ref].get_residue(chain_id_ref, resno_ref, "");
	 CResidue *res_mov = g.molecules[imol_ligand].get_first_residue();

	 if (! res_ref) {
	    std::cout << "WARNING failed to find reference residue" << std::endl;
	 } else { 
	    if (! res_mov) {
	       std::cout << "WARNING failed to find moving residue" << std::endl;
	    } else {
   
	       coot::graph_match_info_t rtop_info =
		  coot::graph_match(res_mov, res_ref, apply_rtop_flag, match_hydrogens_also);

	       // work atom-names then ref atom-names
	       std::vector<std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string> > > matches = rtop_info.matching_atom_names;

	       std::cout << "found " << matches.size() << " graph matched atoms" << std::endl;
	       std::string rn1 = res_mov->GetResName();
	       std::string rn2 = res_ref->GetResName();

	       if (matches.size()) {
		  int n_fail = 0;

		  for (unsigned int i=0; i<matches.size(); i++) { 
		     std::string energy_type_1 = g.Geom_p()->get_type_energy(matches[i].first.first,  rn1);
		     std::string energy_type_2 = g.Geom_p()->get_type_energy(matches[i].second.first, rn2);

		     std::string extra_space = "";
		     if (i<10) extra_space = " ";
		     std::cout << "   " << extra_space << i << " names: \""
			       << matches[i].second.first << "\" \""
			       << matches[i].first.first << "\" ->  "
			       << "\"" << energy_type_2 << "\"  and  \"" << energy_type_1 << "\"";
		     if (energy_type_1 != energy_type_2) { 
			std::cout << "   #### fail " << std::endl;
			n_fail++;
		     } else { 
			std::cout << std::endl;
		     }
		  }
		  scm_status = SCM_MAKINUM(n_fail);
	       }
	    }
	 }
      }
   }
   return scm_status;
   
}
#endif   

coot::graph_match_info_t
overlap_ligands_internal(int imol_ligand, int imol_ref, const char *chain_id_ref,
			 int resno_ref, bool apply_rtop_flag) {

   coot::graph_match_info_t graph_info;
   
   CResidue *residue_moving = 0;
   CResidue *residue_reference = 0;

   // running best ligands:
   CResidue *best_residue_moving = NULL;
   double best_score = 99999999.9; // low score good.
   clipper::RTop_orth best_rtop;
   

   if (! is_valid_model_molecule(imol_ligand))
      return graph_info;

   if (! is_valid_model_molecule(imol_ref))
      return graph_info;

   CMMDBManager *mol_moving = graphics_info_t::molecules[imol_ligand].atom_sel.mol;
   CMMDBManager *mol_ref    = graphics_info_t::molecules[imol_ref].atom_sel.mol;
   
   for (int imod=1; imod<=mol_moving->GetNumberOfModels(); imod++) { 
      CModel *model_p = mol_moving->GetModel(imod);
      CChain *chain_p;
      int nchains = model_p->GetNumberOfChains();
      for (int ichain=0; ichain<nchains; ichain++) {
	 chain_p = model_p->GetChain(ichain);
	 int nres = chain_p->GetNumberOfResidues();
	 PCResidue residue_p;
	 for (int ires=0; ires<nres; ires++) { 
	    residue_p = chain_p->GetResidue(ires);
	    if (residue_p) { 
	       int n_atoms = residue_p->GetNumberOfAtoms();
	       if (n_atoms > 0) {
		  residue_moving = residue_p;
		  break;
	       }
	    }
	 }
	 if (residue_moving)
	    break;
      }

      if (! residue_moving) {
	 std::cout << "Oops.  Failed to find moving residue" << std::endl;
      } else { 
	 int imodel_ref = 1;
	 CModel *model_ref_p = mol_ref->GetModel(imodel_ref);
	 CChain *chain_p;
	 int nchains = model_ref_p->GetNumberOfChains();
	 for (int ichain=0; ichain<nchains; ichain++) {
	    chain_p = model_ref_p->GetChain(ichain);
	    if (std::string(chain_p->GetChainID()) == std::string(chain_id_ref)) { 
	       int nres = chain_p->GetNumberOfResidues();
	       PCResidue residue_p;
	       for (int ires=0; ires<nres; ires++) { 
		  residue_p = chain_p->GetResidue(ires);
		  if (residue_p) {
		     int seqnum = residue_p->GetSeqNum();
		     if (seqnum == resno_ref) {
			residue_reference = residue_p;
			break;
		     }
		  }
	       }
	       if (residue_reference)
		  break;
	    }
	 }

	 if (!residue_reference) {
	    std::cout << "Oops.  Failed to find reference residue" << std::endl;
	 } else {
	    bool match_hydrogens_also = 0;
	    coot::graph_match_info_t rtop_info =
	       coot::graph_match(residue_moving, residue_reference, apply_rtop_flag, match_hydrogens_also);
	    if (rtop_info.success) {
	       if (rtop_info.dist_score < best_score) { // low score good.
		  best_score = rtop_info.dist_score;
		  best_residue_moving = residue_moving;
		  best_rtop = rtop_info.rtop;
		  graph_info = rtop_info;
	       }
	    } else {
	       // std::cout << "Oops.  Match failed somehow" << std::endl;
	    }
	 }
      }
   }
   if (apply_rtop_flag) { 
      if (best_residue_moving) {
	 // move just the best ligand:
	 graphics_info_t::molecules[imol_ligand].transform_by(best_rtop, best_residue_moving);
	 // delete everything except the best ligand:
	 graphics_info_t::molecules[imol_ligand].delete_all_except_res(best_residue_moving);
	 graphics_draw();
      }
   }
   return graph_info;
}





void ligand_expert() {  /* sets the flag to have expert option ligand entries in the GUI */

   graphics_info_t::ligand_expert_flag = 1;
}

/*! \brief set the protein molecule for ligand searching */
void set_ligand_search_protein_molecule(int imol) {
   graphics_info_t::set_ligand_protein_mol(imol);
   
}

/*! \brief set the map molecule for ligand searching */
void set_ligand_search_map_molecule(int imol_map) {
   graphics_info_t::set_ligand_map_mol(imol_map);
}

/*! \brief add a ligand molecule to the list of ligands to search for
  in ligand searching */
void add_ligand_search_wiggly_ligand_molecule(int imol_ligand) {
   graphics_info_t::find_ligand_add_flexible_ligand(imol_ligand);
}

/*! \brief add a ligand molecule to the list of ligands to search for
  in ligand searching */
void add_ligand_search_ligand_molecule(int imol_ligand) {
   if (is_valid_model_molecule(imol_ligand))
      graphics_info_t::find_ligand_add_rigid_ligand(imol_ligand);
}

void add_ligand_clear_ligands() {

   graphics_info_t g;
   g.find_ligand_clear_ligand_mols();
} 




// execute_find_ligands_real, you might say
// 
#ifdef USE_GUILE
SCM execute_ligand_search() {

   std::vector<int> solutions = execute_ligand_search_internal();
   return generic_int_vector_to_list_internal(solutions);
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *execute_ligand_search_py() {

   std::vector<int> solutions = execute_ligand_search_internal();
   return generic_int_vector_to_list_internal_py(solutions);
}
#endif // USE_PYTHON

std::vector<int>
execute_ligand_search_internal() {
   
   std::cout << "Executing ligand search..." << std::endl;
   std::vector<int> solutions;

   graphics_info_t g;

   if (! is_valid_model_molecule(g.find_ligand_protein_mol())) {
      std::cout << "Protein molecule for ligand search not set" << std::endl;
      std::cout << "Aborting ligand search" << std::endl;
      return solutions; 
   }
   if (! is_valid_map_molecule(g.find_ligand_map_mol())) {
      std::cout << "Map molecule for ligand search not set" << std::endl;
      std::cout << "Aborting ligand search" << std::endl;
      return solutions; 
   }
   if (g.find_ligand_ligand_mols().size() == 0) {
      std::cout << "No defined ligand molecules" << std::endl;
      std::cout << "Aborting ligand search" << std::endl;
      return solutions; 
   } 
   
   CMMDBManager *protein_mol = 
      g.molecules[g.find_ligand_protein_mol()].atom_sel.mol;

   coot::wligand wlig;
   if (g.ligand_verbose_reporting_flag) { 
      wlig.set_verbose_reporting();
      // debugging, output the post-conformer generation ligands wligand-*.pdb
      // (but pre-idealized).
      wlig.set_debug_wiggly_ligands();
   } 
   wlig.import_map_from(g.molecules[g.find_ligand_map_mol()].xmap_list[0]);
   std::vector<std::pair<int, bool> > ligands = g.find_ligand_ligand_mols();

   for(unsigned int i=0; i<ligands.size(); i++) {

      std::cout << "INFO:: ligand number " << i << " is molecule number "
		<< g.find_ligand_ligand_mols()[i].first << "  " 
		<< " with wiggly flag: "
		<< g.find_ligand_ligand_mols()[i].second << std::endl;

      if (ligands[i].second) {
	 // argh (i).
	 coot::minimol::molecule mmol(g.molecules[ligands[i].first].atom_sel.mol);

//	 for(unsigned int ifrag=0; ifrag<mmol.fragments.size(); ifrag++) {
// 	    for (int ires=mmol[ifrag].min_res_no(); ires<=mmol[ifrag].max_residue_number();
//		 ires++) {
// 	       if (mmol[ifrag][ires].n_atoms() > 0) {
// 		  std::cout << "DEBUG:: in execute_ligand_search:  mmol["
// 			    << ifrag << "][" << ires << "].name :"
// 			    <<  mmol[ifrag][ires].name << ":" << std::endl;
// 	       }
//	    }
//	 }

	 // std::pair<short int, std::string> istat_pair =
	 try { 
	    bool optim_geom = true;
	    bool fill_vec = false;
	    wlig.install_simple_wiggly_ligands(g.Geom_p(), mmol,
					       g.ligand_wiggly_ligand_n_samples,
					       optim_geom, fill_vec);
	 }
	 catch (std::runtime_error mess) {
	    std::cout << "Error in flexible ligand definition.\n";
	    std::cout << mess.what() << std::endl;
	    if (graphics_info_t::use_graphics_interface_flag) { 
	       GtkWidget *w = wrapped_nothing_bad_dialog(mess.what());
	       gtk_widget_show(w);
	    }
	    return solutions;
	 }
      } else { 
	 // argh (ii).
	 wlig.install_ligand(g.molecules[ligands[i].first].atom_sel.mol);
      }
   }


   short int mask_waters_flag; // treat waters like other atoms?
   mask_waters_flag = g.find_ligand_mask_waters_flag;
   if (! g.find_ligand_here_cluster_flag) { 
      int imol = graphics_info_t::create_molecule();
      if (graphics_info_t::map_mask_atom_radius > 0) {
	 // only do this if it was set by the user.
	 wlig.set_map_atom_mask_radius(graphics_info_t::map_mask_atom_radius);
      } else { 
	 wlig.set_map_atom_mask_radius(2.0);  // Angstroms
      } 
      
      std::string name("ligand masked map");
      // std::cout << "DEBUG:: calling mask_map\n";
      wlig.mask_map(protein_mol, mask_waters_flag); // mask by protein
      // std::cout << "DEBUG:: done mask_map\n";
      g.molecules[imol].new_map(wlig.masked_map(), wlig.masked_map_name());

      // This should not be be scroll map?
      if (0) { 
	 g.scroll_wheel_map = imol;  // change the current scrollable map to
                                     // the masked map.
      }
      wlig.find_clusters(g.ligand_cluster_sigma_level);  // trashes the xmap
      wlig.set_acceptable_fit_fraction(g.ligand_acceptable_fit_fraction);
      wlig.fit_ligands_to_clusters(g.find_ligand_n_top_ligands); // 10 clusters

   } else {

      // don't search the map, just use the peak/cluser at the screen
      // centre.

      wlig.mask_map(protein_mol, mask_waters_flag);
      // wlig.set_acceptable_fit_fraction(g.ligand_acceptable_fit_fraction);
      
      clipper::Coord_orth pt(g.X(), g.Y(), g.Z()); // close to 3GP peak (not in it).
      float n_sigma = g.ligand_cluster_sigma_level; // cluster points must be more than this.
      wlig.cluster_from_point(pt, n_sigma);
      wlig.fit_ligands_to_clusters(1); // just this cluster.
      
   }
   wlig.make_pseudo_atoms(); // put anisotropic atoms at the ligand sites

   // now add in the solution ligands:
   int n_clusters = wlig.n_clusters_final();

   int n_new_ligand = 0;
   coot::minimol::molecule m;
   for (unsigned int iclust=0; iclust<n_clusters; iclust++) {

      // frac_lim is the fraction of the score of the best solutions
      // that we should consider as solutions. 0.5 is generous, I
      // think.
      float frac_lim = g.find_ligand_score_by_correl_frac_limit; // 0.7;
      float correl_frac_lim = 0.9;
      // nino-mode
      unsigned int nlc = wlig.n_ligands_for_cluster(iclust, frac_lim);
      wlig.score_and_resort_using_correlation(iclust, nlc);

      // false is the default case
      if (g.find_ligand_multiple_solutions_per_cluster_flag == false) {
	 nlc = 1;
	 correl_frac_lim = 0.975;
      }

      if (nlc > 12) nlc = 12; // arbitrary limit of max 12 solutions per cluster
      float tolerance = 20.0;
      wlig.limit_solutions(iclust, correl_frac_lim, nlc, tolerance, true);
			   
      for (unsigned int isol=0; isol<nlc; isol++) { 

	 m = wlig.get_solution(isol, iclust);
	 if (! m.is_empty()) {
	    float bf = graphics_info_t::default_new_atoms_b_factor;
	    CMMDBManager *ligand_mol = m.pcmmdbmanager();
	    coot::hetify_residues_as_needed(ligand_mol);
	    atom_selection_container_t asc = make_asc(ligand_mol);
	    int g_mol = graphics_info_t::create_molecule();
	    std::string label = "Fitted ligand #";
	    label += g.int_to_string(iclust);
	    label += "-";
	    label += g.int_to_string(isol);
	    g.molecules[g_mol].install_model(g_mol, asc, label, 1);
	    g.molecules[g_mol].assign_hetatms();
	    solutions.push_back(g_mol);
	    n_new_ligand++;
	    if (g.go_to_atom_window){
	       g.update_go_to_atom_window_on_new_mol();
	       g.update_go_to_atom_window_on_changed_mol(g_mol);
	    }
	 }
      }
   }

   if (graphics_info_t::use_graphics_interface_flag) {
      if (n_new_ligand) {
	 
   // We need some python code here to match post-ligand-fit-gui
#if defined USE_GUILE && !defined WINDOWS_MINGW
	 safe_scheme_command("(post-ligand-fit-gui)");
#else
   // BL says:: guess we shall do it for python too (done it...)
#ifdef USE_PYGTK
	 safe_python_command("post_ligand_fit_gui()");
#endif // USE_PYGTK
#endif // USE_GUILE

	 GtkWidget *w = create_new_ligands_info_dialog();
	 GtkWidget *label = lookup_widget(w, "new_ligands_info_dialog_label");
	 std::string label_str("  Found ");
	 label_str += graphics_info_t::int_to_string(n_new_ligand);
	 if (n_new_ligand == 1) 
	    label_str += " acceptable ligand  ";
	 else 
	    label_str += " acceptable ligands  ";
	 gtk_label_set_text(GTK_LABEL(label), label_str.c_str());
	 gtk_widget_show(w);
      } else { 
	 GtkWidget *w = create_no_new_ligands_info_dialog();
	 gtk_widget_show(w);
      }
   }

   graphics_draw();
   return solutions;
}


std::vector<int> ligand_search_make_conformers_internal() { 

   std::vector<int> mol_list;
   graphics_info_t g;
   coot::wligand wlig;
   if (g.ligand_verbose_reporting_flag)
      wlig.set_verbose_reporting();
   std::vector<std::pair<int, bool> > ligands = g.find_ligand_ligand_mols();
   for(unsigned int i=0; i<ligands.size(); i++) {
      if (ligands[i].second) {
	 try { 
	    bool optim_geom = true;
	    bool fill_return_vec = false; // would give input molecules (not conformers)
	    coot::minimol::molecule mmol(g.molecules[ligands[i].first].atom_sel.mol);
	    wlig.install_simple_wiggly_ligands(g.Geom_p(), mmol,
					       g.ligand_wiggly_ligand_n_samples,
					       optim_geom, fill_return_vec);
	 }
	 catch (std::runtime_error mess) {
	    std::cout << "Error in flexible ligand definition.\n";
	    std::cout << mess.what() << std::endl;
	    if (graphics_info_t::use_graphics_interface_flag) { 
	       GtkWidget *w = wrapped_nothing_bad_dialog(mess.what());
	       gtk_widget_show(w);
	    }
	 }
      }
   }

   std::vector<coot::minimol::molecule> conformers = wlig.get_conformers();
   for (unsigned int iconf=0; iconf<conformers.size(); iconf++) {
      atom_selection_container_t asc = make_asc(conformers[iconf].pcmmdbmanager());
      int g_mol = graphics_info_t::create_molecule();
      std::string label = "conformer-";
      label+= coot::util::int_to_string(iconf);
      graphics_info_t::molecules[g_mol].install_model(g_mol, asc, label, 1);
      mol_list.push_back(g_mol);
   }

   return mol_list;
} 

/*! \brief make conformers of the ligand search molecules, each in its
  own molecule.  

  Don't search the density. */
#ifdef USE_GUILE
SCM
ligand_search_make_conformers_scm() {

   std::vector<int> v = ligand_search_make_conformers_internal();
   return generic_int_vector_to_list_internal(v);
}
#endif // USE_GUILE

#ifdef USE_PYTHON
PyObject *ligand_search_make_conformers_py() {

   std::vector<int> v = ligand_search_make_conformers_internal();
   return generic_int_vector_to_list_internal_py(v);
} 
#endif // USE_PYTHON

void set_ligand_cluster_sigma_level(float f) { /* default 2.2 */
   graphics_info_t::ligand_cluster_sigma_level = f;
}

void set_find_ligand_mask_waters(int i) {

   graphics_info_t::find_ligand_mask_waters_flag = i;

}

/*! \brief set the atom radius for map masking */
void set_map_mask_atom_radius(float rad) {
   graphics_info_t::map_mask_atom_radius = rad; 
}

/*! \brief get the atom radius for map masking, -99 is initial "unset" */
float map_mask_atom_radius() {
   return graphics_info_t::map_mask_atom_radius;
}


void set_ligand_flexible_ligand_n_samples(int i) {
   graphics_info_t::ligand_wiggly_ligand_n_samples = i;
} 


void set_ligand_acceptable_fit_fraction(float f) {
   if (f >= 0.0 && f<= 1.0) {
      graphics_info_t::ligand_acceptable_fit_fraction = f;
      // std::cout << "ligand acceptable fit fraction set to " << f << "\n";
   } else {
      // std::cout << "ligand acceptable fit fraction " << f << " ignored\n";
   }
}

void set_find_ligand_n_top_ligands(int n) { /* fit the top n ligands,
					      not all of them, default
					      10. */
   graphics_info_t g;
   g.find_ligand_n_top_ligands = n;

}

void set_find_ligand_multi_solutions_per_cluster(float lim_1, float lim_2) {
   graphics_info_t g;
   g.find_ligand_multiple_solutions_per_cluster_flag = true;
   g.find_ligand_score_by_correl_frac_limit = lim_1;
   g.find_ligand_score_correl_frac_interesting_limit = lim_2;
}


/* flip the ligand (usually active residue) around its eigen vectoros
   to the next flip number.  Immediate replacement (like flip
   peptide). */
void flip_ligand(int imol, const char *chain_id, int resno) {

   // note that if the ligand's current flip_number is not 0, then we
   // need to undo a the current flip number before applying the next
   // one.  If the new flip_number is 0, we just undo of course.
   if (is_valid_model_molecule(imol)) {
      coot::minimol::molecule m = 
	 graphics_info_t::molecules[imol].eigen_flip_residue(chain_id, resno);

      if (0) { 
	 atom_selection_container_t asc = make_asc(m.pcmmdbmanager());
	 int g_mol = graphics_info_t::create_molecule();
	 std::string label = "flipping residue";
	 graphics_info_t::molecules[g_mol].install_model(g_mol, asc, label, 1);
      }
   }
   graphics_draw();
}



/*  ----------------------------------------------------------------------- */
/*                  Mask                                                    */
/*  ----------------------------------------------------------------------- */
/* The idea is to generate a new map that has been masked by some
   coordinates. */
/*!     (mask-map-by-molecule map-no mol-no invert?)  creates and
        displays a masked map, cuts down density where the coordinates
        are not.  If invert? is #t, cut the density down where the
        atoms are.  */
int mask_map_by_molecule(int map_mol_no, int coord_mol_no, short int invert_flag) {

   int imol_new_map = -1;

   // create a new map
   //
   // c.f. a new map that is created "Masked by Protein"
   // we use molecule_class_info_t's new_map()
   //

   // Where should the bulk of this function be?  Let's put it here for now.
   coot::ligand lig;
   graphics_info_t g;
   if (map_mol_no >= g.n_molecules()) {
      std::cout << "No such molecule (no map) at molecule number " << map_mol_no << std::endl;
   } else {

      if (coord_mol_no >= g.n_molecules()) {
	 std::cout << "No such molecule (no coords) at molecule number " << map_mol_no << std::endl;
      } else { 
      
	 if (!g.molecules[map_mol_no].has_map()) {
	    std::cout << "No map in molecule number " << map_mol_no << std::endl;
	 } else {
	    
	    if (! g.molecules[coord_mol_no].has_model()) {
	       std::cout << "No model in molecule number " << map_mol_no << std::endl;
	    } else {
	       short int mask_waters_flag; // treat the waters like protein atoms?
	       mask_waters_flag = graphics_info_t::find_ligand_mask_waters_flag;
	       lig.import_map_from(g.molecules[map_mol_no].xmap_list[0]);
	       int selectionhandle = g.molecules[coord_mol_no].atom_sel.mol->NewSelection();

	       if (graphics_info_t::map_mask_atom_radius > 0) {
		  lig.set_map_atom_mask_radius(graphics_info_t::map_mask_atom_radius);
	       }

	       // make a selection:
	       std::string rnames = "*";
	       if (!mask_waters_flag)
		  rnames = "!HOH,WAT"; // treat waters differently to regular atoms.
	       g.molecules[coord_mol_no].atom_sel.mol->SelectAtoms(selectionhandle, 0, "*",
								   ANY_RES, "*",
								   ANY_RES, "*",
								   (char *) rnames.c_str(),
								   "*", "*", "*");
	       
	       lig.mask_map(g.molecules[coord_mol_no].atom_sel.mol, selectionhandle, invert_flag);
	       g.molecules[coord_mol_no].atom_sel.mol->DeleteSelection(selectionhandle);
	       imol_new_map = graphics_info_t::create_molecule();
	       std::cout << "INFO:: Creating masked  map in molecule number "
			 << imol_new_map << std::endl;
	       g.molecules[imol_new_map].new_map(lig.masked_map(), "Generic Masked Map");
	       graphics_draw();
	    }
	 }
      }
   }
   return imol_new_map; 
}

int
mask_map_by_atom_selection(int map_mol_no, int coords_mol_no, const char *mmdb_atom_selection, short int invert_flag) {

   int imol_new_map = -1;
   graphics_info_t g;
   if (is_valid_map_molecule(map_mol_no)) {
      if (is_valid_model_molecule(coords_mol_no)) {
	 // std::cout << "making lig..." << std::endl;
	 coot::ligand lig;
	 lig.import_map_from(g.molecules[map_mol_no].xmap_list[0]);

	 if (graphics_info_t::map_mask_atom_radius > 0) {
	    lig.set_map_atom_mask_radius(graphics_info_t::map_mask_atom_radius);
	 }
	 int selectionhandle = g.molecules[coords_mol_no].atom_sel.mol->NewSelection();
	 g.molecules[coords_mol_no].atom_sel.mol->Select(selectionhandle, STYPE_ATOM,
							 (char *) mmdb_atom_selection,
							 SKEY_NEW);
	 lig.mask_map(g.molecules[coords_mol_no].atom_sel.mol, selectionhandle, invert_flag);
	 imol_new_map = graphics_info_t::create_molecule();
	 g.molecules[imol_new_map].new_map(lig.masked_map(), "Generic Masked Map");
	 graphics_draw();
      } else {
	 std::cout << "No model molecule in " << coords_mol_no << std::endl;
      }
   } else {
      std::cout << "No map molecule in " << map_mol_no << std::endl;
   } 
   return imol_new_map; 
}




void do_find_ligands_dialog() {
   GtkWidget *dialog;
   int istate;
   dialog = create_find_ligand_dialog();
   istate = fill_ligands_dialog(dialog); /* return OK, we have map(s), ligand(s), masking(s) */
   if (istate == 0) {
      gtk_widget_destroy(dialog);
      std::string s("Problem finding maps, coords or ligands!");
      graphics_info_t g;
      g.add_status_bar_text(s);
      std::cout << s << std::endl;
   }
   else 
     gtk_widget_show(dialog); 

}

/*  ----------------------------------------------------------------------- */
/*                  monomer lib                                             */
/*  ----------------------------------------------------------------------- */
std::vector<std::pair<std::string, std::string> > monomer_lib_3_letter_codes_matching(const std::string &search_string, short int allow_minimal_descriptions_flag) {

   graphics_info_t g;
   std::vector<std::pair<std::string, std::string > > v = g.Geom_p()->matching_names(search_string, allow_minimal_descriptions_flag);

   // search the list of monomers for text search_text:

   return v;
} 


// we allocate new memory here, without ever giving it back.  The
// memory should be freed when the dialog is destroyed.
// 
int
handle_make_monomer_search(const char *text, GtkWidget *viewport) {

   int stat = 0;
   bool use_sbase_molecules = 0;
   std::string t(text);

   GtkWidget *vbox_current = lookup_widget(viewport, "monomer_search_results_vbox");
   GtkWidget *checkbutton =
      lookup_widget(viewport, "monomer_search_minimal_descriptions_checkbutton");
   GtkWidget *use_sbase_checkbutton =
      lookup_widget(viewport, "monomer_search_sbase_molecules_checkbutton");
   
   short int allow_minimal_descriptions_flag = 0;
   GtkWidget *dialog = lookup_widget(viewport, "monomer_search_dialog");

   if (GTK_TOGGLE_BUTTON(checkbutton)->active)
      allow_minimal_descriptions_flag = 1;

   if (GTK_TOGGLE_BUTTON(use_sbase_checkbutton)->active)
      use_sbase_molecules = 1;

   graphics_info_t g;
   std::vector<std::pair<std::string, std::string> > v;
   if (! use_sbase_molecules) 
      v = monomer_lib_3_letter_codes_matching(t, allow_minimal_descriptions_flag);
   else 
      v = g.Geom_p()->matching_ccp4srs_residues_names(t);

   std::cout << "DEBUG::  " << use_sbase_molecules
	     << " found " << v.size() << " matching molecules "
	     << " using string :" << t << ":" 
	     << std::endl;

   // std::cout << "DEBUG:: " << v.size() << " solutions matching" << std::endl;

   // here clear the current contents of the monomer vbox:
   // delete the user_data assocated with the buttons too.
    GList *children = gtk_container_children(GTK_CONTAINER(vbox_current));
    int nchild = 0; 
    while (children) {
       // std::cout << "child " << nchild << "  " << (GtkWidget *) children->data << std::endl;
       gtk_widget_destroy((GtkWidget *) children->data); 
       nchild++;
       children = g_list_remove_link(children, children);
       
    }

   GtkWidget *vbox = vbox_current;

   // std::cout << "DEBUG:: monomers v.size() " << v.size() << std::endl;
   // add new buttons
   for (unsigned int i=0; i<v.size(); i++) {
      // std::cout << i << " " << v[i].first << std::endl;
      std::string l = v[i].first;
      l += " : ";
      l += v[i].second;
      // std::cout << "Giving the button the label :" << l << ":" << std::endl;
      GtkWidget *button = gtk_button_new_with_label(l.c_str());
      // std::cout << "Adding button: " << button << std::endl;
      std::string button_name = "monomer_button_";

      // gets embedded as user data (hmm).
      string *s = new string(v[i].first); // the 3-letter-code/comp_id (for user data).
      button_name += v[i].first;
      gtk_widget_ref (button);
      gtk_object_set_data_full (GTK_OBJECT (dialog), 
				button_name.c_str(), button,
				(GtkDestroyNotify) gtk_widget_unref);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (button), 2);

      if (! use_sbase_molecules)
	 gtk_signal_connect(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC (on_monomer_lib_search_results_button_press), s);
      else
	 gtk_signal_connect(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC (on_monomer_lib_sbase_molecule_button_press), s);
      gtk_widget_show(button);
   }

   int new_box_size = v.size() * 28 + 120; // plus extra for static parts.
   if (new_box_size > 520)
      new_box_size = 520;
   
   // we need to set widget size to new_box_size.  On the dialog?

   gtk_widget_set_usize(dialog, dialog->allocation.width, new_box_size);
      
   // a box of 14 is 400 pixels.  400 is about max size, I'd say 
   gtk_signal_emit_by_name(GTK_OBJECT(vbox), "check_resize");
   gtk_widget_show (vbox);
   return stat;

} 

void
on_monomer_lib_sbase_molecule_button_press (GtkButton *button,
					    gpointer user_data) {
   std::string *s = (std::string *) user_data;
   get_sbase_monomer(s->c_str());
   
}

void
on_monomer_lib_search_results_button_press (GtkButton *button,
					    gpointer user_data) {

   std::string *s = (std::string *) user_data;
   get_monomer(s->c_str());

} 

// Sigh.  This is wrong. We don't want to replace the coordinates - we
// do that in molecule_class_info_t::replace_coords()
//
// But this may be useful in future
// (not not yet compile tested)
// 
// atom_selection_container_t rigid_body_asc;

//       std::vector<CAtom *> atoms;
//       CAtom *at;

//       for (int n=0; n<g.molecules[imol].atom_sel.n_selected_atoms; n++) {
// 	 at = g.molecules[imol].atom_sel.atom_selection[n];
// 	 std::string mmdb_chain(at->residue-GetChainID());
// 	 for(int ifrag=0; ifrag<mol.fragments.size(); ifrag++) {
// 	    for (int ires=1; ires<=mol[ifrag].n_residues(); ires++) {
// 	       for (int iatom=0; iatom<mol[ifrag][ires].atoms.size(); iatom++) {
// 		  if (mmdb_chain == mol[ifrag].fragment_id) {
// 		     if (ires == at->residue->seqNum) {
// 			std::string mmdb_name(at->name);
// 			if (mmdb_name == mol[ifrag][ires][iatom].name) {
// 			   atoms.push_back(at);
// 			}
// 		     }
// 		  }
// 	       }
// 	    }
// 	 }
//       }

//       PPCAtom atom_selection = new CAtom *[atoms.size()];
//       for (int i=0; i<atoms.size(); i++) 
// 	 atom_selection[i] = atoms[i];

//       rigid_body_asc.atom_selection = atom_selection;
//       rigid_body_asc.n_selected_atoms = atoms.size();
//       std::cout << "INFO:: make_atom_selection found " << atoms.size()
// 		<< " atoms" << std::endl;

//       rigid_body_asc.mol = g.molecules[g.imol_rigid_body_refine].atom_sel.mol;


// c.f. molecule_class_info_t::atom_index
//
// This is not a member class of molecule_class_info_t because we dont
// want minimol dependency in molecule_class_info_t and because we
// want the number of atoms *and* the pointer to be returned... messy
// messy - lets do that here.
//
// We only use the n_selected_atoms and atom_selection fields of this
// atom_selection_container_t.
// 
// atom_selection_container_t 
// make_atom_selection(int imol, const coot::minimol::molecule &mol) {

// }

//       std::vector<coot::minimol::atom *> atoms = range_mol.select_atoms_serial();
//       for (int i=0; i<atoms.size(); i++) {
// 	 std::cout << "range mol atom: " << atoms[i]->pos.format() << std::endl;
//       }


#ifdef USE_GUILE
SCM add_dipole_scm(int imol, const char* chain_id, int res_no, const char *ins_code) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> res_specs;
      coot::residue_spec_t rs(chain_id, res_no, ins_code);
      res_specs.push_back(rs);
      graphics_info_t g;
      std::pair<coot::dipole, int> dp =
	 g.molecules[imol].add_dipole(res_specs, *g.Geom_p());
      r = dipole_to_scm(dp);
   }
   graphics_draw();
   return r;
}
#endif

#ifdef USE_GUILE
SCM dipole_to_scm(std::pair<coot::dipole, int> dp) {

   SCM r = SCM_EOL;

   clipper::Coord_orth co = dp.first.get_dipole();
   SCM co_scm = SCM_EOL;
   co_scm = scm_cons(scm_double2num(co.z()), co_scm);
   co_scm = scm_cons(scm_double2num(co.y()), co_scm);
   co_scm = scm_cons(scm_double2num(co.x()), co_scm);
   r = scm_cons(co_scm, r);
   r = scm_cons(SCM_MAKINUM(dp.second), r);
   return r;
}
#endif


#ifdef USE_GUILE
/*! \brief generate a dipole from all atoms in the given residues. */
SCM add_dipole_for_residues_scm(int imol, SCM residue_specs) {

   // int r = -1;
   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> res_specs;
      SCM residue_spec_length = scm_length(residue_specs);
      int l = scm_to_int(residue_spec_length);
      for (int ispec=0; ispec<l; ispec++) {
	 SCM residue_spec_scm = scm_list_ref(residue_specs, SCM_MAKINUM(ispec));
	 coot::residue_spec_t rs = residue_spec_from_scm(residue_spec_scm);
	 res_specs.push_back(rs);
      } 
      graphics_info_t g;
      std::pair<coot::dipole, int> d =
	 g.molecules[imol].add_dipole(res_specs, *g.Geom_p());
      r = dipole_to_scm(d);
   }
   graphics_draw();
   return r;
} 
#endif /* USE_GUILE */

#ifdef USE_PYTHON
PyObject *add_dipole_py(int imol, const char* chain_id, int res_no, const char *ins_code) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> res_specs;
      coot::residue_spec_t rs(chain_id, res_no, ins_code);
      res_specs.push_back(rs);
      graphics_info_t g;
      std::pair<coot::dipole, int> dp =
	 g.molecules[imol].add_dipole(res_specs, *g.Geom_p());
      // set r
      r = dipole_to_py(dp);
   }
   graphics_draw();
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif

#ifdef USE_PYTHON
PyObject *dipole_to_py(std::pair<coot::dipole, int> dp) {

  PyObject *r = PyList_New(2);

  clipper::Coord_orth co = dp.first.get_dipole();
  PyObject *co_py = PyList_New(3);
  PyList_SetItem(co_py, 0, PyFloat_FromDouble(co.x()));
  PyList_SetItem(co_py, 1, PyFloat_FromDouble(co.y()));
  PyList_SetItem(co_py, 2, PyFloat_FromDouble(co.z()));
  PyList_SetItem(r, 0, PyInt_FromLong(dp.second));
  PyList_SetItem(r, 1, co_py);
  return r;
}
#endif


#ifdef USE_PYTHON
/*! \brief generate a dipole from all atoms in the given residues. */
PyObject *add_dipole_for_residues_py(int imol, PyObject *residue_specs) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> res_specs;
      int l = PyObject_Length(residue_specs);
      for (unsigned int ispec=0; ispec<l; ispec++) {
	 PyObject *residue_spec_py = PyList_GetItem(residue_specs, ispec);
	 coot::residue_spec_t rs = residue_spec_from_py(residue_spec_py);
	 res_specs.push_back(rs);
      } 
      graphics_info_t g;
      std::pair<coot::dipole, int> dp =
	 g.molecules[imol].add_dipole(res_specs, *g.Geom_p());
      // set r as a python object here.
      r = dipole_to_py(dp);
   }
   graphics_draw();
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }
   return r;
}
#endif /* USE_PYTHON */


void delete_dipole(int imol, int dipole_number) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].delete_dipole(dipole_number);
   }
   graphics_draw();
}

#ifdef USE_GUILE
SCM non_standard_residue_names_scm(int imol) {

   SCM r = SCM_EOL;
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      std::vector<std::string> resnames =
	 coot::util::non_standard_residue_types_in_molecule(mol);
      
      // remove water if it is there
      std::vector<std::string>::iterator it =
	 std::find(resnames.begin(), resnames.end(), "HOH");
      if (it != resnames.end())
	 resnames.erase(it);
      
      r = generic_string_vector_to_list_internal(resnames);
   }

   return r;
} 
#endif

#ifdef USE_PYTHON
PyObject *non_standard_residue_names_py(int imol) {

  PyObject *r = PyList_New(0);
   if (is_valid_model_molecule(imol)) {
      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      std::vector<std::string> resnames =
	 coot::util::non_standard_residue_types_in_molecule(mol);

      // remove water if it is there
      std::vector<std::string>::iterator it =
	 std::find(resnames.begin(), resnames.end(), "HOH");
      if (it != resnames.end())
	 resnames.erase(it);
      
      r = generic_string_vector_to_list_internal_py(resnames);
   }

   return r;
}
#endif


#ifdef USE_PYTHON 
PyObject *map_peaks_py(int imol_map, float n_sigma) {

   PyObject *r = Py_False;

   if (is_valid_map_molecule(imol_map)) {
      const clipper::Xmap<float> &xmap = graphics_info_t::molecules[imol_map].xmap_list[0];
      int do_positive_levels_flag = 1;
      int also_negative_levels_flag = 0;
      coot::peak_search ps(xmap);
      std::vector<std::pair<clipper::Coord_orth, float> > peaks = 
	 ps.get_peaks(xmap, n_sigma, do_positive_levels_flag, also_negative_levels_flag);
      r = PyList_New(peaks.size());
      for (unsigned int i=0; i<peaks.size(); i++) {
	 PyObject *coords = PyList_New(3);
	 PyList_SetItem(coords, 0, PyFloat_FromDouble(peaks[i].first.x()));
	 PyList_SetItem(coords, 1, PyFloat_FromDouble(peaks[i].first.y()));
	 PyList_SetItem(coords, 2, PyFloat_FromDouble(peaks[i].first.z()));
	 PyList_SetItem(r, i, coords);
      }
   }
 
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }

   return r;
}
#endif 

#ifdef USE_PYTHON
PyObject *map_peaks_near_point_py(int imol_map, float n_sigma, float x, float y, float z,
				  float radius) {
   
   PyObject *r = Py_False;

   if (is_valid_map_molecule(imol_map)) {

      CAtom *at = new CAtom;
      at->SetCoordinates(x,y,z, 1.0, 10.0);
      at->SetAtomName(" CA ");
      at->SetElementName(" C");

      graphics_info_t g;
      CMMDBManager *mol = coot::util::create_mmdbmanager_from_atom(at);
      mol->SetSpaceGroup(g.molecules[imol_map].xmap_list[0].spacegroup().symbol_hm().c_str());
      coot::util::set_mol_cell(mol, g.molecules[imol_map].xmap_list[0].cell());
      
      const clipper::Xmap<float> &xmap = graphics_info_t::molecules[imol_map].xmap_list[0];
      int do_positive_levels_flag = 1;
      int also_negative_levels_flag = 0;
      coot::peak_search ps(xmap);
      std::vector<std::pair<clipper::Coord_orth, float> > peaks = 
	 ps.get_peaks(xmap, mol, n_sigma, do_positive_levels_flag, also_negative_levels_flag);
      clipper::Coord_orth ref_pt(x,y,z);
      std::vector<std::pair<clipper::Coord_orth, float> > close_peaks;
      for (unsigned int i=0; i<peaks.size(); i++) {
	 if (clipper::Coord_orth::length(ref_pt, peaks[i].first) < radius) {
	    close_peaks.push_back(peaks[i]);
	 }
      } 
      r = PyList_New(close_peaks.size());
      for (unsigned int i=0; i<close_peaks.size(); i++) {
	 PyObject *coords = PyList_New(4);
	 PyList_SetItem(coords, 0, PyFloat_FromDouble(close_peaks[i].first.x()));
	 PyList_SetItem(coords, 1, PyFloat_FromDouble(close_peaks[i].first.y()));
	 PyList_SetItem(coords, 2, PyFloat_FromDouble(close_peaks[i].first.z()));
	 PyList_SetItem(coords, 3, PyFloat_FromDouble(close_peaks[i].second));
	 PyList_SetItem(r, i, coords);
      }
      delete mol;
   } 

   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }

   return r;
}
#endif 


#ifdef USE_GUILE
SCM map_peaks_scm(int imol_map, float n_sigma) {

   SCM r = SCM_BOOL_F;

   if (is_valid_map_molecule(imol_map)) {
      r = SCM_EOL;
      const clipper::Xmap<float> &xmap = graphics_info_t::molecules[imol_map].xmap_list[0];
      int do_positive_levels_flag = 1;
      int also_negative_levels_flag = 0;
      coot::peak_search ps(xmap);
      std::vector<std::pair<clipper::Coord_orth, float> > peaks = 
	 ps.get_peaks(xmap, n_sigma, do_positive_levels_flag, also_negative_levels_flag);
      for (unsigned int i=0; i<peaks.size(); i++) {
	 SCM pt = SCM_EOL;
	 SCM x_scm = scm_double2num(peaks[i].first.x());
	 SCM y_scm = scm_double2num(peaks[i].first.y());
	 SCM z_scm = scm_double2num(peaks[i].first.z());
	 pt = scm_cons(x_scm, pt);
	 pt = scm_cons(y_scm, pt);
	 pt = scm_cons(z_scm, pt);
	 pt = scm_reverse(pt);
	 r = scm_cons(pt, r);
      }
   }
   return r;
} 
#endif 

#ifdef USE_GUILE
SCM map_peaks_near_point_scm(int imol_map, float n_sigma, float x, float y, float z,
			     float radius) {

   SCM r = SCM_BOOL_F;
   if (is_valid_map_molecule(imol_map)) {

      graphics_info_t g;
      CAtom *at = new CAtom;
      at->SetCoordinates(x,y,z,1.0,10.0);
      at->SetAtomName(" CA ");
      at->SetElementName(" C");

      CMMDBManager *mol = coot::util::create_mmdbmanager_from_atom(at);
      mol->SetSpaceGroup(g.molecules[imol_map].xmap_list[0].spacegroup().symbol_hm().c_str());
      coot::util::set_mol_cell(mol, g.molecules[imol_map].xmap_list[0].cell());
      
      const clipper::Xmap<float> &xmap = graphics_info_t::molecules[imol_map].xmap_list[0];
      int do_positive_levels_flag = 1;
      int also_negative_levels_flag = 0;
      coot::peak_search ps(xmap);
      std::vector<std::pair<clipper::Coord_orth, float> > peaks = 
	 ps.get_peaks(xmap, mol, n_sigma, do_positive_levels_flag, also_negative_levels_flag);
      clipper::Coord_orth ref_pt(x,y,z);
      r = SCM_EOL;
      std::vector<std::pair<clipper::Coord_orth, float> > close_peaks;
      for (unsigned int i=0; i<peaks.size(); i++) {
	 if (clipper::Coord_orth::length(ref_pt, peaks[i].first) < radius) {
	    close_peaks.push_back(peaks[i]);
	 }
      }

      if (1) { // debug
	 for (unsigned int i=0; i<close_peaks.size(); i++) {
	    std::cout << "close peak " << i << " " << close_peaks[i].first.format() << "   "
		      << close_peaks[i].second << std::endl;
	 }
      } 
      
      for (unsigned int i=0; i<close_peaks.size(); i++) {
	 SCM pt = SCM_EOL;
	 SCM d     = scm_double2num(close_peaks[i].second);
	 SCM x_scm = scm_double2num(close_peaks[i].first.x());
	 SCM y_scm = scm_double2num(close_peaks[i].first.y());
	 SCM z_scm = scm_double2num(close_peaks[i].first.z());
	 pt = scm_cons(d, pt);
	 pt = scm_cons(x_scm, pt);
	 pt = scm_cons(y_scm, pt);
	 pt = scm_cons(z_scm, pt);
	 pt = scm_reverse(pt);
	 r = scm_cons(pt, r);
      }
      r = scm_reverse(r);
      delete mol;
   }
   return r;
} 
#endif 
// (map-peaks-near-point-scm 1 4 44.7 11 13.6 6)

#ifdef USE_GUILE
/*! \brief return a list of compoundIDs of in SBase of which the
  given string is a substring of the compound name */
SCM matching_compound_names_from_sbase_scm(const char *compound_name_fragment) {

   graphics_info_t g;
   std::vector<std::pair<std::string, std::string> > matching_comp_ids =
      g.Geom_p()->matching_ccp4srs_residues_names(compound_name_fragment);

   std::vector<std::string> rv;
   for (unsigned int i=0; i<matching_comp_ids.size(); i++)
      rv.push_back(matching_comp_ids[i].first);
   
   SCM r = generic_string_vector_to_list_internal(rv);
   return r;
}
#endif

#ifdef USE_PYTHON
/*! \brief return a list of compoundIDs of in SBase of which the
  given string is a substring of the compound name */
PyObject *matching_compound_names_from_sbase_py(const char *compound_name_fragment) {

   graphics_info_t g;
   std::vector<std::pair<std::string, std::string> > matching_comp_ids =
      g.Geom_p()->matching_ccp4srs_residues_names(compound_name_fragment);

   std::vector<std::string> rv;
   for (unsigned int i=0; i<matching_comp_ids.size(); i++)
      rv.push_back(matching_comp_ids[i].first);
   
   PyObject *r = generic_string_vector_to_list_internal_py(rv);
   return r;
}
#endif /* USE_PYTHON */

#ifdef USE_GUILE
/*! \brief return a list of compoundIDs in the dictionary of which the
  given string is a substring of the compound name */
SCM matching_compound_names_from_dictionary_scm(const char *compound_name_fragment,
						short int allow_minimal_descriptions_flag) {

   graphics_info_t g;
   std::vector<std::pair<std::string, std::string> > matching_comp_ids =
      g.Geom_p()->matching_names(compound_name_fragment,
				 allow_minimal_descriptions_flag);
   std::vector<std::string> rv;
   for (unsigned int i=0; i<matching_comp_ids.size(); i++)
      rv.push_back(matching_comp_ids[i].first);
   
   SCM r = generic_string_vector_to_list_internal(rv);
   return r;

}

#endif /* USE_GUILE */


/*! \brief add residue name to the list of residue names that don't
  get auto-loaded from the Refmac dictionary. */
void add_non_auto_load_residue_name(const char *s) {
   graphics_info_t g;
   g.Geom_p()->add_non_auto_load_residue_name(s);
}

/*! \brief remove residue name from the list of residue names that don't
  get auto-loaded from the Refmac dictionary. */
void remove_non_auto_load_residue_name(const char *s) {
   graphics_info_t g;
   g.Geom_p()->remove_non_auto_load_residue_name(s);
}

/*! \brief try to auto-load the dictionary for comp_id from the refmac monomer library.

   return 0 on failure.
   return 1 on successful auto-load.
   return 2 on already-read.
   */
int auto_load_dictionary(const char *comp_id) {

   graphics_info_t g;
   int r = 0;
   if (comp_id) {
      std::string s = comp_id;
      if (g.Geom_p()->have_dictionary_for_residue_type_no_dynamic_add(s)) { 
	 r = 2;
      } else { 
	 int status = g.Geom_p()->try_dynamic_add(s, g.cif_dictionary_read_number);
	 if (status)
	    r = 1;
      }
   }
   return r;
}

/*! \brief as above, but dictionary is cleared and re-read if it already exists */
int reload_dictionary(const char *comp_id) {
   
   graphics_info_t g;
   int r = 0;
   if (comp_id) {
      std::string s = comp_id;
      int status = g.Geom_p()->try_dynamic_add(s, g.cif_dictionary_read_number);
      if (status)
	 r = 1;
   }
   return r;

} 




#ifdef USE_PYTHON
/*! \brief return a list of compoundIDs in the dictionary which the
  given string is a substring of the compound name */
PyObject *matching_compound_names_from_dictionary_py(const char *compound_name_fragment,
						     short int allow_minimal_descriptions_flag) {

   graphics_info_t g;
   std::vector<std::pair<std::string, std::string> > matching_comp_ids =
      g.Geom_p()->matching_names(compound_name_fragment,
				 allow_minimal_descriptions_flag);

   std::vector<std::string> rv;
   for (unsigned int i=0; i<matching_comp_ids.size(); i++)
      rv.push_back(matching_comp_ids[i].first);
   
   PyObject *r = generic_string_vector_to_list_internal_py(rv);
   return r;
} 
#endif /* USE_PYTHON */



int get_sbase_monomer(const char *comp_id) {

   return get_ccp4srs_monomer_and_dictionary(comp_id);

}
      
int get_ccp4srs_monomer_and_dictionary(const char *comp_id) {

   int imol = -1;

   graphics_info_t g;
   CResidue *residue_p = g.Geom_p()->get_ccp4srs_residue(comp_id);
   if (residue_p) {
      CMMDBManager *mol = new CMMDBManager;
      CModel *model_p = new CModel;
      CChain *chain_p = new CChain;
      residue_p->SetResID(comp_id, 1, "");
      chain_p->AddResidue(residue_p);
      chain_p->SetChainID("A");
      model_p->AddChain(chain_p);
      mol->AddModel(model_p);
      imol = g.create_molecule();
      std::string name = "Monomer ";
      name += coot::util::upcase(comp_id);
      graphics_info_t::molecules[imol].install_model(imol, make_asc(mol), name, 1, 0);
      move_molecule_to_screen_centre_internal(imol);
      g.Geom_p()->fill_using_ccp4srs(comp_id); // if you can (should be able to)
      graphics_draw();
   }

   return imol;
}


#ifdef USE_GUILE
/*! \brief return the monomer name
  return scheme false if not found */
SCM comp_id_to_name_scm(const char *comp_id) {
   SCM r = SCM_BOOL_F;
   graphics_info_t g;
   std::pair<bool, std::string> name = g.Geom_p()->get_monomer_name(comp_id);
   if (name.first) {
      r = scm_makfrom0str(name.second.c_str());
   }
   return r;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
/*! \brief return the monomer name

  return python false if not found */
PyObject *comp_id_to_name_py(const char *comp_id) {

   PyObject *r = Py_False;

   graphics_info_t g;
   std::pair<bool, std::string> name = g.Geom_p()->get_monomer_name(comp_id);
   if (name.first) {
      r = PyString_FromString(name.second.c_str());
   }
   if (PyBool_Check(r)) {
      Py_INCREF(r);
   }
   return r;
}

#endif // USE_PYTHON



/*  ----------------------------------------------------------------------- */
//                  animated ligand interactions
/*  ----------------------------------------------------------------------- */
void add_animated_ligand_interaction(int imol, const coot::fle_ligand_bond_t &lb) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t::molecules[imol].add_animated_ligand_interaction(lb);
   } 
} 




/* Just pondering, for use with exporting ligands from the 2D sketcher
   to the main coot window. It should find the residue that this
   residue is sitting on top of that is in a molecule that has lot of
   atoms (i.e. is a protein) and create a new molecule that is a copy
   of the molecule without the residue/ligand that this (given)
   ligand overlays - and a copy of this given ligand.  */
int exchange_ligand(int imol_lig, const char *chain_id_lig, int resno_lig, const char *ins_code_lig) {

   int imol = -1;

   return imol;

} 

/*! \brief Match ligand atom names

  By using graph matching, make the names of the atoms of the
  given ligand/residue match those of the reference residue/ligand as
  closely as possible - where there would be a atom name clash, invent
  a new atom name.
 */
void match_ligand_atom_names(int imol_ligand, const char *chain_id_ligand, int resno_ligand, const char *ins_code_ligand,
			     int imol_reference, const char *chain_id_reference, int resno_reference, const char *ins_code_reference) {

   if (! is_valid_model_molecule(imol_ligand)) { 
      std::cout << "Not a valid model number " << imol_ligand << std::endl;
   } else {
      if (! is_valid_model_molecule(imol_reference)) {
	      std::cout << "Not a valid model number " << imol_reference << std::endl;
      } else {
	 graphics_info_t g;
	 CResidue *res_ref =
	    g.molecules[imol_reference].get_residue(chain_id_reference, resno_reference, ins_code_reference);
	 if (! res_ref) {
	    std::cout << "No reference residue " << chain_id_reference << " " << resno_reference
		      << " "  << ins_code_reference << std::endl;
	 } else { 
	    // now lock res_ref - when multi-threaded
	    g.molecules[imol_ligand].match_ligand_atom_names(chain_id_ligand, resno_ligand, ins_code_ligand, res_ref);
	    graphics_draw();
	 }
      }
   }
}


/*  return a list of chiral centre ids as determined from topological
    equivalence analysis based on the bond info (and element names).

    A better OO design would put this inside protein-geometry.

*/
std::vector<std::string>
topological_equivalence_chiral_centres(const std::string &residue_type) {

   std::vector<std::string> centres;
#ifdef HAVE_GOOCANVAS

   graphics_info_t g;

   std::pair<bool, coot::dictionary_residue_restraints_t> p = 
      g.Geom_p()->get_monomer_restraints(residue_type);

   if (p.first) {

      const coot::dictionary_residue_restraints_t &restraints = p.second;
      lig_build::molfile_molecule_t mm(restraints); // makes dummy atoms
      widgeted_molecule_t wm(mm, NULL); // we have the atom names already.
      topological_equivalence_t top_eq(wm.atoms, wm.bonds);
      centres = top_eq.chiral_centres();

      std::cout << "-------- chiral centres by topology analysis -----------"
		<< std::endl;
      for (unsigned int ic=0; ic<centres.size(); ic++) { 
	 std::cout << "     " << ic << "  " << centres[ic] << std::endl;
      }
      std::cout << "-------------------" << std::endl;
   } 

#endif // HAVE_GOOCANVAS
   return centres;
} 


// This doesn't reset the view, perhaps it should (normally, we do not
// reset the view when using install_model).
// 
int read_small_molecule_cif(const char *file_name) {

   int imol = -1;
   coot::smcif smcif;
   CMMDBManager *mol = smcif.read_sm_cif(file_name);

   if (mol) {
      graphics_info_t g;
      imol = g.create_molecule();
      g.molecules[imol].install_model(imol, mol, file_name, 0, 1);
      update_go_to_atom_window_on_new_mol();
      graphics_draw();
   } 

   return imol;
}



// This doesn't reset the view, perhaps it should (normally, we do not
// reset the view when using install_model).
// 
int read_small_molecule_data_cif(const char *file_name) {

   int imol = -1;
   coot::smcif smcif;
   bool state = smcif.read_data_sm_cif(file_name);
   if (state) {
      graphics_info_t g;
      imol = g.create_molecule();

      std::pair<clipper::Xmap<float>, clipper::Xmap<float> > maps = smcif.sigmaa_maps();
      if (not (maps.first.is_null())) {
	 g.molecules[imol].new_map(maps.first, file_name);
	 int imol_diff = g.create_molecule();
	 g.molecules[imol_diff].new_map(maps.second, file_name);
	 g.molecules[imol_diff].set_map_is_difference_map();
      } else { 
	 clipper::Xmap<float> xmap = smcif.map();
	 g.molecules[imol].new_map(xmap, file_name);
      }
      graphics_draw();
   } 
   return imol;
}



#ifdef USE_GUILE
void
multi_residue_torsion_scm(int imol, SCM residues_specs_scm) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::vector<coot::residue_spec_t> residue_specs =
	 scm_to_residue_specs(residues_specs_scm);
      g.multi_torsion_residues(imol, residue_specs);

      graphics_draw();
   } 

}
#endif


#ifdef USE_PYTHON
void
multi_residue_torsion_py(int imol, PyObject *residues_specs_py) {

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      std::vector<coot::residue_spec_t> residue_specs = py_to_residue_specs(residues_specs_py);
      g.multi_torsion_residues(imol, residue_specs);
      graphics_draw();
   } 
} 

#endif // USE_PYTHON


void
multi_residue_torsion_fit(int imol, const std::vector<coot::residue_spec_t> &residue_specs, int n_trials) {

   if (is_valid_model_molecule(imol)) {
      if (is_valid_map_molecule(imol_refinement_map())) {
	 graphics_info_t g;
	 const clipper::Xmap<float> &xmap =
	    g.molecules[imol_refinement_map()].xmap_list[0];
	 g.molecules[imol].multi_residue_torsion_fit(residue_specs, xmap, n_trials, g.Geom_p());
	 graphics_draw();
      }
   }
} 


/*! \brief fit residues

(note: fit to the current-refinement map)
*/
#ifdef USE_GUILE
SCM multi_residue_torsion_fit_scm(int imol, SCM residues_specs_scm, int n_trials) {

   SCM r = SCM_BOOL_F;
   if (is_valid_model_molecule(imol)) {
      if (is_valid_map_molecule(imol_refinement_map())) {
	 graphics_info_t g;
	 std::vector<coot::residue_spec_t> residue_specs =
	    scm_to_residue_specs(residues_specs_scm);
	 const clipper::Xmap<float> &xmap =
	    g.molecules[imol_refinement_map()].xmap_list[0];
	 g.molecules[imol].multi_residue_torsion_fit(residue_specs, xmap, n_trials, g.Geom_p());
	 graphics_draw();
	 r = SCM_BOOL_T; // we did something at least.
      }
   }
   return r;
} 
#endif


/*! \brief fit residues

(note: fit to the current-refinement map)
*/
#ifdef USE_PYTHON
PyObject *multi_residue_torsion_fit_py(int imol, PyObject *residues_specs_py, int n_trials) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      if (is_valid_map_molecule(imol_refinement_map())) {
	 graphics_info_t g;
	 std::vector<coot::residue_spec_t> residue_specs =
       py_to_residue_specs(residues_specs_py);
	 const clipper::Xmap<float> &xmap =
	    g.molecules[imol_refinement_map()].xmap_list[0];
	 g.molecules[imol].multi_residue_torsion_fit(residue_specs, xmap, n_trials, g.Geom_p());
	 graphics_draw();
	 r = Py_True; // we did something at least.
      }
   }
   Py_INCREF(r);
   return r;
} 
#endif // PYTHON


void
setup_multi_residue_torsion() {

   graphics_info_t g;
   g.in_multi_residue_torsion_define = true; // we are about to pick atoms
   g.multi_residue_torsion_picked_residue_specs.clear(); // clear previous picked residues
   pick_cursor_maybe(); // depends on ctrl key for rotate

   g.multi_residue_torsion_reverse_fragment_mode = false;
   GtkWidget *w = create_multi_residue_torsion_pick_dialog();
   gtk_widget_show(w);
} 


/* show the rotatable bonds dialog */
void
show_multi_residue_torsion_dialog() {

   graphics_info_t g;
   if (g.multi_residue_torsion_picked_residue_specs.size()) {
      g.multi_torsion_residues(g.multi_residue_torsion_picked_residues_imol,
			       g.multi_residue_torsion_picked_residue_specs);
      g.in_multi_residue_torsion_mode = true;
   }
   graphics_draw();
}

void clear_multi_residue_torsion_mode() {

   graphics_info_t g;
   g.in_multi_residue_torsion_mode = false;
}

void set_multi_residue_torsion_reverse_mode(short int mode) {

   graphics_info_t g;
   g.multi_residue_torsion_reverse_fragment_mode = mode;
} 


/* ------------------------------------------------------------------------- */
/*                      prodrg import function                               */
/* ------------------------------------------------------------------------- */
// the function passed to lbg, which is called when a new
// prodrg-in.mdl file has been made.  We no longer have a timeout
// function waiting for prodrg-in.mdl to be updated/written.
// 
void prodrg_import_function(std::string file_name) {

   std::string func_name = "import-from-3d-generator-from-mdl";
   std::vector<coot::command_arg_t> args;
   args.push_back(single_quote(file_name));
   coot::scripting_function(func_name, args);
}


/* ------------------------------------------------------------------------- */
/*                       SBase import function                               */
/* ------------------------------------------------------------------------- */
// the function passed to lbg, so that it calls it when a new
// SBase comp_id is required.  We no longer have a timeout
// function waiting for prodrg-in.mdl to be updated/written.
// 
void sbase_import_function(std::string comp_id) {

   bool done = false; 
#ifdef USE_PYTHON
   if (graphics_info_t::prefer_python) {
      std::string s = "get_sbase_monomer_and_overlay(";
      s += single_quote(comp_id);
      s += ")";
      safe_python_command(s);
      done = true;
   }
#endif

#ifdef USE_GUILE
   if (! done) { 
      std::string s = "(get-sbase-monomer-and-overlay ";
      s += single_quote(comp_id);
      s += ")";
      safe_scheme_command(s);
   }
#endif
   
}


// return a spec for the first residue with the given type.
// test the returned spec for unset_p().
// 
coot::residue_spec_t
get_residue_by_type(int imol, const std::string &residue_type) {

   coot::residue_spec_t spec;

   if (is_valid_model_molecule(imol)) {
      graphics_info_t g;
      spec = g.molecules[imol].get_residue_by_type(residue_type);
   } 
   return spec;
} 


#ifdef USE_GUILE
SCM get_residue_by_type_scm(int imol, const std::string &residue_type) {

   SCM r = SCM_BOOL_F;
   coot::residue_spec_t spec = get_residue_by_type(imol, residue_type);
   if (! spec.unset_p())
      r = scm_residue(spec);
   return r;
}
#endif


#ifdef USE_PYTHON
PyObject *get_residue_by_type_py(int imol, const std::string &residue_type) {

   PyObject *r = Py_False;
   coot::residue_spec_t spec = get_residue_by_type(imol, residue_type);
   if (! spec.unset_p())
      r = py_residue(spec);
   if (PyBool_Check(r)) {
     Py_INCREF(r);
   }

   return r;
} 
#endif


#ifdef USE_GUILE
SCM het_group_residues_scm(int imol) {

   SCM r = SCM_EOL;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> specs = graphics_info_t::molecules[imol].het_groups();
      for (unsigned int i=0; i<specs.size(); i++) { 
	 SCM s = scm_residue(specs[i]);
	 r = scm_cons(s, r);
      }
      r = scm_reverse(r);
   } 
   return r;
}
#endif

#ifdef USE_PYTHON
PyObject *het_group_residues_py(int imol) {

   PyObject *r = Py_False;
   if (is_valid_model_molecule(imol)) {
      std::vector<coot::residue_spec_t> specs = graphics_info_t::molecules[imol].het_groups();
      r = PyList_New(specs.size());
      for (unsigned int i=0; i<specs.size(); i++) { 
         PyList_SetItem(r, i, py_residue(specs[i]));
      }
   } 
   if (PyBool_Check(r)) {
      Py_INCREF(r);
   }
      
   return r;
}
#endif

/*! \brief return the number of non-hydrogen atoms in the given
  het-group (comp-id). 

Return -1 on comp-id not found in dictionary.  */
int het_group_n_atoms(const char *comp_id) {

   graphics_info_t g;
   int r = g.Geom_p()->n_non_hydrogen_atoms(comp_id);
   return r;
}

#include "c-interface-ligands-swig.hh"

#ifdef USE_GUILE
SCM new_molecule_sans_biggest_ligand_scm(int imol) {

   SCM r = SCM_BOOL_F;
   std::pair<CResidue *, int> res = new_molecule_sans_biggest_ligand(imol);
   if (res.first) {
      r = scm_list_2(SCM_MAKINUM(res.second), scm_residue(res.first));
   }
   return r;
}
#endif


std::pair<CResidue *, int>
new_molecule_sans_biggest_ligand(int imol) {

   CResidue *res = NULL;
   int imol_new = -1;
   if (is_valid_model_molecule(imol)) {

      CMMDBManager *mol = graphics_info_t::molecules[imol].atom_sel.mol;
      CResidue *r = coot::util::get_biggest_hetgroup(mol);
      if (r) {

	 // copy mol, create a new molecule and delete the residue
	 // with the spec of r from the new molecule.
	 //
	 res = r;
	 CMMDBManager *new_mol = new CMMDBManager;
	 new_mol->Copy(mol, MMDBFCM_All);
	 atom_selection_container_t asc = make_asc(new_mol);
	 std::string label = "Copy_of_";
	 label += graphics_info_t::molecules[imol].name_;
	 imol_new = graphics_info_t::create_molecule();
	 graphics_info_t::molecules[imol_new].install_model(imol_new, asc, label, 1);
	 update_go_to_atom_window_on_new_mol();
      }
   }
   return std::pair<CResidue *, int> (res, imol_new);
}


					      
#ifdef USE_GUILE
bool
residues_torsions_match_scm(int imol_1, SCM res_1,
			    int imol_2, SCM res_2,
			    float tolerance) {

   bool r = false;

   graphics_info_t g;
   if (is_valid_model_molecule(imol_1)) {
      if (is_valid_model_molecule(imol_2)) {
	 coot::residue_spec_t rs1 = residue_spec_from_scm(res_1);
	 coot::residue_spec_t rs2 = residue_spec_from_scm(res_2);
	 if (!rs1.unset_p() && !rs2.unset_p()) {
	    CResidue *r_1 = g.molecules[imol_1].get_residue(rs1);
	    CResidue *r_2 = g.molecules[imol_2].get_residue(rs2);
	    if (r_1 && r_2) {
	       CMMDBManager *mol1 = g.molecules[imol_1].atom_sel.mol;
	       CMMDBManager *mol2 = g.molecules[imol_2].atom_sel.mol;
// 	       r = coot::compare_residue_torsions(mol1, r_1,
// 						  mol2, r_2,
// 						  tolerance,
// 						  g.Geom_p());
	    }
	 }
      }
   }
   return r;
}
#endif // USE_GUILE

#ifdef USE_PYTHON
bool
residues_torsions_match_py(int imol_1, PyObject *res_1,
			   int imol_2, PyObject *res_2,
			   float tolerance) {

   bool r = false;

   graphics_info_t g;
   if (is_valid_model_molecule(imol_1)) {
      if (is_valid_model_molecule(imol_2)) {
	 coot::residue_spec_t rs1 = residue_spec_from_py(res_1);
	 coot::residue_spec_t rs2 = residue_spec_from_py(res_2);
	 if (!rs1.unset_p() && !rs2.unset_p()) {
	    CResidue *r_1 = g.molecules[imol_1].get_residue(rs1);
	    CResidue *r_2 = g.molecules[imol_2].get_residue(rs2);
	    if (r_1 && r_2) {
	       CMMDBManager *mol1 = g.molecules[imol_1].atom_sel.mol;
	       CMMDBManager *mol2 = g.molecules[imol_2].atom_sel.mol;
// 	       r = coot::compare_residue_torsions(mol1, r_1,
// 						  mol2, r_2,
// 						  tolerance,
// 						  g.Geom_p());
	    }
	 }
      }
   } 
   return r;
}
#endif // USE_PYTHON


