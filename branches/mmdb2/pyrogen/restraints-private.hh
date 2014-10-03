
#include <GraphMol/Substruct/SubstructMatch.h>

namespace coot {

   // private (no SWIG interface)
   // 
   // the engine for the above calls
   std::pair<mmdb::Manager *, mmdb::Residue *>
   regularize_inner(PyObject *rdkit_mol,
		    PyObject *restraints_py,
		    const std::string &res_name);

   std::pair<mmdb::Manager *, mmdb::Residue *>
   regularize_inner(RDKit::ROMol &mol,
		    PyObject *restraints_py,
		    const std::string &res_name);
   

   bool is_const_torsion(const RDKit::ROMol &mol,
			 const RDKit::Atom *at_2,
			 const RDKit::Atom *at_3);
   // also private (no interface)
   // 
   // now update the atom positions of the rdkit_molecule from residue_p
   // (perhaps this should be in rdkit-interface.hh?)
   // 
   void update_coords(RDKit::RWMol *mol, int iconf, mmdb::Residue *residue_p);

      // alter restraints
   int assign_chirals(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_atoms(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints,
			     bool quartet_planes, bool quartet_hydrogen_planes);
   // alter restraints
   void add_chem_comp_aromatic_planes(const RDKit::ROMol &mol,
				      coot::dictionary_residue_restraints_t *restraints,
				      bool quartet_planes, bool quartet_hydrogen_planes);
   // which calls:
   dict_plane_restraint_t add_chem_comp_aromatic_plane_all_plane(const RDKit::MatchVectType &match,
								 const RDKit::ROMol &mol,
								 int plane_id_idx,
								 bool quartet_hydrogen_planes);
   // and
   // modify restraints
   void add_quartet_hydrogen_planes(const RDKit::ROMol &mol,
				    coot::dictionary_residue_restraints_t *restraints);

   // and
   //
   // return the number of added planes
   int add_chem_comp_aromatic_plane_quartet_planes(const RDKit::MatchVectType &match,
						   const RDKit::ROMol &mol,
						   coot::dictionary_residue_restraints_t *restraints,
						   int plane_id_idx);

   // alter restraints
   void add_chem_comp_deloc_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_sp2_N_planes(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);


   // alter restraints
   void fill_with_energy_lib_bonds(const RDKit::ROMol &mol,
				   const coot::energy_lib_t &energy_lib,
				   coot::dictionary_residue_restraints_t *restraints);
   // alter restraints
   void fill_with_energy_lib_angles(const RDKit::ROMol &mol,
				    const coot::energy_lib_t &energy_lib,
				    coot::dictionary_residue_restraints_t *restraints);

   // alter restraints
   void fill_with_energy_lib_torsions(const RDKit::ROMol &mol,
				      const coot::energy_lib_t &energy_lib,
				      coot::dictionary_residue_restraints_t *restraints);

   std::string convert_to_energy_lib_bond_type(RDKit::Bond::BondType bt);


   int assign_chirals_rdkit_tags(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);
   int assign_chirals_mmcif_tags(const RDKit::ROMol &mol, coot::dictionary_residue_restraints_t *restraints);

   // Use the pointer to test if the match was successful.
   std::pair<mmdb::Residue *, dictionary_residue_restraints_t>
   match_restraints_to_amino_acids(const dictionary_residue_restraints_t &restraints,
				   mmdb::Residue *residue_p);

}
