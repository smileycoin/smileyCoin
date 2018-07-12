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

typedef std::pair<int64_t, int> CAddressIndex;
typedef std::map< CScript, CAddressIndex> mapScriptPubKeys;

class CRichList
{
private:
	mapScriptPubKeys maddresses;

	bool IsRelevant(const CScript &scriptpubkey) const { return scriptpubkey.IsPayToPublicKeyHash() || scriptpubkey.IsPayToScriptHash(); }

	CScript ScriptPubKey(const CRichListIterator &it) const { return it -> first; }

	int Height(const CRichListIterator &it) const { return it -> second.second; }

	int64_t Balance(const CRichListIterator &it) const { return it -> second.first; }

	void SetAddressHeight(const CRichListIterator &it, const int &nHeight) { it -> second.second = nHeight; }

	void AddAddressBalance(const CRichListIterator &it, const int64_t &nValue) { it -> second.first += nValue; }

	bool UpdateAddressIndex(const CScript &scriptpubkey, const int64_t &nValue, const int &nHeight, const bool &fCoinBase, const bool fUndo);


public:

	friend void CBlockTreeDB::ReadRichAddresses(CRichList &richlist);
// henda út?
	bool GetHeight(const CScript &scriptpubkey, int &nHeight);

	bool GetBalance(const CScript &scriptpubkey, int64_t &nBalance);

	bool UpdateRichAddressHeights();

	bool NextRichScriptPubKey(CScript &scriptpubkey);

	CScript NextEIASScriptPubKey(const int &nHeight);

};




#endif
