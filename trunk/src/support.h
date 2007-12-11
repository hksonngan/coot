#ifndef BEGIN_C_DECLS
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif
#endif

BEGIN_C_DECLS

/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
/*#  define gettext(String) (String) */
/* #  define dgettext(Domain,Message) (Message) */
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif


/*
 * Public Functions.
 */

/*
 * This function returns a widget in a component created by Glade.
 * Call it with the toplevel widget in the component (i.e. a window/dialog),
 * or alternatively any widget in the component, and the name of the widget
 * you want returned.
 */
GtkWidget*  lookup_widget              (GtkWidget       *widget,
                                        const gchar     *widget_name);

/* get_widget() is deprecated. Use lookup_widget instead. */
#define get_widget lookup_widget

/* Use this function to set the directory containing installed pixmaps. */
void        add_pixmap_directory       (const gchar     *directory);


/*
 * Private Functions.
 */

/* This is used to create the pixmaps in the interface. */
GtkWidget*  create_pixmap              (GtkWidget       *widget,
                                        const gchar     *filename);

/* This is an internally used function to find pixmap files. */
static gchar*
find_pixmap_file                       (const gchar     *filename);

/* This is used to create the pixbufs used in the interface - from Gtk2's support.h. */
#if (GTK_MAJOR_VERSION > 1) 
GdkPixbuf*  create_pixbuf              (const gchar     *filename);
#endif /*  (GTK_MAJOR_VERSION > 1)  */


END_C_DECLS

