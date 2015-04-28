/*
 * textdic.c -- handle dictionary of a plain text file
 *
 * Copyright (C) 2010 Free Software Initiative of Japan
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of Anthy, Japanese Kana-Kanji Conversion Engine
 * Library.
 *
 * Anthy is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "anthy/textdic.h"

#define BUFSIZE 1024

int
anthy_textdic_scan (const char *name, long offset, void *data,
		    int (*func)(void *, long, const char *, const char *))
{
  FILE *fp;
  char buf[BUFSIZE];

  fp = fopen (name, "r");
  if (!fp)
    return -1;

  if (fseek (fp, offset, SEEK_SET) < 0)
    {
      fclose (fp);
      return -1;
    }

  /*
   * Call FUNC for each line and if FUNC success, stop at there
   */
  while (fgets (buf, BUFSIZE, fp) != NULL)
    {
      int i;
      const char *column0, *column1;
      char *end_of_column0;

      column0 = buf;
      column1 = NULL;
      end_of_column0 = NULL;
      for (i = 0; i < BUFSIZE; i++)
	if (buf[i])
	  {
	    if (end_of_column0 == NULL)
	      {
		if (buf[i] == ' ')
		  end_of_column0 = &buf[i];
	      }
	    else
	      if (column1 == NULL && buf[i] != ' ')
		column1 = &buf[i];
	  }
	else
	  break;

      offset += i;
      if (column1 == NULL)
	continue;

      buf[i - 1] = '\0';	/* chop newline */
      *end_of_column0 = '\0';

      if (func (data, offset, column0, column1))
	break;
    }

  fclose (fp);
  return 0;
}

static char *
tempfile (const char *orig_filename, FILE **fp_p)
{
  char *filename = (char *)malloc ((strlen (orig_filename) + 6 + 1));
  int fd;
  FILE *fp;

  strcpy (filename, orig_filename);
  strcat (filename, "XXXXXX");
  fd = mkstemp (filename);
  if (fd < 0)
    {
      free (filename);
      return NULL;
    }

  if ((fp = fdopen (fd, "w")) == NULL)
    {
      close (fd);
      unlink (filename);
      free (filename);
      return NULL;
    }

  *fp_p = fp;
  return filename;
}

static int
do_textdic_at (const char *name, long offset, void *data,
	       int (*work) (void *, FILE *, FILE *))
{
  FILE *fp_r, *fp_w;
  char buf[BUFSIZE];
  char *filename;
  int i, r;

  if ((fp_r = fopen (name, "r")) == NULL)
    return -1;

  if ((filename = tempfile (name, &fp_w)) == NULL)
    goto error_w;

  for (i = 0; i < offset - BUFSIZE; i += BUFSIZE)
    if ((r = fread (buf, 1, BUFSIZE, fp_r)) < BUFSIZE)
      goto error_file;
    else if ((r = fwrite (buf, 1, BUFSIZE, fp_w)) < BUFSIZE)
      goto error_file;

  if ((offset - i) > 0)
    {
      if ((r = fread (buf, 1, offset - i, fp_r)) < offset - i)
	goto error_file;
      else if ((r = fwrite (buf, 1, offset - i, fp_w))  < offset - i)
	goto error_file;
    }

  if (work (data, fp_r, fp_w) < 0)
    goto error_file;

  while (1)
    {
      int r0;

      r = fread (buf, 1, BUFSIZE, fp_r);
      r0 = fwrite (buf, 1, r, fp_w);
      if (r0 != r)
	goto error_file;

      if (r < BUFSIZE)
	{
	  if (feof (fp_r))
	    break;
	  else
	    goto error_file;
	}
    }

  fclose (fp_w);
  fclose (fp_r);

  rename (filename, name);
  free (filename);
  return 0;

 error_file:
  fclose (fp_w);
  unlink (filename);
 error_w:
  fclose (fp_r);
  free (filename);
  return -1;
}

static int
delete_line (void *data, FILE *fp_r, FILE *fp_w)
{
  char buf[BUFSIZE];

  (void)data;
  (void)fp_w;
  if (fgets (buf, BUFSIZE, fp_r) == NULL)
    return -1;

  return 0;
}

int
anthy_textdic_delete_line (const char *name, long offset)
{
  return do_textdic_at (name, offset, NULL, delete_line);
}

static int
insert_line (void *data, FILE *fp_r, FILE *fp_w)
{
  const char *line = (const char *)data;
  size_t r;

  (void)fp_r;
  r = fwrite (line, 1, strlen (line), fp_w);
  if (r < strlen (line))
    return -1;

  return 0;
}

int
anthy_textdic_insert_line (const char *name, long offset, const char *line)
{
  return do_textdic_at (name, offset, (void *)line, insert_line);
}
