// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXDB_LEVELDB_H
#define BITCOIN_TXDB_LEVELDB_H

#include "leveldbwrapper.h"
#include "main.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class CBigNum;
class CCoins;
class uint256;
class CRichList;
class CServiceList;
class CServiceItemList;

// -dbcache default (MiB)
static const int64_t nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t nMaxDbCache = sizeof(void*) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t nMinDbCache = 4;

/** CCoinsView backed by the LevelDB coin database (chainstate/) */
class CCoinsViewDB : public CCoinsView
{
protected:
    CLevelDBWrapper db;
public:
    CCoinsViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool GetCoins(const uint256 &txid, CCoins &coins);
    bool SetCoins(const uint256 &txid, const CCoins &coins);
    bool GetAddressInfo(const CScript &key, std::pair<int64_t,int> &value);
    bool SetAddressInfo(const CScript &key, const std::pair<int64_t,int> &value);
    bool GetServiceInfo(const std::string &key, std::tuple<std::string, std::string, std::string> &value);
    bool SetServiceInfo(const std::string &key, const std::tuple<std::string, std::string, std::string> &value);
    bool GetTicketList(const std::string &key, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value);
    bool SetTicketList(const std::string &key, const std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value);
    bool GetUbiList(const std::string &key, std::tuple<std::string, std::string> &value);
    bool SetUbiList(const std::string &key, const std::tuple<std::string, std::string> &value);
    bool GetDexList(const std::string &key, std::tuple<std::string, std::string, std::string> &value);
    bool SetDexList(const std::string &key, const std::tuple<std::string, std::string, std::string> &value);
    bool GetBookList(const std::string &key, std::tuple<std::string, std::string, std::string> &value);
    bool SetBookList(const std::string &key, const std::tuple<std::string, std::string, std::string> &value);

    bool HaveCoins(const uint256 &txid);
    uint256 GetBestBlock();
    bool SetBestBlock(const uint256 &hashBlock);
    bool BatchWrite(const std::map<uint256, CCoins> &mapCoins, const std::map<CScript,std::pair<int64_t,int> > &mapAddressInfo,
                    const std::map<std::string, std::tuple<std::string, std::string, std::string> > &mapServiceInfo,
                    const std::map<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &mapServiceTicketList,
                    const std::map<std::string, std::tuple<std::string, std::string> > &mapServiceUbiList,
                    const std::map<std::string, std::tuple<std::string, std::string, std::string> > &mapServiceDexList,
                    const std::map<std::string, std::tuple<std::string, std::string, std::string> > &mapServiceBookList,
                    const uint256 &hashBlock);
    bool GetStats(CCoinsStats &stats);

    bool GetRichAddresses(CRichList &richlist);
    bool GetServiceAddresses(CServiceList &servicelist);
    bool GetTicketList(CServiceItemList &ticketlist);
    bool GetUbiList(CServiceItemList &ubilist);
    bool GetDexList(CServiceItemList &dexlist);
    bool GetBookList(CServiceItemList &booklist);
};

/** Access to the block database (blocks/index/) */
class CBlockTreeDB : public CLevelDBWrapper
{
public:
    CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
private:
    CBlockTreeDB(const CBlockTreeDB&);
    void operator=(const CBlockTreeDB&);
public:
    bool WriteBlockIndex(const CDiskBlockIndex& blockindex);
    bool WriteBestInvalidWork(const CBigNum& bnBestInvalidWork);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &fileinfo);
    bool WriteBlockFileInfo(int nFile, const CBlockFileInfo &fileinfo);
    bool ReadLastBlockFile(int &nFile);
    bool WriteLastBlockFile(int nFile);
    bool WriteReindexing(bool fReindex);
    bool ReadReindexing(bool &fReindex);
    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> > &list);
    bool WriteRichListFork(bool fForked);
    bool ReadRichListFork(bool &fForked);
    bool WriteServiceListFork(bool fForked);
    bool ReadServiceListFork(bool &fForked);
    bool WriteServiceTicketListFork(bool fForked);
    bool ReadServiceTicketListFork(bool &fForked);
    bool WriteServiceUbiListFork(bool fForked);
    bool ReadServiceUbiListFork(bool &fForked);
    bool WriteServiceDexListFork(bool fForked);
    bool ReadServiceDexListFork(bool &fForked);
    bool WriteServiceBookListFork(bool fForked);
    bool ReadServiceBookListFork(bool &fForked);
    bool WriteFlag(const std::string &name, bool fValue);
    bool ReadFlag(const std::string &name, bool &fValue);
    bool LoadBlockIndexGuts();
};

#endif // BITCOIN_TXDB_LEVELDB_H
