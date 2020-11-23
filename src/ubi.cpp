#include "ubi.h"

#include <algorithm>
#include <vector>

#include <boost/filesystem.hpp>

using namespace std;

// guards nCurrentHeigth, vBatch and vRecipients
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
static void InitCirculation()
{
    std::multiset<std::pair<std::string, 
                  std::tuple<std::string, std::string>>> ubilist;

    ServiceItemList.GetUbiList(ubilist);

    for (auto r : ubilist)
    {
        std::string toAddress = get<1>(r.second);
        if (toAddress == "" {
            vRecipients.push_back({0, CBitcoinAddress(it.first)});
        }
    }

    vBatch.assign(min(nBatchSize, vRecipients.size()), CScript());
}

// goes through vRecipients and selects the addresses that haven't been paid
// for the longest time.
vector<CScript> NextBatch(const unsigned int nHeight)
{
    boost::call_once(&InitCirculation, ubiInitFlag);

    LOCK(cs_ubi);
    // multiple miners run at the same time on the same block so we only need to compute
    // the vBatch once for each height, if the height is the same then return the same
    // batch
    if (nHeight == nCurrentHeight)
        return vBatch;

    // update our current height and compute the new vBatch
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

        CScript s;
        s.SetDestination(it->address.Get());
        vBatch[i] = s;

        // mark it as paid
        it->lastpaid = nHeight;
    }

    return vBatch;
}

// Similar to GetBlockValueDividends but doesn't depend on block height since we only
// harvest from nFees
int64_t GetUBIDividends(const int64_t nFees)
{
    if (vRecipients.empty())
        return 0;

    return (int64_t)(9 * nFees / 10) / (int64_t)min(nBatchSize, vRecipients.size());
}

} // namespace UBI
