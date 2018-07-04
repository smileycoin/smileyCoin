// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


//#include "richlistdb.h"
//#include "init.h"

#ifndef BITCOIN_RICHLISTDB_H
#define BITCOIN_RICHLISTDB_H

#include "db.h"
#include "init.h" //script og string
#include "core.h"

typedef std::pair<int64_t, int> CAddressIndex;
typedef std::map< CScript, CAddressIndex> mapScriptPubKeys;
typedef std::map< CScript, CAddressIndex>::iterator CRichListIterator;

class CRichList;

extern CRichList RichList;

class CRichListDB : public CDB
{
public: 
    CRichListDB(std::string strFilename, const char* pszMode="r+") : CDB(strFilename.c_str(), pszMode) {}

private:
    CRichListDB(const CRichListDB&); //TODO: tilgangslaust?

    void operator=(const CRichListDB&); //TODO: tilgangslaust?


public:
    bool EraseAddress(const CScript scriptpubkey) { return Erase(scriptpubkey); }

	bool WriteAddress(const CScript &scriptpubkey, const CAddressIndex &ai) { return CDB::Write(scriptpubkey, ai); }
   
    bool ReadAddress(CScript &scriptpubkey, CAddressIndex &ai) { return CDB::Read(scriptpubkey,ai); }
    
    /*The primary use of SaveToMap is to save the existing richlist.dat file to a map which is used to maintain the
      richlist while the node is running. However, it also finds at what height in the blockchain richlist.dat was 
      updated last, and in init.cpp we check if this height matches the best block at startup. It should do that, 
      but in case of an unexpected shutdown, richlist.dat might be behind. In that case it is resolved in init.*/
    void SaveToRichList(CRichList &richlist, int &bestheight);

    void Write(CRichList &richlist);

    bool Exist(CScript s) { return Exists(s); }
};






class CRichList
{
private:
	mapScriptPubKeys maddresses;

	std::map<CScript,CRichListIterator> mrich;

	bool IsRelevant(const CScript &scriptpubkey) const { return scriptpubkey.IsPayToPublicKeyHash() || scriptpubkey.IsPayToScriptHash(); }

	bool IsRich(const CRichListIterator it) const { return Balance(it) >= 25000000*COIN; }

	CScript ScriptPubKey(const CRichListIterator &it) const { return it -> first; }

	int Height(const CRichListIterator &it) const { return it -> second.second; }

	int64_t Balance(const CRichListIterator &it) const { return it -> second.first; }

	void SetAddressHeight(const CRichListIterator &it, const int &nHeight) { it -> second.second = nHeight; }

	void AddAddressBalance(const CRichListIterator &it, const int64_t &nValue) { it -> second.first += nValue; }

	bool UpdateAddressIndex(const CScript &scriptpubkey, const int64_t &nValue, const int &nHeight, const bool &fCoinBase, const bool fUndo);


public:

	friend void CRichListDB::SaveToRichList(CRichList &richlist, int &bestheight);

	friend void CRichListDB::Write(CRichList &richlist);

	void clear() {maddresses.clear(); mrich.clear();}
// henda út?
	bool GetHeight(const CScript &scriptpubkey, int &nHeight);

	bool GetBalance(const CScript &scriptpubkey, int64_t &nBalance);

	// Four near identical cases.
	// Heights shouldn't be updated if we are undoing and then there is the case of subtracting vs adding funds
	bool UpdateTxOut(const CScript &scriptpubkey, const int64_t &nValue, const int &nHeight, const bool &fCoinBase) {
		assert(nValue >= 0);
		return UpdateAddressIndex(scriptpubkey, nValue, nHeight, fCoinBase, false);
	}

	bool UpdateTxInUndo(const CScript &scriptpubkey, const int64_t &nValue, const int &nHeight, const bool &fCoinBase) {
		assert(nValue >= 0);
		return UpdateAddressIndex(scriptpubkey, -nValue, nHeight, fCoinBase, false);
	}

	bool UndoTxInUndo(const CScript &scriptpubkey, const int64_t &nValue, const int &nHeight, const bool &fCoinBase) {
		assert(nValue >= 0);
		return UpdateAddressIndex(scriptpubkey, nValue, nHeight, fCoinBase, true);
	}

	bool UndoTxOut(const CScript &scriptpubkey, const int64_t &nValue, const int &nHeight, const bool &fCoinBase) {
		assert(nValue >= 0);
		return UpdateAddressIndex(scriptpubkey, -nValue, nHeight, fCoinBase, true);
	}

	bool UpdateRichAddressHeights();

	bool NextRichScriptPubKey(CScript &scriptpubkey);

	CScript NextEIASScriptPubKey(const int &nHeight);

};




#endif
