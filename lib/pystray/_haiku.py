import ctypes
import os
import threading # For running pystray_run in a separate thread
import pathlib
import tempfile # For _update_icon with PIL.Image

# Determine the library path
# The C++ helper executable, when run, provides the Deskbar replicant.
# It also exports the C API symbols. On Haiku, executables can be loaded like shared libraries.
# We expect "PystrayHaikuHelper" (matching APP_NAME in Makefile) to be in a directory
# where it can be found by the dynamic linker, or alongside the Python executable,
# or in a known location relative to this module.

HELPER_EXECUTABLE_NAME = "PystrayHaikuHelper" # As defined in C++ Makefile

_lib = None
_lib_load_tried = False # Flag to ensure loading is attempted only once.

def _load_library():
    """
    Loads the Haiku helper library.
    This function should only be called once.
    """
    global _lib, _lib_load_tried

    if _lib_load_tried:
        return _lib # Return cached result (None or the library)

    _lib_load_tried = True

    # Paths to attempt for loading the library
    library_candidates = [
        HELPER_EXECUTABLE_NAME,
        str(pathlib.Path(__file__).resolve().parent / HELPER_EXECUTABLE_NAME),
        f"/boot/home/config/apps/{HELPER_EXECUTABLE_NAME}",
        f"/boot/system/apps/{HELPER_EXECUTABLE_NAME}",
        # Example for running from a development tree:
        # str(pathlib.Path(__file__).resolve().parent.parent.parent / "build" / "haiku_helper" / HELPER_EXECUTABLE_NAME)
    ]

    loaded_lib = None
    for path_attempt in library_candidates:
        try:
            loaded_lib = ctypes.CDLL(path_attempt)
            print(f"Pystray Haiku: Loaded helper library from: {path_attempt}")
            break # Success
        except OSError:
            # print(f"Pystray Haiku: Failed to load from {path_attempt}")
            pass

    _lib = loaded_lib # Assign to global _lib

    if _lib is None:
        print(
            f"WARNING: Pystray Haiku: Helper library '{HELPER_EXECUTABLE_NAME}' not found "
            f"in any of the attempted paths. Pystray will not function on Haiku."
        )
    return _lib

# Perform library loading attempt when the module is first imported.
# Subsequent calls to _load_library() will return the cached result.
_load_library()


def _setup_ctypes_functions():
    """Defines argtypes and restype for C functions if _lib is loaded."""
    # _lib is now global and should be set by _load_library()
    if not _lib:
        return

    try:
        _lib.pystray_init.argtypes = []
        _lib.pystray_init.restype = ctypes.c_int

        _lib.pystray_run.argtypes = []
        _lib.pystray_run.restype = None

        _lib.pystray_stop.argtypes = []
        _lib.pystray_stop.restype = None

        _lib.pystray_show_icon.argtypes = []
        _lib.pystray_show_icon.restype = None

        _lib.pystray_hide_icon.argtypes = []
        _lib.pystray_hide_icon.restype = None

        _lib.pystray_update_icon.argtypes = [ctypes.c_char_p]
        _lib.pystray_update_icon.restype = None

        _lib.pystray_update_title.argtypes = [ctypes.c_char_p]
        _lib.pystray_update_title.restype = None

        _lib.pystray_update_menu.argtypes = []
        _lib.pystray_update_menu.restype = None # Placeholder

        _lib.pystray_notify.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        _lib.pystray_notify.restype = None

        _lib.pystray_remove_notification.argtypes = []
        _lib.pystray_remove_notification.restype = None # Placeholder
    except AttributeError as e:
        print(f"Pystray Haiku: Error setting up ctypes: {e}. C API functions missing in helper.")
        global _lib # To modify the global _lib variable
        _lib = None # Invalidate library if it's not as expected

_setup_ctypes_functions()

def _call_lib_func(func_name, *args, default_return=None):
    """Safely calls a C function, handling unloaded library or missing functions."""
    if not _lib:
        if func_name == "pystray_init": return -1 # Critical failure indicator
        return default_return

    try:
        func = getattr(_lib, func_name)
        return func(*args)
    except AttributeError:
        print(f"Pystray Haiku: Function '{func_name}' not found in helper library.")
    except Exception as e:
        print(f"Pystray Haiku: Error calling '{func_name}': {e}")

    if func_name == "pystray_init": return -1
    return default_return

# Renamed API functions for clarity within the Icon class scope
def _init_backend(): return _call_lib_func("pystray_init")
def _run_backend(): return _call_lib_func("pystray_run")
def _stop_backend(): return _call_lib_func("pystray_stop")
def _show_icon_backend(): return _call_lib_func("pystray_show_icon")
def _hide_icon_backend(): return _call_lib_func("pystray_hide_icon")
def _update_icon_backend(path_bytes): return _call_lib_func("pystray_update_icon", path_bytes)
def _update_title_backend(title_bytes): return _call_lib_func("pystray_update_title", title_bytes)
def _update_menu_backend(): return _call_lib_func("pystray_update_menu") # Placeholder
def _notify_backend(msg_bytes, title_bytes): return _call_lib_func("pystray_notify", msg_bytes, title_bytes)
# def _remove_notification_backend(): return _call_lib_func("pystray_remove_notification") # Placeholder


class Icon(object):
    """Haiku backend for pystray.Icon"""

    def __init__(self, name, icon=None, title=None, menu=None):
        self._name = name # Used as a fallback for notification titles
        self._icon_path_str = None
        self._title_str = title
        self._menu_items = menu # Store for future dynamic menu updates
        self._visible = False # Icon is not visible until run() is called
        self._running = False # Backend event loop state
        self._thread = None

        if isinstance(icon, str):
            self._icon_path_str = icon
        elif hasattr(icon, 'save'): # Basic check for PIL.Image like objects
            # Real implementation would save to a temp file and store the path.
            # For now, this backend will only work if a path is provided for the icon.
            print("Pystray Haiku: PIL.Image icon type support is minimal. Please provide a path string.")
            # self._icon_path_str = self._save_to_tempfile(icon) # Placeholder concept

        if _init_backend() != 0:
            raise RuntimeError(
                f"Pystray Haiku: Failed to initialize helper. "
                f"Ensure '{HELPER_EXECUTABLE_NAME}' is compiled and accessible."
            )

        # Initial settings if values are provided
        if self._icon_path_str: self.icon = self._icon_path_str
        if self._title_str: self.title = self._title_str
        # if self._menu_items: self.menu = self._menu_items # Requires full menu handling
        self._setup_event = threading.Event()


    def _run_loop_target(self):
        """Target for the backend thread. Calls the blocking C++ run function."""
        # This method is intended to be run in a separate thread.
        print("Pystray Haiku: Backend thread started, calling _run_backend().")
        _run_backend() # This is the blocking call to BApplication::Run()
        self._running = False # Mark as no longer running when _run_backend returns
        print("Pystray Haiku: Backend thread finished (_run_backend() returned).")

    def run(self, setup=None):
        """User-facing method to display the icon and start the backend event loop."""
        if not _lib:
            print("Pystray Haiku: Cannot run, helper library not loaded.")
            return
        if self._running:
            print("Pystray Haiku: Already running.")
            return

        self._running = True
        self._setup_event.clear()

        if setup:
            print("Pystray Haiku: Executing user setup function.")
            setup(self)
            print("Pystray Haiku: User setup function finished.")

        self._setup_event.set() # Signal that setup is complete

        # Ensure icon is shown according to current visible state (usually True by default after run)
        # This will call the visible.setter, which in turn calls _show_icon_backend()
        self.visible = True

        self._thread = threading.Thread(target=self._run_loop_target, name="PystrayHaikuThread")
        self._thread.daemon = True
        self._thread.start()
        print("Pystray Haiku: Backend thread launched.")

    def _stop_backend_resources(self):
        """Internal method to stop the C++ backend and join its thread."""
        if not _lib:
            print("Pystray Haiku: Library not loaded, cannot stop backend resources.")
            return

        print("Pystray Haiku: Requesting C++ backend to stop (_stop_backend()).")
        _stop_backend() # Tell C++ BApplication to quit

        current_thread = self._thread # Capture before it's potentially set to None
        if current_thread and current_thread.is_alive():
            print(f"Pystray Haiku: Joining backend thread ({current_thread.name})...")
            current_thread.join(timeout=2.0)
            if current_thread.is_alive():
                print("Pystray Haiku: Warning - backend thread did not stop cleanly after timeout.")

        self._thread = None
        self._running = False # Explicitly mark as not running
        print("Pystray Haiku: Backend resources stopped and thread joined.")


    def stop(self):
        """User-facing method to hide the icon and stop the backend event loop."""
        if not self._running:
            # print("Pystray Haiku: Not running.") # Can be too verbose if called multiple times
            return

        print("Pystray Haiku: Initiating stop sequence.")
        # Hide the icon first.
        # The visible.setter will call _hide_icon_backend().
        self.visible = False

        # Then, stop the backend resources (C++ app and thread).
        self._stop_backend_resources()
        print("Pystray Haiku: Stop sequence complete.")

    # _run and _stop methods as potentially called by a generic Controller
    # For this backend, _run starts the thread, _stop calls the internal resource cleanup.
    _run = run
    _stop = stop

    def _notify(self, message, title=None, **kwargs):
        """Internal method to display a notification. Called by pystray controller."""
        if not self._running: return

        message_bytes = str(message).encode('utf-8')
        # Use provided title, or icon's title, or icon's name as fallback
        effective_title = title or self._title_str or self._name
        title_bytes = str(effective_title).encode('utf-8')

        _notify_backend(message_bytes, title_bytes)

    def _remove_notification(self, **kwargs):
        """Internal method to remove a notification. Called by pystray controller."""
        # The C++ backend for this is a placeholder.
        if not self._running: return
        # _remove_notification_backend() # This is the C API call
        print("Pystray Haiku: _remove_notification called (current C++ backend is a placeholder).")


    @property
    def icon(self):
        return self._icon_path_str

    @icon.setter
    def icon(self, value):
        if isinstance(value, str):
            self._icon_path_str = value
        # Add PIL.Image handling here if fully supported (save to temp file)
        else:
            print("Pystray Haiku: Icon property only supports path strings currently.")
            return

        if self._running and self._visible and self._icon_path_str:
            _update_icon_backend(self._icon_path_str.encode('utf-8'))

    def _update_icon(self):
        """Internal method to update the icon. Called by pystray controller."""
        if not (self._running and self._visible):
            return

        icon_source = self._icon_path_str # This should be what Icon.icon property has set
        path_to_send = None

        if isinstance(icon_source, str):
            path_to_send = icon_source
        elif hasattr(icon_source, 'save') and hasattr(icon_source, 'format'): # Heuristic for PIL.Image
            try:
                # Attempt to save PIL.Image to a temporary file
                # We need to ensure the temporary file persists as long as the C++ side needs it.
                # This is a known challenge. For now, use a named temporary file.
                # The pystray contract might require the Icon class to manage this temp file's lifetime.
                suffix = f".{icon_source.format.lower()}" if icon_source.format else ".png"
                # We should store this temp_file object on self, so it doesn't get garbage collected too soon
                # and we can clean it up later. This is a simplification.
                if hasattr(self, '_temp_icon_file') and self._temp_icon_file:
                    try:
                        self._temp_icon_file.close() # Close previous temp file
                    except Exception:
                        pass # Ignore errors closing old file

                self._temp_icon_file = tempfile.NamedTemporaryFile(suffix=suffix, delete=False)
                icon_source.save(self._temp_icon_file.name)
                path_to_send = self._temp_icon_file.name
                print(f"Pystray Haiku: Saved PIL.Image to temp file: {path_to_send}")
            except Exception as e:
                print(f"Pystray Haiku: Failed to save icon to temporary file: {e}")
                return # Cannot update icon if save fails

        if path_to_send:
            _update_icon_backend(path_to_send.encode('utf-8'))
        else:
            # This case means self._icon_path_str was not a string and not a recognized image object
            # or it was an image object but failed to save.
            # We might want to send a signal to use a default icon or clear the icon.
            # For now, we do nothing if there's no valid path.
            print(f"Pystray Haiku: _update_icon called but no valid icon path to send. Current source: {icon_source}")


    @property
    def title(self):
        return self._title_str

    @title.setter
    def title(self, value):
        self._title_str = str(value) # Ensure string
        if self._running and self._visible:
            # _update_title_backend(self._title_str.encode('utf-8'))
            self._update_title() # Call internal update method

    def _update_title(self):
        """Internal method to update the title. Called by pystray controller."""
        if not (self._running and self._visible):
            return
        _update_title_backend(self._title_str.encode('utf-8'))

    @property
    def menu(self):
        return self._menu_items

    @menu.setter
    def menu(self, value):
        self._menu_items = value
        if self._running and self._visible:
            # _update_menu_backend()
            self._update_menu()

    def _update_menu(self):
        """Internal method to update the menu. Called by pystray controller."""
        # The C++ side is a placeholder. This call is also a placeholder.
        # A full implementation would serialize self._menu_items and send it.
        if not (self._running and self._visible):
            return
        _update_menu_backend()
        print("Pystray Haiku: _update_menu called (current C++ backend is a placeholder).")


    def notify(self, message_text, title_text=None):
        if not self._running: return

        msg_bytes = str(message_text).encode('utf-8')
        title_bytes = str(title_text or self._title_str or self._name).encode('utf-8')
        _notify_backend(msg_bytes, title_bytes)

    @property
    def visible(self):
        return self._visible

    @visible.setter
    def visible(self, value):
        value = bool(value)
        if self._visible == value: return

        self._visible = value
        if self._running: # Only interact with backend if event loop is active
            if self._visible:
                _show_icon_backend()
            else:
                _hide_icon_backend()
        # If not self._running, visibility state is just stored and applied by run()

    def _show(self):
        """Internal method to show the icon. Called by pystray controller."""
        # This assumes that if _show is called, the intent is to be visible.
        # The visible.setter handles the call to the backend.
        self.visible = True

    def _hide(self):
        """Internal method to hide the icon. Called by pystray controller."""
        # This assumes that if _hide is called, the intent is to be not visible.
        # The visible.setter handles the call to the backend.
        self.visible = False

    def __del__(self):
        if self._running:
            print("Pystray Haiku: Icon object deleted while backend was running. Attempting to stop.")
            # Clean up temporary icon file if it exists
            if hasattr(self, '_temp_icon_file') and self._temp_icon_file:
                try:
                    os.unlink(self._temp_icon_file.name) # Ensure file is deleted
                    self._temp_icon_file.close()
                    print(f"Pystray Haiku: Cleaned up temporary icon file {self._temp_icon_file.name}")
                except Exception as e:
                    print(f"Pystray Haiku: Error cleaning up temporary icon file: {e}")
                self._temp_icon_file = None
            self.stop()

Backend = Icon
