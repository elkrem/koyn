#ifndef BitcoinHeader_h
#define BitcoinHeader_h

#define ERROR			0
#define HEADER_VALID	1
#define SAME_HEADER		2
#define FORKED			3
#define FORK_VALID		4
#define INVALID			5

class BitcoinHeader
{
public:
	BitcoinHeader();
	BitcoinHeader(uint8_t *,uint32_t );
	BitcoinHeader(const BitcoinHeader &);
	void getVersion(uint8_t *);
	void getPrevHash(uint8_t *);
	void getMerkle(uint8_t *);
	void getTimeStamp(uint8_t *);
	void getBits(uint8_t *);
	void getNonce(uint8_t *);
	void getHash(uint8_t *);
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
	void printHeader();
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
