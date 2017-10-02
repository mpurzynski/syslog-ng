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
#include <string.h>
#include <iv.h>

typedef struct
{
  LogSource super;
  struct iv_timer generator_timer;
} GeneratorSource;

typedef struct 
{
  LogSrcDriver super;
  GeneratorSource *generator;
  LogSourceOptions gsource_options;
} GeneratorSrcDriver;

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
  self->generator_timer.expires.tv_sec += 3;
  iv_timer_register(&self->generator_timer);
}

static void
generator_timer_handler(void *cookie)
{
  GeneratorSource *self = (GeneratorSource *) cookie;
  LogMessage *msg = log_msg_new_empty();
  log_msg_set_value(msg, LM_V_MESSAGE, "YIPPIKAYE", -1);
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

static gboolean
generator_source_init(LogPipe *s)
{
  GeneratorSource *self = (GeneratorSource*) s;
 
  if(!log_source_init(s))
    return FALSE;

  generator_timer_update_and_trigger(self);
  return TRUE;
}

static gboolean
generator_source_deinit(LogPipe *s)
{
  return log_source_deinit(s);
}

static GeneratorSource *
generator_source_new(GeneratorSrcDriver *owner)
{
  GeneratorSource *self = g_new0(GeneratorSource, 1);
  GlobalConfig *cfg = log_pipe_get_config(&owner->super.super.super);

  log_source_init_instance(&self->super, cfg);
  log_source_set_options(&self->super, &owner->gsource_options,
                        owner->super.super.id, "generator_source",
                        FALSE, FALSE, owner->super.super.super.expr_node);
  self->super.super.init = generator_source_init;
  self->super.super.deinit = generator_source_deinit;

  generator_timer_init(self);
  return self;
}

static gboolean
generator_sd_init(LogPipe *s)
{
  GeneratorSrcDriver *self = (GeneratorSrcDriver *) s;
  GlobalConfig *cfg = log_pipe_get_config(s);

  if (!log_src_driver_init_method(s))
    return FALSE;

  log_source_options_init(&self->gsource_options, cfg, self->super.super.group);
  self->generator = generator_source_new(self);
  log_pipe_append(&self->generator->super.super, s);
  if (!log_pipe_init(&self->generator->super.super))
    {
      log_pipe_unref(&self->generator->super.super);
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
      log_pipe_deinit(&self->generator->super.super);
      log_pipe_unref(&self->generator->super.super);
      self->generator = NULL;
    }

  return TRUE;
}

static void
generator_sd_free(LogPipe *s)
{
  GeneratorSrcDriver *self = (GeneratorSrcDriver *) s;

  log_source_options_destroy(&self->gsource_options);
  log_src_driver_free(s);
}

LogDriver *
generator_sd_new(GlobalConfig *cfg)
{
  GeneratorSrcDriver *self = g_new0(GeneratorSrcDriver, 1);

  log_src_driver_init_instance((LogSrcDriver *)&self->super, cfg);
  log_source_options_defaults(&self->gsource_options);
  self->super.super.super.init = generator_sd_init;
  self->super.super.super.deinit = generator_sd_deinit;
  self->super.super.super.free_fn = generator_sd_free;
  return (LogDriver *)&self->super.super;
}
