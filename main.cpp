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

#include "typedef.hpp"
#include "twitter_client.hpp"
#include "keys/apikeys.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

using namespace std;

// バージョン
static const string THIS_VERSION	= "0.0.2";

// 設定ファイル保存先
static const string	DEFAULT_AUTH_FILE = ".authkey_";
static const string	APP_DIR = ".ctw";

static bool do_Verbose = false;		// -vが指定されるとTRUE


static int makedir(const string &dirname,mode_t mode)
{
#if defined(__MINGW32__)
	return mkdir(dirname.c_str());
#else
	return mkdir(dirname.c_str(),mode);
#endif
}

// アプリ設定ファイル用ディレクトリを取得＆作成
static bool get_app_dir(string &dirname)
{
	struct stat st;
	dirname = getenv("HOME");
	dirname += '/';
	dirname += APP_DIR;
	
	if(stat(dirname.c_str(),&st) != 0){
		if(makedir(dirname.c_str(),0700) != 0){
			return false;
		}
	}
	dirname += '/';
	
	return true;

}


inline void ReplaceString(std::string &src,const std::string &from,const std::string &to)
{
	string::size_type p;
	p = src.find(from);
	while(p != string::npos){
		src.replace(p,from.length(),to);
		p += to.length();
		p = src.find(from, p);
	}
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

void putRequestError(TwitterClient &client)
{
	cout << "リクエストに失敗" << endl;
	cout << client.getLastErrorMessage() << endl;
}

// 内部コールバック用関数
static size_t request_test_callbk(char* ptr,size_t size,size_t nmemb,void* userdata)
{
	if(size == 0)		return 0;
	
	size_t wsize = size*nmemb;
	cout << "read data size " << wsize <<endl;
	cout << ptr << endl;
	
	return wsize;
}

void RequestTest(TwitterClient &client,bool streamapi)
{
	string url;
	string responce;
	string param,value;
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	int getpost;
	bool bget=false;
	unsigned long rescode=0;
	if(streamapi){
		cout << "!!! Streaming APIモードです !!!" << endl;
	}
	cout << "APIのURLを指定してください" << endl;
	cin >> url;
	
	cout << "GETかPOSTか指定してください(Get=1 post=それ以外)" << endl;
	cin >> getpost;
	if(getpost==1){
		bget = true;
	}
	cout << "パラメータ、RETURN、値…の順番で入力してください。CTRL+D で終わります" << endl;
	
	while(1){
		cin >> param;
		if(cin.eof())break;
		if(cin.fail())break;
		cin >> value;
		if(cin.eof())break;
		if(cin.fail())break;
		
		httpdata[param] = value;
		cout << param << ":" << value << endl;
	}
	cout << "リクエストを送信しています..." << endl;
	
	if(! streamapi){
		// REST版
		if(! client.testRequest(
			url,
			httpdata,
			bget,
			jsonval,
			responce,
			rescode)
		){
			putRequestError(client);
		}
		cout << "\n" << endl;
		cout << "\nHTTP Responce Data" << endl;	
		cout << responce << endl;
		cout << "\nHTTP Responce Code" << endl;	
		cout << rescode << endl;
	}else{
		// Stream版
		if(! client.testRequestRaw(
			url,
			httpdata,
			bget,
			request_test_callbk,
			NULL,
			rescode)
		){
			putRequestError(client);
		}
		cout << "\n" << endl;
		cout << "\nHTTP Responce Code" << endl;	
		cout << rescode << endl;
	}
	
}


// Twitterでは標準時刻(UTC)が記述されているのでこれを現地時刻に直す
inline static void get_local_time_string(const std::string &src,std::string &dst)
{
#if defined(__MSYS__)
	// TODO: MSYS2にはstrptimeが機能していないので今はこうしている…
	dst = src;
#else
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
#endif	//__MINGW32__
}

static void printTweet(picojson::object &tweet)
{
	using namespace picojson;
	using namespace std;
	string tmstr,textstr,rtmstr;
	
	if(! tweet["user"].is<object>()){
		// ユーザ情報なし
		return;
	}
	object uobj = tweet["user"].get<object>();	// ユーザ情報取得
	object robj;
	object rtusr;
	bool retweeted=false;
	
	get_local_time_string(tweet["created_at"].to_str(),tmstr);
	
	// Twitterでは&lt &gt &ampだけは変換されるというわけわからん仕様みたいなので元に戻す
	if(tweet["retweeted_status"].is<object>()){
		// リツィート時の完全な本文はretweeted_statusに含まれるそうな
		robj = tweet["retweeted_status"].get<object>();
		// 情報が足りない…
		if(! robj["user"].is<object>()){
			return;
		}
		
		rtusr = robj["user"].get<object>();
		get_local_time_string(robj["created_at"].to_str(),rtmstr);
		textstr = robj["text"].to_str();
		retweeted = true;
	}else{
		textstr = tweet["text"].to_str();
	}
	ReplaceString(textstr,"&lt;","<");
	ReplaceString(textstr,"&gt;",">");
	ReplaceString(textstr,"&amp;","&");
	// 実際に出力
	cout << "\033[32m";
	if(retweeted){
		cout << "RT: " << rtusr["name"].to_str() << " @" << rtusr["screen_name"].to_str() << " " 
			 << robj["id_str"].to_str() << " " << rtmstr << endl;
		cout << " from: " << uobj["name"].to_str() << " @" << uobj["screen_name"].to_str() << " " 
			 << tweet["id_str"].to_str() << " " << tmstr << endl;
	}else{
		cout << uobj["name"].to_str() << " @" << uobj["screen_name"].to_str() << " " 
			 << tweet["id_str"].to_str() << " " << tmstr << endl;
	}
	cout << "\033[37m";
	cout << textstr << endl;
	cout << "\033[0m";
}

static void printDM(picojson::object &tweet)
{
	using namespace picojson;
	using namespace std;
	string tmstr,textstr;
	
	if(! tweet["sender"].is<object>()){
		// Sender情報なし
		return;
	}
	if(! tweet["recipient"].is<object>()){
		// recipient情報なし
		return;
	}
	object sender	= tweet["sender"].get<object>();	// DMを送ったユーザ情報取得
	object receiver	= tweet["recipient"].get<object>();	// DMを受け取ったユーザ情報取得
	// 時間を直す
	get_local_time_string(tweet["created_at"].to_str(),tmstr);
	// Twitterでは&lt &gt &ampだけは変換されるというわけわからん仕様みたいなので元に戻す
	textstr = tweet["text"].to_str();
	ReplaceString(textstr,"&lt;","<");
	ReplaceString(textstr,"&gt;",">");
	ReplaceString(textstr,"&amp;","&");
	// 実際に出力
	cout << "\033[32m";
	cout << "Fm: " << sender["name"].to_str() << " @" << sender["screen_name"].to_str() << " " 
		 << tweet["id_str"].to_str() << " " << tmstr << endl;
	cout << "To: "<< receiver["name"].to_str() << " @" << receiver["screen_name"].to_str() << endl;
	
	cout << "\033[37m";
	cout << textstr << endl;
	cout << "\033[0m";
}



// タイムラインを実際に出力する。検索結果表示やらHOME表示やらに使ってる
static void printTimeline(picojson::array &timeline)
{
	using namespace picojson;
	using namespace std;

	// Twitterからの戻りは先が最新なので、逆順に表示
	array::reverse_iterator it;
	
	for(it=timeline.rbegin();it!=timeline.rend();it++){
		if(! it->is<object>()) continue;
		object obj = it->get<object>();
		printTweet(obj);
	}
//	cout << "\033[0m";
}


// DMを実際に出力する。検索結果表示やらHOME表示やらに使ってる
static void printDMline(picojson::array &timeline)
{
	using namespace picojson;
	using namespace std;

	// Twitterからの戻りは先が最新なので、逆順に表示
	array::reverse_iterator it;
	
	for(it=timeline.rbegin();it!=timeline.rend();it++){
		if(! it->is<object>()) continue;
		object obj = it->get<object>();
		printDM(obj);
	}
//	cout << "\033[0m";
}


static void printList(picojson::array &lists)
{
	using namespace picojson;
	using namespace std;
	
	string tmstr,textstr;

	array::iterator it;
	
	for(it=lists.begin();it!=lists.end();it++){
		object obj = it->get<object>();
		cout << "\033[32m";
		cout << obj["slug"].to_str() << "  " << obj["member_count"].to_str() << "users"
			<< "  [" << obj["mode"].to_str() << "]" << endl;
		cout << "\033[37m";
		cout << obj["description"].to_str() << endl;
	}
	cout << "\033[0m";
}


// ユーザ情報の初期化
bool initUserInfo(TwitterClient &client)
{
	picojson::object info;
	// ただユーザ名とスクリーンネームとIDが知りたいだけ
	if(! client.verifyAccount(info)){
		putRequestError(client);
		return false;
	}
	return true;
}


// タイムラインを読む
void ReadMemtioonTimeline(TwitterClient &client)
{
	picojson::array timeline;
	if(! client.getMentionsTimeline(
		200,
		"",
		"",
		timeline)
	){
		putRequestError(client);
		return;
	}
	printTimeline(timeline);
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
		putRequestError(client);
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
			putRequestError(client);
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
			putRequestError(client);
			return;
		}
	}
	printTimeline(timeline);
}


void ReadTweet(TwitterClient &client,const std::string &idstr)
{
	picojson::object tweet;
	if(! client.showTweet(idstr,tweet)){
		putRequestError(client);
		return;
	}
	printTweet(tweet);
}


void RemoveTimeline(TwitterClient &client,const std::string &idstr,bool silent)
{
	picojson::object tweet;
	if(! client.destroyStatus(idstr,tweet)){
		putRequestError(client);
		return;
	}
	if(! silent)	printTweet(tweet);
}

// 投稿する
void PostTimeline(TwitterClient &client,const std::string &status,bool silent)
{
	picojson::object tweet;
	if(! client.postStatus(status,"",tweet)){
		putRequestError(client);
		return;
	}
	if(! silent)	printTweet(tweet);
}

// リプライする
void ReplyTimeline(TwitterClient &client,const std::string &status,const std::string &idstr,bool silent)
{
	picojson::object tweet;
	
	// @付けてるかチェックする
	if(status.find_first_of('@') != string::npos){
		if(! client.postStatus(status,idstr,tweet)){
			putRequestError(client);
			return;
		}
	}else{
		// @の指定がなかった場合、IDの発言より自動的にユーザを取得する
		picojson::object srctweet;
		if(! client.showTweet(idstr,srctweet)){
			putRequestError(client);
			return;
		}
		if(! srctweet["user"].is<picojson::object>()){
			// ユーザ情報なし
			return;
		}
		picojson::object uobj = srctweet["user"].get<picojson::object>();	// ユーザ情報取得
		std::string newstatus = "@" + uobj["screen_name"].to_str() + " " + status;
		if(! client.postStatus(newstatus,idstr,tweet)){
			putRequestError(client);
			return;
		}
	}
	
	if(! silent)	printTweet(tweet);
}


void RetweetTimeline(TwitterClient &client,const std::string &idstr,bool silent)
{
	picojson::object tweet;
	if(! client.retweetStatus(idstr,tweet)){
		putRequestError(client);
		return;
	}
	if(! silent)	printTweet(tweet);
}

void FavoriteTimeline(TwitterClient &client,const std::string &idstr,bool silent)
{
	picojson::object tweet;
	if(! client.createFavorites(idstr,tweet)){
		putRequestError(client);
		return;
	}
	if(! silent)	printTweet(tweet);
}


void ReadDirectMessaeg(TwitterClient &client)
{
	picojson::array timeline;
	if(! client.getDirectMessage(
		200,
		"",
		"",
		timeline)
	){
		putRequestError(client);
		return;
	}
	printDMline(timeline);
}

void ReadDirectPost(TwitterClient &client)
{
	picojson::array timeline;
	if(! client.getDirectPosting(
		200,
		"",
		"",
		timeline)
	){
		putRequestError(client);
		return;
	}
	printDMline(timeline);
}

void PostDirectMessage(TwitterClient &client,const std::string &sname,const std::string &text,bool silent)
{
	picojson::object tweet;
	if(! client.postDirectMessage(
		"",
		sname,
		text,
		tweet)
	){
		putRequestError(client);
		return;
	}
	if(! silent) printDM(tweet);
}

void RemoveDirectMessage(TwitterClient &client,const std::string &idstr,bool silent)
{
	picojson::object tweet;
	if(! client.removeDirectMessage(
		idstr,
		tweet)
	){
		putRequestError(client);
		return;
	}
	if(! silent) printDM(tweet);
}


void PutUserLists(TwitterClient &client,const std::string &name)
{
	picojson::array lists;
	
	if(name.empty()){
		if(! client.getMyList(lists)
		){
			putRequestError(client);
			return;
		}
	}else{
		if(! client.getUserList(
			"",
			name,
			lists)
		){
			putRequestError(client);
			return;
		}
	}
	printList(lists);
}


void ReadListTimeline(TwitterClient &client,const std::string &name,const std::string &listname)
{
	picojson::array timeline;

	if(name.empty()){
		if(! client.getMyListTimeline(
			listname,
			200,
			"",
			"",
			true,			// RT含める
			timeline)
		){
			putRequestError(client);
			return;
		}
	}else{
		if(! client.getUserListTimeline(
			listname,
			"",
			name,
			200,
			"",
			"",
			true,			// RT含める
			timeline)
		){
			putRequestError(client);
			return;
		}
	}
	printTimeline(timeline);
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
		putRequestError(client);
		return;
	}
	printTimeline(timeline);
}



// ユーザストリーム用コールバック関数。ユーザストリームからデータが来るごとに呼び出される
bool UserStreamCallback(picojson::object &jobj,void* userdata)
{
	picojson::object nobj;
	if(! jobj["text"].is<picojson::null>()){
		printTweet(jobj);
		return true;
	}
	if(! jobj["friends"].is<picojson::null>()){
		cout << "get friends object " << endl;
		return true;
	}
	if(! jobj["friends_str"].is<picojson::null>()){
		cout << "get friends_str object " << endl;
		return true;
	}
	if(! jobj["direct_message"].is<picojson::null>()){
		cout << "Direct Messageイベントが着ました ----------------------------" << endl;
		nobj = jobj["direct_message"].get<picojson::object>();
		// DMはそのままやってくるっぽい
		printDM(nobj);
		cout << "-------------------------------------------------------------" << endl;
		return true;
	}
	if(! jobj["event"].is<picojson::null>()){
		cout << "get event object " << endl;
		cout << jobj["event"].serialize(true) << endl;
		return true;
	}
	if(! jobj["delete"].is<picojson::null>()){
		cout << "つい消しを検出 ----------------------------------------------" << endl;
		picojson::value tmpval = jobj["delete"].get("status");
		if (tmpval.is<picojson::null>()){
			cout << jobj["delete"].serialize(true) << endl;
		}else{
			nobj = tmpval.get<picojson::object>();
			cout << "ユーザID " << nobj["user_id_str"].to_str() << " が 発言ID " << nobj["id_str"].to_str() << " を消しました" << endl;
		}
		cout << "-------------------------------------------------------------" << endl;
		return true;
	}
	if(! jobj["scrub_geo"].is<picojson::null>()){
		cout << "get scrub_geo object " << endl;
		return true;
	}
	if(! jobj["limit"].is<picojson::null>()){
		cout << "get limit object " << endl;
		cout << jobj["limit"].serialize(true) << endl;
		return true;
	}
	if(! jobj["status_withheld"].is<picojson::null>()){
		cout << "get status_withheld object " << endl;
		return true;
	}
	if(! jobj["user_withheld"].is<picojson::null>()){
		cout << "get user_withheld object " << endl;
		return true;
	}
	if(! jobj["disconnect"].is<picojson::null>()){
		cout << "get disconnect object " << endl;
		cout << jobj["disconnect"].serialize(true) << endl;
		return true;
	}
	if(! jobj["warning"].is<picojson::null>()){
		cout << "get warning object " << endl;
		cout << jobj["warning"].serialize(true) << endl;
		return true;
	}
	cout << "get ??? object " << endl;
	
	return true;
}

// UserStreamを使ってタイムラインを取得する
static void ReadUserStream(TwitterClient &client,const std::string &trackword)
{
	if(! client.getUserStreaming(
			false,				// リプライはユーザに関連するもののみ
			true,				// ユーザとフォローに関連するもののみ
			trackword,
			UserStreamCallback,
			NULL)
	){
		putRequestError(client);
		return;	
	}
}



static void usage(FILE *fp, int argc, char **argv)
{
	fprintf(fp,
	 "Command-line Twritter Version [%s]\n"
	 "Usage: %s [options]\n"
	 "初めて使用するときは -a オプションでこのアプリの認証を行ってください\n"
	 "\n"
	 "[Options]\n"
	 "-h | --help          Print this message\n"
	 "-a | --auth          [再]認証を行う\n"
	 "                     -u オプションでエイリアスを指定できます\n"
	 "-D | --Direct        DM関連の操作を行う\n"
	 "                     -p オプションと-n オプションでユーザ名指定でダイレクトメッセージを送る\n"
	 "                     -r オプションでダイレクトメッセージを読む\n"
	 "                     -r と -n オプションで\"\"と指定すると自分の発言を読む\n"
	 "                     -d と -i オプションでID指定でメッセージを消す\n"
	 "-p | --post status   タイムラインへ投稿\n"
	 "                     -i オプションでそのIDに対してのリプライ動作\n"
	 "                     (@は自分で付けてください。@省略時は@が自動付与されます)\n"
	 "-s | --search word   ワードで検索\n"
	 "-S | --Silent        サイレントモード (投稿やFavなどの結果を表示しない)\n"
	 "-r | --readtl        ホームのタイムラインを読む\n"
	 "                     -x オプションでUserStreamを使って読む(以後のオプションは無視)\n"
	 "                     -n オプションでユーザ名指定すると指定ユーザを読む\n"
	 "                     -n オプションで\"\"と指定すると自分の発言を読む\n"
	 "                     -n オプションで\"@\"と指定すると自分へのメンションを読む\n"
	 "                     -i オプションで単発のツイートを読む\n"
	 "-d | --del           発言の削除 -iでID指定\n"
	 "-R | --Retweet       リツイートする -iでID指定\n"
	 "-F | --Fav           お気に入りに追加する -iでID指定\n"
	 "-l | --list name     自分のリストnameの内容を読む\n"
	 "                     nameで\"\"と指定すると自分のリスト一覧を表示する\n"
	 "-n | --name          指定が必要な場合のユーザスクリーンネーム\n"
	 "-i | --id            指定が必要な場合の発言ID\n"
	 "-u | --user alies    エイリアス名指定:省略可(-a とも併用可能)\n"
	 "-x | --xstream       Streaming APIを使う(使用可能な場合)\n"
	 "-T | --Test          (テスト用)APIのエンドポイントを指定してAPIリクエストを行う\n"
	 "                     -x オプションでStreaming向け接続を行う\n"
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
	 THIS_VERSION.c_str(),argv[0]
	);
}

namespace CMDLINE_OPT
{
	enum {
		PUT_HELP	= 1,
		AUTH,
		DELTW,
		DIRECT,
		FAVORITES,
		POST,
		READTL,
		RETWEET,
		SCREEN,
		SEARCH,
		SILENT,
		USER,
		ID,
		LIST,
		STREAMAPI,
		TEST,
		VERBOSE,
	};
};

static const struct option
long_options[] = {
	{ "auth",		no_argument,		NULL, CMDLINE_OPT::AUTH			},
	{ "del",		no_argument,		NULL, CMDLINE_OPT::DELTW		},
	{ "Direct",		no_argument,		NULL, CMDLINE_OPT::DIRECT		},
	{ "Fav",		no_argument,		NULL, CMDLINE_OPT::FAVORITES	},
	{ "help",		no_argument,		NULL, CMDLINE_OPT::PUT_HELP		},
	{ "id",			required_argument,	NULL, CMDLINE_OPT::ID			},
	{ "list",		required_argument,	NULL, CMDLINE_OPT::LIST			},
	{ "name",		required_argument,	NULL, CMDLINE_OPT::SCREEN		},
	{ "post",		required_argument,	NULL, CMDLINE_OPT::POST			},
	{ "readtl",		no_argument,		NULL, CMDLINE_OPT::READTL		},
	{ "Retweet",	no_argument,		NULL, CMDLINE_OPT::RETWEET		},
	{ "search",		required_argument,	NULL, CMDLINE_OPT::SEARCH		},
	{ "Silent",		no_argument,		NULL, CMDLINE_OPT::SILENT		},
	{ "user",		required_argument,	NULL, CMDLINE_OPT::USER			},
	{ "xstream",	no_argument,		NULL, CMDLINE_OPT::STREAMAPI	},
	{ "Test",		no_argument,		NULL, CMDLINE_OPT::TEST			},
	{ "verbose",	no_argument,		NULL, CMDLINE_OPT::VERBOSE		},
	{ 0, 0, 0, 0 }
};


int main(int argc,char *argv[])
{
	TwitterClient client;
	bool doReadTL			= false;
	bool doRetweetTL		= false;
	bool doFavTL			= false;
	bool doPostTL			= false;
	bool doSearchTL			= false;
	bool doAuth				= false;
	bool doDeltw			= false;
	bool doList				= false;
	bool doTest				= false;
	bool doSilent			= false;
	bool doDirect			= false;
	bool useStreamAPI		= false;
	bool setScerrnName		= false;
	string status,aries,screenuser,idstr,listname;

	do_Verbose = false;
	if(argc == 1){
		usage(stderr, argc, argv);
		return 0;
	}
	tzset();
	// このアプリのコンシューマキーなどを設定
	client.setComsumerPair(AP_COMSUMER_KEY,AP_COMSUMER_SECRET);
	
	while(1){
		int c = getopt_long_only(argc,argv,"",long_options,NULL);
		
		if(c == -1)		break;		// -1は解析終わり
		switch (c) {
		case 0:
			break;
		case CMDLINE_OPT::PUT_HELP:
			usage(stdout, argc, argv);
			return 0;
			
		case CMDLINE_OPT::AUTH:
			doAuth = true;
			break;

		case CMDLINE_OPT::DELTW:
			doDeltw = true;
			break;

		case CMDLINE_OPT::DIRECT:
			doDirect = true;
			break;
			
		case CMDLINE_OPT::POST:
			status = optarg;
			doPostTL = true;
	        break;
			
		case CMDLINE_OPT::READTL:
			doReadTL = true;
	        break;

		case CMDLINE_OPT::RETWEET:
			doRetweetTL = true;
	        break;
			
		case CMDLINE_OPT::FAVORITES:
			doFavTL = true;
	        break;
			
		case CMDLINE_OPT::SEARCH:
			status = optarg;
			doSearchTL = true;
	        break;

		case CMDLINE_OPT::LIST:
			doList = true;
			listname = optarg;
	        break;
			
		case CMDLINE_OPT::ID:
			idstr = optarg;
	        break;

		case CMDLINE_OPT::SCREEN:
			setScerrnName = true;
			screenuser = optarg;
	        break;
			
		case CMDLINE_OPT::USER:
			aries = optarg;
	        break;

		case CMDLINE_OPT::SILENT:
			doSilent = true;
	        break;
			
		case CMDLINE_OPT::STREAMAPI:
			useStreamAPI = true;
	        break;
			
		case CMDLINE_OPT::VERBOSE:
			do_Verbose = true;
	        break;

		case CMDLINE_OPT::TEST:
			doTest = true;
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
	if(doTest){
		RequestTest(client,useStreamAPI);
		return 0;
	}
	
	if(doDirect){
		if(doPostTL){
			if(screenuser.empty()){
				cout << "スクリーンネームを必ず指定してください" << endl;
				return 0;
			}
			PostDirectMessage(client,screenuser,status,doSilent);
		}else if(doReadTL){
			if((!setScerrnName) && (screenuser.empty())){
			// スクリーンネームが指定されてない場合はHOMEを表示
				ReadDirectMessaeg(client);
			}else{
				ReadDirectPost(client);
			}
		}else if(doDeltw){
			if(idstr.empty()){
				cout << "IDを必ず指定してください" << endl;
				return 0;
			}
			RemoveDirectMessage(client,idstr,doSilent);
		}
		return 0;
	}
	
	if(doPostTL){
		if(idstr.empty()){
			PostTimeline(client,status,doSilent);
		}else{
			ReplyTimeline(client,status,idstr,doSilent);
		}
		return 0;
	}
	
	if(doDeltw){
		if(idstr.empty()){
			// IDが指定されていない場合はとりあえず表示
			ReadUserTimeline(client,"");
			// IDを指定させる
			cout << "発言を消すIDを指定してください" << endl;
			cin >> idstr;
		}
		RemoveTimeline(client,idstr,doSilent);
		return 0;
	}
	if(doRetweetTL){
		if(idstr.empty()){
			// IDが指定されていない場合はとりあえず表示
			ReadHomeTimeline(client);
			// IDを指定させる
			cout << "発言をリツイートしたいIDを指定してください" << endl;
			cin >> idstr;
		}
		RetweetTimeline(client,idstr,doSilent);
		if(! doFavTL)	return 0;		// RtしてFavしたいこともあるので
	}
	
	if(doFavTL){
		if(idstr.empty()){
			// IDが指定されていない場合はとりあえず表示
			ReadHomeTimeline(client);
			// IDを指定させる
			cout << "発言をお気に入りに入れたいIDを指定してください" << endl;
			cin >> idstr;
		}
		FavoriteTimeline(client,idstr,doSilent);
		return 0;
	}
	
	if(doSearchTL){
		SearchTimeline(client,status);
		return 0;
	}
	
	if(doList){
		if(listname.empty()){
			PutUserLists(client,"");
			// IDを指定させる
			cout << "表示したいリスト名を指定してください" << endl;
			cin >> listname;
		}
		ReadListTimeline(client,"",listname);
		return 0;
	}
	
	
	if(doReadTL){
		if(useStreamAPI){
			ReadUserStream(client,"");
		}else if(! idstr.empty()){
			// IDに何か指定されている場合は対象のIDを表示
			ReadTweet(client,idstr);
		}else if((!setScerrnName) && (screenuser.empty())){
			// スクリーンネームが指定されてない場合はHOMEを表示
			ReadHomeTimeline(client);
		}else if(screenuser == "@"){
			// 自分に対するメンション
			ReadMemtioonTimeline(client);
		}else{
			// スクリーンネームが何か指定されているか空である場合
			ReadUserTimeline(client,screenuser);
		}
		return 0;
	}

	return 0;
}


