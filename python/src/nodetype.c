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

#include "nodetype.h"

#include <structmember.h>

PyMODINIT_FUNC initnode(PyObject *module)
{
  PyObject *dict;
  PyObject *res;
  const char *cmd =
      "class Node:\n"
      "    def __init__(self):\n"
      "        self.data = None\n"
      "        self.children = []\n"
      "\n"
      "    def __str__(self):\n"
      "        return '\\n'.join(self.get_str_list(0))\n"
      "\n"
      "    def get_str_list(self, level):\n"
      "        str = []\n"
      "        indent = '  '*level\n"
      "        if self.data:\n"
      "            str.append(indent + self.data.__str__())\n"
      "        else:\n"
      "            str.append(indent + 'None')\n"
      "        for child in self.children:\n"
      "            str.extend(child.get_str_list(level+1))\n"
      "        return str\n"
      "\n";

  dict = PyModule_GetDict(module);
  res = PyRun_String(cmd, Py_file_input, dict, dict);
  Py_XDECREF(res);
}

PyObject* clawsmail_node_new(PyObject *module)
{
  PyObject *class, *dict;

  dict = PyModule_GetDict(module);
  class = PyDict_GetItemString(dict, "Node");
  return PyObject_CallObject(class, NULL);
}
