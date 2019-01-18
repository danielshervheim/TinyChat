#
# Daniel Shervheim © 2018-2019
# danielshervheim@gmail.com
# www.github.com/danielshervheim
#

CXX = gcc
LIBS = `pkg-config --cflags --libs gtk+-3.0` -rdynamic
LIBDIRS = -I src
CXXFLAGS = -Wall $(LIBDIRS) $(LIBS)
BIN = tinychat



# DEFAULT target

all: $(BIN)_server $(BIN)_client



# GRESOURCE rules
# compiles the icons and glade files into a .c and .h file to be linked in at compile time

FILE = data/gresource/$(BIN).gresource.xml
SOURCE = --sourcedir data/glade/ --sourcedir data/icons/ --sourcedir data/css/
TARGET = --c-name $(BIN) --target data/gresource/compiled/$(BIN)_gresource

data/gresource/compiled:
	mkdir -p data/gresource/compiled

compile_resources: data/gresource/compiled
	glib-compile-resources $(SOURCE) --generate-source $(TARGET).c $(FILE)
	glib-compile-resources $(SOURCE) --generate-header $(TARGET).h $(FILE)

clean_resources:
	rm -rf data/gresource/compiled



# APPLICATION COMPILATION rules
# compiles and links the source code into a finished executable

build:
	mkdir -p build

$(BIN)_client: compile_resources build src/client/*.c
	$(CXX) -o build/$(BIN)_client src/client/*.c data/gresource/compiled/*.c $(CXXFLAGS)

$(BIN)_server: build src/server/*.c
	$(CXX) -o build/$(BIN)_server src/server/*.c $(CXXFLAGS)

clean_build:
	rm -rf build



# CLEAN rules
# cleans all the build, dep, resourec files

clean: clean_build clean_resources



# INSTALLATION rules
# copies the executables and .desktop files to their respective destinations
# (this target most likely will need to be run as sudo)

install: install_server install_client

install_server: $(BIN)_server
	cp build/$(BIN)_server /usr/bin/
	cp data/desktop/tinychat_server.desktop /usr/share/applications/
	
install_client: $(BIN)_client
	cp build/$(BIN)_client /usr/bin/
	cp data/desktop/tinychat_client.desktop /usr/share/applications/


	

# UNINSTALLATION rules
# removes the executables and .desktop files from their respective
# installation destinations.
# (this target most likely will need to be run as sudo)

uninstall: uninstall_server uninstall_client

uninstall_server:
	rm -rf /usr/bin/$(BIN)_server
	rm -rf /usr/share/applications/tinychat_server.desktop

uninstall_client:
	rm -rf /usr/bin/$(BIN)_client
	rm -rf /usr/share/applications/tinychat_client.desktop



# STATS rules
# prints out stats about the project

src_stats:
	cd src; find -type f | xargs wc -l; cd ..