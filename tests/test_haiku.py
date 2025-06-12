# -*- coding: utf-8 -*-
#
# Copyright (C) 2023 Moses Palmér
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import sys
import unittest
from unittest import mock

# Mock sys.platform to simulate Haiku environment
with mock.patch.object(sys, 'platform', 'haiku'):
    import pystray
    from pystray import Icon, Menu, MenuItem


class TestHaikuBackend(unittest.TestCase):
    @mock.patch.object(sys, 'platform', 'haiku')
    def test_icon_instantiation(self):
        """Test that pystray.Icon can be instantiated on Haiku."""
        try:
            icon = Icon('test_icon')
            self.assertIsInstance(icon, pystray._haiku.Icon)
        except Exception as e:
            self.fail(f"Icon instantiation failed on Haiku: {e}")

    @mock.patch.object(sys, 'platform', 'haiku')
    def test_basic_methods(self):
        """Test basic methods of the Haiku Icon class."""
        icon = Icon('test_icon')
        try:
            icon.show()
            icon.hide()
            # _update_icon, _update_title, _update_menu are internal
            # and called by other methods or properties, so we don't test them directly.
            icon.notify("Test notification")
            icon.remove_notification()
            icon.stop()
        except Exception as e:
            self.fail(f"Calling basic methods failed on Haiku: {e}")

    @mock.patch.object(sys, 'platform', 'haiku')
    def test_run_detached_method(self):
        """Test the _run_detached method of the Haiku Icon class."""
        icon = Icon('test_icon')
        try:
            # _run_detached is meant to be called internally by run_detached()
            # We are testing the internal placeholder here.
            icon._run_detached()
        except Exception as e:
            self.fail(f"_run_detached method failed on Haiku: {e}")

    @mock.patch.object(sys, 'platform', 'haiku')
    def test_run_method(self):
        """Test the _run method of the Haiku Icon class."""
        icon = Icon('test_icon')
        try:
            # _run is meant to be called internally by run()
            # We are testing the internal placeholder here.
            # Since _run is blocking, we need to handle it carefully in a real scenario.
            # For a placeholder, we just call it.
            icon._run()
        except Exception as e:
            self.fail(f"_run method failed on Haiku: {e}")


if __name__ == '__main__':
    unittest.main()
