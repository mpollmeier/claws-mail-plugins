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
 * $Id: mailimf_types.c,v 1.1 2003-10-24 00:42:54 hoa Exp $
 */

#include "mailimf_types.h"
#include "mmapstring.h"
#include <stdlib.h>

void mailimf_atom_free(char * atom)
{
  free(atom);
}

void mailimf_dot_atom_free(char * dot_atom)
{
  free(dot_atom);
}

void mailimf_dot_atom_text_free(char * dot_atom)
{
  free(dot_atom);
}

void mailimf_quoted_string_free(char * quoted_string)
{
  free(quoted_string);
}

void mailimf_word_free(char * word)
{
  free(word);
}

void mailimf_phrase_free(char * phrase)
{
  free(phrase);
}

void mailimf_unstructured_free(char * unstructured)
{
  free(unstructured);
}


struct mailimf_date_time *
mailimf_date_time_new(int day, int month, int year,
		      int hour, int min, int sec, int zone)
{
  struct mailimf_date_time * date_time;

  date_time = malloc(sizeof(* date_time));
  if (date_time == NULL)
    return NULL;

  date_time->day = day;
  date_time->month = month;
  date_time->year = year;
  date_time->hour = hour;
  date_time->min = min;
  date_time->sec = sec;
  date_time->zone = zone;

  return date_time;
}


void mailimf_date_time_free(struct mailimf_date_time * date_time)
{
  free(date_time);
}




struct mailimf_address *
mailimf_address_new(int type, struct mailimf_mailbox * mailbox,
		    struct mailimf_group * group)
{
  struct mailimf_address * address;

  address = malloc(sizeof(* address));
  if (address == NULL)
    return NULL;

  address->type = type;
  address->mailbox = mailbox;
  address->group = group;

  return address;
}

void mailimf_address_free(struct mailimf_address * address)
{
  if (address->mailbox != NULL)
    mailimf_mailbox_free(address->mailbox);
  if (address->group != NULL)
    mailimf_group_free(address->group);
  free(address);
}

struct mailimf_mailbox *
mailimf_mailbox_new(char * display_name, char * addr_spec)
{
  struct mailimf_mailbox * mb;

  mb = malloc(sizeof(* mb));
  if (mb == NULL)
    return NULL;

  mb->display_name = display_name;
  mb->addr_spec = addr_spec;

  return mb;
}

void mailimf_mailbox_free(struct mailimf_mailbox * mailbox)
{
  if (mailbox->display_name != NULL)
    mailimf_display_name_free(mailbox->display_name);
  mailimf_addr_spec_free(mailbox->addr_spec);
  free(mailbox);
}


void mailimf_angle_addr_free(char * angle_addr)
{
  free(angle_addr);
}


struct mailimf_group *
mailimf_group_new(char * display_name, struct mailimf_mailbox_list * mb_list)
{
  struct mailimf_group * group;

  group = malloc(sizeof(* group));
  if (group == NULL)
    return NULL;

  group->display_name = display_name;
  group->mb_list = mb_list;
  
  return group;
}

void mailimf_group_free(struct mailimf_group * group)
{
  if (group->mb_list)
    mailimf_mailbox_list_free(group->mb_list);
  mailimf_display_name_free(group->display_name);
  free(group);
}

void mailimf_display_name_free(char * display_name)
{
  mailimf_phrase_free(display_name);
}


struct mailimf_mailbox_list *
mailimf_mailbox_list_new(clist * list)
{
  struct mailimf_mailbox_list * mb_list;

  mb_list = malloc(sizeof(* mb_list));
  if (mb_list == NULL)
    return NULL;

  mb_list->list = list;

  return mb_list;
}

void mailimf_mailbox_list_free(struct mailimf_mailbox_list * mb_list)
{
  clist_foreach(mb_list->list, (clist_func) mailimf_mailbox_free, NULL);
  clist_free(mb_list->list);
  free(mb_list);
}



struct mailimf_address_list *
mailimf_address_list_new(clist * list)
{
  struct mailimf_address_list * addr_list;

  addr_list = malloc(sizeof(* addr_list));
  if (addr_list == NULL)
    return NULL;

  addr_list->list = list;

  return addr_list;
}

void mailimf_address_list_free(struct mailimf_address_list * addr_list)
{
  clist_foreach(addr_list->list, (clist_func) mailimf_address_free, NULL);
  clist_free(addr_list->list);
  free(addr_list);
}


void mailimf_addr_spec_free(char * addr_spec)
{
  free(addr_spec);
}

void mailimf_local_part_free(char * local_part)
{
  free(local_part);
}

void mailimf_domain_free(char * domain)
{
  free(domain);
}

void mailimf_domain_literal_free(char * domain_literal)
{
  free(domain_literal);
}



struct mailimf_message * mailimf_message_new(struct mailimf_fields * fields,
					     struct mailimf_body * body)
{
  struct mailimf_message * message;
  
  message = malloc(sizeof(* message));
  if (message == NULL)
    return NULL;

  message->fields = fields;
  message->body = body;

  return message;
}

void mailimf_message_free(struct mailimf_message * message)
{
  mailimf_body_free(message->body);
  mailimf_fields_free(message->fields);
  free(message);
}


struct mailimf_body * mailimf_body_new(char * text, size_t size)
{
  struct mailimf_body * body;

  body = malloc(sizeof(* body));
  if (body == NULL)
    return NULL;
  body->text = text;
  body->size = size;

  return body;
}

void mailimf_body_free(struct mailimf_body * body)
{
  free(body);
}



struct mailimf_field *
mailimf_field_new(int type,
    struct mailimf_return * return_path,
    struct mailimf_orig_date * resent_date,
    struct mailimf_from * resent_from,
    struct mailimf_sender * resent_sender,
    struct mailimf_to * resent_to,
    struct mailimf_cc * resent_cc,
    struct mailimf_bcc * resent_bcc,
    struct mailimf_message_id * resent_msg_id,
    struct mailimf_orig_date * orig_date,
    struct mailimf_from * from,
    struct mailimf_sender * sender,
    struct mailimf_reply_to * reply_to,
    struct mailimf_to * to,
    struct mailimf_cc * cc,
    struct mailimf_bcc * bcc,
    struct mailimf_message_id * message_id,
    struct mailimf_in_reply_to * in_reply_to,
    struct mailimf_references * references,
    struct mailimf_subject * subject,
    struct mailimf_comments * comments,
    struct mailimf_keywords * keywords,
    struct mailimf_optional_field * optional_field)
{
  struct mailimf_field * field;

  field = malloc(sizeof(* field));
  if (field == NULL)
    return NULL;

  field->type = type;
  switch (type) {
  case MAILIMF_FIELD_RETURN_PATH:
    field->field.return_path = return_path;
    break;
  case MAILIMF_FIELD_RESENT_DATE:
    field->field.resent_date = resent_date;
    break;
  case MAILIMF_FIELD_RESENT_FROM:
    field->field.resent_from = resent_from;
    break;
  case MAILIMF_FIELD_RESENT_SENDER:
    field->field.resent_sender = resent_sender;
    break;
  case MAILIMF_FIELD_RESENT_TO:
    field->field.resent_to = resent_to;
    break;
  case MAILIMF_FIELD_RESENT_CC:
    field->field.resent_cc = resent_cc;
    break;
  case MAILIMF_FIELD_RESENT_BCC:
    field->field.resent_bcc = resent_bcc;
    break;
  case MAILIMF_FIELD_RESENT_MSG_ID:
    field->field.resent_msg_id = resent_msg_id;
    break;
  case MAILIMF_FIELD_ORIG_DATE:
    field->field.orig_date = orig_date;
    break;
  case MAILIMF_FIELD_FROM:
    field->field.from = from;
    break;
  case MAILIMF_FIELD_SENDER:
    field->field.sender = sender;
    break;
  case MAILIMF_FIELD_REPLY_TO:
    field->field.reply_to = reply_to;
    break;
  case MAILIMF_FIELD_TO:
    field->field.to = to;
    break;
  case MAILIMF_FIELD_CC:
    field->field.cc = cc;
    break;
  case MAILIMF_FIELD_BCC:
    field->field.bcc = bcc;
    break;
  case MAILIMF_FIELD_MESSAGE_ID:
    field->field.message_id = message_id;
    break;
  case MAILIMF_FIELD_IN_REPLY_TO:
    field->field.in_reply_to = in_reply_to;
    break;
  case MAILIMF_FIELD_REFERENCES:
    field->field.references = references;
    break;
  case MAILIMF_FIELD_SUBJECT:
    field->field.subject = subject;
    break;
  case MAILIMF_FIELD_COMMENTS:
    field->field.comments = comments;
    break;
  case MAILIMF_FIELD_KEYWORDS:
    field->field.keywords = keywords;
    break;
  case MAILIMF_FIELD_OPTIONAL_FIELD:
    field->field.optional_field = optional_field;
    break;
  }
  
  return field;
}

void mailimf_field_free(struct mailimf_field * field)
{
  switch (field->type) {
  case MAILIMF_FIELD_RETURN_PATH:
    mailimf_return_free(field->field.return_path);
    break;
  case MAILIMF_FIELD_RESENT_DATE:
    mailimf_orig_date_free(field->field.resent_date);
    break;
  case MAILIMF_FIELD_RESENT_FROM:
    mailimf_from_free(field->field.resent_from);
    break;
  case MAILIMF_FIELD_RESENT_SENDER:
    mailimf_sender_free(field->field.resent_sender);
    break;
  case MAILIMF_FIELD_RESENT_TO:
    mailimf_to_free(field->field.resent_to);
    break;
  case MAILIMF_FIELD_RESENT_CC:
    mailimf_cc_free(field->field.resent_cc);
    break;
  case MAILIMF_FIELD_RESENT_BCC:
    mailimf_bcc_free(field->field.resent_bcc);
    break;
  case MAILIMF_FIELD_RESENT_MSG_ID:
    mailimf_message_id_free(field->field.resent_msg_id);
    break;
  case MAILIMF_FIELD_ORIG_DATE:
    mailimf_orig_date_free(field->field.orig_date);
    break;
  case MAILIMF_FIELD_FROM:
    mailimf_from_free(field->field.from);
    break;
  case MAILIMF_FIELD_SENDER:
    mailimf_sender_free(field->field.sender);
    break;
  case MAILIMF_FIELD_REPLY_TO:
    mailimf_reply_to_free(field->field.reply_to);
    break;
  case MAILIMF_FIELD_TO:
    mailimf_to_free(field->field.to);
    break;
  case MAILIMF_FIELD_CC:
    mailimf_cc_free(field->field.cc);
    break;
  case MAILIMF_FIELD_BCC:
    mailimf_bcc_free(field->field.bcc);
    break;
  case MAILIMF_FIELD_MESSAGE_ID:
    mailimf_message_id_free(field->field.message_id);
    break;
  case MAILIMF_FIELD_IN_REPLY_TO:
    mailimf_in_reply_to_free(field->field.in_reply_to);
    break;
  case MAILIMF_FIELD_REFERENCES:
    mailimf_references_free(field->field.references);
    break;
  case MAILIMF_FIELD_SUBJECT:
    mailimf_subject_free(field->field.subject);
    break;
  case MAILIMF_FIELD_COMMENTS:
    mailimf_comments_free(field->field.comments);
    break;
  case MAILIMF_FIELD_KEYWORDS:
    mailimf_keywords_free(field->field.keywords);
    break;
  case MAILIMF_FIELD_OPTIONAL_FIELD:
    mailimf_optional_field_free(field->field.optional_field);
    break;
  }
  
  free(field);
}

struct mailimf_fields * mailimf_fields_new(clist * list)
{
  struct mailimf_fields * fields;

  fields = malloc(sizeof(* fields));
  if (fields == NULL)
    return NULL;

  fields->list = list;

  return fields;
}

void mailimf_fields_free(struct mailimf_fields * fields)
{
  if (fields->list != NULL) {
    clist_foreach(fields->list, (clist_func) mailimf_field_free, NULL);
    clist_free(fields->list);
  }
  free(fields);
}


struct mailimf_orig_date * mailimf_orig_date_new(struct mailimf_date_time *
						 date_time)
{
  struct mailimf_orig_date * orig_date;

  orig_date = malloc(sizeof(* orig_date));
  if (orig_date == NULL)
    return NULL;

  orig_date->date_time = date_time;

  return orig_date;
}

void mailimf_orig_date_free(struct mailimf_orig_date * orig_date)
{
  if (orig_date->date_time != NULL)
    mailimf_date_time_free(orig_date->date_time);
  free(orig_date);
}

struct mailimf_from * mailimf_from_new(struct mailimf_mailbox_list * mb_list)
{
  struct mailimf_from * from;

  from = malloc(sizeof(* from));
  if (from == NULL)
    return NULL;
  
  from->mb_list = mb_list;

  return from;
}

void mailimf_from_free(struct mailimf_from * from)
{
  if (from->mb_list != NULL)
    mailimf_mailbox_list_free(from->mb_list);
  free(from);
}

struct mailimf_sender * mailimf_sender_new(struct mailimf_mailbox * mb)
{
  struct mailimf_sender * sender;

  sender = malloc(sizeof(* mb));
  if (sender == NULL)
    return NULL;

  sender->mb = mb;

  return sender;
}

void mailimf_sender_free(struct mailimf_sender * sender)
{
  if (sender->mb != NULL)
    mailimf_mailbox_free(sender->mb);
  free(sender);
}

struct mailimf_reply_to *
mailimf_reply_to_new(struct mailimf_address_list * addr_list)
{
  struct mailimf_reply_to * reply_to;

  reply_to = malloc(sizeof(* reply_to));
  if (reply_to == NULL)
    return NULL;

  reply_to->addr_list = addr_list;

  return reply_to;
}

void mailimf_reply_to_free(struct mailimf_reply_to * reply_to)
{
  if (reply_to->addr_list != NULL)
    mailimf_address_list_free(reply_to->addr_list);
  free(reply_to);
}

struct mailimf_to * mailimf_to_new(struct mailimf_address_list * addr_list)
{
  struct mailimf_to * to;

  to = malloc(sizeof(* to));
  if (to == NULL)
    return NULL;

  to->addr_list = addr_list;

  return to;
}

void mailimf_to_free(struct mailimf_to * to)
{
  if (to->addr_list != NULL)
    mailimf_address_list_free(to->addr_list);
  free(to);
}

struct mailimf_cc * mailimf_cc_new(struct mailimf_address_list * addr_list)
{
  struct mailimf_cc * cc;

  cc = malloc(sizeof(* cc));
  if (cc == NULL)
    return NULL;

  cc->addr_list = addr_list;

  return cc;
}

void mailimf_cc_free(struct mailimf_cc * cc)
{
  if (cc->addr_list != NULL)
    mailimf_address_list_free(cc->addr_list);
  free(cc);
}

struct mailimf_bcc * mailimf_bcc_new(struct mailimf_address_list * addr_list)
{
  struct mailimf_bcc * bcc;

  bcc = malloc(sizeof(* bcc));
  if (bcc == NULL)
    return NULL;

  bcc->addr_list = addr_list;

  return bcc;
}

void mailimf_bcc_free(struct mailimf_bcc * bcc)
{
  if (bcc->addr_list != NULL)
    mailimf_address_list_free(bcc->addr_list);
  free(bcc);
}

struct mailimf_message_id * mailimf_message_id_new(char * msg_id)
{
  struct mailimf_message_id * message_id;

  message_id = malloc(sizeof(* message_id));
  if (message_id == NULL)
    return NULL;

  message_id->value = msg_id;

  return message_id;
}

void mailimf_message_id_free(struct mailimf_message_id * message_id)
{
  if (message_id->value != NULL)
    mailimf_msg_id_free(message_id->value);
  free(message_id);
}

struct mailimf_in_reply_to * mailimf_in_reply_to_new(clist * msg_id_list)
{
  struct mailimf_in_reply_to * in_reply_to;

  in_reply_to = malloc(sizeof(* in_reply_to));
  if (in_reply_to == NULL)
    return NULL;

  in_reply_to->msg_id_list = msg_id_list;

  return in_reply_to;
}

void mailimf_in_reply_to_free(struct mailimf_in_reply_to * in_reply_to)
{
  clist_foreach(in_reply_to->msg_id_list,
		(clist_func) mailimf_msg_id_free, NULL);
  clist_free(in_reply_to->msg_id_list);
  free(in_reply_to);
}

struct mailimf_references * mailimf_references_new(clist * msg_id_list)
{
  struct mailimf_references * ref;

  ref = malloc(sizeof(* ref));
  if (ref == NULL)
    return NULL;

  ref->msg_id_list = msg_id_list;

  return ref;
}

void mailimf_references_free(struct mailimf_references * references)
{
  clist_foreach(references->msg_id_list,
		(clist_func) mailimf_msg_id_free, NULL);
  clist_free(references->msg_id_list);
  free(references);
}

void mailimf_msg_id_free(char * msg_id)
{
  free(msg_id);
}

void mailimf_id_left_free(char * id_left)
{
  free(id_left);
}

void mailimf_id_right_free(char * id_right)
{
  free(id_right);
}

void mailimf_no_fold_quote_free(char * nfq)
{
  free(nfq);
}

void mailimf_no_fold_literal_free(char * nfl)
{
  free(nfl);
}

struct mailimf_subject * mailimf_subject_new(char * value)
{
  struct mailimf_subject * subject;

  subject = malloc(sizeof(* subject));
  if (subject == NULL)
    return NULL;

  subject->value = value;

  return subject;
}

void mailimf_subject_free(struct mailimf_subject * subject)
{
  mailimf_unstructured_free(subject->value);
  free(subject);
}

struct mailimf_comments * mailimf_comments_new(char * value)
{
  struct mailimf_comments * comments;

  comments = malloc(sizeof(* comments));
  if (comments == NULL)
    return NULL;

  comments->value = value;

  return comments;
}

void mailimf_comments_free(struct mailimf_comments * comments)
{
  mailimf_unstructured_free(comments->value);
  free(comments);
}

struct mailimf_keywords * mailimf_keywords_new(clist * list)
{
  struct mailimf_keywords * keywords;

  keywords = malloc(sizeof(* keywords));
  if (keywords == NULL)
    return NULL;

  keywords->list = list;
  
  return keywords;
}

void mailimf_keywords_free(struct mailimf_keywords * keywords)
{
  clist_foreach(keywords->list, (clist_func) mailimf_phrase_free, NULL);
  clist_free(keywords->list);
  free(keywords);
}

struct mailimf_return *
mailimf_return_new(struct mailimf_path * path)
{
  struct mailimf_return * return_path;

  return_path = malloc(sizeof(* return_path));
  if (return_path == NULL)
    return NULL;

  return_path->path = path;

  return return_path;
}

void mailimf_return_free(struct mailimf_return * return_path)
{
  mailimf_path_free(return_path->path);
  free(return_path);
}


struct mailimf_path * mailimf_path_new(char * addr_spec)
{
  struct mailimf_path * path;

  path = malloc(sizeof(* path));
  if (path == NULL)
    return NULL;

  path->addr_spec = addr_spec;

  return path;
}

void mailimf_path_free(struct mailimf_path * path)
{
  if (path->addr_spec != NULL)
    mailimf_addr_spec_free(path->addr_spec);
  free(path);
}

struct mailimf_optional_field *
mailimf_optional_field_new(char * name, char * value)
{
  struct mailimf_optional_field * opt_field;

  opt_field = malloc(sizeof(* opt_field));
  if (opt_field == NULL)
    return NULL;
  
  opt_field->name = name;
  opt_field->value = value;

  return opt_field;
}

void mailimf_optional_field_free(struct mailimf_optional_field * opt_field)
{
  mailimf_field_name_free(opt_field->name);
  mailimf_unstructured_free(opt_field->value);
  free(opt_field);
}

void mailimf_field_name_free(char * field_name)
{
  free(field_name);
}
