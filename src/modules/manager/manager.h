/*
 * @Author: WCB
 * @Date: 2019-11-05 08:59:19
 * @LastEditors: WCB
 * @LastEditTime: 2020-05-14 14:13:35
 * @Description: file content
 */
#ifndef SLOONGNET_MODULE_MANAGER_MAIN_H
#define SLOONGNET_MODULE_MANAGER_MAIN_H

#include "core.h"
#include "export.h"
#include "servermanage.h"

extern "C"
{
	CResult RequestPackageProcesser(void *, CDataTransPackage *);
	CResult ResponsePackageProcesser(void *, CDataTransPackage *);
	CResult EventPackageProcesser(CDataTransPackage *);
	CResult NewConnectAcceptProcesser(ConnectSession *);
	CResult ModuleInitialization(GLOBAL_CONFIG *);
	CResult ModuleInitialized(SOCKET, IControl *);
	CResult CreateProcessEnvironment(void **);
}

namespace Sloong
{
	class CConfiguation;
	class SloongControlService
	{
	public:
		SloongControlService() {}
		virtual ~SloongControlService() {}

		CResult Initialization(GLOBAL_CONFIG *);
		CResult Initialized(IControl *);

		inline CResult CreateProcessEnvironmentHandler(void **);
		void EventPackageProcesser(CDataTransPackage *);

	protected:
		void ResetControlConfig(GLOBAL_CONFIG *);

		void OnSocketClose(SharedEvent);

	protected:
		list<unique_ptr<CServerManage>> m_listServerManage;
		IControl *m_pControl = nullptr;
		CLog *m_pLog = nullptr;
		GLOBAL_CONFIG *m_pConfig;
		string m_strDBFilePath="./configuation.db";

	public:
		static unique_ptr<SloongControlService> Instance;
	};

} // namespace Sloong

#endif //SLOONGNET_MODULE_MANAGER_MAIN_H
