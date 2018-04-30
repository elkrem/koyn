
#include "JsonListener.h"
#include "Koyn.h"

void JsonListener::whitespace(char c) {
  // Serial.println("whitespace");
}

void JsonListener::startDocument() {
////  Serial.println("start document");
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
  // Serial.println("end array. ");
	levelArray--;
	if(levelArray == 1)
	{
		Koyn.firstLevel= true;
	}
}

void JsonListener::endObject() {
  // Serial.println("end object. ");
	endOfObject = true;
	levelObject--;
}

void JsonListener::endDocument() {
  // Serial.println("end document. ");
	endOfDocument = true;
}

void JsonListener::startArray() {
   // Serial.println("start array. ");
	levelArray++;
}

void JsonListener::startObject() {
	endOfObject = false;
	levelObject++;
}
