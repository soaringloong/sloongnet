#include "DataTransPackage.h"

void Sloong::CDataTransPackage::Initialize(SmartConnect conn, CLog* pLog)
{
	m_pCon = conn;	
	m_pLog = pLog;
}

void Sloong::CDataTransPackage::PrepareSendPackageData()
{
	if (m_pLog!= nullptr)
	{
		m_pLog->Verbos(CUniversal::Format("SEND<<<[%d][%d]<<<%s&&&EXDATA<<<[%d]",m_pTransPackage->prioritylevel(),
										m_pTransPackage->serialnumber(),m_pTransPackage->content(), m_pTransPackage->extend().length()));
	}
	m_pTransPackage->SerializeToString(&m_strPackageData);
	m_nPackageSize = (int)m_strPackageData.length();
}

void Sloong::CDataTransPackage::RequestPackage( shared_ptr<DataPackage> pack )
{
	m_pTransPackage = pack;
	PrepareSendPackageData();
}


void Sloong::CDataTransPackage::ResponsePackage( shared_ptr<DataPackage> pack )
{
	m_pTransPackage = pack;
	PrepareSendPackageData();
}


void Sloong::CDataTransPackage::ResponsePackage(ResultType result, const string& message,const string* exdata /*=nullptr*/)
{
	m_pTransPackage->set_result(result);
	m_pTransPackage->set_content(message);
	if( exdata ) m_pTransPackage->set_extend(*exdata);
	PrepareSendPackageData();
}

void Sloong::CDataTransPackage::ResponsePackage(const CResult& result)
{
	m_pTransPackage->set_result(result.Result());
	m_pTransPackage->set_content(result.Message());
	PrepareSendPackageData();
}

/**
 * @Remarks: 
 * @Params: 
 * @Return: if send succeed, return true. need listen read message. 
 *          else return false, need listen write message.
 */
ResultType Sloong::CDataTransPackage::SendPackage()
{
	int nSentSize = m_pCon->SendPackage(m_strPackageData, m_nSent);
	if (nSentSize < 0){
		if( m_pLog) m_pLog->Warn(CUniversal::Format("Error when send data. Socket[%s:%d].Errno[%d].", m_pCon->m_strAddress, m_pCon->m_nPort, m_pCon->GetErrno()));
		return ResultType::Error;
	}else{
		m_nSent += nSentSize;
	}

	if( m_pLog ) m_pLog->Verbos(CUniversal::Format("Send Info : AllSize[%d],Sent[%d]", m_nPackageSize, m_nSent));

	if (m_nSent < m_nPackageSize){
		return ResultType::Retry;
	}else{
		if( m_pLog ) m_pLog->Verbos(CUniversal::Format("Message package send succeed, remove from send list. All size[%d]", m_nSent));
		return ResultType::Succeed;
	}
}

ResultType Sloong::CDataTransPackage::RecvPackage(int timeout)
{
	string result;
	auto net_res = m_pCon->RecvPackage(result,timeout);
	if( net_res != ResultType::Succeed )
	{
		if( m_pLog ) m_pLog->Warn(CUniversal::Format("Error when receive data. Socket[%s:%d].Errno[%d].", m_pCon->m_strAddress, m_pCon->m_nPort, m_pCon->GetErrno() ));
		return net_res;
	}

	m_pTransPackage = make_shared<DataPackage>();
	if(!m_pTransPackage->ParseFromString(result))
	{
		if( m_pLog ) m_pLog->Error("Parser receive data error.");
		return ResultType::Error;
	}

	if (m_pTransPackage->prioritylevel() > s_PriorityLevel || m_pTransPackage->prioritylevel() < 0)
	{
		if( m_pLog ) m_pLog->Error(CUniversal::Format("Receive priority level error. the data is %d, the config level is %d. add this message to last list", m_pTransPackage->prioritylevel(), s_PriorityLevel));
		return ResultType::Error;
	}
	
	if( m_pLog ) m_pLog->Verbos(CUniversal::Format("RECV<<<[%d][%d]<<<%s",m_pTransPackage->prioritylevel(),m_pTransPackage->serialnumber(),m_pTransPackage->content()));

	if( m_pTransPackage->checkstring().length() > 0 ){
		string rmd5 = CMD5::Encode(m_pTransPackage->content());
		if ( strcasecmp(m_pTransPackage->checkstring().c_str(),rmd5.c_str()) != 0)
		{
			// handle error.
			string msg = CUniversal::Format("MD5 check fialed.Message:[%s].recv MD5:[%s].local md5[%s]",m_pTransPackage->content(), rmd5, m_pTransPackage->checkstring() );
			if( m_pLog )m_pLog->Warn(msg);
			ResponsePackage(ResultType::Error,msg);
			return ResultType::Invalid;
		}
	}

	return ResultType::Succeed;
}