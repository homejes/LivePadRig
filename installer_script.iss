[Setup]
AppName=LivePadRig
AppVersion=1.0
AppPublisher=Ryan Dharmawan
DefaultDirName={autodocs}\LivePadRig_Audio
DisableProgramGroupPage=yes
OutputDir=.\InstallerOutput
OutputBaseFilename=LivePadRig_Win_Installer
Compression=lzma
SolidCompression=yes
DirExistsWarning=no
PrivilegesRequired=admin

; Perbaikan warning x64 menjadi x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
; Menyalin Audio Folder ke lokasi yang dipilih user
Source: "assets\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; Menyalin PDF Manual ke folder audio
Source: "LIVEPAD RIG USER MANUAL.pdf"; DestDir: "{app}"; Flags: ignoreversion

; Menyalin VST3 langsung ke folder sistem VST3 64-bit
Source: "Builds\VisualStudio2022\x64\Release\VST3\LivePadRig.vst3\*"; DestDir: "{commoncf}\VST3\LivePadRig.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Run]
Filename: "{app}\LIVEPAD RIG USER MANUAL.pdf"; Description: "View LivePadRig User Manual"; Flags: shellexec postinstall skipifsilent

[Messages]
WizardSelectDir=Select Audio Data Location
SelectDirDesc=Where should the LivePadRig audio files (Pads & Drums) be installed?
SelectDirLabel3=Setup will install the audio data and manual into the following folder. The VST3 plugin will be automatically installed to your Common Files/VST3 folder.