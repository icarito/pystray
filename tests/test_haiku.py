import unittest
from unittest import mock
import ctypes
import os
import tempfile
import pathlib
from PIL import Image # For testing icon updates with PIL images

# Adjust Python path to import the backend from lib/pystray
import sys
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

# Module to be tested
# The import of _haiku and the patching of CDLL needs to be carefully ordered.
# Patch CDLL *before* _haiku is imported for the first time.

# 1. Global mock for the C library
mock_clib_global = mock.MagicMock(spec=ctypes.CDLL)
mock_clib_global.pystray_init = mock.MagicMock(return_value=0)
mock_clib_global.pystray_run = mock.MagicMock()
mock_clib_global.pystray_stop = mock.MagicMock()
mock_clib_global.pystray_show_icon = mock.MagicMock()
mock_clib_global.pystray_hide_icon = mock.MagicMock()
mock_clib_global.pystray_update_icon = mock.MagicMock()
mock_clib_global.pystray_update_title = mock.MagicMock()
mock_clib_global.pystray_update_menu = mock.MagicMock()
mock_clib_global.pystray_notify = mock.MagicMock()
mock_clib_global.pystray_remove_notification = mock.MagicMock()

# 2. Patch ctypes.CDLL using a class decorator or context manager for the test class,
#    OR patch it globally here and ensure _haiku is imported *after* the patch is active.
#    Global patch is simpler for this script structure.
patcher = mock.patch('ctypes.CDLL', new=mock_clib_global)
patcher.start() # Start the patch before importing _haiku

# 3. Now import the module to be tested
from lib.pystray import _haiku as haiku_backend
# Ensure the imported module uses our mock by explicitly setting its _lib and re-running setup
haiku_backend._lib = mock_clib_global
haiku_backend._setup_ctypes_functions()


class TestHaikuBackend(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        """Stop the global CDLL patcher after all tests."""
        patcher.stop()

    def setUp(self):
        """Reset mocks and manage temporary files for each test."""
        for attr_name in dir(mock_clib_global):
            attr = getattr(mock_clib_global, attr_name)
            if isinstance(attr, mock.MagicMock):
                attr.reset_mock()
        mock_clib_global.pystray_init.return_value = 0  # Default success
        self._temp_files = []

    def tearDown(self):
        """Clean up temporary files."""
        for temp_file_path in self._temp_files:
            try:
                os.unlink(temp_file_path)
            except OSError:
                pass
        # Clean up any temp file object potentially stored on icon instances by tests
        # This is a safeguard. Tests creating such attributes should ideally clean them.
        for item in gc.get_objects(): # Requires `import gc`
             if isinstance(item, haiku_backend.Icon) and hasattr(item, '_temp_icon_file'):
                 try:
                     if item._temp_icon_file and hasattr(item._temp_icon_file, 'name'):
                         os.unlink(item._temp_icon_file.name)
                     if item._temp_icon_file and hasattr(item._temp_icon_file, 'close'):
                         item._temp_icon_file.close()
                 except Exception: pass # Ignore errors during cleanup
                 delattr(item, '_temp_icon_file')


    def _create_temp_image_path(self, suffix=".png"):
        """Creates a temporary image file and returns its path. Adds to cleanup."""
        img = Image.new('RGB', (60, 30), color='red')
        # delete=False is important as the file is used externally by the (mocked) C lib
        with tempfile.NamedTemporaryFile(suffix=suffix, delete=False) as tmp_file:
            img.save(tmp_file.name)
            self._temp_files.append(tmp_file.name)
            return tmp_file.name

    def test_icon_instantiation(self):
        haiku_backend.Icon("test_icon")
        mock_clib_global.pystray_init.assert_called_once()

    @mock.patch('threading.Thread')
    def test_icon_run_stop(self, mock_thread_class):
        mock_thread_instance = mock.MagicMock()
        mock_thread_class.return_value = mock_thread_instance
        icon = haiku_backend.Icon("test_icon")
        mock_clib_global.pystray_init.reset_mock()

        icon.run()
        mock_thread_class.assert_called_once_with(target=icon._run_loop_target, name="PystrayHaikuThread", daemon=True)
        mock_thread_instance.start.assert_called_once()
        mock_clib_global.pystray_show_icon.assert_called_once()

        mock_thread_instance.is_alive.return_value = True
        icon.stop()
        mock_clib_global.pystray_hide_icon.assert_called_once()
        mock_clib_global.pystray_stop.assert_called_once()
        mock_thread_instance.join.assert_called_once_with(timeout=2.0)

    def test_icon_property_path_string(self):
        icon = haiku_backend.Icon("test_icon")
        icon._running = True; icon._visible = True # Simulate active state
        test_path = self._create_temp_image_path() # Use a real temp path
        icon.icon = test_path
        mock_clib_global.pystray_update_icon.assert_called_once_with(test_path.encode('utf-8'))

    @mock.patch('tempfile.NamedTemporaryFile')
    def test_icon_property_pil_image(self, mock_ntf):
        # Configure the mock for NamedTemporaryFile
        # This mock will be used by the Icon._update_icon method
        mock_temp_file_obj = mock.MagicMock(spec=tempfile.SpooledTemporaryFile) # or other file-like mock
        mock_temp_file_obj.name = "/tmp/mocked_temp_icon.png"
        # If _update_icon uses 'with tempfile.NamedTemporaryFile(...) as f:',
        # then return_value.__enter__.return_value should be set.
        # If it's `f = tempfile.NamedTemporaryFile(...); ...; f.close()`, then return_value is enough.
        # The Haiku backend's _update_icon uses: self._temp_icon_file = tempfile.NamedTemporaryFile(...)
        mock_ntf.return_value = mock_temp_file_obj

        icon = haiku_backend.Icon("test_icon")
        icon._running = True; icon._visible = True

        img = Image.new('RGB', (60, 30), color='blue')
        icon.icon = img # This triggers the setter and _update_icon

        mock_ntf.assert_called_once_with(suffix=".png", delete=False)
        # PIL.Image.save should have been called by production code on mock_temp_file_obj.name
        # To assert this, we'd need to mock `Image.Image.save` or pass a mock image.
        # For now, we check that the C function was called with the mocked temp file name.
        mock_clib_global.pystray_update_icon.assert_called_once_with(mock_temp_file_obj.name.encode('utf-8'))
        self.assertEqual(icon._temp_icon_file, mock_temp_file_obj) # Check if it stored the mock file handle

    def test_title_property(self):
        icon = haiku_backend.Icon("test_icon")
        icon._running = True; icon._visible = True
        new_title = "New Title"
        icon.title = new_title
        mock_clib_global.pystray_update_title.assert_called_once_with(new_title.encode('utf-8'))

    def test_menu_property(self):
        icon = haiku_backend.Icon("test_icon")
        icon._running = True; icon._visible = True
        # from pystray import Menu # If using real menu items
        mock_menu = mock.MagicMock()
        icon.menu = mock_menu
        mock_clib_global.pystray_update_menu.assert_called_once()

    def test_visible_property(self):
        icon = haiku_backend.Icon("test_icon")
        icon._running = True # Must be "running" for visible setter to call backend

        icon.visible = True
        mock_clib_global.pystray_show_icon.assert_called_once()
        mock_clib_global.pystray_show_icon.reset_mock() # Reset for next assertion

        icon.visible = False
        mock_clib_global.pystray_hide_icon.assert_called_once()
        mock_clib_global.pystray_hide_icon.reset_mock()

        icon.visible = False # Set to same value
        mock_clib_global.pystray_hide_icon.assert_not_called() # Should not call backend again

    def test_notify_method(self): # Public notify method
        icon = haiku_backend.Icon("test_icon", title="Default")
        icon._running = True

        icon.notify("Message", "Title")
        mock_clib_global.pystray_notify.assert_called_once_with("Message".encode('utf-8'), "Title".encode('utf-8'))
        mock_clib_global.pystray_notify.reset_mock()

        icon.notify("Message Only") # Uses icon title as default
        mock_clib_global.pystray_notify.assert_called_once_with("Message Only".encode('utf-8'), icon.title.encode('utf-8'))

    def test_backend_flags(self):
        self.assertFalse(haiku_backend.Icon.HAS_MENU_RADIO)
        # Check other flags like HAS_CUSTOM_QUIT, HAS_DEFAULT_ICON if they are defined

    def test_stop_before_run(self):
        icon = haiku_backend.Icon("test_icon")
        icon.stop() # Should be a no-op
        mock_clib_global.pystray_stop.assert_not_called()

    @mock.patch('threading.Thread')
    def test_run_twice(self, mock_thread_class):
        mock_thread_instance = mock.MagicMock()
        mock_thread_class.return_value = mock_thread_instance
        icon = haiku_backend.Icon("test_icon")

        icon.run() # First run
        self.assertTrue(icon._running)
        mock_thread_instance.start.assert_called_once() # Thread started

        icon.run() # Second run should be a no-op
        mock_thread_instance.start.assert_called_once() # Not called again


if __name__ == "__main__":
    import gc # Import gc here for the tearDown method
    unittest.main()
