/* Include ESPWiFi library. */
#include <ESP8266WiFi.h>
/* Include Koyn library. */
#include "Koyn.h"

/* Assign a paying button to pin16 on ESP8266. */
#define PAY_BUTTON 16
/* Payment value to be spent each time you push the button. */
#define PAYMENT_VALUE 1000000
/* Fee value to be included in transaction. */
#define FEE_VALUE 300
/* Create an address given the address's private key as hexString ex."246B9E613AFA65908187183402B8A23C8CFD6A0ABE5F60F82FF464E62A71A449" */
//BitcoinAddress myAddress("PRIVATE_KEY", KEY_PRIVATE);
/* Create an address given the address's WIF private key as string ex."cUAWx86EW3Wmk651aBQ58UxeNMow27hzz1cNU2p2r1RR31NqyP9a" */
BitcoinAddress myAddress("WIF_FORMAT_PRIVATE_KEY", KEY_WIF);
/* Create your friend's address to pay for him. */
BitcoinAddress myFriendAddress("ENCODED_KEY_ADDRESS", ADDRESS_ENCODED);
/* A flag saves the last state of the button. */
bool buttonLastState = false;

void setup() {
  /* Make pay button pin input. */
  pinMode(PAY_BUTTON, INPUT);
  /* Connect to your WiFi using ESPWiFi library functions. */
  // CONNECT TO WIFI
  /* Start Koyn library. */
  Koyn.begin();
}

void loop() {
  /* Check first if the board is synced with the network. */
  if (Koyn.isSynced()) {
    /* Track address. */
    Koyn.trackAddress(&myAddress);
  }
  /* Check if spending address is tracked and button is pressed. */
  if (Koyn.isAddressTracked(&myAddress) && digitalRead(PAY_BUTTON) == HIGH)
  {
    /* If the last state of button was pressed don't spend again. */
    if (!buttonLastState)
    {
      /* Pay 0.01BTC to my friend address and if there's a change automatically return it to myAddress. */
      Koyn.spend(&myAddress, &myFriendAddress, PAYMENT_VALUE, FEE_VALUE);
    }
    /* Set flag to true. */
    buttonLastState = true;
    /* Delay for 2 seconds. */
    Koyn.delay(2000);
  } else
  {
    /* Reset flag if no action taken. */
    buttonLastState = false;
  }
  /* Run the library and check for changes. */
  Koyn.run();
}