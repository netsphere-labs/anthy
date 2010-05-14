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
