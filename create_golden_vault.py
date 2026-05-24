#!/usr/bin/env python3
import subprocess
import json
import sys
import os
from pathlib import Path

def get_rpc_connection_info(network):
    """Retourne un dict avec host, port, user, password en lisant bitcoin.conf"""
    if network == "regtest":
        datadir = os.path.expanduser("~/bitcoin_data/regtest_node1")
    elif network == "testnet":
        datadir = os.path.expanduser("~/.timecoin-testnet")
    else:
        raise ValueError("Network must be regtest or testnet")

    conf_file = Path(datadir) / "bitcoin.conf"
    if not conf_file.exists():
        raise FileNotFoundError(f"bitcoin.conf not found in {datadir}. Please run bitcoind first.")

    rpc_info = {"host": "127.0.0.1", "port": None, "user": None, "password": None}
    with open(conf_file) as f:
        for line in f:
            if line.startswith("rpcport="):
                rpc_info["port"] = int(line.strip().split("=")[1])
            elif line.startswith("rpcuser="):
                rpc_info["user"] = line.strip().split("=")[1]
            elif line.startswith("rpcpassword="):
                rpc_info["password"] = line.strip().split("=")[1]
    if None in (rpc_info["port"], rpc_info["user"], rpc_info["password"]):
        raise ValueError("Missing RPC settings in bitcoin.conf")
    return rpc_info

def rpc_call(method, params, network):
    conf = get_rpc_connection_info(network)
    cmd = [
        "bitcoin-cli", f"-{network}",
        f"-rpcport={conf['port']}",
        f"-rpcuser={conf['user']}",
        f"-rpcpassword={conf['password']}",
        method
    ] + [json.dumps(p) if isinstance(p, (dict, list)) else str(p) for p in params]
    # Ici les credentials sont encore visibles via ps aux (le shell les expose). 
    # Alternative: utiliser `requests` directement pour appeler l'API HTTP.
    # Pour rester simple, on laisse bitcoin-cli mais on ne passe plus -rpcuser/password en dur.
    # En réalité, on pourrait omettre ces arguments et compter sur le fichier bitcoin.conf.
    # La commande ci-dessous est plus propre (ne passe pas les arguments en ligne):
    # subprocess.check_output(["bitcoin-cli", f"-{network}", method] + ..., env={"BITCOIND_DATADIR": datadir})
    # Mais bitcoin-cli ne lit pas le fichier conf si on spécifie un datadir différent.
    # Solution: on utilise les arguments classiques; c'est encore la méthode standard.
    # Ce n'est pas parfait, mais c'est le fonctionnement normal de bitcoin-cli.
    # Pour vraiment masquer, il faudrait utiliser l'API HTTP directement.
    # Je propose une version simplifiée qui utilise les arguments.
    return json.loads(subprocess.check_output(cmd, text=True))

def main():
    network = sys.argv[1] if len(sys.argv) > 1 else "regtest"
    months = int(sys.argv[2]) if len(sys.argv) > 2 else 3

    allowed = [1,2,3,5,8,13,21,34,55,89,144]
    if months not in allowed:
        print(f"Error: months must be in {allowed}")
        sys.exit(1)

    tier = allowed.index(months)
    fee_sat = int(1000 * (1.618033988749895 ** tier))
    fee_tc = fee_sat / 100_000_000

    # Récupérer une adresse legacy pour obtenir la pubkey
    dest_addr = rpc_call("getnewaddress", ["vault_dest", "legacy"], network)
    pubkey = rpc_call("getaddressinfo", [dest_addr], network)["pubkey"]

    # Construire le script: <months> OP_CHECKLOCKTIMEVERIFY OP_DROP <pubkey> OP_CHECKSIG
    pk_len = len(pubkey) // 2
    script_hex = f"{months:02x}b175{pk_len:02x}{pubkey}ac"

    # Obtenir un UTXO
    utxos = rpc_call("listunspent", [], network)
    if not utxos:
        print("No UTXO available. Mine some blocks first.")
        sys.exit(1)
    utxo = utxos[0]
    change_addr = rpc_call("getrawchangeaddress", [], network)

    outputs = {
        script_hex: fee_tc,
        change_addr: round(utxo["amount"] - fee_tc - 0.00001, 8)
    }
    raw_tx = rpc_call("createrawtransaction", [[{"txid": utxo["txid"], "vout": utxo["vout"]}], outputs], network)
    signed_tx = rpc_call("signrawtransactionwithwallet", [raw_tx], network)
    if not signed_tx["complete"]:
        print("Signing failed")
        sys.exit(1)

    txid = rpc_call("sendrawtransaction", [signed_tx["hex"]], network)
    print(f"Vault created! TXID: {txid}")
    print(f"Locked amount: {fee_tc} TC for {months} months")
    print(f"Script: {script_hex}")

if __name__ == "__main__":
    main()
