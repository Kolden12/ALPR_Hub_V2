using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Media;

namespace SabreMDT.ViewModels
{
    public class VideoViewModel : INotifyPropertyChanged
    {
        private float _batteryVoltage = 12.6f;
        public float BatteryVoltage { get => _batteryVoltage; set { _batteryVoltage = value; OnPropertyChanged(); OnPropertyChanged(nameof(BatteryColor)); } }
        public Brush BatteryColor => _batteryVoltage < 12.0f ? Brushes.Red : Brushes.Lime;

        private float _jetsonTemp = 45.0f;
        public float JetsonTemp { get => _jetsonTemp; set { _jetsonTemp = value; OnPropertyChanged(); OnPropertyChanged(nameof(TempColor)); } }
        public Brush TempColor => _jetsonTemp > 85.0f ? Brushes.Orange : Brushes.Lime;

        private string _guardianStatus = "ACTIVE";
        public string GuardianStatus { get => _guardianStatus; set { _guardianStatus = value; OnPropertyChanged(); } }
        public Brush HeartbeatColor => Brushes.Lime;

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
