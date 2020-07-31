// Copyright (c) 2012-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coins.h"
#include "base58.h"

#include <assert.h>

//Calculate number of bytes for the bitmask, and its number of non-zero bytes
void CCoins::CalcMaskSize(unsigned int &nBytes, unsigned int &nNonzeroBytes) const {
    unsigned int nLastUsedByte = 0;
    for (unsigned int b = 0; 2+b*8 < vout.size(); b++) {
        bool fZero = true;
        for (unsigned int i = 0; i < 8 && 2+b*8+i < vout.size(); i++) {
            if (!vout[2+b*8+i].IsNull()) {
                fZero = false;
                continue;
            }
        }
        if (!fZero) {
            nLastUsedByte = b + 1;
            nNonzeroBytes++;
        }
    }
    nBytes += nLastUsedByte;
}

bool CCoins::Spend(const COutPoint &out, CTxInUndo &undo) {
    if (out.n >= vout.size())
        return false;
    if (vout[out.n].IsNull())
        return false;
    undo = CTxInUndo(vout[out.n]);
    vout[out.n].SetNull();
    Cleanup();
    if (vout.size() == 0) {
        undo.nHeight = nHeight;
        undo.fCoinBase = fCoinBase;
        undo.nVersion = this->nVersion;
    }
    return true;
}

bool CCoins::Spend(int nPos) {
    CTxInUndo undo;
    COutPoint out(0, nPos);
    return Spend(out, undo);
}


bool CCoinsView::GetCoins(const uint256 &txid, CCoins &coins) { return false; }
bool CCoinsView::SetCoins(const uint256 &txid, const CCoins &coins) { return false; }
bool CCoinsView::GetAddressInfo(const CScript &key, std::pair<int64_t,int> &value) { return false; }
bool CCoinsView::SetAddressInfo(const CScript &key, const std::pair<int64_t,int> &value) { return false; }
bool CCoinsView::GetServiceInfo(const CScript &key, std::tuple<std::string, std::string, std::string> &value) { return false; }
bool CCoinsView::SetServiceInfo(const CScript &key, const std::tuple<std::string, std::string, std::string> &value) { return false; }
bool CCoinsView::GetServiceAddressInfo(const CScript &key, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) { return false; }
bool CCoinsView::SetServiceAddressInfo(const CScript &key, const std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) { return false; }
bool CCoinsView::HaveCoins(const uint256 &txid) { return false; }
uint256 CCoinsView::GetBestBlock() { return uint256(0); }
bool CCoinsView::SetBestBlock(const uint256 &hashBlock) { return false; }
bool CCoinsView::BatchWrite(const std::map<uint256, CCoins> &mapCoins, const std::map<CScript, std::pair<int64_t,int> > &mapAddressInfo,
                            const std::map<CScript, std::tuple<std::string, std::string, std::string> > &mapServiceInfo,
                            const std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &mapServiceAddressInfo,
                            const uint256 &hashBlock) { return false; }
bool CCoinsView::GetStats(CCoinsStats &stats) { return false; }


CCoinsViewBacked::CCoinsViewBacked(CCoinsView &viewIn) : base(&viewIn) { }
bool CCoinsViewBacked::GetCoins(const uint256 &txid, CCoins &coins) { return base->GetCoins(txid, coins); }
bool CCoinsViewBacked::SetCoins(const uint256 &txid, const CCoins &coins) { return base->SetCoins(txid, coins); }
bool CCoinsViewBacked::GetAddressInfo(const CScript &key, std::pair<int64_t,int> &value) { return base->GetAddressInfo(key, value); }
bool CCoinsViewBacked::SetAddressInfo(const CScript &key, const std::pair<int64_t,int> &value) { return base->SetAddressInfo(key, value); }
bool CCoinsViewBacked::GetServiceInfo(const CScript &key, std::tuple<std::string, std::string, std::string> &value) { return base->GetServiceInfo(key, value); }
bool CCoinsViewBacked::SetServiceInfo(const CScript &key, const std::tuple<std::string, std::string, std::string> &value) { return base->SetServiceInfo(key, value); }
bool CCoinsViewBacked::GetServiceAddressInfo(const CScript &key, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) { return base->GetServiceAddressInfo(key, value); }
bool CCoinsViewBacked::SetServiceAddressInfo(const CScript &key, const std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) { return base->SetServiceAddressInfo(key, value); }
bool CCoinsViewBacked::HaveCoins(const uint256 &txid) { return base->HaveCoins(txid); }
uint256 CCoinsViewBacked::GetBestBlock() { return base->GetBestBlock(); }
bool CCoinsViewBacked::SetBestBlock(const uint256 &hashBlock) { return base->SetBestBlock(hashBlock); }
void CCoinsViewBacked::SetBackend(CCoinsView &viewIn) { base = &viewIn; }
bool CCoinsViewBacked::BatchWrite(const std::map<uint256, CCoins> &mapCoins, const std::map<CScript, std::pair<int64_t,int> > &mapAddressInfo,
                                  const std::map<CScript, std::tuple<std::string, std::string, std::string> > &mapServiceInfo,
                                  const std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &mapServiceAddressInfo,
                                  const uint256 &hashBlock) { return base->BatchWrite(mapCoins, mapAddressInfo, mapServiceInfo, mapServiceAddressInfo, hashBlock); }
bool CCoinsViewBacked::GetStats(CCoinsStats &stats) { return base->GetStats(stats); }

CCoinsViewCache::CCoinsViewCache(CCoinsView &baseIn, bool fDummy) : CCoinsViewBacked(baseIn), hashBlock(0) { }

bool CCoinsViewCache::GetCoins(const uint256 &txid, CCoins &coins) {
    if (cacheCoins.count(txid)) {
        coins = cacheCoins[txid];
        return true;
    }
    if (base->GetCoins(txid, coins)) {
        cacheCoins[txid] = coins;
        return true;
    }
    return false;
}

bool CCoinsViewCache::GetAddressInfo(const CScript &key, std::pair<int64_t,int> &value) {
    std::map<CScript,std::pair<int64_t,int> >::iterator it = cacheAddressInfo.find(key);
    if(it!=cacheAddressInfo.end()) {
        value = it->second;
        return true;
    }
    if(base->GetAddressInfo(key,value)) {
        cacheAddressInfo[key] = value;
        return true;
    }
    return false;
}

bool CCoinsViewCache::SetAddressInfo(const CScript &key, const std::pair<int64_t,int> &value) {
    cacheAddressInfo[key] = value;
    return true;
}

bool CCoinsViewCache::GetServiceInfo(const CScript &key, std::tuple<std::string, std::string, std::string> &value) {
    std::map<CScript, std::tuple<std::string, std::string, std::string> >::iterator it = cacheServiceInfo.find(key);

    if(it!=cacheServiceInfo.end()) {
        value = it->second;
        return true;
    }
    if(base->GetServiceInfo(key,value)) {
        cacheServiceInfo[key] = value;
        return true;
    }
    return false;
}

bool CCoinsViewCache::SetServiceInfo(const CScript &key, const std::tuple<std::string, std::string, std::string> &value) {
    cacheServiceInfo[key] = value;
    return true;
}

bool CCoinsViewCache::GetServiceAddressInfo(const CScript &key, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) {
    std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> >::iterator it = cacheServiceAddressInfo.find(key);

    if(it!=cacheServiceAddressInfo.end()) {
        value = it->second;
        return true;
    }
    if(base->GetServiceAddressInfo(key,value)) {
        cacheServiceAddressInfo[key] = value;
        return true;
    }
    return false;
}

bool CCoinsViewCache::SetServiceAddressInfo(const CScript &key, const std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> &value) {
    cacheServiceAddressInfo[key] = value;
    return true;
}

std::map<uint256,CCoins>::iterator CCoinsViewCache::FetchCoins(const uint256 &txid) {
    std::map<uint256,CCoins>::iterator it = cacheCoins.lower_bound(txid);
    if (it != cacheCoins.end() && it->first == txid)
        return it;
    CCoins tmp;
    if (!base->GetCoins(txid,tmp))
        return cacheCoins.end();
    std::map<uint256,CCoins>::iterator ret = cacheCoins.insert(it, std::make_pair(txid, CCoins()));
    tmp.swap(ret->second);
    return ret;
}

CCoins &CCoinsViewCache::GetCoins(const uint256 &txid) {
    std::map<uint256,CCoins>::iterator it = FetchCoins(txid);
    assert(it != cacheCoins.end());
    return it->second;
}

bool CCoinsViewCache::SetCoins(const uint256 &txid, const CCoins &coins) {
    cacheCoins[txid] = coins;
    return true;
}

bool CCoinsViewCache::HaveCoins(const uint256 &txid) {
    return FetchCoins(txid) != cacheCoins.end();
}

uint256 CCoinsViewCache::GetBestBlock() {
    if (hashBlock == uint256(0))
        hashBlock = base->GetBestBlock();
    return hashBlock;
}

bool CCoinsViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
    return true;
}

bool CCoinsViewCache::BatchWrite(const std::map<uint256, CCoins> &mapCoins, 
                                 const std::map<CScript, std::pair<int64_t,int> > &mapAddressInfo,
                                 const std::map<CScript, std::tuple<std::string, std::string, std::string> > &mapServiceInfo,
                                 const std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &mapServiceAddressInfo,
                                 const uint256 &hashBlockIn) {
    for (std::map<uint256, CCoins>::const_iterator it = mapCoins.begin(); it != mapCoins.end(); it++)
        cacheCoins[it->first] = it->second;
    for (std::map<CScript, std::pair<int64_t,int> >::const_iterator it = mapAddressInfo.begin(); it != mapAddressInfo.end(); it++)
        cacheAddressInfo[it->first] = it->second;
    for (std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it = mapServiceInfo.begin(); it != mapServiceInfo.end(); it++)
        cacheServiceInfo[it->first] = it->second;
    for (std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> >::const_iterator it = mapServiceAddressInfo.begin(); it != mapServiceAddressInfo.end(); it++)
        cacheServiceAddressInfo[it->first] = it->second;

    hashBlock = hashBlockIn;
    return true;
}

bool CCoinsViewCache::Flush() {
    bool fOk = base->BatchWrite(cacheCoins, cacheAddressInfo, cacheServiceInfo, cacheServiceAddressInfo, hashBlock);
    if (fOk) {
        cacheCoins.clear();
        cacheAddressInfo.clear();
        cacheServiceInfo.clear();
        cacheServiceAddressInfo.clear();
    }
    return fOk;
}

unsigned int CCoinsViewCache::GetCacheSize() {
    return cacheCoins.size();
}

const CTxOut &CCoinsViewCache::GetOutputFor(const CTxIn& input)
{
    const CCoins &coins = GetCoins(input.prevout.hash);
    assert(coins.IsAvailable(input.prevout.n));
    return coins.vout[input.prevout.n];
}

int64_t CCoinsViewCache::GetValueIn(const CTransaction& tx)
{
    if (tx.IsCoinBase())
        return 0;

    int64_t nResult = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
        nResult += GetOutputFor(tx.vin[i]).nValue;

    return nResult;
}

bool CCoinsViewCache::HaveInputs(const CTransaction& tx)
{
    if (!tx.IsCoinBase()) {
        // first check whether information about the prevout hash is available
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            const COutPoint &prevout = tx.vin[i].prevout;
            if (!HaveCoins(prevout.hash))
                return false;
        }

        // then check whether the actual outputs are available
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            const COutPoint &prevout = tx.vin[i].prevout;
            const CCoins &coins = GetCoins(prevout.hash);
            if (!coins.IsAvailable(prevout.n))
                return false;
        }
    }
    return true;
}

double CCoinsViewCache::GetPriority(const CTransaction &tx, int nHeight)
{
    if (tx.IsCoinBase())
        return 0.0;
    double dResult = 0.0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        const CCoins &coins = GetCoins(txin.prevout.hash);
        if (!coins.IsAvailable(txin.prevout.n)) continue;
        if (coins.nHeight < nHeight) {
            dResult += coins.vout[txin.prevout.n].nValue * (nHeight-coins.nHeight);
        }
    }
    return tx.ComputePriority(dResult);
}
