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

	bool WriteAddress(CScript &scriptPubKey, std::pair<int64,int> & par)
    {
    	//std::pair<int64, int> bh = std::make_pair(balance,height);
    	//int64 r = balance; 
        return Write(scriptPubKey, par);// std::make_pair(balance, height));
    }
   
    bool ReadAddress(CScript &scriptPubKey, std::pair <int64,int>& par)// int64& balance, int &height)
    {
        //std::pair<int64, int> balanceheight = std::make_pair(balance, height);
        //int64 kk = balance;
        return Read(scriptPubKey,par);
    }
    //Add new address, with its balance and height of block when last used:
    bool UpdateAddressBalance(std::string address, int64 balance)
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
CScript NextRichPubkey()
{
    bool fAllAccounts = true;
    std::cout<< "KK" << std::endl;
    Dbc* pcursor = GetCursor();
    if (!pcursor)
    {
    	std::cout<< "KK8" << std::endl;
		//return;
	}
    unsigned int fFlags = DB_SET_RANGE;
    /* loop
    {
        std::cout<< "KK1" << std::endl;
        // Read next record
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);

        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        int ret = ReadAtCursor(pcursor, ssKey, ssValue);
        fFlags = DB_NEXT;
        if(ret==DB_NOTFOUND){
            std::cout << "LL "<< std::endl;
            return;
                 }

        else if (ret != 0)
        {
            std::cout<< "KK2" << std::endl; 
            pcursor->close();
            return;
        }
     }*/

    /*try
    {
        // Database open omitted for clarity
        // Get a cursor
        Dbt key, data;
        int ret;
        // Iterate over the database, retrieving each record in turn.
        while ((ret = pcursor->get(&key, &data, DB_NEXT)) == 0)
        {
            //str d[10] = key.det_data();
            //	std::cout << (std::string*)key.data<<std::endl;
            // Do interesting things with the Dbts here.
            std::cout << "MM" << std::endl;
            CScript pubkeytoprint;
            pubkeytoprint.deserialize(key.get_data());
            //void *pubkeytovoid = key.get_data();
            //char *pubkeytostring = (char *)pubkeytovoid;
            //std::vector<unsigned char> pubkeyfromhex = ParseHex(pubkeytostring);
            //PrintHex(*pubkeyfromhex);
            //HexStr(*pubkeytoprint).c_str()
            std::cout << pubkeytoprint.ToString() << std::endl;
            //CScript *pubkeytoprint = (CScript *)pubkeytovoid;
            //std::cout << ParseHex(pubkeytostring) << std::endl;
         }
         if (ret != DB_NOTFOUND)
         {
             // ret should be DB_NOTFOUND upon exiting the loop.
             // Dbc::get() will by default throw an exception if any
             // significant errors occur, so by default this if block
             // can never be reached.
             std::cout << "LL" << std::endl;
         }
    }*/
    int minheight = 1000000;
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
        CScript pubkeytoprint;
        ssKey >> pubkeytoprint;
        std::pair<int64, int> balanceandheight;
        this->ReadAddress(pubkeytoprint, balanceandheight);
        //ssValue >> balanceandheight;
        if(balanceandheight.first >= 2500000000000000 && balanceandheight.second < minheight && balanceandheight.second > 0)
        {
            //std::cout << "ASDF" << std::endl;
            minheight = balanceandheight.second;
            nextrichpubkey = pubkeytoprint;
            //std::cout << nextrichpubkey.ToString() << std::endl;
        }
        //std::cout << pubkeytoprint.ToString() << " " << std::to_string(balanceandheight.first) << " " << std::to_string(balanceandheight.second) << std::endl;
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
        std:: pair <int64,int> acentry;
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
    return nextrichpubkey;
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
bool CRichListDB::UpdateAddressBalance(CBitcoinAddress address, int64 balance)
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
