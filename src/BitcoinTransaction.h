#ifndef BitcoinTransaction_h
#define BitcoinTransaction_h



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

