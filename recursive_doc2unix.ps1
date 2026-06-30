Get-ChildItem -Path ".\JoltBindings" -Filter '*.*' -Recurse | ForEach-Object { dos2unix $_.FullName }
Get-ChildItem -Path ".\LibExportImportCppTest" -Filter '*.*' -Recurse | ForEach-Object { dos2unix $_.FullName }
Get-ChildItem -Path ".\joltphysics_v5.3.0" -Filter '*.*' -Recurse | ForEach-Object { dos2unix $_.FullName }

