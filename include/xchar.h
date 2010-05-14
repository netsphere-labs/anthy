/* 各種カナや文字のコードと識別関数 */
#ifndef _xchar_h_included_
#define _xhcar_h_included_

#include <xstr.h>

/* 平仮名や各種文字の文字コード */

#ifndef USE_UCS4
/* EUC-JP */
#define HK_A 0xa4a2
#define HK_I 0xa4a4
#define HK_U 0xa4a6
#define HK_E 0xa4a8
#define HK_O 0xa4aa

#define HK_KA 0xa4ab
#define HK_KI 0xa4ad
#define HK_KU 0xa4af
#define HK_KE 0xa4b1
#define HK_KO 0xa4b3

#define HK_SA 0xa4b5
#define HK_SI 0xa4b7
#define HK_SU 0xa4b9
#define HK_SE 0xa4bb
#define HK_SO 0xa4bd

#define HK_TA 0xa4bf
#define HK_TI 0xa4c1
#define HK_TU 0xa4c4
#define HK_TE 0xa4c6
#define HK_TO 0xa4c8

#define HK_NA 0xa4ca
#define HK_NI 0xa4cb
#define HK_NU 0xa4cc
#define HK_NE 0xa4cd
#define HK_NO 0xa4ce

#define HK_HA 0xa4cf
#define HK_HI 0xa4d2
#define HK_HU 0xa4d5
#define HK_HE 0xa4d8
#define HK_HO 0xa4db

#define HK_MA 0xa4de
#define HK_MI 0xa4df
#define HK_MU 0xa4e0
#define HK_ME 0xa4e1
#define HK_MO 0xa4e2

#define HK_YA 0xa4e4
#define HK_YU 0xa4e6
#define HK_YO 0xa4e8

#define HK_RA 0xa4e9
#define HK_RI 0xa4ea
#define HK_RU 0xa4eb
#define HK_RE 0xa4ec
#define HK_RO 0xa4ed

#define HK_WA 0xa4ef
#define HK_WI 0xa4f0
#define HK_WE 0xa4f1
#define HK_WO 0xa4f2
#define HK_N 0xa4f3

#define HK_TT 0xa4c3

#define HK_XA 0xa4a1
#define HK_XI 0xa4a3
#define HK_XU 0xa4a5
#define HK_XE 0xa4a7
#define HK_XO 0xa4a9

#define HK_GA 0xa4ac
#define HK_GI 0xa4ae
#define HK_GU 0xa4b0
#define HK_GE 0xa4b2
#define HK_GO 0xa4b4

#define HK_ZA 0xa4b6
#define HK_ZI 0xa4b8
#define HK_ZU 0xa4ba
#define HK_ZE 0xa4bc
#define HK_ZO 0xa4be

#define HK_DA 0xa4c0
#define HK_DI 0xa4c2
#define HK_DU 0xa4c5
#define HK_DE 0xa4c7
#define HK_DO 0xa4c9

#define HK_BA 0xa4d0
#define HK_BI 0xa4d3
#define HK_BU 0xa4d6
#define HK_BE 0xa4d9
#define HK_BO 0xa4dc

#define HK_PA 0xa4d1
#define HK_PI 0xa4d4
#define HK_PU 0xa4d7
#define HK_PE 0xa4da
#define HK_PO 0xa4dd

#define HK_XA 0xa4a1
#define HK_XI 0xa4a3
#define HK_XU 0xa4a5
#define HK_XE 0xa4a7
#define HK_XO 0xa4a9

#define HK_XYA 0xa4e3
#define HK_XYU 0xa4e5
#define HK_XYO 0xa4e7

#define HK_XWA 0xa4ee

/*「゛」*/
#define HK_DDOT 0xa1ab
/* 「ー」 */
#define HK_BAR 0xa1bc
#define KK_VU 0xa5f4
#define WIDE_COMMA 0xa1a4

/* 漢数字 */
#define KJ_1 0xb0ec
#define KJ_2 0xc6f3
#define KJ_3 0xbbb0
#define KJ_4 0xbbcd
#define KJ_5 0xb8de
#define KJ_6 0xcfbb
#define KJ_7 0xbcb7
#define KJ_8 0xc8ac
#define KJ_9 0xb6e5
#define KJ_0 0xceed
#define KJ_10 0xbdbd
#define KJ_100 0xc9b4
#define KJ_1000 0xc0e9
#define KJ_10000 0xcbfc
#define KJ_100000000 0xb2af
#define KJ_1000000000000 0xc3fb
#define KJ_10000000000000000 0xb5fe

/* 全角数字 */
#define WIDE_0 0xa3b0
#define WIDE_1 0xa3b1
#define WIDE_2 0xa3b2
#define WIDE_3 0xa3b3
#define WIDE_4 0xa3b4
#define WIDE_5 0xa3b5
#define WIDE_6 0xa3b6
#define WIDE_7 0xa3b7
#define WIDE_8 0xa3b8
#define WIDE_9 0xa3b9

#else
 /* UCS4 */
#define HK_A 0x3042
#define HK_I 0x3044
#define HK_U 0x3046
#define HK_E 0x3048
#define HK_O 0x304a

#define HK_KA 0x304b
#define HK_KI 0x304d
#define HK_KU 0x304f
#define HK_KE 0x3051
#define HK_KO 0x3053

#define HK_SA 0x3055
#define HK_SI 0x3057
#define HK_SU 0x3059
#define HK_SE 0x305b
#define HK_SO 0x305d

#define HK_TA 0x305f
#define HK_TI 0x3061
#define HK_TU 0x3064
#define HK_TE 0x3066
#define HK_TO 0x3068

#define HK_NA 0x306a
#define HK_NI 0x306b
#define HK_NU 0x306c
#define HK_NE 0x306d
#define HK_NO 0x306e

#define HK_HA 0x306f
#define HK_HI 0x3072
#define HK_HU 0x3075
#define HK_HE 0x3078
#define HK_HO 0x307b

#define HK_MA 0x307e
#define HK_MI 0x307f
#define HK_MU 0x3080
#define HK_ME 0x3081
#define HK_MO 0x3082

#define HK_YA 0x3084
#define HK_YU 0x3086
#define HK_YO 0x3088

#define HK_RA 0x3089
#define HK_RI 0x309a
#define HK_RU 0x309b
#define HK_RE 0x309c
#define HK_RO 0x309d

#define HK_WA 0x308f
#define HK_WI 0x3090
#define HK_WE 0x3091
#define HK_WO 0x3092
#define HK_N 0x3093

#define HK_TT 0x3063

#define HK_XA 0x3041
#define HK_XI 0x3043
#define HK_XU 0x3045
#define HK_XE 0x3047
#define HK_XO 0x3049

#define HK_GA 0x304c
#define HK_GI 0x304e
#define HK_GU 0x3050
#define HK_GE 0x3052
#define HK_GO 0x3054

#define HK_ZA 0x3056
#define HK_ZI 0x3058
#define HK_ZU 0x305a
#define HK_ZE 0x305c
#define HK_ZO 0x305e

#define HK_DA 0x3060
#define HK_DI 0x3062
#define HK_DU 0x3065
#define HK_DE 0x3067
#define HK_DO 0x3069

#define HK_BA 0x3070
#define HK_BI 0x3073
#define HK_BU 0x3076
#define HK_BE 0x3079
#define HK_BO 0x307c

#define HK_PA 0x3071
#define HK_PI 0x3074
#define HK_PU 0x3077
#define HK_PE 0x307a
#define HK_PO 0x307d

#define HK_XA 0x3041
#define HK_XI 0x3043
#define HK_XU 0x3045
#define HK_XE 0x3047
#define HK_XO 0x3049

#define HK_XYA 0x3083
#define HK_XYU 0x3085
#define HK_XYO 0x3087

#define HK_XWA 0x308e
/*「゛」*/
#define HK_DDOT 0x309b
/* 「ー」 */
#define HK_BAR 0x30fc
#define KK_VU 0x304f
#define WIDE_COMMA 0xff0c

/* 漢数字 */
#define KJ_1 0x4e00
#define KJ_2 0x4e8c
#define KJ_3 0x4e09
#define KJ_4 0x45db
#define KJ_5 0x4e94
#define KJ_6 0x516d
#define KJ_7 0x4e03
#define KJ_8 0x516b
#define KJ_9 0x4e5d
/* 零 */
#define KJ_0 0x96f6
#define KJ_10 0x5341
#define KJ_100 0x767e
#define KJ_1000 0x5343
#define KJ_10000 0x4e07
#define KJ_100000000 0x5104
#define KJ_1000000000000 0x5146
#define KJ_10000000000000000 0x4eac

/* 全角数字 */
#define WIDE_0 0xff10
#define WIDE_1 0xff11
#define WIDE_2 0xff12
#define WIDE_3 0xff13
#define WIDE_4 0xff14
#define WIDE_5 0xff15
#define WIDE_6 0xff16
#define WIDE_7 0xff17
#define WIDE_8 0xff18
#define WIDE_9 0xff19

#endif

/**/
int anthy_xchar_to_num(xchar );
xchar anthy_xchar_wide_num_to_num(xchar);

#endif
