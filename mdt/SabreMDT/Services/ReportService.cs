using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace SabreMDT.Services
{
    public class ReportService
    {
        public string GenerateShiftReport(string officerId, string vehicleId, IEnumerable<dynamic> hits)
        {
            var html = new StringBuilder();
            html.Append("<html><head><style>body{font-family:Arial;background:#f4f4f4;}table{width:100%;border-collapse:collapse;}th,td{padding:8px;border:1px solid #ddd;text-align:left;}th{background:#333;color:white;}</style></head><body>");
            html.Append($"<h1>Sabre ALPR Hub - Shift Report</h1>");
            html.Append($"<p><b>Officer:</b> {officerId} | <b>Vehicle:</b> {vehicleId}</p>");
            html.Append($"<p><b>Generated:</b> {DateTime.Now:yyyy-MM-dd HH:mm:ss}</p>");

            html.Append("<table><tr><th>Plate</th><th>YMMV</th><th>GPS / Maps</th><th>Speed</th><th>SHA-256 Hash</th></tr>");

            foreach (var hit in hits)
            {
                string mapsLink = $"https://www.google.com/maps?q={hit.Lat},{hit.Lon}";
                html.Append("<tr>");
                html.Append($"<td><b>{hit.Plate}</b></td>");
                html.Append($"<td>{hit.Ymmv}</td>");
                html.Append($"<td><a href='{mapsLink}'>{hit.Lat}, {hit.Lon}</a></td>");
                html.Append($"<td>{hit.Speed} KPH</td>");
                html.Append($"<td><small>{hit.Hash}</small></td>");
                html.Append("</tr>");
            }

            html.Append("</table></body></html>");

            string path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Desktop), $"SabreReport_{DateTime.Now:yyyyMMdd_HHmm}.html");
            File.WriteAllText(path, html.ToString());
            return path;
        }
    }
}
