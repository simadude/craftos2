CC=gcc
CXX=g++
PRINT_TYPE?=pdf
CFLAGS:=$(CFLAGS) -g -c -I/usr/include/lua5.1 -I/usr/include/jsoncpp
CXXFLAGS:= $(CXXFLAGS) -std=c++11 -DPRINT_TYPE=$(PRINT_TYPE)
ODIR=obj
SDIR=src
LIBS=-L/usr/local/include -llua5.1 -lm -ldl -lpthread -lcurl -ljsoncpp -lSDL2_mixer -lPocoNet -lPocoFoundation

ifeq ($(PRINT_TYPE), pdf)
LIBS:=$(LIBS) -lhpdf
endif
ifndef NO_PNG
LIBS:=$(LIBS) -lpng
endif
ifdef NO_PNG
CXXFLAGS:=$(CXXFLAGS) -DNO_PNG
endif

_OBJ=Computer.o config.o font.o fs_handle.o fs.o http_handle.o http.o http_server.o lib.o main.o mounter.o os.o periphemu.o peripheral.o term.o TerminalWindow.o peripheral_monitor.o peripheral_printer.o peripheral_computer.o peripheral_modem.o peripheral_drive.o liolib.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: $(ODIR) craftos

craftos: $(OBJ) $(ODIR)/platform.o
	$(CXX) -o $@ $^ $(LIBS) -lSDL2 -lSDL2main

macapp: $(OBJ) $(ODIR)/platform_macapp.o
	mkdir -p CraftOS-PC.app/Contents/MacOS
	mkdir -p CraftOS-PC.app/Contents/Resources
	clang++ -o CraftOS-PC.app/Contents/MacOS/craftos $^ $(LIBS) -F/Library/Frameworks -framework Foundation -framework SDL2
	install_name_tool -add_rpath @executable_path/../Frameworks CraftOS-PC.app/Contents/MacOS/craftos
	install_name_tool -change /usr/local/opt/lua@5.1/lib/liblua.5.1.dylib "@rpath/liblua.5.1.dylib" CraftOS-PC.app/Contents/MacOS/craftos
	install_name_tool -change /usr/local/opt/jsoncpp/lib/libjsoncpp.21.dylib "@rpath/libjsoncpp.1.9.0.dylib" CraftOS-PC.app/Contents/MacOS/craftos
	install_name_tool -change /usr/local/opt/libharu/lib/libhpdf-2.3.0.dylib "@rpath/libhpdf-2.3.0.dylib" CraftOS-PC.app/Contents/MacOS/craftos
	install_name_tool -change /usr/local/opt/libpng/lib/libpng16.16.dylib "@rpath/libpng16.16.dylib" CraftOS-PC.app/Contents/MacOS/craftos
	install_name_tool -change /usr/local/lib/libcurl.4.dylib "@rpath/libcurl.4.dylib" CraftOS-PC.app/Contents/MacOS/craftos
	cp Info.plist CraftOS-PC.app/Contents/

$(ODIR):
	mkdir obj

$(ODIR)/main.o: $(SDIR)/main.cpp $(SDIR)/Computer.hpp
	$(CXX) -o $@ $(CXXFLAGS) $(CFLAGS) $<

$(ODIR)/platform_macapp.o: $(SDIR)/platform_macapp.mm $(SDIR)/platform.hpp
	clang++ -o $@ $(CXXFLAGS) $(CFLAGS) $<

$(ODIR)/platform.o: $(SDIR)/platform.cpp $(SDIR)/platform.hpp $(SDIR)/platform_linux.cpp $(SDIR)/platform_darwin.cpp
	$(CXX) -o $@ $(CXXFLAGS) $(CFLAGS) $<

$(ODIR)/peripheral.o: $(SDIR)/peripheral/peripheral.cpp $(SDIR)/peripheral/peripheral.hpp $(SDIR)/lib.hpp
	$(CXX) -o $@ $(CXXFLAGS) $(CFLAGS) $<

$(ODIR)/http_server.o: $(SDIR)/http_server.cpp
	$(CXX) -o $@ $(CXXFLAGS) $(CFLAGS) $<

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -o $@ $(CFLAGS) $<

$(ODIR)/%.o: $(SDIR)/%.cpp $(SDIR)/%.hpp $(SDIR)/lib.hpp
	$(CXX) -o $@ $(CXXFLAGS) $(CFLAGS) $<

$(ODIR)/peripheral_%.o: $(SDIR)/peripheral/%.cpp $(SDIR)/peripheral/%.hpp $(SDIR)/peripheral/peripheral.hpp
	$(CXX) -o $@ $(CXXFLAGS) $(CFLAGS) $<

$(ODIR)/peripheral_computer.o: $(SDIR)/peripheral/computer_p.cpp $(SDIR)/peripheral/computer.hpp $(SDIR)/peripheral/peripheral.hpp
	$(CXX) -o $@ $(CXXFLAGS) $(CFLAGS) $<

clean: $(ODIR)
	rm -f craftos
	rm -f obj/*

rebuild: clean craftos

test: craftos
	./craftos --headless --script $(shell pwd)/CraftOSTest.lua
