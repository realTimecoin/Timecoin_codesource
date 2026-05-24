#ifndef BITCOIN_GOLDEN_VAULT_H
#define BITCOIN_GOLDEN_VAULT_H

#include <amount.h>
#include <primitives/transaction.h>
#include <consensus/params.h>
#include <uint256.h>
#include <vector>
#include <map>

class GoldenVault
{
public:
    // Calcul de la hauteur de déverrouillage (blocs) à partir du nombre de mois
    static int64_t GetLockHeight(int months, const Consensus::Params& params);

    // Calcul des frais pour un palier (0..10) selon φ^n * 1000 Instants
    static CAmount ComputeFee(int tierIndex);

    // Récompense progressive pour un vault à une hauteur donnée
    static CAmount GetProgressiveReward(int64_t startHeight, int64_t unlockHeight, CAmount totalFee, int64_t currentHeight, CAmount alreadyPaid);

    // Ajouter un nouveau vault (appelé lors de la validation d'une transaction)
    static void AddVault(const uint256& txid, int64_t startHeight, int64_t unlockHeight, CAmount totalFee);

    // Collecter toutes les récompenses dues pour le bloc en cours (appelé par miner.cpp)
    static CAmount CollectRewardsForBlock(int64_t currentHeight);

    // Initialiser le module (vider la liste)
    static void Init();

    // Vérifier si une sortie est un Golden Vault et extraire les paramètres (mois, frais)
    // Détection via script : OP_CHECKLOCKTIMEVERIFY avec push du nombre de mois
    static bool IsGoldenVaultOutput(const CTxOut& out, int& months, CAmount& fee);
};

#endif
