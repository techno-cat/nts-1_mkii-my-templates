##############################################################################
# Configuration for Makefile
#

PROJECT := osc_tmpl
PROJECT_TYPE := osc

##############################################################################
# Sources
#

# C sources
UCSRC = ./user/header.c $(wildcard ./user/lib/*.c)

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

