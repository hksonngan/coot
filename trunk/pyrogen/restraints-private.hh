
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
   int assign_chirals(const RDKit::ROMol &mol, dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_atoms(const RDKit::ROMol &mol, dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_planes(const RDKit::ROMol &mol, dictionary_residue_restraints_t *restraints,
			     bool quartet_planes, bool quartet_hydrogen_planes);
   // alter restraints
   void add_chem_comp_aromatic_planes(const RDKit::ROMol &mol,
				      dictionary_residue_restraints_t *restraints,
				      bool quartet_planes, bool quartet_hydrogen_planes);
   // which calls:
   dict_plane_restraint_t add_chem_comp_aromatic_plane_all_plane(const RDKit::MatchVectType &match,
								 const RDKit::ROMol &mol,
								 int plane_id_idx,
								 bool quartet_hydrogen_planes);
   // and
   // modify restraints
   void add_quartet_hydrogen_planes(const RDKit::ROMol &mol,
				    dictionary_residue_restraints_t *restraints);

   // and
   //
   // return the number of added planes
   int add_chem_comp_aromatic_plane_quartet_planes(const RDKit::MatchVectType &match,
						   const RDKit::ROMol &mol,
						   dictionary_residue_restraints_t *restraints,
						   int plane_id_idx);

   // alter restraints
   void add_chem_comp_deloc_planes(const RDKit::ROMol &mol, dictionary_residue_restraints_t *restraints);
   // alter restraints
   void add_chem_comp_sp2_N_planes(const RDKit::ROMol &mol, dictionary_residue_restraints_t *restraints);


   // alter restraints
   void fill_with_energy_lib_bonds(const RDKit::ROMol &mol,
				   const energy_lib_t &energy_lib,
				   dictionary_residue_restraints_t *restraints);
   // which calls
   bool add_torsion_to_restraints(dictionary_residue_restraints_t *restraints,
				  const RDKit::ROMol &mol,
				  const RDKit::ATOM_SPTR at_1,
				  const RDKit::ATOM_SPTR at_2,
				  const RDKit::ATOM_SPTR at_3,
				  const RDKit::ATOM_SPTR at_4,
				  unsigned int *tors_no,
				  unsigned int *const_no,
				  const energy_lib_t &energy_lib);


   // alter restraints
   void fill_with_energy_lib_angles(const RDKit::ROMol &mol,
				    const energy_lib_t &energy_lib,
				    dictionary_residue_restraints_t *restraints);

   // alter restraints
   void fill_with_energy_lib_torsions(const RDKit::ROMol &mol,
				      const energy_lib_t &energy_lib,
				      dictionary_residue_restraints_t *restraints);

   std::string convert_to_energy_lib_bond_type(RDKit::Bond::BondType bt);


   int assign_chirals_rdkit_tags(const RDKit::ROMol &mol, dictionary_residue_restraints_t *restraints);
   int assign_chirals_mmcif_tags(const RDKit::ROMol &mol, dictionary_residue_restraints_t *restraints);

   // for returning best graph-match data (for dictionary atom name map to reference)
   // 
   class matching_dict_t {
      mmdb::Residue *residue;
      bool filled_flag;
   public:
      dictionary_residue_restraints_t dict;
      matching_dict_t() {
	 residue = NULL;
	 filled_flag = false;
      }
      matching_dict_t(mmdb::Residue *res, const dictionary_residue_restraints_t &d) {
	 residue = res;
	 dict = d;
	 filled_flag = true;
      }
      bool filled() const { return filled_flag; }
   };

   

   // Use the pointer to test if the match was successful.
   // Old-style embedded list of test compounds
   matching_dict_t
   match_restraints_to_amino_acids(const dictionary_residue_restraints_t &restraints,
				   mmdb::Residue *residue_p);

   matching_dict_t
   match_restraints_to_reference_dictionaries(const coot::dictionary_residue_restraints_t &restraints,
					      mmdb::Residue *residue_p,
					      const std::vector<std::string> &test_comp_ids,
					      const std::vector<std::string> &test_mmcif_file_names);


}

