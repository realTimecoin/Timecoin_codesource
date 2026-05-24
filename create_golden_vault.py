#!/usr/bin/env python3
import subprocess
import json
import sys
import time

# Configuration RPC (regtest ou testnet, à adapter)
RPC_CONF = {
    "regtest": {
        "host": "127.0.0.1",
        "port": 16181,
        "user": "timecoin",
        "password": "test"
    },
    "testnet": {
        "host": "127.0.0.1",
        "port": 26181,
        "user": "timecoin",
        "password": "GoldenSpiral2026"
    }
}

def rpc_call(method, params, network="regtest"):
    conf = RPC_CONF[network]
    cmd = [
        "./src/bitcoin-cli", f"-{network}",
        f"-rpcport={conf['port']}",
        f"-rpcuser={conf['user']}",
        f"-rpcpassword={conf['password']}",
        method
    ] + [json.dumps(p) if isinstance(p, (dict, list)) else str(p) for p in params]
    return json.loads(subprocess.check_output(cmd, text=True))

def main():
    network = sys.argv[1] if len(sys.argv) > 1 else "regtest"
    months = int(sys.argv[2]) if len(sys.argv) > 2 else 3

    # Vérifier les paliers Fibonacci autorisés
    allowed = [1,2,3,5,8,13,21,34,55,89,144]
    if months not in allowed:
        print(f"Erreur : le nombre de mois doit être dans {allowed}")
        sys.exit(1)

    # Calculer les frais correspondants (index du palier)
    tier = allowed.index(months)
    fee_sat = int(1000 * (1.618033988749895 ** tier))  # Instants
    fee_tc = fee_sat / 100_000_000

    # Récupérer une adresse de destination (vous pouvez la passer en argument)
    dest_addr = rpc_call("getnewaddress", ["vault_dest", "legacy"], network)
    pubkey = rpc_call("getaddressinfo", [dest_addr], network)["pubkey"]

    # Construire le script: <months> OP_CHECKLOCKTIMEVERIFY OP_DROP <pubkey> OP_CHECKSIG
    pk_len = len(pubkey) // 2
    script_hex = f"{months:02x}b175{pk_len:02x}{pubkey}ac"

    # Obtenir un UTXO
    utxos = rpc_call("listunspent", [], network)
    if not utxos:
        print("Aucun UTXO disponible. Miner d'abord des blocs.")
        sys.exit(1)
    utxo = utxos[0]
    change_addr = rpc_call("getrawchangeaddress", [], network)

    # Préparer les sorties
    outputs = {
        script_hex: fee_tc,
        change_addr: utxo["amount"] - fee_tc - 0.00001  # 0.00001 pour les frais de transaction
    }
    raw_tx = rpc_call("createrawtransaction", [[{"txid": utxo["txid"], "vout": utxo["vout"]}]], network)
    # On doit reconstruire avec les sorties correctes
    raw_tx = rpc_call("createrawtransaction", [[{"txid": utxo["txid"], "vout": utxo["vout"]}], outputs], network)

    signed_tx = rpc_call("signrawtransactionwithwallet", [raw_tx], network)
    if not signed_tx["complete"]:
        print("Erreur de signature")
        sys.exit(1)

    txid = rpc_call("sendrawtransaction", [signed_tx["hex"]], network)
    print(f"Vault créé ! TXID : {txid}")
    print(f"Montant verrouillé : {fee_tc} TC pour {months} mois")
    print(f"Script : {script_hex}")

if __name__ == "__main__":
    main()
