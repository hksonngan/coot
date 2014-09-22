
#include <algorithm>
#include "coot-utils/coot-coord-extras.hh"
#include "coot-utils/coot-map-heavy.hh"
#include "simple-restraint.hh"

// this can throw an exception
// 
std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> >
coot::torsionable_bonds(mmdb::Manager *mol, mmdb::PPAtom atom_selection,
			int n_selected_atoms,
			coot::protein_geometry *geom_p) { 

   std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> > v;
   bool include_pyranose_ring_torsions_flag = false;

   std::vector<mmdb::Residue *> residues;
   std::map<mmdb::Residue *, std::vector<int> > atoms_in_residue;
   // fill residues and atoms_in_residue
   for (unsigned int i=0; i<n_selected_atoms; i++) {
      mmdb::Residue *r = atom_selection[i]->residue;
      if (std::find(residues.begin(), residues.end(), r) == residues.end())
	 residues.push_back(r);
      atoms_in_residue[r].push_back(i);
   }
   std::map<mmdb::Residue *, coot::dictionary_residue_restraints_t> res_restraints;
   for (unsigned int ires=0; ires<residues.size(); ires++) { 
      std::string rn = residues[ires]->GetResName();
      std::pair<bool, coot::dictionary_residue_restraints_t> rest =
	 geom_p->get_monomer_restraints(rn);
      if (! rest.first) {
	 std::string m = "Restraints not found for type ";
	 m += rn;
	 throw std::runtime_error(m);
      }
      res_restraints[residues[ires]] = rest.second;
   }

   for (unsigned int ires=0; ires<residues.size(); ires++) {
      // a coot-coord-extras function
      std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> > v_inner =
	 coot::torsionable_bonds_monomer_internal(residues[ires],
						  atom_selection,
						  n_selected_atoms,
						  include_pyranose_ring_torsions_flag,
						  geom_p);
      // std::cout << "found " << v_inner.size() << " monomer internal torsions for "
      // << residues[ires]->GetResName() << std::endl;
      for (unsigned int ip=0; ip<v_inner.size(); ip++)
	 v.push_back(v_inner[ip]);
   }

   std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> > v_link =    
      coot::torsionable_link_bonds(residues, mol, geom_p);
   for (unsigned int il=0; il<v_link.size(); il++)
      v.push_back(v_link[il]);

   if (0) // debug
      for (unsigned int ipair=0; ipair<v.size(); ipair++) { 
	 std::cout << "   torsionable bond: "
		   << coot::atom_spec_t(v[ipair].first) << "  "
		   << coot::atom_spec_t(v[ipair].second)
		   << std::endl;
      }
   
   return v;
}

std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> >
coot::torsionable_link_bonds(std::vector<mmdb::Residue *> residues_in,
			     mmdb::Manager *mol, coot::protein_geometry *geom_p) {

   std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> > v;

   if (! mol)
      return v;
   
   std::vector<std::pair<bool, mmdb::Residue *> > residues(residues_in.size());
   for (unsigned int i=0; i<residues_in.size(); i++)
      residues[i] = std::pair<bool, mmdb::Residue *> (0, residues_in[i]);

   std::vector<coot::atom_spec_t> dummy_fixed_atom_specs;
   coot::restraints_container_t restraints(residues, *geom_p, mol, dummy_fixed_atom_specs);
   coot::bonded_pair_container_t bpc = restraints.bonded_residues_from_res_vec(*geom_p);

   // add in the torsion: CB-CG-ND2-C1 (Psi-N)
   // add in the torsion: CG-ND2-C1-O5 (Phi-N)
   // t.geom.link_add_torsion("NAG-ASN", 1, 2, 2, 2, " C1 ", "ND2 ", " CG ", " CB ", 180, 40, 3, "Psi-N");
   // t.geom.link_add_torsion("NAG-ASN", 1, 1, 2, 2, " O5 ", " C1 ", "ND2 ", " CG ", 180, 40, 3, "Psi-N");
      
   // std::cout << "found LINKR linked pairs:\n   " <<  bpc;

   for (unsigned int i=0; i<bpc.bonded_residues.size(); i++) { 
      coot::dictionary_residue_link_restraints_t link = geom_p->link(bpc[i].link_type);
      if (link.link_id != "") {
	 if (0)
	    std::cout << "   dictionary link found " << link.link_id << " with "
		      << link.link_bond_restraint.size() << " bond restraints and "
		      << link.link_torsion_restraint.size() << " link torsions " << std::endl;
	 
	 for (unsigned int ib=0; ib<link.link_bond_restraint.size(); ib++) { 
	    // we need to get the atoms and add them to "pairs".

	    if (0) 
	       std::cout << "   "
			 << link.link_bond_restraint[ib].atom_id_1_4c() << " "
			 << link.link_bond_restraint[ib].atom_1_comp_id << " to "
			 << link.link_bond_restraint[ib].atom_id_2_4c() << " " 
			 << link.link_bond_restraint[ib].atom_2_comp_id << " "
			 << " of "
			 << bpc[i].res_1->GetResName() << " to "
			 << bpc[i].res_2->GetResName()
			 << std::endl;
	    
	    mmdb::Atom *link_atom_1 = bpc[i].res_1->GetAtom(link.link_bond_restraint[ib].atom_id_1_4c().c_str());
	    mmdb::Atom *link_atom_2 = bpc[i].res_2->GetAtom(link.link_bond_restraint[ib].atom_id_2_4c().c_str());
	    if (link_atom_1 && link_atom_2) { 
	       std::pair<mmdb::Atom *, mmdb::Atom *> pair(link_atom_1, link_atom_2);
	       v.push_back(pair);
	    }
	 }


	 // Add in link torsion atoms if they were not already added
	 // above (because they were link bond restraints)
	 // 
	 for (unsigned int it=0; it<link.link_torsion_restraint.size(); it++) {
	    mmdb::Residue *res_for_at_2 = bpc[i].res_1;
	    mmdb::Residue *res_for_at_3 = bpc[i].res_1;
	    if (link.link_torsion_restraint[it].atom_2_comp_id == 1) res_for_at_2 = bpc[i].res_1;
	    if (link.link_torsion_restraint[it].atom_2_comp_id == 2) res_for_at_2 = bpc[i].res_2;
	    if (link.link_torsion_restraint[it].atom_3_comp_id == 1) res_for_at_3 = bpc[i].res_1;
	    if (link.link_torsion_restraint[it].atom_3_comp_id == 2) res_for_at_3 = bpc[i].res_2;

	    if (res_for_at_2 && res_for_at_3) {
	       mmdb::Atom *link_atom_1 = res_for_at_2->GetAtom(link.link_torsion_restraint[it].atom_id_2_4c().c_str());
	       mmdb::Atom *link_atom_2 = res_for_at_3->GetAtom(link.link_torsion_restraint[it].atom_id_3_4c().c_str());

	       if (link_atom_1 && link_atom_2) {
		  std::pair<mmdb::Atom *, mmdb::Atom *> pair(link_atom_1, link_atom_2);
		  if (std::find(v.begin(), v.end(), pair) == v.end())
		     v.push_back(pair);
	       }
	    }
	 }
      }
   }


   if (0) {
      std::cout << "---------------- torsionable_link_bonds() returns: " << std::endl;
      for (unsigned int i=0; i<v.size(); i++) { 
	 std::cout << "    " << i << " " << coot::atom_spec_t(v[i].first) << " - "
		   << coot::atom_spec_t(v[i].second) << std::endl;
      }
   }
   
   return v;
}


// And the atom_quad version of the above 2 functions (for setting link torsions)
// 
// this can throw an exception
// 
std::vector<coot::torsion_atom_quad>
coot::torsionable_quads(mmdb::Manager *mol, mmdb::PPAtom atom_selection,
			int n_selected_atoms,
			coot::protein_geometry *geom_p) {

   bool pyranose_ring_torsion_flag = false; // no thanks
   std::vector<coot::torsion_atom_quad> quads;
   std::vector<mmdb::Residue *> residues;
   for (unsigned int i=0; i<n_selected_atoms; i++) { 
      mmdb::Residue *r = atom_selection[i]->residue;
      if (std::find(residues.begin(), residues.end(), r) == residues.end())
	 residues.push_back(r);
   }
   std::vector<coot::torsion_atom_quad> link_quads =
      coot::torsionable_link_quads(residues, mol, geom_p);
   for (unsigned int iquad=0; iquad<link_quads.size(); iquad++)
      quads.push_back(link_quads[iquad]);
   for (unsigned int ires=0; ires<residues.size(); ires++) {
      mmdb::PPAtom residue_atoms = 0;
      int n_residue_atoms;
      residues[ires]->GetAtomTable(residue_atoms, n_residue_atoms);
      std::vector<coot::torsion_atom_quad> monomer_quads =
	 coot::torsionable_bonds_monomer_internal_quads(residues[ires], residue_atoms,
							n_residue_atoms,
							pyranose_ring_torsion_flag, geom_p);
      for (unsigned int iquad=0; iquad<monomer_quads.size(); iquad++)
	 quads.push_back(monomer_quads[iquad]);
   }
   return quads;
}

// And the atom_quad version of that (for setting link torsions)
// 
std::vector<coot::torsion_atom_quad>
coot::torsionable_link_quads(std::vector<mmdb::Residue *> residues_in,
			     mmdb::Manager *mol, coot::protein_geometry *geom_p) {

   std::vector<coot::torsion_atom_quad> quads;
   std::vector<std::pair<bool, mmdb::Residue *> > residues(residues_in.size());
   for (unsigned int i=0; i<residues_in.size(); i++)
      residues[i] = std::pair<bool, mmdb::Residue *> (0, residues_in[i]);

   // We want a quick way of getting to the restaints of an atom's
   // residue (link_atom_1 and link_atom_2 below).
   // So here we set up res_restraints map indexed by a mmdb::Residue *.
   // 
   std::map<mmdb::Residue *, coot::dictionary_residue_restraints_t> res_restraints;
   for (unsigned int ires=0; ires<residues_in.size(); ires++) { 
      std::string rn = residues_in[ires]->GetResName();
      std::pair<bool, coot::dictionary_residue_restraints_t> rest =
	 geom_p->get_monomer_restraints(rn);
      if (! rest.first) {
	 std::string m = "Restraints not found for type ";
	 m += rn;
	 throw std::runtime_error(m);
      }
      res_restraints[residues_in[ires]] = rest.second;
   }

   std::vector<coot::atom_spec_t> dummy_fixed_atom_specs;
   coot::restraints_container_t restraints(residues, *geom_p, mol, dummy_fixed_atom_specs);
   coot::bonded_pair_container_t bpc = restraints.bonded_residues_from_res_vec(*geom_p);
   for (unsigned int i=0; i<bpc.bonded_residues.size(); i++) { 
      coot::dictionary_residue_link_restraints_t link = geom_p->link(bpc[i].link_type);
      // std::cout << "    " << i << " link_id: \"" << link.link_id << "\'" << std::endl;
      if (link.link_id != "") {


	 // Don't use bonds - use link torsions if you can.
	 // 
	 std::cout << "link " << link.link_id << " has " << link.link_torsion_restraint.size()
		   << " torsion restraints " << std::endl;

	 if (link.link_torsion_restraint.size() > 0) { 
	 
	    for (unsigned int il=0; il<link.link_torsion_restraint.size(); il++) {
	       // std::cout << "----------- link torsion restraint " << il << std::endl;
	       const coot::dict_link_torsion_restraint_t &rest = link.link_torsion_restraint[il];
	       if (rest.is_pyranose_ring_torsion()) {
		  // pass
	       } else { 
		  mmdb::Residue *r_1 = bpc[i].res_1;
		  mmdb::Residue *r_2 = bpc[i].res_1;
		  mmdb::Residue *r_3 = bpc[i].res_1;
		  mmdb::Residue *r_4 = bpc[i].res_1;
		  if (rest.atom_1_comp_id == 2)
		     r_1 = bpc[i].res_2;
		  if (rest.atom_2_comp_id == 2)
		     r_2 = bpc[i].res_2;
		  if (rest.atom_3_comp_id == 2)
		     r_3 = bpc[i].res_2;
		  if (rest.atom_4_comp_id == 2)
		     r_4 = bpc[i].res_2;
		  mmdb::Atom *link_atom_1 = r_1->GetAtom(rest.atom_id_1_4c().c_str());
		  mmdb::Atom *link_atom_2 = r_2->GetAtom(rest.atom_id_2_4c().c_str());
		  mmdb::Atom *link_atom_3 = r_3->GetAtom(rest.atom_id_3_4c().c_str());
		  mmdb::Atom *link_atom_4 = r_4->GetAtom(rest.atom_id_4_4c().c_str());
	    
		  std::cout << "   link residues "
			    << coot::residue_spec_t(r_1) << " " << coot::residue_spec_t(r_2) << " "
			    << coot::residue_spec_t(r_3) << " " << coot::residue_spec_t(r_4)
			    << std::endl;
		  std::cout << "   link_atoms: "
			    << coot::atom_spec_t(link_atom_1) << " " << coot::atom_spec_t(link_atom_2) << " "
			    << coot::atom_spec_t(link_atom_3) << " " << coot::atom_spec_t(link_atom_4) << " "
			    << std::endl;
		  if (link_atom_1 && link_atom_2 && link_atom_3 && link_atom_4) {
		     coot::torsion_atom_quad q(link_atom_1, link_atom_2, link_atom_3, link_atom_4,
					       rest.angle(),
					       rest.angle_esd(),
					       rest.period());
		     q.name = rest.id();
		     quads.push_back(q);
		  }
	       }
	    }

	 } else {

	    std::cout << "torsion made from bond restraint"  << std::endl;

	    // bleugh... OK, no torsion restraints.
	    // So use a bond restaint to make one torsion (around the link bond).
	    // 
	    for (unsigned int ib=0; ib<link.link_bond_restraint.size(); ib++) { 
	       mmdb::Atom *link_atom_1 = bpc[i].res_1->GetAtom(link.link_bond_restraint[ib].atom_id_1_4c().c_str());
	       mmdb::Atom *link_atom_2 = bpc[i].res_2->GetAtom(link.link_bond_restraint[ib].atom_id_2_4c().c_str());
	       if (link_atom_1 && link_atom_2) {
		  // What are the neightbours of link_atom_1 (and link_atom_2)?
		  // Try to find a non-hydrogen atom to which it is bonded.
		  bool H_flag = false;
		  std::string atom_name_1 = link_atom_1->name;
		  std::string atom_name_2 = link_atom_2->name;
		  std::vector<std::string> n1;
		  std::vector<std::string> n2;
		  n1 = res_restraints[link_atom_1->residue].neighbours(atom_name_1, H_flag);
		  n2 = res_restraints[link_atom_2->residue].neighbours(atom_name_2, H_flag);
		  if (n1.size() && n2.size()) {
		     std::string neigbhour_1_name = n1[0];
		     std::string neigbhour_2_name = n2[0];
		     mmdb::Atom *n_at_1 = bpc[i].res_1->GetAtom(neigbhour_1_name.c_str());
		     mmdb::Atom *n_at_2 = bpc[i].res_2->GetAtom(neigbhour_2_name.c_str());
		     if (n_at_1 && n_at_2) {
 			coot::torsion_atom_quad q(n_at_1, link_atom_1, link_atom_2, n_at_2,
						  180.0, 40, 3); // synthetic values
			q.name = "Bond-derived synthetic torsion";
			quads.push_back(q);
		     } 
		  } 
	       } else {
		  std::cout << "WARNING:: oops missing link atoms " << std::endl;
		  if (! link_atom_1)
		     std::cout << "   " << link.link_bond_restraint[ib].atom_id_1_4c().c_str() << std::endl;
		  if (! link_atom_2)
		     std::cout << "   " << link.link_bond_restraint[ib].atom_id_2_4c().c_str() << std::endl;
	       } 
	    }
	 } 
      }
   }
   return quads;
}


// this can throw an exception.
void
coot::multi_residue_torsion_fit_map(mmdb::Manager *mol,
				    const clipper::Xmap<float> &xmap,
				    int n_trials,
				    coot::protein_geometry *geom_p) {


   std::vector<std::pair<std::string, int> > atom_numbers = coot::util::atomic_number_atom_list();
   
   try { 
      mmdb::PPAtom atom_selection = 0;
      int n_selected_atoms;
      int selhnd = mol->NewSelection(); // d
      mol->SelectAtoms(selhnd, 0, "*",
		       mmdb::ANY_RES, "*",
		       mmdb::ANY_RES, "*",
		       "*", "*", "*", "*"); 
      mol->GetSelIndex(selhnd, atom_selection, n_selected_atoms);
      std::vector<std::pair<mmdb::Atom *, float> > atoms(n_selected_atoms); // for density fitting
      for (unsigned int iat=0; iat<n_selected_atoms; iat++) {
	 int atomic_number = coot::util::atomic_number(atom_selection[iat]->element, atom_numbers);
	 float z = atomic_number;
	 if (atomic_number == -1)
	    z = 6.0f;
	 atoms[iat] = std::pair<mmdb::Atom *, float> (atom_selection[iat], z);
      } 

      if (n_selected_atoms > 0) { 
	 std::vector<coot::torsion_atom_quad> quads = 
	    coot::torsionable_quads(mol, atom_selection, n_selected_atoms, geom_p);

	 if (0) 
	    for (unsigned int iquad=0; iquad<quads.size(); iquad++)
	       std::cout << "   " << iquad << " "
			 << coot::atom_spec_t(quads[iquad].atom_1) << " " 
			 << coot::atom_spec_t(quads[iquad].atom_2) << " " 
			 << coot::atom_spec_t(quads[iquad].atom_3) << " " 
			 << coot::atom_spec_t(quads[iquad].atom_4) << " \""
			 << quads[iquad].name << "\" torsion: "
			 << quads[iquad].torsion() 
			 << std::endl;


	 coot::contact_info contacts(mol, selhnd, quads, geom_p);
	 std::vector<std::vector<int> > contact_indices =
	    contacts.get_contact_indices_with_reverse_contacts();
	 coot::atom_tree_t tree(contact_indices, 0, mol, selhnd);

	 bool reverse_flag = 1;
	 double pre_score = coot::util::z_weighted_density_score_new(atoms, xmap);

	 double best_score = pre_score;
	 int n_quads = quads.size();
	 std::vector<double> best_quads(n_quads, -1);
	 std::vector<atom_tree_t::tree_dihedral_quad_info_t> best_tree_dihedral_quads;
	 coot::map_index_t fixed_index(0);

	 // save the current
	 for (unsigned int iquad=0; iquad<n_quads; iquad++)
	    best_quads[iquad] = quads[iquad].torsion();
      
	 for (unsigned int itrial=0; itrial<n_trials; itrial++) {

	    if (0)
	       std::cout << "Round " << itrial << " of " << n_trials << " for " << n_quads << " quads "
			 << std::endl;
	    
	    std::vector<atom_tree_t::tree_dihedral_quad_info_t> torsion_quads;
	    for (unsigned int iquad=0; iquad<n_quads; iquad++) {
	       // quads[iquad] is passed for debugging
               double rand_angle = get_rand_angle(best_quads[iquad], quads[iquad], itrial, n_trials);
	       atom_tree_t::tree_dihedral_quad_info_t tor(quads[iquad], rand_angle, fixed_index);
	       torsion_quads.push_back(tor);
	    }
	    tree.set_dihedral_multi(torsion_quads);
	    double this_score = coot::util::z_weighted_density_score_new(atoms, xmap);

	    // debugging of scores
	    if (0) { 
	       std::cout << "this_score: " << this_score;
	       for (unsigned int iquad=0; iquad<quads.size(); iquad++)
		  std::cout << "   " << quads[iquad].torsion();
	       std::cout << std::endl;
	    }
	    
	    if (this_score > best_score) {
	       // save best torsion angles
	       best_score = this_score;
	       for (unsigned int iquad=0; iquad<n_quads; iquad++)
		  best_quads[iquad] = quads[iquad].torsion();
	       best_tree_dihedral_quads = torsion_quads;
	    }

	    // std::string file_name = "trial-" + util::int_to_string(itrial) + ".pdb";
	    // mol->WritePDBASCII(file_name.c_str());
	 }
	 tree.set_dihedral_multi(best_tree_dihedral_quads);
      
	 mol->DeleteSelection(selhnd);
      }
   }
   catch (const std::runtime_error &rte) {
      std::cout << "WARNING:: " << rte.what() << std::endl;
   }
} 


double 
coot::get_rand_angle(double current_angle, 
                     const coot::torsion_atom_quad &quad, 
                     int itrial, int n_trials) { 

   double r = current_angle;
   double minus_one_to_one = -1 + 2 * double(coot::util::random())/double(RAND_MAX);
   double trial_factor = double(itrial)/double(n_trials);
   // double angle_scale_factor = 0.2 + 0.8*(1-trial_factor);
   double angle_scale_factor = 0.2 + 0.8 - trial_factor;
   
   r += 30 * minus_one_to_one * angle_scale_factor;

   // allow gauche+/gauche-/trans
   double rn = float(coot::util::random())/float(RAND_MAX);
   if (rn < (0.3 - 0.25*trial_factor)) {
      double rn_2 = float(coot::util::random())/float(RAND_MAX);
      double step = floor(6 * rn_2) * 60.0;
      // std::cout << "      step " << step << std::endl;
      r += step;
   } 

   if (0)
      std::cout << "   varying " << quad.name << " was " << current_angle << " now "
		<< r << " delta: " << r - current_angle << std::endl;
   
   return r; 
} 
