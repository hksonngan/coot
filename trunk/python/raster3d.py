# raster3d.py

# Copyright 2005, 2006 by Bernhard Lohkamp
# Copyright 2007 by Bernhard Lohkamp, The University of York
# Copyright 2005, 2006 by Paul Emsley, The University of York

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc.,  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# BL pyton script for rendering images in COOT

# args not including the output filename
def povray_args():
    return " +FN16 +A"
# BL says: dont know how usefull this function is/will be....

# run raster3d
def render_image():
    import os, webbrowser

    coot_r3d_file_name="coot.r3d"
    if (os.name == 'nt'):
	coot_image_file_name = "coot.tif"
	image_format = " -tiff "
    else:
	coot_image_file_name = "coot.png"
	image_format = " -png "
    raster3d(coot_r3d_file_name)
    r3d_exe = find_exe("render", "PATH")
    if (r3d_exe):
       r3d_dir = os.path.dirname(r3d_exe)
       os.environ['R3D_LIB'] = r3d_dir + "/materials"
       r3d_call = r3d_exe + image_format + coot_image_file_name + " < " + coot_r3d_file_name
       print "BL DEBUG:: r3d_call is ", r3d_call
       print "calling render..."
       
       status = os.system(r3d_call)
       print "calling display..."
       try:
         webbrowser.open(coot_image_file_name,1,1)
       except OSError:
         print "BL WARNING:: We can't find rendered file ",coot_image_file_name

# Run either raster3d or povray
#
# @var{image_type} is either 'raster3d' or 'povray'
def raytrace(image_type, source_file_name, image_file_name, x_size, y_size):
   import os, webbrowser

   if (image_type == "raster3d"):
    # BL says: this is for windows, since tif file
    if os.name == "nt":
	render_exe = "render.exe"
	image_file_name += ".tif"
	image_format = " -tiff "
    else:
	render_exe = "render"
	image_format = " -png "
    r3d_exe = find_exe("render", "PATH")
    if (r3d_exe):
       r3d_dir = os.path.dirname(r3d_exe)
       os.environ['R3D_LIB'] = r3d_dir + "/materials"
       #we have to check filenames for spaces for dodgy windows path
       image_file_name_mod, source_file_name_mod, space_flag = \
		check_file_names_for_space_and_move(image_file_name, source_file_name)
       r3d_call = r3d_exe + image_format + image_file_name_mod + " < " + source_file_name_mod
       print "BL DEBUG:: r3d_call is ", r3d_call
       print "calling render..."

       status = os.system(r3d_call)
       # now we have to copy files back if necessary!!
       if (space_flag):
          import shutil
          shutil.move(image_file_name_mod,image_file_name)
          shutil.move(source_file_name_mod,source_file_name)
       else: pass

       print "calling display..."
       try:
         webbrowser.open(image_file_name,1,1)
       except OSError:
         print "BL WARNING:: We can't find rendered file ",image_file_name

   elif (image_type == "povray"):
    image_file_name += ".png"
    #BL says: conversion of filename, again! Windows is driving me crazy...
    image_file_name = os.path.normpath(image_file_name)
    # again, we just assume povray exe is pvengine on all systems
    povray_exe = find_exe(povray_command_name, "PATH")
    if (povray_exe):
      image_file_name_mod, source_file_name_mod, space_flag = \
		check_file_names_for_space_and_move(image_file_name, source_file_name)
      if (os.name == 'nt'):
		args = " /EXIT /RENDER "
      else:
		args = " "
      args = args + source_file_name_mod + povray_args() + " -UV" + " +W" + str(x_size) + " +H" + str(y_size)
      print "BL INFO:: run povray with args: ", args
      povray_call = povray_exe + args + " +o" + image_file_name_mod
      print "BL DEBUG:: povray command line", povray_call
      os.system(povray_call)
      # now we have to copy files back if necessary!!
      if (space_flag):
         import shutil
         shutil.move(image_file_name_mod,image_file_name)
         shutil.move(source_file_name_mod,source_file_name)
      else: pass
      print "calling display..."
      try:
         webbrowser.open(image_file_name,1,1)
      except OSError:
         print "BL WARNING:: We can't find rendered file ",image_file_name

   else:
     print "Image type ", image_type, " unknown!"

# Converts a ppm file to a bmp file (for windows users)
def ppm2bmp(ppm_file_name):
    import os, webbrowser

    bmp_file_name ,extension = os.path.splitext(ppm_file_name)
    bmp_file_name += ".bmp"

    # BL says: need to do some wired stuff to make sure cmd/system works with
    # space in file name , e.g. ' & " thingys
    cmd = 'ppm2bmp "'
    ppm2bmp_call = cmd + ppm_file_name + '"'
    os.system(ppm2bmp_call)
    if (os.path.isfile(bmp_file_name)):
      print "calling display..."
      try:
         webbrowser.open(bmp_file_name,1,1)
      except OSError:
         print "BL WARNING:: We can't find screendump file ",bmp_file_name
    else:
      print "BL WARNING:: Cannot find bmp file ",bmp_file_name

# Tests file names for spaces. there is certainly on problem on windows
# not sure about other OS, yet
def check_file_names_for_space_and_move(image_file_name,source_file_name):

    import string, os, shutil

    space_flag = False

    if (((image_file_name.find(" ") > -1) or (source_file_name.find(" ") > -1)) and (os.name == 'nt')):
       # we have spaces, so tmp copy src to C: and run there, then
       # copy tmp back to where it should be
       image_file_name_mod = "C:\\" + os.path.basename(image_file_name)
       source_file_name_mod = "C:\\" + os.path.basename(source_file_name)
       shutil.move(source_file_name,source_file_name_mod)
       space_flag = True
       return image_file_name_mod, source_file_name_mod, space_flag
    else:
       return image_file_name, source_file_name, space_flag
       
#raytrace("povray","coot.pov","coot.png",600,600)
