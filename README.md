# Timecoin – Une monnaie décentralisée régie par le Nombre d'Or

Timecoin est un fork de Bitcoin Core intégrant une émission basée sur la suite de Fibonacci, un algorithme de difficulté LWMA et le protocole Golden Vault.

## Caractéristiques principales

- **Offre maximale** : 16 180 339,887 TC (φ × 10⁷)
- **Récompense initiale** : 377 TC/bloc (F(14))
- **Réduction** : division par φ à chaque saison (durée des saisons suivant Fibonacci en mois)
- **Difficulté** : LWMA (fenêtre 144 blocs, recalcule à chaque bloc)
- **Golden Vault** : time-locks natifs avec paliers Fibonacci, frais redistribués aux mineurs
- **Pré‑mine** : 0 TC (lancement équitable)
- **Ports** : mainnet 16180, testnet 26180
- **Adresses** : bech32 avec préfixe `tc1` (mainnet), `t1` (testnet)

## Compilation sur Pop!_OS / Ubuntu

```bash
sudo apt update && sudo apt install -y build-essential libtool autoconf automake pkg-config \
  libssl-dev libevent-dev libboost-all-dev libsqlite3-dev libminiupnpc-dev libzmq3-dev python3 git
git clone https://github.com/realTimecoin/Timecoin.git
cd Timecoin
./autogen.sh
./configure --with-gui=no --disable-tests --disable-bench
make -j$(nproc)
{ _ble_edit_exec_gexec__save_lastarg "$@"; } 4>&1 5>&2 &>/dev/null
