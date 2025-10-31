# Script đơn giản để build và flash trong terminal VS Code
# Chạy: .\build_and_flash.ps1

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "ESP-IDF Build & Flash" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Source ESP-IDF environment
$idfPath = "C:\Users\phan\esp\v5.5.1\esp-idf"
$exportScript = Join-Path $idfPath "export.ps1"

if (Test-Path $exportScript) {
    Write-Host "[1/4] Đang load ESP-IDF environment..." -ForegroundColor Yellow
    & $exportScript | Out-Null
    Write-Host "       ESP-IDF đã sẵn sàng!" -ForegroundColor Green
} else {
    Write-Host "ERROR: Không tìm thấy ESP-IDF tại $idfPath" -ForegroundColor Red
    exit 1
}

# Build project
Write-Host "`n[2/4] Đang build project..." -ForegroundColor Yellow
idf.py build

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBuild thất bại!" -ForegroundColor Red
    exit 1
}

Write-Host "       Build thành công!`n" -ForegroundColor Green

# Flash project
Write-Host "[3/4] Đang flash project..." -ForegroundColor Yellow
Write-Host "       Nếu gặp lỗi quyền COM port, thử chạy script admin:`n" -ForegroundColor Cyan
Write-Host "       powershell -ExecutionPolicy Bypass -File flash_with_admin.ps1`n" -ForegroundColor Cyan

idf.py flash

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[4/4] Flash thành công!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
} else {
    Write-Host "`n[4/4] Flash thất bại! (Lỗi quyền?)" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "`nHãy thử chạy với quyền Admin:" -ForegroundColor Yellow
    Write-Host "powershell -ExecutionPolicy Bypass -File flash_with_admin.ps1" -ForegroundColor Cyan
}
