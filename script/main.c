/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/i18n.h>
#include <grub/parser.h>
#include <grub/script_sh.h>

grub_err_t
grub_normal_parse_line (char *line, grub_reader_getline_t getline)
{
  struct grub_script *parsed_script;

  /* Parse the script.  */
  parsed_script = grub_script_parse (line, getline);

  if (parsed_script)
    {
      /* Execute the command(s).  */
      grub_script_execute (parsed_script);

      /* The parsed script was executed, throw it away.  */
      grub_script_free (parsed_script);
    }

  return grub_errno;
}

static grub_command_t cmd_break;

void
grub_script_init (void)
{
  cmd_break = grub_register_command ("break", grub_script_break,
				     N_("[n]"), N_("Exit from loops"));
}

void
grub_script_fini (void)
{
  if (cmd_break)
    grub_unregister_command (cmd_break);
  cmd_break = 0;
}
