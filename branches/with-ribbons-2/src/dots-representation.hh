
#ifndef DOTS_REPRESENTATION_HH
#define DOTS_REPRESENTATION_HH
#include <string>
#include <vector>
#include <clipper/core/coords.h>
#include <mmdb/mmdb_manager.h>

#include "coot-colour.hh"
#include "coot-utils/coot-coord-utils.hh"

namespace coot { 
   class dots_representation_info_t {
      bool is_closed;
      double get_radius(const std::string &ele) const;
      coot::colour_t get_colour(const std::string &ele) const;      
      std::string name_;
   public:
      dots_representation_info_t() {
	 is_closed = 0;
      }
      std::vector<std::pair<coot::colour_t, std::vector<clipper::Coord_orth> > > points;
      // 20111123 modern usage
      dots_representation_info_t(const std::string &name_in) {
	 name_ = name_in;
      }
      dots_representation_info_t(const std::vector<clipper::Coord_orth> &points_in) {
	 points.push_back(std::pair<coot::colour_t, std::vector<clipper::Coord_orth> > (coot::colour_t(0.3, 0.4, 0.5), points_in));
	 is_closed = 0;
      }
      dots_representation_info_t(CMMDBManager *mol);
      // make dots around the atoms of mol, only if they are close to
      // atoms of mol_exclude
      dots_representation_info_t(CMMDBManager *mol, CMMDBManager *mol_exclude);
      // mol_exclude can be NULL.
      void add_dots(int SelHnd_in, CMMDBManager *mol, CMMDBManager *mol_exclude,
		    double dots_density, const colour_t &single_col, bool use_single_col);
      void close_yourself() {
	 points.clear();
	 is_closed = 1;
      }
      void pure_points(CMMDBManager *mol); // don't surface mol, the surface points *are* the
					   // (synthetic) atoms in mol.
      bool is_open_p() const {
	 int r = 1 - is_closed;
	 return r;
      }
      void set_name(const std::string &name_in) { name_ = name_in; }
      std::string name() const { return name_;}

      // This is added later (20100413)
      // 
      // what is the fraction solvent exposure of the atoms in the atom
      // selection?  Calculate it for all the atoms in the selection (of
      // mol) and use all the atoms of mol to "bump into" each atom (and
      // that of course reduces the fraction of solvent exposure.
      // 
      std::vector<std::pair<CAtom *, float> > solvent_exposure(int SelHnd_in, CMMDBManager *mol) const;

      // create (and later delete, of course) a new molecule by deep
      // copying and assembling the passed residues.  Use that to make
      // a atom selection which gets passed to
      // dots_representation_info_t::solvent_exposure()
      // 
      std::vector<std::pair<coot::atom_spec_t, float> >
      solvent_accessibilities(CResidue *res_ref, const std::vector<CResidue *> &residues) const;
      std::vector<solvent_exposure_difference_helper_t>
      solvent_exposure_differences(CResidue *res_ref, const std::vector<CResidue *> &residues) const;
   };

}

#endif // DOTS_REPRESENTATION_HH
