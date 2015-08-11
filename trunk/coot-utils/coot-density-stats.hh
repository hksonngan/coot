
#ifndef COOT_DENSITY_STATS_HH
#define COOT_DENSITY_STATS_HH

namespace coot {

   enum map_stats_t {
      SIMPLE,
      WITH_KOLMOGOROV_SMIRNOV_DIFFERENCE_MAP_TEST};

   namespace util {

      class density_stats_info_t {
	 
      public:
	 // these are doubles because we can potentially do lots (millions) of tiny adds.
	 double n;
	 double sum_sq; // sum of squared elements
	 double sum;
	 double sum_weight;

	 density_stats_info_t() {
	    n = 0.0;
	    sum = 0.0;
	    sum_sq = 0.0;
	    sum_weight = 0.0;
	 }
	 void add(const double &v) {
	    n += 1.0;
	    sum += v;
	    sum_sq += v*v;
	    sum_weight += 1.0;
	 } 
	 void add(const double &v, const double &weight) {
	    n += weight;
	    sum += weight*v;
	    sum_sq += weight*v*v;
	    sum_weight += 1.0;
	 }
	 std::pair<double, double> mean_and_variance() const {
	    double mean = -1;
	    double var  = -1;
	    if (n > 0) {
	       mean = sum/sum_weight;
	       var = sum_sq/sum_weight - mean*mean;
	    }
	    return std::pair<double, double> (mean, var);
	 }
      };


      class density_correlation_stats_info_t {
      public:
	 double n;
	 double sum_xy;
	 double sum_sqrd_x;
	 double sum_sqrd_y;
	 double sum_x;
	 double sum_y;
	 // for doing KS tests (against normal distribution) , we want
	 // all the density samples
	 std::vector<double> density_values;
	 density_correlation_stats_info_t() {
	    n = 0;
	    sum_xy = 0;
	    sum_sqrd_x = 0;
	    sum_sqrd_y = 0;
	    sum_x = 0;
	    sum_y = 0;
	 }
	 density_correlation_stats_info_t(double n_in,
					  double sum_xy_in,
					  double sum_sqrd_x_in,
					  double sum_sqrd_y_in,
					  double sum_x_in,
					  double sum_y_in) {
	    n = n_in;
	    sum_xy = sum_xy_in;
	    sum_sqrd_x = sum_sqrd_x_in;
	    sum_sqrd_y = sum_sqrd_y_in;
	    sum_x = sum_x_in;
	    sum_y = sum_y_in;
	 }
	 double var_x() const {
	    double mean_x = sum_x/n;
	    return (sum_sqrd_x/n - mean_x * mean_x);
	 } 
	 double var_y() const {
	    double mean_y = sum_y/n;
	    return (sum_sqrd_y/n - mean_y * mean_y);
	 } 
	 double correlation() const {
	    double top = n * sum_xy - sum_x * sum_y;
	    double b_1 = n * sum_sqrd_x - sum_x * sum_x;
	    double b_2 = n * sum_sqrd_y - sum_y * sum_y;
	    if (b_1 < 0) b_1 = 0;
	    if (b_2 < 0) b_2 = 0;
	    double c = top/(sqrt(b_1) * sqrt(b_2));
	    return c;
	 } 
      }; 

   }
}

#endif // COOT_DENSITY_STATS_HH
