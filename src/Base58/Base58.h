#include <stdio.h>
#include <string.h>

#define BASE58_DECODE_BASE 0
#define BASE58_ENCODE_BASE 0
#define BASE58_DECODE_OUT_OF_RANGE 1

#if defined(ENABLE_DEBUG_MESSAGES)
inline void print_array(unsigned char* arr, int len){
	int i = 0;
	for (i=0; i<len; ++i){
		Serial.printf("%02X ",arr[i]);
	}
	Serial.printf("\n");
}
#endif

static unsigned char alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

inline int base58Encode(uint8_t * input,const unsigned int inLen,char *output,unsigned int outLen)
{
	int i,j,tmp;
	memset(output,0,outLen);
	for(i=0;i<inLen;i++)
	{
		unsigned int c = input[i] & (0xff) ;
		for(j=outLen-1;j>=0;j--)
		{
			tmp = output[j] * 256 + c;
			c = tmp/58;
			output[j] = tmp%58;
		}
	}
	for(j=0; j<outLen; ++j)
	{
		output[j] = alphabet[output[j]];
	}
	return BASE58_ENCODE_BASE;
}




inline int base58Decode(const char* addr,const unsigned int addrLen,uint8_t *buf,unsigned int bufLen)
{
	int i,j;
	unsigned int tmp;
	memset(buf,0,bufLen);
	for(i=0;i<addrLen;i++)
	{
		unsigned int c = addr[i];

		if (addr[i] < 58){ // Numbers
			c -= 49;
		}else if (addr[i] < 73){ // A-H
			c -= 56;
		}else if (addr[i] < 79){ // J-N
			c -= 57;
		}else if (addr[i] < 91){ // P-Z
			c -= 58;
		}else if (addr[i] < 108){ // a-k
			c -= 64;
		}else{ // m-z
			c -= 65;
		}

		for(j=bufLen-1;j>=0;j--)
		{
			tmp = buf[j] * 58 + c;
			c = (tmp & (~0xff)) >> 8;
			buf[j] = tmp & (0xff);
		}
	}

	return BASE58_DECODE_BASE;
}

