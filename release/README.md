# LivePadRig — Struktur Folder `release/`

Folder ini di-generate otomatis oleh script build release. **Jangan commit isi folder ini**
(sudah masuk `.gitignore`). Jalankan script sesuai OS target:

| OS | Script | Output |
|----|--------|--------|
| Windows | `scripts/build_release_windows.ps1` | `release/windows/` |
| macOS   | `scripts/build_release_macos.sh [--dmg]` | `release/macos/` |

## Struktur hasil build

```
release/
├── README.md                          (file ini)
├── windows/
│   ├── LivePadRig_Windows_v1.3/       (folder portable siap distribusi)
│   │   ├── LivePadRig.vst3/           (plugin VST3 64-bit)
│   │   ├── Audio/                     (aset pad & drum kit bawaan)
│   │   ├── config/default_settings.xml (state default → %APPDATA%\LivePadRig\settings.xml)
│   │   ├── LIVEPAD RIG USER MANUAL.pdf
│   │   └── README.txt                 (petunjuk instalasi)
│   ├── LivePadRig_Windows_v1.3.zip    (portable, zip dari folder di atas)
│   └── LivePadRig_Installer_v1.3.exe  (installer Inno Setup, jika ISCC tersedia)
│
└── macos/
    ├── LivePadRig_macOS_v1.3/         (folder portable siap distribusi)
    │   ├── LivePadRig.vst3/           (plugin VST3)
    │   ├── LivePadRig.component/      (plugin AU)
    │   ├── Audio/                     (aset pad & drum kit bawaan)
    │   ├── config/default_settings.xml (state default → ~/Library/Application Support/LivePadRig/)
    │   ├── LIVEPAD RIG USER MANUAL.pdf
    │   └── README.txt                 (petunjuk instalasi)
    └── LivePadRig_macOS_v1.3.dmg      (opsional, jalankan dengan flag --dmg)
```

## Lokasi default state & audio per OS

| Item | Windows | macOS |
|------|---------|-------|
| Aset audio | `C:\Users\Public\Documents\LivePadRig\Audio` | `/Users/Shared/LivePadRig/Audio` |
| State (settings.xml) | `%APPDATA%\LivePadRig\settings.xml` | `~/Library/Application Support/LivePadRig/settings.xml` |
| Preset user | `%APPDATA%\LivePadRig\presets\*.xml` | `~/Library/Application Support/LivePadRig/presets/*.xml` |
| Preset MIDI | `%APPDATA%\LivePadRig\midi_presets\*.xml` | `~/Library/Application Support/LivePadRig/midi_presets/*.xml` |

## Prasyarat build

**Windows (lokal):**
1. Visual Studio 2022 (dengan workload Desktop C++) — project VS sudah di-commit di `Builds/VisualStudio2022/`
2. JUCE di `D:\JUCE\JUCE` (path default `.jucer`) — atau biarkan script clone otomatis
3. Inno Setup 6 (opsional, untuk `.exe` installer) — tanpa ini script tetap menghasilkan portable ZIP

**macOS:**
1. Xcode + Command Line Tools
2. JUCE akan di-clone otomatis oleh script

**CI (GitHub Actions):** kedua workflow di `.github/workflows/` sudah diperbarui ke v1.3 dan
menghasilkan installer + portable package sebagai artifact — cukup jalankan lewat tab
*Actions → Run workflow*.
