/* menuentry.c - menuentry command */
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/normal.h>

static const struct grub_arg_option options[] =
  {
    {"class", 1, GRUB_ARG_OPTION_REPEATABLE,
     N_("Menu entry type."), "STRING", ARG_TYPE_STRING},
    {"users", 2, 0,
     N_("Users allowed to boot this entry."), "USERNAME", ARG_TYPE_STRING},
    {"hotkey", 3, 0,
     N_("Keyboard key for this entry."), "KEY", ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static struct
{
  char *name;
  int key;
} hotkey_aliases[] =
  {
    {"backspace", '\b'},
    {"tab", '\t'},
    {"delete", GRUB_TERM_DC}
  };

/* Add a menu entry to the current menu context (as given by the environment
   variable data slot `menu').  As the configuration file is read, the script
   parser calls this when a menu entry is to be created.  */
static grub_err_t
add_menu_entry (int argc, const char **args, char **classes,
		const char *users, const char *hotkey,
		const char *sourcecode)
{
  unsigned i;
  int menu_hotkey = 0;
  char *menu_users = NULL;
  char *menu_title = NULL;
  char *menu_sourcecode = NULL;
  struct grub_menu_entry_class *menu_classes = NULL;

  grub_menu_t menu;
  grub_menu_entry_t *last;

  menu = grub_env_get_menu ();
  if (! menu)
    return grub_error (GRUB_ERR_MENU, "no menu context");

  last = &menu->entry_list;

  menu_sourcecode = grub_strdup (sourcecode);
  if (! menu_sourcecode)
    return grub_errno;

  if (classes)
    {
      for (i = 0; classes[i]; i++); /* count # of menuentry classes */
      menu_classes = grub_zalloc (sizeof (struct grub_menu_entry_class) * i);
      if (! menu_classes)
	goto fail;

      for (i = 0; classes[i]; i++)
	{
	  menu_classes[i].name = grub_strdup (classes[i]);
	  if (! menu_classes[i].name)
	    goto fail;
	  menu_classes[i].next = classes[i + 1] ? &menu_classes[i + 1] : NULL;
	}
    }

  if (users)
    {
      menu_users = grub_strdup (users);
      if (! menu_users)
	goto fail;
    }

  if (hotkey)
    {
      for (i = 0; i < ARRAY_SIZE (hotkey_aliases); i++)
	if (grub_strcmp (hotkey, hotkey_aliases[i].name) == 0)
	  {
	    menu_hotkey = hotkey_aliases[i].key;
	    break;
	  }
      if (i > ARRAY_SIZE (hotkey_aliases))
	goto fail;
    }

  if (! argc)
    {
      grub_error (GRUB_ERR_MENU, "menuentry is missing title");
      goto fail;
    }

  menu_title = grub_strdup (args[0]);
  if (! menu_title)
    goto fail;

  /* XXX: pass args[1..end] as parameters to block arg. */

  /* Add the menu entry at the end of the list.  */
  while (*last)
    last = &(*last)->next;

  *last = grub_zalloc (sizeof (**last));
  if (! *last)
    goto fail;

  (*last)->title = menu_title;
  (*last)->hotkey = menu_hotkey;
  (*last)->classes = menu_classes;
  if (menu_users)
    (*last)->restricted = 1;
  (*last)->users = menu_users;
  (*last)->sourcecode = menu_sourcecode;

  menu->size++;
  return GRUB_ERR_NONE;

 fail:

  grub_free (menu_sourcecode);
  for (i = 0; menu_classes && menu_classes[i].name; i++)
    grub_free (menu_classes[i].name);
  grub_free (menu_classes);
  grub_free (menu_users);
  grub_free (menu_title);
  return grub_errno;
}

static grub_err_t
grub_cmd_menuentry (grub_extcmd_context_t ctxt, int argc, char **args)
{
  char ch;
  char *src;
  unsigned len;
  grub_err_t r;

  if (! argc || ! ctxt->script)
    return GRUB_ERR_BAD_ARGUMENT;

  src = args[argc - 1];
  args[argc - 1] = NULL;

  len = grub_strlen(src);
  ch = src[len - 1];
  src[len - 1] = '\0';

  r = add_menu_entry (argc - 1, (const char **) args,
		      ctxt->state[0].args, ctxt->state[1].arg,
		      ctxt->state[2].arg, src + 1);

  src[len - 1] = ch;
  args[argc - 1] = src;
  return r;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(menuentry)
{
  cmd = grub_register_extcmd ("menuentry", grub_cmd_menuentry,
			      GRUB_COMMAND_FLAG_BOTH | GRUB_COMMAND_FLAG_BLOCKS,
			      N_("BLOCK"), N_("Define a menuentry."), options);
}

GRUB_MOD_FINI(menuentry)
{
  grub_unregister_extcmd (cmd);
}
