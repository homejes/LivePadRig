#define MyAppName "LivePadRig"
#define MyAppVersion "1.3.1"
#define MyAppPublisher "AG Project"
#define MyAppExeName "LivePadRig"

[Setup]
AppId={{A7C4B2E1-5F3D-4E8A-9C6B-1D2E3F4A5B6C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DisableDirPage=no
DefaultDirName={commondocs}\{#MyAppName}\Audio
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=LivePadRig_Installer_v{#MyAppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
DirExistsWarning=no

[Messages]
WizardSelectDir=Select Audio Data Location
SelectDirDesc=Where should the LivePadRig audio files (Pads & Drums) be installed?
SelectDirLabel3=Setup will install the audio data into the following folder. The VST3 plugin will be automatically installed to your Common Files/VST3 folder.

[Files]
; 1) Plugin VST3 ke folder standar Windows (64-bit)
Source: "Builds\VisualStudio2022\x64\Release\VST3\LivePadRig.vst3\*"; DestDir: "{commoncf64}\VST3\LivePadRig.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

; 2) Aset audio (pads & drums) ke folder audio yang dipilih user (default: Public Documents)
Source: "assets\audio\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; 3) Konfigurasi default - hanya dibuat jika user BELUM punya settings.xml
;    (agar tidak menimpa state/preset lama user saat reinstall/upgrade)
Source: "config\default_settings_windows.xml"; DestDir: "{userappdata}\LivePadRig"; DestName: "settings.xml"; Flags: ignoreversion onlyifdoesntexist

; 4) Manual ke folder Documents\LivePadRig
Source: "LIVEPAD RIG USER MANUAL.pdf"; DestDir: "{userdocs}\LivePadRig"; Flags: ignoreversion

[Icons]
Name: "{group}\LivePadRig User Manual"; Filename: "{userdocs}\LivePadRig\LIVEPAD RIG USER MANUAL.pdf"

[Run]
Filename: "{userdocs}\LivePadRig\LIVEPAD RIG USER MANUAL.pdf"; Description: "View LivePadRig User Manual"; Flags: shellexec postinstall skipifsilent
