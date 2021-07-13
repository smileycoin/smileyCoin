#ifndef SMILEYCOIN_SERVICEITEMLISTDB_H
#define SMILEYCOIN_SERVICEITEMLISTDB_H

#include "txdb.h"

class CServiceItemList;
extern CServiceItemList ServiceItemList;

typedef std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> CServiceCoupon;
typedef std::tuple<std::string, std::string> CServiceUbi;
typedef std::tuple<std::string, std::string, std::string> CServiceDex;
typedef std::tuple<std::string, std::string, std::string> CServiceBook;
typedef std::tuple<std::string, std::string, std::string> CServiceNP;

typedef std::map<std::string, CServiceCoupon> mapServiceCouponList;
typedef std::map<std::string, CServiceUbi> mapServiceUbiList;
typedef std::map<std::string, CServiceDex> mapServiceDexList;
typedef std::map<std::string, CServiceBook> mapServiceBookList;
typedef std::map<std::string, CServiceNP> mapServiceNPList;

class CServiceItemList
{
private:
    mapServiceCouponList taddresses;
    mapServiceUbiList uaddresses;
    mapServiceDexList daddresses;
    mapServiceBookList baddresses;
    mapServiceNPList naddresses;
    bool fForked;

    std::string CouponAddress(const mapServiceCouponList::iterator &it) const { return it -> first; }
    std::string CouponAction(const mapServiceCouponList::iterator &it) const { return get<0>(it -> second); }
    std::string CouponToAddress(const mapServiceCouponList::iterator &it) const { return get<1>(it -> second); }
    std::string CouponLocation(const mapServiceCouponList::iterator &it) const { return get<2>(it -> second); }
    std::string CouponName(const mapServiceCouponList::iterator &it) const { return get<3>(it -> second); }
    std::string CouponDateAndTime(const mapServiceCouponList::iterator &it) const { return get<4>(it -> second); }
    std::string CouponValue(const mapServiceCouponList::iterator &it) const { return get<5>(it -> second); }

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

    std::string NPAddress(const mapServiceNPList::iterator &it) const { return it -> first; }
    std::string NPAction(const mapServiceNPList::iterator &it) const { return get<0>(it -> second); }
    std::string NPToAddress(const mapServiceNPList::iterator &it) const { return get<1>(it -> second); }
    std::string NPDescription(const mapServiceNPList::iterator &it) const { return get<2>(it -> second); }

public:

    friend bool CCoinsViewDB::GetCouponList(CServiceItemList &couponlist);
    friend bool CCoinsViewDB::GetUbiList(CServiceItemList &ubilist);
    friend bool CCoinsViewDB::GetDexList(CServiceItemList &dexlist);
    friend bool CCoinsViewDB::GetBookList(CServiceItemList &booklist);
    friend bool CCoinsViewDB::GetNPList(CServiceItemList &nplist);

    bool GetCouponList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> &retset) const;
    bool IsCoupon(std::string address);
    bool GetUbiList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string>>> &retset) const;
    bool IsUbi(std::string address);
    bool GetDexList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool IsDex(std::string address);
    bool GetBookList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool IsChapter(std::string address);
    bool GetNPList(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool IsNP(std::string address);


    bool SetForked(const bool &fFork);
    bool IsForked(){return fForked;}

    bool UpdateCouponList(const std::map<std::string, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &map);
    bool UpdateUbiList(const std::map<std::string, std::tuple<std::string, std::string> > &map);
    bool UpdateDexList(const std::map<std::string, std::tuple<std::string, std::string, std::string> > &map);
    bool UpdateBookList(const std::map<std::string, std::tuple<std::string, std::string, std::string> > &map);
    bool UpdateNPList(const std::map<std::string, std::tuple<std::string, std::string, std::string> > &map);

    bool UpdateCouponListHeights();
    bool UpdateUbiListHeights();
    bool UpdateDexListHeights();
    bool UpdateBookListHeights();
    bool UpdateNPListHeights();
};


#endif //SMILEYCOIN_SERVICEITEMLISTDB_H
