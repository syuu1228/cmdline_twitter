cmdline_twitter
===============
Linuxのターミナルなどで使えるTwitterクライアントです。
MSYS2のターミナル(minty)でも使用できます。
1ラインで投稿とかできるので、cronと併用してBOT的な使い方もできます。


使い方
===============
コンシューマキーとシークレットの取り扱いの関係上、バイナリはありません。
使うにはまず「コンパイル」してください。
環境はLinux(Ubuntuで確認)とWindows(MSYS2を使用)でコンパイル可能です。

コンパイル
--------

cmdline_twitterは以下のパッケージを使用しています。
* g++
* STL(libstdc++)
* libcurl-dev (opensslでもnssでも何でもいい)

ので、あらかじめインストールしてください。

Ubuntuの場合は次のようにしたら良いかもしれません。
````
$ sudo apt-get install g++ libstdc++6 libcurl4-nss-dev
````

Windows環境(MSYS2)の場合は次のようにすると良いかもしれません
````
$ pacman -Si mingw-w64-i686-gcc  mingw-w64-i686-gcc-libs libcurl  libcurl-devel
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
* -a | --auth          [再]認証を行う<br>
                     -u オプションでエイリアスを指定できます<br>
* -p | --post status   タイムラインへ投稿
* -s | --search word   ワードで検索
* -r | --readtl        ホームのタイムラインを読む<br>
                     -n オプションでユーザ名指定すると指定ユーザを読む<br>
                     -n オプションで""と指定すると自分の発言を読む<br>
                     -n オプションで"@"と指定すると自分へのメンションを読む<br>
* -d | --del           発言の削除 -iでID指定
* -R | --Retweet       リツイートする -iでID指定
* -F | --Fav           お気に入りに追加する -iでID指定
* -l | --list name     自分のリストnameの内容を読む<br>
                     nameで""と指定すると自分のリスト一覧を表示する<br>
* -n | --name          指定が必要な場合のユーザスクリーンネーム
* -i | --id            指定が必要な場合の発言ID
* -u | --user alies    エイリアス名指定:省略可(-a とも併用可能)
* -T | --Test          (テスト用)APIのエンドポイントを指定してAPIリクエストを行う
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

今のところ、Homeタイムラインの表示でRTや@は表示しない仕様になっています。

MSYS2環境では発言のタイムスタンプが標準時刻になっています。


TODO
===============
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

