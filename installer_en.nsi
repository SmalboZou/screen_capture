; AIcp Screen Recording Tool Installation Script
; Using NSIS 3.x

;--------------------------------
; Include Modern UI
!include "MUI2.nsh"

;--------------------------------
; Basic Information
!define PRODUCT_NAME "AIcp"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "AIcp Team"
!define PRODUCT_WEB_SITE "https://github.com/SmalboZou/screen_capture"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\AIcp.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Set installer name
Name "${PRODUCT_NAME} Setup"

; Set output file name
OutFile "AIcp_Setup_v${PRODUCT_VERSION}.exe"

; Set default installation directory
InstallDir "$PROGRAMFILES64\AIcp"

; Get installation path from registry
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""

; Request administrator privileges
RequestExecutionLevel admin

; Show installation details
ShowInstDetails show
ShowUnInstDetails show

; Compression method
SetCompressor lzma

;--------------------------------
; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "resources\app.ico"
!define MUI_UNICON "resources\app.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; License agreement page
!insertmacro MUI_PAGE_LICENSE "LICENSE"

; Choose installation directory page
!insertmacro MUI_PAGE_DIRECTORY

; Start menu folder page
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "NSIS:StartMenuDir"
var StartMenuFolder
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

; Installation page
!insertmacro MUI_PAGE_INSTFILES

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\AIcp.exe"
!insertmacro MUI_PAGE_FINISH

; Uninstall pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Version Information
VIProductVersion "${PRODUCT_VERSION}.0"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "AIcp Screen Recording Tool"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "© 2025 ${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "AIcp Screen Recording Tool Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${PRODUCT_VERSION}"

;--------------------------------
; Installation Section
Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  
  ; Copy main program and Qt dependencies
  File /r "deploy\*.*"
  
  ; Copy resource files
  SetOutPath "$INSTDIR\resources"
  File "resources\app.ico"
  File "resources\app.png"

  ; Create start menu shortcuts
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\AIcp.lnk" "$INSTDIR\AIcp.exe" "" "$INSTDIR\resources\app.ico" 0
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall AIcp.lnk" "$INSTDIR\uninst.exe" "" "$INSTDIR\resources\app.ico" 0
  !insertmacro MUI_STARTMENU_WRITE_END

  ; Create desktop shortcut
  CreateShortCut "$DESKTOP\AIcp.lnk" "$INSTDIR\AIcp.exe" "" "$INSTDIR\resources\app.ico" 0
  
  ; Registry entries
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\AIcp.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\resources\app.ico"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd

;--------------------------------
; Uninstall Section
Section Uninstall
  ; Delete registry keys
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  
  ; Delete shortcuts
  Delete "$DESKTOP\延时录屏工具.lnk"
  
  ; Delete start menu items
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\延时录屏工具.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall 延时录屏工具.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
  
  ; Delete all files and directories
  RMDir /r "$INSTDIR"
  
  SetAutoClose true
SectionEnd

;--------------------------------
; Callback Functions
Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) has been successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd
