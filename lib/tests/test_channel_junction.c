/*
 * Copyright (c) 2017 Balabit
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "cfg.h"
#include "apphook.h"
#include "config_parse_lib.h"
#include "cfg-grammar.h"
#include "plugin.h"
#include <criterion/criterion.h>
#include <glib/gprintf.h>

typedef enum
{
  LP_MISSED,
  LP_PASSED,
} LogPipeMsgStatus;

typedef struct
{
  LogPipe super;
  LogPipeMsgStatus status;
} LogPipeCheckpoint;

typedef struct
{
  LogPipeCheckpoint *checkpoints;
  guint  len;
} LogPipeCheckpoints;

static void
_init(void)
{
  app_startup();
  configuration = cfg_new_snippet(VERSION_VALUE);
}

static void
_deinit(void)
{
  cfg_deinit(configuration);
  cfg_free(configuration);
}

LogPipeCheckpoints *
log_pipe_checkpoints_new(guint number_of_checkpoints)
{
  LogPipeCheckpoints *self = g_new0(LogPipeCheckpoints, 1);
  self->checkpoints = g_new0(LogPipeCheckpoint, number_of_checkpoints);

  for (guint i = 0; i < self->len; i++)
    log_pipe_init_instance(self->checkpoints[i]->super, configuration);

  self->len = number_of_checkpoints;
  return self;
}

LogPipe *
create_config_element(gchar *config_snippet)
{
  //gboolean result = parse_config(raw_config, LL_CONTEXT_ROOT, NULL, NULL);
  parse_config(config_snippet, LL_CONTEXT_ROOT, NULL, NULL);
  cfg_init(configuration);

  // do we want to test full configs only (i.e. having config objects + log paths),
  // or should it be tested separately (e.g. source statements w channels)?
  // Graph only exists on LogExprNode level before compilation

  return ((LogPipe *) g_ptr_array_index(configuration->tree.initialized_pipes, 0));
}

LogPipe *
attach_checkpoint_pipes(LogPipe *logpath, guint number_of_checkpoints)
{
  LogPipeCheckpoints *check_points = log_pipe_checkpoints_new(number_of_checkpoints);

  // attach to beginning of tested logpath
  log_pipe_append(check_points[0]->super, logpath);

  // attach to end of tested logpath
  LogPipe *pipe = logpath;
  while (pipe->next)
    {
        pipe = pipe->next;
    }
  log_pipe_append(pipe, check_points[1]->super);

  // attach to given point (junctions!)

  return logpath_beginning;
}

static void
send_logmsg_to_pipe_chain(LogMsg *msg, LogPipe *tested_logpath)
{
  log_pipe_queue(tested_logpath, msg, log_path_options);
}

static void
assert_msg_arrived_to_checkpoints(LogPipeCheckpoints *self, ExpectedStatus *expected_status)
{
  // setup expected values per-channel.
  // If msg is expected to be dropped by channel #1 how to track the status?

  assert(self->checkpoints[i]->status == expected_status[i]);
}

TestSuite(channel_junction, .init = _init, .fini = _deinit);

Test(channel_junction, test_proto)
{
  gchar *config = 
  "source s_file { }; log {source(s_file);};";

  LogPipe *logpath = create_config_element(config);
  logpath_beginning = attach_checkpoint_pipes(logpath, 2);
  send_logmsg_to_pipe_chain(msg, logpath_beginning);
  assert_msg_arrived_to_checkpoints();
}

// test log path flags

// test grammar: where is it allowed to use junction/channel.

// test message is intact in(after) channel #2 if LogMessage is modifed on channel #1.

// test message duplication: channel #1 drops message, channel #2 accepts (is it a valid use-case ??)

