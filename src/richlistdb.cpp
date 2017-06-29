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
    	//std::pair<int64_t, int> bh = std::make_pair(balance,height);
    	//int64_t r = balance; 
        return Write(scriptPubKey, par);// std::make_pair(balance, height));
    }
   
    bool ReadAddress(CScript &scriptPubKey, std::pair <int64_t,int>& par)// int64_t& balance, int &height)
    {
        //std::pair<int64_t, int> balanceheight = std::make_pair(balance, height);
        //int64_t kk = balance;
        return Read(scriptPubKey,par);
    }
    //Add new address, with its balance and height of block when last used:
    bool UpdateAddressBalance(std::string address, int64_t balance)
{
    //EraseAddress(address);
    //mymap[address].first = balance;
    //WriteAddress(address, mymap[address].first, mymap[address].second);
    return true;
}
bool UpdateAddressHeight(std::string address, int height)
{
  //  EraseAddress(address);
   // mymap[address].second = height;
    //WriteAddress(address, mymap[address].first, mymap[address].second);
    return true;
}
void SaveToMap(std::map<CScript,std::pair<int64_t,int> > pubmap)
{
    bool fAllAccounts = true;
    //std::cout<< "KK" << std::endl;
    Dbc* pcursor = GetCursor();
    if (!pcursor)
    {
    	//std::cout<< "KK8" << std::endl;
		return;
	}
    unsigned int fFlags = DB_SET_RANGE;
    //int minheight;
    bool first = true;
    CScript nextrichpubkey;
    loop
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
            //throw runtime_error("CWalletDB::ListAccountCreditDebit() : error scanning DB");
        }
        
        // Unserialize
        CScript pubkeytosave;
        ssKey >> pubkeytosave;
        std::pair<int64_t, int> balanceandheight;
        this->ReadAddress(pubkeytosave, balanceandheight);
        std::pair<CScript, std::pair<int64_t, int> > pairtosave = std::make_pair(pubkeytosave, balanceandheight);
        pubmap.insert(pairtosave);
    }
    
    
    //catch(DbException &e)
    //{
 	//}
       /* Dbt *s1;
        Dbt *pp;
        u_int32_t r = DB_NEXT;
        while ((pcursor->get(s1,pp,DB_NEXT))==0){
        	std::cout<< "h"<< std::endl;
        }

        // Unserialize
       /* std::string strType;
        ssKey >> strType;
        std:: pair <int64_t,int> acentry;
        //ssKey >> acentry.strAccount;
        //if (!fAllAccounts && acentry.strAccount != strAccount)
        //    break;

        ssValue >> acentry;
        //ssKey >> acentry.nEntryNo;
        //entries.push_back(acentry);
        std::cout << strType<< " : " << acentry.first << ": " << acentry.second << std::endl;
    }
*/
    pcursor->close();
    //return nextrichpubkey;
}

bool Exist(CScript s)
{
    return Exists(s);
}
//Update address balance?
//Update address age?
};

#endif


/*CRichListDB::CRichListDB()
{
    pathRichList = GetDataDir() / "richlist.dat";

}
bool CRichListDB::UpdateAddressBalance(CBitcoinAddress address, int64_t balance)
{
    EraseAddress(address, mymap[address].first, mymap[address].second);
    mymap[address].first = balance;
    WriteAddress(address, mymap[address].first, mymap[address].second);
}
bool CRichListDB::UpdateAddressHeight(CBitcoinAddress address, int height)
{
    EraseAddress(address, mymap[address].first, mymap[address].second);
    mymap[address].second = balance;
    WriteAddress(address, mymap[address].first, mymap[address].second);
}

void CRichListDB::Hey() {
	    std::cout <<"HEY"<< std::endl;
}*/
