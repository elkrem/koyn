#include "Koyn.h"
#include "Servers.h"



KoynClass::KoynClass()
{
	initialize();
}

void KoynClass::initialize()
{
	totalBlockNumb = 0;
	chunkNo =0;
	saveResToFile = 0;
	isMessage = 0;
	mainClient = NULL;
	reqData = NULL;
	bigFile = false;
	synchronized = false;
	saveNextHistory = false;
	reparseFile=false;
	confirmedFlag=false;
	isInit=false;
	clientTimeoutTaken=false;
	for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++){userAddressPointerArray[i]=NULL;}
	request.resetRequests();
}

void KoynClass::begin()
{
	#ifdef	USE_MAIN_NET
	#error Main net currently not supported
	#endif
	#if	(defined(USE_MAIN_NET)&&defined(USE_TEST_NET))
	#error Error, both main net and test net configured
	#endif
	if(!isInit)
	{
		checkSDCardMounted();
		checkDirAvailability();
		connectToServers();
		isInit=true;
	}
}

void KoynClass::checkSDCardMounted()
{
	do{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Mount SD card"));
		#endif
	}while(!SD.begin(SD_CARD_CHIP_SELECT,SD_SCK_MHZ(10)));
}

void KoynClass::checkDirAvailability()
{
	if (!SD.chdir()) {
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("chdir to root failed.\n"));
		#endif
	}
	if(SD.exists("koyn/responses"))
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Removing Response folder"));
		#endif
		String fileName = "/koyn/responses";
		FatFile file = SD.open(fileName);
		if(file.isDir())
		{
			file.rmRfStar();
		}else if(file.isFile())
		{
			Serial.println(F("Response folder was corrupted!"));
			SD.remove(&fileName[0]);
		}
	}
	::delay(200);
	SD.mkdir("koyn/responses");

	if(SD.exists("koyn/blkhdrs"))
	{
		File blkHeaderFile =  SD.open("koyn/blkhdrs",FILE_READ);

		if(!blkHeaderFile){
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Cannot open file"));
		#endif
		return;
		}

		if(blkHeaderFile.available())
		{
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(F("Verification Skiped.."));
			#endif
			totalBlockNumb = ((blkHeaderFile.size()) / 80)-1;
			chunkNo= totalBlockNumb/2016;
			blkHeaderFile.seek(blkHeaderFile.size() - 80);
			int i=0;
			uint8_t lastHeaderFromFile[80];
			while (blkHeaderFile.available())
			{
				lastHeaderFromFile[i] = blkHeaderFile.read();
				i++;
			}
			lastHeader.setHeader(lastHeaderFromFile,totalBlockNumb);
			#if defined(ENABLE_DEBUG_MESSAGES)
			lastHeader.printHeader();
			Serial.println(totalBlockNumb);
			Serial.println(chunkNo);
			#endif
		}
	}else
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Cannot find blkHdr file"));
		#endif
	}
}

void KoynClass::syncWithServers()
{
	request.sendVersion();
	request.subscribeToBlockHeaders();
	// request.subscribeToPeers();
}

uint8_t KoynClass::verifyBlockHeaders(BitcoinHeader * currhdr)
{
	reorganizeMainChain();
	if(currhdr && currhdr->isHeaderValid())
	{
		int32_t headerHeight = currhdr->getHeight();
		if(headerHeight==0)
		{
			uint8_t genisisByte[32];
			uint8_t hash[32];
			hex2bin(genisisByte,GENESIS_BLOCK_HASH_TESTNET, 64);
			reverseBin(genisisByte,32);
			currhdr->getHash(hash);
			if(memcmp(hash,genisisByte,32))
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Genisis Block not verified"));
				#endif
				return INVALID;
			}else {
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Genisis Block verified"));
				#endif
				lastHeader=*currhdr;
				return HEADER_VALID;
			}
		}else{

			/* Checking Pow */
			uint8_t target[32]= {};
			uint8_t currentHeaderHash[32];
			uint8_t currentTargetBits[4]={};
			currhdr->getBits(currentTargetBits);
			reverseBin(currentTargetBits,4);
			memcpy(&target[32-currentTargetBits[0]],&currentTargetBits[1],3);
			currhdr->getHash(currentHeaderHash);
			reverseBin(currentHeaderHash,32);
			uint8_t c=0;
			while(target[c]==currentHeaderHash[c])
			{
				c++;
			}
			if(target[c]>currentHeaderHash[c])
			{

				if(headerHeight>lastHeader.height)
				{
				    uint8_t hash1[32];
				    uint8_t hash2[32];
				    lastHeader.getHash(hash1);
						currhdr->getPrevHash(hash2);
						if(!memcmp(hash1,hash2,32))
						{
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.print(headerHeight);
							Serial.println(F("Valid "));
							#endif
							File blkHdrFile = SD.open("koyn/blkhdrs",FILE_WRITE);
							if(blkHdrFile)
							{
								blkHdrFile.write(currhdr->completeHeader,80);
								blkHdrFile.close();
							}else
							{
								return ERROR_OPEN_FILE;
							}
							lastHeader = *currhdr;
							if(isNewBlockCallbackAssigned)
							{
								(*newBlockCallback)(headerHeight);
							}
							return HEADER_VALID;
						}else
						{
							return catchingUpFork(currhdr);
						}
				}else if(headerHeight == lastHeader.height)
				{
					if(lastHeader == *currhdr)
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						lastHeader.printHeader();
						Serial.println(lastHeader.height);
						Serial.println(F("Same header"));
						#endif
						return SAME_HEADER;
					}else
					{
						return catchingUpFork(currhdr);
					}
				}else
				{
					bool fork;
					for(int i=0;i<MAX_CONNECTED_SERVERS;i++){if(forks[i].exists()){fork=true;break;}}
					if(fork && (prevHeader.getHeight()<0 || prevHeader!=*currhdr))
					{
						BitcoinHeader oldHeader;
						getHeaderFromMainChain(&oldHeader,headerHeight);
						if(oldHeader!=*currhdr)
						{
							request.getBlockHeader(headerHeight-1);
							prevHeader=*currhdr;
							return catchingUpFork(currhdr);
						}else
						{
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.println(F("PH"));
							#endif
							currhdr->parentHeader=true;
							switch(catchingUpFork(currhdr))
							{
								case FORK_VALID:
								#if defined(ENABLE_DEBUG_MESSAGES)
								Serial.println(F("Got parent header "));
								Serial.println(headerHeight);
								#endif
								prevHeader=*currhdr;
								return FORK_VALID;
							}
							return HEADER_ERROR;
						}
					}else
					{
						return HEADER_ERROR;
					}
				}
			}else
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.print(F("New Header is not Valid "));
				Serial.println(currhdr->getHeight());
				#endif
				return INVALID;
			}
		}
	}else
	{
		return INVALID;
	}
}

uint8_t KoynClass::catchingUpFork(BitcoinHeader *currhdr)
{
	#if defined(ENABLE_DEBUG_MESSAGES)
	Serial.println("Catching Fork ");
	#endif
	int32_t headerHeight = currhdr->getHeight();
	if(!SD.exists("koyn/forks"))
	{
		SD.mkdir("koyn/forks");
	}else
	{
		for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
		{
			if(forks[i].exists())
			{
				String fileName = String("koyn/forks/")+"fork"+i;
				if(headerHeight > forks[i].getLastHeader()->height)
				{
					uint8_t hash1[32];
					uint8_t hash2[32];
					forks[i].getLastHeader()->getHash(hash1);
					currhdr->getPrevHash(hash2);
					if(!memcmp(hash1,hash2,32))
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.print(headerHeight);
						Serial.println(F("Fork Valid "));
						#endif
						File forkFile = SD.open(fileName,FILE_WRITE);
						if(forkFile)
						{
							forkFile.write(currhdr->completeHeader,80);
							uint32_t currHdrHeight = currhdr->getHeight();
							forkFile.write((uint8_t *)&currHdrHeight,4);
							forkFile.close();
							forks[i].setLastHeader(currhdr);
							if(isNewBlockCallbackAssigned)
							{
								(*newBlockCallback)(headerHeight);
							}
							return FORK_VALID;
						}else{
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.println(F("Failed to open 1"));
							#endif
							return ERROR_OPEN_FILE;
						}
					}
				}else if(headerHeight == forks[i].getLastHeader()->height)
				{
					if((*forks[i].getLastHeader()) == *currhdr)
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("Same fork header"));
						#endif
						return SAME_HEADER;
					}else
					{
						/* Here we should create another fork, as the same last height wasn't equal the two branches*/
						return HEADER_ERROR;
					}
				}else if(headerHeight < forks[i].getLastHeader()->height)
				{
					File forkFile = SD.open(fileName,FILE_READ);
					if(forkFile)
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("Opened and header smaller"));
						#endif
						if(forkFile.available())
						{
							BitcoinHeader tempHeader;
							uint8_t tempHeaderFromFile[84];
							for(int j=0;j<84;j++){tempHeaderFromFile[j]=forkFile.read();}
							uint32_t tempHeight = *((uint32_t *)(tempHeaderFromFile+80));
							tempHeader.setHeader(tempHeaderFromFile,tempHeight);
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.println(tempHeader.getHeight());
							#endif
							if(tempHeader==*currhdr)
							{
								#if defined(ENABLE_DEBUG_MESSAGES)
								Serial.println(F("Same old fork header"));
								#endif
								return SAME_HEADER;
							}else if(tempHeader.height>headerHeight)
							{
								uint8_t hash1[32];
								uint8_t hash2[32];
								currhdr->getHash(hash1);
								tempHeader.getPrevHash(hash2);
								if(!memcmp(hash1,hash2,32))
								{
									File tempFile = SD.open("koyn/temp",FILE_WRITE);
									if(tempFile)
									{
										#if defined(ENABLE_DEBUG_MESSAGES)
										Serial.print(headerHeight);
										Serial.println(F("Previous header at fork"));
										#endif
										tempFile.write(currhdr->completeHeader,80);
										/* Write the heigth to file */
										uint32_t currHdrHeight = currhdr->getHeight();
										tempFile.write((uint8_t *)&currHdrHeight,4);
										#if defined(ENABLE_DEBUG_MESSAGES)
										Serial.println(F("Copying data"));
										#endif
										forkFile.seek(0);
										while(forkFile.available()){
											uint8_t data =forkFile.read();
											#if defined(ENABLE_DEBUG_MESSAGES)
											Serial.write(data);
											#endif
											tempFile.write(data);
										}
										forkFile.close();
										SD.remove(&fileName[0]);
										tempFile.rename(SD.vwd(),&fileName[0]);
										tempFile.close();
										if(currhdr->parentHeader)
										{
											forks[i].setParentHeader(currhdr);
										}else
										{
											request.getBlockHeader(headerHeight-1);
										}
										return FORK_VALID;
									}else
									{
										return ERROR_OPEN_FILE;
									}
								}
							}else if(tempHeader.height<headerHeight)
							{
								/* Not error but this is a case where the fork is updated while still the previous headers
								   didn't reach
							    */
								return HEADER_ERROR;
							}
						}else
						{
							return ERROR_NO_DATA_IN_FILE;
						}
					}else{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("Failed to open 2"));
						#endif
						return ERROR_OPEN_FILE;
					}
				}
			}else
			{
				return HEADER_ERROR;
			}
		}
	}

	String fileName = String("koyn/forks/")+"fork"+currentClientNo;
	File forkFile = SD.open(fileName,FILE_WRITE);
	if(forkFile)
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Fork created"));
		#endif
		forkFile.write(currhdr->completeHeader,80);
		uint32_t currHdrHeight = currhdr->getHeight();
		forkFile.write((uint8_t *)&currHdrHeight,4);
		forkFile.close();
	}else{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Failed to open 0"));
		#endif
		return ERROR_OPEN_FILE;
	}
	forks[currentClientNo].setLastHeader(currhdr);
	request.getBlockHeader(headerHeight-1);
	return FORKED;
}

void KoynClass::reorganizeMainChain()
{
	BitcoinHeader tempHeader=lastHeader;
	for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(forks[i].exists()&&forks[i].gotParentHeader())
		{
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.print(F("Checking difficulty with forks "));
			Serial.println(i);
			#endif
			BitcoinHeader t1 = *forks[i].getLastHeader();
			uint32_t forkDifficulty = 0;
			memcpy(&forkDifficulty, t1.completeHeader+72, sizeof(forkDifficulty));
			uint32_t tempDifficulty = 0;
			memcpy(&tempDifficulty, tempHeader.completeHeader+72, sizeof(tempDifficulty));
			if(forkDifficulty<tempDifficulty)
			{
				tempHeader=*forks[i].getLastHeader();
			}
		}
	}
	if(tempHeader!=lastHeader)
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Chain with bigger difficulty"));
		#endif
		for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
		{
			if(forks[i].exists()&&forks[i].gotParentHeader())
			{
				String fileName = String("koyn/forks/")+"fork"+i;
				if(tempHeader==*forks[i].getLastHeader())
				{
					File forkFile = SD.open(fileName,FILE_READ);
					File blkHeaderFile = SD.open("koyn/blkhdrs",FILE_WRITE);
					if(forkFile&&blkHeaderFile)
					{
						uint32_t seekValue=forks[i].getParentHeader()->getHeight();
						blkHeaderFile.seek(seekValue*80);
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.print(F("Seek "));
						#endif
						while(forkFile.available())
						{
							uint8_t tempHeaderFromFile[84];
							for(int j=0;j<84;j++){tempHeaderFromFile[j]=forkFile.read();}
							blkHeaderFile.write(tempHeaderFromFile,80);
						}
						forkFile.close();
						blkHeaderFile.close();
						updateTotalBlockNumb();
						lastHeader = *forks[i].getLastHeader();
						for(int j=0;j<MAX_CONNECTED_SERVERS;j++){String fileName = String("koyn/forks/")+"fork"+j;SD.remove(&fileName[0]);forks[j].setNull();}
						return;
					}
				}
			}
		}
	}else
	{
		tempHeader.setNull();
		for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
		{
			if(forks[i].exists()&&forks[i].gotParentHeader())
			{
				String fileName = String("koyn/forks/")+"fork"+i;
				if(tempHeader.getHeight()<forks[i].getParentHeader()->getHeight())
				{
					tempHeader = *forks[i].getParentHeader();
				}
				uint32_t diff=0;
				diff = forks[i].getLastHeader()->getHeight()-forks[i].getParentHeader()->getHeight();
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Getting longest chain"));
				#endif
				if(diff>=LONGEST_CHAIN_AT_FORK)
				{
					#if defined(ENABLE_DEBUG_MESSAGES)
					Serial.println(F("Chain is long"));
					#endif
					File forkFile = SD.open(fileName,FILE_READ);
					File blkHeaderFile = SD.open("koyn/blkhdrs",FILE_WRITE);
					if(forkFile&&blkHeaderFile)
					{
						uint32_t seekValue=forks[i].getParentHeader()->getHeight();
						blkHeaderFile.seek(seekValue*80);
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.print(F("Seek "));
						Serial.println(seekValue);
						#endif
						while(forkFile.available())
						{
							uint8_t tempHeaderFromFile[84];
							for(int j=0;j<84;j++){tempHeaderFromFile[j]=forkFile.read();}
							blkHeaderFile.write(tempHeaderFromFile,80);
						}
						forkFile.close();
						blkHeaderFile.close();
						updateTotalBlockNumb();
						lastHeader = *forks[i].getLastHeader();
						for(int j=0;j<MAX_CONNECTED_SERVERS;j++){String fileName = String("koyn/forks/")+"fork"+j;SD.remove(&fileName[0]);forks[j].setNull();}
						return;
					}

				}else{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.print(F("Chain diff "));
						Serial.println(diff);
						#endif
				}
			}
		}
		if(tempHeader.getHeight()>0&&lastHeader.getHeight()-tempHeader.getHeight()>LONGEST_CHAIN_AT_FORK)
		{
			for(int j=0;j<MAX_CONNECTED_SERVERS;j++){String fileName = String("koyn/forks/")+"fork"+j;SD.remove(&fileName[0]);forks[j].setNull();}
			return;
		}
	}
}

void KoynClass::connectToServers()
{
	uint16_t networkArraySize =  sizeof(testnetServerNames)/sizeof(testnetServerNames[0]);
	uint16_t serverNamesCount =0;
	for(uint16_t i =0;i<MAX_CONNECTED_SERVERS;i++)
	{
		char  servName[strlen_P(testnetServerNames[serverNamesCount])+1];
		while(!clientsArray[i].connected() && !clientsArray[i].connect(strcpy_P(servName,testnetServerNames[serverNamesCount]),testnetPortNumber[serverNamesCount]))
		{
			if(serverNamesCount < networkArraySize-1)
			{
				serverNamesCount++;
			}
			else
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Cannot connect to listed servers"));        
				#endif
				if(i!=0)
				{
					request.resetRequests();
					setMainClient();
					syncWithServers();
				}else
				{
					connectToServers();
				}   
				return;
			}
		}
		serverNamesCount++;
		String dirName = String("koyn/responses/")+"client"+i;
		if(!SD.exists(&dirName[0]))
		{
			SD.mkdir(&dirName[0]);
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(String("Created directory ")+dirName);
			Serial.print(String("Client ")+String(i)+String(" connected to "));
			Serial.println(servName);
			#endif
		}
	}
	setMainClient();
	syncWithServers();
}

void KoynClass::reconnectToServers()
{
	uint8_t disconnectedClientsCount = 0;
	for(int i=0 ;i < MAX_CONNECTED_SERVERS; i++)
	{
		if(!clientsArray[i].connected())
		{
			disconnectedClientsCount++;
		}
	}
	if(disconnectedClientsCount==MAX_CONNECTED_SERVERS)
	{
		for(int i=0 ; i<MAX_TRACKED_ADDRESSES_COUNT;i++)
		{
			if(userAddressPointerArray[i]!=NULL)
			{
				userAddressPointerArray[i]->resetTracked();	
				userAddressPointerArray[i]=NULL;
			}
		}
		synchronized = false;
		mainClient = NULL;
		connectToServers();
	}
	/* timeout function to break the looping for connecting disconnected clients */
	if(millis() - clientTimeout > MAX_TIMEOUT_FOR_CLIENT_CONNECTION)
	{
		clientTimeout = millis();
		connectToServers();
	}
}

void KoynClass::run()
{
	if(isInit)
	{
		if(millis()-lastTimeTaken>PINGING_PERIOD)
		{
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(F("Pinging"));
			#endif
			request.ping();
			lastTimeTaken = millis();
		}
		if(synchronized)
		{
			removeUnconfirmedTransactions();
			for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
			{
				if(userAddressPointerArray[i]!=NULL)
				{
					char addr[36]; 
					userAddressPointerArray[i]->getEncoded(addr);
					String dirName = "koyn/addresses/" + String(&addr[26]);
					String fileNameUnverifiedHistory = dirName+"/"+"his_unveri";
					String fileNameHistory = dirName+"/"+"history";
					String fileNameMerkle = dirName+"/"+"merkle";
					/* Requesting Merkle proofs */
					if(SD.exists(&fileNameUnverifiedHistory[0])&&(userAddressPointerArray[i]->addressHistory.lastMerkleVerified||userAddressPointerArray[i]->addressHistory.isFirstMerkle))
					{
						File unverifiedHistoryFile = SD.open(&fileNameUnverifiedHistory[0],FILE_READ);
						if(unverifiedHistoryFile&&userAddressPointerArray[i]->addressHistory.historyFileLastPos==unverifiedHistoryFile.size())
						{
							if(SD.exists(&fileNameHistory[0]))
							{
								File oldHistoryFile = SD.open(&fileNameHistory[0],FILE_WRITE);
								while(unverifiedHistoryFile.available())
								{
									oldHistoryFile.write(unverifiedHistoryFile.read());
								}
								oldHistoryFile.close();
								SD.remove(&fileNameUnverifiedHistory[0]);
								userAddressPointerArray[i]->addressHistory.historyFileLastPos=0;
							}else
							{
								if (!SD.chdir(&dirName[0])) {
									#if defined(ENABLE_DEBUG_MESSAGES)
									Serial.println(F("chdir failed"));
									#endif
								}
								unverifiedHistoryFile.rename(SD.vwd(), "history");
								userAddressPointerArray[i]->addressHistory.historyFileLastPos=0;
								if (!SD.chdir()) {
									#if defined(ENABLE_DEBUG_MESSAGES)
									Serial.println(F("chdir failed"));
									#endif
								}
							}
						}else if(unverifiedHistoryFile)
						{
							uint8_t container[36];
							unverifiedHistoryFile.seek(userAddressPointerArray[i]->addressHistory.historyFileLastPos);
							if(unverifiedHistoryFile.available())
							{
								for(int i=0;i<36;i++)
								{
									container[i]=unverifiedHistoryFile.read();
								}
							}
							userAddressPointerArray[i]->addressHistory.copyData(container);
							char txHash_str[65];
							txHash_str[64]='\0';
							userAddressPointerArray[i]->addressHistory.getStringTxHash(txHash_str);
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.println(F("Getting Merkle Proofs"));
							#endif
							char addressScriptHash[65]={};
							userAddressPointerArray[i]->getScriptHash(addressScriptHash,64);
							request.getMerkleProof(addressScriptHash,(const char *)txHash_str,userAddressPointerArray[i]->addressHistory.getHeight());
							if(userAddressPointerArray[i]->addressHistory.isFirstMerkle){userAddressPointerArray[i]->addressHistory.isFirstMerkle=false;}
							userAddressPointerArray[i]->addressHistory.historyFileLastPos = unverifiedHistoryFile.curPosition();
							userAddressPointerArray[i]->addressHistory.lastMerkleVerified = false;
						}
					}

					if(SD.exists(&fileNameMerkle[0]))
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("Merkle"));
						#endif
						File merkleFile = SD.open(&fileNameMerkle[0],FILE_READ);
						uint8_t hash[32]={};
						uint8_t arrayToHash[64];
						uint8_t merkleLeaf[32];
						uint32_t leafPos = userAddressPointerArray[i]->addressHistory.getLeafPos();
						uint32_t numberOfLeafs = merkleFile.size()/32;
						userAddressPointerArray[i]->addressHistory.getTxHash(hash);
						reverseBin(hash,32);
						for(unsigned int j = 0 ;j<numberOfLeafs;j++)
						{
							for(int i=0;i<32;i++){merkleLeaf[i]=merkleFile.read();}
							reverseBin(merkleLeaf,32);
							if((leafPos>>j)&0x01)
							{
								memcpy(arrayToHash,merkleLeaf,32);
								memcpy(arrayToHash+32,hash,32);
							}else
							{
								memcpy(arrayToHash+32,merkleLeaf,32);
								memcpy(arrayToHash,hash,32);
							}
							doubleSha256(hash,arrayToHash,64);
						}
						if(!memcmp(merkleRoot,hash,32))
						{
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.println(F("Merkle Verified ..! "));
							#endif
							userAddressPointerArray[i]->addressHistory.lastMerkleVerified = true;
							for(int j=0;j<MAX_TRANSACTION_COUNT;j++)
							{
								if(incomingTx[j].isUsed()&&!incomingTx[j].inBlock())
								{
									uint8_t txHash[32];
									uint8_t hash[32];
									incomingTx[j].getHash(txHash);
									userAddressPointerArray[i]->addressHistory.getTxHash(hash);
									reverseBin(hash,32);
									if(!memcmp(txHash,hash,32))
									{
										incomingTx[j].setHeight(userAddressPointerArray[i]->addressHistory.getHeight());
										if(isTransactionCallbackAssigned)
										{
											(*transactionCallback)(incomingTx[j]);
										}
									}
								}
							}
						}else
						{
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.println(F("Bad History Removing .."));
							#endif
							userAddressPointerArray[i]->addressHistory.lastMerkleVerified = false;
						}
						SD.remove(&fileNameMerkle[0]);
					}
				}
			}
		}
		for(int i=0 ;i<MAX_CONNECTED_SERVERS;i++)
		{
			bool opened = false;
			File responseFile;
			reconnectToServers();
			while(clientsArray[i].available())
			{
				char data =clientsArray[i].read();
				if(data != '\n')
				{
					if(!opened)
					{
						String fileName = "koyn/responses/client"+String(i)+"/"+"uncomplete";
						responseFile = SD.open(&fileName[0],FILE_WRITE);
						opened = true;
						if(responseFile){
						#if defined(ENABLE_DEBUG_MESSAGES)
						/*Serial.println(F("File Opened"));*/
						#endif	
						}else{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("File not opened"));
						#endif
						}
					}
					#if defined(ENABLE_DEBUG_MESSAGES)
					Serial.write(data);
					#endif
					if(responseFile)
					{
						responseFile.write(data);
					}
				}else
				{
					String dirName = "koyn/responses/client"+String(i);
					FatFile directory = SD.open(&dirName[0]);
					if (!responseFile.rename(&directory, &String(millis())[0]))
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("Cannot rename file"));
						#endif
					}
					responseFile.close();
					opened = false;
				}
			}
			responseFile.close();
		}

		for(int i =0 ;i<MAX_CONNECTED_SERVERS;i++)
		{
			char buff[14];
			buff[13]='\0';
			currentClientNo = i;
			String dirName = "koyn/responses/client"+String(i);
			FatFile directory = SD.open(dirName);

			while (file.openNext(&directory,O_READ)) {
				file.getName(buff,13);
				if(!(String(buff)==String("uncomplete")))
				{
					#if defined(ENABLE_DEBUG_MESSAGES)
					Serial.print(F("parsing file "));
					Serial.println(buff);
					#endif
					JsonStreamingParser parser;
					parser.setListener(&listener);
					while(file.available())
					{
						parser.parse(file.read());
						if(bigFile){bigFile=false;break;}
					}
					if(reparseFile)
					{
						file.seek(0);
						JsonStreamingParser parser;
						parser.setListener(&listener);
						while(file.available())
						{
							parser.parse(file.read());
							if(bigFile){bigFile=false;break;}
						}
					}

					if(saveNextHistory){saveNextHistory=false;}
					if(reqData)
					{
						reqData->reqType=0;
						reqData->resetUsed();
						reqData = NULL;
						reparseFile = false;
					}
					file.close();
					if(directory.remove(&directory,&buff[0]))
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.print(F("Removed file "));
						Serial.println(buff);
						#endif
					}else
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.print(F("Failed to remove file "));
						Serial.println(buff);
						#endif
					}
				}else
				{
					file.close();
				}
			}
		}
	}
}

bool KoynClass::parseReceivedChunk()
{
	if(!file){
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Cannot open file"));
		#endif
		return false;
	}
	file.seek(0);
	if(file.available())
	{
		char data[8];
		file.read(data,8);
		while(memcmp(data,"\"result\"",8))
		{
			for(int i=0;i<7;i++){data[i]=data[i+1];}
			data[7]=file.read();
		}
		while(file.read()!='"');
		bigFile = true;
		uint32_t currentHeight = chunkNo*2016;
		if(totalBlockNumb>currentHeight)
		{
			uint16_t hdrPos = lastHeader.getPos();
			file.seekCur((hdrPos*160));
			currentHeight = totalBlockNumb;
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.print(F("Last Header Position "));
			Serial.println(lastHeader.getPos());
			#endif
		}

		while (file.available())
		{
			char data;
			bool endOfObject = false;
			uint8_t currentHeader[80]={};
			char currentHeaderString[161]={};
			currentHeaderString[160]='\0';
			for (uint16_t j = 0; j < 160; j++)
			{
				data = file.read();
				if(data == '"' || data == '}'){endOfObject = true;}else{
					currentHeaderString[j] = data;
				}
			}
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(currentHeaderString);
			#endif
			if(!endOfObject)
			{
				hex2bin(currentHeader,currentHeaderString,160);
				header.setHeader(currentHeader,currentHeight);
				switch(verifyBlockHeaders(&header))
				{
					case HEADER_VALID: break;
					case INVALID:
		                #if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("Not Verified"));
	                    Serial.println(currentHeaderString);
		                Serial.write(header.completeHeader,80);
						#endif
		                return false;
				}
				header.setNull();
				currentHeight++;
			}

			if(millis()-lastTimeTaken>PINGING_PERIOD)
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Pinging"));
				#endif
				request.ping();
				lastTimeTaken = millis();
			}
		}
		reparseFile = false;
		return true;
	}else
	{
		return false;
	}
}

void KoynClass::parseReceivedTx()
{
	if(!file){
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Cannot open file"));
		#endif
	}
	file.seek(0);
	if(file.available())
	{
		char data[8];
		file.read(data,8);
		while(memcmp(data,"\"result\"",8))
		{
			for(int i=0;i<7;i++){data[i]=data[i+1];}
			data[7]=file.read();
		}
		while(file.read()!='"');
		bigFile = true;

		uint32_t pos = file.curPosition();
		uint32_t count=0;
		while(file.read()!='"'){count++;}
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(count);
		#endif
		if(count/2>MAX_TRANSACTION_SIZE){Serial.println(F("Raw transaction is too big"));return;}
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Locating space in Ram"));
		#endif
		char stringRawTx[count];
		file.seek(pos);
		for(unsigned int i=0;i<count;i++){stringRawTx[i]=file.read();}
		for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
		{
			if(!incomingTx[i].isUsed())
			{
				if(incomingTx[i].setRawTx(stringRawTx,count))
				{
					#if defined(ENABLE_DEBUG_MESSAGES)
					Serial.println(F("Success"));
					#endif
					incomingTx[i].setHeight(0);
					if(isTransactionCallbackAssigned)
					{
						(*transactionCallback)(incomingTx[i]);
					}
					if(REMOVE_CONFIRMED_TRANSACTION_AFTER==0)
					{
						incomingTx[i].resetTx();
					}
				}
				break;
			}
		}
	}
	reparseFile = false;
}

void KoynClass::processInput(String key,String value)
{
	if(key == "jsonrpc")
	{}else if(key == "id")
	{
		if(reqData==NULL)
		{
			reqData = request.getElectrumRequestData(value.toInt());
			if(reqData&&reqData->reqType&(uint32_t)(0x01<<BLOCK_CHUNKS_BIT))
			{
				if(parseReceivedChunk())
				{
					noOfChunksNeeded--;
					if(noOfChunksNeeded)
					{
						chunkNo++;
						while(++currentClientNo!=MAX_CONNECTED_SERVERS&&!clientsArray[currentClientNo].connected())
						{}
						if(currentClientNo==MAX_CONNECTED_SERVERS)
						{
							currentClientNo=MAIN_CLIENT;
						}
						request.getBlockChunks(currentClientNo);
						synchronized =false;
					}else
					{
						#if defined(ENABLE_DEBUG_MESSAGES)
						Serial.println(F("Got All Chunks"));
						#endif
						synchronized = true;
					}
					updateTotalBlockNumb();
				}else
				{
					/* This should be handled as a bad chunk from the server and we should disconnect and reconnect to another server */
					while(++currentClientNo!=MAX_CONNECTED_SERVERS&&!clientsArray[currentClientNo].connected())
					{}
					if(currentClientNo==MAX_CONNECTED_SERVERS)
					{
						currentClientNo=MAIN_CLIENT;
					}
					request.getBlockChunks(currentClientNo+1);
					synchronized = false;
				}
			}else if(reqData&&reqData->reqType&(uint32_t)(0x01<<TRANSACTION_BIT))
			{
				parseReceivedTx();
			}
		}
	}else if(key == "method"){
		if(value == "blockchain.headers.subscribe")
		{
			isMessage |= (0x01<<BLOCK_HEAD_SUB);
		}else if(value == "blockchain.scripthash.subscribe")
		{
			isMessage |= (0x01<<ADDRESS_SUB);
		}
	}else if(key == "result" && reqData)
	{
			uint32_t methodType = reqData->reqType;
			if(methodType&(uint32_t)(0x01<<VERSION_BIT))
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(value);
				#endif
			}else if(methodType&(uint32_t)(0x01<<ADDRESS_SUBS_BIT)){
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Address status "));
				#endif
				char status[65];
				int8_t index = getAddressPointerIndex(reqData);
				if(index<0){return;}
				userAddressPointerArray[index]->getStatus(status);
				char addr[36]; 
				char addressScriptHash[65]={};
				userAddressPointerArray[index]->getEncoded(addr);
				userAddressPointerArray[index]->getScriptHash(addressScriptHash,64);
				String dirName = "koyn/addresses/" + String(&addr[26]);
				String fileNameUtxo = dirName+"/"+"utxo";
				String fileNameStatus = dirName+"/"+"status";
				if(value!="null"&&!String(status).length())
				{
					request.getAddressHistory(addressScriptHash);
					request.listUtxo(addressScriptHash);
					if(SD.exists(&fileNameUtxo[0]))
					{
						SD.remove(&fileNameUtxo[0]);
						userAddressPointerArray[index]->clearBalance();
					}
					File statusFile = SD.open(&fileNameStatus[0],FILE_WRITE);
					if(statusFile)
					{
						statusFile.seek(0);
						statusFile.write(&value[0],64);
					}
					statusFile.close();
					userAddressPointerArray[index]->setAddressStatus(&value[0]);
				}else if(value!="null"&&String(status).length()&&!memcmp(&value[0],status,64))
				{
					#if defined(ENABLE_DEBUG_MESSAGES)
					Serial.println(F("Same status"));
					#endif
				}else if(value!="null")
				{
					request.getAddressHistory(addressScriptHash);
					request.listUtxo(addressScriptHash);
					if(SD.exists(&fileNameUtxo[0]))
					{
						SD.remove(&fileNameUtxo[0]);
						userAddressPointerArray[index]->clearBalance();
					}
					File statusFile = SD.open(&fileNameStatus[0],FILE_WRITE);
					if(statusFile)	
					{
						statusFile.seek(0);
						statusFile.write(&value[0],64);
					}
					statusFile.close();
					userAddressPointerArray[index]->setAddressStatus(&value[0]);
				}else
				{
					#if defined(ENABLE_DEBUG_MESSAGES)
					Serial.println(F("Address is new"));
					#endif
				}
			}else if(methodType&(uint32_t)(0x01<<PEERS_SUBS_BIT)){
				File myFile = SD.open("koyn/recServ",FILE_WRITE);
				if(myFile)
				{
					if(firstLevel)
					{
						myFile.println();
						firstLevel = false;
					}
					if(listener.levelArray == 2)
					{
						myFile.print(value);
						myFile.print(":");
					}else if(listener.levelArray == 3)
					{
						myFile.print(value);
						myFile.print(":");
					}
				}
				myFile.close();
			}else if(methodType&(uint32_t)(0x01<<BROADCAST_TRANSACTION_BIT)){
			}else{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("request doesn't exist"));
				#endif
			}
	}else if(key == "block_height" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||(reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT)))))
	{
		header.setNull();
		int32_t _height = my_atoll(&value[0]);
		header.height =  _height;
		header.calcPos();
	}else if(key == "version" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||(reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT)))))
	{
		uint32_t version = my_atoll(&value[0]);
		memcpy(header.completeHeader,&version,4);
	}else if(key == "prev_block_hash" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||(reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT)))))
	{
		uint8_t container[32];
		hex2bin(container,&value[0],64);
		reverseBin(container,32);
		memcpy(header.completeHeader+4,container,32);
	}else if(key == "merkle_root" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||(reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT)))))
	{
		uint8_t container[32];
		hex2bin(container,&value[0],64);
		reverseBin(container,32);
		memcpy(header.completeHeader+36,container,32);
	}else if(key == "timestamp" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||(reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT)))))
	{
		uint32_t timestamp = my_atoll(&value[0]);
		memcpy(header.completeHeader+68,&timestamp,4);
	}else if(key == "bits" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||(reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT)))))
	{
		uint32_t bits = my_atoll(&value[0]);
		memcpy(header.completeHeader+72,&bits,4);
	}else if(key == "nonce" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||(reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT)))))
	{
		uint32_t nonce = my_atoll(&value[0]);
		memcpy(header.completeHeader+76,&nonce,4);
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("New Header"));
		Serial.println(header.height);
		header.printHeader();
		Serial.println();
		#endif
		header.isValid = true;
		int32_t headerHeight = header.getHeight();
		if(checkBlckNumAndValidate(headerHeight))
		{
			switch(verifyBlockHeaders(&header))
			{
				case HEADER_VALID:
					updateTotalBlockNumb();
					for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
					{
						if(incomingTx[i].isUsed()&&incomingTx[i].inBlock())
						{
							if(isTransactionCallbackAssigned && incomingTx[i].getConfirmations()>0 && incomingTx[i].getConfirmations()<REMOVE_CONFIRMED_TRANSACTION_AFTER)
							{
								#if defined(ENABLE_DEBUG_MESSAGES)
								Serial.print(F("Block Difference "));
								Serial.println(incomingTx[i].getConfirmations());
								#endif
								(*transactionCallback)(incomingTx[i]);
							}else
							{
								incomingTx[i].resetTx();
							}
						}
					}
					break;
				case SAME_HEADER:break;
				case FORKED:break;
				case FORK_VALID:

				for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
				{
					if(incomingTx[i].isUsed()&&incomingTx[i].inBlock())
					{
						if(isTransactionCallbackAssigned && incomingTx[i].getConfirmations()>0 && incomingTx[i].getConfirmations()<REMOVE_CONFIRMED_TRANSACTION_AFTER)
						{
							#if defined(ENABLE_DEBUG_MESSAGES)
							Serial.print(F("Block Difference "));
							Serial.println(incomingTx[i].getConfirmations());
							#endif
							(*transactionCallback)(incomingTx[i]);
						}else
						{
							incomingTx[i].resetTx();
						}
					}
				}break;
				case INVALID:break;
			}
		}
		isMessage &= ~(1UL << BLOCK_HEAD_SUB);
	}else if(key == "params" && (isMessage&(0x01<<ADDRESS_SUB)))
	{
		if(addressPointer == NULL)
		{
			for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
			{
				if(userAddressPointerArray[i]==NULL){continue;}
				char addressScriptHash[65]={};
				userAddressPointerArray[i]->getScriptHash(addressScriptHash,64);
				if(!memcmp((uint8_t *)addressScriptHash,&value[0],64))
				{
					addressPointer = userAddressPointerArray[i];
				}
			}
		}else
		{
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(F("Address status "));
			#endif
			char status[65];
			addressPointer->getStatus(status);
			char addr[36];
			char addressScriptHash[65]={};
			addressPointer->getEncoded(addr);
			addressPointer->getScriptHash(addressScriptHash,64);
			String dirName = "koyn/addresses/" + String(&addr[26]);
			String fileNameUtxo = dirName+"/"+"utxo";
			String fileNameStatus = dirName+"/"+"status";
			if(value!="null"&&!String(status).length())
			{
				request.getAddressHistory(addressScriptHash);
				request.listUtxo(addressScriptHash);
				if(SD.exists(&fileNameUtxo[0]))
				{
					SD.remove(&fileNameUtxo[0]);
					addressPointer->clearBalance();
				}
				File statusFile = SD.open(&fileNameStatus[0],FILE_WRITE);
				if(statusFile)
				{
					statusFile.seek(0);
					statusFile.write(&value[0],64);
				}
				statusFile.close();
				addressPointer->setAddressStatus(&value[0]);
				addressPointer->resetGotAddress();
			}else if(value!="null"&&String(status).length()&&!memcmp(&value[0],status,64))
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Same status"));
				#endif
				addressPointer->resetGotAddress();
			}else if(value!="null")
			{
				request.getAddressHistory(addressScriptHash);
				request.listUtxo(addressScriptHash);
				if(SD.exists(&fileNameUtxo[0]))
				{
					SD.remove(&fileNameUtxo[0]);
					addressPointer->clearBalance();
				}
				File statusFile = SD.open(&fileNameStatus[0],FILE_WRITE);
				if(statusFile)
				{
					statusFile.seek(0);
					statusFile.write(&value[0],64);
				}
				statusFile.close();
				addressPointer->setAddressStatus(&value[0]);
				addressPointer->resetGotAddress();
			}
			isMessage &= ~(1UL << ADDRESS_SUB);
			addressPointer=NULL;
		}
	}else if(key == "confirmed" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADDRESS_BALANCE_BIT))))
	{
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		userAddressPointerArray[index]->setConfirmedBalance(my_atoll(&value[0]));
	}else if(key == "unconfirmed" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADDRESS_BALANCE_BIT))))
	{
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		userAddressPointerArray[index]->setUnconfirmedBalance(my_atoll(&value[0]));
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("Got Address Balance"));
		#endif
	}else if(key == "tx_hash" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADRRESS_HISTORY_BIT))))
	{
		tempAddressHistory.setTxHash(&value[0]);
	}else if(key == "height" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADRRESS_HISTORY_BIT))))
	{
		tempAddressHistory.setHeight(my_atoll(&value[0]));
		int32_t heightData = tempAddressHistory.getHeight();
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		char addr[36];
		userAddressPointerArray[index]->getEncoded(addr);	
		String dirName = "koyn/addresses/" + String(&addr[26]);
		String fileNameUnverifiedHistory = dirName+"/"+"his_unveri";
		if(heightData >0 && (saveNextHistory|| userAddressPointerArray[index]->addressHistory.isNull()))
		{
			File historyFile = SD.open(&fileNameUnverifiedHistory[0],FILE_WRITE);
			if(historyFile)
			{
				uint8_t hash[32];
				tempAddressHistory.getTxHash(hash);
				historyFile.write(hash,32);
				historyFile.write((uint8_t*)&heightData,4);
				// tempAddressHistory.setNull();
			}
			historyFile.close();
		}
		if(tempAddressHistory==userAddressPointerArray[index]->addressHistory)
		{
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(F("Reached last history save next "));
			#endif
			saveNextHistory= true;
		}
	}else if(key == "fee" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADRRESS_HISTORY_BIT))))
	{
		char txHash_str[65];
		txHash_str[64]='\0';
		tempAddressHistory.getStringTxHash(txHash_str);
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(txHash_str);
		#endif
		for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
		{
			if(incomingTx[i].isUsed())
			{
				uint8_t transactionHash[32];
				uint8_t historyHash[32];
				tempAddressHistory.getTxHash(historyHash);
				reverseBin(historyHash,32);
				incomingTx[i].getHash(transactionHash);
				if(!memcmp(transactionHash,historyHash,32))
				{
					return;
				}
			}
		}
		request.getTransaction(txHash_str);
	}else if(key == "block_height" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<MERKLE_PROOF))))
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(value);
		#endif
		uint32_t seekValue = (my_atoll(&value[0]));
		/* get Merkle root from blkhdrs */
		File blkHdrFile = SD.open("koyn/blkhdrs",FILE_READ);
		blkHdrFile.seek((seekValue*80)+36);
		for(int i=0;i<32;i++){merkleRoot[i]=blkHdrFile.read();}
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.write(merkleRoot,32);
		#endif
	}else if(key == "merkle" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<MERKLE_PROOF))))
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(value);
		#endif
		uint8_t txHash[32];
		hex2bin(txHash,&value[0],64);
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		char addr[36];
		userAddressPointerArray[index]->getEncoded(addr);	
		String fileNameMerkle = "koyn/addresses/" + String(&addr[26])+"/"+"merkle";
		File merkleFile = SD.open(&fileNameMerkle[0],FILE_WRITE);
		if(merkleFile)
		{
			merkleFile.write(txHash,32);
		}
		merkleFile.close();
	}else if(key == "pos" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<MERKLE_PROOF))))
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(value);
		#endif
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		userAddressPointerArray[index]->addressHistory.setLeafPos(my_atoll(&value[0]));
	}else if(key == "tx_hash" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADDRESS_UTXO_BIT)))){
				int8_t index = getAddressPointerIndex(reqData);
				if(index<0){return;}
				char addr[36];
				userAddressPointerArray[index]->getEncoded(addr);	
				String fileNameUtxo = "koyn/addresses/" + String(&addr[26])+"/"+"utxo";
				File addressUtxo = SD.open(&fileNameUtxo[0],FILE_WRITE);
				if(addressUtxo)
				{
					uint8_t hash[32]={};
					hex2bin(hash,&value[0],64);
					reverseBin(hash,32);
					addressUtxo.write(hash,32);
				}
				addressUtxo.close();
	}else if(key == "tx_pos" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADDRESS_UTXO_BIT)))){
				int8_t index = getAddressPointerIndex(reqData);
				if(index<0){return;}				
				char addr[36];
				userAddressPointerArray[index]->getEncoded(addr);	
				String fileNameUtxo = "koyn/addresses/" + String(&addr[26])+"/"+"utxo";
				File addressUtxo = SD.open(&fileNameUtxo[0],FILE_WRITE);
				if(addressUtxo)
				{
					uint32_t pos=my_atoll(&value[0]);
					addressUtxo.write((uint8_t*)&pos,4);
				}
				addressUtxo.close();
	}else if(key == "height" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADDRESS_UTXO_BIT)))){
				int8_t index = getAddressPointerIndex(reqData);
				if(index<0){return;}
				char addr[36];
				userAddressPointerArray[index]->getEncoded(addr);	
				String fileNameUtxo = "koyn/addresses/" + String(&addr[26])+"/"+"utxo";
				uint32_t height=0;
				File addressUtxo = SD.open(&fileNameUtxo[0],FILE_WRITE);
				if(addressUtxo)
				{
					height=my_atoll(&value[0]);
					addressUtxo.write((uint8_t*)&height,4);
				}
				addressUtxo.close();
				if(height){confirmedFlag = true;}else{confirmedFlag=false;}
	}else if(key == "value" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADDRESS_UTXO_BIT)))){
				int8_t index = getAddressPointerIndex(reqData);
				if(index<0){return;}
				char addr[36];
				userAddressPointerArray[index]->getEncoded(addr);	
				String fileNameUtxo = "koyn/addresses/" + String(&addr[26])+"/"+"utxo";
				uint64_t val=0;
				File addressUtxo = SD.open(&fileNameUtxo[0],FILE_WRITE);
				if(addressUtxo)
				{
					val=my_atoll(&value[0]);
					addressUtxo.write((uint8_t*)&val,8);
				}
				addressUtxo.close();
				if(confirmedFlag){userAddressPointerArray[index]->confirmedBalance+=val;}else{userAddressPointerArray[index]->unconfirmedBalance+=val;}
	}else
	{
		reparseFile = true;
	}
}

uint8_t KoynClass::trackAddress(BitcoinAddress * userAddress)
{
	if(isInit)
	{
		if(userAddress->address[0]=='1'||userAddress->address[0]=='3'){return MAIN_NET_NOT_SUPPORTED;}
		if(!userAddress->isTracked()&&synchronized)
		{
			int i;
			for(i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
			{
				if(userAddressPointerArray[i]==NULL)
				{
					userAddressPointerArray[i]=userAddress;
					break;
				}
			}
			if(i==5){return MAX_ADDRESSES_TRACKED_REACHED;}
			char addr[36];
			userAddress->getEncoded(addr);
			char addressScriptHash[65]={};
			userAddress->getScriptHash(addressScriptHash,64);
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(F("Tracking"));
			#endif
			request.subscribeToAddress(addressScriptHash);
			request.listUtxo(addressScriptHash);
			userAddress->setTracked();
			String dirName = "koyn/addresses/" + String(&addr[26]);
			if(!SD.exists(&dirName[0]))
			{
				SD.mkdir(&dirName[0]);
			}
			String fileNameStatus=dirName+"/"+"status";
			if(SD.exists(&fileNameStatus[0]))
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Old Status copied"));
				#endif
				File statusFile = SD.open(&fileNameStatus[0],FILE_READ);
				statusFile.read(userAddress->status,64);
			}
			String fileNameHistory=dirName+"/"+"history";
			if(SD.exists(&fileNameHistory[0]))
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Last TxHash copied"));
				#endif
				File historyFile = SD.open(&fileNameHistory[0],FILE_READ);
				uint32_t noOfTxHashes = historyFile.size()/36;
				historyFile.seek((noOfTxHashes-1)*36);
				uint8_t lastInFile[36];
				historyFile.read(lastInFile,36);
				userAddress->addressHistory.copyData(lastInFile);
			}
			String fileNameUtxo = dirName+"/"+"utxo";
			if(SD.exists(&fileNameUtxo[0]))
			{
				SD.remove(&fileNameUtxo[0]);
				userAddress->clearBalance();
			}
			return TRACKING_ADDRESS;
		}else
		{
				return TRACKING_ADDRESS_ERROR;
		}
	}else
	{
			return LIBRARY_NOT_INITIALIZED;
	}
}

void KoynClass::unTrackAddress(BitcoinAddress * userAddress)
{
	if(isInit)
	{
		for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
		{
			if(userAddressPointerArray[i]!=NULL && !strcmp(userAddressPointerArray[i]->address,userAddress->address))
			{
				userAddressPointerArray[i]=NULL;
				char addr[36];
				userAddress->getEncoded(addr);	
				String addressFolder = "koyn/addresses/" + String(&addr[26]);
				if(SD.exists(&addressFolder[0]))
				{
					#if defined(ENABLE_DEBUG_MESSAGES)
					Serial.println(F("Removing address folder"));
					#endif
					String dirName = "/koyn/addresses/"+String(&addr[26]);
			  		FatFile directory = SD.open(dirName);
			  		directory.rmRfStar();
				}
				return;
			}
		}
	}
}

void KoynClass::unTrackAllAddresses()
{
	if(isInit)
	{
		for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
		{
			userAddressPointerArray[i]=NULL;
		}
		if(SD.exists("koyn/addresses"))
		{
			#if defined(ENABLE_DEBUG_MESSAGES)
			Serial.println(F("Removing addresses folder"));
			#endif
			String dirName = "/koyn/addresses";
	  		FatFile directory = SD.open(dirName);
	  		directory.rmRfStar();
		}
	}
}

bool KoynClass::isAddressTracked(BitcoinAddress * userAddress)
{
	if(isInit)
	{
		return userAddress->tracked;
	}else
	{
		return TRACKING_ADDRESS_ERROR;
	}
}

WiFiClient * KoynClass::getMainClient()
{
	if(mainClient)
	{
		return mainClient;
	}else
	{
		return NULL;
	}
}

void KoynClass::setMainClient()
{
	for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(clientsArray[i].connected()&&!mainClient)
		{
			mainClient = &clientsArray[i];
			return;
		}
	}
}

WiFiClient * KoynClass::getClient(uint16_t no)
{
	return &clientsArray[no];
}

void KoynClass::updateTotalBlockNumb()
{
	if(SD.exists("koyn/blkhdrs"))
	{
		#if defined(ENABLE_DEBUG_MESSAGES)
		Serial.println(F("totalBlockNumb Updated"));
		#endif
		File blkHdrFile = SD.open("koyn/blkhdrs",FILE_WRITE);
		if(blkHdrFile){totalBlockNumb = ((blkHdrFile.size()) / 80)-1;}
	}
}

bool KoynClass::isSynced()
{
	if(isInit)
	{
		return synchronized;
	}else
	{
		return false;
	}
}

void KoynClass::onNewTransaction(void (* usersFunction)(BitcoinTransaction newTx))
{
	if(isInit)
	{
		transactionCallback=usersFunction;
		isTransactionCallbackAssigned=true;
	}
}

void KoynClass::onNewBlockHeader(void (*usersFunction)(uint32_t height))
{
	if(isInit)
	{
		newBlockCallback=usersFunction;
		isNewBlockCallbackAssigned=true;	
	}
}

void KoynClass::onError(void (*usersFunction)(uint8_t errCode))
{}

void KoynClass::getHeaderFromMainChain(BitcoinHeader * hdr,uint32_t height)
{
	File blkHdrFile = SD.open("koyn/blkhdrs",FILE_READ);
	if(blkHdrFile)
	{
		blkHdrFile.seek(height*80);
		uint8_t tempHeaderFromFile[80];
		for(int j=0;j<80;j++){tempHeaderFromFile[j]=blkHdrFile.read();}
		hdr->setHeader(tempHeaderFromFile,height);
	}
}

bool KoynClass::checkBlckNumAndValidate(uint32_t currentHeaderHeight)
{
	if(!noOfChunksNeeded)
	{
		uint32_t tempForkHeight=0;
		for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
		{
			uint32_t currentForkHeight = forks[i].getLastHeader()->getHeight();
			if(forks[i].exists() && currentForkHeight>tempForkHeight){tempForkHeight=currentForkHeight;}
		}
		if(tempForkHeight)
		{
			int32_t diff = currentHeaderHeight - tempForkHeight;
			if(currentHeaderHeight>tempForkHeight && diff>1 && diff<=5)
			{
				if(!requestsSent)
				{
					for(unsigned int i=0;i<(currentHeaderHeight-tempForkHeight);i++){request.getBlockHeader(totalBlockNumb+i);}
					fallingBackBlockHeight = currentHeaderHeight;
					requestsSent =true;
				}
				return false;

			}else if(diff <=1)
			{
				if(totalBlockNumb>=fallingBackBlockHeight){requestsSent=false;}
				synchronized =true;
				return true;
			}else
			{
				/* Drop connection, server is too slow */
				return false;
			}
		}
		if(totalBlockNumb<=currentHeaderHeight)
		{
			int32_t diff = currentHeaderHeight - totalBlockNumb;
			if(diff && diff<=5 && diff>1)
			{
				if(!requestsSent)
				{
					for(int i=0;i<=diff;i++){request.getBlockHeader(totalBlockNumb+i);}
					fallingBackBlockHeight = currentHeaderHeight;
					requestsSent =true;
				}
				return false;
			}else if(totalBlockNumb>2016 && diff>5)
			{
				uint32_t temp1 = totalBlockNumb;
				uint32_t temp2 = currentHeaderHeight;
				while(temp1%2016!=0){temp1--;}
				while(temp2%2016!=0){temp2++;}
				noOfChunksNeeded = (temp2 - temp1)/2016;
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.print(F("No of Chunks needed "));
				Serial.println(noOfChunksNeeded);
				#endif
				request.getBlockChunks(MAIN_CLIENT);
				return false;
			}else if(!totalBlockNumb)
			{
				noOfChunksNeeded = currentHeaderHeight/2016;
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.print(F("No of Chunks needed "));
				Serial.println(noOfChunksNeeded);
				#endif
				request.getBlockChunks(MAIN_CLIENT);
				return false;
			}else
			{
				if(totalBlockNumb>=fallingBackBlockHeight){requestsSent=false;}
				synchronized =true;
				return true;
			}
		}else if(totalBlockNumb>currentHeaderHeight)
		{
			return false;
		}else
		{
			return false;
		}
	}else
	{
		return false;
	}
}

uint32_t KoynClass::getBlockNumber()
{
	if(isInit)
	{
		return totalBlockNumb;
	}else
	{
		return 0;
	}
}

uint8_t KoynClass::spend(BitcoinAddress * from, BitcoinAddress * to, uint64_t amount, uint64_t fees, BitcoinAddress * change)
{
	if(isInit)
	{
		if(from->address[0]=='1'||from->address[0]=='3'){return MAIN_NET_NOT_SUPPORTED;}
		unsigned long long totalTransactionAmount = amount+fees;
		uint8_t privKey[32];
		char toAddr[36];
		char changeAddr[36];
		from->getPrivateKey(privKey);
		to->getEncoded(toAddr);
		change->getEncoded(changeAddr);

		if(!amount)
		{
			return INVALID_AMOUNT;
		}

		if(!from->isTracked())
		{
			return ADDRESS_NOT_TRACKED;
		}

		if(from->getConfirmedBalance()<=0)
		{
			return ADDRESS_NO_FUNDS;
		}

		if(totalTransactionAmount > from->getConfirmedBalance())
		{
			return ADDRESS_INSUFFICIENT_BALANCE;
		}

		if(!memcmp(privKey,emptyArray,32))
		{
			return ADDRESS_NO_PRIVATE_KEY;
		}

		if(!memcmp(toAddr,emptyArray,34)||!memcmp(changeAddr,emptyArray,34))
		{
			return ADDRESS_INVALID;
		}
		char addr[36];
		from->getEncoded(addr);
		String fileNameUtxo = "koyn/addresses/" + String(&addr[26])+"/"+"utxo";
		String fileNameTx = "koyn/addresses/" + String(&addr[26])+"/"+"tx";
		String fileNameFinalTx = "koyn/addresses/" + String(&addr[26])+"/"+"finaltx";
		if(SD.exists(&fileNameTx[0])){SD.remove(&fileNameTx[0]);}
		if(SD.exists(&fileNameFinalTx[0])){SD.remove(&fileNameFinalTx[0]);}
		if(SD.exists(&fileNameUtxo[0]))
		{
			File utxoFile = SD.open(&fileNameUtxo[0],FILE_READ);
			if(utxoFile.isOpen())
			{
				uint32_t unspentTransactionCount = utxoFile.size()/48;
				uint64_t amountArray[unspentTransactionCount];
				uint8_t amountArrayIndex[unspentTransactionCount];
				for(unsigned int i=0;i<unspentTransactionCount;i++){amountArrayIndex[i]=i;}
				Serial.println(F("Allocated memory"));
				uint32_t i=0;
				while(utxoFile.available())
				{
					utxoFile.seekCur(40);
					utxoFile.read((uint8_t*)&amountArray[i],8);
					i++;
				}
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Got All UTXO's Amount"));
				#endif
				for(unsigned int l=0;l<unspentTransactionCount;l++)
				{
					for(unsigned int j=l+1;j<unspentTransactionCount;j++)
					{
						if(amountArray[l]>amountArray[j])
						{
							uint64_t temp;
							temp=amountArray[j];
							amountArray[j]=amountArray[l];
							amountArray[l]=temp;
							uint8_t temp1;
							temp1=amountArrayIndex[j];
							amountArrayIndex[j]=amountArrayIndex[l];
							amountArrayIndex[l]=temp1;
						}
					}
				}
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.println(F("Re-ordered UTXO's Amounts"));
				#endif
				uint64_t changeAmount=0;
				uint32_t version=0x01;
				uint32_t hashCode = 0x01;
				uint32_t sequence = 0xffffffff;
				uint32_t lockTime= 0x00;
				bool gotIndividualTx=false;
				int8_t indexArray[unspentTransactionCount];
				uint64_t accumilativeAmount=0;
				i=0;
				for(unsigned int j=0;j<unspentTransactionCount;j++)
				{
					if(amountArray[j]>=totalTransactionAmount)
					{
						indexArray[0]=amountArrayIndex[j];
						accumilativeAmount=amountArray[j];
						gotIndividualTx=true;
						i++;
						break;
					}
				}
				if(!gotIndividualTx)
				{
					for(int j=unspentTransactionCount-1;j>=0;j--)
					{
						accumilativeAmount+=amountArray[j];
						indexArray[i]=amountArrayIndex[j];
						i++;
						if(accumilativeAmount>totalTransactionAmount){break;}
					}
				}
				if(accumilativeAmount >= totalTransactionAmount){changeAmount = accumilativeAmount-totalTransactionAmount;}else{return TRANSACTION_BUILD_ERROR;}
				uint8_t hash[32];
				uint8_t signature[64];
				uint8_t scriptPubKey[25];
				uint8_t scriptPubKeyLen = 0x19;
				uint8_t outputNo=1;
				uint8_t inputNo=i;
				uint8_t * ptr;
				uint32_t preImageSize = 4+1+(66*i)+1+8+1+25+4+4;
				if(changeAmount){preImageSize+=8+1+25;outputNo+=1;}
				uint8_t preImage[preImageSize];
				ptr=(uint8_t *)memcpy(preImage,(uint8_t*)&version,4);ptr+=4;
				memcpy(ptr,&inputNo,1);ptr+=1;
				utxoFile.seekSet(0);
				for(int j=0;j<inputNo;j++)
				{
					uint8_t data[36];
					utxoFile.seek(48*indexArray[j]);
					utxoFile.read(data,36);
					memcpy(ptr,data,32);ptr+=32;
					memcpy(ptr,data+32,4);ptr+=4;
					memcpy(ptr,&scriptPubKeyLen,1);ptr+=1;
					from->getScript(scriptPubKey,25);
					memcpy(ptr,scriptPubKey,25);ptr+=25;
					memcpy(ptr,(uint8_t*)&sequence,4);ptr+=4;
				}
				memcpy(ptr,&outputNo,1);ptr+=1;
				to->getScript(scriptPubKey,25);
				memcpy(ptr,(uint8_t *)&amount,8);ptr+=8;
				memcpy(ptr,&scriptPubKeyLen,1);ptr+=1;
				memcpy(ptr,scriptPubKey,25);ptr+=25;
				if(changeAmount)
				{
					change->getScript(scriptPubKey,25);
					memcpy(ptr,(uint8_t *)&changeAmount,8);ptr+=8;
					uint8_t scriptPubKeyLen = 0x19;
					memcpy(ptr,&scriptPubKeyLen,1);ptr+=1;
					memcpy(ptr,scriptPubKey,25);ptr+=25;
				}
				memcpy(ptr,(uint8_t *)&lockTime,4);ptr+=4;
				memcpy(ptr,(uint8_t *)&hashCode,4);

			    File transactionFile = SD.open(&fileNameTx[0],FILE_WRITE);
			    if(transactionFile.isOpen())
			    {
			    	transactionFile.write((uint8_t *)&version,4);
			    	transactionFile.write(i);
						for(unsigned int j=0;j<i;j++)
						{
							transactionHash(hash,preImage,preImageSize,j);
							do {
						        uECC_sign(privKey, hash, 32, signature, from->curve);
						        yield();
						    } while ((signature[0] & 0x80) || (signature[32] & 0x80));
					    uint8_t derSignatureLen = 0x47;
					    uint8_t derSignature[derSignatureLen];
					    derSignature[0]= ASN1_BMPSTRING;
					    derSignature[1]= 0x44;
					    derSignature[2]= ASN1_INTEGER;
					    derSignature[3]= 0x20;
					    memcpy(derSignature+4,signature,32);
					    derSignature[36]= ASN1_INTEGER;
					    derSignature[37]= 0x20;
					    memcpy(derSignature+38,signature+32,32);
					    derSignature[70]=0x01;

				    	uint8_t data[36];
							utxoFile.seek(48*indexArray[j]);
							utxoFile.read(data,36);
				    	transactionFile.write(data,32);
				    	transactionFile.write(data+32,4);
				    	uint8_t scriptSigLen = derSignatureLen+33+2;
				    	transactionFile.write(&scriptSigLen,1);
				    	transactionFile.write(&derSignatureLen,1);
				    	transactionFile.write(derSignature,derSignatureLen);
				    	transactionFile.write(0x21);
				    	uint8_t compPubKey[33];
				    	from->getPublicKey(compPubKey);
				    	transactionFile.write(compPubKey,33);
				    	transactionFile.write((uint8_t*)&sequence,4);
						}

						transactionFile.write(outputNo);
			    	transactionFile.write((uint8_t*)&amount,8);
			    	transactionFile.write(0x19);
			    	to->getScript(scriptPubKey,25);
			    	transactionFile.write(scriptPubKey,25);
			    	if(changeAmount)
			    	{
			    		transactionFile.write((uint8_t*)&changeAmount,8);
				    	transactionFile.write(0x19);
				    	change->getScript(scriptPubKey,25);
				    	transactionFile.write(scriptPubKey,25);
			    	}
		    		transactionFile.write((uint8_t *)&lockTime,4);
			    }else
					{
						return ERROR_OPEN_FILE;
					}
			    transactionFile.close();
			    transactionFile = SD.open(&fileNameTx[0],FILE_READ);
			    File finalFile = SD.open(&fileNameFinalTx[0],FILE_WRITE);
			    if(convertFileToHexString(&transactionFile,&finalFile))
			    {
			    	transactionFile.close();
			    	SD.remove(&fileNameTx[0]);
			    	transactionFile = SD.open(&fileNameFinalTx[0],FILE_READ);
			    	if(transactionFile.isOpen())
				    {
					    request.broadcastTransaction(&transactionFile);
					    transactionFile.close();
					    SD.remove(&fileNameFinalTx[0]);
				    }else
						{
							return ERROR_OPEN_FILE;
						}
			    }else
					{
						return TRANSACTION_BUILD_ERROR;
					}
			}else
			{
				return ERROR_OPEN_FILE;
			}
		    utxoFile.close();
		    return TRANSACTION_PASSED;
		}else
		{
			return MISSING_FILE;
		}
	}else
	{
		return LIBRARY_NOT_INITIALIZED;
	}
}

uint8_t KoynClass::spend(BitcoinAddress * from, BitcoinAddress * to, uint64_t amount, uint64_t fees)
{
	return spend(from,to,amount,fees,from);
}


void KoynClass::delay(unsigned long time)
{
	if(isInit)
	{
		unsigned long now=millis();
		while((millis()<(now+time)))
		{
			run();
		}
	}
}

void KoynClass::removeUnconfirmedTransactions()
{
	for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
	{
		if(incomingTx[i].isUsed()&&!incomingTx[i].inBlock())
		{
			if(incomingTx[i].getUnconfirmedIterations()==REMOVE_UNCONFIRMED_TRANSACTION_AFTER)
			{
				#if defined(ENABLE_DEBUG_MESSAGES)
				Serial.print(F("Removing unconfirmed transaction"));
				#endif
				incomingTx[i].resetTx();
			}
		}
	}
}

int8_t KoynClass::getAddressPointerIndex(ElectrumRequestData * reqDataPointer)
{
	for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
	{
		if(userAddressPointerArray[i]==NULL){continue;}
		char addressScriptHash[65]={};
		userAddressPointerArray[i]->getScriptHash(addressScriptHash,64);
		if(!strcmp(addressScriptHash,(char *)reqDataPointer->dataString))
		{
			return i;
		}
	}
	return -1;
}

KoynClass Koyn;
