#include <splitter.h>
#include <wtype.h>
#include <segclass.h>
#include "wordborder.h"

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
    if (dc == DEP_RAW) {
      seg_class = SEG_FUZOKUGO;
    } else if (dc == DEP_END) {
      seg_class = SEG_SHUTAN;
    } else if (dc == DEP_RENYOU) {
      seg_class = SEG_RENYOU;
    } else if (dc == DEP_RENTAI) {
      seg_class = SEG_RENTAI;
    } else if (dc == DEP_KAKUJOSHI) {
      seg_class = SEG_KAKUJOSHI;
    } else {
      seg_class = SEG_FUZOKUGO;
    }
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
	seg_class = SEG_DOUSHI;
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
    case POS_A:
      if (dc == DEP_RAW) {
	seg_class = SEG_KEIYOUSHI;
      } else if (dc == DEP_END) {
	seg_class = SEG_KEIYOUSHI_SHUTAN;
      } else if (dc == DEP_RENYOU) {
	seg_class = SEG_KEIYOUSHI_RENYOU;
      } else if (dc == DEP_RENTAI) {
	seg_class = SEG_KEIYOUSHI_RENTAI;
      } else {
	seg_class = SEG_KEIYOUSHI_FUZOKUGO;
      }
      break;
    case POS_AJV:
      if (dc == DEP_RAW) {
	seg_class = SEG_KEIYOUDOUSHI;
      } else if (dc == DEP_END) {
	seg_class = SEG_KEIYOUDOUSHI_SHUTAN;
      } else if (dc == DEP_RENYOU) {
	seg_class = SEG_KEIYOUDOUSHI_RENYOU;
      } else if (dc == DEP_RENTAI) {
	seg_class = SEG_KEIYOUDOUSHI_RENTAI;
      } else {
	seg_class = SEG_KEIYOUDOUSHI_FUZOKUGO;
      }
      break;
    case POS_AV:
      seg_class = SEG_FUKUSHI;
      break;
    case POS_ME:
      seg_class = SEG_RENTAISHI;
      break;
    case POS_CONJ:
      seg_class = SEG_SETSUZOKUGO;
      break;
    case POS_IJ:
      seg_class = SEG_DOKURITSUGO;
      break;
    case POS_OPEN:
      seg_class = SEG_HIRAKIKAKKO;
      break;
    case POS_CLOSE:
      seg_class = SEG_TOJIKAKKO;
      break;
    default:
      seg_class = SEG_DOKURITSUGO;
      break;
    }
  }
  wl->seg_class = seg_class;
}

int anthy_seg_class_is_depword(enum seg_class sc)
{
  if (sc == SEG_FUZOKUGO ||
      sc == SEG_KAKUJOSHI ||
      sc == SEG_RENYOU ||
      sc == SEG_RENTAI ||
      sc == SEG_SHUTAN) {
    return 1;
  } else {
    return 0;
  }
}
