
#include <map>
#include <algorithm>

// these are needed to compile molecule-compile-info.h:
// 
#include "coords/Cartesian.h"
#include "coords/mmdb-extras.h"
#include "coords/mmdb-crystal.h"

// morphing
#include "coot-utils/coot-coord-utils.hh"
#include "ligand/rigid-body.hh"

#include "molecule-class-info.h"

class atom_morph_info_t {
public:
   double weight;
   clipper::Coord_orth residue_centre;
   clipper::RTop_orth residue_rtop;
   atom_morph_info_t(const clipper::RTop_orth &rtop_in,
		     const clipper::Coord_orth &co_in,
		     double w_in) {
      weight = w_in;
      residue_rtop = rtop_in;
      residue_centre = co_in;
   }
   std::pair<clipper::RTop_orth, float> rtop_and_weight() const {
      return std::pair<clipper::RTop_orth, float> (residue_rtop, weight);
   } 
};

// return an index of the new restraint
int
molecule_class_info_t::add_extra_bond_restraint(coot::atom_spec_t atom_1,
						coot::atom_spec_t atom_2,
						double bond_dist, double esd) {
   int r = -1; // unset
   CAtom *at_1 = get_atom(atom_1);
   CAtom *at_2 = get_atom(atom_2);
   if (at_1) {
      int atom_index = -1;
      at_1->GetUDData(atom_sel.UDDAtomIndexHandle, atom_index); // set atom_index
      atom_1.int_user_data = atom_index;
   }
   if (at_2) {
      int atom_index = -1;
      at_2->GetUDData(atom_sel.UDDAtomIndexHandle, atom_index); // set atom_index
      atom_2.int_user_data = atom_index;
   }
   if (at_1 && at_2) { 
      coot::extra_restraints_t::extra_bond_restraint_t bond(atom_1, atom_2, bond_dist, esd);
      extra_restraints.bond_restraints.push_back(bond);
      update_extra_restraints_representation();
      r = extra_restraints.bond_restraints.size() -1;
   }
   return r;
}


void molecule_class_info_t::remove_extra_bond_restraint(coot::atom_spec_t atom_1, coot::atom_spec_t atom_2) {

   int n_bonds_pre = extra_restraints.bond_restraints.size();
   std::vector<coot::extra_restraints_t::extra_bond_restraint_t>::iterator it;
   for (it=extra_restraints.bond_restraints.begin(); it != extra_restraints.bond_restraints.end(); it++) { 
      if (((it->atom_1 == atom_1) &&
	   (it->atom_2 == atom_2)) ||
	  ((it->atom_2 == atom_1) &&
	   (it->atom_1 == atom_2))) {
	 extra_restraints.bond_restraints.erase(it);
	 std::cout << "deleted extra bond restraint " << atom_1 << " to " << atom_2 << std::endl;
	 // break;
      }
   }
   int n_bonds_post = extra_restraints.bond_restraints.size();
   std::cout << "DEBUG:: pre: " << n_bonds_pre << " post " << n_bonds_post << std::endl;
   update_extra_restraints_representation();
}

void
molecule_class_info_t::add_refmac_extra_restraints(const std::string &file_name) {

   coot::extra_restraints_t r;
   r.read_refmac_extra_restraints(file_name);
   extra_restraints.add_restraints(r);
   std::cout << "INFO:: add_refmac_extra_restraints(): have " << extra_restraints.bond_restraints.size()
	     << " bond restraints " << std::endl;
   update_extra_restraints_representation();
}

void
molecule_class_info_t::delete_extra_restraints_for_residue(const coot::residue_spec_t &rs) {

   unsigned int pre_n = extra_restraints.bond_restraints.size(); 
   extra_restraints.delete_restraints_for_residue(rs);
   unsigned int post_n = extra_restraints.bond_restraints.size();
   if (post_n != pre_n)
      update_extra_restraints_representation();
}


// 20131014 unused currently.
bool
spec_pair_comparer(const std::pair<coot::atom_spec_t, coot::atom_spec_t> &p1,
		   const std::pair<coot::atom_spec_t, coot::atom_spec_t> &p2) {

//    if (p1.first < p2.first) { 
//       return true;
//    } else { 
//       if (p2.first < p1.first) { 
// 	 return false;
//       } else {
// 	 return (p1.second < p2.second);
//       }
//    }
   
   if (p1.first < p2.first) {
      std::cout << "spec_pair_comparer A " << "[" << p1.first << " , " << p1.second << "]" << " < " "[" << p2.first << " , " << p2.second << "]" << std::endl;
      return true;
   } else { 
      if (p2.first < p1.first) { 
	 std::cout << "spec_pair_comparer B " << "[" << p2.first << " , " << p2.second << "]" " < " "[" << p1.first << " , " << p1.second << "]" << std::endl;
	 return false;
      } else {
	 bool v = (p1.second < p2.second);
	 if (v) {
	    std::cout << "spec_pair_comparer C " << "[" << p1.first << " , " << p1.second << "]" " < " "[" << p2.first << " , " << p2.second << "]" << std::endl;
	    return true;
	 } else {
	    std::cout << "spec_pair_comparer D " << "[" << p2.first << " , " << p2.second << "]" " < " "[" << p1.first << " , " << p1.second << "]" << std::endl;
	    return false;
	 }
      }
   }
}

void
molecule_class_info_t::delete_extra_restraints_worse_than(const double &n_sigma) {


   unsigned int pre_n = extra_restraints.bond_restraints.size();

   std::vector<coot::extra_restraints_t::extra_bond_restraint_t> ebrv_l;

   std::map<coot::atom_spec_t, CAtom *> atom_map;
   std::map<coot::atom_spec_t, CAtom *>::const_iterator it_1;
   std::map<coot::atom_spec_t, CAtom *>::const_iterator it_2;
   // first fill the dist_map and fill the atom_map as you do so.
   for (unsigned int i=0; i<extra_restraints.bond_restraints.size(); i++) {
      coot::extra_restraints_t::extra_bond_restraint_t &br = extra_restraints.bond_restraints[i];
      CAtom *at_1 = NULL;
      CAtom *at_2 = NULL;
      it_1 = atom_map.find(br.atom_1);
      it_2 = atom_map.find(br.atom_2);
      if (it_1 == atom_map.end()) {
	 at_1 = get_atom(br.atom_1);
	 atom_map[br.atom_1] = at_1;
      } else {
	 at_1 = it_1->second; // most of the hits, I hope
      }
      // and the same for 2:
      if (it_2 == atom_map.end()) {
	 at_2 = get_atom(br.atom_2);
	 atom_map[br.atom_2] = at_2;
      } else {
	 at_2 = it_2->second; 
      }
      if (at_1 && at_2) {
	 double dx = at_1->x - at_2->x;
	 double dy = at_1->y - at_2->y;
	 double dz = at_1->z - at_2->z;
	 double dist_sq = dx*dx + dy*dy + dz*dz;
	 double dist = sqrt(dist_sq);

	 double this_n_sigma = fabs((br.bond_dist -dist)/br.esd);
	 if (this_n_sigma < n_sigma) {
	    ebrv_l.push_back(br);
	 }
      } else {
	 if (! at_1)
	    std::cout << "WARNING: missing atom_1 " << br.atom_1 << " when constructing dist_map" << std::endl;
	 if (! at_2)
	    std::cout << "WARNING: missing atom_2 " << br.atom_2 << " when constructing dist_map" << std::endl;
      }
   }
   extra_restraints.bond_restraints = ebrv_l;

   // remove_if and erase formulation.  Should work.  Crashes for some reason.
   // 
   // extra_restraints.bond_restraints.erase(std::remove_if(extra_restraints.bond_restraints.begin(), extra_restraints.bond_restraints.end(), coot::extra_restraints_t::bond_eraser(dist_map, n_sigma)), extra_restraints.bond_restraints.end());

   unsigned int post_n = extra_restraints.bond_restraints.size();
   if (post_n != pre_n)
      update_extra_restraints_representation();
   std::cout << "INFO deleted : " << pre_n - post_n << " of " << pre_n << " extra bond restraints" << std::endl;
}

void
molecule_class_info_t::set_extra_restraints_prosmart_sigma_limits(double limit_low, double limit_high) {

   extra_restraints_representation.prosmart_restraint_display_limit_low  = limit_low;
   extra_restraints_representation.prosmart_restraint_display_limit_high = limit_high;
   
   // and redraw

   update_extra_restraints_representation();
}



void
molecule_class_info_t::clear_extra_restraints() {
   extra_restraints.clear();
} 

// return an index of the new restraint
int
molecule_class_info_t::add_extra_torsion_restraint(coot::atom_spec_t atom_1,
						   coot::atom_spec_t atom_2,
						   coot::atom_spec_t atom_3,
						   coot::atom_spec_t atom_4,
						   double torsion_angle, double esd, int period) {

   CAtom *at_1 = get_atom(atom_1);
   CAtom *at_2 = get_atom(atom_2);
   CAtom *at_3 = get_atom(atom_3);
   CAtom *at_4 = get_atom(atom_4);
   if (at_1) {
      int atom_index = -1;
      at_1->GetUDData(atom_sel.UDDAtomIndexHandle, atom_index); // set atom_index
      atom_1.int_user_data = atom_index;
   }
   if (at_2) {
      int atom_index = -1;
      at_2->GetUDData(atom_sel.UDDAtomIndexHandle, atom_index); // set atom_index
      atom_2.int_user_data = atom_index;
   }
   if (at_3) {
      int atom_index = -1;
      at_3->GetUDData(atom_sel.UDDAtomIndexHandle, atom_index); // set atom_index
      atom_3.int_user_data = atom_index;
   }
   if (at_4) {
      int atom_index = -1;
      at_4->GetUDData(atom_sel.UDDAtomIndexHandle, atom_index); // set atom_index
      atom_4.int_user_data = atom_index;
   }
   coot::extra_restraints_t::extra_torsion_restraint_t tors(atom_1, atom_2,
							    atom_3,atom_4,
							    torsion_angle, esd, period);
   extra_restraints.torsion_restraints.push_back(tors);
   update_extra_restraints_representation();
   return extra_restraints.torsion_restraints.size() -1;
}


int
molecule_class_info_t::morph_fit_all(const clipper::Xmap<float> &xmap_in, float transformation_average_radius) {

   int imod = 1;
   CModel *model_p = atom_sel.mol->GetModel(imod);
   CChain *chain_p;
   int n_neighb=2; // either side of central residue
   int n_chains = model_p->GetNumberOfChains();

   // the central residue and it's upstream and downstream neighbours (if it has them)
   std::vector<std::pair<CResidue *, std::vector<CResidue *> > > moving_residues;
   
   for (int ichain=0; ichain<n_chains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      int nres = chain_p->GetNumberOfResidues();

      for (int ires=0; ires<nres; ires++) { // residue-in-chain loop
	 std::vector<CResidue *> v; // up and downstream neighbours

	 for (int ifragres=-n_neighb; ifragres<=n_neighb; ifragres++) {
	    if (ifragres != 0) {
	       int idx = ires+ifragres;
	       if ((idx >=0) && (idx<nres)) { 
		  CResidue *r = chain_p->GetResidue(idx);
		  if (r)
		     v.push_back(r);
	       }
	    }
	 }
	 std::pair<CResidue *, std::vector<CResidue *> > p(chain_p->GetResidue(ires), v);
	 moving_residues.push_back(p);
      }
   }
   return morph_fit_residues(moving_residues, xmap_in, transformation_average_radius);
}

int
molecule_class_info_t::morph_fit_chain(const std::string &chain_id,
				       const clipper::Xmap<float> &xmap_in, float transformation_average_radius) {

   int imod = 1;
   CModel *model_p = atom_sel.mol->GetModel(imod);
   CChain *chain_p;
   int n_neighb=2; // either side of central residue
   int n_chains = model_p->GetNumberOfChains();

   // the central residue and it's upstream and downstream neighbours (if it has them)
   std::vector<std::pair<CResidue *, std::vector<CResidue *> > > moving_residues;
   
   for (int ichain=0; ichain<n_chains; ichain++) {
      chain_p = model_p->GetChain(ichain);
      if (std::string(chain_p->GetChainID()) == chain_id) { 
	 int nres = chain_p->GetNumberOfResidues();
	 for (int ires=0; ires<nres; ires++) { // residue-in-chain loop
	    std::vector<CResidue *> v; // up and downstream neighbours
	    
	    for (int ifragres=-n_neighb; ifragres<=n_neighb; ifragres++) {
	       if (ifragres != 0) {
		  int idx = ires+ifragres;
		  if ((idx >=0) && (idx<nres)) { 
		     CResidue *r = chain_p->GetResidue(idx);
		     if (r)
			v.push_back(r);
		  }
	       }
	    }
	    std::pair<CResidue *, std::vector<CResidue *> > p(chain_p->GetResidue(ires), v);
	    moving_residues.push_back(p);
	 }
      }
   }
   return morph_fit_residues(moving_residues, xmap_in, transformation_average_radius);

}

int
molecule_class_info_t::morph_fit_residues(const std::vector<coot::residue_spec_t> &residue_specs,
					  const clipper::Xmap<float> &xmap_in, float transformation_average_radius) {

   // fill this from specs:
   std::vector<std::pair<CResidue *, std::vector<CResidue *> > > moving_residues;
   for (unsigned int i=0; i<residue_specs.size(); i++) { 
      CResidue *r = get_residue(residue_specs[i]);
      if (r) { 
	 std::vector<CResidue *> env_residues =
	    coot::residues_near_residue(r, atom_sel.mol, transformation_average_radius);
	 std::pair<CResidue *, std::vector<CResidue *> > p(r, env_residues);
	 moving_residues.push_back(p);
      }
   }
   return morph_fit_residues(moving_residues, xmap_in, transformation_average_radius);
} 


int
molecule_class_info_t::morph_fit_residues(std::vector<std::pair<CResidue *, std::vector<CResidue *> > > moving_residues,
					  const clipper::Xmap<float> &xmap_in, float transformation_average_radius) {

   int success = 0;

   // construct minimol fragments 

   // store the local origin too.
   std::map<CResidue *, morph_rtop_triple> rtop_map;

   for (unsigned int ires=0; ires<moving_residues.size(); ires++) {
      
      std::vector<CResidue *> fragment_residues;
      CResidue *residue_p = moving_residues[ires].first;
      std::pair<bool, clipper::Coord_orth> local_centre = residue_centre(residue_p);
      if (local_centre.first) { 
	 std::cout << "\rINFO:: Getting RTops for " << coot::residue_spec_t(residue_p);
	 std::cout.flush();

	 fragment_residues.push_back(moving_residues[ires].first);
	 for (unsigned int ires_l=0; ires_l<moving_residues[ires].second.size(); ires_l++) {
	    CResidue *r = moving_residues[ires].second[ires_l];
	    fragment_residues.push_back(r);
	 }
	 coot::minimol::fragment f;
	 for (unsigned int ifr=0; ifr<fragment_residues.size(); ifr++) {
	    f.addresidue(coot::minimol::residue(fragment_residues[ifr]), false);
	 } 
	 coot::minimol::molecule m(f);
	 
	 coot::minimol::molecule m_copy = m; // for debugging
	 
	 // returns the local rtop (relative to local centre) to move m into map.
	 std::pair<bool, clipper::RTop_orth> rtop = coot::get_rigid_body_fit_rtop(&m, local_centre.second, xmap_in);
	 if (rtop.first) { 
	    morph_rtop_triple rt(local_centre.second, rtop);
	    rtop_map[residue_p] = rt;
	    
	    // debugging block
	    //
	    if (0) { 
	       coot::rigid_body_fit(&m_copy, xmap_in);
	       std::string file_name = "morph-" + coot::util::int_to_string(ires);
	       file_name += ".pdb";
	       m_copy.write_file(file_name, 10);
	       std::cout << "    rtop for " << residue_p->GetSeqNum() << " " << residue_p->GetResName()
			 << " local centre " << local_centre.second.format() << " is " << std::endl;
	       std::cout << rt.rtop.format() << std::endl;
	    }
	 }
      }
      std::cout << std::endl; // for \r RTop specs
   }

   std::map<CResidue *, morph_rtop_triple>::const_iterator it;
   // std::cout << "rtop_map.size(): " << rtop_map.size() << std::endl;
   if (rtop_map.size()) {
      success = 1;
      make_backup();

      std::map<CResidue *, morph_rtop_triple> simple_shifts;
      std::map<CResidue *, morph_rtop_triple> smooth_shifts;
      
      for (it=rtop_map.begin(); it!=rtop_map.end(); it++) {
	 CResidue *this_residue = it->first;
	 if (it->second.valid) {

	    // Morphing step is super-fast
	    // std::cout << "\rINFO:: Morphing " << coot::residue_spec_t(this_residue);
	    // std::cout.flush();
	    
	    std::vector<std::pair<clipper::RTop_orth, float> > rtops;
	    // std::cout << "this residue:\n" << it->second.second.format() << std::endl;
	    rtops.push_back(std::pair<clipper::RTop_orth,float>(it->second.rtop, 1));
 	    std::vector<CResidue *> neighb_residues =
 	       coot::residues_near_residue(this_residue, atom_sel.mol, transformation_average_radius);

  	    for (unsigned int i_n_res=0; i_n_res<neighb_residues.size(); i_n_res++) { 
	       std::map<CResidue *, morph_rtop_triple>::const_iterator it_for_neighb =
		  rtop_map.find(neighb_residues[i_n_res]);
	       if (it_for_neighb != rtop_map.end()) { 
		  if (it_for_neighb->second.valid) {
		     float weight = 0.1;
		     float d_r = distance_between_residues(this_residue, neighb_residues[i_n_res]);
		     if (d_r > 0) {
			weight = 3.8/d_r;
			if (weight > 1.0)
			   weight = 1.0; // weight of central residue, shouldn't be more than that.
			// std::cout << "distance " << d_r << " weight " << weight << std::endl;
		     } 
		     std::pair<clipper::RTop_orth, float> p(it_for_neighb->second.rtop, weight);
		     rtops.push_back(p);
		     if (0)
			std::cout << "adding rtop for "
				  << coot::residue_spec_t(neighb_residues[i_n_res]) << "\n"
				  << it_for_neighb->second.rtop.format() << std::endl;
		  }
	       }
	    }

	    // pre-local shifts and quaternion-based rtop averaging
	    // morph_residue_atoms_by_average_rtops(this_residue, rtops);

	    coot::util::quaternion q(0,0,0,0);
	    // clipper::RTop_orth rtop = q.centroid_rtop(rtops);
	    bool robust_filter = true;
	    clipper::RTop_orth rtop = q.centroid_rtop(rtops, robust_filter);

	    translate_by_internal(-it->second.co, it->first);
	    transform_by_internal(rtop,           it->first);
	    translate_by_internal(it->second.co,  it->first);

	    // debugging: save just to view them
	    simple_shifts[this_residue] = it->second; 
	    smooth_shifts[this_residue] = morph_rtop_triple(it->second.co,
							    std::pair<bool, clipper::RTop_orth>(true, rtop));
	    
	 } else {
	    std::cout << "no RTop for " << coot::residue_spec_t(it->first) << std::endl;
	 } 
      }
      std::cout << std::endl;
      atom_sel.mol->PDBCleanup(PDBCLEAN_SERIAL|PDBCLEAN_INDEX);
      atom_sel.mol->FinishStructEdit();
      have_unsaved_changes_flag = 1;
      make_bonds_type_checked();

      // morph_show_shifts(simple_shifts, smooth_shifts);
      
   }
   return success;
}

// I fail to make a function that does a good "average" of RTops,
// so do it long-hand by generating sets of coordinates by applying
// each rtop to each atom - weights are transfered in the second part of the pair.
//
// This doesn't do backups or unsaved changes marking of course.
void
molecule_class_info_t::morph_residue_atoms_by_average_rtops(CResidue *residue_p,
							    const std::vector<std::pair<clipper::RTop_orth, float> > &rtops) {

   PPCAtom residue_atoms = 0;
   int n_residue_atoms;
   if (rtops.size()) { 
      residue_p->GetAtomTable(residue_atoms, n_residue_atoms);
      for (unsigned int iat=0; iat<n_residue_atoms; iat++) {
	 CAtom *at = residue_atoms[iat];
	 clipper::Coord_orth pt(at->x, at->y, at->z);
	 clipper::Coord_orth sum_transformed_pts(0,0,0);
	 double sum_weights = 0.0;
	 for (unsigned int i_rtop=0; i_rtop<rtops.size(); i_rtop++) { 
	    clipper::Coord_orth t_pt = pt.transform(rtops[i_rtop].first);
	    double weight = rtops[i_rtop].second;
	    sum_weights += weight;
	    sum_transformed_pts += t_pt*weight;
	 }
	 if (sum_weights > 0.0) {
	    double inv_weight = 1.0/sum_weights;
	    clipper::Coord_orth new_pt(sum_transformed_pts.x() * inv_weight,
				       sum_transformed_pts.y() * inv_weight,
				       sum_transformed_pts.z() * inv_weight);
	    at->x = new_pt.x();
	    at->y = new_pt.y();
	    at->z = new_pt.z();
	 }
      }
   }
}



void
molecule_class_info_t::morph_show_shifts(const std::map<CResidue *, morph_rtop_triple> &simple_shifts,
					 const std::map<CResidue *, morph_rtop_triple> &smooth_shifts) const {

   // write a file
   
   std::map<CResidue *, morph_rtop_triple>::const_iterator it;
   std::ofstream f("morph-shifts.scm");

   std::string ss;
   ss = "(define simple-shifts (new-generic-object-number \"simple-shifts\"))";
   f << ss << "\n";
   ss = "(define smooth-shifts (new-generic-object-number \"smooth-shifts\"))";
   f << ss << "\n";
   ss = "(set-display-generic-object simple-shifts 1)";
   f << ss << "\n";
   ss = "(set-display-generic-object smooth-shifts 1)";
   f << ss << "\n";

   for (it=simple_shifts.begin(); it!=simple_shifts.end(); it++) {
      CResidue *r = it->first;
      std::pair<bool, clipper::Coord_orth> rc = residue_centre(r);
      CAtom *C_alpha = r->GetAtom(" CA ");
      if (! C_alpha)
	 C_alpha = r->GetAtom(" P  ");
      if (C_alpha) {
	 clipper::Coord_orth ca_pos(C_alpha->x, C_alpha->y, C_alpha->z);
	 if (rc.first) {
	    std::string s;
	    std::string line_colour = "yellow";
	    std::string ball_colour = "yellow";

	    clipper::RTop_orth rtop_for_centre(clipper::Mat33<double>(1,0,0,0,1,0,0,0,1), it->second.co);
	    clipper::RTop_orth rtop_for_centre_i(clipper::Mat33<double>(1,0,0,0,1,0,0,0,1), -it->second.co);
	    clipper::Coord_orth tp_1   = ca_pos.transform(rtop_for_centre_i);
	    clipper::Coord_orth tp_2   =   tp_1.transform(it->second.rtop);
	    clipper::Coord_orth to_pos =   tp_2.transform(rtop_for_centre);
	 
	    s += "(to-generic-object-add-line  simple-shifts ";
	    s += "\"";
	    s += line_colour;
	    s += "\"";
	    s += "  2 ";
	    s += coot::util::float_to_string(ca_pos.x());
	    s += " ";
	    s += coot::util::float_to_string(ca_pos.y());
	    s += " ";
	    s += coot::util::float_to_string(ca_pos.z());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.x());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.y());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.z());
	    s += " ";
	    s += ")";
	    f << s << "\n";
	    s = "";
	    s += "(to-generic-object-add-point simple-shifts ";
	    s += "\"";
	    s += ball_colour;
	    s += "\"";
	    s += " 12                   ";
	    s += coot::util::float_to_string(to_pos.x());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.y());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.z());
	    s += " ";
	    s += ")";
	    f << s << "\n";
	 }
      }
   }
   for (it=smooth_shifts.begin(); it!=smooth_shifts.end(); it++) {
      CResidue *r = it->first;
      std::pair<bool, clipper::Coord_orth> rc = residue_centre(r);
      CAtom *C_alpha = r->GetAtom(" CA ");
      if (! C_alpha)
	 C_alpha = r->GetAtom(" P  ");
      if (C_alpha) {
	 clipper::Coord_orth ca_pos(C_alpha->x, C_alpha->y, C_alpha->z);
	 if (rc.first) {
	    std::string s;
	    std::string line_colour = "red";
	    std::string ball_colour = "red";

	    clipper::RTop_orth rtop_for_centre(clipper::Mat33<double>(1,0,0,0,1,0,0,0,1), it->second.co);
	    clipper::RTop_orth rtop_for_centre_i(clipper::Mat33<double>(1,0,0,0,1,0,0,0,1), -it->second.co);
	    clipper::Coord_orth tp_1   = ca_pos.transform(rtop_for_centre_i);
	    clipper::Coord_orth tp_2   =   tp_1.transform(it->second.rtop);
	    clipper::Coord_orth to_pos =   tp_2.transform(rtop_for_centre);
	 
	    s += "(to-generic-object-add-line  smooth-shifts ";
	    s += "\"";
	    s += line_colour;
	    s += "\"";
	    s += " 2 ";
	    s += coot::util::float_to_string(ca_pos.x());
	    s += " ";
	    s += coot::util::float_to_string(ca_pos.y());
	    s += " ";
	    s += coot::util::float_to_string(ca_pos.z());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.x());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.y());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.z());
	    s += " ";
	    s += ")";
	    f << s << "\n";
	    s = "";
	    s += "(to-generic-object-add-point smooth-shifts ";
	    s += "\"";
	    s += ball_colour;
	    s += "\"";
	    s += " 14                 ";
	    s += coot::util::float_to_string(to_pos.x());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.y());
	    s += " ";
	    s += coot::util::float_to_string(to_pos.z());
	    s += " ";
	    s += ")";
	    f << s << "\n";
	 }
      }
   }
   f.close();
}

int
molecule_class_info_t::fit_by_secondary_structure_elements(const std::string &chain_id,
							   const clipper::Xmap<float> &xmap_in) {

   int status = 0;
   float local_radius = 16;

   int imodel = 1;
   bool simple_move = false;
   
   if (atom_sel.mol) {
      CModel *model_p = atom_sel.mol->GetModel(imodel);
      if (model_p) {

	 make_backup();
	 bool model_changed = true;

	 int nhelix = model_p->GetNumberOfHelices();
	 int nsheet = model_p->GetNumberOfSheets();
	 PCHelix helix_p;
	 PCSheet sheet_p;
	 PCStrand strand_p;

	 CChain *chain_p = model_p->GetChain(chain_id.c_str());
	 if (chain_p) { 

	    // store the RTops for some residues (we also need the
	    // local around which the rtop_orth is performed)
	    // 
	    std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> > rtop_map; 

	    std::cout << "INFO:: " << nhelix << " helices" << std::endl;
	    std::cout << "INFO:: " << nsheet << " sheets"  << std::endl;

	    for (int ih=1; ih<=nhelix; ih++) {
	       helix_p = model_p->GetHelix(ih);
	       if (helix_p) {

		  std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> > rtops_fragment = 
		     fit_by_secondary_structure_fragment(chain_p, chain_id, helix_p->initSeqNum, helix_p->endSeqNum,
							 xmap_in, simple_move);
		  // add rtops_fragment bits to overall rtops_map;
		  std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> >::const_iterator it;
		  for (it=rtops_fragment.begin(); it!=rtops_fragment.end(); it++)
		     rtop_map[it->first] = it->second;

	       } else {
		  std::cout << "ERROR: no helix!?" << std::endl;
	       }
	    }

	    for (int is=1; is<=nsheet; is++) {
	       sheet_p = model_p->GetSheet(is);

	       int nstrand = sheet_p->nStrands;
	       for (int istrand=0; istrand<nstrand; istrand++) {
		  strand_p = sheet_p->Strand[istrand];
		  if (strand_p) { 
		     std::cout << "---- handle strand ------ id: " << strand_p->sheetID << " # "
			       << strand_p->strandNo << " " << strand_p->initChainID << " "
			       << strand_p->initSeqNum << " "
			       << strand_p->endChainID << " "
			       << strand_p->endSeqNum
			       << std::endl;
		     
		     if (std::string(strand_p->initChainID) == chain_id) {
			if (std::string(strand_p->endChainID) == chain_id) {
			   std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> > rtops_fragment = 
			      fit_by_secondary_structure_fragment(chain_p, chain_id,
								  strand_p->initSeqNum,
								  strand_p->endSeqNum,
								  xmap_in, simple_move);

			   // add rtops_fragment bits to overall rtops_map;
			   std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> >::const_iterator it;
			   for (it=rtops_fragment.begin(); it!=rtops_fragment.end(); it++)
			      rtop_map[it->first] = it->second;
			}
		     }
		  }
	       }
	    }

	    // OK, so now some residues (those in SSE) have rtops
	    //
	    // Now run over the residues and atoms of the chain and apply
	    // the local weighted average of the RTops to the coordinates
	    //
	    // The residues in a given SSE all have the same RTop_orth.
	    //
	    int nres = chain_p->GetNumberOfResidues();
	    CResidue *residue_p;
	    CAtom *at;
	    std::map<CResidue *, clipper::Coord_orth> residue_centres;
	    for (int ires=0; ires<nres; ires++) {

	       residue_p = chain_p->GetResidue(ires);
	       std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> >::const_iterator it_ss =
		  rtop_map.find(residue_p);
	       if (it_ss != rtop_map.end()) {
		  
		  // OK this was a residue in a SSE.  We know how to move these atoms (i.e. use their own
		  // RTop, not morphing). This block is what Israel Sanchez-Fernandez wanted.
		  // 
		  PPCAtom residue_atoms = 0;
		  int n_residue_atoms;
		  residue_p->GetAtomTable(residue_atoms, n_residue_atoms);
		  clipper::Coord_orth centre = it_ss->second.first;
		  for (unsigned int iat=0; iat<n_residue_atoms; iat++) {
		     CAtom *at = residue_atoms[iat];
		     clipper::Coord_orth pt_1 = coot::co(at);
		     clipper::Coord_orth pt_2 = pt_1 - centre;
		     clipper::Coord_orth pt_3 = pt_2.transform(it_ss->second.second);
		     clipper::Coord_orth pt_4 = pt_3 + centre;
		     coot::update_position(at, pt_4);
		  }
		  
	       } else {

		  // ------------------------------------------------------------------
		  //    morph:  move the atoms in these residues based on the RTops of
		  //            the residues in their environments.
		  // ------------------------------------------------------------------

		  int n_atoms = residue_p->GetNumberOfAtoms();

		  // get the centre of this residue (residue_p) from
		  // the centre cache (if not in the cache, add it to
		  // the cache).
		  clipper::Coord_orth this_residue_centre(0,0,0);
		  if (residue_centres.find(residue_p) == residue_centres.end()) {
		     std::pair<bool, clipper::Coord_orth> pp = coot::util::get_residue_centre(residue_p);
		     if (pp.first) {
			this_residue_centre = pp.second;
			residue_centres[residue_p] = pp.second;
		     }
		  } else {
		     this_residue_centre = residue_centres[residue_p];
		  } 

		  // get the centre of the environment residues from
		  // the cache (if not in the cache, add them to the
		  // cache).
		  std::vector<CResidue *> env_residues =
		     coot::residues_near_residue(residue_p, atom_sel.mol, local_radius);
		  for (unsigned int ires=0; ires<env_residues.size(); ires++) { 
		     if (residue_centres.find(env_residues[ires]) == residue_centres.end()) {
			std::pair<bool, clipper::Coord_orth> pp =
			   coot::util::get_residue_centre(env_residues[ires]);
			if (pp.first)
			   residue_centres[env_residues[ires]] = pp.second;
		     }
		  }

		  for (int iat=0; iat<n_atoms; iat++) {
		     at = residue_p->GetAtom(iat);
		     clipper::Coord_orth pt_atom = coot::co(at);
		     // std::tuple<clipper::RTop_orth, clipper::Coord_orth, float> 
		     std::vector<atom_morph_info_t> rtops_for_atom;
		     for (unsigned int ier=0; ier<env_residues.size(); ier++) { 
			std::map<CResidue *, clipper::Coord_orth>::const_iterator it;
			it = residue_centres.find(env_residues[ier]);
			if (it != residue_centres.end()) {
			   const clipper::Coord_orth &pt_e_r = it->second;
			   double d_sqrd = (pt_e_r - pt_atom).lengthsq();
			   if (d_sqrd < 1.0) d_sqrd = 1.0;
			   double d = sqrt(d_sqrd);
			   double w = 1.0/d;
			   std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> >::const_iterator it_rtop =
			      rtop_map.find(env_residues[ier]);
			   if (it_rtop != rtop_map.end()) { 
			      atom_morph_info_t t(it_rtop->second.second, pt_e_r, w);
			      rtops_for_atom.push_back(t);
			   } 
			}
		     }

		     if (rtops_for_atom.size()) {

			std::vector<std::pair<clipper::RTop_orth,float> > rtop_pairs_for_atom(rtops_for_atom.size());
			for (unsigned int i=0; i<rtops_for_atom.size(); i++)
			   rtop_pairs_for_atom[i] = rtops_for_atom[i].rtop_and_weight();
			
			coot::util::quaternion q(0,0,0,0);
			bool robust_filter = true;
			clipper::RTop_orth rtop_for_atom = q.centroid_rtop(rtop_pairs_for_atom, robust_filter);
			clipper::Coord_orth p_1 = coot::co(at);
			clipper::Coord_orth p_2 = p_1 - this_residue_centre;
			clipper::Coord_orth p_3 = p_2.transform(rtop_for_atom);
			clipper::Coord_orth p_4 = p_3 + this_residue_centre;
			coot::update_position(at, p_4);
		     }
		  }
	       }
	    }

	 
	    if (model_changed) {
	       atom_sel.mol->FinishStructEdit();
	       have_unsaved_changes_flag = 1;
	       make_bonds_type_checked();
	    }
	 }
      }
   }
   return status;
}

std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> >
molecule_class_info_t::fit_by_secondary_structure_fragment(CChain *chain_p,
							   const std::string &chain_id,
							   int initSeqNum,
							   int endSeqNum,
							   const clipper::Xmap<float> &xmap_in,
							   bool simple_move) {

   
   std::map<CResidue *, std::pair<clipper::Coord_orth, clipper::RTop_orth> > rtop_map;
   
   coot::minimol::fragment f(chain_id);
   std::vector<CResidue *> added_residues;
   for (int res_no=initSeqNum; res_no<=endSeqNum; res_no++) {
      CResidue *residue_p = chain_p->GetResidue(res_no, "");
      if (residue_p) { 
	 f.addresidue(coot::minimol::residue(residue_p), false);
	 added_residues.push_back(residue_p);
      } else {
	 std::cout << "Null residue for " << chain_id << " " << res_no << std::endl;
      } 
   }

   if (!added_residues.size()) {
      std::cout << "no added residues for helix " << chain_id << " " 
		<< initSeqNum << " " << endSeqNum << std::endl;
   } else {
      coot::minimol::molecule m(f);
      std::pair<bool, clipper::Coord_orth> sse_centre = coot::centre_of_residues(added_residues);
      if (sse_centre.first) { 
		     
	 // returns the local rtop (relative to local centre) to move m into map.
	 //
	 std::pair<bool, clipper::RTop_orth> rtop =
	    coot::get_rigid_body_fit_rtop(&m, sse_centre.second, xmap_in);
	 
	 if (rtop.first) {
	    if (0)
	       std::cout << "Got and RTop for SSE " << chain_id << " "
			 << initSeqNum << " -- " << endSeqNum
			 << std::endl;
	    for (unsigned int ires=0; ires<added_residues.size(); ires++) {
	       std::pair<clipper::Coord_orth, clipper::RTop_orth> p(sse_centre.second, rtop.second);
	       rtop_map[added_residues[ires]] = p;
	    }

	    if (simple_move) { 
	       // simple move the coordinates
	       m.transform(rtop.second);
	       clipper::Mat33<double> mat(1,0,0,0,1,0,0,0,1);
	       clipper::RTop_orth rtop_synth(mat, sse_centre.second);
	       m.transform(rtop_synth);
	       atom_selection_container_t asc = make_asc(m.pcmmdbmanager());
	       replace_fragment(asc);
	    }
	 }
      }
   }
   return rtop_map;
} 




// maybe extra parameters are needed here (e.g. for colouring later, perhaps).
void
coot::extra_restraints_representation_t::add_parallel_plane(const lsq_plane_info_t &pi_1,
							    const lsq_plane_info_t &pi_2) {

      double offset_d = 0.4;
      clipper::Coord_orth n_1 = pi_1.normal();
      clipper::Coord_orth n_2 = pi_2.normal();
      clipper::Coord_orth offset_pt_1_a = pi_1.centre() + offset_d * n_1;
      clipper::Coord_orth offset_pt_1_b = pi_1.centre() - offset_d * n_1;
      clipper::Coord_orth offset_pt_2_a = pi_2.centre() + offset_d * n_2;
      clipper::Coord_orth offset_pt_2_b = pi_2.centre() - offset_d * n_2;

      clipper::Coord_orth op_1 = offset_pt_1_a;
      if (clipper::Coord_orth(offset_pt_1_b - pi_2.centre()).lengthsq() <
	  clipper::Coord_orth(offset_pt_1_a - pi_2.centre()).lengthsq())
	 op_1 = offset_pt_1_b;
      clipper::Coord_orth op_2 = offset_pt_2_a;
      if (clipper::Coord_orth(offset_pt_2_b - pi_1.centre()).lengthsq() <
	  clipper::Coord_orth(offset_pt_2_a - pi_1.centre()).lengthsq())
	 op_2 = offset_pt_2_b;
	 
      double f_top_1 = -(pi_2.a()*op_1.x() + pi_2.b()*op_1.y() + pi_2.c()*op_1.z() - pi_2.d());
      double f_top_2 = -(pi_1.a()*op_2.x() + pi_1.b()*op_2.y() + pi_1.c()*op_2.z() - pi_1.d());
      double f_bot   = pi_2.a() * pi_1.a() + pi_2.b() * pi_1.b() + pi_2.c() * pi_1.c();
      double s_1 = f_top_1/f_bot;
      double s_2 = f_top_2/f_bot;

      // projected plane points
      clipper::Coord_orth ppp_1 = op_1 + s_1 * pi_1.normal();
      clipper::Coord_orth ppp_2 = op_2 + s_2 * pi_2.normal();

      extra_parallel_planes_restraints_representation_t eppr_1(op_1, ppp_1, pi_1.normal(), 1.3, 0.2);
      extra_parallel_planes_restraints_representation_t eppr_2(op_2, ppp_2, pi_2.normal(), 1.3, 0.2);
      parallel_planes.push_back(eppr_1);
      parallel_planes.push_back(eppr_2);

}
