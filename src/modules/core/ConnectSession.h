/*
 * @Author: WCB
 * @Date: 1970-01-01 08:00:00
 * @LastEditors: WCB
 * @LastEditTime: 2020-04-17 12:30:12
 * @Description: file content
 */
#ifndef SOCKINFO_H
#define SOCKINFO_H

#include "IObject.h"

#include "EasyConnect.h"
namespace Sloong
{
    typedef queue<unique_ptr<DataPackage>> ReceivePackageList;
    typedef TResult<ReceivePackageList> ReceivePackageListResult;
	class ConnectSession : IObject
	{
	public:
		ConnectSession();
		~ConnectSession();

		void Initialize(IControl *, UniqueConnection); 

		/**
		 * @Remarks: When data can receive, should call this function to receive the package.
		 * @Params: 
		 * @Return: if receive done, return Succeed.
		 * 		if happened errors, return Error.
		 * @Note: It always read all data in one time, so no return Retry.
		 */
		ReceivePackageListResult OnDataCanReceive();

		/**
		 * @Remarks: When data can send, should call this function to send the package.
		 * @Params: 
		 * @Return: if send done, return Succeed.
		 * 			if happened erros, return Error.
		 * 			if happened EAGAIN signal, return Retry. 
		 */
		ResultType OnDataCanSend();

		/**
		 * @Remarks: When need response data package,call this function. 
		 * @Params: 
		 * @Return: if send data succeed, return Succeed.
		 * 			if happened erros, return Error.
		 * 			if have extend data or all data is no send and have EAGAIN sinal , return Retry.
		 */
		ResultType SendDataPackage(UniquePackage);



		inline bool TrySendLock()
		{
			return m_oSockSendMutex.try_lock();
		}
		inline bool TryReceiveLock()
		{
			return m_oSockReadMutex.try_lock();
		}

	protected:
		void ProcessPrepareSendList();
		ResultType ProcessSendList();
		queue_ex<UniquePackage> *GetSendPackage();
		void AddToSendList(UniquePackage);

	public:
		queue_ex<UniquePackage> *m_pSendList; // the send list of the bytes.
		queue_ex<UniquePackage> m_oPrepareSendList;

		time_t m_ActiveTime;
		UniqueConnection m_pConnection = nullptr;

		mutex m_oSockReadMutex;
		mutex m_oSockSendMutex;
		bool m_bIsSendListEmpty = true;
	};

} // namespace Sloong

#endif // SOCKINFO_H