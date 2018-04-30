#ifndef BitcoinFork_h
#define BitcoinFork_h

#define LONGEST_CHAIN 3


class BitcoinFork{
public:
	BitcoinFork();
	void setNull();
	void setParentHeader(BitcoinHeader *);
	void setLastHeader(BitcoinHeader *);
	BitcoinHeader * getParentHeader();
	BitcoinHeader * getLastHeader();
	bool exists();
	bool gotParentHeader();
private:
	BitcoinHeader parentHeader;
	BitcoinHeader lastHeader;
};



#endif
