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
 * $Id: mailmbox_types.h,v 1.1 2003-10-24 00:42:54 hoa Exp $
 */

#ifndef MAILMBOX_TYPES_H

#define MAILMBOX_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <limits.h>

#include "mailimf.h"
#include "carray.h"
#include "chash.h"

enum {
  MAILMBOX_NO_ERROR = 0,
  MAILMBOX_ERROR_PARSE,
  MAILMBOX_ERROR_INVAL,
  MAILMBOX_ERROR_FILE_NOT_FOUND,
  MAILMBOX_ERROR_MEMORY,
  MAILMBOX_ERROR_TEMPORARY_FILE,
  MAILMBOX_ERROR_FILE,
  MAILMBOX_ERROR_MSG_NOT_FOUND,
  MAILMBOX_ERROR_READONLY,
};


struct mailmbox_folder {
  char filename[PATH_MAX];

  time_t mtime;

  int fd;
  int read_only;
  int no_uid;

  int changed;
  uint32_t deleted_count;
  
  char * mapping;
  size_t mapping_size;

  uint32_t written_uid;
  uint32_t max_uid;

  chash * hash;
  carray * tab;
};

struct mailmbox_folder * mailmbox_folder_new(char * filename);
void mailmbox_folder_free(struct mailmbox_folder * folder);


struct mailmbox_msg_info {
  uint32_t index;
  uint32_t uid;
  int written_uid;
  int deleted;

  size_t start;
  size_t start_len;

  size_t headers;
  size_t headers_len;

  size_t body;
  size_t body_len;

  size_t size;

  size_t padding;
};


int mailmbox_msg_info_update(struct mailmbox_folder * folder,
			     size_t start, size_t start_len,
			     size_t headers, size_t headers_len,
			     size_t body, size_t body_len,
			     size_t size, size_t padding,
			     uint32_t uid);

struct mailmbox_msg_info *
mailmbox_msg_info_new(size_t start, size_t start_len,
		      size_t headers, size_t headers_len,
		      size_t body, size_t body_len,
		      size_t size, size_t padding,
		      uint32_t uid);

void mailmbox_msg_info_free(struct mailmbox_msg_info * info);

struct mailmbox_append_info {
  char * message;
  size_t size;
};

struct mailmbox_append_info *
mailmbox_append_info_new(char * message, size_t size);

void mailmbox_append_info_free(struct mailmbox_append_info * info);

#ifdef __cplusplus
}
#endif

#endif
