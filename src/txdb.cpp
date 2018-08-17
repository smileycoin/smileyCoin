// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "richlistdb.h"
#include "core.h"
#include "uint256.h"

#include <stdint.h>

using namespace std;

static const char DB_COINS = 'c';
static const char DB_BLOCK_FILES = 'f';
static const char DB_TXINDEX = 't';
static const char DB_ADDRESSINFO = 'a';
static const char DB_BLOCK_INDEX = 'b';

static const char DB_BEST_BLOCK = 'B';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_RICHLIST_FORK_FLAG = 'V';
static const char DB_LAST_BLOCK = 'l';


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
                              const uint256 &hashBlock) {
    LogPrint("coindb", "Committing %u changed transactions and %u address balances to coin database...\n",(unsigned int)mapCoins.size(), (unsigned int)mapAddressInfo.size());

    CLevelDBBatch batch;
    for (std::map<uint256, CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        BatchWriteCoins(batch, it->first, it->second);
    for (std::map<CScript, std::pair<int64_t,int> >::const_iterator it = mapAddressInfo.begin(); it != mapAddressInfo.end(); it++)
        BatchWriteAddressInfo(batch, it->first, it->second);
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
