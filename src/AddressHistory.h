#ifndef AddressHistory_h
#define AddressHistory_h




class AddressHistory{
public:
	AddressHistory();
	AddressHistory(const AddressHistory &);
	void getTxHash(uint8_t *);
	void getStringTxHash(char *);
	int32_t getHeight();
	uint32_t getLeafPos();
	void setTxHash(char * );
	void setHeight(int32_t);
	void setNull();
	void setLeafPos(uint32_t);
	void copyData(uint8_t *);
	bool isNull();
	bool operator==(AddressHistory& );
private:
	uint8_t txHash[32];
	int32_t height;
	uint32_t leafPos;
};



#endif