// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "richlistdb.h"

#include "main.h"
#include "base58.h"

#include <vector>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>

const string EIASaddresses[10] = {"BEaZDZ8gCbbP1y3t2gPNKwqZa76rUDfR73",
                                  "BDwfNiAvpb4ipNfPkdAXcPGExWHeeMdRcK",
                                  "BPVuYwyeJiXExEmEXHfCwPtRXDRqxBxTNW",
                                  "B4gB18iZWZ8nTuAvi9kq9cWbavCj6xSmny",
                                  "BGFEYswtWfo5nrKRT53ToGwZWyuRvwC8xs",
                                  "B7WWgP1fnPDHTL2z9vSHTpGqptP53t1UCB",
                                  "BEtL36SgjYuxHuU5dg8omJUAyTXDQL3Z8V",
                                  "BQaNeMcSyrzGkeKknjw6fnCSSLUYAsXCVd",
                                  "BDLAaqqtBNoG9EjbJCeuLSmT5wkdnSB8bc",
                                  "BQTar7kTE2hu4f4LrRmomjkbsqSW9rbMvy"};

inline int Height(const CAddressIndex &ai) {return ai.second;}
inline int Balance(const CAddressIndex &ai) {return ai.first;}

bool CRichList::GetBalance(const CScript &scriptpubkey, int64_t &nBalance)
{
    CRichListIterator it = maddresses.find(scriptpubkey);
    if(it!=maddresses.end())
    {
        nBalance = Balance(it);
        return true;
    }
    else
    {
        nBalance = 0;
        return false;
    }
}

bool CRichList::GetHeight(const CScript &scriptpubkey, int &nHeight)
{
    CRichListIterator it = maddresses.find(scriptpubkey);
    if(it!=maddresses.end())
    {
        nHeight = Height(it);
        return true;
    }
    else
    {
        nHeight = -1;
        return false;
    }
}

bool CRichList::NextRichScriptPubKey(CScript &scriptpubkey)
{
    //TODO:
    // pass prevblocks hash to check against the current tip?
    // return a NULL CScript instead of false?
    if(maddresses.empty())
        return false;
    int minheight = 0;
    bool fFirst = true;    
    for(mapScriptPubKeys::const_iterator it = maddresses.begin(); it != maddresses.end(); it++)
    {
        if(fFirst || it->second.first <= minheight)
        {
            scriptpubkey = it->first;
            minheight = it->second.first;
            fFirst = false;         
        }
    }
    return true;
}

CScript CRichList::NextEIASScriptPubKey(const int &nHeight){
    CScript ret;
    ret.SetDestination(CBitcoinAddress(EIASaddresses[nHeight % 10]).Get());
    return ret;
}

bool CRichList::UpdateRichAddressHeights()
{
    CBlockIndex *ind = chainActive.Tip();
    CBlock block;

    std::map<CScript,CRichListIterator> mrichfork;
    for(std::map<CScript,CRichListIterator>::iterator it = mrich.begin(); it!=mrich.end(); it++)
    {
        if(Height(it->second) > ind->nHeight)
            mrichfork.insert(*it);
    }

    if(fDebug)
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mrichfork.size());

    while(ind->pprev && !mrichfork.empty())
    {       
        ReadBlockFromDisk(block,ind);
        block.BuildMerkleTree();
        BOOST_FOREACH(const CTransaction &tx, block.vtx)
        {
            for(unsigned int j = 0; j < tx.vout.size(); j++)
            {
                CScript scriptpubkey = tx.vout[j].scriptPubKey;
                std::map<CScript,CRichListIterator>::iterator it = mrichfork.find(scriptpubkey);
                if(it!=mrichfork.end())
                {   // in principle we shouldn't update heights unless there is voluntary movement or an address is becoming rich
                    // and therefore not when an address receives smlys, except from the coinbase.
                    CRichListIterator itRich = it->second;
                    if(ind->nHeight < nRichForkV2Height 
                        || tx.IsCoinBase()
                        || (IsRich(itRich) && Balance(itRich) - tx.vout[j].nValue < 25000000*COIN) ) // if it isn't currently rich but was once the output will be caught first in the undo block
                    {
                        SetAddressHeight(itRich, ind->nHeight);
                        mrichfork.erase(it);
                        if(fDebug)
                        {
                            CTxDestination dest;
                            ExtractDestination(scriptpubkey, dest);
                            CBitcoinAddress addr;
                            addr.Set(dest);
                            LogPrintf("%s found at height %d\n",addr.ToString(),ind->nHeight);  
                        }   
                    }                           
                }
            }
        }

        CBlockUndo undo;
        CDiskBlockPos pos = ind ->GetUndoPos();
        if (undo.ReadFromDisk(pos, ind->pprev->GetBlockHash())) //TODO: hvenær klikkar þetta?
        {
            for (unsigned int i=0; i<undo.vtxundo.size(); i++)
            {
                for (unsigned int j=0; j<undo.vtxundo[i].vprevout.size(); j++)
                {
                    CScript scriptpubkey = undo.vtxundo[i].vprevout[j].txout.scriptPubKey;
                    std::map<CScript,CRichListIterator>::iterator it = mrichfork.find(scriptpubkey);
                    if(it!=mrichfork.end())
                    {
                        SetAddressHeight(it->second, ind->nHeight);
                        mrichfork.erase(it);
                        if(fDebug)
                        {
                            CTxDestination dest;
                            ExtractDestination(scriptpubkey, dest);
                            CBitcoinAddress addr;
                            addr.Set(dest);
                            LogPrintf("%s found at height %d\n",addr.ToString(),ind->nHeight);  
                        }              
                    }
                }
            }
        }
        ind = ind -> pprev;
    }
    return mrichfork.empty();
}



bool CRichList::UpdateAddressIndex(const CScript &scriptpubkey, const int64_t &nValue, const int &nHeight, const bool &fCoinBase, const bool fUndo)
{
    if(nValue == 0 || !IsRelevant(scriptpubkey))
        return true;
    CRichListIterator it = maddresses.find(scriptpubkey);
    if(nValue > 0)
    {
        if(it!=maddresses.end())
        {
            AddAddressBalance(it, nValue);
            bool fBecameRich = IsRich(it) && Balance(it) - nValue < 25000000*COIN;
            // In principle we shouldn't update heights unless there is voluntary movement or an address is becoming rich
            // and therefore not when an address receives smlys, except from the coinbase.
            // Still, we can't set correct height while undoing; taken care of later.
            if( !fUndo &&  
                ( nHeight < nRichForkV2Height || fCoinBase || fBecameRich )
                )
                SetAddressHeight(it, nHeight);
            if(fBecameRich)
                mrich.insert(std::make_pair(scriptpubkey,it));
        }
        else
        {
            std::pair<CRichListIterator,bool> ret = maddresses.insert(std::make_pair(scriptpubkey, std::make_pair(nValue, nHeight)));
            if(nValue >= 25000000*COIN)
                mrich.insert(std::make_pair(scriptpubkey,ret.first));
        }
    }
    else
    {
        if(it==maddresses.end())
            return false; // something is wrong, can't subtract from unknown address
        AddAddressBalance(it, nValue);
        bool fBecamePoor = !IsRich(it) && Balance(it) - nValue >= 25000000*COIN; //nValue is negative
        if(fBecamePoor)
            mrich.erase(scriptpubkey);
        if(Balance(it)==0)
            maddresses.erase(it);
        else if(!fUndo) // can't set correct height while undoing; taken care of later
            SetAddressHeight(it, nHeight);
    } 
    return true;
}
