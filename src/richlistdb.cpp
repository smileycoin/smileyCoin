// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


//#include "richlistdb.h"
//#include "init.h"

#ifndef BITCOIN_RICHLISTDB_H
#define BITCOIN_RICHLISTDB_H

#include "db.h"
#include "init.h"


#include <boost/version.hpp>
#include <boost/filesystem.hpp>

class CRichListDB : public CDB
{
public:
    CRichListDB(std::string strFilename, const char* pszMode="r+") : CDB(strFilename.c_str(), pszMode)
    {
    }
private:
    CRichListDB(const CRichListDB&);
    void operator=(const CRichListDB&);
public:
    bool EraseAddress(CScript scriptPubKey)
    {
        return Erase(scriptPubKey);
    }

	bool WriteAddress(CScript &scriptPubKey, std::pair<int64_t,int> & par)
    {
        return Write(scriptPubKey, par);
    }
   
    bool ReadAddress(CScript &scriptPubKey, std::pair <int64_t,int>& par)// int64_t& balance, int &height)
    {
        return Read(scriptPubKey,par);
    }
    
    /*The primary use of SaveToMap is to save the existing richlist.dat file to a map which is used to maintain the
      richlist while the node is running. However, it also finds at what height in the blockchain richlist.dat was 
      updated last, and in init.cpp we check if this height matches the best block at startup. It should do that, 
      but in case of an unexpected shutdown, richlist.dat might be behind. In that case it is resolved in init.*/
    void SaveToMap(std::map<CScript,std::pair<int64_t,int> > &pubmap, int &maxheight)
    {
        Dbc* pcursor = GetCursor();
        if (!pcursor)
        {
            return;
        }
        unsigned int fFlags = DB_SET_RANGE;
        maxheight = 0;
        while (true)
        {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
            fFlags = DB_NEXT;
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
            {
                pcursor->close();
                throw runtime_error("CWalletDB::ListAccountCreditDebit() : error scanning DB");
            }
            
            // Unserialize
            CScript pubkeytosave;
            ssKey >> pubkeytosave;
            std::pair<int64_t, int> balanceandheight;
            this->ReadAddress(pubkeytosave, balanceandheight);
            if(balanceandheight.second > maxheight)
            {
                maxheight = balanceandheight.second;
            }
            std::pair<CScript, std::pair<int64_t, int> > pairtosave = std::make_pair(pubkeytosave, balanceandheight);
            pubmap.insert(pairtosave);
        }
        pcursor->close();
    }

    bool Exist(CScript s)
    {
        return Exists(s);
    }
};

#endif
