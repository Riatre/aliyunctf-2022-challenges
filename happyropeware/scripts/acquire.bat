@echo off

cd /D "%~dp0"

winpmem_mini_x64_rc2.exe memory.raw
wmic shadowcopy call create Volume=C:\
CyLR.exe -of forensic-artifacts.zip -d custom.txt --usnjrnl
dism /Capture-Image /ImageFile:user-files.wim /CaptureDir:\\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1\Users /Name:"User Files"
pause
