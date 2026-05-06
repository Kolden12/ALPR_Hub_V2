# Sabre Command Center - Tactical Setup Manual

The Sabre Command Center is the centralized management hub for provisioning and auditing your fleet of Sabre ALPR Hubs.

## 1. On-Premise Installation (Hardware Lab)
*Designed for Ubuntu Server running on Proxmox or bare metal.*

### Networking
- Assign a **Static IP** to the server.
- Configure your local DNS (or `/etc/hosts`) to point `sabre.local` to the static IP.
- Ensure the server can reach your local NAS (TrueNAS/Synology) for mass storage.

### Security
- The `setup_command_center.sh` script installs an Nginx reverse proxy.
- For internal labs, you can utilize self-signed certificates or an internal Windows CA.

### Deployment
```bash
cd scripts
chmod +x setup_command_center.sh
./setup_command_center.sh
```

## 2. Cloud Installation (Zero-Footprint)
*Designed for AWS (EC2), DigitalOcean, or Azure.*

### Security Groups (Firewall)
- **Port 80/443:** Allow traffic from your fleet's LTE IP range.
- **Port 22:** Restrict to your management IP.

### SSL / Certificates
- The setup script includes `certbot`.
- Run `sudo certbot --nginx -d yourdomain.com` after the stack is up to enable production SSL.

### Global Branding
- Upload your agency logo to `/static/branding/agency_logo.png`.
- This asset is automatically served via URL to all MDT terminals for Shift Reports.

## 3. The Provisioning Handshake
1. Fleet Manager generates a **One-Time Token (OTT)** on the Command Center.
2. Officer enters the **Vehicle ID** and **OTT** on the MDT terminal.
3. The Hub connects to the `/auth/provision` endpoint to receive its permanent JWT and "Golden Configuration."
