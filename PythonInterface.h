#ifndef _PYTHONINTERFACE_H_
#define _PYTHONINTERFACE_H_

#include <python2.7/Python.h>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

using namespace llvm;

// A ModulePass allows global initialization and finalization, which we need
// for calling Py_Init and Py_Finalize.
class PythonInterface : public ModulePass {
public:
  // Holds each object we need to get a reference to.
  struct PythonObjInfo {
    PythonObjInfo(const char *Mod, const char *Fn)
      : Mod(Mod), Class(nullptr), Fn(Fn) { }
    PythonObjInfo(const char *Mod, const char *Class, const char *Fn)
      : Mod(Mod), Class(Class), Fn(Fn) { }

    const char *Mod, *Class, *Fn;
  };

  // Holds the looked-up objects.
  struct PythonObjVec : public std::vector<PyObject*> {
    PythonObjVec(std::vector<PyObject*> Objs) : std::vector<PyObject*>(Objs) { }

    PyObject *getObj(unsigned Idx) const {
      return (*this).at(Idx);
    }
  };

  static char ID;
  PythonInterface() : ModulePass(ID) { }

  static std::string toString(PyObject *String);
  static std::vector<PyObject*> toVector(PyObject *List);

  virtual bool runOnModule(Module&) {
    return false;
  }

  virtual bool doInitialization(Module&);
  virtual bool doFinalization(Module&);

  PyObject *getModule (const char *Mod);
  PyObject *getClass  (const char *Mod, const char *Class);
  PyObject *getAttr   (const char *Mod, const char *Class, const char *Attr);
  PyObject *getAttr   (const char *Mod,                    const char *Attr);
  PyObject *getBuiltin(const char *Attr);

  PyObject *createTuple(std::initializer_list<PyObject*> Items);

  PythonObjVec *createObjVec(std::initializer_list<PythonObjInfo> Infos);
  PythonObjVec *createObjVec(std::vector<PythonObjInfo> Infos);

  PyObject *call(PyObject *Fn, PyObject *Tuple);
  PyObject *call(PyObject *Fn, std::initializer_list<PyObject*> Items);
  PyObject *call(PythonObjVec *Vec, unsigned Idx, PyObject *Tuple);
  PyObject *call(PythonObjVec *Vec, unsigned Idx,
                 std::initializer_list<PyObject*> Items);

  PyObject *callSelf(const char *Fn, PyObject *Self, PyObject *Tuple);
  PyObject *callSelf(PyObject *FnStr, PyObject *Self, PyObject *Tuple);
  PyObject *callSelf(const char *Fn, PyObject *Self,
                     std::initializer_list<PyObject*> Items);

private:
  std::map<const char*, PyObject*> Modules_;
  std::map<std::pair<const char*, const char*>, PyObject*> Classes_;
};

raw_ostream& operator<<(raw_ostream& OS, PyObject &Obj);

#endif

