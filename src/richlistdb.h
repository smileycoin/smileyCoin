// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


//#include "richlistdb.h"
//#include "init.h"

#ifndef BITCOIN_RICHLISTDB_H
#define BITCOIN_RICHLISTDB_H

#include "txdb.h"
#include "init.h" //script og string
#include "core.h"

class CRichList;
extern CRichList RichList;

struct RichOrderCompare
{
    bool operator()(const std::pair< CScript, std::pair<int64_t, int> >& lhs, const std::pair< CScript, std::pair<int64_t, int> >& rhs)
    {
        return lhs.second.second < rhs.second.second;
    }
};

typedef std::pair<int64_t, int> CAddressIndex;
typedef std::map< CScript, CAddressIndex> mapScriptPubKeys;

class CRichList
{
private:
	mapScriptPubKeys maddresses;
	bool fForked; // TODO: skrifa frekar flag í blocktreedb?

	bool IsRelevant(const CScript &scriptpubkey) const { return scriptpubkey.IsPayToPublicKeyHash() || scriptpubkey.IsPayToScriptHash(); }
	bool IsRich(const mapScriptPubKeys::iterator &it) const { return it->second.first >= 25000000*COIN; }
	CScript ScriptPubKey(const mapScriptPubKeys::iterator &it) const { return it -> first; }
	int Height(const mapScriptPubKeys::iterator &it) const { return it -> second.second; }
	int64_t Balance(const mapScriptPubKeys::iterator &it) const { return it -> second.first; }
	int Height(const mapScriptPubKeys::const_iterator &it) const { return it -> second.second; }
	int64_t Balance(const mapScriptPubKeys::const_iterator &it) const { return it -> second.first; }

public:

	friend bool CCoinsViewDB::GetRichAddresses(CRichList &richlist);
	bool GetRichAddresses(std::multiset< std::pair< CScript, std::pair<int64_t, int> >, RichOrderCompare > &retset) const;

	bool SetForked(const bool &fFork);
	bool IsForked(){return fForked;}
// henda út?
	bool GetHeight(const CScript &scriptpubkey, int &nHeight);

	bool GetBalance(const CScript &scriptpubkey, int64_t &nBalance);
	bool UpdateAddressIndex(const std::map<CScript, std::pair<int64_t, int> > &map);
	bool UpdateRichAddressHeights();


	bool NextRichScriptPubKey(CScript &scriptpubkey);

	CScript NextEIASScriptPubKey(const int &nHeight);

};




#endif
