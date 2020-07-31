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
typedef std::map< CScript, CServiceInfo> mapServiceScriptPubKeys;

typedef std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> CServiceAddressInfo;
typedef std::map<CScript, CServiceAddressInfo> mapServiceInfoScriptPubKeys;

class CServiceList
{
private:
    mapServiceScriptPubKeys maddresses;
    mapServiceInfoScriptPubKeys infoaddress;
    bool fForked;

    CScript ScriptPubKey(const mapServiceScriptPubKeys::iterator &it) const { return it -> first; }
    std::string ServiceAddress(const mapServiceScriptPubKeys::iterator &it) const { return get<0>(it -> second); }
    std::string ServiceName(const mapServiceScriptPubKeys::iterator &it) const { return get<1>(it -> second); }
    std::string ServiceType(const mapServiceScriptPubKeys::iterator &it) const { return get<2>(it -> second); }

    CScript ScriptInfoPubKey(const mapServiceInfoScriptPubKeys::iterator &it) const { return it -> first; }
    std::string ServiceInfoToAddress(const mapServiceInfoScriptPubKeys::iterator &it) const { return get<0>(it -> second); }
    std::string ServiceInfoLocation(const mapServiceInfoScriptPubKeys::iterator &it) const { return get<1>(it -> second); }
    std::string ServiceInfoName(const mapServiceInfoScriptPubKeys::iterator &it) const { return get<2>(it -> second); }
    std::string ServiceInfoDateAndTime(const mapServiceInfoScriptPubKeys::iterator &it) const { return get<3>(it -> second); }
    std::string ServiceInfoValue(const mapServiceInfoScriptPubKeys::iterator &it) const { return get<4>(it -> second); }
    std::string ServiceInfoAddress(const mapServiceInfoScriptPubKeys::iterator &it) const { return get<5>(it -> second); }


public:

    friend bool CCoinsViewDB::GetServiceAddresses(CServiceList &servicelist);
    friend bool CCoinsViewDB::GetServiceAddressInfo(CServiceList &servicelist);

    bool GetServiceAddresses(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool GetMyServiceAddresses(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string>>> &retset) const;
    bool GetServiceAddressInfo(std::multiset<std::pair<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>>> &info) const;

    bool SetForked(const bool &fFork);
    bool IsForked(){return fForked;}

    bool UpdateServiceInfo(const std::map<CScript, std::tuple<std::string, std::string, std::string> > &map);
    bool UpdateServiceAddressInfo(const std::map<CScript, std::tuple<std::string, std::string, std::string, std::string, std::string, std::string> > &map);

    bool UpdateServiceAddressHeights();
    bool UpdateServiceAddressInfoHeights();
};


#endif //SMILEYCOIN_SERVICELISTDB_H
