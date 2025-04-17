//gwUtil.h 辅助函数

#pragma once

#include <cJSON/cJSON.h> 
#include "myUtil.h" 
using namespace ns_my_std;

//返回的是文件长度
int getFileCheckSum(char const* fullname,string &checksum);

//压缩数据，如果压缩后更大就不压缩，压缩后以'Z'开始
string& TryCompressMessasge(string& msg);
//解数据，如果压缩后更大就不压缩，压缩数据以'Z'开始，否则不处理
string& TryUnCompressMessasge(string& msg);

//获取临时文件目录，如果是当前目录返回“./”
string const& getTmpDir();