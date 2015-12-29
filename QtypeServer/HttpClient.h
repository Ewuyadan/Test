/********************************************************************
	created:	2013/10/15
	created:	15:10:2013   19:14
	file base:	HttpClient
	author:		WitSun	
	purpose:	curl的封装.	用法如下:
		int _tmain(int argc, _TCHAR* argv[])
		{
			CHttpClient::It().Init();
			IAgent *p = new CAddBattleRecordAgent;
			p->SetRequestUrl("http://localhost:8080/addrecord", "record={ABCDEFGHIJKLMN}&timestamp=1381835137&signature=8270171bf7ced48c6085ab0dd693365f");
			CHttpClient::It().ReqAgent( p );
			while (true)
			{
				CHttpClient::It().Update();
				Sleep(1);
			}
			CHttpClient::It().Release();
			return 0;
		}
*********************************************************************/
#pragma once
#include <curl/curl.h>
#include <map>

#pragma comment(lib,"libcurl_imp.lib")

class IAgent
{
public:
	IAgent();
	IAgent(const std::string &strURL, const std::string &strFields);
	virtual ~IAgent(void);

public:
	bool SetRequestUrl(const std::string &strURL, const std::string &strFields);
	void SetHeaderData(std::string strHeader);
	void AddHeaderData(std::string strData);
	std::string m_strTag;
	//void SetOpt(CURLoption opt,double dParam);
public:
	bool ReceiveComplete(CURLcode);// { return true; }

public:
	virtual bool Init();
	virtual void Release();
	virtual size_t ReceiveData(void *pBuf, size_t nSize, size_t nMemb);
	
public:
	inline CURL* GetURL() const { return m_pURL; }
	inline const std::string& GetReceiveData() const { return m_strReceiveData; }
	inline const std::string& GetRequestURL() const { return m_strRequestURL; }
	inline const std::string& GetFileds() const { return m_strFileds; }

private:
	CURL*			m_pURL;
	curl_slist*		m_pHeaderData;
	std::string		m_strRequestURL;	// 请求地址.如:"http://localhost:8080/addrecord"
	std::string		m_strFileds;		// 表单数据(Post).如:"record={ABCDEFGHIJKLMN}&timestamp=1381835137&signature=8270171bf7ced48c6085ab0dd693365f"
	std::string		m_strReceiveData;	// 接收到的数据
};

//////////////////////////////////////////////////////////////////////////

class CHttpClient
{
public:
	CHttpClient(void);
	~CHttpClient(void);

public:
	static CHttpClient& It();

public:
	bool Init();
	void Update();
	bool Release();
	bool ReqAgent(IAgent *pAgent);
	std::string GetToken(){ return m_strToken; }
	void SetToken(std::string strToken){ m_strToken = strToken; }
	
	std::string GetUrl(){ return m_strUrl; }
	void SetUrl(std::string strUrl){ m_strUrl = strUrl; }

private:
	bool AddAgent(IAgent *pAgent);

private:
	typedef std::map<CURL*,IAgent*>		MapAgent;

private:
	bool ProcessMsg(CURLMsg *pMsg);
	bool RemoveMsg(CURLMsg *pMsg);

private:
	static size_t ReceiveHttpData(void *pBuf, size_t nSize, size_t nMemb, void *pUser);

private:
	CURLM*				m_pURLMgr;
	MapAgent			m_mapAgent;
	std::string			m_strToken;
	std::string			m_strUrl;
};

