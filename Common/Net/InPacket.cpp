#include "InPacket.h"
#include <iostream>


InPacket::InPacket(unsigned char* buff, unsigned short size)
	: aBuff(buff),
	  nPacketSize(size), nReadPos(0)
{
}

InPacket::~InPacket()
{
}

char InPacket::Decode1()
{
	if(nReadPos >= nPacketSize || nReadPos + sizeof(char) > nPacketSize)
	{
		std::cout << "InPacket::Decode1 錯誤 : 存取異常。" << std::endl;
		return 0;
	}
	char ret = *(char*)(aBuff + nReadPos);
	nReadPos += sizeof(char);
	return ret;
}

short InPacket::Decode2()
{
	if (nReadPos >= nPacketSize || nReadPos + sizeof(short) > nPacketSize)
	{
		std::cout << "InPacket::Decode2 錯誤 : 存取異常。" << std::endl;
		return 0;
	}
	short ret = *(short*)(aBuff + nReadPos);
	nReadPos += sizeof(short);
	return ret;
}

int InPacket::Decode4()
{
	if (nReadPos >= nPacketSize || nReadPos + sizeof(int) > nPacketSize)
	{
		std::cout << "InPacket::Decode4 錯誤 : 存取異常。" << std::endl;
		return 0;
	}
	int ret = *(int*)(aBuff + nReadPos);
	nReadPos += sizeof(int);
	return ret;
}

long long int InPacket::Decode8()
{
	if (nReadPos >= nPacketSize || nReadPos + sizeof(long long int) > nPacketSize)
	{
		std::cout << "InPacket::Decode8 錯誤 :　存取異常。" << std::endl;
		return 0;
	}
	long long int ret = *(long long int*)(aBuff + nReadPos);
	nReadPos += sizeof(long long int);
	return ret;
}

std::string InPacket::DecodeStr()
{
	if (nReadPos >= nPacketSize || nReadPos + sizeof(short) > nPacketSize)
	{
		std::cout << "InPacket::DecodeStr 錯誤 : 存取異常。 (發生於解析字串長度時)" << std::endl;
		return 0;
	}
	short size = Decode2();
	if (nReadPos >= nPacketSize || nReadPos + size > nPacketSize)
	{
		std::cout << "InPacket::DecodeStr 錯誤 : 存取異常。 (發生於解析字串時)" << std::endl;
		return "null";
	}
	std::string ret((char*)aBuff + nReadPos, size);
	nReadPos += size;
	return ret;
}

void InPacket::DecodeBuffer(unsigned char* dst, int size)
{
	if (nReadPos >= nPacketSize || nReadPos + size > nPacketSize)
	{
		std::cout << "InPacket::DecodeBuffer 錯誤 : 存取異常。" << std::endl;
		return;
	}
	if(dst)
		memcpy(dst, aBuff + nReadPos, size);
	nReadPos += size;
}

void InPacket::Print()
{
	std::cout << "InPacket封包內容：";
	for (int i = 0; i < nPacketSize; ++i)
		printf("0x%02X ", (int)aBuff[i]);
	std::cout << std::endl;
}

unsigned char* InPacket::GetPacket() const
{
	return aBuff;
}

unsigned short InPacket::GetPacketSize() const
{
	return nPacketSize;
}

void InPacket::RestorePacket()
{
	nReadPos = 0;
}