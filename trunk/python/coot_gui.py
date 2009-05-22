# coot-gui.py
#
# Copyright 2001 by Neil Jerram
# Copyright 2002-2008, 2009 by The University of York
# Copyright 2007 by Paul Emsley
# Copyright 2006, 2007, 2008, 2009 by Bernhard Lohkamp
# with adaptions from
#   Interactive Python-GTK Console
#   Copyright (C), 1998 James Henstridge <james@daa.com.au>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or (at
# your option) any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc.,  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


# BL adaption of coot-gui scripting window running with pygtk
global do_function

# Fire up the coot scripting gui.  This function is called from the
# main C++ code of coot.  Not much use if you don't have a gui to
# type functions in to start with.
#

import pygtk, gtk, pango
try:
   import gobject
except:
   print "WARNING:: no gobject available"

import types
from types import *

global histpos
global history
histpos = 0
history = ['']

# Fire up the coot scripting gui.  This function is called from the
# main C++ code of coot.  Not much use if you don't have a gui to
# type functions in to start with.
#
def coot_gui():
   
   global coot_listener_socket
   import sys, string
   import re

   have_socket = False
   if (coot_listener_socket):
      have_socket = True
   
   def delete_event(*args):
       window.destroy()
       if (have_socket):
          gtk.main_quit()
       return False

   def hist_func(widget, event, entry, textbuffer, text):
       if event.keyval in (gtk.keysyms.KP_Up, gtk.keysyms.Up):
          entry.emit_stop_by_name("key_press_event")
          historyUp()
          gobject.idle_add(entry.grab_focus)
       elif event.keyval in (gtk.keysyms.KP_Down, gtk.keysyms.Down):
          entry.emit_stop_by_name("key_press_event")
          historyDown()
          gobject.idle_add(entry.grab_focus)
       else:
          pass

   def historyUp():
       global histpos
       if histpos > 0:
          l = entry.get_text()
          if len(l) > 0 and l[0] == '\n': l = l[1:]
          if len(l) > 0 and l[-1] == '\n': l = l[:-1]
          history[histpos] = l
          histpos = histpos - 1
          entry.set_text(history[histpos])
#       print "BL DEBUG:: hist and pos", history, histpos
                        
   def historyDown():
       global histpos
       if histpos < len(history) - 1:
          l = entry.get_text()
          if len(l) > 0 and l[0] == '\n': l = l[1:]
          if len(l) > 0 and l[-1] == '\n': l = l[:-1]
          history[histpos] = l
          histpos = histpos + 1
          entry.set_text(history[histpos])
#       print "BL DEBUG:: hist and pos", history, histpos

   def scroll_to_end():
       end = textbuffer.get_end_iter()
       textbuffer.place_cursor(end)
       text.scroll_mark_onscreen(textbuffer.get_insert())
       return gtk.FALSE
       
   def insert_normal_text(s):
       end = textbuffer.get_end_iter()
       textbuffer.insert_with_tags(end,str(s),textbuffer.create_tag(foreground="black"))
       scroll_to_end()

   def insert_tag_text(tagg,s):
       end = textbuffer.get_end_iter()
       textbuffer.insert_with_tags(end,str(s),tagg)
       scroll_to_end()

   def insert_prompt():
       end = textbuffer.get_end_iter()
       insert_normal_text("coot >> ")
       scroll_to_end()

   def warning_event(entry, event):
      if (event.state & gtk.gdk.CONTROL_MASK):
        # BL says:: for windows we want to exclude Ctrl so that we can copy things
        # alhough not really needed since we can use middle mouse  or drag to copy
        return
      else:
       tag1 = textbuffer.create_tag(foreground="red",weight=pango.WEIGHT_BOLD)
       insert_tag_text(tag1,"Don't type here.  Type in the white entry bar\n")
       insert_prompt()
       while gtk.events_pending():
          gtk.main_iteration(False)
       return

   # BL says:: now we check if entry was guile command (by mistake?!)
   # if so, let's translate it
   #
   def test_and_translate_guile(py_func):
       # test for '(' at start
       # and if a '-' before the '(' [i.e. if we have a guile command instead of a python command in the beginning,
       # we need to allow '-' after the '(']
       if ((string.find(py_func[0:py_func.find('(')],"-") > 0) or (string.find(py_func,"(") == 0)):
          #remove ()
          tmp = string.rstrip(string.lstrip(py_func,("(")),")")
          tmp_list = string.split(tmp)
          tmp_command = string.replace(tmp_list[0],"-","_")
          tmp_list[0:1] = []
          #convert string args from list to int, float, string...
          arg = tmp_list
          for i in range(len(arg)):
              try:
                 arg[i] = int(arg[i])
              except ValueError:
                 try:
                    arg[i] = float(arg[i])
                 except ValueError:
                    # it should be a string then
                    try:
                       arg[i] = arg[i].strip('"')
                    except:
                       print "BL WARNING:: unknown type of argument!"

          python_function = tmp_command + "(*arg)"
#          print "BL DEBUG:: python func and args are", python_function, tmp_command, arg

          try:
             res = eval(python_function)
          except SyntaxError:
             exec python_function in locals()
             res = None
             return True
          except:
             return False
          if res is not None:
             print "BL DEBUG:: result is", res
             insert_normal_text(str(res) + "\n")
             return True
       else:
          print "This is not a guile command!"
          return False

   def do_function(widget, entry):
       global histpos
       entry_text = entry.get_text()
       print "BL INFO:: command input is: ", entry_text
       if (entry_text != None):
          insert_tag_text(textbuffer.create_tag(foreground="red"),
                          entry_text + "\n")
       while gtk.events_pending():
         gtk.main_iteration(False)
       his = False
       res = None
       try:
             res = eval(entry_text)
             his = True
       except SyntaxError:
         try:
             test_equal = entry_text.split("=")
             if ((len(test_equal) > 1)
                 and (test_equal[0].strip() in globals())
                 and (callable(eval(test_equal[0].strip())))):
                # we try to assign a value to a build-in function! Shouldnt!
                warning_text = "BL WARNING:: you tried to assign a value to the\nbuild-in function: " + test_equal[0].strip() + "!\nThis is not allowed!!\n"
                insert_normal_text(warning_text)
                res = False
                his = False
             else:
                exec entry_text in globals()
                res = None
                his = True
         except:
          guile_function = test_and_translate_guile(entry_text)
          if guile_function:
             print "BL INFO::  We have a guile command!"
             insert_normal_text("BL INFO:: Detected guile scripting!\n\
             You should use python commands!!\n\
             But I'm a nice guy and translated it for you, this time...!\n")
          else:
             insert_normal_text("BL WARNING:: Python syntax error!\n\
             (Or you attempted to use an invalid guile command...)\n")
             type_error, error_value = sys.exc_info()[:2]
             error = str(error_value)
             insert_normal_text("Python error:\n") 
             insert_normal_text(error + "\n")
             insert_normal_text(str(type_error) + "\n")

       except:
          if (entry_text != None):
             guile_function =  test_and_translate_guile(entry_text)
             if guile_function:
                insert_normal_text("BL INFO:: Detected guile scripting!\nYou should use python commands!!\nBut I'm a nice guy and translated it for you, this time...!\n")
             else:
                insert_normal_text("BL WARNING:: Python error!\n(Or you attempted to use an invalid guile command...)\n")
                type_error, error_value = sys.exc_info()[:2]
                error = str(error_value)
                insert_normal_text("Python error:\n") 
                insert_normal_text(error + "\n")
                insert_normal_text(str(type_error) + "\n")
          else:
             print "BL WARNING:: None input"

       if res is not None:
             print "BL INFO:: result is", res
             insert_normal_text(str(res) + "\n")

       if his:
             l = entry_text + '\n'
             if len(l) > 1 and l[0] == '\n': l = l[1:]
             histpos = len(history) - 1
             # print "pedebug", l[-1]
             # can't test l[-] because python gives us a error
             # SystemError: ../Objects/longobject.c:223: bad argument to internal function
             # when we do so (on set_monomer_restriaints()).
             # if len(l) > 0 and l[-1] == '\n':
             if len(l) > 0:
                history[histpos] = l[:-1]
             else:
                history[histpos] = l

             histpos += 1
             history.append('')
       
       entry.set_text("")
       entry_text = ""
       insert_prompt()
       

   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   window.set_title("Coot Python Scripting")
   text = gtk.TextView()
   textbuffer = text.get_buffer()
   scrolled_win = gtk.ScrolledWindow()
   entry = gtk.Entry()
   completion = gtk.EntryCompletion()
   entry.set_completion(completion)
   liststore = gtk.ListStore(gobject.TYPE_STRING)
   for i in globals():
      tmp = [i][0]
      if (not tmp[-3:len(tmp)] == '_py'):
          liststore.append([i])
   completion.set_model(liststore)
   completion.set_text_column(0)
#   completion.connect("match-selected", match_cb)
   close_button = gtk.Button("  Close  ")
   vbox = gtk.VBox(False, 0)
   hbox = gtk.HBox(False, 0)
   label = gtk.Label("Command: ")
   ifont = gtk.gdk.Font("fixed")
   window.set_default_size(400,250)
   window.add(vbox)
   vbox.set_border_width(5)

   hbox.pack_start(label, False, False, 0)   # expand fill padding ???
   hbox.pack_start(entry, True, True, 0)
   vbox.pack_start(hbox, False, False, 5)
   scrolled_win.add(text)
   vbox.add(scrolled_win)
   scrolled_win.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)
   vbox.pack_end(close_button, False, False, 5)

   entry.set_text("")
   entry.set_editable(True)
#   entry.connect("activate", do_function, entry, textbuffer, text)
   entry.connect("activate", do_function, entry)
#bl testing
   entry.connect("key_press_event", hist_func, entry, textbuffer, text)
   window.connect("delete_event", delete_event)
   close_button.connect("clicked", delete_event)

   text.set_editable(False)
   text.set_cursor_visible(False)
   text.connect("key_press_event", warning_event)
   end = textbuffer.get_end_iter()
   tag = textbuffer.create_tag(foreground="black")
   textbuffer.insert_with_tags(end,"coot >> ",tag)
#   textbuffer.set_text("coot >> ")

   text.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse("#bfe6bf"))
   window.show_all()
   # run in own gtk loop only when using sockets!
   if (have_socket):
      gtk.main()


# The callback from pressing the Go button in the smiles widget, an
# interface to run libcheck.
#
def handle_smiles_go(tlc_entry, smiles_entry):

    import os, stat, shutil

    tlc_text = tlc_entry.get_text()
    smiles_text = smiles_entry.get_text()

    if (len(smiles_text) > 0):
       if ((len(tlc_text) > 0) and (len(tlc_text) < 4)): three_letter_code = tlc_text
       # next should actually not happen since I restrict the input to 3 letters!
       elif (len(tlc_text) > 0): three_letter_code = tlc_text[0:3]
       else: three_letter_code = "DUM"
          
       # ok let's run libcheck
       smiles_file = "coot-" + three_letter_code + ".smi"
       libcheck_data_lines = ["N","MON " + three_letter_code, "FILE_SMILE " + smiles_file,""]
       log_file_name = "libcheck-" + three_letter_code + ".log"
       pdb_file_name = "libcheck_" + three_letter_code + ".pdb"
       cif_file_name = "libcheck_" + three_letter_code + ".cif"
#       print "BL DEBUG:: all others: ", libcheck_data_lines, log_file_name, pdb_file_name ,cif_file_name
       #write smiles strings to a file
       smiles_input = file(smiles_file,'w')
       smiles_input.write(smiles_text)
       smiles_input.close()
       #now let's run libcheck (based on libcheck.py)
       libcheck_exe_file = find_exe(libcheck_exe, "CCP4_BIN", "PATH")

       if (libcheck_exe_file):
          status = popen_command(libcheck_exe_file, [], libcheck_data_lines, log_file_name, True)
          if (status == 0):
             if (os.path.isfile("libcheck.lib")):
                if (os.path.isfile(cif_file_name)):
                   # if we have cif_file_name already we cant move to it
                   # and I dont want to overwrite it, so we make a backup
                   try:
                      os.rename(cif_file_name, cif_file_name + ".bak")
                   except OSError:
                      # bak file exists, so let's remove and overwrite it
                      os.remove(cif_file_name + ".bak")
                      os.rename(cif_file_name,cif_file_name + ".bak")
                      print "BL INFO:: overwriting %s since same three letter code used again..." %(cif_file_name + ".bak")
                   except:
                      print "BL WARNING:: %s exists and we cant remove/overwrite it!" %(cif_file_name + ".bak")
                shutil.copy("libcheck.lib", cif_file_name)
                print "BL INFO:: renamed %s to %s " %("libcheck.lib", cif_file_name)
             sc = rotation_centre()
             print "BL INFO:: reading ",pdb_file_name
             imol = handle_read_draw_molecule_with_recentre(pdb_file_name,0)
             if (is_valid_model_molecule(imol)):
                mc = molecule_centre(imol)
                sc_mc = [sc[i]-mc[i] for i in range(len(mc))]
                translate_molecule_by(imol,*sc_mc)
             read_cif_dictionary(cif_file_name)
          else: print "BL WARNING:: libcheck didnt run ok!"
       else: print " BL WARNING:: libcheck not found!"
    else:
       print "BL WARNING:: Wrong input (no smiles text)! Can't continue!"

# smiles GUI
#
def smiles_gui():

   def smiles_gui_internal():
      def delete_event(*args):
         smiles_window.destroy()
         return False
   
      def go_button_pressed(widget, tlc_entry, smiles_entry,smiles_window):
         handle_smiles_go(tlc_entry,smiles_entry)
         smiles_window.destroy()
         delete_event()

      def smiles_connect(widget, event, tlc_entry, smiles_entry, smiles_window):
         if (event.keyval == 65293):
            handle_smiles_go(tlc_entry,smiles_entry)
            smiles_window.destroy()

      smiles_window = gtk.Window(gtk.WINDOW_TOPLEVEL)
      smiles_window.set_title("SMILES GUI")
      vbox = gtk.VBox(False, 0)
      hbox1 = gtk.HBox(False, 0)
      hbox2 = gtk.HBox(False, 0)
      tlc_label = gtk.Label("  3-letter code ")
      tlc_entry = gtk.Entry(max=3)
      tlc_entry.set_text("")
      smiles_label = gtk.Label("SMILES string ")
      smiles_entry = gtk.Entry()
      text = gtk.Label("  [SMILES interface works by using CCP4's LIBCHECK]  ")
      go_button = gtk.Button("  Go  ")
      vbox.pack_start(hbox1, False, False, 0)
      vbox.pack_start(hbox2, False, False, 4)
      vbox.pack_start(text, False, False, 2)
      vbox.pack_start(go_button, False, False, 6)
      hbox1.pack_start(tlc_label, False, False, 0)
      hbox1.pack_start(tlc_entry, False, False, 0)
      hbox2.pack_start(smiles_label, False, False, 0)
      hbox2.pack_start(smiles_entry, True, True, 4)
      smiles_window.add(vbox)
      vbox.set_border_width(6)

      smiles_entry.connect("key-press-event", smiles_connect, tlc_entry, smiles_entry, smiles_window)
      go_button.connect("clicked", go_button_pressed,tlc_entry,smiles_entry,smiles_window)
      smiles_window.connect("delete_event",delete_event)

      smiles_window.show_all()
   # first check that libcheck is available... if not put up and info
   # dialog.
   if (find_exe(libcheck_exe, "CCP4_BIN", "PATH")):
      smiles_gui_internal()
   else:
      info_dialog("You need to setup CCP4 (specifically LIBCHECK) first.")
      

# Generic single entry widget
# 
# Pass the hint labels of the entries and a function that gets called
# when user hits "Go".  The handle-go-function accepts one argument
# that is the entry text when the go button is pressed.
#
def generic_single_entry(function_label, entry_1_default_text, go_button_label, handle_go_function):

    def delete_event(*args):
       window.destroy()
       return False

    def go_function_event(widget, smiles_entry, *args):
        handle_go_function(smiles_entry.get_text())
        delete_event()

    def key_press_event(widget, event, smiles_entry, *args):
        if (event.keyval == 65293):
           handle_go_function(smiles_entry.get_text())
           delete_event()

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    vbox = gtk.VBox(False, 0)
    hbox1 = gtk.HBox(False, 0)
    hbox2 = gtk.HBox(False, 0)
    hbox3 = gtk.HBox(False, 0)
    function_label = gtk.Label(function_label)
    smiles_entry = gtk.Entry()
    cancel_button = gtk.Button("  Cancel  ")
    go_button = gtk.Button(go_button_label)

    vbox.pack_start(hbox1, False, False, 0)
    vbox.pack_start(hbox2, False, False, 4)
    vbox.pack_start(hbox3, False, False, 4)
    hbox3.pack_start(go_button, True, True, 6)
    hbox3.pack_start(cancel_button, True, True, 6)
    hbox1.pack_start(function_label, False, False, 0)
    hbox2.pack_start(smiles_entry, False, False, 0)
    window.add(vbox)
    vbox.set_border_width(6)
 
    if isinstance(entry_1_default_text,types.StringTypes):
       smiles_entry.set_text(entry_1_default_text)
    else:
       print "BL WARNING:: entry_1_default_text was no string!!"
   
    cancel_button.connect("clicked", delete_event)

    go_button.connect("clicked", go_function_event, smiles_entry)

    smiles_entry.connect("key_press_event", key_press_event, smiles_entry)

    window.show_all()

# generic double entry widget, now with a check button
# ...and returns teh widget if requested
# 
# pass a the hint labels of the entries and a function 
# (handle-go-function) that gets called when user hits "Go" (which
# takes two string aguments and the active-state of the check button
# (either True of False).
# 
# if check-button-label not a string, then we don't display (or
# create, even) the check-button.  If it *is* a string, create a
# check button and add the callback handle-check-button-function
# which takes as an argument the active-state of the the checkbutton.
#
def generic_double_entry(label_1, label_2,
                         entry_1_default_text, entry_2_default_text,
                         check_button_label, handle_check_button_function,
                         go_button_label, handle_go_function,
                         return_widget = False):

    def delete_event(*args):
       window.destroy()
       return False

    def go_function_event(*args):
	if check_button:
	        handle_go_function(tlc_entry.get_text(), smiles_entry.get_text(), check_button.get_active())
	else:
        	handle_go_function(tlc_entry.get_text(), smiles_entry.get_text())
        delete_event()

    def key_press_event(widget, event, tlc_entry, smiles_entry, check_button):
        if (event.keyval == 65293):
		if check_button:
			handle_go_function(tlc_entry.get_text(), smiles_entry.get_text(), check_button.get_active())
	        else:
			handle_go_function(tlc_entry.get_text(), smiles_entry.get_text())
           	delete_event()

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    vbox = gtk.VBox(False, 0)
    hbox1 = gtk.HBox(False, 0)
    hbox2 = gtk.HBox(False, 0)
    hbox3 = gtk.HBox(False, 0)
    tlc_label = gtk.Label(label_1)
    tlc_entry = gtk.Entry()
    smiles_label = gtk.Label(label_2)
    smiles_entry = gtk.Entry()
    h_sep = gtk.HSeparator()
    cancel_button = gtk.Button("  Cancel  ")
    go_button = gtk.Button(go_button_label)

    vbox.pack_start(hbox1, False, False, 0)
    vbox.pack_start(hbox2, False, False, 0)
    hbox3.pack_start(go_button, True, False, 6)
    hbox3.pack_start(cancel_button, True, False, 6)
    hbox1.pack_start(tlc_label, False, False, 0)
    hbox1.pack_start(tlc_entry, False, False, 0)
    hbox2.pack_start(smiles_label, False, False, 0)
    hbox2.pack_start(smiles_entry, False, False, 0)

    if type(check_button_label) is StringType:

	def check_callback(*args):
		active_state = c_button.get_active()
		handle_check_button_function(active_state)

	c_button = gtk.CheckButton(check_button_label)
	vbox.pack_start(c_button, False, False, 2)
	c_button.connect("toggled", check_callback)
	check_button = c_button
    else:
	check_button = False 	# the check-button when we don't want to see it

    vbox.pack_start(h_sep, True, False, 3)
    vbox.pack_start(hbox3, False, False, 0)
    window.add(vbox)
    vbox.set_border_width(6)

    if isinstance(entry_1_default_text,types.StringTypes):
       tlc_entry.set_text(entry_1_default_text)
    else:
       print "BL WARNING:: entry_1_default_text was no string!!"
 
    if isinstance(entry_2_default_text,types.StringTypes):
       smiles_entry.set_text(entry_2_default_text)
    else:
       print "BL WARNING:: entry_2_default_text was no string!!"

    go_button.connect("clicked", go_function_event)
    cancel_button.connect("clicked", delete_event)

    smiles_entry.connect("key-press-event", key_press_event, tlc_entry, smiles_entry, check_button)

    window.show_all()

    # return the widget
    if (return_widget):
       return window

# generic double entry widget, now with a check button
#
# OLD
#
# 
# pass a the hint labels of the entries and a function 
# (handle-go-function) that gets called when user hits "Go" (which
# takes two string aguments and the active-state of the check button
# (either True of False).
# 
# if check-button-label not a string, then we don't display (or
# create, even) the check-button.  If it *is* a string, create a
# check button and add the callback handle-check-button-function
# which takes as an argument the active-state of the the checkbutton.
#
def generic_multiple_entries_with_check_button(entry_info_list, check_button_info, go_button_label, handle_go_function):

    def delete_event(*args):
       window.destroy()
       return False

    def go_function_event(*args):
       print "Here.................. check-button is ", check_button
       if check_button:
          handle_go_function(map(lambda entry: entry.get_text(), entries), check_button.get_active())
       else:
          handle_go_function(map(lambda entry: entry.get_text(), entries))
       delete_event()

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    vbox = gtk.VBox(False, 0)
    hbox3 = gtk.HBox(False, 0)
    h_sep = gtk.HSeparator()
    cancel_button = gtk.Button("  Cancel  ")
    go_button = gtk.Button(go_button_label)

    # all the labelled entries
    #
    entries = []
    for entry_info in entry_info_list:

       entry_1_hint_text = entry_info[0]
       entry_1_default_text = entry_info[1]
       hbox1 = gtk.HBox(False, 0)

       label = gtk.Label(entry_1_hint_text)
       entry = gtk.Entry()
       entries.append(entry)

       if type(entry_1_default_text) is StringType:
          entry.set_text(entry_1_default_text)

       hbox1.pack_start(label, False, 0)
       hbox1.pack_start(entry, False, 0)
       vbox.pack_start(hbox1, False, False, 0)

    print "debug:: check-button-info: ", check_button_info
    if not (type(entry_info_list) is ListType and len(check_button_info) == 2):
       print "check_button_info failed list and length test"
       check_button = False
    else:
       if type(check_button_info[0]) is StringType:

          def check_callback(*args):
		active_state = c_button.get_active()
		check_button_info[1] = active_state
                
          c_button = gtk.CheckButton(check_button_info[0])
          vbox.pack_start(c_button, False, False, 2)
          c_button.connect("toggled", check_callback)
          check_button = c_button
       else:
          check_button = False      # the check-button when we don't want to see it

    print "Here check button creation.................. check-button is ", check_button
    vbox.pack_start(h_sep, True, False, 3)
    vbox.pack_start(hbox3, False, False, 0)
    window.add(vbox)
    vbox.set_border_width(6)

    hbox3.pack_start(go_button, True, False, 6)
    hbox3.pack_start(cancel_button, True, False, 6)


    cancel_button.connect("clicked", delete_event)
    go_button.connect("clicked", go_function_event)

    window.show_all()

# A demo gui to move about to molecules
def molecule_centres_gui():

    def delete_event(*args):
        window.destroy()
        return False

    def callback_func(widget,molecule_number,label):
        s = "Centred on " + label
        add_status_bar_text(s)
        set_rotation_centre(*molecule_centre(molecule_number))

    # first, we create a window and a frame to be put into it.
    # 
    # we create a vbox (a vertical box container) that will contain the
    # buttons for each of the coordinate molecules
    # 
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    frame = gtk.Frame("Molecule Centres")
    vbox = gtk.VBox(False, 3)
    
    # add the frame to the window and the vbox to the frame
    # 
    window.add(frame)
    frame.add(vbox)
    vbox.set_border_width(6)

    # for each molecule, test if this is a molecule that has a
    # molecule (it might have been closed or contain a map).  The we
    # construct a button label that is the molecule number and the
    # molecule name and create a button with that label.
    # 
    # then we attach a signal to the "clicked" event on the newly
    # created button.  In the function that subsequently happen (on a
    # click) we add a text message to the status bar and set the
    # graphics rotation centre to the centre of the molecule.  Each
    # of these buttons is packed into the vbox (the #f #f means no
    # expansion and no filling).  The padding round each button is 3
    # pixels.
    # 
    for molecule_number in molecule_number_list():
        if (is_valid_model_molecule(molecule_number)):
           name = molecule_name(molecule_number)
           label = str(molecule_number) + " " + name
           button = gtk.Button(label)
           button.connect("clicked",callback_func,molecule_number,label)
           vbox.add(button)
    window.show_all()

# A BL function to analyse the libcheck log file
#
def check_libcheck_logfile(logfile_name):
    # to check if error or warning occured in libcheck
    # maybe want to have something like that for all logfiles
    # and produce a warning for users...

    fp = open(logfile_name, 'r')
    lines = fp.readlines()
    for line in lines:
        if "WARNING: no such keyword :FILE" in line:
           print "BL WARNING:: libcheck didn't seem to run ok! Please check output carefully!!"
        else: pass
    fp.close()

# old coot test
def old_coot_qm():

   import time, random
   # when making a new date, recall that guile times seem to be
   # differently formatted to python times (god knows why)
   # run e.g.: time.mktime((2008, 7, 24, 0,0,0,0,0,0))
   # 
   # new_release_time = 1199919600 # 10 Jan 2008
   # new_release_time = 1205622000 # 16 Mar 2008 0.3.3
   # new_release_time = 1216854000 # 24 Jul 2008 0.4
   # new_release_time = 1237244400 # 17 March 2009
   new_release_time = 1249945200 # 11 August 2009 : 0.5
   current_time = int(time.time())
   time_diff = current_time - new_release_time
   if (time_diff > 0):
      if (time_diff > 8600):
         s = "This is an Old Coot!\n\nIt's time to upgrade."
      else:
         if (random.randint(0,3) == 0):
            # Jorge Garcia:
            s = "(Nothing says \"patriotism\" like an Ireland shirt...)\n"
         else:
            s = "This is an Old Coot!\n\nIt's time to upgrade."
      info_dialog(s)

if (not coot_has_guile()):
   old_coot_qm()

# We can either go to a place (in which case the element is a list of
# button label (string) and 3 numbers that represent the x y z
# coordinates) or an atom (in which case the element is a list of a
# button label (string) followed by the molecule-number chain-id
# residue-number ins-code atom-name altconf)
# 
# e.g. interesting_things_gui(
#       "Bad things by Analysis X",
#       [["Bad Chiral",0,"A",23,"","CA","A"],
#        ["Bad Density Fit",0,"B",65,"","CA",""],
#        ["Interesting blob",45.6,46.7,87.5]]
# 
def interesting_things_gui(title,baddie_list):

    interesting_things_with_fix_maybe(title, baddie_list)

# In this case, each baddie can have a function at the end which is
# called when the fix button is clicked.
# And extra string arguments can give the name of the button and a tooltip
# as these two are ambigious, they can only be:
# 1 extra string:  button name
# 2 extra strings: button name and tooltip
# 
def interesting_things_with_fix_maybe(title, baddie_list):

   # does this baddie have a fix at the end?.  If yes, return the
   # func, if not, return #f.
   # BL says::
   # OBS in python the fix function is a list with the 1st element the function
   # hope this will work!?
   def baddie_had_fix_qm(baddie):
       
       l = len(baddie)
       if (l < 5):
          # certainly no fix
          return False, False, False
       else:
          func_maybe1 = baddie[l-1]
          func_maybe2 = baddie[l-2]
          func_maybe3 = baddie[l-3]

          if (isinstance(func_maybe1, types.ListType) and len(func_maybe1)>0):
             # the last one is probably a funcn (no button name)
             func_maybe_strip = func_maybe1[0]
#             print "BL DEBUG:: func_maybe_strip is", func_maybe_strip
             if (callable(func_maybe_strip)):
                return func_maybe1, False, False
             else:
                return False, False, False
             
          elif (isinstance(func_maybe2, types.ListType) and len(func_maybe2)>0
                and isinstance(func_maybe1, types.StringType)):
             # the second last is function, last is button name
             func_maybe_strip = func_maybe2[0]
             button_name = func_maybe1
             if (callable(func_maybe_strip)):
                return func_maybe2, button_name, False
             else:
                return False, False, False
             
          elif (isinstance(func_maybe3, types.ListType) and len(func_maybe3)>0
                and isinstance(func_maybe2, types.StringType)
                and isinstance(func_maybe1, types.StringType)):
             # the third last is function, second last is button name, last is tooltip
             func_maybe_strip = func_maybe3[0]
             button_name = func_maybe2
             tooltip_str = func_maybe1
             if (callable(func_maybe_strip)):
                return func_maybe3, button_name, tooltip_str
             elif (func_maybe_strip == "dummy"):
                return False, False, tooltip_str
             else:
                return False, False, False
          
             
          else:
             return False, False, False

   def fix_func_call(widget, call_func):
       func_maybe_strip = call_func[0]
       func_args = call_func[1:len(call_func)]
       func_maybe_strip(*func_args)

   def delete_event(*args):
       window.destroy()
       gtk.main_quit()
       return False

   def callback_func1(widget,coords):
       set_rotation_centre(*coords)

   def callback_func2(widget,mol_no,atom_info):
       print "Attempt to go to chain: %s resno: %s atom-name: %s" %(atom_info[0],atom_info[1],atom_info[2])
       set_go_to_atom_molecule(mol_no)
       success = set_go_to_atom_chain_residue_atom_name(*atom_info)
       if success == 0:           # failed?!
          new_name = unmangle_hydrogen_name(atom_info[2])
          success2 = set_go_to_atom_chain_residue_atom_name(atom_info[0],atom_info[1],new_name)
          if success2 == 0:
             print "Failed to centre on demangled name: ", new_name
             set_go_to_atom_chain_residue_atom_name(atom_info[0],atom_info[1]," CA ")

   # main body
   # to accomodated tooltips we need to either have a gtk.Window with gtk.main()
   # or a dialog and run() it! We use Window to not block events and hope not to
   # interfere with the gtk_main() of coot itself
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   scrolled_win = gtk.ScrolledWindow()
   outside_vbox = gtk.VBox(False,2)
   inside_vbox = gtk.VBox(False,0)

   window.set_default_size(250,250)
   window.set_title(title)
   inside_vbox.set_border_width(4)

   window.add(outside_vbox)
   outside_vbox.add(scrolled_win)
   scrolled_win.add_with_viewport(inside_vbox)
   scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)
   tooltips = gtk.Tooltips()

   for baddie_items in baddie_list:

       if baddie_items == '' :
          print 'Done'
       else:
          hbox = gtk.HBox(False,0) # hbox to contain Baddie button and
                                   # and fix it button

          label = baddie_items[0]
          button = gtk.Button(label)

          inside_vbox.pack_start(hbox,False,False,2)
          hbox.pack_start(button, True, True, 1)

          # add the a button for the fix func if it exists.  Add
          # the callback.
          fix_func, button_name, tooltip_str = baddie_had_fix_qm(baddie_items)
          if (fix_func):
             if (button_name):
                fix_button_name = button_name
             else:
                fix_button_name = "  Fix  "
             fix_button = gtk.Button(fix_button_name)
             hbox.pack_end(fix_button, False, False, 2)
             fix_button.show()
             fix_button.connect("clicked", fix_func_call, fix_func)

          if (tooltip_str):
             # we have a tooltip str
             tooltips.set_tip(button, tooltip_str)


          if (len(baddie_items) == 4):               # e.g. ["blob",1,2,3]
             # we go to a place
             coords = [baddie_items[1], baddie_items[2], baddie_items[3]]
             button.connect("clicked", callback_func1, coords)

          else:
             # we go to an atom
             mol_no = baddie_items[1]
             atom_info = [baddie_items[2], baddie_items[3], baddie_items[5]]
             button.connect("clicked", callback_func2, mol_no, atom_info)

       
   outside_vbox.set_border_width(4)
   ok_button = gtk.Button("  OK  ")
   outside_vbox.pack_start(ok_button, False, False, 6)
   ok_button.connect("clicked", delete_event)

   window.connect("destroy", delete_event)

   window.show_all()
   gtk.main()

#interesting_things_gui("Bad things by Analysis X",[["Bad Chiral",0,"A",23,"","CA","A"],["Bad Density Fit",0,"B",65,"","CA",""],["Interesting blob",45.6,46.7,87.5],["Interesting blob 2",45.6,41.7,80.5]])
#interesting_things_gui("Bad things by Analysis X",[["Bad Chiral",0,"A",23,"","CA","A",[print_sequence_chain,0,'A']],["Bad Density Fit",0,"B",65,"","CA",""],["Interesting blob",45.6,46.7,87.5],["Interesting blob 2",45.6,41.7,80.5]])


# Fill an option menu with the "right type" of molecules.  If
# filter_function returns True then add it.  Typical value of
# filter_function is valid-model-molecule_qm
#
def fill_option_menu_with_mol_options(menu, filter_function):

    mol_ls = []
    n_molecules = graphics_n_molecules()
   
    for mol_no in molecule_number_list():
        if filter_function(mol_no):
           label_str = molecule_name(mol_no)
           if (isinstance(label_str,types.StringTypes)):
              mlabel_str = str(mol_no) + " " + label_str
              menu.append_text(mlabel_str)
              menu.set_active(0)
              mol_ls.append(mol_no)
           else:
              print "OOps molecule name for molecule %s is %s" %(mol_no_ls,label_str)
    return mol_ls

# Fill an option menu with maps and return the list of maps
#
def fill_option_menu_with_map_mol_options(menu):
    return fill_option_menu_with_mol_options(menu, valid_map_molecule_qm)

# Helper function for molecule chooser.  Not really for users.
# 
# Return a list of models, corresponding to the menu items of the
# option menu.
# 
# The returned list will not contain references to map or closed
# molecules.
# 
def fill_option_menu_with_coordinates_mol_options(menu):
    return fill_option_menu_with_mol_options(menu, valid_model_molecule_qm)

#
def fill_option_menu_with_number_options(menu, number_list, default_option_value):

    for number in number_list:
       mlabel_str = str(number)
       menu.append_text(mlabel_str)
       if (default_option_value == number):
          count = number_list.index(number)
          menu.set_active(count)
          print "setting menu active ", default_option_value, count

# Helper function for molecule chooser.  Not really for users.
# 
# return the molecule number of the active item in the option menu,
# or return False if there was a problem (e.g. closed molecule)
#
# BL says:: we do it for gtk_combobox instead! option_menu is deprecated
#
def get_option_menu_active_molecule(option_menu, model_mol_list):

    model = option_menu.get_model()
    active_item = option_menu.get_active()
    # combobox has no children as such, so we just count the rows
    children = len(model)

    if (children == len(model_mol_list)):
       try:
          all_model = model[active_item][0]
          imol_model, junk = all_model.split(' ', 1)
       
          return int(imol_model)
       except:
          print "INFO:: could not get active_item"
          return False
    else:
       print "Failed children length test : ",children, model_mol_list
       return False

# BL says:: we do it for gtk_combobox instead! option_menu is deprecated
# Here we return the active item in an option menu of generic items
#
def get_option_menu_active_item(option_menu, item_list):

   model = option_menu.get_model()
   active_item = option_menu.get_active()
   # combobox has no children as such, so we just count the rows
   children = 0
   for i in model:
        children += 1

   if (children == len(item_list)):
       try:
          all_model = model[active_item][0]
          return all_model
       except:
          print "INFO:: could not get active_item"
          return False
   else:
       print "Failed children length test : ",children, item_list
       return False


def molecule_chooser_gui_generic(chooser_label, callback_function, option_menu_fill_function):
 
    def delete_event(*args):
       window.destroy()
       return False

    def on_ok_clicked(*args):
        # what is the molecule number of the option menu?
        active_mol_no = get_option_menu_active_molecule(option_menu, model_mol_list)
        try:
           active_mol_no = int(active_mol_no)
           print "INFO: operating on molecule number ", active_mol_no
           try:
              callback_function(active_mol_no)
           except:
              print "BL INFO:: problem in callback_function", callback_function.func_name
           delete_event()
        except:
           print "Failed to get a (molecule) number"

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    label = gtk.Label(chooser_label)
    vbox = gtk.VBox(False,6)
    hbox_buttons = gtk.HBox(False,5)
# BL says:: option menu is depricated, so we use combox instead, maybe!?!
    option_menu = gtk.combo_box_new_text()
    ok_button = gtk.Button("  OK  ")
    cancel_button = gtk.Button(" Cancel ")
    h_sep = gtk.HSeparator()
    model_mol_list = option_menu_fill_function(option_menu)

    window.set_default_size(370,100)
    window.add(vbox)
    vbox.pack_start(label,False,False,5)
    vbox.pack_start(option_menu,True,True,0)
    vbox.pack_start(h_sep,True,False,2)
    vbox.pack_start(hbox_buttons,False,False,5)
    hbox_buttons.pack_start(ok_button,True,False,5)
    hbox_buttons.pack_start(cancel_button,True,False,5)

    # button callbacks:
    ok_button.connect("clicked",on_ok_clicked,option_menu,model_mol_list)
    cancel_button.connect("clicked",delete_event)

    window.show_all()
#molecule_chooser_gui("test-gui",print_sequence_chain(0,"A"))

# Fire up a molecule chooser dialog, with a given label and on OK we
# call the call_back_fuction with an argument of the chosen molecule
# number. 
# 
# chooser-label is a directive to the user such as "Choose a Molecule"
# 
# callback-function is a function that takes a molecule number as an
# argument.
#
def molecule_chooser_gui(chooser_label,callback_function):
    molecule_chooser_gui_generic(chooser_label,callback_function,fill_option_menu_with_coordinates_mol_options)

# As above but for maps
#
def map_molecule_chooser_gui(chooser_label,callback_function):
    molecule_chooser_gui_generic(chooser_label,callback_function,fill_option_menu_with_map_mol_options)

# A pair of widgets, a molecule chooser and an entry.  The
# callback_function is a function that takes a molecule number and a
# text string.
#
def generic_chooser_and_entry(chooser_label,entry_hint_text,default_entry_text,callback_function):

    import operator

    def delete_event(*args):
       window.destroy()
       return False

    def on_ok_button_clicked(*args):
        # what is the molecule number of the option menu?
        active_mol_no = get_option_menu_active_molecule(option_menu,model_mol_list)

        try:
           active_mol_no = int(active_mol_no)
           print "INFO: operating on molecule number ", active_mol_no
           text = entry.get_text()
           callback_function(active_mol_no,text)
           delete_event()
        except:
           print "Failed to get a (molecule) number"

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    label = gtk.Label(chooser_label)
    vbox = gtk.VBox(False, 2)
    hbox_for_entry = gtk.HBox(False, 0)
    entry = gtk.Entry()
    entry_label = gtk.Label(entry_hint_text)
    hbox_buttons = gtk.HBox(True, 2)
    option_menu = gtk.combo_box_new_text()
    ok_button = gtk.Button("  OK  ")
    cancel_button = gtk.Button(" Cancel ")
    h_sep = gtk.HSeparator()
    model_mol_list = fill_option_menu_with_coordinates_mol_options(option_menu)
    
    window.set_default_size(400,100)
    window.add(vbox)
    vbox.pack_start(label, False, False, 5)
    vbox.pack_start(option_menu, True, True, 0)
    vbox.pack_start(hbox_for_entry, False, False, 5)
    vbox.pack_start(h_sep, True, False, 2)
    vbox.pack_start(hbox_buttons, False, False, 5)
    hbox_buttons.pack_start(ok_button, True, False, 5)
    hbox_buttons.pack_start(cancel_button, False, False, 5)
    hbox_for_entry.pack_start(entry_label, False, False, 4)
    hbox_for_entry.pack_start(entry, True, True, 4)
    entry.set_text(default_entry_text)

    # button callbacks
    ok_button.connect("clicked",on_ok_button_clicked, entry, option_menu, callback_function)
    cancel_button.connect("clicked", delete_event)

    window.show_all()

# A pair of widgets, a chooser entry and a file selector.  The
# callback_function is a function that takes a molecule number and a
# text string (e.g. chain_id and file_name)
#
# chooser_filter is typically valid_map_molecule_qm or valid_model_molecule_qm
#
def generic_chooser_entry_and_file_selector(chooser_label, chooser_filter, entry_hint_text, default_entry_text, file_selector_hint, callback_function):

    import operator

    def delete_event(*args):
       window.destroy()
       return False

    def on_ok_button_clicked(*args):
        # what is the molecule number of the option menu?
        active_mol_no = get_option_menu_active_molecule(option_menu, model_mol_list)

        try:
           active_mol_no = int(active_mol_no)
           text = entry.get_text()
           file_sel_text = file_sel_entry.get_text()
           callback_function(active_mol_no, text, file_sel_text)
           delete_event()
        except:
           print "Failed to get a (molecule) number"

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    label = gtk.Label(chooser_label)
    vbox = gtk.VBox(False, 2)
    hbox_for_entry = gtk.HBox(False, 0)
    entry = gtk.Entry()
    entry_label = gtk.Label(entry_hint_text)
    hbox_buttons = gtk.HBox(True, 2)
    option_menu = gtk.combo_box_new_text()
    ok_button = gtk.Button("  OK  ")
    cancel_button = gtk.Button(" Cancel ")
    h_sep = gtk.HSeparator()
    model_mol_list = fill_option_menu_with_mol_options(option_menu, chooser_filter)
    
    window.set_default_size(400,100)
    window.add(vbox)
    vbox.pack_start(label, False, False, 5)
    vbox.pack_start(option_menu, True, True, 0)
    vbox.pack_start(hbox_for_entry, False, False, 5)
    hbox_buttons.pack_start(ok_button, True, False, 5)
    hbox_buttons.pack_start(cancel_button, False, False, 5)
    hbox_for_entry.pack_start(entry_label, False, False, 4)
    hbox_for_entry.pack_start(entry, True, True, 4)
    entry.set_text(default_entry_text)

    file_sel_entry = file_selector_entry(vbox, file_selector_hint)
    vbox.pack_start(h_sep, True, False, 2)
    vbox.pack_start(hbox_buttons, False, False, 5)
    

    # button callbacks
    ok_button.connect("clicked",on_ok_button_clicked, entry, option_menu, callback_function)
    cancel_button.connect("clicked", delete_event)

    window.show_all()

# A pair of widgets, a molecule chooser and a file selector.  The
# callback_function is a function that takes a molecule number and a
# file_name
#
# chooser_filter is typically valid_map_molecule_qm or valid_model_molecule_qm
#
def generic_chooser_and_file_selector(chooser_label, chooser_filter, file_selector_hint, default_file_name, callback_function):

    import operator

    def delete_event(*args):
       window.destroy()
       return False

    def on_ok_button_clicked(*args):
        # what is the molecule number of the option menu?
        active_mol_no = get_option_menu_active_molecule(option_menu, model_mol_list)

        try:
           active_mol_no = int(active_mol_no)
           file_sel_text = file_sel_entry.get_text()
           callback_function(active_mol_no, file_sel_text)
           delete_event()
        except:
           print "Failed to get a (molecule) number"

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    label = gtk.Label(chooser_label)
    vbox = gtk.VBox(False, 2)
    hbox_for_entry = gtk.HBox(False, 0)
    hbox_buttons = gtk.HBox(True, 2)
    option_menu = gtk.combo_box_new_text()
    ok_button = gtk.Button("  OK  ")
    cancel_button = gtk.Button(" Cancel ")
    h_sep = gtk.HSeparator()
    model_mol_list = fill_option_menu_with_mol_options(option_menu, chooser_filter)
    
    window.set_default_size(400,100)
    window.add(vbox)
    vbox.pack_start(label, False, False, 5)
    vbox.pack_start(option_menu, True, True, 0)
    hbox_buttons.pack_start(ok_button, True, False, 5)
    hbox_buttons.pack_start(cancel_button, False, False, 5)

    file_sel_entry = file_selector_entry(vbox, file_selector_hint)
    vbox.pack_start(h_sep, True, False, 2)
    vbox.pack_start(hbox_buttons, False, False, 5)
    

    # button callbacks
    ok_button.connect("clicked",on_ok_button_clicked, option_menu, callback_function)
    cancel_button.connect("clicked", delete_event)

    window.show_all()


# If a menu with label menu-label is not found in the coot main
# menubar, then create it and return it. 
# If it does exist, simply return it.
#  
def coot_menubar_menu(menu_label):

   try:
    import coot_python
    coot_main_menubar = coot_python.main_menubar()

    def menu_bar_label_list():
      ac_lab_ls = []
      ac_lab = []
      for menu_child in coot_main_menubar.get_children():
          lab = []
          # ac-lab-ls is a GtkAccelLabel in a list
          lab.append(menu_child.get_children()[0].get_text())
          lab.append(menu_child)
          ac_lab_ls.append(lab)
#      print "BL DEBUG:: list is", ac_lab_ls, ac_lab, lab
      return ac_lab_ls

    # main body
    #
    found_menu = False
    for f in menu_bar_label_list():
       if menu_label in f: 
          #print "BL DEBUG:: found menu label is ", f
          # we shall return the submenu and not the menuitem
          found_menu = f[1].get_submenu()
    if found_menu:
       return found_menu
    else:
       menu = gtk.Menu()
       menuitem = gtk.MenuItem(menu_label)
       menuitem.set_submenu(menu)
       coot_main_menubar.append(menuitem)
       menuitem.show()
       return menu
   except: print """BL WARNING:: could not import coot_python module!!\n
                    Some things, esp. extensions, may be crippled!"""


# Given that we have a menu (e.g. one called "Extensions") provide a
# cleaner interface to adding something to it:
# 
# activate_function is a thunk.
#
def add_simple_coot_menu_menuitem(menu,menu_item_label,activate_function):

    submenu = gtk.Menu()
    sub_menuitem = gtk.MenuItem(menu_item_label)

    menu.append(sub_menuitem)
    sub_menuitem.show()

    sub_menuitem.connect("activate",activate_function)


# Make an interesting things GUI for residues of molecule number
# imol that have alternate conformations.
#
def alt_confs_gui(imol):

   interesting_residues_gui(imol, "Residues with Alt Confs",
                            residues_with_alt_confs(imol))

# Make an interesting things GUI for residues with missing atoms
#
def missing_atoms_gui(imol):

   interesting_residues_gui(imol, "Residues with missing atoms",
                            missing_atom_info(imol))

# Make an interesting things GUI for residues with zero occupancy atoms
#
def zero_occ_atoms_gui(imol):

   interesting_things_gui("Residues with zero occupancy atoms",
                          atoms_with_zero_occ(imol))


# Make an interesting things GUI for residues of molecule number
# imol for the given imol.   A generalization of alt-confs gui.
#
def interesting_residues_gui(imol, title, interesting_residues):

   from types import ListType
   centre_atoms = []
   if valid_model_molecule_qm(imol):
      residues = interesting_residues
      for spec in residues:
         if (type(spec) is ListType):
            centre_atoms.append(residue_spec2atom_for_centre(imol, *spec))
         else:
            centre_atoms.append([False])

      interesting_things_gui(
         title,
         map(lambda residue_cpmd, centre_atom: 
             [residue_cpmd[0] + " " +
              str(residue_cpmd[1]) + " " +
              residue_cpmd[2] + " " +
              centre_atom[0] + " " + centre_atom[1],
              imol, residue_cpmd[0], residue_cpmd[1], residue_cpmd[2],
              centre_atom[0], centre_atom[1]],
             residues, centre_atoms))
      
   else:
      print "BL WARNING:: no valid molecule", imol

                
# If a toolbutton with label button_label is not found in the coot main
# toolbar, then create it and return it.
# If it does exist, the icon will be overwritten. The callback function wont!
# [NOTE: if its a Coot internal Toolbar you can
# only add a function but not overwrite its internal function!!!]
#
# return False if we cannot create the button and/or wrong no of arguments
#
# we accept 3 arguments:
#   button_label
#   callback_function   (is String!!!)
#   gtk-stock-item (or icon_widget, whatever that is)
#  
def coot_toolbar_button(*args):

   #print "BL DEBUG:: create toolbutton with args!", args
   icon_name = None
   if ((len(args) > 3) or (len(args) <2)):
      print "BL WARNING:: wrong no of arguments!\nEither 2 (button_label) or 3 (plus icon)!"
      return False
   elif (len(args) == 3):
      icon_name = args[2]
   button_label = args[0]
   callback_function = args[1]
   if (not type(callback_function) is StringType):
      print "BL WARNING:: callback function wasn't a string! Cannot create toolbarbutton!"
      return False
   
   try:
      import coot_python
   except:
      print """BL WARNING:: could not import coot_python module!!
      Some things, esp. extensions, may be crippled!"""
      return False
   
   coot_main_toolbar = coot_python.main_toolbar()

   # main body
   #
   found_button = False
   for f in toolbar_label_list():
      if button_label in f: 
         found_button = f[1]
   if found_button:
      # here we only try to add a new icon, we cannot overwrite the callback function!
      toolbutton = found_button
   else:
      toolbutton = gtk.ToolButton(icon_widget=None, label=button_label)
      coot_main_toolbar.insert(toolbutton, -1)       # insert at the end
      toolbutton.set_is_important(True)              # to display the text, otherwise only icon
      toolbutton.connect("clicked", lambda w: eval(callback_function))
      toolbutton.show()
   if (icon_name):
      # try to add a stock item
      try:
         toolbutton.set_stock_id(icon_name)
      except:
         # try to add a icon widget
         try:
            toolbutton.set_icon_widget(icon_name)
         except:
            print "BL INFO::  icon name/widget given but could not add the icon"

   return toolbutton

   
# returns a list of existing toolbar buttons
# [[label, toolbutton],[]...]
# or False if coot_python is not available
def toolbar_label_list():
   try:
      import coot_python
   except:
      return False
   
   coot_main_toolbar = coot_python.main_toolbar()
   button_label_ls = []
   for toolbar_child in coot_main_toolbar.get_children():
      ls = []
      try:
        label = toolbar_child.get_label()
      except:
        # try to do something with the gtk build in label/icons
        # for now just pass
        pass
      else:
        ls.append(toolbar_child.get_label())
        ls.append(toolbar_child)
        button_label_ls.append(ls)
   return button_label_ls


# button-list is a list of pairs (improper list) the first item of
# which is the button label text the second item is a lambda
# function, what to do when the button is pressed.
#
def generic_button_dialog(dialog_name, button_list):

	def delete_event(*args):
		window.destroy()
		return False

	# main body
	window = gtk.Window(gtk.WINDOW_TOPLEVEL)
	scrolled_win = gtk.ScrolledWindow()
	outside_vbox = gtk.VBox(False, 2)
	inside_vbox = gtk.VBox(False, 0)

	window.set_default_size(250,250)
	window.set_title(dialog_name)
	inside_vbox.set_border_width(4)

	window.add(outside_vbox)
	outside_vbox.add(scrolled_win)
	scrolled_win.add_with_viewport(inside_vbox)
	scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)

	for button_item in button_list:
		if button_item and len(button_item)==2:
			button_label = button_item[0]
			action = button_item[1]

			button = gtk.Button(button_label)
			inside_vbox.pack_start(button, False, False, 2)
			button.connect("clicked", action)
			button.show()

	outside_vbox.set_border_width(4)
	ok_button = gtk.Button("  OK  ")
	outside_vbox.pack_start(ok_button, False, False, 6)
	ok_button.connect("clicked", delete_event)

	window.show_all()


# Generic interesting things gui: user passes a function that takes 4
# args: the chain-id, resno, inscode and residue-serial-number
# (should it be needed) and returns either #f or something
# interesting (e.g. a label/value).  It is the residue-test-func of
# the residue-matching-criteria function.
# 
def generic_interesting_things(imol,gui_title_string,residue_test_func):

	if valid_model_molecule_qm(imol):

		interesting_residues = residues_matching_criteria(imol, residue_test_func)
		for i in range(len(interesting_residues)): interesting_residues[i][0] = imol
		centre_atoms = map(residue_spec, interesting_residues)
		if centre_atoms:
			# BL says:: ignoring "Atom in residue name failure" for nor
			interesting_things_gui(gui_title_string,
			map(lambda interesting_residue, centre_atom:
			[interesting_residue[0] + " " 
			+ interesting_residue[1] + " " 
			+ str(interesting_residue[2]) + " " 
			+ interesting_residue[3] + " " 
			+ centre_atom[0] + " " + centre_atom[1]]
			+ interesting_residue[1:len(interesting_residue)] + centre_atom,
			interesting_residues, centre_atoms)
			)
	else:
		print "BL WARNING:: no valid model molecule ", imol

def generic_number_chooser(number_list, default_option_value, hint_text, go_button_label, go_function):

    def delete_event(*args):
       window.destroy()
       return False

    def go_button_pressed(*args):
        active_number = int(get_option_menu_active_item(option_menu, number_list))
        try:
#           print "BL DEBUG:: go_function is:", go_function
#           print "BL DEBUG:: active_number is:", active_number
           go_function(active_number)
        except:
           print "Failed to get execute function"
        delete_event()


    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    vbox = gtk.VBox(False, 0)
    hbox1 = gtk.HBox(False, 0)
    hbox2 = gtk.HBox(True, 0)      # for Go and Cancel
    function_label = gtk.Label(hint_text)
    h_sep = gtk.HSeparator()
    go_button = gtk.Button(go_button_label)
    cancel_button = gtk.Button("  Cancel  ")
# BL says:: option menu is depricated, so we use combox instead, maybe!?!
    option_menu = gtk.combo_box_new_text()

    fill_option_menu_with_number_options(option_menu, number_list, default_option_value)

    vbox.pack_start(hbox1, True, False, 0)
    vbox.pack_start(function_label, False, 0)
    vbox.pack_start(option_menu, True, 0)
    vbox.pack_start(h_sep)
    vbox.pack_start(hbox2, False, False, 0)
    hbox2.pack_start(go_button, False, False, 6)
    hbox2.pack_start(cancel_button, False, False, 6)
    window.add(vbox)
    vbox.set_border_width(6)
    hbox1.set_border_width(6)
    hbox2.set_border_width(6)
    go_button.connect("clicked", go_button_pressed, option_menu, number_list, go_function)
    cancel_button.connect("clicked", delete_event)

    window.show_all()

# vbox is the vbox to which this compound widget should be added.
# button-press-func is the lambda function called on pressing return
# or the button, which takes one argument, the entry.
#
# Add this widget to the vbox here.
#
def entry_do_button(vbox, hint_text, button_label, button_press_func):

	hbox = gtk.HBox(False, 0)
	entry = gtk.Entry()
	button = gtk.Button(button_label)
	label = gtk.Label(hint_text)

	hbox.pack_start(label, False, False, 2)
	hbox.pack_start(entry, True, False, 2)
	hbox.pack_start(button, False, False, 2)
	button.connect("clicked", button_press_func, entry)

	label.show()
	button.show()
	entry.show()
	hbox.show()
	vbox.pack_start(hbox, True, False)
	return entry

# pack a hint text and a molecule chooser option menu into the given vbox.
# 
# return the option-menu and model molecule list:
def generic_molecule_chooser(hbox, hint_text):

	menu = gtk.Menu()
# BL says:: option menu is depricated, so we use combox instead, maybe!?!
	option_menu = gtk.combo_box_new_text()
	label = gtk.Label(hint_text)
	model_mol_list = fill_option_menu_with_coordinates_mol_options(option_menu)

	hbox.pack_start(label, False, False, 2)
	hbox.pack_start(option_menu, True, True, 2)
        return [option_menu, model_mol_list]

# Return an entry, the widget is inserted into the hbox passed to
# this function
#
def file_selector_entry(hbox, hint_text):

   if (file_chooser_selector_state() == 0 or gtk.pygtk_version < (2,3,90)):

	vbox = gtk.VBox(False, 0)

	def file_func1(*args):
		def file_ok_sel(*args):
			t = fs_window.get_filename()
			print t
			entry.set_text(t)
			fs_window.destroy()

		fs_window = gtk.FileSelection("file selection")
		fs_window.ok_button.connect("clicked", file_ok_sel)
		fs_window.cancel_button.connect("clicked",
				lambda w: fs_window.destroy())
		fs_window.show()

	entry = entry_do_button(vbox, hint_text, "  File...  ", file_func1)

	hbox.pack_start(vbox, False, False, 2)
	vbox.show()
        return entry
     
   else:
      return file_chooser_entry(hbox, hint_text)

# This is the same as the file_selector_entry, but using the modern FileChooser
# Return an entry, the widget is inserted into the hbox passed to
# this function
#
def file_chooser_entry(hbox, hint_text):

   if gtk.pygtk_version > (2,3,90):

	vbox = gtk.VBox(False, 0)

	def file_func1(*args):
		def file_ok_sel(*args):
			t = fc_window.get_filename()
			print t
			entry.set_text(t)
			fc_window.destroy()

		fc_window = gtk.FileChooserDialog("file selection",
                                                  None,
                                                  gtk.FILE_CHOOSER_ACTION_OPEN,
                                                  (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                                   gtk.STOCK_OPEN, gtk.RESPONSE_OK))
                response = fc_window.run()
                if response == gtk.RESPONSE_OK:
                   file_ok_sel(fc_window, entry)
                elif response == gtk.RESPONSE_CANCEL:
                   fc_window.destroy()

	entry = entry_do_button(vbox, hint_text, "  File...  ", file_func1)

	hbox.pack_start(vbox, False, False, 2)
	vbox.show()
        return entry
   else:
        print "PyGtk 2.3.90 or later required for this function!"
        return False

# The gui for the strand placement function
#
def place_strand_here_gui():

    generic_number_chooser(number_list(4,12), 7, 
                            " Estimated number of residues in strand",
                            "  Go  ",
                            lambda n: place_strand_here(n, 15))

# Cootaneer gui
#
def cootaneer_gui(imol):

	def delete_event(*args):
		window.destroy()
		return False

	def go_function_event(widget, imol):
		print "apply the sequence info here\n"
		print "then cootaneer\n"

		# no active atom won't do.  We need
		# to find the nearest atom in imol to (rotation-centre).
		#
		# if it is too far away, give a
		# warning and do't do anything.

		n_atom = closest_atom(imol)
		if n_atom:
			imol	= n_atom[0]
			chain_id= n_atom[1]
			resno	= n_atom[2]
			inscode	= n_atom[3]
			at_name	= n_atom[4]
			alt_conf= n_atom[5]
			cootaneer(imol_map, imol, [chain_id, resno, inscode, 
				at_name, alt_conf])
		else:
			print "BL WARNING:: no close atom found!"


	def add_text_to_text_box(text_box, description):
		start = text_box.get_start_iter()
		text_box.create_tag("tag", foreground="black", 
			background = "#c0e6c0")
		text_box.insert_with_tags_by_name(start, description, "tag")

	# return the (entry . textbuffer/box)
	#
	def entry_text_pair_frame(seq_info):

		frame = gtk.Frame()
		vbox = gtk.VBox(False, 3)
		entry = gtk.Entry()
		textview = gtk.TextView()
                textview.set_wrap_mode(gtk.WRAP_WORD_CHAR)
		text_box = textview.get_buffer()
		chain_id_label = gtk.Label("Chain ID")
		sequence_label = gtk.Label("Sequence")
	
		frame.add(vbox)
		vbox.pack_start(chain_id_label, False, False, 2)
		vbox.pack_start(entry, False, False, 2)
		vbox.pack_start(sequence_label, False, False, 2)
		vbox.pack_start(textview, False, False, 2)
		add_text_to_text_box(text_box, seq_info[1])
		entry.set_text(seq_info[0])
		return [frame, entry, text_box]

	# main body
	imol_map = imol_refinement_map()
	if (imol_map == -1):
           show_select_map_dialog()
           print "BL DEBUG:: probably should wait here for input!?"
	
	window = gtk.Window(gtk.WINDOW_TOPLEVEL)
	outside_vbox = gtk.VBox(False, 2)
	inside_vbox = gtk.VBox(False, 2)
	h_sep = gtk.HSeparator()
	buttons_hbox = gtk.HBox(True, 2)
	go_button = gtk.Button("  Cootaneer!  ")
	cancel_button = gtk.Button("  Cancel  ")

	seq_info_ls = sequence_info(imol)
        # print "BL DEBUG:: sequence_list and imol is", seq_info_ls, imol

	for seq_info in seq_info_ls:
		seq_widgets = entry_text_pair_frame(seq_info)
		inside_vbox.pack_start(seq_widgets[0], False, False, 2)

	outside_vbox.pack_start(inside_vbox, False, False, 2)
	outside_vbox.pack_start(h_sep, False, False, 2)
	outside_vbox.pack_start(buttons_hbox, True, False, 2)
	buttons_hbox.pack_start(go_button, True, False, 6)
	buttons_hbox.pack_start(cancel_button, True, False, 6)

	cancel_button.connect("clicked", delete_event)

	go_button.connect("clicked", go_function_event, imol)

        window.add(outside_vbox)
	window.show_all()	


# The gui for saving views
#
def view_saver_gui():

	def local_view_name():
		view_count = 0
		while view_count or view_count == 0:
			strr = "View"
			if view_count > 0:
				strr = strr + "-" + str(view_count)
			# now is a view already called str?
			jview = 0
			while jview or jview == 0:
				jview_name = view_name(jview)
				if jview >= n_views(): return strr
				elif jview_name == False: return strr
				elif strr == jview_name:
					view_count += 1
					break
				else:
					jview +=1
		return strr

	generic_single_entry("View Name: ", local_view_name(), " Add View ",
		lambda text: add_view_here(text))

# geometry is an improper list of ints
#
# return the h_box of the buttons
#
# a button is a list of [label, callback, text_description]
#
# Note:
#  - if label is "HSep" a horizontal separator is inserted instead of a button
#  - the description is optional
#
def dialog_box_of_buttons(window_name, geometry, buttons, close_button_label):
   

   def add_text_to_text_widget(text_box, description):
      textbuffer = text_box.get_buffer()
      start = textbuffer.get_start_iter()
      textbuffer.create_tag("tag", foreground="black", 
                            background = "#c0e6c0")
      textbuffer.insert_with_tags_by_name(start, description, "tag")

   # main line
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   scrolled_win = gtk.ScrolledWindow()
   outside_vbox = gtk.VBox(False, 2)
   inside_vbox = gtk.VBox(False, 0)
   
   window.set_default_size(geometry[0], geometry[1])
   window.set_title(window_name)
   inside_vbox.set_border_width(2)
   window.add(outside_vbox)
   outside_vbox.pack_start(scrolled_win, True, True, 0) # expand fill padding
   scrolled_win.add_with_viewport(inside_vbox)
   scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)
   
   for button_info in buttons:
      button_label = button_info[0]
      if ((button_label == "HSep") and (len(button_info) == 1)):
         # insert a HSeparator rather than a button
         button = gtk.HSeparator()
      else:
         callback = button_info[1]
         if (len(button_info) == 2):
            description = False
         else:
            description = button_info[2]
         button = gtk.Button(button_label)

         # BL says:: in python we should pass the callback as a string
         if type(callback) is StringType:
            def callback_func(button, call):
               eval(call)
            button.connect("clicked", callback_func, callback)
         elif (type(callback) is ListType):
            def callback_func(button, call):
               for item in call:
                  eval(item)
            button.connect("clicked", callback_func, callback)                   
         else:
            button.connect("clicked", callback)

      inside_vbox.pack_start(button, False, False, 2)

   outside_vbox.set_border_width(2)
   ok_button = gtk.Button(close_button_label)
   outside_vbox.pack_end(ok_button, False, False, 0)
   ok_button.connect("clicked", lambda w: window.destroy())
	
   window.show_all()

# geometry is an improper list of ints
# buttons is a list of: [[["button_1_label, button_1_action],
#                         ["button_2_label, button_2_action]], [next pair of buttons]]
# The button_1_action function is a string
# The button_2_action function is a string
# 
def dialog_box_of_pairs_of_buttons(imol, window_name, geometry, buttons, close_button_label):

        window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        scrolled_win = gtk.ScrolledWindow()
        outside_vbox = gtk.VBox(False, 2)
        inside_vbox = gtk.VBox(False, 0)

        window.set_default_size(geometry[0], geometry[1])
        window.set_title(window_name)
        inside_vbox.set_border_width(2)
        window.add(outside_vbox)
        outside_vbox.pack_start(scrolled_win, True, True, 0) # expand fill padding
        scrolled_win.add_with_viewport(inside_vbox)
        scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)

	for button_info in buttons:
           #print "button_info ", button_info
           if type(button_info) is ListType:
              button_label_1 = button_info[0][0]
              callback_1 = button_info[0][1]

              button_label_2 = button_info[1][0]
              callback_2 = button_info[1][1]

              button_1 = gtk.Button(button_label_1)
              h_box = gtk.HBox(False, 2)

              #print "button_label_1 ", button_label_1
              #print "callback_1 ", callback_1
              #print "button_label_2 ", button_label_2
              #print "callback_2 ", callback_2

              def callback_func(button, call):
                 eval(call)
              button_1.connect("clicked", callback_func, callback_1)
              h_box.pack_start(button_1, False, False, 2)

              if callback_2:
                 button_2 = gtk.Button(button_label_2)
                 button_2.connect("clicked", callback_func, callback_2)
                 h_box.pack_start(button_2, False, False, 2)
              inside_vbox.pack_start(h_box, False, False, 2)

        outside_vbox.set_border_width(2)
        ok_button = gtk.Button(close_button_label)
        outside_vbox.pack_end(ok_button, False, False, 2)
        ok_button.connect("clicked", lambda w: window.destroy())
        window.show_all()

# as the dialog_box_of_buttons, but we can put in an extra widget (extra_widget)
#
def dialog_box_of_buttons_with_widget(window_name, geometry, buttons, extra_widget, close_button_label):

	def add_text_to_text_widget(text_box, description):
		textbuffer = text_box.get_buffer()
		start = textbuffer.get_start_iter()
		textbuffer.create_tag("tag", foreground="black", 
						background = "#c0e6c0")
		textbuffer.insert_with_tags_by_name(start, description, "tag")

	# main line
	window = gtk.Window(gtk.WINDOW_TOPLEVEL)
	scrolled_win = gtk.ScrolledWindow()
	outside_vbox = gtk.VBox(False, 2)
	inside_vbox = gtk.VBox(False, 0)
        h_sep = gtk.HSeparator()

        window.set_default_size(geometry[0], geometry[1])
	window.set_title(window_name)
	inside_vbox.set_border_width(2)
	window.add(outside_vbox)
	outside_vbox.pack_start(scrolled_win, True, True, 0) # expand fill padding
	scrolled_win.add_with_viewport(inside_vbox)
	scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)

	for button_info in buttons:
		button_label = button_info[0]
		callback = button_info[1]
		if len(button_info)==2:
			description = False
		else:
			description = button_info[2]
		button = gtk.Button(button_label)

# BL says:: in python we should pass the callback as a string
		if type(callback) is StringType:
			def callback_func(button,call):
				eval(call)
			button.connect("clicked", callback_func, callback)
                elif (type(callback) is ListType):
                   def callback_func(button, call):
                      for item in call:
                         eval(item)
                   button.connect("clicked", callback_func, callback) 
		else:
			button.connect("clicked", callback)

		if type(description) is StringType:
			text_box = gtk.TextView()
			text_box.set_editable(False)
			add_text_to_text_widget(text_box, description)
			inside_vbox.pack_start(text_box, False, False, 2)
			text_box.realize()
# BL says:: not working here
#			text_box.thaw()

		inside_vbox.pack_start(button, False, False, 2)

        # for the extra widget
        inside_vbox.pack_start(h_sep, False, False, 2)
        inside_vbox.pack_start(extra_widget, False, False, 2)
        
	outside_vbox.set_border_width(2)
	ok_button = gtk.Button(close_button_label)
	outside_vbox.pack_end(ok_button, False, False, 0)
	ok_button.connect("clicked", lambda w: window.destroy())
	
	window.show_all()

# A dialog box with radiobuttons e.g. to cycle thru loops
#
# the button list shall be in the form of:
# [[button_label1, "button_function1"],
#  [button_label2, "button_function2"]]
#
# function happens when toggled
# obs: button_functions are strings, but can be tuples for multiple functions
# go_function is string too!
# selected button is the button to be toggled on (default is first)
#
def dialog_box_of_radiobuttons(window_name, geometry, buttons,
                               go_button_label, go_button_function,
                               selected_button = 0,
                               cancel_button_label = "",
                               cancel_function = False):
	
   def go_function_event(widget, button_ls):
      eval(go_button_function)
      window.destroy()
      return False

   def cancel_function_cb(widget):
      #print "BL DEBUG:: just for the sake"
      if (cancel_function):
         eval(cancel_function)
      window.destroy()
      return False
	
   # main line
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   scrolled_win = gtk.ScrolledWindow()
   outside_vbox = gtk.VBox(False, 2)
   inside_vbox = gtk.VBox(False, 0)
   button_hbox = gtk.HBox(False, 0)

   window.set_default_size(geometry[0], geometry[1])
   window.set_title(window_name)
   inside_vbox.set_border_width(2)
   window.add(outside_vbox)
   outside_vbox.pack_start(scrolled_win, True, True, 0) # expand fill padding
   scrolled_win.add_with_viewport(inside_vbox)
   scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)

   button = None
   button_ls = []
   for button_info in buttons:
      button_label = button_info[0]
      callback = button_info[1]
      button = gtk.RadioButton(button, button_label)

      # BL says:: in python we should pass the callback as a string
      if type(callback) is StringType:
         def callback_func(button, call):
            eval(call)
         button.connect("toggled", callback_func, callback)
      else:
         button.connect("toggled", callback)

      inside_vbox.pack_start(button, False, False, 2)
      button_ls.append(button)

   outside_vbox.set_border_width(2)
   go_button     = gtk.Button(go_button_label)
   outside_vbox.pack_start(button_hbox, False, False, 2)
   button_hbox.pack_start(go_button, True, True, 6)
   go_button.connect("clicked", go_function_event, button_ls)
   if (cancel_button_label):
      cancel_button = gtk.Button(cancel_button_label)
      button_hbox.pack_start(cancel_button, True, True, 6)
      cancel_button.connect("clicked", cancel_function_cb)
		
   # switch on the first or selected button
   # somehow I need to emit the toggled signal too (shouldnt have to!?)
   button_ls[selected_button].set_active(True)
   button_ls[selected_button].toggled()

   window.show_all()

# A gui showing views
#
def views_panel_gui():

	number_of_views = n_views()
	buttons = []

	for button_number in range(number_of_views):
		button_label = view_name(button_number)
		desciption = view_description(button_number)
# BL says:: add the decisption condition!!
		buttons.append([button_label, "go_to_view_number(" + str(button_number) + ",0)", desciption])

	if len(buttons) > 1:
		def view_button_func():
			import time
			go_to_first_view(1)
			time.sleep(1)
			play_views()
		view_button = ["  Play Views ", lambda func: view_button_func()]
		buttons.insert(0,view_button)

	dialog_box_of_buttons("Views", [200,140], buttons, "  Close  ")

# nudge screen centre box.  Useful when Ctrl left-mouse has been
# taken over by another function.
#
# This is using the absolute coordinates
# 
def nudge_screen_centre_paule_gui():

	zsc = 0.02
# BL says:: in python we need some helper functn, no 'proper' lambda
# axis 0-2 = x,y,z and 0=+ and 1=-
	def nudge_screen_func(axis, operand):
		nudge = zsc * zoom_factor()
		rc = rotation_centre()
		if operand == 0:
			rc[axis] += nudge
		elif operand == 1:
			rc[axis] -= nudge
		else:
			# We should never be here
			print "BL WARNING:: something went wrong!!!"
		set_rotation_centre(*rc)

	buttons = [ 
		["Nudge +X", lambda func: nudge_screen_func(0,0)],
		["Nudge -X", lambda func: nudge_screen_func(0,1)],
		["Nudge +Y", lambda func: nudge_screen_func(1,0)],
		["Nudge -Y", lambda func: nudge_screen_func(1,1)],
		["Nudge +Z", lambda func: nudge_screen_func(2,0)],
		["Nudge -Z", lambda func: nudge_screen_func(2,1)],
	]

	dialog_box_of_buttons("Nudge Screen Centre", [200,240], buttons, "  Close ")
		

# nudge screen centre box.  Useful when Ctrl left-mouse has been
# taken over by another function.
#
# This is using the viewing coordinates
# 
def nudge_screen_centre_gui():

	zsc = 0.02
# BL says:: in python we need some helper functn, no 'proper' lambda
# axis 0-2 = x,y,z and 0=+ and 1=-
	def nudge_screen_func(axis, operand):
		nudge_factor = zsc * zoom_factor()
		rc = rotation_centre()
                mat = view_matrix_transp()
                nudge_vector = []
                for i in range(axis, axis + 7, 3):
                   nudge_vector.append(mat[i])
		if operand == 0:
                   rc = map(lambda x, y: x + y, rc, nudge_vector)
		elif operand == 1:
                   rc = map(lambda x, y: x - y, rc, nudge_vector)
		else:
			# We should never be here
			print "BL WARNING:: something went wrong!!!"
		set_rotation_centre(*rc)

	buttons = [ 
		["Nudge +X", lambda func: nudge_screen_func(0,0)],
		["Nudge -X", lambda func: nudge_screen_func(0,1)],
		["Nudge +Y", lambda func: nudge_screen_func(1,0)],
		["Nudge -Y", lambda func: nudge_screen_func(1,1)],
		["Nudge +Z", lambda func: nudge_screen_func(2,0)],
		["Nudge -Z", lambda func: nudge_screen_func(2,1)],
	]

	dialog_box_of_buttons("Nudge Screen Centre", [200,240], buttons, "  Close ")

# as nudge_screen_centre_gui but with clipping and zoom control
def nudge_screen_centre_extra_gui():

   # this is for the centre nudging
   zsc = 0.02
# BL says:: in python we need some helper functn, no 'proper' lambda
# axis 0-2 = x,y,z and 0=+ and 1=-
   def nudge_screen_func(axis, operand):
	nudge_factor = zsc * zoom_factor()
	rc = rotation_centre()
        mat = view_matrix_transp()
        nudge_vector = []
        for i in range(axis, axis + 7, 3):
           nudge_vector.append(mat[i])
        if operand == 0:
           rc = map(lambda x, y: x + y, rc, nudge_vector)
        elif operand == 1:
           rc = map(lambda x, y: x - y, rc, nudge_vector)
        else:
           # We should never be here
           print "BL WARNING:: something went wrong!!!"
        set_rotation_centre(*rc)

   buttons = [ 
		["Nudge +X", lambda func: nudge_screen_func(0,0)],
		["Nudge -X", lambda func: nudge_screen_func(0,1)],
		["Nudge +Y", lambda func: nudge_screen_func(1,0)],
		["Nudge -Y", lambda func: nudge_screen_func(1,1)],
		["Nudge +Z", lambda func: nudge_screen_func(2,0)],
		["Nudge -Z", lambda func: nudge_screen_func(2,1)],
	]

   # and this is for the clipping and zooming
   vbox = gtk.VBox(False, 0)

   def change_clipp(*args):
        set_clipping_front(clipp_adj.value)
        set_clipping_back (clipp_adj.value)

   def change_zoom(*args):
        set_zoom(zoom_adj.value)
        graphics_draw()

   # for clipping
   clipp_label = gtk.Label("Clipping")
   clipp_adj = gtk.Adjustment(0.0, -10.0, 20.0, 0.05, 4.0, 10.1)
   clipp_scale = gtk.HScale(clipp_adj)
   vbox.pack_start(clipp_label, False, False, 0)
   vbox.pack_start(clipp_scale, False, False, 0)
   clipp_label.show()
   clipp_scale.show()

   clipp_adj.connect("value_changed", change_clipp)
   
   h_sep = gtk.HSeparator()
   vbox.pack_start(h_sep, False, False, 5)

   # for zooming
   zoom = zoom_factor()
   zoom_label = gtk.Label("Zoom")
   zoom_adj = gtk.Adjustment(zoom, zoom*0.125, zoom*8, 0.01, 0.5, zoom)
   zoom_scale = gtk.HScale(zoom_adj)
   vbox.pack_start(zoom_label, False, False, 0)
   vbox.pack_start(zoom_scale, False, False, 0)
   zoom_label.show()
   zoom_scale.show()

   zoom_adj.connect("value_changed", change_zoom)

   dialog_box_of_buttons_with_widget("Nudge Screen Centre with Extras", [200,400], buttons, vbox, "  Close ")


# A gui to make a difference map (from arbitrarily gridded maps
# (that's it's advantage))
#
def make_difference_map_gui():
   
   def delete_event(*args):
      window.destroy()
      return False

   def go_function_event(widget):
      print "make diff map here\n"
      active_mol_no_ref = get_option_menu_active_molecule(option_menu_ref_mol, map_molecule_list_ref)
      active_mol_no_sec = get_option_menu_active_molecule(option_menu_sec_mol, map_molecule_list_sec)
      scale_text = scale_entry.get_text()
      scale = False
      try:
         scale = float(scale_text)
      except:
         print "can't decode scale", scale_text
      if (scale):
         difference_map(active_mol_no_ref, active_mol_no_sec, scale)
      delete_event()

      
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   diff_map_vbox = gtk.VBox(False, 2)
   h_sep = gtk.HSeparator()
   title = gtk.Label("Make a Difference Map")
   ref_label = gtk.Label("Reference Map:")
   sec_label = gtk.Label("Substract this map:")
   second_map_hbox = gtk.HBox(False, 2)
   buttons_hbox = gtk.HBox(True, 6)
   option_menu_ref_mol = gtk.combo_box_new_text()
   option_menu_sec_mol = gtk.combo_box_new_text()
   scale_label = gtk.Label("Scale")
   scale_entry = gtk.Entry()
   ok_button = gtk.Button("   OK   ")
   cancel_button = gtk.Button(" Cancel ")

   map_molecule_list_ref = fill_option_menu_with_map_mol_options(option_menu_ref_mol)
   map_molecule_list_sec = fill_option_menu_with_map_mol_options(option_menu_sec_mol)
  
   window.add(diff_map_vbox)
   diff_map_vbox.pack_start(title, False, False, 2)
   diff_map_vbox.pack_start(ref_label, False, False, 2)
   diff_map_vbox.pack_start(option_menu_ref_mol, True, True, 2)

   diff_map_vbox.pack_start(sec_label, False, False, 2)
   diff_map_vbox.pack_start(second_map_hbox, False, False, 2)

   second_map_hbox.pack_start(option_menu_sec_mol, True, True, 2)
   second_map_hbox.pack_start(scale_label, False, False, 2)
   second_map_hbox.pack_start(scale_entry, False, False, 2)

   diff_map_vbox.pack_start(h_sep, True, False, 2)
   diff_map_vbox.pack_start(buttons_hbox, True, False, 2)
   buttons_hbox.pack_start(ok_button, True, False, 2)
   buttons_hbox.pack_start(cancel_button, True, False, 2)
   scale_entry.set_text("1.0")

   ok_button.connect("clicked", go_function_event)

   cancel_button.connect("clicked", delete_event)

   window.show_all()
   

def cis_peptides_gui(imol):

   def get_ca(atom_list):

      if (atom_list == []):
         return False
      else:
         for atom in atom_list:
            atom_name = atom[0][0]
            if (atom_name == ' CA '):
               print "BL DEBUG:: returning ", atom
               return atom     # check me

   def make_list_of_cis_peps(imol, list_of_cis_peps):

      ret = []

      for cis_pep_spec in list_of_cis_peps:
         r1 = cis_pep_spec[0]
         r2 = cis_pep_spec[1]
         omega = cis_pep_spec[2]
         atom_list_r1 = residue_info(imol, *r1[1:4])
         atom_list_r2 = residue_info(imol, *r2[1:4])
         ca_1 = get_ca(atom_list_r1)
         ca_2 = get_ca(atom_list_r2)
         chain_id = r1[1]

         if ((not ca_1) and (not ca_2)):
            ret.append(["Cis Pep (atom failure) " + r1[1] + " " + str(r1[2]),
                    imol, chain_id, r1[3], r1[2], "CA", ""])
         else:
            p1 = ca_1[2]
            p2 = ca_2[2]
            pos = map(lambda x, y: (x + y) / 2.0, p1, p2)
            tors_s1 = str(omega)
            if (len(tors_s1) < 6):
               tors_string = tors_s1
            else:
               tors_string = tors_s1[0:6]
            mess = ("Cis Pep: " + chain_id + " " +
                   str(r1[2]) + " " + 
                   residue_name(imol, *r1[1:4]) + " - " +
                   str(r2[2]) + " " +
                   residue_name(imol, *r1[1:4]) + "   " +
                   tors_string)
            ls = pos
            ls.insert(0,mess)
            ret.append(ls)
      return ret

   cis_peps = cis_peptides(imol)

   if (cis_peps == []):
      info_dialog("No Cis Peptides found")
   else:
      list_of_cis_peptides = make_list_of_cis_peps(imol, cis_peps)
      interesting_things_gui("Cis Peptides:", list_of_cis_peptides)

#
def transform_map_using_lsq_matrix_gui():

  def delete_event(*args):
     window.destroy()
     return False

  def on_ok_button_clicked(*args):
     active_mol_ref = get_option_menu_active_molecule(frame_info_ref[1], frame_info_ref[2])
     active_mol_mov = get_option_menu_active_molecule(frame_info_mov[1], frame_info_mov[2])
     
     chain_id_ref     = frame_info_ref[3].get_text()
     resno_1_ref_text = frame_info_ref[4].get_text()
     resno_2_ref_text = frame_info_ref[5].get_text()

     chain_id_mov     = frame_info_mov[3].get_text()
     resno_1_mov_text = frame_info_mov[4].get_text()
     resno_2_mov_text = frame_info_mov[5].get_text()

     radius_text = radius_entry.get_text()

     imol_map = imol_refinement_map()
     cont = False
     try:
        resno_1_ref = int(resno_1_ref_text)
        resno_2_ref = int(resno_2_ref_text)
        resno_1_mov = int(resno_1_mov_text)
        resno_2_mov = int(resno_2_mov_text)
        radius = float(radius_text)
        cont = True
     except:
        print "BL ERROR:: conversion from input text to numbers failed"
        
     if (cont):
        if (not valid_map_molecule_qm(imol_map)):
           print "Must set the refinement map"
        else:
           imol_copy = copy_molecule(active_mol_mov)
           new_map_number = transform_map_using_lsq_matrix(active_mol_ref, chain_id_ref, resno_1_ref, resno_2_ref,
                                                           imol_copy, chain_id_mov, resno_1_mov, resno_2_mov,
                                                           imol_map, rotation_centre(), radius)
           set_molecule_name(imol_copy,
                             "Transformed copy of " + strip_path(molecule_name(active_mol_mov)))

           s =  "Transformed map: from map " + str(imol_map) + \
               " by matrix that created coords " + str(imol_copy)
           set_molecule_name(new_map_number,
                             "Transformed map: from map " + str(imol_map) + \
                             " by matrix that created coords " + str(imol_copy))
           
           set_mol_active(imol_copy, 0)
           set_mol_displayed(imol_copy, 0)

     window.destroy()
   
  # atom-sel-type is either 'Reference or 'Moving
  # 
  # return the list [frame, option_menu, model_mol_list, entries...]
  def atom_sel_frame(atom_sel_type):
     frame = gtk.Frame(atom_sel_type)
     # option_menu == combobox
     option_menu = gtk.combo_box_new_text()
     model_mol_list = fill_option_menu_with_coordinates_mol_options(option_menu)
     atom_sel_vbox = gtk.VBox(False, 2)
     atom_sel_hbox = gtk.HBox(False, 2)
     chain_id_label = gtk.Label(" Chain ID ")
     resno_1_label = gtk.Label(" Resno Start ")
     resno_2_label = gtk.Label(" Resno End ")
     chain_id_entry = gtk.Entry()
     resno_1_entry = gtk.Entry()
     resno_2_entry = gtk.Entry()

     frame.add(atom_sel_vbox)
     atom_sel_vbox.pack_start(option_menu,    False, False, 2)
     atom_sel_vbox.pack_start(atom_sel_hbox,  False, False, 2)
     atom_sel_hbox.pack_start(chain_id_label, False, False, 2)
     atom_sel_hbox.pack_start(chain_id_entry, False, False, 2)
     atom_sel_hbox.pack_start(resno_1_label,  False, False, 2)
     atom_sel_hbox.pack_start(resno_1_entry,  False, False, 2)
     atom_sel_hbox.pack_start(resno_2_label,  False, False, 2)
     atom_sel_hbox.pack_start(resno_2_entry,  False, False, 2)

     return [frame, option_menu, model_mol_list, chain_id_entry, resno_1_entry, resno_2_entry]
  
  window = gtk.Window(gtk.WINDOW_TOPLEVEL)
  dialog_name = "Map Transformation"
  main_vbox = gtk.VBox(False, 2)
  buttons_hbox = gtk.HBox(False, 2)
  cancel_button = gtk.Button("  Cancel  ")
  ok_button = gtk.Button("  Transform  ")
  usage = "Note that this will transform the current refinement map " + \
          "about the screen centre"
  usage_label = gtk.Label(usage)
  h_sep = gtk.HSeparator()
  frame_info_ref = atom_sel_frame("Reference")
  frame_info_mov = atom_sel_frame("Moving")
  radius_hbox = gtk.HBox(False, 2)
  radius_label = gtk.Label("  Radius ")
  radius_entry = gtk.Entry()
  window.set_title(dialog_name)


  radius_hbox.pack_start(radius_label, False, False, 2)
  radius_hbox.pack_start(radius_entry, False, False, 2)

  buttons_hbox.pack_start(ok_button,     False, False, 4)
  buttons_hbox.pack_start(cancel_button, False, False, 4)

  window.add(main_vbox)
  main_vbox.pack_start(frame_info_ref[0], False, False, 2)
  main_vbox.pack_start(frame_info_mov[0], False, False, 2)
  main_vbox.pack_start(radius_hbox, False, False, 2)
  main_vbox.pack_start(usage_label, False, False, 4)
  main_vbox.pack_start(h_sep, False, False, 2)
  main_vbox.pack_start(buttons_hbox, False, False, 6)

  frame_info_ref[3].set_text("A")
  frame_info_ref[4].set_text("1")
  frame_info_ref[5].set_text("10")
  frame_info_mov[3].set_text("B")
  frame_info_mov[4].set_text("1")
  frame_info_mov[5].set_text("10")

  radius_entry.set_text("8.0")

  cancel_button.connect("clicked", delete_event)
  ok_button.connect("clicked",on_ok_button_clicked)

  window.show_all()

def ncs_ligand_gui():
   
   def delete_event(*args):
      window.destroy()
      return False

   def go_button_function(widget):
      print "ncs ligand function here\n"
      active_mol_no_ref = get_option_menu_active_molecule(option_menu_ref_mol, molecule_list_ref)
      active_mol_no_lig = get_option_menu_active_molecule(option_menu_lig_mol, molecule_list_lig)
      chain_id_lig = chain_id_lig_entry.get_text()
      chain_id_ref = chain_id_ref_entry.get_text()
      resno_start = False
      resno_end = False
      try:
         resno_start = int(resno_start_entry.get_text())
      except:
         print "can't decode resno_start", resno_start_entry.get_text()
         
      resno_end_t = resno_end_entry.get_text()
      try:
         resno_end = int(resno_end_t)
      except:
         if (resno_end_t == ""):
            resno_end = resno_start
         else:
            print "can't decode resno_end", resno_end_t

      if (resno_end and resno_start):
         make_ncs_ghosts_maybe(active_mol_no_ref)
         print "ncs ligand with", active_mol_no_ref, \
               chain_id_ref, active_mol_no_lig, chain_id_lig, \
               resno_start, resno_end
         ncs_ligand(active_mol_no_ref,
                    chain_id_ref,
                    active_mol_no_lig,
                    chain_id_lig,
                    resno_start,
                    resno_end)

      delete_event()
      
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   ncs_ligands_vbox = gtk.VBox(False, 2)
   title = gtk.Label("Find NCS-Related Ligands")
   ref_label = gtk.Label("Protein with NCS:")
   ref_chain_hbox = gtk.HBox(False, 2)
   chain_id_ref_label = gtk.Label("NCS Master Chain")
   chain_id_ref_entry = gtk.Entry()
   lig_label = gtk.Label("Molecule containing ligand")
   specs_hbox = gtk.HBox(False, 2)
   h_sep = gtk.HSeparator()
   buttons_hbox = gtk.HBox(True, 6)
   chain_id_lig_label= gtk.Label("Chain ID: ")
   resno_start_label = gtk.Label(" Residue Number ")
   to_label = gtk.Label("  to  ")
   chain_id_lig_entry = gtk.Entry()
   resno_start_entry = gtk.Entry()
   resno_end_entry = gtk.Entry()
   ok_button = gtk.Button("   Find Candidate Positions  ")
   cancel_button = gtk.Button("    Cancel    ")
   option_menu_ref_mol = gtk.combo_box_new_text()
   option_menu_lig_mol = gtk.combo_box_new_text()

   molecule_list_ref = fill_option_menu_with_coordinates_mol_options(option_menu_ref_mol)
   molecule_list_lig = fill_option_menu_with_coordinates_mol_options(option_menu_lig_mol)

   window.add(ncs_ligands_vbox)
   ncs_ligands_vbox.pack_start(title, False, False, 6)
   ncs_ligands_vbox.pack_start(ref_label, False, False, 2)
   ncs_ligands_vbox.pack_start(option_menu_ref_mol, True, False, 2)
   ncs_ligands_vbox.pack_start(ref_chain_hbox, False, False, 2)
   ncs_ligands_vbox.pack_start(lig_label, False, False, 2)
   ncs_ligands_vbox.pack_start(option_menu_lig_mol, True, False, 2)
   ncs_ligands_vbox.pack_start(specs_hbox, False, False, 2)
   ncs_ligands_vbox.pack_start(h_sep, False, False, 2)
   ncs_ligands_vbox.pack_start(buttons_hbox, False, False, 2)

   buttons_hbox.pack_start(ok_button,     True, False, 4)
   buttons_hbox.pack_start(cancel_button, True, False, 4)

   ref_chain_hbox.pack_start(chain_id_ref_label, False, False, 2)
   ref_chain_hbox.pack_start(chain_id_ref_entry, False, False, 2)

   specs_hbox.pack_start(chain_id_lig_label, False, False, 2)
   specs_hbox.pack_start(chain_id_lig_entry, False, False, 2)
   specs_hbox.pack_start(resno_start_label, False, False, 2)
   specs_hbox.pack_start(resno_start_entry, False, False, 2)
   specs_hbox.pack_start(to_label, False, False, 2)
   specs_hbox.pack_start(resno_end_entry, False, False, 2)
   specs_hbox.pack_start(gtk.Label(" "), False, False, 2) # neatness ?!

   chain_id_lig_entry.set_size_request(32, -1)
   chain_id_ref_entry.set_size_request(32, -1)
   resno_start_entry.set_size_request(50, -1)
   resno_end_entry.set_size_request(50, -1)
   chain_id_ref_entry.set_text("A")
   chain_id_lig_entry.set_text("A")
   resno_start_entry.set_text("1")

   tooltips = gtk.Tooltips()
   tooltips.set_tip(chain_id_ref_entry, "'A' is a reasonable guess at the NCS master chain id.  " +
                    "If your ligand (specified below) is NOT bound to the protein's " +
                    "'A' chain, then you will need to change this chain and also " +
                    "make sure that the master molecule is specified appropriately " +
                    "in the Draw->NCS Ghost Control window.")
   tooltips.set_tip(resno_end_entry, "Leave blank for a single residue")

   ok_button.connect("clicked", go_button_function)

   cancel_button.connect("clicked", delete_event)

   window.show_all()


# GUI for ligand superpositioning by graph matching
#
def superpose_ligand_gui():
   
   def delete_event(*args):
      window.destroy()
      return False

   def go_button_function(widget):
      active_mol_no_ref_lig = get_option_menu_active_molecule(*option_menu_ref_mol_pair)
      active_mol_no_mov_lig = get_option_menu_active_molecule(*option_menu_mov_mol_pair)
      chain_id_ref = chain_id_ref_entry.get_text()
      chain_id_mov = chain_id_mov_entry.get_text()
      resno_ref = False
      resno_mov = False
      try:
         resno_ref = int(resno_ref_entry.get_text())
      except:
         print "can't decode reference resno", resno_ref_entry.get_text()
         
      try:
         resno_mov = int(resno_mov_entry.get_text())
      except:
         print "can't decode moving resno", resno_mov_entry.get_text()
         

      if (resno_ref and resno_mov):
         overlay_my_ligands(active_mol_no_mov_lig, chain_id_mov, resno_mov,
                            active_mol_no_ref_lig, chain_id_ref, resno_ref)

      delete_event()
      
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   title = gtk.Label("Superpose Ligands")
   ligands_vbox = gtk.VBox(False, 2)
   ref_chain_hbox = gtk.HBox(False, 2)
   chain_id_ref_label = gtk.Label("Ligand Chain ID: ")
   chain_id_ref_entry = gtk.Entry()
   resno_ref_label    = gtk.Label(" Residue Number ") 
   resno_ref_entry = gtk.Entry()
   
   mov_chain_hbox = gtk.HBox(False, 2)
   chain_id_mov_label= gtk.Label("Ligand Chain ID: ")
   chain_id_mov_entry = gtk.Entry()
   resno_mov_label = gtk.Label(" Residue Number ")
   resno_mov_entry = gtk.Entry()

   h_sep = gtk.HSeparator()

   buttons_hbox = gtk.HBox(True, 6)

   ok_button = gtk.Button("   Superpose 'em  ")
   cancel_button = gtk.Button("    Cancel    ")

   window.add(ligands_vbox)
   ligands_vbox.pack_start(title, False, False, 6)
   option_menu_ref_mol_pair = generic_molecule_chooser(ligands_vbox, "Model with reference ligand")
   ligands_vbox.pack_start(ref_chain_hbox, False, False, 2)

   option_menu_mov_mol_pair = generic_molecule_chooser(ligands_vbox, "Model with moving ligand")   
   ligands_vbox.pack_start(mov_chain_hbox, False, False, 2)

   ligands_vbox.pack_start(h_sep, False, False, 2)
   ligands_vbox.pack_start(buttons_hbox, False, False, 2)

   buttons_hbox.pack_start(ok_button,     True, False, 4)
   buttons_hbox.pack_start(cancel_button, True, False, 4)

   ref_chain_hbox.pack_start(chain_id_ref_label, False, False, 2)
   ref_chain_hbox.pack_start(chain_id_ref_entry, False, False, 2)
   ref_chain_hbox.pack_start(resno_ref_label, False, False, 2)
   ref_chain_hbox.pack_start(resno_ref_entry, False, False, 2)

   mov_chain_hbox.pack_start(chain_id_mov_label, False, False, 2)
   mov_chain_hbox.pack_start(chain_id_mov_entry, False, False, 2)
   mov_chain_hbox.pack_start(resno_mov_label, False, False, 2)
   mov_chain_hbox.pack_start(resno_mov_entry, False, False, 2)

#   chain_id_lig_entry.set_size_request(32, -1)
#   chain_id_ref_entry.set_size_request(32, -1)
#   resno_start_entry.set_size_request(50, -1)
#   resno_end_entry.set_size_request(50, -1)
#   chain_id_ref_entry.set_text("A")
#   chain_id_lig_entry.set_text("A")
#   resno_start_entry.set_text("1")

#   tooltips = gtk.Tooltips()
#   tooltips.set_tip(chain_id_ref_entry, "'A' is a reasonable guess at the NCS master chain id.  " +
#                    "If your ligand (specified below) is NOT bound to the protein's " +
#                    "'A' chain, then you will need to change this chain and also " +
#                    "make sure that the master molecule is specified appropriately " +
#                    "in the Draw->NCS Ghost Control window.")
#   tooltips.set_tip(resno_end_entry, "Leave blank for a single residue")

   ok_button.connect("clicked", go_button_function)

   cancel_button.connect("clicked", delete_event)

   window.show_all()

global std_key_bindings
std_key_bindings = [["^g", "keyboard-go-to-residue"],
                    ["a", "refine with auto-zone"],
                    ["b", "toggle baton swivel"],
                    ["c", "toggle cross-hairs"],
                    ["d", "reduce depth of field"],
                    ["f", "increase depth of field"],
                    ["u", "undo last navigation"],
                    ["i", "toggle spin mode"],
                    ["l", "label closest atom"],
                    ["m", "zoom out"],
                    ["n", "zoom in"],
                    ["o", "other NCS chain"],
                    ["p", "update position to closest atom"],
                    ["s", "update skeleton"],
                    [".", "up in button list"],
                    [",", "down in button list"]]
   
def key_bindings_gui():

   global std_key_bindings
   def delete_event(*args):
      window.destroy()
      return False

   def box_for_binding(item, inside_vbox, buttonize_flag):
      binding_hbox = gtk.HBox(False, 2)
      txt = str(item[1])
      key_label = gtk.Label("   " + txt + "   ")
      name_label = gtk.Label(item[2])

      if (buttonize_flag):
         button_label = "   " + txt + "   " + item[2]
         button = gtk.Button(button_label)
         #al = gtk.Alignment(0, 0, 0, 0)
         #label = gtk.Label(button_label)
         #button.add(al)
         #al.add(label)
         binding_hbox.pack_start(button, True, True, 0)
         inside_vbox.pack_start(binding_hbox, False, False, 0)
         button.connect("clicked", lambda *args: apply(item[3]))
         
      else:
         binding_hbox.pack_start(key_label, False, False, 2)
         binding_hbox.pack_start(name_label, False, False, 2)
         inside_vbox.pack_start(binding_hbox, False, False, 2)
      
   # main line
   #
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   scrolled_win = gtk.ScrolledWindow()
   outside_vbox = gtk.VBox(False, 2)
   inside_vbox = gtk.VBox(False, 0)
   dialog_name = "Key Bindings"
   buttons_hbox = gtk.HBox(False, 2)
   close_button = gtk.Button("  Close  ")
   std_frame = gtk.Frame("Standard Key Bindings:")
   usr_frame = gtk.Frame("User-defined Key Bindings:")
   std_frame_vbox = gtk.VBox(False, 2)
   usr_frame_vbox = gtk.VBox(False, 2)
   close_button.connect("clicked", delete_event)

   window.set_default_size(250, 350)
   window.set_title(dialog_name)
   inside_vbox.set_border_width(4)

   window.add(outside_vbox)
   outside_vbox.add(scrolled_win)
   scrolled_win.add_with_viewport(inside_vbox)
   scrolled_win.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)

   inside_vbox.pack_start(std_frame, False, False, 2)
   inside_vbox.pack_start(usr_frame, False, False, 2)
   
   std_frame.add(std_frame_vbox)
   usr_frame.add(usr_frame_vbox)

   py_and_scm_keybindings = key_bindings
   if (coot_has_guile()):
      scm_key_bindings = run_scheme_command("*key-bindings*")
      # filter out doublicates
      for item in scm_key_bindings:
         print "BL DEBUG:: item", item
         scm_code, scm_key, text, tmp = item
         py_keys  = [elem[1] for elem in py_and_scm_keybindings]
         py_codes = [elem[0] for elem in py_and_scm_keybindings]
         print "BL DEBUG:: py_keys and py_codes", py_keys, py_codes
         if ((not scm_code in py_codes) and (not scm_key in py_keys)):
            py_and_scm_keybindings.append(item)

      py_and_scm_keybindings += scm_key_bindings
      
   for items in py_and_scm_keybindings:
      box_for_binding(items, usr_frame_vbox, True)

   for items in std_key_bindings:
      box_for_binding(["dummy"] + items, std_frame_vbox, False)

   buttons_hbox.pack_end(close_button, False, False, 6)
   outside_vbox.pack_start(buttons_hbox, False, False, 6)

   window.show_all()

# for news infos
(
   INSERT_NO_NEWS,
   INSERT_NEWS,
   STATUS,
   NEWS_IS_READY,
   GET_NEWS,
   SET_TEXT,
   STOP,
   NO_NEWS,
   ) = range(8)

global news_status
news_status = NO_NEWS
global news_string_1
global news_string_2
news_string_1 = False
news_string_2 = False

def coot_news_info(*args):

   import threading
   import urllib
   global text_1, text_2
   url = "http:" + \
         "//www.biop.ox.ac.uk/coot/software" + \
         "/source/pre-releases/release-notes"

   def test_string():
      import time
      time.sleep(2)
      ret = "assssssssssssssssssssssssssssssssssssssss\n\n" + \
            "assssssssssssssssssssssssssssssssssssssss\n\n" + \
            "assssssssssssssssssssssssssssssssssssssss\n\n" + \
            "\n-----\n" + \
            "bill asssssssssssssssssssssssssssssssssssss\n\n" + \
            "fred asssssssssssssssssssssssssssssssssssss\n\n" + \
            "george sssssssssssssssssssssssssssssssssssss\n\n" + \
            "\n-----\n"
      return ret

   def stop():
      return

   # return [pre_release_news_string, std_release_news_string]
   def trim_news(s):
      sm_pre = s.find("-----")
      if (sm_pre == -1):
         return ["nothing", "nothing"]
      else:
         pre_news = s[0:sm_pre]
         post_pre = s[sm_pre:-1].lstrip("-")
         sm_std = post_pre.find("-----")
         if (sm_std == -1):
            return [pre_news, "nothing"]
         else:
            return [pre_news, post_pre[0:sm_std]]

   def get_news_thread():
      global news_status
      global news_string_1
      global news_string_2
      import urllib
      try:
         s = urllib.urlopen(url).read()
         both_news = trim_news(s)
         news_string_1 = both_news[1]
         news_string_2 = both_news[0]
         news_status = NEWS_IS_READY
      except:
         pass

   def coot_news_error_handle(key, *args):
      # not used currently
      print "error: news: error in %s with args %s" %(key, args)

   def get_news():
      # no threading for now. Doesnt do the right thing
      run_python_thread(get_news_thread, ())
      
   def insert_string(s, text):
      background_colour = "#c0e6c0"
      end = text.get_end_iter()
      text.insert(end, str(s))

   def insert_news():
      global news_string_1
      global news_string_2
      insert_string(news_string_1, text_1)
      insert_string(news_string_2, text_2)
      
   def insert_no_news():
      insert_string("  No news\n", text_1)
      insert_string("  Yep - there really is no news\n", text_2)

   if (len(args) == 1):
      arg = args[0]
      if (arg == STOP):
         stop()
      if (arg == STATUS):
         return news_status
      if (arg == INSERT_NEWS):
         insert_news()
      if (arg == INSERT_NO_NEWS):
         insert_no_news()
      if (arg == GET_NEWS):
         get_news()
   if (len(args) == 3):
      if (args[0] == SET_TEXT):
         text_1 = args[1]
         text_2 = args[2]

def whats_new_dialog():
   global text_1, text_2
   text_1 = False         # the text widget
   text_2 = False
   timer_label = False
   global count
   count = 0
   ms_step = 200

   def on_close_button_clicked(*args):
      coot_news_info(STOP)
      delete_event(*args)
      
   def delete_event(*args):
      window.destroy()
      return False
   
   def check_for_new_news():
      global count
      count += 1
      timer_string = str(count * ms_step / 1000.) + "s"
      timer_label.set_alignment(0.96, 0.5)
      timer_label.set_text(timer_string)
      #if (count > 100):
      if (count > 20):
         timer_label.set_text("Timeout")
         coot_news_info(INSERT_NO_NEWS)
         return False  # turn off the gtk timeout function ?!
      else:
         if (coot_news_info(STATUS) == NEWS_IS_READY):
            coot_news_info(INSERT_NEWS)
            return False
         return True

   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   vbox = gtk.VBox(False, 2)
   inside_vbox = gtk.VBox(False, 2)
   scrolled_win_1 = gtk.ScrolledWindow()
   scrolled_win_2 = gtk.ScrolledWindow()
   label = gtk.Label("Lastest Coot Release Info")
   text_view_1 = gtk.TextView()
   text_view_2 = gtk.TextView()
   text_view_1.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse("#bfe6bf"))
   text_view_2.modify_base(gtk.STATE_NORMAL, gtk.gdk.color_parse("#bfe6bf"))
   text_1 = text_view_1.get_buffer()
   text_2 = text_view_2.get_buffer()
   h_sep = gtk.HSeparator()
   close_button = gtk.Button("   Close   ")
   notebook = gtk.Notebook()
   notebook_label_pre = gtk.Label("Pre-release")
   notebook_label_std = gtk.Label("Release")
   timer_label = gtk.Label("0.0s")

   window.set_default_size(540, 400)
   vbox.pack_start(label, False, False, 10)
   vbox.pack_start(timer_label, False, False, 2)
   vbox.pack_start(notebook, True, True, 4)
   notebook.append_page(scrolled_win_1, notebook_label_std)
   notebook.append_page(scrolled_win_2, notebook_label_pre)
   vbox.pack_start(h_sep, False, False, 4)
   vbox.pack_start(close_button, False, False, 2)
   window.add(vbox)

   coot_news_info(SET_TEXT, text_1, text_2)
   coot_news_info(GET_NEWS)

   scrolled_win_1.add(text_view_1)
   scrolled_win_2.add(text_view_2)
   scrolled_win_1.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)
   scrolled_win_2.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)

   close_button.connect("clicked", on_close_button_clicked)

   gobject.timeout_add(ms_step, check_for_new_news)

   window.show_all()
   

# Cootaneer/sequencing gui modified by BL with ideas from KC
# based on Paul's cootaneer gui and generic_chooser_entry_and_file_selector
def cootaneer_gui_bl():

	# unfortunately currently I dont see a way to avoid a global variable here
	# contains flag if file was imported and no of sequences
	global imported_sequence_file_flags
	imported_sequence_file_flags = [False, 0]

	def delete_event(*args):
		window.destroy()
		return False

	def refine_function_event(widget):
		# doesnt need to do anything
		status = refine_check_button.get_active()
		if (status):
			print "INFO:: refinement on"
		else:
			print "INFO:: refinement off"

	def go_function_event(widget):
		print "apply the sequence info here\n"
		print "then cootaneer\n"

		# no active atom won't do.  We need
		# to find the nearest atom in imol to (rotation-centre).
		#
		# if it is too far away, give a
		# warning and do't do anything.
		active_mol_no = get_option_menu_active_molecule(option_menu, model_mol_list)
		imol = int(active_mol_no)
		imol_map = imol_refinement_map()

		do_it = assign_sequences_to_mol(imol)

		if (do_it):
			# now cootaneer it
			chain_ls = chain_ids(imol)
			for chain_id in chain_ls:
				res_name = resname_from_serial_number(imol, chain_id, 0)
				res_no = seqnum_from_serial_number(imol, chain_id, 0)
				ins_code = insertion_code_from_serial_number(imol, chain_id, 0)
				alt_conf = ""
				at_name = residue_spec2atom_for_centre(imol, chain_id, res_no, ins_code)[0]
				cootaneer(imol_map, imol, [chain_id, res_no, ins_code, 
							   at_name, alt_conf])
				
				refine_qm = refine_check_button.get_active()
			# refine?
			window.hide()
			if (refine_qm):
				fit_protein(imol)
					
			delete_event()


        def import_function_event(widget, selector_entry):
		# we import a sequence file and update the cootaneer table
		global imported_sequence_file_flags
		imported_sequence_file_qm = imported_sequence_file_flags[0]
		active_mol_no = get_option_menu_active_molecule(option_menu, model_mol_list)
		imol = int(active_mol_no)
		
		seq_info_ls = []
		seq_file_name = selector_entry.get_text()
		if (seq_file_name):
		    # get and set sequence info
		    assign_sequence_from_file(imol, str(seq_file_name))
		    seq_info_ls = sequence_info(imol)
		    no_of_sequences = len(seq_info_ls)

		    # remove children if new file
		    if not imported_sequence_file_qm:
			    table_children = seq_table.get_children()
			    for child in table_children:
				    seq_table.remove(child)
			    widget_range = range(no_of_sequences)
		    else:
			    # we update the number of sequences
			    spin_len = int(spin_button.get_value())
			    widget_range = range(spin_len, no_of_sequences)
			    
		    # make new table
		    imported_sequence_file_flags = [True, no_of_sequences]
		    spin_button.set_value(no_of_sequences)
		    seq_table.resize(no_of_sequences, 1)
		    for i in widget_range:
			    seq_widget = entry_text_pair_frame_with_button(seq_info_ls[i])
			    seq_table.attach(seq_widget[0], 0, 1, i, i+1)
			    seq_widget[0].show_all()
		else:
			print "BL WARNING:: no filename"

	def fill_table_with_sequences(*args):
		# fills the table with sequences if they have been associated with the model imol
		# already
		global imported_sequence_file_flags
		active_mol_no = get_option_menu_active_molecule(option_menu, model_mol_list)
		imol = int(active_mol_no)
		seq_info_ls = sequence_info(imol)
		if (seq_info_ls):
			# we have a sequence and fill the table
			no_of_sequences = len(seq_info_ls)
			imported_sequence_file_flags = [True, no_of_sequences]
			spin_button.set_value(no_of_sequences)
			seq_table.resize(no_of_sequences, 1)
			# remove existing children
			table_children = seq_table.get_children()
			for child in table_children:
				seq_table.remove(child)
			widget_range = range(no_of_sequences)
			for i in widget_range:
				seq_widget = entry_text_pair_frame_with_button(seq_info_ls[i])
				seq_table.attach(seq_widget[0], 0, 1, i, i+1)
				seq_widget[0].show_all()
		else:
			# no sequence information, reset the table
			clear_function_event()
			


	def add_text_to_text_buffer(text_buffer, description):
		start = text_buffer.get_start_iter()
		text_buffer.create_tag("tag", foreground="black", 
			background = "#c0e6c0")
		text_buffer.insert_with_tags_by_name(start, description, "tag")

	# return the (entry . textbuffer/box)
	#
	def entry_text_pair_frame_with_button(seq_info):
           
           def fragment_go_event(widget):
		active_mol_no = get_option_menu_active_molecule(option_menu, model_mol_list)
		imol = int(active_mol_no)
		imol_map = imol_refinement_map()
		print "apply the sequence info here\n"
		print "then cootaneer\n"

		# no active atom won't do.  We need
		# to find the nearest atom in imol to (rotation-centre).
		#
		# if it is too far away, give a
		# warning and do't do anything.

		do_it = assign_sequences_to_mol(imol)
		if (do_it):

			n_atom = closest_atom(imol)
			if n_atom:
				imol	= n_atom[0]
				chain_id= n_atom[1]
				res_no	= n_atom[2]
				ins_code= n_atom[3]
				at_name	= n_atom[4]
				alt_conf= n_atom[5]
				cootaneer(imol_map, imol, [chain_id, res_no, ins_code, 
							   at_name, alt_conf])
				refine_qm = refine_check_button.get_active()
				if (chain_check_button.get_active()):
					# we try to change the chain
					from_chain_id = chain_id
					to_chain_id = entry.get_text()
					start_resno = seqnum_from_serial_number(imol, from_chain_id, 0)
					end_resno = seqnum_from_serial_number(imol, from_chain_id, (chain_n_residues(from_chain_id, imol) - 1))
					[istat, message] = change_chain_id_with_result_py(imol, from_chain_id, to_chain_id, 1, start_resno, end_resno)
					if (istat == 1):
						# renaming ok
						chain_id = to_chain_id
					else:
						# renaming failed
						if (refine_qm):
							message += "\nRefinement proceeded with old=new chain "
							message += chain_id
						info_dialog(message)
					
				if (refine_qm):
					fit_chain(imol, chain_id)
			else:
				print "BL WARNING:: no close atom found!"
		else:
			print "BL ERROR:: something went wrong assigning the sequence"

	   def chain_toggled(widget):
		   # doesnt need to do anything
		   status = chain_check_button.get_active()
		   if(status):
			   print "BL INFO:: assign chain_id too"
		   else:
			   print "BL INFO:: do not assign chain_id"
               
           frame = gtk.Frame()
           vbox = gtk.VBox(False, 3)
           hbox = gtk.HBox(False, 3)
           entry = gtk.Entry()
           textview = gtk.TextView()
           textview.set_wrap_mode(gtk.WRAP_WORD_CHAR)
	   textview.set_editable(True)
	   textview.set_size_request(300, -1)
	   textview.modify_font(pango.FontDescription("Courier 11"))
           text_buffer = textview.get_buffer()
           chain_id_label = gtk.Label("Chain ID")
           sequence_label = gtk.Label("Sequence")
	   vbox_for_buttons = gtk.VBox(False, 3)
           fragment_button = gtk.Button("  Sequence closest fragment  ")
	   chain_check_button = gtk.CheckButton("Assign Chain ID as well?")
           
           frame.add(hbox)
           vbox.pack_start(chain_id_label, False, False, 2)
           vbox.pack_start(entry, False, False, 2)
           vbox.pack_start(sequence_label, False, False, 2)
           vbox.pack_start(textview, True, False, 2)
           add_text_to_text_buffer(text_buffer, seq_info[1])
           entry.set_text(seq_info[0])
           hbox.pack_start(vbox, False, False, 2)
	   vbox_for_buttons.pack_start(chain_check_button, False, False, 6)
	   vbox_for_buttons.pack_start(fragment_button, False, False, 6)
           hbox.pack_start(vbox_for_buttons, False, False, 2)
              
           fragment_button.connect("clicked", fragment_go_event)
	   chain_check_button.connect("toggled", chain_toggled)
              
           return [frame, entry, text_buffer]

        # redraw the table when spin_button is changed
        def spin_button_changed(widget):
		global imported_sequence_file_flags
		no_of_frames = int(spin_button.get_value())
		imported_sequence_file_qm = imported_sequence_file_flags[0]
		no_of_sequences = imported_sequence_file_flags[1]

		# get table information
		table_children = seq_table.get_children()
		no_of_children = len(table_children)

		# make range for redraw
		if (imported_sequence_file_qm):
			# we only make extra ones
			redraw_range = range(no_of_sequences, no_of_frames)
			if (no_of_sequences > no_of_frames):
				child_range = []    # do not remove children
				redraw_range = []   # do not redraw
				spin_button.set_value(no_of_sequences)
				no_of_frames = no_of_sequences
			else:
				if (no_of_children == no_of_sequences):
					# do not remove any
					child_range = []
				else:
					child_range = range(0, no_of_children - no_of_sequences)
					# children seem to be added in the beginning ???????
		else:
			# we make everything new
			redraw_range = range(no_of_frames)
			child_range = range(no_of_children)
			
		# lets remove the children
		for j in child_range:
			child = table_children[j]
			seq_table.remove(child)
			
		# make new cells
		for i in redraw_range:
			make_cell(i)

		# resize the table
		seq_table.resize(no_of_frames, 1)

	# reset the table
	def clear_function_event(widget = None, file_sel_entry = None):
		global imported_sequence_file_flags
		imported_sequence_file_flags = [False, 0]
		seq_table.resize(1, 1)
		# remove children
		table_children = seq_table.get_children()
		for child in table_children:
			seq_table.remove(child)
		# make new
		make_cell(0)
		spin_button.set_value(1)
                # reset the filechooser entry
                if file_sel_entry:
                   file_sel_entry.set_text("")

	# make one cell in line with deafult fill
	def make_cell(line):
		seq_widget = entry_text_pair_frame_with_button(["",
                                                                "Cut and Paste Sequence to here or import a sequence file"])
		seq_table.attach(seq_widget[0], 0, 1, line, line + 1)
		seq_widget[0].show_all()
		
	# assign the in table given sequences to the model imol
	def assign_sequences_to_mol(imol):
		no_of_seq = int(spin_button.get_value())
		frame_list = seq_table.get_children()
		seq_all = []
		write_sequence = True
		for i in range(no_of_seq):
			fframe = frame_list[i]
			hhbox = fframe.get_children()[0]
			vvbox = hhbox.get_children()[0]
			child_list = vvbox.get_children()
			chain_id = child_list[1].get_text()
			seq_textbuffer = child_list[3].get_buffer()
			startiter, enditer = seq_textbuffer.get_bounds() 
			seq_in = seq_textbuffer.get_text(startiter, enditer)
			pair = [chain_id, seq_in]
			if (len(chain_id) == 0) or (
			    "Cut and Paste Sequence to here or import a sequence file") in pair:
				print "ERROR:: the input contains an invalid chain and/or sequence"
				write_sequence = False
			seq_all.append(pair)

		if (write_sequence):
			for element in seq_all:
				chain_id_new = element[0]
				seq = element[1].upper()
				# first check if chain_id is already in mol
				# if so delete it so that it can be replaced by the new sequence
				seq_info = sequence_info(imol)
				for info in seq_info:
					chain_id_old = info[0]
					if (chain_id_new == chain_id_old):
						delete_sequence_by_chain_id(imol, chain_id_old)
				
				# replace space, eol etc in sequence first
				seq = seq.replace(" ", "")
				seq = seq.replace("\r", "")       # mac?
				seq = seq.replace("\r\n", "")     # win?
				seq = seq.replace("\n", "")       # unix?
				assign_sequence_from_string(imol, chain_id_new, seq)
			return True
		else:
			add_status_bar_text("Invalid chain_id and/or sequence provided")
			return False


	# main body
	imol_map = imol_refinement_map()
	if (imol_map == -1):
		show_select_map_dialog()
	
	window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        window.set_title("Sequencing GUI")
        tooltips = gtk.Tooltips()
        label = gtk.Label("Molecule to be sequenced")
	vbox = gtk.VBox(False, 2)
        option_menu = gtk.combo_box_new_text()
        model_mol_list = fill_option_menu_with_mol_options(option_menu, valid_model_molecule_qm)
	inside_vbox = gtk.VBox(False, 2)
	seq_table = gtk.Table(1, 1, True)
        hbox_for_spin = gtk.HBox(False, 0)
        spin_label = gtk.Label("Number of Sequences:")
        spin_adj = gtk.Adjustment(1, 1, 10, 1, 4, 0)
        spin_button = gtk.SpinButton(spin_adj, 0, 0)
	refine_check_button = gtk.CheckButton("Auto-fit-refine after sequencing?")
	h_sep = gtk.HSeparator()
	h_sep2 = gtk.HSeparator()
	buttons_hbox = gtk.HBox(False, 2)
        import_button = gtk.Button("  Import and associate sequence from file  ")
	go_button = gtk.Button("  Sequence all fragments!  ")
        tooltips.set_tip(go_button, "This currently ignores all chain IDs")
	cancel_button = gtk.Button("  Cancel  ")
	clear_button = gtk.Button("  Clear all  ")

        window.set_default_size(400, 200)
        window.add(vbox)
        vbox.pack_start(label, False, False, 5)
        vbox.pack_start(option_menu, False, False, 0)

        hbox_for_spin.pack_start(spin_label, False, False, 2)
        hbox_for_spin.pack_start(spin_button, False, False, 2)
	hbox_for_spin.pack_end(refine_check_button, False, False, 2)
        vbox.pack_start(hbox_for_spin, False, False, 5)

        vbox.pack_start(inside_vbox, False, False, 2)
        inside_vbox.add(seq_table)
	make_cell(0)
	fill_table_with_sequences()
	
        vbox.pack_start(h_sep, False, False, 2)
        file_sel_entry = file_selector_entry(vbox, "Select PIR file")
        vbox.pack_start(import_button, False, False, 6)

	buttons_hbox.pack_start(go_button, False, False, 6)
	buttons_hbox.pack_start(cancel_button, False, False, 6)
	buttons_hbox.pack_start(clear_button, False, False, 6)

        vbox.pack_start(h_sep2, False, False, 2)
        vbox.pack_start(buttons_hbox, False, False, 5)


        import_button.connect("clicked", import_function_event, file_sel_entry)

	cancel_button.connect("clicked", delete_event)

	go_button.connect("clicked", go_function_event)

	clear_button.connect("clicked", clear_function_event, file_sel_entry)
                               
	spin_adj.connect("value_changed", spin_button_changed)

	refine_check_button.connect("toggled", refine_function_event)

	option_menu.connect("changed", fill_table_with_sequences)

#        window.add(vbox)
	window.show_all()

# a function to run a pygtk widget in a function as a thread
#
def run_with_gtk_threading(function, *args):
   import gobject
   def idle_func():
      gtk.gdk.threads_enter()
      try:
         # function(*args, **kw)
         function(*args)
         return False
      finally:
         gtk.gdk.threads_leave()
   gobject.idle_add(idle_func)


def generic_check_button(vbox, label_text, handle_check_button_function):
   def check_callback(*args):
      active_state = check_button.get_active()
      set_state = 0
      if (active_state):
         set_state = 1
      handle_check_button_function(set_state)
   check_button = gtk.CheckButton(label_text)
   vbox.pack_start(check_button, False, False, 2)
   check_button.connect("toggled", check_callback)
   return check_button

# a master gui to set all kinds of refinement parameters
#
def refinement_options_gui():

   def set_matrix_func(button, entry):
      text = entry.get_text()
      try:
         t = float(text)
         s = "Matrix set to " + text
         set_matrix(t)
         add_status_bar_text(s)
      except:
         add_status_bar_text("Failed to read a number") 

   def delete_event(*rags):
      window.destroy()
      return False

   def go_function_event(*args):
      set_matrix_func('dummy', matrix_entry)
      window.destroy()
      return False

   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   vbox = gtk.VBox(False, 0)
   hbox = gtk.HBox(False, 0)
   h_sep = gtk.HSeparator()
   h_sep2 = gtk.HSeparator()
   h_sep3 = gtk.HSeparator()
   go_button = gtk.Button("   Ok   ")
   cancel_button = gtk.Button("  Cancel  ")

   window.add(vbox)
   # add the matrix entry
   matrix_entry = entry_do_button(vbox, "set matrix: (smaller means better geometry)",
                                  "Set", set_matrix_func)
   matrix_entry.set_text(str(matrix_state()))

   vbox.pack_start(h_sep2, False, False, 2)

   # use torsion restrains?
   torsion_restraints_button = generic_check_button(vbox,
                                                 "Use Torsion Restraints?",
                                                 lambda state: set_refine_with_torsion_restraints(state))
   if (refine_with_torsion_restraints_state() == 1):
      torsion_restraints_button.set_active(True)
   else:
      torsion_restraints_button.set_active(False)

   # planar peptide restrains?
   planar_restraints_button = generic_check_button(vbox,
                                                 "Use Planar Peptide Restraints?",
                                                 lambda state: remove_planar_peptide_restraints() if state == 0 else add_planar_peptide_restraints())
   if (planar_peptide_restraints_state() == 1):
      planar_restraints_button.set_active(True)
   else:
      planar_restraints_button.set_active(False)

   # use ramachandran restrains?
   rama_restraints_button = generic_check_button(vbox,
                                                 "Use Ramachandran Restraints?",
                                                 lambda state: set_refine_ramachandran_angles(state))
   if (refine_ramachandran_angles_state() == 1):
      rama_restraints_button.set_active(True)
   else:
      rama_restraints_button.set_active(False)      

   vbox.pack_start(h_sep3, False, False, 2)

   # add rotamer check button
   rotamer_autofit_button = generic_check_button(vbox,
                                                 "Real Space Refine after Auto-fit Rotamer?",
                                                 lambda state: set_rotamer_auto_fit_do_post_refine(state))
   if (rotamer_auto_fit_do_post_refine_state() == 1):
      rotamer_autofit_button.set_active(True)
   else:
      rotamer_autofit_button.set_active(False)

   # add mutate check button
   mutate_autofit_button = generic_check_button(vbox,
                                                "Real Space Refine after Mutate and Auto-fit?",
                                                lambda state: set_mutate_auto_fit_do_post_refine(state))
   if (mutate_auto_fit_do_post_refine_state() == 1):
      mutate_autofit_button.set_active(True)
   else:
      mutate_autofit_button.set_active(False)

   # add terminal residue check button
   terminal_autofit_button = generic_check_button(vbox,
                                                  "Real Space Refine after Add Terminal Residue?",
                                                  lambda state: set_add_terminal_residue_do_post_refine(state))
   if (add_terminal_residue_do_post_refine_state() == 1):
      terminal_autofit_button.set_active(True)
   else:
      terminal_autofit_button.set_active(False)

   # add a b-factor button
   reset_b_factor_button = generic_check_button(vbox,
                                                "Reset B-Factors to Default Value after Refinement/Move?",
                                                lambda state: set_reset_b_factor_moved_atoms(state))
   if (get_reset_b_factor_moved_atoms_state() == 1):
      reset_b_factor_button.set_active(True)
   else:
      reset_b_factor_button.set_active(False)

   vbox.pack_start(h_sep, False, False, 2)
   vbox.pack_start(hbox, False, False, 0)
   hbox.pack_start(go_button, False, False, 6)
   hbox.pack_start(cancel_button, False, False, 6)

   go_button.connect("clicked", go_function_event)
   cancel_button.connect("clicked", delete_event)
   window.show_all()

# a simple window to show a progress bar
# return the window (to be destroyed elsewhere)
#
def show_progressbar(text):

   gtk.gdk.threads_init()
   def progress_run(pbar):
      pbar.pulse()
      return True

   def destroy_cb(widget, timer):
      gobject.source_remove(timer)
      timer = 0
      gtk.main_quit()
      return False
   
   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   window.set_title("External Program Progress")
   window.set_border_width(0)
   window.set_default_size(300, 50)

   vbox = gtk.VBox(False, 5)
   vbox.set_border_width(10)
   window.add(vbox)

   pbar = gtk.ProgressBar()
   pbar.pulse()
   pbar.set_text(text)
   vbox.pack_start(pbar, False, False, 5)

   timer = gobject.timeout_add (100, progress_run, pbar)
   
   window.connect("destroy", destroy_cb, timer)

   window.show_all()
   global python_return
   python_return = window
   gtk.main()

import threading
# helper function to push the python threads
# this only runs python threads for 20ms every 50ms
def python_thread_sleeper():
   global python_thread_sleep
   sleep_time = python_thread_sleep
   time.sleep(sleep_time / 1000.)
   if (threading.activeCount() == 1):
     #print "BL DEBUG:: stopping timeout"
     return False
   return True

# this has locked, so that no one else can use it
global python_thread_return
python_thread_return = False
#global python_thread_sleep
#python_thread_sleep = 20

# function to run a python thread with function using
# args which is a tuple
# optionally pass sleep time in ms (default is 20) - usefull
# for computationally expensive threads which may have run longer
# N.B. requires gobject hence in coot_gui.py
#
def run_python_thread(function, args, sleep_time=20):

   import gobject
   
   class MyThread(threading.Thread):
      def __init__(self):
         threading.Thread.__init__(self)
      def run(self):
         global python_thread_return
         python_return_lock = threading.Lock()
         python_return_lock.acquire()
         try:
            python_thread_return = function(*args)
         finally:
            python_return_lock.release()

   global python_thread_sleep
   if (not sleep_time == 20):
      python_thread_sleep = sleep_time
   if (threading.activeCount() == 1):
      gobject.timeout_add(50, python_thread_sleeper)
   MyThread().start()


def map_sharpening_gui(imol):

   window = gtk.Window(gtk.WINDOW_TOPLEVEL)
   vbox = gtk.VBox(False, 2)
   hbox = gtk.HBox(False, 2)
   adj = gtk.Adjustment(0.0, -30, 60, 0.05, 2, 30.1)
   slider = gtk.HScale(adj)
   label = gtk.Label("\nSharpen Map:")
   lab2  = gtk.Label("Add B-factor: ")

   vbox.pack_start(label,  False, False, 2)
   vbox.pack_start(hbox,   False, False, 2)
   hbox.pack_start(lab2,   False, False, 2)
   hbox.pack_start(slider, True,  True,  2)
   window.add(vbox)
   window.set_size_request(500, 100)
   # slider.add_mark(-30, -30, 0) # not yet

   adj.connect("value_changed", lambda func: sharpen(imol, adj.value))
   
   window.show_all()

# let the c++ part of mapview know that this file was loaded:
set_found_coot_python_gui()
