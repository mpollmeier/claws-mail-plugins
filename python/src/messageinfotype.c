/* Python plugin for Claws-Mail
 * Copyright (C) 2009 Holger Berndt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "pluginconfig.h"

#include "messageinfotype.h"
#include <structmember.h>


typedef struct {
    PyObject_HEAD
    PyObject *from;
    PyObject *to;
    PyObject *subject;
    MsgInfo *msginfo;
} clawsmail_MessageInfoObject;


static void MessageInfo_dealloc(clawsmail_MessageInfoObject* self)
{
  Py_XDECREF(self->from);
  Py_XDECREF(self->to);
  Py_XDECREF(self->subject);
  self->ob_type->tp_free((PyObject*)self);
}

static int MessageInfo_init(clawsmail_MessageInfoObject *self, PyObject *args, PyObject *kwds)
{
  Py_INCREF(Py_None);
  self->from = Py_None;

  Py_INCREF(Py_None);
  self->to = Py_None;

  Py_INCREF(Py_None);
  self->subject = Py_None;

  return 0;
}

static PyObject* MessageInfo_str(PyObject *self)
{
  PyObject *str;
  str = PyString_FromString("MessageInfo: ");
  PyString_ConcatAndDel(&str, PyObject_GetAttrString(self, "from"));
  PyString_ConcatAndDel(&str, PyString_FromString(" / "));
  PyString_ConcatAndDel(&str, PyObject_GetAttrString(self, "subject"));
  return str;
}

static PyMethodDef MessageInfo_methods[] = {
    {NULL}
};

static PyMemberDef MessageInfo_members[] = {
    {"from", T_OBJECT_EX, offsetof(clawsmail_MessageInfoObject, from), 0, "from header"},
    {"to", T_OBJECT_EX, offsetof(clawsmail_MessageInfoObject, to), 0, "to header"},
    {"subject", T_OBJECT_EX, offsetof(clawsmail_MessageInfoObject, subject), 0, "subject header"},
    {NULL}
};

static PyTypeObject clawsmail_MessageInfoType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "clawsmail.MessageInfo",   /* tp_name*/
    sizeof(clawsmail_MessageInfoObject), /* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)MessageInfo_dealloc, /* tp_dealloc*/
    0,                         /* tp_print*/
    0,                         /* tp_getattr*/
    0,                         /* tp_setattr*/
    0,                         /* tp_compare*/
    0,                         /* tp_repr*/
    0,                         /* tp_as_number*/
    0,                         /* tp_as_sequence*/
    0,                         /* tp_as_mapping*/
    0,                         /* tp_hash */
    0,                         /* tp_call*/
    MessageInfo_str,           /* tp_str*/
    0,                         /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /* tp_flags*/
    "MessageInfo objects.",    /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    MessageInfo_methods,       /* tp_methods */
    MessageInfo_members,       /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MessageInfo_init,/* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

PyMODINIT_FUNC initmessageinfo(PyObject *module)
{
    clawsmail_MessageInfoType.tp_new = PyType_GenericNew;
    if(PyType_Ready(&clawsmail_MessageInfoType) < 0)
        return;

    Py_INCREF(&clawsmail_MessageInfoType);
    PyModule_AddObject(module, "MessageInfo", (PyObject*)&clawsmail_MessageInfoType);
}

#define MSGINFO_STRING_TO_PYTHON_MESSAGEINFO_MEMBER(fis, pms)     \
  do {                                                            \
    if(fis) {                                                     \
      PyObject *str;                                              \
      str = PyString_FromString(fis);                             \
      if(str) {                                                   \
        int retval;                                               \
        retval = PyObject_SetAttrString((PyObject*)ff, pms, str); \
        Py_DECREF(str);                                           \
        if(retval == -1)                                          \
          goto err;                                               \
      }                                                           \
    }                                                             \
  } while(0)

PyObject* clawsmail_msginfo_new(MsgInfo *msginfo)
{
  clawsmail_MessageInfoObject *ff;

  if(!msginfo)
    return NULL;

  ff = (clawsmail_MessageInfoObject*) PyObject_CallObject((PyObject*) &clawsmail_MessageInfoType, NULL);
  if(!ff)
    return NULL;

  MSGINFO_STRING_TO_PYTHON_MESSAGEINFO_MEMBER(msginfo->from, "from");
  MSGINFO_STRING_TO_PYTHON_MESSAGEINFO_MEMBER(msginfo->to, "to");
  MSGINFO_STRING_TO_PYTHON_MESSAGEINFO_MEMBER(msginfo->subject, "subject");

  ff->msginfo = msginfo;
  return (PyObject*)ff;

  err:
    Py_XDECREF(ff);
    return NULL;
}
