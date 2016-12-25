/*
 * This file is part of Log Writer Library (LWL).
 * Copyright (C) 2002, 2003, 2004, 2005 Nicolas Darnis <ndarnis@free.fr>
 *
 * The LWL library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * The LWL library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the LWL library; see the file COPYING.
 * If not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 * $RCSfile: lwl.c,v $
 * $Revision: 1.12 $
 * $Date: 2005/01/05 16:05:22 $
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "lwl.h"

#define IP_LGTH 16

struct _lwl_handle
{
  lwl_priority_t log_level;
  char*          log_prefix;
  lwl_option_t   log_options;
  FILE*          log_files [LWL_PRI_DEBUG+2]; /* One file per level */
  char           log_ip [IP_LGTH];
  char*          log_adr;
};

static const char* 
level_to_string (lwl_priority_t log_priority);

extern lwlh_t
lwl_alloc (void)
{
  int i;
  lwlh_t h;
  
  h = (lwlh_t) malloc (sizeof (struct _lwl_handle));

  if (h == NULL)
    {
      return NULL;
    }
  
  h->log_level   = LWL_PRI_DEBUG;
  h->log_prefix  = NULL;
  h->log_options = 0;
  h->log_adr     = NULL;

  for (i = LWL_PRI_EMERG; i <= LWL_PRI_DEBUG; i++)
    {
      h->log_files[i] = NULL;
    }

  h->log_files[i] = stdout;

  return h;
}

extern void
lwl_free (lwlh_t h)
{
  if (h->log_prefix != NULL)
    {
      free (h->log_prefix);
    }

  if (h->log_adr != NULL)
    {
      free (h->log_adr);
    }

  free (h);
}

extern void
lwl_write_log (lwlh_t h, lwl_priority_t pri, char *format, ...)
{
  va_list ap;
  char* msg = NULL;
  char* msg2 = NULL;
  size_t msg2_size = 1;
  int space = 0;
  int taille = 64;
  int n;
  FILE* default_file;

  if (h->log_level < pri)
    {
      return;
    }

  if (h->log_files[pri] == NULL)
    {
      default_file = h->log_files [LWL_PRI_DEBUG+1];
    }
  else
    {
      default_file = h->log_files[pri];
    }

  va_start (ap, format);

  while (1)
    {
      msg = realloc (msg, taille);

      if (msg == NULL)
	{
	  break;
	}
      
      n = vsnprintf (msg, taille, format, ap);

      if (n >= 0 && n < taille)
	{
	  break;
	}

      taille += 64;
    }

  va_end (ap);
  
  if (msg == NULL)
    {
      return;
    }
  
  if (h->log_options & LWL_OPT_DATE || h->log_options & LWL_OPT_TIME)
    {
      char      date[20];
      char      heure[20];
      time_t    time_now;
      struct tm temps_now;
      
      time (&time_now);
      localtime_r (&time_now, &temps_now);

      if (h->log_options & LWL_OPT_DATE)
	{
	  msg2 = (char*) realloc (msg2, msg2_size + sizeof (date));

	  if (msg2 != NULL)
	    {
	      if (h->log_options & LWL_OPT_USE_LOCALE)
		{
		  msg2_size += strftime (&msg2[msg2_size-1], 1 + sizeof (date), "%x", &temps_now);
		}
	      else
		{
		  msg2_size += strftime (&msg2[msg2_size-1], 1 + sizeof (date), "%Y.%m.%d", &temps_now);
		}

	      space = 1;
	    }
	}
      
      if (h->log_options & LWL_OPT_TIME)
	{
	  msg2 = (char*) realloc (msg2, msg2_size + space + sizeof (heure));

	  if (msg2 != NULL)
	    {
	      if (space == 1) 
		{
		  msg2_size += sprintf (&msg2[msg2_size-1], " ");
		}
	      
	      if (h->log_options & LWL_OPT_USE_LOCALE)
		{
		  msg2_size += strftime (&msg2[msg2_size-1], 1 + sizeof (heure), "%X", &temps_now);
		}
	      else
		{
		  msg2_size += strftime (&msg2[msg2_size-1], 1 + sizeof (heure), "%H:%M:%S", &temps_now);
		}

	      space = 1;
	    }
	}
    }

  if (h->log_options & LWL_OPT_IP)
    {
      msg2 = (char*) realloc (msg2, msg2_size + space + strlen (h->log_ip));

      if (msg2 != NULL)
	{
	  msg2_size += snprintf (&msg2[msg2_size-1], 1 + space + strlen (h->log_ip), "%c%s", space == 1 ? ' ' : 0, h->log_ip);
	  space = 1;
	}
    }
  
  if (h->log_options & LWL_OPT_ADR)
    {
      msg2 = (char*) realloc (msg2, msg2_size + space + strlen (h->log_adr));

      if (msg2 != NULL)
	{
	  msg2_size += snprintf (&msg2[msg2_size-1], 1 + space + strlen (h->log_adr), "%c%s", space == 1 ? ' ' : 0, h->log_adr);
	  space = 1;
	}
    }
  
  if (h->log_options & LWL_OPT_PREFIX)
    {
      msg2 = (char*) realloc (msg2, msg2_size + space + strlen (h->log_prefix));

      if (msg2 != NULL)
	{
	  msg2_size += snprintf (&msg2[msg2_size-1], 1 + space + strlen (h->log_prefix), "%c%s", space == 1 ? ' ' : 0, h->log_prefix);
	  space = 1;
	}
    }
  
  if (h->log_options & LWL_OPT_PID)
    {
      msg2 = (char*) realloc (msg2, msg2_size + space + 10);

      if (msg2 != NULL)
	{
	  msg2_size += snprintf (&msg2[msg2_size-1], 1 + space + 10, "%c%10d", space == 1 ? ' ' : 0, getpid ());
	  space = 1;
	}
    }
  
  if (h->log_options & LWL_OPT_PRIORITY)
    {
      msg2 = (char*) realloc (msg2, msg2_size + space + strlen (level_to_string (pri)));

      if (msg2 != NULL)
	{
	  msg2_size += snprintf (&msg2[msg2_size-1], 1 + space + strlen (level_to_string (pri)), "%c%s", space == 1 ? ' ' : 0, level_to_string (pri));
	}
    }

  if (msg2 != NULL)
    {
      fprintf (default_file, "%s: %s\n", msg2, msg);

      if (h->log_options & LWL_OPT_PERROR)
	{
	  fprintf (stderr, "%s: %s\n", msg2, msg);
	}

      free (msg2);
    }
  else
    {
      fprintf (default_file, "%s\n", msg);
    }

  if (!(h->log_options & LWL_OPT_NO_FLUSH))
    {
      fflush (default_file);

      if (h->log_options & LWL_OPT_PERROR)
	{
	  fflush (stderr);
	}
    }

  free (msg);
}

extern int
lwl_set_attributes (lwlh_t h, lwl_tag_t tag, ...)
{
  va_list ap;
  lwl_tag_t tagl = tag;

  va_start  (ap, tag);

  while (tagl != LWL_TAG_DONE)
    {
      switch (tagl)
	{
	case LWL_TAG_LEVEL:
	  {
	    lwl_priority_t pri = va_arg (ap, lwl_priority_t);
	    if (pri < LWL_PRI_EMERG || pri > LWL_PRI_DEBUG)
	      {
		return -1;
	      }
	    h->log_level = pri;
	  }
	  break;

	case LWL_TAG_PREFIX:
	  if (h->log_prefix != NULL)
	    {
	      free (h->log_prefix);
	    }

	  h->log_prefix = strdup (va_arg (ap, char*));

	  if (h->log_prefix == NULL)
	    {
	      return -1;
	    }
	  break;
	  
	case LWL_TAG_OPTIONS:
	  {
	    lwl_option_t opt = va_arg (ap, lwl_option_t);
	    
	    if (opt != h->log_options)
	      {
		h->log_options = opt;
		
		if (h->log_options & LWL_OPT_IP || h->log_options & LWL_OPT_ADR)
		  {
		    char hostname [128];
		    
		    if (gethostname (hostname, sizeof (hostname)) == 0)
		      {
			struct hostent* he = gethostbyname (hostname);

			if (he != NULL)
			  {
			    int n;
			    struct in_addr* ip = (struct in_addr*) (he->h_addr_list [0]);
			    strncpy (h->log_ip, inet_ntoa (*ip), IP_LGTH);
			    
			    n = 1 + strlen (he->h_name);
			    
			    if (h->log_adr != NULL)
			      {
				free (h->log_adr);
			      }
			    
			    h->log_adr = (char*) malloc (n * sizeof (char));
			    if (h->log_adr != NULL)
			      {
				strncpy (h->log_adr, he->h_name, n);
			      }
			  }
		      }
		  }
	      }
	  }
	  break;

	case LWL_TAG_FILE:
	  h->log_files[LWL_PRI_DEBUG+1] = va_arg (ap, FILE*);
	  break;

	case LWL_TAG_FILE_EMERG:
	  h->log_files[LWL_PRI_EMERG] = va_arg (ap, FILE*);
	  break;

	case LWL_TAG_FILE_ALERT:
	  h->log_files[LWL_PRI_ALERT] = va_arg (ap, FILE*);
	  break;

	case LWL_TAG_FILE_CRIT:
	  h->log_files[LWL_PRI_CRIT] = va_arg (ap, FILE*);
	  break;

	case LWL_TAG_FILE_ERR:
	  h->log_files[LWL_PRI_ERR] = va_arg (ap, FILE *);
	  break;

	case LWL_TAG_FILE_WARNING:
	  h->log_files[LWL_PRI_WARNING] = va_arg (ap, FILE*);
	  break;

	case LWL_TAG_FILE_NOTICE:
	  h->log_files[LWL_PRI_NOTICE] = va_arg (ap, FILE*);
	  break;

	case LWL_TAG_FILE_INFO:
	  h->log_files[LWL_PRI_INFO] = va_arg (ap, FILE*);
	  break;

	case LWL_TAG_FILE_DEBUG:
	  h->log_files[LWL_PRI_DEBUG] = va_arg (ap, FILE*);
	  break;

	default:
	  return -1;
	}

      tagl = va_arg (ap, lwl_tag_t);
    }
  
  va_end (ap);

  return 0;
}

extern lwl_priority_t
lwl_get_log_level (const lwlh_t h)
{
  return h->log_level;
}

extern FILE*
lwl_get_default_log_file (const lwlh_t h)
{
  return h->log_files [LWL_PRI_DEBUG+1];
}

extern const char*
lwl_get_hostname (const lwlh_t h)
{
  return h->log_adr;
}

static const char* 
level_to_string (lwl_priority_t pri)
{
  switch (pri)
    {
    case LWL_PRI_EMERG:
      return "EMERG";

    case LWL_PRI_ALERT:
      return "ALERT";

    case LWL_PRI_CRIT:
      return "CRIT";

    case LWL_PRI_ERR:
      return "ERROR";

    case LWL_PRI_WARNING:
      return "WARNING";

    case LWL_PRI_NOTICE:
      return "NOTICE";

    case LWL_PRI_INFO:
      return "INFO";

    case LWL_PRI_DEBUG:
      return "DEBUG";
    }

  return "UNKNOWN_PRIORITY";
}

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset: 4
 * End:
 */
