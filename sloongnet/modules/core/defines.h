/*
 * @Author: WCB
 * @Date: 1970-01-01 08:00:00
 * @LastEditors: WCB
 * @LastEditTime: 2020-05-14 14:24:10
 * @Description: file content
 */
#ifndef SLOONGNET_DEFINES_H
#define SLOONGNET_DEFINES_H

// univ head file
#include "univ/defines.h"
#include "univ/univ.h"
#include "univ/log.h"
#include "univ/exception.h"
#include "univ/threadpool.h"
#include "univ/hash.h"
#include "univ/lua.h"
#include "univ/luapacket.h"
#include "univ/base64.h"
using namespace Sloong;
using namespace Sloong::Universal;


#include "protocol/core.pb.h"
using namespace Core;

template<typename T> inline
T TYPE_TRANS(LPVOID p)
{
	T tmp = static_cast<T>(p);
	assert(tmp);
	return tmp;
}


const int s_llLen = 8;
const int s_lLen = 4;
const int s_PriorityLevel = 5;


enum RecvStatus
{
	Wait=0,
	Receiving=1,
	Saveing=2,
	Done=3,
	VerificationError=4,
	OtherError=5,
};

// Receive file struce for GFunc
struct RecvDataPackage
{
	string strMD5 = "";
	RecvStatus emStatus = RecvStatus::Wait;
	string strName = "";
	string strPath = "";
};

enum HashType
{
	MD5 = 0,
	SHA_1 = 1,
	SHA_256 = 2,
	SHA_512 = 3,
};

template<class T> inline
unique_ptr<T> ConvertStrToObj(string obj){
	unique_ptr<T> item = make_unique<T>();
	if(!item->ParseFromString(obj))
		return nullptr;
	return item;
}

inline string ConvertObjToStr(::google::protobuf::Message* obj){
	string str_res;
	if(!obj->SerializeToString(&str_res))
		return "";
	return str_res;
}

#endif