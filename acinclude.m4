# adding Id cause confusion
# Autoconf の互換マクロ
# 新しい Autoconf (2.5x) の環境では ifdef も define も
# undefine されているので、これは無視される
ifdef([AC_HELP_STRING], [],
  [define([AC_HELP_STRING], [builtin([format], [  %-22s  %s], [$1], [$2])])])
