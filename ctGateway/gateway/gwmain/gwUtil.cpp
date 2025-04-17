//gwUtil.cpp 辅助函数

#include "../interface/gwUtil.h" 
#include<openssl/md5.h>
#include "myZIP.h"
#include "mimetype.h"


int getFileCheckSum(char const* fullname, string& checksum)
{
	CEasyFile easyfile;
	CBuffer filedata;
	int file_size;
	if (easyfile.ReadFile(fullname, filedata))
	{
		unsigned  char  md5[16];
		MD5((unsigned char*)filedata.data(), filedata.size(), md5);
		file_size = filedata.size();
		checksum= CMyTools::ToHex((char*)md5, 16);
	}
	else
	{
		checksum= "err";
		file_size = 0;
	}
	return file_size;
}

string& TryCompressMessasge(string& msg)
{
	CBuffer cBuffer;
	if (!CMyZip::Compress(msg.c_str(), msg.size(), cBuffer))
	{
		thelog << "压缩失败" << ende;
	}
	else
	{
		unsigned char* _buf = new unsigned char[1 + msg.size() * 2];
		unsigned char* base64buf = _buf + 1;
		_buf[0] = 'Z';
		long base64Size = CBase64::Base64Enc(base64buf, (unsigned char*)cBuffer.getbuffer(), cBuffer.size());
		thelog << "原始长度 " << msg.size() << " 压缩后 " << cBuffer.size() << " 压缩率 " << (100 - cBuffer.size() * 100. / (msg.size() ? msg.size() : 1)) << "%  base64Size " << base64Size << " " << (100 - base64Size * 100. / (msg.size() ? msg.size() : 1)) << "%" << endi;

		if (base64Size + 1 < (long)msg.size())
		{
			msg = (char*)_buf;
		}
		delete[]_buf;
	}
	return msg;
}

string& TryUnCompressMessasge(string& msg)
{
	if ('Z' == msg[0])
	{
		CBuffer cBuffer;
		unsigned char* encryptedBuf = new unsigned char[msg.size()];
		long encryptedSize = CBase64::Base64Dec((char*)encryptedBuf, msg.c_str() + 1, msg.size() - 1);
		if (encryptedSize < 0)
		{
			thelog << msg.size() - 1 << " base64解码失败 " << encryptedSize << ende;
		}
		//thelog << msg.size() - 1 << " base64解码 " << encryptedSize << endi;
		if (!CMyZip::UnCompress((char*)encryptedBuf, encryptedSize, cBuffer))
		{
			thelog << "解压缩失败" << ende;
		}
		else
		{
			msg = cBuffer.data();
			//thelog << "=============================" << endl << msg << endi;
		}
	}
	return msg;
}

string const& getTmpDir()
{
	// TODO: 在此处插入 return 语句
	static string tmpdir;
	if (tmpdir.size() == 0)
	{
		CEasyFile easyfile;
		string dirname = "/var/volatile";
		if (easyfile.IsFileExist(dirname.c_str()))
		{
			tmpdir = dirname + "/";
		}
		else
		{
			tmpdir = "./";
		}
	}
	return tmpdir;
}
