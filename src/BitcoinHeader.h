#ifndef BitcoinHeader_h
#define BitcoinHeader_h

class BitcoinHeader
{
public:
	BitcoinHeader();
	BitcoinHeader(uint8_t *,uint32_t );
	BitcoinHeader(const BitcoinHeader &);
	uint8_t getVersion(uint8_t *);
	uint8_t getPrevHash(uint8_t *);
	uint8_t getMerkle(uint8_t *);
	uint8_t getTimeStamp(uint8_t *);
	uint8_t getBits(uint8_t *);
	uint8_t getNonce(uint8_t *);
	uint8_t getHash(uint8_t *);
	uint32_t getVersionAsInt();
	uint32_t getTimeStampAsInt();
	uint32_t getBitsAsInt();
	uint32_t getNonceAsInt();
	int16_t getPos();
	int32_t getHeight();
	bool isHeaderValid();
	bool operator==(BitcoinHeader& );
	bool operator!=(BitcoinHeader&);
	void setHeader(uint8_t *,uint32_t );
	#if defined(ENABLE_DEBUG_MESSAGES)
	void printHeader();
	#endif
private:
	uint8_t completeHeader[80];
	uint8_t hash[32];
	int16_t pos;
	int32_t height;
	bool isValid;
	bool parentHeader;
	void calcPos();
	void calcHash();
	void setNull();
	friend class KoynClass;
	friend class BitcoinFork;
};

#endif
