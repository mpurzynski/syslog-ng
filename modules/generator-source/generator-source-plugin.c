/*
 * Copyright (c) 2017 Gabor Nagy
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

#include "cfg-parser.h"
#include "plugin.h"
#include "plugin-types.h"

extern CfgParser generator_source_parser;

static Plugin generator_source_plugins[] =
{
  {
    .type = LL_CONTEXT_SOURCE,
    .name = "generator",
    .parser = &generator_source_parser,
  },
};

gboolean
generator_source_module_init(GlobalConfig *cfg, CfgArgs *args)
{
  plugin_register(cfg, generator_source_plugins, G_N_ELEMENTS(generator_source_plugins));
  return TRUE;
}

const ModuleInfo module_info =
{
  .canonical_name = "generator_source",
  .version = SYSLOG_NG_VERSION,
  .description = "Please fill this description",
  .core_revision = SYSLOG_NG_SOURCE_REVISION,
  .plugins = generator_source_plugins,
  .plugins_len = G_N_ELEMENTS(generator_source_plugins),
};
