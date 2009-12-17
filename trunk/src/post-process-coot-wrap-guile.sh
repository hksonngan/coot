
function add_ifdefs { 

pre_tmp=$1
post=$2
    
echo adding gtk2 and guile ifdefs from $pre_tmp $post
echo "#ifdef USE_GUILE"  > $post
if [ "$gtk2" = gtk2 ] ; then
   echo "#ifdef COOT_USE_GTK2_INTERFACE"  >> $post
else 
   echo "#ifndef COOT_USE_GTK2_INTERFACE"  >> $post
fi    
cat $pre_tmp >> $post
echo "#endif // COOT_USE_GTK2_INTERFACE" >> $post
echo "#endif // USE_GUILE" >> $post

}


#####  start ################

if [ $# != 4 ] ; then
   echo must provide path-to-guile-config gtk2-flag and 2 file arguments
   echo we got: $*
   exit 2
fi


pre="$3"
post="$4"

guile_config=$1
gtk2=$2 

guile_version=`$guile_config --version 2>&1`

echo guile_config:  $guile_config
echo guile_version: $guile_version
echo gtk2: $gtk2

# if guile_config was blank (as can be the case when we compile
# without guile, then we want a new blank file for $post.
# Otherwise, do the filtering and addtion of the ifdefs
#
if [ -z "$guile_config" ] ; then 
   if [ -e "$post" ] ; then 
      rm -f "$post"
   fi
   touch "$post"

else 


   case $guile_version in

      *1.6*) 
      echo sed -e 's/static char .gswig_const_COOT_SCHEME_DIR/static const char *gswig_const_COOT_SCHEME_DIR/' $pre $post
           sed -e 's/static char .gswig_const_COOT_SCHEME_DIR/static const char *gswig_const_COOT_SCHEME_DIR/' $pre > $pre.tmp
      add_ifdefs $pre.tmp $post
      ;; 
   
      *1.8*)
      # SCM_MUST_MALLOC to be replaced by scm_gc_malloc is more complicated.  scm_gc_malloc takes 2 args and 
      # SCM_MUST_MALLOC takes one.  Leave it to be fixed in SWIG.
      echo sed -e 's/SCM_STRING_CHARS/scm_to_locale_string/'  \
               -e 's/SCM_STRINGP/scm_is_string/'              \
               -e 's/SCM_STRING_LENGTH/scm_c_string_length/'  \
               -e 's/static char .gswig_const_COOT_SCHEME_DIR/static const char *gswig_const_COOT_SCHEME_DIR/' \
               -e '/.libguile.h./{x;s/.*/#include <cstdio>/;G;}' \
               $pre ..to.. $post
           sed -e 's/SCM_STRING_CHARS/scm_to_locale_string/'      \
               -e 's/SCM_STRINGP/scm_is_string/'                  \
               -e 's/SCM_STRING_LENGTH/scm_c_string_length/'      \
               -e 's/static char .gswig_const_COOT_SCHEME_DIR/static const char *gswig_const_COOT_SCHEME_DIR/' \
               -e '/.libguile.h./{x;s/.*/#include <cstdio>/;G;}' \
               $pre > $pre.tmp
      add_ifdefs $pre.tmp $post
      ;; 
   esac

fi




