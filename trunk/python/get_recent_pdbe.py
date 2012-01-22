#    get_recent_pdbe.py
#    Copyright (C) 2011  Bernhard Lohkamp
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.


# FIXME: review this initiallisations
import gtk
import gobject
import os
gobject.threads_init()
# some timeout setting (used?)
import socket
socket.setdefaulttimeout(30)

import thread
safe_print = thread.allocate_lock()
def print_thread(txt):
    safe_print.acquire()
    print txt
    safe_print.release()

#global coot_pdbe_image_cache_dir
coot_pdbe_image_cache_dir = "coot-pdbe-images"  # can be shared (dir should
                                                # be writable by sharers)

def coot_pdbe():
    pass

#
def get_recent_json(file_name):

    import json  # maybe should be globally imported
    
    if not os.path.isfile(file_name):
        print "file not found", file_name
        return False
    else:
        fin = open(file_name, 'r')
        s = json.load(fin)
        fin.close()
        return s

# geometry is an improper list of ints.
# 
# return the h_box of the buttons.
#
# a button is a list of [label, callback-thunk, text-description]
#
# If check-button-label is False, don't make one, otherwise create with with
# the given label and "on" state.
#
def dialog_box_of_buttons_with_async_ligands(window_name, geometry,
                                             buttons, close_button_label):

    """geometry is an improper list of ints.
    
    return the h_box of the buttons.
    
    a button is a list of [label, callback-thunk, text-description]
    
    If check-button-label is False, don't make one, otherwise create with with
    the given label and "on" state.
    """

    # the async function is evaluated here
    #
    def add_button_info_to_box_of_buttons_vbox_for_ligand_images(button_info, vbox):
        button_label = button_info[0]
        callback = button_info[1]
        active_button_label_func = button_info[2] \
                                   if (len(button_info) > 2) \
                                   else False  # it doesnt have one
        button = gtk.Button()
        button_hbox         = gtk.HBox(False, 0)   # pixmaps and labels get added here.
        protein_ribbon_hbox = gtk.HBox(False, 0)  # do we need a box here?!
        ligands_hbox        = gtk.HBox(False, 0)

        button_hbox.pack_start(gtk.Label(button_label), False, 0)
        # or just button.set_label?!
        button_hbox.pack_start(ligands_hbox, False, 0)
        button_hbox.pack_start(protein_ribbon_hbox, False, 0)
        button.add(button_hbox)
        button.connect("clicked", callback)
        # this is for function, but thread here!?!
        #if callable(active_button_label_func):
        #   active_button_label_func(button_hbox,
        #                             ligands_hbox,
        #                             protein_ribbon_hbox)
        # 
        # if active_button_label_func:
        # ignore nmr for now
        if not callable(active_button_label_func):
            entry_id = active_button_label_func[0]
            ligand_tlc_list = active_button_label_func[1]
            make_active_ligand_button(entry_id, ligand_tlc_list,
                                      ligands_hbox, protein_ribbon_hbox)
            # we pass the button to update in thread!
        
        vbox.pack_start(button, False, False, 2)
        button.show()

        # the 'new' version, we threaded download and add the
    # images to the button's hboxes
    def make_active_ligand_button(entry_id, ligand_tlc_list,
                                  ligands_hbox, protein_ribbon_hbox):

        # main line
        image_size = 100
        for tlc in ligand_tlc_list:
            image_url = "http://www.ebi.ac.uk/pdbe-srv/pdbechem/image/showNew?code=" + \
                        tlc + "&size=" + str(image_size)
            image_name = os.path.join(coot_pdbe_image_cache_dir,
                                      (tlc + "-" + str(image_size) + ".gif"))
            cache_or_net_get_image(image_url, image_name, ligands_hbox)

        # now do the protein icon:
        image_size = 120
        image_name_stub = entry_id + "_cbc" + str(image_size) + ".png"
        image_url = "http://www.ebi.ac.uk/pdbe-srv/view/images/entry/" + \
                    image_name_stub
        
        entry_image_file_name = os.path.join(coot_pdbe_image_cache_dir,
                                             image_name_stub)
        cache_or_net_get_image(image_url, entry_image_file_name,
                               protein_ribbon_hbox)
        

    # main line
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    scrolled_win = gtk.ScrolledWindow()
    outside_vbox = gtk.VBox(False, 2)
    h_sep = gtk.HSeparator()
    inside_vbox = gtk.VBox(False, 0)

    window.set_default_size(geometry[0], geometry[1])
    window.set_title(window_name)

    inside_vbox.set_border_width(2)
    window.add(outside_vbox)

    outside_vbox.pack_start(scrolled_win, True, True, 0) # expand, fill, padding
    scrolled_win.add_with_viewport(inside_vbox)
    scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)
    from time import time
    map(lambda button_info:
        add_button_info_to_box_of_buttons_vbox_for_ligand_images(button_info,
                                                                 inside_vbox),
        buttons)

    outside_vbox.set_border_width(2)
    outside_vbox.pack_start(h_sep, False, False, 2)
    ok_button = gtk.Button(close_button_label)
    outside_vbox.pack_end(ok_button, False, False, 0)

    def delete_event(*args):
        # global setting of inside vbox (locked?)?
        inside_vbox = False
        window.destroy()
        return False
    
    ok_button.connect("clicked", delete_event)
    window.connect("destroy", delete_event)
    window.show_all()
    return inside_vbox

# Get image-name (caller doesn't care how) and when it is in place run func.
# This is a generally useful function, so it has been promoted outside of dig-table.
# mMMMmmm not sure if I want to run a lambda function here?! Let's see
# maybe this should be better queued, otherwise we may start lots of
# threads
#
def cache_or_net_get_image(image_url, image_name, hbox):

    import thread  # try to get away with the simple ones
    import urllib  # move to a proper place FIXME
    import threading

    def show_image_when_ready(image_name, hbox):
        if os.path.isfile(image_name):
            #apply(func)
            # instead of function we put image into vbox here
            pixmap = gtk.Image()
            pixmap.set_from_file(image_name)
            hbox.pack_start(pixmap, False, False, 1)
            pixmap.show()
            return False  # stop
        else:
            return True # continue

    def get_image(url, file_name):
        try:
            file_name, url_info = urllib.urlretrieve(url, file_name)
        except socket.timeout:
            print "BL ERROR:: timout download", url[-30:]
        except:
            print "BL ERROR:: failed download", url[-30:]
    # this takes time!! Use queue!?
    thread.start_new_thread(get_image, (image_url, image_name))
    gobject.timeout_add(1000, show_image_when_ready, image_name, hbox)


# return refmac_result or False
def refmac_calc_sfs_make_mtz(pdb_in_file_name, mtz_file_name,
                             mtz_refmaced_file_name):

    global refmac_extra_params
    
    refmac_stub = os.path.join("coot-refmac",
                               strip_path(file_name_sans_extension(pdb_in_file_name)))
    pdb_out_file_name = refmac_stub + "-refmaced.pdb"
    mtz_out_file_name = mtz_refmaced_file_name
    extra_cif_lib_filename = ""
    imol_refmac_count = 0
    swap_map_colours_post_refmac = 0
    imol_mtz_molecule = False
    show_diff_map_flag = 0
    phase_combine_flag = 0
    phib_fom_pair = False
    force_n_cycles = 0
    make_molecules_flag = 0 # ??

    save_refmac_extra_params = refmac_extra_params
    if isinstance(refmac_extra_params, types.ListType):
        refmac_extra_params.append("MAKE NEWLIGAND CONTINUE")
    else:
        refmac_extra_params = ["MAKE NEWLIGAND CONTINUE"]
        
    refmac_result = run_refmac_by_filename(pdb_in_file_name,
                                           pdb_out_file_name,
                                           mtz_file_name, mtz_out_file_name,
                                           extra_cif_lib_filename,
                                           imol_refmac_count,
                                           swap_map_colours_post_refmac,
                                           imol_mtz_molecule,
                                           show_diff_map_flag,
                                           phase_combine_flag,
                                           phib_fom_pair,
                                           force_n_cycles,
                                           make_molecules_flag,
                                           "", "F.F_sigF.F", "F.F_sigF.sigF", "Rfree.Flag.flag")
    # ccp4i-project-dir, f-col, sig-f-col, r-free-col

    # restore refmac-extra-params to what it used to be
    #
    refmac_extra_params = save_refmac_extra_params

    if os.path.isfile(mtz_refmaced_file_name):
        return refmac_result
    else:
        return False

# include_get_sfs_flag is either "no-sfs" or "include-sfs"
#
def pdbe_get_pdb_and_sfs_cif(include_get_sfs_flag,
                             entry_id, method_string=""):

    import urllib

    global download_thread_status
    download_thread_status = None     # start

    status = make_directory_maybe("coot-download")
    if (not status == 0):
        info_dialog("Failed to make download directory")
    else:
        # do it!

        # just a small bit of abstraction.
        #
        def make_and_draw_map_local(refmac_out_mtz_file_name):
            make_and_draw_map(refmac_out_mtz_file_name, "FWT", "PHWT", "", 0, 0)
            make_and_draw_map(refmac_out_mtz_file_name, "DELFWT", "PHDELWT", "", 0, 1)

        def get_sfs_run_refmac(sfs_cif_url, sfs_cif_file_name,
                               sfs_mtz_file_name, pdb_file_name,
                               refmac_out_mtz_file_name,
                               cif_progress_bar, window):

            global download_thread_status

            #
            def convert_to_mtz_and_refmac(sfs_cif_file_name,
                                          sfs_mtz_file_name,
                                          pdb_file_name):

                global download_thread_status
                # OK, let's run convert to mtz and run refmac
                #
                download_thread_status = "converting-to-mtz"
                convert_status = mmcif_sfs_to_mtz(sfs_cif_file_name,
                                                  sfs_mtz_file_name)
                if not (convert_status == 1):
                    # why cant we make a dialog?! (if this is threaded)
                    txt = "WARNING:: Failed to convert " +  \
                          sfs_cif_file_name + " to an mtz file"
                    print txt
                    #info_dialog(txt)
                    download_thread_status = "fail"
                else:
                    # why extra function!?
                    #refmac_inner(pdb_file_name, sfs_mtz_file_name,
                    #             refmac_out_mtz_file_name)
                    download_thread_status = "running-refmac-for-phases"
                    refmac_result = refmac_calc_sfs_make_mtz(pdb_file_name,
                                                             sfs_mtz_file_name,
                                                             refmac_out_mtz_file_name)
                    print "      refmac-result: ", refmac_result

                    # if refmac_result is good? (is tuple not list)
                    # good enough if it's not false?!
                    if not (isinstance(refmac_result, types.TupleType)):
                        download_thread_status = "fail-refmac"
                    else:
                        # make map
                        # cant do here!! we are in thread!!
                        #make_and_draw_map_local(refmac_out_mtz_file_name)
                        download_thread_status = "done"  #??

            # main line get_sfs_run_refmac
            print "in get_sfs_run_refmac", sfs_cif_file_name, sfs_mtz_file_name, pdb_file_name, refmac_out_mtz_file_name

            # check for cached results: only run refmac if
            # output file does not exist or is empty
            xxx = os.path.isfile(refmac_out_mtz_file_name)
            print "BL DEBUG:: file ", xxx
            if xxx: print ".. and size",os.stat(refmac_out_mtz_file_name).st_size
            if (os.path.isfile(refmac_out_mtz_file_name) and
                os.stat(refmac_out_mtz_file_name).st_size > 0):
                # using cached result, i.e. skip
                download_thread_status = "done"
            else:
                # the coot refmac interface writes its
                # output in coot-refmac directory.  If
                # that doesn't exist and we can't make
                # it, then give up.
                #
                if (not make_directory_maybe("coot_refmac") == 0):
                    info_dialog("Can't make output directory coot-refmac")
                else:
                    if (os.path.isfile(sfs_cif_file_name) and
                        os.stat(sfs_cif_file_name).st_size > 0):
                        # we have sfs_cif_file_name already
                        download_thread_status = "convert" # instruct to?
                        # needed
                    else:
                        # need to get sfs_mtz_file_name
                        download_thread_status = "downloading-sfs"
                        cif_thread = thread.start_new_thread(download_file_and_update_widget, (sfs_cif_url, sfs_cif_file_name, cif_progress_bar, window))
                        while download_thread_status == "downloading-sfs":
                            gtk.main_iteration(False)
                        print "BL DEBUG:: done with cif thread?!"
                    if (not download_thread_status == "fail"):
                        # threaded!?
                        convert_to_mtz_and_refmac(sfs_cif_file_name,
                                                  sfs_mtz_file_name,
                                                  pdb_file_name)

            
        # return a list of the progress bars and the window 
        # 
        # (the pdb-file-name and sfs-cif-file-name are passed so
        # that the cancel button knows what transfers to cancel (if
        # needed)).
        # FIXME:: all needed?!?
        def progress_dialog(pdb_file_name, sfs_cif_file_name):
            window = gtk.Window(gtk.WINDOW_TOPLEVEL)
            dialog_name = "Download and make SFS for " + entry_id
            main_vbox = gtk.VBox(False, 6)
            cancel_button = gtk.Button("  Cancel  ")
            buttons_hbox = gtk.HBox(False, 2)
            pdb_hbox = gtk.HBox(False, 6)
            cif_hbox = gtk.HBox(False, 6)
            refmac_hbox = gtk.HBox(False, 6)
            pdb_label = gtk.Label("Download PDB:  ")
            cif_label = gtk.Label("Download SFs cif:")
            refmac_label = gtk.Label("Running Refmac:")
            # FIXME: shouldnt we rather change the label than making
            # or adding a new one?!
            refmac_fail_label = gtk.Label("Running Refmac Failed")
            fail_label = gtk.Label("Something Went Wrong")
            pdb_progress_bar = gtk.ProgressBar()
            cif_progress_bar = gtk.ProgressBar()
            refmac_progress_bar = gtk.ProgressBar()

            pdb_execute_icon = gtk.image_new_from_stock("gtk-execute", 1)
            cif_execute_icon = gtk.image_new_from_stock("gtk-execute", 1)
            refmac_execute_icon = gtk.image_new_from_stock("gtk-execute", 1)
            pdb_good_icon = gtk.image_new_from_stock("gtk-ok", 1)
            cif_good_icon = gtk.image_new_from_stock("gtk-ok", 1)
            refmac_good_icon = gtk.image_new_from_stock("gtk-ok", 1)

            pdb_fail_icon = gtk.image_new_from_stock("gtk-no", 1)
            cif_fail_icon = gtk.image_new_from_stock("gtk-no", 1)
            refmac_fail_icon = gtk.image_new_from_stock("gtk-no", 1)
            h_sep = gtk.HSeparator()

            window.set_title(dialog_name)
            buttons_hbox.pack_start(cancel_button, True, False, 2)
            pdb_hbox.pack_start(pdb_label, True, False, 2)
            pdb_hbox.pack_start(pdb_progress_bar, True, False, 3)
            # NOTE: shouldnt we rather replace the icons?! Not sure
            pdb_hbox.pack_start(pdb_execute_icon, False, False, 2)
            pdb_hbox.pack_start(pdb_good_icon, False, False, 2)
            pdb_hbox.pack_start(pdb_fail_icon, False, False, 2)
            cif_hbox.pack_start(cif_label, True, False, 2)
            cif_hbox.pack_start(cif_progress_bar, True, False, 3)
            cif_hbox.pack_start(cif_execute_icon, False, False, 2)
            cif_hbox.pack_start(cif_good_icon, False, False, 2)
            cif_hbox.pack_start(cif_fail_icon, False, False, 2)
            refmac_hbox.pack_start(refmac_label, True, False, 2)
            refmac_hbox.pack_start(refmac_progress_bar, True, False, 3)
            refmac_hbox.pack_start(refmac_execute_icon, False, False, 2)
            refmac_hbox.pack_start(refmac_good_icon, False, False, 2)
            refmac_hbox.pack_start(refmac_fail_icon, False, False, 2)

            main_vbox.pack_start(pdb_hbox, True, False, 4)
            main_vbox.pack_start(cif_hbox, True, False, 4)
            main_vbox.pack_start(refmac_hbox, True, False, 4)
            main_vbox.pack_start(refmac_fail_label, True, False, 2)
            main_vbox.pack_start(fail_label, True, False, 2)
            main_vbox.pack_start(h_sep, True, False, 4)
            main_vbox.pack_start(buttons_hbox, True, False, 4)
            main_vbox.set_border_width(6)

            window.add(main_vbox)
            window.set_border_width(4)
            window.show_all()
            # hide the icons
            pdb_good_icon.hide()
            cif_good_icon.hide()
            refmac_good_icon.hide()
            pdb_fail_icon.hide()
            cif_fail_icon.hide()
            refmac_fail_icon.hide()
            refmac_fail_label.hide()
            fail_label.hide()

            pdb_execute_icon.set_sensitive(False)
            cif_execute_icon.set_sensitive(False)
            refmac_execute_icon.set_sensitive(False)

            def cancel_button_cb(*args):
                global download_thread_status
                window.destroy()
                download_thread_status = "cancelled"

            cancel_button.connect("clicked", cancel_button_cb)

            # return a dictionary (cant we do a lookup directly?!)
            return {"window": window,
                    "pdb_progress_bar": pdb_progress_bar,
                    "cif_progress_bar": cif_progress_bar,
                    "refmac_progress_bar": refmac_progress_bar,
                    "refmac_fail_label": refmac_fail_label,
                    "fail_label": fail_label,
                    "pdb_execute_icon": pdb_execute_icon,
                    "cif_execute_icon": cif_execute_icon,
                    "refmac_execute_icon": refmac_execute_icon,
                    "pdb_fail_icon": pdb_fail_icon,
                    "cif_fail_icon": cif_fail_icon,
                    "refmac_fail_icon": refmac_fail_icon,
                    "pdb_good_icon": pdb_good_icon,
                    "cif_good_icon": cif_good_icon,
                    "refmac_good_icon": refmac_good_icon}

        # main line
        pdb_url = "http://www.ebi.ac.uk/pdbe-srv/view/files/" + \
                  entry_id + ".ent"
        sfs_cif_url = "http://www.ebi.ac.uk/pdbe-srv/view/files/r" + \
                      entry_id + "sf.ent"
        pdb_file_name = os.path.join("coot-download", entry_id + ".ent")
        sfs_cif_file_name = os.path.join("coot-download", "r" + entry_id + "sf.cif")
        sfs_mtz_file_name = os.path.join("coot-download", "r" + entry_id + "sf.mtz")
        refmac_out_mtz_file_name = os.path.join("coot-download",
                                                "r" + entry_id + "-refmac.mtz")
        refmac_log_file_name = "refmac-from-coot-" + \
                               str(refmac_count) + ".log" # set in run_refmac_by_filename
        progr_widgets = progress_dialog(pdb_file_name, sfs_cif_file_name)
        window = progr_widgets["window"]
        pdb_progress_bar = progr_widgets["pdb_progress_bar"]
        cif_progress_bar = progr_widgets["cif_progress_bar"]
        refmac_progress_bar = progr_widgets["refmac_progress_bar"]
        refmac_fail_label = progr_widgets["refmac_fail_label"]
        fail_label = progr_widgets["fail_label"]
        pdb_execute_icon = progr_widgets["pdb_execute_icon"]
        cif_execute_icon = progr_widgets["cif_execute_icon"]
        refmac_execute_icon = progr_widgets["refmac_execute_icon"]
        pdb_good_icon = progr_widgets["pdb_good_icon"]
        cif_good_icon = progr_widgets["cif_good_icon"]
        refmac_good_icon = progr_widgets["refmac_good_icon"]
        pdb_fail_icon = progr_widgets["pdb_fail_icon"]
        cif_fail_icon = progr_widgets["cif_fail_icon"]
        refmac_fail_icon = progr_widgets["refmac_fail_icon"]

        def download_file_and_update_widget(url, file_name,
                                            progress_bar, window):
            global download_thread_status
            def update_progressbar_in_download(numblocks, blocksize,
                                               filesize, progress_bar):
                global download_thread_status
                percent = min((numblocks*blocksize*100)/filesize, 100)
                #print "BL DEBUG:: update pg bar" , percent, numblocks, blocksize, filesize, progress_bar
                if download_thread_status == "cancelled":
                    sys.exit()
                gobject.idle_add(progress_bar.set_fraction, percent/100.)
                # in the end should change icons?!

            try:
                print "BL DEBUG:: downloading", url
                
                file_name_local, url_info = urllib.urlretrieve(url, file_name,
                                                               lambda nb, bs, fs, progress_bar=progress_bar:
                                                               update_progressbar_in_download(nb, bs, fs, progress_bar))
                download_thread_status = "done-download"
            except s1ocket.timeout:
                print "BL ERROR:: timout download", url
                download_thread_status = "fail"
            except IOError:
                print "BL ERROR:: ioerror downloading", url
                download_thread_status = "fail"
            except:
                print "BL ERROR:: general problem downloading", url
                download_thread_status = "fail"

        # or shall this be in the timeout function!?
        def update_refmac_progress_bar(refmac_progress_bar, log_file_name):
            # refmac puts out 100 lines of text before it starts running. 
            # Let's not count those as progress of the computation (otherwise 
            # we jump to 22% after a fraction of a second).
            max_lines = 350     # thats 450 - 100
            n_lines = file_n_lines(log_file_name)
            if n_lines:  # check for number?
                n_lines_rest = n_lines - 100.
                if (n_lines > 0):
                    f = n_lines_rest / max_lines
                    if (f < 1):
                        refmac_progress_bar.set_fraction(f)

        def update_dialog_and_check_download(*args):
            global download_thread_status

            if (download_thread_status == "downloading-pdb"):
                pdb_execute_icon.set_sensitive(True)

            if (download_thread_status == "downloading-sfs"):
                pdb_progress_bar.set_fraction(1.)
                pdb_execute_icon.hide()
                pdb_good_icon.show()
                cif_execute_icon.set_sensitive(True)

            if (download_thread_status == "converting-to-mtz"):
                pdb_progress_bar.set_fraction(1.)
                cif_progress_bar.set_fraction(1.)

            if (download_thread_status == "running-refmac-for-phases"):
                pdb_progress_bar.set_fraction(1.)
                cif_progress_bar.set_fraction(1.)
                refmac_execute_icon.set_sensitive(True)
                cif_execute_icon.hide()
                cif_good_icon.show()
                # de-sensitise cancel?!
                update_refmac_progress_bar(refmac_progress_bar,
                                           refmac_log_file_name)

            if (download_thread_status == "fail-refmac"):
                refmac_fail_label.show()
                refmac_execute_icon.hide()
                refmac_fail_icon.show()
                return False  # dont continue

            # generic fail (don't turn off execute icons because we 
            # don't know *what* failed. (Not so useful).
            if (download_thread_status == "fail"):
                fail_label.show()
                return False # dont continue

            if (download_thread_status == "done"):
                make_and_draw_map_local(refmac_out_mtz_file_name)
                window.destroy()
                return False  # we are done!
            return True

        # could update progressbar here too!?
        gobject.timeout_add(200, update_dialog_and_check_download,
                            window,
                            pdb_execute_icon, pdb_good_icon, pdb_fail_icon,
                            cif_execute_icon, cif_good_icon, cif_fail_icon,
                            pdb_progress_bar, cif_progress_bar,
                            refmac_progress_bar, refmac_log_file_name,
                            refmac_fail_label, fail_label)

        # Try with individual threads just for downloading, alternative use
        # Paul's approach and make functions to be executed in the the idle
        # function

        # Get the PDB file if we don't have it already.
        if not os.path.isfile(pdb_file_name):
            download_thread_status = "downloading-pdb"
            pdb_thread = thread.start_new_thread(download_file_and_update_widget,
                                                 (pdb_url, pdb_file_name, pdb_progress_bar,
                                                  window))
            while download_thread_status == "downloading-pdb":
                gtk.main_iteration(False)
            print "BL DEBUG:: done with pdb thread?!"
        # read the pdb
        imol = read_pdb(pdb_file_name)
        if not valid_model_molecule_qm(imol):
            s = "Oops - failed to correctly read " + pdb_file_name
            info_dialog(s)
        if (include_get_sfs_flag != "include-sfs"):
            # an NMR structure
            #
            # FIXME:: better later as timeout_add
            # read_pdb(pdb_file_name)
            download_thread_status = "done"  #?
            print "BL DEBUG:: NMR structure!?"
        else:
            # An X-ray structure
            #
            # now read the sfs cif and if that is good then
            # convert to mtz and run refmac, but for now, let's
            #  just show the PDB file.
            if not (os.path.isfile(sfs_cif_file_name) and
                    os.path.isfile(refmac_out_mtz_file_name)):
                # download and run refmac
                #get_sfs_run_refmac(sfs_cif_url, sfs_cif_file_name,
                thread.start_new_thread(get_sfs_run_refmac, (sfs_cif_url, sfs_cif_file_name,
                                   sfs_mtz_file_name, pdb_file_name,
                                   refmac_out_mtz_file_name,
                                   cif_progress_bar, window))
            else:
                # OK, files are here already.
                #
                # FIXME:: again for later
                # read_pdb(pdb_file_name)
                # make_and_draw_map_local(refmac_out_mtz_file_name)
                download_thread_status = "done"

    
def recent_structure_browser(t):

    import string
    import thread
    global refmac_count
    global download_thread_status
    download_thread_status = None  # initiallise

    def get_dic_all_entries(js):
        # simple minded, we know where the entries are,
        # so return the dictionary of these
        return js["ResultSet"]["Result"]

    n_atoms_limit_small_ligands = 6
    
    # truncate (if needed) and newlinify string
    #
    def pad_and_truncate_name(name):
        sl = len(name)
        if (sl < 60):
            return "\n     " + name
        else:
            return "\n     " + name[0:60] + "..."
        
    # as above but no preceeding newline and tab
    def truncate_name(name):
        sl = len(name)
        if (sl < 70):
            return name
        else:
            return name[0:70] + "..."

    #
    def make_ligands_string(ligand_dic):

        if not ligand_dic:
            return ""
        else:
            ligand_string_list = []
            for ligand in ligand_dic:
                name = ligand["Name"]
                code = ligand["Code"]
                n_atoms = ligand["NumberOfAtoms"]

                if not name:
                    ligand_string_list.append("")
                    continue
                if not n_atoms:
                    ligand_string_list.append(pad_and_truncate_name(name))
                    continue
                if n_atoms > n_atoms_limit_small_ligands:
                    if code:
                        name = code + ": " + name
                    ligand_string_list.append(pad_and_truncate_name(name))
                else:
                    ligand_string_list.append("")  # too small to be interesting
            ligand_string = string.join(ligand_string_list, "")
            if (len(ligand_string) > 0):
                return "\nLigands:" + ligand_string
            else:
                return ""

    # return a list (possibly empty) of the three-letter-codes of the
    # ligands in the entry.  The tlc are strings -  (if some
    # problem ocurred on dehashing or it was a small/uninteresting
    # ligand), it wont be added to the list
    #
    def make_ligands_tlc_list(ligand_dic):
        if not ligand_dic:
            return []
        else:
            ligand_list = []
            for ligand in ligand_dic:
                code = ligand["Code"]
                n_atoms = ligand["NumberOfAtoms"]
                if not code:
                    continue
                if not n_atoms:
                    ligand_list.append(code)
                    continue
                else:
                    if n_atoms > n_atoms_limit_small_ligands:
                        ligand_list.append(code)
            return ligand_list

    # The idea here is that we have a list of ligand tlcs.  From that,
    # we make a function, which, when passed a button-hbox (you can put
    # things into a button) will download images from pdbe and add them
    # to the button.  We download the images in the background
    # (sub-thread) and put them in coot-pdbe-ligand-images directory.
    # When/if the tranfer of the file is completed and good, set the
    # status to a function.
    #
    # a timeout function watches the value of the status and when it is
    # a function, it runs it.  (The function puts the image in the
    # button-hbox.)
    # 
    # Also we construct the url of a ribbon picture give the entry id
    #
    def make_active_ligand_button_func(entry_id, ligand_tlc_list):
        # do we need this?!
        return


    # dic is a dictionary with keys: "Title", "Resolution", "EntryID", etc.
    #
    def handle_pdb_entry_entity(dic):

        # return a string. Return "" on failure
        def make_authors_string(citation_dic):
            citation = ""
            if not citation_dic:
                citation = "No Authors listed"
            else:
                citation_js = citation_dic[0]
                if not citation_js:
                    citation = "No Citation table"
                else:
                    citation = string_append_with_spaces(citation_js["Authors"])
            return citation

        #global coot_pdbe_image_cache_dir
        make_directory_maybe(coot_pdbe_image_cache_dir)

        # now make a button list (a label and what to do)
        entry_id = str(dic["EntryID"])
        entry_label = entry_id if entry_id else "missing entry id"  # should not happen, the latter!?
        method_item = dic["Method"]
        resolution_item = dic["Resolution"]
        title_item = dic["Title"]
        authors_string = make_authors_string(dic["Citation"])
        method_label = method_item if method_item else "Unknown method"  # should not happen, the latter!?
        title_label = truncate_name(title_item) if title_item else ""
        authors_label = pad_and_truncate_name(authors_string) \
                        if (len(authors_string) > 0)  else ""
        ligands = dic["ContainsLigands"]
        ligands_string = make_ligands_string(ligands)
        ligand_tlc_list = make_ligands_tlc_list(ligands)
        resolution_string = ("     Resolution: " + resolution_item) \
                            if resolution_item else ""
        label = title_label + "\n" + entry_id + ": " + method_label + \
                resolution_string + authors_label + ligands_string

        if (method_label == "x-ray diffraction"):
            return[label,
                   lambda func: pdbe_get_pdb_and_sfs_cif("include-sfs",
                                                         entry_id,
                                                         method_label),
                   # we dont do a funtion here, but use the args
                   # when we construct the button!?
                   [entry_id, ligand_tlc_list]]
        else:
            return[label,
                   lambda func: pdbe_get_pdb_and_sfs_cif("no-sfs",
                                                         entry_id,
                                                         method_label),
                   [entry_id, ligand_tlc_list]]
                   # this one (may) has to be lambda func too
                   #lambda func2:
                   #make_active_ligand_button_func(entry_id, ligand_tlc_list)]

    # return a list of buttons
    #
    def handle_pdb_entry_entities(dic):
        return map(handle_pdb_entry_entity, dic)
                    
    # main line!?
    button_list = handle_pdb_entry_entities(get_dic_all_entries(t))
    dialog_box_of_buttons_with_async_ligands("Recent Entries", [700, 500],
                                             button_list, " Close ")
    
def recent_entries_progress_dialog():

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    dialog_name = "Getting recent entries list..."
    main_vbox = gtk.VBox(False, 4)
    label = gtk.Label("Getting recent entries list...")
    progress_bar = gtk.ProgressBar()

    window.set_title(dialog_name)
    main_vbox.pack_start(label, False, False, 4)
    main_vbox.pack_start(progress_bar, False, False, 4)
    window.add(main_vbox)
    window.set_border_width(4)
    window.show_all()

    return (progress_bar, window)
    

# Sameer Velankar says to get this file for the latest releases
# "http://www.ebi.ac.uk/pdbe-apps/jsonizer/latest/released/" (note the end "/")
#
def pdbe_latest_releases_gui():

    import threading
    import urllib

    url = "http://www.ebi.ac.uk/pdbe-apps/jsonizer/latest/released/"
    #url = "http://www.ebi.ac.uk/xxxxxxxxxxxxxxxxx"
    json_file_name = "latest-releases.json"

    add_status_bar_text("Retrieving list of latest releases...")
    # FIXME:: progress bar!?
    progress_bars = recent_entries_progress_dialog()

    class MyURLopener(urllib.FancyURLopener):
        def http_error_default(self, url, fp, errcode, errmsg, headers):
            # handle errors the way you'd like to
            # raise StandardError, ("File not found?")
            pass
    
    class GetUrlThread(threading.Thread):
        def __init__(self, url, file_name, progressbar, window):
            self.url = url
            self.file_name = file_name
            self.window = window
            self.progressbar = progressbar
            self.status = None
            threading.Thread.__init__(self)

        def get_url_status(self):
            return self.status

        def update_function(self, count, blockSize, totalSize):
            # total size is -1! Baeh!!
            percent = int(count*blockSize*100/totalSize)
            gobject.idle_add(self.update_progressbar, percent)
        
        def update_progressbar(self, count):
            self.progressbar.pulse()
            return False

        def destroy_window(self):
            self.window.destroy()
            
        def run(self):
            try:
                # FIXME: local file name has to be empty I think, so remove first
                # maybe not, but add a timeout, otherwise we may hang!
                self.file_name, url_info = MyURLopener().retrieve(self.url, self.file_name, self.update_function)
                self.status = 0 #?
            except socket.timeout:
                print "BL DEBUG:: timout download", self.url
            except IOError:
                print "BL DEBUG:: ioerror"
            except:
                self.status = 1
                # FIXME here dies with the thread. need to go to main thread
                gobject.idle_add(info_dialog, "Failed to get recent entry list from server.")
                    
            gobject.idle_add(self.destroy_window)

    #w = gtk.Window()
    #l = gtk.Label()
    #w.add(l)
    #w.show_all()
    #w.connect("destroy", lambda _: gtk.main_quit())
    #thread = GetUrlThread(progressbar)
    thread = GetUrlThread(url, json_file_name,
                          progress_bars[0], progress_bars[1])
    thread.start()

    def start_table():
        if thread.get_url_status() == 0:
            recent_structure_browser(get_recent_json(json_file_name))
            return False  # stop
        return True  # continue
            
    gobject.timeout_add(600, start_table)
    
