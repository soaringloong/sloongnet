#include "NetworkCenter.h"
#include "serverconfig.h"
#include "epollex.h"
#include "sockinfo.h"
#include "NetworkEvent.h"
#include "IData.h"

using namespace Sloong::Events;

Sloong::CNetworkCenter::CNetworkCenter()
{
    m_pEpoll = make_unique<CEpollEx>();
}

Sloong::CNetworkCenter::~CNetworkCenter()
{
	if (m_pCTX)
	{
		SSL_CTX_free(m_pCTX);
	}
}


void Sloong::CNetworkCenter::Initialize(IControl* iMsg)
{
    IObject::Initialize(iMsg);

    m_pConfig = IData::GetServerConfig();
    m_pEpoll->Initialize(m_iC);

	if( m_pConfig->m_nPriorityLevel < 0 || m_pConfig->m_nPriorityLevel > 5 )
	{
		throw new normal_except("PriorityLevel only can 0-5. please check.");
	}

    if (m_pConfig->m_bEnableSSL)
	{
		EnableSSL(m_pConfig->m_strCertFile, m_pConfig->m_strKeyFile, m_pConfig->m_strPasswd);
	}

	if (m_pConfig->m_nClientCheckTime > 0 && m_pConfig->m_strClientCheckKey.length() > 0)
	{
		m_bEnableClientCheck = true;
		m_nClientCheckKeyLength = m_pConfig->m_strClientCheckKey.length();
	}

	m_pEpoll->SetEventHandler(std::bind(&CNetworkCenter::OnNewAccept, this, std::placeholders::_1),
		std::bind(&CNetworkCenter::OnDataCanReceive, this, std::placeholders::_1),
		std::bind(&CNetworkCenter::OnCanWriteData, this, std::placeholders::_1),
		std::bind(&CNetworkCenter::OnOtherEventHappened, this, std::placeholders::_1));

	m_iC->RegisterEvent(MSG_TYPE::ReveivePackage);
	m_iC->RegisterEvent(MSG_TYPE::SocketClose);
    m_iC->RegisterEvent(MSG_TYPE::SendMessage);
	m_iC->RegisterEvent(MSG_TYPE::MonitorSendStatus);
	m_iC->RegisterEventHandler(MSG_TYPE::ProgramStart, std::bind(&CNetworkCenter::Run, this, std::placeholders::_1));
	m_iC->RegisterEventHandler(MSG_TYPE::ProgramExit, std::bind(&CNetworkCenter::Exit, this, std::placeholders::_1));
	m_iC->RegisterEventHandler(MSG_TYPE::SendMessage, std::bind(&CNetworkCenter::SendMessageEventHandler,this,std::placeholders::_1));
	m_iC->RegisterEventHandler(MSG_TYPE::SocketClose, std::bind(&CNetworkCenter::CloseConnectEventHandler, this, std::placeholders::_1));
	m_iC->RegisterEventHandler(MSG_TYPE::MonitorSendStatus, std::bind(&CNetworkCenter::MonitorSendStatusEventHandler, this, std::placeholders::_1));
}

void Sloong::CNetworkCenter::Run(SmartEvent event)
{
    m_bIsRunning = true;
	m_pEpoll->Run(m_pConfig->m_nPort,m_pConfig->m_nEPoolThreadQuantity);
    CThreadPool::AddWorkThread( std::bind(&CNetworkCenter::CheckTimeoutWorkLoop, this, std::placeholders::_1), nullptr);
}

void Sloong::CNetworkCenter::Exit(SmartEvent event)
{
    m_bIsRunning = false;
    m_oSync.notify_all();
	m_pEpoll->Exit();
}



void Sloong::CNetworkCenter::SendMessageEventHandler(SmartEvent event)
{
	auto send_evt = dynamic_pointer_cast<CNetworkEvent>(event);
	SmartPackage pack = send_evt->GetDataPackage();
	int socket = pack->GetSocketID();
	shared_ptr<CSockInfo> info = m_SockList[socket];
	
	auto res = info->ResponseDataPackage(pack);
	if (res == NetworkResult::Retry )
	{
		m_pEpoll->MonitorSendStatus(socket);
	}
	if (res == NetworkResult::Error)
	{
		SendCloseConnectEvent(socket);
	}
}


void Sloong::CNetworkCenter::CloseConnectEventHandler(SmartEvent event)
{
	auto net_evt = dynamic_pointer_cast<CNetworkEvent>(event);
	auto id = net_evt->GetSocketID();
	auto item = m_SockList.find(id);
	if (item == m_SockList.end())
		return;

	shared_ptr<CSockInfo> info = m_SockList[id];
	unique_lock<mutex> sockLck(m_oSockListMutex);
	m_SockList.erase(item);
	sockLck.unlock();
	if (!info)
		return;

	// in here no need delete the send list and read list
	// when delete the SocketInfo object , it will delete the list .
	info->m_pCon->Close();
	m_pLog->Info(CUniversal::Format("close connect:%s:%d.", info->m_pCon->m_strAddress, info->m_pCon->m_nPort));
}


void Sloong::CNetworkCenter::MonitorSendStatusEventHandler(SmartEvent event)
{
	auto net_evt = dynamic_pointer_cast<CNetworkEvent>(event);
	m_pEpoll->MonitorSendStatus(net_evt->GetSocketID());
}


void Sloong::CNetworkCenter::SendCloseConnectEvent(int socket)
{
	shared_ptr<CSockInfo> info = m_SockList[socket];
	if(info==nullptr)
		return;

	auto event = make_shared<CNetworkEvent>(MSG_TYPE::SocketClose);
	event->SetSocketID(socket);
	event->SetUserInfo(info->m_pUserInfo.get());
	event->SetHandler(this);
	m_iC->SendMessage(event);
}

void Sloong::CNetworkCenter::EnableSSL(string certFile, string keyFile, string passwd)
{
	int ret = lConnect::G_InitializeSSL(m_pCTX,certFile, keyFile, passwd);
	if (ret != S_OK)
	{
		m_pLog->Error("Initialize SSL environment error.");
		m_pLog->Error(lConnect::G_FormatSSLErrorMsg(ret));
	}
}


/*************************************************
* Function: * check_connect_timeout
* Description: * 检测长时间没反应的网络连接，并关闭删除
* Input: *
* Output: *
* Others: *
*************************************************/
void Sloong::CNetworkCenter::CheckTimeoutWorkLoop(SMARTER param)
{
	int tout = m_pConfig->m_nConnectTimeout * 60;
	int tinterval = m_pConfig->m_nTimeoutInterval * 60;

	m_pLog->Verbos("Check connect timeout thread is running.");
	while (m_bIsRunning)
	{
		m_pLog->Verbos("Check connect timeout start.");
		RecheckTimeout:
		unique_lock<mutex> sockLck(m_oSockListMutex);
		for (map<int, shared_ptr<CSockInfo>>::iterator it = m_SockList.begin(); it != m_SockList.end(); ++it)
		{
			if (it->second != NULL && time(NULL) - it->second->m_ActiveTime > tout)
			{
				sockLck.unlock();
				m_pLog->Info(CUniversal::Format("[Timeout]:[Close connect:%s]", it->second->m_pCon->m_strAddress));
				SendCloseConnectEvent(it->first);
				goto RecheckTimeout;
			}
		}
		sockLck.unlock();
		m_pLog->Verbos(CUniversal::Format("Check connect timeout done. wait [%d] seconds.", tinterval));
		m_oSync.wait_for(tinterval);
	}
	m_pLog->Info("check timeout connect thread is exit ");
}



/// 有新链接到达。
/// 接收链接之后，需要客户端首先发送客户端校验信息。只有校验成功之后才会进行SSL处理
NetworkResult Sloong::CNetworkCenter::OnNewAccept( int conn_sock )
{
	m_pLog->Verbos("Accept function is called.");

		// start client check when acdept
		if (m_bEnableClientCheck)
		{
			char* pCheckBuf = new char[m_nClientCheckKeyLength + 1];
			memset(pCheckBuf, 0, m_nClientCheckKeyLength + 1);
			// In Check function, client need send the check key in 3 second. 
			// 这里仍然使用Universal提供的ReceEx。这里不需要进行SSL接收
			int nLen = CUniversal::RecvEx(conn_sock, pCheckBuf, m_nClientCheckKeyLength, m_pConfig->m_nClientCheckTime);
			if (nLen != m_nClientCheckKeyLength || 0 != strcmp(pCheckBuf, m_pConfig->m_strClientCheckKey.c_str()))
			{
				m_pLog->Warn(CUniversal::Format("Check Key Error.Length[%d]:[%d].Server[%s]:[%s]Client", m_nClientCheckKeyLength, nLen, m_pConfig->m_strClientCheckKey.c_str(), pCheckBuf));
				return NetworkResult::Error;
			}
		}

		auto info = make_shared<CSockInfo>();
		info->Initialize(m_iC,conn_sock,m_pCTX);
	
		unique_lock<mutex> sockLck(m_oSockListMutex);
		m_SockList[conn_sock] = info;
		sockLck.unlock();

		m_pLog->Info(CUniversal::Format("Accept client:[%s:%d].", info->m_pCon->m_strAddress, info->m_pCon->m_nPort));
		
		return NetworkResult::Succeed;
}

NetworkResult Sloong::CNetworkCenter::OnDataCanReceive( int nSocket )
{
    shared_ptr<CSockInfo> info = m_SockList[nSocket];

	if (info == NULL)
	{
		m_pLog->Error("OnDataCanReceive called, but SockInfo is null.");
		return NetworkResult::Error;
	}
		
 
	auto res = info->OnDataCanReceive();
	if( res  == NetworkResult::Error)
		SendCloseConnectEvent(nSocket);
	return res;
}

NetworkResult Sloong::CNetworkCenter::OnCanWriteData( int nSocket )
{
    // 可以写入事件
	shared_ptr<CSockInfo> info = m_SockList[nSocket];

	if (info == NULL)
	{
		m_pLog->Error("OnCanWriteData called, but SockInfo is null.");
		return NetworkResult::Error;
	}

	auto res = info->OnDataCanSend();
	if( res  == NetworkResult::Error)
		SendCloseConnectEvent(nSocket);
	return res;
}


NetworkResult Sloong::CNetworkCenter::OnOtherEventHappened( int nSocket )
{
	SendCloseConnectEvent(nSocket);
	return NetworkResult::Succeed;
}