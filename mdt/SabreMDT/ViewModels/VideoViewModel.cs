using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Net.WebSockets;
using System.Threading;
using System.Text;
using Newtonsoft.Json.Linq;

namespace SabreMDT.ViewModels
{
    public class VideoViewModel : INotifyPropertyChanged
    {
        private bool _isOffloading = false;
        public bool IsOffloading
        {
            get => _isOffloading;
            set { _isOffloading = value; OnPropertyChanged(); }
        }

        private bool _showHitAlert = false;
        public bool ShowHitAlert
        {
            get => _showHitAlert;
            set { _showHitAlert = value; OnPropertyChanged(); }
        }

        private string _lastPlate = "---";
        public string LastPlate { get => _lastPlate; set { _lastPlate = value; OnPropertyChanged(); } }

        private string _lastYMMV = "SEARCHING...";
        public string LastYMMV { get => _lastYMMV; set { _lastYMMV = value; OnPropertyChanged(); } }

        private string _lastGPS = "0.0, 0.0";
        public string LastGPS { get => _lastGPS; set { _lastGPS = value; OnPropertyChanged(); } }

        public void StartGStreamerPipelines()
        {
            // Implementation requires GstSharp.
            // For production, this initializes the 4 camera streams with low-latency flags:
            // "rtspsrc location=... latency=0 buffer-mode=0 ! rtph264depay ! h264parse ! decodebin ! videoconvert ! appsink"
        }

        public async Task StartAlertListener(string hubIp)
        {
            using (var ws = new ClientWebSocket())
            {
                try {
                    await ws.ConnectAsync(new Uri($"ws://{hubIp}:8000/ws"), CancellationToken.None);
                    var buffer = new byte[1024 * 4];
                    while (ws.State == WebSocketState.Open)
                    {
                        var result = await ws.ReceiveAsync(new ArraySegment<byte>(buffer), CancellationToken.None);
                        if (result.MessageType == WebSocketMessageType.Text)
                        {
                            var json = Encoding.UTF8.GetString(buffer, 0, result.Count);
                            var data = JObject.Parse(json);

                            LastPlate = data["plate"]?.ToString() ?? "---";
                            LastYMMV = data["ymmv"]?.ToString() ?? "---";
                            LastGPS = $"{data["lat"]}, {data["lon"]}";

                            ShowHitAlert = true;
                            _ = Task.Delay(10000).ContinueWith(_ => ShowHitAlert = false);
                        }
                    }
                } catch (Exception) {
                    await Task.Delay(5000);
                    _ = StartAlertListener(hubIp);
                }
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }
}
