using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace SabreMDT.ViewModels
{
    public class MainViewModel : INotifyPropertyChanged
    {
        private bool _isProvisioned = false;
        public bool IsProvisioned
        {
            get => _isProvisioned;
            set { _isProvisioned = value; OnPropertyChanged(); }
        }

        public void CheckProvisioningStatus()
        {
            // Lock UI in First Run Wizard if no valid JWT/Profile exists on the Hub
            if (string.IsNullOrEmpty(ConfigurationManager.GetJwt()))
            {
                IsProvisioned = false;
            }
            else
            {
                IsProvisioned = true;
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public static class ConfigurationManager
    {
        public static string GetJwt() => ""; // Implementation
    }
}
