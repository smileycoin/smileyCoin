#include "ubi.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/version.hpp>

/* To the reader:
 * We calculate the next batch of UBI recipients for every thread,
 * but we can do it only for each block height when someone has
 * actually been paid. 
 *
 * Could move initialization to init.cpp to conform to the other code
 * but that's annoying.
 */

using namespace std;

// compare recipients only via block-last-paid, the address is irrelevant
bool operator<(const Recipient& lhs, const Recipient& rhs)
{
    return lhs.lastpaid < rhs.lastpaid;
}

// our list of recipients
vector<Recipient> vRecipients;

// how many recipient are paid per block
const size_t nBatchSize = 5;

static boost::once_flag ubiInitFlag = BOOST_ONCE_INIT;

namespace UBI
{

// read ~/.smileycoin/ubi_addresses and put them into vRecipients
// it's alright if there is no file, then it behaves like -ubi=0
static void InitCirculation()
{
    auto addressfile_path = GetDataDir() / "ubi_addresses";

    ifstream addressfile(addressfile_path.string());

    if (!addressfile.good())
        LogPrintf("UBI::InitCirculation() couldn't read ubi_addresses file, not paying any UBI\n");


    string ln;
    while (getline(addressfile, ln))
    {
        // allow empty lines or comments
        if (ln[0] == '#' || ln.empty())
            continue;

        // user has not been paid, so he is initialized with the height 0
        vRecipients.push_back({0, CBitcoinAddress(ln)});
    }
}

// goes through vRecipients and selects the addresses that haven't been paid
// for the longest time.
// in: nHeight, height of the block that pays the addresses
// return: vector of the CScript's that pay the block's ubi recipients
vector<CScript> NextBatch(const unsigned int nHeight)
{
    boost::call_once(&InitCirculation, ubiInitFlag);

    int stop = min(vRecipients.size(), nBatchSize);

    vector<Recipient> batch(vRecipients.begin(), vRecipients.begin()+stop);

    // choose the recipients with the lowest lastpaid values
    for (auto &r : vRecipients)
    {
        // using our operator< get the iterator of the recipient in batch that
        // has been paid most recently
        auto it = max_element(batch.begin(), batch.end());

        if (r < *it)
        {
            *it = r;
            r.lastpaid = nHeight;
        }
    }

    vector<CScript> ret;
    for (auto r : batch)
    {
        CScript s;
        s.SetDestination(r.address.Get());
        ret.push_back(s);
    }

    return ret;
}

// Similar to GetBlockValueDividends but doesn't depend on block height
// in: nFees, block fees
// return: dividends per recipient, 0 if there are none
int64_t GetUBIDividends(const int64_t nFees)
{
    if (vRecipients.empty())
        return 0;

    int64_t nRecipientCount = min(nBatchSize, vRecipients.size());

    return (int64_t)(0.9 * nFees) / nRecipientCount;
}

} // namespace UBI
