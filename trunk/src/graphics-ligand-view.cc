/* src/graphics-ligand-view.cc
 * 
 * Copyright 2011 by The University of Oxford
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
 * Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#include <string.h>
#include <string>
#include <GL/gl.h>
#include <GL/glut.h> // for GLUT bitmap fonts, maybe deletable.
#include <mmdb/mmdb_manager.h>
#include "graphics-ligand-view.hh"

#include "coot-sysdep.h"

#ifdef MAKE_ENTERPRISE_TOOLS
#include "rdkit-interface.hh"
#endif 

void 
graphics_ligand_molecule::generate_display_list(bool dark_background) {

   if (0)
      std::cout << "graphics_ligand_molecule::store() molecule graphics based on "
		<< atoms.size() << " atoms and "
		<< bonds.size() << " bonds" << std::endl;

   if (glIsList(display_list_tag)) { 
      glDeleteLists(display_list_tag, 1);
   }
   display_list_tag = glGenLists(1); // range of 1.
   glNewList(display_list_tag, GL_COMPILE);
   gl_bonds(dark_background);
   glEndList();
}

coot::colour_t
graphics_ligand_atom::get_colour(bool against_a_dark_background) const { 

   coot::colour_t col(0.6,0.6,0.6);
   if ((element == "F") ||  (element == "Br") || (element == "Cl")) {
      col.col[0] = 0.3;
      col.col[1] = 0.8;
      col.col[2] = 0.3;
   }
   if (element == "O") {
      col.col[0] = 0.9;
      col.col[1] = 0.3;
      col.col[2] = 0.3;
   }
   if (element == "P")  {
      col.col[0] = 0.7;
      col.col[1] = 0.3;
      col.col[2] = 0.9;
   }
   if ((element == "S") || (element == "Se")) {
      col.col[0] = 0.76;
      col.col[1] = 0.76;
      col.col[2] = 0.2;
   }
   if (element == "N")  {
      col.col[0] = 0.4;
      col.col[1] = 0.4;
      col.col[2] = 0.95;
   }
   return col;
}

// c.f. lbg_info_t::render_from_molecule()
// 
void graphics_ligand_molecule::gl_bonds(bool dark_background) {

   for (unsigned int ib=0; ib<bonds.size(); ib++) {
      int idx_1 = bonds[ib].get_atom_1_index();
      int idx_2 = bonds[ib].get_atom_2_index();
      if ((idx_1 != UNASSIGNED_INDEX) && (idx_2 != UNASSIGNED_INDEX)) { 
	 lig_build::bond_t::bond_type_t bt = bonds[ib].get_bond_type();

	 // c.f. widgeted_bond_t::construct_internal()
	 bool shorten_first = false;
	 bool shorten_second = false;
	 if (atoms[idx_1].atom_id != "C") { 
	    shorten_first = true;
	 } 
	 if (atoms[idx_2].atom_id != "C") { 
	    shorten_second = true;
	 }
	 lig_build::pos_t pos_1 =  atoms[idx_1].atom_position;
	 lig_build::pos_t pos_2 =  atoms[idx_2].atom_position;
	 // c.f. canvas_item_for_bond
	 bonds[ib].gl_bond(pos_1, pos_2, shorten_first, shorten_second, bt);
      }
   }


   for (unsigned int iat=0; iat<atoms.size(); iat++) { 
      std::string ele = atoms[iat].element;
      if (ele != "C") { 
	 std::vector<int> local_bonds = bonds_having_atom_with_atom_index(iat);
	 bool gl_flag = true;
	 lig_build::atom_id_info_t atom_id_info = make_atom_id_by_using_bonds(iat, ele, local_bonds, gl_flag);
	 // atoms[iat].set_atom_id(atom_id_info.atom_id); // quick hack
	 bool background_black = true;
	 coot::colour_t col = atoms[iat].get_colour(background_black); // using ele
	 atoms[iat].make_text_item(atom_id_info, col);
      }
   }
}

// c.f. widgeted_bond's canvas_item_for_bond (all bonds are made this way (apparently))
void
graphics_ligand_bond::gl_bond(const lig_build::pos_t &pos_1_raw, const lig_build::pos_t &pos_2_raw,
			      bool shorten_first, bool shorten_second,
			      lig_build::bond_t::bond_type_t bt) {

//    std::cout << "generate gl bond between atoms at " << pos_1 << " " << pos_2
// 	     << " " << shorten_first << " " << shorten_second << std::endl;

   double shorten_fraction = 0.76;
   
   lig_build::pos_t pos_1 = pos_1_raw;
   lig_build::pos_t pos_2 = pos_2_raw;

   // fraction_point() returns a point that is (say) 0.8 of the way
   // from p1 (first arg) to p2 (second arg).
   // 
   if (shorten_first)
      pos_1 = lig_build::pos_t::fraction_point(pos_2_raw, pos_1_raw, shorten_fraction);
   if (shorten_second)
      pos_2 = lig_build::pos_t::fraction_point(pos_1_raw, pos_2_raw, shorten_fraction);


   switch (bt) {
   case lig_build::bond_t::SINGLE_BOND:
      // Add new cases, a bit of a hack of course.
   case lig_build::bond_t::SINGLE_OR_DOUBLE:
   case lig_build::bond_t::SINGLE_OR_AROMATIC:
   case lig_build::bond_t::AROMATIC_BOND:
   case lig_build::bond_t::BOND_ANY:

      glBegin(GL_LINES);
      glVertex3d(pos_1.x, pos_1.y, 0);
      glVertex3d(pos_2.x, pos_2.y, 0);
      glEnd();
      break;
      
   case lig_build::bond_t::DOUBLE_BOND:
      
   case lig_build::bond_t::DOUBLE_OR_AROMATIC:
      {
	 if (have_centre_pos()) {
	    gl_bond_double_aromatic_bond(pos_1, pos_2);
	 } else {
	    gl_bond_double_bond(pos_1, pos_2);
	 } 
      }
      break;
   case lig_build::bond_t::TRIPLE_BOND:
      { 
	 lig_build::pos_t buv = (pos_2-pos_1).unit_vector();
	 lig_build::pos_t buv_90 = buv.rotate(90);
	 double small = 0.25;
	 lig_build::pos_t p1 = pos_1 + buv_90 * small;
	 lig_build::pos_t p2 = pos_2 + buv_90 * small;
	 lig_build::pos_t p3 = pos_1;
	 lig_build::pos_t p4 = pos_2;
	 lig_build::pos_t p5 = pos_1 - buv_90 * small;
	 lig_build::pos_t p6 = pos_2 - buv_90 * small;

	 glBegin(GL_LINES);
	 glVertex3d(p1.x, p1.y,0);
	 glVertex3d(p2.x, p2.y,0);
	 
	 glVertex3d(p3.x, p3.y,0);
	 glVertex3d(p4.x, p4.y,0);
	 
	 glVertex3d(p5.x, p5.y,0);
	 glVertex3d(p6.x, p6.y,0);
	 glEnd();
      }
      break;
   case IN_BOND:
      {
	 // set of lines
	 std::vector<std::pair<lig_build::pos_t, lig_build::pos_t> > vp =
	    lig_build::pos_t::make_wedge_in_bond(pos_1, pos_2);
	 if (vp.size()) { 
	    glBegin(GL_LINES);
	    for (unsigned int i=0; i<vp.size(); i++) { 
	       glVertex3d(vp[i].first.x, vp[i].first.y, 0);
	       glVertex3d(vp[i].second.x, vp[i].second.y, 0);
	    }
	 }
	 glEnd();
      }
      break;
   case OUT_BOND:
      {
	 // filled shape
	 std::vector<lig_build::pos_t> v =
	    lig_build::pos_t::make_wedge_out_bond(pos_1, pos_2);
	 glBegin(GL_POLYGON);
	 for (unsigned int i=0; i<v.size(); i++)
	       glVertex3d(v[i].x,  v[i].y, 0);
	 glEnd();
      }
      break;
   case BOND_UNDEFINED:
      break;
   }

}

void
graphics_ligand_bond::gl_bond_double_aromatic_bond(const lig_build::pos_t &pos_1,
						   const lig_build::pos_t &pos_2) {

   std::pair<lig_build::pos_t, lig_build::pos_t> p = 
      make_double_aromatic_short_stick(pos_1, pos_2);

   glBegin(GL_LINES);
   glVertex3d(pos_1.x, pos_1.y, 0);
   glVertex3d(pos_2.x, pos_2.y, 0);
   glVertex3d(p.first.x, p.first.y, 0);
   glVertex3d(p.second.x, p.second.y, 0);
   glEnd();
}

void
graphics_ligand_bond::gl_bond_double_bond(const lig_build::pos_t &pos_1, const lig_build::pos_t &pos_2) {

   std::pair<std::pair<lig_build::pos_t, lig_build::pos_t>, std::pair<lig_build::pos_t, lig_build::pos_t> > p = make_double_bond(pos_1, pos_2);

   glBegin(GL_LINES);
   glVertex3d(p.first.first.x,   p.first.first.y,  0);
   glVertex3d(p.first.second.x,  p.first.second.y, 0);
   glVertex3d(p.second.first.x,  p.second.first.y,  0);
   glVertex3d(p.second.second.x, p.second.second.y, 0);
   glEnd();
}


void 
graphics_ligand_atom::make_text_item(const lig_build::atom_id_info_t &atom_id_info_in,
				     const coot::colour_t &fc) const { 

   if (atom_id_info_in.atom_id != "C") {
      glColor3f(fc.col[0], fc.col[1], fc.col[2]);
   
      for (unsigned int i=0; i<atom_id_info_in.size(); i++) {
	 double x_o = -0.25; 
	 double y_o = -0.25;
	 if (atom_id_info_in[i].text_pos_offset == lig_build::offset_text_t::UP)
	    y_o += 0.7;
	 if (atom_id_info_in[i].text_pos_offset == lig_build::offset_text_t::DOWN)
	    y_o -= 0.7;

	 double x_pos = atom_position.x + 0.06 * atom_id_info_in.offsets[i].tweak.x + x_o;
	 double y_pos = atom_position.y + 0.06 * atom_id_info_in.offsets[i].tweak.y + y_o;

	 if (0)
	    std::cout << "Rendering tweak " << i << " :" << atom_id_info_in[i].text
		      << ": with tweak " << atom_id_info_in[i].tweak << std::endl;

	 if (atom_id_info_in.offsets[i].subscript)
	    y_pos -= 0.3;
	    
	 glRasterPos3d(x_pos, y_pos, 0);

	 if (0) {  // Yes the 0,0 position is bottom left of the letter
	    glLineWidth(1.0);
	    glBegin(GL_LINES);
	    glVertex3d(x_pos-1, y_pos,   0);
	    glVertex3d(x_pos+1, y_pos,   0);
	    glVertex3d(x_pos,   y_pos-1, 0);
	    glVertex3d(x_pos,   y_pos+1, 0);
	    glEnd();
	 }
	 bitmap_text(atom_id_info_in.offsets[i].text.c_str());
      }
   }
}

void
graphics_ligand_atom::bitmap_text(const std::string &s) const {
   glPushAttrib (GL_LIST_BIT);
   for (unsigned int i = 0; i < s.length(); i++)
      glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, s[i]);
   glPopAttrib();
} 



void 
graphics_ligand_molecule::render() {

//    std::cout << "graphics_ligand_molecule::render() draw the stored graphics here"
// 	     << std::endl;
   glCallList(display_list_tag);
} 

bool
graphics_ligand_molecule::setup_from(CResidue *residue_p,
				     coot::protein_geometry *geom_p,
				     bool against_a_dark_background) {

   bool status = false; // "failed" status initially
   
#ifdef MAKE_ENTERPRISE_TOOLS

   if (residue_p) {
      try {
	 std::string res_name = residue_p->GetResName();
	 std::pair<bool, coot::dictionary_residue_restraints_t> p = 
	    geom_p->get_monomer_restraints_at_least_minimal(res_name);
	 if (! p.first) {
	    std::cout << "DEBUG:: No restraints for \"" << res_name << "\"" << std::endl;
	 } else {
	    const coot::dictionary_residue_restraints_t &restraints = p.second;
	    RDKit::RWMol rdkm = coot::rdkit_mol(residue_p, restraints);
	    RDKit::ROMol *rdk_mol_with_no_Hs_ro = RDKit::MolOps::removeHs(rdkm);
	    RDKit::RWMol rdk_mol_with_no_Hs = *rdk_mol_with_no_Hs_ro;

	    // clear out any cached properties
	    rdk_mol_with_no_Hs.clearComputedProps();
	    // clean up things like nitro groups
	    RDKit::MolOps::cleanUp(rdk_mol_with_no_Hs);
	    // update computed properties on atoms and bonds:
	    rdk_mol_with_no_Hs.updatePropertyCache();
	    RDKit::MolOps::Kekulize(rdk_mol_with_no_Hs);
	    RDKit::MolOps::assignRadicals(rdk_mol_with_no_Hs);
	    
	    // then do aromaticity perception
	    // RDKit::MolOps::setAromaticity(rdkm);
    
	    // set conjugation
	    RDKit::MolOps::setConjugation(rdk_mol_with_no_Hs);
	       
	    // set hybridization
	    RDKit::MolOps::setHybridization(rdk_mol_with_no_Hs); // non-linear ester bonds.

	    // remove bogus chirality specs:
	    RDKit::MolOps::cleanupChirality(rdk_mol_with_no_Hs);

	    double weight_for_3d_distances = 0.005;
	    int mol_2d_depict_conformer =
	       coot::add_2d_conformer(&rdk_mol_with_no_Hs, weight_for_3d_distances);

	    // why is there no connection between a lig_build molecule_t
	    // and a rdkit molecule conformer?

	    // For now hack around using a molfile molecule...
	    //
	    // I think I should have a rdkit_mol->lig_build::molecule_t converter
	    // (for later).
	 
	    lig_build::molfile_molecule_t m =
	       coot::make_molfile_molecule(rdk_mol_with_no_Hs, mol_2d_depict_conformer);
	    init_from_molfile_molecule(m, against_a_dark_background);

	    status = true; // OK, if we got to here...
	 }
      }
      catch (std::runtime_error coot_error) {
	 std::cout << coot_error.what() << std::endl;
      }
      catch (std::exception rdkit_error) {
	 std::cout << rdkit_error.what() << std::endl;
      }
   }
#endif   // MAKE_ENTERPRISE_TOOLS
   return status;
} 



void
graphics_ligand_molecule::init_from_molfile_molecule(const lig_build::molfile_molecule_t &mol_in,
						     bool dark_background_flag) {

   atoms.clear();
   bonds.clear();
   for (unsigned int iat=0; iat<mol_in.atoms.size(); iat++) {
      const lig_build::molfile_atom_t &at_in = mol_in.atoms[iat];
      graphics_ligand_atom at(lig_build::pos_t(at_in.atom_position.x(), at_in.atom_position.y()),
			      at_in.element, at_in.formal_charge);
      at.atom_name = at_in.name;
      at.aromatic = at_in.aromatic;
      // what about chiral here (a lig_build::atom_t does not have chiral information).
      atoms.push_back(at);
   }

   for (unsigned int ib=0; ib<mol_in.bonds.size(); ib++) {
      const lig_build::molfile_bond_t &bond_in = mol_in.bonds[ib];
      graphics_ligand_bond b(bond_in.index_1, bond_in.index_2, bond_in.bond_type);
      bonds.push_back(b);
   }
   assign_ring_centres();
   scale_correction = mol_in.get_scale_correction();
   generate_display_list(dark_background_flag);
} 
