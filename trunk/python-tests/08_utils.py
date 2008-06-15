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

class UtilTestFunctions(unittest.TestCase):

    def test01_0(self):
        """Test key symbols"""

        add_key_binding("name", "missing","")

        test_list = [[key_sym_code("a-symbol"), -1],
                     [key_sym_code(["a", "b"]), -1],
                     [key_sym_code("A"), 65],
                     [key_sym_code("a"), 97],
                     [key_sym_code("9"), 57],
                     [key_sym_code("cent"), 162]
                     ]

        for code, result in test_list:
            self.failUnlessEqual(code, result, " fail on key_sym_code, %s is not equal to %s " %(code, result))


    def test02_0(self):
        """Test running a scheme function"""

        if (have_test_skip):
            self.skipIf(not coot_has_guile())
        else:
            if (not coot_has_guile()):
                print "Skipping guile test (actually passing!)"
                return

        tot = run_scheme_command("(+ 2 2)")
        self.failUnlessEqual(tot, 4)
