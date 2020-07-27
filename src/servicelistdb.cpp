//
// Created by Lenovo on 4/24/2020.
//

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developersg
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "servicelistdb.h"

#include "base58.h"
#include "net.h"
#include "netbase.h"
#include "rpcserver.h"
#include "util.h"
#include <vector>

#include "init.h"
#include "main.h"
#include "sync.h"
#include "wallet.h"

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

inline std::string ServiceType(const CServiceInfo &ai) {return get<2>(ai);}
inline std::string ServiceName(const CServiceInfo &ai) {return get<1>(ai);}
inline std::string ServiceAddress(const CServiceInfo &ai) {return get<0>(ai);}

bool CServiceList::SetForked(const bool &fFork)
{
    fForked = fFork;
    return true;
}

bool CServiceList::UpdateServiceInfo(const std::map<CScript, std::tuple<std::string, std::string, std::string> > &map)
{
    for(std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();
        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            maddresses.insert(*it);
        }
    }
    return true;
}

//ÆTTI NOTIFYSERVICEPAGE AD VERA HER FREKAR??

// The heights need to be rolled back before new blocks are connected if any were disconnected.
// TODO: We should try to get rid of this and write the height undo information to the disk instead.
bool CServiceList::UpdateServiceAddressHeights()
{
    if(!fForked)
        return true;

    CBlockIndex* pindexSeek = mapBlockIndex.find(pcoinsTip->GetBestBlock())->second;
    if(!chainActive.Contains(pindexSeek)) {
        return false;
    }

    CBlock block;
    std::map<CScript, std::tuple<std::string, std::string, std::string> > serviceInfo;
    mapServiceScriptPubKeys mforkedAddresses;

    for(mapServiceScriptPubKeys::const_iterator it = maddresses.begin(); it!=maddresses.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();

        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            maddresses.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedAddresses.size());
    }

    while(pindexSeek->pprev && !mforkedAddresses.empty())
    {
        // return false;
        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                mapServiceScriptPubKeys::iterator it = mforkedAddresses.find(key);
                if(it == mforkedAddresses.end())
                    continue;
                LogPrintStr("SERVICELISTDB UPDATESERVICEADDRESSHEIGHTS");
                serviceInfo.insert(std::make_pair(key, std::make_tuple(ServiceAddress(it), ServiceName(it), ServiceType(it))));
                mforkedAddresses.erase(it);
                if(fDebug) {
                    CTxDestination dest;
                    ExtractDestination(key, dest);
                    CBitcoinAddress addr;
                    addr.Set(dest);
                    LogPrintf("%s found at height %d\n",addr.ToString(),pindexSeek->nHeight);
                }
            }
        }

        CBlockUndo undo;
        CDiskBlockPos pos = pindexSeek ->GetUndoPos();
        if (undo.ReadFromDisk(pos, pindexSeek->pprev->GetBlockHash())) //TODO: hvenær klikkar þetta?
        {
            for (unsigned int i=0; i<undo.vtxundo.size(); i++)
            {
                for (unsigned int j=0; j<undo.vtxundo[i].vprevout.size(); j++)
                {
                    CScript key = undo.vtxundo[i].vprevout[j].txout.scriptPubKey;
                    mapServiceScriptPubKeys::iterator it = mforkedAddresses.find(key);
                    if(it == mforkedAddresses.end())
                        continue;
                    serviceInfo.insert(std::make_pair(it->first, std::make_tuple(ServiceAddress(it), ServiceName(it), ServiceType(it))));
                    mforkedAddresses.erase(it);
                    if(fDebug) {
                        CTxDestination dest;
                        ExtractDestination(key, dest);
                        CBitcoinAddress addr;
                        addr.Set(dest);
                        LogPrintf("%s found at height %d\n",addr.ToString(),pindexSeek->nHeight);
                    }
                }
            }
        }
        else {
            LogPrintf("UpdateServiceAddressHeights(): Failed to read undo information\n");
            break;
        }
        pindexSeek = pindexSeek -> pprev;
    }

    bool ret;
    typedef std::pair<CScript, std::tuple<std::string,std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, serviceInfo) {
        ret = pcoinsTip->SetServiceInfo(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateServiceInfo(serviceInfo))
        return false;
    return mforkedAddresses.empty();
}

bool CServiceList::GetServiceAddresses(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for(std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it=maddresses.begin(); it!=maddresses.end(); it++)
    {
        retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), get<2>(it->second))));
    }
    return true;
}

bool CServiceList::GetMyServiceAddresses(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for (std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it = maddresses.begin();it != maddresses.end(); it++) {
        if (IsMine(*pwalletMain, CBitcoinAddress(get<1>(it->second)).Get())) {
            retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), get<2>(it->second))));
        }
    }
    return true;
}