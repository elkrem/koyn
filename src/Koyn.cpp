#include "Koyn.h"



KoynClass::KoynClass()
{
	totalBlockNumb = 0;
	chunkNo =0;
	saveResToFile = 0;
	isMessage = 0;
	historyFileLastPos = 0;
	mainClient = NULL;
	reqData = NULL;
	bigFile = false;
	synchronized = false;
	lastMerkleVerified = false;
	isFirstMerkle = true;
	saveNextHistory = false;
	reparseFile=false;
	confirmedFlag=false;
	isInit=false;
	for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++){userAddressPointerArray[i]=NULL;}
}



void KoynClass::begin(bool _verify)
{
	#ifdef	USE_MAIN_NET
	#error Main net currently not supported
	#endif
	verify = _verify;
	checkSDCardMounted();
	checkDirAvailability();
/* connection to servers using clients*/
	connectToServers();
/* setting a main client to communicate with for extra electrum requests.*/
	setMainClient();
	syncWithServers();
}

void KoynClass::checkSDCardMounted()
{
	/* Stick here until SD card mounted. */
	do{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Mount SD card"));
		#endif
	}while(!SD.begin(SD_CARD_CHIP_SELECT,SD_SCK_MHZ(4)));
}

void KoynClass::checkDirAvailability()
{
	if (!SD.chdir()) {
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("chdir to root failed.\n"));
		#endif
	}
	/* Make sure to remove the files in the directory first to remove the directory*/
	if(SD.exists("koyn/responses"))
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Removing Response folder"));
		#endif
		String dirName = "/koyn/responses";
  		FatFile directory = SD.open(dirName);
  		directory.rmRfStar();
	}
	::delay(200);
	SD.mkdir("koyn/responses");

	if(SD.exists("koyn/blkhdrs"))
	{
		File blkHeaderFile =  SD.open("koyn/blkhdrs",FILE_READ);

		if(!blkHeaderFile){
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Cannot open file"));
		#endif
		return;
		}

		if(blkHeaderFile.available())
		{
			if(verify)
			{
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Start Verifying"));
				#endif
				while (blkHeaderFile.available())
				{
					uint8_t currentHeader[80];
					totalBlockNumb = (blkHeaderFile.size() / 80)-1;  /*blockNum will be checked in the bootstrap file and if the same so we are updated with the bootsrap file but still not the network*/
					for (uint32_t i = 0; i < totalBlockNumb; i++)
					{
						for (uint16_t j = 0; j < 80; j++)
						{
							currentHeader[j] = blkHeaderFile.read();
						}
						header.setHeader(currentHeader,i);
						switch(verifyBlockHeaders(&header))
						{
							case HEADER_VALID: break;
							case INVALID: return;
						}
						header.setNull();
					}
				}
			}else
			{
				#if defined(ENABLE_DEBUG_MESSAGE)
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
				#if defined(ENABLE_DEBUG_MESSAGE)
				lastHeader.printHeader();
				/* We must make a check here to verify that we got a correct block header */
				Serial.println(totalBlockNumb);
				Serial.println(chunkNo);
				#endif
			// return true;
			}
		}
	}else
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Cannot find blkHdr file"));
		#endif
	}
}

void KoynClass::syncWithServers()
{
/* We Should open the file on the SD card and get the latest block number and request block number from the servers to
compare them and check if we are not up to date.
- Open File, get last block number, save it in Electrum client.
- Request block numbers from server.
- Compare Block number and if the local block number is less than the returned by the server request the block headers
first and update them in the SD card file.
- Update bootstrap file with the last block number and block header.*/
	request.sendVersion();
	request.subscribeToBlockHeaders();
	// request.subscribeToPeers();	 /* get peers and save them in a file on SD card and make sure that there are no servers name duplicate, also we should disconnect from the bootsraped servers and connect to new servers returned.*/
// request.getBlockChunks(0);	/* Syncing block chunks will be performed with only 1 client connected at first before connecting to another clients and mixing up data in file.*/
}

int8_t KoynClass::verifyBlockHeaders(BitcoinHeader * currhdr)
{
/* This function verifies the block headers in the file and make sure that they are all valid.
   we will give a user a tool to generate .bin file having the headers till now and upload it to
   spiffs or SD.
   The verifier will check the file called (blockHeaders.bin) to know the last block number in the system
   and the last header.
   Whenever we finished verifying and we are sure that the headers are ok we will update the bootstrap file
   with the last block number and header so when the system goes offline always return back to know the last
   updated variables.

   In the stated file there should be a simple variable to prevent/allow the hardware not to go through the
   verifying process from all over again.

   Make sure also to know how electrum verifies another entities like the target, this will help us if the device is
   offline the user will be able to accept transactions.

   update 27/12/2017
   verifyBlockHeaders should return states and not a true or false, which means if the header is new it should return 1 or if the
   header is new but fork it should return 2 and if same it should return 3 and if not valid at all it should return 0.

   update 4/02/2018
   Always make sure that we are 1 block behind before validating, otherwise update the system first
*/
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
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Genisis Block not verified"));
				#endif
				return INVALID;
			}else {
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Genisis Block verified"));
				#endif
				lastHeader=*currhdr;
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
			/* We Should also check the timestamp as a validation */
			if(target[c]>currentHeaderHash[c])
			{

				if(headerHeight>lastHeader.height)
				{
					//  Check time difference here if more than 2 hours return Invalid, but make sure to disable this check
					//    verifying a chunk coincidentally we stopped at wrong chain.
				    uint8_t hash1[32];
				    uint8_t hash2[32];
				    lastHeader.getHash(hash1);
					currhdr->getPrevHash(hash2);
					if(!memcmp(hash1,hash2,32))
					{
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.print(headerHeight);
						Serial.println(F("Valid "));
						#endif
						/* Save new header to file */
						/* Save current header into blkhdrs file.*/
						File blkHdrFile = SD.open("koyn/blkhdrs",FILE_WRITE);
						if(blkHdrFile)
						{
							blkHdrFile.write(currhdr->completeHeader,80);
							blkHdrFile.close();
						}
						lastHeader = *currhdr;
						if(isNewBlockCallbackAssigned)
						{
							(*newBlockCallback)(headerHeight);
						}
						return HEADER_VALID;
					}else
					{
						/* A fork happened at this server and was solved or we got a fork already*/
						/* Create a backward fork, create a file called fork and save the previous header */
						return catchingUpFork(currhdr);
					}
				}else if(headerHeight == lastHeader.height)
				{
					if(lastHeader == *currhdr)
					{
						#if defined(ENABLE_DEBUG_MESSAGE)
						lastHeader.printHeader();
						Serial.println(lastHeader.height);
						Serial.println(F("Same header"));
						#endif
						return SAME_HEADER;
					}else if(lastHeader !=*currhdr)
					{
						/* get previous header and make check if the current header is built upon */
						/* if built over the last header from main chain so this is a current fork */
						/* else if not so this is a backward fork, jump back a step and get the header*/
						/* offcourse we gonna create a fork file */
						return catchingUpFork(currhdr);
					}
				}else if(headerHeight < lastHeader.height)
				{
					/* We are working here on a backward fork */
					/* check all forking files and make a chain */
					/* Make sure not to request old headers multiple times */
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
							#if defined(ENABLE_DEBUG_MESSAGE)
							Serial.println(F("PH"));
							#endif
							currhdr->parentHeader=true;
							switch(catchingUpFork(currhdr))
							{
								case FORK_VALID:
								#if defined(ENABLE_DEBUG_MESSAGE)
								Serial.println(F("Got parent header "));
								Serial.println(headerHeight);
								#endif
								break;
							}
							prevHeader=*currhdr;
							return FORK_VALID;
						}
					}
				}
			}else
			{
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.print(F("New Header is not Valid "));
				Serial.println(currhdr->getHeight());
				#endif
				return INVALID;
			}
		}
	}
}

int8_t KoynClass::catchingUpFork(BitcoinHeader *currhdr)
{
	#if defined(ENABLE_DEBUG_MESSAGE)
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
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.print(headerHeight);
						Serial.println(F("Fork Valid "));
						#endif
						File forkFile = SD.open(fileName,FILE_WRITE);
						if(forkFile)
						{
							forkFile.write(currhdr->completeHeader,80);
							/* Write the heigth to file */
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
							#if defined(ENABLE_DEBUG_MESSAGE)
							Serial.println(F("Failed to open 1"));
							#endif
						}
					}
				}else if(headerHeight == forks[i].getLastHeader()->height)
				{
					if((*forks[i].getLastHeader()) == *currhdr)
					{
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Same fork header"));
						#endif
						return SAME_HEADER;
					}else
					{
						/* Here we should create another fork, as the same last height wasn't equal the two branches*/
					}
				}else if(headerHeight < forks[i].getLastHeader()->height)
				{
					File forkFile = SD.open(fileName,FILE_READ);
					if(forkFile)
					{
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Opened and header smaller"));
						#endif
						if(forkFile.available())
						{
							BitcoinHeader tempHeader;
							uint8_t tempHeaderFromFile[84];
							for(int j=0;j<84;j++){tempHeaderFromFile[j]=forkFile.read();}
							uint32_t tempHeight = *((uint32_t *)(tempHeaderFromFile+80));
							tempHeader.setHeader(tempHeaderFromFile,tempHeight);
							#if defined(ENABLE_DEBUG_MESSAGE)
							Serial.println(tempHeader.getHeight());
							#endif
							if(tempHeader==*currhdr)
							{
								#if defined(ENABLE_DEBUG_MESSAGE)
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
										/* Save previous header in a temp file */
										#if defined(ENABLE_DEBUG_MESSAGE)
										Serial.print(headerHeight);
										Serial.println(F("Previous header at fork"));
										#endif
										tempFile.write(currhdr->completeHeader,80);
										/* Write the heigth to file */
										uint32_t currHdrHeight = currhdr->getHeight();
										tempFile.write((uint8_t *)&currHdrHeight,4);
										#if defined(ENABLE_DEBUG_MESSAGE)
										Serial.println(F("Copying data"));
										#endif
										forkFile.seek(0);
										while(forkFile.available()){
											uint8_t data =forkFile.read();
											#if defined(ENABLE_DEBUG_MESSAGE)
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
									}
								}
							}else if(tempHeader.height<headerHeight)
							{
								/* Not error but this is a case where the fork is updated while still the previous headers
								   didn't reach
							    */
								return ERROR;
							}
						}
					}else{
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Failed to open 2"));
						#endif
					}
				}
			}
		}
	}
	/* No Fork was found so this is a new fork solved remotely by the servers */
	/* Request the previous header */
	String fileName = String("koyn/forks/")+"fork"+currentClientNo;
	File forkFile = SD.open(fileName,FILE_WRITE);
	if(forkFile)
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println("Fork created");
		#endif
		forkFile.write(currhdr->completeHeader,80);
		/* Write the heigth to file */
		uint32_t currHdrHeight = currhdr->getHeight();
		forkFile.write((uint8_t *)&currHdrHeight,4);
		forkFile.close();
	}else{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Failed to open 0"));
		#endif
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
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.print(F("Checking difficulty with forks "));
			Serial.println(i);
			#endif
			BitcoinHeader t1 = *forks[i].getLastHeader();
			uint32_t forkDifficulty = *((uint32_t *)(t1.completeHeader+72));
			uint32_t tempDifficulty = *((uint32_t*)(tempHeader.completeHeader+72));
			if(forkDifficulty<tempDifficulty)
			{
				tempHeader=*forks[i].getLastHeader();
			}
		}
	}
	if(tempHeader!=lastHeader)
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Chain with bigger difficulty"));
		#endif
		for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
		{
			if(forks[i].exists()&&forks[i].gotParentHeader())
			{
				String fileName = String("koyn/forks/")+"fork"+i;
				if(tempHeader==*forks[i].getLastHeader())
				{
					/* reorg chain to have this fork and remove all other forks and set forks with Null then update totalBlockNumb and break */
					File forkFile = SD.open(fileName,FILE_READ);
					File blkHeaderFile = SD.open("koyn/blkhdrs",FILE_WRITE);
					if(forkFile&&blkHeaderFile)
					{
						uint32_t seekValue=forks[i].getParentHeader()->getHeight();
						blkHeaderFile.seek(seekValue*80);
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.print("Seek ");
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
		/* wait for tallest chain height and then swap the chain with main chain and set all forks with Null */
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
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Getting longest chain"));
				#endif
				if(diff>=LONGEST_CHAIN_AT_FORK)
				{
					#if defined(ENABLE_DEBUG_MESSAGE)
					Serial.println(F("Chain is long"));
					#endif
					/* reorg chain to have this fork and remove all other forks and set forks with Null then update totalBlockNumb and break */
					File forkFile = SD.open(fileName,FILE_READ);
					File blkHeaderFile = SD.open("koyn/blkhdrs",FILE_WRITE);
					if(forkFile&&blkHeaderFile)
					{
						uint32_t seekValue=forks[i].getParentHeader()->getHeight();
						blkHeaderFile.seek(seekValue*80);
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.print("Seek ");
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
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.print(F("Chain diff "));
						Serial.println(diff);
						#endif
				}
			}
		}
		/* Solving case if all forks stopped growing and the main chain is growing only */
		if(tempHeader.getHeight()>0&&lastHeader.getHeight()-tempHeader.getHeight()>LONGEST_CHAIN_AT_FORK)
		{
			for(int j=0;j<MAX_CONNECTED_SERVERS;j++){String fileName = String("koyn/forks/")+"fork"+j;SD.remove(&fileName[0]);forks[j].setNull();}
			return;
		}
	}
}

void KoynClass::connectToServers()
{
	/*In this method we will grab the bootstrap servers from the file (bootstrap.bin) and try to connect to them.
 	  max clients will be updated by the user from the Koyn.h file according to the users settings, so we will make
  	  an iteration to loop on the clients and then make sure that they are all connected. If anyone of them didn't
  	  connect successfully we will go fetch another server to connect to.
	*/
	uint8_t mainNetArrSize =  sizeof(testnetServerNames)/sizeof(testnetServerNames[0]);
	uint16_t serverNamesCount =0;
	for(uint16_t i =0;i<MAX_CONNECTED_SERVERS;i++)
	{
		char  servName[strlen_P(testnetServerNames[serverNamesCount])+1];
		while(!clientsArray[i].connect(strcpy_P(servName,testnetServerNames[serverNamesCount]),testnetPortNumber[serverNamesCount]))
		{
			if(serverNamesCount <= mainNetArrSize)
			{
				serverNamesCount++;
			}
			else{
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Cannot connect to listed servers"));
				#endif
				return;
			}
		}
		serverNamesCount++;
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.print(String("Client ")+String(i)+String(" connected to "));
		Serial.println(servName);
		#endif
		String dirName = String("koyn/responses/")+"client"+i;
		if(!SD.exists(&dirName[0]))
		{
			SD.mkdir(&dirName[0]);
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.println(String("Created directory ")+dirName);
			#endif
		}
	}
}

void KoynClass::run()
{
	/* process data file here */
	/* after process is finished please empty the file with nulls so that we can rewrite over the data again.
	   Problems:
	   1- We need to empty the file after we process the data inside just to make sure not to parse the same data again in the next loops.
	   2- We should also make sure that all the data is parsed and that there are no incomplete data before clearing the file, preventing data loss
	   from the next loop.
	   3- In case of huge data (Chunks-Peers-History) we must make sure that there are no different data in between like (Version etc.) preventing
	   incorrect data parsing.
	   4- Parsing large data like chunks will be done individually before entering any other functionality in the system.
	   5- Peers should be parsed and saved directly to a file called (recServ.txt) making sure there are no objects inside them.
	   6- Transaction history hashes&height should be copied to a new file called history accompained with the address appended at the top
	   of the file to make sure that the next hashes are for that certain address. and make more files for more addresses history.
	   7- Merkle checking also has the same issue like tx history as we will save the data in a file first then we will do the
	 */
	if(millis()-lastTimeTaken>55*1000)
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Sending Version"));
		#endif
		request.sendVersion();
		lastTimeTaken = millis();
	}
	if(synchronized)
	{
		removeUnconfirmedTransactions();
		for(int i=0;i<MAX_ADDRESSES_TRACKED_COUNT;i++)
		{
			if(userAddressPointerArray[i]!=NULL)
			{
				char addr[36]; /* We must always check type of address before declaring the array to know the length */
				userAddressPointerArray[i]->getEncoded(addr);
				String dirName = "koyn/addresses/" + String(&addr[26]);
				String fileNameUnverifiedHistory = dirName+"/"+"his_unveri";
				String fileNameHistory = dirName+"/"+"history";
				String fileNameMerkle = dirName+"/"+"merkle";
				/* Requesting Merkle proofs */
				if(SD.exists(&fileNameUnverifiedHistory[0])&&(lastMerkleVerified||isFirstMerkle))
				{
					File historyFile = SD.open(&fileNameUnverifiedHistory[0],FILE_READ);
					if(historyFile&&historyFileLastPos==historyFile.size())
					{
						if(SD.exists(&fileNameHistory[0]))
						{
							File oldFile = SD.open(&fileNameHistory[0],FILE_WRITE);
							while(historyFile.available())
							{
								oldFile.write(historyFile.read());
							}
							oldFile.close();
							SD.remove(&fileNameUnverifiedHistory[0]);
							historyFileLastPos=0;
						}else
						{
							if (!SD.chdir(&dirName[0])) {
								#if defined(ENABLE_DEBUG_MESSAGE)
								Serial.println(F("chdir failed"));
								#endif
							}
							/* Rename file */
							historyFile.rename(SD.vwd(), "history");
							historyFileLastPos=0;
							if (!SD.chdir()) {
								#if defined(ENABLE_DEBUG_MESSAGE)
								Serial.println(F("chdir failed"));
								#endif
							}
						}
					}else if(historyFile)
					{
						uint8_t container[36];
						historyFile.seek(historyFileLastPos);
						if(historyFile.available())
						{
							for(int i=0;i<36;i++)
							{
								container[i]=historyFile.read();
							}
						}
						userAddressPointerArray[i]->lastTxHash.copyData(container);
						char txHash_str[65];
						txHash_str[64]='\0';
						userAddressPointerArray[i]->lastTxHash.getStringTxHash(txHash_str);
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Getting Merkle Proofs"));
						#endif
						request.getMerkleProof(addr,(const char *)txHash_str,userAddressPointerArray[i]->lastTxHash.getHeight());
						if(isFirstMerkle){isFirstMerkle=false;}
						historyFileLastPos = historyFile.curPosition();
						lastMerkleVerified = false;
					}
				}
				/* Verifying Merkle tree */
				if(SD.exists(&fileNameMerkle[0]))
				{
					#if defined(ENABLE_DEBUG_MESSAGE)
					Serial.println(F("Merkle"));
					#endif
					File merkleFile = SD.open(&fileNameMerkle[0],FILE_READ);
					/* We should make sure first that we got the right blockheader */
					uint8_t hash[32]={};
					uint8_t arrayToHash[64];
					uint8_t merkleLeaf[32];
					uint32_t leafPos = userAddressPointerArray[i]->lastTxHash.getLeafPos();
					uint32_t numberOfLeafs = merkleFile.size()/32;
					userAddressPointerArray[i]->lastTxHash.getTxHash(hash);
					reverseBin(hash,32);
					for(int j = 0 ;j<numberOfLeafs;j++)
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
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Merkle Verified ..! "));
						#endif
						lastMerkleVerified = true;
						for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
						{
							if(incomingTx[i].isUsed()&&!incomingTx[i].inBlock())
							{
								uint8_t txHash[32];
								uint8_t hash[32];
								incomingTx[i].getHash(txHash);
								userAddressPointerArray[i]->lastTxHash.getTxHash(hash);
								reverseBin(hash,32);
								if(!memcmp(txHash,hash,32))
								{
									incomingTx[i].setHeight(userAddressPointerArray[i]->lastTxHash.getHeight());
									/* Consider calling user callback here */
									if(isTransactionCallbackAssigned)
									{
										(*transactionCallback)(incomingTx[i]);
									}
								}
							}
						}
					}else
					{
						/* Remove history File and request it from another server */
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Bad History Removing .."));
						#endif
						lastMerkleVerified = false;
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
		if(!clientsArray[i].connected())
		{
			/*
				TODO:
				- In case client is disconnected, we should recover by reconnecting to another client.
				- Also we should pick randomly from recServ file.
			*/
			connectToServers();
		}
		while(clientsArray[i].available())
		{
		 /*save all incoming data in a file called data on SD card and then after there are no data available from the clients
		   just close the file then open it again and process the data inside it.

		   this will solve the problem of chunks that we will have to save the large files inside the data file and then parse them
		   byte by byte

		   I am having a concern that when we are processing the large data from the data file it won't be able to get the data
		   from the clients and the buffer will be overflowed and the data will be lost.*/

			char data =clientsArray[i].read();
			if(data != '\n')
			{
				if(!opened)
				{
					String fileName = "koyn/responses/client"+String(i)+"/"+"uncomplete";
					responseFile = SD.open(&fileName[0],FILE_WRITE);
					opened = true;
					if(responseFile){
					#if defined(ENABLE_DEBUG_MESSAGE)
					/*Serial.println("File Opened");*/
					#endif	
					}else{
					#if defined(ENABLE_DEBUG_MESSAGE)
					Serial.println(F("File not opened"));
					#endif
					}
				}
				#if defined(ENABLE_DEBUG_MESSAGE)
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
				if (!responseFile.rename(&directory, &String(millis())[0])) /* In this line we should rename the file using the timeStamp*/
				{
					#if defined(ENABLE_DEBUG_MESSAGE)
					Serial.println(F("Cannot rename file"));
					#endif
				}
				responseFile.close();
				opened = false;
			}
		}
		responseFile.close();
	}
	/* Parsing completed files*/
	for(int i =0 ;i<MAX_CONNECTED_SERVERS;i++)
	{
		char buff[13];
		currentClientNo = i;
		String dirName = "koyn/responses/client"+String(i);
		FatFile directory = SD.open(dirName);

		while (file.openNext(&directory,O_READ)) {
			file.getName(buff,13);
			if(!(String(buff)==String("uncomplete")))
			{
				#if defined(ENABLE_DEBUG_MESSAGE)
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
				/* Reseting saveNextHistory so that the next time a history received won't save it again */
				if(saveNextHistory){saveNextHistory=false;}
				if(reqData)
				{
					reqData->reqType=0;
					reqData->resetUsed();
					reqData = NULL;
					reparseFile = false;
				}
				file.close();
				/* remove file*/
				if(directory.remove(&directory,&buff[0]))
				{
					#if defined(ENABLE_DEBUG_MESSAGE)
					Serial.print(F("Removed file "));
					Serial.println(buff);
					#endif
				}else
				{
					#if defined(ENABLE_DEBUG_MESSAGE)
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

bool KoynClass::parseReceivedChunk()
{
	if(!file){
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Cannot open file"));
		#endif
		return false;
	}
	file.seek(0);
	if(file.available())
	{	char data[8];
		file.read(data,8);
		while(memcmp(data,"\"result\"",8))
		{
			for(int i=0;i<=7;i++){data[i]=data[i+1];}
			data[7]=file.read();
		}
		while(file.read()!='"');
		bigFile = true;
		uint32_t currentHeight = chunkNo*2016;
		/* We must check that totalBlockNum is < than currentHeight otherwise our system is in the middle of the current block somwhere*/
		if(totalBlockNumb>currentHeight)
		{
			/* Set the position to make sure first that the header we are last waiting is the same in this chunk */
			uint16_t hdrPos = lastHeader.getPos();
			file.seekCur((hdrPos*160));
			currentHeight = totalBlockNumb;
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.print(F("Last Header Position "));
			Serial.println(lastHeader.getPos());
			#endif
		}

		while (file.available())
		{
			char data;
			bool endOfObject = false;
			uint8_t currentHeader[80]={};
			char currentHeaderString[160]={};
			for (uint16_t j = 0; j < 160; j++)
			{
				data = file.read();
				if(data == '"' || data == '}'){endOfObject = true;}else{
					currentHeaderString[j] = data;
				}
			}
			#if defined(ENABLE_DEBUG_MESSAGE)
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
		                #if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Not Verified"));
	                    Serial.println(currentHeaderString);
		                Serial.write(header.completeHeader,80);
						#endif
		                return false;
				}
				header.setNull();
				currentHeight++;
			}
			/* While validating the whole 2016 blocks it takes more than 13 minutes so the servers disconnects,
			   with this solution we will be able to send local request (without saving it) to just maintain
			   the connection with servers
		    */
			if(millis()-lastTimeTaken>90*1000)
			{
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Sending Version"));
				#endif
				request.sendVersion();
				lastTimeTaken = millis();
			}
		}
		reparseFile = false;
		return true;
	}
}

void KoynClass::parseReceivedTx()
{
	if(!file){
		#if defined(ENABLE_DEBUG_MESSAGE)
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
			for(int i=0;i<=7;i++){data[i]=data[i+1];}
			data[7]=file.read();
		}
		while(file.read()!='"');
		bigFile = true;

		uint32_t pos = file.curPosition();
		uint32_t count=0;
		while(file.read()!='"'){count++;}
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(count);
		#endif
		if(count/2>MAX_TRANSACTION_SIZE){Serial.println(F("Raw transaction is too big"));return;}
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Locating space in Ram"));
		#endif
		char stringRawTx[count];
		file.seek(pos);
		for(int i=0;i<count;i++){stringRawTx[i]=file.read();}
		for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
		{
			if(!incomingTx[i].isUsed())
			{
				if(incomingTx[i].setRawTx(stringRawTx,count))
				{
					#if defined(ENABLE_DEBUG_MESSAGE)
					Serial.println(F("Success"));
					#endif
					incomingTx[i].setHeight(0);
					if(isTransactionCallbackAssigned)
					{
						(*transactionCallback)(incomingTx[i]);
					}
					/* reset tx object if user added 0 confirmations*/
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
	{
	/* Just get the value and do nothing */
	}else if(key == "id")
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
						if(++currentClientNo==MAX_CONNECTED_SERVERS){currentClientNo=MAIN_CLIENT;}
						request.getBlockChunks(currentClientNo);
						synchronized =false;
					}else
					{
						#if defined(ENABLE_DEBUG_MESSAGE)
						Serial.println(F("Got All Chunks"));
						#endif
						synchronized = true;
					}
					updateTotalBlockNumb();
				}else
				{
					if(++currentClientNo==MAX_CONNECTED_SERVERS){currentClientNo=MAIN_CLIENT;}
				/* Re-request chunks but from another server */
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
		}else if(value == "blockchain.address.subscribe")
		{
			isMessage |= (0x01<<ADDRESS_SUB);
		}
	}else if(key == "result" && reqData)
	{
			uint32_t methodType = reqData->reqType;
			if(methodType&(uint32_t)(0x01<<VERSION_BIT))
			{
/* Version type data, we should check on the version of the server we are connected to, and if it's less than certain number we should drop this client and connect to another server */
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(value);
				#endif
			}else if(methodType&(uint32_t)(0x01<<TRANSACTION_BIT)){
	/* Hash includes the transaction in block*/
			}else if(methodType&(uint32_t)(0x01<<ADDRESS_SUBS_BIT)){
				/* Here we must check if status file already exists and then grab the status to the Address Object*/
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Address status "));
				#endif
				char status[65];
				int8_t index = getAddressPointerIndex(reqData);
				if(index<0){return;}
				userAddressPointerArray[index]->getStatus(status);
				char addr[36]; /* We must always check type of address before declaring the array to know the length */
				userAddressPointerArray[index]->getEncoded(addr);
				String dirName = "koyn/addresses/" + String(&addr[26]);
				String fileNameUtxo = dirName+"/"+"utxo";
				String fileNameStatus = dirName+"/"+"status";
				if(value!="null"&&!String(status).length())
				{
					request.getAddressHistory(addr);
					request.listUtxo(addr);
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
					#if defined(ENABLE_DEBUG_MESSAGE)
					Serial.println(F("Same status"));
					#endif
				}else if(value!="null")
				{
					/* Status changed request history mempool and check the transaction validity and update to last status*/
					request.getAddressHistory(addr);
					request.listUtxo(addr);
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
					#if defined(ENABLE_DEBUG_MESSAGE)
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
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("request doesn't exist"));
				#endif
			}
	}else if(key == "block_height" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT))))
	{
	/* Still dunno what to do with blockHeight from block header*/
		header.setNull();
		int32_t _height = my_atoll(&value[0]);
		header.height =  _height;
		header.calcPos();
	}else if(key == "version" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT))))
	{
		uint32_t version = my_atoll(&value[0]);
		memcpy(header.completeHeader,&version,4);
	}else if(key == "prev_block_hash" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT))))
	{
		uint8_t container[32];
		hex2bin(container,&value[0],64);
		reverseBin(container,32);
		memcpy(header.completeHeader+4,container,32);
	}else if(key == "merkle_root" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT))))
	{
		uint8_t container[32];
		hex2bin(container,&value[0],64);
		reverseBin(container,32);
		memcpy(header.completeHeader+36,container,32);
	}else if(key == "timestamp" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT))))
	{
		uint32_t timestamp = my_atoll(&value[0]);
		memcpy(header.completeHeader+68,&timestamp,4);
	}else if(key == "bits" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT))))
	{
		uint32_t bits = my_atoll(&value[0]);
		memcpy(header.completeHeader+72,&bits,4);
	}else if(key == "nonce" && ((isMessage&(0x01<<BLOCK_HEAD_SUB))||reqData&&(reqData->reqType&(uint32_t)(0x01<<BLOCK_HEADER_BIT)||reqData->reqType&(uint32_t)(0x01<<HEADERS_SUBS_BIT))))
	{
		uint32_t nonce = my_atoll(&value[0]);
		memcpy(header.completeHeader+76,&nonce,4);
	/* Verify block header and update totalBlockNum*/
		#if defined(ENABLE_DEBUG_MESSAGE)
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
								#if defined(ENABLE_DEBUG_MESSAGE)
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
				/* Check if there are any transactions to be sent as confirmations to user */
				for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
				{
					if(incomingTx[i].isUsed()&&incomingTx[i].inBlock())
					{
						int32_t diff = totalBlockNumb-incomingTx[i].getBlockNumber();
						if(isTransactionCallbackAssigned && incomingTx[i].getConfirmations()>0 && incomingTx[i].getConfirmations()<REMOVE_CONFIRMED_TRANSACTION_AFTER)
						{
							#if defined(ENABLE_DEBUG_MESSAGE)
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
		/* TODO:
				- Message type returns address and status in parameters
				- Support if address is new by checking the first parameter with our address and saving the second parameter
		*/
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		if(!userAddressPointerArray[index]->gotAddress())
		{
			/*
			TODO:
			- Check on address with our address.
			- Future there will be multiple addresses each has its status.
			- Save incoming status in temp_status file first then we wait until the transaction is confirmed in at least 6 blocks
			  then copy it to the original status file.
			*/
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.print(F("Address "));
			Serial.println(value);
			#endif
			userAddressPointerArray[index]->setGotAddress();
		}else
		{
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.println(F("Address status "));
			#endif
			char status[65];
			userAddressPointerArray[index]->getStatus(status);
			char addr[36]; /* We must always check type of address before declaring the array to know the length */
			userAddressPointerArray[index]->getEncoded(addr);
			String dirName = "koyn/addresses/" + String(&addr[26]);
			String fileNameUtxo = dirName+"/"+"utxo";
			String fileNameStatus = dirName+"/"+"status";
			if(value!="null"&&!String(status).length())
			{
				request.getAddressHistory(addr);
				request.listUtxo(addr);
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
				userAddressPointerArray[index]->resetGotAddress();
			}else if(value!="null"&&String(status).length()&&!memcmp(&value[0],status,64))
			{
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.println(F("Same status"));
				#endif
				userAddressPointerArray[index]->resetGotAddress();
			}else if(value!="null")
			{
			/* Status has changed request history again */
				request.getAddressHistory(addr);
				request.listUtxo(addr);
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
				userAddressPointerArray[index]->resetGotAddress();
			}
			isMessage &= ~(1UL << ADDRESS_SUB);
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
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("Got Address Balance"));
		#endif
	}else if(key == "tx_hash" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADRRESS_HISTORY_BIT))))
	{
		addHistory.setTxHash(&value[0]);
	}else if(key == "height" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADRRESS_HISTORY_BIT))))
	{
		/*
		 TODO:
	     - First check if lastTxHash hash is not equal zero to make sure that this is the first time we are getting the history.
	     - If it's not the first time so we got the last state of the history in lastTxHash from the history file.
	     - Then before saving the new incoming history hashes compare the height of the incoming data with lastTxHash height
	       to make sure to save the new ones
	    */
		addHistory.setHeight(my_atoll(&value[0]));
		int32_t heightData = addHistory.getHeight();
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		char addr[36]; /* We must always check type of address before declaring the array to know the length */
		userAddressPointerArray[index]->getEncoded(addr);	
		String dirName = "koyn/addresses/" + String(&addr[26]);
		String fileNameUnverifiedHistory = dirName+"/"+"his_unveri";
		if(heightData >0 && (saveNextHistory|| userAddressPointerArray[index]->lastTxHash.isNull()))
		{
			File historyFile = SD.open(&fileNameUnverifiedHistory[0],FILE_WRITE);
			if(historyFile)
			{
				/*
				 TODO:
			   	 - If his_unveri has data make sure that the new incoming unverified history is not inside this temp file
			   	   otherwise it will be written to the file again and will be verified multiple time. This case will happen
			   	   if rapidly the server sent multiple unverified txHashes and we haven't updated lastTxHash yet.
				*/
				uint8_t hash[32];
				addHistory.getTxHash(hash);
				historyFile.write(hash,32);
				historyFile.write((uint8_t*)&heightData,4);
				// addHistory.setNull();
			}
			historyFile.close();
		}
		if(addHistory==userAddressPointerArray[index]->lastTxHash)
		{
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.println(F("Reached last history save next "));
			#endif
			saveNextHistory= true;
		}
	}else if(key == "fee" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADRRESS_HISTORY_BIT))))
	{
		/* save txHash in transaction object */
		char txHash_str[65];
		txHash_str[64]='\0';
		addHistory.getStringTxHash(txHash_str);
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(txHash_str);
		#endif
		/* Preventing re-requesting raw transactions */
		for(int i=0;i<MAX_TRANSACTION_COUNT;i++)
		{
			if(incomingTx[i].isUsed())
			{
				uint8_t transactionHash[32];
				uint8_t historyHash[32];
				addHistory.getTxHash(historyHash);
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
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(value);
		#endif
		uint32_t seekValue = (my_atoll(&value[0]));
		/* get Merkle root from blkhdrs */
		File blkHdrFile = SD.open("koyn/blkhdrs",FILE_READ);
		blkHdrFile.seek((seekValue*80)+36);
		for(int i=0;i<32;i++){merkleRoot[i]=blkHdrFile.read();}
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.write(merkleRoot,32);
			#endif
	}else if(key == "merkle" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<MERKLE_PROOF))))
	{
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(value);
		#endif
		uint8_t txHash[32];
		hex2bin(txHash,&value[0],64);
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		char addr[36]; /* We must always check type of address before declaring the array to know the length */
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
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(value);
		#endif
		int8_t index = getAddressPointerIndex(reqData);
		if(index<0){return;}
		userAddressPointerArray[index]->lastTxHash.setLeafPos(my_atoll(&value[0]));
	}else if(key == "tx_hash" && (reqData&&(reqData->reqType&(uint32_t)(0x01<<ADDRESS_UTXO_BIT)))){
				int8_t index = getAddressPointerIndex(reqData);
				if(index<0){return;}
				char addr[36]; /* We must always check type of address before declaring the array to know the length */
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
				char addr[36]; /* We must always check type of address before declaring the array to know the length */
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
				char addr[36]; /* We must always check type of address before declaring the array to know the length */
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
				char addr[36]; /* We must always check type of address before declaring the array to know the length */
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
	if(userAddress->address[0]=='1'||userAddress->address[0]=='3'){return MAIN_NET_NOT_SUPPORTED;}
	if(!userAddress->isTracked())
	{
		int i;
		for(i=0;i<MAX_ADDRESSES_TRACKED_COUNT;i++)
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
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.println(F("Tracking"));
			#endif
			File statusFile = SD.open(&fileNameStatus[0],FILE_READ);
			statusFile.read(userAddress->status,64);
		}
		String fileNameHistory=dirName+"/"+"history";
		if(SD.exists(&fileNameHistory[0]))
		{
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.println(F("Last TxHash copied"));
			#endif
			File historyFile = SD.open(&fileNameHistory[0],FILE_READ);
			uint32_t noOfTxHashes = historyFile.size()/36;
			historyFile.seek((noOfTxHashes-1)*36);
			uint8_t lastInFile[36];
			historyFile.read(lastInFile,36);
			userAddress->lastTxHash.copyData(lastInFile);
		}
		String fileNameUtxo = dirName+"/"+"utxo";
		if(SD.exists(&fileNameUtxo[0]))
		{
			SD.remove(&fileNameUtxo[0]);
			userAddress->clearBalance();
		}
		/* Check also if history file exists get the last history and save it to the lastTxHash object */
	}
}

void KoynClass::unTrackAddress(BitcoinAddress * userAddress)
{
	for(int i=0;i<MAX_ADDRESSES_TRACKED_COUNT;i++)
	{
		for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
		{
			if(!strcmp(userAddressPointerArray[i]->address,userAddress->address))
			{
				userAddressPointerArray[i]=NULL;
				return;
			}
		}
	}
}

void KoynClass::unTrackAllAddresses()
{
	for(int i=0;i<MAX_ADDRESSES_TRACKED_COUNT;i++)
	{
		for(int i=0;i<MAX_TRACKED_ADDRESSES_COUNT;i++)
		{
			userAddressPointerArray[i]=NULL;
		}
	}
}

bool KoynClass::isAddressTracked(BitcoinAddress * userAddress)
{
	return userAddress->tracked;
}

WiFiClient * KoynClass::getMainClient()
{
	if(mainClient)
	{
		return mainClient;
	}
}

void KoynClass::setMainClient()
{
	for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
	{
		if(clientsArray[i].connected())
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
		#if defined(ENABLE_DEBUG_MESSAGE)
		Serial.println(F("totalBlockNumb Updated"));
		#endif
		File blkHdrFile = SD.open("koyn/blkhdrs",FILE_WRITE);
		if(blkHdrFile){totalBlockNumb = ((blkHdrFile.size()) / 80)-1;}
	}
}

bool KoynClass::isSynced()
{
	return synchronized;
}

void KoynClass::onNewTransaction(void (* usersFunction)(BitcoinTransaction newTx))
{
	transactionCallback=usersFunction;
	isTransactionCallbackAssigned=true;
}

void KoynClass::onNewBlockHeader(void (*usersFunction)(uint32_t height))
{
	newBlockCallback=usersFunction;
	isNewBlockCallbackAssigned=true;
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

bool KoynClass::checkBlckNumAndValidate(int32_t currentHeaderHeight)
{
	if(!noOfChunksNeeded)
	{
		/* We also make sure that if there's any forks to check up with the current heights */
		int32_t tempForkHeight=0;
		for(int i=0;i<MAX_CONNECTED_SERVERS;i++)
		{
			uint32_t currentForkHeight = forks[i].getLastHeader()->getHeight();
			if(forks[i].exists() && currentForkHeight>tempForkHeight){tempForkHeight=currentForkHeight;}
		}
		if(tempForkHeight)
		{
			/* This means that there's a fork in our system */
			int32_t diff = currentHeaderHeight - tempForkHeight;
			if(currentHeaderHeight>tempForkHeight && diff>1 && diff<=5)
			{
				if(!requestsSent)
				{
					/* If last time was sync and this is a hop, request the prev header and set sync by false
						preventing next same headers to request prev again */
					for(int i=0;i<(currentHeaderHeight-tempForkHeight);i++){request.getBlockHeader(totalBlockNumb+i);}
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
			}
		}
		if(totalBlockNumb<=currentHeaderHeight)
		{
			int32_t diff = currentHeaderHeight - totalBlockNumb; /* Sometimes a problem occurs here as currentBlockOnServers is less than blockNumber*/
			if(diff && diff<=5 && diff>1)
			{
				if(!requestsSent)
				{
					/* If last time was sync and this is a hop, request the prev header and set sync by false
						preventing next same headers to request prev again */
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
				#if defined(ENABLE_DEBUG_MESSAGE)
				Serial.print(F("No of Chunks needed "));
				Serial.println(noOfChunksNeeded);
				#endif
				request.getBlockChunks(MAIN_CLIENT);
				return false;
			}else if(!totalBlockNumb)
			{
				noOfChunksNeeded = currentHeaderHeight/2016;
				#if defined(ENABLE_DEBUG_MESSAGE)
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
		}else if(totalBlockNumb>currentHeaderHeight) /* we should here check all servers tips to be nearly close to each other otherwise a server is lagging and is not update so drop it*/
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
	return totalBlockNumb;
}

uint8_t KoynClass::spend(BitcoinAddress * from, BitcoinAddress * to, uint64_t amount, uint64_t fees, BitcoinAddress * change)
{
	if(from->address[0]=='1'||from->address[0]=='3'){return MAIN_NET_NOT_SUPPORTED;}
	long long totalTransactionAmount = amount+fees;
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

	/* By this check we should calculate confirmed balance from the UTXO's and not by sending a getBalance request to servers,
	 * so in this case we will check if we already got the list of unspent transaction of the address or not.
	 */
	if(from->getConfirmedBalance()<=0)
	{
		return ADDRESS_NO_FUNDS;
	}

	if(totalTransactionAmount > from->getConfirmedBalance())
	{
		return ADDRESS_INSUFFECIENT_BALANCE;
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
	if(SD.exists(&fileNameUtxo[0]))
	{
		File utxoFile = SD.open(&fileNameUtxo[0],FILE_READ);
		if(utxoFile.isOpen())
		{
			uint32_t unspentTransactionCount = utxoFile.size()/48;
			uint64_t amountArray[unspentTransactionCount];
			uint8_t amountArrayIndex[unspentTransactionCount];
			for(int i=0;i<unspentTransactionCount;i++){amountArrayIndex[i]=i;}
			Serial.println("Allocated memory");
			uint32_t i=0;
			while(utxoFile.available())
			{
				utxoFile.seekCur(40);
				utxoFile.read((uint8_t*)&amountArray[i],8);
				i++;
			}
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.println("Got All UTXO's Amount");
			#endif
			for(int l=0;l<unspentTransactionCount;l++)
			{
				for(int j=l+1;j<unspentTransactionCount;j++)
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
			#if defined(ENABLE_DEBUG_MESSAGE)
			Serial.println("Re-ordered UTXO's Amounts");
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
			for(int j=0;j<unspentTransactionCount;j++)
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
			if(accumilativeAmount!=0){changeAmount = accumilativeAmount-totalTransactionAmount;}else{/*Return Error*/}
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
				from->getScriptPubKey(scriptPubKey,25);
				memcpy(ptr,scriptPubKey,25);ptr+=25;
				memcpy(ptr,(uint8_t*)&sequence,4);ptr+=4;
			}
			memcpy(ptr,&outputNo,1);ptr+=1;
			to->getScriptPubKey(scriptPubKey,25);
			memcpy(ptr,(uint8_t *)&amount,8);ptr+=8;
			memcpy(ptr,&scriptPubKeyLen,1);ptr+=1;
			memcpy(ptr,scriptPubKey,25);ptr+=25;
			if(changeAmount)
			{
				change->getScriptPubKey(scriptPubKey,25);
				memcpy(ptr,(uint8_t *)&changeAmount,8);ptr+=8;
				uint8_t scriptPubKeyLen = 0x19;
				memcpy(ptr,&scriptPubKeyLen,1);ptr+=1;
				memcpy(ptr,scriptPubKey,25);ptr+=25;
			}
			memcpy(ptr,(uint8_t *)&lockTime,4);ptr+=4;
			memcpy(ptr,(uint8_t *)&hashCode,4);
			/* Now we got the message to be hashed */
		    File transactionFile = SD.open(&fileNameTx[0],FILE_WRITE);
		    if(transactionFile.isOpen())
		    {
		    	transactionFile.write((uint8_t *)&version,4);
		    	transactionFile.write(i);
				for(int j=0;j<i;j++)
				{
					transactionHash(hash,preImage,preImageSize,j);
					do {
				        uECC_sign(privKey, hash, 32, signature, from->curve);
				        yield();
				    } while ((signature[0] & 0x80) || (signature[32] & 0x80));
				    uint8_t derSignatureLen = 0x47;  /* We already took care of signed MSB signature so it will be always 71 byte */
				    uint8_t derSignature[derSignatureLen];
				    derSignature[0]= ASN1_BMPSTRING; /* ASN1 start byte */
				    derSignature[1]= 0x44;  /* Signature with ASN1 identifiers */
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
			    	uint8_t scriptSigLen = derSignatureLen+33+2; /* derSigLen+derSig+compPubLen+compPub*/
			    	transactionFile.write(&scriptSigLen,1);
			    	transactionFile.write(&derSignatureLen,1);
			    	transactionFile.write(derSignature,derSignatureLen);
			    	transactionFile.write(0x21);
			    	uint8_t compPubKey[33];
			    	from->getCompressedPublicKey(compPubKey);
			    	transactionFile.write(compPubKey,33);
			    	transactionFile.write((uint8_t*)&sequence,4);
				}

				transactionFile.write(outputNo);
		    	transactionFile.write((uint8_t*)&amount,8);
		    	transactionFile.write(0x19);
		    	to->getScriptPubKey(scriptPubKey,25);
		    	transactionFile.write(scriptPubKey,25);
		    	if(changeAmount)
		    	{
		    		transactionFile.write((uint8_t*)&changeAmount,8);
			    	transactionFile.write(0x19);
			    	change->getScriptPubKey(scriptPubKey,25);
			    	transactionFile.write(scriptPubKey,25);
		    	}
	    		transactionFile.write((uint8_t *)&lockTime,4);
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
			    	/* Broadcast transaction */
				    // request.broadcastTransaction(&transactionFile);
				    transactionFile.close();
				    // SD.remove(&fileNameFinalTx[0]);
			    }
		    }
		}
	    utxoFile.close();
	}
}

uint8_t KoynClass::spend(BitcoinAddress * from, BitcoinAddress * to, uint64_t amount, uint64_t fees)
{
	spend(from,to,amount,fees,from);
}


void KoynClass::delay(unsigned long time)
{
	unsigned long now=millis();
	while((millis()<(now+time)))
	{
		run();
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
				#if defined(ENABLE_DEBUG_MESSAGE)
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
		if(!strcmp(userAddressPointerArray[i]->address,(char *)reqDataPointer->dataString))
		{
			return i;
		}
	}
	return -1;
}

KoynClass Koyn;
