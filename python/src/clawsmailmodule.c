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

#include "clawsmailmodule.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "pluginconfig.h"

#include "nodetype.h"
#include "composewindowtype.h"
#include "foldertype.h"

#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "mainwindow.h"
#include "toolbar.h"

#include <glib.h>

static PyObject *cm_module = NULL;

PyObject* get_gobj_from_address(gpointer addr)
{
  GObject *obj;

  if (!G_IS_OBJECT(addr))
      return NULL;

  obj = G_OBJECT(addr);

  if (!obj)
      return NULL;

  return pygobject_new(obj);
}

static PyObject* private_wrap_gobj(PyObject *self, PyObject *args)
{
    void *addr;

    if (!PyArg_ParseTuple(args, "l", &addr))
        return NULL;

    return get_gobj_from_address(addr);
}

static PyObject *get_mainwindow_action_group(PyObject *self, PyObject *args)
{
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();
  if(mainwin)
    return get_gobj_from_address(mainwin->action_group);
  else
    return NULL;
}

static PyObject *get_folderview_selected_folder(PyObject *self, PyObject *args)
{
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();
  if(mainwin && mainwin->folderview) {
    FolderItem *item;
    item = folderview_get_selected_item(mainwin->folderview);
    if(item)
      return clawsmail_folder_new(item);
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *folderview_select_folder(PyObject *self, PyObject *args)
{
  MainWindow *mainwin;

  mainwin =  mainwindow_get_mainwindow();
  if(mainwin && mainwin->folderview) {
    FolderItem *item;
    PyObject *folder;
    folder = PyTuple_GetItem(args, 0);
    if(!folder)
      return NULL;
    Py_INCREF(folder);
    item = clawsmail_folder_get_item(folder);
    Py_DECREF(folder);
    if(item)
      folderview_select(mainwin->folderview, item);
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static gboolean setup_folderitem_node(GNode *item_node, GNode *item_parent, PyObject **pyparent)
{
  PyObject *pynode, *children;
  int retval, n_children, i_child;
  PyObject *folder;

  /* create a python node for the folderitem node */
  pynode = clawsmail_node_new(cm_module);
  if(!pynode)
    return FALSE;

  /* store Folder in pynode */
  folder = clawsmail_folder_new(item_node->data);
  retval = PyObject_SetAttrString(pynode, "data", folder);
  Py_DECREF(folder);
  if(retval == -1) {
    Py_DECREF(pynode);
    return FALSE;
  }

  if(pyparent && *pyparent) {
    /* add this node to the parent's childs */
    children = PyObject_GetAttrString(*pyparent, "children");
    retval = PyList_Append(children, pynode);
    Py_DECREF(children);

    if(retval == -1) {
      Py_DECREF(pynode);
      return FALSE;
    }
  }
  else if(pyparent) {
    *pyparent = pynode;
    Py_INCREF(pynode);
  }

  /* call this function recursively for all children of the new node */
  n_children = g_node_n_children(item_node);
  for(i_child = 0; i_child < n_children; i_child++) {
    if(!setup_folderitem_node(g_node_nth_child(item_node, i_child), item_node, &pynode)) {
      Py_DECREF(pynode);
      return FALSE;
    }
  }

  Py_DECREF(pynode);
  return TRUE;
}

static PyObject* get_folder_tree_from_account_name(const char *str)
{
  PyObject *result;
  GList *walk;

  result = Py_BuildValue("[]");
  if(!result)
    return NULL;

  for(walk = folder_get_list(); walk; walk = walk->next) {
    Folder *folder = walk->data;
    if((!str || !g_strcmp0(str, folder->name)) && folder->node) {
      PyObject *root;
      int n_children, i_child, retval;

      /* create root nodes */
      root = clawsmail_node_new(cm_module);
      if(!root) {
        Py_DECREF(result);
        return NULL;
      }

      n_children = g_node_n_children(folder->node);
      for(i_child = 0; i_child < n_children; i_child++) {
        if(!setup_folderitem_node(g_node_nth_child(folder->node, i_child), folder->node, &root)) {
          Py_DECREF(root);
          Py_DECREF(result);
          return NULL;
        }
      }
      retval = PyList_Append(result, root);
      Py_DECREF(root);
      if(retval == -1) {
        Py_DECREF(result);
        return NULL;
      }
    }
  }
  return result;
}

static PyObject* get_folder_tree_from_folderitem(FolderItem *item)
{
  PyObject *result;
  GList *walk;

  for(walk = folder_get_list(); walk; walk = walk->next) {
    Folder *folder = walk->data;
    if(folder->node) {
      GNode *root_node;

      root_node = g_node_find(folder->node, G_PRE_ORDER, G_TRAVERSE_ALL, item);
      if(!root_node)
        continue;

      result = NULL;
      if(!setup_folderitem_node(root_node, NULL, &result))
        return NULL;
      else
        return result;
    }
  }

  PyErr_SetString(PyExc_LookupError, "Folder not found");
  return NULL;
}

static PyObject* get_folder_tree(PyObject *self, PyObject *args)
{
  PyObject *arg;
  PyObject *result;
  int retval;

  Py_INCREF(Py_None);
  arg = Py_None;
  retval = PyArg_ParseTuple(args, "|O", &arg);
  Py_DECREF(Py_None);
  if(!retval)
    return NULL;

  /* calling possibilities:
   * nothing, the mailbox name in a string, a Folder object */

  /* no arguments: build up a list of folder trees */
  if(PyTuple_Size(args) == 0) {
    result = get_folder_tree_from_account_name(NULL);
  }
  else if(PyString_Check(arg)){
    const char *str;
    str = PyString_AsString(arg);
    if(!str)
      return NULL;

    result = get_folder_tree_from_account_name(str);
  }
  else if(PyObject_TypeCheck(arg, clawsmail_folder_get_type_object())) {
    result = get_folder_tree_from_folderitem(clawsmail_folder_get_item(arg));
  }
  else {
    PyErr_SetString(PyExc_TypeError, "Parameter must be nothing, a mailbox string or a Folder object.");
    return NULL;
  }

  return result;
}


static PyMethodDef ClawsMailMethods[] = {
    /* public */
    {"get_mainwindow_action_group",  get_mainwindow_action_group, METH_NOARGS, "Get action group of main window menu."},
    {"get_folder_tree",  get_folder_tree, METH_VARARGS, "Get folder tree."},
    {"get_folderview_selected_folder",  get_folderview_selected_folder, METH_NOARGS, "Get selected folder in folderview."},
    {"folderview_select_folder",  folderview_select_folder, METH_VARARGS, "Select folder in folderview. Takes an argument of type Folder."},

     /* private */
     {"__gobj", private_wrap_gobj, METH_VARARGS, "Wraps a C GObject"},

    {NULL, NULL, 0, NULL}
};

void claws_mail_python_init(void)
{
  if (!Py_IsInitialized())
      Py_Initialize();

  /* create module */
  cm_module = Py_InitModule("clawsmail", ClawsMailMethods);

  /* initialize classes */
  initnode(cm_module);
  initcomposewindow(cm_module);
  initfolder(cm_module);

  PyRun_SimpleString("import clawsmail\n");
}
