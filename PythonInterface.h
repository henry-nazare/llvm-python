#ifndef _PYTHONINTERFACE_H_
#define _PYTHONINTERFACE_H_

#include <python2.7/Python.h>

#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

raw_ostream& operator<<(raw_ostream& OS, PyObject &Obj);

// A ModulePass allows global initialization and finalization, which we need
// for calling Py_Init and Py_Finalize.
class PythonInterface : public ModulePass {
public:
  static char ID;
  PythonInterface() : ModulePass(ID) { }

  virtual bool runOnModule(Module&) {
    return false;
  }

  virtual bool doInitialization(Module&);
  virtual bool doFinalization(Module&);
};

// Holds each function we need to get a reference to.
class PythonObjInfo {
public:
  PythonObjInfo(const char *Mod, const char *Fn)
    : Mod_(Mod), Class_(nullptr), Fn_(Fn) { }
  PythonObjInfo(const char *Mod, const char *Class, const char *Fn)
    : Mod_(Mod), Class_(Class), Fn_(Fn) { }

  PyObject *operator()(std::initializer_list<PyObject*> Items);

private:
  bool populate();

  const char *Mod_, *Class_, *Fn_;
  PyObject *Obj_;
};

namespace llvmpy {

struct PyObjectHolder {
  PyObjectHolder(PyObject *Obj) : Obj_(Obj) {}

  virtual PyObject *get() const {
    return Obj_;
  }

private:
  PyObject *Obj_;
};

template <typename T>
struct topy {
};

template <>
struct topy<int> : public PyObjectHolder {
  topy(int Int);
};

template <>
struct topy<const char*> : public PyObjectHolder {
  topy(const char *Str);
};

template <>
struct topy<std::string> : public topy<const char*> {
  topy(std::string Str);
};

template <>
struct topy<APInt> : public PyObjectHolder {
  topy(APInt Int);
};

template <typename T>
PyObject *Get(const T Val) {
  return topy<T>(Val).get();
}

} // end namespace llvmpy

raw_ostream& operator<<(raw_ostream& OS, llvmpy::PyObjectHolder &Holder);

#endif

