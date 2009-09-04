# fitting.py
# Copyright 2005, 2006, 2007 by Bernhard Lohkamp 
# Copyright 2004, 2005, 2006, 2008, 2009 by The University of York
# Copyright 2009 by Bernhard Lohkamp
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


# For each residue in the protein (molecule number @var{imol}), do a
# rotamer fit and real-space refinement.  Update the graphics and
# rotate the scene at each residue for eye candy goodness.
#
# Note that residue with alt confs do not undergo auto-fit-rotamer.
# This is because that autofit-rotamer then refine will tend to put
# both rotamers into the same place.  Not good.  It seems a
# reasonable expectation that residues with an alternate conformation
# are already resonably well-fitted.  So residues with alternate
# conformations undergo only real space refinement.
#
# This is simple-minded and outdated now we have the interruptible
# version (below).
#
def fit_protein(imol):

    set_go_to_atom_molecule(imol)
    make_backup(imol) # do a backup first
    backup_mode = backup_state(imol)
    imol_map  = imol_refinement_map()
    replacement_state = refinement_immediate_replacement_state()

    if imol_map == -1:
	info_dialog("Oops.  Must set a map to fit")
    else:

	turn_off_backup(imol)
	set_refinement_immediate_replacement(1)
	  
	for chain_id in chain_ids(imol):
         if (not is_solvent_chain_qm(imol,chain_id)):
	     n_residues = chain_n_residues(chain_id,imol)
             print "There are %(a)i residues in chain %(b)s" % {"a":n_residues,"b":chain_id}
	       
	     for serial_number in range(n_residues):
		  
                res_name = resname_from_serial_number(imol,chain_id,serial_number)
                res_no = seqnum_from_serial_number(imol,chain_id,serial_number)
                ins_code = insertion_code_from_serial_number(imol,chain_id,serial_number)
                
                if (ins_code is not None):
                    if (not res_name=="HOH"):
                        for alt_conf in residue_alt_confs(imol, chain_id, res_no, ins_code):
                            print "centering on ",chain_id,res_no," CA"
                            set_go_to_atom_chain_residue_atom_name(chain_id,res_no,"CA")
                            rotate_y_scene(30,0.3) # n-frames frame-interval(degrees)
                            if (alt_conf == ""):
                                auto_fit_best_rotamer(res_no, alt_conf, ins_code, chain_id, imol,
                                                      imol_map, 1, 0.1)
                            if (imol_map >= 0):
	 			refine_zone(imol, chain_id, res_no, res_no, alt_conf)
                                accept_regularizement()
                            rotate_y_scene(30,0.3)
      
    if (replacement_state == 0):
	  set_refinement_immediate_replacement(0)
    if (backup_mode == 1):
	  turn_on_backup(imol)

# Paul: 20090517: thinking about making the fit-protein function
# interuptable with a toolbar button press.  How do we do that? 
# fit_protein needs to be split into 2 parts, one, that generates a
# list of residues specs the other that does a refinement given a
# residue spec, then we run and idle function that calls
# fit_residue_by_spec or each spec in turn and at the end return
#

# These 2 variables are used by multi-refine function(s), called by
# idle functions to refine just one residue.
# 
global continue_multi_refine
global multi_refine_spec_list
global multi_refine_idle_proc
global multi_refine_stop_button
global multi_refine_cancel_button
global multi_refine_continue_button
continue_multi_refine = False
multi_refine_spec_list = []
multi_refine_idle_proc = False
multi_refine_stop_button = False
multi_refine_cancel_button = False
multi_refine_continue_button = False

# Return a list of residue specs
#
# chain-specifier can be a string, where it is the chain of interest.
# or 'all-chains, where all chains are chosen.
#
def fit_protein_make_specs(imol, chain_specifier):
    from types import StringType
    if (not valid_model_molecule_qm(imol)):
        return []
    else:
        if (chain_specifier == 'all-chains'):
            chain_list = chain_ids(imol)
        elif (type(chain_specifier) == StringType):
            chain_list = [chain_specifier]
        else:
            chain_list = []   # return null list - incomprehensible

        spec_list = []
        for chain_id in chain_list:
            serial_number_list = range(chain_n_residues(chain_id, imol))
            print " seqnum from ", imol, chain_id, serial_number_list
            for serial_number in serial_number_list:
                res_no   = seqnum_from_serial_number(imol, chain_id,
                                                   serial_number)
                ins_code = insertion_code_from_serial_number(imol, chain_id,
                                                             serial_number)
                if ins_code is not None:
                    spec_list.append([imol, chain_id, res_no, ins_code])
                else:
                    # bad ins_code mean no such residue -> pass
                    pass
        return spec_list

def fit_protein_fit_function(res_spec, imol_map):

    imol     = res_spec[0]
    chain_id = res_spec[1]
    res_no   = res_spec[2]
    ins_code = res_spec[3]

    # BL says: we dont use with_auto_accept here, get's too messy
    replace_state = refinement_immediate_replacement_state()
    set_refinement_immediate_replacement(1)

    for alt_conf in residue_alt_confs(imol, chain_id, res_no, ins_code):
        print "centering on", chain_id, res_no, "CA"
        set_go_to_atom_chain_residue_atom_name(chain_id, res_no, "CA")
        rotate_y_scene(10, 0.3) # n_frames frame_interval(degrees)

        res_name = residue_name(imol, chain_id, res_no, ins_code)
        if (not res_name == "HOH"):
            if (alt_conf == ""):
                auto_fit_best_rotamer(res_no, alt_conf, ins_code, chain_id, imol,
                                      imol_map, 1, 0.1)
            if (imol_map >= 0):
                refine_zone(imol, chain_id, res_no, res_no, alt_conf)
                accept_regularizement()
            rotate_y_scene(10, 0.3)

    if (replace_state == 0):
        set_refinement_immediate_replacement(0)

def fit_protein_stepped_refine_function(res_spec, imol_map, use_rama = False):
    
    imol     = res_spec[0]
    chain_id = res_spec[1]
    res_no   = res_spec[2]
    ins_code = res_spec[3]
    current_steps_per_frame = dragged_refinement_steps_per_frame()
    current_rama_state = refine_ramachandran_angles_state()

    # BL says: again we dont use with_auto_accept here, get's too messy
    replace_state = refinement_immediate_replacement_state()
    set_refinement_immediate_replacement(1)

    for alt_conf in residue_alt_confs(imol, chain_id, res_no, ins_code):
        if use_rama:
            set_refine_ramachandran_angles(1)
        res_name = residue_name(imol, chain_id, res_no, ins_code)
        if (not res_name == "HOH"):
            print "centering on", chain_id, res_no, "CA"
            set_go_to_atom_chain_residue_atom_name(chain_id, res_no, "CA")
            rotate_y_scene(10, 0.3) # n_frames frame_interval(degrees)
            refine_auto_range(imol, chain_id, res_no, alt_conf)
            accept_regularizement()
            rotate_y_scene(10, 0.3)    

    set_refine_ramachandran_angles(current_rama_state)
    set_dragged_refinement_steps_per_frame(current_steps_per_frame)
    
    if (replace_state == 0):
        set_refinement_immediate_replacement(0)

def fit_protein_rama_fit_function(res_spec, imol_map):
    # BL says: make it more generic
    fit_protein_stepped_refine_function(res_spec, imol_map, True)

# func is a refinement function that takes 2 args, one a residue
# spec, the other the imol_refinement_map.  e.g. fit_protein_fit_function
#
def interruptible_fit_protein(imol, func):

    import gobject
    global multi_refine_spec_list
    global continue_multi_refine
    global multi_refine_idle_proc
    global multi_refine_stop_button
    specs = fit_protein_make_specs(imol, 'all-chains')
    if specs:
        multi_refine_stop_button = coot_toolbar_button("Stop", "stop_interruptible_fit_protein()", "gtk-stop")
        multi_refine_spec_list = specs
        def idle_func():
            global multi_refine_spec_list
            global continue_multi_refine
            global multi_refine_idle_proc
            global multi_refine_stop_button
            if not multi_refine_spec_list:
                #set_visible_toolbar_multi_refine_stop_button(0)
                #set_visible_toolbar_multi_refine_continue_button(0)
                multi_refine_stop_button.destroy()
                multi_refine_stop_button = False
                return False
            if continue_multi_refine:
                imol_map = imol_refinement_map()
                func(multi_refine_spec_list[0], imol_map)
                del multi_refine_spec_list[0]
                return True
            else:
                return False
        gobject.idle_add(idle_func)
        multi_refine_idle_proc = idle_func
        set_go_to_atom_molecule(imol)

# this will stop the currently running interuptible fit protein function
#
def stop_interruptible_fit_protein():
    global continue_multi_refine
    global multi_refine_stop_button
    global multi_refine_cancel_button
    global multi_refine_continue_button
    
    continue_multi_refine = False

    multi_refine_cancel_button = coot_toolbar_button("Cancel",
                                                     "cancel_interruptible_fit_protein()",
                                                     "gtk-cancel")
    multi_refine_continue_button = coot_toolbar_button("Continue",
                                                     "continue_interruptible_fit_protein()",
                                                     "gtk-apply")
    
    multi_refine_stop_button.set_sensitive(False)
    #multi_refine_cancel_button.set_sensitive(True)
    #multi_refine_continue_button.set_sensitive(True)

# Continue with the interruptible fitting
#
def continue_interruptible_fit_protein():
    global multi_refine_stop_button
    global multi_refine_cancel_button
    global multi_refine_continue_button
    global continue_multi_refine
    global multi_refine_idle_proc

    multi_refine_stop_button.set_sensitive(True)
    multi_refine_cancel_button.destroy()
    multi_refine_cancel_button = False
    multi_refine_continue_button.destroy()
    multi_refine_continue_button = False
    continue_multi_refine = True
    gobject.idle_add(multi_refine_idle_proc)

# use to completely stop the interruptible fit function
#
def cancel_interruptible_fit_protein():
    global multi_refine_stop_button
    global multi_refine_cancel_button
    global multi_refine_continue_button
    global continue_multi_refine

    continue_multi_refine = False
    multi_refine_stop_button.destroy()
    multi_refine_stop_button = False
    multi_refine_cancel_button.destroy()
    multi_refine_cancel_button = False
    multi_refine_continue_button.destroy()
    multi_refine_continue_button = False
    

# For each residue in chain chain-id of molecule number imol, do a
# rotamer fit and real space refinement of each residue.  Don't
# update the graphics while this is happening (which makes it faster
# than fit-protein, but much less interesting to look at).
#
def fit_chain(imol,chain_id):

    make_backup(imol)
    backup_mode = backup_state(imol)
    imol_map = imol_refinement_map()
    alt_conf = ""
    replacement_state = refinement_immediate_replacement_state()

    turn_off_backup(imol)
    set_refinement_immediate_replacement(1)

    if (imol_map == -1):
       print "WARNING:: fit-chain undefined imol-map. Skipping!!"
    else:
       n_residues = chain_n_residues(chain_id,imol)
       for serial_number in range(n_residues):
           res_name = resname_from_serial_number(imol,chain_id,serial_number)
           res_no = seqnum_from_serial_number(imol,chain_id,serial_number)
           ins_code = insertion_code_from_serial_number(imol,chain_id,serial_number)
           if ins_code is not None:
               if (not res_name == "HOH"):
                   print "centering on ", chain_id, res_no, " CA"
                   set_go_to_atom_chain_residue_atom_name(chain_id,res_no,"CA")
                   auto_fit_best_rotamer(res_no,alt_conf,ins_code,chain_id,imol,imol_map,1,0.1)
                   if (imol_map >= 1):
                       refine_zone(imol,chain_id,res_no,res_no,alt_conf)
                       accept_regularizement()
                   
    if (replacement_state == 0):
          set_refinement_immediate_replacement(0)
    if (backup_mode == 1):
          turn_on_backup(imol)

# As fit_chain, but only for a residue range from resno_start to resno_end
def fit_residue_range(imol, chain_id, resno_start, resno_end):

    make_backup(imol)
    backup_mode = backup_state(imol)
    imol_map = imol_refinement_map()
    alt_conf = ""
    replacement_state = refinement_immediate_replacement_state()

    turn_off_backup(imol)
    set_refinement_immediate_replacement(1)

    if (imol_map == -1):
       print "WARNING:: fit-chain undefined imol-map. Skipping!!"
    else:
       n_residues = chain_n_residues(chain_id,imol)
       ins_code = ""
       for res_no in number_list(resno_start,resno_end):
           print "centering on ", chain_id, res_no, " CA"
           set_go_to_atom_chain_residue_atom_name(chain_id,res_no,"CA")
           auto_fit_best_rotamer(res_no,alt_conf,ins_code,chain_id,imol,imol_map,1,0.1)
           if (imol_map >= 1):
              refine_zone(imol,chain_id,res_no,res_no,alt_conf)
              accept_regularizement()

    if (replacement_state == 0):
          set_refinement_immediate_replacement(0)
    if (backup_mode == 1):
          turn_on_backup(imol)


# For each residue in the solvent chains of molecule number
# @var{imol}, do a rigid body fit of the water to the density.
#
# BL says: we pass *args where args[0]=imol and args[1]=animate_qm (if there)
def fit_waters(imol, animate_qm = False):

    print "animate?:", animate_qm
    imol_map = imol_refinement_map()
    do_animate_qm = False
    if (animate_qm):
        do_animate_qm = True

    print "do_animate?: ", do_animate_qm
 
    if (imol_map != -1):
        replacement_state = refinement_immediate_replacement_state()
        backup_mode = backup_state(imol)
        alt_conf = ""

        turn_off_backup(imol)
        set_refinement_immediate_replacement(1)
        set_go_to_atom_molecule(imol)

        # refine waters
        for chain_id in chain_ids(imol):
            if (is_solvent_chain_qm(imol, chain_id)):
                n_residues = chain_n_residues(chain_id, imol)
                print "There are %(a)i residues in chain %(b)s" % {"a":n_residues,"b":chain_id}
                for serial_number in range(n_residues):
                    res_no = seqnum_from_serial_number(imol,chain_id,serial_number)
                    if do_animate_qm:
                        res_info = residue_info(imol, chain_id, res_no, "")
                        if not res_info == []:
                            atom = res_info[0]
                            atom_name = atom[0]
                            set_go_to_atom_chain_residue_atom_name(chain_id, res_no, atom_name)
                            refine_zone(imol,chain_id,res_no,res_no,alt_conf)
                            rotate_y_scene(30, 0.6)	# n-frames frame-interval(degrees)
                    else:
                        refine_zone(imol,chain_id,res_no,res_no,alt_conf)
                    accept_regularizement()

        if (replacement_state == 0):
            set_refinement_immediate_replacement(0)
        if (backup_mode == 1):
            turn_on_backup(imol)

    else:
        add_status_bar_text("You need to define a map to fit the waters")


# BL thingy:
# as fit_waters, but will only fit a range of waters (start to end).
# speeds things up when refining a lot of waters
#
def fit_waters_range(imol, chain_id, start, end):

    imol_map = imol_refinement_map()
    if (imol_map != -1):
       replacement_state = refinement_immediate_replacement_state()
       backup_mode = backup_state(imol)
       alt_conf = ""

       turn_off_backup(imol)
       set_refinement_immediate_replacement(1)

       if (is_solvent_chain_qm(imol,chain_id)):
          serial_number = start
          while serial_number <= end:
                res_no = serial_number
                refine_zone(imol,chain_id,res_no,res_no,alt_conf)
                accept_regularizement()
                serial_number += 1

       if (replacement_state == 0):
          set_refinement_immediate_replacement(0)
       if (backup_mode == 1):
          turn_on_backup(imol)

# Step through the residues of molecule number imol and at each step
# do a residue range refinement (unlike fit-protein for example,
# which does real-space refinement for every residue).
#
# The step is set internally to 2.
#
def stepped_refine_protein(imol, res_step = 2):

    import types
    from types import IntType

    imol_map = imol_refinement_map()
    if (not valid_map_molecule_qm(imol_map)):            
        info_dialog("Oops, must set map to refine to")
    else:
        def refine_func(chain_id, res_no):
            #print "centering on ",chain_id,res_no," CA"
            set_go_to_atom_chain_residue_atom_name(chain_id, res_no, "CA")
            rotate_y_scene(30, 0.3) # n-frames frame-interval(degrees)
            refine_auto_range(imol, chain_id, res_no, "")
            accept_regularizement()
            rotate_y_scene(30,0.3)
        stepped_refine_protein_with_refine_func(imol, refine_func, res_step)


# refine each residue with ramachandran restraints
#
def stepped_refine_protein_for_rama(imol):

    imol_map = imol_refinement_map()
    if (not valid_map_molecule_qm(imol_map)):
        info_dialog("Oops, must set map to refine to")
    else:
        current_steps_frame = dragged_refinement_steps_per_frame()
        current_rama_state  = refine_ramachandran_angles_state()
        def refine_func(chain_id, res_no):
            set_go_to_atom_chain_residue_atom_name(chain_id, res_no, "CA")
            refine_auto_range(imol, chain_id, res_no, "")
            accept_regularizement()

        set_dragged_refinement_steps_per_frame(200)
        set_refine_ramachandran_angles(1)
        stepped_refine_protein_with_refine_func(imol, refine_func, 1)
        set_refine_ramachandran_angles(current_rama_state)
        set_dragged_refinement_steps_per_frame(current_steps_frame)

#
def stepped_refine_protein_with_refine_func(imol, refine_func, res_step):

    set_go_to_atom_molecule(imol)
    make_backup(imol)
    backup_mode = backup_state(imol)
    alt_conf = ""
    imol_map = imol_refinement_map()
    replacement_state = refinement_immediate_replacement_state()

    if (imol_map == -1):
        # actually shouldnt happen as we set check the map earlier...
        add_status_bar_text("Oops.  Must set a map to fit")
    else:
        turn_off_backup(imol)
        set_refinement_immediate_replacement(1)
        res_step = int(res_step)
        if (res_step <= 1):
            set_refine_auto_range_step(1)
            res_step = 1
        else:
            set_refine_auto_range_step(int(res_step / 2))

        for chain_id in chain_ids(imol):
            n_residues = chain_n_residues(chain_id,imol)
            print "There are %(a)i residues in chain %(b)s" % {"a":n_residues,"b":chain_id}
            
            for serial_number in range(0, n_residues, res_step):
                res_name = resname_from_serial_number(imol, chain_id, serial_number)
                res_no = seqnum_from_serial_number(imol, chain_id, serial_number)
                ins_code = insertion_code_from_serial_number(imol, chain_id, serial_number)
                if ins_code is not None:
                    print "centering on ", chain_id, res_no, " CA"
                    refine_func(chain_id, res_no)
                
        if (replacement_state == 0):
            set_refinement_immediate_replacement(0)
        if (backup_mode == 1):
            turn_on_backup(imol)


# The gui that you see after ligand finding. 
# 
def post_ligand_fit_gui():

     def test(imol):
       if not is_valid_model_molecule(imol):
          return False
       else:
          name = molecule_name(imol)
          if "Fitted ligand" in name:
             list = [name]
             # BL says: I think there must be a cleverer way!
             for i in molecule_centre(imol):
                list.append(i)
             return list
          else:
             return False 
     molecules_matching_criteria(lambda imol: test(imol))

# test_func is a function given one argument (a molecule number) that
# returns either False if the condition is not satisfied or something
# else if it is.  And that "something else" can be a list like
# [label, x, y, z]
# or 
# ["Bad Chiral", 0, "A", 23, "", "CA", "A"]
# 
# It is used in the create a button label and "what to do when the
# button is pressed".
# 
def molecules_matching_criteria(test_func):

    try:
        import pygtk
        pygtk.require("2.0")
        import gtk, pango

        # first count the number of fitted ligands, and post this if is
        # is greater than 0.

        passed_molecules = []
        for molecule_numbers in molecule_number_list():

            if (test_func(molecule_numbers)):
               passed_molecules.append(molecule_numbers)

        if (len(passed_molecules) > 0):
        # ok proceed

           def delete_event(*args):
               window.destroy()
               return False

           def centre_on_mol(*args):
               s = "Centred on" + name
               add_status_bar_text(s)
               centre = molecule_centre(imol)
               set_rotation_centre(*centre)

           window = gtk.Window(gtk.WINDOW_TOPLEVEL)
           scrolled_win = gtk.ScrolledWindow()
           outside_vbox = gtk.VBox(False,2)
           inside_vbox = gtk.VBox(False,0)

           window.set_default_size(200,140)
           window.set_title("Fitted Ligands")
           inside_vbox.set_border_width(2)

           window.add(outside_vbox)
           outside_vbox.pack_start(scrolled_win, True, True, 0) # expand fill padding

           scrolled_win.add_with_viewport(inside_vbox)
           scrolled_win.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)

           # (format #t "debug:: passed-molecules ~s~%" passed-molecules)
           # ((("Fitted ligand #9" 68.4 11.9 4.6) 21)
           #  (("Fitted ligand #8" 68.3 12.8 8.1) 20))

           for molecule_numbers in passed_molecules:
             imol = molecule_numbers
             name = molecule_name(imol)
             button = gtk.Button(str(name))
             inside_vbox.pack_start(button, False, False, 1)
             button.connect("clicked",centre_on_mol,imol,name)

           outside_vbox.set_border_width(6)
           ok_button = gtk.Button("OK")
           outside_vbox.pack_start(ok_button,False,False,0)
           ok_button.connect_object("clicked",delete_event,window)
           window.connect("delete_event", delete_event)

           window.show_all()

        else:
           # no matching molecules
           add_status_bar_text("No matching molecules!")

    except:
        print "BL WARNING:: no pygtk2. This function doesnt work!"

# This totally ignores insertion codes.  A clever algorithm would
# need a re-write, I think.  Well, we'd have at this end a function
# that took a chain_id res_no_1 ins_code_1 res_no_2 ins_code_2 
# 
# And refine-zone would need to be re-written too, of course.  So
# let's save that for a rainy day (days... (weeks)).
# 
# BL says:: this seems to ignore backup!
def refine_active_residue_generic(side_residue_offset):

    active_atom = active_residue()

    if not active_atom:
       print "No active atom"
    else:
       imol       = active_atom[0]
       chain_id   = active_atom[1]
       res_no     = active_atom[2]
       ins_code   = active_atom[3]
       atom_name  = active_atom[4]
       alt_conf   = active_atom[5]
    
       print "active-atom:", active_atom
       imol_map = imol_refinement_map()
       replacement_state = refinement_immediate_replacement_state()
       if imol_map == -1:
          info_dialog("Oops.  Must Select Map to fit to!")
       else:
          set_refinement_immediate_replacement(1)
          refine_zone(imol, chain_id, res_no - side_residue_offset,
                                      res_no + side_residue_offset,
                                      alt_conf)
          accept_regularizement()
       if replacement_state == 0:
          set_refinement_immediate_replacement(0)

# Function for keybinding. Speaks for itself
def refine_active_residue():
    refine_active_residue_generic(0)

# And another one
def refine_active_residue_triple():
    refine_active_residue_generic(1)


# For just one (this) residue, side-residue-offset is 0.
# 
def manual_refine_residues(side_residue_offset):

    active_atom = active_residue()

    if not active_atom:
       print "No active atom"
    else:
       imol       = active_atom[0]
       chain_id   = active_atom[1]
       res_no     = active_atom[2]
       ins_code   = active_atom[3]
       atom_name  = active_atom[4]
       alt_conf   = active_atom[5]

    imol_map = imol_refinement_map()

    if (imol_map == -1):
        info_dialog("Oops.  Must Select Map to fit to!")
    else:
        refine_zone(imol, chain_id,
                    res_no - side_residue_offset,
                    res_no + side_residue_offset,
                    alt_conf)


# Pepflip the active residue - needs a key binding
#
def pepflip_active_residue():
    active_atom = active_residue()
    if not active_atom:
       print "No active atom"
    else:
       imol       = active_atom[0]
       chain_id   = active_atom[1]
       res_no     = active_atom[2]
       ins_code   = active_atom[3]
       atom_name  = active_atom[4]
       alt_conf   = active_atom[5]

    pepflip(imol, chain_id, res_no, ins_code, alt_conf)
    

# Another cool function that needs a key binding
#
def auto_fit_rotamer_active_residue():

    active_atom = active_residue()

    if not active_atom:
       print "No active atom"
    else:
       imol       = active_atom[0]
       chain_id   = active_atom[1]
       res_no     = active_atom[2]
       ins_code   = active_atom[3]
       atom_name  = active_atom[4]
       alt_conf   = active_atom[5]
   
       print "active-atom:", active_atom
       imol_map = imol_refinement_map()
       replacement_state = refinement_immediate_replacement_state()
       if imol_map == -1:
          info_dialog("Oops.  Must Select Map to fit to!")
       else:
          auto_fit_best_rotamer(res_no, alt_conf, ins_code, chain_id, imol, imol_map, 1, 0.1)

