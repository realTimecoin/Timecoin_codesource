#include <consensus/subsidy.h>
#include <consensus/params.h>
#include <cmath>

namespace consensus {

static const double PHI = 1.6180339887498948482;
static const CAmount INITIAL_SUBSIDY = 377 * COIN;  // 377 TC
static const int BLOCKS_PER_MONTH = 4380;

static int FibMonth(int m) {
    if (m <= 0) return 0;
    if (m == 1) return 1;
    int a = 1, b = 1;
    for (int i = 3; i <= m; ++i) {
        int c = a + b;
        a = b;
        b = c;
    }
    return b;
}

CAmount GetBlockSubsidy(int nHeight, const Params& consensusParams) {
    if (nHeight == 0) return 0;

    int season = 1;
    int blockStart = 1;
    int monthIdx = 1;
    while (true) {
        int durationMonths = FibMonth(monthIdx);
        int durationBlocks = durationMonths * BLOCKS_PER_MONTH;
        if (nHeight < blockStart + durationBlocks) break;
        blockStart += durationBlocks;
        season++;
        monthIdx++;
    }

    double subsidy = INITIAL_SUBSIDY;
    for (int s = 2; s <= season; ++s) subsidy /= PHI;
    CAmount result = (CAmount)std::floor(subsidy);
    if (result < 1 && nHeight < 10000000) result = 1;
    return result;
}

} // namespace
