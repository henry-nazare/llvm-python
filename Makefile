##======- lib/*/*/Makefile ------------------------------*- Makefile -*-======##
##===----------------------------------------------------------------------===##

LEVEL = ../../..
LIBRARYNAME = Python
LOADABLE_MODULE = 1

include $(LEVEL)/Makefile.common

CXXFLAGS += -std=c++0x -Wno-deprecated-declarations -fexceptions -Wall -Wextra
LIBS += -lpython2.7

