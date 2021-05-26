#include "base58.h"
#include "main.h"

#include "core.h"
#include "init.h"
#include "txdb.h"

// height last-paid and receiving address of a ubi recipient
typedef struct {
    unsigned int lastpaid;
    CBitcoinAddress address;
} Recipient;

namespace UBI
{
std::vector<CScript> NextBatch(const unsigned int nHeight);
int64_t GetUBIDividends(const int64_t nFees);
}

