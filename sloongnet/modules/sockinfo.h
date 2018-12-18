#ifndef SOCKINFO_H
#define SOCKINFO_H


#include "lconnect.h"

#include "DataTransPackage.h"

#include "IObject.h"
using namespace Sloong::Interface;

namespace Sloong
{
	namespace Universal
	{
		class CLuaPacket;
	}
    typedef struct _PrepareSendInfo
    {
        shared_ptr<CDataTransPackage> pSendInfo;
        int nPriorityLevel;
    }PRESENDINFO;


	using namespace Universal;
	class CSockInfo : IObject
	{
	private:
		CSockInfo(){}
	public:
		CSockInfo( int nPriorityLevel , bool ,bool );
		~CSockInfo();

		void Initialize(IMessage* iMsg, IData* iData,int sock, SSL_CTX* ctx);

		/**
		 * @Remarks: When data can receive, should call this function to receive the package.
		 * @Params: 
		 * @Return: if receive done, return Succeed.
		 * 		if happened errors, return Error.
		 * @Note: It always read all data in one time, so no return Retry.
		 */
		NetworkResult OnDataCanReceive();

		/**
		 * @Remarks: When data can send, should call this function to send the package.
		 * @Params: 
		 * @Return: if send done, return Succeed.
		 * 			if happened erros, return Error.
		 * 			if happened EAGAIN signal, return Retry. 
		 */
		NetworkResult OnDataCanSend();

	protected:
		void ProcessPrepareSendList();
		NetworkResult ProcessSendList();
		int GetSendInfoList(queue<shared_ptr<CDataTransPackage>>*& list );
		shared_ptr<CDataTransPackage> GetSendInfo(queue<shared_ptr<CDataTransPackage>>* list);

	public:
        queue<shared_ptr<CDataTransPackage>>* m_pSendList; // the send list of the bytes.
        queue<PRESENDINFO> m_oPrepareSendList;

		time_t m_ActiveTime;
		shared_ptr<lConnect> m_pCon;

		unique_ptr<CLuaPacket> m_pUserInfo;
        mutex m_oSockReadMutex;
        mutex m_oSockSendMutex; 
		mutex m_oSendListMutex;
        mutex m_oPreSendMutex;
		int m_nPriorityLevel;
		bool m_bEnableMD5Check;
		bool m_bEnableSwiftNumber;
		int m_nLastSentTags = -1;
        bool m_bIsSendListEmpty = true;
	};

}

#endif // SOCKINFO_H
