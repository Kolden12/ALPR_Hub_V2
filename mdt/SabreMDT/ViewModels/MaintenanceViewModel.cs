using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Net.Http;

namespace SabreMDT.ViewModels
{
    public class MaintenanceViewModel : INotifyPropertyChanged
    {
        private string _exportStatus = "Ready";
        public string ExportStatus
        {
            get => _exportStatus;
            set { _exportStatus = value; OnPropertyChanged(); }
        }

        public async Task TriggerUsbExport()
        {
            ExportStatus = "Exporting Evidence...";
            try
            {
                using (var client = new HttpClient())
                {
                    // Trigger Hub's USB export API
                    var response = await client.PostAsync("http://192.168.1.10:8000/maintenance/export-usb", null);
                    if (response.IsSuccessStatusCode)
                        ExportStatus = "Export Successful. Remove Drive.";
                    else
                        ExportStatus = "Export Failed. Check Drive.";
                }
            }
            catch
            {
                ExportStatus = "Error Connecting to Hub.";
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
