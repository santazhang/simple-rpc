#include <Python.h>

#include "rpc/utils.h"

using namespace rpc;

static PyObject* _pyrpc_a_add_b(PyObject* self, PyObject* args) {
    int a;
    int b;
    int result;
    if (!PyArg_ParseTuple(args, "ii", &a, &b))
        return NULL;
    Log::info("a = %d", a);
    Log::info("b = %d", a);
    result = a + b;
    Log::info("a+b = %d", result);
    return Py_BuildValue("i", result);
}

static PyMethodDef _pyrpcMethods[] = {
    {"a_add_b", _pyrpc_a_add_b, METH_VARARGS, "do a+b math"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pyrpc(void) {
    PyObject* m;
    m = Py_InitModule("_pyrpc", _pyrpcMethods);
    if (m == NULL)
        return;
}
