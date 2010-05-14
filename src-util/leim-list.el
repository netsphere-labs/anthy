;;
(register-input-method "japanese-anthy" "Japanese"
		       'anthy-leim-activate "[anthy]"
		       "Anthy Kana Kanji conversion system")
(dont-compile
  (require 'anthy))
;;
