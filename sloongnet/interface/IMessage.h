#pragma once

#include "main.h"
#include "IEvent.h"
namespace Sloong
{
	typedef std::function<void(shared_ptr<IEvent>)> MsgHandlerFunc;
	
	class IMessage
	{
	public:
		// Data 
		virtual bool Add(DATA_ITEM item, void* object) = 0;
		virtual void* Get(DATA_ITEM item) = 0;
		virtual bool Remove(DATA_ITEM item) = 0;
		virtual bool AddTemp(string name, void* object) = 0;
		virtual void* GetTemp(string name) = 0;
		// Message
		virtual void SendMessage(MSG_TYPE msgType) = 0;
		virtual void SendMessage(SmartEvent evt) = 0;
		virtual void RegisterEvent(MSG_TYPE t) = 0;
		virtual void RegisterEventHandler(MSG_TYPE t, MsgHandlerFunc func) = 0;
	};
}

