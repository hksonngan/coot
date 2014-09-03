
#include "dots-representation.hh"

// ---------------------------------------------------------------------------------------
//                           dots
// ---------------------------------------------------------------------------------------

// make a surface on mol
coot::dots_representation_info_t::dots_representation_info_t(mmdb::Manager *mol) {

   is_closed = 0;
   int SelHnd = mol->NewSelection();
   mol->SelectAtoms(SelHnd, 0, "*", ANY_RES, "*", ANY_RES, "*", "*", "*", "*", "*");
   mmdb::Manager *dum = NULL;
   colour_t dummy_col;
   add_dots(SelHnd, mol, dum, 1.0, dummy_col, false);
   mol->DeleteSelection(SelHnd);
}

coot::dots_representation_info_t::dots_representation_info_t(mmdb::Manager *mol,
							     mmdb::Manager *mol_exclude) {

   is_closed = 0;
   int SelHnd = mol->NewSelection();
   mol->SelectAtoms(SelHnd, 0, "*", ANY_RES, "*", ANY_RES, "*", "*", "*", "*", "*");
   colour_t dummy_col;
   add_dots(SelHnd, mol, mol_exclude, 1.0, dummy_col, false);
   mol->DeleteSelection(SelHnd);
}

double
coot::dots_representation_info_t::get_radius(const std::string &ele) const {

   double radius = 1.70;
   if (ele == " H")
      radius = 1.20;
   if (ele == " N")
      radius = 1.55;
   if (ele == " O")
      radius = 1.52;
   if (ele == " S")
      radius = 1.8;
   // PDBv3
   if (ele == "H")
      radius = 1.20;
   if (ele == "N")
      radius = 1.55;
   if (ele == "O")
      radius = 1.52;
   if (ele == "S")
      radius = 1.8;
   return radius;
}

coot::colour_t
coot::dots_representation_info_t::get_colour(const std::string &ele) const {

   coot::colour_t col(0.4, 0.4, 0.4);
   if (ele == " C")
      col = coot::colour_t(0.33, 0.4, 0.2);
   if (ele == " N")
      col = coot::colour_t(0.2, 0.2, 0.6);
   if (ele == " O")
      col = coot::colour_t(0.6, 0.2, 0.2);
   if (ele == " S")
      col = coot::colour_t(0.5, 0.5, 0.2);
   // PDB 3
   if (ele == "C")
      col = coot::colour_t(0.33, 0.4, 0.3);
   if (ele == "N")
      col = coot::colour_t(0.2, 0.2, 0.6);
   if (ele == "O")
      col = coot::colour_t(0.6, 0.2, 0.2);
   if (ele == "S")
      col = coot::colour_t(0.5, 0.5, 0.2);

   return col;
} 

// 20111123 modern usage
//
void
coot::dots_representation_info_t::add_dots(int SelHnd, mmdb::Manager *mol,
					   mmdb::Manager *mol_exclude,
					   double dot_density,
					   const coot::colour_t &single_colour,
					   bool use_single_colour) {

   mmdb::PPAtom atoms = NULL;
   int n_atoms;

   double phi_step = 5.0 * (M_PI/180.0);
   double theta_step = 5.0 * (M_PI/180.0);
   if (dot_density > 0.0) { 
      phi_step   /= dot_density;
      theta_step /= dot_density;
   }
   mol->GetSelIndex(SelHnd, atoms, n_atoms);
   std::vector<double> radius(n_atoms);
   std::vector<double> radius_exclude;
   std::vector<coot::colour_t> colour(n_atoms);
   for (unsigned int iat=0; iat<n_atoms; iat++) {
      std::string ele(atoms[iat]->element);
      radius[iat] = get_radius(ele);
      if (use_single_colour)
	 colour[iat] = single_colour;
      else 
	 colour[iat] = get_colour(ele);
   }


   int n_atoms_exclude = 0;
   mmdb::PPAtom atoms_exclude = NULL;
   int SelHnd_exclude = 0;
   if (mol_exclude) {
      SelHnd_exclude = mol_exclude->NewSelection();
      mol_exclude->SelectAtoms(SelHnd_exclude, 0, "*", ANY_RES, "*", ANY_RES, "*", "*", "*", "*", "*");
      mol_exclude->GetSelIndex(SelHnd_exclude, atoms_exclude, n_atoms_exclude);
      radius_exclude.resize(n_atoms_exclude);
      for (unsigned int iat=0; iat<n_atoms_exclude; iat++) {
	 std::string ele(atoms_exclude[iat]->element);
	 radius_exclude[iat] = get_radius(ele);
      }
   }
   
   for (int iatom=0; iatom<n_atoms; iatom++) {
      std::vector<clipper::Coord_orth> local_points;
      coot::colour_t col = colour[iatom];
      if (! atoms[iatom]->isTer()) { 
	 clipper::Coord_orth centre(atoms[iatom]->x,
				    atoms[iatom]->y,
				    atoms[iatom]->z);
	 bool even = 1;
	 for (double theta=0; theta<M_PI; theta+=theta_step) {
	    double phi_step_inner = phi_step + 0.1 * pow(theta-0.5*M_PI, 2);
	    for (double phi=0; phi<2*M_PI; phi+=phi_step_inner) {
	       if (even) {

		  // Is there another atom in the same residue as this
		  // atom, that is closer to pt than the centre atom?
		  // If so, don't draw this point.

		  double atom_radius = radius[iatom];
		  double atom_radius_squard = atom_radius * atom_radius;

		  clipper::Coord_orth pt(atom_radius*cos(phi)*sin(theta),
					 atom_radius*sin(phi)*sin(theta),
					 atom_radius*cos(theta));
		  pt += centre;

		  bool draw_it = 1;

		  // it might be possible to speed this up by precalculating all dot
		  // points and then doing a findcontacts to the atoms of the atom
		  // selection. That is involved though.
	       
		  for (int jatom=0; jatom<n_atoms; jatom++) {
		     if (jatom != iatom) {
			if (! atoms[jatom]->isTer()) {
			   double radius_j = radius[jatom];
			   double radius_j_squared = radius_j * radius_j;
			   clipper::Coord_orth pt_j(atoms[jatom]->x, atoms[jatom]->y, atoms[jatom]->z);
			   if ((pt-pt_j).lengthsq() < radius_j_squared) {
			      draw_it = 0;
			      break;
			   }
			}
		     }
		  }

		  // and now, don't draw if far from exclude molecule
		  // atoms (if there are any).
		  //
		  if (n_atoms_exclude) { 
		     if (draw_it) {
			draw_it = 0;
			double dist_j = 4.0;
			double dist_j_squared = dist_j * dist_j;
			for (int jatom=0; jatom<n_atoms_exclude; jatom++) {
			   if (! atoms_exclude[jatom]->isTer()) {
			      clipper::Coord_orth pt_j(atoms_exclude[jatom]->x,
						       atoms_exclude[jatom]->y,
						       atoms_exclude[jatom]->z);
			      if ((pt-pt_j).lengthsq() < dist_j_squared) {
				 draw_it = 1;
				 break;
			      }
			   }
			}
		     }
		  }
		  
		  if (draw_it) {
		     local_points.push_back(pt);
		  }
	       }
	       even = 1 - even;
	    }
	 }
      }
      std::pair<coot::colour_t, std::vector<clipper::Coord_orth> > p(col, local_points);
      points.push_back(p);
   }
   if (mol_exclude) {
      mol_exclude->DeleteSelection(SelHnd_exclude);
   }
}
