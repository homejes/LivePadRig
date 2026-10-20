#define MyAppName "LivePadRig"
#define MyAppVersion "1.2"
#define MyAppPublisher "AG Project"
#define MyAppExeName "LivePadRig"

[Setup]
AppId={{YOUR-UNIQUE-GUID-HERE}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=LivePadRig_Installer_v1.2
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64

[Files]
; Copy VST3 plugin to the standard Windows VST3 folder
Source: "Builds\VisualStudio2022\x64\Release\VST3\LivePadRig.vst3\*"; DestDir: "{commoncf64}\VST3\LivePadRig.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
; Copy the manual to the user's Documents folder
Source: "LIVEPAD RIG USER MANUAL.pdf"; DestDir: "{userdocs}\LivePadRig"; Flags: ignoreversion

[Icons]
; Optional: Create a shortcut to the manual in the Start Menu
Name: "{group}\LivePadRig User Manual"; Filename: "{userdocs}\LivePadRig\LIVEPAD RIG USER MANUAL.pdf"