; PsPixel Installer Script
; Creates a professional installer like Photoshop

!include "MUI2.nsh"

; General settings
Name "PsPixel"
OutFile "PsPixel_Setup.exe"
InstallDir "$PROGRAMFILES64\PsPixel"
InstallDirRegKey HKCU "Software\PsPixel" ""
RequestExecutionLevel admin

; Interface settings
!define MUI_ABORTWARNING
!define MUI_ICON "assets\icon.ico"
!define MUI_UNICON "assets\icon.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"

; Version Information
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName" "PsPixel"
VIAddVersionKey "Comments" "Professional Pixel Art Editor"
VIAddVersionKey "CompanyName" "Jace Sleeman"
VIAddVersionKey "LegalTrademarks" "PsPixel is a trademark of Jace Sleeman"
VIAddVersionKey "LegalCopyright" "© 2024 Jace Sleeman"
VIAddVersionKey "FileDescription" "PsPixel Installer"
VIAddVersionKey "FileVersion" "1.0.0.0"

; Installer sections
Section "PsPixel" SecMain
  SetOutPath "$INSTDIR"
  
  ; Copy all files from deploy folder
  File /r "..\deploy\PsPixel\*.*"
  
  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\PsPixel"
  CreateShortCut "$SMPROGRAMS\PsPixel\PsPixel.lnk" "$INSTDIR\PsPixel.exe" "" "$INSTDIR\PsPixel.exe" 0
  CreateShortCut "$SMPROGRAMS\PsPixel\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortCut "$DESKTOP\PsPixel.lnk" "$INSTDIR\PsPixel.exe" "" "$INSTDIR\PsPixel.exe" 0
  
  ; Register file associations for .pspx files
  WriteRegStr HKCR ".pspx" "" "PsPixel.Document"
  WriteRegStr HKCR "PsPixel.Document" "" "PsPixel Document"
  WriteRegStr HKCR "PsPixel.Document\DefaultIcon" "" "$INSTDIR\PsPixel.exe,0"
  WriteRegStr HKCR "PsPixel.Document\shell\open\command" "" '"$INSTDIR\PsPixel.exe" "%1"'
  
  ; Store installation folder
  WriteRegStr HKCU "Software\PsPixel" "" $INSTDIR
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ; Add to Add/Remove Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel" "DisplayName" "PsPixel"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel" "DisplayIcon" "$INSTDIR\PsPixel.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel" "Publisher" "Jace Sleeman"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel" "DisplayVersion" "1.0.0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel" "NoRepair" 1
SectionEnd

; Uninstaller section
Section "Uninstall"
  ; Remove files
  RMDir /r "$INSTDIR"
  
  ; Remove shortcuts
  Delete "$SMPROGRAMS\PsPixel\*.*"
  RMDir "$SMPROGRAMS\PsPixel"
  Delete "$DESKTOP\PsPixel.lnk"
  
  ; Remove registry entries
  DeleteRegKey HKCR ".pspx"
  DeleteRegKey HKCR "PsPixel.Document"
  DeleteRegKey HKCU "Software\PsPixel"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\PsPixel"
SectionEnd 