#include <Python.h>

#include "rpc/utils.h"
#include "rpc/server.h"

using namespace rpc;

static PyObject* _pyrpc_init_server(PyObject* self, PyObject* args) {
    Log::info("init_server called");
    Server* svr = new Server;
    return Py_BuildValue("k", svr);
}

static PyObject* _pyrpc_fini_server(PyObject* self, PyObject* args) {
    Log::info("fini_server called");
    unsigned long u;
    if (!PyArg_ParseTuple(args, "k", &u))
        return NULL;
    Server* svr = (Server *) u;
    delete svr;
    Py_RETURN_NONE;
}

static PyMethodDef _pyrpcMethods[] = {
    {"init_server", _pyrpc_init_server, METH_VARARGS, NULL},
    {"fini_server", _pyrpc_fini_server, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pyrpc(void) {
    PyObject* m;
    m = Py_InitModule("_pyrpc", _pyrpcMethods);
    if (m == NULL)
        return;
}
