/*

Receiving Bitcoins Example

By using this example, you can turn on a light bulb relay connected
to pin 16 when 0.01 Bitcoins is sent to a specific address.

Note: You need to connect to your WiFi network at the beginning of
the setup() function before calling Koyn.begin()

*/

/* Include Koyn library. */
#include "Koyn.h"

/* Assign a light bulb to pin 16. */
#define LIGHT_BULB_RELAY 16
/* Required payment value for the light bulb in satoshis. (0.01 BTC) */
#define PAYMENT_VALUE 1000000
/* Create a Bitcoin address with the encoded address ex."mqBPVCaTaJDVGruNTkTD3CKTVMYqmVow6R" */
BitcoinAddress myAddress("ENCODED_KEY_ADDRESS", ADDRESS_ENCODED);

void setup()
{
  /*********************************************************/
  /*                        REQUIRED                       */
  /* Connect to your WiFi using ESPWiFi library functions. */
  /*********************************************************/

  /* Initialize the library and the microSD card. */
  Koyn.begin();
  /* Subscribe to new incoming transactions with a callback. */
  Koyn.onNewTransaction(&paymentCallback);
  /* Set light bulb relay pin mode to output. */
  pinMode(LIGHT_BULB_RELAY, OUTPUT);
}

void loop()
{
  /* Check if the board is synced with the network. */
  if (Koyn.isSynced())
  {
    /* Track my address to receive transactions callbacks. */
    Koyn.trackAddress(&myAddress);
  }
  /* Performs the library routines (syncing, validation, tracking, ..etc). */
  Koyn.run();
}

/* This callback function will be called when a BTC transaction is received. */
void paymentCallback(BitcoinTransaction tx)
{
  unsigned long totalAmountSentToMe = 0;
  /* Loop on all transaction outputs to sum up all received bitcoins. */
  for (int i = 0; i < tx.getOutputsCount(); i++)
  {
    /* Create an empty address. */
    BitcoinAddress to;
    /* Retrieving to address from transaction. */
    tx.getOutput(i, &to);
    /* Check if the bitcoins were sent to me. */
    if (to == myAddress)
    {
      /* Sum up the amount. */
      totalAmountSentToMe += tx.getOutputAmount(i);
    }
  }

  /* Check if the total amount sent to me is enough. */
  if (totalAmountSentToMe >= PAYMENT_VALUE)
  {
    /* Turn on the light bulb. */
    digitalWrite(LIGHT_BULB_RELAY, HIGH);
    /* Delay for a second. */
    Koyn.delay(1000);
    /* Turn off the light bulb. */
    digitalWrite(LIGHT_BULB_RELAY, LOW);
  }
}
