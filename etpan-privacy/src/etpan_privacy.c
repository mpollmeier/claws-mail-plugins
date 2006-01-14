/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto & the Sylpheed-Claws team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "defs.h"
#include <glib.h>

#include "utils.h"
#include "privacy.h"
#include "procmime.h"
#include "prefs_common.h"
#include "etpan_privacy.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libetpan.h>
#include <limits.h>

static struct mailprivacy * privacy = NULL;

struct mailprivacy * get_etpan_privacy(void)
{
	return privacy;
}

static void free_privacydata(PrivacyData *_data)
{
	/* do nothing */
}

static void hash_add_param(gpointer name, gpointer value, gpointer user_data)
{
	clist * list;
	char * ep_name;
	char * ep_value;
	struct mailmime_parameter * param;
	int r;

	list = user_data;
	
	ep_name = strdup(name);
	if (ep_name == NULL)
		goto err;
	
	ep_value = strdup(value);
	if (ep_value == NULL)
		goto free_name;
	
	param = mailmime_parameter_new(ep_name, ep_value);
	if (param == NULL)
		goto free_value;
	
	r = clist_append(list, param);
	if (r < 0)
		goto free_param;
	
	return;
	
 free_param:
	mailmime_parameter_free(param);
	goto err;
 free_value:
	free(ep_value);
 free_name:
	free(ep_name);
 err:
	return;
}

static clist * parameters_to_etpan(GHashTable * parameters)
{
	clist * list;
	
	list = clist_new();
	if (list == NULL)
		return list;
	
        g_hash_table_foreach(parameters, hash_add_param, list);
	
	return list;
}

static int typeetpan[] = {
	MAILMIME_DISCRETE_TYPE_TEXT,
	MAILMIME_DISCRETE_TYPE_IMAGE,
	MAILMIME_DISCRETE_TYPE_AUDIO,
	MAILMIME_DISCRETE_TYPE_VIDEO,
	MAILMIME_DISCRETE_TYPE_APPLICATION,
	MAILMIME_DISCRETE_TYPE_ERROR,
	MAILMIME_DISCRETE_TYPE_ERROR,
	MAILMIME_DISCRETE_TYPE_ERROR,
};

static struct mailmime_discrete_type *
discrete_type_to_etpan(MimeInfo * mimeinfo)
{
	char * extension;
	struct mailmime_discrete_type * dt;
	int dt_type;

	switch (mimeinfo->type) {
	case MIMETYPE_TEXT:
	case MIMETYPE_IMAGE:
	case MIMETYPE_AUDIO:
	case MIMETYPE_VIDEO:
	case MIMETYPE_APPLICATION:
		dt_type = typeetpan[mimeinfo->type];
		extension = NULL;
		break;
		
	case MIMETYPE_UNKNOWN:
		dt_type = MAILMIME_DISCRETE_TYPE_EXTENSION;
		extension = strdup("unknown");
		if (extension == NULL)
			return NULL;
		break;

	default:
		return NULL;
	}
	
	dt = mailmime_discrete_type_new(dt_type, extension);
	if (dt == NULL) {
		if (extension != NULL)
			free(extension);
		return NULL;
	}
	
	return dt;
}

static struct mailmime_composite_type *
composite_type_to_etpan(MimeInfo * mimeinfo)
{
	struct mailmime_composite_type * ct;
	int ct_type;
	
	switch (mimeinfo->type) {
	case MIMETYPE_MULTIPART:
		ct_type = MAILMIME_COMPOSITE_TYPE_MULTIPART;
		break;
		
	case MIMETYPE_MESSAGE:
		ct_type = MAILMIME_COMPOSITE_TYPE_MESSAGE;
		break;

	default:
		return NULL;
	}
	
	return mailmime_composite_type_new(ct_type, NULL);
}

static struct mailmime_content * content_type_to_etpan(MimeInfo * mimeinfo)
{
	struct mailmime_content * content_type;
	struct mailmime_type * type;
	struct mailmime_composite_type * composite_type;
	struct mailmime_discrete_type * discrete_type;
	int tp_type;
	int ct_type;
	clist * parameters;
	char * subtype;
	
	discrete_type = NULL;
	composite_type = NULL;
	
	switch (mimeinfo->type) {
	case MIMETYPE_TEXT:
	case MIMETYPE_IMAGE:
	case MIMETYPE_AUDIO:
	case MIMETYPE_VIDEO:
	case MIMETYPE_APPLICATION:
	case MIMETYPE_UNKNOWN:
		discrete_type = discrete_type_to_etpan(mimeinfo);
		if (discrete_type == NULL)
			return NULL;
		tp_type = MAILMIME_TYPE_DISCRETE_TYPE;
		break;
		
	case MIMETYPE_MESSAGE:
	case MIMETYPE_MULTIPART:
		composite_type = composite_type_to_etpan(mimeinfo);
		if (composite_type == NULL)
			return NULL;
		tp_type = MAILMIME_TYPE_COMPOSITE_TYPE;
		break;
		
	default:
		goto err;
	}

	type = mailmime_type_new(tp_type, discrete_type, composite_type);
	if (type == NULL) {
		if (discrete_type != NULL)
			mailmime_discrete_type_free(discrete_type);
		if (composite_type != NULL)
			mailmime_composite_type_free(composite_type);
		goto err;
	}
	
	parameters = parameters_to_etpan(mimeinfo->typeparameters);
	if (parameters == NULL)
		goto free_type;

	subtype = strdup(mimeinfo->subtype);
	if (subtype == NULL)
		goto free_parameters;

	content_type = mailmime_content_new(type, subtype, parameters);
	if (content_type == NULL)
		goto free_subtype;
	
	return content_type;

 free_subtype:
	free(subtype);
 free_parameters:
	clist_foreach(parameters, (clist_func) mailmime_parameter_free, NULL);
 free_type:
	mailmime_type_free(type);
 err:
	return NULL;
}


/*
  procmime_to_file()
  
  do minimal conversion
*/

static int procmime_to_file(char * filename,
			    MimeInfo * mimeinfo)
{
	struct mailmime * mime;
	clist * param;
	struct mailmime_content * content_type;
	struct mailmime_fields * mime_fields;
	struct mailmime_mechanism * enc;
	int fd;
	struct stat stat_info;
	char * mapping;
	FILE * f_dest;
	int enctype;
	int col;
	int r;
	const char * storefilename;
	int mapped;
	
	content_type = content_type_to_etpan(mimeinfo);
	if (content_type == NULL)
		goto err;

	switch (mimeinfo->encoding_type) {
	case ENC_7BIT:
		enctype = MAILMIME_MECHANISM_7BIT;
		break;

	case ENC_8BIT:
		enctype = MAILMIME_MECHANISM_8BIT;
		break;

	case ENC_QUOTED_PRINTABLE:
		enctype = MAILMIME_MECHANISM_QUOTED_PRINTABLE;
		break;
		
	case ENC_BASE64:
		enctype = MAILMIME_MECHANISM_BASE64;
		break;
	
	case ENC_BINARY:
	case ENC_X_UUENCODE:
	case ENC_UNKNOWN:
		enctype = MAILMIME_MECHANISM_BINARY;
		break;
	}
	
	mime_fields = mailmime_fields_new_encoding(enctype);
	if (mime_fields == NULL)
		goto free_content_type;

	if (mimeinfo->content == MIMECONTENT_EMPTY) {
		goto free_content_type;
	}
	
	if (mimeinfo->content == MIMECONTENT_FILE) {
		storefilename = mimeinfo->data.filename;
	}
	
	mapped = 0;
	mapping = NULL;
	if (storefilename != NULL) {
		r = stat(storefilename, &stat_info);
		if (r < 0)
			goto free_content_type;
		
		fd = open(storefilename, O_RDONLY);
		if (fd < 0)
			goto free_content_type;
		
		mapping = mmap(NULL, stat_info.st_size,
			       PROT_READ, MAP_PRIVATE, fd, 0);
		if (mapping == MAP_FAILED)
			goto close;
		
		mapped = 1;
	}
	else {
		mapping = mimeinfo->data.mem;
	}
	
	f_dest = fopen(filename, "w");
	if (f_dest == NULL)
		goto unmap;
	
	col = 0;
	mailmime_content_write(f_dest, &col, content_type);
	mailmime_fields_write(f_dest, &col, mime_fields);
	fprintf(f_dest, "\r\n");
	fwrite(mapping + mimeinfo->offset, 1, mimeinfo->length, f_dest);
	fclose(f_dest);
	if (mapped) {
		munmap(mapping, stat_info.st_size);
	}
	
	close(fd);
	mailmime_fields_free(mime_fields);
	mailmime_content_free(content_type); 
	
	return 0;
	
 unmap:
	munmap(mapping, stat_info.st_size);
 close:
	close(fd);
 free_mime_fields:
	mailmime_fields_free(mime_fields);
 free_content_type:
	mailmime_content_free(content_type); 
 err:
	return -1; 
}


static struct mailmime * procmime_to_etpan(struct mailprivacy * privacy,
					   MimeInfo * mimeinfo)
{
	FILE * f_dest;
	char tmpfile[PATH_MAX];
	int fd;
	struct stat stat_info;
	char * mapping;
	int r;
	struct mailmime * mime;

	f_dest = mailprivacy_get_tmp_file(privacy, tmpfile, sizeof(tmpfile));
	if (f_dest == NULL)
		goto err;
	
	fclose(f_dest);
	
	r = procmime_to_file(tmpfile, mimeinfo);
	if (r < 0)
		goto unlink;
	
	/* parse tmp file */
	
	r = stat(tmpfile, &stat_info);
	if (r < 0)
		goto unlink;

	fd = open(tmpfile, O_RDONLY);
	if (fd < 0)
		goto unlink;
	
	mapping = mmap(NULL, stat_info.st_size,
		       PROT_READ, MAP_PRIVATE, fd, 0);
	if (mapping == MAP_FAILED)
		goto close_tmp;
	
	r = mailprivacy_get_mime(privacy, 0, 1, mapping, stat_info.st_size,
				 &mime);
	if (r != MAIL_NO_ERROR)
		goto unmap_tmp;
	
	munmap(mapping, stat_info.st_size);
	close(fd);

	unlink(tmpfile);
	
	return mime;
	
 unmap_tmp:
	munmap(mapping, stat_info.st_size);
 close_tmp:
	close(fd);
 unlink:
	unlink(tmpfile);
 err:
	return NULL; 
}


static MimeInfo * mime_to_sylpheed(struct mailmime * mime)
{
	FILE * f;
	char tmpfile[PATH_MAX];
	int col;
	int r;
	MimeInfo * mimeinfo;
	GNode * main_node;
	MimeInfo * main_mime;

	f = mailprivacy_get_tmp_file(privacy, tmpfile, sizeof(tmpfile));
	if (f == NULL)
		goto err;
	
	col = 0;
	fprintf(f, "MIME-Version: 1.0\r\n");
	mailmime_write(f, &col, mime);
	fclose(f);
	
	mimeinfo = procmime_scan_file(tmpfile);
	if (mimeinfo == NULL)
		goto unlink;
	
	mimeinfo->tmp = 1;
	
	return mimeinfo;
	
 unlink:
	unlink(tmpfile);
 err:
	return NULL;
}

static int is_encrypted(MimeInfo *mimeinfo)
{
	struct mailmime * mime;
	int r;
	mailmessage * msg;
	int res;
	char tmpfile[PATH_MAX];
	FILE * f;
	int fd;
	struct stat stat_info;
	char * mapping;

	f = mailprivacy_get_tmp_file(privacy, tmpfile, sizeof(tmpfile));
	if (f == NULL)
		goto err;
	
	fclose(f);
	
	r = procmime_to_file(tmpfile, mimeinfo);
	if (r < 0) {
		fprintf(stderr, "could not procmime to file\n");
		goto unlink;
	}
	
	stat(tmpfile, &stat_info);
	fd = open(tmpfile, O_RDONLY);
	
	mapping = mmap(NULL, stat_info.st_size,
		       PROT_READ, MAP_PRIVATE, fd, 0);
	if (mapping == MAP_FAILED)
		goto close;

	msg = data_message_init(mapping, stat_info.st_size);
	if (msg == NULL)
		goto unmap;
	
	r = mailmessage_get_bodystructure(msg, &mime);
	if (r != MAIL_NO_ERROR)
		goto free_msg;
	
	res = mailprivacy_is_encrypted(privacy, msg,
				       mime->mm_data.mm_message.mm_msg_mime);
	
	mailmessage_free(msg);
	
	munmap(mapping, stat_info.st_size);
	close(fd);
	unlink(tmpfile);
	
	return res;

 free_msg:
	mailmessage_free(msg);
 unmap:
	munmap(mapping, stat_info.st_size);
 close:
	close(fd);
 unlink:
	unlink(tmpfile);
 err:
	return 0;
}

static MimeInfo * decrypt(MimeInfo *mimeinfo)
{
	MimeInfo * decrypted_mimeinfo;
	struct mailmime * decrypted_mime;
	struct mailmime * child_mime;
	struct mailmime * mime;
	int r;
	mailmessage * msg;
	int res;
	char tmpfile[PATH_MAX];
	FILE * f;
	int fd;
	struct stat stat_info;
	char * mapping;

	f = mailprivacy_get_tmp_file(privacy, tmpfile, sizeof(tmpfile));
	if (f == NULL)
		goto err;
	
	fclose(f);
	
	r = procmime_to_file(tmpfile, mimeinfo);
	if (r < 0)
		goto unlink;
	
	stat(tmpfile, &stat_info);
	fd = open(tmpfile, O_RDONLY);
	
	mapping = mmap(NULL, stat_info.st_size,
		       PROT_READ, MAP_PRIVATE, fd, 0);
	if (mapping == MAP_FAILED)
		goto close;
	
	r = mailprivacy_get_mime(privacy, 1, 1,
				 mapping, stat_info.st_size,
				 &decrypted_mime);
	if (r != MAIL_NO_ERROR)
		goto unmap;
	
	if (decrypted_mime->mm_type != MAILMIME_MESSAGE)
		goto free_decrypted;
	
	child_mime = decrypted_mime->mm_data.mm_message.mm_msg_mime;
	
	if (child_mime->mm_content_type->ct_subtype == NULL)
		goto free_decrypted;
	
	if ((strcasecmp(child_mime->mm_content_type->ct_subtype,
		       "x-verified") != 0) && 
	    (strcasecmp(child_mime->mm_content_type->ct_subtype,
			"x-decrypted") != 0))
		goto free_decrypted;
	
	decrypted_mimeinfo = mime_to_sylpheed(decrypted_mime);
	if (decrypted_mimeinfo == NULL)
		goto free_decrypted;
	
	mailprivacy_mime_clear(decrypted_mime);
	mailmime_free(decrypted_mime);
	
	munmap(mapping, stat_info.st_size);
	close(fd);
	unlink(tmpfile);
	
	return decrypted_mimeinfo;
	
 free_decrypted:
	mailprivacy_mime_clear(decrypted_mime);
	mailmime_free(decrypted_mime);
 unmap:
	munmap(mapping, stat_info.st_size);
 close:
	close(fd);
 unlink:
	unlink(tmpfile);
 err:
	return NULL;
}


static PrivacySystem etpan_privacy_system = {
	"etpan-privacy",
	"etPan! Privacy",               /* name */

	free_privacydata,	        /* free_privacydata */

	NULL,            		/* is_signed(MimeInfo *) */
	NULL,	                        /* check_signature(MimeInfo *) */
	NULL,		                /* get_sig_status(MimeInfo *) */
	NULL,	                        /* get_sig_info_short(MimeInfo *) */
	NULL,	                        /* get_sig_info_full(MimeInfo *) */

	is_encrypted,		        /* is_encrypted(MimeInfo *) */
	decrypt,		        /* decrypt(MimeInfo *) */
	
	0,
	NULL,
	
	0,
	NULL,
	NULL,
};

int etpan_privacy_init(void)
{
	gchar * tmp_dir;
	int r;
	
	tmp_dir = get_tmp_dir();
	privacy = mailprivacy_new(tmp_dir, 0);
	
	if (privacy == NULL)
		goto err;
	
	r = mailprivacy_gnupg_init(privacy);
	if (r != MAIL_NO_ERROR)
		goto free_privacy;

	r = mailprivacy_smime_init(privacy);
	if (r != MAIL_NO_ERROR)
		goto done_gnupg;
	
	privacy_register_system(&etpan_privacy_system);
	
	return 0;
	
 done_gnupg:
	mailprivacy_gnupg_done(privacy);
 free_privacy:
	mailprivacy_free(privacy);
 err:
	return -1;
}

void etpan_privacy_done(void)
{
	mailprivacy_smime_done(privacy);
	mailprivacy_gnupg_done(privacy);
	mailprivacy_free(privacy);
	privacy_unregister_system(&etpan_privacy_system);
}
