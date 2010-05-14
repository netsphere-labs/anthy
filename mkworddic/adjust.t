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
#\mofidy_freq のし #T35 のし down
# 「ながいじかん」が「永井次官」になる
\modify_freq じかん #JNSUC 次官 kill
