using System;
using System.Security.Cryptography;
using System.Text;

namespace SabreMDT.Services
{
    public class SecurityService
    {
        // Cryptographically hashed maintenance password (example hash)
        private const string MaintenanceHash = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";

        public bool VerifyMaintenanceAccess(string password)
        {
            using (var sha256 = SHA256.Create())
            {
                var hashedBytes = sha256.ComputeHash(Encoding.UTF8.GetBytes(password));
                var hashString = BitConverter.ToString(hashedBytes).Replace("-", "").ToLower();
                return hashString == MaintenanceHash;
            }
        }
    }
}
