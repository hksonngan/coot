
#ifndef HAVE_GL_BITS_HH
#define HAVE_GL_BITS_HH

class gl_context_info_t {
public:
   GtkWidget *widget_1;
   GtkWidget *widget_2;

   gl_context_info_t(GtkWidget *widget_1_in, GtkWidget *widget_2_in) {
      widget_1 = widget_1_in;
      widget_2 = widget_2_in;
   } 
};


#endif // HAVE_GL_BITS_HH
