#ifndef BitcoinAddress_h
#define BitcoinAddress_h

extern "C" {

  inline int RNG(uint8_t *dest, unsigned size) {
    // Use the least-significant bits from the ADC for an unconnected pin (or connected to a source of
    // random noise). This can take a long time to generate random data if the result of analogRead(0)
    // doesn't change very frequently.
    while (size) {
      uint8_t val = 0;
      for (unsigned i = 0; i < 8; ++i) {
        int init = analogRead(A0);
        int count = 0;
        while (analogRead(A0) == init) {
          ++count;
        }

        if (count == 0) {
          val = (val << 1) | (init & 0x01);
        } else {
          val = (val << 1) | (count & 0x01);
        }
      }
      *dest = val;
      ++dest;
      --size;
    }
    // NOTE: it would be a good idea to hash the resulting random data using SHA-256 or similar.
    return 1;
  }

}  // extern "C"

class BitcoinAddress
{
public:
	BitcoinAddress(bool=false);
	BitcoinAddress(const char *,uint8_t);
	BitcoinAddress(uint8_t *,uint8_t);
	uint8_t getPrivateKey(uint8_t *);
	uint8_t getPrivateKey(char *);
	uint8_t getPublicKey(uint8_t *);
	uint8_t getPublicKey(char *);
	uint8_t getWif(char *);
	uint8_t getEncoded(char *);
	bool isTracked();
	uint64_t getBalance();
	uint64_t getConfirmedBalance();
	uint64_t getUnconfirmedBalance();
	uint8_t getScript(uint8_t *,uint8_t);
	uint8_t getScriptHash(char *,uint8_t);
	uint8_t getAddressType();
	bool operator==(BitcoinAddress& );

private:
	void init();
	uint8_t getStatus(char *);
	bool gotAddress();
	void setTracked();
	void resetTracked();
	void setConfirmedBalance(uint32_t);
	void setUnconfirmedBalance(uint32_t);
	void setGotAddress();
	void setAddressStatus(char *);
	void resetGotAddress();
	void calculateAddress(uint8_t );
	void clearBalance();
	uint8_t privateKey[32];
	uint8_t compPubKey[33];
	char address[36];
	char status[65];
	bool tracked;
	bool gotAddressRef;
	bool isShellAddress;
	uint64_t confirmedBalance;
	uint64_t unconfirmedBalance;
	AddressHistory addressHistory;
	const struct uECC_Curve_t * curve;
	friend class KoynClass;
	friend class BitcoinTransaction;
};


#endif
