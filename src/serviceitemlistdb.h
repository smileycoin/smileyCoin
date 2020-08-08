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
typedef std::tuple<std::string, std::string, std::string> CServiceBook;

typedef std::map<std::string, CServiceTicket> mapServiceTicketList;
typedef std::map<std::string, CServiceUbi> mapServiceUbiList;
typedef std::map<std::string, CServiceDex> mapServiceDexList;
typedef std::map<std::string, CServiceBook> mapServiceBookList;

class CServiceItemList
{
private:
    mapServiceTicketList taddresses;
    mapServiceUbiList uaddresses;
    mapServiceDexList daddresses;
    mapServiceBookList baddresses;
    bool fForked;

    std::string TicketAddress(const mapServiceTicketList::iterator &it) const { return it -> first; }
    std::string TicketAction(const mapServiceTicketList::iterator &it) const { return get<0>(it -> second); }
    std::string TicketToAddress(const mapServiceTicketList::iterator &it) const { return get<1>(it -> second); }
    std::string TicketLocation(const mapServiceTicketList::iterator &it) const { return get<2>(it -> second); }
    std::string TicketName(const mapServiceTicketList::iterator &it) const { return get<3>(it -> second); }
    std::string TicketDateAndTime(const mapServiceTicketList::iterator &it) const { return get<4>(it -> second); }
    std::string TicketValue(const mapServiceTicketList::iterator &it) const { return get<5>(it -> second); }

    std::string UbiAddress(const mapServiceUbiList::iterator &it) const { return it -> first; }
    std::string UbiAction(const mapServiceUbiList::iterator &it) const { return get<0>(it -> second); }
    std::string UbiToAddress(const mapServiceUbiList::iterator &it) const { return get<1>(it -> second); }

    std::string DexAddress(const mapServiceDexList::iterator &it) const { return it -> first; }
    std::string DexAction(const mapServiceDexList::iterator &it) const { return get<0>(it -> second); }
    std::string DexToAddress(const mapServiceDexList::iterator &it) const { return get<1>(it -> second); }
    std::string DexDescription(const mapServiceDexList::iterator &it) const { return get<2>(it -> second); }

    std::string ChapterAddress(const mapServiceBookList::iterator &it) const { return it -> first; }
    std::string ChapterAction(const mapServiceBookList::iterator &it) const { return get<0>(it -> second); }
    std::string ChapterToAddress(const mapServiceBookList::iterator &it) const { return get<1>(it -> second); }
    std::string ChapterNum(const mapServiceBookList::iterator &it) const { return get<2>(it -> second); }

public:

    friend bool CCoinsViewDB::GetTicketList(CServiceItemList &ticketlist);
    friend bool CCoinsViewDB::GetUbiList(CServiceItemList &ubilist);
    friend bool CCoinsViewDB::GetDexList(CServiceItemList &dexlist);
    friend bool CCoinsViewDB::GetBookList(CServiceItemList &booklist);

    bool GetTicketList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> &retset) const;
    bool GetUbiList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string>>> &retset) const;
    bool GetDexList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool GetBookList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const;
    
    bool SetForked(const bool &fFork);
    bool IsForked(){return fForked;}

    bool UpdateTicketList(const std::map<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &map);
    bool UpdateUbiList(const std::map<std::string, std::tuple<std::string, std::string> > &map);
    bool UpdateDexList(const std::map<std::string, std::tuple<std::string, std::string, std::string> > &map);
    bool UpdateBookList(const std::map<std::string, std::tuple<std::string, std::string, std::string> > &map);

    bool UpdateTicketListHeights();
    bool UpdateUbiListHeights();
    bool UpdateDexListHeights();
    bool UpdateBookListHeights();
};


#endif //SMILEYCOIN_SERVICEITEMLISTDB_H