#include "servermanage.h"

#include "utility.h"
#include "NetworkEvent.hpp"
#include <jsoncpp/json/json.h>
using namespace Sloong::Events;


CResult Sloong::CServerManage::Initialize()
{
	m_listFuncHandler["RegisteWorker"]=std::bind(&CServerManage::RegisteWorkerHandler, this, std::placeholders::_1,std::placeholders::_2);
	m_listFuncHandler["RegisteNode"]=std::bind(&CServerManage::RegisteNodeHandler, this, std::placeholders::_1,std::placeholders::_2);
	m_listFuncHandler["AddTemplate"]=std::bind(&CServerManage::AddTemplateHandler, this, std::placeholders::_1,std::placeholders::_2);
	m_listFuncHandler["DeleteTemplate"]=std::bind(&CServerManage::DeleteTemplateHandler, this, std::placeholders::_1,std::placeholders::_2);
	m_listFuncHandler["SetTemplate"]=std::bind(&CServerManage::SetTemplateHandler, this, std::placeholders::_1,std::placeholders::_2);
	m_listFuncHandler["QueryTemplate"]=std::bind(&CServerManage::QueryTemplateHandler, this, std::placeholders::_1,std::placeholders::_2);
	m_listFuncHandler["QueryNode"]=std::bind(&CServerManage::QueryNodeHandler, this, std::placeholders::_1,std::placeholders::_2);

	if(!CConfiguation::Instance->IsInituialized())
	{
		auto res = CConfiguation::Instance->Initialize("/data/configuation.db");
		if (res.IsFialed()) return res;
	}

	// Initialize template list
	auto list = CConfiguation::Instance->GetTemplateList();
	for (auto item : list)
	{
		TemplateItem addItem(item);
		m_oTemplateList[addItem.ID] = addItem;
	}

	auto res = CConfiguation::Instance->GetTemplate(1);
	if (res.IsFialed())
		return CResult::Succeed();
	else
		return res;
}


CResult Sloong::CServerManage::ResetManagerTemplate(GLOBAL_CONFIG* config)
{
	string config_str;
	if(!config->SerializeToString(&config_str))
	{
		return CResult::Make_Error("Config SerializeToString error.");
	}
	TemplateInfo info;
	info.id = 1;
	info.name = "Manager";
	info.note = "This template just for the manager node.";
	info.replicas = 1;
	info.configuation = CBase64::Encode(config_str);
	CResult res(ResultType::Succeed);
	if( CConfiguation::Instance->CheckTemplateExist(1))
		res = CConfiguation::Instance->SetTemplate(1,info);
	else
		res = CConfiguation::Instance->AddTemplate(info,nullptr);
	m_oTemplateList[1] = info;
	return res;
}

int Sloong::CServerManage::SearchNeedCreateTemplate()
{
	// First time find the no created
	for (auto item : m_oTemplateList)
	{
		if (item.second.Replicas == 0 || item.second.ID == 1 )
			continue;

		if (item.second.Created.size() == item.second.Replicas)
			continue;

		if (item.second.Created.size() == 0)
			return item.first;
	}

	// Sencond time find the created < replicas
	for (auto item : m_oTemplateList)
	{
		if (item.second.Replicas == 0 || item.second.ID == 1 )
			continue;

		if (item.second.Created.size() == item.second.Replicas)
			continue;

		if (item.second.Created.size() < item.second.Replicas)
			return item.first;
	}
	return -1;
}


CResult Sloong::CServerManage::ProcessHandler(CDataTransPackage* pack)
{
	Json::Reader reader;
	Json::Value jReq;
	auto str_req = pack->GetRecvMessage();
	if (!reader.parse(str_req, jReq))
	{
		pack->ResponsePackage(ResultType::Error, CUniversal::Format("Parser json [%s] error.", str_req));
		return CResult::Succeed();
	}
	if (jReq["Function"].isNull())
	{
		pack->ResponsePackage(ResultType::Error, "Request no set [Function] node.");
		return CResult::Succeed();
	}
	auto function = jReq["Function"].asString();
	m_pLog->Verbos(CUniversal::Format("Request [%s]:[%s]", function, str_req));
	if(!m_listFuncHandler.exist(function))
	{
		pack->ResponsePackage(ResultType::Error, CUniversal::Format("Function [%s] no handler.",function));
		return CResult::Succeed();
	}

	auto res = m_listFuncHandler[function](jReq,pack);

	m_pLog->Verbos(CUniversal::Format("Response [%s]:[%s][%s].", function, Protocol::ResultType_Name(res.Result()), res.Message() ));
	pack->ResponsePackage(res);
	return CResult::Succeed();
}


CResult Sloong::CServerManage::RegisteWorkerHandler(const Json::Value& jRequest,CDataTransPackage* pack)
{
	auto sender = pack->GetRecvPackage()->sender();
	if( sender.length() == 0 )
	{
		sender = CUtility::GenUUID();
	}
	auto sender_info = m_oWorkerList.try_get(sender);
	if( sender_info == nullptr)
	{
		ServerItem item;
		item.Address = pack->GetSocketIP();
		item.Port = pack->GetSocketPort();
		item.UUID = sender;
		m_oWorkerList[sender] = item;
		m_pLog->Verbos(CUniversal::Format("Module[%s:%d] regist to system. Allocating uuid [%s].", item.Address, item.Port, item.UUID));
		return CResult(ResultType::Retry,sender);
	}

	auto index = SearchNeedCreateTemplate();
	if (index == -1)
	{
		return CResult(ResultType::Retry,sender);
	}
	
	if( sender_info == nullptr)
	{
		return CResult::Make_Error("Add server info to ServerList fialed.");
	}
	
	auto tpl = m_oTemplateList[index];
	
	Json::Value root;
	root["TemplateID"] = tpl.ID ;
	root["Configuation"] = m_oTemplateList[tpl.ID].Configuation;
	m_pLog->Verbos(CUniversal::Format("Allocating module[%s] Type to [%s]", sender_info->UUID, tpl.Name));
	return CResult::Make_OK(root.toStyledString());
}


CResult Sloong::CServerManage::RegisteNodeHandler(const Json::Value& jRequest,CDataTransPackage* pack)
{
	auto sender = pack->GetRecvPackage()->sender();
	if( !jRequest["TemplateID"].isInt() || sender.length() == 0)
		return CResult::Make_Error("The required parameter check error.");
	
	int id = jRequest["TemplateID"].asInt();
	if(!m_oTemplateList.exist(id))
		return CResult::Make_Error(CUniversal::Format("The template id [%d] is no exist.",id));
	
	if( !m_oWorkerList.exist(sender))
		return CResult::Make_Error(CUniversal::Format("The sender [%d] is no regitser.",sender));

	if( id == 1)
		return CResult::Make_Error("Template id error.");

	m_oWorkerList[sender].TemplateName = m_oTemplateList[id].Name;
	m_oWorkerList[sender].TemplateID = m_oTemplateList[id].ID;
	m_oTemplateList[id].Created.unique_insert(sender);
	return CResult::Succeed();
}

CResult Sloong::CServerManage::AddTemplateHandler(const Json::Value& jRequest,CDataTransPackage* pack)
{
	auto jReq = jRequest["Template"];
	if(!jReq["Name"].isString()&&!jReq["Replicas"].isInt()&&!jReq["Configuation"].isString())
	{
		return CResult::Make_Error("The required parameter check error.");
	}

	TemplateItem item;
	item.Name = jReq["Name"].asString();
	item.Note = jReq["Note"].asString();
	item.Replicas = jReq["Replicas"].asInt();
	item.Configuation = jReq["Configuation"].asString();
	int id = 0;
	auto res = CConfiguation::Instance->AddTemplate(item.ToTemplateInfo(),&id);
	if( res.IsFialed() )
	{
		return res;
	}
	item.ID = id;
	m_oTemplateList[id] = item;
	return CResult::Make_OK(CUniversal::ntos(id));
}


CResult Sloong::CServerManage::DeleteTemplateHandler(const Json::Value& jRequest,CDataTransPackage* pack)
{
	if(!jRequest["TemplateID"].isInt())
	{
		return CResult::Make_Error("The required parameter check error.");
	}

	int id = jRequest["TemplateID"].asInt();
	if( !m_oTemplateList.exist(id))
	{
		return CResult::Make_Error(CUniversal::Format("The template id [%d] is no exist.",id));
	}
	if( id == 1)
	{
		return CResult::Make_Error("Cannot delete this template.");
	}

	auto res = CConfiguation::Instance->DeleteTemplate(id);
	if( res.IsFialed() )
	{
		return res;
	}
	m_oTemplateList.erase(id);
	return CResult::Succeed();
}


CResult Sloong::CServerManage::SetTemplateHandler(const Json::Value& jRequest,CDataTransPackage* pack)
{
	auto jReq = jRequest["Template"];
	if (!jReq["ID"].isInt() || !m_oTemplateList.exist(jReq["ID"].asInt()))
	{
		return CResult::Make_Error("Check the templeate ID error, please check.");;
	}
	
	auto info = m_oTemplateList[jReq["ID"].asInt()];
	if (!jReq["Name"].isNull())
		info.Name = jReq["Name"].asString();

	if (!jReq["Note"].isNull())
		info.Note = jReq["Note"].asString();

	if (!jReq["Replcas"].isNull())
		info.Replicas = jReq["Replcas"].asInt();

	if (!jReq["Configuation"].isNull())
		info.Configuation = jReq["Configuation"].asString();

	auto res = CConfiguation::Instance->SetTemplate(info.ID, info.ToTemplateInfo());
	if (res.IsFialed())
	{
		return res;
	}

	m_oTemplateList[info.ID] =  info;
	return CResult::Succeed();
}

CResult Sloong::CServerManage::QueryTemplateHandler(const Json::Value& jRequest,CDataTransPackage* pack)
{
	Json::Value list;
	if( jRequest["TemplateID"].isInt() )
	{
		int id = jRequest["TemplateID"].asInt();
		if(!m_oTemplateList.exist(id))
		{
			return CResult::Make_Error(CUniversal::Format("The template id [%d] is no exist.",id));
		}

		list.append(m_oTemplateList[id].ToJson());
	}
	else
	{
		for (auto& i : m_oTemplateList)
		{
			list.append(i.second.ToJson());
		}
	}
	
	Json::Value root;
	root["TemplateList"] = list;
	return CResult::Make_OK(root.toStyledString());
}

CResult Sloong::CServerManage::QueryNodeHandler(const Json::Value& jRequest,CDataTransPackage* pack)
{
	Json::Value list;
	for (auto& i : m_oWorkerList)
	{
		list.append(i.second.ToJson());
	}
	Json::Value root;
	root["NodeList"] = list;
	return CResult::Make_OK(root.toStyledString());
}
