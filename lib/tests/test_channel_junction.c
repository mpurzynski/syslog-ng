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
  LP_MSG_MISSED,
  LP_MSG_PASSED,
  LP_MSG_DROP
} LogPipeMsgStatus;

typedef struct
{
  LogPipe super;
  LogPipeMsgStatus status;
  LogPipeMsgStatus expected_status;
} LogCheckpoint;


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

static LogPipe *log_checkpoint_pipe_new(GlobalConfig *cfg, LogPipeMsgStatus expected_status);

static void
log_checkpoint_pipe_queue(LogPipe *s, LogMessage *msg, const LogPathOptions *path_options, gpointer user_data)
{
  LogCheckpoint *self = (LogCheckpoint *)s;
  self->status = LP_MSG_PASSED;
  fprintf(stderr, "*** %s: MESSAGE PASSED !!! in pipe %p\n", __func__, s);
  log_pipe_forward_msg(s, msg, path_options);
}

/* cloning needed the first time when cfg is compiled twice (for LogPipe init functionality) */
static LogPipe *
log_checkpoint_pipe_clone(LogPipe *s)
{
  LogCheckpoint *self = (LogCheckpoint *) s;
  LogCheckpoint *clone = (LogCheckpoint *)log_checkpoint_pipe_new(self->super.cfg, 0);
  clone->status = self->status;
  clone->expected_status = self->expected_status;
  return &clone->super;
}

static LogPipe *
log_checkpoint_pipe_new(GlobalConfig *cfg, LogPipeMsgStatus expected_status)
{
  LogCheckpoint *self = g_new0(LogCheckpoint, 1);
  log_pipe_init_instance(&self->super, cfg);

  self->super.clone = log_checkpoint_pipe_clone;
  self->super.queue = log_checkpoint_pipe_queue;
  self->status = LP_MSG_MISSED;
  self->expected_status = expected_status;

  return &self->super;
}

static LogExprNode *
log_checkpoint_node_new(GlobalConfig *cfg, LogPipeMsgStatus expected_status)
{
  return log_expr_node_new_pipe(log_checkpoint_pipe_new(configuration, expected_status), NULL);
}

static void
config_add_pipe_node_as_children(gpointer key, gpointer value, gpointer user_data)
{
  LogExprNode *rule = (LogExprNode *) value;
  log_expr_node_print_node_info(rule, "parent_rule:"); /*FIXME*/
  cr_assert(rule->next == NULL);

  while (rule->children)
    {
      rule = rule->children;
      cr_assert(rule->next == NULL);
      log_expr_node_print_node_info(rule, "children_rule"); /*FIXME*/
    }

  LogExprNode *checkpoint_node = log_checkpoint_node_new(configuration, LP_MSG_PASSED);
  log_expr_node_set_children(rule, checkpoint_node);
}

LogPipe *
create_config_element(gchar *config_snippet, LogPipe **logpath_tail)
{
  LogPipe *logpath_head = NULL;
  CfgTree *cfg_tree = &configuration->tree;

  cr_assert(parse_config(config_snippet, LL_CONTEXT_ROOT, NULL, NULL));

  /* we have no drivers in configuration yet as configuration blocks are empty */
  /* add drivers: objects - GHashTable */
  g_hash_table_foreach(cfg_tree->objects, config_add_pipe_node_as_children, NULL);

  /* todo: add drivers: rules - GPtrArray */

  /* todo: change cfg_tree_start/cfg_init to iterate through cfgtree->rules and compile each with compile_node
   * and we will have the logpaths begin./end (except if source drivers used, then we will not have the begin.) */
  /* todo: have to save number of log paths = CfgTree->rules->len */
  for (guint i = 0; i < cfg_tree->rules->len; i++)
    {
      LogExprNode *rule = (LogExprNode *) g_ptr_array_index(cfg_tree->rules, i);
      cr_assert(cfg_tree_compile_node(cfg_tree, rule, &logpath_head, logpath_tail));
    };
  cfg_tree->compiled = TRUE;
  cr_assert(cfg_tree_start(cfg_tree)); // need for log_pipe_init phase

  /* todo: we need to copy the log_pipe_init mechanism */
  /* todo: LC_CATCHALL log path flag is inside cfg_tree start */

  // return logpath_head;
  return ((LogPipe *) g_ptr_array_index(cfg_tree->initialized_pipes, 0));
}

void
attach_checkpoint_pipes(LogPipe *logpath_head, LogPipe *logpath_tail)
{
  // TODO: attach to beginning of tested logpath

  // attach to end of tested logpath
  LogCheckpoint *checkpoint_tail = (LogCheckpoint *) log_checkpoint_new(configuration);
  cr_assert(log_pipe_init(&checkpoint_tail->super));
  log_pipe_append(logpath_tail, &checkpoint_tail->super);

  // TODO: attach to given point (junctions!)

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
/*
static void
assert_msg_arrived_to_checkpoints(LogCheckpoints *self, ExpectedStatus *expected_status)
{
  // setup expected values per-channel.
  // If msg is expected to be dropped by channel #1 how to track the status?

  assert(self->checkpoints[i]->status == expected_status[i]);
}
*/
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
  LogPipe *logpath_tail = NULL;
  LogPipe *logpath_head = create_config_element(config, &logpath_tail);
  fprintf(stderr, "*** created log path: head:%p tail:%p\n", logpath_head, logpath_tail);
  attach_checkpoint_pipes(logpath_head, logpath_tail);
  send_logmsg_to_pipe_chain(logpath_head);
  // assert_msg_arrived_to_checkpoints();
}





