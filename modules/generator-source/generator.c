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

#include "generator.h"
#include "messages.h"
#include "logsource.h"
#include "template/templates.h"
#include <string.h>
#include <iv.h>

typedef struct
{
  LogSourceOptions super;
  guint generating_periodicity;
} GeneratorSourceOptions;

typedef struct
{
  LogSource super;
  struct iv_timer generator_timer;
  GeneratorSourceOptions *options;
  LogTemplate *template;
} GeneratorSource;

typedef struct
{
  LogSrcDriver super;
  LogSource *generator;
  GeneratorSourceOptions gsource_options;
} GeneratorSrcDriver;

void
generator_source_options_set_freq(LogSourceOptions *o, guint freq_value)
{
  GeneratorSourceOptions *self  = (GeneratorSourceOptions *)o;
  self->generating_periodicity = freq_value;
}

LogSourceOptions *
get_generator_options(LogDriver *s)
{
  GeneratorSrcDriver *self = (GeneratorSrcDriver *)s;
  return &self->gsource_options.super;
}

void
generator_source_options_defaults(GeneratorSourceOptions *self)
{
  log_source_options_defaults(&self->super);
  self->generating_periodicity = 2;
}

void
generator_source_options_init(GeneratorSourceOptions *self, GlobalConfig *cfg, const gchar *group_name)
{
  log_source_options_init(&self->super, cfg, group_name);
}

void
generator_source_options_destroy(GeneratorSourceOptions *self)
{
  // free dynamic atrributes in GeneratorSourceOptions: no dynamic attr.
  log_source_options_destroy(&self->super);
}

static void
generator_timer_update_and_trigger(GeneratorSource *self)
{
  if (iv_timer_registered(&self->generator_timer))
    {
      msg_warning("Generator timer is already registered! Unregistering...");
      iv_timer_unregister(&self->generator_timer);
    }

  iv_validate_now();
  self->generator_timer.expires = iv_now;
  self->generator_timer.expires.tv_sec += self->options->generating_periodicity;
  iv_timer_register(&self->generator_timer);
}

static void
generator_timer_handler(void *cookie)
{
  GeneratorSource *self = (GeneratorSource *) cookie;
  LogMessage *msg = log_msg_new_empty();
  GString *template_result = g_string_new("");
  log_msg_set_value(msg, LM_V_HOST, "template-host", -1);

  log_template_format(self->template, msg, NULL, LTZ_LOCAL, 1, NULL, template_result);
  log_msg_set_value(msg, LM_V_MESSAGE, template_result->str, -1);
  g_string_free(template_result, TRUE);

  log_source_post(&self->super, msg);
  generator_timer_update_and_trigger(self);
}

static void
generator_timer_init(GeneratorSource *self)
{
  IV_TIMER_INIT(&self->generator_timer);
  self->generator_timer.cookie = self;
  self->generator_timer.handler = generator_timer_handler;
}

static void
generator_source_init_template(LogSource *s)
{
  GeneratorSource *self = (GeneratorSource *)s;
  GlobalConfig *cfg = log_pipe_get_config(&self->super.super);
  GError *error = NULL;

  self->template = log_template_new(cfg, "generator");
  gboolean success = log_template_compile(self->template, "this is the message host=$HOST pid=${PID}\n", &error);
  if(success)
    fprintf(stderr, "*** %s: compiled the template! \n", __func__);
}


void
generator_source_set_options(LogSource *s, GeneratorSourceOptions *options,
                             const gchar *stats_id, const gchar *stats_instance,
                             gboolean threaded, gboolean pos_tracked, LogExprNode *expr_node)
{
  GeneratorSource *self = (GeneratorSource *)s;

  self->options = options;
  log_source_set_options(&self->super, &options->super,
                         stats_id, stats_instance,
                         threaded, pos_tracked, expr_node);
}

gboolean
generator_source_init(LogPipe *s)
{
  GeneratorSource *self = (GeneratorSource *) s;

  if(!log_source_init(s))
    return FALSE;

  generator_source_init_template(&self->super);
  generator_timer_update_and_trigger(self);
  return TRUE;
}

gboolean
generator_source_deinit(LogPipe *s)
{
  return log_source_deinit(s);
}

LogSource *
generator_source_new(GlobalConfig *cfg)
{
  GeneratorSource *self = g_new0(GeneratorSource, 1);

  log_source_init_instance(&self->super, cfg);
  self->super.super.init = generator_source_init;
  self->super.super.deinit = generator_source_deinit;

  generator_timer_init(self);
  return &self->super;
}

static gboolean
generator_sd_init(LogPipe *s)
{
  GeneratorSrcDriver *self = (GeneratorSrcDriver *) s;
  GlobalConfig *cfg = log_pipe_get_config(s);

  if (!log_src_driver_init_method(s))
    return FALSE;

  generator_source_options_init(&self->gsource_options, cfg, self->super.super.group);
  self->generator = generator_source_new(cfg);
  generator_source_set_options(self->generator, &self->gsource_options,
                               self->super.super.id, "generator",
                               FALSE, FALSE, self->super.super.super.expr_node);

  log_pipe_append(&self->generator->super, s);
  if (!log_pipe_init(&self->generator->super))
    {
      log_pipe_unref(&self->generator->super);
      self->generator = NULL;
      return FALSE;
    }

  return TRUE;
}

static gboolean
generator_sd_deinit(LogPipe *s)
{
  GeneratorSrcDriver *self = (GeneratorSrcDriver *) s;
  if (!log_src_driver_deinit_method(s))
    return FALSE;

  if(self->generator)
    {
      log_pipe_deinit(&self->generator->super);
      log_pipe_unref(&self->generator->super);
      self->generator = NULL;
    }

  return TRUE;
}

static void
generator_sd_free(LogPipe *s)
{
  GeneratorSrcDriver *self = (GeneratorSrcDriver *) s;

  generator_source_options_destroy(&self->gsource_options);
  log_src_driver_free(s);
}

LogDriver *
generator_sd_new(GlobalConfig *cfg)
{
  GeneratorSrcDriver *self = g_new0(GeneratorSrcDriver, 1);

  log_src_driver_init_instance((LogSrcDriver *)&self->super, cfg);
  generator_source_options_defaults(&self->gsource_options);
  self->super.super.super.init = generator_sd_init;
  self->super.super.super.deinit = generator_sd_deinit;
  self->super.super.super.free_fn = generator_sd_free;
  return (LogDriver *)&self->super.super;
}
