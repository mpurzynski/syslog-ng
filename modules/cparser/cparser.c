/*
 * Copyright (c) 2017 Balabit
 * Copyright (c) 2017 Gabor Nagy <gabor.nagy@balabit.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 */
#include "cparser.h"
#include "scratch-buffers.h"

#include <string.h>
#include <ctype.h>

typedef struct _CParser
{
  LogParser super;
}CParser;

static gboolean
cparser_process(LogParser *s, LogMessage **pmsg,
                const LogPathOptions *path_options,
                const gchar *input, gsize input_len)
{
  CParser *self = (CParser *) s;
  LogMessage *msg;
  char* name = "foo";
  char* value= "foovalue";
  fprintf(stderr,"******** yippikaye: %s********\n", __func__);

  fprintf(stderr,"input string: %s\n", input);

  msg = log_msg_make_writable(pmsg, path_options);
  log_msg_set_value_by_name(msg, name, value, strlen(value));
  return TRUE;
}

static void
cparser_free(LogPipe *s)
{
  CParser *self = (CParser *) s;
  fprintf(stderr,"******** yippikaye: %s********\n", __func__);

  log_parser_free_method(s);
}

LogParser*
cparser_new(GlobalConfig *cfg)
{
  CParser *self = g_new0(CParser, 1);

  fprintf(stderr,"******** yippikaye: %s********\n", __func__);
  log_parser_init_instance(&self->super, cfg);
  //self->super.super.init = cparser_init;
  //self->super.super.deinit = cparser_deinit;
  self->super.super.free_fn = cparser_free;
  //self->super.super.clone = cparser_clone;
  self->super.process = cparser_process;

  return &self->super;
}
