/* src/graphics-info-mutate.cc
 * 
 * Copyright 2004, 2005 by The University of York
 * Copyright 2008, 2009 by The University of Oxford.
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
 * Foundation, Inc.,  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// For stat, mkdir:
#include <sys/types.h>
#include <sys/stat.h>

#if !defined _MSC_VER
#include <unistd.h>
#else
#define PKGDATADIR "C:/coot/share"
#endif

#ifndef HAVE_STRING
#define HAVE_STRING
#include <string>
#endif

#ifndef HAVE_VECTOR
#define HAVE_VECTOR
#include <vector>
#endif


#include <iostream>

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h"

#include "Cartesian.h"
#include "Bond_lines.h"

#include <gtk/gtk.h>  // must come after mmdb_manager on MacOS X Darwin
#include <GL/glut.h>  // for some reason...  // Eh?

#include "interface.h"

#include "coot-coord-utils.hh"
#include "molecule-class-info.h"

#include "coot-sysdep.h"

#include "graphics-info.h"
#include "manipulation-modes.hh"

// #include "coot-utils.hh"


// This should be in coot-coord-utils.
// 
atom_selection_container_t
graphics_info_t::add_side_chain_to_terminal_res(atom_selection_container_t asc,
						std::string res_type) {

   atom_selection_container_t rasc = asc; 
   int istat;
   molecule_class_info_t molci;
   short int display_in_display_control_widget_status = 0;
   molci.install_model(0, asc, "terminal residue", display_in_display_control_widget_status);

   // every (usually 1, occasionally 2) residue in the molecule
   CModel *model_p = asc.mol->GetModel(1);
   
   CChain *chain;
   // run over chains of the existing mol
   int nchains = model_p->GetNumberOfChains();
   if (nchains <= 0) { 
      std::cout << "bad nchains in add_cb_to_terminal_res: "
		<< nchains << std::endl;
   } else {

      //std::string target_res_type("ALA");
      std::string target_res_type(res_type);
      CResidue *std_res = molci.get_standard_residue_instance(target_res_type);

      if (std_res == NULL) {
	 std::cout << "WARNING:: Can't find standard residue for " 
                   << target_res_type << "\n";
      } else { 
      
	 for (int ichain=0; ichain<nchains; ichain++) {
	    chain = model_p->GetChain(ichain);
	    if (chain == NULL) {  
	       // This should not be happen.
	       std::cout << "NULL chain in add_cb_to_terminal_res" << std::endl;
	    } else { 
	       CResidue *std_res_copy = coot::deep_copy_this_residue(std_res, "", 1, -1);
	       if (std_res_copy) { 
		  int nres = chain->GetNumberOfResidues();
		  for (int ires=0; ires<nres; ires++) { 
		     PCResidue residue_p = chain->GetResidue(ires);
		     //
		     std::cout << "INFO:: mutating residue in add_cb_to_terminal_res\n";
		     istat = molci.move_std_residue(std_res_copy, residue_p);

		     if (istat) { 

			PPCAtom residue_atoms;
			int nResidueAtoms;
			residue_p->GetAtomTable(residue_atoms, nResidueAtoms);

			PPCAtom std_residue_atoms;
			int n_std_ResidueAtoms;
			std_res_copy->GetAtomTable(std_residue_atoms, n_std_ResidueAtoms);

			// set the b factor for the new atoms.
			for(int i=0; i<n_std_ResidueAtoms; i++) {
			   std_residue_atoms[i]->tempFactor = default_new_atoms_b_factor;
			};
		     
			bool verb = 0;
			if (verb) { 
			   std::cout << "Mutate Atom Tables" << std::endl;
			   std::cout << "Before" << std::endl;
			   for(int i=0; i<nResidueAtoms; i++) {
			      std::cout << residue_atoms[i] << std::endl;
			   };

			   std::cout << "To be replaced by:" << std::endl;
			   for(int i=0; i<n_std_ResidueAtoms; i++) {
			      std::cout << std_residue_atoms[i] << std::endl;
			   }
			}

			for(int i=0; i<nResidueAtoms; i++) {
			   std::string residue_this_atom (residue_atoms[i]->name);
			   if (residue_this_atom != " O  ")
			      residue_p->DeleteAtom(i);
			};

			for(int i=0; i<n_std_ResidueAtoms; i++) {
			   std::string std_residue_this_atom (std_residue_atoms[i]->name);
			   if (std_residue_this_atom != " O  ") {
			      // std::cout << "Adding atom " << std_residue_atoms[i] << std::endl;
			      residue_p->AddAtom(std_residue_atoms[i]);
			   } 
			};
			// strcpy(residue_p->name, std_res->name);
			residue_p->TrimAtomTable();
		     }
		  }
	       }

	       molci.atom_sel.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
	       molci.atom_sel.mol->FinishStructEdit();
	       
	       rasc = make_asc(molci.atom_sel.mol);
	       
// 	       for (int ii=0; ii<rasc.n_selected_atoms; ii++) 
// 		  std::cout << "New res with cb: " << rasc.atom_selection[ii] << "\n";
	    }
	 }
      }
   }
   return rasc; 
}

// We need the action of Add Alt Conf... to provide a new alt conf in
// the the most likely (rotamer 0) position.
//
// The problem is that the moving residues from an "Add Alt Conf" are
// not necessarily a whole residue.  We may need to dig out the main
// chain atoms too.
//
// Also, we have a moving atoms, so we don't need to generate a new
// one as we do above in generate_moving_atoms_from_rotamer
//
// Or do we want to act as if "Rotamers" was pressed at the end of an
// "Add Alt Conf" - yes, I think that we do... How does that work then?
// We simply need the an atom index of an atom in the new alt conf.
//
// atom_spec_to_atom_index? 
// So I need to know the spec of a newly created atom
// 

void
graphics_info_t::do_mutation(const std::string &residue_type, short int do_stub_flag) {

   if (residue_type_chooser_auto_fit_flag) {
      molecules[mutate_auto_fit_residue_imol].mutate(mutate_auto_fit_residue_atom_index, 
						     residue_type, do_stub_flag);

      // 20071005 No longer check for stub state.  It doesn't make
      // sense to autofit a stub.  So ignore the stub state and just
      // autofit as normal.
      
      int imol_map = Imol_Refinement_Map();
      if (imol_map >= 0) {
	    
	 // float f =
	 int mode = graphics_info_t::rotamer_search_mode;
	 molecules[mutate_auto_fit_residue_imol].auto_fit_best_rotamer(mode,mutate_auto_fit_residue_atom_index, imol_map, rotamer_fit_clash_flag, rotamer_lowest_probability, *Geom_p());

	 if (mutate_auto_fit_do_post_refine_flag) {
	    // Run refine zone with autoaccept, autorange on
	    // the "clicked" atom:
	    CAtom *at = molecules[mutate_auto_fit_residue_imol].atom_sel.atom_selection[mutate_auto_fit_residue_atom_index];
	    std::string chain_id = at->GetChainID();
	    short int auto_range = 1;
	    refine(mutate_auto_fit_residue_imol, auto_range,
		   mutate_auto_fit_residue_atom_index,
		   mutate_auto_fit_residue_atom_index);
	 }

	 // This is the wrong function, isn't it?
	 update_go_to_atom_window_on_changed_mol(mutate_residue_imol);

	 run_post_manipulation_hook(mutate_auto_fit_residue_imol, MUTATED);
	    
      } else { 
	    
	 // imol map chooser
	 show_select_map_dialog();
      }
   } else {
      // simple mutation
      molecules[mutate_residue_imol].mutate(mutate_residue_atom_index, residue_type,
					    do_stub_flag); 
      update_go_to_atom_window_on_changed_mol(mutate_residue_imol);
      run_post_manipulation_hook(mutate_auto_fit_residue_imol, MUTATED);

   }
   graphics_draw();
}

void
graphics_info_t::do_mutation_auto_fit(const std::string &residue_type,
				      short int do_stub_flag) {

   molecules[mutate_residue_imol].mutate(mutate_residue_atom_index, residue_type,
					 do_stub_flag); 
   graphics_draw();
   run_post_manipulation_hook(mutate_residue_imol, MUTATED);
}


void
graphics_info_t::read_standard_residues() {

   std::string standard_env_dir = "COOT_STANDARD_RESIDUES";
   
   const char *filename = getenv(standard_env_dir.c_str());
   if (! filename) {

      std::string standard_file_name = PKGDATADIR;
      standard_file_name += "/";
      standard_file_name += "standard-residues.pdb";

      struct stat buf;
      int status = stat(standard_file_name.c_str(), &buf);  
      if (status != 0) { // standard-residues file was not found in
	                 // default location either...
	 std::cout << "WARNING: Can't find standard residues file in the "
		   << "default location \n";
	 std::cout << "         and environment variable for standard residues ";
	 std::cout << standard_env_dir << "\n";
	 std::cout << "         is not set.";
	 std::cout << " Mutations will not be possible\n";
	 // mark as not read then:
	 standard_residues_asc.read_success = 0;
	 standard_residues_asc.n_selected_atoms = 0;
	 // std::cout << "DEBUG:: standard_residues_asc marked as empty" << std::endl;
      } else { 
	 // stat success:
	 standard_residues_asc = get_atom_selection(standard_file_name);
      }
   } else { 
      standard_residues_asc = get_atom_selection(filename);
   }
} 



void
graphics_info_t::mutate_chain(int imol, const std::string &chain_id,
			      const std::string &seq,
			      bool do_auto_fit_flag,
			      bool renumber_residues_flag) {

   if (imol < n_molecules()) { 
      if (imol >= 0) { 
	 if (molecules[imol].has_model()) {
	    std::cout << "INFO:: aligning to mol number " << imol << "chain: "
		      << chain_id << std::endl;
	    coot::chain_mutation_info_container_t mutation_info = 
	       molecules[imol].align_and_mutate(chain_id, coot::fasta(seq), renumber_residues_flag);

	    info_dialog_alignment(mutation_info);

	    if (do_auto_fit_flag) {
	       int imol_map = Imol_Refinement_Map();
	       if (is_valid_map_molecule(imol_map)) {


		  // For now let's refine the whole chain
		  //		  
                  std::vector<std::string> s;
                  s.push_back("fit-chain");
                  s.push_back(coot::util::int_to_string(imol));
                  s.push_back(coot::util::single_quote(chain_id));
#ifdef USE_GUILE
                  safe_scheme_command(schemize_command_strings(s));
#endif // USE_GUILE
		  

		  // This doesn't work (yet) because after alignment,
		  // the residue numbers of the mutations do not
		  // correspond the structure (because of insertions
		  // and deletions).
		  // 
		  // =============================================
		  if (0) { 
		     if (mutation_info.mutations.size()) {
			int replacement_state = refinement_immediate_replacement_flag;
			refinement_immediate_replacement_flag = 1;
			for (unsigned int i=0; i<mutation_info.mutations.size(); i++) { 
			   coot::residue_spec_t res_spec = mutation_info.mutations[i].first;
			   // now autofit rotamer on res_spec.
			
			   int mode = graphics_info_t::rotamer_search_mode;
			   int clash_flag = 1;
			   float lowest_probability = 0.01;
			   std::string altloc = "";
			   molecules[imol].auto_fit_best_rotamer(mode,
								 res_spec.resno,
								 altloc,
								 res_spec.insertion_code,
								 res_spec.chain,
								 imol_map,
								 clash_flag,
								 lowest_probability,
								 *Geom_p());
			   short int auto_range = 1;
			   refine_residue_range(imol,
						res_spec.chain, res_spec.chain,
						res_spec.resno, res_spec.insertion_code,
						res_spec.resno, res_spec.insertion_code,
						altloc, 0);
		  
			}
			refinement_immediate_replacement_flag = replacement_state;
		     }
		  } // end non-working block. =====================
		  
	       } else {
		  std::cout << "WARNING:: refinement map set " << std::endl;
	       } 
	    } 
	 }
      }
   }
}



// --------------------------------------
// not really mutation, but hey ho...
// ------- cis trans conversion ---------
void
graphics_info_t::cis_trans_conversion(CAtom *at, int imol, short int is_N_flag) {

   if (molecules[imol].has_model()) { 
      int istatus = molecules[imol].cis_trans_conversion(at, is_N_flag);
      if (istatus > 0)
	 graphics_draw();
   }
}
