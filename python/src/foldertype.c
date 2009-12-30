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

#include "foldertype.h"

#include <structmember.h>


typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *path;
    FolderItem *folderitem;
} clawsmail_FolderObject;


static void Folder_dealloc(clawsmail_FolderObject* self)
{
  Py_XDECREF(self->name);
  Py_XDECREF(self->path);
  self->ob_type->tp_free((PyObject*)self);
}

static int Folder_init(clawsmail_FolderObject *self, PyObject *args, PyObject *kwds)
{
  Py_INCREF(Py_None);
  self->name = Py_None;

  Py_INCREF(Py_None);
  self->path = Py_None;

  return 0;
}

static PyObject* Folder_str(PyObject *self)
{
  PyObject *str;
  str = PyString_FromString("Folder: ");
  PyString_ConcatAndDel(&str, PyObject_GetAttrString(self, "name"));
  return str;
}

static PyMethodDef Folder_methods[] = {
    {NULL}
};

static PyMemberDef Folder_members[] = {
    {"name", T_OBJECT_EX, offsetof(clawsmail_FolderObject, name), 0, "name of folder"},
    {"path", T_OBJECT_EX, offsetof(clawsmail_FolderObject, path), 0, "path of folder"},
    {NULL}
};

static PyTypeObject clawsmail_FolderType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "clawsmail.Folder",        /* tp_name*/
    sizeof(clawsmail_FolderObject), /* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)Folder_dealloc, /* tp_dealloc*/
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
    Folder_str,                /* tp_str*/
    0,                         /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /* tp_flags*/
    "Folder objects.",         /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Folder_methods,            /* tp_methods */
    Folder_members,            /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Folder_init,     /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

PyMODINIT_FUNC initfolder(PyObject *module)
{
    clawsmail_FolderType.tp_new = PyType_GenericNew;
    if(PyType_Ready(&clawsmail_FolderType) < 0)
        return;

    Py_INCREF(&clawsmail_FolderType);
    PyModule_AddObject(module, "Folder", (PyObject*)&clawsmail_FolderType);
}

#define FOLDERITEM_STRING_TO_PYTHON_FOLDER_MEMBER(fis, pms)       \
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

PyObject* clawsmail_folder_new(FolderItem *folderitem)
{
  clawsmail_FolderObject *ff;

  if(!folderitem)
    return NULL;

  ff = (clawsmail_FolderObject*) PyObject_CallObject((PyObject*) &clawsmail_FolderType, NULL);
  if(!ff)
    return NULL;

  FOLDERITEM_STRING_TO_PYTHON_FOLDER_MEMBER(folderitem->name, "name");
  FOLDERITEM_STRING_TO_PYTHON_FOLDER_MEMBER(folderitem->path, "path");

  ff->folderitem = folderitem;
  return (PyObject*)ff;

err:
  Py_XDECREF(ff);
  return NULL;
}

FolderItem* clawsmail_folder_get_item(PyObject *self)
{
  return ((clawsmail_FolderObject*)self)->folderitem;
}

PyTypeObject* clawsmail_folder_get_type_object()
{
  return &clawsmail_FolderType;
}

gboolean clawsmail_folder_check(PyObject *self)
{
  return (PyObject_TypeCheck(self, &clawsmail_FolderType) != 0);
}
