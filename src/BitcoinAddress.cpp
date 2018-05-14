#include "Koyn.h"


BitcoinAddress::BitcoinAddress(const char * key,uint8_t keyType)
{
	tracked = false;
	gotAddressRef = false;
	init();
	uint8_t keyLen = strlen(key);
	if(keyType == KEY_ENCODED && (keyLen >=25 && keyLen <=35))
	{
		/* This is a normal address set by user. make sure to add error if address doesn't comply to the chosen network by the user
		*/
		if(address[0]!='m'||address[0]!='n'||address[0]!='2'||address[0]!='1'||address[0]!='3')
		{
			// #error Wrong address;
		}
		for(int i=0;i<keyLen;i++)
		{
			if(address[i]=='0'||address[i]=='O'||address[i]=='I'||address[i]=='l')
			{
				// #error Wrong address;
			}
		}
		memcpy(address,key,keyLen);
	}else if(keyType == KEY_PRIVATE && keyLen  == 64)
	{
		/* Original private key */
		hex2bin(privateKey,key,keyLen);
		calculateAddress(KEY_PRIVATE);
	}else if(keyType == KEY_WIF && keyLen == 52)
	{
		/* This is a WIF type private key and we must make sure from the first character.*/
		// #if defined(USE_MAIN_NET)
		// if(key[0]!='5'||key[0]!='K'||key[0]!='L')
		// #error Network doesn't support Key
		// #elif defined(USE_TEST_NET)
		// if(key[0]!='9'||key[0]!='c')
		// #error "Network doesn't support Key"
		// #endif
		uint8_t privKey[38];
		base58Decode(key,keyLen,privKey,38);
		memcpy(privateKey,privKey+1,32);
		calculateAddress(KEY_WIF);
	}else if(keyType == KEY_COMPRESSED_PUBLIC && keyLen == 66)
	{
		uint8_t pubKey[33];
		hex2bin(pubKey,key,keyLen);
		memcpy(compPubKey,pubKey,33);
		calculateAddress(KEY_COMPRESSED_PUBLIC);
	}else if(keyType == KEY_PUBLIC && keyLen == 129)
	{

		uint8_t pubKey[65];
		hex2bin(pubKey,key,keyLen);
		memcpy(publicKey,pubKey,65);
		calculateAddress(KEY_PUBLIC);
	}else
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		/* Something wrong with key*/
		Serial.println("Something wrong with key");
		#endif
	}
}

BitcoinAddress::BitcoinAddress(uint8_t * key,uint8_t keyType)
{
	tracked = false;
	gotAddressRef = false;
	init();
    if(keyType == KEY_PRIVATE)
	{
		/* Original private key */
		memcpy(privateKey,key,32);
		calculateAddress(KEY_PRIVATE);
	}if(keyType == KEY_COMPRESSED_PUBLIC)
	{
		memcpy(compPubKey,key,33);
		calculateAddress(KEY_COMPRESSED_PUBLIC);
	}else if(keyType == KEY_PUBLIC)
	{
		memcpy(publicKey,key,65);
		calculateAddress(KEY_PUBLIC);
	}else
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		/* Something wrong with key*/
		Serial.println("Something wrong with key");
		#endif
	}
}

BitcoinAddress::BitcoinAddress(bool generateKeysImplicitly)
{
	if(generateKeysImplicitly)
	{
		tracked= false;
		init();
		uECC_make_key(publicKey,privateKey,curve);
		calculateAddress(KEY_PRIVATE);
	}else
	{
		tracked= false;
		init();
	}
}

void BitcoinAddress::init()
{
	uECC_set_rng(&RNG);
	curve =  uECC_secp256k1();
	memset(privateKey,0,32);
	memset(publicKey,0,65);
	memset(compPubKey,0,33);
	memset(address,0,36);
	status[64]='\0';
}

void BitcoinAddress::getPrivateKey(uint8_t * container)
{
	memcpy(container,privateKey,32);
}

void BitcoinAddress::getPrivateKey(const char * container)
{
	bin2hex((char*)container,privateKey,32);
}

void BitcoinAddress::getPublicKey(uint8_t * container)
{
	memcpy(container,publicKey,64);
}

void BitcoinAddress::getPublicKey(const char * container)
{
	bin2hex((char*)container,publicKey,65);
}

void BitcoinAddress::getCompressedPublicKey(uint8_t * container)
{
	memcpy(container,compPubKey,33);
}

void BitcoinAddress::getCompressedPublicKey(const char * container)
{
	bin2hex((char*)container,compPubKey,33);
}

void BitcoinAddress::getWif(const char * container)
{
	if(!memcmp(privateKey,emptyArray,32)){return;}
	uint8_t final[38];
	uint8_t hash[32];
	memcpy(final+1,privateKey,32);
	final[0]=0xEF;
	final[33]=0x01;
	doubleSha256(hash,final,34);
	memcpy(final+34,hash,4);
	base58Encode(final,sizeof(final),(char*)container,52);
}

void BitcoinAddress::getEncoded(char * container)
{
	strcpy(container,address);
}

void BitcoinAddress::getStatus(char * container)
{
	memcpy(container,status,65);
}

void BitcoinAddress::calculateAddress(uint8_t keyType)
{
	uint8_t hash[32];
	uint8_t final[25];
	if(keyType == KEY_PRIVATE || keyType == KEY_WIF)
	{
		uECC_compute_public_key(privateKey,publicKey,curve);
		uECC_compress(publicKey,compPubKey,curve);
		sha256(hash,compPubKey,33);
		mbedtls_ripemd160(hash,32,final+1);
	}else if(keyType == KEY_PUBLIC)
	{
		uECC_compress(publicKey,compPubKey,curve);
		sha256(hash,compPubKey,33);
		mbedtls_ripemd160(hash,32,final+1);
	}else if(keyType == KEY_COMPRESSED_PUBLIC)
	{
		sha256(hash,compPubKey,33);
		mbedtls_ripemd160(hash,32,final+1);
	}else if(keyType == KEY_SCRIPT_HASH)
	{
		/* Copying out the script hash we already pasted in the compressed public key array to the final to calculate the address */
		memcpy(final+1,compPubKey,20);
		memset(compPubKey,0,33);
	}
	// #if defined(USE_TEST_NET)
	final[0]=TEST_NET_VERSION;
	// #elif defined(USE_MAIN_NET)
	// versionByteHash[0]=MAIN_NET_VERSION;
	// #endif
	doubleSha256(hash,final,21);
	memcpy(final+21,hash,4);
	base58Encode(final,sizeof(final),address,36);
	for(int i=0;i<36;i++){address[i]=address[i+2];} /* Shifting array to the left */
	/* Quick and Dirty solution for Base58 encoding, as there should be a generic solution for supporting all addresses lenght*/
	address[34]='\0';
	address[35]='\0';
}

bool BitcoinAddress::isTracked()
{
	return tracked;
}

void BitcoinAddress::setTracked()
{
	this->tracked = true;
}

void BitcoinAddress::setConfirmedBalance(uint32_t _confirmedBalance)
{
	this->confirmedBalance = _confirmedBalance;
}

void BitcoinAddress::setUnconfirmedBalance(uint32_t _unconfirmedBalance)
{
	this->unconfirmedBalance = _unconfirmedBalance;
}

uint64_t BitcoinAddress::getBalance()
{
	return this->confirmedBalance;
}

uint64_t BitcoinAddress::getConfirmedBalance()
{
	return this->confirmedBalance;
}

uint64_t BitcoinAddress::getUnconfirmedBalance()
{
	return this->unconfirmedBalance;
}
bool BitcoinAddress::gotAddress()
{
	return gotAddressRef;
}
void BitcoinAddress::setGotAddress()
{
	gotAddressRef = true;
}
void BitcoinAddress::resetGotAddress()
{
	gotAddressRef = false;
}

void BitcoinAddress::setAddressStatus(char * _status)
{
	memcpy(status,_status,64);
}

void BitcoinAddress::getScriptPubKey(uint8_t * container,uint8_t len)
{
	uint8_t data[25];
 	base58Decode(address,strlen(address),data,25);
	if((address[0]=='m'||address[0]=='n'||address[0]=='1')&&len==25)
	{
		container[0]=OP_DUP;
		container[1]=OP_HASH160;
		container[2]=SIZE_OF_RIPE_HASH;
		container[23]=OP_EQUALVERIFY;
		container[24]=OP_CHECKSIG;
		memcpy(container+3,data+1,SIZE_OF_RIPE_HASH);
	}else if((address[0]=='2'||address[0]=='3')&&len==23)
	{
		container[0]=OP_HASH160;
		container[1]=SIZE_OF_RIPE_HASH;
		container[22]=OP_EQUAL;
		memcpy(container+2,data+1,SIZE_OF_RIPE_HASH);
	}
}

uint8_t BitcoinAddress::getAddressType()
{
	char data = address[0];
	switch(data)
	{
		// #if defined(USE_MAIN_NET)
		// case '1':	return P2PKH_ADDRESS;
		// case '3':	return P2SH_ADDRESS;
		// #endif
		// #if defined(USE_TEST_NET)
		case 'm':	return P2PKH_ADDRESS;
		case 'n':	return P2PKH_ADDRESS;
		case '2':	return P2SH_ADDRESS;
		// #endif
	}
}

bool BitcoinAddress::operator==(BitcoinAddress& other)
{
	if(!strcmp(this->address,other.address))
	{
		return true;
	}else
	{
		return false;
	}
}

void BitcoinAddress::clearBalance()
{
	this->confirmedBalance=0;
	this->unconfirmedBalance=0;
}