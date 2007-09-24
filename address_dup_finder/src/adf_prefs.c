/* Address duplicate finder plugin for Claws-Mail
 * Copyright (C) 2007 Holger Berndt
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

#include "adf_prefs.h"

#include "defs.h"

AdfPrefs adf_config;

PrefParam adf_param[] = {
  {"include_same_book", "TRUE", &adf_config.include_same_book,
   P_BOOL, NULL, NULL, NULL},
  {"include_other_books", "TRUE", &adf_config.include_other_books,
   P_BOOL, NULL, NULL, NULL},
  {NULL, NULL, NULL, P_OTHER, NULL, NULL, NULL}
};

void adf_save_config(void)
{
  PrefFile *pfile;
  gchar *rcpath;

  debug_print("Saving adf plugin configuration...\n");

  rcpath = g_strconcat(get_rc_dir(), G_DIR_SEPARATOR_S, COMMON_RC, NULL);
  pfile = prefs_write_open(rcpath);
  g_free(rcpath);
  if(!pfile || (prefs_set_block_label(pfile, "AddressDupesFinderPlugin") < 0))
    return;
  
  if(prefs_write_param(adf_param, pfile->fp) < 0) {
    debug_print("failed!\n");
    debug_print("Failed to write plugin configuration to file\n");
    prefs_file_close_revert(pfile);
    return;
  }
  fprintf(pfile->fp, "\n");
  prefs_file_close(pfile);
  debug_print("done.\n");
}
