#include <Python.h>
#include <random>
#include <ctime>
#include <cstdio>
#include "server/vector_type.hpp"
#include "sdk/aquery.h"
#include "unistd.h"
__AQEXPORT__(bool)
draw(vector_type<int> x, vector_type<int> y) {
    puts("sibal!");
    auto pid = fork();
    int status = 0;
    if (pid == 0) {
        //PyOS_AfterFork();
        Py_InitializeEx
        Py_Initialize();

        PyRun_SimpleString("print(globals())");
        PyRun_SimpleString("import os");
        PyRun_SimpleString("sys.path.append(os.getcwd()+'/demo/Python Integration')");
        //PyErr_Print();
        PyRun_SimpleString("print('fuck')");
        auto py_strapp = PyUnicode_DecodeFSDefault("app");
        auto py_module = PyImport_Import(py_strapp);
        // Py_DECREF(py_strapp);
        auto py_entrypt = PyObject_GetAttrString(py_module, "draw");

        auto mvx = PyMemoryView_FromMemory((char*)x.container, x.size * sizeof(int), PyBUF_WRITE), 
            mvy = PyMemoryView_FromMemory((char*)y.container, y.size * sizeof(int), PyBUF_WRITE);
        PyObject_CallObject(py_entrypt, PyTuple_Pack(2, mvx, mvy));
        // Py_DECREF(mvx);
        // Py_DECREF(mvy);
        // Py_DECREF(py_entrypt);
        // Py_DECREF(py_module);
        return 0;
    }
    else {
        while(wait(&status) > 0);
        //getchar();
    }
    return true;
    //return Py_FinalizeEx() >= 0;
}

int main(){
    draw({1,2,3}, {4,5,6});
}