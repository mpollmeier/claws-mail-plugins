/*
 * libEtPan! -- a mail stuff library
 *
 * Copyright (C) 2001, 2002 - DINH Viet Hoa
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
 * $Id: mailmbox_types.c,v 1.1 2003-10-24 00:42:54 hoa Exp $
 */

#include "mailmbox_types.h"

#include <string.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* *********************************************************************** */

int mailmbox_msg_info_update(struct mailmbox_folder * folder,
			     size_t start, size_t start_len,
			     size_t headers, size_t headers_len,
			     size_t body, size_t body_len,
			     size_t size, size_t padding,
			     uint32_t uid)
{
  struct mailmbox_msg_info * info;
  int res;
  chashdatum key;
  chashdatum data;
  int r;
  
  key.data = (char *) &uid;
  key.len = sizeof(uid);
  r = chash_get(folder->hash, &key, &data);
  if (r < 0) {
    uint32_t index;

    info = mailmbox_msg_info_new(start, start_len, headers, headers_len,
				 body, body_len, size, padding, uid);
    if (info == NULL) {
      res = MAILMBOX_ERROR_MEMORY;
      goto err;
    }

    r = carray_add(folder->tab, info, &index);
    if (r < 0) {
      mailmbox_msg_info_free(info);
      res = MAILMBOX_ERROR_MEMORY;
      goto err;
    }

    if (uid != 0) {
      chashdatum key;
      chashdatum data;
      
      key.data = (char *) &uid;
      key.len = sizeof(uid);
      data.data = (char *) info;
      data.len = 0;
      
      r = chash_set(folder->hash, &key, &data, NULL);
      if (r < 0) {
	mailmbox_msg_info_free(info);
	carray_delete(folder->tab, index);
	res = MAILMBOX_ERROR_MEMORY;
	goto err;
      }
    }
    
    info->index = index;
  }
  else {
    info = (struct mailmbox_msg_info *) data.data;
    
    info->start = start;
    info->start_len = start_len;
    info->headers = headers;
    info->headers_len = headers_len;
    info->body = body;
    info->body_len = body_len;
    info->size = size;
    info->padding = padding;
  }

  return MAILMBOX_NO_ERROR;

 err:
  return res;
}


struct mailmbox_msg_info *
mailmbox_msg_info_new(size_t start, size_t start_len,
		      size_t headers, size_t headers_len,
		      size_t body, size_t body_len,
		      size_t size, size_t padding,
		      uint32_t uid)
{
  struct mailmbox_msg_info * info;

  info = malloc(sizeof(* info));
  if (info == NULL)
    return NULL;

  info->index = 0;
  info->uid = uid;
  if (uid != 0)
    info->written_uid = TRUE;
  else
    info->written_uid = FALSE;
  info->deleted = FALSE;

  info->start = start;
  info->start_len = start_len;

  info->headers = headers;
  info->headers_len = headers_len;

  info->body = body;
  info->body_len = body_len;

  info->size = size;

  info->padding = padding;

  return info;
}

void mailmbox_msg_info_free(struct mailmbox_msg_info * info)
{
  free(info);
}


/* append info */

struct mailmbox_append_info *
mailmbox_append_info_new(char * message, size_t size)
{
  struct mailmbox_append_info * info;

  info = malloc(sizeof(* info));
  if (info == NULL)
    return NULL;

  info->message = message;
  info->size = size;

  return info;
}

void mailmbox_append_info_free(struct mailmbox_append_info * info)
{
  free(info);
}

struct mailmbox_folder * mailmbox_folder_new(char * filename)
{
  struct mailmbox_folder * folder;

  folder = malloc(sizeof(* folder));
  if (folder == NULL)
    goto err;

  strncpy(folder->filename, filename, PATH_MAX);

  folder->mtime = (time_t) -1;

  folder->fd = -1;
  folder->read_only = TRUE;
  folder->no_uid = TRUE;

  folder->changed = FALSE;
  folder->deleted_count = 0;
  
  folder->mapping = NULL;
  folder->mapping_size = 0;

  folder->written_uid = 0;
  folder->max_uid = 0;

  folder->hash = chash_new(CHASH_DEFAULTSIZE, CHASH_COPYKEY);
  if (folder->hash == NULL)
    goto free;
  
  folder->tab = carray_new(128);
  if (folder->tab == NULL)
    goto free_hash;

  return folder;

 free_hash:
  chash_free(folder->hash);
 free:
  free(folder);
 err:
  return NULL;
}

void mailmbox_folder_free(struct mailmbox_folder * folder)
{
  uint32_t i;

  for(i = 0 ; i < folder->tab->len ; i++) {
    struct mailmbox_msg_info * info;

    info = carray_get(folder->tab, i);
    if (info != NULL)
      mailmbox_msg_info_free(info);
  }

  carray_free(folder->tab);
  
  chash_free(folder->hash);

  free(folder);
}
