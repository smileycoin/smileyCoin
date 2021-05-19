// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "netbase.h"
#include "rpcserver.h"
#include "util.h"
#include "richlistdb.h"
#include "servicelistdb.h"
#include "serviceitemlistdb.h"
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
#ifdef ENABLE_WALLET
#include "wallet.h"
#include "walletdb.h"
#endif

#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;


Value getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,           (numeric) the wallet build version\n"
            "  \"build_date\": xxxxx,        (string) the wallet build date\n"
            "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total smileycoin balance of the wallet\n"
            "  \"blocks\": xxxxxx,           (numeric) the current number of blocks processed in the server\n"
            "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
            "  \"connections\": xxxxx,       (numeric) the number of connections\n"
            "  \"proxy\": \"host:port\",     (string, optional) the proxy used by the server\n"
            "  \"difficulty\": xxxxxx,       (numeric) the current difficulty\n"
            "  \"testnet\": true|false,      (boolean) if the server is using testnet or not\n"
            "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds since GMT epoch) of the oldest pre-generated key in the key pool\n"
            "  \"keypoolsize\": xxxx,        (numeric) how many new keys are pre-generated\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set in smly/kb\n"
            "  \"relayfee\": x.xxxx,         (numeric) minimum relay fee for non-free transactions in smly/kb\n"
            "  \"errors\": \"...\"           (string) any error messages\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);
    CScript richpubkey;

    Object obj;
    obj.push_back(Pair("version",         (int)CLIENT_VERSION));
    obj.push_back(Pair("build_date",      CLIENT_DATE));
    obj.push_back(Pair("protocolversion", (int)PROTOCOL_VERSION));
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
    }
#endif
    obj.push_back(Pair("blocks",             (int)chainActive.Height()));
    obj.push_back(Pair("timeoffset",         GetTimeOffset()));
    obj.push_back(Pair("connections",        (int)vNodes.size()));
    obj.push_back(Pair("proxy",              (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("pow_algo_id",        miningAlgo));
    obj.push_back(Pair("pow_algo",           GetAlgoName(miningAlgo)));
    obj.push_back(Pair("difficulty",         (double)GetDifficulty(NULL, miningAlgo)));
    obj.push_back(Pair("difficulty_sha256d", (double)GetDifficulty(NULL, ALGO_SHA256D)));
    obj.push_back(Pair("difficulty_scrypt",  (double)GetDifficulty(NULL, ALGO_SCRYPT)));
    obj.push_back(Pair("difficulty_groestl", (double)GetDifficulty(NULL, ALGO_GROESTL)));
    obj.push_back(Pair("difficulty_skein",   (double)GetDifficulty(NULL, ALGO_SKEIN)));
    obj.push_back(Pair("difficulty_qubit",   (double)GetDifficulty(NULL, ALGO_QUBIT)));
    obj.push_back(Pair("testnet",            TestNet()));
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.push_back(Pair("keypoololdest", pwalletMain->GetOldestKeyPoolTime()));
        obj.push_back(Pair("keypoolsize",   (int)pwalletMain->GetKeyPoolSize()));
    }
    if (pwalletMain && pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", nWalletUnlockTime));
    obj.push_back(Pair("paytxfee",      ValueFromAmount(nTransactionFee)));
#endif
    obj.push_back(Pair("relayfee",      ValueFromAmount(CTransaction::nMinRelayTxFee)));
    if(RichList.NextRichScriptPubKey(richpubkey)) {
        CTxDestination des;
        ExtractDestination(richpubkey, des);
        obj.push_back(Pair("oldest_rich_address", CBitcoinAddress(des).ToString()));
    }
    else obj.push_back(Pair("oldest_rich_address", ""));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    return obj;
}


Value getrichaddresses(const Array& params, bool fHelp)
{

    if (fHelp || params.size() != 0)
        throw runtime_error("getrichaddresses\n"
                            "Returns all rich addresses, ordered by height.\n"
                            );
    Object obj;
    std::multiset< std::pair< CScript, std::pair<int64_t, int> >, RichOrderCompare > retset;
    RichList.GetRichAddresses(retset);
    for(std::set< std::pair< CScript, std::pair<int64_t, int> >, RichOrderCompare >::const_iterator it = retset.begin(); it!=retset.end(); it++ )
    {
        CTxDestination des;
        ExtractDestination(it->first, des);
        obj.push_back(Pair(CBitcoinAddress(des).ToString(), it -> second.second));
    }
    return obj;
}

Value adddex(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
                "adddex \"servicename\" \"dexaddress\" \"dexdescription\" \n"
                "\n Add new DEX address to DEX service.\n"
                + HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1. \"servicename\"  (string, required) The DEX service name associated with the new DEX address.\n"
                "2. \"dexaddress\"  (string, required) The smileycoin DEX address associated with the DEX service.\n"
                "3. \"dexdescription\"   (string, required) A short description of the DEX. \n"

                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("adddex", "SmileyDEX 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd dexdescription")
                + HelpExampleCli("adddex", "SmileyDEX 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd dexdescription")
                + HelpExampleRpc("adddex", "SmileyDEX 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd dexdescription")
        );

    std::string serviceName = params[0].get_str();
    std::string dexAddress = params[1].get_str();
    std::string dexDescription = params[2].get_str();

    CBitcoinAddress address(dexAddress);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Smileycoin address");
    } else if (ServiceItemList.IsDex(dexAddress)) {
        throw runtime_error("The entered address is already on DEX list. Please use another address.");
    } else if (dexDescription.length() > 30) {
        throw runtime_error("Dex description cannot be more than 30 characters long.");
    }

    // Amount
    int64_t nValue = 1*COIN;
    int64_t DEFAULT_AMOUNT = 0;

    vector<pair<CScript, int64_t> > vecSend;

    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> myServices;
    ServiceList.GetMyServiceAddresses(myServices);

    std::string dexServiceAddress = "";
    // Send new ticket transaction to corresponding service address
    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
    {
        if(serviceName == get<1>(it->second)) {
            dexServiceAddress = it->first;
        }
    }

    if (!ServiceList.IsService(dexServiceAddress)) {
        throw runtime_error("The entered DEX service name cannot be found on service list.");
    }

    // Parse Smileycoin address
    CBitcoinAddress dServiceAddress(dexServiceAddress);
    CScript scriptPubKey;
    scriptPubKey.SetDestination(dServiceAddress.Get());

    // Pay 1 SMLY to own DEX service address
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    vector<string> str;
    int64_t amount = 0;

    std::string txData = HexStr("ND " + dexAddress + " " + dexDescription, false);
    str.push_back(txData);
    vector<unsigned char> data = ParseHexV(str[0], "Data");

    // Create op_return script in the form -> ND dexAddress dexDescription
    vecSend.push_back(make_pair(CScript() << OP_RETURN << data, max(DEFAULT_AMOUNT, amount) * COIN));

    CWalletTx wtx;

    EnsureWalletIsUnlocked();

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value addubi(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
                "addubi \"servicename\" \"ubiaddress\" \n"
                "\n Add new UBI recipient to UBI service.\n"
                + HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1. \"servicename\"  (string, required) The UBI service name associated with the new UBI recipient address.\n"
                "2. \"ubiaddress\"  (string, required) The smileycoin UBI recipient address associated with the UBI service.\n"

                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("addubi", "SmileyUBI 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleCli("addubi", "SmileyUBI 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleRpc("addubi", "SmileyUBI 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
        );

    std::string serviceName = params[0].get_str();
    std::string ubiAddress = params[1].get_str();

    CBitcoinAddress address(ubiAddress);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Smileycoin address");
    } else if (ServiceItemList.IsUbi(ubiAddress)) {
        throw runtime_error("The entered address is already on UBI recipient list. Please use another address.");
    }

    // Amount
    int64_t nValue = 1*COIN;
    int64_t DEFAULT_AMOUNT = 0;

    vector<pair<CScript, int64_t> > vecSend;

    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> myServices;
    ServiceList.GetMyServiceAddresses(myServices);

    std::string ubiServiceAddress = "";
    // Send new ticket transaction to corresponding service address
    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
    {
        if(serviceName == get<1>(it->second)) {
            ubiServiceAddress = it->first;
        }
    }

    if (!ServiceList.IsService(ubiServiceAddress)) {
        throw runtime_error("The entered UBI service name cannot be found on service list.");
    }

    // Parse Smileycoin address
    CBitcoinAddress uServiceAddress(ubiServiceAddress);
    CScript scriptPubKey;
    scriptPubKey.SetDestination(uServiceAddress.Get());

    // Pay 1 SMLY to own UBI service address
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    vector<string> str;
    int64_t amount = 0;

    std::string txData = HexStr("NU " + ubiAddress, false);
    str.push_back(txData);
    vector<unsigned char> data = ParseHexV(str[0], "Data");

    // Create op_return script in the form -> NU ubiAddress
    vecSend.push_back(make_pair(CScript() << OP_RETURN << data, max(DEFAULT_AMOUNT, amount) * COIN));

    CWalletTx wtx;

    EnsureWalletIsUnlocked();

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value addchapter(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
                "addchapter \"servicename\" \"chapternumber\" \"chapteraddress\" \n"
                "\n Add new chapter to book chapter service.\n"
                + HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1. \"servicename\"  (string, required) The book chapter service name associated with the new chapter address.\n"
                "2. \"chapternumber\"  (string, required) The smileycoin chapter address associated with the book chapter service.\n"
                "3. \"chapteraddress\"   (string, required) The smileycoin chapter address associated with the book chapter service. \n"

                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("addchapter", "Dracula 1 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleCli("addchapter", "Dracula 2 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleRpc("addchapter", "Dracula 3 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
        );

    std::string serviceName = params[0].get_str();
    std::string chapterNum = params[1].get_str();
    std::string chapterAddress = params[2].get_str();

    CBitcoinAddress address(chapterAddress);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Smileycoin address");
    } else if (ServiceItemList.IsChapter(chapterAddress)) {
        throw runtime_error("The entered address is already on chapter list. Please use another address.");
    }

    // Amount
    int64_t nValue = 1*COIN;
    int64_t DEFAULT_AMOUNT = 0;

    vector<pair<CScript, int64_t> > vecSend;

    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> myServices;
    ServiceList.GetMyServiceAddresses(myServices);

    std::string bookServiceAddress = "";
    // Send new chapter transaction to corresponding service address
    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
    {
        if(serviceName == get<1>(it->second)) {
            bookServiceAddress = it->first;
        }
    }

    if (!ServiceList.IsService(bookServiceAddress)) {
        throw runtime_error("The entered book chapter service name cannot be found on service list.");
    }

    // Parse Smileycoin address
    CBitcoinAddress bServiceAddress(bookServiceAddress);
    CScript scriptPubKey;
    scriptPubKey.SetDestination(bServiceAddress.Get());

    // Pay 1 SMLY to own book chapter service address
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    vector<string> str;
    int64_t amount = 0;

    std::string txData = HexStr("NB " + chapterNum + " " + chapterAddress, false);
    str.push_back(txData);
    vector<unsigned char> data = ParseHexV(str[0], "Data");

    // Create op_return script in the form -> NB chapterNum chapterAddress
    vecSend.push_back(make_pair(CScript() << OP_RETURN << data, max(DEFAULT_AMOUNT, amount) * COIN));

    CWalletTx wtx;
    EnsureWalletIsUnlocked();

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value createservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
                "createservice \"servicename\" \"serviceaddress\" \"servicetype\" \n"
                "\nCreate a new service on the blockchain. The price of creating a service is 10 SMLY.\n"
                + HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1. \"servicename\"  (string, required) The service name associated with the service.\n"
                "2. \"serviceaddress\"   (string, required) The smileycoin service address associated with the service. \n"
                "3. \"servicetype\"    (numeric, required) The service type.\n"
                "\nService types:\n"
                "1 = Ticket Sales \n"
                "2 = UBI \n"
                "3 = Book Chapter \n"
                "4 = Traceability \n"
                "5 = Nonprofit Organization \n"
                "6 = DEX \n"
                "7 = Survey \n \n"

                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("createservice", "Cinema 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd 1")
                + HelpExampleCli("createservice", "Dracula 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd 3")
                + HelpExampleRpc("createservice", "UNICEF 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd 5")
        );

    std::string serviceName = params[0].get_str();
    std::string serviceAddress = params[1].get_str();
    std::string serviceType = params[2].get_str();

    // VANTAR CHECK FYRIR LENGD

    CBitcoinAddress address(serviceAddress);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Smileycoin address");
    } else if (ServiceList.IsService(serviceAddress)) {
        throw runtime_error("The entered address is already on service list. Please use another address.");
    } else if (serviceType != "1" && serviceType != "2" && serviceType != "3"&& serviceType != "4"&& serviceType != "5" && serviceType != "6" && serviceType != "7") {
        throw runtime_error("Invalid service type. Please choose a type between 1 - 7.");
    }

    // Amount
    int64_t nValue = 10*COIN;
    int64_t DEFAULT_AMOUNT = 0;

    vector<pair<CScript, int64_t> > vecSend;

    // Parse Smileycoin address
    CBitcoinAddress toAddress("B9TRXJzgUJZZ5zPZbywtNfZHeu492WWRxc");
    CScript scriptPubKey;
    scriptPubKey.SetDestination(toAddress.Get());

    // Pay 10 SMLY to official service address
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    vector<string> str;
    int64_t amount = 0;

    std::string txData = HexStr("NS " + serviceName + " " + serviceAddress + " " + serviceType, false);
    str.push_back(txData);
    vector<unsigned char> data = ParseHexV(str[0], "Data");

    // Create op_return script in the form -> NS serviceName serviceAddress serviceType
    vecSend.push_back(make_pair(CScript() << OP_RETURN << data, max(DEFAULT_AMOUNT, amount) * COIN));

    CWalletTx wtx;

    EnsureWalletIsUnlocked();

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value deleteservice(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "deleteservice \"serviceaddress\" \n"
                "\nThis service will be deleted when the transaction has been confirmed if you are the service owner. You can't undo this action.\n"
                + HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1. \"serviceaddress\"   (string, required) The smileycoin service address associated with the service. \n"

                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("deleteservice", "1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleCli("deleteservice", "1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleRpc("deleteservice", "1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
        );

    std::string serviceAddress = params[0].get_str();

    CBitcoinAddress address(serviceAddress);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Smileycoin address");
    } else if (!IsMine(*pwalletMain, address.Get())) {
        throw runtime_error("Permission denied. The service address is not owned by you.");
    } else if (!ServiceList.IsService(serviceAddress)) {
        throw runtime_error("Invalid Smileycoin service address");
    }

    // Amount
    int64_t nValue = 1*COIN;
    int64_t DEFAULT_AMOUNT = 0;

    vector<pair<CScript, int64_t> > vecSend;

    // Parse Smileycoin address
    CBitcoinAddress toAddress(serviceAddress);
    CScript scriptPubKey;
    scriptPubKey.SetDestination(toAddress.Get());

    // Pay 1 SMLY to service address
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    vector<string> str;
    int64_t amount = 0;

    std::string txData = HexStr("DS " + serviceAddress, false);
    str.push_back(txData);
    vector<unsigned char> data = ParseHexV(str[0], "Data");

    // Create op_return script in the form -> DS serviceAddress
    vecSend.push_back(make_pair(CScript() << OP_RETURN << data, max(DEFAULT_AMOUNT, amount) * COIN));

    CWalletTx wtx;

    EnsureWalletIsUnlocked();

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value createticket(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
                "createticket \"servicename\" \"ticketlocation\" \"ticketname\" \"ticketdatetime\" \"ticketprice\" \"ticketaddress\" \n"
                "\nCreate a new ticket on the blockchain. \n"
                + HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1. \"servicename\"  (string, required) The service name associated with the new ticket.\n"
                "2. \"ticketlocation\"   (string, required) The smileycoin ticket sales service address associated with the service. \n"
                "3. \"ticketname\"    (string, required) The ticket name.\n"
                "4. \"ticketdatetime\"    (datetime, required) The ticket date and time.\n"
                "5. \"ticketprice\"    (numeric, required) The ticket price.\n"
                "6. \"ticketaddress\"    (string, required) The smileycoin ticket address associated with the ticket.\n"

                "\nDateTime format:\n"
                "dd/MM/yyyyhh:mm \n \n"

                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("createticket", "Paradiso Alfabakka Shining 22/08/202022:00 30 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleCli("createticket", "AirSmiley BSI Akureyri 10/12/202009:00 1500000 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleRpc("createticket", "Paradiso Kringla Alien 30/10/202023:00 40 1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
        );

    std::string serviceName = params[0].get_str();
    std::string ticketLocation = params[1].get_str();
    std::string ticketName = params[2].get_str();
    std::string ticketDateTime= params[3].get_str();
    std::string ticketPrice = params[4].get_str();
    std::string ticketAddress = params[5].get_str();

    // VANTAR CHECK FYRIR EF ADDRESSA ER NU ÞEGAR SERVICE OG LENGD OG EF TYPE ER 1-6

    CBitcoinAddress address(ticketAddress);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Smileycoin address");
    } else if (ServiceItemList.IsTicket(ticketAddress)) {
        throw runtime_error("The entered address is already on ticket list. Please use another address.");
    } else if (!is_number(ticketPrice)) {
        throw runtime_error("Price must be a number.");
    } else if (!is_date(ticketDateTime)) {
        throw runtime_error("Date and time must be in the format dd/MM/yyyyhh:mm (e.g. 22/08/202207:00)");
    } else if (!is_before(ticketDateTime)) {
        throw runtime_error("The entered ticket date and time has already expired.");
    } else if (ticketLocation.length() > 20) {
        throw runtime_error("Ticket location cannot be more than 20 characters long.");
    } else if (ticketName.length() > 20) {
        throw runtime_error("Ticket name cannot be more than 20 characters long.");
    }

    // Amount
    int64_t nValue = 1*COIN;
    int64_t DEFAULT_AMOUNT = 0;

    vector<pair<CScript, int64_t> > vecSend;

    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> myServices;
    ServiceList.GetMyServiceAddresses(myServices);

    std::string ticketServiceAddress = "";
    // Send new ticket transaction to corresponding service address
    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = myServices.begin(); it!=myServices.end(); it++ )
    {
        if(serviceName == get<1>(it->second)) {
            ticketServiceAddress = it->first;
        }
    }

    if (!ServiceList.IsService(ticketServiceAddress)) {
        throw runtime_error("The entered service name cannot be found on service list.");
    }

    // Parse Smileycoin address
    CBitcoinAddress tServiceAddress(ticketServiceAddress);
    CScript scriptPubKey;
    scriptPubKey.SetDestination(tServiceAddress.Get());

    // Pay 1 SMLY to official service address
    vecSend.push_back(make_pair(scriptPubKey, nValue));

    vector<string> str;
    int64_t amount = 0;

    std::string txData = HexStr("NT " + ticketLocation + " " + ticketName + " " + ticketDateTime + " " + ticketPrice + " " + ticketAddress, false);
    str.push_back(txData);
    vector<unsigned char> data = ParseHexV(str[0], "Data");

    // Create op_return script in the form -> NT ticketLoc ticketName ticketDateTime ticketPrice ticketAddress
    vecSend.push_back(make_pair(CScript() << OP_RETURN << data, max(DEFAULT_AMOUNT, amount) * COIN));

    CWalletTx wtx;

    EnsureWalletIsUnlocked();

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value buyticket(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "buyticket \"ticketaddress\" \n"
                "\nBuy a ticket on the blockchain. \n"
                + HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1. \"ticketaddress\"    (string, required) The smileycoin ticket address associated with the ticket.\n"

                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("buyticket", "1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
                + HelpExampleRpc("buyticket", "1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd")
        );

    std::string ticketAddress = params[0].get_str();

    CBitcoinAddress address(ticketAddress);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Smileycoin address");
    } else if (!ServiceItemList.IsTicket(ticketAddress)) {
        throw runtime_error("The entered address is not on ticket list. Please use correct ticket address.");
    }

    // Amount on ticket
    int64_t DEFAULT_AMOUNT = 0;
    vector<pair<CScript, int64_t> > vecSend;

    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> tickets;
    ServiceItemList.GetTicketList(tickets);

    std::string ticketValue = "";
    // Get ticket value
    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > >::const_iterator it = tickets.begin(); it!=tickets.end(); it++ )
    {
        if (ticketAddress == it->first) {
            ticketValue = get<5>(it->second);
        }
    }

    // Parse Smileycoin address
    CScript scriptPubKey;
    scriptPubKey.SetDestination(address.Get());
    vecSend.push_back(make_pair(scriptPubKey, std::stoi(ticketValue)*COIN));

    vector<string> str;
    int64_t amount = 0;

    std::string txData = HexStr("BT " + ticketAddress, false);
    str.push_back(txData);
    vector<unsigned char> data = ParseHexV(str[0], "Data");

    // Create op_return script in the form -> NT ticketLoc ticketName ticketDateTime ticketPrice ticketAddress
    vecSend.push_back(make_pair(CScript() << OP_RETURN << data, max(DEFAULT_AMOUNT, amount) * COIN));

    CWalletTx wtx;

    EnsureWalletIsUnlocked();

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value getserviceaddresses(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error("getserviceaddresses\n"
                            "Returns all verified addresses, ordered by the type of services they provide.\n"
        );

    Object root;
    Array tservices; /* TicketSales */
    Array bservices; /* BookChapter */
    Array nservices; /* NPO */
    Array dservices; /* DEX */
    Array sservices; /* Survey */
    Object name_address;

    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> services;
    ServiceList.GetServiceAddresses(services);

    // Loop through all existing services
    for(std::multiset< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator s = services.begin(); s!=services.end(); s++ )
    {
        name_address.clear();
        // Ef service type er ticketsales
        if (get<2>(s->second) == "Ticket Sales") { // "1"
            name_address.push_back(Pair("name", get<1>(s->second)));
            name_address.push_back(Pair("address", s->first));
            tservices.push_back(name_address);
        } else if (get<2>(s->second) == "Book Chapter") { // "3"
            name_address.push_back(Pair("name", get<1>(s->second)));
            name_address.push_back(Pair("address", s->first));
            bservices.push_back(name_address);
        } else if (get<2>(s->second) == "Nonprofit Organization") { // "5"
            name_address.push_back(Pair("name", get<1>(s->second)));
            name_address.push_back(Pair("address", s->first));
            nservices.push_back(name_address);
        } else if (get<2>(s->second) == "DEX") { // "6"
            name_address.push_back(Pair("name", get<1>(s->second)));
            name_address.push_back(Pair("address", s->first));
            dservices.push_back(name_address);
        } else if (get<2>(s->second) == "Survey") { // "7"
            name_address.push_back(Pair("name", get<1>(s->second)));
            name_address.push_back(Pair("address", s->first));
            sservices.push_back(name_address);
        }
    }

    root.push_back(Pair("Ticket Sales", tservices));
    root.push_back(Pair("Book Chapter", bservices));
    root.push_back(Pair("Nonprofit Organization", nservices));
    root.push_back(Pair("DEX", dservices));
    root.push_back(Pair("Survey", sservices));

    return root;
}

Value getticketlist(const Array& params, bool fHelp)
{

    if (fHelp || params.size() != 1)
        throw runtime_error("getticketlist \"address\"\n"
                            "Returns all tickets that belong to the specified ticket service address\n"
        );

    CBitcoinAddress address = CBitcoinAddress(params[0].get_str());
    if(!address.IsValid())
        throw runtime_error("Invalid Smileycoin address");

    if (!ServiceList.IsService(params[0].get_str()))
        throw runtime_error("Invalid Smileycoin service address");

    Object obj2;
    Array arr;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string > > > info;
    ServiceItemList.GetTicketList(info);

    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > >::const_iterator it = info.begin(); it!=info.end(); it++ )
    {
        std::string serviceAddress = get<1>(it->second);
        std::string dateOfTicket = get<4>(it->second);

        if (serviceAddress == address.ToString() && is_before(dateOfTicket)) {
            Object obj;
            obj.push_back(Pair("Name: ", get<3>(it->second)));
            obj.push_back(Pair("Location: ", get<2>(it->second)));
            obj.push_back(Pair("Date and Time: ", get<4>(it->second)));
            obj.push_back(Pair("Price: ", get<5>(it->second)));
            obj.push_back(Pair("Ticket Address: ", it->first));
            arr.push_back(obj);
        }
    }

    obj2.push_back(Pair("Tickets: " , arr));
    return obj2;
}


Value getubilist(const Array& params, bool fHelp)
{

    if (fHelp || params.size() != 1)
        throw runtime_error("getubilist \"address\"\n"
                            "Returns all UBI recipient addresses that belong to the specified UBI service address\n"
                            );
    
    CBitcoinAddress address = CBitcoinAddress(params[0].get_str());
    if(!address.IsValid())
        throw runtime_error("Not a valid Smileycoin address");
    
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string> > > services;
    ServiceList.GetServiceAddresses(services);
    bool isUbi = false;
    for(std::multiset< std::pair<std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = services.begin(); it!=services.end(); it++ ) {
        if (get<2>(it->second) == "UBI") {
            isUbi = true;
        }
    }
    if (!ServiceList.IsService(params[0].get_str()))
        throw runtime_error("Not a valid service address");

    if (!isUbi)
        throw runtime_error("Not a UBI address");

    Object obj;
    std::multiset<std::pair< std::string, std::tuple<std::string, std::string>>> info;

    ServiceItemList.GetUbiList(info);

    
    for(std::set< std::pair< std::string, std::tuple<std::string, std::string> > >::const_iterator it = info.begin(); it!=info.end(); it++ )
    {
        std::string toAddress = get<1>(it->second);
        if (toAddress == address.ToString()) {
            obj.push_back(Pair("UBI Recipient Address: ", it->first));
        }
    }
    
    return obj;
}

Value getdexlist(const Array& params, bool fHelp)
{

    if (fHelp || params.size() != 1)
        throw runtime_error("getdexlist \"address\"\n"
                            "Returns all DEX addresses that belong to the specified DEX service address\n"
                            );

    Object obj;
    std::multiset<std::pair< std::string, std::tuple<std::string, std::string, std::string>>> info;

    ServiceItemList.GetDexList(info);

    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = info.begin(); it!=info.end(); it++ )
    {
        obj.push_back(Pair("DEX Address: ", it->first));
        obj.push_back(Pair("Description: ", get<2>(it->second)));
    }

    return obj;
}

Value getnpolist(const Array& params, bool fHelp)
{

    if (fHelp || params.size() != 1)
        throw runtime_error("getnpolist\n"
                            "Returns all non profit organization addresses\n"
                            );

    Object obj;
    /*std::multiset<std::pair< CScript, std::tuple<std::string, std::string, std::string>>> info;

    ServiceItemList.GetNpoList(info);

    for(std::set< std::pair< CScript, std::tuple<std::string, std::string, std::string> > >::const_iterator it = info.begin(); it!=info.end(); it++ )
    {
        obj.push_back(Pair("Npo name: ", get<1>(it->second)));
        obj.push_back(Pair("Npo address: ", get<2>(it->second)));
    }*/

    return obj;
}

Value getbooklist(const Array& params, bool fHelp)
{

    if (fHelp || params.size() != 1)
        throw runtime_error("getbooklist \"address\"\n"
                            "Returns all book chapters that belong to the specified book service address\n"
                            );

    Object obj;
    std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> info;
    ServiceItemList.GetBookList(info);

    //TODO bæta við book name, book author og year
    for(std::set< std::pair< std::string, std::tuple<std::string, std::string, std::string> > >::const_iterator it = info.begin(); it!=info.end(); it++ )
    {
        obj.push_back(Pair("Chapter Number: ", get<2>(it->second)));
        obj.push_back(Pair("Chapter Address: ", it->first));
    }

    return obj;
}

Value getaddressinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error("getaddressbalance\n"
                            "Returns the balance and height of a given address.\n"
                            );
    CBitcoinAddress address = CBitcoinAddress(params[0].get_str());
    if(!address.IsValid())
        throw runtime_error("Invalid Smileycoin address");
    Object obj;
    CScript key;    
    key.SetDestination(address.Get());
    std::pair<int64_t, int> value;
    if(!pcoinsTip->GetAddressInfo(key, value))
        throw runtime_error("No information available - address has been emptied or never used.");
    obj.push_back(Pair("Balance", ValueFromAmount(value.first)));
    obj.push_back(Pair("Height", value.second));

    return obj; 
}

#ifdef ENABLE_WALLET
class DescribeAddressVisitor : public boost::static_visitor<Object>
{
public:
    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;
        pwalletMain->GetPubKey(keyID, vchPubKey);
        obj.push_back(Pair("isscript", false));
        obj.push_back(Pair("pubkey", HexStr(vchPubKey)));
        obj.push_back(Pair("iscompressed", vchPubKey.IsCompressed()));
        return obj;
    }

    Object operator()(const CScriptID &scriptID) const {
        Object obj;
        obj.push_back(Pair("isscript", true));
        CScript subscript;
        pwalletMain->GetCScript(scriptID, subscript);
        std::vector<CTxDestination> addresses;
        txnouttype whichType;
        int nRequired;
        ExtractDestinations(subscript, whichType, addresses, nRequired);
        obj.push_back(Pair("script", GetTxnOutputType(whichType)));
        obj.push_back(Pair("hex", HexStr(subscript.begin(), subscript.end())));
        Array a;
        BOOST_FOREACH(const CTxDestination& addr, addresses)
            a.push_back(CBitcoinAddress(addr).ToString());
        obj.push_back(Pair("addresses", a));
        if (whichType == TX_MULTISIG)
            obj.push_back(Pair("sigsrequired", nRequired));
        return obj;
    }
};
#endif

Value validateaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddress \"smileycoinaddress\"\n"
            "\nReturn information about the given smileycoin address.\n"
            "\nArguments:\n"
            "1. \"smileycoinaddress\"     (string, required) The smileycoin address to validate\n"
            "\nResult:\n"
            "{\n"
            "  \"isvalid\" : true|false,            (boolean) If the address is valid or not. If not, this is the only property returned.\n"
            "  \"address\" : \"smileycoinaddress\", (string) The smileycoin address validated\n"
            "  \"ismine\" : true|false,             (boolean) If the address is yours or not\n"
            "  \"isscript\" : true|false,           (boolean) If the key is a script\n"
            "  \"pubkey\" : \"publickeyhex\",       (string) The hex value of the raw public key\n"
            "  \"iscompressed\" : true|false,       (boolean) If the address is compressed\n"
            "  \"account\" : \"account\"            (string) The account associated with the address, \"\" is the default account\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
            + HelpExampleRpc("validateaddress", "\"1PSSGeFHDnKNxiEyFrD1wcEaHr9hrQDDWc\"")
        );

    CBitcoinAddress address(params[0].get_str());
    bool isValid = address.IsValid();

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
#ifdef ENABLE_WALLET
        bool fMine = pwalletMain ? IsMine(*pwalletMain, dest) : false;
        ret.push_back(Pair("ismine", fMine));
        if (fMine) {
            Object detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain && pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest].name));
#endif
    }
    return ret;
}

//
// Used by addmultisigaddress / createmultisig:
//
CScript _createmultisig(const Array& params)
{
    int nRequired = params[0].get_int();
    const Array& keys = params[1].get_array();

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();
#ifdef ENABLE_WALLET
        // Case 1: Bitcoin address and we have full public key:
        CBitcoinAddress address(ks);
        if (pwalletMain && address.IsValid())
        {
            CKeyID keyID;
            if (!address.GetKeyID(keyID))
                throw runtime_error(
                    strprintf("%s does not refer to a key",ks));
            CPubKey vchPubKey;
            if (!pwalletMain->GetPubKey(keyID, vchPubKey))
                throw runtime_error(
                    strprintf("no full public key for address %s",ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }

        // Case 2: hex public key
        else
#endif
        if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }
    CScript result;
    result.SetMultisig(nRequired, pubkeys);
    return result;
}

Value createmultisig(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 2)
    {
        string msg = "createmultisig nrequired [\"key\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "It returns a json object with the address and redeemScript.\n"

            "\nArguments:\n"
            "1. nrequired      (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keys\"       (string, required) A json array of keys which are smileycoin addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"key\"    (string) smileycoin address or hex-encoded public key\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of the hex-encoded redemption script.\n"
            "}\n"

            "\nExamples:\n"
            "\nCreate a multisig address from 2 addresses\n"
            + HelpExampleCli("createmultisig", "2 \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("createmultisig", "2, \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"")
        ;
        throw runtime_error(msg);
    }

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig(params);
    CScriptID innerID = inner.GetID();
    CBitcoinAddress address(innerID);

    Object result;
    result.push_back(Pair("address", address.ToString()));
    result.push_back(Pair("redeemScript", HexStr(inner.begin(), inner.end())));

    return result;
}

Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage \"smileycoinaddress\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"smileycoinaddress\"  (string, required) The smileycoin address to use for the signature.\n"
            "2. \"signature\"          (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"            (string, required) The message that was signed.\n"
            "\nResult:\n"
            "true|false   (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", \"signature\", \"my message\"")
        );

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CBitcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == keyID);
}

#pragma clang diagnostic pop
