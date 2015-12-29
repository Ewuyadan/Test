#include "HttpClient.h"
#include <assert.h>
#include "json\json.h"
IAgent::IAgent() : m_pURL(NULL)
{
}

IAgent::IAgent(const std::string &strURL, const std::string &strFields) : m_pURL(NULL)
{
	SetRequestUrl(strURL, strFields);
}

IAgent::~IAgent(void)
{
	assert(m_pURL==NULL);
}

bool IAgent::SetRequestUrl(const std::string &strURL, const std::string &strFields)
{
	if (strURL.empty())
	{
		return false;
	}
	m_strRequestURL = strURL;
	m_strFileds = strFields;
	return true;
}

bool IAgent::Init()
{
	if (m_strRequestURL.empty())
	{
		return false;
	}
	if (m_pURL != NULL)
	{
		assert(false&&"IAgent::Create m_pURL != NULL");
		return false;
	}
	m_pURL = curl_easy_init();
	if (!m_pURL)
	{
		assert(false);
		return false;
	}
	curl_easy_setopt(m_pURL, CURLOPT_URL, m_strRequestURL.c_str());
	if (!m_strFileds.empty())
	{
		curl_easy_setopt(m_pURL, CURLOPT_POSTFIELDS, m_strFileds.c_str());
	}
	curl_easy_setopt(m_pURL, CURLOPT_SSL_VERIFYPEER, 0L);
	if (m_pHeaderData)
	{
		if (curl_easy_setopt(m_pURL, CURLOPT_HTTPHEADER, m_pHeaderData) != CURLE_OK)
		{
			assert(false);
		}
	}
	return true;
}

void IAgent::Release()
{
	m_strRequestURL.clear();
	if (m_pHeaderData)
	{
		curl_slist_free_all(m_pHeaderData);
	}
	if (m_pURL)
	{
		curl_easy_cleanup(m_pURL);
		m_pURL = NULL;
	}
	delete this;
}
bool IAgent::ReceiveComplete(CURLcode)
{
	if (m_strTag == "access")
	{
		Json::Reader reader;
		Json::Value json_object;
		if (!reader.parse(m_strReceiveData, json_object))
			return false;
		std::string strUrl = json_object["api_url"].asString();
		std::string strToken = json_object["access_token"].asString();
		std::cout << strUrl << std::endl;
		std::cout << strToken << std::endl;
		//std::cout << json_object["api_url"] << std::endl;

		CHttpClient::It().SetUrl(strUrl);
		CHttpClient::It().SetToken(strToken);
	}
	return true;
}
size_t IAgent::ReceiveData(void *pBuf, size_t nSize, size_t nMemb)
{
	const char *pData = static_cast<char*>(pBuf);
	if(pData==NULL || nSize*nMemb<=0)
	{
		return 0;
	}
	m_strReceiveData.append(pData, nSize*nMemb);
	return nSize*nMemb;
}
void IAgent::AddHeaderData(std::string strData)
{
	m_pHeaderData = curl_slist_append(m_pHeaderData, strData.c_str());
	/* pass our list of custom made headers */
	//curl_easy_setopt(m_pURL, CURLOPT_HTTPHEADER, m_pHeaderData);
	//curl_easy_perform(m_pURL); /* transfer http */
	//curl_slist_free_all(headers); /* free the header list */
}
void IAgent::SetHeaderData(std::string strHeader)
{

	
	curl_easy_setopt(m_pURL, CURLOPT_HEADERDATA, strHeader.c_str());
}

//////////////////////////////////////////////////////////////////////////

CHttpClient::CHttpClient(void) : m_pURLMgr(NULL)
{
}


CHttpClient::~CHttpClient(void)
{
}

CHttpClient& CHttpClient::It()
{
	static CHttpClient it;
	return it;
}

void CHttpClient::Update()
{
	if (!m_pURLMgr)
	{
		return;
	}
	// 请求连接
	{
		int nStillRun = 0;
		while(CURLM_CALL_MULTI_PERFORM == curl_multi_perform(m_pURLMgr, &nStillRun))
		{
		}
	}
	//// 检查超时
	//{
	//	fd_set fdread;
	//	fd_set fdwrite;
	//	fd_set fdexcep;
	//	FD_ZERO(&fdread);
	//	FD_ZERO(&fdwrite);
	//	FD_ZERO(&fdexcep);
	//	/* set a suitable timeout to play around with */
	//	struct timeval timeout;
	//	timeout.tv_sec = 10;
	//	timeout.tv_usec = 0;
	//	int maxfd = 0;
	//	curl_multi_fdset(m_pURLMgr, &fdread, &fdwrite, &fdexcep, &maxfd);
	//	int rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
	//	switch(rc) {
	//	case -1:
	//		/* select error */
	//		break;
	//	case 0:
	//		/* timeout, do something else */
	//		break;
	//	default:
	//		/* one or more of curl's file descriptors say there's data to read
	//		or write */
	//		break;
	//	}
	//}
	// 处理已完成的消息
	{
		CURLMsg *pMsg = NULL;
		int nMsgLeft = 0;
		while(NULL != (pMsg = curl_multi_info_read(m_pURLMgr, &nMsgLeft)))
		{
			ProcessMsg(pMsg);
			RemoveMsg(pMsg);
		}
	}
}

bool CHttpClient::ProcessMsg(CURLMsg *pMsg)
{
	assert(pMsg);
	assert(pMsg->easy_handle);
	CURL *pURL = pMsg->easy_handle;
	CURLcode result = pMsg->data.result;
	//When msg is CURLMSG_DONE, the message identifies a transfer that
	//	is done, and then result contains the return code for the easy handle
	//	that just completed.
	//	At this point, there are no other msg types defined.
	switch(pMsg->msg)
	{
	case CURLMSG_DONE:
		{
			MapAgent::iterator it = m_mapAgent.find(pURL);
			if (it != m_mapAgent.end())
			{
				
				IAgent *pAgent = it->second;
				assert(pAgent);
				return pAgent->ReceiveComplete(result);
			}
			else
			{
				assert(false&&"it != m_mapAgent.end()");
			}
		}
		break;
	default:
		assert(false&&"At this point, there are no other msg types defined.");
		break;
	}
	return true;
}

bool CHttpClient::RemoveMsg(CURLMsg *pMsg)
{
	CURL *pUrl = pMsg->easy_handle;
	// Remove
	curl_multi_remove_handle(m_pURLMgr, pUrl);
	// Release Agent
	{
		MapAgent::iterator it = m_mapAgent.find(pUrl);
		if (it != m_mapAgent.end())
		{
			IAgent *pAgent = it->second;
			assert(pAgent);
			pAgent->Release();
			m_mapAgent.erase(it);
		}
		else
		{
			assert(false&&"it==m_mapAgent.end()? Shit! What a FUCK?");
			// 
			curl_easy_cleanup(pUrl);
		}
	}
	return true;
}

bool CHttpClient::AddAgent(IAgent *pAgent)
{
	if (pAgent == NULL)
	{
		assert(false);
		return false;
	}
	CURL* pURL = pAgent->GetURL();
	if (!pURL)
	{
		assert(false);
		return false;
	}
	MapAgent::iterator it = m_mapAgent.find(pURL);
	if( it != m_mapAgent.end() )
	{
		assert(false);
		return false;
	}
	curl_easy_setopt(pURL, CURLOPT_WRITEFUNCTION, &ReceiveHttpData);
	curl_easy_setopt(pURL, CURLOPT_WRITEDATA, pAgent);
	CURLMcode ret = curl_multi_add_handle(m_pURLMgr, pURL);
	if (ret != CURLM_OK)
	{
		assert(false);
		return false;
	}
	m_mapAgent[pURL] = pAgent;
	return true;
}

size_t CHttpClient::ReceiveHttpData(void *pBuf, size_t nSize, size_t nMemb, void *pUser)
{
	IAgent *pAgent = static_cast<IAgent*>(pUser);
	if (!pAgent)
	{
		return 0;
	}
	return pAgent->ReceiveData(pBuf, nSize, nMemb);
}

bool CHttpClient::Init()
{
	if (m_pURLMgr != NULL)
	{
		return false;
	}
	CURLcode ret = curl_global_init(CURL_GLOBAL_ALL);
	if (ret != CURLE_OK)
	{
		return false;
	}
	m_pURLMgr = curl_multi_init();
	if (m_pURLMgr == NULL)
	{
		return false;
	}
	return true;
}

bool CHttpClient::Release()
{
	curl_multi_cleanup(m_pURLMgr);
	while(!m_mapAgent.empty())
	{
		MapAgent::iterator it = m_mapAgent.begin();
		IAgent *pAgent = it->second;
		assert(pAgent);
		pAgent->Release();
		m_mapAgent.erase(it);
	}
	curl_global_cleanup();
	m_pURLMgr = NULL;
	return true;
}

bool CHttpClient::ReqAgent(IAgent *pAgent)
{
	if (pAgent == NULL)
	{
		return false;
	}
	bool ret = false;
	if (pAgent->Init())
	{
		ret = AddAgent(pAgent);
	}
	if (!ret)
	{
		pAgent->Release();
	}
	return ret;
}
