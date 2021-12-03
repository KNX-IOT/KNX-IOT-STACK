#include <Python.h>

static int numargs = 0;

// Action to take on left button press
// This is exposed in the corresponding Python script
// as the knx.handle_left() function
static PyObject *
knx_handle_left(PyObject *self, PyObject *args)
{
  // don't care about args, so don't check them
  (void)self;
  (void)args;
  printf("Left from C!\n");
  Py_RETURN_NONE;
}

// Action to take on left button press
// This is exposed in the corresponding Python script
// as the knx.handle_left() function
static PyObject *
knx_handle_mid(PyObject *self, PyObject *args)
{
  // don't care about args, so don't check them
  (void)self;
  (void)args;
  printf("Mid from C!\n");
  Py_RETURN_NONE;
}

// Action to take on left button press
// This is exposed in the corresponding Python script
// as the knx.handle_left() function
static PyObject *
knx_handle_right(PyObject *self, PyObject *args)
{
  // don't care about args, so don't check them
  (void)self;
  (void)args;
  printf("Right from C!\n");
  Py_RETURN_NONE;
}

// Definition of the methods within the knx module.
// Extend this array if you need to add more Python->C functions
static PyMethodDef KnxMethods[] = {
  { "handle_left", knx_handle_left, METH_NOARGS,
    "Inform KNX of left button press" },
  { "handle_mid", knx_handle_mid, METH_NOARGS,
    "Inform KNX of mid button press" },
  { "handle_right", knx_handle_right, METH_NOARGS,
    "Inform KNX of right button press" },
  { NULL, NULL, 0, NULL }
};

// Boilerplate to initialize the knx module
static PyModuleDef KnxModule = {
  PyModuleDef_HEAD_INIT, "knx", NULL, -1, KnxMethods, NULL, NULL, NULL, NULL
};

static PyObject *
PyInit_knx(void)
{
  return PyModule_Create(&KnxModule);
}

int
main(void)
{
  // Make Python aware of the knx module defined by KnxModule and KnxMethods
  PyImport_AppendInittab("knx", PyInit_knx);

  Py_Initialize();
  PyObject *pName = PyUnicode_DecodeFSDefault("simpleclient");
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("import os");
  PyRun_SimpleString("import signal");
  PyRun_SimpleString("sys.path.append(os.getcwd())");

  PyObject *pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  // initialize the PiHat - prints stuff to the LCD
  PyRun_SimpleString("import simpleclient");
  PyRun_SimpleString("simpleclient.init()");

  while (1) {
    // Wait for signals - this is how the button presses are detected
    // 0.1 is the time to wait for (in seconds), before handing execution
    // back to C.
    if (PyRun_SimpleString("signal.sigtimedwait([], 0.1)") != 0) {
      PyErr_Print();
      return -1;
    }
  }
  return 0;
}