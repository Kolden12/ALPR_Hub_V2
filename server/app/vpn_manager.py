import subprocess
import os

class VPNManager:
    def __init__(self, interface="wg0"):
        self.interface = interface
        self.config_path = "/etc/wireguard"

    def generate_peer_config(self, vehicle_id: str):
        """Generate WireGuard keys and peer configuration."""
        # 1. Generate Private/Public keys
        priv_key = subprocess.check_output(["wg", "genkey"]).decode().strip()
        pub_key = subprocess.check_output(["echo", priv_key, "|", "wg", "pubkey"], shell=True).decode().strip()

        # 2. Assign Internal IP (Conceptually)
        client_ip = f"10.8.0.{hash(vehicle_id) % 254 + 1}"

        # 3. Add Peer to Server
        subprocess.run(["wg", "set", self.interface, "peer", pub_key, "allowed-ips", f"{client_ip}/32"])

        # 4. Return Client Config for Hub
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
