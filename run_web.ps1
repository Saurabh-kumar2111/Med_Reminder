Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "      MediPulse C++ Smart Medicine Reminder System" -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "[1/2] Opening browser to http://localhost:8080/ ..." -ForegroundColor Yellow
Start-Process "http://localhost:8080"
Write-Host "[2/2] Launching C++ REST API Server..." -ForegroundColor Yellow
Write-Host "Press Ctrl+C to stop the server anytime." -ForegroundColor DarkGray
Write-Host ""
.\MedicineServer.exe

