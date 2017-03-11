/*
 * Copyright (c) 2017 Balabit
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
 *
 */

#include "cfg.h"
#include "apphook.h"
#include "config_parse_lib.h"
#include "cfg-grammar.h"
#include "plugin.h"
#include "wildcard-source.h"
#include <criterion/criterion.h>

static void
_init(void)
{
  app_startup();
  configuration = cfg_new(VERSION_VALUE);
  cr_assert(plugin_load_module("affile", configuration, NULL));
}

static void
_deinit(void)
{
  cfg_deinit(configuration);
  cfg_free(configuration);
}

static gboolean
_parse_config(const gchar *config)
{
  static gchar raw_config[2048];
  snprintf(raw_config, 2048, "source s_test { wildcard-file(%s); }; log { source(s_test); };", config);
  return parse_config(raw_config, LL_CONTEXT_ROOT, NULL, NULL);
}

static WildcardSourceDriver *
_create_wildcard_filesource(const gchar *wildcard_config)
{
  cr_assert(_parse_config(wildcard_config), "Parsing the given configuration failed");
  cr_assert(cfg_init(configuration), "Config initialization failed");
  LogExprNode *expr_node = cfg_tree_get_object(&configuration->tree, ENC_SOURCE, "s_test");
  cr_assert(expr_node != NULL);
  WildcardSourceDriver *driver = (WildcardSourceDriver *)expr_node->children->children->object;
  cr_assert(driver != NULL);
  return driver;
}

TestSuite(wildcard_source, .init = _init, .fini = _deinit);

Test(wildcard_source, initial_test)
{
  WildcardSourceDriver *driver = _create_wildcard_filesource("base-dir(/test_non_existent_dir)"
                                 "filename-pattern(*.log)"
                                 "recursive(yes)"
                                 "max-files(100)"
                                 "force-directory-polling(no)");
  cr_assert_str_eq(driver->base_dir->str, "/test_non_existent_dir");
  cr_assert_str_eq(driver->filename_pattern->str, "*.log");
  cr_assert_eq(driver->max_files, 100);
  cr_assert_eq(driver->recursive, TRUE);
  cr_assert_eq(driver->force_dir_polling, FALSE);
}

Test(wildcard_source, test_option_inheritance)
{
  WildcardSourceDriver *driver = _create_wildcard_filesource("base-dir(/test_non_existent_dir)"
                                 "filename-pattern(*.log)"
                                 "recursive(yes)"
                                 "max-files(100)"
                                 "force-directory-polling(no)"
                                 "follow-freq(10)"
                                 "follow_freq(10.0)"
                                 "pad_size(5)"
                                 "multi-line-mode(regexp)"
                                 "multi-line-prefix(\\d+)"
                                 "multi-line-garbage(garbage)");
  cr_assert_eq(driver->file_reader_options.follow_freq, 10000);
  cr_assert_eq(driver->file_reader_options.pad_size, 5);
  cr_assert_eq(driver->file_reader_options.multi_line_mode, MLM_PREFIX_GARBAGE);
  cr_assert(driver->file_reader_options.multi_line_prefix != NULL);
  cr_assert(driver->file_reader_options.multi_line_garbage != NULL);
}

Test(wildcard_source, test_option_duplication)
{
  WildcardSourceDriver *driver = _create_wildcard_filesource("base-dir(/tmp)"
                                 "filename-pattern(*.txt)"
                                 "base-dir(/test_non_existent_dir)"
                                 "filename-pattern(*.log)");
  cr_assert_str_eq(driver->base_dir->str, "/test_non_existent_dir");
  cr_assert_str_eq(driver->filename_pattern->str, "*.log");
}

Test(wildcard_source, test_filename_pattern_required_options)
{
  start_grabbing_messages();
  cr_assert(_parse_config("base-dir(/tmp)"));
  cr_assert(!cfg_init(configuration), "Config initialization should be failed");
  stop_grabbing_messages();
  cr_assert(assert_grabbed_messages_contain_non_fatal("filename-pattern option is required", NULL));
  reset_grabbed_messages();
}

Test(wildcard_source, test_base_dir_required_options)
{
  start_grabbing_messages();
  cr_assert(_parse_config("filename-pattern(/tmp)"));
  cr_assert(!cfg_init(configuration), "Config initialization should be failed");
  stop_grabbing_messages();
  cr_assert(assert_grabbed_messages_contain_non_fatal("base-dir option is required", NULL));
  reset_grabbed_messages();
}

Test(wildcard_source, test_minimum_window_size)
{
  WildcardSourceDriver *driver = _create_wildcard_filesource("base-dir(/test_non_existent_dir)"
                                 "filename-pattern(*.log)"
                                 "recursive(yes)"
                                 "max_files(100)"
                                 "log_iw_size(1000)");
  cr_assert_eq(driver->file_reader_options.reader_options.super.init_window_size, MINIMUM_WINDOW_SIZE);
}

Test(wildcard_source, test_window_size)
{

  WildcardSourceDriver *driver = _create_wildcard_filesource("base-dir(/test_non_existent_dir)"
                                 "filename-pattern(*.log)"
                                 "recursive(yes)"
                                 "max_files(10)"
                                 "log_iw_size(10000)");
  cr_assert_eq(driver->file_reader_options.reader_options.super.init_window_size, 1000);
}
