#include "Koyn.h"



AddressHistory::AddressHistory()
{
	memset(txHash,0,32);
	height = -1;
	leafPos = 0;
}

AddressHistory::AddressHistory(const AddressHistory &history)
{
	/* Copy hash. */
	memcpy(this->txHash,history.txHash,32);
	/* Copy height. */
	this->height = history.height;
	this->leafPos = history.leafPos;
}

void AddressHistory::getTxHash(uint8_t * containter)
{
	memcpy(containter,txHash,32);
}

void AddressHistory::getStringTxHash(char * txHash_str)
{
	bin2hex(txHash_str,txHash,32);
}

int32_t AddressHistory::getHeight()
{
	return this->height;
}

uint32_t AddressHistory::getLeafPos()
{
	return this->leafPos;
}

void AddressHistory::setTxHash(char * _txHash)
{
	hex2bin(txHash,_txHash,64);
}

void AddressHistory::setHeight(int32_t _height)
{
	this->height = _height;
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

void AddressHistory::copyData(uint8_t * data)
{
	memcpy(txHash,data,32);
	height = (height <<8) | data[35];
	height = (height << 8) | data[34];
	height = (height << 8) | data[33];
	height = (height << 8) | data[32];
}

void AddressHistory::setLeafPos(uint32_t pos)
{
	this->leafPos= pos;
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
