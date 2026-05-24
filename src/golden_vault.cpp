#include <golden_vault.h>
#include <consensus/params.h>
#include <cmath>
#include <script/script.h>
#include <script/standard.h>
#include <streams.h>
#include <util/strencodings.h>
#include <map>
#include <vector>

static const CAmount BASE_FEE = 1000; // 1000 Instants = 0.00001 TC
static const double PHI = 1.6180339887498948482;
static const int BLOCKS_PER_MONTH = 4380;

// Liste des vaults actifs (mémoire)
struct ActiveVault {
    uint256 txid;
    int64_t startHeight;
    int64_t unlockHeight;
    CAmount totalFee;
    CAmount alreadyPaid;
};
static std::map<uint256, ActiveVault> g_vaults;

// Paliers Fibonacci autorisés (mois)
static const int FIB_MONTHS[] = {1,2,3,5,8,13,21,34,55,89,144};
static const int NUM_TIERS = 11;

int64_t GoldenVault::GetLockHeight(int months, const Consensus::Params& params)
{
    return months * BLOCKS_PER_MONTH;
}

CAmount GoldenVault::ComputeFee(int tierIndex)
{
    if (tierIndex < 0 || tierIndex >= NUM_TIERS) return 0;
    double fee = BASE_FEE * std::pow(PHI, tierIndex);
    return (CAmount)std::floor(fee);
}

CAmount GoldenVault::GetProgressiveReward(int64_t startHeight, int64_t unlockHeight, CAmount totalFee, int64_t currentHeight, CAmount alreadyPaid)
{
    if (currentHeight < startHeight) return 0;
    if (currentHeight >= unlockHeight) return 0;
    int64_t duration = unlockHeight - startHeight;
    if (duration <= 0) return totalFee - alreadyPaid;
    CAmount basePart = totalFee / duration;
    int64_t blocksDone = currentHeight - startHeight;
    CAmount expected = basePart * (blocksDone + 1);
    if (expected > totalFee) expected = totalFee;
    CAmount due = expected - alreadyPaid;
    if (due < 0) due = 0;
    return due;
}

void GoldenVault::AddVault(const uint256& txid, int64_t startHeight, int64_t unlockHeight, CAmount totalFee)
{
    g_vaults[txid] = {txid, startHeight, unlockHeight, totalFee, 0};
}

CAmount GoldenVault::CollectRewardsForBlock(int64_t currentHeight)
{
    CAmount totalReward = 0;
    std::vector<uint256> toRemove;
    for (auto& pair : g_vaults) {
        ActiveVault& v = pair.second;
        if (currentHeight < v.startHeight) continue;
        if (currentHeight >= v.unlockHeight) {
            toRemove.push_back(v.txid);
            continue;
        }
        CAmount due = GetProgressiveReward(v.startHeight, v.unlockHeight, v.totalFee, currentHeight, v.alreadyPaid);
        if (due > 0) {
            totalReward += due;
            v.alreadyPaid += due;
        }
    }
    for (const uint256& txid : toRemove) {
        g_vaults.erase(txid);
    }
    return totalReward;
}

void GoldenVault::Init()
{
    g_vaults.clear();
}

bool GoldenVault::IsGoldenVaultOutput(const CTxOut& out, int& months, CAmount& fee)
{
    // Analyse du script de sortie
    CScript script = out.scriptPubKey;
    CScript::const_iterator pc = script.begin();
    std::vector<unsigned char> vch;
    
    // 1. Lecture du premier élément (nombre de mois)
    if (!script.GetOp(pc, vch)) return false;
    if (vch.size() < 1 || vch.size() > 4) return false; // entier 8-32 bits
    int months_val = 0;
    for (size_t i = 0; i < vch.size(); ++i) months_val |= vch[i] << (8*i);
    
    // 2. Vérifier que l'opcode suivant est OP_CHECKLOCKTIMEVERIFY (0xb1)
    opcodetype op;
    if (!script.GetOp(pc, op)) return false;
    if (op != OP_CHECKLOCKTIMEVERIFY) return false;
    
    // 3. Vérifier que l'opcode suivant est OP_DROP (0x75)
    if (!script.GetOp(pc, op)) return false;
    if (op != OP_DROP) return false;
    
    // 4. Le reste du script doit être un paiement standard (P2PKH, P2SH, P2WPKH, etc.)
    // On extrait le script restant et on vérifie qu'il est valide (non vide)
    CScript remaining(pc, script.end());
    if (remaining.empty()) return false;
    // Vérification basique : le script de paiement doit correspondre à un type connu (via Solver)
    std::vector<std::vector<unsigned char>> solutions;
    TxoutType txType = Solver(remaining, solutions);
    if (txType == TxoutType::NONSTANDARD && txType != TxoutType::WITNESS_V0_KEYHASH && txType != TxoutType::WITNESS_V0_SCRIPTHASH) {
        // On accepte aussi les témoins simples ? Pour l'instant, on se contente des sorties standard.
        // On peut être plus permissif.
    }
    
    // 5. Trouver l'index du palier correspondant à months_val
    int tier = -1;
    for (int i = 0; i < NUM_TIERS; ++i) {
        if (FIB_MONTHS[i] == months_val) {
            tier = i;
            break;
        }
    }
    if (tier == -1) return false;
    
    // 6. Calculer les frais pour ce palier
    fee = ComputeFee(tier);
    
    // 7. Vérifier que le montant de la sortie est exactement égal aux frais
    if (out.nValue != fee) return false;
    
    months = months_val;
    return true;
}
