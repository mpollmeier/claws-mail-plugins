/* vim:et:ts=4:sw=4:et:sts=4:ai:set list listchars=tab\:»·,trail\:·: */

/*
 * Claws Mail -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2008 Michael Rasmussen and the Claws Mail Team
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
 * 
 */

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "libarchive_archive.h"

#ifndef _TEST
#	include "archiver.h"
#	include "utils.h"
#	include "mainwindow.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <glib.h>
#include <libgen.h>
#include <glib/gi18n.h>

#define READ_BLOCK_SIZE 10240

struct file_info {
	char* path;
	char* name;
};

static GSList* file_list = NULL;
static gboolean stop_action = FALSE;

#ifdef _TEST
static int permissions = 0;
#endif

static void archive_free_file_info(struct file_info* file) {
	if (! file)
		return;
	if (file->path)
		g_free(file->path);
	if (file->name)
		g_free(file->name);
	g_free(file);
	file = NULL;
}

void stop_archiving() {
	debug_print("stop action set to true\n");
	stop_action = TRUE;
}

void archive_free_file_list(gboolean md5) {
	struct file_info* file = NULL;
	gchar* path = NULL;

	debug_print("freeing file list\n");
	if (! file_list)
		return;
	while (file_list) {
		file = (struct file_info *) file_list->data;
		if (md5 && g_str_has_suffix(file->name, ".md5")) {
			path = g_strdup_printf("%s/%s", file->path, file->name);
			debug_print("unlinking %s\n", path);
			g_unlink(path);
			g_free(path);
		}
		archive_free_file_info(file);
		file_list->data = NULL;
		file_list = g_slist_next(file_list);
	}
	if (file_list) {
		g_slist_free(file_list);
		file_list = NULL;
	}
}

static struct file_info* archive_new_file_info() {
	struct file_info* new_file_info = malloc(sizeof(struct file_info));

	new_file_info->path = NULL;
	new_file_info->name = NULL;
	return new_file_info;
}

static void archive_add_to_list(struct file_info* file) {
	if (! file)
		return;
	file_list = g_slist_prepend(file_list, (gpointer) file);
}

static gchar* strip_leading_dot_slash(gchar* path) {
	gchar* stripped = path;
	gchar* result = NULL;

	if (stripped && stripped[0] == '.') {
		++stripped;
		if (stripped && stripped[0] == '/')
			++stripped;
		result = g_strdup(stripped);
	}
	else
		result = g_strdup(path);
	return result;
}

static gchar* get_full_path(struct file_info* file) {
	char* path = malloc(PATH_MAX);

	if (file->path && *(file->path))
		sprintf(path, "%s/%s", file->path, file->name);
	else
		sprintf(path, "%s", file->name);
	return path;
}

#ifdef _TEST
static gchar* strip_leading_slash(gchar* path) {
	gchar* stripped = path;
	gchar* result = NULL;

	if (stripped && stripped[0] == '/') {
		++stripped;
		result = g_strdup(stripped);
	}
	else
		result = g_strdup(path);
	return result;
}

static int archive_get_permissions() {
	return permissions;
}


void archive_set_permissions(int perm) {
	permissions = perm;
}

static int archive_copy_data(struct archive* in, struct archive* out) {
	const void* buf;
	size_t size;
	off_t offset;
	int res = ARCHIVE_OK;

	while (res == ARCHIVE_OK) {
		res = archive_read_data_block(in, &buf, &size, &offset);
		if (res == ARCHIVE_OK) {
			res = archive_write_data_block(out, buf, size, offset);
		}
	}
	return (res == ARCHIVE_EOF) ? ARCHIVE_OK : res;
}
#endif

void archive_add_file(gchar* path) {
	struct file_info* file = archive_new_file_info();
	gchar* filename = NULL;

	g_return_if_fail(path != NULL);

#ifndef _TEST
	debug_print("add %s to list\n", path);
#endif
	filename = g_strrstr_len(path, strlen(path), "/");
	if (! filename)
		g_warning(path);
	g_return_if_fail(filename != NULL);

	filename++;
	file->name = g_strdup(filename);
	file->path = strip_leading_dot_slash(dirname(path));
	archive_add_to_list(file);
}

GSList* archive_get_file_list() {
	return file_list;
}

#ifdef _TEST
const gchar* archive_extract(const char* archive_name, int flags) {
	struct archive* in;
	struct archive* out;
	struct archive_entry* entry;
	int res = ARCHIVE_OK;
	gchar* buf = NULL;
	const char* result == NULL;

	g_return_val_if_fail(archive_name != NULL, ARCHIVE_FATAL);

	fprintf(stdout, "%s: extracting\n", archive_name);
	in = archive_read_new();
	if ((res = archive_read_support_format_tar(in)) == ARCHIVE_OK) {
		if ((res = archive_read_support_compression_gzip(in)) == ARCHIVE_OK) {
			if ((res = archive_read_open_file(
				in, archive_name, READ_BLOCK_SIZE)) != ARCHIVE_OK) {
				buf = g_strdup_printf(
						"%s: %s\n", archive_name, archive_error_string(in));
				g_warning(buf);
				g_free(buf);
				result = archive_error_string(in);
			}
			else {
				out = archive_write_disk_new();
				if ((res = archive_write_disk_set_options(
								out, flags)) == ARCHIVE_OK) {
					res = archive_read_next_header(in, &entry);
					while (res == ARCHIVE_OK) {
						fprintf(stdout, "%s\n", archive_entry_pathname(entry));
						res = archive_write_header(out, entry);
						if (res != ARCHIVE_OK) {
							buf = g_strdup_printf("%s\n", 
											archive_error_string(out));
							g_warning(buf);
							g_free(buf);
							/* skip this file an continue */
							res = ARCHIVE_OK;
						}
						else {
							res = archive_copy_data(in, out);
							if (res != ARCHIVE_OK) {
								buf = g_strdup_printf("%s\n", 
												archive_error_string(in));
								g_warning(buf);
								g_free(buf);
								/* skip this file an continue */
								res = ARCHIVE_OK;
							}
							else
								res = archive_read_next_header(in, &entry);
						}
					}
					if (res == ARCHIVE_EOF)
						res = ARCHIVE_OK;
					if (res != ARCHIVE_OK) {
						buf = g_strdup_printf("%s\n", archive_error_string(in));
						if (*buf == '\n') {
							g_free(buf);
							buf = g_strdup_printf("%s: Unknown error\n", archive_name);
						}
						g_warning(buf);
						g_free(buf);
						result = archive_error_string(in);
					}
				}
				else
					result = archive_error_string(out);
				archive_read_close(in);
			}
			archive_read_finish(in);
		}
		else
			result = archive_error_string(in);
	}
	else
		result = archive_error_string(in);
	return result;
}
#endif

const gchar* archive_create(const char* archive_name, GSList* files,
			COMPRESS_METHOD method, ARCHIVE_FORMAT format) {
	struct archive* arch;
	struct archive_entry* entry;
	char* buf = NULL;
	ssize_t len;
	int fd;
	struct stat st;
	struct file_info* file;
	gchar* filename = NULL;
	gchar* msg = NULL;

#ifndef _TEST
	gint num = 0;
	gint total = g_slist_length (files);
/*	MainWindow* mainwin = mainwindow_get_mainwindow();*/
#endif

	if (! files)
		g_return_val_if_fail(files != NULL, "No files for archiving");

	debug_print("File: %s\n", archive_name);
	arch = archive_write_new();
	switch (method) {
		case ZIP:
			if (archive_write_set_compression_gzip(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;
		case BZIP2:
			if (archive_write_set_compression_bzip2(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;
/*		case COMPRESS:
			if (archive_write_set_compression_gzip(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;*/
		case NO_COMPRESS:
			if (archive_write_set_compression_none(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;
	}
	switch (format) {
		case TAR:
			if (archive_write_set_format_ustar(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;
		case SHAR:
			if (archive_write_set_format_shar(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;
		case PAX:
			if (archive_write_set_format_pax(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;
		case CPIO:
			if (archive_write_set_format_cpio(arch) != ARCHIVE_OK)
				return archive_error_string(arch);
			break;
		case NO_FORMAT:
			return "Missing archive format";
	}
	if (archive_write_open_file(arch, archive_name) != ARCHIVE_OK)
		return archive_error_string(arch);

	while (files && ! stop_action) {
#ifndef _TEST
		set_progress_print_all(num++, total, 30);
#endif
		file = (struct file_info *) files->data;
		if (!file)
			continue;
		filename = get_full_path(file);
		/* libarchive will crash if instructed to add archive to it self */
		if (g_utf8_collate(archive_name, filename) == 0) {
			buf = NULL;
			buf = g_strdup_printf(
						"%s: Not dumping to %s", archive_name, filename);
			g_warning(buf);
#ifndef _TEST
			debug_print("%s\n", buf);
#endif
			g_free(buf);
		}
		else {
#ifndef _TEST
			debug_print("Adding: %s\n", filename);
			msg = g_strdup_printf(_("Archiving %s"), filename);
			/*set_progress_file_label(msg);*/
			g_free(msg);
#endif
			entry = archive_entry_new();
			lstat(filename, &st);
			if ((fd = open(filename, O_RDONLY)) == -1) {
				perror("open file");
			}
			else {
				archive_entry_copy_stat(entry, &st);
				archive_entry_set_pathname(entry, filename);
				if (S_ISLNK(st.st_mode)) {
					buf = NULL;
					buf = malloc(PATH_MAX + 1);
					if ((len = readlink(filename, buf, PATH_MAX)) < 0)
						perror("error in readlink");
					else
						buf[len] = '\0';
					archive_entry_set_symlink(entry, buf);
					g_free(buf);
					archive_entry_set_size(entry, 0);
					archive_write_header(arch, entry);
				}
				else {
					archive_write_header(arch, entry);
					buf = NULL;
					buf = malloc(READ_BLOCK_SIZE);
					len = read(fd, buf, READ_BLOCK_SIZE);
					/*debug_print("First read: %d byte(s) read\n", len);*/
					while (len > 0) {
						archive_write_data(arch, buf, len);
						memset(buf, 0, READ_BLOCK_SIZE);
						len = read(fd, buf, READ_BLOCK_SIZE);
						/*debug_print("Read: %d byte(s) read\n", len);*/
					}
					g_free(buf);
				}
				close(fd);
				archive_entry_free(entry);
			}
		}
		g_free(filename);
		files = g_slist_next(files);
	}
#ifndef _TEST
	if (stop_action)
		unlink(archive_name);
	stop_action = FALSE;
#endif
	archive_write_close(arch);
	archive_write_finish(arch);
	return NULL;
}

#ifdef _TEST
void archive_scan_folder(const char* dir) {
	struct stat st;
	DIR* root;
	struct dirent* ent;
	gchar cwd[PATH_MAX];
	gchar path[PATH_MAX];
	
	getcwd(cwd, PATH_MAX);

	if (stat(dir, &st) == -1)
		return;
	if (! S_ISDIR(st.st_mode))
		return;
	if (!(root = opendir(dir)))
		return;
	chdir(dir);

	while ((ent = readdir(root)) != NULL) {
		/*fprintf(stdout, "%s\n", ent->d_name);*/
		if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
			continue;
		stat(ent->d_name, &st);
		sprintf(path, "%s/%s", dir, ent->d_name);
		if (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) {
			/*struct file_info* file = archive_new_file_info();
			file->path = strip_leading_dot_slash((char *) dir);
			file->name = g_strdup(ent->d_name);
			archive_add_to_list(file);*/
			/*archive_free_file_info(file);*/
			archive_add_file(path);
		}
		else if (S_ISDIR(st.st_mode)) {
			archive_scan_folder(path);
		}
	}
	chdir(cwd);
	closedir(root);
}

int main(int argc, char** argv) {
	char* archive = NULL;
	char buf[PATH_MAX];
	int pid;
	int opt;
	int perm = ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME |
		ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS | ARCHIVE_EXTRACT_SECURE_SYMLINKS;
	gchar cwd[PATH_MAX];
	gboolean remove = FALSE;
	const char *p = NULL;
	int res;

	getcwd(cwd, PATH_MAX);

	while (*++argv && **argv == '-') {
		p = *argv + 1;

		while ((opt = *p++) != '\0') {
			switch(opt) {
				case 'a':
					if (*p != '\0')
						archive = (char *) p;
					else
						archive = *++argv;
					p += strlen(p);
					break;
				case 'r':
					remove = TRUE;
					break;
			}
		}
	}
	if (! archive) {
		fprintf(stderr, "Missing archive name!\n");
		return EXIT_FAILURE;
	}
	if (!*argv) {
		fprintf(stderr, "Expected arguments after options!\n");
		return EXIT_FAILURE;
	}
	
	while (*argv) {
		archive_scan_folder(*argv++);
		res = archive_create(archive, file_list);
		if (res != ARCHIVE_OK) {
			fprintf(stderr, "%s: Creating archive failed\n", archive);
			return EXIT_FAILURE;
		}
	}
	pid = (int) getpid();
	sprintf(buf, "/tmp/%d", pid);
	fprintf(stdout, "Creating: %s\n", buf);
	mkdir(buf, 0700);
	chdir(buf);
	if (strcmp(dirname(archive), ".") == 0) 
		sprintf(buf, "%s/%s", cwd, basename(archive));
	else
		sprintf(buf, "%s", archive);
	archive_extract(buf, perm);
/*		while (file_list) {
			long i = 0;
			struct file_info* file = (struct file_info *) file_list->data;
			fprintf(stdout, "[%ld] Path: %s -> Name: %s\n",
					i++, file->path, file->name);
			file_list = g_slist_next(file_list);
		}*/
	chdir(cwd);
	if (remove) {
		sprintf(buf, "rm -rf /tmp/%d", pid);
		fprintf(stdout, "Executing: %s\n", buf);
		system(buf);
	}
	archive_free_list(file_list);
	return EXIT_SUCCESS;
}
#endif
