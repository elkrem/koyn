#ifndef Koyn_h
#define Koyn_h

#include <SPI.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include "SdFat/SdFat.h"
#include "uECC/uECC.h"
#include "Base58/Base58.h"
#include "Ripemd160/Ripemd160.h"
#include "Config.h"
#include "Literals.h"
#include "ElectrumRequests.h"
#include "Hash.h"
#include "Utils.h"
#include "JsonStreamingParser/JsonStreamingParser.h"
#include "JsonStreamingParser/JsonListener.h"
#include "AddressHistory.h"
#include "BitcoinHeader.h"
#include "BitcoinAddress.h"
#include "BitcoinTransaction.h"
#include "BitcoinFork.h"

static const uint8_t emptyArray[64]={};

class KoynClass {
public:
	KoynClass();
	void begin();
	void run();
	void initialize();
	uint8_t trackAddress(BitcoinAddress * );
	void unTrackAddress(BitcoinAddress * );
	void unTrackAllAddresses();
	bool isAddressTracked(BitcoinAddress * );
	bool isSynced();
	void onNewTransaction(void (*)(BitcoinTransaction));
	void onNewBlockHeader(void (*)(uint32_t));
	void onError(void (*)(uint8_t));
	void delay(unsigned long);
	uint8_t spend(BitcoinAddress *, BitcoinAddress *, uint64_t , uint64_t , BitcoinAddress *); 
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
	uint8_t verifyBlockHeaders(BitcoinHeader*);
	uint8_t catchingUpFork(BitcoinHeader*);
	int8_t getAddressPointerIndex(ElectrumRequestData * );
	void syncWithServers();
	void connectToServers();
	void reconnectToServers();
	void processInput(String ,String );
	bool parseReceivedChunk();
	void checkSDCardMounted();
	void checkDirAvailability();
	void parseReceivedTx();
	void getHeaderFromMainChain(BitcoinHeader *,uint32_t);
	void reorganizeMainChain();
	void removeUnconfirmedTransactions();
	bool checkBlckNumAndValidate(uint32_t);
	void (*transactionCallback)(BitcoinTransaction);
	void (*newBlockCallback)(uint32_t);
	uint32_t lastTimeTaken;
	uint32_t clientTimeout;
	uint32_t totalBlockNumb;
	uint32_t fallingBackBlockHeight;
	uint8_t saveResToFile;
	uint8_t isMessage;
	uint8_t merkleRoot[32];
	bool synchronized;
	bool firstLevel;
	bool requestsSent;
	bool bigFile;
	bool isTransactionCallbackAssigned;
	bool isNewBlockCallbackAssigned;
	bool saveNextHistory;
	bool reparseFile;
	bool confirmedFlag;
	bool isInit;
	bool clientTimeoutTaken;
	BitcoinHeader header;
	BitcoinHeader lastHeader;
	BitcoinHeader prevHeader;
	BitcoinAddress * userAddressPointerArray[MAX_TRACKED_ADDRESSES_COUNT];
	BitcoinAddress * addressPointer;
	AddressHistory tempAddressHistory;
	uint8_t currentClientNo;
	uint32_t chunkNo;
	uint32_t noOfChunksNeeded;
	friend class ElectrumRequests;
	friend class JsonListener;
	friend class BitcoinTransaction;
};

extern KoynClass Koyn;
#endif
