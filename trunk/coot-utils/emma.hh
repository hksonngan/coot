
namespace coot { 
   namespace util {
      
      class emma {
	 void sfs_from_boxed_molecule(CMMDBManager *mol, float border);
	 double f(double r) const;
	 double	 f(const clipper::HKL_info::HKL_reference_index &hri_model,
		   const clipper::HKL_info::HKL_reference_index &hri_map,
		   double va) const;
      public:
	 emma(CMMDBManager *mol, float border) {
	    sfs_from_boxed_molecule(mol, border);
	 } 
	 clipper::Spacegroup spacegroup;
	 clipper::Cell cell;
	 clipper::Resolution reso;
	 clipper::HKL_info hkl_info;
	 clipper::HKL_data<clipper::data32::F_phi> fc_from_model;
	 void overlap(const clipper::Xmap<float> &xmap) const;
	 void overlap_simple(const clipper::Xmap<float> &xmap) const;
	 void test() const;
      };
   }
}
  
