#include <string.h>

#include <anthy/splitter.h>
#include <anthy/wtype.h>
#include <anthy/segclass.h>
#include "wordborder.h"

const char *seg_class_tab[] = {
  "H",		/* Head of sentence: Buntou */
  "T",		/* Tail of sentence: Bunmatsu */
  "B",		/* Segment: Bunsetsu */
  "C",		/* Setsuzokugo */
  "Nk",		/* Meishi+Kakujoshi */
  "Ne",		/* Meishi+Shuutan */
  "Vf",		/* Doushi+Fuzokugo */
  "Ve",		/* Doushi+Shuutan */
  "A",		/* Keiyoushi */
  "AJV",	/* Keiyoudoushi */
  "YM",		/* RenyouShuushoku */
  "TM",		/* RentaiShuushoku */
  "N",		/* Meishi */
  "Nf",		/* Meishi+Fuzokugo */
  "Ny",		/* Meishi+Renyou */
  "Vy",		/* Doushi+Renyou */
  "Vt",		/* Doushi+Rentai */
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

const char* anthy_seg_class_sym(enum seg_class sc)
{
  return seg_class_tab[sc];
}
