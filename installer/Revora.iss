#define MyAppName "Revora"
#ifndef MyAppVersion
  #define MyAppVersion "0.2.0"
#endif
#define MyAppPublisher "REVORA"
#define BuildDir "..\build\Revora_artefacts\Release"

[Setup]
AppId={{F2853749-DA50-426B-9A9F-C7E3FF9E8ED2}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=Output
OutputBaseFilename=Revora-{#MyAppVersion}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\Assets\logo.ico
UninstallDisplayIcon={app}\logo.ico
WizardImageFile=..\Assets\wizard_big.bmp
WizardSmallImageFile=..\Assets\wizard_small.bmp
UninstallDisplayName={#MyAppName}

[Types]
Name: "full"; Description: "Full installation (VST3 plugin + standalone app)"
Name: "vst3only"; Description: "VST3 plugin only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full vst3only custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full

[Files]
Source: "{#BuildDir}\VST3\{#MyAppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#MyAppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\{#MyAppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

; icon used by the uninstall entry and the start menu shortcut
Source: "..\Assets\logo.ico"; DestDir: "{app}"; Flags: ignoreversion

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: standalone; Flags: unchecked

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"; Components: standalone
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"; Components: standalone; Tasks: desktopicon

[Messages]
FinishedLabel=Revora has been installed.%n%nThe VST3 plugin is in C:\Program Files\Common Files\VST3. Rescan plugins in your DAW if Revora does not show up.%n%nUser presets are kept in %APPDATA%\Revora\Presets and are NOT removed when you uninstall.
