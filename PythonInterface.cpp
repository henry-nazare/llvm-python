//===------------------------- PythonInterface.cpp ------------------------===//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "python-interface"

#include "PythonInterface.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

raw_ostream& operator<<(raw_ostream& OS, PyObject &Obj) {
  PyObject *Repr = PyObject_Repr(&Obj);
  OS << PyString_AsString(Repr);
  return OS;
}

static RegisterPass<PythonInterface>
  X("python-interface", "LLVM/Python interface");
char PythonInterface::ID = 0;

static PyObject *GetModule(const char *Mod) {
  PyObject *ModStr = PyString_FromString(Mod);
  return PyImport_Import(ModStr);
}

static PyObject *GetClass(const char *Mod, const char *Class) {
  PyEval_GetBuiltins();
  PyObject *DictObj = PyModule_GetDict(GetModule(Mod));
  return PyDict_GetItemString(DictObj, Class);
}

static PyObject *GetAttr(const char *Mod, const char *Class, const char *Attr) {
  PyObject *ClassObj = GetClass(Mod, Class);
  return PyObject_GetAttrString(ClassObj, Attr);
}

static PyObject *GetAttr(const char *Mod, const char *Attr) {
  return PyObject_GetAttrString(GetModule(Mod), Attr);
}

static PyObject *GetBuiltin(const char *Attr) {
  PyObject *Builtins = PyEval_GetBuiltins();
  return PyDict_GetItemString(Builtins, Attr);
}

static PyObject *CreateTuple(std::initializer_list<PyObject*> Items) {
  PyObject *Tuple = PyTuple_New(Items.size());
  unsigned Idx = 0;
  for (auto &Item : Items) {
    PyTuple_SetItem(Tuple, Idx++, Item);
  }

  return Tuple;
}

bool PythonInterface::doInitialization(Module&) {
  static char *Argv[] = { (char*)"opt" };
  Py_Initialize();
  PySys_SetArgv(1, Argv);
  return true;
}

bool PythonInterface::doFinalization(Module&) {
  Py_Finalize();
  return true;
}

bool PythonObjInfo::populate() {
  if (!strcmp(Mod_, "__builtins__")) {
    Obj_ = GetBuiltin(Fn_);
  } else if (Class_) {
    Obj_ = Fn_ ? GetAttr(Mod_, Class_, Fn_) : GetClass(Mod_, Class_);
  } else {
    Obj_ = GetAttr(Mod_, Fn_);
  }

  if (Obj_) {
    return true;
  }

  if (Class_) {
    if (Fn_) {
      errs() << "PythonInterface: could not get member function " << Mod_ << "."
          << Class_ << "." << Fn_ << "\n";
    } else {
      errs() << "PythonInterface: could not get class " << Mod_ << "." << Class_
          << "\n";
    }
  } else {
    errs() << "PythonInterface: could not get module function " << Mod_ << "."
        << Fn_ << "\n";
  }

  return false;
}

PyObject *PythonObjInfo::operator()(std::initializer_list<PyObject*> Items) {
  if (!populate()) {
    assert(false && "Could not find requested Python function");
  }

  PyObject *Res = PyObject_CallObject(Obj_, CreateTuple(Items));
  if (PyErr_Occurred()) {
    PyErr_Print();
    assert(false && "Python returned an exception");
  }

  return Res;
}

llvmpy::topy<int>::topy(int Int) : TopyBase(PyInt_FromLong(Int)) {
}

llvmpy::topy<const char*>::topy(const char *Str)
    : TopyBase(PyString_FromString(Str)) {
}

llvmpy::topy<std::string>::topy(std::string Str)
    : llvmpy::topy<const char*>(Str.c_str()) {
}

llvmpy::topy<APInt>::topy(APInt Int)
    : TopyBase(PyInt_FromLong(Int.getSExtValue())) {
}

