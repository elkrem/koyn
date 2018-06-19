/*

Spending Bitcoins Example

By using this example, you can pay 0.01 Bitcoins to a friend when
a button placed on pin 16 is pressed.

Note: You need to connect to your WiFi network at the beginning of
the setup() function before calling Koyn.begin()

*/

/* Include Koyn library. */
#include "Koyn.h"

/* Assign a paying button to pin 16. */
#define PAY_BUTTON 16
/* Payment value to be spent each time you push the button (in satoshis). (0.01 BTC) . */
#define PAYMENT_VALUE 1000000
/* Fee value to be included in transaction. (in satoshis) */
#define FEE_VALUE 300
/* Create a bitcoin address given the address's WIF private key ex."cUAWx86EW3Wmk651aBQ58UxeNMow27hzz1cNU2p2r1RR31NqyP9a" */
BitcoinAddress myAddress("WIF_FORMAT_PRIVATE_KEY", KEY_WIF);
/* Create a friend's Bitcoin address with the encoded address. */
BitcoinAddress myFriendAddress("ENCODED_KEY_ADDRESS", ADDRESS_ENCODED);
/* A flag saves the last state of the button. */
bool buttonLastState = false;

void setup()
{
  /*********************************************************/
  /*                        REQUIRED                       */
  /* Connect to your WiFi using ESPWiFi library functions. */
  /*********************************************************/

  /* Initialize the library and the microSD card. */
  Koyn.begin();
  /* Set the pay button pin mode to input. */
  pinMode(PAY_BUTTON, INPUT);
}

void loop()
{
  /* Check if the board is synced with the network. */
  if (Koyn.isSynced())
  {
    /* Track my address to be able to spend from it. */
    Koyn.trackAddress(&myAddress);
  }
  /* Check if the spending address is tracked and the button is pressed. */
  if (Koyn.isAddressTracked(&myAddress) && digitalRead(PAY_BUTTON) == HIGH)
  {
    /* If the last state of button was pressed don't spend again. */
    if (!buttonLastState)
    {
      /* Pay 0.01BTC to my friend 's address. */
      Koyn.spend(&myAddress, &myFriendAddress, PAYMENT_VALUE, FEE_VALUE);
    }
    /* Set flag to true. */
    buttonLastState = true;
    /* Delay for 2 seconds. */
    Koyn.delay(2000);
  }
  else
  {
    /* Reset flag if no action taken. */
    buttonLastState = false;
  }
  /* Performs the library routines (syncing, validation, tracking, ..etc). */
  Koyn.run();
}
