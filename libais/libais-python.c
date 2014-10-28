//
//  libais-python.c
//  libais
//
//  Created by Chris Bünger on 27/10/14.
//  Copyright (c) 2014 Chris Bünger. All rights reserved.
//

#include <Python.h>

#include "structmember.h"
#include "libais-python.h"
#include "libais.h"
//#include "gps.h"

static struct gps_device_t session;

static int
Decoder_init(Decoder *self, PyObject *args)
{
//    self->dict = PyDict_New();
    return 0;
}

static void
Decoder_dealloc(Decoder *self)
{
    Py_XDECREF(self->dict);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Decoder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Decoder *self;
    
    self = (Decoder *)type->tp_alloc(type, 0);
    
    if (self != NULL) {
        self->session = struct gps_device_t;
//        self->dict = PyDict_New();
//        if (self->dict == NULL)
//        {
//            Py_DECREF(self);
//            return NULL;
//        }
    }
    return (PyObject *)self;
}

static PyObject*
libais_decode(PyObject* self, PyObject* args)
{
    const char* msg;
    
    if (!PyArg_ParseTuple(args, "s", &msg))
        return NULL;
    
//    printf("Message: %s, length: %ul\n", msg, sizeof(msg));
    
    struct ais_t ais;
    
    char buf[JSON_VAL_MAX * 2 + 1];
    size_t buflen = sizeof(buf);
    
//    printf("Buffer: %s, length: %ul\n", buf, sizeof(buf));
    
    aivdm_decode(msg, strlen(msg)+1, &session, &ais, 0);
//    printf("type: %d, repeat: %d, mmsi: %d\n", ais.type, ais.repeat, ais.mmsi);
    json_aivdm_dump(&ais, NULL, true, buf, buflen);
//    printf("JSON: %s", buf);

    return Py_BuildValue("s", buf);
}

static PyMemberDef Decoder_members[] = {
//    {"dict", T_OBJECT, offsetof(Decoder, dict), 0, "State of the decoder."},
    {"session", T_STRUCT, offsetof(Decoder, session), 0, "State of the decoder."},
    { NULL }
};

static PyMethodDef Decoder_methods[] = {
    {NULL}
};

static PyTypeObject DecoderType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "libais.Decoder",             /*tp_name*/
    sizeof(Decoder),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Decoder_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Decoder objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Decoder_methods,             /* tp_methods */
    Decoder_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Decoder_init,      /* tp_init */
    0,                         /* tp_alloc */
    Decoder_new,                 /* tp_new */
};

static PyMethodDef libais_methods[] =
{
    {"decode", libais_decode, METH_VARARGS, "Decode AIVDM sentence."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initlibais(void)
{
    PyObject *m;
    
    if (PyType_Ready(&DecoderType) < 0)
        return;

    m = Py_InitModule("libais", libais_methods);
    
    if (m == NULL)
        return;
    
    Py_INCREF(&DecoderType);
    PyModule_AddObject(m, "Decoder", (PyObject *)&DecoderType);
}
