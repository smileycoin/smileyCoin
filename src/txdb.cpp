// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "richlistdb.h"
#include "servicelistdb.h"
#include "serviceitemlistdb.h"
#include "core.h"
#include "uint256.h"
#include "base58.h"

#include <stdint.h>

using namespace std;

static const char DB_COINS = 'c';
static const char DB_BLOCK_FILES = 'f';
static const char DB_TXINDEX = 't';
static const char DB_SERVICEINFO = 'z';
static const char DB_SERVICETICKETLIST = 'k';
static const char DB_SERVICEUBILIST = 'u';
static const char DB_SERVICEDEXLIST = 'd';
static const char DB_SERVICEBOOKLIST = 'm';
static const char DB_ADDRESSINFO = 'a';
static const char DB_BLOCK_INDEX = 'b';

static const char DB_BEST_BLOCK = 'B';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_RICHLIST_FORK_FLAG = 'V';
static const char DB_LAST_BLOCK = 'l';
static const char DB_SERVICELIST_FORK_FLAG = 'S';
static const char DB_SERVICETICKETLIST_FORK_FLAG = 'O';
static const char DB_SERVICEUBILIST_FORK_FLAG = 'U';
static const char DB_SERVICEDEXLIST_FORK_FLAG = 'D';
static const char DB_SERVICEBOOKLIST_FORK_FLAG = 'M';


void static BatchWriteCoins(CLevelDBBatch &batch, const uint256 &hash, const CCoins &coins) {
    if (coins.IsPruned())
        batch.Erase(make_pair(DB_COINS, hash));
    else
        batch.Write(make_pair(DB_COINS, hash), coins);
}

void static BatchWriteAddressInfo(CLevelDBBatch &batch, const CScript &key, const std::pair<int64_t, int> &value) {
    if(value.first == 0)
        batch.Erase(make_pair(DB_ADDRESSINFO, key));
    else 
        batch.Write(make_pair(DB_ADDRESSINFO, key), value);
}

void static BatchWriteServiceInfo(CLevelDBBatch &batch, const std::string &key, const std::tuple<std::string, std::string, std::string> &value) {
    // If op_return begins with "DS" (delete service)
    if (get<0>(value) == "DS") {
        // Erase the service associated with address
        batch.Erase(make_pair(DB_SERVICEINFO, key));
    } else if (get<0>(value) == "NS") { // If op_return begins with NS (new service)
        batch.Write(make_pair(DB_SERVICEINFO, key), value);
    }
}

void static BatchWriteServiceTicketList(CLevelDBBatch &batch, const std::string &key, const std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) {
    // If op_return begins with "DT" (delete ticket)
    if (get<0>(value) == "DT") {
        // Erase the ticket associated with address
        batch.Erase(make_pair(DB_SERVICETICKETLIST, key));
    } else if (get<0>(value) == "NT") { // If op_return begins with NT (new ticket)
        batch.Write(make_pair(DB_SERVICETICKETLIST, key), value);
    }
}

void static BatchWriteServiceUbiList(CLevelDBBatch &batch, const std::string &key, const std::tuple<std::string, std::string> &value) {
    // If op_return begins with "DU" (delete ubi recipient)
    if (get<0>(value) == "DU") {
        // Erase the ubi recipient associated with address
        batch.Erase(make_pair(DB_SERVICEUBILIST, key));
    } else if (get<0>(value) == "NU") { // If op_return begins with NU (new ubi recipient)
        batch.Write(make_pair(DB_SERVICEUBILIST, key), value);
    }
}

void static BatchWriteServiceDexList(CLevelDBBatch &batch, const std::string &key, const std::tuple<std::string, std::string, std::string> &value) {
    // If op_return begins with "DD" (delete dex)
    if (get<0>(value) == "DD") {
        // Erase the dex associated with address
        batch.Erase(make_pair(DB_SERVICEDEXLIST, key));
    } else if (get<0>(value) == "ND") { // If op_return begins with ND (new dex)
        batch.Write(make_pair(DB_SERVICEDEXLIST, key), value);
    }
}

void static BatchWriteServiceBookList(CLevelDBBatch &batch, const std::string &key, const std::tuple<std::string, std::string, std::string> &value) {
    // If op_return begins with "DB" (delete book chapter)
    if (get<0>(value) == "DB") {
        // Erase the ticket associated with address
        batch.Erase(make_pair(DB_SERVICEBOOKLIST, key));
    } else if (get<0>(value) == "NB") { // If op_return begins with NB (new book chapter)
        batch.Write(make_pair(DB_SERVICEBOOKLIST, key), value);
    }
}

void static BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
    batch.Write(DB_BEST_BLOCK, hash);
}

CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe) {
}

bool CCoinsViewDB::GetAddressInfo(const CScript &key, std::pair<int64_t, int> &value) {
    return db.Read(make_pair(DB_ADDRESSINFO, key), value);
}

bool CCoinsViewDB::SetAddressInfo(const CScript &key, const std::pair<int64_t, int> &value) {
    CLevelDBBatch batch;
    BatchWriteAddressInfo(batch, key, value);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::GetServiceInfo(const std::string &key, std::tuple<std::string, std::string, std::string> &value) {
    return db.Read(make_pair(DB_SERVICEINFO, key), value);
}

bool CCoinsViewDB::SetServiceInfo(const std::string &key, const std::tuple<std::string, std::string, std::string> &value) {
    CLevelDBBatch batch;
    BatchWriteServiceInfo(batch, key, value);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::GetTicketList(const std::string &key, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) {
    return db.Read(make_pair(DB_SERVICETICKETLIST, key), value);
}

bool CCoinsViewDB::SetTicketList(const std::string &key, const std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) {
    CLevelDBBatch batch;
    BatchWriteServiceTicketList(batch, key, value);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::GetUbiList(const std::string &key, std::tuple<std::string, std::string> &value) {
    return db.Read(make_pair(DB_SERVICEUBILIST, key), value);
}

bool CCoinsViewDB::SetUbiList(const std::string &key, const std::tuple<std::string, std::string> &value) {
    CLevelDBBatch batch;
    BatchWriteServiceUbiList(batch, key, value);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::GetDexList(const std::string &key, std::tuple<std::string, std::string, std::string> &value) {
    return db.Read(make_pair(DB_SERVICEDEXLIST, key), value);
}

bool CCoinsViewDB::SetDexList(const std::string &key, const std::tuple<std::string, std::string, std::string> &value) {
    CLevelDBBatch batch;
    BatchWriteServiceDexList(batch, key, value);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::GetBookList(const std::string &key, std::tuple<std::string, std::string, std::string> &value) {
    return db.Read(make_pair(DB_SERVICEBOOKLIST, key), value);
}

bool CCoinsViewDB::SetBookList(const std::string &key, const std::tuple<std::string, std::string, std::string> &value) {
    CLevelDBBatch batch;
    BatchWriteServiceBookList(batch, key, value);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::GetCoins(const uint256 &txid, CCoins &coins) {
    return db.Read(make_pair(DB_COINS, txid), coins);
}

bool CCoinsViewDB::SetCoins(const uint256 &txid, const CCoins &coins) {
    CLevelDBBatch batch;
    BatchWriteCoins(batch, txid, coins);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::HaveCoins(const uint256 &txid) {
    return db.Exists(make_pair(DB_COINS, txid));
}

uint256 CCoinsViewDB::GetBestBlock() {
    uint256 hashBestChain;
    if (!db.Read(DB_BEST_BLOCK, hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool CCoinsViewDB::SetBestBlock(const uint256 &hashBlock) {
    CLevelDBBatch batch;
    BatchWriteHashBestChain(batch, hashBlock);
    return db.WriteBatch(batch);
}

bool CCoinsViewDB::BatchWrite(const std::map<uint256, CCoins> &mapCoins,
                              const std::map<CScript, std::pair<int64_t,int> > &mapAddressInfo,
                              const std::map<std::string, std::tuple<std::string, std::string, std::string>> &mapServiceInfo,
                              const std::map<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>> &mapServiceTicketList,
                              const std::map<std::string, std::tuple<std::string, std::string>> &mapServiceUbiList,
                              const std::map<std::string, std::tuple<std::string, std::string, std::string>> &mapServiceDexList,
                              const std::map<std::string, std::tuple<std::string, std::string, std::string>> &mapServiceBookList,
                              const uint256 &hashBlock) {
    LogPrint("coindb", "Committing %u changed transactions and %u address balances to coin database...\n",(unsigned int)mapCoins.size(), (unsigned int)mapAddressInfo.size());

    CLevelDBBatch batch;
    for (std::map<uint256, CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        BatchWriteCoins(batch, it->first, it->second);
    for (std::map<CScript, std::pair<int64_t,int> >::const_iterator it = mapAddressInfo.begin(); it != mapAddressInfo.end(); it++)
        BatchWriteAddressInfo(batch, it->first, it->second);
    for (std::map<std::string, std::tuple<std::string, std::string, std::string> >::const_iterator it = mapServiceInfo.begin(); it != mapServiceInfo.end(); it++)
        BatchWriteServiceInfo(batch, it->first, it->second);
    for (std::map<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>::const_iterator it = mapServiceTicketList.begin(); it != mapServiceTicketList.end(); it++)
        BatchWriteServiceTicketList(batch, it->first, it->second);
    for (std::map<std::string, std::tuple<std::string, std::string>>::const_iterator it = mapServiceUbiList.begin(); it != mapServiceUbiList.end(); it++)
        BatchWriteServiceUbiList(batch, it->first, it->second);
    for (std::map<std::string, std::tuple<std::string, std::string, std::string>>::const_iterator it = mapServiceDexList.begin(); it != mapServiceDexList.end(); it++)
        BatchWriteServiceDexList(batch, it->first, it->second);
    for (std::map<std::string, std::tuple<std::string, std::string, std::string>>::const_iterator it = mapServiceBookList.begin(); it != mapServiceBookList.end(); it++)
        BatchWriteServiceBookList(batch, it->first, it->second);

    if (hashBlock != uint256(0))
        BatchWriteHashBestChain(batch, hashBlock);

    return db.WriteBatch(batch);
}

bool CCoinsViewDB::GetRichAddresses(CRichList &richlist) {

    leveldb::Iterator *pcursor = db.NewIterator();
    
    std::vector<unsigned char> v;
    v.assign(21,'0');
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(DB_ADDRESSINFO, CScript(v));
    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid()) 
    {
        boost::this_thread::interruption_point();
        try 
        { 
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            std::pair<char,CScript> key;
            ssKey >> key;
            if (key.first == DB_ADDRESSINFO) 
            {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                std::pair<int64_t,int> addressinfo; 
                ssValue >> addressinfo;
                if(addressinfo.first >= RICH_AMOUNT)
                        richlist.maddresses.insert(make_pair(key.second, addressinfo));
                pcursor->Next();
            } else {
                break;
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}

bool CCoinsViewDB::GetServiceAddresses(CServiceList &servicelist) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(DB_SERVICEINFO, string("service"));

    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid())
    {
        boost::this_thread::interruption_point();
        try
        {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            std::pair<char, string> key;
            ssKey >> key;
            if (key.first == DB_SERVICEINFO)
            {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                std::tuple<std::string, std::string, std::string> serviceinfo;
                ssValue >> serviceinfo;
                servicelist.saddresses.insert(make_pair(key.second, serviceinfo));
                pcursor->Next();

            } else {
                break;
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}


bool CCoinsViewDB::GetTicketList(CServiceItemList &ticketlist) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(DB_SERVICETICKETLIST, string("ticket"));
    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid())
    {
        boost::this_thread::interruption_point();
        try
        {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            std::pair<char, string> key;
            ssKey >> key;
            if (key.first == DB_SERVICETICKETLIST)
            {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> ticketitem;
                ssValue >> ticketitem;
                ticketlist.taddresses.insert(make_pair(key.second, ticketitem));
                pcursor->Next();
            } else {
                break;
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}


bool CCoinsViewDB::GetUbiList(CServiceItemList &ubilist) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(DB_SERVICEUBILIST, string("ubi"));
    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid())
    {
        boost::this_thread::interruption_point();
        try
        {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            std::pair<char,string> key;
            ssKey >> key;
            if (key.first == DB_SERVICEUBILIST)
            {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                std::tuple<std::string, std::string> ubiitem;
                ssValue >> ubiitem;
                ubilist.uaddresses.insert(make_pair(key.second, ubiitem));
                pcursor->Next();
            } else {
                break;
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}


bool CCoinsViewDB::GetDexList(CServiceItemList &dexlist) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(DB_SERVICEDEXLIST, string("dex"));
    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid())
    {
        boost::this_thread::interruption_point();
        try
        {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            std::pair<char, string> key;
            ssKey >> key;
            if (key.first == DB_SERVICEDEXLIST)
            {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                std::tuple<std::string, std::string, std::string> dexitem;
                ssValue >> dexitem;
                dexlist.daddresses.insert(make_pair(key.second, dexitem));
                pcursor->Next();
            } else {
                break;
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}

bool CCoinsViewDB::GetBookList(CServiceItemList &booklist) {
    leveldb::Iterator *pcursor = db.NewIterator();
    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(DB_SERVICEBOOKLIST, string("chapter"));
    pcursor->Seek(ssKeySet.str());

    while (pcursor->Valid())
    {
        boost::this_thread::interruption_point();
        try
        {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            std::pair<char, string> key;
            ssKey >> key;
            if (key.first == DB_SERVICEBOOKLIST)
            {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                std::tuple<std::string, std::string, std::string> bookitem;
                ssValue >> bookitem;
                booklist.baddresses.insert(make_pair(key.second, bookitem));
                pcursor->Next();
            } else {
                break;
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe) {
}

bool CBlockTreeDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(make_pair(DB_BLOCK_INDEX, blockindex.GetBlockHash()), blockindex);
}

bool CBlockTreeDB::WriteBestInvalidWork(const CBigNum& bnBestInvalidWork)
{
    // Obsolete; only written for backward compatibility.
    return Write('I', bnBestInvalidWork);
}

bool CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo &info) {
    return Write(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::WriteLastBlockFile(int nFile) {
    return Write(DB_LAST_BLOCK, nFile);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

bool CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists(DB_REINDEX_FLAG);
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read(DB_LAST_BLOCK, nFile);
}

bool CCoinsViewDB::GetStats(CCoinsStats &stats) {
    leveldb::Iterator *pcursor = db.NewIterator();
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = GetBestBlock();
    ss << stats.hashBlock;
    int64_t nTotalAmount = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            if (chType == DB_COINS) {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                CCoins coins;
                ssValue >> coins;
                uint256 txhash;
                ssKey >> txhash;
                ss << txhash;
                ss << VARINT(coins.nVersion);
                ss << (coins.fCoinBase ? DB_COINS : 'n');
                ss << VARINT(coins.nHeight);
                stats.nTransactions++;
                for (unsigned int i=0; i<coins.vout.size(); i++) {
                    const CTxOut &out = coins.vout[i];
                    if (!out.IsNull()) {
                        stats.nTransactionOutputs++;
                        ss << VARINT(i+1);
                        ss << out;
                        nTotalAmount += out.nValue;
                    }
                }
                stats.nSerializedSize += 32 + slValue.size();
                ss << VARINT(0);
            }
            pcursor->Next();
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;
    stats.nHeight = mapBlockIndex.find(GetBestBlock())->second->nHeight;
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    return true;
}

bool CBlockTreeDB::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return Read(make_pair(DB_TXINDEX, txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >&vect) {
    CLevelDBBatch batch;
    for (std::vector<std::pair<uint256,CDiskTxPos> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(make_pair(DB_TXINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::WriteRichListFork(bool fForked) {
    if (fForked)
        return Write(DB_RICHLIST_FORK_FLAG, '1');
    else
        return Erase(DB_RICHLIST_FORK_FLAG);
}

bool CBlockTreeDB::ReadRichListFork(bool &fForked) {
    fForked = Exists(DB_RICHLIST_FORK_FLAG);
    return true;
}

bool CBlockTreeDB::WriteServiceListFork(bool fForked) {
    if (fForked)
        return Write(DB_SERVICELIST_FORK_FLAG, '1');
    else
        return Erase(DB_SERVICELIST_FORK_FLAG);
}

bool CBlockTreeDB::ReadServiceListFork(bool &fForked) {
    fForked = Exists(DB_SERVICELIST_FORK_FLAG);
    return true;
}

bool CBlockTreeDB::WriteServiceTicketListFork(bool fForked) {
    if (fForked)
        return Write(DB_SERVICETICKETLIST_FORK_FLAG, '1');
    else
        return Erase(DB_SERVICETICKETLIST_FORK_FLAG);
}

bool CBlockTreeDB::ReadServiceTicketListFork(bool &fForked) {
    fForked = Exists(DB_SERVICETICKETLIST_FORK_FLAG);
    return true;
}

bool CBlockTreeDB::WriteServiceUbiListFork(bool fForked) {
    if (fForked)
        return Write(DB_SERVICEUBILIST_FORK_FLAG, '1');
    else
        return Erase(DB_SERVICEUBILIST_FORK_FLAG);
}

bool CBlockTreeDB::ReadServiceUbiListFork(bool &fForked) {
    fForked = Exists(DB_SERVICEUBILIST_FORK_FLAG);
    return true;
}

bool CBlockTreeDB::WriteServiceDexListFork(bool fForked) {
    if (fForked)
        return Write(DB_SERVICEDEXLIST_FORK_FLAG, '1');
    else
        return Erase(DB_SERVICEDEXLIST_FORK_FLAG);
}

bool CBlockTreeDB::ReadServiceDexListFork(bool &fForked) {
    fForked = Exists(DB_SERVICEDEXLIST_FORK_FLAG);
    return true;
}

bool CBlockTreeDB::WriteServiceBookListFork(bool fForked) {
    if (fForked)
        return Write(DB_SERVICEBOOKLIST_FORK_FLAG, '1');
    else
        return Erase(DB_SERVICEBOOKLIST_FORK_FLAG);
}

bool CBlockTreeDB::ReadServiceBookListFork(bool &fForked) {
    fForked = Exists(DB_SERVICEBOOKLIST_FORK_FLAG);
    return true;
}

bool CBlockTreeDB::WriteFlag(const std::string &name, bool fValue) {
    return Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string &name, bool &fValue) {
    char ch;
    if (!Read(std::make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::LoadBlockIndexGuts()
{
    leveldb::Iterator *pcursor = NewIterator();

    CDataStream ssKeySet(SER_DISK, CLIENT_VERSION);
    ssKeySet << make_pair(DB_BLOCK_INDEX, uint256(0));
    pcursor->Seek(ssKeySet.str());

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->key();
            CDataStream ssKey(slKey.data(), slKey.data()+slKey.size(), SER_DISK, CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            if (chType == DB_BLOCK_INDEX) {
                leveldb::Slice slValue = pcursor->value();
                CDataStream ssValue(slValue.data(), slValue.data()+slValue.size(), SER_DISK, CLIENT_VERSION);
                CDiskBlockIndex diskindex;
                ssValue >> diskindex;

                // Construct block index object
                CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev          = InsertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight        = diskindex.nHeight;
                pindexNew->nFile          = diskindex.nFile;
                pindexNew->nDataPos       = diskindex.nDataPos;
                pindexNew->nUndoPos       = diskindex.nUndoPos;
                pindexNew->nVersion       = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime          = diskindex.nTime;
                pindexNew->nBits          = diskindex.nBits;
                pindexNew->nNonce         = diskindex.nNonce;
                pindexNew->nStatus        = diskindex.nStatus;
                pindexNew->nTx            = diskindex.nTx;

                if (!pindexNew->CheckIndex())
                    return error("LoadBlockIndex() : CheckIndex failed: %s", pindexNew->ToString());

                pcursor->Next();
            } else {
                break; // if shutdown requested or finished loading block index
            }
        } catch (std::exception &e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pcursor;

    return true;
}
