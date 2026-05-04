#!/bin/bash
# Sabre Command Center - Automated Installation (v2.0 VPN Ready)

set -e

VPN_MODE=false
while [[ "\$#" -gt 0 ]]; do
    case \$1 in
        --vpn) VPN_MODE=true ;;
    esac
    shift
done

echo "🛡️ Sabre Command Center - Deployment"

# 1. Base Dependencies
sudo apt-get update
sudo apt-get install -y docker.io docker-compose wireguard iptables

# 2. VPN Configuration
if [ "\$VPN_MODE" = true ]; then
    echo "Configuring WireGuard Gateway..."
    WG_PRIV=\$(wg genkey)
    WG_PUB=\$(echo \$WG_PRIV | wg pubkey)

    cat <<EOF | sudo tee /etc/wireguard/wg0.conf
[Interface]
PrivateKey = \$WG_PRIV
Address = 10.8.0.1/24
ListenPort = 51820
PostUp = iptables -A FORWARD -i wg0 -j ACCEPT; iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
PostDown = iptables -D FORWARD -i wg0 -j ACCEPT; iptables -t nat -D POSTROUTING -o eth0 -j MASQUERADE
EOF
    sudo systemctl enable wg-quick@wg0
    sudo systemctl start wg-quick@wg0
fi

# 3. Docker Launch
sudo docker-compose up -d

echo "✅ Command Center Active."
[ "\$VPN_MODE" = true ] && echo "VPN Gateway: 10.8.0.1 (UDP 51820 Open)"
