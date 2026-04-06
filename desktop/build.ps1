Write-Host "=== PsPixel Build & Package ===" -ForegroundColor Green

# Set all paths
$env:PATH = "C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Program Files (x86)\NSIS;" + $env:PATH

Write-Host "Step 1: Checking icons..." -ForegroundColor Yellow
if (-not (Test-Path "assets\icon.png")) {
    Write-Host "ERROR - icon.png not found!" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path "assets\icon.ico")) {
    Write-Host "ERROR - icon.ico not found!" -ForegroundColor Red
    exit 1
}
Write-Host "OK - Using existing icon files" -ForegroundColor Green

Write-Host "Step 2: Building..." -ForegroundColor Yellow
if (Test-Path "build\CMakeCache.txt") { Remove-Item "build\CMakeCache.txt" }
if (Test-Path "build\CMakeFiles") { Remove-Item "build\CMakeFiles" -Recurse -Force }
if (-not (Test-Path "build")) { New-Item -ItemType Directory "build" | Out-Null }

Set-Location "build"
cmake -G Ninja -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\mingw_64" -DCMAKE_BUILD_TYPE=Release ..
if ($LASTEXITCODE -ne 0) { 
    Write-Host "ERROR - Build failed" -ForegroundColor Red
    Set-Location ".."
    exit 1
}
ninja
if ($LASTEXITCODE -ne 0) { 
    Write-Host "ERROR - Build failed" -ForegroundColor Red
    Set-Location ".."
    exit 1
}
Set-Location ".."
Write-Host "OK - Built successfully" -ForegroundColor Green

Write-Host "Step 3: Deploying..." -ForegroundColor Yellow
if (Test-Path "deploy") { Remove-Item "deploy" -Recurse -Force }
New-Item -ItemType Directory -Path "deploy\PsPixel" -Force | Out-Null
Copy-Item "build\bin\PsPixel.exe" "deploy\PsPixel\"
Copy-Item "assets\icon.png" "deploy\PsPixel\" -ErrorAction SilentlyContinue
Copy-Item "assets\icon.ico" "deploy\PsPixel\" -ErrorAction SilentlyContinue
& "C:\Qt\6.9.1\mingw_64\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw "deploy\PsPixel\PsPixel.exe" | Out-Null
Write-Host "OK - Deployed successfully" -ForegroundColor Green

Write-Host "Step 4: Creating installer..." -ForegroundColor Yellow
makensis "installer.nsi" | Out-Null
Write-Host "OK - Installer created" -ForegroundColor Green

Write-Host ""
Write-Host "SUCCESS!" -ForegroundColor Green
Write-Host "Portable app: deploy\PsPixel\" -ForegroundColor Cyan  
Write-Host "Installer: PsPixel_Setup.exe" -ForegroundColor Cyan

if (Test-Path "PsPixel_Setup.exe") {
    $size = [math]::Round((Get-Item "PsPixel_Setup.exe").Length/1MB, 1)
    Write-Host "Installer size: $size MB" -ForegroundColor White
}

Write-Host ""
Write-Host "Ready to distribute!" -ForegroundColor Yellow
pause 