//
// コマンドラインTwitter
//
// The MIT License (MIT)
//
// Copyright (c) <2014> chromabox <chromarockjp@gmail.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <getopt.h>

#include "include/typedef.hpp"
#include "twitter_client.hpp"
#include "keys/apikeys.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

using namespace std;

// 設定ファイル保存先
static const string	DEFAULT_AUTH_FILE = ".authkey_";
static const string	APP_DIR = ".ctw";

static bool do_Verbose = false;		// -vが指定されるとTRUE

// アプリ設定ファイル用ディレクトリを取得＆作成
static bool get_app_dir(string &dirname)
{
	struct stat st;
	dirname = getenv("HOME");
	dirname += '/';
	dirname += APP_DIR;
	
	if(stat(dirname.c_str(),&st) != 0){
		if(mkdir(dirname.c_str(),0700) != 0){
			return false;
		}
	}
	dirname += '/';
	
	return true;

}

// 一連の認証動作を行う
void do_Authentication(TwitterClient &client,const string &aries)
{
	string rurl;
	string pincode;
	
	cout << "認証手続きをしています..." << endl;
	if(!client.Authentication_GetURL(rurl)){
		return;
	}
	cout << "以下のURLにアクセスして認証後、PINを入力してください" << endl;
	cout << rurl << endl;
	cin >> pincode;
	cout << "認証中です..." << endl;
	if(!client.Authentication_Finish(pincode)){
		cout << "認証に失敗しました。再度認証しなおしてください" << endl;
		return;
	}
	cout << "認証に成功しました" << endl;
	
	string key,sec,fname;
	ofstream fout;
	
	// ~/.ctw/以下に保存する
	if(! get_app_dir(fname)){
		cout << "HOME/.ctw/ ディレクトリを作成できません。" << endl;
		return;
	}
	fname += DEFAULT_AUTH_FILE;
	if(! aries.empty()) fname += aries;
	
	client.getUserAccessPair(key,sec);
	fout.open(fname.c_str(),ios::out);
	if(! fout.is_open()){
		cout << "設定ファイルを開けませんでした。" << endl;
		cout << "再度認証しなおしてください" << endl;
		
		return;
	}
	fout << key << endl;
	fout << sec << endl;
	fout.close();
	return;
}

// 保存している認証コードを読みこむ
bool readAccessKey(TwitterClient &client,const string &aries)
{
	string key,sec,fname;
	ifstream fin;
	
	// ~/.ctw/以下から読み込みする
	if(! get_app_dir(fname)){
		cout << "HOME/.ctw/ ディレクトリを作成できません。" << endl;
		return false;
	}
	fname += DEFAULT_AUTH_FILE;
	if(! aries.empty()) fname += aries;
	fin.open(fname.c_str());
	if(! fin.is_open()){
		cout << "設定ファイルを開けませんでした。" << endl;
		cout << "指定されたエイリアス名が間違っているかもしれません。" << endl;
		cout << "または、--authオプションで認証する必要があります" << endl;
		return false;
	}
	
	fin >> key;
	fin >> sec;
	fin.close();
	if(key.empty() || sec.empty()){
		cout << "設定ファイルが壊れています。再度認証してください" << endl;
		return false;
	}
	client.setUserAccessPair(key,sec);
	return true;
}

// Twitterでは標準時刻(UTC)が記述されているのでこれを現地時刻に直す
inline static void get_local_time_string(const std::string &src,std::string &dst)
{
	string tmstr;
	time_t tm;
	struct tm tm_src,tm_dest;
	char tmpstr[64];
	
	memset(&tm_src,0,sizeof(struct tm));
	memset(&tm_dest,0,sizeof(struct tm));
	
	// 文字形式のをまずは変換
	strptime(src.c_str(),"%a %b %d %T %z %Y",&tm_src);
	// 現地時間に直す
	tm = timegm(&tm_src);		// mktimeはtmがローカルである場合だけ使える。この場合はこれ
	localtime_r(&tm,&tm_dest);
	
	strftime(tmpstr,sizeof(tmpstr),"[%Y-%m-%d %T]",&tm_dest);
	dst = tmpstr;
}

// タイムラインを実際に出力する。検索結果表示やらHOME表示やらに使ってる
static void printTimeline(picojson::array &timeline)
{
	using namespace picojson;
	using namespace std;
	
	string tmstr;

	// Twitterからの戻りは先が最新なので、逆順に表示
	array::reverse_iterator it;
	
	for(it=timeline.rbegin();it!=timeline.rend();it++){
		object obj = it->get<object>();
		object uobj = obj["user"].get<object>();	// 要素が中にある場合はこれ
		get_local_time_string(obj["created_at"].to_str(),tmstr);
		cout << "\033[32m";
		cout << uobj["name"].to_str() << " @" << uobj["screen_name"].to_str() <<" " << tmstr << endl;
		cout << "\033[37m";
		cout << obj["text"].to_str() << endl;
	}
	cout << "\033[0m";
}



// ユーザ情報の初期化
bool initUserInfo(TwitterClient &client)
{
	picojson::object info;
	// ただユーザ名とスクリーンネームとIDが知りたいだけ
	if(! client.verifyAccount(info)){
		return false;
	}
	return true;
}

// タイムラインを読む
void ReadHomeTimeline(TwitterClient &client)
{
	picojson::array timeline;
	if(! client.getHomeTimeline(
		200,
		"",
		"",
		false,			// RT含めない
		false,			// @含めない
		timeline)
	){
		return;
	}
	printTimeline(timeline);
}

// 指定ユーザのタイムラインを読む
void ReadUserTimeline(TwitterClient &client,const std::string &name)
{
	picojson::array timeline;
	
	if(name.empty()){
		if(! client.getMyTimeline(
			200,
			"",
			"",
			true,			// RT含める
			true,			// @含める
			timeline)
		){
			return;
		}
	}else{
		if(! client.getUserTimeline(
			"",
			name,
			200,
			"",
			"",
			true,			// RT含める
			true,			// @含める
			timeline)
		){
			return;
		}
	}
	printTimeline(timeline);
}


// 投稿する
void PostTimeline(TwitterClient &client,const std::string &status)
{
	client.postStatus(status);
}

// キーワード検索
void SearchTimeline(TwitterClient &client,const std::string &ques)
{
	picojson::array timeline;
	if(! client.searchTweets(
		ques,
		"ja",
		TwitterRest1_1::SEARCH_RESTYPE_RECENT,
		"",
		"",
		timeline)
	){
		return;
	}
	printTimeline(timeline);
}


static void usage(FILE *fp, int argc, char **argv)
{
	fprintf(fp,
	 "Usage: %s [options]\n"
	 "初めて使用するときは -a オプションでこのアプリの認証を行ってください\n"
	 "\n"
	 "[Options]\n"
	 "-h | --help          Print this message\n"
	 "-a | --auth          [再]認証を行う\n"
	 "                     -u オプションでエイリアスを指定できます\n"
	 "-p | --post status   タイムラインへ投稿\n"
	 "-s | --search word   ワードで検索\n"
	 "-r | --readhome      ホームのタイムラインを読む\n"
	 "                     -n オプションでユーザ名指定すると指定ユーザを読む\n"
	 "                     -n オプションで\"\"と指定すると自分の発言を読む\n"
	 "-n | --screen        指定が必要な場合のユーザスクリーンネーム\n"
	 "-u | --user alies    エイリアス名指定:省略可(-a とも併用可能)\n"
	 "-v | --verbose       (デバッグ用)余計な文字を出力しまくる\n"
	 "\n"
	 "-u で指定できるエイリアス名は別アカウントで使いたい場合に便利です\n"
	 "実際のスクリーンネームでなくてもかまいません。\n"
	 "\n"
	 "[Example]\n"
	 " i am testman と投稿する:\n"
	 "ctw -p \" i am testman \" \n"
	 "\n"
	 " エイリアス egg で i am egg と投稿する:\n"
	 "ctw -u egg -p \" i am egg \" \n"
	 "\n"
	 " エイリアス egg として認証作業を行う:\n"
	 "ctw -u egg -a\n"
	 "",
	  argv[0]
);
}

static const char short_options[] = "hap:rtn:s:u:v";

static const struct option
long_options[] = {
	{ "help",		no_argument,		NULL, 'h' },
	{ "auth",		no_argument,		NULL, 'a' },
	{ "post",		required_argument,	NULL, 'p' },
	{ "readhome",	no_argument,		NULL, 'r' },
	{ "screen",		required_argument,	NULL, 'n' },
	{ "search",		required_argument,	NULL, 's' },
	{ "user",		required_argument,	NULL, 'u' },
	{ "verbose",	no_argument,		NULL, 'v' },
	{ 0, 0, 0, 0 }
};

int main(int argc,char *argv[])
{
	TwitterClient client;

	bool doReadTL=false;
	bool doPostTL=false;
	bool doSearchTL = false;
	bool doAuth = false;
	bool setScerrnName = false;
	string status,aries,screenuser;

	do_Verbose = false;
	if(argc == 1){
		usage(stderr, argc, argv);
		return -1;
	}
	tzset();
	// このアプリのコンシューマキーなどを設定
	client.setComsumerPair(AP_COMSUMER_KEY,AP_COMSUMER_SECRET);
	
	for (;;) {
		int idx;
		int c;

		c = getopt_long(argc, argv,short_options, long_options, &idx);
		if (c == -1)	break;
		switch (c) {
		case 0: /* getopt_long() flag */
			break;

		case 'h':
			usage(stdout, argc, argv);
			return 0;
		case 'a':
			doAuth = true;
			break;
		case 'p':
			status = optarg;
			doPostTL = true;
	        break;
		case 'r':
			doReadTL = true;
	        break;
		case 's':
			status = optarg;
			doSearchTL = true;
	        break;
		case 'n':
			setScerrnName = true;
			screenuser = optarg;
	        break;
		case 'u':
			aries = optarg;
	        break;
		case 'v':
			do_Verbose = true;
	        break;
			
		default:
	        usage(stderr, argc, argv);
	        return -1;
		}
	}

	client.serVerbose(do_Verbose);
	if(doAuth){
		do_Authentication(client,aries);
		// 認証の場合はいったんここで終わり
		return 0;
	}
	// ここから先はユーザのアクセスキーが必要
	if(! readAccessKey(client,aries)){
		return -1;
	}
	// とりあえず自分自身の情報を取得
	initUserInfo(client);
	
	if(doPostTL)	PostTimeline(client,status);
	if(doReadTL){
		if((!setScerrnName) && (screenuser.empty())){
			ReadHomeTimeline(client);
		}else{
			ReadUserTimeline(client,screenuser);
		}
	}
	if(doSearchTL)	SearchTimeline(client,status);

	return 0;
}


