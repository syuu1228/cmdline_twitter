cmdline_twitter
===============
Linuxのターミナルなどで使えるTwitterクライアントです。
1ラインで投稿とかできるので、cronと併用してBOT的な使い方もできます。


使い方
===============
コンシューマキーとシークレットの取り扱いの関係上、バイナリはありません。
使うにはまず「コンパイル」してください。

コンパイル
--------

cmdline_twitterは以下のパッケージを使用しています。
* g++
* STL(libstdc++)
* Boost
* libcurl-dev (opensslでもnssでも何でもいい)

ので、あらかじめインストールしてください。

Ubuntuの場合は次のようにしたら良いかもしれません。
````
$ sudo apt-get install g++ boost-all libstdc++6 libboost-all-dev libcurl4-nss-dev
````

次に、http://blog.uklab.jp/web/add-application-for-twitter/ のあたりを参考に、Twitterのdepeloper登録を行い、適当にアプリケーションの登録を行ってください。
コールバックURLは空欄でかまいません。

次に、sample_setkey.shを開いて、CONSUMER_KEY と CONSUMER_SECRET を書き換えます。
書き換え終わったら、
````
$ . ./sample_setkey.sh
$ make
````
でクライアントが作成されます。
. ./sample_setkey.sh は環境変数を設定しています。
一度設定したら、ターミナルを閉じるまでしなくてOKです。


コマンドライン
--------
コマンドは ctw です。
以下のオプションがあります。

* -h | --help          Print this message
* -a | --auth          [再]認証を行う -u オプションでエイリアスを指定できます
* -p | --post status   タイムラインへ投稿
* -s | --search word   ワードで検索
* -r | --readhome      ホームのタイムラインを読む
* -u | --user alies    エイリアス名指定:省略可(-a とも併用可能
* -v | --verbose       (デバッグ用)余計な文字を出力しまくる

まず認証する必要があるので、-a オプションで認証をしてください。
-u オプションでユーザエイリアスを指定することができます。(指定しなくても運用はできます)
エイリアスは複数アカウントを運用するときに使うと便利です。

ユーザのトークンキーとシークレットは /home/現在のLinuxログインユーザ名/.ctw/ 以下に保存されます。
(~/.ctw/ 以下です)
.authkey_エイリアス名 がそれですので、取り扱いには注意してください。

例示
--------
* エイリアス名eggで認証する<br>
ctw -u egg -a

* エイリアス名eggで投稿<br>
ctw -u egg -p "i am egg"

* エイリアス名eggのHomeタイムラインを見る<br>
ctw -u egg -r

* ｢github｣という投稿があるかどうか検索する<br>
ctw -s github


制限とか
--------
Homeタイムラインの取得はREST APIを使っています。
つまり、UserStreamではないので一分間に15回以上やるとエラーになります。

ろくにエラーチェックしてません。投稿がミスってもしれっとしてます。



TODO
===============
* もうちょっと機能を追加する(RTとか@はどう表現すればよいかな…)
* 画像系アップロードへ対応
* 対話モードの追加
* なるべくライブラリに依存しない
* 見栄えをよくする
* 色指定できるようにする
* UserStream対応
* 他の環境への対応

謝辞
===============
* picojsonのkazuho氏 (https://github.com/kazuho/picojson)<br>
JSONの解析に使用しています。picojsonはSTLの親和性高くて便利です

