# Makefile for Pystray Haiku Helper

# Application specifics
APP_NAME = PystrayHaikuHelper
# The APP_SIGNATURE must match the one used in App.cpp and DeskbarView.cpp (for "add_on")
APP_SIGNATURE = application/x-vnd.pystray-haiku-helper
APP_VERSION = 0.1

# Source files
SRCS = App.cpp MainWindow.cpp DeskbarView.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Compiler and Linker
CC = g++

# Standard Haiku build flags
# Includes for Haiku headers. These are standard locations.
# You might need to adjust if your Haiku dev environment is different.
HAIKU_INCLUDES = -I/boot/system/develop/headers \
                 -I/boot/system/develop/headers/be \
                 -I/boot/system/develop/headers/posix \
                 -I/boot/system/develop/headers/add-ons/input_server \
                 # Add more specific paths if needed, e.g. /boot/system/develop/headers/be/interface

# Compiler flags:
# -Wall: Enable all warnings
# -O2: Optimization level 2
# -std=c++11: Use C++11 standard (or newer if required by your code)
CFLAGS = -Wall -O2 -std=c++11 $(HAIKU_INCLUDES)

# Linker flags:
# -lbe: Links against the Be API library (libbe.so)
# -ltracker: Links against the Tracker library (libtracker.so) if Tracker add-on features are used
#            (not strictly necessary for a basic Deskbar replicant but often useful)
# -ltranslation: For BTranslationUtils (e.g. GetBitmapFile)
LDFLAGS = -lbe -ltracker -ltranslation

# Name of the resources file, if you have one (e.g., for icons, app version info)
# RSRC_FILE = $(APP_NAME).rsrc

# Build rules
all: $(APP_NAME)

$(APP_NAME): $(OBJS)
	@echo "Linking $(APP_NAME)..."
	$(CC) $(LDFLAGS) -o $(APP_NAME) $(OBJS)
	@echo "Setting application signature and type (executable)..."
	# mimeset is used to set the BEOS:APP_SIG attribute and make the file executable
	# The -f flag forces overwriting if attributes already exist.
	mimeset -f $(APP_NAME)
	# It's good practice to also set the application signature using addattr
	addattr BEOS:APP_SIGNATURE "$(APP_SIGNATURE)" $(APP_NAME)
	# Optionally, set other BEOS attributes like APP_VERSION
	# addattr BEOS:APP_VERSION %APP_VERSION% $(APP_NAME) # Note: %APP_VERSION% might need specific handling in make
	# Add application resources if you have an .rsrc file
	# ifneq ($(RSRC_FILE),)
	# @echo "Adding resources from $(RSRC_FILE)..."
	# rc -o $(APP_NAME) $(RSRC_FILE)
	# resattr -o $(APP_NAME) $(RSRC_FILE)
	# endif
	@echo "Build complete: $(APP_NAME)"
	@echo "To run from Terminal: ./$(APP_NAME)"
	@echo "To install to Deskbar, usually the app needs to be in a common path,"
	@echo "or Deskbar needs to be told about it (often done by running the app once)."

%.o: %.cpp %.h # Depend on header files as well for recompilation
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	@echo "Cleaning up..."
	rm -f $(OBJS) $(APP_NAME)
	@echo "Cleanup complete."

# Phony targets (targets that are not actual files)
.PHONY: all clean install uninstall

# Example install/uninstall (very basic)
# These usually require specific paths for Haiku.
# For Deskbar replicants, often just running the app from anywhere is enough
# for it to register itself if it's coded to do so.
# Or, copy to /boot/home/config/add-ons/Deskbar (less common now)
# or /boot/system/apps or /boot/home/config/apps
# INSTALL_PATH = /boot/home/config/apps

# install: $(APP_NAME)
# 	@echo "Installing $(APP_NAME) to $(INSTALL_PATH)..."
# 	mkdir -p $(INSTALL_PATH)
# 	cp $(APP_NAME) $(INSTALL_PATH)/
# 	@echo "Installation complete."

# uninstall:
# 	@echo "Uninstalling $(APP_NAME) from $(INSTALL_PATH)..."
# 	rm -f $(INSTALL_PATH)/$(APP_NAME)
# 	@echo "Uninstallation complete."
