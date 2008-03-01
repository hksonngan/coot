# Copyright 2008 by The University of York
# Author: Bernhard Lohkamp
# Copyright 2007, 2008 by The University of Oxford
# Author: Paul Emsley

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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA

import unittest
import os

class SsmTestFunctions(unittest.TestCase):

    def test01_0(self):
        """SSM - Frank von Delft's Example"""

        imol_a = handle_read_draw_molecule_with_recentre(os.path.join(unittest_data_dir, "1wly.pdb"), 0)
        imol_b = handle_read_draw_molecule_with_recentre(os.path.join(unittest_data_dir, "1yb5.pdb"), 1)

        self.failUnless(valid_model_molecule_qm(imol_a) and valid_model_molecule_qm(imol_b))

        graphics_to_ca_plus_ligands_representation(imol_a)
        graphics_to_ca_plus_ligands_representation(imol_b)

        superpose_with_atom_selection(imol_a, imol_b, "A/2-111", "A/6-115", 0)
        set_rotation_centre(65.65, -3, -4)
        view_number = add_view([49.7269, 7.69693, 3.93221],
                               [-0.772277, 0.277494, 0.292497, 0.490948],
                               98.9608, "SSM View")
        go_to_view_number(view_number, 1)
        rotate_y_scene(rotate_n_frames(100), 0.1)
        set_mol_displayed(imol_a, 0)
        set_mol_displayed(imol_b, 0)
        # didnt crash....

    def test02_0(self):
        """SSM - Alice Dawson's Example"""
        imol_s = handle_read_draw_molecule_with_recentre(os.path.join(unittest_data_dir, "1pyd.pdb"), 0)

        graphics_to_ca_plus_ligands_representation(imol_s)
        set_graphics_window_size(678, 452)

        print_molecule_names()

        superpose_with_atom_selection(imol_s, imol_s, "A/100-400", "B/50-450", 1)
        imol_copy = graphics_n_molecules() - 1
        graphics_to_ca_plus_ligands_representation(imol_copy)
        rotate_y_scene(rotate_n_frames(100), 0.1)
        # didnt crash...
        
