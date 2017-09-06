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
#include "logmsg/logmsg.h"
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
  app_shutdown();
}

LogPipeCheckpoints *
log_pipe_checkpoints_new(guint number_of_checkpoints)
{
  LogPipeCheckpoints *self = g_new0(LogPipeCheckpoints, 1);
  self->checkpoints = g_new0(LogPipeCheckpoint, number_of_checkpoints);

  for (guint i = 0; i < self->len; i++)
    {
      log_pipe_init_instance(self->checkpoints[i]->super, configuration);
      // void (*queue)(LogPipe *self, LogMessage *msg, const LogPathOptions *path_options, gpointer user_data);
      self->checkpoints[i]->super = checkpointpipe_queue;
    }
  self->len = number_of_checkpoints;
  return self;
}

static void
config_definition_iterator(gpointer key, gpointer value, gpointer user_data)
{
  LogExprNode *rule = (LogExprNode *) value;
  log_expr_node_print_node_info(rule, "(1   ) "); /*FIXME*/
  cr_assert(rule->next == NULL);

  while (rule->children)
    {
      rule = rule->children;
      cr_assert(rule->next == NULL);
      log_expr_node_print_node_info(rule, NULL); /*FIXME*/
    }

  LogExprNode *pipe_expr = log_expr_node_new_pipe(log_pipe_new(configuration), NULL);
  log_expr_node_set_children(rule, pipe_expr);
}

LogPipe *
create_config_element(gchar *config_snippet)
{
  //gboolean result = parse_config(raw_config, LL_CONTEXT_ROOT, NULL, NULL);
  cr_assert(parse_config(config_snippet, LL_CONTEXT_ROOT, NULL, NULL));

  /* we have no drivers in configuration yet as configuration blocks are empty */
  /* add drivers: objects - GHashTable */
  g_hash_table_foreach(configuration->tree.objects, config_definition_iterator, NULL);

  /* TODO: add drivers: rules - GPtrArray */

  /* TODO: change cfg_tree_start/cfg_init to iterate through cfgtree->rules and compile each with compile_node
   * and we will have the logpaths begin./end (except if source drivers used, then we will not have it) */
  cr_assert(cfg_tree_start(&configuration->tree));
  /* TODO: we need to copy the log_pipe_init mechanism */
  /* TODO: LC_CATCHALL log path flag is inside cfg_tree start */

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
send_logmsg_to_pipe_chain(LogPipe *tested_logpath)
{
  gchar *msg_content = "a brave beacon message";
  LogPathOptions path_options = {FALSE, FALSE, NULL };

  LogMessage *msg = log_msg_new_empty();
  log_msg_set_value_by_name(msg, "MESSAGE", msg_content, -1);

  log_pipe_queue(tested_logpath, msg, &path_options);
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
  /* If we want to have config as input for test, then we have to face the problem
   * that we cannot add a general driver to the config.
   * Using existing modules would not be a good way.
   *
    "source s_source { dummy(source); }; \n"
    "log { source(s_source); \n"
    "      parser  { dummy(parser);  }; \n"
    "      rewrite { dummy(rewrite); }; \n"
    "};\n";
  */

  gchar *config =
    "source s_source { }; \n"
    "parser p_parser { }; \n"
    "log { source (s_source); \n"
    "      parser (p_parser); \n"
    "};\n";

  LogPipe *logpath = create_config_element(config);
  logpath_beginning = attach_checkpoint_pipes(logpath, 2);
  send_logmsg_to_pipe_chain(logpath);
  assert_msg_arrived_to_checkpoints();
}

// test log path flags

// test grammar: where is it allowed to use junction/channel.

// test message is intact in(after) channel #2 if LogMessage is modifed on channel #1.

// test message duplication: channel #1 drops message, channel #2 accepts (is it a valid use-case ??)

