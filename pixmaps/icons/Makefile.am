
EXTRA_DIST = hicolor_apps_48x48_coot.png   \
	     hicolor_apps_64x64_coot.png   \
	     hicolor_apps_96x96_coot.png   \
	     hicolor_apps_256x256_coot.png \
	     hicolor_apps_scalable_coot.svg

install-data-local:
	for icon in $(EXTRA_DIST); do \
		THEME=`   echo $$icon | cut -d_ -f1`; \
		CONTEXT=` echo $$icon | cut -d_ -f2`; \
		SIZE=`    echo $$icon | cut -d_ -f3`; \
		ICONFILE=`echo $$icon | cut -d_ -f4`; \
		mkdir -p $(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;
