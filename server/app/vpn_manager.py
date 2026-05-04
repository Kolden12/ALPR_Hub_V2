import subprocess
import os

class VPNManager:
    def __init__(self, interface="wg0"):
        self.interface = interface

    def generate_peer_config(self, vehicle_id: str):
        priv_key = subprocess.check_output(["wg", "genkey"]).decode().strip()
        pub_key = subprocess.check_output(["echo", priv_key, "|", "wg", "pubkey"], shell=True).decode().strip()
        client_ip = f"10.8.0.{hash(vehicle_id) % 254 + 1}"

        client_config = f"""
[Interface]
PrivateKey = {priv_key}
Address = {client_ip}/24
DNS = 1.1.1.1

[Peer]
PublicKey = SERVER_PUB_KEY_PLACEHOLDER
Endpoint = SERVER_EXT_IP:51820
AllowedIPs = 0.0.0.0/0
PersistentKeepalive = 25
"""
        return client_config
