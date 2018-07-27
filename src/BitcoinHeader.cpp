#include "Koyn.h"

BitcoinHeader::BitcoinHeader()
{
	setNull();
}

BitcoinHeader::BitcoinHeader(uint8_t * hdr,uint32_t _height)
{
	setHeader(hdr,_height);
}

BitcoinHeader::BitcoinHeader(const BitcoinHeader &hdr)
{
	memcpy(this->completeHeader,hdr.completeHeader,4);
	memcpy(this->completeHeader+4,hdr.completeHeader+4,32);
	memcpy(this->completeHeader+36,hdr.completeHeader+36,32);
	memcpy(this->completeHeader+68,hdr.completeHeader+68,4);
	memcpy(this->completeHeader+72,hdr.completeHeader+72,4);
	memcpy(this->completeHeader+76,hdr.completeHeader+76,4);
	memcpy(this->hash,hdr.hash,32);
	this->height = hdr.height;
	this->pos = hdr.pos;
	this->isValid = hdr.isValid;
}

void BitcoinHeader::setHeader(uint8_t * hdr,uint32_t _height)
{
	isValid = true;
	memcpy(completeHeader,hdr,4);
	memcpy(completeHeader+4,hdr+4,32);
	memcpy(completeHeader+36,hdr+36,32);
	memcpy(completeHeader+68,hdr+68,4);
	memcpy(completeHeader+72,hdr+72,4);
	memcpy(completeHeader+76,hdr+76,4);
	height = _height;
	calcPos();
}
bool BitcoinHeader::isHeaderValid()
{
	return isValid;
}

void BitcoinHeader::setNull()
{
	memset(completeHeader, 0, 80);
  	memset(hash, 0, 32);
	pos = -1;
	height = -1;
	isValid = false;
	parentHeader = false;
}

uint8_t BitcoinHeader::getVersion(uint8_t * container)
{
	memcpy(container,completeHeader,4);
	return 4;
}

uint8_t BitcoinHeader::getPrevHash(uint8_t * container)
{
	memcpy(container,completeHeader+4,32);
	return 32;
}

uint8_t BitcoinHeader::getMerkle(uint8_t * container)
{
	memcpy(container,completeHeader+36,32);
	return 32;
}

uint8_t BitcoinHeader::getTimeStamp(uint8_t * container)
{
	memcpy(container,completeHeader+68,4);
	return 4;
}

uint8_t BitcoinHeader::getBits(uint8_t * container)
{
	memcpy(container,completeHeader+72,4);
	return 4;
}

uint8_t BitcoinHeader::getNonce(uint8_t * container)
{
	memcpy(container,completeHeader+76,4);
	return 4;
}

uint8_t BitcoinHeader::getHash(uint8_t * container)
{
	calcHash();
	memcpy(container,hash,32);
	return 32;
}

uint32_t BitcoinHeader::getVersionAsInt()
{
	uint32_t temp;
	memcpy(&temp, completeHeader, sizeof(temp));
	return temp;
}

uint32_t BitcoinHeader::getTimeStampAsInt()
{
	uint32_t temp;
	memcpy(&temp, completeHeader+68, sizeof(temp));
	return temp;
}

uint32_t BitcoinHeader::getBitsAsInt()
{
	uint32_t temp;
	memcpy(&temp, completeHeader+72, sizeof(temp));
	return temp;
}

uint32_t BitcoinHeader::getNonceAsInt()
{
	uint32_t temp;
	memcpy(&temp, completeHeader+76, sizeof(temp));
	return temp;
}

int32_t BitcoinHeader::getHeight()
{
	return height;
}

int16_t BitcoinHeader::getPos()
{
	return pos;
}

void BitcoinHeader::calcPos()
{
	int32_t temp = height;
	if(!temp)
	{
		pos = 0;
		return ;
	}
	while(temp%2016!=0)
	{
		temp++;
	}
	pos = 2016-(temp-height);
}

void BitcoinHeader::calcHash()
{
	doubleSha256(hash,completeHeader,80);
}

bool BitcoinHeader::operator==(BitcoinHeader& other)
{
    if(this->getHeight()==other.getHeight()&&!memcmp(this->completeHeader,other.completeHeader,80))
    {
    	return true;
    }else
    {
    	return false;
    }
}

bool BitcoinHeader::operator!=(BitcoinHeader& other)
{
    if(this->getHeight()!=other.getHeight()||memcmp(this->completeHeader,other.completeHeader,80))
    {
    	return true;
    }else
    {
    	return false;
    }
}
#if defined(ENABLE_DEBUG_MESSAGES)
void BitcoinHeader::printHeader()
{
	char hdr2[161]={};
	hdr2[160]='\0';
	bin2hex(hdr2,completeHeader,80);
	Serial.print(hdr2);
}
#endif