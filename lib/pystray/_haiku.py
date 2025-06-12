# -*- coding: utf-8 -*-
#
# Copyright (C) 2020-2021 Moses Palmér
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

from . import _base


class Icon(_base.Icon):
    """A dummy backend for Haiku."""
    HAS_DEFAULT_ACTION = False
    HAS_MENU = False
    HAS_MENU_RADIO = False
    HAS_NOTIFICATION = False

    def _show(self):
        # Haiku-specific API call to show the icon
        pass

    def _hide(self):
        # Haiku-specific API call to hide the icon
        pass

    def _update_icon(self):
        # Haiku-specific API call to update the icon image
        pass

    def _update_title(self):
        # Haiku-specific API call to update the icon title
        pass

    def _update_menu(self):
        # Haiku-specific API call to update the menu
        pass

    def _run(self):
        # Haiku-specific API call to run the icon's main loop
        pass

    def _run_detached(self):
        # Haiku-specific API call to run the icon's main loop in a detached thread
        pass

    def _stop(self):
        # Haiku-specific API call to stop the icon's main loop
        pass

    def _notify(self, message, title=None):
        # Haiku-specific API call to display a notification
        pass

    def _remove_notification(self):
        # Haiku-specific API call to remove a notification
        pass
