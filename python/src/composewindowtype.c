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

#include "composewindowtype.h"

#include "clawsmailmodule.h"

#include "mainwindow.h"
#include "account.h"

#include <structmember.h>

typedef struct {
    PyObject_HEAD
    PyObject *ui_manager;
    PyObject *text;
    Compose *compose;
} clawsmail_ComposeWindowObject;

static void ComposeWindow_dealloc(clawsmail_ComposeWindowObject* self)
{
  Py_XDECREF(self->ui_manager);
  Py_XDECREF(self->text);
  self->ob_type->tp_free((PyObject*)self);
}

#define WRAP_GOBJECT_IN_MEMBER_VAR(ptr, var) \
  do {                                       \
    var = get_gobj_from_address(ptr);        \
    if(var) {                                \
      tmp = self->var;                       \
      Py_INCREF(var);                        \
      self->var = var;                       \
      Py_XDECREF(tmp);                       \
      tmp = NULL;                            \
    }                                        \
  } while(0)


static void composewindow_set_compose(clawsmail_ComposeWindowObject *self, Compose *compose)
{
  PyObject *ui_manager = NULL;
  PyObject *tmp = NULL;
  PyObject *text = NULL;

  self->compose = compose;
  WRAP_GOBJECT_IN_MEMBER_VAR(compose->ui_manager, ui_manager);
  WRAP_GOBJECT_IN_MEMBER_VAR(compose->text, text);
}

static int ComposeWindow_init(clawsmail_ComposeWindowObject *self, PyObject *args, PyObject *kwds)
{
  MainWindow *mainwin;
  PrefsAccount *ac = NULL;
  FolderItem *item;
  GList* list;
  GList* cur;
  gboolean did_find_compose;
  Compose *compose = NULL;
  const char *ss;
  unsigned char open_window;
  /* if __open_window is set to 0/False,
   * composewindow_set_compose must be called before this object is valid */
  static char *kwlist[] = {"address", "__open_window", NULL};

  ss = NULL;
  open_window = 1;
  PyArg_ParseTupleAndKeywords(args, kwds, "|sb", kwlist, &ss, &open_window);

  if(open_window) {
    mainwin = mainwindow_get_mainwindow();
    item = mainwin->summaryview->folder_item;
    did_find_compose = FALSE;

    if(ss) {
      ac = account_find_from_address(ss, FALSE);
      if (ac && ac->protocol != A_NNTP) {
        compose = compose_new_with_folderitem(ac, item, NULL);
        did_find_compose = TRUE;
      }
    }
    if(!did_find_compose) {
      if (item) {
        ac = account_find_from_item(item);
        if (ac && ac->protocol != A_NNTP) {
          compose = compose_new_with_folderitem(ac, item, NULL);
          did_find_compose = TRUE;
        }
      }

      /* use current account */
      if (!did_find_compose && cur_account && (cur_account->protocol != A_NNTP)) {
        compose = compose_new_with_folderitem(cur_account, item, NULL);
        did_find_compose = TRUE;
      }

      if(!did_find_compose) {
        /* just get the first one */
        list = account_get_list();
        for (cur = list ; cur != NULL ; cur = g_list_next(cur)) {
          ac = (PrefsAccount *) cur->data;
          if (ac->protocol != A_NNTP) {
            compose = compose_new_with_folderitem(ac, item, NULL);
            did_find_compose = TRUE;
          }
        }
      }
    }

    if(!did_find_compose)
      return -1;

    composewindow_set_compose(self, compose);
  }
  return 0;
}

/* this is here because wrapping GTK_EDITABLEs in PyGTK is buggy */
static PyObject* get_python_object_from_gtk_entry(GtkWidget *entry)
{
  return Py_BuildValue("s", gtk_entry_get_text(GTK_ENTRY(entry)));
}

static PyObject* set_gtk_entry_from_python_object(GtkWidget *entry, PyObject *args)
{
  const char *ss;

  if(!PyArg_ParseTuple(args, "s", &ss))
    return NULL;

  gtk_entry_set_text(GTK_ENTRY(entry), ss);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* ComposeWindow_get_subject(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  return get_python_object_from_gtk_entry(self->compose->subject_entry);
}

static PyObject* ComposeWindow_set_subject(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  return set_gtk_entry_from_python_object(self->compose->subject_entry, args);
}

static PyObject* ComposeWindow_get_from(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  return get_python_object_from_gtk_entry(self->compose->from_name);
}

static PyObject* ComposeWindow_set_from(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  return set_gtk_entry_from_python_object(self->compose->from_name, args);
}

static PyObject* ComposeWindow_add_To(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  const char *ss;

  if(!PyArg_ParseTuple(args, "s", &ss))
    return NULL;

  compose_entry_append(self->compose, ss, COMPOSE_TO, PREF_NONE);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* ComposeWindow_add_Cc(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  const char *ss;

  if(!PyArg_ParseTuple(args, "s", &ss))
    return NULL;

  compose_entry_append(self->compose, ss, COMPOSE_CC, PREF_NONE);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* ComposeWindow_add_Bcc(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  const char *ss;

  if(!PyArg_ParseTuple(args, "s", &ss))
    return NULL;

  compose_entry_append(self->compose, ss, COMPOSE_BCC, PREF_NONE);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* ComposeWindow_attach(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  PyObject *olist;
  Py_ssize_t size, iEl;
  GList *list = NULL;

  if(!PyArg_ParseTuple(args, "O!", &PyList_Type, &olist))
    return NULL;

  size = PyList_Size(olist);
  for(iEl = 0; iEl < size; iEl++) {
    char *ss;
    PyObject *element = PyList_GET_ITEM(olist, iEl);

    if(!element)
      continue;

    Py_INCREF(element);
    if(!PyArg_Parse(element, "s", &ss)) {
      Py_DECREF(element);
      if(list)
        g_list_free(list);
      return NULL;
    }
    list = g_list_prepend(list, ss);
    Py_DECREF(element);
  }

  compose_attach_from_list(self->compose, list, FALSE);
  g_list_free(list);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* ComposeWindow_get_header_list(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  GSList *walk;
  PyObject *retval;

  retval = Py_BuildValue("[]");
  for(walk = self->compose->header_list; walk; walk = walk->next) {
    ComposeHeaderEntry *headerentry = walk->data;
    const gchar *header;
    const gchar *text;

    header = gtk_editable_get_chars(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(headerentry->combo))), 0, -1);
    text = gtk_entry_get_text(GTK_ENTRY(headerentry->entry));

    if(text && strcmp("", text)) {
      PyObject *ee;
      int ok;

      ee = Py_BuildValue("(ss)", header, text);
      ok = PyList_Append(retval, ee);
      Py_DECREF(ee);
      if(ok == -1) {
        Py_DECREF(retval);
        return NULL;
      }
    }
  }
  return retval;
}

static PyObject* ComposeWindow_add_header(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  const char *header;
  const char *text;
  gint num;

  if(!PyArg_ParseTuple(args, "ss", &header, &text))
    return NULL;

  /* add a dummy, and modify it then */
  compose_entry_append(self->compose, "dummy1dummy2dummy3", COMPOSE_TO, PREF_NONE);
  num = g_slist_length(self->compose->header_list);
  if(num > 1) {
    ComposeHeaderEntry *headerentry;
    headerentry = g_slist_nth_data(self->compose->header_list, num-2);
    if(headerentry) {
      GtkEditable *editable;
      gint pos;
      gtk_entry_set_text(GTK_ENTRY(headerentry->entry), text);
      editable = GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(headerentry->combo)));
      gtk_editable_delete_text(editable, 0, -1);
      gtk_editable_insert_text(editable, header, -1, &pos);
    }
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/* this is pretty ugly, as the compose struct does not maintain a pointer to the account selection combo */
static PyObject* ComposeWindow_get_account_selection(clawsmail_ComposeWindowObject *self, PyObject *args)
{
  GList *children, *walk;

  children = gtk_container_get_children(GTK_CONTAINER(self->compose->header_table));
  for(walk = children; walk; walk = walk->next) {
    if(GTK_IS_HBOX(walk->data)) {
      GList *children2, *walk2;
      children2 = gtk_container_get_children(GTK_CONTAINER(walk->data));
      for(walk2 = children2; walk2; walk2 = walk2->next) {
        if(GTK_IS_EVENT_BOX(walk2->data)) {
          return get_gobj_from_address(gtk_container_get_children(GTK_CONTAINER(walk2->data))->data);
        }
      }
    }
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef ComposeWindow_methods[] = {
    {"set_subject", (PyCFunction)ComposeWindow_set_subject, METH_VARARGS, "Set subject to text."},
    {"get_subject", (PyCFunction)ComposeWindow_get_subject, METH_NOARGS, "Get subject"},
    {"set_from", (PyCFunction)ComposeWindow_set_from, METH_VARARGS, "Set from header entry to text."},
    {"get_from", (PyCFunction)ComposeWindow_get_from, METH_NOARGS, "Get from header entry"},
    {"add_To",  (PyCFunction)ComposeWindow_add_To,  METH_VARARGS, "Append another To header."},
    {"add_Cc",  (PyCFunction)ComposeWindow_add_Cc,  METH_VARARGS, "Append another Cc header."},
    {"add_Bcc", (PyCFunction)ComposeWindow_add_Bcc, METH_VARARGS, "Append another Bcc header."},
    {"attach",  (PyCFunction)ComposeWindow_attach, METH_VARARGS, "Attach a list of files."},
    {"get_header_list", (PyCFunction)ComposeWindow_get_header_list, METH_NOARGS, "Get list of headers."},
    {"add_header", (PyCFunction)ComposeWindow_add_header, METH_VARARGS, "Add a custom header."},
    {"get_account_selection", (PyCFunction)ComposeWindow_get_account_selection, METH_NOARGS, "Get account selection widget."},
    {NULL}
};

static PyMemberDef ComposeWindow_members[] = {
    {"ui_manager", T_OBJECT_EX, offsetof(clawsmail_ComposeWindowObject, ui_manager), 0, "gtk ui manager"},
    {"text", T_OBJECT_EX, offsetof(clawsmail_ComposeWindowObject, text), 0, "text view widget"},
    {NULL}
};

static PyTypeObject clawsmail_ComposeWindowType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "clawsmail.ComposeWindow", /*tp_name*/
    sizeof(clawsmail_ComposeWindowObject), /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ComposeWindow_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    /* tp_doc */
    "ComposeWindow objects. Optional argument to constructor: sender account address. ",
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    ComposeWindow_methods,     /* tp_methods */
    ComposeWindow_members,     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)ComposeWindow_init, /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

PyMODINIT_FUNC initcomposewindow(PyObject *module)
{
    clawsmail_ComposeWindowType.tp_new = PyType_GenericNew;
    if(PyType_Ready(&clawsmail_ComposeWindowType) < 0)
        return;

    Py_INCREF(&clawsmail_ComposeWindowType);
    PyModule_AddObject(module, "ComposeWindow", (PyObject*)&clawsmail_ComposeWindowType);
}

PyObject* clawsmail_compose_new(PyObject *module, Compose *compose)
{
  PyObject *class, *dict;
  PyObject *self, *args, *kw;

  if(!compose) {
    Py_INCREF(Py_None);
    return Py_None;
  }

  dict = PyModule_GetDict(module);
  class = PyDict_GetItemString(dict, "ComposeWindow");
  args = Py_BuildValue("()");
  kw = Py_BuildValue("{s:b}", "__open_window", 0);
  self = PyObject_Call(class, args, kw);
  Py_DECREF(args);
  Py_DECREF(kw);
  composewindow_set_compose((clawsmail_ComposeWindowObject*)self, compose);
  return self;
}
