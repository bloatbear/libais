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

#define MAXDEVICES	1024

static struct gps_device_t session[MAXDEVICES];

static bool assigned[MAXDEVICES];

static PyObject*
libais_decode(PyObject* self, PyObject* args, PyObject *kwargs)
{
    const char* msg;
    int decoderId=-1;
    
    static char *kwlist[] = {"message","decoderId", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i", kwlist, &msg, &decoderId))
        return NULL;

    if ( decoderId == -1 ) {
        printf("Warning: No decoder selected. Using default decoder '0'.\n");
        decoderId=0;
        if (!assigned[0]) {
            assigned[0]=true;
        } else {
            printf("Error: Default decoder already assigned. Get your own!.\n");
            return NULL;
        }
    }
    
    if (!assigned[decoderId]) {
        printf("Warning: Selected decoder '%d' was not assigned. Now it is.\n", decoderId);
        assigned[decoderId]=true;
    }
    
//    printf("Message: %s, length: %ul\n", msg, sizeof(msg));
    
    struct ais_t ais;
    
    char buf[JSON_VAL_MAX * 2 + 1];
    size_t buflen = sizeof(buf);
    
//    printf("Buffer: %s, length: %ul\n", buf, sizeof(buf));
    
    if (aivdm_decode(msg, strlen(msg)+1, &(session[decoderId]), &ais, 1)) {
//      printf("type: %d, repeat: %d, mmsi: %d\n", ais.type, ais.repeat, ais.mmsi);
        json_aivdm_dump(&ais, NULL, true, buf, buflen);
//      printf("JSON: %s", buf);
        return Py_BuildValue("s", buf);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
libais_getDecoderId(PyObject* self)
{
    for (int i=0; i<MAXDEVICES; i++) {
        if (assigned[i]==false) {
            assigned[i]=true;
            return Py_BuildValue("i", i);
        }
    }
    // No unassigned decoders available
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
libais_releaseDecoderId(PyObject* self, PyObject* args)
{
    int decoderId;
    
    if (!PyArg_ParseTuple(args, "i", &decoderId))
        return NULL;
    
    if (assigned[decoderId]==true) {
        assigned[decoderId]=false;
        return Py_BuildValue("i", 0);
    }
    return NULL;
}

static PyMethodDef libais_methods[] =
{
    {"decode", (PyCFunction)libais_decode, METH_VARARGS|METH_KEYWORDS, "Decode AIVDM sentence."},
    {"getDecoderId" , (PyCFunction)libais_getDecoderId, METH_NOARGS, "Get a decoder id. Returns 'None' if no decoders are available."},
    {"releaseDecoderId", (PyCFunction)libais_releaseDecoderId, METH_VARARGS, "Give decoderId back."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initlibais(void)
{
    // Initialize deocder assignment array
    for (int i=0; i<MAXDEVICES; i++) {
        assigned[i]=false;
    }

    (void) Py_InitModule("libais", libais_methods);
}
