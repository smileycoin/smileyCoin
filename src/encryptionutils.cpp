#include "encryptionutils.h"
#include "wallet.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "jeeq.h"

/*
    Validate a public key.
*/
bool validatePublicKeyFromHexStr(std::string pubKeyHex) {
    CPubKey pubKey(ParseHex(pubKeyHex));
    return pubKey.IsFullyValid();
}

/*
    Returns the Smileycoin address derived from a public key
*/
std::string addressFromPublicKey(std::string pubKeyHex) {
    CPubKey pubKey(ParseHex(pubKeyHex));
    CBitcoinAddress address(pubKey.GetID());
    return CBitcoinAddress(pubKey.GetID()).ToString();
}

/*
    Encrypts a string using a public key. Returns the data as
    hex encoded string
*/
bool encryptString(std::string data, std::string pubKeyHex, std::string &resultHex) {
    CPubKey pubKey(ParseHex(pubKeyHex));
    if (!pubKey.IsFullyValid()) {
        return false;
    }

    std::vector<uint8_t> encryptedData = Jeeq::EncryptMessage(pubKey, data);
    if (encryptedData.size() == 0)
        return false;

    resultHex = HexStr(encryptedData);
    return true;
}

bool decryptData(std::string txData, CBitcoinAddress address, const CWallet *wallet, std::string &decryptedData) {
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        return false;

    CKey vchSecret;
    if (!wallet->GetKey(keyID, vchSecret))
        return false;

    if (!IsHex(txData))
        return false;

    std::string decryptedString = Jeeq::DecryptMessage(vchSecret, ParseHex(txData));

    if (decryptedString.length() == 0)
        return false;

    decryptedData = HexStr(decryptedString);
    return true;
}