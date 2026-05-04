using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace SabreMDT.ViewModels
{
    public class MainViewModel : INotifyPropertyChanged
    {
        private BitmapImage? _agencyLogo;
        public BitmapImage? AgencyLogo
        {
            get => _agencyLogo;
            set { _agencyLogo = value; OnPropertyChanged(); }
        }

        public async void SyncBranding(string logoUrl)
        {
            // Implementation to download logo from URL, cache locally, and bind to UI
            // WatermarkOpacity = 0.3;
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
