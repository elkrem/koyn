/********* Koyn Library Definitions *********/

/* BitcoinAddress class definitions */
#define P2PKH_ADDRESS	0x00
#define P2SH_ADDRESS	0x01

#define ADDRESS_NOT_TRACKED 			0x00
#define ADDRESS_NO_FUNDS				0x01
#define ADDRESS_INSUFFICIENT_BALANCE 	0x02
#define ADDRESS_NO_PRIVATE_KEY			0x03
#define ADDRESS_INVALID					0x04
#define INVALID_AMOUNT					0x05

#define MAIN_NET_VERSION 0x00
#define TEST_NET_VERSION 0x6F

#define ADDRESS_ENCODED			0x01
#define KEY_WIF  				0x02
#define KEY_PRIVATE	 			0x03
#define KEY_COMPRESSED_PUBLIC	0x04
#define KEY_PUBLIC 				0x05
#define KEY_SCRIPT_HASH			0x06

#define TRACKING_ADDRESS_ERROR		   0x00
#define MAX_ADDRESSES_TRACKED_REACHED  0x01
#define TRACKING_ADDRESS			   0x02
#define MAIN_NET_NOT_SUPPORTED		   0x03

/* Koyn class definitions */
#define MAIN_CLIENT	0
#define GENESIS_BLOCK_HASH_TESTNET	"000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943"
#define VERSION_STRING    "1.2"
#define CLIENT_NAME       "KOYN"
#define DEFAULT_MAIN_NET_PORT	50001
#define MAX_TIMEOUT_FOR_CLIENT_CONNECTION 60*1000*7
#define PINGING_PERIOD  60*1000*5

#define BLOCK_NUM_SUB					0
#define BLOCK_HEAD_SUB					1
#define ADDRESS_SUB						2

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

#define LIBRARY_NOT_INITIALIZED 0xFF
#define MISSING_FILE  0xFE
#define ERROR_OPEN_FILE 0xFD
#define ERROR_NO_DATA_IN_FILE 0xFC
/* BitcoinTransaction class definitions */
#define ASN1_BMPSTRING		0x30
#define ASN1_INTEGER		0x02
#define SIZE_OF_RIPE_HASH 	0x14

#define OP_0  				0x00
#define OP_2  				0x52
#define OP_PUSHDATA1 		0x4C
#define OP_DUP				0x76
#define OP_HASH160 			0xA9
#define OP_EQUALVERIFY		0x88
#define OP_CHECKSIG			0xAC
#define OP_EQUAL 			0x87


#define TRANSACTION_PASSED 				0x00
#define ADDRESS_RETRIEVED				0x01
#define INDEX_ERROR						0x02
#define ADDRESS_TYPE_ERROR				0x03
#define TRANSACTION_BUILD_ERROR			0x04

/* BitcoinFork class definitions */
#define LONGEST_CHAIN_AT_FORK 6

/* BitcoinHeader class definitions */
#define HEADER_ERROR	0x00
#define HEADER_VALID	0x01
#define SAME_HEADER		0x02
#define FORKED			0x03
#define FORK_VALID		0x04
#define INVALID			0x05
