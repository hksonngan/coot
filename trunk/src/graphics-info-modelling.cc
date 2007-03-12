/* src/graphics-info.cc
 * 
 * Copyright 2002, 2003, 2004, 2005, 2006 by Paul Emsley, The University of York
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
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef HAVE_STRING
#define HAVE_STRING
#include <string>
#endif

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif

#include <gtk/gtk.h>  // must come after mmdb_manager on MacOS X Darwin
#include <GL/glut.h>  // for some reason...  // Eh?
#include <gtkgl/gtkglarea.h>

#include <iostream>
#include <dirent.h>   // for refmac dictionary files

#include <sys/types.h> // for stating
#include <sys/stat.h>

#if !defined _MSC_VER
#include <unistd.h>
#else
#define AddAtomA AddAtom
#endif

#ifdef USE_GUILE
#include <guile/gh.h>
#endif

#ifdef USE_PYTHON
#include "Python.h"
#endif // USE_PYTHON


#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "Cartesian.h"
#include "Bond_lines.h"
#include "dunbrack.hh"

#include "clipper/core/map_utils.h" // Map_stats
#include "graphical_skel.h"

#include "interface.h"

#include "molecule-class-info.h"


#include "globjects.h"
#include "ligand.hh"
#include "graphics-info.h"


#include "coot-utils.hh"


// Idealize the geometry without considering the map.
//
// If you change things in here - also consider changing
// copy_mol_and_refine.
//
short int
graphics_info_t::copy_mol_and_regularize(int imol,
					 int resno_1, 
					 int resno_2, 
					 std::string altconf,// use this (e.g. "A") or "".
					 std::string chain_id_1) {

   short int irest = 0;  // make 1 if restraints were found

#ifdef HAVE_GSL

   imol_moving_atoms = imol;  // for use when we accept the
			      // regularization and want to copy the
			      // coordinates back.
   
   // make the selection and build a new molecule inside restraints.

   short int have_flanking_residue_at_start = 0;
   short int have_flanking_residue_at_end = 0;
   short int have_disulfide_residues = 0;  // other residues are included in the
   // residues_mol for disphide
   // restraints.
   
   // 9 Sept 2003: The atom selection goes mad if residue with seqnum
   // iend_res+1 does not exist, but is not at the end of the chain.

   // Therefore we will set 2 flags, which tell us if istart_res-1 and
   // iend_res+1 exist.  And we do that by trying to select atoms from
   // them - if they exist, the number of selected atoms will be more
   // than 0.

//    istart_minus_flag = 0;  // from simple restraint code
//    iend_plus_flag    = 0;

   CMMDBManager *mol = molecules[imol].atom_sel.mol; // short-hand usage
   int SelHnd_ends = mol->NewSelection();
   int n_atoms_ends;
   PPCAtom atoms_end;
   mol->SelectAtoms(SelHnd_ends, 0, (char *) chain_id_1.c_str(), resno_1-1, "*", resno_1-1, "*","*","*","*","*");
   mol->GetSelIndex(SelHnd_ends, atoms_end, n_atoms_ends);
   if (n_atoms_ends > 0)
      have_flanking_residue_at_start = 1; // we have residue istart_res-1
   mol->DeleteSelection(SelHnd_ends);

   SelHnd_ends = mol->NewSelection();
   mol->SelectAtoms(SelHnd_ends, 0, (char *) chain_id_1.c_str(), resno_2+1, "*", resno_2+1, "*","*","*","*","*");
   mol->GetSelIndex(SelHnd_ends, atoms_end, n_atoms_ends);
   if (n_atoms_ends > 0)
      have_flanking_residue_at_end = 1; // we have residue iend_res+1
   mol->DeleteSelection(SelHnd_ends);

   // First make an atom selection of the residues selected to regularize.
   // 
   int selHnd = molecules[imol].atom_sel.mol->NewSelection();
   int nSelResidues;
   PCResidue *SelResidues = NULL;

   // Consider as the altconf the altconf of one of the residues (we
   // must test that the altlocs of the selected atoms to be either
   // the same as each other (A = A) or one of them is "".  We need to
   // know the mmdb syntax for "either".  Well, now I know that's ",A"
   // (for either blank or "A").
   // 
   // 
   // 
   int iselection_resno_start = resno_1;
   int iselection_resno_end   = resno_2;
   if (have_flanking_residue_at_start) iselection_resno_start--;
   if (have_flanking_residue_at_end)   iselection_resno_end++;
   // 
   molecules[imol].atom_sel.mol->Select(selHnd, STYPE_RESIDUE, 0,
					(char *) chain_id_1.c_str(),
					iselection_resno_start, "*",
					iselection_resno_end, "*",
					"*",  // residue name
					"*",  // Residue must contain this atom name?
					"*",  // Residue must contain this Element?
					"*",  // altLocs
					SKEY_NEW // selection key
					);
   molecules[imol].atom_sel.mol->GetSelIndex(selHnd, SelResidues, nSelResidues);
   std::pair<int, std::vector<std::string> > icheck = 
      check_dictionary_for_residues(SelResidues, nSelResidues);

   if (icheck.first == 0) { 
      for (unsigned int icheck_res=0; icheck_res<icheck.second.size(); icheck_res++) { 
	 std::cout << "WARNING:: Failed to find restraints for :" 
		   << icheck.second[icheck_res] << ":" << std::endl;
      }
   }

   std::cout << "INFO:: " << nSelResidues 
	     << " residues selected for refined and flanking residues"
	     << std::endl;

   if (nSelResidues > 0) {

      std::vector<CAtom *> fixed_atoms;
      char *chn = (char *) chain_id_1.c_str();

      if (nSelResidues > refine_regularize_max_residues) { 

	 std::cout << "WARNING:: Hit heuristic fencepost! Too many residues to refine\n "
		   << "          FYI: " << nSelResidues
		   << " > " << refine_regularize_max_residues
		   << " (which is your current maximum).\n";
	 std::cout << "Use (set-refine-max-residues "
		   << 2*refine_regularize_max_residues
		   << ") to increase limit\n";

      } else { 

	 // Notice that we have to make 2 atom selections, one, which includes
	 // flanking (and disulphide eventually) residues that is used for the
	 // restraints (restraints_container_t constructor) and one that is the
	 // moving atoms (which does not have flanking atoms).
	 // 
	 // The restraints_container_t moves the atom of the mol that is passes to
	 // it.  This must be the same mol as the moving atoms mol so that the
	 // changed atom positions can be seen.  However (as I said) the moving
	 // atom mol should not have flanking residues shown.  So we make an asc
	 // that has the same mol as that passed to the restraints but a different
	 // atom selection (it is the atom selection that is used in the bond
	 // generation).
	 // 
	 CMMDBManager *residues_mol = 
	    create_mmdbmanager_from_res_selection(SelResidues, nSelResidues, 
						  have_flanking_residue_at_start,
						  have_flanking_residue_at_end,
						  altconf, 
						  chain_id_1, 
						  0, // 0 because we are not in alt conf split
						  imol);

	 coot::restraints_container_t restraints(resno_1,
						 resno_2,
						 have_flanking_residue_at_start,
						 have_flanking_residue_at_end,
						 have_disulfide_residues,
						 altconf,
						 chn,
						 residues_mol,
						 fixed_atoms);

	 atom_selection_container_t local_moving_atoms_asc;
	 local_moving_atoms_asc.mol = (MyCMMDBManager *) residues_mol;
	 // local_moving_atoms_asc.UDDOldAtomIndexHandle = residues_mol_pair.second; // not yet
	 local_moving_atoms_asc.UDDAtomIndexHandle = -1;
	 local_moving_atoms_asc.UDDOldAtomIndexHandle = -1;

	 local_moving_atoms_asc.SelectionHandle = residues_mol->NewSelection();
	 residues_mol->SelectAtoms (local_moving_atoms_asc.SelectionHandle, 0, "*",
				   resno_1, // starting resno, an int
				   "*",     // any insertion code
				   resno_2, // ending resno
				   "*", // ending insertion code
				   "*", // any residue name
				   "*", // atom name
				   "*", // elements
				   "*"  // alt loc.
				   );

	 residues_mol->GetSelIndex(local_moving_atoms_asc.SelectionHandle,
				   local_moving_atoms_asc.atom_selection,
				   local_moving_atoms_asc.n_selected_atoms);

	 // coot::restraint_usage_Flags flags = coot::BONDS;
	 // coot::restraint_usage_Flags flags = coot::BONDS_AND_ANGLES;
	 // coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_AND_PLANES;
	 // coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_TORSIONS_AND_PLANES; 
	 coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_PLANES_AND_NON_BONDED;
	 // flags = coot::BONDS_ANGLES_PLANES_NON_BONDED_AND_CHIRAL;
	 flags = coot::BONDS_ANGLES_PLANES_AND_NON_BONDED;

	 short int do_link_torsions = 0;
	 short int do_residue_internal_torsions = 0;  // fixme 

	 if (do_torsion_restraints) { 
	    flags = coot::BONDS_ANGLES_TORSIONS_PLANES_AND_NON_BONDED;
	 } 

	 // coot::pseudo_restraint_bond_type pseudos = coot::NO_PSEUDO_BONDS;
	 coot::pseudo_restraint_bond_type pseudos = graphics_info_t::pseudo_bonds_type;
	 int nrestraints = restraints.make_restraints(*geom_p, flags,
						      do_residue_internal_torsions,
						      do_peptide_torsion_restraints,
						      pseudos);

	 if (nrestraints > 0) {
	    restraints.minimize(flags);
	    last_restraints = restraints;

	    // coot::add_atom_index_udd_as_old(*moving_atoms_asc);
      
	    // int do_disulphide_flag = 0;
	    regularize_object_bonds_box.clear_up();

	    make_moving_atoms_graphics_object(local_moving_atoms_asc); // sets moving_atoms_asc
	    std::cout << "moving_atoms_asc->UDDOldAtomIndexHandle: "
		      << moving_atoms_asc->UDDOldAtomIndexHandle << std::endl;
	    moving_atoms_asc_type = coot::NEW_COORDS_REPLACE;
	    irest = 1;
	 } else {
	    GtkWidget *widget = create_no_restraints_info_dialog();
	    gtk_widget_show(widget);
	 } 
      }
   } else {
      std::cout << "No Atoms!!!!  This should never happen: " << std::endl;
      std::cout << "  in create_regularized_graphical_object" << std::endl;
   } 
#else 
   std::cout << "Cannot refine without compilation with GSL" << std::endl;
#endif // HAVE_GSL

   return irest;
}




// Regularize *and* fit to density.
//
// Cut and pasted from above.  You might ask why I didn't factor out
// the common stuff.
//
// mmdb doesn't seem to lend itself to consiseness and because I
// didn't want mmdb_manager in graphics-info.h (which I would have
// needed in the factorization of the common elements).
//
// Note that this refinement routine uses moving_atoms_asc.
//
// Sigh.
// 
short int 
graphics_info_t::copy_mol_and_refine(int imol_for_atoms,
				     int imol_for_map,
				     int resno_1, 
				     int resno_2, 
				     std::string altconf,// use this (e.g. "A") or "".
				     std::string chain_id_1) {

   short int irest = 0; // make 1 when restraints found.

#ifdef HAVE_GSL

   int imol = imol_for_atoms;
   imol_moving_atoms = imol_for_atoms;  // for use when we accept the
			      // regularization and want to copy the
			      // coordinates back.
   
   // make the selection and build a new molecule inside restraints.

   short int have_flanking_residue_at_start = 0;
   short int have_flanking_residue_at_end = 0;
   short int have_disulfide_residues = 0;  // other residues are included in the
   // residues_mol for disphide
   // restraints.
   
   // 9 Sept 2003: The atom selection goes mad if residue with seqnum
   // iend_res+1 does not exist, but is not at the end of the chain.

   // Therefore we will set 2 flags, which tell us if istart_res-1 and
   // iend_res+1 exist.  And we do that by trying to select atoms from
   // them - if they exist, the number of selected atoms will be more
   // than 0.

//    istart_minus_flag = 0;  // from simple restraint code
//    iend_plus_flag    = 0;

   CMMDBManager *mol = molecules[imol].atom_sel.mol; // short-hand usage

   // We want to check for flanking atoms if the dictionary "group"
   // entry is not non-polymer.  So let's do a quick residue selection
   // of the first residue and find its residue type, look it up and
   // get the group.  If it is "non-polymer", then we can tinker with
   // the have_flanking_residue_at_* flags.

   int SelHnd_first = mol->NewSelection();
   int n_residue_first;
   PCResidue *residue_first = NULL;
   mol->Select(SelHnd_first, STYPE_RESIDUE, 0,
	       (char *) chain_id_1.c_str(),
	       resno_1, "*",
	       resno_1, "*",
	       "*",  // residue name
	       "*",  // Residue must contain this atom name?
	       "*",  // Residue must contain this Element?
	       "*",  // altLocs
	       SKEY_NEW); // selection key
   mol->GetSelIndex(SelHnd_first, residue_first, n_residue_first);
   std::string group = "L-peptide";
   if (n_residue_first > 0) {
      std::string residue_type_first = residue_first[0]->name;
      // does a dynamic add if needed.
      int status =
	 geom_p->have_dictionary_for_residue_type(residue_type_first,
						  cif_dictionary_read_number);
      std::pair<short int, coot::dictionary_residue_restraints_t> p =
	 geom_p->get_monomer_restraints(residue_type_first);
      if (p.first) {
	 group = p.second.residue_info.group;
      }
      cif_dictionary_read_number++;
   }
   mol->DeleteSelection(SelHnd_first);

   if (group != "non-polymer") { // i.e. it is (or can be) a polymer
      int SelHnd_ends = mol->NewSelection();
      int n_atoms_ends;
      PPCAtom atoms_end = 0;
      mol->SelectAtoms(SelHnd_ends, 0, (char *) chain_id_1.c_str(),
		       resno_1-1, "*", resno_1-1, "*","*","*","*","*");
      mol->GetSelIndex(SelHnd_ends, atoms_end, n_atoms_ends);
      if (n_atoms_ends > 0)
	 have_flanking_residue_at_start = 1; // we have residue istart_res-1
      mol->DeleteSelection(SelHnd_ends);

      SelHnd_ends = mol->NewSelection();
      mol->SelectAtoms(SelHnd_ends, 0, (char *) chain_id_1.c_str(),
		       resno_2+1, "*", resno_2+1, "*","*","*","*","*");
      mol->GetSelIndex(SelHnd_ends, atoms_end, n_atoms_ends);
      if (n_atoms_ends > 0)
	 have_flanking_residue_at_end = 1; // we have residue iend_res+1
      mol->DeleteSelection(SelHnd_ends);
   }


   // Consider as the altconf the altconf of one of the residues (we
   // must test that the altlocs of the selected atoms to be either
   // the same as each other (A = A) or one of them is "".  We need to
   // know the mmdb syntax for "either".
   // 
   // 
   // 
   int iselection_resno_start = resno_1;
   int iselection_resno_end   = resno_2;
   if (have_flanking_residue_at_start) iselection_resno_start--;
   if (have_flanking_residue_at_end)   iselection_resno_end++;
   //
   int selHnd = mol->NewSelection();
   int nSelResidues; 
   PCResidue *SelResidues = NULL;
   mol->Select(selHnd, STYPE_RESIDUE, 0,
	       (char *) chain_id_1.c_str(),
	       iselection_resno_start, "*",
	       iselection_resno_end, "*",
	       "*",  // residue name
	       "*",  // Residue must contain this atom name?
	       "*",  // Residue must contain this Element?
	       "*",  // altLocs
	       SKEY_NEW // selection key
	       );
//    std::cout << "Selecting from chain id " << chain_id_1 << std::endl;
//    std::cout << "selecting from residue " << iselection_resno_start
// 	     << " to " << iselection_resno_end << std::endl;
   molecules[imol].atom_sel.mol->GetSelIndex(selHnd, SelResidues, nSelResidues);

   std::pair<int, std::vector<std::string> > icheck = 
      check_dictionary_for_residues(SelResidues, nSelResidues);

   if (icheck.first == 0) { 
      for (unsigned int icheck_res=0; icheck_res<icheck.second.size(); icheck_res++) { 
	 std::cout << "WARNING:: Failed to find restraints for :" 
		   << icheck.second[icheck_res] << ":" << std::endl;
      }
   }

   std::cout << "INFO:: " << nSelResidues 
	     << " residues selected for refined and flanking residues"
	     << std::endl;

   if (nSelResidues > 0) {

      std::vector<CAtom *> fixed_atoms;
      char *chn = (char *) chain_id_1.c_str();

      if (nSelResidues > refine_regularize_max_residues) { 

	 std::cout << "WARNING:: Hit heuristic fencepost! Too many residues "
		   << "to refine\n "
		   << "          FYI: " << nSelResidues
		   << " > " << refine_regularize_max_residues
		   << " (which is your current maximum).\n";
	 std::cout << "Use (set-refine-max-residues "
		   << 2*refine_regularize_max_residues
		   << ") to increase limit\n";

      } else { 

	 // notice that we have to make 2 atom selections, one, which includes
	 // flanking (and disulphide) residues that is used for the restraints
	 // (restraints_container_t constructor) and one that is the moving atoms
	 // (which does not have flanking atoms).
	 // 
	 // The restraints_container_t moves the atom of the mol that is passes to
	 // it.  This must be the same mol as the moving atoms mol so that the
	 // changed atom positions can be seen.  However (as I said) the moving
	 // atom mol should not have flanking residues shown.  So we make an asc
	 // that has the same mol as that passed to the restraints but a different
	 // atom selection (it is the atom selection that is used in the bond
	 // generation).
	 //
	 short int in_alt_conf_split_flag = 0;
	 if (altconf != "")
	    in_alt_conf_split_flag = 1;
	    
	 CMMDBManager *residues_mol = 
	    create_mmdbmanager_from_res_selection(SelResidues, nSelResidues, 
						  have_flanking_residue_at_start,
						  have_flanking_residue_at_end,
						  altconf,
						  chain_id_1,
						  // 0, // 0 because we are not in alt conf split
						  in_alt_conf_split_flag, 
						  imol);

	 coot::restraints_container_t restraints(resno_1,
						 resno_2,
						 have_flanking_residue_at_start,
						 have_flanking_residue_at_end,
						 have_disulfide_residues,
						 altconf,
						 chn,
						 residues_mol,
						 fixed_atoms, 
						 molecules[imol_for_map].xmap_list[0],
                                                 geometry_vs_map_weight);

	 atom_selection_container_t local_moving_atoms_asc;
	 local_moving_atoms_asc.mol = (MyCMMDBManager *) residues_mol;
	 local_moving_atoms_asc.UDDOldAtomIndexHandle = -1;  // true?
	 local_moving_atoms_asc.UDDAtomIndexHandle = -1;

	 local_moving_atoms_asc.SelectionHandle = residues_mol->NewSelection();
	 residues_mol->SelectAtoms (local_moving_atoms_asc.SelectionHandle, 0, "*",
				   resno_1, // starting resno, an int
				   "*", // any insertion code
				   resno_2, // ending resno
				   "*", // ending insertion code
				   "*", // any residue name
				   "*", // atom name
				   "*", // elements
				   "*"  // alt loc.
				   );

	 residues_mol->GetSelIndex(local_moving_atoms_asc.SelectionHandle,
				   local_moving_atoms_asc.atom_selection,
				   local_moving_atoms_asc.n_selected_atoms);


	 // coot::restraint_usage_Flags flags = coot::BONDS;
	 // coot::restraint_usage_Flags flags = coot::BONDS_AND_ANGLES;
	 // coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_AND_PLANES;
	 // coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_TORSIONS_AND_PLANES; 
	 coot::restraint_usage_Flags flags = coot::BONDS_ANGLES_PLANES_AND_NON_BONDED;
	 // flags = coot::BONDS_ANGLES_PLANES_NON_BONDED_AND_CHIRAL;
	 flags = coot::BONDS_ANGLES_PLANES_AND_NON_BONDED;

	 short int do_link_torsions = 0;
	 short int do_residue_internal_torsions = 0;

	 if (do_torsion_restraints) { 
	    do_residue_internal_torsions = 1;
	    flags = coot::BONDS_ANGLES_TORSIONS_PLANES_AND_NON_BONDED;
	 } 
	    
	 if (do_peptide_torsion_restraints) {
	    do_link_torsions = 1;
	    flags = coot::BONDS_ANGLES_TORSIONS_PLANES_AND_NON_BONDED;
	 }

	 // coot::pseudo_restraint_bond_type pseudos = coot::NO_PSEUDO_BONDS;
	 coot::pseudo_restraint_bond_type pseudos = graphics_info_t::pseudo_bonds_type;
	 int nrestraints = 
	    restraints.make_restraints(*geom_p, flags,
				       do_residue_internal_torsions,
				       do_link_torsions, pseudos);
	 
	 if (nrestraints > 0) { 
	    irest = 1;
	    moving_atoms_asc_type = coot::NEW_COORDS_REPLACE;
	    last_restraints = restraints;

	    if (0) { 
	       restraints.minimize(flags);
	       regularize_object_bonds_box.clear_up();
	       make_moving_atoms_graphics_object(local_moving_atoms_asc); // sets moving_atoms_asc
	    } else {
	       regularize_object_bonds_box.clear_up();
	       make_moving_atoms_graphics_object(local_moving_atoms_asc); // sets moving_atoms_asc
	       short int continue_flag = 1;
	       int step_count = 0; 
	       print_initial_chi_squareds_flag = 1; // unset by drag_refine_idle_function
	       while (step_count < 10000 && continue_flag) { 
		  int retval = drag_refine_idle_function(NULL);
		  step_count += dragged_refinement_steps_per_frame;
		  if (retval == GSL_SUCCESS)
		     continue_flag = 0;
		  if (retval == GSL_ENOPROG)
		     continue_flag = 0;
	       }
	    }
	 } else { 
	    GtkWidget *widget = create_no_restraints_info_dialog();
	    gtk_widget_show(widget);
	 } 
      }
   } else {
      std::cout << "No Atoms!!!!  This should never happen: " << std::endl;
      std::cout << "  in create_regularized_graphical_object" << std::endl;
   } 
#else 
   std::cout << "Cannot refine without compilation with GSL" << std::endl;
#endif // HAVE_GSL

   return irest;

}


// Return 0 if any of the residues don't have a dictionary entry
// 
std::pair<int, std::vector<std::string> >
graphics_info_t::check_dictionary_for_residues(PCResidue *SelResidues, int nSelResidues) {

   std::pair<int, std::vector<std::string> > r;

   int status;
   int fail = 0; // not fail initially.

   for (int ires=0; ires<nSelResidues; ires++) { 
      std::string resname(SelResidues[ires]->name);
      if (resname == "UNK") resname = "ALA"; // hack for KC/buccaneer.
      if (resname.length() > 2)
	 if (resname[2] == ' ')
	    resname = resname.substr(0,2);
      status = geom_p->have_dictionary_for_residue_type(resname,
							cif_dictionary_read_number);
      cif_dictionary_read_number++;
      // This bit is redundant now that try_dynamic_add has been added
      // to have_dictionary_for_residue_type():
      if (status == 0) { 
	 status = geom_p->try_dynamic_add(resname, cif_dictionary_read_number++);
	 if (status == 0) {
	    fail = 1; // we failed to find it then.
	    r.second.push_back(resname);
	 }
      }
   }

   if (fail)
      r.first = 0;
   return r;
} 



// The flanking residues (if any) are in the residue selection (SelResidues).
// The flags are not needed now we have made adjustments in the calling
// function.
// 
// create_mmdbmanager_from_res_selection must make adjustments
// 
// Note: there is now a molecule-class-info version of this - perhaps
// we should call it?  Next bug fix here: move over to the function call.
// 
// 
CMMDBManager *
graphics_info_t::create_mmdbmanager_from_res_selection(PCResidue *SelResidues, 
						       int nSelResidues, 
						       int have_flanking_residue_at_start,
						       int have_flanking_residue_at_end, 
						       const std::string &altconf,
						       const std::string &chain_id_1,
						       short int residue_from_alt_conf_split_flag,
						       int imol) { 

   int start_offset = 0;
   int end_offset = 0;
   
//    if (have_flanking_residue_at_start)
//       start_offset = -1;
//    if (have_flanking_residue_at_end)
//       end_offset = +1; 

   CMMDBManager *residues_mol = new CMMDBManager;
   CModel *model = new CModel;
   CChain *chain = new CChain;
   short int whole_res_flag = 0; // not all alt confs, only this one ("A") and "".

   // For the active residue range (i.e. not the flanking residues) we only want
   // to refine the atoms that have the alt conf the same as the picked atom
   // (and that is altconf, passed here).
   // 
   // However, for *flanking residues* it's different.  Say we are refining a
   // non-split residue with alt conf "": Say that residue has a flanking
   // residue that is completely split, into A and B.  In that case we want
   // either "" or "A" for the flanking atoms.
   // 
   // And say we want to refine the A alt conf of a completely split residue
   // that has a flanking neighbour that is completely unsplit (""), we want
   // atoms that are either "A" or "".
   // 
   // So let's try setting whole_res_flag to 1 for flanking residues.

   CResidue *r;
   int atom_index_udd = molecules[imol].atom_sel.UDDAtomIndexHandle;
   for (int ires=start_offset; ires<(nSelResidues + end_offset); ires++) { 

      if ( (ires == 0) || (ires == nSelResidues -1) ) { 
	 if (! residue_from_alt_conf_split_flag)
	    whole_res_flag = 1;
      } else { 
	 whole_res_flag = 0;
      }

//       std::cout << "DEBUG in create_mmdbmanager_from_res_selection, whole_res_flag is "
// 		<< whole_res_flag << " for altconf " << altconf
// 		<< "\n       residue_from_alt_conf_split_flag "
// 		<< residue_from_alt_conf_split_flag << std::endl;
      
      r = coot::deep_copy_this_residue(SelResidues[ires], altconf, whole_res_flag, 
				       atom_index_udd);
      chain->AddResidue(r);
      r->seqNum = SelResidues[ires]->GetSeqNum();
      r->SetResName(SelResidues[ires]->GetResName());
   }
   chain->SetChainID(chain_id_1.c_str());
   model->AddChain(chain);
   residues_mol->AddModel(model);
   residues_mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
   residues_mol->FinishStructEdit();

   return residues_mol;
}



// on reading a pdb file, we get a list of residues, use these to
// load monomers from the dictionary, to be used in refinement.
// 
// This could be substantially speeded up, we currently do a most
// simple-minded search...  If both vectors were sorted on the
// monomer/residue name we could speed up a lot.
//
int
graphics_info_t::load_needed_monomers(const std::vector<std::string> &pdb_residue_types) { 

   int iloaded=0;

   for (unsigned int ipdb=0; ipdb<pdb_residue_types.size(); ipdb++) { 
      short int ifound = 0;
      for (int igeom=0; igeom<geom_p->size(); igeom++) {
	 if (pdb_residue_types[ipdb] == (*geom_p)[igeom].comp_id) { 
	    ifound = 1;
	    break;
	 }
      }
      if (ifound == 0) { 
	 // read in monomer for type pdb_residue_types[ipdb]
	 geom_p->try_dynamic_add(pdb_residue_types[ipdb],
				 cif_dictionary_read_number++);
	 iloaded++;
      } 
   }
   return iloaded;
}



void
graphics_info_t::regularize(int imol, short int auto_range_flag, int i_atom_no_1, int i_atom_no_2) { 

   // What are we going to do here:
   // 
   // How do we get the atom selection (the set of atoms that will be
   // refined)?  Just do a SelectAtoms() (see notes on atom selection). 
   //
   // Next flash the selection (to be refined).
   // How do we get the bonds for that?
   // 
   // Next, convert the atom selection to the output of refmac.  The
   // output of refmac has a different atom indexing.  We need to
   // construct a ppcatom with the same indexing as the refmac file.
   // We will do that as part of the idealization class (currently
   // simple-restraints)
   // 

   int tmp; 
   if (i_atom_no_1 > i_atom_no_2) { 
      tmp = i_atom_no_1; 
      i_atom_no_1 = i_atom_no_2;
      i_atom_no_2 = tmp; 
   }
   // now i_atom_no_2 is greater than i_atom_no_1.

   cout << "regularize: molecule " << imol << " atom index " << i_atom_no_1
	<< " to " << i_atom_no_2 << endl; 

   int resno_1, resno_2; 

   PPCAtom SelAtom = molecules[imol].atom_sel.atom_selection; 

   resno_1 = SelAtom[i_atom_no_1]->residue->seqNum;
   resno_2 = SelAtom[i_atom_no_2]->residue->seqNum;

   if (resno_1 > resno_2) { 
      tmp = resno_1;
      resno_1 = resno_2;
      resno_2 = tmp;
   } 

   std::string chain_id_1(SelAtom[i_atom_no_1]->residue->GetChainID());
   std::string chain_id_2(SelAtom[i_atom_no_2]->residue->GetChainID());
   std::string altconf(SelAtom[i_atom_no_2]->altLoc);

   if (auto_range_flag) { 
      // we want the residues that are +/- 1 [typically] from the residues that
      // contains the atom with the index i_atom_no_1.
      std::pair<int, int> p = auto_range_residues(i_atom_no_1, imol);
      resno_1 = p.first;
      resno_2 = p.second;
      // std::cout << "DEBUG:: auto_range_residues: " << resno_1 << " " << resno_2 << std::endl;
   } 

   // if ( chain_id_1 != chain_id_2 ) {
      // pointer comparison:
   if (SelAtom[i_atom_no_1]->GetChain() != SelAtom[i_atom_no_1]->GetChain()) {
      std::cout << "Picked atoms are not in the same chain.  Failure" << std::endl;
      std::cout << "FYI: chain ids are: \"" << chain_id_1
		<< "\" and \"" << chain_id_2 << "\"" << std::endl;
      cout << "Picked atoms are not in the same chain.  Failure" << endl; 
   } else { 
      flash_selection        (imol, resno_1, resno_2, altconf, chain_id_1);
      short int istat = copy_mol_and_regularize(imol, resno_1, resno_2, altconf, chain_id_1);
      if (istat) { 
	 graphics_draw();
	 if (! refinement_immediate_replacement_flag)
	    do_accept_reject_dialog("Regularization");
      }
      
   } // same chains test
}

std::pair<int, int> 
graphics_info_t::auto_range_residues(int atom_index, int imol) const { 
   std::pair<int, int> r;
   
   CAtom *this_atom =  molecules[imol].atom_sel.atom_selection[atom_index];
   CResidue *this_res = this_atom->residue;
   CChain *this_chain = this_res->chain;
   int resno = this_res->GetSeqNum();
   char *inscode = this_res->GetInsCode();
   
   CResidue *prev_res = this_chain->GetResidue(resno-refine_auto_range_step, inscode);
   CResidue *next_res = this_chain->GetResidue(resno+refine_auto_range_step, inscode);

   if (prev_res) { 
      r.first = resno-refine_auto_range_step;
   } else { 
      r.first = resno;
   }

   if (next_res) { 
      r.second = resno+refine_auto_range_step;
   } else { 
      r.second = resno;
   }
   
   return r;
} 


void
graphics_info_t::flash_selection(int imol,
				 int resno_1, 
				 int resno_2, 
				 std::string altconf,
				 std::string chain_id_1) { 

   // First make an atom selection of the residues selected to regularize.
   // 
   int selHnd = ((CMMDBManager *)molecules[imol].atom_sel.mol)->NewSelection();
   int nSelAtoms;
   PPCAtom SelAtom;
   char *chn = (char *) chain_id_1.c_str();

   ((CMMDBManager *)molecules[imol].atom_sel.mol)->SelectAtoms(selHnd, 0, 
							       // (char *) chain_id_1.c_str(),
							       chn,
							       resno_1,"*",resno_2, "*",
							       "*",      // RNames
							       "*","*",  // ANames, Elements
							       "*" );    // Alternate locations.


   ((CMMDBManager *)molecules[imol].atom_sel.mol)->GetSelIndex(selHnd,
							       SelAtom,
							       nSelAtoms);
   cout << nSelAtoms << " atoms selected to regularize from residue "
	<< resno_1 << " to " << resno_2 << " chain " << chn << endl;

   if (nSelAtoms) { 
      // now we can make an atom_selection_container_t with our new
      // atom selection that we will use to find bonds.

      atom_selection_container_t asc; 
      asc.mol = molecules[imol].atom_sel.mol; 
      asc.atom_selection = SelAtom; 
      asc.n_selected_atoms = nSelAtoms; 

      int fld = 0;
      Bond_lines_container bonds(asc, fld); // don't flash disulfides

      graphical_bonds_container empty_box; 
      graphical_bonds_container regular_box = bonds.make_graphical_bonds();
      
      int flash_length = residue_selection_flash_frames_number;

      if (glarea) { 
	 for (int iflash=0; iflash<2; iflash++) { 
	    regularize_object_bonds_box = regular_box; 
	    for (int i=0; i<flash_length; i++)
	       graphics_draw();
	    regularize_object_bonds_box = empty_box; 
	    for (int i=0; i<flash_length; i++)
	       graphics_draw();
	 }
      }
      regularize_object_bonds_box = empty_box; 

   } // atoms selected
   molecules[imol].atom_sel.mol->DeleteSelection(selHnd);
   graphics_draw();
}

void
graphics_info_t::refine(int imol, short int auto_range_flag, int i_atom_no_1, int i_atom_no_2) {
   int tmp; 
   if (i_atom_no_1 > i_atom_no_2) { 
      tmp = i_atom_no_1; 
      i_atom_no_1 = i_atom_no_2;
      i_atom_no_2 = tmp; 
   }
   // now i_atom_no_2 is greater than i_atom_no_1.

   cout << "refine (fit to map): molecule " << imol
	<< " atom index " << i_atom_no_1
	<< " to " << i_atom_no_2 << endl; 

   int resno_1, resno_2;

   int imol_map = Imol_Refinement_Map();
   if (imol_map == -1) { // magic number check,
      // if not -1, then it has been set by user

      show_select_map_dialog();

   } else { 

      PPCAtom SelAtom = molecules[imol].atom_sel.atom_selection; 

      resno_1 = SelAtom[i_atom_no_1]->GetSeqNum();
      resno_2 = SelAtom[i_atom_no_2]->GetSeqNum();

      if (auto_range_flag) { 
	 // we want the residues that are +/- 1 [typically] from the residues that
	 // contains the atom with the index i_atom_no_1.
	 std::pair<int, int> p = auto_range_residues(i_atom_no_1, imol);
	 resno_1 = p.first;
	 resno_2 = p.second;
      }

      if (resno_1 > resno_2) { 
	 tmp = resno_1;
	 resno_1 = resno_2;
	 resno_2 = tmp;
      }
      std::string chain_id_1(SelAtom[i_atom_no_1]->residue->GetChainID());
      std::string chain_id_2(SelAtom[i_atom_no_2]->residue->GetChainID());
      std::string altconf(SelAtom[i_atom_no_2]->altLoc);
      short int is_water_flag = 0;
      std::string resname_1(SelAtom[i_atom_no_1]->GetResName());
      std::string resname_2(SelAtom[i_atom_no_2]->GetResName());
      if (resname_1 == "WAT" || resname_1 == "HOH" ||
	  resname_2 == "WAT" || resname_2 == "HOH")
	 is_water_flag = 1;
      refine_residue_range(imol, chain_id_1, chain_id_2, resno_1, resno_2, altconf,
			   is_water_flag);
   }
}

// The calling function need to check that if chain_id_1 and
// chain_id_2 are not the same chain (CChain *), then we don't call
// this function.  We don't want to do an atom selection here (we can
// do that in copy_mol_and_refine), so we need to pass is_water_flag
// and the auto_range is determined by the calling function.  Here we
// are passed the results of any auto_range calculation.
// 
void
graphics_info_t::refine_residue_range(int imol,
				      const std::string &chain_id_1,
				      const std::string &chain_id_2,
				      int resno_1,
				      int resno_2,
				      const std::string &altconf,
				      short int is_water_flag) {

   int imol_map = Imol_Refinement_Map();
   if (imol_map == -1) { // magic number check,
      // if not -1, then it has been set by user
      show_select_map_dialog();
   } else { 

      //       if ( chain_id_1 != chain_id_2 ) {
      // Used to be pointer comparison, let that be done in the calling function.
      if (chain_id_1 != chain_id_2) {

	 // for now we will bug out.  In futre, we will want to be
	 // able to refine glycosylation.
	 //
	 std::cout << "Picked atoms are not in the same chain.  Failure" << std::endl;
	 std::cout << "FYI: chain ids are: \"" << chain_id_1
		   << "\" and \"" << chain_id_2 << "\"" << std::endl;
      } else {
	 if (molecules[imol_map].has_map()) {  // it may have been
					       // closed after it was
					       // selected.
	    short int simple_water = 0;
	    if (resno_1 == resno_2) {
	       if (is_water_flag) {
		  simple_water = 1;
// 		  std::string s = "That water molecule doesn't have restraints.\n";
// 		  s += "Using Rigid Body Fit Zone";
// 		  GtkWidget *w = wrapped_nothing_bad_dialog(s);
// 		  gtk_widget_show(w);
		  
		  // rigid body refine uses residue_range_atom_index_1
		  // and residue_range_atom_index_2, which should be
		  // defined in graphics-info-defines refine section.
		  //
		  imol_rigid_body_refine = imol;  // Fix GSK water refine crash.

		  // OK, in the simple water case, we do an atom selection

		  set_residue_range_refine_atoms(resno_1, resno_2, chain_id_1, imol);
		  // There are now set by that function:
		  // residue_range_atom_index_1 = i_atom_no_1;
		  // residue_range_atom_index_2 = i_atom_no_1; // refining just one atom.
		  
		  execute_rigid_body_refine(0); // no autorange for waters.
	       }
	    }
	    if (!simple_water) { 
	       flash_selection(imol, resno_1, resno_2, altconf, chain_id_1);
	       long t0 = glutGet(GLUT_ELAPSED_TIME);
	       short int istat = 
		  copy_mol_and_refine(imol, imol_map, resno_1, resno_2, altconf, chain_id_1);
	       long t1 = glutGet(GLUT_ELAPSED_TIME);
	       std::cout << "Refinement elapsed time: " << float(t1-t0)/1000.0 << std::endl;
	       if (istat) { 
		  graphics_draw();
		  if (! refinement_immediate_replacement_flag)
		     do_accept_reject_dialog("Refinement");
	       }
	    }
	 } else {
	    std::cout << "Can't refine to a closed map.  Choose another map"
		      << std::endl;
	    show_select_map_dialog();
	 } 
      }
   } // same chains test
}


// Question to self: Are you sure that imol_rigid_body_refine (the
// coordinates molecule) is set when we get here?
// 
// Also: are residue_range_atom_index_1 and residue_range_atom_index_2 set?
// They should be.
// 
void
graphics_info_t::execute_rigid_body_refine(short int auto_range_flag) { /* atom picking has happened.
						Actually do it */

   CAtom *atom1;
   CAtom *atom2;

   int ires1;  // set according to auto_range_flag
   int ires2;
   char *chain_id_1;
   char *chain_id_2 = 0;

   if (auto_range_flag) { 
      std::pair<int, int> p = auto_range_residues(residue_range_atom_index_1,
						  imol_rigid_body_refine);
      ires1 = p.first;
      ires2 = p.second;
      atom1 = molecules[imol_rigid_body_refine].atom_sel.atom_selection[residue_range_atom_index_1];
      chain_id_1 =  atom1->residue->GetChainID();
   } else { 
   
      // make sure that the atom indices are in the right order:
      // 
      if (residue_range_atom_index_1 > residue_range_atom_index_2) {
	 int tmp;
	 tmp = residue_range_atom_index_2; 
	 residue_range_atom_index_2 = residue_range_atom_index_1;
	 residue_range_atom_index_1 = tmp;
      }

      std::cout << "imol_rigid_body_refine " << imol_rigid_body_refine << std::endl;
      std::cout << "residue_range_atom_index_1 "
		<< residue_range_atom_index_1 << std::endl;
      std::cout << "residue_range_atom_index_2 "
		<< residue_range_atom_index_2 << std::endl;

      atom1 = molecules[imol_rigid_body_refine].atom_sel.atom_selection[residue_range_atom_index_1];
      atom2 = molecules[imol_rigid_body_refine].atom_sel.atom_selection[residue_range_atom_index_2];
      ires1 = atom1->residue->seqNum;
      ires2 = atom2->residue->seqNum;
      chain_id_1 =  atom1->residue->GetChainID();
      chain_id_2 =  atom2->residue->GetChainID();
   }

   // duck out now if the chains were not the same!
   if (chain_id_1 != chain_id_2) {
      std::string info_string("Atoms must be in the same chain");
      statusbar_text(info_string);
      return; 
   }
   
   std::string chain(chain_id_1);

   std::cout << " Rigid Body Refinement "
	     << " imol: " << imol_rigid_body_refine << " residue "
	     << ires1 << " to " << ires2 << " chain " << chain << std::endl;

   //

   coot::ligand lig;
   int imol_ref_map = Imol_Refinement_Map();  // -1 is a magic number

   if (Imol_Refinement_Map() == -1 ) { // magic number
      //
      std::cout << "Please set a map against which the refimentment should occur"
		<< std::endl;
      show_select_map_dialog();
   } else {
      lig.import_map_from(molecules[imol_ref_map].xmap_list[0], 
			  molecules[imol_ref_map].map_sigma());

      short int mask_water_flag = 0; // don't mask waters

      coot::minimol::molecule mol(molecules[imol_rigid_body_refine].atom_sel.mol);
       
      coot::minimol::molecule range_mol;
      int ir = range_mol.fragment_for_chain(chain);
      coot::minimol::residue empty_res;
	 
      for (unsigned int ifrag=0; ifrag<mol.fragments.size(); ifrag++) {
	 if (mol[ifrag].fragment_id == chain) {
	    for (int ires=mol.fragments[ifrag].min_res_no(); ires<=mol.fragments[ifrag].max_residue_number(); ires++) {
	       if (ires>=ires1 && ires<=ires2) {
		  // moving residue range
		  range_mol[ir].addresidue(mol[ifrag][ires],1); // Add even if empty.
		                                                // Sensible?
		  // make mol have an empty residue at this position
		  mol[ifrag][ires] = empty_res;
	       }
	    }
	 }
      }
      coot::minimol::molecule mol_without_moving_zone = mol;

      std::vector<coot::minimol::atom *> range_atoms = range_mol.select_atoms_serial();
      std::cout << "There are " << range_atoms.size() << " atoms from initial ligand " << std::endl;
      
      lig.install_ligand(range_mol);
      lig.find_centre_by_ligand(0); // don't test ligand size
      lig.mask_map(mol_without_moving_zone,mask_water_flag);
      lig.set_dont_write_solutions();
      lig.set_dont_test_rotations();
      lig.set_acceptable_fit_fraction(graphics_info_t::rigid_body_fit_acceptable_fit_fraction);
      lig.fit_ligands_to_clusters(1);
      coot::minimol::molecule moved_mol = lig.get_solution(0);

      std::vector<coot::minimol::atom *> atoms = moved_mol.select_atoms_serial();

      std::cout << "There are " << atoms.size() << " atoms from fitted zone."
		<< std::endl;
      
      // lig.make_pseudo_atoms(); uncomment for a clipper mmdb crash (sigh)

      // range_mol.write_file("range_mol.pdb");
      // mol_without_moving_zone.write_file("mol_without_moving_zone.pdb");

      // Fine.  Now we have to go back to using MMDB to interface with
      // the rest of the program.  So let's create an asc that has
      // this atom_sel.mol and the moving atoms as the
      // atom_selection. (c.f. accepting refinement or
      // regularization).

      if (atoms.size() > 0) { 

	 atom_selection_container_t rigid_body_asc;
// 	 rigid_body_asc.mol = (MyCMMDBManager *) moved_mol.pcmmdbmanager();

// 	 int SelHnd = rigid_body_asc.mol->NewSelection();
// 	 rigid_body_asc.mol->SelectAtoms(SelHnd, 0, "*",
// 					 ANY_RES, // starting resno, an int
// 					 "*", // any insertion code
// 					 ANY_RES, // ending resno
// 					 "*", // ending insertion code
// 					 "*", // any residue name
// 					 "*", // atom name
// 					 "*", // elements
// 					 "*"  // alt loc.
// 					 );
// 	 rigid_body_asc.mol->GetSelIndex(SelHnd,
// 					 rigid_body_asc.atom_selection,
// 					 rigid_body_asc.n_selected_atoms);
	 
	 rigid_body_asc = make_asc(moved_mol.pcmmdbmanager(default_new_atoms_b_factor));

	 moving_atoms_asc_type = coot::NEW_COORDS_REPLACE;
	 imol_moving_atoms = imol_rigid_body_refine;
	 make_moving_atoms_graphics_object(rigid_body_asc);
// 	 std::cout << "DEBUG:: execute_rigid_body_refine " 
// 		   << " make_moving_atoms_graphics_object UDDOldAtomIndexHandle " 
// 		   << moving_atoms_asc->UDDOldAtomIndexHandle << std::endl;
	 graphics_draw();
	 if (! refinement_immediate_replacement_flag)
	    do_accept_reject_dialog("Rigid Body Fit");
	 // 
      } else {
	 GtkWidget *w = create_rigid_body_refinement_failed_dialog();
	 gtk_widget_show(w);
      } 
   }
}


void
graphics_info_t::set_residue_range_refine_atoms(int resno_start, int resno_end,
						const std::string &chain_id, int imol) {

   // do 2 atom selections to find the atom indices
   if (imol < n_molecules) {
      if (molecules[imol].has_model()) {

	 // recall that we can't use the
	 // full_atom_spec_to_atom_index() of the class because we
	 // don't have an atom name here. 
	 
	 int ind_1 = -1, ind_2 = -1; // flags, for having found atoms
	 
	 int SelHnd = molecules[imol].atom_sel.mol->NewSelection();
	 PPCAtom selatoms;
	 int nselatoms;
	 molecules[imol].atom_sel.mol->SelectAtoms(SelHnd, 0, (char *) chain_id.c_str(),
						   resno_start, // starting resno, an int
						   "*", // any insertion code
						   resno_start, // ending resno
						   "*", // ending insertion code
						   "*", // any residue name
						   "*", // atom name
						   "*", // elements
						   "*"  // alt loc.
						   );
	 molecules[imol].atom_sel.mol->GetSelIndex(SelHnd, selatoms, nselatoms);
	 if (nselatoms > 0) {
	    if (selatoms[0]->GetUDData(molecules[imol].atom_sel.UDDAtomIndexHandle, ind_1) == UDDATA_Ok) {
	       residue_range_atom_index_1 = ind_1;
	    }
	 }
	 molecules[imol].atom_sel.mol->DeleteSelection(SelHnd);

	 // and again for the second atom seletion:
	 // 
	 SelHnd = molecules[imol].atom_sel.mol->NewSelection();
	 molecules[imol].atom_sel.mol->SelectAtoms(SelHnd, 0, (char *) chain_id.c_str(),
						 resno_end, // starting resno, an int
						 "*", // any insertion code
						 resno_end, // ending resno
						 "*", // ending insertion code
						 "*", // any residue name
						 "*", // atom name
						 "*", // elements
						 "*"  // alt loc.
						 );
	 molecules[imol].atom_sel.mol->GetSelIndex(SelHnd, selatoms, nselatoms);
	 if (nselatoms > 0) {
	    if (selatoms[0]->GetUDData(molecules[imol].atom_sel.UDDAtomIndexHandle, ind_2) == UDDATA_Ok) {
	       residue_range_atom_index_2 = ind_2;
	    }
	 }
	 molecules[imol].atom_sel.mol->DeleteSelection(SelHnd);

	 //   if (ind_1 >= 0 && ind_2 >= 0)
	 //     execute_rigid_body_refine(0); // not autorange
      }
   }
}


#include "residue_by_phi_psi.hh"

// The passed residue type is either N, C or (now [20031222]) M.
// 
void 
graphics_info_t::execute_add_terminal_residue(int imol, 
					      const std::string &terminus_type,
					      const CResidue *res_p,
					      const std::string &chain_id, 
					      const std::string &res_type,
					      short int immediate_addition_flag) {
   
   int imol_map = Imol_Refinement_Map();
   if (imol_map == -1) { 
      show_select_map_dialog();
   } else { 

      if (terminus_type == "not-terminal-residue") {
	 std::cout << "that residue was not at a terminus" << std::endl;
      } else {

	 imol_moving_atoms = imol;
	 coot::residue_by_phi_psi addres(molecules[imol].atom_sel.mol,
					 terminus_type, res_p, chain_id, res_type);

	 // std::cout << "DEBUG:: term_type: " << terminus_type << std::endl;

	 // This was for debugging, so that we get *some* solutions at least.
	 // 	
	 addres.set_acceptable_fit_fraction(0.0); //  the default is 0.5, I think

	 // map value over protein, stops rigid body refinement down
	 // into previous residue?  Yes, but only after I altered the
	 // scoring in ligand to use this masked map (and not the
	 // pristine_map as it has previously done).
	 //
	 // The mask value used to be -2.0, but what happened is that
	 // in low resolution maps, the N atom of the fragment "felt"
	 // some of the -2.0 because of interpolation/cubic-spline.
	 // So -2.0 seems too much for low res.  Let's try -1.0.
	 // 
	 float masked_map_val = -1.0;
	 addres.set_map_atom_mask_radius(1.2);
	 if (terminus_type == "MC" || terminus_type == "MN" ||
	     terminus_type == "singleton")
	    masked_map_val = 0.0; 
	 addres.set_masked_map_value(masked_map_val);   
	 addres.import_map_from(molecules[imol_map].xmap_list[0], 
				molecules[imol_map].map_sigma());

	 // This masked map will be the one that is used for rigid
	 // body refinement, unlike normal ligand class usage which
	 // uses the xmap_pristine.
	 // 

	 // old style mask by all the protein atoms - slow? (especially on sgis?)
	 // 	 short int mask_waters_flag = 1; 
	 //      addres.mask_map(molecules[imol].atom_sel.mol, mask_waters_flag);
	 //
	 PPCAtom atom_sel = NULL;
	 int n_selected_atoms = 0;
	 realtype radius = 8.0;  // more than enough for 2 residue mainchains.
	 int SelHndSphere = molecules[imol].atom_sel.mol->NewSelection();
	 CAtom *terminal_at = NULL;
	 std::string atom_name = "Unassigned";
	 if (terminus_type == "MC" || terminus_type == "C" ||
	     terminus_type == "singleton")
	    atom_name = " C  ";
	 if (terminus_type == "MN" || terminus_type == "N")
	    atom_name = " N  ";
	 if (atom_name != "Unassigned") { 
	    PPCAtom residue_atoms;
	    int nResidueAtoms;
	    CResidue *res_tmp_p = (CResidue *) res_p;
	    res_tmp_p->GetAtomTable(residue_atoms, nResidueAtoms);
	    for (int i=0; i<nResidueAtoms; i++)
	       if (atom_name == residue_atoms[i]->name) {
		  terminal_at = residue_atoms[i];
		  break;
	       }

	    if (terminal_at) { 
	       molecules[imol].atom_sel.mol->SelectSphere(SelHndSphere, STYPE_ATOM,
							  terminal_at->x,
							  terminal_at->y,
							  terminal_at->z,
							  radius, SKEY_NEW);
	       molecules[imol].atom_sel.mol->GetSelIndex(SelHndSphere, atom_sel, n_selected_atoms);
	       int invert_flag = 0;
	       addres.mask_map(molecules[imol].atom_sel.mol, SelHndSphere, invert_flag);
	       molecules[imol].atom_sel.mol->DeleteSelection(SelHndSphere);
	    }
	 } else {
	    std::cout << "WARNING:: terminal atom not assigned - no masking!" << std::endl;
	 }

	 
 	 // addres.output_map("terminal-residue.map");
	 // This bit can be deleted later:
	 // 
	 if (terminal_residue_do_rigid_body_refine) 
	    std::cout << "fitting terminal residue with rigid body refinement using " 
		      << add_terminal_residue_n_phi_psi_trials << " trials " 
		      << std::endl;
	 else
	    std::cout << "fitting terminal residue with " 
		      << add_terminal_residue_n_phi_psi_trials << " random trials" 
		      << std::endl;

  	 coot::minimol::molecule mmol = 
	    addres.best_fit_phi_psi(add_terminal_residue_n_phi_psi_trials, 
				    terminal_residue_do_rigid_body_refine,
				    add_terminal_residue_add_other_residue_flag);

	 std::vector<coot::minimol::atom *> mmatoms = mmol.select_atoms_serial();
	 // std::cout << "---- ----- mmol has " << mmatoms.size() 
	 // << " atoms" << std::endl;
	 mmol.check();

	 if (mmol.is_empty()) {
	    
	    // this should not happen:
	    std::cout <<  "WARNING: empty molecule: "
		      << "failed to find a fit for terminal residue"
		      << std::endl;

	 } else { 

	    // check that we are adding some atoms:
	    // 
	    std::vector<coot::minimol::atom *> mmatoms = mmol.select_atoms_serial();
	    if (mmatoms.size() == 0) { 
	       std::cout << "WARNING: failed to find a fit for terminal residue"
			 << std::endl;
	       GtkWidget *w = create_add_terminal_residue_finds_none_dialog();
	       gtk_widget_show(w);

	    } else { 

	       atom_selection_container_t terminal_res_asc;
	       float bf = default_new_atoms_b_factor;
	       terminal_res_asc.mol = (MyCMMDBManager *) mmol.pcmmdbmanager(bf);

	       int SelHnd = terminal_res_asc.mol->NewSelection();
	       terminal_res_asc.mol->SelectAtoms(SelHnd, 0, "*",
						 ANY_RES, // starting resno, an int
						 "*", // any insertion code
						 ANY_RES, // ending resno
						 "*", // ending insertion code
						 "*", // any residue name
						 "*", // atom name
						 "*", // elements
						 "*"  // alt loc.
						 );
	       terminal_res_asc.mol->GetSelIndex(SelHnd,
						 terminal_res_asc.atom_selection,
						 terminal_res_asc.n_selected_atoms);

	       // Now we add in the cb of this residue (currently it
	       // only has main chain atoms). This is somewhat
	       // involved - the methods to manipulate the standard
	       // residues are part of molecule_class_info_t - so we
	       // need to make an instance of that class.
	       // 
// 	       std::cout << "-------------- terminal_res_asc --------" << std::endl;
// 	       debug_atom_selection_container(terminal_res_asc);

	       // terminal_res_asc.mol->WritePDBASCII("terminal_res_asc.pdb");
	       
	       atom_selection_container_t tmp_asc = add_cb_to_terminal_res(terminal_res_asc);

// 	       std::cout << "-------------- tmp_asc --------" << std::endl;
// 	       debug_atom_selection_container(tmp_asc);

	       // If this is wrong also consider fixing execute_rigid_body_refine()
	       // 
// 	       std::cout << "debug: add_residue asc has n_selected_atoms = "
// 			 << terminal_res_asc.n_selected_atoms << " "
// 			 << terminal_res_asc.atom_selection << std::endl;

// 	       for (int i=0; i< terminal_res_asc.n_selected_atoms; i++) { 
// 		  std::cout << "debug: add_residue asc has chain_id: " 
// 			    << terminal_res_asc.atom_selection[i]->GetChainID()
// 			    << " for " << terminal_res_asc.atom_selection[i] 
// 			    << std::endl;
// 	       }

	       if (! immediate_addition_flag) { 
		  make_moving_atoms_graphics_object(tmp_asc);
		  moving_atoms_asc_type = coot::NEW_COORDS_INSERT;
		  graphics_draw();
		  do_accept_reject_dialog("Terminal Residue");
	       } else {
		  molecules[imol_moving_atoms].insert_coords(tmp_asc);
		  graphics_draw();
	       }
	    }
	 }
      }
   }
}




void
graphics_info_t::execute_rotate_translate_ready() { // manual movement


      
   CAtom *atom1 = molecules[imol_rot_trans_object].atom_sel.atom_selection[rot_trans_atom_index_1];
   CAtom *atom2 = molecules[imol_rot_trans_object].atom_sel.atom_selection[rot_trans_atom_index_2];

   char *altLoc = atom1->altLoc;

   char *chain_id_1 = atom1->GetChainID(); // not const, sigh.
   char *chain_id_2 = atom2->GetChainID(); // not const, sigh.

   if (chain_id_1 != chain_id_2) {
      std::string info_string("Atoms must be in the same chain");
      statusbar_text(info_string);
      
   } else {
      // OK, the atoms were in the same chain...
      GtkWidget *widget = create_rotate_translate_obj_dialog();
      GtkWindow *main_window = GTK_WINDOW(lookup_widget(graphics_info_t::glarea,
							"window1"));
      gtk_window_set_transient_for(GTK_WINDOW(widget), main_window);
      // setup_rotate_translate_buttons(widget); // old style buttons, not adjustments
      do_rot_trans_adjustments(widget);

      // set its position if it was shown before
      if (rotate_translate_x_position > -100) {
	 gtk_widget_set_uposition(widget,
				  rotate_translate_x_position,
				  rotate_translate_y_position);
      }
      gtk_widget_show(widget);

   int resno_1 = atom1->GetSeqNum();
      int resno_2 = atom2->GetSeqNum();

      if (resno_1 > resno_2) { 
	 int tmp = resno_1; 
	 resno_1 = resno_2;
	 resno_2 = tmp;
      }

      atom_selection_container_t rt_asc;
      // No! It cannot point to the same CAtoms.
      // rt_asc.mol = molecules[imol_rot_trans_object].atom_sel.mol; 
      // MyCMMDBManager *mol = new MyCMMDBManager;
      // mol->Copy(molecules[imol_rot_trans_object].atom_sel.mol, MMDBFCM_All);
      // how about we instead use:
      // CMMDBManager *mol = create_mmdbmanager_from_res_selection();
      //
      PCResidue *sel_residues = NULL;
      int n_sel_residues;
      int selHnd = molecules[imol_rot_trans_object].atom_sel.mol->NewSelection();
      molecules[imol_rot_trans_object].atom_sel.mol->Select(selHnd, STYPE_RESIDUE, 0,
							    chain_id_1,
							    resno_1, "*",
							    resno_2, "*",
							    "*",  // residue name
							    "*",  // Residue must contain this atom name?
							    "*",  // Residue must contain this Element?
							    "*",  // altLocs
							    SKEY_NEW // selection key
							    );
      molecules[imol_rot_trans_object].atom_sel.mol->GetSelIndex(selHnd, sel_residues, n_sel_residues);
      short int alt_conf_split_flag = 0;
      std::string altloc_string(altLoc);
      if (altloc_string != "")
	 alt_conf_split_flag = 1;

      // create a complete new clean copy of chains/residues/atoms
      std::pair<CMMDBManager *, int> mp = 
	 coot::util::create_mmdbmanager_from_res_selection(molecules[imol_rot_trans_object].atom_sel.mol,
							   sel_residues, n_sel_residues,
							   0, 0, altloc_string, chain_id_1,
							   alt_conf_split_flag);
      rt_asc = make_asc(mp.first);
      rt_asc.UDDOldAtomIndexHandle = mp.second;

      molecules[imol_rot_trans_object].atom_sel.mol->DeleteSelection(selHnd);

      //    std::cout << "DEBUG:: rt_asc: has n_selected_atoms: " << rt_asc.n_selected_atoms
      // 	     << std::endl;
      imol_moving_atoms = imol_rot_trans_object;
      moving_atoms_asc_type = coot::NEW_COORDS_REPLACE; 
      make_moving_atoms_graphics_object(rt_asc); // shallow copy rt_asc to moving_atoms_asc

      // set the rotation centre atom index:
      //   rot_trans_atom_index_rotation_origin_atom = 
      //       find_atom_index_in_moving_atoms(chain_id, 
      // 				      atom2->GetSeqNum(),
      // 				      atom2->name);  // uses moving_atoms_asc

      // This uses moving_atoms_asc internally, we don't need to pass it:
      coot::atom_spec_t atom_spec(atom2);
      rot_trans_rotation_origin_atom = find_atom_in_moving_atoms(atom_spec);
      //    std::cout << "DEBUG:: in execute_rotate_translate_read, found rotation atom: "
      // 	     << rot_trans_rotation_origin_atom << std::endl;
      
      if (rot_trans_rotation_origin_atom == NULL) { 
	 std::cout << "ERROR:: rot_trans_atom_rotation_origin not found" << std::endl;
      }
      graphics_draw();

      std::string info_string("Drag on an atom to translate residue, Ctrl Drag off atoms to rotate residue");
      statusbar_text(info_string);
   }
}


void 
graphics_info_t::do_rot_trans_adjustments(GtkWidget *dialog) { 

   std::vector<std::string> hscale_lab;
   
   hscale_lab.push_back("rotate_translate_obj_xtrans_hscale");
   hscale_lab.push_back("rotate_translate_obj_ytrans_hscale");
   hscale_lab.push_back("rotate_translate_obj_ztrans_hscale");
   hscale_lab.push_back("rotate_translate_obj_xrot_hscale");
   hscale_lab.push_back("rotate_translate_obj_yrot_hscale");
   hscale_lab.push_back("rotate_translate_obj_zrot_hscale");

// GtkObject *gtk_adjustment_new( gfloat value,
//                                gfloat lower,
//                                gfloat upper,
//                                gfloat step_increment,
//                                gfloat page_increment,
//                                gfloat page_size );

   for (unsigned int i=0; i<hscale_lab.size(); i++) { 
      GtkWidget *hscale = lookup_widget(dialog, hscale_lab[i].c_str());
      GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -180.0, 360.0, 0.1, 1.0, 0));
      gtk_range_set_adjustment(GTK_RANGE(hscale), GTK_ADJUSTMENT(adj));
      gtk_signal_connect(GTK_OBJECT(adj), 
			 "value_changed",
			 GTK_SIGNAL_FUNC(graphics_info_t::rot_trans_adjustment_changed), 
			 GINT_TO_POINTER(i));
   }
} 

// static 
void 
graphics_info_t::rot_trans_adjustment_changed(GtkAdjustment *adj, gpointer user_data) { 

   graphics_info_t g;  // because rotate_round_vector is not static - it should be.  
                       // FIXME at some stage.
   double v = adj->value;
   
   int i_hscale = GPOINTER_TO_INT(user_data);
   short int do_rotation;
   if (i_hscale < 3) 
      do_rotation = 0;
   else 
      do_rotation = 1;

   // std::cout << "rot_trans_adjustment_changed: user_data: " << i_hscale << std::endl;
   double x_diff = v;
   if ( previous_rot_trans_adjustment[i_hscale] > -9999) { 
      x_diff = v - previous_rot_trans_adjustment[i_hscale];
   }
   previous_rot_trans_adjustment[i_hscale] = v;

   // std::cout << "using  " << x_diff << "  " << v << "  " 
   //      << previous_rot_trans_adjustment[i_hscale] << std::endl;

   coot::Cartesian centre = unproject_xyz(0, 0, 0.5);
   coot::Cartesian front  = unproject_xyz(0, 0, 0.0);
   coot::Cartesian right  = unproject_xyz(1, 0, 0.5);
   coot::Cartesian top    = unproject_xyz(0, 1, 0.5);

   coot::Cartesian screen_x = (right - centre);
   coot::Cartesian screen_y = (top   - centre);
   coot::Cartesian screen_z = (front - centre);

   screen_x.unit_vector_yourself();
   screen_y.unit_vector_yourself();
   screen_z.unit_vector_yourself();

   float x_add = 0.0;
   float y_add = 0.0;
   float z_add = 0.0;

   if (i_hscale == 0) { 
      x_add = screen_x.x() * x_diff * 0.002 * zoom;
      y_add = screen_x.y() * x_diff * 0.002 * zoom;
      z_add = screen_x.z() * x_diff * 0.002 * zoom;
   }
   if (i_hscale == 1) { 
      x_add = screen_y.x() * x_diff * 0.002 * zoom;
      y_add = screen_y.y() * x_diff * 0.002 * zoom;
      z_add = screen_y.z() * x_diff * 0.002 * zoom;
   }
   if (i_hscale == 2) { 
      x_add = screen_z.x() * x_diff * 0.002 * zoom;
      y_add = screen_z.y() * x_diff * 0.002 * zoom;
      z_add = screen_z.z() * x_diff * 0.002 * zoom;
   }

   if (do_rotation) { 

      clipper::Coord_orth screen_vector; // the vector to rotate about
      if (i_hscale == 3) {
	 do_rotation = 1;
	 screen_vector = clipper::Coord_orth(screen_x.x(), 
					     screen_x.y(), 
					     screen_x.z());
      }
      if (i_hscale == 4) {
	 do_rotation = 1;
	 screen_vector = clipper::Coord_orth(screen_y.x(), 
					     screen_y.y(), 
					     screen_y.z());
      }
      if (i_hscale == 5) {
	 do_rotation = 1;
	 screen_vector = clipper::Coord_orth(screen_z.x(), 
					     screen_z.y(), 
					     screen_z.z());
      }
   

      // int indx = rot_trans_atom_index_rotation_origin_atom;
      CAtom *rot_centre = rot_trans_rotation_origin_atom;
      clipper::Coord_orth rotation_centre(rot_centre->x, 
					  rot_centre->y, 
					  rot_centre->z);

      for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
	 clipper::Coord_orth co(moving_atoms_asc->atom_selection[i]->x,
				moving_atoms_asc->atom_selection[i]->y,
				moving_atoms_asc->atom_selection[i]->z);
	 clipper::Coord_orth new_pos = 
	    g.rotate_round_vector(screen_vector, co, rotation_centre, x_diff * 0.018);
	 moving_atoms_asc->atom_selection[i]->x = new_pos.x();
	 moving_atoms_asc->atom_selection[i]->y = new_pos.y();
	 moving_atoms_asc->atom_selection[i]->z = new_pos.z();
      }
   } else { 

      for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
	 moving_atoms_asc->atom_selection[i]->x += x_add;
	 moving_atoms_asc->atom_selection[i]->y += y_add;
	 moving_atoms_asc->atom_selection[i]->z += z_add;
      }
   }
   int do_disulphide_flag = 0;

   if (molecules[imol_moving_atoms].Bonds_box_type() == coot::CA_BONDS ||
       molecules[imol_moving_atoms].Bonds_box_type() == coot::CA_BONDS_PLUS_LIGANDS ||
       molecules[imol_moving_atoms].Bonds_box_type() == coot::CA_BONDS_PLUS_LIGANDS_SEC_STRUCT_COLOUR ||
       molecules[imol_moving_atoms].Bonds_box_type() == coot::COLOUR_BY_RAINBOW_BONDS) {
      
      Bond_lines_container bonds;
      bonds.do_Ca_bonds(*moving_atoms_asc, 1.0, 4.7);
      regularize_object_bonds_box.clear_up();
      regularize_object_bonds_box = bonds.make_graphical_bonds();
   } else {
      Bond_lines_container bonds(*moving_atoms_asc, do_disulphide_flag);
      regularize_object_bonds_box.clear_up();
      regularize_object_bonds_box = bonds.make_graphical_bonds();
   }
   graphics_draw();
} 


// Old style buttons for rotate/translate
// 
// Delete me (and in graphics-info.h of course).
void 
graphics_info_t::setup_rotate_translate_buttons(GtkWidget *window) { 

   std::vector<std::string> butts;
   butts.push_back("rotate_translate_obj_xtrans_button");
   butts.push_back("rotate_translate_obj_ytrans_button");
   butts.push_back("rotate_translate_obj_ztrans_button");
   butts.push_back("rotate_translate_obj_xrot_button");
   butts.push_back("rotate_translate_obj_yrot_button");
   butts.push_back("rotate_translate_obj_zrot_button");

//    for (int i=0; i<butts.size(); i++) {
//       button = lookup_widget(GTK_WIDGET(window), butts[i].c_str());
//       coot::rottrans_buttons::setup_button(button, butts[i]);
//    }

}

void
graphics_info_t::execute_db_main() { 

   int imol = db_main_imol;
   CAtom *at1 = molecules[imol].atom_sel.atom_selection[db_main_atom_index_1];
   CAtom *at2 = molecules[imol].atom_sel.atom_selection[db_main_atom_index_2];
   std::string chain_id = at1->GetChainID();
   int iresno_start = at1->GetSeqNum();
   int iresno_end   = at2->GetSeqNum();

   std::string direction_string("forwards"); // forwards
   execute_db_main(imol, chain_id, iresno_start, iresno_end, direction_string);
}

// 
void
graphics_info_t::execute_db_main(int imol,
				 std::string chain_id,
				 int iresno_start,
				 int iresno_end,
				 std::string direction_string) { 
   
   int ilength = 6;
   int idbfrags = 0;
   
   if (main_chain.is_empty()) {
      idbfrags = main_chain.fill_with_fragments(ilength);
   }

   // should be filled now
   // 
   if (main_chain.is_empty()) { 
      std::cout << "Sorry cannot do a db fitting without reference structures"
		<< std::endl;
      std::string s("Sorry cannot do a main-chain fitting without reference structures");
      wrapped_nothing_bad_dialog(s);
   } else { 

      if (iresno_start > iresno_end) { 
	 int tmp = iresno_end;
	 iresno_end = iresno_start;
	 iresno_start = tmp;
      }

      // mt is a minimol of the Baton Atoms:
      coot::minimol::molecule mt(molecules[imol].atom_sel.mol);
      coot::minimol::molecule target_ca_coords;

      if (direction_string != "backwards") { 
	 for (unsigned int i=0; i<mt.fragments.size(); i++)
	    if (mt.fragments[i].fragment_id == chain_id)
	       target_ca_coords.fragments.push_back(mt.fragments[i]);
      } else { // backwards code.
	 for (unsigned int i=0; i<mt.fragments.size(); i++)
	    if (mt[i].fragment_id == chain_id) {
	       // put in the residues of mt.fragments[i] backwards:
	       
	       // The seqnum of the residues is ignored, the only
	       // important thing is the ires.
	       
	       int ifrag = target_ca_coords.fragment_for_chain(chain_id);
	       if (mt[i].max_residue_number() > 1) { 
		  for (int ires=mt[i].max_residue_number(); ires>=mt[i].min_res_no(); ires--) {
		     target_ca_coords[ifrag].residues.push_back(mt[ifrag][ires]);
		  }
		  break;
	       }
	    }
      }

      // now target_ca_coords has only one chain, the chain of the zone.
      // Not that match_target_fragment selects CAs from target_ca_coords
      // so we don't need to filter them out here. 

      main_chain.match_target_fragment(target_ca_coords,
				       iresno_start,
				       iresno_end,
				       ilength);

      main_chain.merge_fragments();
      coot::minimol::molecule mol;
      mol.fragments.push_back(main_chain.mainchain_fragment());
      mol.write_file("db-mainchain.pdb", 20.0); 
      if (!mol.is_empty()) {
	 std::pair<std::vector<float>, std::string> cell_spgr = 
	    molecules[imol].get_cell_and_symm();
	 float bf = default_new_atoms_b_factor;
	 atom_selection_container_t asc = make_asc(mol.pcmmdbmanager(bf));
	 set_mmdb_cell_and_symm(asc, cell_spgr); // tinker with asc. 
	                                         // Consider asc as an object.
	 molecules[n_molecules].install_model(asc, "mainchain", 1);
	 // molecules[n_molecules].set_mmdb_cell_and_symm(cell_spgr);
	 n_molecules++;
	 graphics_draw();
      } else {
	 std::string s("Sorry, failed to convert that residue range.\nToo short, perhaps?");
	 wrapped_nothing_bad_dialog(s);
      }
      main_chain.clear_results();
   }
}

// --------------------------------------------------------------------------------
//                 Rotamer stuff
// --------------------------------------------------------------------------------

void
graphics_info_t::do_rotamers(int atom_index, int imol) {
   
   // display the buttons for the rotamer options and display
   // the most likely in the graphics as a
   // moving_atoms_asc.

   rotamer_residue_atom_index = atom_index;  // save for button
					     // callbacks, so that we
					     // can get the residue.
   rotamer_residue_imol = imol;
   std::string altconf = molecules[imol].atom_sel.atom_selection[atom_index]->altLoc;
   
   GtkWidget *window = create_rotamer_selection_dialog();
   set_transient_and_position(COOT_ROTAMER_SELECTION_DIALOG, window);
   rotamer_dialog = window;

   // Test if this was an alt confed atom.
   // If it was, then we should set up the hscale.
   // It it was not, then we should destroy the hscale
   if (altconf.length() > 0) {
      GtkWidget *hscale = lookup_widget(window, "new_alt_conf_occ_hscale");
      float v = add_alt_conf_new_atoms_occupancy;
     // The max value is 3rd arg - 6th arg (here 2 and 1 is the same as 1 and 0)
      GtkAdjustment *adj = GTK_ADJUSTMENT(gtk_adjustment_new(v, 0.0, 2.0, 0.01, 0.1, 1.0));
      gtk_range_set_adjustment(GTK_RANGE(hscale), GTK_ADJUSTMENT(adj));
      gtk_signal_connect(GTK_OBJECT(adj), 
			 "value_changed",
			 GTK_SIGNAL_FUNC(graphics_info_t::new_alt_conf_occ_adjustment_changed), 
			 NULL);
      
   } else {
      GtkWidget *frame = lookup_widget(window, "new_alt_conf_occ_frame");
      gtk_widget_destroy(frame);
   }

   
   /* Events for widget must be set before X Window is created */
   gtk_widget_set_events(GTK_WIDGET(window),
			 GDK_KEY_PRESS_MASK);
   /* Capture keypress events */
//    rotamer_key_press_event is not defined (yet)
//    gtk_signal_connect(GTK_OBJECT(window), "key_press_event",
// 		      GTK_SIGNAL_FUNC(rotamer_key_press_event), NULL);
   /* set focus to glarea widget - we need this to get key presses. */
   GTK_WIDGET_SET_FLAGS(window, GTK_CAN_FOCUS);
   gtk_widget_grab_focus(GTK_WIDGET(glarea)); // but set focus to the graphics.
   
   fill_rotamer_selection_buttons(window, atom_index, imol);

   // act as if the button for the first rotamer was pressed
   short int stat = generate_moving_atoms_from_rotamer(0);

   if (stat)
      gtk_widget_show(window);
}


// static
void graphics_info_t::new_alt_conf_occ_adjustment_changed(GtkAdjustment *adj,
							  gpointer user_data) {

   graphics_info_t g;
   g.add_alt_conf_new_atoms_occupancy = adj->value;

   // Change the occupancies of the intermediate atoms:
   //
   if (moving_atoms_asc) {
      for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
	 // this if test is a kludge!
	 // Don't change the alt conf for fully occupied atoms.
	 if (moving_atoms_asc->atom_selection[i]->occupancy < 0.99) 
	    moving_atoms_asc->atom_selection[i]->occupancy = adj->value;
      }
   }
}


void
graphics_info_t::fill_rotamer_selection_buttons(GtkWidget *window, int atom_index, int imol) const {

   // for each rotamer do this:

   GtkWidget *rotamer_selection_radio_button;
   GtkWidget *rotamer_selection_dialog = window;
   GtkWidget *rotamer_selection_button_vbox = lookup_widget(window,
							    "rotamer_selection_button_vbox");
   GSList *gr_group = NULL;

   graphics_info_t g;
   CResidue *residue = g.molecules[imol].atom_sel.atom_selection[atom_index]->residue;
   
      
   coot::dunbrack d(residue, g.molecules[imol].atom_sel.mol, g.rotamer_lowest_probability, 0);

   std::vector<float> probabilities = d.probabilities();
   // std::cout << "There are " << probabilities.size() << " probabilities"
   // << std::endl;

   // Attach the number of residues to the dialog so that we can get
   // that data item when we make a synthetic key press due to
   // keyboard (arrow?) key press:
   gtk_object_set_user_data(GTK_OBJECT(window), GINT_TO_POINTER(probabilities.size()));

   GtkWidget *frame;
   for (unsigned int i=0; i<probabilities.size(); i++) {
      std::string button_label = int_to_string(i+1);
      button_label += ":  ";
      button_label += float_to_string(probabilities[i]);
      button_label += "% Chi_1 = ";
      button_label += float_to_string(d.Chi1(i));
      std::string button_name = "rotamer_selection_button_rot_";
      button_name += int_to_string(i);
   
      rotamer_selection_radio_button =
	 gtk_radio_button_new_with_label (gr_group, button_label.c_str());
      gr_group = gtk_radio_button_group (GTK_RADIO_BUTTON (rotamer_selection_radio_button));
      gtk_widget_ref (rotamer_selection_radio_button);
      gtk_object_set_data_full (GTK_OBJECT (rotamer_selection_dialog),
				button_name.c_str(), rotamer_selection_radio_button,
				(GtkDestroyNotify) gtk_widget_unref);
      
      int *iuser_data = new int;
      *iuser_data = i;
      gtk_signal_connect (GTK_OBJECT (rotamer_selection_radio_button), "toggled",
			  GTK_SIGNAL_FUNC (on_rotamer_selection_button_toggled),
			  iuser_data);
       
       gtk_widget_show (rotamer_selection_radio_button);
       frame = gtk_frame_new(NULL);
       gtk_container_add(GTK_CONTAINER(frame), rotamer_selection_radio_button);
       gtk_box_pack_start (GTK_BOX (rotamer_selection_button_vbox),
			   frame, FALSE, FALSE, 0);
       gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
       gtk_widget_show(frame);
   }
} 


void
graphics_info_t::on_rotamer_selection_button_toggled (GtkButton       *button,
						      gpointer         user_data) {

   int *i_tmp = (int *) user_data;
   int i = *i_tmp;
   
   graphics_info_t g;
   g.generate_moving_atoms_from_rotamer(i);
   
} 

// Return 1 for valid (i.e. non-GLY, non-ALA) residue, 0 otherwise
// (including residue type not found).
// 
short int 
graphics_info_t::generate_moving_atoms_from_rotamer(int irot) {

   int imol = rotamer_residue_imol;
   int atom_index = rotamer_residue_atom_index; 

   CAtom    *at_rot   = molecules[imol].atom_sel.atom_selection[atom_index];
   CResidue *residue  = molecules[imol].atom_sel.atom_selection[atom_index]->residue;
   int atom_index_udd = molecules[imol].atom_sel.UDDAtomIndexHandle;

   if (std::string(residue->name) == "GLY" ||
       std::string(residue->name) == "ALA") {
      std::cout << "INFO:: This residue ("<< residue->name
		<< ") doesn't have rotamers\n";
      return 0;
   }

   // We need to filter out atoms that are not (either the same
   // altconf as atom_index or "")
   // 
   CResidue *tres = coot::deep_copy_this_residue(residue, 
						 std::string(at_rot->altLoc),
						 0, atom_index_udd);
   PPCAtom residue_atoms;
   int nResidueAtoms;
   std::string mol_atom_altloc;
   std::string atom_altloc = molecules[imol].atom_sel.atom_selection[atom_index]->altLoc;
   tres->GetAtomTable(residue_atoms, nResidueAtoms);
   for (int iat=0; iat<nResidueAtoms; iat++) {
      mol_atom_altloc = std::string(residue_atoms[iat]->altLoc);
      if (! ((mol_atom_altloc ==  atom_altloc) || (mol_atom_altloc == ""))) { 
	 tres->DeleteAtom(iat);
      }
   }
   tres->TrimAtomTable();

   coot::dunbrack d(tres, molecules[imol].atom_sel.mol,
		    rotamer_lowest_probability, 0);

   // std::cout << "generate_moving_atoms_from_rotamer " << irot << std::endl;

   // The magic happens here:
   CResidue *moving_res = d.GetResidue(irot);
   //
   if (moving_res == NULL) {
      std::cout << "Failure to find rotamer for residue type: "
		<< residue->name << std::endl;
      return 0;
   } else { 

      MyCMMDBManager *mol = new MyCMMDBManager;
      CModel *model_p = new CModel;
      CChain *chain_p = new CChain;
      CResidue *res_p = new CResidue;
      res_p->seqNum = ((CResidue *)residue)->GetSeqNum();
      strcpy(res_p->name, residue->name);
   
      PPCAtom residue_atoms_2;
      int nResidueAtoms_2;
      ((CResidue *)moving_res)->GetAtomTable(residue_atoms_2, nResidueAtoms_2);
      CAtom *atom_p;
      int i_add;
      for(int iat=0; iat<nResidueAtoms_2; iat++) {
	 atom_p = new CAtom;
	 atom_p->Copy(residue_atoms_2[iat]);
	 i_add = res_p->AddAtom(atom_p);
      }
      chain_p->AddResidue(res_p);
      chain_p->SetChainID(residue->GetChainID());
      model_p->AddChain(chain_p);
      mol->AddModel(model_p);
      mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
      mol->FinishStructEdit();

      imol_moving_atoms = imol;
      *moving_atoms_asc = make_asc(mol);
      //    std::cout << "there are " << moving_atoms_asc->n_selected_atoms
      // 	     << " selected atoms in the moving_atoms_asc" << std::endl;

      moving_atoms_asc_type = coot::NEW_COORDS_REPLACE;
      make_moving_atoms_graphics_object(*moving_atoms_asc);
      graphics_draw();
      return 1;
   }
}



// wiggly ligands support
// 
std::vector <coot::dict_torsion_restraint_t>
graphics_info_t::get_monomer_torsions_from_geometry(const std::string &monomer_type) const {
   return geom_p->get_monomer_torsions_from_geometry(monomer_type, find_hydrogen_torsions);
} 

// Each new atom goes in its own residue.
// Residue type is HOH.
// 
void
graphics_info_t::place_dummy_atom_at_pointer() {

   int imol = pointer_atom_molecule();
   molecules[imol].add_pointer_atom(RotationCentre()); // update bonds
   graphics_draw();

}

void 
graphics_info_t::place_typed_atom_at_pointer(const std::string &type) { 

   int imol = pointer_atom_molecule();
   if (imol >= 0 && imol < n_molecules) {
      if (molecules[imol].open_molecule_p()) { 
	 molecules[imol].add_typed_pointer_atom(RotationCentre(), type); // update bonds
	 graphics_draw();
      } else {
	 std::cout << "ERROR:: invalid (closed) molecule in place_typed_atom_at_pointer: "
		   << imol << std::endl;
      }
   } else {
      std::cout << "WARNING:: invalid molecule number in place_typed_atom_at_pointer"
		<< imol << std::endl;
   }
}

// Tinker with the atom positions of residue
// Return 1 on success.
// We need to pass the asc for the mol because we need it for seekcontacts()
// Of course the asc that is passed is the moving atoms asc.
// 
short int 
graphics_info_t::update_residue_by_chi_change(CResidue *residue,
					      atom_selection_container_t &asc,
					      int nth_chi, double diff) {

   // add phi/try rotamers? no.
   coot::chi_angles chi_ang(residue, 0);
   
   int nResidueAtoms = residue->GetNumberOfAtoms();
      
   // Contact indices:
   //
   coot::contact_info contact = coot::getcontacts(asc);

   std::vector<std::vector<int> > contact_indices;

   // Change the coordinates in the residue to which this object contains a
   // pointer:
   // 
   // Return non-zero on failure:
   // change_by() generates the contact indices (trying regular_residue = 1)
   std::pair<short int, float> istat = chi_ang.change_by(nth_chi, diff, geom_p);

   if (istat.first) { // failure

      // std::cout << "DEBUG:: Simple chi change failed" << std::endl;

      // Hack, hack.  See comments on get_contact_indices_from_restraints.
      // 
      contact_indices = coot::util::get_contact_indices_from_restraints(residue, geom_p, 0);
      // try looking up the residue in 
      istat = chi_ang.change_by(nth_chi, diff, contact_indices, geom_p,
				chi_angles_clicked_atom_spec,
				find_hydrogen_torsions);
   }

   setup_flash_bond_internal(nth_chi);

   if (istat.first == 0) { // success.
      display_density_level_this_image = 1;
      display_density_level_screen_string = "  Chi ";
      display_density_level_screen_string += int_to_string(nth_chi);
      display_density_level_screen_string += "  =  ";
      display_density_level_screen_string += float_to_string( (180.8 /M_PI) * istat.second);
      statusbar_text(display_density_level_screen_string);
   } 

   return istat.first;
} 



// Called by mouse motion callback (in_edit_chi_mode_flag)
// 
void
graphics_info_t::rotate_chi(double x, double y) {

   // real values start at 1:
   int chi = edit_chi_current_chi;

   mouse_current_x = x;
   mouse_current_y = y;
   double diff;

   diff  = mouse_current_x - GetMouseBeginX();
   diff += mouse_current_y - GetMouseBeginY();

   diff *= 0.5;

   // std::cout << "graphics_info_t::rotate_chi " << chi << " by "
   // << diff << std::endl;

   // c.f. generate_moving_atoms_from_rotamer(i), except here we will
   // not be changing our moving_atoms_asc, just updating the atom
   // positions.
   //

   short int istat = 1; // failure
   if (! moving_atoms_asc) {
      std::cout << "ERROR: moving_atoms_asc is NULL" << std::endl;
   } else { 
      if (moving_atoms_asc->n_selected_atoms == 0) {
	 std::cout << "ERROR: no atoms in moving_atoms_asc" << std::endl;
      } else { 
	 CModel *model_p = moving_atoms_asc->mol->GetModel(1);
	 if (model_p) {
	    CChain *chain_p = model_p->GetChain(0);
	    if (chain_p) {
	       CResidue *residue_p = chain_p->GetResidue(0);
	       if (residue_p) {
		  istat = update_residue_by_chi_change(residue_p, *moving_atoms_asc, chi, diff);
	       }
	    }
	 }
      }
   }
   
   if (istat == 0) {
      // std::cout << "regenerating object" << std::endl;
      regularize_object_bonds_box.clear_up();
      make_moving_atoms_graphics_object(*moving_atoms_asc); // make new bonds
      graphics_draw();

      //    } else {
      // std::cout << "chi rotate failed  - not regenerating object" << std::endl;
      
   } 
   
}

// 	 //  debug:
// 	 std::cout << "DEBUG:: residue_mol: ----------------- " << std::endl;
// 	 CModel *model_p = residues_mol->GetModel(1);
// 	 CChain *chain_p;
// 	 int nchains = model_p->GetNumberOfChains();
// 	 std::cout << "DEBUG:: residue_mol: nchains " << nchains << std::endl;
// 	 for (int ichain=0; ichain<nchains; ichain++) { 
// 	    chain_p = model_p->GetChain(ichain);
// 	    int nres = chain_p->GetNumberOfResidues();
// 	    for (int ires=0; ires<nres; ires++) { 
// 	       PCResidue residue_p = chain_p->GetResidue(ires);
// 	       std::cout << "DEBUG:: residue " << residue_p->GetChainID()
// 			 << " " << residue_p->GetSeqNum()
// 			 << " " << residue_p->name
// 			 << std::endl;
// 	    }
// 	 } 
 


// If we are splitting a residue, we may need move the altLoc of the
// existing residue from "" to "A". Let's create a new enumerated
// constant NEW_COORDS_INSERT_CHANGE_ALTCONF to flag that.
// 
// What's in the residue     What we clicked   Old Coordinates   New Coordinates
//      ""                        ""                "" -> "A"       "B"
//    "A" "B"                     "A"              no change        "C"
//    "A" "B"                     "B"              no change        "C"
//    "" "A" "B"                  ""               [1]              "C"
//    "" "A" "B"                  "A"              [1]              "C"
//    "" "A" "B"                  "B"              [1]              "C"
//
// [1] depends on the split:
//     whole residue split: "" -> "A" , "A" and "B" remain the same
//     partial split:       no change
// 
// Now that I think about it, it doesn't matter which atom we click.
// 
// ....Oh but it does if we want to follow Stefano's suggestion and
// split at clicked residue...
//
// This is a molecule-class-info function.  What is it doing here?
// It's not here any more.  This is just a wrapper.
// 
void 
graphics_info_t::split_residue(int imol, int atom_index) {

   // do moving molecule atoms:
   // short int do_intermediate_atoms = 0;

   // Actually, we don't want intermediate atoms in the usual case.
   //
   // We *do* want intermediate atoms if the user has set the flag so,
   // or if there are not all the necessary (mainchain) atoms to do a
   // rotamer.
   //
   // What are the issues for split position?  None.  We do however
   // need to know the what the alt conf (and atom spec) of a newly
   // created alt conf atom.

   if (imol<n_molecules) { 
      if (molecules[imol].has_model()) {
	 graphics_info_t::molecules[imol].split_residue(atom_index);
	 graphics_draw();
      } else {
	 std::cout << "WARNING:: split_residue: molecule has no model.\n";
      }
   } else {
      std::cout << "WARNING:: split_residue: no such molecule.\n";
   }
}

void 
graphics_info_t::split_residue(int imol, std::string &chain_id, 
			       int resno, 
			       std::string &altconf) { 

} 

void
graphics_info_t::split_residue_range(int imol, int index_1, int index2) {

}
  

// delete zone
void
graphics_info_t::delete_residue_range(int imol,
				      const coot::residue_spec_t &res1,
				      const coot::residue_spec_t &res2) {

   molecules[imol].delete_zone(res1, res2);
   if (delete_item_widget) {
      GtkWidget *checkbutton = lookup_widget(graphics_info_t::delete_item_widget,
					     "delete_item_keep_active_checkbutton");
      if (GTK_TOGGLE_BUTTON(checkbutton)->active) {
	 // don't destroy it.
      } else {
	 gint upositionx, upositiony;
	 gdk_window_get_root_origin (delete_item_widget->window, &upositionx, &upositiony);
	 delete_item_widget_x_position = upositionx;
	 delete_item_widget_y_position = upositiony;
	 gtk_widget_destroy(delete_item_widget);
	 delete_item_widget = 0;
	 normal_cursor();
      }
   }

   if ((imol >=0) && (imol < n_molecules)) {
      graphics_info_t::molecules[imol].delete_zone(res1, res2);
      if (graphics_info_t::go_to_atom_window) {
	 update_go_to_atom_window_on_changed_mol(imol);
      }
   }
   graphics_draw();
}


// static
void
graphics_info_t::move_molecule_here_item_select(GtkWidget *item,
						GtkPositionType pos) {

   graphics_info_t::move_molecule_here_molecule_number = pos;

} 


void
graphics_info_t::do_probe_dots_on_rotamers_and_chis() {

   do_interactive_probe();
}


void
graphics_info_t::do_interactive_probe() const { 

#ifdef USE_GUILE
   if (moving_atoms_asc->n_selected_atoms > 0) {
      if (moving_atoms_asc->mol) {
	 moving_atoms_asc->mol->WritePDBASCII("molprobity-tmp-moving-file.pdb");
	 std::string c = "(";
	 c += "interactive-probe ";
	 c += float_to_string(probe_dots_on_chis_molprobity_centre.x());
	 c += " ";
	 c += float_to_string(probe_dots_on_chis_molprobity_centre.y());
	 c += " ";
	 c += float_to_string(probe_dots_on_chis_molprobity_centre.z());
	 c += " ";
	 c += float_to_string(probe_dots_on_chis_molprobity_radius);
	 c += " \"";
	 c += moving_atoms_asc->atom_selection[0]->GetChainID();
	 c += "\" ";
	 c += int_to_string(moving_atoms_asc->atom_selection[0]->GetSeqNum());
	 c += ")";
	 std::cout << "interactive probe debug: " << c << std::endl;
	 scm_c_eval_string(c.c_str());
      }
   }

#else
#ifdef USE_PYTHON

   if (moving_atoms_asc->n_selected_atoms > 0) {
      if (moving_atoms_asc->mol) {
	 moving_atoms_asc->mol->WritePDBASCII("molprobity-tmp-moving-file.pdb");
	 std::string c = "";
	 c += "interactive_probe(";
	 c += float_to_string(probe_dots_on_chis_molprobity_centre.x());
	 c += ", ";
	 c += float_to_string(probe_dots_on_chis_molprobity_centre.y());
	 c += ", ";
	 c += float_to_string(probe_dots_on_chis_molprobity_centre.z());
	 c += ", ";
	 c += float_to_string(probe_dots_on_chis_molprobity_radius);
	 c += ", \"";
	 c += moving_atoms_asc->atom_selection[0]->GetChainID();
	 c += "\", ";
	 c += int_to_string(moving_atoms_asc->atom_selection[0]->GetSeqNum());
	 c += ")";
	 PyRun_SimpleString((char *)c.c_str());
      }
   }
   
#endif // USE_PYTHON
#endif // USE_GUILE

   
}
