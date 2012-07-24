/* ligand/find-waters.cc
 * 
 * Copyright 2004, 2005, 2006, 2007 The University of York
 * Copyright 2008 by The University of Oxford
 * Author: Paul Emsley
  * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



// #include "clipper/ccp4/ccp4_map_io.h"
#include "clipper/ccp4/ccp4_mtz_io.h"
#include "clipper/core/map_interp.h"
#include "clipper/core/hkl_compute.h"

#include "coot-map-utils.hh"


void
coot::util::map_fill_from_mtz(clipper::Xmap<float> *xmap,
			      std::string mtz_file_name,
			      std::string f_col,
			      std::string phi_col,
			      std::string weight_col,
			      short int use_weights,
			      short int is_diff_map) {

   coot::util::map_fill_from_mtz(xmap, mtz_file_name, f_col, phi_col, weight_col,
				 use_weights, is_diff_map, 0, 0);
}

void
coot::util::map_fill_from_mtz(clipper::Xmap<float> *xmap,
			      std::string mtz_file_name,
			      std::string f_col,
			      std::string phi_col,
			      std::string weight_col,
			      short int use_weights,
			      short int is_diff_map,
			      float reso_limit_high,
			      short int use_reso_limit_high) {

  clipper::HKL_info myhkl; 
  clipper::MTZdataset myset; 
  clipper::MTZcrystal myxtl; 

  std::cout << "reading mtz file..." << std::endl; 
  clipper::CCP4MTZfile mtzin; 
  mtzin.open_read( mtz_file_name );       // open new file 
  mtzin.import_hkl_info( myhkl );         // read sg, cell, reso, hkls
  clipper::HKL_data< clipper::datatypes::F_sigF<float> >   f_sigf_data(myhkl, myxtl);
  clipper::HKL_data< clipper::datatypes::Phi_fom<float> > phi_fom_data(myhkl, myxtl);
  clipper::HKL_data< clipper::datatypes::F_phi<float> >       fphidata(myhkl, myxtl); 


  if ( use_weights ) {
     clipper::String dataname = "/*/*/[" + f_col + " " + f_col + "]";
     std::cout << dataname << "\n";
     mtzin.import_hkl_data(  f_sigf_data, myset, myxtl, dataname ); 
     dataname = "/*/*/[" + phi_col + " " + weight_col + "]";
     std::cout << dataname << "\n";
     mtzin.import_hkl_data( phi_fom_data, myset, myxtl, dataname );
     mtzin.close_read(); 
     std::cout << "We should use the weights: " << weight_col << std::endl;

     fphidata.compute(f_sigf_data, phi_fom_data,
		      clipper::datatypes::Compute_fphi_from_fsigf_phifom<float>());
     
  } else {
     clipper::String dataname = "/*/*/[" + f_col + " " + phi_col + "]";
     mtzin.import_hkl_data(     fphidata, myset, myxtl, dataname );
     mtzin.close_read(); 
  }
  std::cout << "Number of reflections: " << myhkl.num_reflections() << "\n"; 
  std::cout << "finding ASU unique map points..." << std::endl;

  clipper::Resolution fft_reso;
  if (use_reso_limit_high) {
     clipper::Resolution user_resolution(reso_limit_high);
     fft_reso = user_resolution;
     coot::util::filter_by_resolution(&fphidata, 99999.0, reso_limit_high);
  } else {
     fft_reso = clipper::Resolution(1.0/sqrt(fphidata.invresolsq_range().max()));
  }
  
  xmap->init( myhkl.spacegroup(), myhkl.cell(),
	      clipper::Grid_sampling( myhkl.spacegroup(),
				      myhkl.cell(),
				      fft_reso));
  std::cout << "Grid..." << xmap->grid_sampling().format() << "\n";
  std::cout << "doing fft..." << std::endl;
  xmap->fft_from( fphidata );                  // generate map
  std::cout << "done fft..." << std::endl;
}

// Note density_at_point in molecule-class-info looks as if its
// returning grid values, not interpollated values.  Is that bad?
//
float
coot::util::density_at_point(const clipper::Xmap<float> &xmap,
			     const clipper::Coord_orth &pos) {

   float dv;
   clipper::Coord_frac a_cf = pos.coord_frac(xmap.cell());
   clipper::Coord_map  a_cm = a_cf.coord_map(xmap.grid_sampling());
   clipper::Interp_cubic::interp(xmap, a_cm, dv);
   return dv;
}

void
coot::util::filter_by_resolution(clipper::HKL_data< clipper::datatypes::F_phi<float> > *fphidata,
				 const float &reso_low,
				 const float &reso_high) {

   float inv_low  = 1.0/(reso_low*reso_low);
   float inv_high = 1.0/(reso_high*reso_high);
   int n_data = 0;
   int n_reset = 0;
      
   for (clipper::HKL_info::HKL_reference_index hri = fphidata->first(); !hri.last(); hri.next()) {
      n_data++;

      if ( hri.invresolsq() > inv_low &&
	   hri.invresolsq() < inv_high) {
      } else {
	 (*fphidata)[hri].f() = 0.0;
	 n_reset++;
      } 
   }
}


float
coot::util::density_at_map_point(const clipper::Xmap<float> &xmap,
				 const clipper::Coord_map   &cm) {

   float dv;
   clipper::Interp_cubic::interp(xmap, cm, dv);
   return dv;
}

coot::util::density_stats_info_t
coot::util::density_around_point(const clipper::Coord_orth &point,
				 const clipper::Xmap<float> &xmap,
				 float d) {

   coot::util::density_stats_info_t s;
   // what are the points?
   // +/- d along the x, y and z axes
   // and 2x4 samples at 45 degrees inclination and 45 degree longitudinal rotation.
   //
   std::vector<clipper::Coord_orth> sample_points(14);

   sample_points[0] = clipper::Coord_orth( 0.0,  0.0,  1.0);
   sample_points[1] = clipper::Coord_orth( 0.0,  0.0, -1.0);
   sample_points[2] = clipper::Coord_orth( 0.0,  1.0,  0.0);
   sample_points[3] = clipper::Coord_orth( 0.0, -1.0,  0.0);
   sample_points[4] = clipper::Coord_orth(-1.0,  0.0,  0.0);
   sample_points[5] = clipper::Coord_orth( 1.0,  0.0,  0.0);

   sample_points[6] = clipper::Coord_orth( 0.5,  0.5,  0.7071);
   sample_points[7] = clipper::Coord_orth(-0.5,  0.5,  0.7071);
   sample_points[8] = clipper::Coord_orth(-0.5, -0.5,  0.7071);
   sample_points[9] = clipper::Coord_orth( 0.5, -0.5,  0.7071);

   sample_points[10] = clipper::Coord_orth( 0.5,  0.5, -0.7071);
   sample_points[11] = clipper::Coord_orth(-0.5,  0.5, -0.7071);
   sample_points[12] = clipper::Coord_orth(-0.5, -0.5, -0.7071);
   sample_points[13] = clipper::Coord_orth( 0.5, -0.5, -0.7071);

   float dv;
   for (float scale = 0.2; (scale-0.0001)<=1.0; scale += 0.4) { 
      for (int i=0; i<14; i++) {
	 dv = density_at_point(xmap, point + d*scale*sample_points[i]);
	 s.add(dv, scale); // scale is the weight, which multiplies
			   // additions internally.
      }
   } 
   
   return s;
}


float
coot::util::map_score(PPCAtom atom_selection,
		      int n_selected_atoms,
		      const clipper::Xmap<float> &xmap,
		      short int with_atomic_weighting) {

   // Thanks Ezra.

   float f = 0.0;
   float f1;

   for (int i=0; i<n_selected_atoms; i++) {
      f1 = density_at_point(xmap, clipper::Coord_orth(atom_selection[i]->x,
						      atom_selection[i]->y,
						      atom_selection[i]->z));
      f1 *= atom_selection[i]->occupancy;
      f += f1;
   }
   return f;
}

float coot::util::map_score_atom(CAtom *atom,
				 const clipper::Xmap<float> &xmap) {

   float f = 0;
   if (atom) { 
      f = density_at_point(xmap, clipper::Coord_orth(atom->x, atom->y, atom->z));
   }
   return f;
}




float
coot::util::max_gridding(const clipper::Xmap<float> &xmap) {

   float a_gridding = xmap.cell().a()/xmap.grid_sampling().nu(); 
   float b_gridding = xmap.cell().b()/xmap.grid_sampling().nv(); 
   float c_gridding = xmap.cell().c()/xmap.grid_sampling().nw(); 

   float gridding_max = 0; 
   
   if ( a_gridding > gridding_max) 
      gridding_max = a_gridding; 
   if ( b_gridding > gridding_max) 
      gridding_max = b_gridding; 
   if ( c_gridding > gridding_max) 
      gridding_max = c_gridding; 

   return gridding_max; 
}

clipper::RTop_orth
coot::util::make_rtop_orth_from(mat44 mat) {

   clipper::Mat33<double> clipper_mat(mat[0][0], mat[0][1], mat[0][2],
				      mat[1][0], mat[1][1], mat[1][2],
				      mat[2][0], mat[2][1], mat[2][2]);
   clipper::Coord_orth  cco(mat[0][3], mat[1][3], mat[2][3]);
   clipper::RTop_orth rtop(clipper_mat, cco);
   
   return rtop;
}

clipper::Xmap<float>
coot::util::sharpen_map(const clipper::Xmap<float> &xmap_in, float sharpen_factor) {

   clipper::HKL_info myhkl; 
   clipper::HKL_data< clipper::datatypes::F_phi<float> >       fphidata(myhkl); 

   xmap_in.fft_to(fphidata);

//    for (clipper::HKL_info::HKL_reference_index hri = fphidata->first(); !hri.last(); hri.next()) {
//       float reso = hri.invresolsq();
//       std::cout << hri.format() << " has reso " << reso << std::endl;
//       float fac = 1.0;
      
//       (*fphidata)[hri].f() = fac;
//    }

   clipper::Xmap<float> r;
   r.fft_from(fphidata);
   return r;
}


clipper::Xmap<float>
coot::util::transform_map(const clipper::Xmap<float> &xmap_in,
			  const clipper::RTop_orth &rtop,
			  const clipper::Coord_orth &to_pt,
			  float box_size) {

   // we now need to create about_pt: i.e. where the map is pulled *from*
   clipper::Coord_orth about_pt = to_pt.transform(rtop.inverse());

   clipper::Xmap<float> xmap;
   xmap.init(xmap_in.spacegroup(), xmap_in.cell(), xmap_in.grid_sampling());
   clipper::Coord_orth pt_new_centre = about_pt.transform(rtop);
   clipper::Grid_sampling grid = xmap.grid_sampling();
   clipper::Grid_range gr(xmap_in.cell(), xmap_in.grid_sampling(), 2*box_size);
   clipper::Coord_grid g, g0, g1;
   clipper::RTop_orth rtop_inv = rtop.inverse();
   typedef clipper::Xmap<float>::Map_reference_coord MRC;
   MRC i0, iu, iv, iw;
   g = pt_new_centre.coord_frac(xmap_in.cell()).coord_grid(xmap_in.grid_sampling());

   std::cout << "DEBUG:: pulling map from point:   " << about_pt.format() << std::endl;
   std::cout << "DEBUG:: creating map about point: " << pt_new_centre.format() << std::endl;
   
   g0 = g + gr.min();
   g1 = g + gr.max();
   i0 = MRC( xmap, g0 );
   for ( iu = i0; iu.coord().u() <= g1.u(); iu.next_u() )
      for ( iv = iu; iv.coord().v() <= g1.v(); iv.next_v() )
	 for ( iw = iv; iw.coord().w() <= g1.w(); iw.next_w() ) {
	    clipper::Coord_orth dpt = iw.coord().coord_frac(xmap.grid_sampling()).coord_orth(xmap_in.cell()).transform(rtop_inv);
	    xmap[iw] = coot::util::density_at_point(xmap_in, dpt);
	 }
   
   return xmap;
}


std::pair<float, float>
coot::util::mean_and_variance(const clipper::Xmap<float> &xmap) {

   clipper::Xmap_base::Map_reference_index ix;
   double sum = 0.0;
   double sum_sq = 0.0;
   int npoints = 0;
   float d;
   for (ix = xmap.first(); !ix.last(); ix.next() ) {
      npoints++;
      d = xmap[ix];
      sum += d;
      sum_sq += d*d;
   }
   double mean = 0.0;
   double var = -1.0; 

   if (npoints > 0) { 
      mean = sum/float(npoints);
      var = sum_sq/float(npoints) - mean*mean;
   }

   return std::pair<float, float> (mean, var);
}


clipper::Xmap<float>
coot::util::laplacian_transform(const clipper::Xmap<float> &xmap_in) {

   clipper::Xmap<float> laplacian = xmap_in;

   clipper::Coord_map pos;
   float val;
   clipper::Grad_map<float> grad;
   clipper::Curv_map<float> curv;
   
   clipper::Xmap_base::Map_reference_index ix;
   for (ix = xmap_in.first(); !ix.last(); ix.next())  {
      // xmap_in.interp_curv(pos, val, grad, curv);
      clipper::Interp_cubic::interp_curv(xmap_in, ix.coord().coord_map(), val, grad, curv);
      val = curv.det();
      laplacian[ix] = -val;
   }
   
   return laplacian;
}

// Spin the torsioned atom round the rotatable bond and find the
// orientation (in degrees) that is in the highest density.
// 
// return a torsion.  Return -1111 (less than -1000) on failure
float
coot::util::spin_search(const clipper::Xmap<float> &xmap, CResidue *res, coot::torsion tors) {

   // The plan:
   //
   // 1) find 4 atoms in residue that correspond to the torsion
   //
   // 2) Use rotate point about vector to generate new points
   //
   //      Test density at new points

   float best_ori = -1111.1; //returned thing
   
   std::vector<CAtom * > match_atoms = tors.matching_atoms(res);

   if (match_atoms.size() != 4) { 
      std::cout << "ERROR:: not all atoms for torsion found in residue!" << std::endl;
      std::cout << "        (found " << match_atoms.size() << " atoms.)" << std::endl;
   } else {

      clipper::Coord_orth pa1(match_atoms[0]->x, match_atoms[0]->y, match_atoms[0]->z);
      clipper::Coord_orth pa2(match_atoms[1]->x, match_atoms[1]->y, match_atoms[1]->z); 
      clipper::Coord_orth pa3(match_atoms[2]->x, match_atoms[2]->y, match_atoms[2]->z);
      clipper::Coord_orth pa4(match_atoms[3]->x, match_atoms[3]->y, match_atoms[3]->z);

      float best_d = -99999999.9;
      for (double theta=0; theta <=360; theta+=3.0) {

	 clipper::Coord_orth dir   = pa3 - pa2;
	 clipper::Coord_orth pos   = pa4;
	 clipper::Coord_orth shift = pa3;
	 clipper::Coord_orth co = coot::util::rotate_round_vector(dir, pos, shift, theta);
	 float this_d = coot::util::density_at_point(xmap, co);
	 if (this_d > best_d) {
	    best_d = this_d;
	    best_ori = theta;
// 	    std::cout << "better density " <<  best_d << " at "
// 		      << co.format() << " " << best_ori << std::endl;
	 }
      }
   }
   // std::cout << "returning " << best_ori << std::endl;
   return best_ori;
}

// return a map and its standard deviation.  scale is applied to
// map_in_2 before substraction.
std::pair<clipper::Xmap<float>, float>
coot::util::difference_map(const clipper::Xmap<float> &xmap_in_1,
			   const clipper::Xmap<float> &xmap_in_2,
			   float map_scale) {
   clipper::Xmap<float> r = xmap_in_1;
   float std_dev = 0.2;

    clipper::Xmap_base::Map_reference_index ix;
    for (ix = r.first(); !ix.last(); ix.next())  {
       clipper::Coord_map  cm1 = ix.coord().coord_map();
       clipper::Coord_frac cf1 = cm1.coord_frac(xmap_in_1.grid_sampling());
       clipper::Coord_orth co  = cf1.coord_orth(xmap_in_1.cell());
       clipper::Coord_frac cf2 =  co.coord_frac(xmap_in_2.cell());
       clipper::Coord_map  cm2 = cf2.coord_map(xmap_in_2.grid_sampling());
       float map_2_val;
       clipper::Interp_cubic::interp(xmap_in_2, cm2, map_2_val);
       r[ix] = xmap_in_1[ix] - map_scale * map_2_val;
    }
   return std::pair<clipper::Xmap<float>, float> (r, std_dev); 
}