#include "Koyn.h"



AddressHistory::AddressHistory()
{
	memset(txHash,0,32);
	height = -1;
	leafPos = 0;
	lastMerkleVerified = false;
	isFirstMerkle = true;
	historyFileLastPos = 0;
}

AddressHistory::AddressHistory(const AddressHistory &history)
{
	memcpy(this->txHash,history.txHash,32);
	this->height = history.height;
	this->leafPos = history.leafPos;
}

uint8_t AddressHistory::getTxHash(uint8_t * containter)
{
	memcpy(containter,txHash,32);
	return 32;
}

uint8_t AddressHistory::getStringTxHash(char * txHash_str)
{
	bin2hex(txHash_str,txHash,32);
	return 64;
}

int32_t AddressHistory::getHeight()
{
	return height;
}

uint32_t AddressHistory::getLeafPos()
{
	return leafPos;
}

void AddressHistory::setTxHash(char * _txHash)
{
	hex2bin(txHash,_txHash,64);
}

void AddressHistory::setHeight(int32_t _height)
{
	height = _height;
}

bool AddressHistory::isNull()
{
	if(height == -1 && !memcmp(txHash,emptyArray,32))
	{
		return true;
	}else
	{
		return false;
	}
}

void AddressHistory::setNull()
{
	memset(txHash,0,32);
	height = -1;
	leafPos = 0;
}

uint8_t AddressHistory::copyData(uint8_t * data)
{
	memcpy(txHash,data,32);
	height = (height <<8) | data[35];
	height = (height << 8) | data[34];
	height = (height << 8) | data[33];
	height = (height << 8) | data[32];
	return 36;
}

void AddressHistory::setLeafPos(uint32_t pos)
{
	leafPos= pos;
}

bool AddressHistory::operator==(AddressHistory& other)
{
    if(this->getHeight()==other.getHeight()&&!memcmp(this->txHash,other.txHash,32))
    {
    	return true;
    }else
    {
    	return false;
    }
}
