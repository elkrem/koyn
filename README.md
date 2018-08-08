# Koyn: Bitcoin Library For Arduino [![Build Status](https://travis-ci.org/elkrem/koyn.svg?branch=master)](https://travis-ci.org/elkrem/koyn)

## Overview

A fully decentralized and trustless Bitcoin light (aka SPV) client implementation for the Arduino platform based on Electrum protocol.

With its simple and easy-to-use API, it allows you to interface Arduino boards to the Bitcoin network. The sky is the limit, you can either do a physical action when a transaction is made or send transactions based on hardware events.

## Readiness

The library should be working but it is still rough around the edges. It is considered to be a **work-in-progress**. It is by all means far from being production ready. It hasn't been thoroughly tested or gone through security reviews.

Currently, it can only works with Bitcoin testnet **(no mainnet support yet)**. Be advised not to use real Bitcoins. We are not responsible for any loss of funds so use it at your own risk.

Arduino boards support is initially limited as well, please check below for more info.

Also, the public interface hasn't been finalized yet, it can be changed a bit during the initial releases.

## Light Clients

Light Bitcoin clients (nodes) doesn't store the full transaction history (blockchain) of the network (unlike full clients). They only download the headers (80 bytes every ~10 minutes) of the created blocks to be able to verify that a particular transaction was indeed included and accepted by the network. The concept is often referred to by the term *SPV* (Simplified Payment Verification).

There are two popular protocols for SPV clients. Bitcoin Core and Electrum, they pretty much have the same concept. The core protocol works with raw bytes and Electrum is JSON based. This library supports only Electrum protocol for now, thus it only works with Electrum servers.

## Hardware Requirements

The library requires an ESP8266 or ESP32 based board connected with an SPI microSD card module to work.

The microSD card needs to be a single partition, FAT formatted, fast (class 10) and has at least 1GB of space. It doesn't need to be empty as we create a new directory on its root named `koyn`.

## Boards Support

These boards have been tested and are known to work:

**ESP8266**
- Adafruit Huzzah ESP8266
- Wemos D1
- SparkFun ESP8266 Thing
- NodeMCU

**ESP32**
- DOIT ESP32 DEVKIT V1
- ESPDUINO-32
- SparkFun ESP32 Thing

These microSD card boards has been tested and known to work:

- Wemos Micro SD Card Shield
- SparkFun microSD Shield

## Installation

Make sure you have the latest version of [Arduino IDE](https://www.arduino.cc/en/Main/Software) with either [ESP8266](https://github.com/esp8266/Arduino#installing-with-boards-manager) or [ESP32](https://github.com/espressif/arduino-esp32#installation-instructions) cores installed.


**Using Library Manager (Recommended)**

Open the Arduino IDE, go to Sketch > Include Library > Manage Libraries.

And then, search for our library in the list, pick the latest version and click install.

**Manual Method**

Download the latest version of the library from the [releases page](https://github.com/Elkrem/Koyn/releases). Unzip it in a new directory named *Koyn* inside your Arduino's libraries directory.

**PlatformIO**

If you are using PlatformIO, the library can be installed from their library manager as well, just search for `Koyn`.

## Blockchain Headers Syncing

You can choose to sync and validate the headers on the board directly starting from the genesis block, but it can take from tens of minutes to hours depending on your sketch and network conditions.

So it is advised to preload the microSD card with the latest headers on a host machine before hand using one of the methods below, then rename it to `blkhdrs` and place it in `/koyn` directory on the microSD.

**Using Our Tool (Recommended)**

We developed a command line tool based on [BitcoinJ](https://github.com/bitcoinj/bitcoinj) library that validates and downloads the Bitcoin blockchain headers to a file on your local machine.

First make sure you have a [Java Runtime Environment](https://java.com/en/download/) installed on your host machine and accessible from your `PATH`. Then download the latest [release](https://github.com/elkrem/koyn-sync/releases) of our tool. Then run the executable or call `java -jar koyn-sync-VERSION.jar` from a command line terminal. (Make sure you are connected to the Internet, and the working directory is writable)

The tool will download the headers to your working directory and create either `./koyn/mainnet/blkhdrs` or `./koyn/testnet/blkhdrs` depending on the supplied [flags](https://github.com/elkrem/koyn-sync#optional-flags). (Logging messages will show its progress)

**From Electrum Website**

If you trust Electrum.org, you can download the headers directly from their website and the library will sync the rest of the headers when started.

- Bitcoin [Testnet Headers](https://download.electrum.org/testnet_headers) (till August, 2017)
- Bitcoin [Mainnet Headers](https://download.electrum.org/blockchain_headers) (till August, 2017)

**From Electrum App**

Install Electrum client on your machine if you don't have it, run it, let it sync.

You may need to supply `--testnet` parameter before running the Electrum executable if you need to sync the **testnet** headers.

On Windows, the headers file is located in `%APPDATA%\Electrum\blockchain_headers` or `%APPDATA%\Electrum\testnet\blockchain_headers` if you are using the testnet. On Mac and Linux, the headers file is located in `~/.electrum/blockchain_headers` or `~/.electrum/testnet/blockchain_headers` if you are using the testnet.

Note: Electrum versions >=3.1.0 **does not** sync the headers from the genesis block and rely on the concept of checkpoints. So to sync using Electrum please use version below 3.1.0.

## Usage

We tried to make our API as simple as possible, here are some samples.

**Bare Minimum**

```
#include "Koyn.h"

void setup() {
  Koyn.begin();
}

void loop() {
  Koyn.run();
}
```

**Transaction Signing and Broadcasting**

```
BitcoinAddress myAddress("PRIVATE_KEY", KEY_PRIVATE);
Koyn.trackAddress(&myAddress);
BitcoinAddress myFriendAddress("BITCOIN_ADDRESS", ADDRESS_ENCODED);
Koyn.spend(&myAddress, &myFriendAddress, 100000000, 100000);
```

**Transactions Tracking**

```
void paymentCallback(BitcoinTransaction tx){

}
```

```
Koyn.onNewTransaction(&paymentCallback);
if(Koyn.isSynced()){
    Koyn.trackAddress(&myAddress);
}
```

**Complete Example: Flash a light bulb with Bitcoin**

```
#include <Koyn.h>

#define LIGHT_BULB 13
#define PAYMENT_VALUE 1000000

BitcoinAddress myAddress("PRIVATE_KEY", KEY_PRIVATE);
BitcoinAddress myFriendAddress("BITCOIN_ADDRESS", ADDRESS_ENCODED);

void setup() {
  pinMode(LIGHT_BULB, OUTPUT);
  ..
  .. Connect to WiFi
  ..
  Koyn.begin(false);
  Koyn.onNewTransaction(&paymentCallback);
}

void loop() {
  if(Koyn.isSynced()){
    Koyn.trackAddress(&myAddress);
  }
  Koyn.run();
}

void paymentCallback(BitcoinTransaction tx){
  for(int i=0;i<tx.getInputsCount();i++)
  {
    BitcoinAddress from;
    tx.getInput(i, &from);
    if(from == myFriendAddress){
      for(int j=0;j<tx.getOutputsCount();j++)
      {
        BitcoinAddress to;
        tx.getOutput(j, &to);
        if(to == myAddress){
          if(tx.getOutputAmount(j) == PAYMENT_VALUE){
              digitalWrite(LIGHT_BULB, HIGH);
              Koyn.delay(1000);
              digitalWrite(LIGHT_BULB, LOW);
          }
          else{
              Koyn.spend(&myAddress, &myFriendAddress, tx.getOutputAmount(j), 100000);
          }
        break;          
        }
      }
      break;
    }
  }
}
```

## Supported Networks

- [x] Bitcoin Testnet
- [ ] Bitcoin Mainnet
- [ ] Bitcoin Cash

## Examples

Examples that cover most of our APIs can be found [here](examples).

<!--## microSD Card Structure-->

## Documentation

Documentation for all of the usable public methods, classes and configuration can be found in our [Wiki](https://github.com/elkrem/koyn/wiki/Docs).

## Configuration And Defaults

Most of the hardcoded library defaults and configuration parameters can be edited [here](src/Config.h).

Debug messages can be enabled on the default Serial interface through the same file as well by uncommenting `#define ENABLE_DEBUG_MESSAGES` line.

## Status and Roadmap

- [x] Connects to multiple Electrum servers.
- [x] Validates the headers one by one starting from the genesis block.
- [x] Downloads (syncs) the latest blockchain headers and validates them.
- [x] Track transactions to user's addresses.
- [x] Validates incoming transactions and making sure they were accepted by the network.
- [x] Sign and send transactions (from user's addresses) to the network.
- [ ] Support headers checkpoints.
- [ ] Support Bitcoin mainnet.
- [ ] Support additional WiFi connected Arduino boards.
- [ ] Support custom transactions.
- [ ] Support P2SH transactions.
- [ ] Support SegWit transactions and addresses.
- [ ] Support hierarchical deterministic wallets (BIP32).
- [ ] Support mnemonic seed wallets (BIP39).
- [ ] Support logical hierarchy for deterministic wallets (BIP44).
- [ ] Add APIs to ease the workflow of payment channels.
- [ ] Add APIs to ease the prototyping of hardware wallets.

## Notes and Caveats

- Only P2PKH transactions and addresses are supported.
- Only non-SegWit transactions and addresses are supported.
- Only addresses generated from compressed public keys are supported.
- The library connects to non-TLS Electrum servers only.
- The library connects to a hardcoded list of bootnodes only for now, the `server.peers.subscribe` call hasn't been implemented yet.
- The library works with Bitcoin testnet only for now.
- The library does cache private keys in memory.
- The library doesn't support true random generators yet (generated private keys and addresses rely on analog pin #0 noise).
- Due to the limitation of hardware, there are some constraints on transaction sizes, tracked addresses number, transactions number kept in memory and others (Their defaults can be overwritten in `Config.h` file)

## Acquiring Testnet Bitcoins

Testnet bitcoins can be easily acquired using online faucets like [this](https://testnet.manu.backend.hamburg/faucet) and [this](https://testnet.coinfaucet.eu/en/).

## Changelog

To see what has changed in recent versions of Koyn library, see the [change log](CHANGELOG.md).

## Contributing

We welcome all contributions from our community. If you've find a bug or can improve the code or want to add a new feature, please follow our [contributions guidelines](CONTRIBUTING.md).

## Learn More

Be the first to know about our future blockchain connected hardware products by subscribing to our mailing list at [Elkrem.io](https://elkrem.io).

## Attributions and Credits

- [aJson](https://github.com/interactive-matter/aJson)
- [Base58](https://github.com/HsuBokai/base58)
- [Json Streaming Parser](https://github.com/squix78/json-streaming-parser)
- [Ripemd160](https://github.com/ARMmbed/mbedtls)
- [SdFat](https://github.com/greiman/SdFat)
- [Sha256](https://github.com/Cathedrow/Cryptosuite)
- [uECC](https://github.com/kmackay/micro-ecc)
- [ElectrumX](https://github.com/kyuupichan/electrumx)
- [Electrum Client](https://github.com/spesmilo/electrum)

## License and Copyright

```
This code is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General
Public License version 3 only, as published by the Free Software Foundation.

This code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
version 3 for more details (a copy is included in the LICENSE.md file that accompanied this code).
```
