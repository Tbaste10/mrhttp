#include <Python.h>
#include <stdbool.h>

#include "mrqprotocol.h"
#include "common.h"
#include "module.h"

// TODO return -1 for any push within 10 seconds of reconnecting
//      Create pool size num conns after reconnect

//static char *resp_buf;

static int server_slotmap[256];

PyObject *MrqClient_new(PyTypeObject* type, PyObject *args, PyObject *kwargs) {
  MrqClient* self = NULL;
  self = (MrqClient*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

void MrqClient_dealloc(MrqClient* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrqClient_init(MrqClient* self, PyObject *args, PyObject *kwargs) {
  if(!PyArg_ParseTuple(args, "i", &self->num_servers)) return 1;

  self->servers = malloc( sizeof(MrqServer*) * self->num_servers );
  for (int i = 0; i < self->num_servers; i++ ) {
    self->servers[i] = malloc( sizeof(MrqServer) );
    MrqServer_init( self->servers[i], self, i );
  }

  //MrqClient_setupConnMap(self);

  return 0;
}

PyObject *MrqClient_cinit(MrqClient* self) {
  Py_RETURN_NONE;
}

void MrqClient_setupConnMap( MrqClient* self ) {
  if ( self->num_servers == 0 ) return;

  int no_servers = 1;
  for (int i = 0; i < self->num_servers; i++ ) {
    if ( self->servers[i]->num_conns != 0 ) no_servers = 0; 
  }

  if ( no_servers ) return;

  int srv = 0;
  for (int i = 0; i < 256; i++ ) {
    while ( self->servers[srv]->num_conns == 0 ) srv = (srv+1)%self->num_servers;
    server_slotmap[i] = srv;
    srv = (srv+1)%self->num_servers;
  }
}


PyObject *MrqClient_addConnection(MrqClient* self, MrqProtocol *conn, int server) {

  int n = self->servers[server]->num_conns;
  MrqServer_addConnection( self->servers[server], conn );

  // This is the first connection for the server so add it
  if ( n == 0 ) {
    MrqClient_setupConnMap(self);
  }

  Py_RETURN_NONE;
}

void MrqClient_connection_lost( MrqClient* self, MrqProtocol *conn ) {
  DBG_MRQ printf("  MrqClient lost server %d\n", conn->server_num);
  MrqServer *srv = self->servers[conn->server_num];
  MrqServer_connection_lost( srv, conn );

  PyObject* func = PyObject_GetAttrString((PyObject*)self, "lost_connection");
  PyObject* snum = PyLong_FromLong(conn->server_num);
  PyObject* tmp = PyObject_CallFunctionObjArgs(func, snum, NULL);
  Py_XDECREF(func);
  Py_XDECREF(tmp);
  Py_DECREF(snum);
  

  int no_servers = 1;
  for (int i = 0; i < self->num_servers; i++ ) {
    if ( self->servers[i]->num_conns != 0 ) no_servers = 0; 
  }

  if ( no_servers ) {
    DBG_MRQ printf(" No servers with a connection\n");
    for (int i = 0; i < 256; i++ ) {
      server_slotmap[i] = -1;
    }
  } else {
    int s = srv->num;
    for (int i = 0; i < 256; i++ ) {
      if ( server_slotmap[i] == s ) server_slotmap[i] = (s+1)%self->num_servers;
    }
  }

}


PyObject *MrqClient_get(MrqClient* self, PyObject *args) {
  int slot;
  PyObject *getargs;
  if(!PyArg_ParseTuple(args, "iO", &slot, &getargs)) return NULL;
  int srv = server_slotmap[slot];
  if ( srv == -1 ) return NULL;
  MrqServer_get( self->servers[srv], slot, getargs); //TODO error check
  return PyLong_FromLong(srv);
}

// TODO We trust the slot is & 0xFF
int MrqClient_push(MrqClient* self, int topic, int slot, char *d, int dsz) {
  DBG_MRQ printf(" MrqClient_push topic %d slot %d\n", topic, slot );
  int server = server_slotmap[slot];
  DBG_MRQ printf(" MrqClient_push server %d\n", server );
  DBG_MRQ printf(" MrqClient_push >%.*s<\n", dsz, d );
  if ( server == -1 ) return -1;
  int rc = MrqServer_push( self->servers[server], topic, slot, d, dsz );
  return rc;
}

int MrqServer_dealloc( MrqServer *self ) {
  free(self->conns);
  return 0;
}

int MrqServer_init( MrqServer *self, MrqClient *client, int server_num ) {
  self->num_conns = 0;
  self->next_conn = 0;
  self->client = client;
  self->num = server_num;
  self->conns = malloc( sizeof(MrqProtocol*) * 16 );
  return 0;
}
int MrqServer_addConnection( MrqServer *self, MrqProtocol *conn) {
  DBG_MRQ printf("  MrqServer add conn %p\n", conn);
  self->conns[ self->num_conns++ ] = conn;
  return 0;
}
void MrqServer_connection_lost( MrqServer* self, MrqProtocol* conn ) {
  DBG_MRQ printf("  MrqServer conn %p lost\n", conn);
  self->num_conns--;
  self->next_conn = 0;
  if ( self->num_conns == 0 ) {
    DBG_MRQ printf("  No more connections\n");
    return;
  }

  // Remove the connection by shifting the rest left    
  MrqProtocol **p = self->conns;
  for (int i = 0; i < self->num_conns+1; i++) {
    p[i] = self->conns[i];
    if ( self->conns[i] != conn ) p++;
  }

}

int MrqServer_get(MrqServer* self, int slot, PyObject *args ) {
  if ( self->num_conns == 0 ) return -1;
  int c = self->next_conn++;
  if ( self->next_conn >= self->num_conns ) self->next_conn = 0;

  char *b;
  Py_ssize_t bsz;
  if(PyBytes_AsStringAndSize(args, &b, &bsz) == -1) return -1;

  MrqProtocol_get( self->conns[c], slot, b, bsz );
  return 0;
}

int MrqServer_push(MrqServer* self, int topic, int slot, char *d, int dsz) {
  DBG_MRQ printf("  MrqServer push num conns %d\n", self->num_conns);
  if ( self->num_conns == 0 ) return -1;
  int c = self->next_conn++;
  if ( self->next_conn >= self->num_conns ) self->next_conn = 0;

  DBG_MRQ printf("mrq server push: %.*s\n",dsz, d);
  MrqProtocol_push( self->conns[c], topic, slot, d, dsz );
  return 0;
}

/*
int MrqServer_push(MrqServer* self, int topic, int slot, char *d, int dsz) {
  DBG_MRQ printf("  MrqServer push num conns %d\n", self->num_conns);
  if ( self->num_conns == 0 ) return -1;
  int c = self->next_conn++;
  if ( self->next_conn >= self->num_conns ) self->next_conn = 0;

  DBG_MRQ printf("mrq server push: %.*s\n",dsz, d);
  MrqProtocol_push( self->conns[c], topic, slot, d, dsz );
  return 0;
}
*/


