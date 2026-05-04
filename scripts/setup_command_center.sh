#!/bin/bash
# Sabre Command Center - Setup Script

set -e

echo "🛡️ Sabre Command Center - Automated Installation"

# 1. Environment Detection
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
fi

echo "Deploying for: $OS"

# 2. Dependency Installation
sudo apt-get update
sudo apt-get install -y docker.io docker-compose certbot nginx

# 3. Directory Preparation
mkdir -p static/branding
mkdir -p config

# 4. Generate Nginx Config (Simple Template)
cat <<EOF > config/nginx.conf
user  nginx;
worker_processes  auto;
events { worker_connections  1024; }
http {
    server {
        listen 80;
        location / {
            proxy_pass http://api:8000;
            proxy_set_header Host \$host;
        }
        location /static/ {
            alias /usr/share/nginx/html/static/;
        }
    }
}
EOF

# 5. Start the Stack
echo "Launching Sabre Stack..."
sudo docker-compose up -d

echo "✅ Command Center is ONLINE."
echo "REST API: http://$(curl -s ifconfig.me):8000"
echo "Setup your Fleet Profiles via the Provisioning endpoint."
