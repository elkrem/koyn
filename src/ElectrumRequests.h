#ifndef ElectrumRequests_h
#define ElectrumRequests_h





#define VERSION_STRING    "2.7.11"


#define VERSION_BIT						0
#define ADRRESS_HISTORY_BIT				1
#define ADDRESS_BALANCE_BIT				2
#define ADDRESS_MEMPOOL_BIT				3
#define ADDRESS_UTXO_BIT				4
#define BLOCK_CHUNKS_BIT				5
#define TRANSACTION_BIT					6
#define BLOCK_HEADER_BIT				7
#define HEADERS_SUBS_BIT				8
#define ADDRESS_SUBS_BIT				9
#define PEERS_SUBS_BIT					10
#define BLOCKS_NO_SUBS_BIT				11
#define RELAY_FEE_BIT					12
#define BROADCAST_TRANSACTION_BIT		13
#define MERKLE_PROOF					14

#define BLOCK_NUM_SUB					0
#define BLOCK_HEAD_SUB					1
#define ADDRESS_SUB						2

class ElectrumRequestData
{
private:
	static uint32_t reqId;
	uint32_t localReqId;
	uint32_t reqType;
	uint8_t  dataString[32];
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
	void sendVersion();
	void subscribeToPeers();
	void subscribeToBlocksNumber();
	void subscribeToBlockHeaders();
	void subscribeToAddress(const char  *);
	void getAddressHistory(const char  *);
	void getAddressBalance(const char  *);
	void getMempool(const char  *);
	void getBlockHeader(int );
	void getMerkleProof(const char * , int );
	void listUtxo(const char *);
	void getBlockChunks(int);
	void relayFee();
	void broadcastTransaction(File *);
	void getTransaction(const char *);
	ElectrumRequestData * getElectrumRequestData();
	ElectrumRequestData * getElectrumRequestData(int);
private:
	ElectrumRequestData electrumRequestDataArray[MAX_REQUEST_NO];
	JsonMessage jsonMessage;
friend class KoynClass;
friend class JsonListener;
};

#endif
