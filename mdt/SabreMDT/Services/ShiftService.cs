using System;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json;

namespace SabreMDT.Services
{
    public class ShiftService
    {
        private readonly string _hubApiUrl = "http://192.168.1.10:8000";

        public async Task<bool> StartShiftAsync(string officerId, string vehicleId)
        {
            try
            {
                using (var client = new HttpClient())
                {
                    var payload = new
                    {
                        officer_id = officerId,
                        vehicle_id = vehicleId,
                        accepted_at = DateTime.UtcNow.ToString("o"),
                        event_type = "DISCLAIMER_ACCEPTED"
                    };

                    var content = new StringContent(JsonConvert.SerializeObject(payload), Encoding.UTF8, "application/json");

                    // Audit trail is recorded on the Hub and synced to the Command Center
                    var response = await client.PostAsync($"{_hubApiUrl}/shift/audit", content);

                    if (response.IsSuccessStatusCode)
                    {
                        Console.WriteLine($"Audit logged: Officer {officerId} started shift in {vehicleId}");
                        return true;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Shift Audit Failed: {ex.Message}");
            }
            return false;
        }
    }
}
