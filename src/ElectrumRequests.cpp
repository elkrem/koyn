#include "Koyn.h"

uint32_t ElectrumRequestData::reqId=0;

ElectrumRequestData::ElectrumRequestData()
{
	localReqId=reqId++;
	reqType=0;
	dataInt = -1;
	isUsed = false;
}

void ElectrumRequestData::setReqType(uint32_t bitShift)
{
	this->reqType |= (0x01<<bitShift);
}

void ElectrumRequestData::setUsed()
{
	this->isUsed = true;
}

void ElectrumRequestData::resetUsed()
{
	this->isUsed = false;
}

bool ElectrumRequestData::isReqUsed()
{
	return this->isUsed;
}

uint32_t ElectrumRequestData::getReqId()
{
	return this->localReqId;
}

uint32_t ElectrumRequestData::getReqType()
{
	return this->reqType;
}

uint8_t * ElectrumRequestData::getDataString()
{
	return this->dataString;
}

uint32_t ElectrumRequestData::getDataInt()
{
	return this->dataInt;
}

/************************************************** ElectrumRequestsClassClass *******************************************************/
ElectrumRequests::ElectrumRequests()
{
}


void ElectrumRequests::sendVersion()
{
	for(int i = 0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(Koyn.getClient(i)->connected())
		{
			ElectrumRequestData  * currentReq = ElectrumRequests::getElectrumRequestData();
			if(currentReq)
			{
				Koyn.getClient(i)->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"server.version",VERSION_STRING))+"\n");
				currentReq->setReqType(VERSION_BIT);
				currentReq->setUsed();
				aJson.deleteItem(jsonMessage.versionMessage);
				free(jsonMessage.json);
			}
		}
	}
}

void ElectrumRequests::subscribeToPeers()
{
	for(int i = 0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(Koyn.getClient(i)->connected())
		{
			int32_t param= -1;
			ElectrumRequestData  * currentReq = ElectrumRequests::getElectrumRequestData();
			if(currentReq)
			{
				Koyn.getClient(i)->print(String(jsonMessage.createJsonMessage(currentReq->getReqId(),"server.peers.subscribe",&param))+"\n");
				currentReq->setReqType(PEERS_SUBS_BIT);
				currentReq->setUsed();
				aJson.deleteItem(jsonMessage.versionMessage);
				free(jsonMessage.json);
			}
		}
	}
}

void ElectrumRequests::subscribeToBlocksNumber()
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		int32_t param= -1;
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessage(currentReq->getReqId(),"blockchain.numblocks.subscribe",&param))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(BLOCKS_NO_SUBS_BIT);
			currentReq->setUsed();
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}


void ElectrumRequests::subscribeToBlockHeaders()
{
	for(int i = 0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(Koyn.getClient(i)->connected())
		{
			int32_t param = -1;
			ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
			if(currentReq)
			{
				Koyn.getClient(i)->print(String(jsonMessage.createJsonMessage(currentReq->getReqId(),"blockchain.headers.subscribe",&param))+"\n"); /*Make sure to call the right message from jsonMessage create*/
				currentReq->setReqType(HEADERS_SUBS_BIT);
				currentReq->setUsed();
				aJson.deleteItem(jsonMessage.versionMessage);
				free(jsonMessage.json);
			}
		}
	}
}

void ElectrumRequests::subscribeToAddress(const char  * address)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"blockchain.address.subscribe",address))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(ADDRESS_SUBS_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));/* memcpy here takes an already declared container with size while current.dataString is just a pointer so this will crash, make sure to pass an array to memcpy*/
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}

void ElectrumRequests::getAddressHistory(const char  * address)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"blockchain.address.get_history",address))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(ADRRESS_HISTORY_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));/* memcpy here takes an already declared container with size while current.dataString is just a pointer so this will crash, make sure to pass an array to memcpy*/
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}

void ElectrumRequests::getAddressBalance(const char  * address)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"blockchain.address.get_balance",address))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(ADDRESS_BALANCE_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));/* memcpy here takes an already declared container with size while current.dataString is just a pointer so this will crash, make sure to pass an array to memcpy*/
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}

void ElectrumRequests::getMempool(const char  * address)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"blockchain.address.get_mempool",address))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(ADDRESS_MEMPOOL_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));/* memcpy here takes an already declared container with size while current.dataString is just a pointer so this will crash, make sure to pass an array to memcpy*/
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}

void ElectrumRequests::getMerkleProof(const char  * address,int height)
{
	/* We should get Merkle proof from another servers and not from the main client,if one of them failed we should shif to the other and
	   get the same merkle proof*/
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessage(currentReq->getReqId(),"blockchain.transaction.get_merkle",address,&height))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(MERKLE_PROOF);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));/* memcpy here takes an already declared container with size while current.dataString is just a pointer so this will crash, make sure to pass an array to memcpy*/
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}

void ElectrumRequests::getBlockHeader(int blockHeight)
{
	for(int i = 0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(Koyn.getClient(i)->connected())
		{
			ElectrumRequestData  * currentReq = ElectrumRequests::getElectrumRequestData();
			if(currentReq)
			{
				Koyn.getClient(i)->print(String(jsonMessage.createJsonMessage(currentReq->getReqId(),"blockchain.block.get_header",&blockHeight))+"\n"); /*Make sure to call the right message from jsonMessage create*/
				currentReq->setReqType(BLOCK_HEADER_BIT);
				currentReq->setUsed();
				currentReq->dataInt = blockHeight;
				aJson.deleteItem(jsonMessage.versionMessage);
				free(jsonMessage.json);
			}
		}
	}
}

void ElectrumRequests::listUtxo(const char  * address)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"blockchain.address.listunspent",address))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(ADDRESS_UTXO_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));/* memcpy here takes an already declared container with size while current.dataString is just a pointer so this will crash, make sure to pass an array to memcpy*/
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}


void ElectrumRequests::getBlockChunks(int clientNo)
{
	if(Koyn.getClient(clientNo)->connected()&&Koyn.noOfChunksNeeded)
	{
		ElectrumRequestData  * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getClient(clientNo)->print(String(jsonMessage.createJsonMessage(currentReq->getReqId(),"blockchain.block.get_chunk",(int32_t*)&Koyn.chunkNo))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(BLOCK_CHUNKS_BIT);
			currentReq->setUsed();
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}


void ElectrumRequests::relayFee()
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		int param= -1;
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessage(currentReq->getReqId(),"blockchain.estimatefee",&param))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(RELAY_FEE_BIT);
			currentReq->setUsed();
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}


void ElectrumRequests::broadcastTransaction(File * transactionFile)
{
	/* We may send them to all the nodes to save time including the tx into a block*/
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			// Koyn.getMainClient()->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"blockchain.transaction.broadcast",(const char*)rawTransaction))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			Koyn.getMainClient()->print("{\"jsonrpc\":\"2.0\",");
			Koyn.getMainClient()->print(String("\"id\":")+String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.address.subscribe\",\"params\":[\"");
			while(transactionFile->available()){Koyn.getMainClient()->print(transactionFile->read());}
			Koyn.getMainClient()->print("\"]}\n");
			currentReq->setReqType(BROADCAST_TRANSACTION_BIT);
			currentReq->setUsed();
		}
	}
}

void ElectrumRequests::getTransaction(const char * txHash)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String(jsonMessage.createJsonMessageString(currentReq->getReqId(),"blockchain.transaction.get",txHash))+"\n"); /*Make sure to call the right message from jsonMessage create*/
			currentReq->setReqType(TRANSACTION_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));/* memcpy here takes an already declared container with size while current.dataString is just a pointer so this will crash, make sure to pass an array to memcpy*/
			aJson.deleteItem(jsonMessage.versionMessage);
			free(jsonMessage.json);
		}
	}
}


ElectrumRequestData * ElectrumRequests::getElectrumRequestData()
{
	uint16_t i=0;
	while(i<MAX_PARALLEL_REQUESTS)
	{
		if(!electrumRequestDataArray[i].isReqUsed())
		{
			return &electrumRequestDataArray[i];
		}
		i++;
	}
	return NULL;
}

ElectrumRequestData * ElectrumRequests::getElectrumRequestData(int reqNo)
{
	uint16_t i=0;
	while(i<MAX_PARALLEL_REQUESTS)
	{
		if(electrumRequestDataArray[i].getReqId()== reqNo)
		{
			return &electrumRequestDataArray[i];
		}
		i++;
	}
	return NULL;
}
