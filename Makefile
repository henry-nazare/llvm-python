##======- Makefile --------------------------------------*- Makefile -*-======##
##===----------------------------------------------------------------------===##

PROJECT_NAME = Python
LIBRARYNAME = Python
LOADABLE_MODULE = 1
USEDLIBS =

LEVEL = .

LLVM_SRC_ROOT = /home/nazare/programs/sra/llvm-3.7
LLVM_OBJ_ROOT = /home/nazare/programs/sra/llvm-3.7/build
PROJ_SRC_ROOT = .
PROJ_OBJ_ROOT = .
PROJ_INSTALL_ROOT = /home/nazare/programs/sra/llvm-3.7/build/Debug+Asserts/lib/..

include $(LLVM_OBJ_ROOT)/Makefile.config

CXXFLAGS += -std=c++0x -Wno-deprecated-declarations -fexceptions -Wall -Wextra
LIBS += -lpython2.7

include $(LLVM_SRC_ROOT)/Makefile.rules
