##################################################
##                                              ##
##      Simple Universal C/C++ Makefile v2.0    ##
##                                              ##
##################################################

##################################################
##            GLOBAL CONFIGURATION              ##
##################################################

# Source directories with the .c and .cpp files. Separate multiple directories by space.
SRC_DIRS = demo

# Include directories with header files. Separate multiple directories by space.
INC_DIRS = lib $(SRC_DIRS)

# Build output directory
OUTPUT_DIRECTORY = build

# C compiler configuration
C_EXTENSION = c
CC  = clang

# C++ compiler configuration
CXX_EXTENSION = cpp
CXX = clang++

##################################################
##            BUILD CONFIGURATIONS              ##
#                                                #
# Adding a configuration is simple:              #
# 1. decide on a config name, e.g., XX           #
# 2. setup variables prefixed with the config    #
#   XX_EXECUTABLE: name of the binary            #
#   XX_C_FLAGS: flags for the C compiler         #
#   XX_CXX_FLAGS: flags for the C++ compiler     #
#   XX_LINK_FLAGS: flags for the linker          #
# 3. (optional) create a shorthand make target   #
#   xx:                                          #
#   	@+make compile BUILD_TYPE=XX             #
#   	@$(OUTPUT_DIRECTORY)/$(XX_EXECUTABLE)    #
##################################################

DEBUG_EXECUTABLE   = debug
DEBUG_COMMON_FLAGS = -DDEBUG -Wall -Wextra -Wshadow -pedantic -g3 -O0 -fsanitize=address,undefined -fno-sanitize-recover=all
DEBUG_C_FLAGS      = $(DEBUG_COMMON_FLAGS) -std=c11
DEBUG_CXX_FLAGS    = $(DEBUG_COMMON_FLAGS) -std=c++20
DEBUG_LINK_FLAGS   = -fsanitize=address,undefined -fno-sanitize-recover=all -lfmt
debug:
	@+make compile BUILD_TYPE=DEBUG
	@$(OUTPUT_DIRECTORY)/$(DEBUG_EXECUTABLE)


RELEASE_EXECUTABLE   = release
RELEASE_COMMON_FLAGS = -Wall -Wextra -Wshadow -pedantic -O3 -fomit-frame-pointer
RELEASE_C_FLAGS      = $(RELEASE_COMMON_FLAGS) -std=c11
RELEASE_CXX_FLAGS    = $(RELEASE_COMMON_FLAGS) -std=c++20
RELEASE_LINK_FLAGS   = -lfmt
release:
	@+make compile BUILD_TYPE=RELEASE
	@$(OUTPUT_DIRECTORY)/$(RELEASE_EXECUTABLE)


##################################################
##            CORE (do not touch)               ##
##################################################

EXECUTABLE = $($(BUILD_TYPE)_EXECUTABLE)
C_FLAGS = $($(BUILD_TYPE)_C_FLAGS)
CXX_FLAGS = $($(BUILD_TYPE)_CXX_FLAGS)
LINK_FLAGS = $($(BUILD_TYPE)_LINK_FLAGS)
OBJ_DIR = obj_$(EXECUTABLE)

# Helper functions to find all files
keep_files = $(foreach x,$(1),$(if $(wildcard $(x)/.),,$(x)))
find_files = $(call keep_files,$(wildcard $(1)/*)) $(foreach dir,$(wildcard $(1)/*/.),$(call find_files,$(dir:/.=)))

# Map existing files into output directory...
FILE_LIST := $(foreach dir,$(SRC_DIRS),$(patsubst $(dir)/%,$(OUTPUT_DIRECTORY)/$(OBJ_DIR)/$(dir)/%,$(call find_files,$(dir))))
# ...split into C and CXX files...
C_LIST := $(filter %.$(C_EXTENSION),$(FILE_LIST))
CXX_LIST := $(filter %.$(CXX_EXTENSION),$(FILE_LIST))
# ...and obtain final object file list
OBJ_FILES := $(C_LIST:.$(C_EXTENSION)=.o) $(CXX_LIST:.$(CXX_EXTENSION)=.o)

# Verbosity flag defaults to 0
V = 0

# if verbosity is set to 0, pipe outputs to null and supress command printing
ifeq ($(V),0)
	SUPPRESS_CMD := @
	PIPE := > /dev/null
endif

# Options to generate dependency files
DEP_FLAGS = -MT $@ -MMD -MP -MF $(OUTPUT_DIRECTORY)/$(OBJ_DIR)/$*.d

# select appropriate linker
ifeq ($(CXX_LIST),)
	LINK := $(CC)
else
	LINK := $(CXX)
endif

# store make invocation time
START_TIME := $(shell date +%s%3N)

# tell make to not print spam on recursive calls
MAKEFLAGS += --no-print-directory

##################################################
.PHONY: clean check compile directories

clean:
	@echo  Removing build artifacts...
	$(SUPPRESS_CMD)rm -rf $(OUTPUT_DIRECTORY)

check:
ifeq ($(EXECUTABLE),)
	$(error No configuration for BUILD_TYPE '$(BUILD_TYPE)')
endif

# Create obj directory and compile
compile: directories $(OUTPUT_DIRECTORY)/$(EXECUTABLE) | check
	@diff=$$(($(shell date +%s%3N) - $(START_TIME))); echo '$(BUILD_TYPE) build completed in '$$(($$diff / 1000))'.'$$(($$diff % 1000))'s'
	@echo

# Create the obj directory
directories: check
	@echo  '_______Building $(BUILD_TYPE)_______'
	@mkdir -p $(OUTPUT_DIRECTORY)/$(OBJ_DIR)/

# Link output
$(OUTPUT_DIRECTORY)/$(EXECUTABLE): $(OBJ_FILES) | check
	@echo
ifeq ($(V), 0)
	@echo  -e 'LINK\t$(EXECUTABLE)'
endif
	$(SUPPRESS_CMD)$(LINK) -o $(OUTPUT_DIRECTORY)/$(EXECUTABLE) $(OBJ_FILES) $(LINK_FLAGS) $(PIPE)
	@echo

# Compile code files
$(OUTPUT_DIRECTORY)/$(OBJ_DIR)/%.o: %.$(C_EXTENSION) Makefile | check
ifeq ($(V), 0)
	@echo  -e 'CC\t$<'
endif
	@mkdir -p '$(dir $@)'
	$(SUPPRESS_CMD)$(CC) -c $< -o $@ $(DEP_FLAGS) $(C_FLAGS) $(foreach dir,$(INC_DIRS),-I $(dir)) $(PIPE)
	@touch $@

$(OUTPUT_DIRECTORY)/$(OBJ_DIR)/%.o: %.$(CXX_EXTENSION) Makefile | check
ifeq ($(V), 0)
	@echo  -e 'CXX\t$<'
endif
	@mkdir -p '$(dir $@)'
	$(SUPPRESS_CMD)$(CXX) -c $< -o $@ $(DEP_FLAGS) $(CXX_FLAGS) $(foreach dir,$(INC_DIRS),-I $(dir)) $(PIPE)
	@touch $@

# Pull in dependency info for existing .o files
-include $(OBJ_FILES:.o=.d)
