/** Ê¸Àá¤Î¥¯¥é¥¹ */
#ifndef _segclass_h_included_
#define _segclass_h_included_

enum dep_class {
  /* ÉÕÂ°¸ì¤Ê¤· */
  DEP_NONE,
  /* ÉÕÂ°¸ì°ìÈÌ */
  DEP_FUZOKUGO,
  /* ³Ê½õ»ì */
  DEP_KAKUJOSHI,
  /* Ï¢ÍÑ */
  DEP_RENYOU,
  /* Ï¢ÂÎ */
  DEP_RENTAI,
  /* ½ªÃ¼ */
  DEP_END,
  /* Ì¾»ìÃ±ÆÈ */
  DEP_RAW
};


enum seg_class {
  /* 0 */
  SEG_HEAD,
  SEG_TAIL,
  SEG_BUNSETSU,
  SEG_SETSUZOKUGO,
  SEG_MEISHI_KAKUJOSHI,
  SEG_MEISHI_SHUTAN,
  SEG_DOUSHI_FUZOKUGO,
  SEG_DOUSHI_SHUTAN,
  SEG_KEIYOUSHI,
  SEG_KEIYOUDOUSHI,
  /* 10 */
  SEG_RENYOU_SHUSHOKU,
  SEG_RENTAI_SHUSHOKU,
  SEG_MEISHI,
  SEG_MEISHI_FUZOKUGO,
  SEG_MEISHI_RENYOU,
  SEG_DOUSHI_RENYOU,
  SEG_DOUSHI_RENTAI,
  SEG_SIZE
};

const char* anthy_seg_class_name(enum seg_class sc);
const char* anthy_seg_class_sym(enum seg_class sc);
enum seg_class anthy_seg_class_by_name(const char *name);

#endif
