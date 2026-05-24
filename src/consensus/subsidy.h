#ifndef BITCOIN_CONSENSUS_SUBSIDY_H
#define BITCOIN_CONSENSUS_SUBSIDY_H

#include <amount.h>
#include <consensus/params.h>

namespace consensus {
    CAmount GetBlockSubsidy(int nHeight, const Params& consensusParams);
}

#endif
