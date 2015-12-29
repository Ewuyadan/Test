#include <stdio.h>
#include "HttpClient.h"
#include "json/json.h"
#pragma comment(lib,"libcurl_imp.lib")
extern void ReqCreateRoom(std::string strUrl);
bool bReqed = false;
void ProcessMsg()
{
	std::string strToken = CHttpClient::It().GetToken();
	std::string strUrl = CHttpClient::It().GetUrl();
	if (strToken.empty())return;
	if (strUrl.empty())return;
	ReqCreateRoom(strUrl);
}
void ReqCreateRoom(std::string strUrl)
{
	if (bReqed)
	{
		return;
	}
	bReqed = true;
	Json::Value root;
	//root["grant_type"] = Json::Value("client_credentials");
	root["appkey"] = Json::Value("0b164754-fc59-4247-8308-661bfe5acafc");
	root["roomName"] = Json::Value("²âÊÔ1");
	root["head"] = Json::Value(Json::nullValue);
	root["roomType"] = Json::Value(1);
	root["scope"] = Json::Value(0);
	//root["keyword"] = Json::Value("test1,test2");
	root["maxUserNumber"] = Json::Value(10);
	// Ð´
	Json::FastWriter writer;
	std::string strOut = writer.write(root);

	strUrl += "/CreateIMRoom";
	IAgent* agent = new IAgent(strUrl, strOut);
	agent->m_strTag = "reqroom";

	/*std::string strHeader = "Accept:application/json\nContent-Type:application/json\nAuthorization:Bearer ";
	strHeader += CHttpClient::It().GetToken();*/
	//sprintf(strHeader, "%s%s", "Accept:application / json\nContent - Type : application / json\nAuthorization : Bearer ",CHttpClient::It().GetToken().c_str());
	//agent->SetHeaderData(strHeader);
	agent->AddHeaderData("Accept:application/json");
	agent->AddHeaderData("Content-Type:application/json");
	agent->AddHeaderData("Authorization:Bearer " + CHttpClient::It().GetToken());
	
	CHttpClient::It().ReqAgent(agent);
}
int main(void)
{
	CHttpClient::It().Init();
	Json::Value root;
	root["grant_type"] = Json::Value("client_credentials");
	root["client_id"] = Json::Value("0b164754-fc59-4247-8308-661bfe5acafc");
	root["client_secret"] = Json::Value("7e87293c0c7641c19007bb24983cd296");
	// Ð´
	Json::FastWriter writer;
	std::string strOut = writer.write(root);
	std::cout << strOut << std::endl;

	IAgent* agent = new IAgent("https://api.gotye.com.cn/api/accessToken", strOut);
	agent->m_strTag = "access";
	CHttpClient::It().ReqAgent(agent);
	while (true)
	{
		CHttpClient::It().Update();
		ProcessMsg();
	}
}