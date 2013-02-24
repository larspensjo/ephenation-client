; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{C788FC6C-60BF-4D59-B6C5-EAB408F761F8}
AppName=Ephenation
AppVersion=4.3
AppPublisher=Ephenation
AppPublisherURL=www.ephenation.net
AppSupportURL=www.ephenation.net
AppUpdatesURL=TBD
DefaultDirName={pf}\Ephenation
DefaultGroupName=Ephenation
AllowNoIcons=yes
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes
InfoBeforeFile=../Disclaimer.rtf
InfoAfterFile=../readme.txt

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "ephenation.exe"; DestDir: "{app}"; DestName: "ephenation.exe"; Flags: ignoreversion
Source: "ephenation.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "../readme.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "contrib/win32/OpenAL/redist/oalinst.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "\MinGW\msys\1.0\bin\libvorbis-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "\MinGW\msys\1.0\bin\libogg-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "\MinGW\msys\1.0\bin\libvorbisfile-3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "\MinGW\msys\1.0\bin\glew32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "\MinGW\bin\pthreadGC2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "\MinGW\bin\libgcc_s_dw2-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "\MinGW\bin\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "../Disclaimer.rtf"; DestDir: "{app}"; Flags: ignoreversion
Source: "sounds\*"; DestDir: "{app}\sounds"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "textures\*"; DestDir: "{app}\textures"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "shaders\*.glsl"; DestDir: "{app}\shaders"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "dialogs\*"; DestDir: "{app}\dialogs"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "models\*"; DestDir: "{app}\models"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "fonts\*"; DestDir: "{app}\fonts"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\Ephenation"; Filename: "{app}\ephenation.exe"; IconFilename: "{app}\ephenation.ico"
Name: "{group}\{cm:UninstallProgram,Ephenation}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Ephenation"; Filename: "{app}\ephenation.exe"; Tasks: desktopicon; IconFilename: "{app}\ephenation.ico"

[Run]
Filename: "{app}\oalinst.exe"; StatusMsg: "Installing OpenAL library..."
Filename: "{app}\ephenation.exe"; Description: "{cm:LaunchProgram,Ephenation}"; Flags: nowait postinstall skipifsilent

