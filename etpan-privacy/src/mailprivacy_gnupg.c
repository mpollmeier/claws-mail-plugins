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
 * $Id: mailprivacy_gnupg.c,v 1.1.1.1 2003-12-10 04:31:37 hoa Exp $
 */

#include "mailprivacy_gnupg.h"

#include "mailprivacy.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "mailprivacy_tools.h"
#include <libetpan.h>

static int pgp_is_encrypted(struct mailmime * mime)
{
  if (mime->mm_content_type != NULL) {
    clistiter * cur;
    
    if (strcasecmp(mime->mm_content_type->ct_subtype, "encrypted") != 0)
      return 0;
    
    for(cur = clist_begin(mime->mm_content_type->ct_parameters) ; cur != NULL ;
        cur = clist_next(cur)) {
      struct mailmime_parameter * param;
      
      param = clist_content(cur);
      
      if ((strcasecmp(param->pa_name, "protocol") == 0) &&
          (strcasecmp(param->pa_value, "application/pgp-encrypted") == 0))
        return 1;
    }
  }
  
  return 0;
}

static int pgp_is_signed(struct mailmime * mime)
{
  if (mime->mm_content_type != NULL) {
    clistiter * cur;
    
    if (strcasecmp(mime->mm_content_type->ct_subtype, "signed") != 0)
      return 0;
    
    for(cur = clist_begin(mime->mm_content_type->ct_parameters) ;
        cur != NULL ; cur = clist_next(cur)) {
      struct mailmime_parameter * param;
      
      param = clist_content(cur);
      
      if ((strcasecmp(param->pa_name, "protocol") == 0) &&
          (strcasecmp(param->pa_value, "application/pgp-signature") == 0))
        return 1;
    }
  }
  
  return 0;
}

#define PGP_SIGNED "-----BEGIN PGP SIGNED MESSAGE-----"

int pgp_is_clearsigned(char * data, size_t len)
{
  if (len >= strlen(PGP_SIGNED))
    if (strncmp(data, PGP_SIGNED, sizeof(PGP_SIGNED) - 1) == 0)
      return 1;

  return 0;
}

#define PGP_CRYPTED "-----BEGIN PGP MESSAGE-----"

int pgp_is_crypted_armor(char * data, size_t len)
{
  if (len >= strlen(PGP_CRYPTED))
    if (strncmp(data, PGP_CRYPTED, sizeof(PGP_CRYPTED) - 1) == 0)
      return 1;

  return 0;
}


#define BUF_SIZE 1024

enum {
  NO_ERROR_PGP = 0,
  ERROR_PGP_CHECK,
  ERROR_PGP_COMMAND,
  ERROR_PGP_FILE,
};

/* write output to a file */

static int get_pgp_output(FILE * dest_f, char * command)
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
    res = ERROR_PGP_COMMAND;
    goto err;
  }
  
  while ((size = fread(buf, 1, sizeof(buf), p)) != 0) {
    size_t written;
    
    written = fwrite(buf, 1, size, dest_f);
    if (written != size) {
      res = ERROR_PGP_FILE;
      goto close;
    }
  }
  status = pclose(p);
  
  if (WEXITSTATUS(status) != 0)
    return ERROR_PGP_CHECK;
  else
    return NO_ERROR_PGP;
  
 close:
  pclose(p);
 err:
  return res;
}

#define PGP_DECRYPT_DESCRIPTION "PGP encrypted part\r\n"
#define PGP_DECRYPT_FAILED "PGP decryption FAILED\r\n"
#define PGP_DECRYPT_SUCCESS "PGP decryption success\r\n"

/* extracted from mailprivacy_smime.c -- begin */

static char * get_first_from_addr(struct mailmime * mime)
{
  clistiter * cur;
  struct mailimf_single_fields single_fields;
  struct mailimf_fields * fields;
  struct mailimf_mailbox * mb;
  
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

/* extracted from mailprivacy_smime.c -- end */

static int pgp_decrypt(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime, struct mailmime ** result)
{
  char default_key[PATH_MAX];
  struct mailmime * version_mime;
  struct mailmime * encrypted_mime;
  clistiter * cur;
  char encrypted_filename[PATH_MAX];
  char description_filename[PATH_MAX];
  FILE * description_f;
  char decrypted_filename[PATH_MAX];
  FILE * decrypted_f;
  char command[PATH_MAX];
  struct mailmime * description_mime;
  struct mailmime * decrypted_mime;
  int r;
  int res;
  int decrypt_ok;
  char quoted_decrypted_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  struct mailmime * multipart;
  char * email;
  
  /* get the two parts of the PGP message */
  
  cur = clist_begin(mime->mm_data.mm_multipart.mm_mp_list);
  if (cur == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  version_mime = clist_content(cur);
  cur = clist_next(cur);
  if (cur == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  encrypted_mime = clist_content(cur);
  
  /* fetch the second section, that's the useful one */
  
  r = mailprivacy_fetch_decoded_to_file(privacy,
      encrypted_filename, sizeof(encrypted_filename),
      msg, encrypted_mime);
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
    goto unlink_encrypted;
  }
  fclose(decrypted_f);

  /* description */
  
  description_f = mailprivacy_get_tmp_file(privacy, 
      description_filename,
      sizeof(description_filename));
  if (description_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_decrypted;
  }
  
  fprintf(description_f, PGP_DECRYPT_DESCRIPTION);
  
  /* get encryption key */
  
  * default_key = '\0';
  email = get_first_from_addr(mime);
  if (email != NULL)
    snprintf(default_key, sizeof(default_key),
        "--default-key %s", email);
  
  /* run the command */
  
  r = mail_quote_filename(quoted_encrypted_filename,
       sizeof(quoted_encrypted_filename), encrypted_filename);
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
  
  decrypt_ok = 0;
  snprintf(command, sizeof(command),
      "gpg -q --batch --yes --out %s %s --decrypt %s",
      quoted_decrypted_filename, default_key, quoted_encrypted_filename);
  
  r = get_pgp_output(description_f, command);
  switch (r) {
  case NO_ERROR_PGP:
    decrypt_ok = 1;
    break;
  case ERROR_PGP_CHECK:
    decrypt_ok = 0;
    break;
  case ERROR_PGP_COMMAND:
    fclose(description_f);
    res = MAIL_ERROR_COMMAND;
    goto unlink_description;
  case ERROR_PGP_FILE:
    fclose(description_f);
    res = MAIL_ERROR_FILE;
    goto unlink_description;
  }
  if (decrypt_ok)
    fprintf(description_f, PGP_DECRYPT_SUCCESS);
  else
    fprintf(description_f, PGP_DECRYPT_FAILED);
  fclose(description_f);
  
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
  unlink(encrypted_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_description:
  unlink(description_filename);
 unlink_decrypted:
  unlink(decrypted_filename);
 unlink_encrypted:
  unlink(encrypted_filename);
 err:
  return res;
}

#define PGP_VERIFY_DESCRIPTION "PGP verify signed message\r\n"
#define PGP_VERIFY_FAILED "PGP verification FAILED\r\n"
#define PGP_VERIFY_SUCCESS "PGP verification success\r\n"

static int
pgp_verify(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime, struct mailmime ** result)
{
  struct mailmime * signed_mime;
  struct mailmime * signature_mime;
  char signed_filename[PATH_MAX];
  char signature_filename[PATH_MAX];
  int res;
  int r;
  clistiter * cur;
  char command[PATH_MAX];
  int sign_ok;
  struct mailmime * description_mime;
  FILE * description_f;
  char description_filename[PATH_MAX];
  char quoted_signed_filename[PATH_MAX];
  char quoted_signature_filename[PATH_MAX];
  struct mailmime * multipart;
  struct mailmime * signed_msg_mime;
  
  /* get the two parts of the PGP message */
  
  cur = clist_begin(mime->mm_data.mm_multipart.mm_mp_list);
  if (cur == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  signed_mime = clist_content(cur);
  cur = clist_next(cur);
  if (cur == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  signature_mime = clist_content(cur);
  
  /* fetch signed part and write it to a file */
  
  r = mailprivacy_fetch_mime_body_to_file(privacy,
      signed_filename, sizeof(signed_filename),
      msg, signed_mime);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto err;
  }
  
  /* fetch signed part and write it to a file */

  r = mailprivacy_fetch_decoded_to_file(privacy,
      signature_filename, sizeof(signature_filename),
      msg, signature_mime);
  if (r != MAIL_NO_ERROR) {
    res = r;
    goto unlink_signed;
  }
  
  /* description */
  
  description_f = mailprivacy_get_tmp_file(privacy,
      description_filename,
      sizeof(description_filename));
  if (description_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_signature;
  }
  
  fprintf(description_f, PGP_VERIFY_DESCRIPTION);
  
  /* run the command */

  r = mail_quote_filename(quoted_signature_filename,
      sizeof(quoted_signature_filename), signature_filename);
  if (r < 0) {
    fclose(description_f);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  r = mail_quote_filename(quoted_signed_filename,
      sizeof(quoted_signed_filename), signed_filename);
  if (r < 0) {
    fclose(description_f);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  sign_ok = 0;
  snprintf(command, sizeof(command), "gpg -q --batch --yes --verify %s %s",
      quoted_signature_filename, quoted_signed_filename);

  r = get_pgp_output(description_f, command);
  switch (r) {
  case NO_ERROR_PGP:
    sign_ok = 1;
    break;
  case ERROR_PGP_CHECK:
    sign_ok = 0;
    break;
  case ERROR_PGP_COMMAND:
    fclose(description_f);
    res = MAIL_ERROR_COMMAND;
    goto unlink_description;
  case ERROR_PGP_FILE:
    fclose(description_f);
    res = MAIL_ERROR_FILE;
    goto unlink_description;
  }
  
  if (sign_ok)
    fprintf(description_f, PGP_VERIFY_SUCCESS);
  else
    fprintf(description_f, PGP_VERIFY_FAILED);
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
      signed_filename, &signed_msg_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  r = mailmime_smart_add_part(multipart, signed_msg_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(signed_msg_mime);
    mailmime_free(signed_msg_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  unlink(description_filename);
  unlink(signature_filename);
  unlink(signed_filename);

  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_description:
  unlink(description_filename);
 unlink_signature:
  unlink(signature_filename);
 unlink_signed:
  unlink(signed_filename);
 err:
  return res;
}


#define PGP_CLEAR_VERIFY_DESCRIPTION "PGP verify clear signed message\r\n"
#define PGP_CLEAR_VERIFY_FAILED "PGP verification FAILED\r\n"
#define PGP_CLEAR_VERIFY_SUCCESS "PGP verification success\r\n"

static int pgp_verify_clearsigned(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime,
    char * content, size_t content_len, struct mailmime ** result)
{
  int r;
  char command[PATH_MAX];
  int res;
  int sign_ok;
  size_t written;
  char signed_filename[PATH_MAX];
  FILE * signed_f;
  char stripped_filename[PATH_MAX];
  FILE * stripped_f;
  char description_filename[PATH_MAX];
  FILE * description_f;
  char quoted_signed_filename[PATH_MAX];
  char quoted_stripped_filename[PATH_MAX];
  struct mailmime * stripped_mime;
  struct mailmime * description_mime;
  struct mailmime * multipart;
  struct mailmime_content * content_type;
  
  if (mime->mm_parent == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }

  if (mime->mm_parent->mm_type == MAILMIME_SINGLE) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  signed_f = mailprivacy_get_tmp_file(privacy,
      signed_filename, sizeof(signed_filename));
  if (signed_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  written = fwrite(content, 1, content_len, signed_f);
  if (written != content_len) {
    fclose(signed_f);
    unlink(signed_filename);
    res = MAIL_ERROR_FILE;
    goto err;
  }
  fclose(signed_f);

  /* XXX - prepare file for PGP, remove trailing WS */
  
  stripped_f = mailprivacy_get_tmp_file(privacy,
      stripped_filename, sizeof(stripped_filename));
  if (stripped_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_signed;
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
  
  fprintf(description_f, PGP_CLEAR_VERIFY_DESCRIPTION);
  
  r = mail_quote_filename(quoted_stripped_filename,
      sizeof(quoted_stripped_filename), stripped_filename);
  if (r < 0) {
    fclose(description_f);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  r = mail_quote_filename(quoted_signed_filename,
      sizeof(quoted_signed_filename), signed_filename);
  if (r < 0) {
    fclose(description_f);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  snprintf(command, sizeof(command),
      "gpg -q --batch --yes --out %s --decrypt %s",
      quoted_stripped_filename, quoted_signed_filename);
  
  sign_ok = 0;
  r = get_pgp_output(description_f, command);
  switch (r) {
  case NO_ERROR_PGP:
    sign_ok = 1;
    break;
  case ERROR_PGP_CHECK:
    sign_ok = 0;
    break;
  case ERROR_PGP_COMMAND:
    res = MAIL_ERROR_COMMAND;
    fclose(description_f);
    goto unlink_description;
  case ERROR_PGP_FILE:
    res = MAIL_ERROR_FILE;
    fclose(description_f);
    goto unlink_description;
  }
  if (sign_ok)
    fprintf(description_f, PGP_CLEAR_VERIFY_SUCCESS);
  else
    fprintf(description_f, PGP_CLEAR_VERIFY_FAILED);
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
  
  /* building the signature stripped part */
  
  stripped_mime = mailprivacy_new_file_part(privacy,
      stripped_filename,
      "application/octet-stream",
      MAILMIME_MECHANISM_8BIT);
  if (stripped_mime == NULL) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  /* place original content type */
  
  content_type = mailmime_content_dup(mime->mm_content_type);
  if (content_type == NULL) {
    mailprivacy_mime_clear(stripped_mime);
    mailmime_free(stripped_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_description;
  }
  
  mailmime_content_free(stripped_mime->mm_content_type);
  stripped_mime->mm_content_type = content_type;

  /* place original MIME fields */
  
  if (mime->mm_mime_fields != NULL) {
    struct mailmime_fields * mime_fields;
    clistiter * cur;

    mime_fields = mailprivacy_mime_fields_dup(privacy, mime->mm_mime_fields);
    if (mime_fields == NULL) {
      mailprivacy_mime_clear(stripped_mime);
      mailmime_free(stripped_mime);
      mailprivacy_mime_clear(multipart);
      mailmime_free(multipart);
      res = MAIL_ERROR_MEMORY;
      goto unlink_description;
    }
    for(cur = clist_begin(mime_fields->fld_list) ;
        cur != NULL ; cur = clist_next(cur)) {
      struct mailmime_field * field;
      
      field = clist_content(cur);
      if (field->fld_type == MAILMIME_FIELD_TRANSFER_ENCODING) {
        mailmime_field_free(field);
        clist_delete(mime_fields->fld_list, cur);
        break;
      }
    }
    clist_concat(stripped_mime->mm_mime_fields->fld_list,
        mime_fields->fld_list);
    mailmime_fields_free(mime_fields);
  }
  
  /* adds the stripped part */
  
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
  unlink(signed_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_description:
  unlink(description_filename);
 unlink_stripped:
  unlink(stripped_filename);
 unlink_signed:
  unlink(signed_filename);
 err:
  return res;
}


#define PGP_DECRYPT_ARMOR_DESCRIPTION "PGP ASCII armor encrypted part\r\n"
#define PGP_DECRYPT_ARMOR_FAILED "PGP ASCII armor decryption FAILED\r\n"
#define PGP_DECRYPT_ARMOR_SUCCESS "PGP ASCII armor decryption success\r\n"

static int pgp_decrypt_armor(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime,
    char * content, size_t content_len, struct mailmime ** result)
{
  char default_key[PATH_MAX];
  FILE * encrypted_f;
  char encrypted_filename[PATH_MAX];
  char description_filename[PATH_MAX];
  FILE * description_f;
  char decrypted_filename[PATH_MAX];
  FILE * decrypted_f;
  size_t written;
  char command[PATH_MAX];
  struct mailmime * description_mime;
  struct mailmime * decrypted_mime;
  struct mailmime * multipart;
  int r;
  int res;
  int sign_ok;
  char quoted_decrypted_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  char * email;

  if (mime->mm_parent == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }

  if (mime->mm_parent->mm_type == MAILMIME_SINGLE) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  encrypted_f = mailprivacy_get_tmp_file(privacy,
      encrypted_filename,
      sizeof(encrypted_filename));
  if (encrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  written = fwrite(content, 1, content_len, encrypted_f);
  if (written != content_len) {
    fclose(encrypted_f);
    unlink(encrypted_filename);
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  fclose(encrypted_f);
  
  /* we are in a safe directory */
  
  decrypted_f = mailprivacy_get_tmp_file(privacy,
      decrypted_filename,
      sizeof(decrypted_filename));
  if (decrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_encrypted;
  }
  fclose(decrypted_f);

  /* description */
  
  description_f = mailprivacy_get_tmp_file(privacy,
      description_filename,
      sizeof(description_filename));
  if (description_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_decrypted;
  }
  
  fprintf(description_f, PGP_DECRYPT_ARMOR_DESCRIPTION);
  
  /* get encryption key */
  
  * default_key = '\0';
  email = get_first_from_addr(mime);
  if (email != NULL)
    snprintf(default_key, sizeof(default_key),
        "--default-key %s", email);
  
  /* run the command */
  
  r = mail_quote_filename(quoted_encrypted_filename,
       sizeof(quoted_encrypted_filename), encrypted_filename);
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
  snprintf(command, sizeof(command),
      "gpg -q --batch --yes --out %s %s --decrypt %s",
      quoted_decrypted_filename, default_key, quoted_encrypted_filename);
  
  r = get_pgp_output(description_f, command);
  switch (r) {
  case NO_ERROR_PGP:
    sign_ok = 1;
    break;
  case ERROR_PGP_CHECK:
    sign_ok = 0;
    break;
  case ERROR_PGP_COMMAND:
    fclose(description_f);
    res = MAIL_ERROR_COMMAND;
    goto unlink_description;
  case ERROR_PGP_FILE:
    fclose(description_f);
    res = MAIL_ERROR_FILE;
    goto unlink_description;
  }
  if (sign_ok)
    fprintf(description_f, PGP_DECRYPT_ARMOR_SUCCESS);
  else
    fprintf(description_f, PGP_DECRYPT_ARMOR_FAILED);
  fclose(description_f);
  
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
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = r;
    goto unlink_description;
  }
  
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
  
  unlink(description_filename);
  unlink(decrypted_filename);
  unlink(encrypted_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_description:
  unlink(description_filename);
 unlink_decrypted:
  unlink(decrypted_filename);
 unlink_encrypted:
  unlink(encrypted_filename);
 err:
  return res;
}


static int mime_is_text(struct mailmime * build_info)
{
  if (build_info->mm_type == MAILMIME_SINGLE) {
    if (build_info->mm_content_type != NULL) {
      if (build_info->mm_content_type->ct_type->tp_type ==
          MAILMIME_TYPE_DISCRETE_TYPE) {
        if (build_info->mm_content_type->ct_type->tp_data.tp_discrete_type->dt_type ==
            MAILMIME_DISCRETE_TYPE_TEXT)
          return TRUE;
      }
    }
    else
      return TRUE;
  }

  return FALSE;
}


static int pgp_test_encrypted(struct mailprivacy * privacy,
    mailmessage * msg, struct mailmime * mime)
{
  int r;
  int res;

  switch (mime->mm_type) {
  case MAILMIME_MULTIPLE:
    return (pgp_is_encrypted(mime) || pgp_is_signed(mime));
    
  case MAILMIME_SINGLE:
    /* clear sign or ASCII armor encryption */
    if (mime_is_text(mime)) {
      char * content;
      size_t content_len;
      char * parsed_content;
      size_t parsed_content_len;
      size_t cur_token;
      int encoding;
      struct mailmime_single_fields single_fields;
      
      r = mailprivacy_msg_fetch_section(privacy, msg, mime,
          &content, &content_len);
      if (r != MAIL_NO_ERROR)
        return 0;
      
      mailmime_single_fields_init(&single_fields, mime->mm_mime_fields,
          mime->mm_content_type);
      if (single_fields.fld_encoding != NULL)
        encoding = single_fields.fld_encoding->enc_type;
      else
        encoding = MAILMIME_MECHANISM_8BIT;
      
      cur_token = 0;
      r = mailmime_part_parse(content, content_len, &cur_token,
          encoding, &parsed_content, &parsed_content_len);
      mailprivacy_msg_fetch_result_free(privacy, msg, content);
      
      if (r != MAILIMF_NO_ERROR)
        return 0;
      
      res = 0;
      
      if (pgp_is_clearsigned(parsed_content, parsed_content_len))
        res = 1;
      else if (pgp_is_crypted_armor(parsed_content, parsed_content_len))
        res = 1;
      
      mmap_string_unref(parsed_content);
      
      return res;
    }
    break;
  }
  
  return 0;
}

static int pgp_handler(struct mailprivacy * privacy,
    mailmessage * msg,
    struct mailmime * mime, struct mailmime ** result)
{
  int r;
  struct mailmime * alternative_mime;
  
  alternative_mime = NULL;
  switch (mime->mm_type) {
  case MAILMIME_MULTIPLE:
    r = MAIL_ERROR_INVAL;
    if (pgp_is_encrypted(mime)) {
      r = pgp_decrypt(privacy, msg, mime, &alternative_mime);
    }
    else if (pgp_is_signed(mime)) {
      r = pgp_verify(privacy, msg, mime, &alternative_mime);
    }
    
    if (r != MAIL_NO_ERROR)
      return r;

    * result = alternative_mime;
    
    return MAIL_NO_ERROR;
    
  case MAILMIME_SINGLE:
    /* clear sign or ASCII armor encryption */
    if (mime_is_text(mime)) {
      char * content;
      size_t content_len;
      char * parsed_content;
      size_t parsed_content_len;
      size_t cur_token;
      int encoding;
      struct mailmime_single_fields single_fields;
      
      r = mailprivacy_msg_fetch_section(privacy, msg, mime,
          &content, &content_len);
      if (r != MAIL_NO_ERROR)
        return MAIL_ERROR_FETCH;
      
      mailmime_single_fields_init(&single_fields, mime->mm_mime_fields,
          mime->mm_content_type);
      if (single_fields.fld_encoding != NULL)
        encoding = single_fields.fld_encoding->enc_type;
      else
        encoding = MAILMIME_MECHANISM_8BIT;
      
      cur_token = 0;
      r = mailmime_part_parse(content, content_len, &cur_token,
          encoding, &parsed_content, &parsed_content_len);
      mailprivacy_msg_fetch_result_free(privacy, msg, content);
      
      if (r != MAILIMF_NO_ERROR)
        return MAIL_ERROR_PARSE;
      
      r = MAIL_ERROR_INVAL;
      if (pgp_is_clearsigned(parsed_content,
              parsed_content_len)) {
        r = pgp_verify_clearsigned(privacy,
            msg, mime, parsed_content, parsed_content_len, &alternative_mime);
      }
      else if (pgp_is_crypted_armor(parsed_content,
                   parsed_content_len)) {
        r = pgp_decrypt_armor(privacy,
            msg, mime, parsed_content, parsed_content_len, &alternative_mime);
      }
      
      mmap_string_unref(parsed_content);
      
      if (r != MAIL_NO_ERROR)
        return r;

      * result = alternative_mime;
      
      return MAIL_NO_ERROR;
    }
    break;
  }
  
  return MAIL_ERROR_INVAL;
}


#if 0
static void prepare_mime_single(struct mailmime * mime)
{
  struct mailmime_single_fields single_fields;
  int encoding;
  int r;
  
  if (mime->mime_fields != NULL) {
    mailmime_single_fields_init(&single_fields, mime->mime_fields,
        mime->content_type);
    if (single_fields.encoding != NULL) {
      encoding = single_fields.encoding->type;
      switch (encoding) {
      case MAILMIME_MECHANISM_8BIT:
      case MAILMIME_MECHANISM_7BIT:
      case MAILMIME_MECHANISM_BINARY:
        single_fields.encoding->type = MAILMIME_MECHANISM_QUOTED_PRINTABLE;
        break;
      }
    }
    else {
      struct mailmime_mechanism * mechanism;
      struct mailmime_field * field;
      
      mechanism =
        mailmime_mechanism_new(MAILMIME_MECHANISM_QUOTED_PRINTABLE, NULL);
      if (mechanism == NULL)
        return;
      
      field = mailmime_field_new(MAILMIME_FIELD_TRANSFER_ENCODING,
          NULL, mechanism, NULL, NULL, 0, NULL, NULL);
      if (field == NULL) {
        mailmime_mechanism_free(mechanism);
        return;
      }
      
      r = clist_append(mime->mime_fields->list, field);
      if (r < 0) {
        mailmime_field_free(field);
        return;
      }
    }
  }
      
  switch (mime->body->encoding) {
  case MAILMIME_MECHANISM_8BIT:
  case MAILMIME_MECHANISM_7BIT:
  case MAILMIME_MECHANISM_BINARY:
    mime->body->encoding = MAILMIME_MECHANISM_QUOTED_PRINTABLE;
    mime->body->encoded = 0;
    break;
  }
}

/*
  prepare_mime()
  
  we assume we built ourself the message.
*/

static void prepare_mime(struct mailmime * mime)
{
  clistiter * cur;
  
  switch (mime->type) {
  case MAILMIME_SINGLE:
    if (mime->body != NULL) {
      prepare_mime_single(mime);
    }
    break;
    
  case MAILMIME_MULTIPLE:
    for(cur = clist_begin(mime->list) ; cur != NULL ; cur = clist_next(cur)) {
      struct mailmime * child;
      
      child = cur->data;
      
      prepare_mime(child);
    }
    break;
    
  case MAILMIME_MESSAGE:
    if (mime->msg_mime) {
      prepare_mime(mime->msg_mime);
    }
    break;
  }
}
#endif

static int pgp_sign_mime(struct mailprivacy * privacy,
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
  char default_key[PATH_MAX];
  struct mailmime * signature_mime;
  struct mailmime * multipart;
  struct mailmime_content * content;
  struct mailmime_parameter * param;
  struct mailmime * signed_msg_mime;
  char * dup_signature_filename;
  char * email;
  
  /* get signing key */
  
  * default_key = '\0';
  email = get_first_from_addr(mime);
  if (email != NULL)
    snprintf(default_key, sizeof(default_key),
        "--default-key %s", email);
  
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
  
  snprintf(command, sizeof(command),
      "gpg -q -a --batch --yes --digest-algo sha1 --out %s %s -b %s 2>/dev/null",
      quoted_signature_filename, default_key, quoted_signed_filename);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_signature;
  }
  
  /* multipart */
  
  multipart = mailprivacy_new_file_part(privacy, NULL,
      "multipart/signed", -1);
  
  content = multipart->mm_content_type;
  
  param = mailmime_param_new_with_data("micalg", "pgp-sha1");
  if (param == NULL) {
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  r = clist_append(content->ct_parameters, param);
  if (r < 0) {
    mailmime_parameter_free(param);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  param = mailmime_param_new_with_data("protocol",
      "application/pgp-signature");
  if (param == NULL) {
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  r = clist_append(content->ct_parameters, param);
  if (r < 0) {
    mailmime_parameter_free(param);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  /* signed part */

  r = mailprivacy_get_part_from_file(privacy, 1,
      signed_filename, &signed_msg_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = r;
    goto unlink_signature;
  }
  
  mailprivacy_prepare_mime(signed_msg_mime);
  
  r = mailmime_smart_add_part(multipart, signed_msg_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(signed_msg_mime);
    mailmime_free(signed_msg_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  /* signature part */
  
  /* reencode the signature file with CRLF */
  dup_signature_filename = mailprivacy_dup_imf_file(privacy,
      signature_filename);
  if (dup_signature_filename == NULL) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_FILE;
    goto unlink_signature;
  }
  
  /* replace the signature file */
  unlink(signature_filename);
  strncpy(signature_filename,
      dup_signature_filename, sizeof(signature_filename));
  signature_filename[sizeof(signature_filename) - 1] = '\0';
  
  signature_mime = mailprivacy_new_file_part(privacy,
      signature_filename,
      "application/pgp-signature",
      MAILMIME_MECHANISM_8BIT);
  if (signature_mime == NULL) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }
  
  r = mailmime_smart_add_part(multipart, signature_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(signature_mime);
    mailmime_free(signature_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signature;
  }

  unlink(signature_filename);
  unlink(signed_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_signature:
  unlink(signature_filename);
 unlink_signed:
  unlink(signed_filename);
 err:
  return res;
}


/* ********************************************************************* */
/* find GPG recipient */

static int recipient_add_mb(char * recipient, size_t * len,
    struct mailimf_mailbox * mb)
{
  char buffer[PATH_MAX];
  size_t buflen;

  if (mb->mb_addr_spec != NULL) {
    snprintf(buffer, sizeof(buffer), "-r %s ", mb->mb_addr_spec);
    buflen = strlen(buffer);
    if (buflen >= * len)
      return MAIL_ERROR_MEMORY;

    strncat(recipient, buffer, * len);
    (* len) -= buflen;
  }

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


static int collect_recipient(char * recipient, size_t size,
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
      if (r != MAIL_NO_ERROR) {
        res = r;
        goto err;
      }
    }
  }
  
  return MAIL_NO_ERROR;
  
 err:
  return res;
}


#define PGP_VERSION "Version: 1\r\n"

static int pgp_sign_encrypt_mime(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char original_filename[PATH_MAX];
  FILE * original_f;
  int res;
  int r;
  int col;
  char encrypted_filename[PATH_MAX];
  FILE * encrypted_f;
  char version_filename[PATH_MAX];
  FILE * version_f;
  char command[PATH_MAX];
  char quoted_original_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  char default_key[PATH_MAX];
  struct mailmime * version_mime;
  struct mailmime * multipart;
  struct mailmime_content * content;
  struct mailmime_parameter * param;
  struct mailmime * encrypted_mime;
  char recipient[PATH_MAX];
  struct mailimf_fields * fields;
  struct mailmime * root;
  size_t written;
  char * email;
  
  root = mime;
  while (root->mm_parent != NULL)
    root = root->mm_parent;
  
  fields = NULL;
  if (root->mm_type == MAILMIME_MESSAGE)
    fields = root->mm_data.mm_message.mm_fields;
  
  /* recipient */
  
  collect_recipient(recipient, sizeof(recipient), fields);
  
  /* get signing key */
  
  * default_key = '\0';
  email = get_first_from_addr(mime);
  if (email != NULL)
    snprintf(default_key, sizeof(default_key),
        "--default-key %s", email);
  
  /* part to encrypt */
  
  /* encode quoted printable all text parts */
  
  mailprivacy_prepare_mime(mime);
  
  original_f = mailprivacy_get_tmp_file(privacy, original_filename,
      sizeof(original_filename));
  if (original_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_write(original_f, &col, mime);
  if (r != MAILIMF_NO_ERROR) {
    fclose(original_f);
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  
  fclose(original_f);
  
  /* prepare destination file for encryption */
  
  encrypted_f = mailprivacy_get_tmp_file(privacy,
      encrypted_filename,
      sizeof(encrypted_filename));
  if (encrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(encrypted_f);
  
  r = mail_quote_filename(quoted_original_filename,
       sizeof(quoted_original_filename), original_filename);
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
      "gpg -q %s -a --batch --yes --digest-algo sha1 --out %s -s %s -e %s 2>/dev/null",
      recipient, quoted_encrypted_filename, default_key,
      quoted_original_filename);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_encrypted;
  }
  
  /* multipart */
  
  multipart = mailprivacy_new_file_part(privacy, NULL,
      "multipart/encrypted", -1);
  
  content = multipart->mm_content_type;
  
  param = mailmime_param_new_with_data("protocol",
      "application/pgp-encrypted");
  if (param == NULL) {
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  r = clist_append(content->ct_parameters, param);
  if (r < 0) {
    mailmime_parameter_free(param);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }

  /* version part */
  
  version_f = mailprivacy_get_tmp_file(privacy,
      version_filename,
      sizeof(version_filename));
  if (version_f == NULL) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_FILE;
    goto unlink_encrypted;
  }
  written = fwrite(PGP_VERSION, 1, sizeof(PGP_VERSION) - 1, version_f);
  if (written != sizeof(PGP_VERSION) - 1) {
    fclose(version_f);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_FILE;
    goto unlink_encrypted;
  }
  fclose(version_f);
  
  version_mime = mailprivacy_new_file_part(privacy,
      version_filename,
      "application/pgp-encrypted",
      MAILMIME_MECHANISM_8BIT);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = r;
    goto unlink_version;
  }

  r = mailmime_smart_add_part(multipart, version_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(version_mime);
    mailmime_free(version_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_version;
  }
  
  /* encrypted part */
  
  encrypted_mime = mailprivacy_new_file_part(privacy,
      encrypted_filename,
      "application/octet-stream",
      MAILMIME_MECHANISM_8BIT);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = r;
    goto unlink_version;
  }
  
  r = mailmime_smart_add_part(multipart, encrypted_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(encrypted_mime);
    mailmime_free(encrypted_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_version;
  }
  
  unlink(version_filename);
  unlink(encrypted_filename);
  unlink(original_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_version:
  unlink(version_filename);
 unlink_encrypted:
  unlink(encrypted_filename);
 unlink_original:
  unlink(original_filename);
 err:
  return res;
}


static int pgp_encrypt_mime(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char original_filename[PATH_MAX];
  FILE * original_f;
  int res;
  int r;
  int col;
  char encrypted_filename[PATH_MAX];
  FILE * encrypted_f;
  char version_filename[PATH_MAX];
  FILE * version_f;
  char command[PATH_MAX];
  char quoted_original_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  struct mailmime * version_mime;
  struct mailmime * multipart;
  struct mailmime_content * content;
  struct mailmime_parameter * param;
  struct mailmime * encrypted_mime;
  char recipient[PATH_MAX];
  struct mailimf_fields * fields;
  struct mailmime * root;
  size_t written;

  root = mime;
  while (root->mm_parent != NULL)
    root = root->mm_parent;
  
  fields = NULL;
  if (root->mm_type == MAILMIME_MESSAGE)
    fields = root->mm_data.mm_message.mm_fields;
  
  /* recipient */
  
  collect_recipient(recipient, sizeof(recipient), fields);
  
  /* part to encrypt */
  
  /* encode quoted printable all text parts */
  
  mailprivacy_prepare_mime(mime);
  
  original_f = mailprivacy_get_tmp_file(privacy,
      original_filename, sizeof(original_filename));
  if (original_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_write(original_f, &col, mime);
  if (r != MAILIMF_NO_ERROR) {
    fclose(original_f);
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  
  fclose(original_f);
  
  /* prepare destination file for encryption */
  
  encrypted_f = mailprivacy_get_tmp_file(privacy,
      encrypted_filename,
      sizeof(encrypted_filename));
  if (encrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(encrypted_f);
  
  r = mail_quote_filename(quoted_original_filename,
       sizeof(quoted_original_filename), original_filename);
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
      "gpg -q %s -a --batch --yes --out %s -e %s 2>/dev/null",
      recipient, quoted_encrypted_filename, quoted_original_filename);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_encrypted;
  }
  
  /* multipart */
  
  multipart = mailprivacy_new_file_part(privacy, NULL,
      "multipart/encrypted", -1);
  
  content = multipart->mm_content_type;
  
  param = mailmime_param_new_with_data("protocol",
      "application/pgp-encrypted");
  if (param == NULL) {
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  r = clist_append(content->ct_parameters, param);
  if (r < 0) {
    mailmime_parameter_free(param);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }

  /* version part */
  
  version_f = mailprivacy_get_tmp_file(privacy,
      version_filename, sizeof(version_filename));
  if (version_f == NULL) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_FILE;
    goto unlink_encrypted;
  }
  written = fwrite(PGP_VERSION, 1, sizeof(PGP_VERSION) - 1, version_f);
  if (written != sizeof(PGP_VERSION) - 1) {
    fclose(version_f);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_FILE;
    goto unlink_encrypted;
  }
  fclose(version_f);
  
  version_mime = mailprivacy_new_file_part(privacy,
      version_filename,
      "application/pgp-encrypted",
      MAILMIME_MECHANISM_8BIT);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = r;
    goto unlink_version;
  }

  r = mailmime_smart_add_part(multipart, version_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(version_mime);
    mailmime_free(version_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_version;
  }
  
  /* encrypted part */
  
  encrypted_mime = mailprivacy_new_file_part(privacy,
      encrypted_filename,
      "application/octet-stream",
      MAILMIME_MECHANISM_8BIT);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = r;
    goto unlink_version;
  }
  
  r = mailmime_smart_add_part(multipart, encrypted_mime);
  if (r != MAIL_NO_ERROR) {
    mailprivacy_mime_clear(encrypted_mime);
    mailmime_free(encrypted_mime);
    mailprivacy_mime_clear(multipart);
    mailmime_free(multipart);
    res = MAIL_ERROR_MEMORY;
    goto unlink_version;
  }
  
  unlink(version_filename);
  unlink(encrypted_filename);
  unlink(original_filename);
  
  * result = multipart;
  
  return MAIL_NO_ERROR;
  
 unlink_version:
  unlink(version_filename);
 unlink_encrypted:
  unlink(encrypted_filename);
 unlink_original:
  unlink(original_filename);
 err:
  return res;
}

static int pgp_clear_sign(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char default_key[PATH_MAX];
  char original_filename[PATH_MAX];
  FILE * original_f;
  char signed_filename[PATH_MAX];
  FILE * signed_f;
  char quoted_original_filename[PATH_MAX];
  char quoted_signed_filename[PATH_MAX];
  int col;
  struct mailmime * signed_mime;
  int res;
  int r;
  char command[PATH_MAX];
  struct mailmime_content * content_type;
  char * email;
  
  if (mime->mm_type != MAILMIME_SINGLE) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  if (mime->mm_data.mm_single == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  /* get signing key */
  
  * default_key = '\0';
  email = get_first_from_addr(mime);
  if (email != NULL)
    snprintf(default_key, sizeof(default_key),
        "--default-key %s", email);
  
  /* get part to sign */
  
  original_f = mailprivacy_get_tmp_file(privacy,
      original_filename,
      sizeof(original_filename));
  if (original_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_data_write(original_f, &col, mime->mm_data.mm_single, 1);
  if (r != MAILIMF_NO_ERROR) {
    fclose(original_f);
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(original_f);

  signed_f = mailprivacy_get_tmp_file(privacy,
      signed_filename,
      sizeof(signed_filename));
  if (signed_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(signed_f);

  r = mail_quote_filename(quoted_original_filename,
      sizeof(quoted_original_filename), original_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signed;
  }

  r = mail_quote_filename(quoted_signed_filename,
      sizeof(quoted_signed_filename), signed_filename);
  if (r < 0) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signed;
  }
  
  snprintf(command, sizeof(command),
      "gpg -q --batch --yes --digest-algo sha1 --out %s %s --clearsign %s 2>/dev/null",
      quoted_signed_filename, default_key, quoted_original_filename);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_signed;
  }

  /* building the signed part */
  
  signed_mime = mailprivacy_new_file_part(privacy, signed_filename,
      NULL, MAILMIME_MECHANISM_8BIT);
  if (signed_mime == NULL) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_signed;
  }
  
  /* place original content type */
  
  content_type = mailmime_content_dup(mime->mm_content_type);
  if (content_type == NULL) {
    mailprivacy_mime_clear(signed_mime);
    mailmime_free(signed_mime);
    res = MAIL_ERROR_MEMORY;
    goto unlink_signed;
  }
  
  mailmime_content_free(signed_mime->mm_content_type);
  signed_mime->mm_content_type = content_type;

  /* place original MIME fields */
  
  if (mime->mm_mime_fields != NULL) {
    struct mailmime_fields * mime_fields;
    clistiter * cur;
    
    mime_fields = mailprivacy_mime_fields_dup(privacy,
        mime->mm_mime_fields);
    if (mime_fields == NULL) {
      mailprivacy_mime_clear(signed_mime);
      mailmime_free(signed_mime);
      res = MAIL_ERROR_MEMORY;
      goto unlink_signed;
    }
    for(cur = clist_begin(mime_fields->fld_list) ;
        cur != NULL ; cur = clist_next(cur)) {
      struct mailmime_field * field;
      
      field = clist_content(cur);
      if (field->fld_type == MAILMIME_FIELD_TRANSFER_ENCODING) {
        mailmime_field_free(field);
        clist_delete(mime_fields->fld_list, cur);
        break;
      }
    }
    clist_concat(signed_mime->mm_mime_fields->fld_list,
        mime_fields->fld_list);
    mailmime_fields_free(mime_fields);
  }
  
  unlink(signed_filename);
  unlink(original_filename);
  
  * result = signed_mime;
  
  return MAIL_NO_ERROR;
  
 unlink_signed:
  unlink(signed_filename);
 unlink_original:
  unlink(original_filename);
 err:
  return res;
}


static int pgp_armor_encrypt(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char original_filename[PATH_MAX];
  FILE * original_f;
  char encrypted_filename[PATH_MAX];
  FILE * encrypted_f;
  char quoted_original_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  int col;
  struct mailmime * encrypted_mime;
  int res;
  int r;
  char command[PATH_MAX];
  struct mailmime_content * content_type;
  struct mailmime * root;
  struct mailimf_fields * fields;
  char recipient[PATH_MAX];

  if (mime->mm_type != MAILMIME_SINGLE) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  if (mime->mm_data.mm_single == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  root = mime;
  while (root->mm_parent != NULL)
    root = root->mm_parent;
  
  fields = NULL;
  if (root->mm_type == MAILMIME_MESSAGE)
    fields = root->mm_data.mm_message.mm_fields;
  
  /* recipient */
  
  collect_recipient(recipient, sizeof(recipient), fields);
  
  /* get part to encrypt */
  
  original_f = mailprivacy_get_tmp_file(privacy, original_filename,
      sizeof(original_filename));
  if (original_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_data_write(original_f, &col, mime->mm_data.mm_single, 1);
  if (r != MAILIMF_NO_ERROR) {
    fclose(original_f);
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(original_f);

  encrypted_f = mailprivacy_get_tmp_file(privacy,
      encrypted_filename,
      sizeof(encrypted_filename));
  if (encrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(encrypted_f);

  r = mail_quote_filename(quoted_original_filename,
      sizeof(quoted_original_filename), original_filename);
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
      "gpg -q %s --batch --yes --out %s -e --armor %s 2>/dev/null",
      recipient, quoted_encrypted_filename, quoted_original_filename);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_encrypted;
  }
  
  /* building the encrypted part */
  
  encrypted_mime = mailprivacy_new_file_part(privacy,
      encrypted_filename,
      "application/octet-stream",
      MAILMIME_MECHANISM_8BIT);
  if (encrypted_mime == NULL) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  /* place original content type */
  
  content_type = mailmime_content_dup(mime->mm_content_type);
  if (content_type == NULL) {
    mailprivacy_mime_clear(encrypted_mime);
    mailmime_free(encrypted_mime);
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  mailmime_content_free(encrypted_mime->mm_content_type);
  encrypted_mime->mm_content_type = content_type;

  /* place original MIME fields */
  
  if (mime->mm_mime_fields != NULL) {
    struct mailmime_fields * mime_fields;
    clistiter * cur;
    
    mime_fields = mailprivacy_mime_fields_dup(privacy, mime->mm_mime_fields);
    if (mime_fields == NULL) {
      mailprivacy_mime_clear(encrypted_mime);
      mailmime_free(encrypted_mime);
      res = MAIL_ERROR_MEMORY;
      goto unlink_encrypted;
    }
    for(cur = clist_begin(mime_fields->fld_list) ;
        cur != NULL ; cur = clist_next(cur)) {
      struct mailmime_field * field;
      
      field = clist_content(cur);
      if (field->fld_type == MAILMIME_FIELD_TRANSFER_ENCODING) {
        mailmime_field_free(field);
        clist_delete(mime_fields->fld_list, cur);
        break;
      }
    }
    clist_concat(encrypted_mime->mm_mime_fields->fld_list,
        mime_fields->fld_list);
    mailmime_fields_free(mime_fields);
  }

  unlink(encrypted_filename);
  unlink(original_filename);
  
  * result = encrypted_mime;
  
  return MAIL_NO_ERROR;
  
 unlink_encrypted:
  unlink(encrypted_filename);
 unlink_original:
  unlink(original_filename);
 err:
  return res;
}


static int pgp_armor_sign_encrypt(struct mailprivacy * privacy,
    struct mailmime * mime, struct mailmime ** result)
{
  char default_key[PATH_MAX];
  char original_filename[PATH_MAX];
  FILE * original_f;
  char encrypted_filename[PATH_MAX];
  FILE * encrypted_f;
  char quoted_original_filename[PATH_MAX];
  char quoted_encrypted_filename[PATH_MAX];
  int col;
  struct mailmime * encrypted_mime;
  int res;
  int r;
  char command[PATH_MAX];
  struct mailmime_content * content_type;
  struct mailmime * root;
  struct mailimf_fields * fields;
  char recipient[PATH_MAX];
  char * email;
  
  if (mime->mm_type != MAILMIME_SINGLE) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  if (mime->mm_data.mm_single == NULL) {
    res = MAIL_ERROR_INVAL;
    goto err;
  }
  
  /* get signing key */
  
  * default_key = '\0';
  email = get_first_from_addr(mime);
  if (email != NULL)
    snprintf(default_key, sizeof(default_key),
        "--default-key %s", email);
  
  root = mime;
  while (root->mm_parent != NULL)
    root = root->mm_parent;
  
  fields = NULL;
  if (root->mm_type == MAILMIME_MESSAGE)
    fields = root->mm_data.mm_message.mm_fields;
  
  /* recipient */
  
  collect_recipient(recipient, sizeof(recipient), fields);
  
  /* get part to encrypt */
  
  original_f = mailprivacy_get_tmp_file(privacy,
      original_filename,
      sizeof(original_filename));
  if (original_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto err;
  }
  
  col = 0;
  r = mailmime_data_write(original_f, &col, mime->mm_data.mm_single, 1);
  if (r != MAILIMF_NO_ERROR) {
    fclose(original_f);
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(original_f);

  encrypted_f = mailprivacy_get_tmp_file(privacy,
      encrypted_filename,
      sizeof(encrypted_filename));
  if (encrypted_f == NULL) {
    res = MAIL_ERROR_FILE;
    goto unlink_original;
  }
  fclose(encrypted_f);

  r = mail_quote_filename(quoted_original_filename,
      sizeof(quoted_original_filename), original_filename);
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
      "gpg -q %s --batch --yes --digest-algo sha1 "
      "--out %s %s -e -s -a %s 2>/dev/null",
      recipient, encrypted_filename, default_key, original_filename);
  
  r = system(command);
  if (WEXITSTATUS(r) != 0) {
    res = MAIL_ERROR_COMMAND;
    goto unlink_encrypted;
  }
  
  /* building the encrypted part */
  
  encrypted_mime = mailprivacy_new_file_part(privacy,
      encrypted_filename,
      "application/octet-stream",
      MAILMIME_MECHANISM_8BIT);
  if (encrypted_mime == NULL) {
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  /* place original content type */
  
  content_type = mailmime_content_dup(mime->mm_content_type);
  if (content_type == NULL) {
    mailprivacy_mime_clear(encrypted_mime);
    mailmime_free(encrypted_mime);
    res = MAIL_ERROR_MEMORY;
    goto unlink_encrypted;
  }
  
  mailmime_content_free(encrypted_mime->mm_content_type);
  encrypted_mime->mm_content_type = content_type;

  /* place original MIME fields */
  
  if (mime->mm_mime_fields != NULL) {
    struct mailmime_fields * mime_fields;
    clistiter * cur;
    
    mime_fields = mailprivacy_mime_fields_dup(privacy, mime->mm_mime_fields);
    if (mime_fields == NULL) {
      mailprivacy_mime_clear(encrypted_mime);
      mailmime_free(encrypted_mime);
      res = MAIL_ERROR_MEMORY;
      goto unlink_encrypted;
    }
    for(cur = clist_begin(mime_fields->fld_list) ;
        cur != NULL ; cur = clist_next(cur)) {
      struct mailmime_field * field;
      
      field = clist_content(cur);
      if (field->fld_type == MAILMIME_FIELD_TRANSFER_ENCODING) {
        mailmime_field_free(field);
        clist_delete(mime_fields->fld_list, cur);
        break;
      }
    }
    clist_concat(encrypted_mime->mm_mime_fields->fld_list,
        mime_fields->fld_list);
    mailmime_fields_free(mime_fields);
  }

  unlink(encrypted_filename);
  unlink(original_filename);
  
  * result = encrypted_mime;
  
  return MAIL_NO_ERROR;
  
 unlink_encrypted:
  unlink(encrypted_filename);
 unlink_original:
  unlink(original_filename);
 err:
  return res;
}


static struct mailprivacy_encryption pgp_encryption_tab[] = {
  /* PGP signed part */
  {
    .name = "signed",
    .description = "PGP signed part",
    .encrypt = pgp_sign_mime,
  },
  
  /* PGP encrypted part */
  
  {
    .name = "encrypted",
    .description = "PGP encrypted part",
    .encrypt = pgp_encrypt_mime,
  },
  
  /* PGP signed & encrypted part */
  
  {
    .name = "signed-encrypted",
    .description = "PGP signed & encrypted part",
    .encrypt = pgp_sign_encrypt_mime,
  },

  /* PGP clear signed part */
  
  {
    .name = "clear-signed",
    .description = "PGP clear signed part",
    .encrypt = pgp_clear_sign,
  },

  /* PGP armor encrypted part */
  
  {
    .name = "encrypted-armor",
    .description = "PGP ASCII armor encrypted part",
    .encrypt = pgp_armor_encrypt,
  },

  /* PGP armor signed & encrypted part */
  
  {
    .name = "signed-encrypted-armor",
    .description = "PGP ASCII armor signed & encrypted part",
    .encrypt = pgp_armor_sign_encrypt,
  },
};

static struct mailprivacy_protocol pgp_protocol = {
  .name = "pgp",
  .description "OpenPGP",
  
  .is_encrypted = pgp_test_encrypted,
  .decrypt = pgp_handler,
  
  .encryption_count =
  (sizeof(pgp_encryption_tab) / sizeof(pgp_encryption_tab[0])),
  
  .encryption_tab = pgp_encryption_tab,
};

int mailprivacy_gnupg_init(struct mailprivacy * privacy)
{
  return mailprivacy_register(privacy, &pgp_protocol);
}

void mailprivacy_gnupg_done(struct mailprivacy * privacy)
{
  mailprivacy_unregister(privacy, &pgp_protocol);
}
