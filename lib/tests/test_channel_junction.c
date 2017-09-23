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

GPtrArray *checkpoints;

static void
_init(void)
{
  app_startup();
  configuration = cfg_new_snippet(VERSION_VALUE);
  checkpoints = g_ptr_array_new();
}

static void
_deinit(void)
{
  g_ptr_array_free(checkpoints, FALSE);
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

  g_ptr_array_add(checkpoints, self);

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

static void
add_drivers_to_empty_cfg_blocks(CfgTree *cfg_tree)
{
  /* we have no drivers in configuration yet as configuration blocks are empty */
  /* add drivers to named cfg objects: CfgTree.objects - GHashTable */
  g_hash_table_foreach(cfg_tree->objects, config_add_pipe_node_as_children, NULL);

  /* todo: add drivers to inline cfg objects: CfgTree.rules - GPtrArray */
}

static void
compile_logexprnodes_into_logpath(CfgTree *cfg_tree, LogPipe **logpath_head, LogPipe **logpath_tail)
{
  /* todo: change cfg_tree_start/cfg_init to iterate through cfgtree->rules and compile each with compile_node
   * and we will have the logpaths begin./end (except if source drivers used, then we will not have the begin.) */
  /* todo: have to save number of log paths = CfgTree->rules->len */
  for (guint i = 0; i < cfg_tree->rules->len; i++)
    {
      LogExprNode *rule = (LogExprNode *) g_ptr_array_index(cfg_tree->rules, i);
      cr_assert(cfg_tree_compile_node(cfg_tree, rule, logpath_head, logpath_tail));
    }

  /* todo logpath flags: LC_CATCHALL log path flag is inside cfg_tree start */

  cfg_tree->compiled = TRUE;
}

static void
initialize_compiled_logpipes(CfgTree *cfg_tree)
{
  cr_assert(cfg_tree_start(cfg_tree)); // need for log_pipe_init phase
  /* todo: need to copy the log_pipe_init mechanism */
}

static CfgTree *
parse_input_for_logpath(gchar *config_snippet)
{
  cr_assert(parse_config(config_snippet, LL_CONTEXT_ROOT, NULL, NULL));
  return &configuration->tree;
}

static gboolean
compile_and_init_logpath(CfgTree *cfg_tree, LogPipe **logpath_head, LogPipe **logpath_tail)
{
  add_drivers_to_empty_cfg_blocks(cfg_tree);
  compile_logexprnodes_into_logpath(cfg_tree, logpath_head, logpath_tail);
  initialize_compiled_logpipes(cfg_tree);

  // cfg_tree_compile overrides logpath_head with NULL if there is a source statement in cfg
  if(*logpath_head == NULL)
    *logpath_head = ((LogPipe *) g_ptr_array_index(cfg_tree->initialized_pipes, 0));

  return TRUE;
}

static void
_check_logpath_is_not_compiled(CfgTree *cfg_tree, gchar *func_name, gchar *checkpoint_target)
{
  cr_assert_not(cfg_tree->compiled, "%s: inserting %s checkpoints into logpath should be done before compile of logpath", func_name, checkpoint_target);
}

static LogExprNode *
find_logexpr_node(LogExprNode *rule, guint node_layout, guint node_content)
{
  ;
}

static gboolean
create_logpath(gchar *config_snippet, LogPipe **logpath_head, LogPipe **logpath_tail)
{
  CfgTree *cfg_tree = parse_input_for_logpath(config_snippet);
  cr_assert(compile_and_init_logpath(cfg_tree, logpath_head, logpath_tail));
  return TRUE;
}

static void
insert_checkpoint_to_beginning(LogPipe *logpath_head, LogPipeMsgStatus expected_status)
{
  LogCheckpoint *checkpoint = (LogCheckpoint *) log_checkpoint_pipe_new(configuration, expected_status);
  cr_assert(log_pipe_init(&checkpoint->super));
  log_pipe_append(&checkpoint->super, logpath_head);
}

static void
insert_checkpoint_to_end(LogPipe *logpath_tail, LogPipeMsgStatus expected_status)
{
  LogCheckpoint *checkpoint = (LogCheckpoint *) log_checkpoint_pipe_new(configuration, expected_status);
  cr_assert(log_pipe_init(&checkpoint->super));
  log_pipe_append(logpath_tail, &checkpoint->super);
}

static void
insert_checkpoint_to_channel_end(CfgTree *cfg_tree, LogPipeMsgStatus expected_status)
{
  _check_logpath_is_not_compiled(cfg_tree, (gchar *)__func__, "channel");

  for (guint i = 0; i < cfg_tree->rules->len; i++)
    {
      LogExprNode *rule = (LogExprNode *) g_ptr_array_index(cfg_tree->rules, i);
      LogExprNode *logexpr = find_logexpr_node(rule, ENL_SEQUENCE, ENC_PIPE);
      if(logexpr)
        {
          LogExprNode *checkpoint_node = log_checkpoint_node_new(configuration, expected_status);
          log_expr_node_append(checkpoint_node, logexpr->next);
          log_expr_node_append(logexpr, checkpoint_node);
          return;
        }
    }
}

static void
insert_checkpoint_to_junction_end(CfgTree *cfg_tree, LogPipeMsgStatus expected_status)
{
  _check_logpath_is_not_compiled(cfg_tree, (gchar *)__func__, "junction");

  for (guint i = 0; i < cfg_tree->rules->len; i++)
    {
      LogExprNode *rule = (LogExprNode *) g_ptr_array_index(cfg_tree->rules, i);
      LogExprNode *logexpr = find_logexpr_node(rule, ENL_JUNCTION, ENC_PIPE);
      if(logexpr)
        {
          LogExprNode *checkpoint_node = log_checkpoint_node_new(configuration, expected_status);
          log_expr_node_append(checkpoint_node, logexpr->next);
          log_expr_node_append(logexpr, checkpoint_node);
          return;
        }
    }
}

static void
send_logmsg_to_logpath(LogPipe *tested_logpath)
{
  gchar *msg_content = "a beacon message";
  LogPathOptions path_options = {FALSE, FALSE, NULL };
  LogMessage *msg = log_msg_new_empty();
  log_msg_set_value_by_name(msg, "MESSAGE", msg_content, -1);

  log_pipe_queue(tested_logpath, msg, &path_options);
}

static void
assert_msg_arrived_to_checkpoints(void)
{
  // If msg is expected to be dropped by channel #1 how to track the status?
  for(guint i=0; i < checkpoints->len; i++)
    {
      LogCheckpoint *checkpoint_pipe  = g_ptr_array_index(checkpoints, i);
      cr_assert(checkpoint_pipe->status == checkpoint_pipe->expected_status);
    }
}

TestSuite(channel_junction, .init = _init, .fini = _deinit);

Test(channel_junction, test_basic_config)
{
  /* If we want to have config as input for test, then we have to face the problem
   * that we cannot add a general driver to the config
   * (unless we create a dummy() driver, which can be source/destination/etc. object).
   * Using existing drivers (e.g. file) would not be a good way.
   *
    "source s_source { dummy(); };
    "log { source(s_source);
    "      parser  { dummy();  };
    "      rewrite { dummy(); };
    "};
  */

  gchar *config =
    "source s_source { }; \n"
    "parser p_parser { }; \n"
    "log { source (s_source); \n"
    "      parser (p_parser); \n"
    "};\n";
  LogPipe *logpath_tail = NULL;
  LogPipe *logpath_head = NULL;

  cr_assert(create_logpath(config, &logpath_head, &logpath_tail));
  fprintf(stderr, "*** created log path: head:%p tail:%p\n", logpath_head, logpath_tail);
  insert_checkpoint_to_beginning(logpath_head, LP_MSG_PASSED);
  insert_checkpoint_to_end(logpath_tail, LP_MSG_PASSED);
  send_logmsg_to_logpath(logpath_head);
  assert_msg_arrived_to_checkpoints();
}

Test(channel_junction, test_channels_in_junction)
{
  gchar *config =
  "log { \n"
  "  junction { \n"
  "    channel { }; \n"
  "    channel { }; \n"
  "  }; \n"
  "}; \n";
  LogPipe *logpath_tail = NULL;
  LogPipe *logpath_head = NULL;

  CfgTree *cfg_tree = parse_input_for_logpath(config);
  insert_checkpoint_to_channel_end(cfg_tree, LP_MSG_PASSED);
  insert_checkpoint_to_channel_end(cfg_tree, LP_MSG_PASSED);
  insert_checkpoint_to_junction_end(cfg_tree, LP_MSG_PASSED);
  cr_assert(compile_and_init_logpath(cfg_tree, &logpath_head, &logpath_tail));
  insert_checkpoint_to_beginning(logpath_head, LP_MSG_PASSED);
  insert_checkpoint_to_end(logpath_tail, LP_MSG_PASSED);

  send_logmsg_to_logpath(logpath_head);
  assert_msg_arrived_to_checkpoints();
}

Test(channel_junction, test_final_flag_in_channel)
{
  gchar *config =
  "log { \n"
  "  junction { \n"
  "    channel { ,flags(final);}; \n"
  "    channel { }; \n"
  "  }; \n"
  "}; \n";
  LogPipe *logpath_tail = NULL;
  LogPipe *logpath_head = NULL;

  CfgTree *cfg_tree = parse_input_for_logpath(config);
  insert_checkpoint_to_channel_end(cfg_tree, LP_MSG_PASSED);
  insert_checkpoint_to_channel_end(cfg_tree, LP_MSG_MISSED);
  insert_checkpoint_to_junction_end(cfg_tree, LP_MSG_PASSED);
  cr_assert(compile_and_init_logpath(cfg_tree, &logpath_head, &logpath_tail));
  insert_checkpoint_to_beginning(logpath_head, LP_MSG_PASSED);
  insert_checkpoint_to_end(logpath_tail, LP_MSG_PASSED);

  send_logmsg_to_logpath(logpath_head);
  assert_msg_arrived_to_checkpoints();
}

