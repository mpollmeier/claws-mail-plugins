/*
 * Sylpheed -- a GTK+ based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2003 Hiroyuki Yamamoto & the Sylpheed-Claws team
 * This file (C) 2004 Colin Leroy <colin@colino.net>
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

#ifdef USE_GPGME

#include "defs.h"
#include <glib.h>
#include <gpgme.h>

#include "utils.h"
#include "privacy.h"
#include "procmime.h"
#include "pgpinline.h"
#include "sgpgme.h"
#include "prefs_common.h"
#include "quoted-printable.h"
#include "base64.h"

typedef struct _PrivacyDataPGP PrivacyDataPGP;

struct _PrivacyDataPGP
{
	PrivacyData	data;
	
	gboolean	done_sigtest;
	gboolean	is_signed;
	GpgmeSigStat	sigstatus;
	GpgmeCtx 	ctx;
};

static PrivacySystem pgpinline_system;

static gint pgpinline_check_signature(MimeInfo *mimeinfo);

static PrivacyDataPGP *pgpinline_new_privacydata()
{
	PrivacyDataPGP *data;

	data = g_new0(PrivacyDataPGP, 1);
	data->data.system = &pgpinline_system;
	data->done_sigtest = FALSE;
	data->is_signed = FALSE;
	data->sigstatus = GPGME_SIG_STAT_NONE;
	gpgme_new(&data->ctx);
	
	return data;
}

static void pgpinline_free_privacydata(PrivacyData *_data)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) _data;
	
	g_free(data);
}

static gchar *get_part_as_string(MimeInfo *mimeinfo, gint lines)
{
	FILE *tmpfp;
	gchar buf[BUFFSIZE];
	gchar *textdata = NULL;
	Base64Decoder *decoder;
	gint i = 0;

	g_return_val_if_fail(mimeinfo != NULL, 0);

	tmpfp = fopen(mimeinfo->filename, "rb");
	if (!tmpfp)
		return NULL;

	fseek(tmpfp, mimeinfo->offset, SEEK_SET);
	
	debug_print("enc_type: %d for %s\n", mimeinfo->encoding_type,
			mimeinfo->filename);

	if (mimeinfo->encoding_type == ENC_BASE64)
		decoder = base64_decoder_new();

	while (fgets(buf, sizeof(buf), tmpfp) != NULL
		&& (lines < 0 || i++ < lines)) {
		gchar *tmp = NULL;
		if (textdata) {
			tmp = g_strdup(textdata);
			g_free(textdata);
		} else {
			tmp = g_strdup("");
		}
		
		if (mimeinfo->encoding_type == ENC_BASE64) {
			gchar outbuf[BUFFSIZE];
			int len = 0;
			
			len = base64_decoder_decode(decoder, buf, outbuf);
			if (len >= 0) {
				strcpy(buf, outbuf);
			}
		} else if (mimeinfo->encoding_type == ENC_QUOTED_PRINTABLE) {
			qp_decode_line(buf);
		}
		
		textdata = g_strdup_printf("%s%s",tmp, buf);
		g_free(tmp);
	}
	
	debug_print("returning '%s'\n", textdata);

	if (mimeinfo->encoding_type == ENC_BASE64)
		base64_decoder_free(decoder);

	fclose(tmpfp);

	return textdata;	
}

static gboolean pgpinline_is_signed(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = NULL;
	const gchar *sig_indicator = "-----BEGIN PGP SIGNED MESSAGE-----";
	gchar *textdata;
	
	g_return_val_if_fail(mimeinfo != NULL, FALSE);
	
	if (procmime_mimeinfo_parent(mimeinfo) == NULL)
		return FALSE; /* not parent */

	if (mimeinfo->type != MIMETYPE_TEXT)
		return FALSE;
	
	if (mimeinfo->privacy != NULL) {
		data = (PrivacyDataPGP *) mimeinfo->privacy;
		if (data->done_sigtest)
			return data->is_signed;
	}
	
	textdata = get_part_as_string(mimeinfo, 10);
	if (!textdata)
		return FALSE;

	if (!strstr(textdata, sig_indicator)) {
		g_free(textdata);
		return FALSE;
	}

	g_free(textdata);
	
	if (data == NULL) {
		data = pgpinline_new_privacydata();
		mimeinfo->privacy = (PrivacyData *) data;
	}
	
	data->done_sigtest = TRUE;
	data->is_signed = TRUE;

	return TRUE;
}

static gint pgpinline_check_signature(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = NULL;
	gchar *textdata = NULL;
	GpgmeData plain = NULL, cipher = NULL;

	g_return_val_if_fail(mimeinfo != NULL, 0);

	if (procmime_mimeinfo_parent(mimeinfo) == NULL)
		return 0; /* not parent */
	if (mimeinfo->type != MIMETYPE_TEXT)
		return 0;

	g_return_val_if_fail(mimeinfo->privacy != NULL, 0);
	data = (PrivacyDataPGP *) mimeinfo->privacy;

	textdata = get_part_as_string(mimeinfo, -1);
	if (!textdata)
		return 0;

	gpgme_data_new(&plain);
	gpgme_data_new(&cipher);
	
	gpgme_data_write(cipher, textdata, strlen(textdata));

	data->sigstatus = 
		sgpgme_verify_signature	(data->ctx, cipher, plain);
	
	gpgme_data_release(plain);
	gpgme_data_release(cipher);
	
	g_free(textdata);
	
	return 0;
}

static SignatureStatus pgpinline_get_sig_status(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) mimeinfo->privacy;
	
	g_return_val_if_fail(data != NULL, SIGNATURE_INVALID);

	if (data->sigstatus == GPGME_SIG_STAT_NONE && 
	    prefs_common.auto_check_signatures)
		pgpinline_check_signature(mimeinfo);

	return sgpgme_sigstat_gpgme_to_privacy(data->ctx, data->sigstatus);
}

static gchar *pgpinline_get_sig_info_short(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) mimeinfo->privacy;
	
	g_return_val_if_fail(data != NULL, g_strdup("Error"));

	if (data->sigstatus == GPGME_SIG_STAT_NONE && 
	    prefs_common.auto_check_signatures)
		pgpinline_check_signature(mimeinfo);
	
	return sgpgme_sigstat_info_short(data->ctx, data->sigstatus);
}

static gchar *pgpinline_get_sig_info_full(MimeInfo *mimeinfo)
{
	PrivacyDataPGP *data = (PrivacyDataPGP *) mimeinfo->privacy;
	
	g_return_val_if_fail(data != NULL, g_strdup("Error"));

	return sgpgme_sigstat_info_full(data->ctx, data->sigstatus);
}



static gboolean pgpinline_is_encrypted(MimeInfo *mimeinfo)
{
	const gchar *enc_indicator = "-----BEGIN PGP MESSAGE-----";
	gchar *textdata;
	
	g_return_val_if_fail(mimeinfo != NULL, FALSE);
	
	if (procmime_mimeinfo_parent(mimeinfo) == NULL)
		return FALSE; /* not parent */
	
	if (mimeinfo->type != MIMETYPE_TEXT)
		return FALSE;
	
	textdata = get_part_as_string(mimeinfo, 10);
	if (!textdata)
		return FALSE;
	
	if (!strstr(textdata, enc_indicator)) {
		g_free(textdata);
		return FALSE;
	}

	g_free(textdata);
	return TRUE;
}

static MimeInfo *pgpinline_decrypt(MimeInfo *mimeinfo)
{
	MimeInfo *decinfo, *parseinfo;
	GpgmeData cipher, plain;
	FILE *dstfp;
	gint nread;
	gchar *fname;
	gchar buf[BUFFSIZE];
	gchar *textdata = NULL;
	static gint id = 0;
	const gchar *src_codeset = NULL;
	GpgmeSigStat sigstat = 0;
	PrivacyDataPGP *data = NULL;
	GpgmeCtx ctx;
	
	if (gpgme_new(&ctx) != GPGME_No_Error)
		return NULL;


	g_return_val_if_fail(mimeinfo != NULL, NULL);
	g_return_val_if_fail(pgpinline_is_encrypted(mimeinfo), NULL);
	
	if (procmime_mimeinfo_parent(mimeinfo) == NULL)
		return NULL; /* not parent */
	if (mimeinfo->type != MIMETYPE_TEXT)
		return NULL;
	

	textdata = get_part_as_string(mimeinfo, -1);
	if (!textdata)
		return NULL;

	debug_print("decrypting '%s'\n", textdata);
	gpgme_data_new(&cipher);
	
	gpgme_data_write(cipher, textdata, strlen(textdata));

	plain = sgpgme_decrypt_verify(cipher, &sigstat, ctx);
	
	gpgme_data_release(cipher);
	
	if (plain == NULL)
		return NULL;
	
    	fname = g_strdup_printf("%s%cplaintext.%08x",
		get_mime_tmp_dir(), G_DIR_SEPARATOR, ++id);

    	if ((dstfp = fopen(fname, "wb")) == NULL) {
        	FILE_OP_ERROR(fname, "fopen");
        	g_free(fname);
        	gpgme_data_release(plain);
		return NULL;
    	}

	src_codeset = procmime_mimeinfo_get_parameter(mimeinfo, "charset");
	if (src_codeset == NULL)
		src_codeset = CS_ISO_8859_1;
		
	fprintf(dstfp, "MIME-Version: 1.0\r\n"
			"Content-Type: text/plain; charset=%s\r\n"
			"Content-Transfer-Encoding: 8bit\r\n"
			"\r\n",
			src_codeset);
	
	gpgme_data_rewind (plain);
	
	while (gpgme_data_read(plain, buf, sizeof(buf), &nread) 
		== GPGME_No_Error) {
      		fwrite(buf, nread, 1, dstfp);
	}
	fclose(dstfp);
	
	gpgme_data_release(plain);

	parseinfo = procmime_scan_file(fname);
	g_free(fname);
	
	if (parseinfo == NULL)
		return NULL;
	decinfo = g_node_first_child(parseinfo->node) != NULL ?
		g_node_first_child(parseinfo->node)->data : NULL;
		
	if (decinfo == NULL)
		return NULL;

	g_node_unlink(decinfo->node);
	procmime_mimeinfo_free_all(parseinfo);

	decinfo->tmpfile = TRUE;

	if (sigstat != GPGME_SIG_STAT_NONE) {
		if (decinfo->privacy != NULL) {
			data = (PrivacyDataPGP *) decinfo->privacy;
		} else {
			data = pgpinline_new_privacydata();
			decinfo->privacy = (PrivacyData *) data;	
		}
		data->done_sigtest = TRUE;
		data->is_signed = TRUE;
		data->sigstatus = sigstat;
		if (data->ctx)
			gpgme_release(data->ctx);
		data->ctx = ctx;
	} else
		gpgme_release(ctx);

	return decinfo;
}

static PrivacySystem pgpinline_system = {
	"pgpinline",			/* id */
	"PGP Inline",			/* name */

	pgpinline_free_privacydata,	/* free_privacydata */

	pgpinline_is_signed,		/* is_signed(MimeInfo *) */
	pgpinline_check_signature,	/* check_signature(MimeInfo *) */
	pgpinline_get_sig_status,	/* get_sig_status(MimeInfo *) */
	pgpinline_get_sig_info_short,	/* get_sig_info_short(MimeInfo *) */
	pgpinline_get_sig_info_full,	/* get_sig_info_full(MimeInfo *) */

	pgpinline_is_encrypted,		/* is_encrypted(MimeInfo *) */
	pgpinline_decrypt,		/* decrypt(MimeInfo *) */
};

void pgpinline_init()
{
	privacy_register_system(&pgpinline_system);
}

void pgpinline_done()
{
	privacy_unregister_system(&pgpinline_system);
}

#endif /* USE_GPGME */
