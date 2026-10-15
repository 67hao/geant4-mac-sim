# Run_Analysis.ps1
# Chay Count.cc tren thu muc chua file hist_<Sample>_D<D>_E<E>.root
# (vd hist_S1_D0p01_E20.root, hist_S1_D0p002_E15.root), xuat MAC_Results.csv
#
# Cach dung:
#   Chinh $RootDir ben duoi roi bam chuot phai -> Run with PowerShell

# --- Tu dong setup moi truong CERN ROOT (neu chua co lenh 'root') ---
$ThisRootPs1 = "C:\root_v6.40.02\bin\thisroot.ps1"
if (-not (Get-Command root -ErrorAction SilentlyContinue)) {
    if (Test-Path $ThisRootPs1) {
        Write-Host "Dang nap moi truong ROOT tu: $ThisRootPs1" -ForegroundColor DarkCyan
        & $ThisRootPs1
    } else {
        Write-Host "[LOI] Khong tim thay thisroot.ps1 tai: $ThisRootPs1" -ForegroundColor Red
        Write-Host "[LOI] Hay kiem tra lai duong dan cai ROOT." -ForegroundColor Red
    }
}
# ----------------------------------------------------------------------

$RootDir   = "."                             # Thu muc chua .root files ("." = cung thu muc voi script)
$macroPath = Join-Path $PSScriptRoot "Count.cc"
$ok        = $true
# Kiem tra Count.cc
if (-not (Test-Path $macroPath)) {
    Write-Host "[LOI] Khong tim thay: $macroPath" -ForegroundColor Red
    $ok = $false
}
# Resolve duong dan ROOT files
if ($ok) {
    if (-not (Test-Path $RootDir)) {
        Write-Host "[LOI] Thu muc khong ton tai: $RootDir" -ForegroundColor Red
        $ok = $false
    } else {
        $rootDir = Resolve-Path $RootDir
    }
}
# Kiem tra co file .root khong
if ($ok) {
    $rootFiles = Get-ChildItem -Path $rootDir -Filter "hist_*.root"
    if ($rootFiles.Count -eq 0) {
        Write-Host "[LOI] Khong tim thay file hist_*.root trong: $rootDir" -ForegroundColor Yellow
        $ok = $false
    }
}
# Chay ROOT macro
if ($ok) {
    Write-Host "Tim thay $($rootFiles.Count) file ROOT trong: $rootDir" -ForegroundColor Cyan
    Write-Host "Chay Count.cc ..."
    Write-Host ("-" * 60)
    Push-Location $rootDir
    root -l -q $macroPath
    Pop-Location
    Write-Host ("-" * 60)
    $resultFile = Join-Path $rootDir "MAC_Results.csv"
    if (Test-Path $resultFile) {
        Write-Host "Da luu: $resultFile" -ForegroundColor Green
        Write-Host ""
        Write-Host "Preview (10 dong dau):"
        Get-Content $resultFile -Encoding UTF8 | Select-Object -First 10
    } else {
        Write-Host "[LOI] MAC_Results.csv khong duoc tao -- xem log ROOT o tren." -ForegroundColor Red
    }
}
Write-Host ""
cmd /c pause
