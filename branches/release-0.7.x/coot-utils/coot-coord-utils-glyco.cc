/* coot-utils/coot-coord-utils-glyco.cc
 * 
 * Copyright 2011 by The University of York
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

#include <queue>
#include <list>
#include <algorithm>

#include "coot-utils.hh"
#include "coot-coord-extras.hh"

#include "coot-sysdep.h"

// Note: this is a simple-minded hack.  The right way of doing this
// is to define a bonding tree that includes atoms from both
// residues.  Then we don't need reference structures - the
// "moving" residue atoms will get placed by internal coordinates.
//
// This can be viewed as starting (or test material) for the Proper
// Way code.
// 
// Scenario Simple Beam-in:
//    User has an ASN onto which they want to beam in a NAG.
// 
//    setup:
//    Make CResidue *s and/or molecule for the N-linked NAG reference residues.
// 
///   Get the CResidue * for the user residue ASN 
//    Get the CAtoms *s for the OD1, ND2, GC and CB in user residue [1]
//    Get the CAtoms *s for the OD1, ND2, GC and CB in N-linked ASN molecule [2]
//
//    LSQ fit the NAG residue from the reference ASN+NAG pair using
//    matrix that rotates [2] onto [1].  (We don't need the ASN from
//    that pair).  Now we can add that rotated NAG CResidue * to user
//    molecule.  we have N-linked-NAG template - consider renaming to
//    ASN-NAG-via-NAG-ASN i.e. the general case
//    {ResType1}-{ResType2}-via-{LinkName} where ResType1 and ResType2
//    are comp-ids, of course.  Actually, NAG-ASN is a pyranose-ASN
//    link (group to comp_id). Hmm...
//
// This can throw a std::runtime_error if we can't find the group of
// the input residues (for example).
// 
coot::beam_in_linked_residue::beam_in_linked_residue(CResidue *residue_ref_in,
						     const std::string &link_type_in,
						     const std::string &new_residue_type,
						     coot::protein_geometry *geom_p_in) {
   
   // do we have a template structure for the given args?
   have_template = false;
   geom_p = geom_p_in;
   link_type = link_type_in;
   comp_id_new = new_residue_type; // save for comparison in get_residue() (because the
                                   // residue retrieved could be of the wrong type (but
                                   // correct group).
   template_res_ref = NULL;
   template_res_mov = NULL;

   if (residue_ref_in) {
      
      residue_ref = residue_ref_in;
      comp_id_ref = residue_ref->GetResName();
      
      std::string g1 = geom_p->get_group(comp_id_ref);
      std::string g2 = geom_p->get_group(comp_id_new);

      bool status = setup_by_comp_id(comp_id_ref, comp_id_new);
      if (! status) {
	 // try comp_id and group
	 //
	 std::cout << "calling setup_by_comp_id_group with args " << comp_id_ref << " "
		   << g2 << std::endl;
	 status = setup_by_comp_id_group(comp_id_ref, g2);
	 if (! status) {
	    // try by group and group
	    setup_by_group_group(g1,g2);
	 }
      }
   } else {
      throw std::runtime_error("NULL input reference residue");
   } 
}

// factor out for clarity.  template_file_name exists before calling this.
// 
bool
coot::beam_in_linked_residue::setup_by_comp_id(const std::string &comp_id_ref,
					       const std::string &comp_id_new) {

   bool status = false;
   
   std::string file_name = comp_id_ref + "-" + comp_id_new;
   file_name += "-via-";
   file_name += link_type;
   file_name += ".pdb";
   
   std::string pkgdatadir = coot::package_data_dir();
   std::string full_path_pdb_filename = pkgdatadir; // and then add to it...
   full_path_pdb_filename += "/";
   full_path_pdb_filename += file_name;
   
   if (coot::file_exists(full_path_pdb_filename)) {
   
      CMMDBManager *t_mol = new CMMDBManager;
      int status = t_mol->ReadPDBASCII(full_path_pdb_filename.c_str());
      if (status != Error_NoError) {
	 std::cout << "ERROR:: on reading " << full_path_pdb_filename << std::endl;
      } else {

	 // More cool.
	 template_res_ref = get_residue(comp_id_ref, t_mol);
	 if (! template_res_ref) {
	    std::cout << "ERROR:: failed to find residue with comp_id " << comp_id_ref
		      << " in " << full_path_pdb_filename << std::endl;
	 } else {

	    // should be this path
	       
	    template_res_mov = get_residue(comp_id_new, t_mol);

	    if (! template_res_mov) {
	       std::cout << "ERROR:: failed to find (adding) residue with comp_id "
			 << comp_id_new << " in " << full_path_pdb_filename << std::endl;
	    } else { 
	       // Happy path
	       status = true;
	       have_template = 1; // template_res_mov and
                                  // template_res_ref are correctly
	                          // set.
	    } 
	 }
      }
   }
   return status;
} 


// This can throw an exception
// 
bool
coot::beam_in_linked_residue::setup_by_group_group(const std::string &group_ref,
						   const std::string &group_new) {

   bool status = false;

   std::string file_name = group_ref;
   file_name += "-";
   file_name += group_new;
   file_name += "-via-";
   file_name += link_type;
   file_name += ".pdb";
      
   std::string pkgdatadir = coot::package_data_dir();
   std::string full_path_pdb_filename = pkgdatadir; // and then add to it...
   full_path_pdb_filename += "/";
   full_path_pdb_filename += file_name;
   if (1)
      std::cout << "debug:: setup_by_group() full_path_pdb_filename "
		<< full_path_pdb_filename
		<< std::endl;
   if (! coot::file_exists(full_path_pdb_filename)) {
      std::cout << "WARNING:: link template file " << full_path_pdb_filename
		<< " does not exist " << std::endl;
   } else { 
      CMMDBManager *t_mol = new CMMDBManager;
      int status = t_mol->ReadPDBASCII(full_path_pdb_filename.c_str());
      if (status != Error_NoError) {
	 std::cout << "ERROR:: on reading " << full_path_pdb_filename << std::endl;
      } else {

	 // More cool.
	 template_res_ref = coot::util::get_nth_residue(1, t_mol);
	 if (! template_res_ref) {
	    std::cout << "ERROR:: failed to find residue with comp_id " << comp_id_ref
		      << " in " << full_path_pdb_filename << std::endl;
	 } else {

	    // should be this path
	       
	    template_res_mov = coot::util::get_nth_residue(2, t_mol); // get the BMA

	    if (! template_res_mov) {
	       std::cout << "ERROR:: failed to find (adding) residue with comp_id "
			 << comp_id_new << " in " << full_path_pdb_filename
			 << std::endl;
	    } else { 
	       // Happy path
	       have_template = 1; // template_res_mov and
	                          // template_res_ref are correctly
	                          // set.
	       status = true;
	    } 
	 }
      }
   }

   return status;
}

// This can throw an exception
// 
bool
coot::beam_in_linked_residue::setup_by_comp_id_group(const std::string &comp_id_ref,
						     const std::string &group_new) {

   bool status = false;

   std::string file_name = comp_id_ref;
   file_name += "-";
   file_name += group_new;
   file_name += "-via-";
   file_name += link_type;
   file_name += ".pdb";
   std::string pkgdatadir = coot::package_data_dir();
   std::string full_path_pdb_filename = pkgdatadir; // and then add to it...
   full_path_pdb_filename += "/";
   full_path_pdb_filename += file_name;
   if (! coot::file_exists(full_path_pdb_filename)) {
      std::cout << "WARNING:: link template file " << full_path_pdb_filename
		<< " does not exist " << std::endl;
   } else {
      CMMDBManager *t_mol = new CMMDBManager;
      int pdb_status = t_mol->ReadPDBASCII(full_path_pdb_filename.c_str());
      if (pdb_status != Error_NoError) {
	 std::cout << "ERROR:: on reading " << full_path_pdb_filename << std::endl;
      } else {

	 // More cool.
	 template_res_ref = coot::util::get_nth_residue(1, t_mol);
	 if (! template_res_ref) {
	    std::cout << "ERROR:: failed to find residue with comp_id " << comp_id_ref
		      << " in " << full_path_pdb_filename << std::endl;
	 } else {

	    // should be this path
	       
	    template_res_mov = coot::util::get_nth_residue(2, t_mol); // get the BMA

	    if (! template_res_mov) {
	       std::cout << "ERROR:: failed to find (adding) residue with comp_id "
			 << comp_id_new << " in " << full_path_pdb_filename
			 << std::endl;
	    } else { 
	       // Happy path
	       have_template = 1; // template_res_mov and
	                          // template_res_ref are correctly
	                          // set.
	       status = true;
	    } 
	 }
      }
   }
   return status; 
} 


// This can return NULL if we were unable to make the residue to be attached.
CResidue *
coot::beam_in_linked_residue::get_residue() const {

   CResidue *r = NULL;
   bool needs_O6_manip = false;
   double current_torsion = 0;
   double template_torsion = 64.0; // degrees (in the file)
   CAtom *at_O6 = NULL;
   clipper::Coord_orth origin_shift;
   clipper::Coord_orth     position;
   clipper::Coord_orth    direction;
   
   if (link_type == "ALPHA1-6" || link_type == "BETA1-6")
      needs_O6_manip = true;

   if (needs_O6_manip) { 
      try { 
	 // We need to find the current torsion around C5-C6, temporarily
	 // move the O6 of the residue to which we are linking, then
	 // rotate rotate it and r back to match current torsion.
	 // Fix-up needed for PDBv3
	 CAtom *at_O5 = residue_ref->GetAtom(" O5 ");
	 CAtom *at_C5 = residue_ref->GetAtom(" C5 ");
	 CAtom *at_C6 = residue_ref->GetAtom(" C6 ");
	 at_O6 = residue_ref->GetAtom(" O6 ");
	 if (at_O5 && at_C5 && at_C6 && at_O6) {
	    coot::atom_quad quad(at_O5, at_C5, at_C6, at_O6);
	    current_torsion = quad.torsion();
	    double diff = clipper::Util::d2rad(template_torsion - current_torsion);
	    clipper::Coord_orth base;
	    base         = clipper::Coord_orth(at_C5->x, at_C5->y, at_C5->z);
	    origin_shift = clipper::Coord_orth(at_C6->x, at_C6->y, at_C6->z);
	    position =     clipper::Coord_orth(at_O6->x, at_O6->y, at_O6->z);
	    direction = origin_shift - base;
	    clipper::Coord_orth new_pos =
	       coot::util::rotate_round_vector(direction, position,
					       origin_shift, diff);
	    at_O6->x = new_pos.x();
	    at_O6->y = new_pos.y();
	    at_O6->z = new_pos.z();
	 }
      }
      catch (std::runtime_error rte) {
	 std::cout << "WARNING:: " << rte.what() << std::endl;
      }
   }
   
   r = get_residue_raw();

   if (needs_O6_manip) {
      if (r) { 
	 // now rotate r and O6 back to current_torsion
	 if (at_O6) {
	    position = clipper::Coord_orth(at_O6->x, at_O6->y, at_O6->z);
	    double diff = clipper::Util::d2rad(template_torsion - current_torsion);
	    clipper::Coord_orth new_pos =
	       coot::util::rotate_round_vector(direction, position, origin_shift, -diff);

	    at_O6->x = new_pos.x();
	    at_O6->y = new_pos.y();
	    at_O6->z = new_pos.z();

	    PPCAtom residue_atoms = 0;
	    int n_residue_atoms;
	    r->GetAtomTable(residue_atoms, n_residue_atoms);
	    for (unsigned int i=0; i<n_residue_atoms; i++) {
	       CAtom *at = residue_atoms[i];
	       clipper::Coord_orth p(at->x, at->y, at->z);
	       clipper::Coord_orth n =
		  coot::util::rotate_round_vector(direction, p, origin_shift, -diff);
	       at->x = n.x();
	       at->y = n.y();
	       at->z = n.z();
	    }
	 }
      } 
   }
   
   return r;
}

// This can return NULL if we were unable to make the residue to be attached.
CResidue *
coot::beam_in_linked_residue::get_residue_raw() const {

   CResidue *r = NULL;
   if (! have_template) {
      std::cout << "WARNING:: no template" << std::endl;
   } else {
      // this mean that comp_id_ref, template_res_ref and
      // template_res_mov are correctly set.

      // depends on the comp_id of the residue_ref.
      // 
      std::vector<std::string> lsq_atom_names_ref = 
	 make_reference_atom_names(comp_id_ref);
      std::vector<std::string> lsq_atom_names_match = 
	 make_reference_atom_names(comp_id_ref);
      if (lsq_atom_names_ref.size() == 0 || lsq_atom_names_ref.size() != lsq_atom_names_match.size()) {
	 std::cout << "WARNING:: reference atoms  for LSQing match problem" << std::endl;
      } else {
	 // fit template_res_ref to residue_ref and move the atoms of template_res_mov
	 //
	 // util::create_mmdbmanager_from_residue(NULL, template_res_ref)->WritePDBASCII("template_res_ref-pre-1.pdb");
	 // util::create_mmdbmanager_from_residue(NULL, template_res_mov)->WritePDBASCII("template_res_mov-pre-1.pdb");
	 // util::create_mmdbmanager_from_residue(NULL, residue_ref)->WritePDBASCII("residue_ref-pre-1.pdb");
	 
	 bool status = lsq_fit(template_res_ref, residue_ref, template_res_mov,
			       lsq_atom_names_ref, lsq_atom_names_match);

	 // debug
	 lsq_fit(template_res_ref, residue_ref, template_res_ref,
		 lsq_atom_names_ref, lsq_atom_names_match);
	 
	 if (status) { 
	    r = template_res_mov;

	    // util::create_mmdbmanager_from_residue(NULL, template_res_ref)->WritePDBASCII("template_res_ref-post-1.pdb");
	    // util::create_mmdbmanager_from_residue(NULL, template_res_mov)->WritePDBASCII("template_res_mov-post-1.pdb");
	    // util::create_mmdbmanager_from_residue(NULL, residue_ref)->WritePDBASCII("residue_ref-post-1.pdb");

	    // Now, if r is a BMA, but we actually want a NAG (or
	    // so) then we need to get a NAG from the dictionary
	    // and LSQ it onto r.  And then replace r.
	    // (comp_id_new is set in the constructor).
	    // 
	    std::string r_res_name(r->GetResName());
	    // std::cout << "DEBUG:: comparing " << r_res_name << " with wanted " << comp_id_new << std::endl;
	    if (r_res_name != comp_id_new) {

	       // Something strange happens with the atom indices,
	       // CResidue *r_new = geom_p->get_residue(comp_id_new, 1);

	       // Try getting a molecule, then extracting the residue
	       // (yes, that works).
	       //
	       CMMDBManager *r_mol = geom_p->mol_from_dictionary(comp_id_new, 1);
	       if (r_mol) {
		  CResidue *r_new = coot::util::get_first_residue(r_mol);
	       
		  if (! r_new) {
		     std::cout << "WARNING:: couldn't get reference residue coords for "
			       << comp_id_new << " substituting "
			       << r_res_name << std::endl;
		  } else {
		     // happy path, lsq_fit: reference_res matcher_res moving_res atom_names
		     bool state = lsq_fit(r_new, r, r_new, lsq_atom_names_ref, lsq_atom_names_match);
		     if (state)
			r = r_new;
		     else 
			std::cout << "WARNING:: couldn't match coords for "
				  << comp_id_new << " substituting "
				  << r_res_name << std::endl;
		  }
	       }
	    }
	 } 
      }
   }

   if (r) {
      try { 
	 // apply the mods given the link type
	 std::pair<coot::protein_geometry::chem_mod, coot::protein_geometry::chem_mod>
	    mods = geom_p->get_chem_mods_for_link(link_type);

	 std::string res_name_ref = residue_ref->GetResName();
	 for (unsigned int i=0; i<mods.first.atom_mods.size(); i++) { 
	    if (mods.first.atom_mods[i].function == CHEM_MOD_FUNCTION_DELETE) {
	       std::string atom_name = mods.first.atom_mods[i].atom_id;
	       // now we need to expand the atom_id;
	       std::string at_name = atom_id_mmdb_expand(atom_name, res_name_ref);
	       // std::cout << ".... delete atom \"" << at_name << "\" in residue_ref"
	       // << std::endl;
	       delete_atom(residue_ref, at_name);
	    }
	 }
	 
	 std::string res_name_new = r->GetResName();
	 for (unsigned int i=0; i<mods.second.atom_mods.size(); i++) { 
	    if (mods.second.atom_mods[i].function == CHEM_MOD_FUNCTION_DELETE) {
	       std::string atom_name = mods.second.atom_mods[i].atom_id;
	       // now we need to expand the atom_id;
	       std::string at_name = atom_id_mmdb_expand(atom_name, res_name_new);
 	       delete_atom(r, at_name);
	    }
	 }
      }
      catch (std::runtime_error rte) {
	 // no chem mod for that link, that's fine.
	 
	 // std::cout << "DEBUG:: no chem mod for link " << link_type
	 // << " - that's OK" << std::endl;
      } 
   }
   // std::cout << "get_residue_raw() returns " << r << std::endl;
   return r;
}

// fit template res ref (i.e. matcher_res) to residue_ref and move the
// atoms of template_res_mov.
// 
bool
coot::beam_in_linked_residue::lsq_fit(CResidue *ref_res,
				      CResidue *matcher_res,
				      CResidue *mov_res,
				      const std::vector<std::string> &lsq_atom_names_ref,
				      const std::vector<std::string> &lsq_atom_names_match) const {

   bool status = false; 
   std::vector<CAtom *> va_1 = get_atoms(    ref_res, lsq_atom_names_ref);
   std::vector<CAtom *> va_2 = get_atoms(matcher_res, lsq_atom_names_match);

   if (va_1.size() != lsq_atom_names_ref.size()) {
      std::cout << "Mismatch atoms length for " << comp_id_ref << " in "
		<< "get_residue() (c.f. reference atoms) "
		<< va_1.size() << " need " << lsq_atom_names_ref.size()
		<< std::endl;
   } else {
      if (va_1.size() != va_2.size()) {
	 std::cout << "Mismatch atoms length for " << comp_id_ref << " in "
		   << "get_residue()" << std::endl;
      } else {
	 // Happy path
	 int n = lsq_atom_names_ref.size();
	 std::vector<clipper::Coord_orth> co_1(n);
	 std::vector<clipper::Coord_orth> co_2(n);
	 for (unsigned int iat=0; iat<va_1.size(); iat++) { 
	    co_1[iat] = clipper::Coord_orth(va_1[iat]->x, va_1[iat]->y, va_1[iat]->z);
	    co_2[iat] = clipper::Coord_orth(va_2[iat]->x, va_2[iat]->y, va_2[iat]->z);
	 }
	 clipper::RTop_orth rtop(co_1, co_2);
	 coot::util::transform_atoms(mov_res, rtop);
	 status = true;
      }
   }
   return status;
}

// apply the chem mod (specifically, the CHEM_MOD_FUNCTION_DELETE
// 
void
coot::beam_in_linked_residue::delete_atom(CResidue *res, const std::string &atom_name) const {

   PPCAtom residue_atoms = 0;
   int n_residue_atoms;
   bool deleted = false;
   res->GetAtomTable(residue_atoms, n_residue_atoms);
   for (unsigned int iat=0; iat<n_residue_atoms; iat++) {
      CAtom *at = residue_atoms[iat];
      if (at) {  // unneeded precaution?
	 std::string at_name(at->name);
	 if (at_name == atom_name) {
	    // std::cout << "..... delete_atom() deleting atom with index " << iat
	    // << " and  name \"" << at_name << "\"" << std::endl;
	    delete at;
	    at = NULL;
	    deleted = true;
	 }
      }
   }

   residue_atoms = NULL;
   res->GetAtomTable(residue_atoms, n_residue_atoms);
   std::string rn = res->GetResName();
   for (unsigned int iat=0; iat<n_residue_atoms; iat++) { 
      CAtom *at = residue_atoms[iat];
   }
   
   if (deleted)
      res->TrimAtomTable();
}

std::string
coot::beam_in_linked_residue::atom_id_mmdb_expand(const std::string &atom_id,
						  const std::string &res_name) const {


   std::string atom_id_expanded = geom_p->atom_id_expand(atom_id, res_name);
   return atom_id_expanded;
} 


std::vector<CAtom *>
coot::beam_in_linked_residue::get_atoms(CResidue *residue_p,
					const std::vector<std::string> &names) const {

   std::vector<CAtom *> v;
   PPCAtom residue_atoms = 0;
   int n_residue_atoms;
   residue_p->GetAtomTable(residue_atoms, n_residue_atoms);
   for (unsigned int iname=0; iname<names.size(); iname++) {
      for (unsigned int iat=0; iat<n_residue_atoms; iat++) {
	 std::string atom_name(residue_atoms[iat]->GetAtomName());
	 if (atom_name == names[iname]) {
	    v.push_back(residue_atoms[iat]);
	    break;
	 } 
      }
   }
   return v;

} 




std::vector<std::string>
coot::beam_in_linked_residue::make_reference_atom_names(const std::string &comp_id) const {

   // Should we pass the group here, instead of simply enumerating all
   // the possible pyranoses (etc.?)

   std::vector<std::string> lsq_reference_atom_names;
   if (comp_id == "ASN") {
      lsq_reference_atom_names.push_back(" ND2");
      lsq_reference_atom_names.push_back(" OD1");
      lsq_reference_atom_names.push_back(" CG ");
      lsq_reference_atom_names.push_back(" CB ");
   }
   if (comp_id == "NAG") {
      lsq_reference_atom_names.push_back(" C1 ");
      lsq_reference_atom_names.push_back(" C2 ");
      lsq_reference_atom_names.push_back(" C3 ");
      lsq_reference_atom_names.push_back(" C4 ");
      lsq_reference_atom_names.push_back(" C5 ");
      lsq_reference_atom_names.push_back(" O5 ");
   }
   if (comp_id == "MAN" || comp_id == "BMA") {
      lsq_reference_atom_names.push_back(" C1 ");
      lsq_reference_atom_names.push_back(" C2 ");
      lsq_reference_atom_names.push_back(" C3 ");
      lsq_reference_atom_names.push_back(" C4 ");
      lsq_reference_atom_names.push_back(" C5 ");
      lsq_reference_atom_names.push_back(" O5 ");
   }
   if (comp_id == "SIA") {
      lsq_reference_atom_names.push_back(" C2 ");
      lsq_reference_atom_names.push_back(" C3 ");
      lsq_reference_atom_names.push_back(" C4 ");
      lsq_reference_atom_names.push_back(" C5 ");
      lsq_reference_atom_names.push_back(" C6 ");
      lsq_reference_atom_names.push_back(" O6 ");
   }
   return lsq_reference_atom_names;
} 

CResidue *
coot::beam_in_linked_residue::get_residue(const std::string &comp_id,
					  CMMDBManager*mol) const {

   CResidue *r = NULL;
   int imod = 1;
   CModel *model_p = mol->GetModel(imod);
   CChain *chain_p;
   int n_chains = model_p->GetNumberOfChains();
   for (int ichain=0; ichain<n_chains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      int nres = chain_p->GetNumberOfResidues();
      CResidue *residue_p;
      CAtom *at;
      for (int ires=0; ires<nres; ires++) { 
	 residue_p = chain_p->GetResidue(ires);
	 std::string res_name(residue_p->GetResName());
	 if (res_name == comp_id) {
	    r = residue_p;
	    break;
	 }
      }
      if (r)
	 break;
   }

   return r;
}

std::ostream&
coot::operator<<(std::ostream &o, const linked_residue_t &lr) {

   if (lr.residue)
      o << lr.link_type << " " << lr.residue->GetResName() << " "
	<< coot::residue_spec_t(lr.residue);
   else 
      o << lr.link_type << " " << lr.residue_name;
   return o;

} 


// should this be part of protein_geometry?
// 
coot::glyco_tree_t::glyco_tree_t(CResidue *residue_p, CMMDBManager *mol,
				 coot::protein_geometry *geom_p_in) {

   float dist_crit = 3.0; // A

   if (residue_p) { 
      geom_p = geom_p_in;

      std::queue<CResidue *> q;
      std::vector<CResidue *> considered;
      std::vector<CResidue *> linked_residues;
      
      if (is_pyranose(residue_p) || std::string(residue_p->name) == "ASN")
	 q.push(residue_p);

      while (q.size()) {
	 CResidue *test_residue = q.front();
	 q.pop();
	 std::vector<CResidue *> residues = coot::residues_near_residue(test_residue, mol, dist_crit);
	 for (unsigned int ires=0; ires<residues.size(); ires++) {
	    if (is_pyranose(residues[ires]) || std::string(residues[ires]->name) == "ASN") {
	       if (std::find(considered.begin(), considered.end(), residues[ires]) == considered.end()) { 
		  q.push(residues[ires]);
		  linked_residues.push_back(residues[ires]);
	       } 
	    }
	    considered.push_back(residues[ires]);
	 }
      }

      std::sort(linked_residues.begin(), linked_residues.end(), residue_comparitor);

      bool have_ASN_rooted_tree = false;
      std::cout << ":::::::::: " << linked_residues.size() << " glycan/ASN residues" << std::endl;

      for (unsigned int ires=0; ires<linked_residues.size(); ires++) { 
	 std::string residue_name(linked_residues[ires]->name);
	 if (residue_name == "ASN") {
	    tree<coot::linked_residue_t> tr = find_ASN_rooted_tree(linked_residues[ires], linked_residues);
	    if (tr.size() > 1) {
	       // std::cout << "found tree with " << tr.size() << " nodes " << std::endl;
	       have_ASN_rooted_tree = true;
	       compare_vs_allowed_trees(tr);
	    }
	 } 
      }

      if (! have_ASN_rooted_tree) {
	 tree<coot::linked_residue_t> tr = find_stand_alone_tree(linked_residues);
      } 
   }
}

bool
coot::glyco_tree_t::is_pyranose(CResidue *residue_p) const {

   bool is_pyranose = false;
   try { 
      std::string group = geom_p->get_group(residue_p);
      if (group == "pyranose" || group == "D-pyranose" || group == "L-pyranose")
	 is_pyranose = true;
   }
   catch (std::runtime_error rte) {
      std::cout << "ERROR::" << rte.what() << std::endl;
   } 

   return is_pyranose;
} 


// find tree rooted on residue_p.
// 
// residue_p is a member of residues.
// 
tree<coot::linked_residue_t> 
coot::glyco_tree_t::find_ASN_rooted_tree(CResidue *residue_p,
					 const std::vector<CResidue *> &residues) const {

   return find_rooted_tree(residue_p, residues);
}

// find tree rooted on residue_p.
// 
// residue_p is a member of residues.
// 
tree<coot::linked_residue_t> 
coot::glyco_tree_t::find_rooted_tree(CResidue *residue_p,
				     const std::vector<CResidue *> &residues) const {

   linked_residue_t first_res(residue_p, "");
   tree<linked_residue_t> glyco_tree;
   tree<linked_residue_t>::iterator top = glyco_tree.insert(glyco_tree.begin(), first_res);
   bool something_added = true; // initial value 
   tree<linked_residue_t>::iterator this_iterator = top; // initial value
   std::vector<std::pair<bool, CResidue *> > done_residues(residues.size());
   for (unsigned int i=0; i<residues.size(); i++) {
      // Mark as not yet done, except if it is the first one, we have
      // inserted that one already.
      if (residues[i] == residue_p) {
	 done_residues[i] = std::pair<bool, CResidue *>(1, residues[i]);
      } else {
	 done_residues[i] = std::pair<bool, CResidue *>(0, residues[i]);
      }
   }
      
   while (something_added) {
      something_added = false; 
      for (unsigned int ires=0; ires<done_residues.size(); ires++) {

	 if (! done_residues[ires].first) { 
	    // iterate over the residues already placed in the tree
	    // 
	    tree<linked_residue_t>::iterator it;
	    for (it=glyco_tree.begin(); it != glyco_tree.end(); it++) {
	       if (it->residue != done_residues[ires].second) {
		  if (0)
		     std::cout << "      considering if "
			       << coot::residue_spec_t(done_residues[ires].second)
			       << "  was linked to tree residue "
			       << coot::residue_spec_t(it->residue) << std::endl;
		  // the residue order here is carefully considered
		  std::pair<std::string, bool> link =
		     geom_p->find_glycosidic_linkage_type_with_order_switch(it->residue,
									    done_residues[ires].second);
		  if (link.first != "") {

		     if (link.first == "NAG-ASN") {
			if (link.second == true) {
			   std::cout << "   Adding "
				     << coot::residue_spec_t(done_residues[ires].second)
				     << " " << "via NAG-ASN" << " to parent "
				     << coot::residue_spec_t(it->residue)
				     << std::endl;
			   linked_residue_t this_linked_residue(done_residues[ires].second, "NAG-ASN");
			   glyco_tree.append_child(it, this_linked_residue);
			   something_added = true;
			   done_residues[ires].first = true;
			}
		     } else {
			std::cout << "found link type " << link.first << " order-switch " 
				  << link.second << std::endl;
			linked_residue_t this_linked_residue(done_residues[ires].second, link.first);
			this_linked_residue.order_switch = link.second;
			std::cout << "   Adding " << coot::residue_spec_t(done_residues[ires].second)
				  << " via " << link.first << " to parent " 
				  << coot::residue_spec_t(it->residue)
				  << std::endl;
			glyco_tree.append_child(it, this_linked_residue);
			something_added = true;
			done_residues[ires].first = true;
		     }
		  }
	       }
	    }
	 }
      }
   }
   if (glyco_tree.size() > 1)
      print(glyco_tree);
   return glyco_tree;
}

tree<coot::linked_residue_t>
coot::glyco_tree_t::find_stand_alone_tree(const std::vector<CResidue *> &residues) const {

   // The task is the find the root of the tree, then we simply call
   // find_rooted_tree with that residue.
   
   tree<linked_residue_t> tr;
   if (!residues.size())
      return tr;
   
   std::vector<std::pair<bool, CResidue *> > done_residues(residues.size());
   for (unsigned int i=0; i<residues.size(); i++)
      done_residues[i] = std::pair<bool, CResidue *>(0, residues[i]);

   CResidue *current_head = residues[0];
   bool something_added = true; 
   while (something_added) {
      something_added = false;
      for (unsigned int ires=0; ires<residues.size(); ires++) {
	 if (residues[ires] != current_head) {
	    std::pair<std::string, bool> link =
	       geom_p->find_glycosidic_linkage_type_with_order_switch(current_head,
								      done_residues[ires].second);
	    std::cout << "find_stand_alone_tree(): glyco_link test on " << coot::residue_spec_t(current_head)
		      << " and " << coot::residue_spec_t(residues[ires]) << " returns "
		      << "\"" << link.first << "\" " << link.second << std::endl;
	    if (link.first != "") { 
	       if (link.second) {
		  std::cout << ".... reseting current_head to " << coot::residue_spec_t(residues[ires])
			    << std::endl;
		  current_head = residues[ires];
		  something_added = true;
		  break;
	       }
	    } 
	 } 
      }
   }

   std::cout << "----------- find_stand_alone_tree() calling find_rooted_tree with current_head "
	     << coot::residue_spec_t(current_head) << std::endl;
   std::cout << "and residues: " << std::endl;
   for (unsigned int i=0; i<residues.size(); i++) { 
      std::cout << "   " << coot::residue_spec_t(residues[i]) << std::endl;
   }
    
   tr = find_rooted_tree(current_head, residues);

   return tr;
}


void 
coot::glyco_tree_t::print(const tree<linked_residue_t> &glyco_tree) const {

   tree<linked_residue_t>::iterator it, this_one;
   for (it=glyco_tree.begin(); it != glyco_tree.end(); it++) {
      int n_space = 36;
      this_one = it;
      bool has_parent = true;
      while (has_parent) { 
	 if (! this_one.node->parent) { 
	    has_parent = false;
	 } else { 
	    n_space -= 4;
	    this_one = this_one.node->parent;
	 } 
      }
      std::string s;
      for (int i=0; i<n_space; i++)
	 s += " ";
      std::cout << "   " << s << " " << *it << std::endl;
   }
}

void
coot::glyco_tree_t::compare_vs_allowed_trees(const tree<linked_residue_t> &tr_for_testing) const {

   tree<coot::linked_residue_t> omt = oligomannose_tree();
   tree<coot::linked_residue_t> hybrid = hybrid_tree();
   tree<coot::linked_residue_t> complex = complex_tree();

   if (0) { 
      std::cout << "Oligomannose" << std::endl;
      print(omt);
      std::cout << "Hybrid" << std::endl;
      print(hybrid);
      std::cout << "Complex" << std::endl;
      print(complex);
   }

   if (compare_trees(tr_for_testing, omt))
      std::cout << "This tree matches \"oligomannose\"" << std::endl;
   else 
      std::cout << "This tree is not oligomannose" << std::endl;
   if (compare_trees(tr_for_testing, hybrid))
      std::cout << "This tree matches \"hybrid\"" << std::endl;
   else 
      std::cout << "This tree is not \"hybrid\"" << std::endl;
   if (compare_trees(tr_for_testing, complex))
      std::cout << "This tree matches \"complex\"" << std::endl;
   else 
      std::cout << "This tree is not \"complex\"" << std::endl;

}

bool
coot::glyco_tree_t::compare_trees(const tree<linked_residue_t> &tr_for_testing,
				  const tree<linked_residue_t> &ref_tree) const {

   tree<linked_residue_t>::leaf_iterator it_leaf;
   tree<linked_residue_t>::iterator it_ref;

//    std::cout << "Ref tree" << std::endl;
//    print(ref_tree);

//    std::cout << "tree for testing" << std::endl;
//    print(tr_for_testing);

   
   // leaf iterators in the ref_tree that have already 
   std::vector<tree<linked_residue_t>::iterator> done_iterators;

   // get a leaf in tr_for_testing
   //    find a potential corresponding leaf in ref_tree (not in done_iterators)
   //      compare the parents of the leaf
   //      if they are the same, add that (reference) leaf iterator to done_iterators
   //
   int n_leaves = 0;
   int n_matched_leaves = 0;
   for (it_leaf=tr_for_testing.begin_leaf(); it_leaf != tr_for_testing.end_leaf(); it_leaf++) {
      
      // std::cout << "------ new leaf for testing --- Here 1 " << *it_leaf << std::endl;
      bool found_a_match = false;
      n_leaves++;
      for (it_ref=ref_tree.begin(); it_ref != ref_tree.end(); it_ref++) {
	 if (std::find(done_iterators.begin(), done_iterators.end(), it_ref) ==
	     done_iterators.end()) {
	    if (*it_leaf == *it_ref) {
	       tree<linked_residue_t>::iterator it_p, it_p_ref;
	       it_p = it_leaf;
	       it_p_ref = it_ref;
	       bool parents_match = true;
	       while (parents_match && it_p.node->parent && it_p_ref.node->parent) {
		  it_p = it_p.node->parent;
		  it_p_ref = it_p_ref.node->parent;
		  if (*it_p == *it_p_ref) {
		     // Good...
		  } else {
		     parents_match = false;
		     break;
		  } 
	       }

	       if (parents_match) {
		  if (0) 
		     std::cout << "   all match " << parents_match << " "
			       << *it_leaf << " and " << *it_ref
			       << std::endl;
		  found_a_match = true;
		  n_matched_leaves++;
		  done_iterators.push_back(it_p_ref);
	       } 
	    } else {
	       // std::cout << "      Here 3 Result: b no leaf match " << std::endl;
	    } 
	 }
      }

      if (! found_a_match) {
	 // std::cout << "No match found - breaking" << std::endl;
	 break;
      }
   }

   if (0)
      std::cout << "n_leaves: " << n_leaves << " n_matched_leaves: " << n_matched_leaves
		<< std::endl;

   bool success = false;
   if (n_matched_leaves == n_leaves)
      success = true;

   return success;
}

tree<coot::linked_residue_t>
coot::glyco_tree_t::oligomannose_tree() const {

   // make oligomannose
   linked_residue_t ASN    ("ASN", "");
   linked_residue_t NAG_1  ("NAG", "NAG-ASN");  // parent is ASN
   linked_residue_t NAG_2  ("NAG", "BETA1-4");  // parent is NAG_1
   linked_residue_t MAN_3  ("BMA", "BETA1-4");  // parent is NAG_2
   linked_residue_t MAN_4_1("MAN", "ALPHA1-6"); // parent is MAN_3
   linked_residue_t MAN_4_2("MAN", "ALPHA1-6"); // parent is MAN_4_1
   linked_residue_t MAN_4_3("MAN", "ALPHA1-2"); // parent is MAN_4_2
   linked_residue_t MAN_5_1("MAN", "ALPHA1-3"); // parent is MAN_4_1
   linked_residue_t MAN_5_2("MAN", "ALPHA1-2"); // parent is MAN_5_1
   linked_residue_t MAN_6_1("MAN", "ALPHA1-3"); // parent is MAN_3
   linked_residue_t MAN_6_2("MAN", "ALPHA1-2"); // parent is MAN_6_1
   linked_residue_t MAN_6_3("MAN", "ALPHA1-2"); // parent is MAN_6_2
   linked_residue_t GLU_6_4("GLC", "ALPHA1-3"); // parent is MAN_6_3
   linked_residue_t GLU_6_5("GLC", "ALPHA1-3"); // parent is MAN_6_4
   linked_residue_t GLU_6_6("GLC", "ALPHA1-2"); // parent is MAN_6_5

   tree<linked_residue_t> omt;
   tree<linked_residue_t>::iterator asn     = omt.insert(omt.begin(), ASN);
   tree<linked_residue_t>::iterator nag_1   = omt.append_child(asn,     NAG_1);
   tree<linked_residue_t>::iterator nag_2   = omt.append_child(nag_1,   NAG_2);
   tree<linked_residue_t>::iterator man_3   = omt.append_child(nag_2,   MAN_3);
   tree<linked_residue_t>::iterator man_4_1 = omt.append_child(man_3,   MAN_4_1);
   tree<linked_residue_t>::iterator man_4_2 = omt.append_child(man_4_1, MAN_4_2);
   tree<linked_residue_t>::iterator man_4_3 = omt.append_child(man_4_2, MAN_4_3);
   tree<linked_residue_t>::iterator man_5_1 = omt.append_child(man_4_1, MAN_5_1);
   tree<linked_residue_t>::iterator man_5_2 = omt.append_child(man_5_1, MAN_5_2);
   tree<linked_residue_t>::iterator man_6_1 = omt.append_child(man_3,   MAN_6_1);
   tree<linked_residue_t>::iterator man_6_2 = omt.append_child(man_6_1, MAN_6_2);
   tree<linked_residue_t>::iterator man_6_3 = omt.append_child(man_6_2, MAN_6_3);
   tree<linked_residue_t>::iterator glu_6_4 = omt.append_child(man_6_3, GLU_6_4);
   tree<linked_residue_t>::iterator glu_6_5 = omt.append_child(glu_6_4, GLU_6_5);
   tree<linked_residue_t>::iterator glu_6_6 = omt.append_child(glu_6_5, GLU_6_6);

   return omt;
} 

tree<coot::linked_residue_t>
coot::glyco_tree_t::hybrid_tree() const {

   linked_residue_t ASN    ("ASN", "");
   linked_residue_t NAG_1  ("NAG", "NAG-ASN");  // parent is ASN
   linked_residue_t NAG_2  ("NAG", "BETA1-4");  // parent is NAG_1
   linked_residue_t MAN_3  ("BMA", "BETA1-4");  // parent is NAG_2
   linked_residue_t MAN_4_1("MAN", "ALPHA1-6"); // parent is MAN_3
   linked_residue_t MAN_4_2("MAN", "ALPHA1-6"); // parent is MAN_4_1
   linked_residue_t MAN_5_1("MAN", "ALPHA1-3"); // parent is MAN_4_1
   linked_residue_t NAG_4  ("NAG", "BETA1-4");  // parent is MAN_3
   linked_residue_t MAN_6_1("MAN", "ALPHA1-3"); // parent is MAN_3
   linked_residue_t NAG_6_2("NAG", "BETA1-4");  // parent is MAN_6_1
   linked_residue_t GAL_6_3("GAL", "BETA1-4");  // parent is NAG_6_2
   linked_residue_t NAG_7_1("NAG", "BETA1-2");  // parent is MAN_6_1
   linked_residue_t GAL_7_2("GAL", "BETA1-4");  // parent is NAG_7_1
   linked_residue_t SIA_7_3("SIA", "ALPHA1-3"); // parent is GAL_7_2
   linked_residue_t FUC_1  ("FUC", "BETA1-6");  // parent is NAG_1
   
   tree<linked_residue_t> t;
   tree<linked_residue_t>::iterator asn     = t.insert(t.begin(), ASN);
   tree<linked_residue_t>::iterator nag_1   = t.append_child(asn,     NAG_1);
   tree<linked_residue_t>::iterator fuc_1   = t.append_child(nag_1,   FUC_1);
   tree<linked_residue_t>::iterator nag_2   = t.append_child(nag_1,   NAG_2);
   tree<linked_residue_t>::iterator man_3   = t.append_child(nag_2,   MAN_3);
   tree<linked_residue_t>::iterator man_4_1 = t.append_child(man_3,   MAN_4_1);
   tree<linked_residue_t>::iterator man_4_2 = t.append_child(man_4_1, MAN_4_2);
   tree<linked_residue_t>::iterator man_5_1 = t.append_child(man_4_1, MAN_5_1);
   tree<linked_residue_t>::iterator nag_4   = t.append_child(man_3,   NAG_4);
   tree<linked_residue_t>::iterator man_6_1 = t.append_child(man_3,   MAN_6_1);
   tree<linked_residue_t>::iterator nag_6_2 = t.append_child(man_6_1, NAG_6_2);
   tree<linked_residue_t>::iterator gal_6_3 = t.append_child(nag_6_2, GAL_6_3);
   tree<linked_residue_t>::iterator nag_7_1 = t.append_child(man_6_1, NAG_7_1);
   tree<linked_residue_t>::iterator gal_7_2 = t.append_child(nag_7_1, GAL_7_2);
   tree<linked_residue_t>::iterator sia_7_3 = t.append_child(gal_7_2, SIA_7_3);

   return t;
}

tree<coot::linked_residue_t>
coot::glyco_tree_t::complex_tree() const {

   linked_residue_t ASN    ("ASN", "");
   linked_residue_t NAG_1  ("NAG", "NAG-ASN");  // parent is ASN
   linked_residue_t FUC_1  ("FUC", "BETA1-6");  // parent is NAG_1
   linked_residue_t FUC_2  ("FUC", "BETA1-3");  // parent is NAG_1
   linked_residue_t NAG_2  ("NAG", "BETA1-4");  // parent is NAG_1
   linked_residue_t MAN_3  ("BMA", "BETA1-4");  // parent is NAG_2
   linked_residue_t NAG_4  ("NAG", "BETA1-4");  // parent is MAN_3

   linked_residue_t XYL_4  ("XYL", "BETA1-2");  // parent is MAN_3
   
   linked_residue_t MAN_4_1("MAN", "ALPHA1-6"); // parent is MAN_3
   linked_residue_t NAG_4_2("NAG", "ALPHA1-6"); // parent is MAN_4_1
   linked_residue_t GAL_4_3("MAN", "BETA1-4");  // parent is NAG_4_2

   linked_residue_t NAG_5_1("NAG", "ALPHA1-2"); // parent is MAN_4_1
   linked_residue_t GAL_5_2("GAL", "BETA1-4");  // parent is NAG_5_1
   
   linked_residue_t MAN_6_1("MAN", "ALPHA1-3"); // parent is MAN_3
   linked_residue_t NAG_6_2("NAG", "BETA1-4");  // parent is MAN_6_1
   linked_residue_t GAL_6_3("GAL", "BETA1-4");  // parent is NAG_6_2
   linked_residue_t NAG_7_1("NAG", "BETA1-2");  // parent is MAN_6_1
   linked_residue_t GAL_7_2("GAL", "BETA1-4");  // parent is NAG_7_1


   tree<linked_residue_t> t;
   tree<linked_residue_t>::iterator asn     = t.insert(t.begin(), ASN);
   tree<linked_residue_t>::iterator nag_1   = t.append_child(asn,     NAG_1);
   tree<linked_residue_t>::iterator fuc_1   = t.append_child(nag_1,   FUC_1);
   tree<linked_residue_t>::iterator fuc_2   = t.append_child(nag_1,   FUC_2);
   tree<linked_residue_t>::iterator nag_2   = t.append_child(nag_1,   NAG_2);
   tree<linked_residue_t>::iterator man_3   = t.append_child(nag_2,   MAN_3);
   tree<linked_residue_t>::iterator man_4_1 = t.append_child(man_3,   MAN_4_1);
   tree<linked_residue_t>::iterator nag_4_2 = t.append_child(man_4_1, NAG_4_2);
   tree<linked_residue_t>::iterator gal_4_3 = t.append_child(nag_4_2, GAL_4_3);
   
   tree<linked_residue_t>::iterator nag_5_1 = t.append_child(man_4_1, NAG_5_1);
   tree<linked_residue_t>::iterator gal_5_2 = t.append_child(nag_5_1, GAL_5_2);
   
   tree<linked_residue_t>::iterator nag_4   = t.append_child(man_3,   NAG_4);
   tree<linked_residue_t>::iterator xyl_4   = t.append_child(man_3,   XYL_4);

   tree<linked_residue_t>::iterator man_6_1 = t.append_child(man_3,   MAN_6_1);
   tree<linked_residue_t>::iterator nag_6_2 = t.append_child(man_6_1, NAG_6_2);
   tree<linked_residue_t>::iterator gal_6_3 = t.append_child(nag_6_2, GAL_6_3);
   tree<linked_residue_t>::iterator nag_7_1 = t.append_child(man_6_1, NAG_7_1);
   tree<linked_residue_t>::iterator gal_7_2 = t.append_child(nag_7_1, GAL_7_2);


   return t;
}