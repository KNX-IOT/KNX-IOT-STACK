#include <Python.h>

static int numargs=0;

/* Return the number of arguments of the application command line 
static PyObject*
emb_numargs(PyObject *self, PyObject *args)
{
    if(!PyArg_ParseTuple(args, ":numargs"))
        return NULL;
    return PyLong_FromLong(numargs);
}
*/

/* Action to take on left button press */
static PyObject*
knx_handle_left(PyObject *self, PyObject *args)
{
    // don't care about args, so don't check them
    (void) self;
    (void) args;
    printf("Left button press received within C!\n");
    Py_RETURN_NONE;
}

static PyMethodDef KnxMethods[] = {
    {"handle_left", knx_handle_left, METH_VARARGS,
     "Inform the KNX stack that the left button has been pressed."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef KnxModule = {
    PyModuleDef_HEAD_INIT, "knx", NULL, -1, KnxMethods,
    NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_knx(void)
{
    return PyModule_Create(&KnxModule);
}

int main(void) {
	PyImport_AppendInittab("knx", PyInit_knx);

	Py_Initialize();
    PyObject *pName = PyUnicode_DecodeFSDefault("simpleclient");
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("import os");
    PyRun_SimpleString("import signal");
    PyRun_SimpleString("sys.path.append(os.getcwd())");

    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    PyRun_SimpleString("import simpleclient");
    PyRun_SimpleString("simpleclient.init()");

    while(1)
    {
        if (PyRun_SimpleString("signal.sigtimedwait([], 0.1)") != 0)
        {
            PyErr_Print();
            return -1;
        }
    }
	return 0;
}