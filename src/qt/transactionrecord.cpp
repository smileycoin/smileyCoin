// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionrecord.h"

#include "base58.h"
#include "wallet.h"
#include "guiutil.h"
#include "jeeq.h"

#include <stdint.h>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        // Ensures we show generated coins / mined transactions at depth 1
        if (!wtx.IsInMainChain())
        {
            return false;
        }
    }
    return true;
}

/* 
    Try and decrypt data encrypted by public key 
*/
bool _decryptData(std::string txData, CBitcoinAddress address, const CWallet *wallet, std::string &decryptedData) {
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        return false;

    CKey vchSecret;
    if (!wallet->GetKey(keyID, vchSecret))
        return false;

    if (!IsHex(txData))
        return false;

    std::string decryptedString = Jeeq::DecryptMessage(vchSecret, ParseHex(txData));

    if (decryptedString.length() == 0)
        return false;

    decryptedData = HexStr(decryptedString);
    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    int64_t nCredit = wtx.GetCredit(true);
    int64_t nDebit = wtx.GetDebit();
    int64_t nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    bool txDataExists = false;
    bool txDataIsEncrypted = false;
    std::string txData;

    if (nNet > 0 || wtx.IsCoinBase())
    {
        //
        // Credit
        //
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if(wallet->IsMine(txout))
            {
                TransactionRecord sub(hash, nTime);

                CTxDestination address;
                sub.idx = parts.size(); // sequence number
                sub.credit = txout.nValue;

                for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++) {
                    const CTxOut &txout2 = wtx.vout[nOut];
                    // Check if data exists in transaction
                    std::string hexString = HexStr(txout2.scriptPubKey);
                    if (hexString.substr(0, 2) == "6a") {
                        txDataExists = true;
                        txData = hexString.substr(4, hexString.size());
                        if (txData.substr(0, 8) == "6a6a0000") // 6a6a0000 start of jeeq encryption header
                            txDataIsEncrypted = true;
                    }
                }

                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                {
                    // Received by Bitcoin Address
                    CBitcoinAddress addr(address);
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = addr.ToString();

                    // Add data. Decrypt it if needed.
                    if (txDataExists) {
                        if (txDataIsEncrypted) {
                            std::string decryptedData;
                            if (_decryptData(txData, addr, wallet, decryptedData))
                                sub.data += decryptedData;
                        } else {
                            sub.data += txData;
                        }
                    }
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.IsCoinBase())
                {
                    // Generated
                    sub.type = TransactionRecord::Generated;
                }

                parts.append(sub);
            }
        }
    }
    else
    {
        bool fAllFromMe = true;
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
            fAllFromMe = fAllFromMe && wallet->IsMine(txin);

        bool fAllToMe = true;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            fAllToMe = fAllToMe && wallet->IsMine(txout);

        if (fAllFromMe && fAllToMe)
        {   
            // Payment to self
            int64_t nChange = wtx.GetChange();

            parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf, "",
                                           -(nDebit - nChange), nCredit - nChange, ""));
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            int64_t nTxFee = nDebit - wtx.GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++) {
                const CTxOut &txout2 = wtx.vout[nOut];
                // Check if data exists in transaction
                std::string hexString = HexStr(txout2.scriptPubKey);
                if (hexString.substr(0, 2) == "6a") {
                    txDataExists = true;
                    txData = hexString.substr(4, hexString.size());
                    if (txData.substr(0, 8) == "6a6a0000")
                        txDataIsEncrypted = true;
                }
            }

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];
                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();

                bool sentToSelf = false;                
                bool isMine = wallet->IsMine(txout);
                if(isMine && !txDataIsEncrypted)
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address. 
                    // If data is encrypted we need the address.
                    continue;
                }
                
                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Bitcoin Address
                    CBitcoinAddress addr(address);
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = addr.ToString();  
                    

                    if  (txDataExists) {
                        if (txDataIsEncrypted) {
                            std::string decryptedData;
                            if (_decryptData(txData, addr, wallet, decryptedData))
                            {   
                                sentToSelf = true;
                                sub.data += decryptedData;
                            }
                            else if (isMine) 
                            {
                                // We don't need to record change.
                                continue;
                            }
                        } else
                            sub.data += txData;
                    }
                }
                else
                {
                    if (txDataExists) continue; // Transaction with data has already been recorded
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

                int64_t nValue = txout.nValue;
                if (sentToSelf) nValue = 0;
                /* Add fee to first output */
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                parts.append(sub);
            }
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
            parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, "", nNet, 0, ""));
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
            (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
            (wtx.IsCoinBase() ? 1 : 0),
            wtx.nTimeReceived,
            idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity(chainActive.Height() - wtx.GetDepthInMainChain()) > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = chainActive.Height();

    if (!IsFinalTx(wtx, chainActive.Height() + 1))
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.nLockTime - chainActive.Height();
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    // For generated transactions, determine maturity
    else if(type == TransactionRecord::Generated)
    {
        if (wtx.GetBlocksToMaturity(status.depth) > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity(status.cur_num_blocks - status.depth);

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = TransactionStatus::Conflicted;
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }

}

bool TransactionRecord::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);
    return status.cur_num_blocks != chainActive.Height();
}

QString TransactionRecord::getTxID() const
{
    return formatSubTxId(hash, idx);
}

QString TransactionRecord::formatSubTxId(const uint256 &hash, int vout)
{
    return QString::fromStdString(hash.ToString() + strprintf("-%03d", vout));
}