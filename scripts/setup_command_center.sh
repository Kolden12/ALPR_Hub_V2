#!/bin/bash
# Sabre Command Center - Golden Master Setup Script

set -e

echo "🛡️ Sabre Command Center - Automated Installation (v3.0)"

# 1. Environment Selection
read -p "Select Deployment Mode (1: Cloud, 2: On-Prem): " MODE

# 2. Dependency Check
sudo apt-get update
sudo apt-get install -y docker.io docker-compose wireguard iptables certbot

# 3. Configure VPN Gateway
echo "Initializing WireGuard VPN Gateway..."
WG_PRIV=$(wg genkey)
WG_PUB=$(echo $WG_PRIV | wg pubkey)

cat <<EOF | sudo tee /etc/wireguard/wg0.conf
[Interface]
PrivateKey = $WG_PRIV
Address = 10.8.0.1/24
ListenPort = 51820
PostUp = iptables -A FORWARD -i wg0 -j ACCEPT; iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
PostDown = iptables -D FORWARD -i wg0 -j ACCEPT; iptables -t nat -D POSTROUTING -o eth0 -j MASQUERADE
EOF

# 4. Firewall Rules
if [ "$MODE" == "1" ]; then
    echo "Configuring Cloud Security Groups..."
    sudo ufw allow 51820/udp
    sudo ufw allow 80/tcp
    sudo ufw allow 443/tcp
fi

# 5. Database & API Provisioning
sudo docker-compose up -d

echo "✅ Golden Master Deployment Complete."
echo "Public VPN Key: $WG_PUB"
echo "Server Endpoint: $(curl -s ifconfig.me):51820"
