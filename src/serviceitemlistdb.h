#ifndef SMILEYCOIN_SERVICEITEMLISTDB_H
#define SMILEYCOIN_SERVICEITEMLISTDB_H

#include "txdb.h"
#include "init.h"
#include "core.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>


class CServiceItemList;
extern CServiceItemList ServiceItemList;

typedef std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> CServiceTicket;
typedef std::tuple<std::string, std::string> CServiceUbi;
typedef std::tuple<std::string, std::string, std::string> CServiceDex;
typedef std::tuple<std::string, std::string, std::string> CServiceNpo;
typedef std::tuple<std::string, std::string> CServiceBook;

typedef std::map<CScript, CServiceTicket> mapServiceTicketListScriptPubKeys;
typedef std::map<CScript, CServiceUbi> mapServiceUbiListScriptPubKeys;
typedef std::map<CScript, CServiceDex> mapServiceDexListScriptPubKeys;
typedef std::map<CScript, CServiceNpo> mapServiceNpoListScriptPubKeys;
typedef std::map<CScript, CServiceBook> mapServiceBookListScriptPubKeys;

class CServiceItemList
{
private:
    mapServiceTicketListScriptPubKeys ticketitems;
    mapServiceUbiListScriptPubKeys ubiitems;
    mapServiceDexListScriptPubKeys dexitems;
    mapServiceNpoListScriptPubKeys npoitems;
    mapServiceBookListScriptPubKeys bookitems;
    bool fForked;

    CScript ScriptTicketPubKey(const mapServiceTicketListScriptPubKeys::iterator &it) const { return it -> first; }
    std::string TicketToAddress(const mapServiceTicketListScriptPubKeys::iterator &it) const { return get<0>(it -> second); }
    std::string TicketLocation(const mapServiceTicketListScriptPubKeys::iterator &it) const { return get<1>(it -> second); }
    std::string TicketName(const mapServiceTicketListScriptPubKeys::iterator &it) const { return get<2>(it -> second); }
    std::string TicketDateAndTime(const mapServiceTicketListScriptPubKeys::iterator &it) const { return get<3>(it -> second); }
    std::string TicketValue(const mapServiceTicketListScriptPubKeys::iterator &it) const { return get<4>(it -> second); }
    std::string TicketAddress(const mapServiceTicketListScriptPubKeys::iterator &it) const { return get<5>(it -> second); }

    CScript ScriptServiceUbiPubKey(const mapServiceUbiListScriptPubKeys::iterator &it) const { return it -> first; }
    std::string UbiToAddress(const mapServiceUbiListScriptPubKeys::iterator &it) const { return get<0>(it -> second); }
    std::string UbiAddress(const mapServiceUbiListScriptPubKeys::iterator &it) const { return get<1>(it -> second); }
  
    CScript ScriptServiceDexPubKey(const mapServiceDexListScriptPubKeys::iterator &it) const { return it -> first; }
    std::string DexToAddress(const mapServiceDexListScriptPubKeys::iterator &it) const { return get<0>(it -> second); }
    std::string DexAddress(const mapServiceDexListScriptPubKeys::iterator &it) const { return get<1>(it -> second); }
    std::string DexDescription(const mapServiceDexListScriptPubKeys::iterator &it) const { return get<2>(it -> second); }

    CScript ScriptServiceNpoPubKey(const mapServiceNpoListScriptPubKeys::iterator &it) const { return it -> first; }
    std::string NpoToAddress(const mapServiceNpoListScriptPubKeys::iterator &it) const { return get<0>(it -> second); }
    std::string NpoName(const mapServiceNpoListScriptPubKeys::iterator &it) const { return get<1>(it -> second); }
    std::string NpoAddress(const mapServiceNpoListScriptPubKeys::iterator &it) const { return get<2>(it -> second); }
  
    CScript ScriptServiceBookPubKey(const mapServiceBookListScriptPubKeys::iterator &it) const { return it -> first; }
    std::string BookToAddress(const mapServiceBookListScriptPubKeys::iterator &it) const { return get<0>(it -> second); }
    std::string BookChapter(const mapServiceBookListScriptPubKeys::iterator &it) const { return get<1>(it -> second); }
  
public:

    friend bool CCoinsViewDB::GetTicketList(CServiceItemList &ticketlist);
    
    friend bool CCoinsViewDB::GetUbiList(CServiceItemList &ubilist);
    
    friend bool CCoinsViewDB::GetDexList(CServiceItemList &dexlist);
    
    friend bool CCoinsViewDB::GetNpoList(CServiceItemList &npolist);

    friend bool CCoinsViewDB::GetBookList(CServiceItemList &booklist);

    bool GetTicketList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> &retset) const;
    
    bool GetUbiList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string>>> &retset) const;
    
    bool GetDexList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const;
    
    bool GetNpoList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const;
    
    bool GetBookList(std::multiset<std::pair<CScript, std::tuple<std::string, std::string>>> &retset) const;
    
    bool SetForked(const bool &fFork);
    
    bool IsForked(){return fForked;}

    bool UpdateTicketList(const std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &map);

    bool UpdateUbiList(const std::map<CScript, std::tuple<std::string, std::string> > &map);

    bool UpdateDexList(const std::map<CScript, std::tuple<std::string, std::string, std::string> > &map);

    bool UpdateNpoList(const std::map<CScript, std::tuple<std::string, std::string, std::string> > &map);

    bool UpdateBookList(const std::map<CScript, std::tuple<std::string, std::string> > &map);

    bool UpdateTicketListHeights();
    
    bool UpdateUbiListHeights();

    bool UpdateDexListHeights();
    
    bool UpdateNpoListHeights();
    
    bool UpdateBookListHeights();
};


#endif //SMILEYCOIN_SERVICEITEMLISTDB_H