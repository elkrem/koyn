
#include "JsonListener.h"
#include "Koyn.h"

void JsonListener::whitespace(char c) {
  // Serial.println(F("whitespace"));
}

void JsonListener::startDocument() {
////  Serial.println(F("start document"));
//  started = true;
	endOfDocument = false;
}

void JsonListener::key(String key) {
	// Serial.println(key);
	_key=key;
}

void JsonListener::value(String value) {
	Koyn.processInput(_key,value);
}

void JsonListener::endArray() {
  // Serial.println(F("end array. "));
	levelArray--;
	if(levelArray == 1)
	{
		Koyn.firstLevel= true;
	}
}

void JsonListener::endObject() {
  // Serial.println(F("end object. "));
	endOfObject = true;
	levelObject--;
}

void JsonListener::endDocument() {
  // Serial.println(F("end document. "));
	endOfDocument = true;
}

void JsonListener::startArray() {
   // Serial.println(F("start array. "));
	levelArray++;
}

void JsonListener::startObject() {
	endOfObject = false;
	levelObject++;
}
