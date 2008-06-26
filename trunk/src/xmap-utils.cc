
#include <iostream>

#include "clipper/core/xmap.h"

#include "xmap-utils.h"

template <class T>
mean_and_variance<T>
map_density_distribution(const clipper::Xmap<T> &map, short int write_output_flag) { 


   mean_and_variance<T> mv;  // returned object

   double min = 1e10, max = -1e10, sum = 0.0, sum_sq = 0;
   double v; // optimised?
   double n_point = 0.0;
   
   clipper::Xmap_base::Map_reference_index ix;
   for (ix=map.first(); !ix.last(); ix.next()) {

      n_point += 1.0; 
      v = double (map[ix]); 

      if (v < min) min = v; 
      if (v > max) max = v;

      sum += v;
      sum_sq += v*v; 

   }

   float mean = float( sum/n_point );

   float var = float( (n_point*sum_sq - sum*sum) / (n_point*n_point) );
   float range = float( max - min );
   float inv_range = (range>0.0) ? (1.0/range) : (1.0);


   mv.mean = mean;
   mv.variance = var; 
   mv.range = range;
   mv.max_density = max;
   mv.min_density = min;


   if (write_output_flag) { 
      std::cout << "Map statistics: mean: " << mean << " st.d: " << sqrt(var)
	   << std::endl;
      std::cout <<  "Map statistics: min: " << min << ", max: " << max << std::endl;

      int nbins = 20, bin_no; 
      std::vector<int> bin(nbins+1);

      for (int ii=0; ii<= nbins; ii++)
	 bin[ii] = 0;
      
   
      for (ix=map.first(); !ix.last(); ix.next()) {
	 bin_no = int (nbins*(map[ix] - min)*inv_range);
	 bin[bin_no]++; 
      }

      for (int ii=0; ii<= nbins; ii++) {
	 std::cout.width(8);
	 std::cout << ((float) (ii) + 0.5)*range/20.0 + min << "    " << bin[ii] << std::endl;
      }
   }
   return mv; 
}

// instantiate that 
template mean_and_variance<float> map_density_distribution(const clipper::Xmap<float> &map, short int write_output_flag); 
// template mean_and_variance<int> map_density_distribution(const clipper::Xmap<int> &map, short int write_output_flag); 


// ostream& operator<<(ostream& s, mean_and_variance<T> mv) { 

//    s << "mean: " << mv.mean << ", sigma: " << sqrt(mv.variance); 
//    return s; 
// }




float nearest_step(float val, float step) { 

   return step*nint(val/step); 

} 


int 
nint(float val) { 

  if (val > 0) {
    return int (val + 0.5); 
  } else { 
    return int (val - 0.5); 
  }

} 


