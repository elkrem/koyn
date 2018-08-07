#ifndef ElectrumRequests_h
#define ElectrumRequests_h


class ElectrumRequestData
{
private:
	static uint32_t reqId;
	uint32_t localReqId;
	uint32_t reqType;
	uint8_t  dataString[65];
	int dataInt;
	bool isUsed;
public:
	ElectrumRequestData();
	void setReqType(uint32_t);
	void setUsed();
	void resetUsed();
	bool isReqUsed();
	uint32_t getReqId();
	uint32_t getReqType();
	uint8_t * getDataString();
	uint32_t getDataInt();
friend class ElectrumRequests;
friend class KoynClass;
friend class JsonListener;
};

class ElectrumRequests
{
public:
	ElectrumRequests();
	void ping();
	void sendVersion();
	void subscribeToPeers();
	void subscribeToBlockHeaders();
	void subscribeToAddress(const char  *);
	void getAddressHistory(const char  *);
	void getAddressBalance(const char  *);
	void getMempool(const char  *);
	void getBlockHeader(int );
	void getMerkleProof(const char *,const char * , int );
	void listUtxo(const char *);
	void getBlockChunks(int);
	void relayFee();
	void broadcastTransaction(File *);
	void getTransaction(const char *);
	void resetRequests();
	ElectrumRequestData * getElectrumRequestData();
	ElectrumRequestData * getElectrumRequestData(unsigned int);
private:
	ElectrumRequestData electrumRequestDataArray[MAX_PARALLEL_REQUESTS];
friend class KoynClass;
friend class JsonListener;
};

#endif
