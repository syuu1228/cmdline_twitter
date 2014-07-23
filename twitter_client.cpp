//
// OAUTH 1.0クライアントの実装
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
#include "twitter_client.hpp"
#include "base64.hpp"
#include <boost/format.hpp>

using namespace std;
using namespace TwitterRest1_1;


TwitterClient::TwitterClient()
{}


TwitterClient::~TwitterClient()
{}


void TwitterClient::setComsumerPair(const std::string &key,const std::string &sec)
{
	m_auth.setComsumerKey(key);
	m_auth.setComsumerSecret(sec);
}

void TwitterClient::setUserAccessPair(const std::string &key,const std::string &sec)
{
	m_auth.setAccessKey(key);
	m_auth.setAccessSecret(sec);
}

void TwitterClient::getUserAccessPair(std::string &key,std::string &sec)
{
	key = m_auth.getAccessKey();
	sec = m_auth.getAccessSecret();
}



// アプリの認証を行うURLを取得する。
bool TwitterClient::Authentication_GetURL(std::string &rurl)
{
	string callbk;
	
	rurl.clear();
	m_lasterror.clear();
	// まずはTemporaryCredentialsをする
	if(! m_auth.TemporaryCredentials(
		&m_peer,
		OAUTH1_REQUEST_TOKEN,
		callbk)
	){
		m_lasterror = "TemporaryCredentials failed";
		vprint(m_lasterror);
		return false;
	}
	// Resource Owner AuthorizatioのためのURLを生成する
	// あらかじめブラウザからログインして承認してもらう必要があるため
	m_auth.MakeResourceOwnerHttpData(OAUTH1_AUTHORIZE,rurl);
	return true;

}

// Pinをセットしてアプリの認証を完了させる。
bool TwitterClient::Authentication_Finish(const std::string &pin)
{
	// Owner Authorizationを行い、PINの正当性を確認
	// 成功したらAccessKeyとSecretは以後永続的に使える	
	OAuthOtherRes twitter_res;
	
	m_lasterror.clear();
	m_auth.setVerifier(pin);
	if(! m_auth.TokenCredentials(
		&m_peer,
		OAUTH1_ACCESS_TOKEN,
		twitter_res)
	){
		m_lasterror = "TokenCredentials failed";
		vprint(m_lasterror);
		return false;
	}
	// 認証OKだったのでスクリーンネームとUSERID取得
	m_user_screen	= twitter_res[OAUTH_ANSWER_SCREENNAME];
	m_user_id		= twitter_res[OAUTH_ANSWER_USERID];
	return true;
}

// JSONの解析を実際に行う
bool TwitterClient::parseJson(picojson::value &jsonval)
{
	unsigned long httpcode = m_peer.getLastResponceCode();
	// Verbose時はここで受けたデータを表示
	vprint(m_peer.getResponceString());	

	// 5xx系のエラーの場合はたぶん落ちてるとかそんなのなのでまともな
	// JSONを期待してはいけない。ここはエラーとしてJSON解析はしない
	if(httpcode >= 500){
		m_lasterror = "responce HTTP 5xx \n";
		m_lasterror = "HTTP Message \n";
		m_lasterror += m_peer.getResponceString();
		vprint(m_lasterror);
		return false;
	}
	
	// JSONで帰ってくるので解析をする
	string json_err;
	string responce = m_peer.getResponceString();
		picojson::parse(jsonval,responce.begin(),responce.end(),&json_err);
	if(!json_err.empty()){
		m_lasterror = "[JSON] parse err!!! ";
		m_lasterror += json_err;
		m_lasterror += responce;
		vprint(m_lasterror);
		return false;
	}
	if(httpcode >= 400){
		// パラメータエラーのときはHTTPコードが400系を返す
		picojson::object errobject = jsonval.get<picojson::object>();
		if(! errobject["errors"].is<picojson::array>()){
			// まともなJSONではなさそう…
			m_lasterror = "Twitter responce error \n";
			m_lasterror += m_peer.getResponceString();
			vprint(m_lasterror);
			return false;
		}
		picojson::array errinfoary = errobject["errors"].get<picojson::array>();
		picojson::object errinfo = errinfoary[0].get<picojson::object>();
		
		m_lasterror = "Twitter return error \n";
		m_lasterror += "Code: ";
		m_lasterror += errinfo["code"].to_str();
		m_lasterror += "\nMessage: \n";
		m_lasterror += errinfo["message"].to_str();
		vprint(m_lasterror);
		return false;
	}
	return true;
}



// GETリクエストを投げる共通関数。ついでにJSON解析までやってしまう。
bool TwitterClient::getRequest(const std::string url,HTTPRequestData &hdata,picojson::value &jsonval)
{
	string authdata;
	m_lasterror.clear();
	
	m_auth.makeResuestHeader(
		"GET",
		url,
		hdata,
		authdata
	);
	
	m_peer.appendHeader(authdata);
	if(!m_peer.getRequest(
		url,
		hdata)
	){
		m_lasterror = "HTTP Get request failed";
		vprint(m_lasterror);
		return false;
	}
	// JSONで帰ってくるので解析をする
	return parseJson(jsonval);
}


// POSTリクエストを投げる共通関数。ついでにJSON解析までやってしまう。
bool TwitterClient::postRequest(const std::string url,HTTPRequestData &hdata,picojson::value &jsonval)
{
	string authdata;
	m_lasterror.clear();
	
	m_auth.makeResuestHeader(
		"POST",
		url,
		hdata,
		authdata
	);
	
	m_peer.appendHeader(authdata);
	if(!m_peer.postRequest(
		url,
		hdata)
	){
		m_lasterror = "HTTP Post request failed";
		vprint(m_lasterror);
	}
	// JSONで帰ってくるので解析をする
	return parseJson(jsonval);
}


bool TwitterClient::testRequest(const std::string url,HTTPRequestData &hdata,
	bool getreq,picojson::value &jsonval,std::string &result,
	unsigned long &httpres)
{
	bool ret = true;
	if(getreq){
		if(! getRequest(
			url,
			hdata,
			jsonval)
		){
			ret = false;
		}
	}else{
		if(! postRequest(
			url,
			hdata,
			jsonval)
		){
			ret = false;
		}
	}
	result = m_peer.getResponceString();
	httpres = m_peer.getLastResponceCode();
	return ret;
}


// -------------------------------------------------------------------------------------------
bool TwitterClient::getMentionsTimeline(
	uint16_t count,
	const std::string &since_id,const std::string &max_id,
	picojson::array &rtimeline)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	
	httpdata[PARAM_COUNT] = (boost::format("%d") % count).str();
	
	if(! since_id.empty())	httpdata[PARAM_SINCE_ID]	= since_id;
	if(! max_id.empty())	httpdata[PARAM_MAX_ID]		= max_id;
	
	if(! getRequest(
		TL_RESOURCE_STATUSES_MEMTION,
		httpdata,
		jsonval)
	){
		vprint("err getRequest");
		return false;
	}
	// タイムライン取得は配列である
	if(! jsonval.is<picojson::array>()){
		m_lasterror = "JSON error is not array data";
		vprint(m_lasterror);
		return false;
	}
	rtimeline = jsonval.get<picojson::array>();
	return true;
}

// 指定ユーザの発言を取得する。
// useridとscreennameが同時に指定された場合は、useridを優先する
bool TwitterClient::getUserTimeline(
	const std::string &userid,const std::string &screenname,
	uint16_t count,
	const std::string &since_id,const std::string &max_id,
	bool include_rts,bool include_replies,
	picojson::array &rtimeline)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	
	if(! userid.empty()){
		httpdata[PARAM_USER_ID] = userid;
	}else if(! screenname.empty()){
		httpdata[PARAM_SCREEN_NAME] = screenname;
	}else{
		// どっちかのパラメータをいれること
		return false;
	}
	
	httpdata[PARAM_COUNT] = (boost::format("%d") % count).str();
	httpdata[PARAM_INCLUDE_RTS]	= (include_rts		? VALUE_TRUE : VALUE_FALSE);
	httpdata[PARAM_EXC_REPLIES]	= (include_replies	? VALUE_FALSE : VALUE_TRUE);
	
	if(! since_id.empty())	httpdata[PARAM_SINCE_ID]	= since_id;
	if(! max_id.empty())	httpdata[PARAM_MAX_ID]		= max_id;
	
	
	if(! getRequest(
		TL_RESOURCE_STATUSES_USERTL,
		httpdata,
		jsonval)
	){
		vprint("err getRequest");
		return false;
	}
	// タイムライン取得は配列である
	if(! jsonval.is<picojson::array>()){
		m_lasterror = "JSON error is not array data";
		vprint(m_lasterror);
		return false;
	}	
	rtimeline = jsonval.get<picojson::array>();
	return true;
}

// 自分自身の発言をGET
bool TwitterClient::getMyTimeline(
	uint16_t count,
	const std::string &since_id,const std::string &max_id,
	bool include_rts,bool include_replies,
	picojson::array &rtimeline)
{
	// 今のところスクリーンネームも一意であるはずだけど
	// 今後のこともあると思うので
	return getUserTimeline(
		getMyUserID(),"",
		count,
		since_id,max_id,
		include_rts,include_replies,
		rtimeline
	);
}


// タイムラインの取得
bool TwitterClient::getHomeTimeline(uint16_t count,
	const std::string &since_id,const std::string &max_id,
	bool include_rts,bool include_replies,
	picojson::array &rtimeline)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	httpdata[PARAM_COUNT] = (boost::format("%d") % count).str();
	httpdata[PARAM_INCLUDE_RTS]	= (include_rts		? VALUE_TRUE : VALUE_FALSE);
	httpdata[PARAM_EXC_REPLIES]	= (include_replies	? VALUE_FALSE : VALUE_TRUE);
	
	if(! since_id.empty())	httpdata[PARAM_SINCE_ID]	= since_id;
	if(! max_id.empty())	httpdata[PARAM_MAX_ID]		= max_id;
	
	
	if(! getRequest(
		TL_RESOURCE_STATUSES_HOMETL,
		httpdata,
		jsonval)
	){
		vprint("err getRequest");
		return false;
	}
	// タイムライン取得は配列である
	if(! jsonval.is<picojson::array>()){
		m_lasterror = "JSON error is not array data";
		vprint(m_lasterror);
		return false;
	}
	
	rtimeline = jsonval.get<picojson::array>();
	return true;
}


bool TwitterClient::showTweet(const std::string &idstr,picojson::object &tweet)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;

	httpdata[PARAM_ID]				= idstr;
	httpdata[PARAM_TRIM_USER] 		= VALUE_FALSE;
	httpdata["include_my_retweet"] 	= VALUE_TRUE;
	
	if(! getRequest(
		TW_RESOURCE_STATUSES_SHOW_ID,
		httpdata,
		jsonval)
	){
		vprint("err DestroyStatus");
		return false;
	}
	if(! jsonval.is<picojson::object>()){
		m_lasterror = "JSON error is not object data";
		vprint(m_lasterror);
		return false;
	}
	tweet = jsonval.get<picojson::object>();
	return true;
}


bool TwitterClient::destroyStatus(const std::string &idstr)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;

	string url = TW_RESOURCE_STATUSES_DEL_ID;
	url += idstr;
	url += JSON_ENDPOINT;
	
	if(! postRequest(
		url,
		httpdata,
		jsonval)
	){
		vprint("err DestroyStatus");
		return false;
	}
	return true;
}


// タイムラインへ投稿
bool TwitterClient::postStatus(const std::string &status,picojson::object &tweet)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	
	httpdata["status"] = status;
	httpdata[PARAM_TRIM_USER] = VALUE_FALSE;
	
	if(! postRequest(
		TW_RESOURCE_STATUSES_UPDATE,
		httpdata,
		jsonval)
	){
		vprint("err putStatus");
		return false;
	}
	if(! jsonval.is<picojson::object>()){
		m_lasterror = "JSON error is not object data";
		vprint(m_lasterror);
		return false;
	}
	tweet = jsonval.get<picojson::object>();
	
	return true;
}

// リツイートする
bool TwitterClient::retweetStatus(const std::string &idstr)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;

	string url = TW_RESOURCE_STATUSES_RETWEET;
	url += idstr;
	url += JSON_ENDPOINT;
	
	if(! postRequest(
		url,
		httpdata,
		jsonval)
	){
		vprint("err DestroyStatus");
		return false;
	}
	return true;
}


// タイムライン検索
bool TwitterClient::searchTweets(const std::string &q,const std::string &lang,const std::string &restype,
		const std::string & since_id,const std::string & max_id,picojson::array &rtimeline)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	
	httpdata[PARAM_COUNT] 	= "100";
	httpdata["q"]			= q;
	httpdata["lang"]		= lang;
	httpdata["result_type"]	= restype;
	
	if(! since_id.empty())	httpdata[PARAM_SINCE_ID]	= since_id;
	if(! max_id.empty())	httpdata[PARAM_MAX_ID]		= max_id;
	
	if(! getRequest(
		TW_SEARCH_TWEETS,
		httpdata,
		jsonval)
	){
		vprint("err getRequest");
		return false;
	}
	// statusesの中に配列がある形
	rtimeline = jsonval.get<picojson::object>()["statuses"].get<picojson::array>();
	return true;
}


// お気に入りに追加する
bool TwitterClient::createFavorites(const std::string &idstr)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;

	httpdata[PARAM_ID]	= idstr;
	
	if(! postRequest(
		TW_FABORITES_CREATE,
		httpdata,
		jsonval)
	){
		vprint("err DestroyStatus");
		return false;
	}
	return true;
}

bool TwitterClient::getMyList(picojson::array &rlists)
{
	return getUserList(
		getMyUserID(),"",
		rlists
	);	
}


bool TwitterClient::getUserList(const std::string &userid,const std::string &screenname,picojson::array &rlists)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	
	if(! userid.empty()){
		httpdata[PARAM_USER_ID] = userid;
	}else if(! screenname.empty()){
		httpdata[PARAM_SCREEN_NAME] = screenname;
	}else{
		// どっちかのパラメータをいれること
		return false;
	}
		
	
	if(! getRequest(
		TW_LISTS_LIST,
		httpdata,
		jsonval)
	){
		vprint("err getRequest");
		return false;
	}
	// リストは配列である
	if(! jsonval.is<picojson::array>()){
		m_lasterror = "JSON error is not array data";
		vprint(m_lasterror);
		return false;
	}
	rlists = jsonval.get<picojson::array>();
	return true;	
}


	
bool TwitterClient::getMyListTimeline(const std::string &slug,uint16_t count,
						const std::string &since_id,const std::string &max_id,
						bool include_rts,
						picojson::array &rtimeline)
{
	return getUserListTimeline(
		slug,
		getMyUserID(),"",
		count,
		since_id,max_id,
		include_rts,
		rtimeline
	);
}

bool TwitterClient::getUserListTimeline(const std::string &slug,
						const std::string &userid,const std::string &screenname,
						uint16_t count,
						const std::string &since_id,const std::string &max_id,
						bool include_rts,
						picojson::array &rtimeline)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;
	
	if(slug.empty())	return false;
	
	httpdata[PARAM_SLUG] = slug;
	
	if(! userid.empty()){
		httpdata[PARAM_OWNER_ID] = userid;
	}else if(! screenname.empty()){
		httpdata[PARAM_OWMER_SCREEN_NAME] = screenname;
	}else{
		// どっちかのパラメータをいれること
		return false;
	}
	
	httpdata[PARAM_COUNT] = (boost::format("%d") % count).str();
	httpdata[PARAM_INCLUDE_RTS]	= (include_rts		? VALUE_TRUE : VALUE_FALSE);
	
	if(! since_id.empty())	httpdata[PARAM_SINCE_ID]	= since_id;
	if(! max_id.empty())	httpdata[PARAM_MAX_ID]		= max_id;
	
	
	if(! getRequest(
		TW_LISTS_STATUSES,
		httpdata,
		jsonval)
	){
		vprint("err getRequest");
		return false;
	}
	// タイムライン取得は配列である
	if(! jsonval.is<picojson::array>()){
		m_lasterror = "JSON error is not array data";
		vprint(m_lasterror);
		return false;
	}
	rtimeline = jsonval.get<picojson::array>();
	return true;	
}



// 自分のユーザ情報の取得
// last_status : 最後の発言などを含める
// entities : ProfileのURL情報などを含める(効いてない？？)
bool TwitterClient::verifyAccount(picojson::object &userinfo,bool last_status,bool entities)
{
	HTTPRequestData	httpdata;
	picojson::value jsonval;

	httpdata["include_entities"]	= (entities ? VALUE_TRUE : VALUE_FALSE);
	httpdata["skip_status"] 		= (last_status ? VALUE_FALSE : VALUE_TRUE);
	
	if(! getRequest(
		TW_USERS_ACCOUNT_VERIFY,
		httpdata,
		jsonval)
	){
		vprint("err getRequest");
		return false;
	}
	userinfo = jsonval.get<picojson::object>();
	
	m_user_id		= userinfo[PARAM_ID_STR].to_str();
	m_user_name		= userinfo[PARAM_NAME].to_str();
	m_user_screen	= userinfo[PARAM_SCREEN_NAME].to_str();
	return true;
}





