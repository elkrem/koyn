#include "Koyn.h"



BitcoinFork::BitcoinFork()
{
	parentHeader.setNull();
	lastHeader.setNull();
}

void BitcoinFork::setNull()
{
	parentHeader.setNull();
	lastHeader.setNull();
}

void BitcoinFork::setParentHeader(BitcoinHeader * currhdr)
{
	parentHeader=*currhdr;
}

void BitcoinFork::setLastHeader(BitcoinHeader * currhdr)
{
	lastHeader=*currhdr;
}

BitcoinHeader * BitcoinFork::getParentHeader()
{
	return &parentHeader;
}

BitcoinHeader * BitcoinFork::getLastHeader()
{
	return &lastHeader;
}

bool BitcoinFork::exists()
{
	if(lastHeader.getHeight()>0)
	{
		return true;
	}else
	{
		return false;
	}
}

bool BitcoinFork::gotParentHeader()
{
	if(parentHeader.getHeight()>0)
	{
		return true;
	}else
	{
		return false;
	}
}
