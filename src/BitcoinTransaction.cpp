#include "Koyn.h"


BitcoinTransaction::BitcoinTransaction()
{
	rawTx = NULL;
	height = -1;
	isInBlock=false;
	inputNo=0;
	outNo=0;
	unconfirmedIterations=0;
}

BitcoinTransaction::BitcoinTransaction(const BitcoinTransaction *transaction)
{}

bool BitcoinTransaction::setRawTx(char * rawData , uint32_t len)
{
	if(len%2!=0){return false;}
	#if defined(ENABLE_DEBUG_MESSAGES)
	Serial.println(F("Finding memory for raw transaction"));
	#endif
	rawTx = (uint8_t*)malloc(sizeof(uint8_t)*(len/2));
	if(rawTx == NULL){
	#if defined(ENABLE_DEBUG_MESSAGES)
	Serial.println(F("Cannot locate memory"));
	#endif 
	return false;
	}
	hex2bin(rawTx,rawData,len);
	/* Start parsing from rawData */
	if(rawTx[4]==0x00){
	free(rawTx);rawTx= NULL;
	#if defined(ENABLE_DEBUG_MESSAGES)
	Serial.println(F("SegWit not supported"));
	#endif
	return false;
	} /* Currently segwit transaction are not supported */
	transcationLength = len/2;
	/* Getting no of inputs, currently we are supporting 1 byte (255 addresses) as input but anyway we still have a
	   a rawTx size limitation by default and the user can change it */
	inputNo = rawTx[4];
	uint8_t * lastInptAdd = &rawTx[5];
	for(int i=0;i<inputNo;i++)
	{
		lastInptAdd+=36;
		uint32_t scriptSigLen = 0;
		if(lastInptAdd[0]>0xfc)
		{
			/* This means that the script signature length is more than 1 byte */
			switch (lastInptAdd[0])
			{
				uint8_t * temp;
				/* Getting 2 bytes as length */
				case (0xfd):
				temp = lastInptAdd;
				temp+=2;
				scriptSigLen = (scriptSigLen << 8) | temp[0];
				temp-=1;
				scriptSigLen = (scriptSigLen << 8) | temp[0];
				lastInptAdd+=2;
				break;
				/* Getting 3 bytes as length */
				case (0xfe):
				temp = lastInptAdd;
				temp+=4;
				scriptSigLen = (scriptSigLen << 8) | temp[0];
				temp-=1;
				scriptSigLen = (scriptSigLen << 8) | temp[0];
				temp-=1;
				scriptSigLen = (scriptSigLen << 8) | temp[0];
				temp-=1;
				scriptSigLen = (scriptSigLen << 8) | temp[0];
				lastInptAdd+=4;
				break;
			}
		}else
		{
			scriptSigLen = lastInptAdd[0];
		}
		lastInptAdd+=scriptSigLen+5;
	}
	outNo=lastInptAdd[0];
	outputScriptsStart = lastInptAdd+1;
	return true;
}

void BitcoinTransaction::setHeight(int32_t _height)
{
	height = _height;
	unconfirmedIterations=0;
}

uint8_t BitcoinTransaction::getInput(uint8_t index,BitcoinAddress * addr)
{
	if(!addr->isShellAddress){return ADDRESS_TYPE_ERROR;}
	if(index<inputNo)
	{
		uint8_t * lastInptAdd = &rawTx[5];
		for(int i=0;i<index;i++)
		{
			lastInptAdd+=36;
			uint32_t scriptSigLen = 0;
			if(lastInptAdd[0]>0xfc)
			{
					/* This means that the script signature length is more than 1 byte */
				switch (lastInptAdd[0])
				{
					uint8_t * temp;
					/* Getting 2 bytes as length */
					case (0xfd):
					temp = lastInptAdd;
					temp+=2;
					scriptSigLen = (scriptSigLen << 8) | temp[0];
					temp-=1;
					scriptSigLen = (scriptSigLen << 8) | temp[0];
					lastInptAdd+=2;
					break;
					/* Getting 3 bytes as length */
					case (0xfe):
					temp = lastInptAdd;
					temp+=4;
					scriptSigLen = (scriptSigLen << 8) | temp[0];
					temp-=1;
					scriptSigLen = (scriptSigLen << 8) | temp[0];
					temp-=1;
					scriptSigLen = (scriptSigLen << 8) | temp[0];
					temp-=1;
					scriptSigLen = (scriptSigLen << 8) | temp[0];
					lastInptAdd+=4;
					break;
				}
			}else
			{
				scriptSigLen = lastInptAdd[0];
			}
			lastInptAdd+=scriptSigLen+5;
		}
		lastInptAdd+=36;
		if(lastInptAdd[0]>0xfc)
		{
			/* This means that the script signature length is more than 1 byte */
			switch (lastInptAdd[0])
			{
				/* Getting 2 bytes as length */
				case 0xfd:
				lastInptAdd+=2;break;
				case 0xfe:
				lastInptAdd+=3;break;
			}
		}
		lastInptAdd+=1;
		if(lastInptAdd[0]==OP_0)
		{
			/* This means that the script is a P2SH script */
		}else
		{
			/* P2PKH */
			uint8_t sigLen = lastInptAdd[0];
			lastInptAdd+=sigLen+1;
		}
		if(lastInptAdd[0]>33)
		{
			uint8_t publicKey[65];
			memcpy(publicKey,lastInptAdd+1,65);
			uECC_compress(publicKey,addr->compPubKey,addr->curve);
			addr->calculateAddress(KEY_PUBLIC);
		}else
		{
			memcpy(addr->compPubKey,lastInptAdd+1,33);
			addr->calculateAddress(KEY_COMPRESSED_PUBLIC);
		}
		return ADDRESS_RETRIEVED;
	}else
	{
		return INDEX_ERROR;
	}
}

uint8_t BitcoinTransaction::getOutput(uint8_t index,BitcoinAddress * addr)
{
	if(!addr->isShellAddress){return ADDRESS_TYPE_ERROR;}
	if(index<outNo)
	{
		uint8_t * lastoutAdd = outputScriptsStart;
		for(int i=0;i<index;i++)
		{
			lastoutAdd+=9+lastoutAdd[8];
		}

		if(lastoutAdd[9]==OP_DUP&&lastoutAdd[10]==OP_HASH160)
		{
			lastoutAdd+=11;
		}else if(lastoutAdd[9]==OP_HASH160)
		{
			lastoutAdd+=10;
		}
		memcpy(addr->compPubKey,lastoutAdd+1,lastoutAdd[0]);
		addr->calculateAddress(KEY_SCRIPT_HASH);
		return ADDRESS_RETRIEVED;
	}else
	{
		return INDEX_ERROR;
	}
}

uint64_t BitcoinTransaction::getOutputAmount(uint8_t index)
{
	if(index<outNo)
	{
		uint8_t * lastoutAdd = outputScriptsStart;
		uint64_t temp;
		for(int i=0;i<index;i++)
		{
			lastoutAdd+=9+lastoutAdd[8];
		}
		memcpy(&temp,lastoutAdd,8);
		return temp;
	}else
	{
		return INDEX_ERROR;
	}
}

uint8_t BitcoinTransaction::getHash(uint8_t * container)
{
	doubleSha256(container,rawTx,transcationLength);
	return 32;
}

uint8_t BitcoinTransaction::getHash(char * container)
{
	uint8_t hash[32];
	doubleSha256(hash,rawTx,transcationLength);
	bin2hex(container,hash,32);
	container[65]='\0';
	return 64;
}

uint32_t BitcoinTransaction::getBlockNumber()
{
	return height;
}

bool BitcoinTransaction::inBlock()
{
	if(height>0)
	{
		return true;
	}else
	{
		return false;
	}
}

bool BitcoinTransaction::isUsed()
{
	if(rawTx!=NULL){return true;}else{return false;}
}

void BitcoinTransaction::resetTx()
{
	free(rawTx);
	rawTx = NULL;
	height = -1;
	isInBlock=false;
	unconfirmedIterations=0;
}

uint8_t BitcoinTransaction::getInputsCount()
{
	return inputNo;
}

uint8_t BitcoinTransaction::getOutputsCount()
{
	return outNo;
}

uint32_t BitcoinTransaction::getConfirmations()
{
	if(inBlock())
	{
		return (Koyn.getBlockNumber()-getBlockNumber())+1; 
	}else
	{
		unconfirmedIterations++;
		return 0;
	}
}

uint8_t BitcoinTransaction::getUnconfirmedIterations()
{
	return unconfirmedIterations;
}