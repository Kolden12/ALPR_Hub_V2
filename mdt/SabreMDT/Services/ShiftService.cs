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
                    var data = new
                    {
                        officer_id = officerId,
                        vehicle_id = vehicleId,
                        timestamp = DateTime.UtcNow.ToString("o")
                    };
                    var content = new StringContent(JsonConvert.SerializeObject(data), Encoding.UTF8, "application/json");
                    var response = await client.PostAsync($"{_hubApiUrl}/shift/start", content);
                    return response.IsSuccessStatusCode;
                }
            }
            catch
            {
                return false;
            }
        }
    }
}
