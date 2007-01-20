/*
 * �ե������ޤȤ�Ƽ���ե��������������
 *
 * �ǥե���ȤǤϤҤȤľ�Υǥ��쥯�ȥ��..�פ˳ƥե������
 * �ѥ�̾���դ��뤬�����Υ��ޥ�ɤ��Ф��� -p ���ץ�����
 * �ѹ����뤳�Ȥ��Ǥ��롣
 *
 * entry_num�ĤΥե�������Ф���
 *  0: entry_num �ե�����θĿ�
 *  1: �ƥե�����ξ���
 *    n * 3    : name_offset
 *    n * 3 + 1: strlen(key)
 *    n * 3 + 2: contents_offset
 *  [name_of_section]*entry_num
 *   : �ƥե������̾��
 *  [file]*entry_num
 *   : �ƥե����������
 *
 * Copyright (C) 2005-2006 YOSHIDA Yuichi
 * Copyright (C) 2006 TABATA Yusuke
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <xstr.h>
#include <diclib.h>

#define SECTION_ALIGNMENT 8
#define DIC_NAME "anthy.dic"

struct header_entry {
  const char* key;
  const char* file_name;
};

static void
write_nl(FILE* fp, int i)
{
  i = anthy_dic_htonl(i);
  fwrite(&i, sizeof(int), 1, fp);
}


/** �ե�����Υ�������������� */
static int
get_file_size(const char* fn)
{
  struct stat st;
  if (stat(fn, &st) < 0) {
    return -1;
  }
  return (st.st_size + SECTION_ALIGNMENT - 1) & (-SECTION_ALIGNMENT);
}

static char *
get_file_name(const char *prefix, struct header_entry* entry)
{
  char *fn = malloc(strlen(prefix) + strlen(entry->file_name) + 4);
  sprintf(fn, "%s/%s", prefix, entry->file_name);
  return fn;
}

static int
write_header(FILE* fp, const char *prefix,
	     int entry_num, struct header_entry* entries)
{
  int i;
  int name_offset;
  int contents_offset;

  name_offset = sizeof(int) * (1 + entry_num * 3);
  contents_offset = name_offset;

  for (i = 0; i < entry_num; ++i) {
    contents_offset += strlen(entries[i].key);
  }
  contents_offset =
    (contents_offset + SECTION_ALIGNMENT - 1) & (-SECTION_ALIGNMENT);

  /* �ե�����ο� */
  write_nl(fp, entry_num);

  /* �ƥե�����ξ�����Ϥ��� */
  for (i = 0; i < entry_num; ++i) {
    char *fn = get_file_name(prefix, &entries[i]);
    int file_size = get_file_size(fn);
    if (file_size == -1) {
      fprintf(stderr, "failed to get file size of (%s).\n",
	      fn);
      free(fn);
      return -1;
    }
    free(fn);
    /**/
    write_nl(fp, name_offset);
    write_nl(fp, strlen(entries[i].key));
    write_nl(fp, contents_offset);
    /**/
    name_offset += strlen(entries[i].key);
    contents_offset += file_size;
  }

  /* �ƥե������̾������Ϥ��� */
  for (i = 0; i < entry_num; ++i) {
    fprintf(fp, "%s", entries[i].key);
  }
  return 0;
}



static void
copy_file(FILE *in, FILE *out)
{
  int i;
  size_t nread;
  char buf[BUFSIZ];

  /* Pad OUT to the next aligned offset.  */
  for (i = ftell (out); i & (SECTION_ALIGNMENT - 1); i++) {
    fputc (0, out);
  }

  /* Copy the contents.  */
  rewind(in);
  while ((nread = fread (buf, 1, sizeof buf, in)) > 0) {
    if (fwrite (buf, 1, nread, out) < nread) {
      exit (1);
    }
  }
}

static void
write_contents(FILE* fp, const char *prefix,
	       int entry_num, struct header_entry* entries)
{
  int i;
  for (i = 0; i < entry_num; ++i) {
    FILE* in_fp;
    char *fn = get_file_name(prefix, &entries[i]);

    in_fp = fopen(fn, "r");
    if (in_fp == NULL) {
      printf("failed to open %s\n", fn);
      free(fn);
      break;
    }
    printf("  copying %s (%s)\n", fn, entries[i].key);
    free(fn);
    copy_file(in_fp, fp);
    fclose(in_fp);
  }
}


static void
create_file_dic(const char* fn, const char *prefix,
		int entry_num, struct header_entry* entries)
{
  FILE* fp = fopen(fn, "w");
  int res;
  if (!fp) {
    fprintf(stderr, "failed to open file dictionary file (%s).\n", fn);
    exit(1);
  }
  /* �إå���񤭽Ф� */
  res = write_header(fp, prefix, entry_num, entries);
  if (res) {
    exit(1);
  }

  /* �ե��������Ȥ�񤭽Ф� */
  write_contents(fp, prefix, entry_num, entries);
  fclose(fp);
}


int
main(int argc, char* argv[])
{
  int i;
  const char *prefix = "..";
  const char *prev_arg = "";

  struct header_entry entries[] = {
    {"word_dic", "/mkworddic/anthy.wdic"},
    {"dep_dic", "/depgraph/anthy.dep"},
    {"matrix", "/mkmatrix/matrix"},
  };

  for (i = 1; i < argc; i++) {
    if (!strcmp("-p", prev_arg)) {
      prefix = argv[i];
    }
    /**/
    prev_arg = argv[i];
  }
  printf("file name prefix=[%s] you can change this by -p option.\n", prefix);

  create_file_dic(DIC_NAME, prefix,
		  sizeof(entries)/sizeof(struct header_entry),
		  entries);

  printf("%s done.\n", argv[0]);
  return 0;
}