;; -*- no-byte-compile: t -*-
(register-input-method "japanese-anthy" "Japanese"
		       'anthy-leim-activate "[anthy]"
		       "Anthy Kana Kanji conversion system")

(autoload 'anthy-leim-activate "anthy")
;;
