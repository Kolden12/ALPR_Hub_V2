using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Net.WebSockets;
using System.Threading;
using System.Text;

namespace SabreMDT.ViewModels
{
    public class VideoViewModel : INotifyPropertyChanged
    {
        private string _statusMessage = "System Ready";
        public string StatusMessage
        {
            get => _statusMessage;
            set { _statusMessage = value; OnPropertyChanged(); }
        }

        private string _plateText = "WAITING...";
        public string PlateText
        {
            get => _plateText;
            set { _plateText = value; OnPropertyChanged(); }
        }

        public void InitializeGStreamer()
        {
            // Production logic for GstSharp initialization
            // Gst.Application.Init();
            // var pipeline = Gst.Parse.Launch("rtspsrc location=rtsp://192.168.1.101/stream1 ! decodebin ! autovideosink");
            // pipeline.SetState(Gst.State.Playing);
        }

        public async Task ListenForAlertsAsync(string jetsonIp)
        {
            using (var ws = new ClientWebSocket())
            {
                try
                {
                    await ws.ConnectAsync(new Uri($"ws://{jetsonIp}:8000/ws/alerts"), CancellationToken.None);
                    StatusMessage = "Connected to Hub";
                    var buffer = new byte[1024 * 4];
                    while (ws.State == WebSocketState.Open)
                    {
                        var result = await ws.ReceiveAsync(new ArraySegment<byte>(buffer), CancellationToken.None);
                        if (result.MessageType == WebSocketMessageType.Text)
                        {
                            var msg = Encoding.UTF8.GetString(buffer, 0, result.Count);
                            PlateText = msg; // Simplified: Parse JSON for Plate/YMMV
                        }
                    }
                }
                catch (Exception ex)
                {
                    StatusMessage = $"Connection Lost: {ex.Message}";
                    await Task.Delay(5000);
                    _ = ListenForAlertsAsync(jetsonIp);
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
