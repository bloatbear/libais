//
//  libais-python.c
//  libais
//
//  Created by Chris Bünger on 27/10/14.
//  Copyright (c) 2014 Chris Bünger. All rights reserved.
//

#include <Python.h>

#include "libais-python.h"
#include "libais.h"
//#include "gps.h"

static struct gps_device_t session;

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

static PyMethodDef libais_methods[] =
{
    {"decode", libais_decode, METH_VARARGS, "Decode AIVDM sentence."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initlibais(void)
{
    (void) Py_InitModule("libais", libais_methods);
}