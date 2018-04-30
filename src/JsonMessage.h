#ifndef JsonMessage_h
#define JsonMessage_h

#include <aJson/aJson.h>


class JsonMessage {

public:
	JsonMessage();
	char * createJsonMessageString(int id, const char * methodMessage, const char * data);
	char * createJsonMessage(int id, const char * methodMessage, int * data);
	char * createJsonMessage(int id, const char * methodMessage,const char * stringData, int * data);
private:
	aJsonObject * versionMessage;
	char * json;
	friend class Koyn;
	friend class ElectrumRequests;
};

#endif
