# GStreamer Environment Check for Sabre MDT
# Requires GStreamer MSVC 64-bit installation

$GSTREAMER_PATH = "C:\gstreamer\1.0\msvc_x86_64\bin"

Write-Host "Checking GStreamer Installation..." -ForegroundColor Cyan

if (Test-Path $GSTREAMER_PATH) {
    Write-Host "[OK] GStreamer found at $GSTREAMER_PATH" -ForegroundColor Green

    $envPath = [Environment]::GetEnvironmentVariable("PATH", "Machine")
    if ($envPath -like "*$GSTREAMER_PATH*") {
        Write-Host "[OK] GStreamer is in the System PATH" -ForegroundColor Green
    } else {
        Write-Host "[WARNING] GStreamer is NOT in the System PATH." -ForegroundColor Yellow
        Write-Host "Please add $GSTREAMER_PATH to your PATH environment variable."
    }

    # Check for GstSharp dependencies (conceptually)
    Write-Host "Verifying GstSharp runtime capabilities..."
    & "$GSTREAMER_PATH\gst-inspect-1.0.exe" --version
} else {
    Write-Host "[ERROR] GStreamer NOT found at $GSTREAMER_PATH" -ForegroundColor Red
    Write-Host "Please download and install GStreamer MSVC 64-bit (Runtime and Development)."
    exit 1
}
