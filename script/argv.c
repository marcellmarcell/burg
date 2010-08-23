/* argv.c - methods for constructing argument vector */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/script_sh.h>

/* Return nearest power of two that is >= v.  */
static unsigned
round_up_exp (unsigned v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;

  if (sizeof (v) > 4)
    v |= v >> 32;

  v++;
  v += (v == 0);

  return v;
}

void
grub_script_argv_free (struct grub_script_argv *argv)
{
  unsigned i;

  if (argv->args)
    {
      for (i = 0; i < argv->argc; i++)
	grub_free (argv->args[i]);

      grub_free (argv->args);
    }

  argv->argc = 0;
  argv->args = 0;
}

/* Prepare for next argc.  */
int
grub_script_argv_next (struct grub_script_argv *argv)
{
  char **p = argv->args;

  if (argv->args && argv->args[argv->argc - 1] == 0)
    return 0;

  p = grub_realloc (p, round_up_exp ((argv->argc + 2) * sizeof (char *)));
  if (! p)
    return 1;

  argv->argc++;
  argv->args = p;

  if (argv->argc == 1)
    argv->args[0] = 0;
  argv->args[argv->argc] = 0;
  return 0;
}

/* Append `s' to the last argument.  */
int
grub_script_argv_append (struct grub_script_argv *argv, const char *s)
{
  int a, b;
  char *p = argv->args[argv->argc - 1];

  if (! s)
    return 0;

  a = p ? grub_strlen (p) : 0;
  b = grub_strlen (s);

  p = grub_realloc (p, round_up_exp ((a + b + 1) * sizeof (char)));
  if (! p)
    return 1;

  grub_strcpy (p + a, s);
  argv->args[argv->argc - 1] = p;
  return 0;
}

/* Split `s' and append words as multiple arguments.  */
int
grub_script_argv_split_append (struct grub_script_argv *argv, char *s)
{
  char ch;
  char *p;
  int errors = 0;

  if (! s)
    return 0;

  while (! errors && *s)
    {
      p = s;
      while (*s && ! grub_isspace (*s))
	s++;

      ch = *s;
      *s = '\0';
      errors += grub_script_argv_append (argv, p);
      *s = ch;

      while (*s && grub_isspace (*s))
	s++;

      if (*s)
	errors += grub_script_argv_next (argv);
    }
  return errors;
}