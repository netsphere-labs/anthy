********************
     cannadic改
********************


[ 概要 ]

Anthy、Canna 用の自家製変換辞書です。
cannadic-0.95c をベースに、大幅に手を加えてあります。

エントリ数 (2009/12/30)
Canna         : 247,869
Anthy all     : 270,531
      (main)  : 244,294
      (extra) :  26,237

※変換精度の確認は Anthy で行い、Canna では行っていません。


[ ライセンス ]

cannadic がベースなので、GPL を継承します。
言うまでもありませんが、保証の類は一切ありません。


[ ファイル ]

  (A)  - Anthy用
  (C)  - Canna用
  (AC) - Anthy, Canna共用
  ※一応、末尾が「.t」の辞書は Anthy用と思って下さい

./
  Changes.txt         変更履歴
  README_euc.txt      このファイル

  辞書ファイル: すべて EUC-JP
  gcanna.ctd          自立語辞書(AC)
  gcannaf.ctd         付属語辞書(AC)
  gtankan.ctd         単漢字辞書(送り仮名なしのもの)(AC)
  gt_okuri.ctd        単漢字辞書(送り仮名あり)(C)
  g_fname.ctd         人名フルネーム辞書(C)
  g_fname.t           人名フルネーム辞書(A)

  ※以下のものは cannadic-0.95c そのままのものです。
  COPYING             ライセンス
  Makefile            Canna 用 Makefile
  orig-README.ja      cannadic-0.95c の README

extra/
  (extra/README 参照)

anthy/
  以下のファイルは anthy のものと置き換えて使います。
  mkworddic/compound.t     複合語辞書(修正版)
  mkworddic/extra.t        anthy 独自の品詞コードのもの
  mkworddic/dict.args.in   どの辞書を読み込むかの設定
  calctrans/corpus_info    コーパスパラメータ
  calctrans/weak_words     コーパスパラメータ

./prepare.sh は、このパッケージ内の辞書を使って anthy をビルド
  するための準備をするスクリプトです。
  単に、anthy/ 以下にあるファイルをオリジナルのものと置き換えて
  るだけです。
  以下のような感じで使います:

  $ tar xzf anthy-9100h.tar.gz
  $ tar xjf alt-cannadic-091230.tar.bz2
  $ cd alt-cannadic-091230/
  $ ./prepare.sh
  $ cd ../anthy-9100h/
  $ ./configure && make
  ...

[ 使い方 ]

Wiki を参照してください。
http://sourceforge.jp/projects/alt-cannadic/wiki/


[ cannadic からの主な変更点 ]

○cannadic-0.95c 以外に利用させて頂いた電子ファイルは以下です。

　・単漢字部分
　　「jisx0213 infocenter」(http://www.jca.apc.org/~earthi
    an/aozora/0213.html)の「漢字音訓索引(onkun0213.txt
    [2000-09-11])」
　・地名
　　日本郵政公社のデータファイル(平成17年6月30日更新版)
    (http://www.post.japanpost.jp/zipcode/dl/kogaki.html)
  ・anthy-7100b の base.t, katakana.t, placename.t
　・SKK-JISYO.L(2008/10/15 Download)

　ライセンスがややこしくなるのが嫌だったので、上記以外には
「他の電子辞書ファイルから引っ張ってきて突っ込む」という
ことはしていません。Canna 付属の辞書すら避けました。


○ほぼ全品詞を強化。主なものは以下
   ・敬語丁寧語表現強化(「お〜」「ご〜」、時候挨拶等。まだ
     まだ十分とは言えない)
   ・ことわざ、慣用表現強化
   ・動詞、形容詞の整備(誤りの修正や表記の整備)。
     また、多くの追加を行い、動詞と形容詞に関しては基本的な
     ものはかなりカバーできたと思う。
     (「基本的な」というのは「複合語でない」という意味です)
   ・単漢字の漢字部分を完全再作成
     (jisx0213 infocenterの「漢字音訓索引」がベース)
   ・副詞/形容動詞の強化と整理。普通名詞に含まれていたもの
     を副詞/形容動詞として登録し直したり、擬声擬態を表す語
     を拡充したり等。
   ・地名強化
     日本郵政公社の「ゆうびんホームページ」にあるデータ
     ファイル(平成17年6月30日更新版)を利用。
     また、「MAPOO」(http://www.mapoo.or.jp/station/)
     で調べた全国の駅名を追加。
   ・付属語全般強化

 ・cannadic にあった誤りを気づいた範囲で修正、また無駄な重
   複エントリを削除(修正削除はかなりの数を行ったが、まだまだ
   誤りは残っている。特に普通名詞あたりは伏魔殿。また、私が
   新たに入れてしまった誤りもあるかと思います)
 ・頻度の調整(まだまだ見てない部分が殆ど)
 ・「N2〜」「D2〜」のタイプのものは、「ありえない候補を作
   ってしまう」「その場合、区切り直さなければ出したいものが
   出せない」というデメリットの方が大きいと思われるので、
   今のところほぼ外してある → 必要そうなものは戻したが、
　 現在の Anthy では使えないようになっている
 ・「う゛ぁう゛ぃう゛う゛ぇう゛ぉ」でも「ヴぁヴぃヴヴぇヴぉ」
   でも出せるように、読みの「う゛」を「ヴ」に置換した候補を
   追加(例えば「ヴぃーなす」で「ヴィーナス」を出せるように)

 ・品詞コードはすべて Canna の品詞コードの範囲内に留め、
   Anthy 独自のものは使っていない。
 ・Canna の品詞コードのうちでも形容詞の「mi」「me」「mime」
   や連用形が名詞化することを表す動詞の「r」など、煩瑣なく
   せにあまり意味のなさそうなコードは使わなくした。
   形容詞の「mi」「me」、動詞の「r」は別途名詞として登録。
 ・人名はすべて JN に、地名はすべて CN に統一した。
   敬語表現 OKX も名詞のコードで登録してある(「する」への
　 接続をコントロールしたかったから)。
 ・主要な品詞の頻度を一括で変更(まだ実験中)


[ 謝辞 ]
作成にあたり恩恵を受けました以下の方々に感謝いたします。
・ベースとさせて頂いた cannadic を編纂されたすぎもとさん
・Canna 及び Anthy の開発陣の方々
・「漢字音訓索引(onkun0213.txt [2000-09-11])」を作成/公開
　してくださった方々
・郵便番号辞書を公開してくださっている日本郵政公社殿
・endo-h さんの「pudic+ 補遺」も参考にさせて頂きました
  http://www.remus.dti.ne.jp/~endo-h/wnn/#supplement
・ICOTの「形態素辞書」(morphdic)も参考にさせて頂きました
  http://www.icot.or.jp/ARCHIVE/Museum/IFS/abst/033-J.html
・UTUMI さんの私家版 gcanna.ctd(20061121)
  http://www.geocities.jp/ep3797/snapshot/anthy_dict/anthy_gcanna_ut-20061121.tar.bz2

また、公開に先立ち、UTUMI Hirosi さんから、頻度の調整、新語
や誤りに関して多くの情報とご助言を頂きました。感謝いたします。

次の方々からも誤登録の指摘、異表記/新語追加の点で多くの
ご助力をいただきました。ありがとうございます。
  denson さん
  井汲景太 さん
  登録希望 さん
  ishii さん
  nosuke さん
  n/a さん
  2ch の匿名の方々
  G-HAL さん
  salvan さん
  Awashiro_ikuya さん
  Tonibi_ko さん
  x さん
  TAKADA Yoshihito さん

[ 連絡先 ]
何かありましたら下記まで。
  vagus.xyz あっと gmail.com









