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
	reqType |= (0x01<<bitShift);
}

void ElectrumRequestData::setUsed()
{
	isUsed = true;
}

void ElectrumRequestData::resetUsed()
{
	isUsed = false;
}

bool ElectrumRequestData::isReqUsed()
{
	return isUsed;
}

uint32_t ElectrumRequestData::getReqId()
{
	return localReqId;
}

uint32_t ElectrumRequestData::getReqType()
{
	return reqType;
}

uint8_t * ElectrumRequestData::getDataString()
{
	return dataString;
}

uint32_t ElectrumRequestData::getDataInt()
{
	return dataInt;
}

/************************************************** ElectrumRequestsClassClass *******************************************************/
ElectrumRequests::ElectrumRequests()
{}

void ElectrumRequests::ping()
{
	for(int i = 0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(Koyn.getClient(i)->connected())
		{
			Koyn.getClient(i)->print(String("{\"id\":") + String("255"));
			Koyn.getClient(i)->print(",\"method\":\"server.ping");
			Koyn.getClient(i)->print("\"}\n");
		}
	}
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
				Koyn.getClient(i)->print(String("{\"id\":") + String(currentReq->getReqId()));
				Koyn.getClient(i)->print(",\"method\":\"server.version\",\"params\":[\"");
				Koyn.getClient(i)->print(CLIENT_NAME);
				Koyn.getClient(i)->print("\",\"");
				Koyn.getClient(i)->print(VERSION_STRING);
				Koyn.getClient(i)->print("\"]}\n");
				currentReq->setReqType(VERSION_BIT);
				currentReq->setUsed();
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
			ElectrumRequestData  * currentReq = ElectrumRequests::getElectrumRequestData();
			if(currentReq)
			{
				Koyn.getClient(i)->print(String("{\"id\":") + String(currentReq->getReqId()));
				Koyn.getClient(i)->print(",\"method\":\"server.peers.subscribe");
				Koyn.getClient(i)->print("\"}\n");
				currentReq->setReqType(PEERS_SUBS_BIT);
				currentReq->setUsed();
			}
		}
	}
}

void ElectrumRequests::subscribeToBlockHeaders()
{
	for(int i = 0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(Koyn.getClient(i)->connected())
		{
			ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
			if(currentReq)
			{
				Koyn.getClient(i)->print(String("{\"id\":") + String(currentReq->getReqId()));
				Koyn.getClient(i)->print(",\"method\":\"blockchain.headers.subscribe");
				Koyn.getClient(i)->print("\"}\n");
				currentReq->setReqType(HEADERS_SUBS_BIT);
				currentReq->setUsed();
			}
		}
	}
}

void ElectrumRequests::subscribeToAddress(const char  * addressScriptHash)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.scripthash.subscribe\",\"params\":[\"");
			Koyn.getMainClient()->print(addressScriptHash);
			Koyn.getMainClient()->print("\"]}\n");
			currentReq->setReqType(ADDRESS_SUBS_BIT);
			currentReq->setUsed();
			memcpy(currentReq->dataString,(uint8_t *)addressScriptHash,64);
		}
	}
}

void ElectrumRequests::getAddressHistory(const char  * addressScriptHash)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.scripthash.get_history\",\"params\":[\"");
			Koyn.getMainClient()->print(addressScriptHash);
			Koyn.getMainClient()->print("\"]}\n");
			currentReq->setReqType(ADRRESS_HISTORY_BIT);
			currentReq->setUsed();
			memcpy(currentReq->dataString,(uint8_t *)addressScriptHash,64);
		}
	}
}

void ElectrumRequests::getAddressBalance(const char  * addressScriptHash)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.scripthash.get_balance\",\"params\":[\"");
			Koyn.getMainClient()->print(addressScriptHash);
			Koyn.getMainClient()->print("\"]}\n");
			currentReq->setReqType(ADDRESS_BALANCE_BIT);
			currentReq->setUsed();
			memcpy(currentReq->dataString,(uint8_t *)addressScriptHash,64);
		}
	}
}

void ElectrumRequests::getMempool(const char  * addressScriptHash)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.scripthash.get_mempool\",\"params\":[\"");
			Koyn.getMainClient()->print(addressScriptHash);
			Koyn.getMainClient()->print("\"]}\n");
			currentReq->setReqType(ADDRESS_MEMPOOL_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,addressScriptHash,32);
		}
	}
}

void ElectrumRequests::getMerkleProof(const char * addressScriptHash,const char  * txHash,int height)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.transaction.get_merkle\",\"params\":[\"");
			Koyn.getMainClient()->print(txHash);
			Koyn.getMainClient()->print("\",");
			Koyn.getMainClient()->print(height);
			Koyn.getMainClient()->print("]}\n");
			currentReq->setReqType(MERKLE_PROOF);
			currentReq->setUsed();
			memcpy(currentReq->dataString,(uint8_t *)addressScriptHash,64);
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
				Koyn.getClient(i)->print(String("{\"id\":") + String(currentReq->getReqId()));
				Koyn.getClient(i)->print(",\"method\":\"blockchain.block.get_header\",\"params\":[");
				Koyn.getClient(i)->print(blockHeight);
				Koyn.getClient(i)->print("]}\n");
				currentReq->setReqType(BLOCK_HEADER_BIT);
				currentReq->setUsed();
				currentReq->dataInt = blockHeight;
			}
		}
	}
}

void ElectrumRequests::listUtxo(const char  * addressScriptHash)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.scripthash.listunspent\",\"params\":[\"");
			Koyn.getMainClient()->print(addressScriptHash);
			Koyn.getMainClient()->print("\"]}\n");
			currentReq->setReqType(ADDRESS_UTXO_BIT);
			currentReq->setUsed();
			memcpy(currentReq->dataString,(uint8_t *)addressScriptHash,64);
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
			Koyn.getClient(clientNo)->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getClient(clientNo)->print(",\"method\":\"blockchain.block.get_chunk\",\"params\":[");
			Koyn.getClient(clientNo)->print(Koyn.chunkNo);
			Koyn.getClient(clientNo)->print("]}\n");
			currentReq->setReqType(BLOCK_CHUNKS_BIT);
			currentReq->setUsed();
		}
	}
}


void ElectrumRequests::relayFee()
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.relayfee");
			Koyn.getMainClient()->print("\"}\n");
			currentReq->setReqType(RELAY_FEE_BIT);
			currentReq->setUsed();
		}
	}
}


void ElectrumRequests::broadcastTransaction(File * transactionFile)
{
	if(Koyn.getMainClient()&&Koyn.getMainClient()->connected())
	{
		ElectrumRequestData * currentReq = ElectrumRequests::getElectrumRequestData();
		if(currentReq)
		{
			Koyn.getMainClient()->print(String("{\"id\":")+String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.transaction.broadcast\",\"params\":[\"");
			while(transactionFile->available()){Koyn.getMainClient()->write(transactionFile->read());}
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
			Koyn.getMainClient()->print(String("{\"id\":") + String(currentReq->getReqId()));
			Koyn.getMainClient()->print(",\"method\":\"blockchain.transaction.get\",\"params\":[\"");
			Koyn.getMainClient()->print(txHash);
			Koyn.getMainClient()->print("\"]}\n");
			currentReq->setReqType(TRANSACTION_BIT);
			currentReq->setUsed();
			// memcpy(currentReq->dataString,address,strlen(address));
		}
	}
}


ElectrumRequestData * ElectrumRequests::getElectrumRequestData()
{
	int i=0;
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

ElectrumRequestData * ElectrumRequests::getElectrumRequestData(unsigned int reqNo)
{
	int i=0;
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

void ElectrumRequests::resetRequests()
{
	for(int i=0;i<MAX_PARALLEL_REQUESTS;i++)
	{
		electrumRequestDataArray[i].resetUsed();
	}
}
