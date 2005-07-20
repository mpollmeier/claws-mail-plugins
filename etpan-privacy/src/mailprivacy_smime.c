/*
 * libEtPan! -- a mail library
 *
 * Copyright (C) 2001 - 2003 - DINH Viet Hoa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the libEtPan! project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * $Id: mailprivacy_smime.c,v 1.2.2.2 2005-07-20 09:01:44 twb Exp $
 */

#include "mailprivacy_smime.h"
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "mailprivacy_tools.h"
#include "mailprivacy.h"
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>

/*
  global variable
  
  TODO : instance of privacy drivers
*/

static char cert_dir[PATH_MAX] = "";
static chash * certificates = NULL;
static chash * private_keys = NULL;
static char CAcert_dir[PATH_MAX] = "";
static char * CAfile = NULL;
static int CA_check = 1;
static int store_cert = 0;
static char private_keys_dir[PATH_MAX] = "";

static char * get_cert_file(char * email);

static char * get_private_key_file(char * email);


static int smime_is_signed(struct mailmime * mime)
{
  if (mime->mm_content_type != NULL) {
    clistiter * cur;

    for(cur = clist_begin(mime->mm_content_type->ct_parameters) ; cur != NULL ;
        cur = clist_next(cur)) {
      struct mailmime_parameter * param;
          
      param = cur->data;

      if ((strcasecmp(param->pa_name, "protocol") == 0) &&
          ((strcasecmp(param->pa_value, "application/x-pkcs7-signature") == 0) ||
              (strcasecmp(param->pa_value, "application/pkcs7-signature") == 0)))
        return 1;
    }
  }

  return 0;
}

static int smime_is_encrypted(struct mailmime * mime)
{
  if (mime->mm_content_type != NULL) {
    if ((strcasecmp(mime->mm_content_type->ct_subtype, "x-pkcs7-mime") == 0) ||
        (strcasecmp(mime->mm_content_type->ct_subtype, "pkcs7-mime") == 0))
      return 1;
  }

  return 0;
}

#define BUF_SIZE 1024

enum {
  NO_ERROR_SMIME = 0,
  ERROR_SMIME_CHECK,
  ERROR_SMIME_COMMAND,
  ERROR_SMIME_FILE,
};

/* write output to a file */

static int get_smime_output(FILE * dest_f, char * command)
{
  FILE * p;
  char buf[BUF_SIZE];
  size_t size;
  int res;
  int status;
  char command_redirected[PATH_MAX];
  
  snprintf(command_redirected, sizeof(command_redirected), "%s 2>&1", command);
  
  /*
    flush buffer so that it is not flushed more than once when forking
  */
  fflush(dest_f);
  
  p = popen(command_redirected, "r");
  if (p == NULL) {
    res = ERROR_SMIME_COMMAND;
    goto err;
  }
  
  while ((size = fread(buf, 1, sizeof(buf), p)) != 0) {
    size_t written;
    
    written = fwrite(buf, 1, size, dest_f);
    if (written != size) {
      res = ERROR_SMIME_FILE;
      goto close;
    }
  }
  status = pclose(p);
  
  if (WEXITSTATUS(status) != 0)
    return ERROR_SMIME_CHECK;
  else
    return NO_ERROR_SMIME;
  
 close:
  pclose(p);
 err:
  return res;
}

static char * get_first_from_addr(struct mailmime * mime)
{
  clistiter * cur;
  struct mailimf_single_fields single_fields;
  struct mailimf_fields * fields;
  struct mailimf_mailbox * mb;
  
  while (mime->mm_parent != NULL)
    mime = mime->mm_parent;
  
  if (mime->mm_type != MAILMIME_MESSAGE)
    return NULL;
  
  fields = mime->mm_data.mm_message.mm_fields;
  if (fields == NULL)
    return NULL;
  
  mailimf_single_fields_init(&single_fields, fields);
  
  if (single_fields.fld_from == NULL)
    return NULL;
  
  cur = clist_begin(single_fields.fld_from->frm_mb_list->mb_list);
  if (cur == NULL)
    return NULL;
  
  mb = clist_content(cur);
  
  return mb->mb_addr_spec;
}

#define SMIME_DECRYPT_DESCRIPTION "S/MIME encrypted part\r\n"
#define SMIME_DECRYPT_FAILED "S/MIME decryption FAILED\r\n"
#define SMIME_DECRYPT_SUCCESS "S/MIME decryption success\r\n"

static int smime_decrypt(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime, struct mailmime ** result)
{
  char smime_filename[PATH_MAX];
  char quoted_smime_filename[PATH_MAX];
  char description_filename[PATH_MAX];
  FILE * description_f;
  char decrypted_filename[PATH_MAX];
  FILE * decrypted_f;
  char command[PATH_MAX];
  struct mailmime * description_mime;
  struct mailmime * decrypted_mime;
  int r;
  int res;
  int sign_ok;
  char quoted_decrypted_filename[PATH_MAX];
  struct mailmime * multipart;
  char * smime_cert;
  char * smime_key;
  char quoted_smime_cert[PATH_MAX];
  char quoted_smime_key[PATH_MAX];
  char * email;
  chashiter * iter;
  
  /* fetch the whole multipart and write it to a file */
  
  r = mailprivacy_fetch_mime_body_to_file(privacy,
      smime_filename, sizeof(smime_filename),
      msg, mime);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto err;
  }
  
  /* we are in a safe directory */
  
  decrypted_f = mailprivacy_get_tmp_file(privacy,
      decrypted_filename,
      sizeof(decrypted_filename));
  if (decrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_smime;
  }
  fclose(decrypted_f);

  sign_ok = 0;
  for(iter = chash_begin(private_keys) ; iter != NULL ;
      iter = chash_next(private_keys, iter)) {
    chashdatum key;
    char email_buf[BUF_SIZE];
    
    chash_key(iter, &key);
    
    if (key.len >= sizeof(email_buf))
      continue;
    
    strncpy(email_buf, key.data, key.len);
    email_buf[key.len] = '\0';
    email = email_buf;
    
    /* description */
    
    description_f = mailprivacy_get_tmp_file(privacy,
        description_filename,
        sizeof(description_filename));
    if (description_f == NULL) {
      res = MAIL_ERROR_FILE;
      goto unlink_decrypted;
    }
  
    fprintf(description_f, SMIME_DECRYPT_DESCRIPTION);
  
    /* get encryption key */

#if 0  
    email = get_first_from_addr(mime);
    if (email == NULL) {
      fclose(description_f);
      res = MAIL_ERROR_INVAL;
      goto unlink_description;
    }
#endif
  
    smime_key = get_private_key_file(email);
    smime_cert = get_cert_file(email);
    if ((smime_cert == NULL) || (smime_key == NULL)) {
      fclose(description_f);
      res = MAIL_ERROR_INVAL;
      goto unlink_description;
    }
  
    r = mail_quote_filename(quoted_smime_cert, sizeof(quoted_smime_cert),
        smime_cert);
    if (r < 0) {
      fclose(description_f);
      res = MAIL_ERROR_MEMORY;
      goto unlink_description;
    }
  
    r = mail_quote_filename(quoted_smime_key, sizeof(quoted_smime_key),
        smime_key);
    if (r < 0) {
      fclose(description_f);
      res = MAIL_ERROR_MEMORY;
      goto unlink_description;
    }
  
    /* run the command */
  
    r = mail_quote_filename(quoted_smime_filename,
        sizeof(quoted_smime_filename), smime_filename);
    if (r < 0) {
      fclose(description_f);
      res = MAIL_ERROR_MEMORY;
      goto unlink_description;
    }
  
    r = mail_quote_filename(quoted_decrypted_filename,
        sizeof(quoted_decrypted_filename), decrypted_filename);
    if (r < 0) {
      fclose(description_f);
      res = MAIL_ERROR_MEMORY;
      goto unlink_description;
    }
  
    sign_ok = 0;
    snprintf(command, PATH_MAX,
        "openssl smime -decrypt -in %s -out %s -inkey %s -recip %s",
        quoted_smime_filename, quoted_decrypted_filename,
        quoted_smime_key, quoted_smime_cert);
  
    r = get_smime_output(description_f, command);
    switch (r) {
    case NO_ERROR_SMIME:
      sign_ok = 1;
      break;
    case ERROR_SMIME_CHECK:
      sign_ok = 0;
      break;
    case ERROR_SMIME_COMMAND:
      fclose(description_f);
      res = MAIL_ERROR_COMMAND;
      goto unlink_description;
    case ERROR_SMIME_FILE:
      fclose(description_f);
      res = MAIL_ERROR_FILE;
      goto unlink_description;
    }
    
    if (sign_ok) {
      fprintf(description_f, SMIME_DECRYPT_SUCCESS);
      fclose(description_f);
      break;
    }
    else {
      fclose(description_f);
      unlink(description_filename);
    }
  }
  
  if (!sign_ok) {
    description_f = mailprivacy_get_tmp_file(privacy,
        description_filename,
        sizeof(description_filename));
    if (description_f == NULL) {
      res = MAIL_ERROR_FILE;
      goto unlink_decrypted;
    }
    
    fprintf(description_f, SMIME_DECRYPT_DESCRIPTION);
    fprintf(description_f, SMIME_DECRYPT_FAILED);
    fclose(description_f);
  }
  
  /* building multipart */

  r = mailmime_new_with_content("multipart/x-decrypted", NULL, &multipart);
  if (r != MAILIMF_NO_ERROR) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  /* building the description part */
  
  description_mime = mailprivacy_new_file_part(privacy,
      description_filename,
      "text/plain", MAILMIME_MECHANISM_8BIT);
  if (description_mime == NULL) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  /* adds the description part */
  
  r = mailmime_smart_add_part(multipart, description_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(description_mime);
    mailmime_free(description_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  /* building the decrypted part */
  
  r = mailprivacy_get_part_from_file(privacy, 1,
      decrypted_filename, &decrypted_mime);
  if (r == MAIL_NO_ERROR) {
    /* adds the decrypted part */
    
    r = mailmime_smart_add_part(multipart, decrypted_mime);
    if (r != MAIL_NO_ERROR) {
      mailprivacy_mime_clear(decrypted_mime);
      mailmime_free(decrypted_mime);
      mailprivacy_mime_clear(multipart);
      mailmime_free(multipart);
      res = MAIL_ERROR_MEMORY;
      goto unlink_description;
    }
  }
  
  unlink(description_filename);
  unlink(decrypted_filename);
  unlink(smime_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_description:
  unlink(description_filename);
 unlink_decrypted:
  unlink(decrypted_filename);
 unlink_smime:
  unlink(smime_filename);
 err:
  return res;
}


static int get_cert_from_sig(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime);

#define SMIME_VERIFY_DESCRIPTION "S/MIME verify signed message\r\n"
#define SMIME_VERIFY_FAILED "S/MIME verification FAILED\r\n"
#define SMIME_VERIFY_SUCCESS "S/MIME verification success\r\n"

static int
smime_verify(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime, struct mailmime ** result)
{
  char smime_filename[PATH_MAX];
  char quoted_smime_filename[PATH_MAX];
  int res;
  int r;
  char command[PATH_MAX];
  int sign_ok;
  struct mailmime * description_mime;
  FILE * description_f;
  char description_filename[PATH_MAX];
  struct mailmime * multipart;
  char stripped_filename[PATH_MAX];
  struct mailmime * stripped_mime;
  char quoted_stripped_filename[PATH_MAX];
  FILE * stripped_f;
  char check_CA[PATH_MAX];
  char quoted_CAfile[PATH_MAX];
  char noverify[PATH_MAX];
  
  if (store_cert)
    get_cert_from_sig(privacy, msg, mime);
  
  * check_CA = '\0';
  if (CAfile != NULL) {
    r = mail_quote_filename(quoted_CAfile, sizeof(quoted_CAfile), CAfile);
    if (r < 0) {
      res = MAIL_ERROR_MEMORY;
      goto err;
    }
    
    snprintf(check_CA, sizeof(check_CA), "-CAfile %s", quoted_CAfile);
  }
  
  * noverify = '\0';
  if (!CA_check) {
    snprintf(noverify, sizeof(noverify), "-noverify");
  }
  
  /* fetch the whole multipart and write it to a file */
  
  r = mailprivacy_fetch_mime_body_to_file(privacy,
      smime_filename, sizeof(smime_filename),
      msg, mime);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto err;
  }
  
  stripped_f = mailprivacy_get_tmp_file(privacy,
      stripped_filename,
      sizeof(stripped_filename));
  if (stripped_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_smime;
  }
  fclose(stripped_f);
  
  /* description */
  
  description_f = mailprivacy_get_tmp_file(privacy,
      description_filename,
      sizeof(description_filename));
  if (description_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_stripped;
  }
  
  fprintf(description_f, SMIME_VERIFY_DESCRIPTION);
  
  /* run the command */

  r = mail_quote_filename(quoted_smime_filename,
      sizeof(quoted_smime_filename), smime_filename);
  if (r < 0) {
    fclose(description_f);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }

  r = mail_quote_filename(quoted_stripped_filename,
      sizeof(quoted_stripped_filename), stripped_filename);
  if (r < 0) {
    fclose(description_f);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  sign_ok = 0;
  snprintf(command, PATH_MAX, "openssl smime -verify -in %s -out %s %s %s",
      quoted_smime_filename, quoted_stripped_filename, check_CA, noverify);
  
  r = get_smime_output(description_f, command);
  switch (r) {
  case NO_ERROR_SMIME:
    sign_ok = 1;
    break;
  case ERROR_SMIME_CHECK:
    sign_ok = 0;
    break;
  case ERROR_SMIME_COMMAND:
    fclose(description_f);
    res = MAIL_ERROR_COMMAND;
    goto unlink_description;
  case ERROR_SMIME_FILE:
    fclose(description_f);
    res = MAIL_ERROR_FILE;
    goto unlink_description;
  }
  if (sign_ok)
    fprintf(description_f, SMIME_VERIFY_SUCCESS);
  else
    fprintf(description_f, SMIME_VERIFY_FAILED);
  fclose(description_f);
  
  /* building multipart */
  
  r = mailmime_new_with_content("multipart/x-verified", NULL, &multipart);
  if (r != MAILIMF_NO_ERROR) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  /* building the description part */
  
  description_mime = mailprivacy_new_file_part(privacy,
      description_filename,
      "text/plain", MAILMIME_MECHANISM_8BIT);
  if (description_mime == NULL) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  /* adds the description part */
  
  r = mailmime_smart_add_part(multipart, description_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(description_mime);
    mailmime_free(description_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  r = mailprivacy_get_part_from_file(privacy, 1,
      stripped_filename, &stripped_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = r;
    goto unlink_description;
  }
  
  r = mailmime_smart_add_part(multipart, stripped_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(stripped_mime);
    mailmime_free(stripped_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  unlink(description_filename);
  unlink(stripped_filename);
  unlink(smime_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_description:
  unlink(description_filename);
 unlink_stripped:
  unlink(stripped_filename);
 unlink_smime:
  unlink(smime_filename);
 err:
  return res;
}

static int smime_test_encrypted(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime)
{
  switch (mime->mm_type) {
  case MAILMIME_MULTIPLE:
    return smime_is_signed(mime);
    
  case MAILMIME_SINGLE:
    return smime_is_encrypted(mime);
  }
  
  return 0;
}

static int smime_handler(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime, struct mailmime ** result)
{
  int r;
  struct mailmime * alternative_mime;
  
  alternative_mime = NULL;
  switch (mime->mm_type) {
  case MAILMIME_MULTIPLE:
    r = MAIL_ERROR_INVAL;
    if (smime_is_signed(mime))
      r = smime_verify(privacy, msg, mime, &alternative_mime);
    
    if (r != MAIL_NO_ERROR)
      return r;

    * result = alternative_mime;
    
    return MAIL_NO_ERROR;
    
  case MAILMIME_SINGLE:
    r = MAIL_ERROR_INVAL;
    if (smime_is_encrypted(mime))
      r = smime_decrypt(privacy, msg, mime, &alternative_mime);
    
    if (r != MAIL_NO_ERROR)
      return r;

    * result = alternative_mime;
    
    return MAIL_NO_ERROR;
  }
  
  return MAIL_ERROR_INVAL;
}




static void strip_mime_headers(struct mailmime * mime)
{
  struct mailmime_fields * fields;
  clistiter * cur;
  
  fields = mime->mm_mime_fields;
  
  if (fields == NULL)
    return;
  
  for(cur = clist_begin(fields->fld_list) ; cur != NULL ;
      cur = clist_next(cur)) {
    struct mailmime_field * field;
    
    field = clist_content(cur);
    
    if (field->fld_type == MAILMIME_FIELD_VERSION) {
      mailmime_field_free(field);
      clist_delete(fields->fld_list, cur);
      break;
    }
  }
}

static int smime_sign(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char signed_filename[PATH_MAX];
  FILE * signed_f;
  int res;
  int r;
  int col;
  char signature_filename[PATH_MAX];
  FILE * signature_f;
  char command[PATH_MAX];
  char quoted_signature_filename[PATH_MAX];
  char quoted_signed_filename[PATH_MAX];
  struct mailmime * signed_mime;
  char * smime_cert;
  char * smime_key;
  char quoted_smime_cert[PATH_MAX];
  char quoted_smime_key[PATH_MAX];
  char * email;
  
  /* get signing key */
  
  email = get_first_from_addr(mime);
  if (email == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  smime_key = get_private_key_file(email);
  smime_cert = get_cert_file(email);
  if ((smime_cert == NULL) || (smime_key == NULL)) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  /* part to sign */

  /* encode quoted printable all text parts */
  
  mailprivacy_prepare_mime(mime);
  
  signed_f = mailprivacy_get_tmp_file(privacy,
      signed_filename, sizeof(signed_filename));
  if (signed_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_write(signed_f, &col, mime);
  if (r != MAILIMF_NO_ERROR) {
    fclose(signed_f);
    res = MAIL_ERROR_FILE;
    goto unlink_signed;
  }
  
  fclose(signed_f);
  
  /* prepare destination file for signature */
  
  signature_f = mailprivacy_get_tmp_file(privacy,
      signature_filename,
      sizeof(signature_filename));
  if (signature_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_signed;
  }
  fclose(signature_f);
  
  r = mail_quote_filename(quoted_signed_filename,
       sizeof(quoted_signed_filename), signed_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  r = mail_quote_filename(quoted_signature_filename,
      sizeof(quoted_signature_filename), signature_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  r = mail_quote_filename(quoted_smime_key,
       sizeof(quoted_smime_key), smime_key);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  r = mail_quote_filename(quoted_smime_cert,
       sizeof(quoted_smime_cert), smime_cert);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  snprintf(command, sizeof(command),
      "openssl smime -sign -in %s -out %s -signer %s -inkey %s 2>/dev/null",
      quoted_signed_filename, quoted_signature_filename,
      quoted_smime_cert, quoted_smime_key);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_signature;
  }
  
  /* signature part */
  
  r = mailprivacy_get_part_from_file(privacy, 0,
      signature_filename, &signed_mime);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto unlink_signature;
  }
  strip_mime_headers(signed_mime);
  
  unlink(signature_filename);
  unlink(signed_filename);
  
  * result = signed_mime;
  
  return MAIL_NO_ERROR;
  
 unlink_signature:
  unlink(signature_filename);
 unlink_signed:
  unlink(signed_filename);
 err:
  return res;
}


/* ********************************************************************* */
/* find S/MIME recipient */

static int recipient_add_mb(char * recipient, size_t * len,
    struct mailimf_mailbox * mb)
{
  char * filename;
  char quoted_filename[PATH_MAX];
  size_t buflen;
  int r;
  
  if (mb->mb_addr_spec == NULL)
    return MAIL_NO_ERROR;
  
  filename = get_cert_file(mb->mb_addr_spec);
  if (filename == NULL)
    return MAIL_ERROR_INVAL;
  
  r = mail_quote_filename(quoted_filename, sizeof(quoted_filename),
      filename);
  if (r < 0)
    return MAIL_ERROR_MEMORY;
    
  buflen = strlen(quoted_filename + 1);
  if (buflen >= * len)
    return MAIL_ERROR_MEMORY;
    
  strncat(recipient, quoted_filename, * len);
  (* len) -= buflen;
  strncat(recipient, " ", * len);
  (* len) --;
  
  return MAIL_NO_ERROR;
}

static int recipient_add_mb_list(char * recipient, size_t * len,
    struct mailimf_mailbox_list * mb_list)
{
  clistiter * cur;
  int r;

  for(cur = clist_begin(mb_list->mb_list) ; cur != NULL ;
      cur = clist_next(cur)) {
    struct mailimf_mailbox * mb;

    mb = clist_content(cur);

    r = recipient_add_mb(recipient, len, mb);
    if (r != MAIL_NO_ERROR)
      return r;
  }

  return MAIL_NO_ERROR;
}

static int recipient_add_group(char * recipient, size_t * len,
    struct mailimf_group * group)
{
  return recipient_add_mb_list(recipient, len, group->grp_mb_list);
}

static int recipient_add_addr(char * recipient, size_t * len,
    struct mailimf_address * addr)
{
  int r;

  switch (addr->ad_type) {
  case MAILIMF_ADDRESS_MAILBOX:
    r = recipient_add_mb(recipient, len, addr->ad_data.ad_mailbox);
    break;
  case MAILIMF_ADDRESS_GROUP:
    r = recipient_add_group(recipient, len, addr->ad_data.ad_group);
    break;
  default:
    r = MAIL_ERROR_INVAL;
  }

  return r;
}

static int recipient_add_addr_list(char * recipient, size_t * len,
    struct mailimf_address_list * addr_list)
{
  clistiter * cur;
  int r;

  for(cur = clist_begin(addr_list->ad_list) ; cur != NULL ;
      cur = clist_next(cur)) {
    struct mailimf_address * addr;

    addr = clist_content(cur);

    r = recipient_add_addr(recipient, len, addr);
    if (r != MAIL_NO_ERROR)
      return r;
  }
  
  return MAIL_NO_ERROR;
}

static int collect_smime_cert(char * recipient, size_t size,
    struct mailimf_fields * fields)
{
  struct mailimf_single_fields single_fields;
  int r;
  size_t remaining;
  int res;
  
  * recipient = '\0';
  remaining = size;
  
  if (fields != NULL)
    mailimf_single_fields_init(&single_fields, fields);
  
  if (single_fields.fld_to != NULL) {
    r = recipient_add_addr_list(recipient, &remaining,
        single_fields.fld_to->to_addr_list);
    if (r != MAIL_NO_ERROR) {
      res = r;
      goto err;
    }
  }
  
  if (single_fields.fld_cc != NULL) {
    r = recipient_add_addr_list(recipient, &remaining,
        single_fields.fld_cc->cc_addr_list);
    if (r != MAIL_NO_ERROR) {
      res = r;
      goto err;
    }
  }
  
  if (single_fields.fld_bcc != NULL) {
    if (single_fields.fld_bcc->bcc_addr_list != NULL) {
      r = recipient_add_addr_list(recipient, &remaining,
          single_fields.fld_bcc->bcc_addr_list);
      if (r < 0) {
        res = r;
        goto err;
      }
    }
  }
  
  return MAIL_NO_ERROR;
  
 err:
  return res;
}



static int smime_encrypt(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char encrypted_filename[PATH_MAX];
  FILE * encrypted_f;
  int res;
  int r;
  int col;
  char decrypted_filename[PATH_MAX];
  FILE * decrypted_f;
  char command[PATH_MAX];
  char quoted_decrypted_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  struct mailmime * encrypted_mime;
  struct mailmime * root;
  struct mailimf_fields * fields;
  char recipient[PATH_MAX];
  
  root = mime;
  while (root->mm_parent != NULL)
    root = root->mm_parent;
  
  fields = NULL;
  if (root->mm_type == MAILMIME_MESSAGE)
    fields = root->mm_data.mm_message.mm_fields;

  /* recipient */
  r = collect_smime_cert(recipient, sizeof(recipient), fields);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto err;
  }

  /* part to encrypt */

  /* encode quoted printable all text parts */
  
  mailprivacy_prepare_mime(mime);
  
  decrypted_f = mailprivacy_get_tmp_file(privacy,
      decrypted_filename,
      sizeof(decrypted_filename));
  if (decrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_write(decrypted_f, &col, mime);
  if (r != MAILIMF_NO_ERROR) {
    fclose(decrypted_f);
    res = MAIL_ERROR_FILE;
    goto unlink_decrypted;
  }
  
  fclose(decrypted_f);
  
  /* prepare destination file for encryption */
  
  encrypted_f = mailprivacy_get_tmp_file(privacy,
      encrypted_filename,
      sizeof(encrypted_filename));
  if (encrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_decrypted;
  }
  fclose(encrypted_f);
  
  r = mail_quote_filename(quoted_decrypted_filename,
      sizeof(quoted_decrypted_filename), decrypted_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  r = mail_quote_filename(quoted_encrypted_filename,
       sizeof(quoted_encrypted_filename), encrypted_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  snprintf(command, sizeof(command),
      "openssl smime -encrypt -in %s -out %s %s 2>/dev/null",
      quoted_decrypted_filename, quoted_encrypted_filename, recipient);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_encrypted;
  }
  
  /* encrypted part */
  
  r = mailprivacy_get_part_from_file(privacy, 0,
      encrypted_filename, &encrypted_mime);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto unlink_encrypted;
  }
  strip_mime_headers(encrypted_mime);

  unlink(encrypted_filename);
  unlink(decrypted_filename);
  
  * result = encrypted_mime;
  
  return MAIL_NO_ERROR;
  
 unlink_encrypted:
  unlink(encrypted_filename);
 unlink_decrypted:
  unlink(decrypted_filename);
 err:
  return res;
}


static int smime_sign_encrypt(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char encrypted_filename[PATH_MAX];
  FILE * encrypted_f;
  int res;
  int r;
  int col;
  char signature_filename[PATH_MAX];
  FILE * signature_f;
  char decrypted_filename[PATH_MAX];
  FILE * decrypted_f;
  char command[PATH_MAX];
  char quoted_decrypted_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  char quoted_signature_filename[PATH_MAX];
  struct mailmime * encrypted_mime;
  struct mailmime * root;
  struct mailimf_fields * fields;
  char recipient[PATH_MAX];
  char * smime_cert;
  char * smime_key;
  char quoted_smime_cert[PATH_MAX];
  char quoted_smime_key[PATH_MAX];
  char * email;
  
  root = mime;
  while (root->mm_parent != NULL)
    root = root->mm_parent;
  
  fields = NULL;
  if (root->mm_type == MAILMIME_MESSAGE)
    fields = root->mm_data.mm_message.mm_fields;
  
  /* recipient */
  r = collect_smime_cert(recipient, sizeof(recipient), fields);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto err;
  }

  /* get signing key */
  
  email = get_first_from_addr(mime);
  if (email == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  smime_key = get_private_key_file(email);
  smime_cert = get_cert_file(email);
  if ((smime_cert == NULL) || (smime_key == NULL)) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  /* part to encrypt */

  /* encode quoted printable all text parts */
  
  mailprivacy_prepare_mime(mime);
  
  decrypted_f = mailprivacy_get_tmp_file(privacy,
      decrypted_filename, sizeof(decrypted_filename));
  if (decrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_write(decrypted_f, &col, mime);
  if (r != MAILIMF_NO_ERROR) {
    fclose(decrypted_f);
    res = MAIL_ERROR_FILE;
    goto unlink_decrypted;
  }
  
  fclose(decrypted_f);
  
  /* prepare destination file for signature */
  
  signature_f = mailprivacy_get_tmp_file(privacy,
      signature_filename,
      sizeof(signature_filename));
  if (signature_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_decrypted;
  }
  fclose(signature_f);
  
  r = mail_quote_filename(quoted_decrypted_filename,
      sizeof(quoted_decrypted_filename), decrypted_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  r = mail_quote_filename(quoted_signature_filename,
      sizeof(quoted_signature_filename), signature_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  r = mail_quote_filename(quoted_smime_key,
       sizeof(quoted_smime_key), smime_key);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  r = mail_quote_filename(quoted_smime_cert,
       sizeof(quoted_smime_cert), smime_cert);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  snprintf(command, sizeof(command),
      "openssl smime -sign -in %s -out %s -signer %s -inkey %s 2>/dev/null",
      quoted_decrypted_filename, quoted_signature_filename,
      quoted_smime_cert, quoted_smime_key);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_signature;
  }
  
  
  /* prepare destination file for encryption */
  
  encrypted_f = mailprivacy_get_tmp_file(privacy,
      encrypted_filename,
      sizeof(encrypted_filename));
  if (encrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_signature;
  }
  fclose(encrypted_f);
  
  r = mail_quote_filename(quoted_encrypted_filename,
       sizeof(quoted_encrypted_filename), encrypted_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  snprintf(command, sizeof(command),
      "openssl smime -encrypt -in %s -out %s %s 2>/dev/null",
      quoted_signature_filename, quoted_encrypted_filename, recipient);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_encrypted;
  }
  
  /* encrypted part */
  
  r = mailprivacy_get_part_from_file(privacy, 0,
      encrypted_filename, &encrypted_mime);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto unlink_encrypted;
  }
  strip_mime_headers(encrypted_mime);

  unlink(encrypted_filename);
  unlink(signature_filename);
  unlink(decrypted_filename);
  
  * result = encrypted_mime;
  
  return MAIL_NO_ERROR;
  
 unlink_encrypted:
  unlink(encrypted_filename);
 unlink_signature:
  unlink(signature_filename);
 unlink_decrypted:
  unlink(decrypted_filename);
 err:
  return res;
}



static struct mailprivacy_encryption smime_encryption_tab[] = {
  /* S/MIME signed part */
  {
    .name = "signed",
    .description = "S/MIME signed part",
    .encrypt = smime_sign,
  },
  
  /* S/MIME encrypted part */
  
  {
    .name = "encrypted",
    .description = "S/MIME encrypted part",
    .encrypt = smime_encrypt,
  },

  /* S/MIME signed & encrypted part */
  
  {
    .name = "signed-encrypted",
    .description = "S/MIME signed & encrypted part",
    .encrypt = smime_sign_encrypt,
  },
};

static struct mailprivacy_protocol smime_protocol = {
  .name = "smime",
  .description "S/MIME",
  
  .is_encrypted = smime_test_encrypted,
  .decrypt = smime_handler,
  
  .encryption_count =
  (sizeof(smime_encryption_tab) / sizeof(smime_encryption_tab[0])),
  
  .encryption_tab = smime_encryption_tab,
};

int mailprivacy_smime_init(struct mailprivacy * privacy)
{
  certificates = chash_new(CHASH_DEFAULTSIZE, CHASH_COPYALL);
  if (certificates == NULL)
    goto err;

  private_keys = chash_new(CHASH_DEFAULTSIZE, CHASH_COPYALL);
  if (private_keys == NULL)
    goto free_cert;
  
  CAcert_dir[0] = '\0';
  
  return mailprivacy_register(privacy, &smime_protocol);
  
 free_cert:
  chash_free(certificates);
 err:
  return MAIL_ERROR_MEMORY;
}

void mailprivacy_smime_done(struct mailprivacy * privacy)
{
  mailprivacy_unregister(privacy, &smime_protocol);
  chash_free(private_keys);
  private_keys = NULL;
  chash_free(certificates);
  certificates = NULL;
  if (CAfile != NULL) {
    unlink(CAfile);
    free(CAfile);
  }
  CAfile = NULL;
  CAcert_dir[0] = '\0';
}


static void strip_string(char * str)
{
  char * p;
  size_t len;

  p = strchr(str, '\r');
  if (p != NULL)
    * p = 0;

  p = strchr(str, '\n');
  if (p != NULL)
    * p = 0;

  p = str;
  while ((* p == ' ') || (* p == '\t')) {
    p ++;
  }
  
  len = strlen(p);
  memmove(str, p, len);
  str[len] = 0;
  
  if (len == 0)
    return;
  
  p = str;
  len = len - 1;
  while ((p[len] == ' ') || (p[len] == '\t')) {
    p[len] = '\0';
    
    if (len == 0)
      break;
    
    len --;
  }
}



#define MAX_EMAIL_SIZE 1024

static void set_file(chash * hash, char * email, char * filename)
{
  char * n;
  char buf[MAX_EMAIL_SIZE];
  chashdatum key;
  chashdatum data;

  strncpy(buf, email, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  for(n = buf ; * n != '\0' ; n ++)
    * n = toupper((unsigned char) * n);
  strip_string(buf);
  
  key.data = buf;
  key.len = strlen(buf);
  data.data = filename;
  data.len = strlen(filename) + 1;
  
  chash_set(hash, &key, &data, NULL);
}

static char * get_file(chash * hash, char * email)
{
  chashdatum key;
  chashdatum data;
  char buf[MAX_EMAIL_SIZE];
  char * n;
  int r;
  
  strncpy(buf, email, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  for(n = buf ; * n != '\0' ; n ++)
    * n = toupper((unsigned char) * n);
  
  strip_string(buf);
  key.data = buf;
  key.len = strlen(buf);
  r = chash_get(hash, &key, &data);
  if (r < 0)
    return NULL;
  
  return data.data;
}

void mailprivacy_smime_set_cert_dir(struct mailprivacy * privacy,
    char * directory)
{
  DIR * dir;
  struct dirent * ent;

  chash_clear(certificates);
  
  if (directory == NULL)
    return;
  
  if (* directory == '\0')
    return;

  strncpy(cert_dir, directory, sizeof(cert_dir));
  cert_dir[sizeof(cert_dir) - 1] = '\0';
  
  dir = opendir(directory);
  if (dir == NULL)
    return;
  
  while ((ent = readdir(dir)) != NULL) {
    char filename[PATH_MAX];
    char command[PATH_MAX];
    char buf[MAX_EMAIL_SIZE];
    FILE * p;
    
    snprintf(filename, sizeof(filename),
        "%s/%s", directory, ent->d_name);
    
    snprintf(command, sizeof(command),
        "openssl x509 -email -noout -in %s 2>/dev/null", filename);
    
    p = popen(command, "r");
    if (p == NULL)
      continue;
    
    while (fgets(buf, sizeof(buf), p) != NULL)
      set_file(certificates, buf, filename);
    
    pclose(p);
  }
  closedir(dir);
}

static char * get_cert_file(char * email)
{
  return get_file(certificates, email);
}

static char * get_private_key_file(char * email)
{
  return get_file(private_keys, email);
}

void mail_private_smime_clear_private_keys(struct mailprivacy * privacy)
{
  chash_clear(private_keys);
}

#define MAX_BUF 1024

void mailprivacy_smime_set_CA_dir(struct mailprivacy * privacy,
    char * directory)
{
  DIR * dir;
  struct dirent * ent;
  FILE * f_CA;
  char CA_filename[PATH_MAX];
  
  if (directory == NULL)
    return;
  
  if (* directory == '\0')
    return;
  
  /* make a temporary file that contains all the CAs */
  
  if (CAfile != NULL) {
    unlink(CAfile);
    free(CAfile);
    CAfile = NULL;
  }
  
  f_CA = mailprivacy_get_tmp_file(privacy, CA_filename, sizeof(CA_filename));
  if (f_CA == NULL)
    return;
  
  strncpy(CAcert_dir, directory, sizeof(CAcert_dir));
  CAcert_dir[sizeof(CAcert_dir) - 1] = '\0';
  
  dir = opendir(directory);
  if (dir == NULL) {
    fclose(f_CA);
    goto unlink_CA;
  }
  
  while ((ent = readdir(dir)) != NULL) {
    char filename[PATH_MAX];
    char command[PATH_MAX];
    char buf[MAX_BUF];
    FILE * f;
    
    snprintf(filename, sizeof(filename),
        "%s/%s", directory, ent->d_name);
    
    f = fopen(filename, "r");
    if (f == NULL)
      continue;
    
    while (fgets(buf, sizeof(buf), f) != NULL)
      fputs(buf, f_CA);
    
    fclose(f);
  }
  
  closedir(dir);
  
  fclose(f_CA);
  
  CAfile = strdup(CA_filename);
  if (CAfile == NULL)
    goto unlink_CA;
  
  return;
  
 unlink_CA:
  unlink(CA_filename);
}

void mailprivacy_smime_set_CA_check(struct mailprivacy * privacy,
    int enabled)
{
  CA_check = enabled;
}

void mailprivacy_smime_set_store_cert(struct mailprivacy * privacy,
    int enabled)
{
  store_cert = enabled;
}

static int get_cert_from_sig(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime)
{
  clistiter * cur;
  struct mailmime * signed_mime;
  struct mailmime * signature_mime;
  int res;
  char signature_filename[PATH_MAX];
  char quoted_signature_filename[PATH_MAX];
  char * email;
  char * cert_file;
  char store_cert_filename[PATH_MAX];
  char quoted_store_cert_filename[PATH_MAX];
  int r;
  char command[PATH_MAX];

  if (* cert_dir == '\0')
    return MAIL_ERROR_INVAL;

  if (mime->mm_type != MAILMIME_MULTIPLE)
    return MAIL_ERROR_INVAL;

  email = get_first_from_addr(mime);
  if (email == NULL)
    return MAIL_ERROR_INVAL;
  
  cert_file = get_cert_file(email);
  if (cert_file != NULL)
    return MAIL_NO_ERROR;

  /* get the two parts of the S/MIME message */
  
  cur = clist_begin(mime->mm_data.mm_multipart.mm_mp_list);
  if (cur == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  signed_mime = cur->data;
  cur = clist_next(cur);
  if (cur == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }

  signature_mime = cur->data;
  
  r = mailprivacy_fetch_decoded_to_file(privacy,
      signature_filename, sizeof(signature_filename),
      msg, signature_mime);
  if (r != MAILIMF_NO_ERROR) {
    res = r;
    goto err;
  }
  
  r = mail_quote_filename(quoted_signature_filename,
       sizeof(quoted_signature_filename), signature_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  snprintf(store_cert_filename, sizeof(store_cert_filename),
      "%s/%s-cert.pem", cert_dir, email);
  
  r = mail_quote_filename(quoted_store_cert_filename,
       sizeof(quoted_store_cert_filename), store_cert_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  snprintf(command, sizeof(command),
      "openssl pkcs7 -inform DER -in %s -out %s -print_certs 2>/dev/null",
      quoted_signature_filename, quoted_store_cert_filename);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_signature;
  }

  unlink(signature_filename);
  
  set_file(certificates, email, store_cert_filename);
  
  return MAIL_NO_ERROR;
  
 unlink_signature:
  unlink(signature_filename);
 err:
  return res;
}


static void set_private_key(struct mailprivacy * privacy,
    char * email, char * file)
{
  set_file(private_keys, email, file);
}

#define PRIVATE_KEY_SUFFIX "-private-key.pem"

void mailprivacy_smime_set_private_keys_dir(struct mailprivacy * privacy,
    char * directory)
{
  DIR * dir;
  struct dirent * ent;

  chash_clear(private_keys);
  
  if (directory == NULL)
    return;
  
  if (* directory == '\0')
    return;

  strncpy(private_keys_dir, directory, sizeof(private_keys_dir));
  private_keys_dir[sizeof(private_keys_dir) - 1] = '\0';
  
  dir = opendir(directory);
  if (dir == NULL)
    return;

  while ((ent = readdir(dir)) != NULL) {
    char filename[PATH_MAX];
    char email[PATH_MAX];
    char * p;
    
    snprintf(filename, sizeof(filename),
        "%s/%s", directory, ent->d_name);
    
    strncpy(email, ent->d_name, sizeof(email));
    email[sizeof(email) - 1] = '\0';
    
    p = strstr(email, PRIVATE_KEY_SUFFIX);
    if (p == NULL)
      continue;
    
    if (strlen(p) != sizeof(PRIVATE_KEY_SUFFIX) - 1)
      continue;
    
    * p = 0;
    
    if (* email == '\0')
      continue;
    
    set_private_key(privacy, email, filename);
  }
  closedir(dir);
}
