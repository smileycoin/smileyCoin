#include "ubi.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/version.hpp>

using namespace std;

CCriticalSection cs_ubi;

// the height of vBatch, if the external blockheight changes then vBatch 
// is updated
unsigned int nCurrentHeight = 0;

// where we store our batch between blockchain updates
vector<CScript> vBatch;

// compare first using lastpaid, and then using the CBitcoinAddress
// builtin comparator
bool operator<(const Recipient& lhs, const Recipient& rhs)
{
    if (lhs.lastpaid != rhs.lastpaid)
        return lhs.lastpaid < rhs.lastpaid;
    else
        return lhs.address < rhs.address;
}

// our list of recipients from vRecipients, also only updated on a new blockchain
vector<Recipient> vRecipients;

// how many recipient are paid per block
const size_t nBatchSize = 5;

// remove once we move to servicelist, only for reading ubi_addresses
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

    vBatch.assign(min(nBatchSize, vRecipients.size()), CScript());
}

// goes through vRecipients and selects the addresses that haven't been paid
// for the longest time.
vector<CScript> NextBatch(const unsigned int nHeight)
{
    boost::call_once(&InitCirculation, ubiInitFlag);

    LOCK(cs_ubi);
    // probably called by a sister thread, no need to compute everything
    // all over again for the same heigth
    if (nHeight == nCurrentHeight)
        return vBatch;

    nCurrentHeight = nHeight;

    // choose the recipients with the lowest lastpaid values.
    //   if we just paid nbatchsize many recipients some would get paid multiple
    // times without us knowing who, so we cap it at vRecipients.size() if it's
    // lower than nBatchSize
    for (unsigned int i = 0; i < min(vRecipients.size(), nBatchSize); i++)
    {
        // using our operator< get the iterator of the recipient in batch that
        // has been paid most recently
        auto it = min_element(vRecipients.begin(), vRecipients.end());
        it->lastpaid = nHeight;

        CScript s;
        s.SetDestination(it->address.Get());
        vBatch[i] = s;
    }

    return vBatch;
}

// Similar to GetBlockValueDividends but doesn't depend on block height since we only
// harvest from nFees
int64_t GetUBIDividends(const int64_t nFees)
{
    if (vRecipients.empty())
        return 0;

    return (int64_t)(0.9 * nFees) / (int64_t)min(nBatchSize, vRecipients.size());
}

} // namespace UBI
