//===------------------------- PythonInterface.cpp ------------------------===//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "python-interface"

#include "PythonInterface.h"

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

std::string PythonInterface::toString(PyObject *String) {
  return PyString_AsString(String);
}

std::vector<PyObject*> PythonInterface::toVector(PyObject *List) {
  assert(PyList_Check(List) && "PyObject is not a list");

  std::vector<PyObject*> Vec;
  Py_ssize_t Size = PyList_Size(List);
  for (Py_ssize_t Idx = 0; Idx < Size; ++Idx) {
    Vec.push_back(PyList_GetItem(List, Idx));
  }
  return Vec;
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

PyObject *PythonInterface::getModule(const char *Mod) {
  auto It = Modules_.find(Mod);
  if (It != Modules_.end()) {
    return It->second;
  }

  PyObject *ModStr = PyString_FromString(Mod);
  PyObject *ModObj = PyImport_Import(ModStr);
  Modules_[Mod] = ModObj;
  return ModObj;
}

PyObject *PythonInterface::getClass(const char *Mod, const char *Class) {
  auto It = Classes_.find(std::make_pair(Mod, Class));
  if (It != Classes_.end()) {
    return It->second;
  }

  PyEval_GetBuiltins();
  PyObject *DictObj = PyModule_GetDict(getModule(Mod));
  PyObject *ClassObj = PyDict_GetItemString(DictObj, Class);
  Classes_[std::make_pair(Mod, Class)] = ClassObj;
  return ClassObj;
}

PyObject *PythonInterface::getAttr(const char *Mod, const char *Class,
                                   const char *Attr) {
  PyObject *ClassObj = getClass(Mod, Class);
  return PyObject_GetAttrString(ClassObj, Attr);
}

PyObject *PythonInterface::getAttr(const char *Mod, const char *Attr) {
  return PyObject_GetAttrString(getModule(Mod), Attr);
}

PyObject *PythonInterface::getBuiltin(const char *Attr) {
  PyObject *Builtins = PyEval_GetBuiltins();
  return PyDict_GetItemString(Builtins, Attr);
}

PyObject
  *PythonInterface::createTuple(std::initializer_list<PyObject*> Items) {
  PyObject *Tuple = PyTuple_New(Items.size());
  unsigned Idx = 0;
  for (auto &Item : Items)
    PyTuple_SetItem(Tuple, Idx++, Item);
  return Tuple;
}

PythonInterface::PythonObjVec
  *PythonInterface::createObjVec(std::initializer_list<PythonObjInfo> Infos) {
  return createObjVec(std::vector<PythonObjInfo>(Infos));
}

PythonInterface::PythonObjVec
  *PythonInterface::createObjVec(std::vector<PythonObjInfo> Infos) {
  std::vector<PyObject*> Functions;

  for (auto &Info : Infos) {
    PyObject *Fn;
    if (!strcmp(Info.Mod, "__builtins__")) {
      Fn = getBuiltin(Info.Fn);
    } else if (Info.Class) {
      Fn = Info.Fn ? getAttr(Info.Mod, Info.Class, Info.Fn)
                   : getClass(Info.Mod, Info.Class);
    } else {
      Fn = getAttr(Info.Mod, Info.Fn);
    }

    if (!Fn) {
      if (Info.Class) {
        if (Info.Fn)
          errs() << "PythonInterface: could not get member function "
                 << Info.Mod << "." << Info.Class << "." << Info.Fn << "\n";
        else
          errs() << "PythonInterface: could not get class " << Info.Mod
                 << "." << Info.Class << "\n";
      } else {
        errs() << "PythonInterface: could not get module function "
               << Info.Mod << "." << Info.Fn << "\n";
      }
      return nullptr;
    }

    Functions.push_back(Fn);
  }

  return new PythonObjVec(Functions);
}

PyObject *PythonInterface::call(PyObject *Fn, PyObject *Tuple) {
  DEBUG(dbgs() << "PythonInterface: call: " << *Fn << *Tuple << "\n");
  return PyObject_CallObject(Fn, Tuple);
}

PyObject *PythonInterface::call(PythonObjVec *Vec, unsigned Idx,
                                PyObject *Tuple) {
  return call(Vec->getObj(Idx), Tuple);
}

PyObject *PythonInterface::call(PythonObjVec *Vec, unsigned Idx,
                                std::initializer_list<PyObject*> Items) {
  PyObject *Tuple = createTuple(Items);
  return call(Vec, Idx, Tuple);
}

PyObject *PythonInterface::call(PyObject *Fn,
                                std::initializer_list<PyObject*> Items) {
  PyObject *Tuple = createTuple(Items);
  return call(Fn, Tuple);
}

PyObject *PythonInterface::callSelf(PyObject *FnStr, PyObject *Self,
                                    PyObject *Tuple) {
  PyObject *FnObj = PyObject_GetAttr(Self, FnStr);
  return call(FnObj, Tuple);
}

PyObject *PythonInterface::callSelf(const char *Fn, PyObject *Self,
                                    PyObject *Tuple) {
  PyObject *FnStr = PyString_FromString(Fn);
  return callSelf(FnStr, Self, Tuple);
}

PyObject *PythonInterface::callSelf(const char *Fn, PyObject *Self,
                                    std::initializer_list<PyObject*> Items) {
  PyObject *Tuple = createTuple(Items);
  return callSelf(Fn, Self, Tuple);
}

