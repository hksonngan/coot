
AC_DEFUN([AM_GUILE_NET_HTTP],
[

dnl If Guile is configured for and found then ac_cv_path_GUILE is set to something sensible
dnl If that is the case, then we check for net http, goosh and gui (separately).
dnl If that is NOT the case (e.g. we configured only for python) then
dnl    we do not exit if net http, goosh or gui were not found.

AC_MSG_CHECKING([if we have net hhtp])
if test -z "$ac_cv_path_GUILE" ; then 
   have_net_http=not_installed
else 
   guile -c '(use-modules (oop goops) (oop goops describe) (net http))'
   if test "$?" = 0  ; then 
      have_net_http=yes
   else 
     have_net_http=no
   fi
fi
AC_MSG_RESULT([$have_net_http])
if test "$have_net_http" = no  ; then 
   echo Must install net-http for guile.
   exit 2
fi
])


AC_DEFUN([AM_GUILE_GOOSH],
[
AC_MSG_CHECKING([if we have Guile Goosh])
if test -z "$ac_cv_path_GUILE" ; then 
   have_goosh=not_installed
else 
   guile -c '(use-modules (goosh))'
   if test "$?" = 0  ; then 
      have_goosh=yes
   else 
      have_goosh=no
   fi
fi
AC_MSG_RESULT([$have_goosh])
if test "$have_goosh" = no  ; then 
   echo Must install goosh for guile.
   exit 2
fi
])



AC_DEFUN([AM_GUILE_GUI],
[
AC_MSG_CHECKING([if we have Guile GUI])
if test -z "$ac_cv_path_GUILE" ; then 
   have_guile_gui=not_installed
else 
   guile -c '(use-modules (gui event-loop))'
   if test "$?" = 0  ; then 
      have_guile_gui=yes
   else 
      have_guile_gui=no
   fi
fi
AC_MSG_RESULT([$have_guile_gui])
if test "$have_guile_gui" = no  ; then 
   echo Must install guile-gui for guile.
   exit 2
fi
])

