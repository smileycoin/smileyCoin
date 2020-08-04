//
// Created by Lenovo on 4/24/2020.
//

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developersg
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "serviceitemlistdb.h"

#include "main.h"
#include "base58.h"
#include "chainparams.h"

#include <vector>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

inline std::string TicketToAddress(const CServiceTicket &ai) {return get<0>(ai);}
inline std::string TicketLocation(const CServiceTicket &ai) {return get<1>(ai);}
inline std::string TicketName(const CServiceTicket &ai) {return get<2>(ai);}
inline std::string TicketDateAndTime(const CServiceTicket &ai) {return get<3>(ai);}
inline std::string TicketValue(const CServiceTicket &ai) {return get<4>(ai);}
inline std::string TicketAddress(const CServiceTicket &ai) {return get<5>(ai);}

inline std::string UbiToAddress(const CServiceUbi &ai) {return get<0>(ai);}
inline std::string UbiAddress(const CServiceUbi &ai) {return get<1>(ai);}

inline std::string DexToAddress(const CServiceDex &ai) {return get<0>(ai);}
inline std::string DexAddress(const CServiceDex &ai) {return get<1>(ai);}
inline std::string DexDescription(const CServiceDex &ai) {return get<2>(ai);}

inline std::string NpoToAddress(const CServiceNpo &ai) {return get<0>(ai);}
inline std::string NpoName(const CServiceNpo &ai) {return get<1>(ai);}
inline std::string NpoAddress(const CServiceNpo &ai) {return get<2>(ai);}

inline std::string BookToAddress(const CServiceBook &ai) {return get<0>(ai);}
inline std::string BookAddress(const CServiceBook &ai) {return get<1>(ai);}


bool CServiceItemList::SetForked(const bool &fFork)
{
    fForked = fFork;
    return true;
}


// Tickets

bool CServiceItemList::UpdateTicketList(const std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &map)
{
    for(std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        LogPrintStr("UPDATETICKETLIST");
        LogPrintStr(it->first.ToString());
        LogPrintStr(get<0>(it->second));
        LogPrintStr(get<1>(it->second));
        LogPrintStr(get<2>(it->second));
        LogPrintStr(get<3>(it->second));
        LogPrintStr(get<4>(it->second));

        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();
        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            LogPrintStr(" OPCODE == OP_RETURN ");
            LogPrintStr(script.ToString());
            ticketitems.insert(*it);
        }
    }
    return true;
}

// The heights need to be rolled back before new blocks are connected if any were disconnected.
// TODO: We should try to get rid of this and write the height undo information to the disk instead.
bool CServiceItemList::UpdateTicketListHeights()
{
    LogPrintStr("UPDATETICKETLISTHEIGHTS");
    if(!fForked)
        return true;

    CBlockIndex* pindexSeek = mapBlockIndex.find(pcoinsTip->GetBestBlock())->second;
    if(!chainActive.Contains(pindexSeek)) {
        return false;
    }

    CBlock block;
    std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > ticketItem;
    mapServiceTicketListScriptPubKeys mforkedServiceTicketList;

    for(mapServiceTicketListScriptPubKeys::const_iterator it = ticketitems.begin(); it!=ticketitems.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();

        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            ticketitems.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedServiceTicketList.size());
    }

    while(pindexSeek->pprev && !mforkedServiceTicketList.empty())
    {

        // return false;

        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                mapServiceTicketListScriptPubKeys::iterator it = mforkedServiceTicketList.find(key);
                if(it == mforkedServiceTicketList.end())
                    continue;

                ticketItem.insert(std::make_pair(key, std::make_tuple(TicketToAddress(it), TicketLocation(it), TicketName(it), TicketDateAndTime(it), TicketValue(it), TicketAddress(it))));
                mforkedServiceTicketList.erase(it);
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
                    mapServiceTicketListScriptPubKeys::iterator it = mforkedServiceTicketList.find(key);
                    if(it == mforkedServiceTicketList.end())
                        continue;
                    ticketItem.insert(std::make_pair(it->first, std::make_tuple(TicketToAddress(it), TicketLocation(it), TicketName(it), TicketDateAndTime(it), TicketValue(it), TicketAddress(it))));
                    mforkedServiceTicketList.erase(it);
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
            LogPrintf("UpdateTicketListHeights(): Failed to read undo information\n");
            break;
        }
        pindexSeek = pindexSeek -> pprev;
    }

    bool ret;
    typedef std::pair<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, ticketItem) {
        ret = pcoinsTip->SetTicketList(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateTicketList(ticketItem))
        return false;
    return mforkedServiceTicketList.empty();
}


bool CServiceItemList::GetTicketList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> &retset) const {
    for(std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> >::const_iterator it=ticketitems.begin(); it!=ticketitems.end(); it++)
    {
        retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), get<2>(it->second), get<3>(it->second), get<4>(it->second), get<5>(it->second))));
    }
    return true;
}

// Ubi

bool CServiceItemList::UpdateUbiList(const std::map<CScript, std::tuple<std::string, std::string> > &map)
{
    for(std::map<CScript, std::tuple<std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        LogPrintStr("UPDATEUBILIST");
        LogPrintStr(it->first.ToString());
        LogPrintStr(get<0>(it->second));
        LogPrintStr(get<1>(it->second));

        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();
        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            LogPrintStr(" OPCODE == OP_RETURN ");
            LogPrintStr(script.ToString());
            ubiitems.insert(*it);
        }
    }
    return true;
}

// The heights need to be rolled back before new blocks are connected if any were disconnected.
// TODO: We should try to get rid of this and write the height undo information to the disk instead.
bool CServiceItemList::UpdateUbiListHeights()
{
    LogPrintStr("UPDATEUBILISTHEIGHTS");
    if(!fForked)
        return true;

    CBlockIndex* pindexSeek = mapBlockIndex.find(pcoinsTip->GetBestBlock())->second;
    if(!chainActive.Contains(pindexSeek)) {
        return false;
    }

    CBlock block;
    std::map<CScript, std::tuple<std::string, std::string> > ubiItem;
    mapServiceUbiListScriptPubKeys mforkedServiceUbiList;

    for(mapServiceUbiListScriptPubKeys::const_iterator it = ubiitems.begin(); it!=ubiitems.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();

        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            ubiitems.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedServiceUbiList.size());
    }

    while(pindexSeek->pprev && !mforkedServiceUbiList.empty())
    {

        // return false;

        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                mapServiceUbiListScriptPubKeys::iterator it = mforkedServiceUbiList.find(key);
                if(it == mforkedServiceUbiList.end())
                    continue;

                ubiItem.insert(std::make_pair(key, std::make_tuple(UbiToAddress(it), UbiAddress(it))));
                mforkedServiceUbiList.erase(it);
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
                    mapServiceUbiListScriptPubKeys::iterator it = mforkedServiceUbiList.find(key);
                    if(it == mforkedServiceUbiList.end())
                        continue;
                    ubiItem.insert(std::make_pair(it->first, std::make_tuple(UbiToAddress(it), UbiAddress(it))));
                    mforkedServiceUbiList.erase(it);
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
            LogPrintf("UpdateUbiListHeights(): Failed to read undo information\n");
            break;
        }
        pindexSeek = pindexSeek -> pprev;
    }

    bool ret;
    typedef std::pair<CScript, std::tuple<std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, ubiItem) {
        ret = pcoinsTip->SetUbiList(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateUbiList(ubiItem))
        return false;
    return mforkedServiceUbiList.empty();
}

bool CServiceItemList::GetUbiList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string>>> &retset) const {
    for(std::map<CScript, std::tuple<std::string, std::string> >::const_iterator it=ubiitems.begin(); it!=ubiitems.end(); it++)
    {
        retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second))));
    }
    return true;
}
    
// Dex

bool CServiceItemList::UpdateDexList(const std::map<CScript, std::tuple<std::string, std::string, std::string> > &map)
{
    for(std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        LogPrintStr("UPDATEDEXLIST");
        LogPrintStr(it->first.ToString());
        LogPrintStr(get<0>(it->second));
        LogPrintStr(get<1>(it->second));
        LogPrintStr(get<2>(it->second));

        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();
        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            LogPrintStr(" OPCODE == OP_RETURN ");
            LogPrintStr(script.ToString());
            dexitems.insert(*it);
        }
    }
    return true;
}

// The heights need to be rolled back before new blocks are connected if any were disconnected.
// TODO: We should try to get rid of this and write the height undo information to the disk instead.
bool CServiceItemList::UpdateDexListHeights()
{
    LogPrintStr("UPDATEDEXLISTHEIGHTS");
    if(!fForked)
        return true;

    CBlockIndex* pindexSeek = mapBlockIndex.find(pcoinsTip->GetBestBlock())->second;
    if(!chainActive.Contains(pindexSeek)) {
        return false;
    }

    CBlock block;
    std::map<CScript, std::tuple<std::string, std::string, std::string> > dexItem;
    mapServiceDexListScriptPubKeys mforkedServiceDexList;

    for(mapServiceDexListScriptPubKeys::const_iterator it = dexitems.begin(); it!=dexitems.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();

        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            dexitems.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedServiceDexList.size());
    }

    while(pindexSeek->pprev && !mforkedServiceDexList.empty())
    {

        // return false;

        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                mapServiceDexListScriptPubKeys::iterator it = mforkedServiceDexList.find(key);
                if(it == mforkedServiceDexList.end())
                    continue;

                dexItem.insert(std::make_pair(key, std::make_tuple(DexToAddress(it), DexAddress(it), DexDescription(it))));
                mforkedServiceDexList.erase(it);
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
                    mapServiceDexListScriptPubKeys::iterator it = mforkedServiceDexList.find(key);
                    if(it == mforkedServiceDexList.end())
                        continue;
                    dexItem.insert(std::make_pair(it->first, std::make_tuple(DexToAddress(it), DexAddress(it), DexDescription(it))));
                    mforkedServiceDexList.erase(it);
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
            LogPrintf("UpdateDexListHeights(): Failed to read undo information\n");
            break;
        }
        pindexSeek = pindexSeek -> pprev;
    }

    bool ret;
    typedef std::pair<CScript, std::tuple<std::string, std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, dexItem) {
        ret = pcoinsTip->SetDexList(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateDexList(dexItem))
        return false;
    return mforkedServiceDexList.empty();
}

bool CServiceItemList::GetDexList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for(std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it=dexitems.begin(); it!=dexitems.end(); it++)
    {
        retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), get<2>(it->second))));
    }
    return true;
}

// Npo

bool CServiceItemList::UpdateNpoList(const std::map<CScript, std::tuple<std::string, std::string, std::string> > &map)
{
    for(std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        LogPrintStr("UPDATENPOLIST");
        LogPrintStr(it->first.ToString());
        LogPrintStr(get<0>(it->second));
        LogPrintStr(get<1>(it->second));
        LogPrintStr(get<2>(it->second));

        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();
        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            LogPrintStr(" OPCODE == OP_RETURN ");
            LogPrintStr(script.ToString());
            npoitems.insert(*it);
        }
    }
    return true;
}

// The heights need to be rolled back before new blocks are connected if any were disconnected.
// TODO: We should try to get rid of this and write the height undo information to the disk instead.
bool CServiceItemList::UpdateNpoListHeights()
{
    LogPrintStr("UPDATENPOLISTHEIGHTS");
    if(!fForked)
        return true;

    CBlockIndex* pindexSeek = mapBlockIndex.find(pcoinsTip->GetBestBlock())->second;
    if(!chainActive.Contains(pindexSeek)) {
        return false;
    }

    CBlock block;
    std::map<CScript, std::tuple<std::string, std::string, std::string> > npoItem;
    mapServiceNpoListScriptPubKeys mforkedServiceNpoList;

    for(mapServiceNpoListScriptPubKeys::const_iterator it = npoitems.begin(); it!=npoitems.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();

        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            npoitems.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedServiceNpoList.size());
    }

    while(pindexSeek->pprev && !mforkedServiceNpoList.empty())
    {

        // return false;

        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                mapServiceNpoListScriptPubKeys::iterator it = mforkedServiceNpoList.find(key);
                if(it == mforkedServiceNpoList.end())
                    continue;

                npoItem.insert(std::make_pair(key, std::make_tuple(NpoToAddress(it), NpoName(it), NpoAddress(it))));
                mforkedServiceNpoList.erase(it);
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
                    mapServiceNpoListScriptPubKeys::iterator it = mforkedServiceNpoList.find(key);
                    if(it == mforkedServiceNpoList.end())
                        continue;
                    npoItem.insert(std::make_pair(it->first, std::make_tuple(NpoToAddress(it), NpoName(it), NpoAddress(it))));
                    mforkedServiceNpoList.erase(it);
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
            LogPrintf("UpdateNpoListHeights(): Failed to read undo information\n");
            break;
        }
        pindexSeek = pindexSeek -> pprev;
    }

    bool ret;
    typedef std::pair<CScript, std::tuple<std::string, std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, npoItem) {
        ret = pcoinsTip->SetNpoList(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateNpoList(npoItem))
        return false;
    return mforkedServiceNpoList.empty();
}

bool CServiceItemList::GetNpoList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for(std::map<CScript, std::tuple<std::string, std::string, std::string> >::const_iterator it=npoitems.begin(); it!=npoitems.end(); it++)
    {
        retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second), get<2>(it->second))));
    }
    return true;
}

// Book

bool CServiceItemList::UpdateBookList(const std::map<CScript, std::tuple<std::string, std::string> > &map)
{
    for(std::map<CScript, std::tuple<std::string, std::string> >::const_iterator it = map.begin(); it!= map.end(); it++)
    {
        LogPrintStr("UPDATEBOOKLIST");
        LogPrintStr(it->first.ToString());
        LogPrintStr(get<0>(it->second));
        LogPrintStr(get<1>(it->second));

        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();
        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            LogPrintStr(" OPCODE == OP_RETURN ");
            LogPrintStr(script.ToString());
            bookitems.insert(*it);
        }
    }
    return true;
}

// The heights need to be rolled back before new blocks are connected if any were disconnected.
// TODO: We should try to get rid of this and write the height undo information to the disk instead.
bool CServiceItemList::UpdateBookListHeights()
{
    LogPrintStr("UPDATEBOOKLISTHEIGHTS");
    if(!fForked)
        return true;

    CBlockIndex* pindexSeek = mapBlockIndex.find(pcoinsTip->GetBestBlock())->second;
    if(!chainActive.Contains(pindexSeek)) {
        return false;
    }

    CBlock block;
    std::map<CScript, std::tuple<std::string, std::string> > bookItem;
    mapServiceBookListScriptPubKeys mforkedServiceBookList;

    for(mapServiceBookListScriptPubKeys::const_iterator it = bookitems.begin(); it!=bookitems.end(); it++)
    {
        CScript script = it->first;
        opcodetype opcode;
        CScript::const_iterator pc = script.begin();

        if(script.GetOp(pc, opcode) && opcode == OP_RETURN) {
            bookitems.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedServiceBookList.size());
    }

    while(pindexSeek->pprev && !mforkedServiceBookList.empty())
    {

        // return false;

        ReadBlockFromDisk(block,pindexSeek);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript key = tx.vout[j].scriptPubKey;
                mapServiceBookListScriptPubKeys::iterator it = mforkedServiceBookList.find(key);
                if(it == mforkedServiceBookList.end())
                    continue;

                bookItem.insert(std::make_pair(key, std::make_tuple(BookToAddress(it), BookChapter(it))));
                mforkedServiceBookList.erase(it);
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
                    mapServiceBookListScriptPubKeys::iterator it = mforkedServiceBookList.find(key);
                    if(it == mforkedServiceBookList.end())
                        continue;
                    bookItem.insert(std::make_pair(it->first, std::make_tuple(BookToAddress(it), BookChapter(it))));
                    mforkedServiceBookList.erase(it);
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
            LogPrintf("UpdateBookListHeights(): Failed to read undo information\n");
            break;
        }
        pindexSeek = pindexSeek -> pprev;
    }

    bool ret;
    typedef std::pair<CScript, std::tuple<std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, bookItem) {
        ret = pcoinsTip->SetBookList(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateBookList(bookItem))
        return false;
    return mforkedServiceBookList.empty();
}

bool CServiceItemList::GetBookList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string>>> &retset) const {
    for(std::map<CScript, std::tuple<std::string, std::string> >::const_iterator it=bookitems.begin(); it!=bookitems.end(); it++)
    {
        retset.insert(std::make_pair(it->first, std::make_tuple(get<0>(it->second), get<1>(it->second))));
    }
    return true;
}