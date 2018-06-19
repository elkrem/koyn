/*

Koyn Library Bare Minimum Example

This example shows the bare minimum functions required by the library.

It can be used as a boilerplate for projects.

Note: You need to connect to your WiFi network at the beginning of
the setup() function before calling Koyn.begin()

*/

/* Include Koyn library. */
#include "Koyn.h"

void setup()
{
  /*********************************************************/
  /*                        REQUIRED                       */
  /* Connect to your WiFi using ESPWiFi library functions. */
  /*********************************************************/

  /* Initialize the library and the microSD card. */
  Koyn.begin();
}

void loop()
{
  /* Performs the library routines (syncing, validation, tracking, ..etc). */
  Koyn.run();
}
