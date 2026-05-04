using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Media;

namespace SabreMDT.ViewModels
{
    public class VideoViewModel : INotifyPropertyChanged
    {
        private float _batteryVoltage = 12.6f;
        public float BatteryVoltage
        {
            get => _batteryVoltage;
            set { _batteryVoltage = value; OnPropertyChanged(); OnPropertyChanged(nameof(BatteryColor)); }
        }
        public Brush BatteryColor => _batteryVoltage < 12.0f ? Brushes.Red : Brushes.Lime;

        private float _jetsonTemp = 45.0f;
        public float JetsonTemp
        {
            get => _jetsonTemp;
            set { _jetsonTemp = value; OnPropertyChanged(); OnPropertyChanged(nameof(TempColor)); }
        }
        // Updated Warning Threshold to 88°C for San Antonio environment
        public Brush TempColor => _jetsonTemp > 88.0f ? Brushes.Orange : Brushes.Lime;

        private bool _isLinked = true;
        public bool IsLinked
        {
            get => _isLinked;
            set { _isLinked = value; OnPropertyChanged(); OnPropertyChanged(nameof(HeartbeatColor)); OnPropertyChanged(nameof(GuardianStatus)); }
        }
        public string GuardianStatus => _isLinked ? "HUB LINKED" : "LINK LOST";
        public Brush HeartbeatColor => _isLinked ? Brushes.Lime : Brushes.Red;

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
