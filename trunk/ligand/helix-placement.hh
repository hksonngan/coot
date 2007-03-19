
#include "clipper/core/coords.h"
#include "clipper/core/xmap.h"
#include "mini-mol.hh"
#include "coot-map-utils.hh"

namespace coot {

   class scored_helix_info_t {
   public:
      minimol::molecule mol;
      float score;
      scored_helix_info_t(const minimol::molecule &mol_in, float score_in) {
	 mol = mol_in;
	 score = score_in;
      }
      scored_helix_info_t() { score = -9999999.9;}
   };

   class eigen_info_t {
   public:
      clipper::RTop_orth rtop;
      int best_eigen_value_index;
      std::vector<double> eigen_values;
      eigen_info_t(const clipper::RTop_orth &rtop_in,
		   int best_eigen_index_in,
		   const std::vector<double> &eigen_values_in) {
	 rtop = rtop_in;
	 best_eigen_value_index = best_eigen_index_in;
	 eigen_values = eigen_values_in;
      }
   };

   class helix_placement_info_t {
   public:
      // the first mol [0] is the best fit.  If there is a second one
      // in [1] then it is the reverse fit.
      std::vector<minimol::molecule> mol;
      short int success;
      std::string failure_message;
      helix_placement_info_t(const minimol::molecule &mol_in,
			     short int success_in,
			     const std::string f_message_in) {
	 mol.clear();
	 mol.push_back(mol_in);
	 success = success_in;
	 failure_message = f_message_in;
      }
      helix_placement_info_t(const std::vector<minimol::molecule> &mol_v,
			     short int success_in,
			     const std::string f_message_in) {
	 mol = mol_v;
	 success = success_in;
	 failure_message = f_message_in;
      }
   };
      
   class helix_placement {

      clipper::Xmap<float> xmap;
      clipper::Coord_orth
      move_helix_centre_point_guess(const clipper::Coord_orth &pt) const;
      eigen_info_t helix_eigen_system(const clipper::Coord_orth pt, float search_radius) const;
      helix_placement_info_t get_20_residue_helix_standard_orientation(int n_residues) const;
      helix_placement_info_t get_20_residue_helix(int nresidues) const;
      float score_helix_position(const minimol::molecule &m) const;
      // pair(other atoms, c-betas)
      std::vector<clipper::RTop_orth> 
      optimize_rotation_fit(const minimol::molecule &helix,
					       const clipper::RTop_orth &axis_ori,
					       const clipper::Coord_orth &helix_point) const;
      std::pair<std::vector<clipper::Coord_orth>, std::vector<clipper::Coord_orth> >
      decompose_helix_by_cbeta(coot::minimol::molecule &m) const;
      util::density_stats_info_t score_residue(const coot::minimol::residue &residue) const;
      util::density_stats_info_t score_atoms(const std::vector<clipper::Coord_orth> &atom_pos) const;
      std::pair<int, int> trim_ends(minimol::fragment *m, float min_density_limit) const; // modify m by chopping off residues
      int trim_end(minimol::fragment *m, short int end_type, float min_density_limit) const;

      void build_on_N_end(minimol::fragment *m, float min_density_limit) const;
      void build_on_C_end(minimol::fragment *m, float min_density_limit) const;
      minimol::residue
      build_N_terminal_ALA(const clipper::Coord_orth &prev_n,
			   const clipper::Coord_orth &prev_ca,
			   const clipper::Coord_orth &prev_c,
			   int seqno) const;
      minimol::residue
      build_C_terminal_ALA(const clipper::Coord_orth &prev_n,
			   const clipper::Coord_orth &prev_ca,
			   const clipper::Coord_orth &prev_c,
			   int seqno) const;
      // tinker with m
      void trim_and_grow(minimol::molecule *m, float min_density_limit) const;

      // factoring out for strand tubes
      clipper::RTop_orth
      find_best_tube_orientation(clipper::Coord_orth ptc,
				 double cyl_len, double cyl_rad) const; // uses member data xmap

      scored_helix_info_t fit_strand(const coot::minimol::molecule &mol,
				     const clipper::RTop_orth &rtop) const;
      
   public:
      helix_placement(const clipper::Xmap<float> &xmap_in) {
	 xmap = xmap_in;
      }
      void discrimination_map() const; //debugging

      // Pass the initial testing helix length.  Typically start with
      // 20, try 12 if that fails.
      helix_placement_info_t place_alpha_helix_near(const clipper::Coord_orth &pt,
						    int n_helix_residues_start) const;
      
      // Kevin's engine: do MR-like search on the surface of a
      // cylinder, not just the eigen vectors
      helix_placement_info_t place_alpha_helix_near_kc_version(const clipper::Coord_orth &pt,
							       int n_helix_residues_start) const;

      // and now for strands, we use much of the same code, including
      // the perhaps mis-leading helper class names
      helix_placement_info_t place_strand(const clipper::Coord_orth &pt, int n_residues);
      
   };
}
