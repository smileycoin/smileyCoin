//
// Created by Lenovo on 4/24/2020.
//

#ifndef SMILEYCOIN_SERVICELISTDB_H
#define SMILEYCOIN_SERVICELISTDB_H

#include "txdb.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>


class CServiceList;
extern CServiceList ServiceList;

typedef std::tuple<std::string, std::string, std::string> CServiceInfo;
typedef std::map<std::string, CServiceInfo> mapServiceList;

class CServiceList
{
private:
    mapServiceList saddresses;
    bool fForked;

public:
    friend bool CCoinsViewDB::GetServiceAddresses(CServiceList &servicelist);
    bool GetServiceAddresses(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool GetMyServiceAddresses(std::multiset<std::pair<std::string, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool IsService(std::string address);
    bool SetForked(const bool &fFork);
    bool IsForked(){return fForked;}
    bool UpdateServiceInfo(const std::map<std::string, std::tuple<std::string, std::string, std::string> > &map);
    bool UpdateServiceAddressHeights();
};


#endif //SMILEYCOIN_SERVICELISTDB_H
