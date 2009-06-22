# Copyright 2007 by Bernhard Lohkamp
# Copyright 2006, 2007 by The University of York

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
# Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA

import pygtk, gtk, pango
import time

  
have_coot_python = False
try: 
  import coot_python
  have_coot_python = True
except:
  print """BL WARNING:: could not import coot_python module!!
Some things, esp. extensions, may be crippled!"""

# put all coot svgs in the default icon set
def register_coot_icons():
  import glob
  iconfactory = gtk.IconFactory()
  stock_ids   = gtk.stock_list_ids()
  pixbuf_dir = os.getenv('COOT_PIXMAPS_DIR')
  if (not pixbuf_dir):
    pixbuf_dir = os.path.join(get_pkgdatadir(), "pixmaps")
  patt = os.path.normpath(pixbuf_dir + '/*.svg')
  coot_icon_filename_ls = glob.glob(patt)
  icon_info_ls = []
  for full_name in coot_icon_filename_ls:
    name = os.path.basename(full_name)
    icon_info_ls.append([name, full_name])
  for stock_id, filename in icon_info_ls:
    # only load image files when our stock_id is not present
    if ((stock_id not in stock_ids) and not
        ('phenixed' in filename)):
      if os.path.isfile(filename):
        pixbuf = gtk.gdk.pixbuf_new_from_file(filename)
        iconset = gtk.IconSet(pixbuf)
        iconfactory.add(stock_id, iconset)
  iconfactory.add_default()

if (have_coot_python):

  if coot_python.main_toolbar():

    register_coot_icons()

    def activate_menuitem(widget, item):
      try:
        apply(item[1])
      except:
        print "BL INFO:: unable to execute function", item[1]

    # takes list with [["item_name", func],...]
    def create_show_pop_menu(item_n_funcn_list, event):
      menu = gtk.Menu()
      for item in item_n_funcn_list:
        item_name = item[0]
        #item_func = item[1]
        menuitem = gtk.MenuItem(item_name)
        menu.append(menuitem)
        menuitem.connect("activate", activate_menuitem, item)
      menu.show_all()
      menu.popup(None, None, None, event.button, event.time)

    def make_icons_model():
      import os, glob
      stock_icon_ls = gtk.stock_list_ids()
      # coot icons
      pixbuf_dir = os.getenv('COOT_PIXMAPS_DIR')
      if (not pixbuf_dir):
        pixbuf_dir = os.path.join(get_pkgdatadir(), "pixmaps")
      patt = os.path.normpath(pixbuf_dir + '/*.svg')
      coot_icon_filename_ls = glob.glob(patt)

      model = gtk.ListStore(gtk.gdk.Pixbuf, str, str)
      for icon_filename in coot_icon_filename_ls:
        if (not (('phenixed' in icon_filename) or
                 ('coot-icon' in icon_filename))):
          if os.path.isfile(icon_filename):
            icon = os.path.basename(icon_filename)
            pixbuf = gtk.gdk.pixbuf_new_from_file(icon_filename)
            model.append([pixbuf, icon, icon_filename])

      # build in default gtk icons
      icon_theme = gtk.icon_theme_get_default()
      for icon in stock_icon_ls:
        try:
          pixbuf = icon_theme.load_icon(icon, 16, gtk.ICON_LOOKUP_USE_BUILTIN)
          model.append([pixbuf, icon, None])
        except:
          pass
      return model

    def icon_selection_combobox(model):
        
      combobox = gtk.ComboBox()
      combobox.set_wrap_width(10)
      combobox.set_model(model)
      crpx = gtk.CellRendererPixbuf()
      #crt  = gtk.CellRendererText()
      combobox.pack_start(crpx, False)
      combobox.add_attribute(crpx, 'pixbuf', 0)
      combobox.show_all()
      return combobox


    def make_toolbar_button_gui():

      def delete_event(*args):
        window.destroy()
        return False

      def go_function(*args):
        save_toolbuttons_qm = save_button.get_active()
        #print "do something, maybe, save state", save_toolbuttons_qm
        # iterate thru the widgets and get the state of check button and icon combobox
        frames = frame_vbox.get_children()
        for frame in frames:
          inner_vbox = frame.get_children()[0]
          hboxes = inner_vbox.get_children()
          for hbox in hboxes:
            left_hbox, right_hbox = hbox.get_children()
            check_button, description = left_hbox.get_children()
            label, combobox = right_hbox.get_children()
            model = combobox.get_model()
            button_label = check_button.get_label()
            if (check_button.get_active()):
              # make new button
              iter = combobox.get_active_iter()
              icon = None
              if (iter):
                icon, icon_stock, icon_filename = model.get(iter, 0, 1, 2)
                # print "icon", icon, icon_stock, icon_filename
                if (icon_stock):
                  icon = icon_stock
              toolbar_ls = list_of_toolbar_functions()
              for group in toolbar_ls:
                group_name = group[0]
                group_items = group[1:len(group)]
                for item in group_items:
                  if (len(item) >0):
                    check_button_label = item[0]
                    callback_function  = item[1]
                    description        = item[2]
                    if (check_button_label == button_label):
                      new_toolbutton = coot_toolbar_button(button_label, callback_function, icon)
                      # save
                      save_toolbar_to_init_file(button_label, callback_function, icon)
                      break
            else:
              # remove an existing button?
              # check if the button is in the existing button list
              for toolbar in toolbar_label_list():
                if (button_label == toolbar[0]):
                  coot_python.main_toolbar().remove(toolbar[1])
                  # remove from save
                  remove_toolbar_from_init_file(button_label)

        window.destroy()
        return False
          
      def save_function(*args):
        print "Save me"

      window = gtk.Window(gtk.WINDOW_TOPLEVEL)
      window.set_title("Toolbar Selection")
      window.set_default_size(600, 450)
      scrolled_win = gtk.ScrolledWindow()
      scrolled_win.set_policy(gtk.POLICY_AUTOMATIC,gtk.POLICY_ALWAYS)
      vbox = gtk.VBox(False, 2)
      frame_vbox = gtk.VBox(False, 2)
      h_sep = gtk.HSeparator()
      buttons_hbox = gtk.HBox(False, 2)
      cancel_button = gtk.Button("  Cancel  ")
      go_button = gtk.Button("   Apply   ")
      save_button = gtk.CheckButton(" Save to preferences?")
      save_button.set_active(True)
      icon_model = make_icons_model()
            
      toolbar_ls = list_of_toolbar_functions()

      for group in toolbar_ls:
        group_name = group[0]
        group_items = group[1:len(group)]
        
        inner_vbox = gtk.VBox(False, 2)
        frame = gtk.Frame(group_name)
        frame.add(inner_vbox)

        for item in group_items:
          if (len(item) >0):
            suggested_icon = False
            check_button_label = item[0]
            callback_function  = item[1]
            description        = item[2]
            if (len(item) > 3):
              suggested_icon     = item[3]
          
            def check_button_callback(*args):
              # print "check button toggled"
              # we actually dont do anything and check later for the status?
              pass

            hbox = gtk.HBox(False, 0)
            left_hbox = gtk.HBox(False, 0)
            right_hbox = gtk.HBox(False, 0)
            check_button = gtk.CheckButton(check_button_label)
            label = gtk.Label(description)
            icon_label = gtk.Label("Add icon?")
            icon_combobox = icon_selection_combobox(icon_model)
            button_icon = False

            all_toolbar_labels = map(lambda x: x[0], toolbar_label_list())
            if (check_button_label in all_toolbar_labels):
                check_button.set_active(True)
                # now try to get the icon
                try:
                  ls = toolbar_label_list()
                  button_icon = ls[all_toolbar_labels.index(check_button_label)][1].get_stock_id()
                except:
                  # no icon found
                  pass

            if (not button_icon):
              button_icon = suggested_icon

            # set the icon
            if (button_icon):
              for index in range(len(icon_model)):
                icon_iter = icon_model.get_iter(index)
                stock_id = icon_model.get_value(icon_iter, 1)
                if (stock_id == button_icon):
                  icon_combobox.set_active_iter(icon_iter)
                  break
            
            left_hbox.pack_start(check_button, True, True, 3)
            left_hbox.pack_end(label, True, True, 3)
            right_hbox.pack_start(icon_label, False, False, 3)
            right_hbox.pack_start(icon_combobox, False, False, 3)
            hbox.pack_start(left_hbox, True, True, 3)
            hbox.pack_end(right_hbox, False, False, 3)
            inner_vbox.pack_start(hbox, False, False, 3)
            check_button.connect("toggled", check_button_callback)

        frame_vbox.pack_start(frame, False, False, 3)

      frame_vbox.set_border_width(3)
      scrolled_win.add_with_viewport(frame_vbox)
      buttons_hbox.pack_start(go_button, True, False, 6)
      buttons_hbox.pack_start(cancel_button, True, False, 6)
      vbox.pack_start(scrolled_win, True, True, 0)
      vbox.pack_start(h_sep, False, False, 3)
      vbox.pack_start(save_button, False, False, 3)
      vbox.pack_start(buttons_hbox, False, False, 3)

      cancel_button.connect("clicked", delete_event)
      go_button.connect("clicked", go_function)
      save_button.connect("toggled", save_function)

      window.add(vbox)
      window.show_all()
      
    # simple GUI to remove a toolbar
    def remove_toolbar_button_gui():
      
      def remove_toolbar_button(entry_text):
        print "remove button", entry_text
        for toolbar_child in coot_main_toolbar.get_children():
          button_label = toolbar_child.get_label()
          if (button_label == entry_text):
            coot_main_toolbar.remove(toolbar_child)
            remove_toolbar_from_init_file(button_label)
            break
      generic_single_entry("Remove toolbar button",
                           "button label",
                           "Remove",
                           remove_toolbar_button)

    # GUI to add user-defined toolbars
    # uses Assistant or a simple GUI, depending on PyGTK version
    def add_toolbar_button_gui():
      major, minor, micro = gtk.pygtk_version
      if (major >= 2 and minor >= 10):
        # have assistant
        print "BL DEBUG:: run assistant"
        add_toolbar_button_assistant()
      else:
        # no assistant available -> simple GUI
        add_toolbar_button_simple_gui()
        print "BL DEBUG:: no assistant, so simple GUI"
      

    def toolbar_hide_text():
      coot_main_toolbar.set_style(gtk.TOOLBAR_ICONS)

    def toolbar_show_text():
      coot_main_toolbar.set_style(gtk.TOOLBAR_BOTH_HORIZ)

    # an assistant to add a toolbutton
    def add_toolbar_button_assistant():

      def cb_close(assistant):
        assi.destroy()
        return False

      def cb_name_entry_key_press(entry, event):
        assi.set_page_complete(entry.get_parent(), True)

      def cb_func_entry_key_press(entry, event):
        assi.set_page_complete(entry.get_parent(), True)

      def cb_entry_key_press(entry, event):
        assi.set_page_complete(entry.get_parent(), True)

      def cb_apply(assistant):
        name_str = name_entry.get_text()
        func_str = func_entry.get_text()
        save_qm  = radiobutton_yes.get_active()
        icon_iter = icon_combobox.get_active_iter()
        icon = None
        if (icon_iter):
          icon, icon_stock, icon_filename = icon_model.get(icon_iter, 0, 1, 2)
          if (icon_stock):
            icon = icon_stock

        coot_toolbar_button(name_str, func_str, icon)
        if (save_qm):
          save_toolbar_to_init_file(name_str, func_str, icon)
        
      assi = gtk.Assistant()
      assi.set_title("Toolbutton Assistant")

      assi.connect('delete_event', cb_close)
      assi.connect('close', cb_close)
      assi.connect('cancel', cb_close)
      assi.connect('apply', cb_apply)

      # Construct page 0 (enter the button name)
      vbox = gtk.VBox(False, 5)
      vbox.set_border_width(5)
      vbox.show()
      assi.append_page(vbox)
      assi.set_page_title(vbox, 'Create a new toolbutton')
      assi.set_page_type(vbox, gtk.ASSISTANT_PAGE_CONTENT)

      label = gtk.Label("Please enter the name for the new toolbutton...")
      label.set_line_wrap(True)
      label.show()
      vbox.pack_start(label, True, True, 0)

      name_entry = gtk.Entry()
      name_entry.connect("key-press-event", cb_name_entry_key_press)
      name_entry.show()
      vbox.pack_end(name_entry)
      # check if name exists!!!!

      # Construct page 1 (enter the function)
      vbox = gtk.VBox(False, 5)
      vbox.set_border_width(5)
      vbox.show()
      assi.append_page(vbox)
      assi.set_page_title(vbox, 'Set the function')
      assi.set_page_type(vbox, gtk.ASSISTANT_PAGE_CONTENT)

      label = gtk.Label("Please enter the python function... (e.g. refine_active_residue())")
      label.set_line_wrap(True)
      label.show()
      vbox.pack_start(label, True, True, 0)

      func_entry = gtk.Entry()
      func_entry.connect("key-press-event", cb_func_entry_key_press)
      func_entry.show()
      vbox.pack_end(func_entry)

      # Construct page 2 (save?)
      vbox = gtk.VBox(False, 5)
      vbox.set_border_width(5)
      vbox.show()
      assi.append_page(vbox)
      assi.set_page_title(vbox, 'Save to preferences?')
      assi.set_page_type(vbox, gtk.ASSISTANT_PAGE_CONTENT)

      label = gtk.Label("Do you want to save the button in your preferences?")
      label.set_line_wrap(True)
      label.show()
      vbox.pack_start(label, True, True, 0)

      radiobutton_yes = gtk.RadioButton(None, "Yes")
      radiobutton_no  = gtk.RadioButton(radiobutton_yes, "No")
      radiobutton_yes.set_active(True)
      radiobutton_yes.show()
      radiobutton_no.show()
      vbox.pack_start(radiobutton_yes, True, True, 0)
      vbox.pack_start(radiobutton_no,  True, True, 0)
      assi.set_page_complete(radiobutton_yes.get_parent(), True)

      # Construct page 3 (select icon)
      vbox = gtk.VBox(False, 5)
      vbox.set_border_width(5)
      vbox.show()
      assi.append_page(vbox)
      assi.set_page_title(vbox, 'Select an icon')
      assi.set_page_type(vbox, gtk.ASSISTANT_PAGE_CONTENT)

      label = gtk.Label("Please select an icon or leave as it is...")
      label.set_line_wrap(True)
      label.show()
      vbox.pack_start(label, True, True, 0)

      hbox = gtk.HBox(False, 5)
      icon_model = make_icons_model()
      icon_combobox = icon_selection_combobox(icon_model)
      icon_combobox.show()
      hbox.show()
      hbox.pack_start(icon_combobox, True, False, 2)
      vbox.pack_end(hbox)
      assi.set_page_complete(label.get_parent(), True)

      # Final page
      # As this is the last page needs to be of page_type
      # gtk.ASSISTANT_PAGE_CONFIRM or gtk.ASSISTANT_PAGE_SUMMARY
      label = gtk.Label('Thanks for using the toolbutton assistent!')
      label.set_line_wrap(True)
      label.show()
      assi.append_page(label)
      assi.set_page_title(label, 'Finish')
      assi.set_page_complete(label, True)
      assi.set_page_type(label, gtk.ASSISTANT_PAGE_CONFIRM)

      assi.show()

    def add_toolbar_button_simple_gui():

      def button_func(*args):
        # dummy dont need to do anythin here
        pass

      def do_func(*args):
        name_str = args[0]
        func_str = args[1]
        save_qm  = args[2]
        icon_iter = icon_combobox.get_active_iter()
        icon = None
        if (icon_iter):
          icon, icon_stock, icon_filename = icon_model.get(icon_iter, 0, 1, 2)
          if (icon_stock):
            icon = icon_stock

        coot_toolbar_button(name_str, func_str, icon)
        if (save_qm):
          save_toolbar_to_init_file(name_str, func_str, icon)
        
      entry_widget = generic_double_entry("Button Name:", "Python Function:",
                                          "test", "centre_of_mass(0)",
                                          "Save to Preferences?", button_func,
                                          "Create", do_func,
                                          return_widget = True)

      hbox = gtk.HBox(True, 0)
      label = gtk.Label("Icon:")
      icon_model = make_icons_model()
      icon_combobox = icon_selection_combobox(icon_model)
      hbox.pack_start(label, True, False, 0)
      hbox.pack_start(icon_combobox, True, False, 0)
      children = entry_widget.get_children()
      vbox = children[0]
      vbox.pack_start(hbox, True, False, 2)
      vbox.reorder_child(hbox, 3) 
      hbox.show_all()

    def show_pop_up_menu(widget, event):
      if (event.button == 3):
        create_show_pop_menu([["Manage Buttons (add, delete Buttons)", make_toolbar_button_gui],
                              ["Add a user-defined Button", add_toolbar_button_gui],
                              ["Remove a Button", remove_toolbar_button_gui],
                              ["Hide Text (only icons)", toolbar_hide_text],
                              ["Show Text", toolbar_show_text]],
                             event)

    coot_main_toolbar = coot_python.main_toolbar()
    coot_main_toolbar.connect("button-press-event", show_pop_up_menu)

# save a toolbar button to ~/.coot-preferences/coot_toolbuttons.py
def save_toolbar_to_init_file(button_label, callback_function, icon=None):

  save_str = "coot_toolbar_button(\"" + button_label + "\", \"" + callback_function
  if icon:
    save_str += ("\", \"" + icon)
  save_str += "\")"
  
  home = 'HOME'
  if (os.name == 'nt'):
    home = 'COOT_HOME'
  filename = os.path.join(os.getenv(home), ".coot-preferences", "coot_toolbuttons.py")
  remove_line_containing_from_file(["coot_toolbar_button", button_label], filename)
  save_string_to_file(save_str, filename)

# remove a toolbar from  ~/.coot-preferences/coot_toolbuttons.py
def remove_toolbar_from_init_file(button_label):
  home = 'HOME'
  if (os.name == 'nt'):
    home = 'COOT_HOME'
  filename = os.path.join(os.getenv(home), ".coot-preferences", "coot_toolbuttons.py")
  remove_str_ls = ["coot_toolbar_button", button_label]
  if (os.path.isfile(filename)):
    remove_line_containing_from_file(remove_str_ls, filename)


# returns a list with pre-defined toolbar-functions (stock-id is optional)
# format:
# [[Group1,[toolbarname1, callbackfunction1, description1],
#          [toolbarname2, callbackfunction2, description2, stock-id2],...],
#  [Group2]....]
# OBS: callbackfunction is a string!
def list_of_toolbar_functions():
  ls = [["Display",
         ["Stereo/Mono", "stereo_mono_toggle()", "Toggle between Stereo and Mono view", "stereo-view.svg"],
         ["Side-by-side/Mono", "side_by_side_stereo_mono_toggle()", "Toggle between Side-by-Side Stereo and Mono view", "stereo-view.svg"],
         ["Test", "rotation_centre()", "test function"]],
        ["Refinement",
         ["Refine residue", "refine_active_residue()", "RSR active residue"],
         ["Reset B", "reset_b_factor_active_residue()", "Reset the B-Factor of active Residue"],
         ["Find Waters", "wrapped_create_find_waters_dialog()", "Find water molecules in map", "add-water.svg"],
         ["Add Alt Conf", "altconf()", "Add alternative conformation", "add-alt-conf.svg"],
         ["Edit BB", "setup_backbone_torsion_edit(1)", "Edit Backbone Torsion Angle", "flip-peptide.svg"],
         ['Torsion Gen.', "setup_torsion_general(1)", "Torsion General (after O function)", "edit-chi.svg"],
         ["Run Refmac", "wrapped_create_run_refmac_dialog()", "Launch Refmac for Refinement", "azerbaijan.svg"]],
        ["NMR",[]],
        ["EM",[]],
        ["Sidechains/Alignment",[]]]
  return ls

  
