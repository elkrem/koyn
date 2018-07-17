#ifndef Hash_h
#define Hash_h


#include "Sha256/Sha256.h"
#include "Ripemd160/Ripemd160.h"


inline void sha256(uint8_t * hashArray,uint8_t * dataToHash, uint32_t len)
{
	Sha256.init();
	for (unsigned int i = 0 ; i < len; i++) {
		Sha256.write(dataToHash[i]);
	}
	memcpy(hashArray, Sha256.result(), 32);
}

inline void transactionHash(uint8_t * hashArray,uint8_t * dataToHash, uint32_t len, uint32_t inputNum)
{
	Sha256.init();
	for(int i=0;i<5;i++)
	{
		Sha256.write(dataToHash[i]);
	}
	uint8_t * startPtr = dataToHash+5;
	uint8_t * endPtr = startPtr+dataToHash[4]*66;
	uint8_t * neededScriptPtr = startPtr+inputNum*66+36;
	while(startPtr!=endPtr)
	{
		for(int i=0;i<36;i++)
		{
			Sha256.write(startPtr[i]);
		}
		startPtr+=36;
		if(neededScriptPtr==startPtr)
		{
			for(int i=0;i<26;i++){Sha256.write(startPtr[i]);}
			startPtr+=26;
		}else
		{
			startPtr+=26;
			Sha256.write((uint8_t)0x00);
		}
		for(int i=0;i<4;i++){Sha256.write(startPtr[i]);}
		startPtr+=4;
	}
	int remainder = len-(5+66*dataToHash[4]);
	for(int i=0;i<remainder;i++){Sha256.write(startPtr[i]);}
	memcpy(hashArray, Sha256.result(), 32);
	Sha256.init();
	for (int i = 0 ; i < 32; i++) {
		Sha256.write(hashArray[i]);
	}
	memcpy(hashArray, Sha256.result(), 32);
}


inline void doubleSha256(uint8_t * hashArray,uint8_t * dataToHash, uint32_t len)
{
	for (int j = 0; j < 2; j++)
	{
		Sha256.init();
		if (j == 0)
		{
			for (unsigned int i = 0 ; i < len; i++) {
				Sha256.write(dataToHash[i]);
			}
		} else if (j == 1)
		{
			for (int i = 0 ; i < 32; i++) {
				Sha256.write(hashArray[i]);
			}
		}
		memcpy(hashArray, Sha256.result(), 32);
	}
}

inline void ripemd160(uint8_t * hashArray,uint8_t * dataToHash, uint32_t len)
{
	mbedtls_ripemd160(dataToHash,len,hashArray);
}
#endif
