
// Created: 20170721-PE
// 
// There should be more functions here.

#ifndef IDEAL_TORSION_BONDS_HH
#define IDEAL_TORSION_BONDS_HH

namespace coot {

   // this can throw an exception
   std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> >
   torsionable_bonds(mmdb::Manager *mol, mmdb::PPAtom atom_selection, int n_selected_atoms,
		     protein_geometry *geom);
   // not sure this needs to public
   std::vector<std::pair<mmdb::Atom *, mmdb::Atom *> >
   torsionable_link_bonds(std::vector<mmdb::Residue *> residues_in,
			  mmdb::Manager *mol,
			  protein_geometry *geom);

   // And the atom_quad version of that (for setting link torsions)
   // 
   std::vector<torsion_atom_quad>
   torsionable_quads(mmdb::Manager *mol, mmdb::PPAtom atom_selection, int n_selected_atoms,
		     protein_geometry *geom);
   std::vector<torsion_atom_quad>
   torsionable_link_quads(std::vector<mmdb::Residue *> residues_in,
			  mmdb::Manager *mol, protein_geometry *geom_p);
   
   // this can throw an std::runtime exception
   void multi_residue_torsion_fit_map(mmdb::Manager *mol,
				      const clipper::Xmap<float> &xmap,
				      int n_trials,
				      coot::protein_geometry *geom_p); 
   // which calls 
   double get_rand_angle(double current_angle, const torsion_atom_quad &quad, int itrial,
			 int n_trials,
			 bool allow_conformer_switch,
			 bool small_torsion_changes);
   
   // Does this model bang into itself?
   // Don't give atoms that are both in a quad a bang score
   // 
   double get_self_clash_score(mmdb::Manager *mol,
			       mmdb::PPAtom atom_selection,
			       int n_selected_atoms,
			       const std::vector<coot::torsion_atom_quad> &quads);
   bool both_in_a_torsion_p(mmdb::Atom *at_1,
			    mmdb::Atom *at_2,
			    const std::vector<coot::torsion_atom_quad> &quads);
} 

#endif // IDEAL_TORSION_BONDS_HH
