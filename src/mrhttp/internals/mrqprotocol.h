#pragma once

#include "Python.h"
#include <stdbool.h>

//typedef void (*tMrqCallback)(void*);
//typedef struct {
  //void *connection;
  //tMrqCallback *cb;
//} MrqRequest;

typedef struct {
  PyObject_HEAD
  PyObject* app;
  bool closed;

  PyObject* transport;
  PyObject* write;
  void *client;
  PyObject *q, *pfunc;

  int server_num;
  char *gb;
  char *b;
  char *bb;
  int  *bp4;
  int bsz, gbsz;

  char *rbuf, *rbufp;
  int rbufsz;


} MrqProtocol;

PyObject *MrqProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int       MrqProtocol_init(   MrqProtocol* self, PyObject *args, PyObject *kw);
void      MrqProtocol_dealloc(MrqProtocol* self);


PyObject* MrqProtocol_connection_made (MrqProtocol* self, PyObject* transport);
void*     MrqProtocol_close           (MrqProtocol* self);
PyObject* MrqProtocol_connection_lost (MrqProtocol* self, PyObject* args);
PyObject* MrqProtocol_data_received   (MrqProtocol* self, PyObject* data);
PyObject* MrqProtocol_eof_received    (MrqProtocol* self);

int MrqProtocol_get(MrqProtocol* self, int slot, char *d, int dsz);
int MrqProtocol_push(MrqProtocol* self, int topic, int slot, char *d, int dsz);
//typedef struct {
  //int (*MrqProtocol_asyncGet) (MrqProtocol* self, void* fn);
//} MrqProtocol_CAPI;
