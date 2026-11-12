# ============================================================================
#  LivePadRig - Build & Packaging Script (WINDOWS)
#  Membangun plugin VST3 Release lalu merapikan hasilnya ke folder release/
#
#  Pemakaian (jalankan dari repo root, PowerShell):
#    powershell -ExecutionPolicy Bypass -File scripts\build_release_windows.ps1
#    powershell -ExecutionPolicy Bypass -File scripts\build_release_windows.ps1 -SkipBuild
#
#  Output:
#    release\windows\LivePadRig_Windows_v<ver>\   (folder portable siap zip)
#    release\windows\LivePadRig_Windows_v<ver>.zip
#    release\windows\LivePadRig_Installer_v<ver>.exe   (jika Inno Setup terpasang)
# ============================================================================
param(
    [switch]$SkipBuild,
    [string]$Version = "1.3.1",
    [string]$JucePath = ""   # Opsional: path ke folder JUCE (berisi ./modules)
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot   # folder repo LivePadRig

Write-Host "=== LivePadRig Windows Release Build v$Version ===" -ForegroundColor Cyan
Write-Host "Repo: $RepoRoot"

# ---------------------------------------------------------------------------
# 1) Pastikan JUCE tersedia (untuk rebuild Projucer-generated project)
# ---------------------------------------------------------------------------
$jucerPath  = Join-Path $RepoRoot "LivePadRig.jucer"
if (-not $JucePath) {
    # Path default yang dipakai .jucer: ../../../JUCE/JUCE/modules -> D:\JUCE\JUCE
    $candidate = Join-Path $RepoRoot "..\..\..\JUCE\JUCE"
    if (Test-Path (Join-Path $candidate "modules")) { $JucePath = (Resolve-Path $candidate).Path }
}
if (-not $JucePath -or -not (Test-Path (Join-Path $JucePath "modules"))) {
    Write-Warning "JUCE tidak ditemukan otomatis. Cloning JUCE ke repo..."
    $JucePath = Join-Path $RepoRoot "JUCE"
    if (-not (Test-Path $JucePath)) {
        git clone --depth 1 https://github.com/juce-framework/JUCE.git $JucePath
    }
}
Write-Host "JUCE  : $JucePath"

# ---------------------------------------------------------------------------
# 2) Lokalisasi MSBuild
# ---------------------------------------------------------------------------
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { throw "vswhere tidak ditemukan. Install Visual Studio 2022." }
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild tidak ditemukan." }
Write-Host "MSBuild: $msbuild"

# ---------------------------------------------------------------------------
# 3) Build Release x64 (atau lewati dengan -SkipBuild)
# ---------------------------------------------------------------------------
$sln = Join-Path $RepoRoot "Builds\VisualStudio2022\LivePadRig.sln"
if (-not (Test-Path $sln)) {
    Write-Warning "Project VS2022 belum ada. Generate dengan Projucer (--resave) terlebih dahulu."
    $projucer = Get-ChildItem -Path (Join-Path $JucePath "extras\Projucer\Builds\VisualStudio2022") -Recurse -Filter "Projucer.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $projucer) { throw "Projucer.exe tidak ditemukan. Bangun JUCE Projucer terlebih dahulu." }
    & $projucer.FullName --resave $jucerPath
}

if (-not $SkipBuild) {
    Write-Host ">>> Building LivePadRig.sln (Release|x64)..." -ForegroundColor Yellow
    & $msbuild $sln "/p:Configuration=Release" "/p:Platform=x64" /m /v:m /nologo
    if ($LASTEXITCODE -ne 0) { throw "Build GAGAL (exit code $LASTEXITCODE)." }
    Write-Host ">>> Build sukses." -ForegroundColor Green
}

$vst3Source = Join-Path $RepoRoot "Builds\VisualStudio2022\x64\Release\VST3\LivePadRig.vst3"
if (-not (Test-Path $vst3Source)) { throw "LivePadRig.vst3 tidak ditemukan di: $vst3Source" }

# ---------------------------------------------------------------------------
# 4) Rakit folder release/windows/<name>/
# ---------------------------------------------------------------------------
$releaseRoot = Join-Path $RepoRoot "release\windows"
$pkgName     = "LivePadRig_Windows_v$Version"
$pkgDir      = Join-Path $releaseRoot $pkgName

Write-Host ">>> Menyusun paket: $pkgDir"
if (Test-Path $pkgDir) { Remove-Item $pkgDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $pkgDir, (Join-Path $pkgDir "Audio"), (Join-Path $pkgDir "config") | Out-Null

# 4a) Plugin binary
Copy-Item $vst3Source (Join-Path $pkgDir "LivePadRig.vst3") -Recurse -Force

# 4b) Aset audio (pads & drums)
$assetsAudio = Join-Path $RepoRoot "assets\audio"
if (Test-Path $assetsAudio) {
    Copy-Item (Join-Path $assetsAudio "*") (Join-Path $pkgDir "Audio") -Recurse -Force
} else {
    Write-Warning "Aset audio tidak ditemukan di $assetsAudio"
}

# 4c) Konfigurasi default (rootSoundDir mengarah ke lokasi install standar Windows)
$defaultCfg = Get-Content (Join-Path $RepoRoot "config\default_settings.xml") -Raw
$defaultCfg = $defaultCfg.Replace("__LIVEPADRIG_AUDIO_DIR__", "C:\Users\Public\Documents\LivePadRig\Audio")
Set-Content -Path (Join-Path $pkgDir "config\default_settings.xml") -Value $defaultCfg -Encoding UTF8

# 4d) Manual (jika ada)
$manual = Join-Path $RepoRoot "LIVEPAD RIG USER MANUAL.pdf"
if (Test-Path $manual) { Copy-Item $manual $pkgDir -Force }

# 4e) README instalasi
$readmeText = @"
LIVEPADRIG v$Version - WINDOWS
==============================

ISI PAKET
- LivePadRig.vst3\            : plugin VST3 (64-bit)
- Audio\                      : aset pad & drum kit bawaan
- config\default_settings.xml : konfigurasi default

CARA PASANG (PORTABLE)
1. Salin folder LivePadRig.vst3 ke folder VST3 64-bit:
   C:\Program Files\Common Files\VST3\  (perlu hak admin)
2. Salin isi folder Audio\ ke:
   C:\Users\Public\Documents\LivePadRig\Audio\
3. Salin config\default_settings.xml ke:
   %APPDATA%\LivePadRig\settings.xml
   (langkah 2-3 opsional; bisa juga lewat tombol LOCATE AUDIO di plugin)
4. Buka DAW, scan ulang plugin VST3.

CARA PASANG (INSTALLER)
Jalankan LivePadRig_Installer_v$Version.exe - semua langkah di atas dilakukan otomatis.

CATATAN STATE v1.3
- Semua fader/DSP/preset/routing MIDI tersimpan otomatis dan dipulihkan
  saat aplikasi dibuka kembali.
- Dalam DAW, seluruh state ikut tersimpan di file project (VST3 state).
"@
Set-Content -Path (Join-Path $pkgDir "README.txt") -Value $readmeText -Encoding UTF8

# ---------------------------------------------------------------------------
# 5) ZIP portable
# ---------------------------------------------------------------------------
$zipPath = Join-Path $releaseRoot "$pkgName.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Write-Host ">>> Membuat zip: $zipPath"
Compress-Archive -Path $pkgDir -DestinationPath $zipPath -CompressionLevel Optimal

# ---------------------------------------------------------------------------
# 6) Installer EXE via Inno Setup (opsional, jika ISCC tersedia)
# ---------------------------------------------------------------------------
$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (Test-Path $iscc) {
    Write-Host ">>> Kompilasi installer (Inno Setup)..."
    Push-Location $RepoRoot
    & $iscc "installer.iss" /DAppVersion=$Version
    Pop-Location
    $installerOut = Join-Path $RepoRoot "Output\LivePadRig_Installer_v$Version.exe"
    if (Test-Path $installerOut) { Copy-Item $installerOut (Join-Path $releaseRoot (Split-Path $installerOut -Leaf)) -Force }
} else {
    Write-Warning "Inno Setup (ISCC.exe) tidak terpasang - installer EXE dilewati. ZIP portable tetap tersedia."
}

Write-Host ""
Write-Host "=== SELESAI. Output di: $releaseRoot ===" -ForegroundColor Green
Get-ChildItem $releaseRoot | Format-Table Name, Length -AutoSize
