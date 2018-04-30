#ifndef BitcoinTransaction_h
#define BitcoinTransaction_h

#define MAX_TX_NO	10
#define MAX_RAW_TX_SZ	1000
#define USER_BLOCK_DEPTH	1

#define OP_0  0x00
#define OP_2  0x52
#define OP_PUSHDATA1 0x4C
#define OP_DUP	0x76
#define OP_HASH160 0xA9
#define OP_EQUALVERIFY	0x88
#define OP_CHECKSIG	0xAC
#define OP_EQUAL 	0x87


class BitcoinTransaction{

public:
	BitcoinTransaction();
	BitcoinTransaction(const BitcoinTransaction *);
	bool getInput(uint8_t ,BitcoinAddress *);
	bool getOutput(uint8_t,BitcoinAddress *);
	bool getInput(uint8_t ,char *);
	bool getOutput(uint8_t,char *,uint8_t *);
	uint64_t getOutputAmount(uint8_t);
	uint8_t getInputsCount();
	uint8_t getOutputsCount();
	void getHash(uint8_t *);
	void getHash(const char *);
	uint32_t getBlockNumber();
	uint32_t getConfirmations();

	bool inBlock();
	bool isUsed();
	void resetTx();
	bool setRawTx(char*,uint32_t);
	void setHeight(int32_t);
private:
	uint32_t transcationLength;
	uint8_t * rawTx;
	uint8_t * outputScriptsStart;
	int32_t height;
	uint8_t inputNo;
	uint8_t outNo;
	bool isInBlock;
};


#endif

