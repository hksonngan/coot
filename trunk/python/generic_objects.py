# generic-objects.py
#
# Copyright 2006 by Paul Emsley, The University of York
# Copyright 2006 by Bernhard Lohkamp
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

global probe_command
global reduce_command
global reduce_molecule_updates_current
global interactive_probe_is_OK_qm

# gets set the first time we run interactive_probe.  Takes values
# 'unset' (initial value) 'yes' and 'no')
#
interactive_probe_is_OK_qm = 'unset'

# set reduce  and probe command full path here if you wish
# (or have a look in group_settings!! These overwrite whatever is here), e.g.
# reduce_command = "/home/bernhard/bin/reduce"

# BL says:: just to be consistent with Paul's names
# map to scheme names:
# deftexi generic_object_is_displayed_qm
generic_object_is_displayed_qm = generic_object_is_displayed_p

# map to scheme names:
# deftexi is_closed_generic_object_qm 
is_closed_generic_object_qm = is_closed_generic_object_p

# return a new generic object number for the given object obj-name.
# If there is no such object with name obj-name, then create a new
# one.  (Maybe you want the generic object to be cleaned if it exists
# before returning, this function does not do that).
# 
def generic_object_with_name(obj_name):

  t = generic_object_index(obj_name)
  if (t == -1):
    t = new_generic_object_number(obj_name)
  return t


# display a GUI for generic objects
#
def generic_objects_gui():

    pygtk_flag = False
    try:
      import pygtk
      pygtk.require("2.0")
      import gtk, pango
      pygtk_flag = True
    except:
      print "BL WARNING:: no pygtk2. Function wont work!!!"
	  
    if (pygtk_flag and using_gui()):
      # Now we run the gui
      def delete_event(*args):
	# BL says: first we shall close the generic objects
	# or not
	#for generic_object_number in range(n_objects):
	#	set_display_generic_object(generic_object_number, 0)
	gen_window.destroy()
	return False


      def check_button_callback(widget, generic_object_number):
	button_state = widget.get_active()
	object_state = generic_object_is_displayed_qm(generic_object_number)
	if ((button_state == True) and (object_state == 0)):
	  set_display_generic_object(generic_object_number,1)
	if ((button_state == False) and (object_state == 1)):
	  set_display_generic_object(generic_object_number,0)

      def all_check_button_callback(widget):
	show_all = widget.get_active()
	for check_button in open_generic_objects:
	  if (show_all):
	    check_button.set_active(True)
	  else:
	    check_button.set_active(False)

      n_objects = number_of_generic_objects()
	
      if (n_objects > 0):
	gen_window = gtk.Window(gtk.WINDOW_TOPLEVEL)
	gen_window.set_title("Generic objects")
	vbox = gtk.VBox(False, 0)
	open_generic_objects = []
	no_active_generic_objects = 0

	for generic_object_number in range(n_objects):

	  print "INFO:: generic object attributes: ", \
		generic_object_number, \
		generic_object_name(generic_object_number), \
		is_closed_generic_object_qm(generic_object_number)

	  if (is_closed_generic_object_qm(generic_object_number) == 0):
	    name = generic_object_name(generic_object_number)

	    if (name):
	      label = str(generic_object_number) + "  " + name
	      frame = gtk.Frame(label=None)
	      check_button = gtk.CheckButton(label)

		# this callback gets called by the
		# gtk-toggle-button-set-active just below,
		# which is why we need the state and active
		# test.
	      check_button.connect("toggled", 
		     check_button_callback, generic_object_number)
	      current_state = generic_object_is_displayed_qm(generic_object_number)
	      if (current_state == 1):
		check_button.set_active(True)
		no_active_generic_objects += 1

	      vbox.add(frame)
	      frame.add(check_button)
	      frame.show()
	      check_button.show()
	      open_generic_objects.append(check_button)

	if (len(open_generic_objects) > 1):
	  hsep = gtk.HSeparator()
	  label = "Show/hide all"
	  frame = gtk.Frame(label=None)
	  check_button = gtk.CheckButton(label)
	  check_button.connect("toggled", all_check_button_callback)
	  if (len(open_generic_objects) == no_active_generic_objects):
	    check_button.set_active(True)
	  vbox.add(hsep)
	  hsep.show()
	  vbox.add(frame)
	  frame.add(check_button)
	  frame.show()
	  check_button.show()

	gen_window.connect("delete_event", delete_event)

	vbox.show()
	gen_window.add(vbox)
	gen_window.set_border_width(10)
	gen_window.show()


reduce_molecule_updates_current = False
       
# run molprobity (well reduce and probe) to make generic objects (and
# display the generic objects gui)
#
def probe(imol):
   import os
   global reduce_command, probe_command
    
   if is_valid_model_molecule(imol):

      if not (os.path.isfile(reduce_command)):
	 reduce_command = find_exe("reduce", "PATH")
      # we need to check if probe_command is defined too
      if not(os.path.isfile(probe_command)):
	 probe_command = find_exe("probe", "PATH")
      make_directory_maybe("coot-molprobity")
      mol_pdb_file = "coot-molprobity/for-reduce.pdb"
      reduce_out_pdb_file = "coot-molprobity/reduced.pdb"
      write_pdb_file(imol,mol_pdb_file)
      if (reduce_command):

	 # BL says: I think we should set REDUCE_HET_DICT
	 # so let's set REDUCE_HET_DICT if not set already!
	 if (not os.getenv('REDUCE_HET_DICT')):
	    # we assume the dic is in same dir as reduce command
	    dir, tmp = os.path.split(reduce_command)
	    connection_file = os.path.join(dir, 'reduce_wwPDB_het_dict.txt')
	    if (os.path.isfile(connection_file)):
	       os.environ['REDUCE_HET_DICT'] = connection_file
	    else:
	       print "BL WARNING:: could neither find nor set REDUCE_HET_DICT !"
	 else:
	    # REDUCE_HET_DICT is defined
	    pass

	 print "BL INFO:: run reduce as ", reduce_command + " " + mol_pdb_file + " > " + reduce_out_pdb_file

	 reduce_status = popen_command(reduce_command,
#					["-build", mol_pdb_file],
					["-build", "-oldpdb", mol_pdb_file],
					[],
					reduce_out_pdb_file)
	 if (reduce_status):
	    print "BL WARNING:: reduce didnt run ok, so stop here!"
	 else:
	    if (probe_command):
	       probe_name_stub = strip_extension(strip_path(molecule_name(imol)))
	       probe_pdb_in = "coot-molprobity/" + probe_name_stub + "-with-H.pdb"
	       probe_out = "coot-molprobity/probe-dots.out"
	       
	       prepare_file_for_probe(reduce_out_pdb_file, probe_pdb_in)
	       
	       probe_status = popen_command(probe_command,
					    ["-u", "-stdbonds", "-mc", "ALL", probe_pdb_in],
					    [],
					    probe_out)

	       if (probe_status):
		  print "BL WARNING:: probe failed, cannot continue!"
	       else:
		  # by default, we don't want to click on the
		  # imol-probe molecule (I think :-)
		  recentre_status = recentre_on_read_pdb()
		  novalue = set_recentre_on_read_pdb(0)
		  if (reduce_molecule_updates_current):
		     print "======= update molecule ======="
		     imol_probe = clear_and_update_model_molecule_from_file(imol, probe_pdb_in)
		  else:
		     print "======= read new pdb file ======="
		     imol_probe = read_pdb(probe_pdb_in)

		  if recentre_status == 1:
		     set_recentre_on_read_pdb(1)

                  # show the GUI for USER MODS
                  if using_gui():
                    user_mods_gui(imol_probe, reduce_out_pdb_file)
                    
		  # toggle_active_mol(imol_probe) let's not do
		  # that actually.  I no longer think that the
		  # new probe molecule should not be clickable
		  # when it is initally displayed (that plus
		  # there is some active/displayed logic problem
		  # for the molecules, which means that after
		  # several probes, the wrong molecule is
		  # getting refined).

		  handle_read_draw_probe_dots_unformatted(probe_out, imol_probe, 2)
		  generic_objects_gui()
		  graphics_draw()
	    else:
	       # couldnt find probe
	       print "BL WARNING:: Could not locate the program probe!! Please check if installed!"
      else:
	 # couldnt find reduce
	 print "BL WARNING:: Could not locate the program reduce!! Please check if installed!"

# Prepare file for probe, i.e. remove 'USER' from file
def prepare_file_for_probe(file_in, file_out):
    
    try:
      fin = open(file_in,'r')
    except IOError:
      print "BL WARNING:: Cannot read ", file_in
    try:
      fout = file(file_out,'w')
    except IOError:
      print "BL WARNING:: Cannot write ", file_out
    if (fin and fout):
       lines = fin.readlines()
       for line in lines:
           if (not "USER" in line):
               fout.write(line)
       fin.close()
       fout.close()

 
# run "probe" interactively, 
# which in the current implementation, means that this function 
# can run during a edit-chi angles manipulation, or after
# a real space refine zone.
# 
# Thus function presumes that there are 2 pdb files in the current 
# directory, one of which is the reference pdb file and the other
# is a pdb file containing the tmp/moving atom set.
# 
# The function takes arguments for the centre of the probe dots
# and the radius of the probe dots sphere.  The chain id and 
# residue number are also needed to pass as arguments to probe.
#
def interactive_probe(x_cen, y_cen, z_cen, radius, chain_id, res_no):

    import os, string
    global probe_command
    global interactive_probe_is_OK_qm

    probe_pdb_in_1 = "molprobity-tmp-reference-file.pdb"
    probe_pdb_in_2 = "molprobity-tmp-moving-file.pdb"
    probe_out = "coot-molprobity/molprobity-tmp-probe-dots.out"
    chain_str = ""
    if (not chain_id == ""):
      chain_str = "chain"
    atom_sel = "(file1 within " + str(radius) + " of " \
	       + str(x_cen) + ", " \
	       + str(y_cen) + ", " \
	       + str(z_cen) + ", " \
	       + "not water not (" + chain_str + string.lower(chain_id) + " " \
	       + str(res_no) + ")),file2"

    print "probe command", probe_command, \
          ["-mc", "-u", "-quiet", "-drop", "-stdbonds", "-both",
           atom_sel, "file2", probe_pdb_in_1, probe_pdb_in_2]

    # if unset, then set it.
    if (interactive_probe_is_OK_qm == 'unset'):
      if (command_in_path_qm(probe_command)):
        interactive_probe_is_OK_qm = 'yes'
      else:
        interactive_probe_is_OK_qm = 'no'
        
    if (interactive_probe_is_OK_qm == 'yes'):
       status = popen_command(probe_command,
			      ["-mc", "-u", "-quiet", "-drop", "-stdbonds", "-both",
			       atom_sel, "file2",
			       probe_pdb_in_1, probe_pdb_in_2],
			      [],
			      probe_out)

       # don't show the gui, so the imol is not needed/dummy.
       handle_read_draw_probe_dots_unformatted(probe_out, 0, 0)
       graphics_draw()

#
#
def get_probe_dots_from(pdb_file_name, point, radius):

  global probe_command
  global interactive_probe_is_OK_qm
  import os
  # if unset, then set it, try to make dir too
  if (interactive_probe_is_OK_qm == 'unset'):
    if (not command_in_path_qm(probe_command)):
      interactive_probe_is_OK_qm = 'no'
    else:
      status = make_directory_maybe("coot-molprobity")
      if (status == 0):    # 0 is good for mkdir()
        interactive_probe_is_OK_qm = 'yes'
      else:
        interactive_probe_is_OK_qm = 'no'

  if (interactive_probe_is_OK_qm == 'yes'):
    probe_out = os.path.join("coot-molprobity", "molprobity-tmp-probe-dots.out")
    within_str = "(within " + \
                 str(radius) + \
                 " of " + \
                 str(point[0]) + \
                 ", " + \
                 str(point[1]) + \
                 ", " + \
                 str(point[2]) + \
                 ")"
    args = ["-mc", "-u", "-quiet", "-drop", "-stdbonds",
            "ALL",  # whole residues from sphere selection
                    # was needed to make this work
            # within_str  # problems with atom selection
            pdb_file_name]
    print "popen_comand on", probe_command, args
    popen_command(probe_command, args, [], probe_out, False)
    handle_read_draw_probe_dots_unformatted(probe_out, 0, 0)
    graphics_draw()

#
def probe_local_sphere(imol, radius):

  # We need to select more atoms than we probe because if the atom
  # selection radius and the probe radius are the same, then
  # sometimes the middle atom of a bonded angle set is missing
  # (outside the sphere) and that leads to bad clashes.

  pt = rotation_centre()
  imol_new = new_molecule_by_sphere_selection(imol, pt[0], pt[1], pt[2],
                                              radius, 0)
  set_mol_displayed(imol_new, 0)
  set_mol_active   (imol_new, 0)
  pdb_name = "molprobity-tmp-reference-file.pdb"
  make_directory_maybe("coot-molprobity")
  write_pdb_file_for_molprobity(imol_new, pdb_name)

  get_probe_dots_from(pdb_name, pt, radius)

# add in the conn files by concatting.
#
def write_pdb_file_for_molprobity(imol, pdb_name):

  tmp_pdb_name = file_name_sans_extension(pdb_name) + "-tmp.pdb"

  write_pdb_file(imol, tmp_pdb_name)

  # Let's add on the connectivity cards of the residues that
  # molprobity doesn't know about (which I presume are all
  # non-standard residues).  Cut of (filter) files that didn't
  # write properly.
  #
  conn_file_names = []
  for res_name in non_standard_residue_names(imol):
    f_name = os.path.join("coot-molprobity", "conn-" + res_name + ".txt")
    status = write_connectivity(res_name, f_name)
    if (status == 1):
      con_file_names.append(f_name)

  # now, add (append) each of the con-file-names to the end of
  # pdb-name
  if (not os.path.isfile(tmp_pdb_name)):
    print "ERROR:: tmp file name not found", tmp_pdb_name
  else:
    conn_file_names.insert(0, tmp_pdb_name)
    fn_all = open(pdb_name, 'w')
    for conn_file in conn_file_names:
      if (os.path.isfile(conn_file)):
        cf = open(conn_file, 'r')
        data = cf.read()
        cf.close()
        fn_all.write(data)
    fn_all.close()
      
