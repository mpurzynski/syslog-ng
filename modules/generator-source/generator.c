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

typedef struct
{
  LogSource super;
} GeneratorSource;

typedef struct 
{
  LogSrcDriver super;
  GeneratorSource *generator;
} GeneratorSrcDriver;

static gboolean
generator_source_init(LogPipe *s)
{
  if(!log_source_init(s))
    return FALSE;

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
  self->super.super.init = generator_source_init;
  self->super.super.deinit = generator_source_deinit;
  return self;
}

static gboolean
generator_sd_init(LogPipe *s)
{
  GeneratorSrcDriver *self = (GeneratorSrcDriver *) s;

  if (!log_src_driver_init_method(s))
    return FALSE;

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
  log_src_driver_free(s);
}

LogDriver *
generator_sd_new(GlobalConfig *cfg)
{
  GeneratorSrcDriver *self = g_new0(GeneratorSrcDriver, 1);

  log_src_driver_init_instance((LogSrcDriver *)&self->super, cfg);
  self->super.super.super.init = generator_sd_init;
  self->super.super.super.deinit = generator_sd_deinit;
  self->super.super.super.free_fn = generator_sd_free;
  return (LogDriver *)&self->super.super;
}
