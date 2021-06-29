/*
    Utility functions used for ElGamal encryption
    of data.
*/

#ifndef BITCOIN_ENCRYPTIONUTILS_H
#define BITCOIN_ENCRYPTIONUTILS_H

#include <string>

class CWallet;
class CBitcoinAddress;

bool validatePublicKeyFromHexStr(std::string pubKeyHex);
std::string addressFromPublicKey(std::string pubKeyHex);
bool encryptString(std::string data, std::string pubKeyHex, std::string &resultHex);
bool decryptData(std::string txData, CBitcoinAddress address, const CWallet *wallet, std::string &decryptedData);

#endif