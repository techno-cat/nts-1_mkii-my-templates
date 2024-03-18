##############################################################################
# Configuration for Makefile
#

PROJECT := delfx_simple
PROJECT_TYPE := delfx

##############################################################################
# Sources
#

# C sources
UCSRC = $(wildcard ./user/*.c) $(wildcard ./user/lib/*.c)

# C++ sources
UCXXSRC = ./user/unit.cc

# List ASM source files here
UASMSRC = 

UASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = ./user/lib

##############################################################################
# Library Paths
#

ULIBDIR = 

##############################################################################
# Libraries
#

ULIBS  = -lm

##############################################################################
# Macros
#

UDEFS = 

