/*
 * 構造体アロケータ
 * Funded by IPA未踏ソフトウェア創造事業 2001 8/15
 *
 * $Id: alloc.c,v 1.12 2002/05/15 11:21:10 yusuke Exp $
 *
 * Copyright (C) 2000-2002 TABATA Yusuke, UGAWA Tomoharu
 * Copyright (C) 2002 NIIBE Yutaka
 *
 * dtor: destructor
 * 
 * ページ中のフリーなchunkは単方向リストに継がれている
 * 使用中のchunkは属するページを指している
 */

#include <stdlib.h>

#include <alloc.h>
#include <logger.h>

/*#define MEM_DEBUG*/
/**/
#define PAGE_MAGIC 0x12345678

static int nr_pages;


#define PAGE_SIZE 4096
#ifdef MEM_DEBUG
#define CHUNK_HEADER_SIZE (sizeof(void *) * 2)
#else
#define CHUNK_HEADER_SIZE (sizeof(void *))
#endif
#define PAGE_HEADER_SIZE (sizeof(void *)*3+sizeof(int))

struct chunk {
  void *ptr;
#ifdef MEM_DEBUG
  void *ator;
#endif
  void *storage[1];
};

/*
 * pageのstorage中には 
 * max_obj = (PAGE_SIZE - PAGE_HEADER_SIZE)/ (size + CHUNK_HEADER_SIZE)個の
 * スロットがある。そのうちuse_count個のスロットがfree_listにつながっている、
 * もしくは使用中である。
 */
struct page {
  int magic;
  struct page *prev, *next;
  struct chunk *free_list;
  unsigned char storage[1];
};

struct allocator_priv {
  int size;
  int max_obj;
  int use_count;
  struct page page_list;
  struct allocator_priv *next;
  void (*dtor)(void *);/* sfreeした際に呼ばれる */
};

static struct allocator_priv *allocator_list;

static struct chunk *
get_chunk_address(void *s)
{
  return (struct chunk *)
    ((unsigned long)s - CHUNK_HEADER_SIZE);
}

static struct page *
alloc_page(void)
{
  struct page *p;
  p = malloc(PAGE_SIZE);
  if (!p) {
    anthy_log(0, "Fatal error: Failed to allocate memory.\n");
    exit(1);
  }
  p->magic = PAGE_MAGIC;
  p->free_list = NULL;
  return p;
}

static struct chunk *
get_chunk_from_page(allocator a, struct page *p)
{
  struct chunk *c;
  if (p->free_list) {
    c = (struct chunk *)p->free_list;
    p->free_list = c->ptr;
    c->ptr = p;
    return c;
  }
  if (p != a->page_list.next || a->use_count == a->max_obj) {
    /* It's not the first page or the first page is full. */
    return NULL;
  }
  c = (struct chunk *)
    &p->storage[a->use_count * (a->size + CHUNK_HEADER_SIZE)];
  c->ptr = p;
  a->use_count ++;
  return c;
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
  a->max_obj = (PAGE_SIZE - PAGE_HEADER_SIZE) / (size + CHUNK_HEADER_SIZE);
  a->use_count = 0;
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
  int is_first_page = 1;

  for (p = a->page_list.next; p != &a->page_list; p = p_next) {
    int limit = is_first_page ? a->use_count : a->max_obj;
    int i;

    p_next = p->next;
    if (a->dtor) {
      for (i = 0; i < limit; i++) {
	struct chunk *c;

	c = (struct chunk *)&p->storage[i * (a->size + CHUNK_HEADER_SIZE)];
	if (c->ptr == (void*)p) {
	  a->dtor(c->storage);
	}
      }
    }
    free(p);
    nr_pages--;
    is_first_page = 0;
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
      return c->storage;
    }
  }
  p = alloc_page();
  nr_pages++;

  p->next = a->page_list.next;
  p->prev = &a->page_list;
  a->page_list.next->prev = p;
  a->page_list.next = p;
  a->use_count = 0;
  return anthy_smalloc(a);
}

void
anthy_sfree(allocator a, void *ptr)
{
  struct chunk *c;
  struct page *p;
  c = get_chunk_address(ptr);
  p = (struct page *)c->ptr;
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
  if (a->dtor) {
    a->dtor(ptr);
  }
  c->ptr = p->free_list;
  p->free_list = (void *)c;
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
