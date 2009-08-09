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

#include "composewindowtype.h"

#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "mainwindow.h"
#include "toolbar.h"

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


static PyMethodDef ClawsMailMethods[] = {
    /* public */
    {"get_mainwindow_action_group",  get_mainwindow_action_group, METH_VARARGS,
     "Get action group of main window menu."},

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
  //initComposeWindow();
  initcomposewindow(cm_module);

  PyRun_SimpleString("import clawsmail\n");
}
