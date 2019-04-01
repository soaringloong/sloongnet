#pragma once
#include "IEvent.h"

namespace Sloong
{
	namespace Events
	{
		class CNormalEvent : public IEvent
		{
		public:
			CNormalEvent();
			~CNormalEvent();

			void SetEvent(EVENT_TYPE t);
			EVENT_TYPE GetEvent();

			void SetParams(SMARTER p);
			SMARTER GetParams();

			void SetMessage(string str);
			string GetMessage();

			void SetCallbackFunc(LPSMARTFUNC func);
			LPSMARTFUNC GetCallbackFunc();
			void CallCallbackFunc(SMARTER pParams);

			LPVOID GetHandler();
			void SetHandler(LPVOID obj);
		protected:
			LPSMARTFUNC m_pCallbackFunc = nullptr;
			SMARTER m_pParams = nullptr;
			EVENT_TYPE m_emType;
			LPVOID m_pObj;
			string m_strMessage;
		};
	}	
}