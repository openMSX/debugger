# $Id$
#
# openMSX Build System
# ====================
#
# This is the home made build system for openMSX, adapted for the openMSX
# debugger.
#
# Used a lot of ideas from Peter Miller's excellent paper
# "Recursive Make Considered Harmful".
# http://www.tip.net.au/~millerp/rmch/recu-make-cons-harm.html
#
# TODO: Rename OPENMSX_SUBSET to SUBSET?


# Python Interpreter
# ==================

# We need Python from the 2.x series, version 2.5 or higher.
# Usually this executable is available as just "python", but on some systems
# you might have to be more specific, for example "python2" or "python2.6".
# Or if the Python interpreter is not in the search path, you can specify its
# full path.
PYTHON?=python


# Logical Targets
# ===============

# Logical targets which require dependency files.
#DEPEND_TARGETS:=all app default install run bindist
DEPEND_TARGETS:=all app default
# Logical targets which do not require dependency files.
#NODEPEND_TARGETS:=clean config probe dist
NODEPEND_TARGETS:=clean dist
# Mark all logical targets as such.
.PHONY: $(DEPEND_TARGETS) $(NODEPEND_TARGETS)

# Default target; make sure this is always the first target in this Makefile.
MAKECMDGOALS?=default
default: all


# Base Directories
# ================

# All created files will be inside this directory.
BUILD_BASE:=derived

# All global Makefiles are inside this directory.
MAKE_PATH:=build

# Platforms
# =========

ifeq ($(origin OPENMSX_TARGET_OS),environment)
# Do not perform autodetection if platform was specified by the user.
else # OPENMSX_TARGET_OS not from environment

DETECTSYS_PATH:=$(BUILD_BASE)/detectsys
DETECTSYS_MAKE:=$(DETECTSYS_PATH)/detectsys.mk
DETECTSYS_SCRIPT:=$(MAKE_PATH)/detectsys.py

-include $(DETECTSYS_MAKE)

$(DETECTSYS_MAKE): $(DETECTSYS_SCRIPT)
	@echo "Autodetecting native system:"
	@mkdir -p $(@D)
	@$(PYTHON) $< > $@

endif # OPENMSX_TARGET_OS

PLATFORM:=
ifneq ($(origin OPENMSX_TARGET_OS),undefined)
PLATFORM:=$(OPENMSX_TARGET_OS)
endif

# Ignore rest of Makefile if autodetection was not performed yet.
# Note that the include above will force a reload of the Makefile.
ifneq ($(PLATFORM),)

# Load OS specific settings.
#$(call DEFCHECK,OPENMSX_TARGET_OS)
#include $(MAKE_PATH)/platform-$(OPENMSX_TARGET_OS).mk
# Check that all expected variables were defined by OS specific Makefile:
# - executable file name extension
#$(call DEFCHECK,EXEEXT)
# - platform supports symlinks?
#$(call BOOLCHECK,USE_SYMLINK)


# Paths
# =====

#BUILD_PATH:=$(BUILD_BASE)/$(PLATFORM)-$(OPENMSX_FLAVOUR)
BUILD_PATH:=$(BUILD_BASE)
GEN_SRC_PATH:=$(BUILD_PATH)/src

SOURCES_PATH:=src
RESOURCES_PATH:=resources

ifeq ($(OPENMSX_TARGET_OS),darwin)
# Note: Make cannot deal with spaces inside paths: it will even see BINARY_FULL
#       as two files instead of one. Therefore, we use an underscore during
#       development and we'll have to rename the app folder for the release
#       versions.
APP_SUPPORT_PATH:=build/package-darwin
APP_PATH:=$(BUILD_PATH)/openMSX_Debugger.app
APP_PLIST:=$(APP_PATH)/Contents/Info.plist
APP_ICON:=$(APP_PATH)/Contents/Resources/debugger-logo.icns
APP_RESOURCES:=$(APP_ICON)
BINARY_PATH:=$(APP_PATH)/Contents/MacOS
PKGINFO_FULL:=$(APP_PATH)/Contents/PkgInfo
else
BINARY_PATH:=$(BUILD_PATH)/bin
endif
#BINARY_FILE:=openmsx-debugger$(EXEEXT)
BINARY_FILE:=openmsx-debugger
BINARY_FULL:=$(BINARY_PATH)/$(BINARY_FILE)

VERSION_SCRIPT:=build/version2code.py
VERSION_HEADER:=$(BUILD_PATH)/config/Version.ii
GENERATED_HEADERS:=$(VERSION_HEADER)


# Filesets
# ========

# If there will be more resource files, introduce a new category in node.mk.
RESOURCES_FULL:=$(RESOURCES_PATH)/resources.qrc

# Force evaluation upon assignment.
SOURCES_FULL:=
HEADERS_FULL:=
MOC_HDR_FULL:=
UI_FULL:=
DIST_FULL:=
# Include root node.
CURDIR:=
include node.mk
# Remove "./" in front of file names.
# It can cause trouble because Make removes it automatically in rules.
SOURCES_FULL:=$(SOURCES_FULL:./%=%)
HEADERS_FULL:=$(HEADERS_FULL:./%=%)
MOC_HDR_FULL:=$(MOC_HDR_FULL:./%=%)
UI_FULL:=$(UI_FULL:./%=%)
DIST_FULL:=$(DIST_FULL:./%=%)
# Apply subset to sources list.
SOURCES_FULL:=$(filter $(SOURCES_PATH)/$(OPENMSX_SUBSET)%,$(SOURCES_FULL))
ifeq ($(SOURCES_FULL),)
$(error Sources list empty $(if \
	$(OPENMSX_SUBSET),after applying subset "$(OPENMSX_SUBSET)*"))
endif
# Sanity check: only .cpp files are allowed in sources list,
# because we don't have any way to build other sources.
NON_CPP_SOURCES:=$(filter-out %.cpp,$(SOURCES_FULL))
ifneq ($(NON_CPP_SOURCES),)
$(error The following sources files do not have a .cpp extension: \
$(NON_CPP_SOURCES))
endif

MOC_SRC_FULL:=$(patsubst \
	$(SOURCES_PATH)/%.h,$(GEN_SRC_PATH)/moc_%.cpp,$(MOC_HDR_FULL) \
	)
RES_SRC_FULL:=$(patsubst \
	$(RESOURCES_PATH)/%.qrc,$(GEN_SRC_PATH)/qrc_%.cpp,$(RESOURCES_FULL) \
	)
UI_HDR_FULL:=$(patsubst \
	$(SOURCES_PATH)/%.ui,$(GEN_SRC_PATH)/ui_%.h,$(UI_FULL) \
	)
GEN_SRC_FULL:=$(MOC_SRC_FULL) $(RES_SRC_FULL)

SOURCES:=$(SOURCES_FULL:$(SOURCES_PATH)/%.cpp=%)
GEN_SRC:=$(GEN_SRC_FULL:$(GEN_SRC_PATH)/%.cpp=%)
HEADERS:=$(HEADERS_FULL:$(SOURCES_PATH)/%=%)

DEPEND_PATH:=$(BUILD_PATH)/dep
DEPEND_FULL:=$(addsuffix .d,$(addprefix $(DEPEND_PATH)/,$(SOURCES)))
DEPEND_FULL+=$(addsuffix .d,$(addprefix $(DEPEND_PATH)/,$(GEN_SRC)))

OBJECTS_PATH:=$(BUILD_PATH)/obj
OBJECTS_FULL:=$(addsuffix .o,$(addprefix $(OBJECTS_PATH)/,$(SOURCES)))
GEN_OBJ_FULL:=$(addsuffix .o,$(addprefix $(OBJECTS_PATH)/,$(GEN_SRC)))

ifeq ($(OPENMSX_TARGET_OS),mingw32)
RESOURCE_SRC:=$(RESOURCES_PATH)/openmsx-debugger.rc
RESOURCE_OBJ:=$(OBJECTS_PATH)/resources.o
RESOURCE_SCRIPT:=build/win_resource.py
RESOURCE_HEADER:=$(BUILD_PATH)/config/resource-info.h
else
RESOURCE_OBJ:=
endif

# Build Rules
# ===========

# Include dependency files.
ifneq ($(filter $(DEPEND_TARGETS),$(MAKECMDGOALS)),)
  -include $(DEPEND_FULL)
endif

# Clean up build tree of current flavour.
# We don't have flavours (yet?), so clean up everything except "detectsys".
clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJECTS_PATH)
	@rm -rf $(DEPEND_PATH)
	@rm -rf $(GEN_SRC_PATH)
ifeq ($(OPENMSX_TARGET_OS),darwin)
	@rm -rf $(APP_PATH)
else
	@rm -rf $(BINARY_PATH)
endif

# Generate version header.
.PHONY: forceversionextraction
forceversionextraction:
$(VERSION_HEADER): forceversionextraction
	@$(PYTHON) $(VERSION_SCRIPT) $@

# Default target.
ifeq ($(OPENMSX_TARGET_OS),darwin)
all: app
else
all: $(BINARY_FULL)
endif

QT_INSTALL_HEADERS:=$(shell qmake -query QT_INSTALL_HEADERS)
QT_INSTALL_LIBS:=$(shell qmake -query QT_INSTALL_LIBS)
QT_INSTALL_BINS:=$(shell qmake -query QT_INSTALL_BINS)
# On MingW32 you get backslashes from qmake -query, which we don't want:
ifeq ($(OPENMSX_TARGET_OS),mingw32)
QT_INSTALL_HEADERS:=$(subst \,/,$(QT_INSTALL_HEADERS))
QT_INSTALL_LIBS:=$(subst \,/,$(QT_INSTALL_LIBS))
QT_INSTALL_BINS:=$(subst \,/,$(QT_INSTALL_BINS))
endif
QT_COMPONENTS:=Core Widgets Gui Network Xml
QT_HEADER_DIRS:=$(addprefix $(QT_INSTALL_HEADERS)/Qt,$(QT_COMPONENTS))
QT_HEADER_DIRS+=$(QT_INSTALL_HEADERS)
ifeq ($(OPENMSX_TARGET_OS),darwin)
QT_HEADER_DIRS+=$(patsubst %,/Library/Frameworks/Qt%.framework/Headers,$(QT_COMPONENTS))
endif

CXX:=g++
WINDRES?=windres
CXXFLAGS:= -g -fPIC
INCLUDE_INTERNAL:=$(sort $(foreach header,$(HEADERS_FULL),$(patsubst %/,%,$(dir $(header)))))
INCLUDE_INTERNAL+=$(BUILD_PATH)/config
COMPILE_FLAGS:=$(addprefix -I,$(QT_HEADER_DIRS) $(INCLUDE_INTERNAL) $(GEN_SRC_PATH))
# Enable C++11
COMPILE_FLAGS+=-std=c++11
ifeq ($(OPENMSX_TARGET_OS),darwin)
LINK_FLAGS:=-F$(QT_INSTALL_LIBS) $(addprefix -framework Qt,$(QT_COMPONENTS))
SDK_PATH:=/Developer/SDKs/MacOSX10.4u.sdk
OSX_VER:=10.4
OSX_MIN_REQ:=1040
COMPILE_ENV:=NEXT_ROOT=$(SDK_PATH) MACOSX_DEPLOYMENT_TARGET=$(OSX_VER)
LINK_ENV:=NEXT_ROOT=$(SDK_PATH) MACOSX_DEPLOYMENT_TARGET=$(OSX_VER)
COMPILE_FLAGS+=-D__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=$(OSX_MIN_REQ)
COMPILE_FLAGS+=-isysroot $(SDK_PATH)
LINK_FLAGS+=-Wl,-syslibroot,$(SDK_PATH)
else
COMPILE_ENV:=
LINK_ENV:=
ifeq ($(OPENMSX_TARGET_OS),mingw32)
COMPILE_FLAGS+=-static-libgcc -static-libstdc++
LINK_FLAGS:=-Wl,-rpath,$(QT_INSTALL_BINS) -L$(QT_INSTALL_BINS) $(addprefix -lQt5,$(QT_COMPONENTS)) -lws2_32 -lsecur32 -mwindows -static-libgcc -static-libstdc++
else
LINK_FLAGS:=-Wl,-rpath,$(QT_INSTALL_LIBS) -L$(QT_INSTALL_LIBS) $(addprefix -lQt5,$(QT_COMPONENTS))
endif
endif
DEPEND_FLAGS:=

# GCC flags:
# (these should be omitted if we ever want to support other compilers)
# Generic compilation flags.
CXXFLAGS+=-pipe
# Stricter warning and error reporting.
CXXFLAGS+=-Wall
# Empty definition of used headers, so header removal doesn't break things.
DEPEND_FLAGS+=-MP

# Generate Meta Object Compiler sources.
$(MOC_SRC_FULL): $(GEN_SRC_PATH)/moc_%.cpp: $(SOURCES_PATH)/%.h
	@echo "Generating $(@F)..."
	@mkdir -p $(@D)
	@$(QT_INSTALL_BINS)/moc -o $@ $<

# Generate resource source.
$(RES_SRC_FULL): $(GEN_SRC_PATH)/qrc_%.cpp: $(RESOURCES_PATH)/%.qrc
	@echo "Generating $(@F)..."
	@mkdir -p $(@D)
	@$(QT_INSTALL_BINS)/rcc -name $(<:$(RESOURCES_PATH)/%.qrc=%) $< -o $@

# Generate ui files.
$(UI_HDR_FULL): $(GEN_SRC_PATH)/ui_%.h: $(SOURCES_PATH)/%.ui
	@echo "Generating $(@F)..."
	@mkdir -p $(@D)
	@$(QT_INSTALL_BINS)/uic -o $@ $<
# This is a workaround for the lack of order-only dependencies in GNU Make
# versions before than 3.80 (for example Mac OS X 10.3 still ships with 3.79).
# It creates a dummy file, which is never modified after its initial creation.
# If a rule that produces a file does not modify that file, Make considers the
# target to be up-to-date. That way, the targets "ui-dummy-file" depends on
# will always be checked before compilation, but they will not cause all object
# files to be considered outdated.
GEN_DUMMY_FILE:=$(GEN_SRC_PATH)/dummy-file
$(GEN_DUMMY_FILE): $(UI_HDR_FULL) $(GENERATED_HEADERS)
	@test -e $@ || touch $@

# Compile and generate dependency files in one go.
SRC_DEPEND_SUBST=$(patsubst $(SOURCES_PATH)/%.cpp,$(DEPEND_PATH)/%.d,$<)
GEN_DEPEND_SUBST=$(patsubst $(GEN_SRC_PATH)/%.cpp,$(DEPEND_PATH)/%.d,$<)
$(OBJECTS_FULL): $(GEN_DUMMY_FILE)
$(OBJECTS_FULL): $(OBJECTS_PATH)/%.o: $(SOURCES_PATH)/%.cpp $(DEPEND_PATH)/%.d
	@echo "Compiling $(patsubst $(SOURCES_PATH)/%,%,$<)..."
	@mkdir -p $(@D)
	@mkdir -p $(patsubst $(OBJECTS_PATH)%,$(DEPEND_PATH)%,$(@D))
	@$(COMPILE_ENV) $(CXX) \
		$(DEPEND_FLAGS) -MMD -MF $(SRC_DEPEND_SUBST) \
		-o $@ $(CXXFLAGS) $(COMPILE_FLAGS) -c $<
	@touch $@ # Force .o file to be newer than .d file.
$(GEN_OBJ_FULL): $(OBJECTS_PATH)/%.o: $(GEN_SRC_PATH)/%.cpp $(DEPEND_PATH)/%.d
	@echo "Compiling $(patsubst $(GEN_SRC_PATH)/%,%,$<)..."
	@mkdir -p $(@D)
	@mkdir -p $(patsubst $(OBJECTS_PATH)%,$(DEPEND_PATH)%,$(@D))
	@$(COMPILE_ENV) $(CXX) \
		$(DEPEND_FLAGS) -MMD -MF $(GEN_DEPEND_SUBST) \
		-o $@ $(CXXFLAGS) $(COMPILE_FLAGS) -c $<
	@touch $@ # Force .o file to be newer than .d file.
# Generate dependencies that do not exist yet.
# This is only in case some .d files have been deleted;
# in normal operation this rule is never triggered.
$(DEPEND_FULL):

# Windows resources that are added to the executable.
ifeq ($(OPENMSX_TARGET_OS),mingw32)
$(RESOURCE_HEADER): forceversionextraction
	@$(PYTHON) $(RESOURCE_SCRIPT) $@
$(RESOURCE_OBJ): $(RESOURCE_SRC) $(RESOURCE_HEADER)
	@echo "Compiling resources..."
	@mkdir -p $(@D)
	@$(WINDRES) $(addprefix --include-dir=,$(^D)) -o $@ -i $<
endif

# Link executable.
$(BINARY_FULL): $(OBJECTS_FULL) $(GEN_OBJ_FULL) $(RESOURCE_OBJ)
ifeq ($(OPENMSX_SUBSET),)
	@echo "Linking $(@F)..."
	@mkdir -p $(@D)
	@$(LINK_ENV) $(CXX) -o $@ $(CXXFLAGS) $^ $(LINK_FLAGS)
else
	@echo "Not linking $(notdir $@) because only a subset was built."
endif

# Application folder.
ifeq ($(OPENMSX_TARGET_OS),darwin)
app: $(BINARY_FULL) $(PKGINFO_FULL) $(APP_PLIST) $(APP_RESOURCES)

$(PKGINFO_FULL):
	@echo "Generating $(@F)..."
	@mkdir -p $(@D)
	@echo "APPLoMXD" > $@

$(APP_PLIST): $(APP_PATH)/Contents/%: $(APP_SUPPORT_PATH)/%
	@echo "Generating $(@F)..."
	@mkdir -p $(@D)
	@sed -e 's/%ICON%/$(notdir $(APP_ICON))/' \
		-e 's/%VERSION%/$(PACKAGE_DETAILED_VERSION)/' < $< > $@

$(APP_RESOURCES): $(APP_PATH)/Contents/Resources/%: $(APP_SUPPORT_PATH)/%
	@echo "Copying $(@F)..."
	@mkdir -p $(@D)
	@cp $< $@
endif


# Source Packaging
# ================

DIST_BASE:=$(BUILD_BASE)/dist
DIST_PATH:=$(DIST_BASE)/$(PACKAGE_FULL)

dist: $(DETECTSYS_SCRIPT)
	@echo "Removing any old distribution files..."
	@rm -rf $(DIST_PATH)
	@echo "Gathering files for distribution..."
	@mkdir -p $(DIST_PATH)
	@build/install-recursive.sh $(DIST_FULL) $(DIST_PATH)
	@build/install-recursive.sh $(HEADERS_FULL) $(DIST_PATH)
	@build/install-recursive.sh $(SOURCES_FULL) $(DIST_PATH)
	@build/install-recursive.sh $(UI_FULL) $(DIST_PATH)
	@echo "Creating tarball..."
	@cd $(DIST_BASE) && \
		GZIP=--best tar zcf $(PACKAGE_FULL).tar.gz $(PACKAGE_FULL)

endif # PLATFORM
