#ifndef BitcoinTransaction_h
#define BitcoinTransaction_h



class BitcoinTransaction{

public:
	BitcoinTransaction();
	BitcoinTransaction(const BitcoinTransaction *);
	uint8_t getInput(uint8_t ,BitcoinAddress *);
	uint8_t getOutput(uint8_t,BitcoinAddress *);
	uint64_t getOutputAmount(uint8_t);
	uint8_t getInputsCount();
	uint8_t getOutputsCount();
	uint8_t getHash(uint8_t *);
	uint8_t getHash(char *);
	uint32_t getBlockNumber();
	uint32_t getConfirmations();
private:
	uint32_t transcationLength;
	uint8_t * rawTx;
	uint8_t * outputScriptsStart;
	int32_t height;
	uint8_t inputNo;
	uint8_t outNo;
	uint8_t unconfirmedIterations;
	bool isInBlock;
	bool inBlock();
	bool isUsed();
	void resetTx();
	bool setRawTx(char*,uint32_t);
	void setHeight(int32_t);
	uint8_t getUnconfirmedIterations();
	friend class KoynClass;
};


#endif

