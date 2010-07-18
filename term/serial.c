/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2004,2005,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/serial.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/terminfo.h>
#include <grub/cpu/io.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/list.h>

#define FOR_SERIAL_PORTS(var) FOR_LIST_ELEMENTS((var), (grub_serial_ports))

/* Argument options.  */
static const struct grub_arg_option options[] =
{
  {"unit",   'u', 0, N_("Set the serial unit."),             0, ARG_TYPE_INT},
  {"port",   'p', 0, N_("Set the serial port address."),     0, ARG_TYPE_STRING},
  {"speed",  's', 0, N_("Set the serial port speed."),       0, ARG_TYPE_INT},
  {"word",   'w', 0, N_("Set the serial port word length."), 0, ARG_TYPE_INT},
  {"parity", 'r', 0, N_("Set the serial port parity."),      0, ARG_TYPE_STRING},
  {"stop",   't', 0, N_("Set the serial port stop bits."),   0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

struct grub_serial_port *grub_serial_ports;

struct grub_serial_output_state
{
  struct grub_terminfo_output_state tinfo;
  struct grub_serial_port *port;
};

struct grub_serial_input_state
{
  struct grub_terminfo_input_state tinfo;
  struct grub_serial_port *port;
};

static grub_uint16_t
grub_serial_getwh (struct grub_term_output *term __attribute__ ((unused)))
{
  const grub_uint8_t TEXT_WIDTH = 80;
  const grub_uint8_t TEXT_HEIGHT = 24;
  return (TEXT_WIDTH << 8) | TEXT_HEIGHT;
}

static void 
serial_put (grub_term_output_t term, const int c)
{
  struct grub_serial_output_state *data = term->data;
  data->port->driver->put (data->port, c);
}

static int
serial_fetch (grub_term_input_t term)
{
  struct grub_serial_input_state *data = term->data;
  return data->port->driver->fetch (data->port);
}

struct grub_serial_input_state grub_serial_terminfo_input =
  {
    .tinfo =
    {
      .readkey = serial_fetch
    }
  };

struct grub_serial_output_state grub_serial_terminfo_output =
  {
    .tinfo =
    {
      .put = serial_put
    }
  };

int registered = 0;

static struct grub_term_input grub_serial_term_input =
{
  .name = "serial",
  .init = grub_terminfo_input_init,
  .checkkey = grub_terminfo_checkkey,
  .getkey = grub_terminfo_getkey,
  .data = &grub_serial_terminfo_input
};

static struct grub_term_output grub_serial_term_output =
{
  .name = "serial",
  .putchar = grub_terminfo_putchar,
  .getwh = grub_serial_getwh,
  .getxy = grub_terminfo_getxy,
  .gotoxy = grub_terminfo_gotoxy,
  .cls = grub_terminfo_cls,
  .setcolorstate = grub_terminfo_setcolorstate,
  .setcursor = grub_terminfo_setcursor,
  .flags = GRUB_TERM_CODE_TYPE_ASCII,
  .data = &grub_serial_terminfo_output,
  .normal_color = GRUB_TERM_DEFAULT_NORMAL_COLOR,
  .highlight_color = GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR,
};



static struct grub_serial_port *
grub_serial_find (char *name)
{
  struct grub_serial_port *port;

  FOR_SERIAL_PORTS (port)
    if (grub_strcmp (port->name, name) == 0)
      break;

  if (!port && grub_memcmp (name, "port", sizeof ("port") - 1) == 0
      && grub_isdigit (name [sizeof ("port") - 1]))
    {
      name = grub_serial_ns8250_add_port (grub_strtoul (&name[sizeof ("port") - 1],
							0, 16));
      if (!name)
	return NULL;

      FOR_SERIAL_PORTS (port)
	if (grub_strcmp (port->name, name) == 0)
	  break;
    }

  return port;
}

static grub_err_t
grub_cmd_serial (grub_extcmd_t cmd, int argc, char **args)
{
  struct grub_arg_list *state = cmd->state;
  char pname[40];
  char *name = NULL;
  struct grub_serial_port *port;
  struct grub_serial_config config;
  grub_err_t err;

  if (state[0].set)
    {
      grub_snprintf (pname, sizeof (pname), "com%ld",
		     grub_strtoul (state[0].arg, 0, 0));
      name = pname;
    }

  if (state[1].set)
    {
      grub_snprintf (pname, sizeof (pname), "port%lx",
		     grub_strtoul (state[1].arg, 0, 0));
      name = pname;
    }

  if (argc >= 1)
    name = args[0];

  if (!name)
    name = "com0";

  port = grub_serial_find (name);
  if (!port)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown serial port");

  config = port->config;

  if (state[2].set)
    config.speed = grub_strtoul (state[2].arg, 0, 0);

  if (state[3].set)
    config.word_len = grub_strtoul (state[3].arg, 0, 0);

  if (state[4].set)
    {
      if (! grub_strcmp (state[4].arg, "no"))
	config.parity = GRUB_SERIAL_PARITY_NONE;
      else if (! grub_strcmp (state[4].arg, "odd"))
	config.parity = GRUB_SERIAL_PARITY_ODD;
      else if (! grub_strcmp (state[4].arg, "even"))
	config.parity = GRUB_SERIAL_PARITY_EVEN;
      else
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad parity");
    }

  if (state[5].set)
    {
      if (! grub_strcmp (state[5].arg, "1"))
	config.stop_bits = GRUB_SERIAL_STOP_BITS_1;
      else if (! grub_strcmp (state[5].arg, "2"))
	config.stop_bits = GRUB_SERIAL_STOP_BITS_2;
      else
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad number of stop bits");
    }

  /* Initialize with new settings.  */
  err = port->driver->configure (port, &config);
  if (err)
    return err;
  if (!registered)
    {
      grub_term_register_input ("serial", &grub_serial_term_input);
      grub_term_register_output ("serial", &grub_serial_term_output);
    }
  grub_serial_terminfo_output.port = port;
  grub_serial_terminfo_input.port = port;
  registered = 1;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_serial_register (struct grub_serial_port *port)
{
  struct grub_term_input *in;
  struct grub_term_output *out;
  struct grub_serial_input_state *indata;
  struct grub_serial_output_state *outdata;

  in = grub_malloc (sizeof (*in));
  if (!in)
    return grub_errno;

  indata = grub_malloc (sizeof (*indata));
  if (!indata)
    {
      grub_free (in);
      return grub_errno;
    }

  grub_memcpy (in, &grub_serial_term_input, sizeof (*in));
  in->data = indata;
  in->name = grub_xasprintf ("serial_%s", port->name);
  grub_memcpy (indata, &grub_serial_terminfo_input, sizeof (*indata));

  if (!in->name)
    {
      grub_free (in);
      grub_free (indata);
      return grub_errno;
    }
  
  out = grub_malloc (sizeof (*out));
  if (!out)
    {
      grub_free (in);
      grub_free (indata);
      grub_free ((char *) in->name);
      return grub_errno;
    }

  outdata = grub_malloc (sizeof (*outdata));
  if (!outdata)
    {
      grub_free (in);
      grub_free (indata);
      grub_free ((char *) in->name);
      grub_free (out);
      return grub_errno;
    }

  grub_memcpy (out, &grub_serial_term_output, sizeof (*out));
  out->data = outdata;
  out->name = in->name;
  grub_memcpy (outdata, &grub_serial_terminfo_output, sizeof (*outdata));

  grub_list_push (GRUB_AS_LIST_P (&grub_serial_ports), GRUB_AS_LIST (port));
  ((struct grub_serial_input_state *) in->data)->port = port;
  ((struct grub_serial_output_state *) out->data)->port = port;
  grub_term_register_input ("serial_*", in);
  grub_term_register_output ("serial_*", out);
  grub_terminfo_output_register (out, "vt100");

  return GRUB_ERR_NONE;
}

void
grub_serial_unregister (struct grub_serial_port *port)
{
  grub_list_remove (GRUB_AS_LIST_P (&grub_serial_ports), GRUB_AS_LIST (port));
  /* FIXME */
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(serial)
{
  cmd = grub_register_extcmd ("serial", grub_cmd_serial,
			      GRUB_COMMAND_FLAG_BOTH,
			      N_("[OPTIONS...]"),
			      N_("Configure serial port."), options);

  grub_ns8250_init ();

#ifdef GRUB_MACHINE_MIPS_YEELOONG
  {
    grub_err_t hwiniterr;
    hwiniterr = grub_ns8250_driver.init ("com0", &serial_settings);
    serial_settings.driver = &grub_ns8250_driver;

    if (hwiniterr == GRUB_ERR_NONE)
      {
	grub_term_register_input_active ("serial", &grub_serial_term_input);
	grub_term_register_output_active ("serial", &grub_serial_term_output);

	registered = 1;
      }
  }
#endif
}

GRUB_MOD_FINI(serial)
{
  while (grub_serial_ports)
    grub_serial_unregister (grub_serial_ports);
  grub_unregister_extcmd (cmd);
}
