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


# Logical Targets
# ===============

# Logical targets which require dependency files.
#DEPEND_TARGETS:=all default install run bindist
DEPEND_TARGETS:=all default
# Logical targets which do not require dependency files.
#NODEPEND_TARGETS:=clean config probe
NODEPEND_TARGETS:=clean
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


# Paths
# =====

#BUILD_PATH:=$(BUILD_BASE)/$(PLATFORM)-$(OPENMSX_FLAVOUR)
BUILD_PATH:=$(BUILD_BASE)

SOURCES_PATH:=src
RESOURCES_PATH:=resources

BINARY_PATH:=$(BUILD_PATH)/bin
#BINARY_FILE:=openmsx-debugger$(EXEEXT)
BINARY_FILE:=openmsx-debugger
#ifeq ($(VERSION_EXEC),true)
#  BINARY_FULL:=$(BINARY_PATH)/openmsx-dev$(CHANGELOG_REVISION)$(EXEEXT)
#else
  BINARY_FULL:=$(BINARY_PATH)/$(BINARY_FILE)
#endif


# Filesets
# ========

# If there will be more resource files, introduce a new category in node.mk.
RESOURCES_FULL:=$(RESOURCES_PATH)/resources.qrc

# Force evaluation upon assignment.
SOURCES_FULL:=
HEADERS_FULL:=
MOC_HDR_FULL:=
DIST_FULL:=
# Include root node.
CURDIR:=
include node.mk
# Remove "./" in front of file names.
# It can cause trouble because Make removes it automatically in rules.
SOURCES_FULL:=$(SOURCES_FULL:./%=%)
HEADERS_FULL:=$(HEADERS_FULL:./%=%)
MOC_HDR_FULL:=$(MOC_HDR_FULL:./%=%)
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

GEN_SRC_PATH:=$(BUILD_PATH)/src
MOC_SRC_FULL:=$(patsubst \
	$(SOURCES_PATH)/%.h,$(GEN_SRC_PATH)/moc_%.cpp,$(MOC_HDR_FULL) \
	)
RES_SRC_FULL:=$(patsubst \
	$(RESOURCES_PATH)/%.qrc,$(GEN_SRC_PATH)/qrc_%.cpp,$(RESOURCES_FULL) \
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


# Build Rules
# ===========

# Include dependency files.
ifneq ($(filter $(DEPEND_TARGETS),$(MAKECMDGOALS)),)
  -include $(DEPEND_FULL)
endif

# Clean up build tree of current flavour.
clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_PATH)

# Default target.
all: $(BINARY_FULL)

# Temporarily(?) hardcoded:
QT_BASE:=/opt/qt4
QT_COMPONENTS:=Core Gui Network
CXX:=g++
CXXFLAGS:=
COMPILE_FLAGS:= \
	$(addprefix -I$(QT_BASE)/include/Qt,$(QT_COMPONENTS)) \
	-I$(QT_BASE)/include
LINK_FLAGS:=-Wl,-rpath,$(QT_BASE)/lib -L$(QT_BASE)/lib $(addprefix -lQt,$(QT_COMPONENTS))
DEPEND_FLAGS:=
USE_SYMLINK:=true

# Generate Meta Object Compiler sources.
$(MOC_SRC_FULL): $(GEN_SRC_PATH)/moc_%.cpp: $(SOURCES_PATH)/%.h
	@echo "Generating $(patsubst $(GEN_SRC_PATH)/%,%,$@)..."
	@mkdir -p $(@D)
	@$(QT_BASE)/bin/moc -o $@ $<

# Generate resource source.
$(RES_SRC_FULL): $(GEN_SRC_PATH)/qrc_%.cpp: $(RESOURCES_PATH)/%.qrc
	@echo "Generating $(patsubst $(GEN_SRC_PATH)/%,%,$@)..."
	@mkdir -p $(@D)
	@$(QT_BASE)/bin/rcc -name $(<:$(RESOURCES_PATH)/%.qrc=%) $< -o $@

# Compile and generate dependency files in one go.
SRC_DEPEND_SUBST=$(patsubst $(SOURCES_PATH)/%.cpp,$(DEPEND_PATH)/%.d,$<)
GEN_DEPEND_SUBST=$(patsubst $(GEN_SRC_PATH)/%.cpp,$(DEPEND_PATH)/%.d,$<)
$(OBJECTS_FULL): $(OBJECTS_PATH)/%.o: $(SOURCES_PATH)/%.cpp $(DEPEND_PATH)/%.d
	@echo "Compiling $(patsubst $(SOURCES_PATH)/%,%,$<)..."
	@mkdir -p $(@D)
	@mkdir -p $(patsubst $(OBJECTS_PATH)%,$(DEPEND_PATH)%,$(@D))
	@$(CXX) \
		$(DEPEND_FLAGS) -MMD -MF $(SRC_DEPEND_SUBST) \
		-o $@ $(CXXFLAGS) $(COMPILE_FLAGS) -c $<
	@touch $@ # Force .o file to be newer than .d file.
$(GEN_OBJ_FULL): $(OBJECTS_PATH)/%.o: $(GEN_SRC_PATH)/%.cpp $(DEPEND_PATH)/%.d
	@echo "Compiling $(patsubst $(GEN_SRC_PATH)/%,%,$<)..."
	@mkdir -p $(@D)
	@mkdir -p $(patsubst $(OBJECTS_PATH)%,$(DEPEND_PATH)%,$(@D))
	@$(CXX) \
		$(DEPEND_FLAGS) -MMD -MF $(GEN_DEPEND_SUBST) \
		-o $@ $(CXXFLAGS) $(COMPILE_FLAGS) -c $<
	@touch $@ # Force .o file to be newer than .d file.
# Generate dependencies that do not exist yet.
# This is only in case some .d files have been deleted;
# in normal operation this rule is never triggered.
$(DEPEND_FULL):

# Link executable.
$(BINARY_FULL): $(OBJECTS_FULL) $(GEN_OBJ_FULL)
ifeq ($(OPENMSX_SUBSET),)
	@echo "Linking $(notdir $@)..."
	@mkdir -p $(@D)
	@$(CXX) -o $@ $(CXXFLAGS) $^ $(LINK_FLAGS)
  ifeq ($(USE_SYMLINK),true)
	@ln -sf $(@:$(BUILD_BASE)/%=%) $(BUILD_BASE)/$(BINARY_FILE)
  else
	@cp $@ $(BUILD_BASE)/$(BINARY_FILE)
  endif
else
	@echo "Not linking $(notdir $@) because only a subset was built."
endif
