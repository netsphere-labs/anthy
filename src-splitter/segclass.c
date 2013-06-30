#include <string.h>

#include <anthy/splitter.h>
#include <anthy/wtype.h>
#include <anthy/segclass.h>
#include "wordborder.h"

static struct {
  const char *name;
  const char *sym;
} seg_class_tab[] = {
  {"Ê¸Æ¬", "H"}, {"Ê¸Ëö", "T"}, {"Ê¸Àá", "B"},
  {"ÀÜÂ³¸ì", "C"}, {"Ì¾»ì+³Ê½õ»ì", "Nk"}, {"Ì¾»ì+½ªÃ¼", "Ne"},
  {"Æ°»ì+ÉÕÂ°¸ì", "Vf"}, {"Æ°»ì+½ªÃ¼", "Ve"}, {"·ÁÍÆ»ì", "A"},
  {"·ÁÍÆÆ°»ì", "AJV"},
  {"Ï¢ÍÑ½¤¾þ", "YM"}, {"Ï¢ÂÎ½¤¾þ", "TM"},
  {"Ì¾»ì", "N"}, {"Ì¾»ì+ÉÕÂ°¸ì", "Nf"}, {"Ì¾»ì+Ï¢ÍÑ", "Ny"},
  {"Æ°»ì+Ï¢ÍÑ", "Vy"},
  {"Æ°»ì+Ï¢ÂÎ", "Vt"},
  {NULL, NULL}
};

void
anthy_set_seg_class(struct word_list* wl)
{
  int head_pos;
  enum dep_class dc;
  enum seg_class seg_class;

  if (!wl) return;

  head_pos = wl->head_pos;
  dc = wl->part[PART_DEPWORD].dc;
  seg_class = SEG_HEAD;

  if (wl->part[PART_CORE].len == 0) {
    seg_class = SEG_BUNSETSU;
  } else {
    switch (head_pos) {
    case POS_NOUN:
    case POS_NUMBER:
      /* BREAK THROUGH */
    case POS_N2T:
      if (dc == DEP_RAW) {
	seg_class = SEG_MEISHI;
      } else if (dc == DEP_END) {
	seg_class = SEG_MEISHI_SHUTAN;
      } else if (dc == DEP_RENYOU) {
	seg_class = SEG_MEISHI_RENYOU;
      } else if (dc == DEP_KAKUJOSHI) {
	seg_class = SEG_MEISHI_KAKUJOSHI;
      } else {
	seg_class = SEG_MEISHI_FUZOKUGO;
      }
      break;
    case POS_V:
      if (dc == DEP_RAW) {
	seg_class = SEG_BUNSETSU;
      } else if (dc == DEP_END) {
	seg_class = SEG_DOUSHI_SHUTAN;
      } else if (dc == DEP_RENYOU) {
	seg_class = SEG_DOUSHI_RENYOU;
      } else if (dc == DEP_RENTAI) {
	seg_class = SEG_DOUSHI_RENTAI;
      } else {
	seg_class = SEG_DOUSHI_FUZOKUGO;
      }
      break;
    case POS_D2KY:
      /* BREAK THROUGH */
    case POS_A:
      seg_class = SEG_KEIYOUSHI;
      if (dc == DEP_RENYOU) {
	seg_class = SEG_RENYOU_SHUSHOKU;
      } else if (dc == DEP_RENTAI) {
	seg_class = SEG_RENTAI_SHUSHOKU;
      }
      break;
    case POS_AJV:
      seg_class = SEG_KEIYOUDOUSHI;
      if (dc == DEP_RENYOU) {
	seg_class = SEG_RENYOU_SHUSHOKU;
      } else if (dc == DEP_RENTAI) {
	seg_class = SEG_RENTAI_SHUSHOKU;
      }
      break;
    case POS_AV:
      seg_class = SEG_RENYOU_SHUSHOKU;
      break;
    case POS_ME:
      seg_class = SEG_RENTAI_SHUSHOKU;
      break;
    case POS_CONJ:
      seg_class = SEG_SETSUZOKUGO;
      break;
    case POS_OPEN:
      seg_class = SEG_BUNSETSU;
      break;
    case POS_CLOSE:
      seg_class = SEG_BUNSETSU;
      break;
    default:
      seg_class = SEG_MEISHI;
      break;
    }
  }
  wl->seg_class = seg_class;
}

const char* anthy_seg_class_name(enum seg_class sc)
{
  return seg_class_tab[sc].name;
}

const char* anthy_seg_class_sym(enum seg_class sc)
{
  return seg_class_tab[sc].sym;
}

enum seg_class
anthy_seg_class_by_name(const char *name)
{
  int i;
  for (i = 0; seg_class_tab[i].name; i++) {
    if (!strcmp(seg_class_tab[i].name, name)) {
      return i;
    }
  }
  return SEG_BUNSETSU;
}
