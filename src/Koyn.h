#ifndef Koyn_h
#define Koyn_h

#include <SPI.h>
#include "SdFat/SdFat.h"
#include "uECC/uECC.h"
#include "Base58/Base58.h"
#include "Ripemd160/Ripemd160.h"
#include <ESP8266WiFi.h>
#include "Config.h"
#include "JsonMessage.h"
#include "ElectrumRequests.h"
#include "Hash.h"
#include "Utils.h"
#include "Servers.h"
#include "JsonStreamingParser/JsonStreamingParser.h"
#include "JsonStreamingParser/JsonListener.h"
#include "BitcoinHeader.h"
#include "BitcoinAddress.h"
#include "AddressHistory.h"
#include "BitcoinTransaction.h"
#include "BitcoinFork.h"

static const uint8_t emptyArray[64]={};

class KoynClass {
public:
	KoynClass();
	void begin(bool=false);
	void run();
	void trackAddress(BitcoinAddress * );
	void unTrackAddress(BitcoinAddress * );
	void unTrackAllAddresses();
	bool isAddressTracked(BitcoinAddress * );
	bool isSynced();
	void onNewTransaction(void (*)(BitcoinTransaction));
	void onNewBlockHeader(void (*)(uint32_t));
	void onError(void (*)(uint8_t));
	void delay(unsigned long);
	uint8_t spend(BitcoinAddress *, BitcoinAddress *, uint64_t , uint64_t , BitcoinAddress *); // return 0 on successfult broadcast
	uint8_t spend(BitcoinAddress *, BitcoinAddress *, uint64_t, uint64_t);
	uint32_t getBlockNumber();
	WiFiClient * getMainClient();
	WiFiClient * getClient(uint16_t);
private:
	SdFat SD;
	JsonListener listener;
	File file;
	WiFiClient clientsArray[MAX_CONNECTED_SERVERS];
	WiFiClient * mainClient;
	ElectrumRequests request;
	ElectrumRequestData * reqData;
	BitcoinTransaction incomingTx[MAX_TRANSACTION_COUNT];
	BitcoinFork forks[MAX_CONNECTED_SERVERS];
	void updateTotalBlockNumb();
	void setMainClient();
	int8_t verifyBlockHeaders(BitcoinHeader*);
	int8_t catchingUpFork(BitcoinHeader*);
	void syncWithServers();
	void connectToServers();
	void processInput(String ,String );
	bool parseReceivedChunk();
	void checkSDCardMounted();
	void checkDirAvailability();
	void parseReceivedTx();
	void getHeaderFromMainChain(BitcoinHeader *,uint32_t);
	void reorganizeMainChain();
	void removeUnconfirmedTransactions();
	bool checkBlckNumAndValidate(int32_t);
	void (*userCallback)(BitcoinTransaction);
	uint32_t lastTimeTaken;
	int32_t totalBlockNumb;
	int32_t fallingBackBlockHeight;
	uint32_t historyFileLastPos;
	uint8_t saveResToFile;
	uint8_t isMessage;
	uint8_t merkleRoot[32];
	bool synchronized;
	bool firstLevel;
	bool requestsSent;
	bool verify;
	bool bigFile;
	bool lastMerkleVerified;
	bool isFirstMerkle;
	bool isCallBackAssigned;
	bool saveNextHistory;
	bool reparseFile;
	BitcoinHeader header;
	BitcoinHeader lastHeader;
	BitcoinHeader prevHeader;
	BitcoinAddress * userAddressPointer;
	AddressHistory addHistory;
	AddressHistory lastTxHash;
	uint8_t currentClientNo;
	uint32_t chunkNo;
	uint32_t noOfChunksNeeded;
	friend class ElectrumRequests;
	friend class JsonListener;
	friend class BitcoinTransaction;
};

extern KoynClass Koyn;
#endif
