/*
 * 構造体アロケータ
 * Funded by IPA未踏ソフトウェア創造事業 2001 8/15
 *
 * $Id: alloc.c,v 1.12 2002/05/15 11:21:10 yusuke Exp $
 *
 * Copyright (C) 2000-2005 TABATA Yusuke, UGAWA Tomoharu
 * Copyright (C) 2002, 2005 NIIBE Yutaka
 *
 * dtor: destructor
 * 
 * ページ中のフリーなchunkは単方向リストに継がれている
 * 使用中のchunkは属するページを指している
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alloc.h>
#include <logger.h>

/*#define MEM_DEBUG*/
/**/
#define PAGE_MAGIC 0x12345678
#define PAGE_SIZE 2048

/* ページ使用量の合計、デバッグの時等に用いる */
static int nr_pages;

/* page内のオブジェクトを表すオブジェクト */
struct chunk {
#ifdef MEM_DEBUG
  void *ator;
#endif
  union {
    void *storage[1];
    /* double型のalignを要求するアーキテクチャ+OSのためのhack */
    double align;
  } u;
};
#define CHUNK_HEADER_SIZE ((size_t)&((struct chunk *)0)->u.storage)

/*
 * pageのstorage中には 
 * max_obj = (PAGE_SIZE - PAGE_HEADER_SIZE)/ (size + CHUNK_HEADER_SIZE)個の
 * スロットがある。そのうちuse_count個のスロットがfree_listにつながっている、
 * もしくは使用中である。
 */
struct page {
  int magic;
  struct page *prev, *next;
};


#define PAGE_HEADER_SIZE (sizeof(struct page))
#define PAGE_AVAIL(p) ((unsigned char*)p + sizeof(struct page))
#define MAX_NUM(size) (int)((PAGE_SIZE - PAGE_HEADER_SIZE) / ((size) + CHUNK_HEADER_SIZE + 1.0 / 8))
#define PAGE_STORAGE(p, size) (PAGE_AVAIL(p) + (MAX_NUM((size)) >> 3) + 1)
#define PAGE_CHUNK(p, size, i)(struct chunk*)(&PAGE_STORAGE(p, size)[((size) + CHUNK_HEADER_SIZE) * (i)])


/**/
struct allocator_priv {
  int size;
  struct page page_list;
  struct allocator_priv *next;
  void (*dtor)(void *);/* sfreeした際に呼ばれる */
};

static struct allocator_priv *allocator_list;

static int bit_get(unsigned char* bits, int pos)
{
  unsigned char filter = 1 << (7 - (pos & 0x7));
  return bits[pos >> 3] & filter ? 1 : 0;
}

static int bit_test(unsigned char* bits, int pos)
{
  /*
     bit_getとほぼ同じだがbit != 0の時に0以外を返すことしか保証しない
   */
  return bits[pos >> 3] & (1 << (7 - (pos & 0x7)));
}


static int bit_set(unsigned char* bits, int pos, int bit)
{
  unsigned char filter = 1 << (7 - (pos & 0x7));
  if (bit == 0) {
    return bits[pos >> 3] &= ~filter;
  } else {
    return bits[pos >> 3] |= filter;
  }
}


static struct chunk *
get_chunk_address(void *s)
{
  return (struct chunk *)
    ((unsigned long)s - CHUNK_HEADER_SIZE);
}

static struct page *
alloc_page(int size)
{
  struct page *p;
  unsigned char* avail;
  int num = MAX_NUM(size);
    
  p = malloc(PAGE_SIZE);
  if (!p) {
    anthy_log(0, "Fatal error: Failed to allocate memory.\n");
    exit(1);
  }

  p->magic = PAGE_MAGIC;
  avail = PAGE_AVAIL(p);
  memset(avail, 0, (num >> 3) + 1);
  return p;
}

static struct chunk *
get_chunk_from_page(allocator a, struct page *p)
{
  int i;

  int num = MAX_NUM(a->size);
  unsigned char* avail = PAGE_AVAIL(p);
  unsigned char* storage = PAGE_STORAGE(p, a->size);

  for (i = 0; i < num; ++i) {
    if (bit_test(avail, i) == 0) {
      bit_set(avail, i, 1);
      return PAGE_CHUNK(p, a->size, i);
    }
  }
  return NULL;  
}

allocator
anthy_create_allocator(int size, void (*dtor)(void *))
{
  allocator a;
  if (size > (int)(PAGE_SIZE - PAGE_HEADER_SIZE - CHUNK_HEADER_SIZE)) {
    anthy_log(0, "Fatal error: too big allocator is requested.\n");
    exit(1);
  }
  a = malloc(sizeof(*a));
  if (!a) {
    anthy_log(0, "Fatal error: Failed to allocate memory.\n");
    exit(1);
  }
  a->size = size;
  a->dtor = dtor;
  a->page_list.next = &a->page_list;
  a->page_list.prev = &a->page_list;
  a->next = allocator_list;
  allocator_list = a;
  return a;
}

static void
anthy_free_allocator_internal(allocator a)
{
  struct page *p, *p_next;

  for (p = a->page_list.next; p != &a->page_list; p = p_next) {
    unsigned char* avail = PAGE_AVAIL(p);

    int limit = MAX_NUM(a->size);
    int i;

    p_next = p->next;
    if (a->dtor) {
      for (i = 0; i < limit; i++) {
	if (bit_test(avail, i)) {
	  struct chunk *c;

	  bit_set(avail, i, 0);
	  c = PAGE_CHUNK(p, a->size, i);
	  a->dtor(c->u.storage);
	}
      }
    }
    free(p);
    nr_pages--;
  }
  free(a);
}

void
anthy_free_allocator(allocator a)
{
  allocator a0, *a_prev_p;

  a_prev_p = &allocator_list;
  for (a0 = allocator_list; a0; a0 = a0->next)
    if (a == a0)
      break;
    else
      a_prev_p = &a0->next;

  *a_prev_p = a->next;
  anthy_free_allocator_internal(a);
}

void *
anthy_smalloc(allocator a)
{
  struct page *p;
  struct chunk *c;
  
  for (p = a->page_list.next; p != &a->page_list; p = p->next) {
    c = get_chunk_from_page(a, p);
    if (c) {
#ifdef MEM_DEBUG
      c->ator = a;
#endif
      return c->u.storage;
    }
  }
  p = alloc_page(a->size);
  nr_pages++;

  p->next = a->page_list.next;
  p->prev = &a->page_list;
  a->page_list.next->prev = p;
  a->page_list.next = p;
  return anthy_smalloc(a);
}

void
anthy_sfree(allocator a, void *ptr)
{
  struct chunk *c = get_chunk_address(ptr);
  struct page *p;
  int index;
  for (p = a->page_list.next; p != &a->page_list; p = p->next) {
    if ((int)p < (int)c && (int)c < (int)p + PAGE_SIZE) {
      break;
    }
  }

  if (p->magic != PAGE_MAGIC) {
    anthy_log(0, "sfree()ing Invalid Object\n");
    abort();
  }
#ifdef MEM_DEBUG
  if (a != c->ator) {
    anthy_log(0, "sfree()ing Invalid Allocator\n");
    abort();
  }
#endif

  index = ((int)c - (int)PAGE_STORAGE(p, a->size)) / (a->size + CHUNK_HEADER_SIZE);  
  bit_set(PAGE_AVAIL(p), index, 0);

  if (a->dtor) {
    a->dtor(ptr);
  }
}

void
anthy_quit_allocator(void)
{
  allocator a, a_next;
  for (a = allocator_list; a; a = a_next) {
    a_next = a->next;
    anthy_free_allocator_internal(a);
  }
  allocator_list = NULL;
}
