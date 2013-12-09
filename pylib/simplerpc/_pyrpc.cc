#include <Python.h>

#include <string>

#include "rpc/server.h"
#include "rpc/client.h"

using namespace rpc;

class GILHelper {
    PyGILState_STATE gil_state;
public:
    GILHelper() {
        gil_state = PyGILState_Ensure();
    }

    ~GILHelper() {
        PyGILState_Release(gil_state);
    }
};

static PyObject* _pyrpc_init_server(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
//    Log::info("init_server called");
    Server* svr = new Server;
    return Py_BuildValue("k", svr);
}

static PyObject* _pyrpc_fini_server(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("fini_server called");
    unsigned long u;
    if (!PyArg_ParseTuple(args, "k", &u))
        return NULL;
    Server* svr = (Server *) u;
    delete svr;
    Py_RETURN_NONE;
}

static PyObject* _pyrpc_server_start(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("server_start called");
    const char* addr;
    unsigned long u;
    if (!PyArg_ParseTuple(args, "ks", &u, &addr))
        return NULL;
    Server* svr = (Server *) u;
    int ret = svr->start(addr);
    return Py_BuildValue("i", ret);
}

static PyObject* _pyrpc_server_unreg(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("server_unreg called");
    unsigned long u;
    int rpc_id;
    if (!PyArg_ParseTuple(args, "ki", &u, &rpc_id))
        return NULL;
    Server* svr = (Server *) u;
    svr->unreg(rpc_id);
    Py_RETURN_NONE;
}

static PyObject* _pyrpc_server_reg(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("server_reg called");
    unsigned long u;
    int rpc_id;
    PyObject* func;
    if (!PyArg_ParseTuple(args, "kiO", &u, &rpc_id, &func))
        return NULL;
    Server* svr = (Server *) u;

    // incr ref_count on PyObject func
    // TODO remember this, and decr ref_count on PyObject func when shutting down server
    Py_XINCREF(func);

    int ret = svr->reg(rpc_id, [func](Request* req, ServerConnection* sconn) {
        // Log::info("rpc handler called");
        std::string enc_req;
        req->m >> enc_req;

        std::string enc_reply;
        {
//            Log::info("before acquiring the lock");
            GILHelper gil_helper;
//            Log::info("after acquiring the lock");
            PyObject* params = Py_BuildValue("(s)", &enc_req[0]);
            PyObject* result = PyObject_CallObject(func, params);
            enc_reply = std::string(PyString_AsString(result), PyString_Size(result));
            Py_XDECREF(params);
            Py_XDECREF(result);
        }

//        Log::debug("result: %s", enc_reply.c_str());

        sconn->begin_reply(req);
        *sconn << enc_reply;
        sconn->end_reply();

        // cleanup as required by simple-rpc
        delete req;
        sconn->release();
        // Log::info("rpc handle returned");
    });

    return Py_BuildValue("i", ret);
}

static PyObject* _pyrpc_init_poll_mgr(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("init_poll_mgr called");
    PollMgr* poll = new PollMgr;
    return Py_BuildValue("k", poll);
}

static PyObject* _pyrpc_init_client(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("init_client called");
    unsigned long u;
    if (!PyArg_ParseTuple(args, "k", &u))
        return NULL;
    PollMgr* poll = (PollMgr *) u;
    Client* clnt = new Client(poll);
    return Py_BuildValue("k", clnt);
}

static PyObject* _pyrpc_fini_client(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("fini_client called");
    unsigned long u;
    if (!PyArg_ParseTuple(args, "k", &u))
        return NULL;
    Client* clnt = (Client *) u;
    clnt->close_and_release();
    Py_RETURN_NONE;
}

static PyObject* _pyrpc_client_connect(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("client_connect called");
    const char* addr;
    unsigned long u;
    if (!PyArg_ParseTuple(args, "ks", &u, &addr))
        return NULL;
    Client* clnt = (Client *) u;
    int ret = clnt->connect(addr);
    return Py_BuildValue("i", ret);
}

static PyObject* _pyrpc_client_sync_call(PyObject* self, PyObject* args) {
    GILHelper gil_helper;
    // Log::info("client_sync_call called");

    PyThreadState *_save;
    _save = PyEval_SaveThread();

    unsigned long u;
    int rpc_id;
    const char* enc_args;
    if (!PyArg_ParseTuple(args, "kis", &u, &rpc_id, &enc_args))
        return NULL;

    Client* clnt = (Client *) u;

    Future* fu = clnt->begin_request(rpc_id);
    if (fu != NULL) {
        *clnt << std::string(enc_args);
    }
    clnt->end_request();

    std::string enc_result;
    int error_code;
    if (fu == NULL) {
        error_code = ENOTCONN;
    } else {
        error_code = fu->get_error_code();
        if (error_code == 0) {
            fu->get_reply() >> enc_result;
        }
        fu->release();
    }

    PyEval_RestoreThread(_save);

    return Py_BuildValue("(is)", error_code, enc_result.c_str());
}

static PyMethodDef _pyrpcMethods[] = {
    {"init_server", _pyrpc_init_server, METH_VARARGS, NULL},
    {"fini_server", _pyrpc_fini_server, METH_VARARGS, NULL},
    {"server_start", _pyrpc_server_start, METH_VARARGS, NULL},
    {"server_unreg", _pyrpc_server_unreg, METH_VARARGS, NULL},
    {"server_reg", _pyrpc_server_reg, METH_VARARGS, NULL},

    {"init_poll_mgr", _pyrpc_init_poll_mgr, METH_VARARGS, NULL},

    {"init_client", _pyrpc_init_client, METH_VARARGS, NULL},
    {"fini_client", _pyrpc_fini_client, METH_VARARGS, NULL},
    {"client_connect", _pyrpc_client_connect, METH_VARARGS, NULL},
    {"client_sync_call", _pyrpc_client_sync_call, METH_VARARGS, NULL},

    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pyrpc(void) {
    PyEval_InitThreads();
    // Log::debug("PyEval_InitThreads called!");
    GILHelper gil_helper;
    PyObject* m;
    m = Py_InitModule("_pyrpc", _pyrpcMethods);
    if (m == NULL)
        return;
}
