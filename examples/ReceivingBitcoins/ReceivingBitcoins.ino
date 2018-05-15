/* Include ESPWiFi library. */
#include <ESP8266WiFi.h>
/* Include Koyn library. */
#include "Koyn.h"

/* Assign a light bulb to pin13. */
#define LIGHT_BULB 13
/* Renting value of light bulb. */
#define PAYMENT_VALUE 1000000
/* Create an address giving it the encoded key address ex."mqBPVCaTaJDVGruNTkTD3CKTVMYqmVow6R" */
BitcoinAddress myAddress("ENCODED_KEY_ADDRESS",KEY_ENCODED);
/* Create friend address. */
BitcoinAddress myFriendAddress("ENCODED_KEY_ADDRESS",KEY_ENCODED);

void setup() {
  /* Make light bulb pin output. */
  pinMode(LIGHT_BULB, OUTPUT);
  /* Connect to your WiFi using ESPWiFi library functions. */
  // CONNECT TO WIFI
  /* Start Koyn library. */
  Koyn.begin();
  /* Subscribe to a new incomming transaction. */ 
  Koyn.onNewTransaction(&paymentCallback);
}

void loop() {
  /* Check first if the board is synced with the network. */
  if(Koyn.isSynced()){
    /* Track address. */
    Koyn.trackAddress(&myAddress);   
  }
  /* Run the library and check for changes. */
  Koyn.run();
}

/* Callback function to take a physical action when a transaction is received. */
void paymentCallback(BitcoinTransaction tx){
  /* Loop over Inputs and check my friend's address. */
  for(int i=0;i<tx.getInputsCount();i++)
  {
    /* Creating an empty address object. */
    BitcoinAddress from;
    /* Retreiving address from transaction. */
    tx.getInput(i, &from);
    if(from == myFriendAddress){
      /* Loop over my address and check the payment value. */
      for(int j=0;j<tx.getOutputsCount();j++)
      {
        /* Create an empty address object. */
        BitcoinAddress to;
        /* Retreiving address from transaction. */
        tx.getOutput(j, &to);
        if(to == myAddress){
          /* Check payment value and get amount using address index. */
          if(tx.getOutputAmount(j) >= PAYMENT_VALUE){
              /* Turn on the light bulb. */
              digitalWrite(LIGHT_BULB, HIGH);
              /* Delay for a second. */
              Koyn.delay(1000);
              /* Turn off the light bulb. */
              digitalWrite(LIGHT_BULB, LOW); 
          }
        break;          
        }
      }
      break;
    }
  }
}
