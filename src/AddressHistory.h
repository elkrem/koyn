#ifndef AddressHistory_h
#define AddressHistory_h




class AddressHistory{
public:
	AddressHistory();
	AddressHistory(const AddressHistory &);
	uint8_t getTxHash(uint8_t *);
	uint8_t getStringTxHash(char *);
	int32_t getHeight();
	uint32_t getLeafPos();
	void setTxHash(char * );
	void setHeight(int32_t);
	void setNull();
	void setLeafPos(uint32_t);
	uint8_t copyData(uint8_t *);
	bool isNull();
	bool operator==(AddressHistory& );
private:
	uint8_t txHash[32];
	int32_t height;
	uint32_t leafPos;
	bool lastMerkleVerified;
	bool isFirstMerkle;
	uint32_t historyFileLastPos;
	friend class KoynClass;
};



#endif