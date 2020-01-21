﻿#include "main.h"
#include "version.h"
#include "ServiceBuilder.h"
#include "utility.h"
#include "fstream_ex.hpp"

void PrintVersion()
{
	cout << PRODUCT_TEXT << endl;
	cout << VERSION_TEXT << endl;
	cout << COPYRIGHT_TEXT << endl;
}

string RegisteToControl(SmartConnect con, string uuid, ModuleType type)
{
	auto req = make_shared<DataPackage>();
	req->set_function(Functions::RegisteServer);
	req->set_receiver(Protocol::ModuleType::Control);
	req->set_sender(uuid);
	req->set_content(ModuleType_Name(type));

	CDataTransPackage dataPackage;
	dataPackage.Initialize(con);
	dataPackage.RequestPackage(req);
	ResultType result = dataPackage.SendPackage();
	if (result != ResultType::Succeed)
		throw string("Send Registe request error.");
	result = dataPackage.RecvPackage(0,0);
	if (result != ResultType::Succeed)
		throw string("Receive Registe result error.");
	auto get_config_response_buf = dataPackage.GetRecvPackage();
	if (!get_config_response_buf)
		throw string("Parse Registe response data error.");

	return get_config_response_buf->content();
}

string GetConfigFromControl(SmartConnect con,string uuid)
{
	auto get_config_request_buf = make_shared<DataPackage>();
	get_config_request_buf->set_function(Functions::GetServerConfig);
	get_config_request_buf->set_receiver(Protocol::ModuleType::Control);
	get_config_request_buf->set_sender(uuid);

	CDataTransPackage dataPackage;
	dataPackage.Initialize(con);
	dataPackage.RequestPackage(get_config_request_buf);
	ResultType result = dataPackage.SendPackage();
	if (result != ResultType::Succeed)
		throw string("Send get config request error.");
	result = dataPackage.RecvPackage(0,0);
	if (result != ResultType::Succeed)
		throw string("Receive get config result error.");
	auto get_config_response_buf = dataPackage.GetRecvPackage();
	if (!get_config_response_buf)
		throw string("Parse the get config response data error.");

	return get_config_response_buf->extend();
}

void PrientHelp()
{
	cout << "sloongnet [<type>] [<address:port>]" << endl;
	cout << "<type>: control,process,proxy,firewall,gateway,data,db" << endl;
}


unique_ptr<GLOBAL_CONFIG> Initialize(int argc, char** args)
{
	// 参数共有2个，类型和地址信息
	// 1 类型，为指定值
	// 2 地址信息：格式->地址：端口。如果类型为control，表示为监听地址
	if (argc != 3)
	{
		PrientHelp();
		return nullptr;
	}

	ModuleType type = Unconfigured;
	if (!ModuleType_Parse(args[1], &type))
	{
		cout << "Parse module type error." << endl;
		return nullptr;
	}
	cout << "Run application by " << ModuleType_Name(type) << " module" << endl;

	vector<string> addr = CUniversal::split(args[2], ':');
	if (addr.size() != 2)
	{
		cout << "Address info format error. Format [addr]:[port]" << endl;
		return nullptr;
	}

	auto config = make_unique<GLOBAL_CONFIG>();
	config->set_type(type);

	if( type == ModuleType::Control)
	{
		int port = atoi(addr[1].c_str());
		if (port == 0)
		{
			cout << "Convert [" << args[1] << "] to int port fialed." << endl;
			return nullptr;
		}
		config->set_listenport(port);
	}
	else
	{
		auto con = make_shared<EasyConnect>();
		con->Initialize(args[2], nullptr);
		if (!con->Connect())
		{
			cout << "Connect to control fialed." << endl;
			return nullptr;
		}
		cout << "Connect to control succeed." << endl;
		string uuid;
		fstream_ex::read_all("uuid.dat", uuid);
		string server_uuid = RegisteToControl(con, uuid, type);
		if (uuid != server_uuid)
		{
			fstream_ex::write_all("uuid.dat", server_uuid);
			uuid = server_uuid;
		}
				
		cout << "Start get configuation." << endl;
		auto serverConfig = GetConfigFromControl(con,uuid);
		if (serverConfig.size() == 0)
		{
			cout << "Control no return config infomation. please check." << endl;
			return nullptr;
		}
		if (!config->ParseFromString(serverConfig))
		{
			cout << "Parse the config struct error. please check." << endl;
			return nullptr;
		}
		
		cout << "Get configuation succeed." << endl;
	}
	return config;
}


int main(int argc, char** args)
{
	try
	{
		CResult code = CResult::Succeed;
		do
		{
			auto config = Initialize(argc, args);
			if (!config) {
				cout << "Initialize error." << endl;
				return -1;
			}

			Sloong::CSloongBaseService::g_pAppService = ServiceBuilder::Build(config.get());
			if (!Sloong::CSloongBaseService::g_pAppService) {
				cout << "Create service object error." << endl;
				return -2;
			}

			auto res = Sloong::CSloongBaseService::g_pAppService->Initialize(config);
			if (!res.IsSucceed()) {
				cout << "Initialize server error. Message: " << res.Message() << endl;
				return -3;
			}

			code = Sloong::CSloongBaseService::g_pAppService->Run();
		} while (code.Result() == ResultType::Retry);
		return 0;
	}
	catch (string & msg)
	{
		cout << "exception happened, message:" << msg << endl;
	}
	catch (exception & exc)
	{
		cout << "exception happened, message:" << exc.what() << endl;
	}
	catch (...)
	{
		cout << "Unhandle exception happened, system will shutdown. " << endl;
		CUtility::write_call_stack();
	}
}
