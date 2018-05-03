#ifndef BitcoinFork_h
#define BitcoinFork_h


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
