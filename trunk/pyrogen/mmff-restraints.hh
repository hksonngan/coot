
#include "Python.h"

// #include <string>
// #include <vector>

#include "lidia-core/rdkit-interface.hh"

namespace coot { 
   // this uses atom indices
   class mmff_bond_restraint_info_t {
   public:
      mmff_bond_restraint_info_t() { sigma = -1;}
      mmff_bond_restraint_info_t(unsigned int idx_1_in,
				 unsigned int idx_2_in,
				 const std::string &type_in,
				 const double bl,
				 const double sigma_in) {
	 idx_1 = idx_1_in;
	 idx_2 = idx_2_in;
	 type = type_in;
	 resting_bond_length = bl;
	 sigma = sigma_in;
      }
      unsigned int idx_1;
      unsigned int idx_2;
      std::string type;
      double resting_bond_length;
      double sigma; // pseudo sigma based on K_{bond}
      unsigned int get_idx_1() const { return idx_1; } 
      unsigned int get_idx_2() const { return idx_2; }
      std::string get_type() const { return type; }
      double get_resting_bond_length() const { return resting_bond_length; }
      double get_sigma() const { return sigma; } 
   };

   // this uses atom indices
   class mmff_angle_restraint_info_t {
   public:
      mmff_angle_restraint_info_t() { sigma = -1;}
      mmff_angle_restraint_info_t(unsigned int idx_1_in,
				  unsigned int idx_2_in,
				  unsigned int idx_3_in,
				  const double angle,
				  const double sigma_in) {
	 idx_1 = idx_1_in;
	 idx_2 = idx_2_in;
	 idx_3 = idx_3_in;
	 resting_angle = angle;
	 sigma = sigma_in;
      }
      unsigned int idx_1;
      unsigned int idx_2;
      unsigned int idx_3;
      double resting_angle;
      double sigma; // pseudo sigma based on K_{angle}
      unsigned int get_idx_1() const { return idx_1; } 
      unsigned int get_idx_2() const { return idx_2; }
      unsigned int get_idx_3() const { return idx_3; }
      double get_resting_angle() const { return resting_angle; }
      double get_sigma() const { return sigma; } 
   };

   class mmff_b_a_restraints_container_t {
   public:
      std::vector<mmff_bond_restraint_info_t>  bonds;
      std::vector<mmff_angle_restraint_info_t> angles;
      mmff_b_a_restraints_container_t() { }
      unsigned int bonds_size() const { return bonds.size(); }
      unsigned int angles_size() const { return angles.size(); }

      // these will crash if you feed them an out-of-bounds index
      mmff_bond_restraint_info_t get_bond(const unsigned int i) {
	 return bonds[i];
      } 
      mmff_angle_restraint_info_t get_angle(const unsigned int i) {
	 return angles[i];
      }
      
   };

   mmff_b_a_restraints_container_t * mmff_bonds_and_angles(RDKit::ROMol &mol);

}
