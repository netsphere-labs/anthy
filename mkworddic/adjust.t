#
# 頻度を補正するコマンド
#
#
# \modify_freq よみ 品詞 単語 {up, down, kill}
#  頻度を調整する
#
# 変換結果からadjust_commandの列を生成して、それを
# 組み込んで再度辞書を生成することにより、高い変換精度を
# 実現することを目標とする。
#
#例
\modify_freq いらく #CN イラク up
\modify_freq いらん #CN イラン up
\modify_freq げつよう #T35 月曜 up
\modify_freq きんよう #T35 金曜 up
\modify_freq ちかく #T35 近く up
\modify_freq うえ #T35 上 up
\modify_freq ぱんや #T35 パン屋 up
\modify_freq つき #T35 月 up
\modify_freq はんしん #KK 阪神 up
\modify_freq がつ #KJ 月 up
\modify_freq おおさか #CN 大阪 up
\modify_freq こうち #CN 高知 up
\modify_freq きょうと #CN 京都 up
\modify_freq こうべ #CN 神戸 up
\modify_freq ながの #CN 長野 up
\modify_freq かんこく #CN 韓国 up
\modify_freq のは #CN 饒波 down
\modify_freq のは #JNS 饒波 down
\modify_freq して #T35 仕手 down
\modify_freq ど #T35 度 down
\modify_freq にすき #T35 荷隙 down
\modify_freq たき #T35 瀧 down
\modify_freq なし #N2T35 なし kill
\modify_freq なし #N2T35 無し kill
\modify_freq あの #RT あの up
\modify_freq この #RT この up
\modify_freq その #RT その up
\modify_freq どの #RT どの up
\modify_freq おおきな #RT 大きな up
\modify_freq どんな #T05 どんな kill
\modify_freq で #T35 出 down
\modify_freq ある #aru up
\modify_freq よそう #T30 予想 up
\modify_freq くろう #T30 苦労 up
\modify_freq ゆうよう #T05 有用 up
\modify_freq せいと #T35 生徒 up
\modify_freq こぶし #T35 拳 up
\modify_freq つごう #T30 都合 up
\modify_freq かれんと #T35 カレント up
\modify_freq ちゅうしゃ #T35 駐車中 kill
\modify_freq ちょく #T35 猪口 down
\modify_freq ついたち #T35 一日 up
\modify_freq つうふう #T35 痛風 down
\modify_freq とにかく #F14 とにかく up
\modify_freq まくら #T35 枕 up
\modify_freq たいしょうせいやく #KK 大正製薬 up
\modify_freq ぴあの #T35 ピアノ up
\modify_freq ぴあ #T35 ピア down
\modify_freq ぴあ #T35 ぴあ down
\modify_freq おやばか #T05 親馬鹿 down
\modify_freq こうじょう #T35 工場 up
\modify_freq よみせ #T35 夜見世 down
\modify_freq がくしょく #T35 学殖 down
\modify_freq おもいで #T35 想い出 down
\modify_freq の #R5 載 down
\modify_freq こだわり #T35 こだわり up
\modify_freq はらわた #T35 腸 down
\modify_freq はんしん #CN 阪神 up
\modify_freq こなみ #KK コナミ up
\modify_freq ぼけ #KSr 惚け down
\modify_freq にがり #T35 苦塩 down
\modify_freq まくのうち #T35 幕の内 up
\modify_freq しゅく #S5 宿 down
\modify_freq う #T35 鵜 down
\modify_freq やせがた #T35 痩せ形 kill
\modify_freq まけ #KSr 負け up
\modify_freq かみがか #R5r 神憑 down
\modify_freq たかはし #JNS 高橋 up
\modify_freq おしあいへしあい #T30 押し合いへし会い kill
\modify_freq しょくざい #T35 食材 up
\modify_freq こうざ #T35 講座 up
\modify_freq しんしゅう #CN 信州 up
\modify_freq ひよけ #T35 日よけ up
\modify_freq ましょう #T35 魔障 down
\modify_freq こうがくきかい #T35 光学機械 kill
\modify_freq ひまらや #CN ヒマラヤ up
\modify_freq ひまらや #CN ヒマラヤ up
\modify_freq はっこういちう #T35 八絋一宇 kill
\modify_freq おもいで #T35 想い出 down
\modify_freq おもしろみ #T35 おもしろみ down
\modify_freq さんぱつ #T30 散髪 up
\modify_freq こうし #T35 講師 up
\modify_freq もの #N2T35 物 kill
\modify_freq ちゅう #T35 中 down
\modify_freq この #JNM この kill

