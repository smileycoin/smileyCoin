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
            mapServiceList::iterator itService = saddresses.find(it->first);
            // If key is found in service list
            if (itService != saddresses.end()) {
                saddresses.erase(itService);
            }
        } else if (get<0>(it->second) == "NS") { // If op_return begins with NS (new service)
            saddresses.insert(*it);
        }
    }
    return true;
}

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
    mapServiceList mforkedAddresses;

    for(mapServiceList::const_iterator it = saddresses.begin(); it!=saddresses.end(); it++)
    {
        if (get<0>(it->second) == "DS") { // If op_return begins with DS
            mapServiceList::iterator itService = saddresses.find(it->first);
            // If key is found in service list
            if (itService != saddresses.end()) {
                saddresses.erase(itService);
            }
        } else if (get<0>(it->second) == "NS") { // If op_return begins with NS
            saddresses.insert(*it);
        }
    }

    if(fDebug) {
        LogPrintf("%d addresses seen at fork and need to be relocated\n", mforkedAddresses.size());
    }

    bool ret;
    typedef std::pair<std::string, std::tuple<std::string, std::string, std::string> > pairType;
    BOOST_FOREACH(const pairType &pair, serviceInfo) {
        ret = pcoinsTip->SetServiceInfo(pair.first, pair.second);
        assert(ret);
    }

    if(!UpdateServiceInfo(serviceInfo))
        return false;
    return mforkedAddresses.empty();
}

bool CServiceList::GetServiceAddresses(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for(std::map<std::string, std::tuple<std::string, std::string, std::string> >::const_iterator it=saddresses.begin(); it!=saddresses.end(); it++)
    {
        std::string displayType;
        // If op_return begins with NS (new service)
        //if(get<0>(it->second) == "NS")
        //{
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
        //}
    }
    return true;
}

bool CServiceList::GetMyServiceAddresses(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const {
    for (std::map<std::string, std::tuple<std::string, std::string, std::string> >::const_iterator it = saddresses.begin();it != saddresses.end(); it++)
    {
        std::string displayType;

        if (IsMine(*pwalletMain, CBitcoinAddress(it->first).Get())) {
            // If op_return begins with NS (new service)
            //if(get<0>(it->second) == "NS")
            //{
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
            //}
        }
    }
    return true;
}

bool CServiceList::IsService(std::string address) {
    for (std::map<std::string, std::tuple<std::string, std::string, std::string> >::const_iterator it = saddresses.begin();it != saddresses.end(); it++)
    {
        // If address found on service list
        if (address == it->first) {
            return true;
        }
    }
    return false;
}