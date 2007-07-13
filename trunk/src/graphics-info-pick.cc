/* src/graphics-info-pick.cc
 * 
 * Copyright 2004, 2005 by Paul Emsley, The University of York
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <string>
#include <vector> // for mmdb-crystal

#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "cos-sin.h"

#include "mmdb_manager.h"
#include "mmdb-extras.h"
#include "mmdb.h"
#include "mmdb-crystal.h" //need for Bond_lines now

#include "Cartesian.h"
#include "Bond_lines.h"

#include "graphics-info.h"

#include "molecule-class-info.h"

#include "cos-sin.h"

#include "globjects.h"

#include "pick.h"


// 
coot::Symm_Atom_Pick_Info_t
graphics_info_t::symmetry_atom_pick() const { 

   coot::Cartesian screen_centre = RotationCentre();

   coot::Cartesian front = unproject(0.0);
   coot::Cartesian back  = unproject(1.0);
   // Cartesian centre_unproj = unproject(0.5); // not needed, use midpoint of front and back.
   coot::Cartesian mid_point = front.mid_point(back);
   float dist_front_to_back = (front - back).amplitude();

   coot::Symm_Atom_Pick_Info_t p_i;
   p_i.success = GL_FALSE; // no atom found initially.
   float dist, min_dist = 0.4;

   for (int imol=0; imol<graphics_info_t::n_molecules; imol++) {

      if (graphics_info_t::molecules[imol].atom_selection_is_pickable()) { 
      
	 if (graphics_info_t::molecules[imol].has_model()) {
	    atom_selection_container_t atom_sel = graphics_info_t::molecules[imol].atom_sel;
	    molecule_extents_t mol_extents(atom_sel, symmetry_search_radius);
	    std::vector<std::pair<symm_trans_t, Cell_Translation> > boxes =
	       mol_extents.which_boxes(screen_centre, atom_sel);

	    if (boxes.size() > 1) {

	       char *spacegroup_str = atom_sel.mol->GetSpaceGroup();
	       if (!spacegroup_str) {
		  std::cout << "ERROR:: null spacegroup_str in symmetry pick\n";
	       } else {
		  std::string s( spacegroup_str );
		  // delete[] spacegroup_str; // no! crash - it's mmdb's I guess.

		  // we need to use init() because the compiler will
		  // choke on fill_hybrid_atoms if we don't (for some
		  // reason).
		  clipper::Spacegroup spg;
		  clipper::Spgr_descr sgd;
		  short int spacegroup_ok = 1;
// 		  std::cout << "DEBUG:: Initing Spgr_desc(\"" << s
// 			    << "\", type=HM)" << std::endl;
		  try {
		     sgd = clipper::Spgr_descr(s, clipper::Spgr_descr::HM);
		  } catch ( clipper::Message_base exc ) {
		     std::cout << "Oops, trouble.  No such spacegroup\n";
		     spacegroup_ok = 0;
		  }
		  if (spacegroup_ok == 0) {

		     // OK, Let's try to feed clipper strings:
		     s = "";
		     for ( int i = 0; i < atom_sel.mol->GetNumberOfSymOps(); i++ ) {
			char* symop_str = atom_sel.mol->GetSymOp(i);
			s += std::string( symop_str );
			s += ";";
			// delete[] symop_str; is mmdb just giving us
			// a pointer, not copying? comment out for
			// safety for now.
		     }
		     spacegroup_ok = 1;
		     try {
			sgd = clipper::Spgr_descr(s, clipper::Spgr_descr::Symops);
		     } catch ( clipper::Message_base exc ) {
			std::cout << "Oops-we're in double trouble.  No spacegroup found from symops "
				  << s << std::endl;
			spacegroup_ok = 0;
		     }
		     
		     if (spacegroup_ok == 0) {
			std::string sbt = "WARNING:: Trouble! No such spacegroup as \"";
			sbt += s;
			sbt += "\"";
			statusbar_text(sbt);
		     }
		  }
		  if (spacegroup_ok == 1) { 
// 		     std::cout << "DEBUG:: Initing clipper::Spacegroup: "
// 			       << spg.symbol_hm() << std::endl;
		     spg.init(sgd);
		     // spg.debug();
		     clipper::Cell cell(clipper::Cell_descr(atom_sel.mol->get_cell().a,
							    atom_sel.mol->get_cell().b,
							    atom_sel.mol->get_cell().c,
							    atom_sel.mol->get_cell().alpha,
							    atom_sel.mol->get_cell().beta,
							    atom_sel.mol->get_cell().gamma)); 

		     // we want to generate all the symmetry atoms for each box
		     // and find the atom with the closest approach.

		     std::vector<coot::clip_hybrid_atom> hybrid_atom(atom_sel.n_selected_atoms);

// 		     std::cout << "symm_atom_pick: there are " << boxes.size() << " boxes" << std::endl;
// 		     std::cout << "Here are the boxes: " << std::endl;
// 		     for (int ii=0; ii< boxes.size(); ii++) {
// 			std::cout << ii << " " << boxes[ii].first << " " << boxes[ii].second  << std::endl;
// 		     } 

		     for (unsigned int ii=0; ii< boxes.size(); ii++) {

			// What are the symmetry atoms for this box?

			// Notice that we do the unusual step of creating the
			// vector here and modifying it via a pointer - this is
			// for speed.
			// 
			fill_hybrid_atoms(&hybrid_atom, atom_sel, spg, cell, boxes[ii]);
			int n = hybrid_atom.size();
			for(int i=0; i<n; i++) { 
			   dist = hybrid_atom[i].pos.distance_to_line(front, back);

			   // 		  if (boxes[ii].isym() == 3 && boxes[ii].x() == 0 && boxes[ii].y() == 0 && boxes[ii].z() == 0) {
			   // 			std::cout << "selected box: " << hybrid_atom[i].atom << " "
			   // 				  << hybrid_atom[i].pos << " "
			   // 				  << "scrcent " << screen_centre << std::endl;
			   // 		  }
		  
			   if (dist < min_dist) {

			      float dist_to_rotation_centre = (hybrid_atom[i].pos - screen_centre).amplitude();
			      // std::cout << "dist_to rotation_centre " << dist_to_rotation_centre << " " 
			      //  			       << hybrid_atom[i].atom << " "
			      // 			       << hybrid_atom[i].pos << " "
			      // 			       << "scrcent " << screen_centre << " "
			      //  			       << boxes[ii].isym() << " " 
			      //  			       << boxes[ii].x() << " " << boxes[ii].y() << " " 
			      //  			       << boxes[ii].z() << " "  << std::endl;

			      if (dist_to_rotation_centre < symmetry_search_radius ||
				  molecules[imol].symmetry_whole_chain_flag || molecules[imol].symmetry_as_calphas) {
				 // 			std::cout << "updating min_dist to " << dist << " " 
				 // 				  << hybrid_atom[i].atom << " "
				 // 				  << boxes[ii].isym() << " " 
				 // 				  << boxes[ii].x() << " " << boxes[ii].y() << " " 
				 // 				  << boxes[ii].z() << " "  << std::endl;

				 // how do we know we are not picking
				 // something behind the back clipping
				 // plane?  e.g. we are zoomed in or have
				 // narrow clipping planes, we don't want to
				 // pick symmetry atoms that we can't see.

				 float front_picked_atom_line_length = (front - hybrid_atom[i].pos).amplitude();
				 float  back_picked_atom_line_length = (back  - hybrid_atom[i].pos).amplitude();
			   
				 // std::cout << "comparing " << front_picked_atom_line_length << " to "
				 // << dist_front_to_back << " to " << back_picked_atom_line_length
				 // << std::endl;

				 // This test make things better, but still
				 // not right, it seem, so lets artificially
				 // slim down the limits - so that we don't
				 // get so many false positives.
			   
				 if ( (front_picked_atom_line_length < dist_front_to_back) &&
				      ( back_picked_atom_line_length < dist_front_to_back)
				      ) { 
				    min_dist = dist;
				    p_i.success = GL_TRUE;
				    p_i.hybrid_atom = hybrid_atom[i];
				    // and these for labelling:
				    p_i.atom_index = i;
				    p_i.imol = imol; 
				    p_i.symm_trans = boxes[ii].first;
				    p_i.pre_shift_to_origin = boxes[ii].second;
				 }
			      }
			   }
			}
		     }
		  }
	       }
	    }
	 }
      }
   }

   if (p_i.success == GL_TRUE) {
      
      // make sure that we are looking at the molecule that had
      // the nearest contact:
      // 
      atom_selection_container_t SelAtom = graphics_info_t::molecules[p_i.imol].atom_sel; 
      
      std::string alt_conf_bit("");
      CAtom *at = molecules[p_i.imol].atom_sel.atom_selection[p_i.atom_index];
      if (strncmp(at->altLoc, "", 1))
	 alt_conf_bit=std::string(",") + std::string(at->altLoc);
      
      std::cout << "(" << p_i.imol << ") " 
		<< SelAtom.atom_selection[p_i.atom_index]->name 
		<< alt_conf_bit << "/"
		<< SelAtom.atom_selection[p_i.atom_index]->GetModelNum()
		<< "/chainid=\""
		<< SelAtom.atom_selection[p_i.atom_index]->GetChainID()  << "\"/"
		<< SelAtom.atom_selection[p_i.atom_index]->GetSeqNum()   
		<< SelAtom.atom_selection[p_i.atom_index]->GetInsCode()  << "/"
		<< SelAtom.atom_selection[p_i.atom_index]->GetResName()
		<< ", occ: " 
		<< SelAtom.atom_selection[p_i.atom_index]->occupancy 
		<< " with B-factor: "
		<< SelAtom.atom_selection[p_i.atom_index]->tempFactor
		<< " element: "
		<< SelAtom.atom_selection[p_i.atom_index]->element
		<< " at " << translate_atom(SelAtom, p_i.atom_index, p_i.symm_trans) << std::endl;
   }

   if (p_i.success) {
      coot::Cartesian tpos = translate_atom(molecules[p_i.imol].atom_sel,
					    p_i.atom_index,
					    p_i.symm_trans);
      std::string ai;
      CAtom *at = molecules[p_i.imol].atom_sel.atom_selection[p_i.atom_index];
      std::string alt_conf_bit("");
      if (strncmp(at->altLoc, "", 1))
	 alt_conf_bit=std::string(",") + std::string(at->altLoc);
      ai += "(mol. no: ";
      ai += graphics_info_t::int_to_string(p_i.imol);
      ai += ") ";
      ai += at->name;
      ai += alt_conf_bit;
      ai += "/";
      ai += graphics_info_t::int_to_string(at->GetModelNum());
      ai += "/";
      ai += at->GetChainID();
      ai += "/";
      ai += graphics_info_t::int_to_string(at->GetSeqNum());
      ai += at->GetInsCode();
      ai += " ";
      ai += at->GetResName();
      ai += " [";
      ai += p_i.symm_trans.str(1);
      ai += "] occ: ";
      ai += graphics_info_t::float_to_string(at->occupancy);
      ai += " bf: ";
      ai += graphics_info_t::float_to_string(at->tempFactor);
      ai += " ele: ";
      ai += at->element;
      ai += " pos: (";
      ai += graphics_info_t::float_to_string(tpos.x());
      ai += ",";
      ai += graphics_info_t::float_to_string(tpos.y());
      ai += ",";
      ai += graphics_info_t::float_to_string(tpos.z());
      ai += ")";
      gtk_statusbar_push(GTK_STATUSBAR(graphics_info_t::statusbar),
			 graphics_info_t::statusbar_context_id,
			 ai.c_str());
   }

//    std::cout << "Picked: status: " << p_i.success << " index: "<< p_i.atom_index
// 	     << " " << p_i.symm_trans << " "
// 	     << p_i.pre_shift_to_origin << std::endl;

   return p_i;
}

// This presumes that the mmdb symop order is the same as the clipper
// symop order [no longer true 12Nov2003]
// 
// private 
//
// So, to recap what happened here: It turns out, that unlike I
// expected, the clipper::Spacegroup::symops() don't come in the same
// order as those of mmdb.  So we have to construct the symop (really
// an RTop_orth here) from the mmdb matrix and transform the
// coordinates with this rtop.  A bit ugly, eh? 
// 
// But on the plus side, it works, no mind-boggling memory allocation
// problems and it's quite fast compared to the old version.
//
void 
graphics_info_t::fill_hybrid_atoms(std::vector<coot::clip_hybrid_atom> *hybrid_atoms, 
				   const atom_selection_container_t &asc,
				   const clipper::Spacegroup &spg, 
				   const clipper::Cell &cell,
				   const std::pair<symm_trans_t, Cell_Translation> &symm_trans) const { 

   clipper::Coord_orth trans_pos; 
   clipper::Coord_orth co; 

   // if (symm_trans.isym() == 3 && symm_trans.x() == 0 && symm_trans.y() == 0 && symm_trans.z() == 0) {
   // this does not return what I expect!  The [3] of P 21 21 21 is
   // 1/2+X,1/2-Y,-Z ,but this returns: -x+1/2, -y, z+1/2, the
   // [1] element of the [zero indexed] symops list.
   // 
   // std::cout << "selected symm_trans: " << spg.symop(symm_trans.isym()).format() << std::endl;

   // so let's create a symop from symm_trans.isym() by creating an rtop_frac
   // 
   // Either that or make which_box return clipper symop.  Yes, I like that.
   // 
   mat44 my_matt;
   mat44 mol_to_origin_mat;

   asc.mol->GetTMatrix(mol_to_origin_mat, 0, 
		       -symm_trans.second.us,
		       -symm_trans.second.vs,
		       -symm_trans.second.ws);
      
   asc.mol->GetTMatrix(my_matt,
		       symm_trans.first.isym(),
		       symm_trans.first.x(),
		       symm_trans.first.y(),
		       symm_trans.first.z());
      
   //       std::cout << "my_matt is: " << std::endl
   // 		<< my_matt[0][0] << " "  << my_matt[0][1] << " "
   // 		<< my_matt[0][2] << " "  << my_matt[0][3] << " "  << std::endl
   // 		<< my_matt[1][0] << " "  << my_matt[1][1] << " "
   // 		<< my_matt[1][2] << " "  << my_matt[1][3] << " "  << std::endl
   // 		<< my_matt[2][0] << " "  << my_matt[2][1] << " "
   // 		<< my_matt[2][2] << " "  << my_matt[2][3] << " "  << std::endl
   // 		<< my_matt[3][0] << " "  << my_matt[3][1] << " "
   // 		<< my_matt[3][2] << " "  << my_matt[3][3] << " "  << std::endl; 
      
   clipper::Mat33<double> clipper_mol_to_ori_mat(mol_to_origin_mat[0][0], mol_to_origin_mat[0][1], mol_to_origin_mat[0][2],
						 mol_to_origin_mat[1][0], mol_to_origin_mat[1][1], mol_to_origin_mat[1][2],
						 mol_to_origin_mat[2][0], mol_to_origin_mat[2][1], mol_to_origin_mat[2][2]);
   clipper::Mat33<double> clipper_mat(my_matt[0][0], my_matt[0][1], my_matt[0][2],
				      my_matt[1][0], my_matt[1][1], my_matt[1][2],
				      my_matt[2][0], my_matt[2][1], my_matt[2][2]);
   clipper::Coord_orth  cco(my_matt[0][3], my_matt[1][3], my_matt[2][3]);
   clipper::Coord_orth  cco_mol_to_ori(mol_to_origin_mat[0][3], mol_to_origin_mat[1][3], mol_to_origin_mat[2][3]);
   clipper::RTop_orth rtop_mol_to_ori(clipper_mol_to_ori_mat, cco_mol_to_ori);
   clipper::RTop_orth rtop(clipper_mat, cco);
      
   for(int i=0; i<asc.n_selected_atoms; i++) {
      co = clipper::Coord_orth(asc.atom_selection[i]->x, 
			       asc.atom_selection[i]->y, 
			       asc.atom_selection[i]->z);
      trans_pos = co.transform(rtop_mol_to_ori).transform(rtop);

//       std::cout << i << "  " << cf.format() << "  " << trans_pos_cf.format() 
// 		<< "  " << spg.symop(symm_trans.isym()).format() << std::endl;
      (*hybrid_atoms)[i] = coot::clip_hybrid_atom(asc.atom_selection[i], 
						  coot::Cartesian(trans_pos.x(),
								  trans_pos.y(),
								  trans_pos.z()));
   }
}


// pickable moving atoms molecule
//
pick_info
graphics_info_t::moving_atoms_atom_pick() const {

   pick_info p_i;
   p_i.success = GL_FALSE;
   p_i.imol = -1;
   coot::Cartesian front = unproject(0.0);
   coot::Cartesian back  = unproject(1.0);
   float dist, min_dist = 0.2;

   // This is the signal that moving_atoms_asc is clear
   if (moving_atoms_asc->n_selected_atoms > 0) {

      for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
	 coot::Cartesian atom( (moving_atoms_asc->atom_selection)[i]->x,
			 (moving_atoms_asc->atom_selection)[i]->y,
			 (moving_atoms_asc->atom_selection)[i]->z);
	 //
	 if (atom.within_box(front,back)) { 
	    dist = atom.distance_to_line(front, back);
	    
	    if (dist < min_dist) {
	       
	       min_dist = dist;
	       p_i.success = GL_TRUE;
	       p_i.atom_index = i;
	    }
	 }
      }
   }

   return p_i;
}


// Setup moving atom-drag if we are.
// (we are acting on button1 down)
//
void
graphics_info_t::check_if_moving_atom_pull() {

   pick_info pi = moving_atoms_atom_pick();
   if (pi.success == GL_TRUE) {

      // quite possibly we will have success if moving_atoms_asc is
      // not null...

      // setup for move (pull) of that atom then (and later we will
      // move the other atoms in some sort of stretch system)
      //
      // Recall that we came here from a button click event, no a
      // motion event - this is a setup function - not a "doit"
      // function
      //
      // This is a bit tedious, because we have to search in the
      // imol_moving_atoms molecule.
      //

      moving_atoms_dragged_atom_index = pi.atom_index;
      in_moving_atoms_drag_atom_mode_flag = 1;

      // Find the fixed_points_sheared_drag from the imol_moving_atoms.
      // and their flag, have_fixed_points_sheared_drag_flag
      // set_fixed_points_for_sheared_drag(); // superceeded
      // 
      if (drag_refine_idle_function_token != -1) { 
// 	 std::cout << "DEBUG:: removing token " << drag_refine_idle_function_token
// 		   << std::endl;
	 gtk_idle_remove(drag_refine_idle_function_token);
	 drag_refine_idle_function_token = -1; // magic "not in use" value
      }
      
   } else {
      in_moving_atoms_drag_atom_mode_flag = 0;
   } 
}

void
graphics_info_t::set_fixed_points_for_sheared_drag() {

} 


// 
void
graphics_info_t::move_moving_atoms_by_shear(int screenx, int screeny,
					    short int squared_flag) {

   // First we have to find the "fixed" points connected to them most
   // distant from the moving_atoms_dragged_atom.
   //
   // So now we ask, how much should we move the moving atom?  We have
   // the current mouse position: x, y and the previous mouse
   // position: GetMouseBeginX(), GetMouseBeginY(). Let's use
   // unproject to find the shift in world coordinates...
   //
   coot::Cartesian old_mouse_real_world = unproject_xyz(int(GetMouseBeginX()),
						  int(GetMouseBeginY()),
						  0.5);
   coot::Cartesian current_mouse_real_world = unproject_xyz(screenx, screeny, 0.5);

   coot::Cartesian diff = current_mouse_real_world - old_mouse_real_world;

   // now tinker with the moving atoms coordinates...
   if (moving_atoms_dragged_atom_index >= 0) {
      if (moving_atoms_dragged_atom_index < moving_atoms_asc->n_selected_atoms) {
	 CAtom *at = moving_atoms_asc->atom_selection[moving_atoms_dragged_atom_index];
	 if (at) {

	    move_moving_atoms_by_shear_internal(diff, squared_flag);

	    // and regenerate the bonds of the moving atoms:
	    // 
	    int do_disulphide_flag = 0;
	    Bond_lines_container bonds(*moving_atoms_asc, do_disulphide_flag);
	    regularize_object_bonds_box.clear_up();
	    regularize_object_bonds_box = bonds.make_graphical_bonds();
	    graphics_draw();
	 } else {
	    std::cout << "ERROR: null atom in move_moving_atoms_by_shear\n";
	 }
      } else {
	 std::cout << "ERROR: out of index (over) in move_moving_atoms_by_shear\n";
      }
   } else {
      std::cout << "ERROR: out of index (under) in move_moving_atoms_by_shear\n";
   }
}


// For rotate/translate moving atoms dragged movement
// 
void
graphics_info_t::move_moving_atoms_by_simple_translation(int screenx, int screeny) { 

   coot::Cartesian old_mouse_real_world = unproject_xyz(int(GetMouseBeginX()),
						  int(GetMouseBeginY()),
						  0.5);
   coot::Cartesian current_mouse_real_world = unproject_xyz(screenx, screeny, 0.5);

   coot::Cartesian diff = current_mouse_real_world - old_mouse_real_world;
   CAtom *at;
   for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
      at = moving_atoms_asc->atom_selection[i];
      at->x += diff.x();
      at->y += diff.y();
      at->z += diff.z();
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

// moving_atoms_dragged_atom_index
void
graphics_info_t::move_single_atom_of_moving_atoms(int screenx, int screeny) {

   
   coot::Cartesian old_mouse_real_world = unproject_xyz(int(GetMouseBeginX()),
						  int(GetMouseBeginY()),
						  0.5);
   coot::Cartesian current_mouse_real_world = unproject_xyz(screenx, screeny, 0.5);

   coot::Cartesian diff = current_mouse_real_world - old_mouse_real_world;
   CAtom *at = moving_atoms_asc->atom_selection[moving_atoms_dragged_atom_index];
   at->x += diff.x();
   at->y += diff.y();
   at->z += diff.z();

   int do_disulphide_flag = 0;
   Bond_lines_container bonds(*moving_atoms_asc, do_disulphide_flag);
   regularize_object_bonds_box.clear_up();
   regularize_object_bonds_box = bonds.make_graphical_bonds();
   graphics_draw();

}

// diff_std is the difference in position of the moving atoms, the
// other atoms of the moving_atoms_asc are moved relative to it by
// different amounts...
// 
void
graphics_info_t::move_moving_atoms_by_shear_internal(const coot::Cartesian &diff_std, short int squared_flag) {

   coot::Cartesian diff = diff_std;
   coot::Cartesian moving_atom(moving_atoms_asc->atom_selection[moving_atoms_dragged_atom_index]->x,
			 moving_atoms_asc->atom_selection[moving_atoms_dragged_atom_index]->y,
			 moving_atoms_asc->atom_selection[moving_atoms_dragged_atom_index]->z);
   float d_to_moving_at_max = -9999999.9;
   int d_array_size = moving_atoms_asc->n_selected_atoms;
   float *d_to_moving_at = new float[d_array_size];
   float frac;
   CAtom *at;
   
   for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {

      at = moving_atoms_asc->atom_selection[i];
      coot::Cartesian this_atom(at->x, at->y, at->z);
      d_to_moving_at[i] = (this_atom - moving_atom).length();
      if (d_to_moving_at[i] > d_to_moving_at_max) {
	 d_to_moving_at_max = d_to_moving_at[i];
      }
   }

   double dr;
   for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {

      if (squared_flag == 0)
	 frac = (1.0 - d_to_moving_at[i]/d_to_moving_at_max);
      else {
	 dr = d_to_moving_at[i]/d_to_moving_at_max;
	 frac = (1.0 - dr)*(1.0 - dr);
      }
	    
      CAtom *at = moving_atoms_asc->atom_selection[i];
      at->x += frac*diff.x();
      at->y += frac*diff.y();
      at->z += frac*diff.z();
   }
   delete [] d_to_moving_at;
} 

void
graphics_info_t::do_post_drag_refinement_maybe() {

#ifdef HAVE_GSL
   if (last_restraints.size() > 0) { 
      // std::cout << "do refinement now!\n";
      graphics_info_t::add_drag_refine_idle_function();
   } else { 
      // std::cout << "DEBUG:: not doing refinement - no restraints."
      // << std::endl;
   }
#endif // HAVE_GSL   
}


// static
void
graphics_info_t::statusbar_ctrl_key_info() { // Ctrl to rotate or pick?

   graphics_info_t g;
   if (graphics_info_t::control_key_for_rotate_flag) {
      g.statusbar_text("Use Ctrl Left-mouse to rotate the view.");
   } else {
      g.statusbar_text("Use Ctrl Left-mouse to pick an atom...");
   }
}

clipper::Coord_orth
graphics_info_t::moving_atoms_centre() const {

   clipper::Coord_orth moving_middle(0,0,0);

   // Let's find the middle of the moving atoms and set
   // rotation_centre to that:
   int n = moving_atoms_asc->n_selected_atoms;
   if (n > 0) {
      float sum_x = 0.0; float sum_y = 0.0; float sum_z = 0.0;
      for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
	 sum_x += moving_atoms_asc->atom_selection[i]->x;
	 sum_y += moving_atoms_asc->atom_selection[i]->y;
	 sum_z += moving_atoms_asc->atom_selection[i]->z;
      }
      moving_middle = clipper::Coord_orth(sum_x/float(n), sum_y/float(n), sum_z/float(n));
   }
   return moving_middle;
} 

								       
// Presumes that rotation centre can be got from CAtom *rot_trans_rotation_origin_atom;
// 
void
graphics_info_t::rotate_intermediate_atoms_round_screen_z(double angle) {

   if (rot_trans_rotation_origin_atom) { 
      if (moving_atoms_asc->mol) {
	 if (moving_atoms_asc->n_selected_atoms > 0) {
	    coot::Cartesian front  = unproject_xyz(0, 0, 0.0);
	    coot::Cartesian centre = unproject_xyz(0, 0, 0.5);
	    coot::Cartesian screen_z = (front - centre);

	    clipper::Coord_orth screen_vector =  clipper::Coord_orth(screen_z.x(), 
								     screen_z.y(), 
								     screen_z.z());
	    CAtom *rot_centre = rot_trans_rotation_origin_atom;
	    clipper::Coord_orth rotation_centre(rot_centre->x, 
						rot_centre->y, 
						rot_centre->z);
	    // But! maybe we have a different rotation centre
	    if (rot_trans_zone_rotates_about_zone_centre) {
	       if (moving_atoms_asc->n_selected_atoms  > 0) {
		  rotation_centre = moving_atoms_centre();
	       }
	    }
	 
	    for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
	       clipper::Coord_orth co(moving_atoms_asc->atom_selection[i]->x,
				      moving_atoms_asc->atom_selection[i]->y,
				      moving_atoms_asc->atom_selection[i]->z);
	       clipper::Coord_orth new_pos = 
		  rotate_round_vector(screen_vector, co, rotation_centre, angle);
	       moving_atoms_asc->atom_selection[i]->x = new_pos.x();
	       moving_atoms_asc->atom_selection[i]->y = new_pos.y();
	       moving_atoms_asc->atom_selection[i]->z = new_pos.z();
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
      }
   }
} 

// Presumes that rotation centre can be got from CAtom *rot_trans_rotation_origin_atom;
// 
void
graphics_info_t::rotate_intermediate_atoms_round_screen_x(double angle) {

   if (rot_trans_rotation_origin_atom) { 
      if (moving_atoms_asc->mol) {
	 if (moving_atoms_asc->n_selected_atoms > 0) {
	    coot::Cartesian front  = unproject_xyz(0, 0, 0.0);
	    coot::Cartesian centre = unproject_xyz(0, 0, 0.5);
	    coot::Cartesian right  = unproject_xyz(1, 0, 0.5);
	    coot::Cartesian screen_z = (front - centre);
	    coot::Cartesian screen_x = (right - centre);

	    clipper::Coord_orth screen_vector =  clipper::Coord_orth(screen_x.x(), 
								     screen_x.y(), 
								     screen_x.z());
	    CAtom *rot_centre = rot_trans_rotation_origin_atom;
	    clipper::Coord_orth rotation_centre(rot_centre->x, 
						rot_centre->y, 
						rot_centre->z);
	 
	    // But! maybe we have a different rotation centre
	    if (rot_trans_zone_rotates_about_zone_centre)
	       if (moving_atoms_asc->n_selected_atoms  > 0)
		  rotation_centre = moving_atoms_centre();
	    
	    for (int i=0; i<moving_atoms_asc->n_selected_atoms; i++) {
	       clipper::Coord_orth co(moving_atoms_asc->atom_selection[i]->x,
				      moving_atoms_asc->atom_selection[i]->y,
				      moving_atoms_asc->atom_selection[i]->z);
	       clipper::Coord_orth new_pos = 
		  rotate_round_vector(screen_vector, co, rotation_centre, angle);
	       moving_atoms_asc->atom_selection[i]->x = new_pos.x();
	       moving_atoms_asc->atom_selection[i]->y = new_pos.y();
	       moving_atoms_asc->atom_selection[i]->z = new_pos.z();
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
      }
   }
} 
