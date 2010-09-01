/*
 * ソートされたテキストから検索を行う
 */
/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <anthy/filemap.h>
#include <anthy/textdict.h>
#include "dic_main.h"

struct textdict {
  char *fn;
  char *ptr;
  struct filemapping *mapping;
};

struct textdict *
anthy_textdict_open(const char *fn, int create)
{
  struct textdict *td = malloc(sizeof(struct textdict));
  if (!td) {
    return NULL;
  }
  td->fn = strdup(fn);
  if (!td->fn) {
    free(td);
    return NULL;
  }
  td->mapping = NULL;
  return td;
}


static void
unmap(struct textdict *td)
{
  if (td->mapping) {
    anthy_munmap(td->mapping);
    td->mapping = NULL;
  }
}

void
anthy_textdict_close(struct textdict *td)
{
  if (!td) {
    return ;
  }
  unmap(td);
  free(td->fn);
  free(td);
}

static int
update_mapping(struct textdict *td)
{
  if (td->mapping) {
    anthy_munmap(td->mapping);
  }
  td->mapping = anthy_mmap(td->fn, 1);
  if (!td->mapping) {
    td->ptr = NULL;
    return 1;
  }
  td->ptr = anthy_mmap_address(td->mapping);
  return 0;
}

static int
expand_file(struct textdict *td, int len)
{
  FILE *fp;
  char buf[256];
  int c;
  fp = fopen(td->fn, "a+");
  if (!fp) {
    return -1;
  }
  memset(buf, '\n', 256);
  c = 1;
  if (len > 256) {
    c *= fwrite(buf, 256, len / 256, fp);
  }
  if (len % 256) {
    c *= fwrite(buf, len % 256, 1, fp);
  }
  fclose(fp);
  if (c == 0) {
    return -1;
  }
  return 0;
}

void
anthy_textdict_scan(struct textdict *td, int offset, void *ptr,
		    int (*fun)(void *, int, const char *, const char *))
{
  FILE *fp;
  char buf[1024];
  if (!td) {
    return ;
  }
  fp = fopen(td->fn, "r");
  if (!fp) {
    return ;
  }
  if (fseek(fp, offset, SEEK_SET)) {
    fclose(fp);
    return ;
  }
  while (fgets(buf, 1024, fp)) {
    char *p = strchr(buf, ' ');
    int len, r;
    len = strlen(buf);
    offset += len;
    buf[len - 1] = 0;
    if (!p) {
      continue;
    }
    *p = 0;
    p++;
    while (*p == ' ') {
      p++;
    }
    /* call it */
    r = fun(ptr, offset, buf, p);
    if (r) {
      break;
    }
  }
  fclose(fp);
}

int
anthy_textdict_delete_line(struct textdict *td, int offset)
{
  FILE *fp;
  char buf[1024];
  int len, size;
  fp = fopen(td->fn, "r");
  if (!fp) {
    return -1;
  }
  if (fseek(fp, offset, SEEK_SET)) {
    fclose(fp);
    return -1;
  }
  if (!fgets(buf, 1024, fp)) {
    fclose(fp);
    return -1;
  }
  len = strlen(buf);
  fclose(fp);
  update_mapping(td);
  if (!td->mapping) {
    return -1;
  }
  size = anthy_mmap_size(td->mapping);
  memmove(&td->ptr[offset], &td->ptr[offset+len], size - offset - len);
  unmap(td);
  if (size - len == 0) {
    unlink(td->fn);
    return 0;
  }
  truncate(td->fn, size - len);
  return 0;
}

int
anthy_textdict_insert_line(struct textdict *td, int offset,
			   const char *line)
{
  int len = strlen(line);
  int size;
  if (!td) {
    return -1;
  }
  if (expand_file(td, len)) {
    return -1;
  }
  update_mapping(td);
  size = anthy_mmap_size(td->mapping);
  memmove(&td->ptr[offset+len], &td->ptr[offset], size - offset - len);
  memcpy(&td->ptr[offset], line, len);
  return 0;
}
