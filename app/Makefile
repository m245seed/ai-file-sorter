# Detect platform
UNAME := $(shell uname | cut -d'-' -f1)

BIN_DIR := ./bin
OBJ_DIR := ./obj
SRC_DIR := ./lib
RESOURCES = resources/resources.c

ifeq ($(UNAME), Linux)
    PLATFORM := Linux
    CXXFLAGS += -DLINUX
    TARGET := $(BIN_DIR)/aifilesorter
    INSTALL_DIR := /usr/local/bin
    RESOURCE_DIR := /usr/local/share/aifilesorter
	LDFLAGS += -lcurl -ljsoncpp -lsqlite3 -lcrypto -lfmt -lspdlog -lssl -lllama -lggml -pthread
	LDFLAGS += -Wl,-rpath,'$$ORIGIN/../lib/precompiled'


else ifeq ($(UNAME), Darwin)
    PLATFORM := MacOS
    CXXFLAGS += -DMACOS
    TARGET := $(BIN_DIR)/aifilesorter
    INSTALL_DIR := /usr/local/bin
    RESOURCE_DIR := /usr/local/share/aifilesorter

    IS_APPLE_SILICON := $(shell sysctl -n machdep.cpu.brand_string | grep -i "Apple" > /dev/null && echo 1 || echo 0)

    ifeq ($(IS_APPLE_SILICON), 1)
        CXXFLAGS += -DENABLE_METAL
    endif

    LDFLAGS += -lcurl -ljsoncpp -lsqlite3 -lcrypto -lfmt -lspdlog -lssl -lllama -lggml -pthread
    LDFLAGS += -Wl,-rpath,'$$ORIGIN/../lib/precompiled'


else ifeq ($(UNAME), MINGW64_NT)
    PLATFORM := Windows (64-bit)
    CXXFLAGS += -DWINDOWS
    TARGET := $(BIN_DIR)/AiFileSorter.exe
	INSTALL_DIR := "C:/Program Files/Ai File Sorter"
    WINDRES = windres
    RC_FILE = resources/exe_icon.rc
    RC_OBJ = resources/exe_icon.o

    LDFLAGS += -mwindows -ljsoncpp -lcrypto -lcurl -lspdlog -lsqlite3 -lssl -lfmt -lllama -lggml
endif

# Compiler and flags
CXX = g++
CXXFLAGS += -std=c++20 -Wall $(shell pkg-config --cflags gtkmm-3.0)
CXXFLAGS += -g -O0

LDFLAGS += $(shell pkg-config --libs gtkmm-3.0)
INCLUDE_DIRS = -I./include -I./include/llama
LIB_DIRS = -L./lib/precompiled

# Source files
SRCS = main.cpp $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

.PHONY: all clean install uninstall

# Main rules
all: $(TARGET)
	@printf "\nFinished building AI File Sorter for %s\n" "$(PLATFORM)"

$(TARGET): $(OBJS) $(RC_OBJ)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(RESOURCES) $(LIB_DIRS) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/main.o: main.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Windows resource compilation
ifeq ($(PLATFORM), Windows (64-bit))
$(RC_OBJ): $(RC_FILE)
	$(WINDRES) -i $< -o $@
endif

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(RC_OBJ)

install: $(TARGET)

ifeq ($(PLATFORM), Linux)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/aifilesorter

else ifeq ($(PLATFORM), MacOS)
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/aifilesorter

else ifeq ($(PLATFORM), Windows (64-bit))
	mkdir -p $(INSTALL_DIR)/certs
	mkdir -p $(INSTALL_DIR)/share

	cp -v $(TARGET) $(INSTALL_DIR)/
	cp -v ./lib/precompiled/*.dll $(INSTALL_DIR)/
	cp -v "C:/msys64/mingw64/bin/gdbus.exe" $(INSTALL_DIR)/

	$(foreach dll, \
		libcairo-2 libfribidi-0 libharfbuzz-0 libpango-1.0-0 libssl-3-x64 \
		libcairo-gobject-2 libgcc_s_seh-1 libiconv-2 libpangocairo-1.0-0 \
		libstdc++-6 libcrypto-3-x64 libgdk-3-0 libgdk_pixbuf-2.0-0 libidn2-0 \
		libpangoft2-1.0-0 libgio-2.0-0 libintl-8 libzstd libjbig-0 libwebp-7 \
		libthai-0 libtiff-6 libcurl-4 libdatrie-1 libgiomm-2.4-1 libjpeg-8 \
		libpcre2-8-0 libpangowin32-1.0-0 libunistring-5 libdeflate libepoxy-0 \
		libglib-2.0-0 libjsoncpp-26 libpng16-16 libwinpthread-1 libLerc libexpat-1 \
		libglibmm-2.4-1 liblzma-5 libpsl-5 libatk-1.0-0 libffi-8 libgmodule-2.0-0 \
		libnghttp2-14 libsharpyuv-0 libbrotlicommon libfmt-11 libgobject-2.0-0 \
		libnghttp3-9 libsigc-2.0-0 zlib1 libbrotlidec libfontconfig-1 libgraphite2 \
		libngtcp2-16 libsqlite3-0 libbz2-1 libfreetype-6 libgtk-3-0 libngtcp2_crypto_ossl \
		libssh2-1 libpixman-1-0, \
		cp -v "C:/msys64/mingw64/bin/$(dll).dll" $(INSTALL_DIR)/;)

	cp -v "C:/msys64/usr/ssl/cert.pem" $(INSTALL_DIR)/certs/cacert.pem
	cp -vr "C:/msys64/mingw64/lib/gdk-pixbuf-2.0/" $(INSTALL_DIR)/gdk-pixbuf-2.0
	cp -vr "C:/msys64/mingw64/share/glib-2.0/schemas/" $(INSTALL_DIR)/schemas
	cp -vr "C:/msys64/mingw64/share/icons/" $(INSTALL_DIR)/share/icons
	cp -vr "C:/msys64/mingw64/share/themes/" $(INSTALL_DIR)/share/themes

	glib-compile-schemas $(INSTALL_DIR)/schemas
endif

uninstall:
	ifeq ($(PLATFORM), Linux)
		rm -f $(INSTALL_DIR)/aifilesorter
	else ifeq ($(PLATFORM), MacOS)
		rm -f $(INSTALL_DIR)/aifilesorter
	else ifeq ($(PLATFORM), Windows (32-bit))
		rm -rf $(INSTALL_DIR)
	else ifeq ($(PLATFORM), Windows (64-bit))
		rm -rf $(INSTALL_DIR)
	endif