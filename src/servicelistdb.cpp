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
inline std::string ServiceAction(const CServiceInfo &ai) {return get<0>(ai);}

inline std::string ServiceInfoToAddress(const CServiceAddressInfo &ai) {return get<0>(ai);}
inline std::string ServiceInfoLocation(const CServiceAddressInfo &ai) {return get<1>(ai);}
inline std::string ServiceInfoName(const CServiceAddressInfo &ai) {return get<2>(ai);}
inline std::string ServiceInfoDateAndTime(const CServiceAddressInfo &ai) {return get<3>(ai);}
inline std::string ServiceInfoValue(const CServiceAddressInfo &ai) {return get<4>(ai);}
inline std::string ServiceInfoAddress(const CServiceAddressInfo &ai) {return get<5>(ai);}

bool CServiceList::SetForked(const bool &fFork)
{
    fForked = fFork;
    return true;
}

bool CServiceList::UpdateServiceInfo(const std::map<std::string, std::tuple<std::string, std::string, std::string> > &map)
{
    for(std::map<std::string, std::tuple<std::string, std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        if (get<0>(it->second) == "DS") { // If op_return begins with DS (delete service)
            mapServiceScriptPubKeys::iterator itService = maddresses.find(it->first);
            // If key is found in service list
            if (itService != maddresses.end()) {
                maddresses.erase(itService);
            }
        } else if (get<0>(it->second) == "NS") { // If op_return begins with NS (new service)
            maddresses.insert(*it);
        }
    }
    return true;
}

bool CServiceList::UpdateServiceAddressInfo(const std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &map)
{
    for(std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();
        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            infoaddress.insert(*it);
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
    std::map<std::string, std::tuple<std::string, std::string, std::string> > serviceInfo;
    mapServiceScriptPubKeys mforkedAddresses;

    for(mapServiceScriptPubKeys::const_iterator it = maddresses.begin(); it!=maddresses.end(); it++)
    {
        if (get<0>(it->second) == "DS") { // If op_return begins with DS
            mapServiceScriptPubKeys::iterator itService = maddresses.find(it->first);
            // If key is found in service list
            if (itService != maddresses.end()) {
                maddresses.erase(itService);
            }
        } else if (get<0>(it->second) == "NS") { // If op_return begins with NS
            maddresses.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedAddresses.size());
    }

    while(pindexSeek->pprev && !mforkedAddresses.empty())
    {
        LogPrintStr(" while i servicelistdb ");
        // return false;
        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                /*mapServiceScriptPubKeys::iterator it = mforkedAddresses.find(key);
                if(it == mforkedAddresses.end())
                    continue;
                serviceInfo.insert(std::make_pair(key, std::make_tuple(ServiceAction(it), ServiceName(it), ServiceType(it))));
                mforkedAddresses.erase(it);*/
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
                    /*mapServiceScriptPubKeys::iterator it = mforkedAddresses.find(key);
                    if(it == mforkedAddresses.end())
                        continue;
                    serviceInfo.insert(std::make_pair(it->first, std::make_tuple(ServiceAction(it), ServiceName(it), ServiceType(it))));
                    mforkedAddresses.erase(it);*/
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
    typedef std::pair<std::string, std::tuple<std::string, std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, serviceInfo) {
        LogPrintStr(" pairType i UpdateServiceAddressHeights ");
        ret = pcoinsTip->SetServiceInfo(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateServiceInfo(serviceInfo))
        return false;
    return mforkedAddresses.empty();
}
// The heights need to be rolled back before new blocks are connected if any were disconnected.
// TODO: We should try to get rid of this and write the height undo information to the disk instead.
bool CServiceList::UpdateServiceAddressInfoHeights()
{
    if(!fForked)
        return true;

    CBlockIndex* pindexSeek = mapBlockIndex.find(pcoinsTip->GetBestBlock())->second;
    if(!chainActive.Contains(pindexSeek)) {
        return false;
    }

    CBlock block;
    std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > serviceAddressInfo;
    mapServiceInfoScriptPubKeys mforkedAddressInfo;

    for(mapServiceInfoScriptPubKeys::const_iterator it = infoaddress.begin(); it!=infoaddress.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();

        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            infoaddress.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedAddressInfo.size());
    }

    while(pindexSeek->pprev && !mforkedAddressInfo.empty())
    {
        // return false;
        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                mapServiceInfoScriptPubKeys::iterator it = mforkedAddressInfo.find(key);
                if(it == mforkedAddressInfo.end())
                    continue;

                serviceAddressInfo.insert(std::make_pair(key, std::make_tuple(ServiceInfoToAddress(it), ServiceInfoLocation(it), ServiceInfoName(it), ServiceInfoDateAndTime(it), ServiceInfoValue(it), ServiceInfoAddress(it))));
                mforkedAddressInfo.erase(it);
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
                    mapServiceInfoScriptPubKeys::iterator it = mforkedAddressInfo.find(key);
                    if(it == mforkedAddressInfo.end())
                        continue;
                    serviceAddressInfo.insert(std::make_pair(it->first, std::make_tuple(ServiceInfoToAddress(it), ServiceInfoLocation(it), ServiceInfoName(it), ServiceInfoDateAndTime(it), ServiceInfoValue(it), ServiceInfoAddress(it))));
                    mforkedAddressInfo.erase(it);
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
    typedef std::pair<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, serviceAddressInfo) {
        ret = pcoinsTip->SetServiceAddressInfo(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateServiceAddressInfo(serviceAddressInfo))
        return false;
    return mforkedAddressInfo.empty();
}

bool CServiceList::GetServiceAddresses(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for(std::map<std::string, std::tuple<std::string, std::string, std::string> >::const_iterator it=maddresses.begin(); it!=maddresses.end(); it++)
    {
        std::string displayType;

        // If op_return begins with NS (new service)
        if(get<0>(it->second) == "NS")
        {
            if (get<2>(it->second) == "1") {
                displayType = "Ticket Sales";
            } else if (get<2>(it->second) == "2") {
                displayType = "UBI";
            } else if (get<2>(it->second) == "3") {
                displayType = "Book Chapter";
            } else if (get<2>(it->second) == "4") {
                displayType = "Traceability";
            } else if (get<2>(it->second) == "5") {
                displayType = "Nonprofit Organization";
            } else if (get<2>(it->second) == "6") {
                displayType = "DEX";
            } else {
                displayType = get<2>(it->second);
            }
            retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), displayType)));
        }
    }
    return true;
}

bool CServiceList::GetMyServiceAddresses(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for (std::map<std::string, std::tuple<std::string, std::string, std::string> >::const_iterator it = maddresses.begin();it != maddresses.end(); it++)
    {
        std::string displayType;

        if (IsMine(*pwalletMain, CBitcoinAddress(it->first).Get())) {
            // If op_return begins with NS
            if(get<0>(it->second) == "NS")
            {
                if (get<2>(it->second) == "1") {
                    displayType = "Ticket Sales";
                } else if (get<2>(it->second) == "2") {
                    displayType = "UBI";
                } else if (get<2>(it->second) == "3") {
                    displayType = "Book Chapter";
                } else if (get<2>(it->second) == "4") {
                    displayType = "Traceability";
                } else if (get<2>(it->second) == "5") {
                    displayType = "Nonprofit Organization";
                } else if (get<2>(it->second) == "6") {
                    displayType = "DEX";
                } else {
                    displayType = get<2>(it->second);
                }
                retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), displayType)));
            }
        }
    }
    return true;
}

bool CServiceList::GetServiceAddressInfo(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> &info) const {
    for(std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> >::const_iterator it=infoaddress.begin(); it!=infoaddress.end(); it++)
    {
        info.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), get<2>(it->second), get<3>(it->second), get<4>(it->second), get<5>(it->second))));
    }
    return true;
}