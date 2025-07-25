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
    INSTALL_LIB_DIR := /usr/local/lib/aifilesorter
	LD_CONF_FILE := /etc/ld.so.conf.d/aifilesorter.conf

	LDFLAGS += -lcurl -ljsoncpp -lsqlite3 -lcrypto -lfmt -lspdlog -lssl -lllama -lggml -pthread
	LDFLAGS += -Wl,-rpath,'$$ORIGIN/../lib/precompiled'
	LDFLAGS += -Wl,-rpath-link=./lib/precompiled


else ifeq ($(UNAME), Darwin)
    export PATH := /opt/homebrew/opt/curl/bin:$(PATH)

    export PKG_CONFIG_PATH := /opt/homebrew/lib/pkgconfig:/opt/homebrew/share/pkgconfig:/opt/homebrew/opt/libffi/lib/pkgconfig:/opt/homebrew/opt/expat/lib/pkgconfig:$(PKG_CONFIG_PATH)

    export LDFLAGS += -L/opt/homebrew/opt/libffi/lib
    export CPPFLAGS += -I/opt/homebrew/opt/libffi/include

    PLATFORM := MacOS
    CXXFLAGS += -DMACOS -DENABLE_METAL -DGGML_USE_METAL -Wno-deprecated -Iinclude/llama
    TARGET := $(BIN_DIR)/aifilesorter
    INSTALL_DIR := /usr/local/bin
	INSTALL_LIB_DIR := /usr/local/lib

    IS_APPLE_SILICON := $(shell sysctl -n machdep.cpu.brand_string | grep -i "Apple" > /dev/null && echo 1 || echo 0)
    SPDLOG_PATH := $(shell if [ "$(IS_APPLE_SILICON)" = "1" ]; then echo "/opt/homebrew/include"; else echo "/usr/local/include"; fi)
    CXXFLAGS += -I$(SPDLOG_PATH)

    LDFLAGS += -lcurl -ljsoncpp -lsqlite3 -lcrypto -lfmt -lspdlog -lssl -lllama -lggml -pthread
	LDFLAGS += -framework Metal -framework Foundation
    LDFLAGS += -Wl,-rpath,@loader_path/lib


else ifeq ($(UNAME), MINGW64_NT)
    PLATFORM := Windows (64-bit)
    CXXFLAGS += -DWINDOWS
    TARGET := $(BIN_DIR)/AiFileSorter.exe
	INSTALL_DIR := "C:/Program Files/Ai File Sorter"
    WINDRES = windres
    RC_FILE = resources/exe_icon.rc
    RC_OBJ = resources/exe_icon.o

    LDFLAGS += -mwindows -lwininet -ljsoncpp -lcrypto -lcurl -lspdlog -lsqlite3 -lssl -lfmt -lllama -lggml
endif

# Compiler and flags
CXX = g++
CXXFLAGS += -std=c++20 -Wall $(shell pkg-config --cflags gtkmm-3.0)
CXXFLAGS += -O2

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
	@echo "Installing binary to $(INSTALL_DIR)..."
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/aifilesorter

	@echo "Installing libraries to $(INSTALL_LIB_DIR)..."
	mkdir -p $(INSTALL_LIB_DIR)
	cp lib/precompiled/*.so $(INSTALL_LIB_DIR)

	@echo "Registering library path with ldconfig..."
	echo "$(INSTALL_LIB_DIR)" > $(LD_CONF_FILE)
	ldconfig

	@echo "Installation complete."

else ifeq ($(PLATFORM), MacOS)
	@echo "Installing binary to $(INSTALL_DIR)..."
	mkdir -p $(INSTALL_DIR)
	cp $(TARGET) $(INSTALL_DIR)/aifilesorter

	@echo "Installing libraries to $(INSTALL_LIB_DIR)..."
	mkdir -p $(INSTALL_LIB_DIR)
	cp lib/precompiled/libggml-base.dylib $(INSTALL_LIB_DIR)
	cp lib/precompiled/libggml-blas.dylib $(INSTALL_LIB_DIR)
	cp lib/precompiled/libggml-cpu.dylib $(INSTALL_LIB_DIR)
	cp lib/precompiled/libggml-metal.dylib $(INSTALL_LIB_DIR)
	cp lib/precompiled/libggml.dylib $(INSTALL_LIB_DIR)
	cp lib/precompiled/libmtmd.dylib $(INSTALL_LIB_DIR)
	cp lib/precompiled/libllama.dylib $(INSTALL_LIB_DIR)

	install_name_tool -add_rpath $(INSTALL_LIB_DIR) $(INSTALL_DIR)/aifilesorter

	@echo "macOS installation complete."

else ifeq ($(PLATFORM), Windows (64-bit))
	mkdir -p $(INSTALL_DIR)/certs
	mkdir -p $(INSTALL_DIR)/share

	cp -v $(TARGET) $(INSTALL_DIR)/
	cp -v ./lib/precompiled/*.dll $(INSTALL_DIR)/
	cp -v "C:/msys64/mingw64/bin/gdbus.exe" $(INSTALL_DIR)/

	$(foreach dll, \
		libcairo-2 libfribidi-0 libharfbuzz-0 libpango-1.0-0 libssl-3-x64 \
		libcairo-gobject-2 libgcc_s_seh-1 libiconv-2 libpangocairo-1.0-0 \
		libstdc++-6 libcrypto-3-x64 libgdk-3-0 libgdk_pixbuf-2.0-0 libgfortran-5 \
		libidn2-0 libpangoft2-1.0-0 libgio-2.0-0 libintl-8 libgomp-1 libzstd \
		libjbig-0 libwebp-7 libthai-0 libtiff-6 libcurl-4 libdatrie-1 \
		libgiomm-2.4-1 libjpeg-8 libpcre2-8-0 libpangowin32-1.0-0 libunistring-5 \
		libdeflate libepoxy-0 libglib-2.0-0 libjsoncpp-26 libpng16-16 libwinpthread-1 \
		libLerc libexpat-1 libglibmm-2.4-1 liblzma-5 libpsl-5 libatk-1.0-0 libffi-8 \
		libgmodule-2.0-0 libnghttp2-14 libsharpyuv-0 libbrotlicommon libfmt-11 \
		libgobject-2.0-0 libnghttp3-9 libsigc-2.0-0 zlib1 libbrotlidec libfontconfig-1 \
		libgraphite2 libopenblas libngtcp2-16 libsqlite3-0 libbz2-1 libfreetype-6 \
		libgtk-3-0 libngtcp2_crypto_ossl libquadmath-0 libssh2-1 libpixman-1-0, \
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
	@echo "Uninstalling aifilesorter binary and libraries..."

	@echo "Removing binary from /usr/local/bin..."
	rm -f /usr/local/bin/aifilesorter

	@echo "Removing libraries from /usr/local/lib/aifilesorter..."
	rm -rf /usr/local/lib/aifilesorter

	@echo "Removing ld config file..."
	rm -f /etc/ld.so.conf.d/aifilesorter.conf

	@echo "Running ldconfig..."
	ldconfig

	@echo "Core uninstallation complete."

	@bash -c 'read -p "Do you also want to delete the downloaded local LLM models in ~/.local/share/aifilesorter/llms/? [y/N] " ans; \
		if [ "$$ans" = "y" ] || [ "$$ans" = "Y" ]; then \
			echo "Deleting ~/.local/share/aifilesorter/llms/..."; \
			rm -rf "$$HOME/.local/share/aifilesorter/llms"; \
		else \
			echo "Keeping downloaded models."; \
		fi'

else ifeq ($(PLATFORM), MacOS)
	@echo "Uninstalling aifilesorter binary and libraries on macOS..."

	@echo "Removing binary from $(INSTALL_DIR)..."
	rm -f $(INSTALL_DIR)/aifilesorter

	@echo "Removing installed libraries..."
	rm -f $(INSTALL_LIB_DIR)/libggml-base.dylib
	rm -f $(INSTALL_LIB_DIR)/libggml-blas.dylib
	rm -f $(INSTALL_LIB_DIR)/libggml-cpu.dylib
	rm -f $(INSTALL_LIB_DIR)/libggml-metal.dylib
	rm -f $(INSTALL_LIB_DIR)/libggml.dylib
	rm -f $(INSTALL_LIB_DIR)/libmtmd.dylib
	rm -f $(INSTALL_LIB_DIR)/libllama.dylib

	@echo "Core uninstallation complete."

	@read -p "Do you also want to delete the downloaded local LLM models in ~/Library/Application\ Support/aifilesorter/llms/? [y/N] " ans; \
	if [ "$$ans" = "y" ] || [ "$$ans" = "Y" ]; then \
		echo "Deleting ~/Library/Application Support/aifilesorter/llms/..."; \
		rm -rf "$$HOME/Library/Application Support/aifilesorter/llms"; \
	else \
		echo "Keeping downloaded models."; \
	fi

else ifeq ($(PLATFORM), Windows (64-bit))
	rm -rf $(INSTALL_DIR)
endif
