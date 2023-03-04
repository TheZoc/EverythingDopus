$script:SevenZip = "${env:ProgramFiles}\7-Zip\7z.exe"
$script:VSDevShell = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"

& $script:VSDevShell -SkipAutomaticLocation

Remove-Item .\Out\ -Recurse -Verbose -ErrorAction SilentlyContinue
Remove-Item .\x64\ -Recurse -Verbose -ErrorAction SilentlyContinue
Remove-Item .\Release\ -Recurse -Verbose -ErrorAction SilentlyContinue

MSBuild.exe .\EverythingDopusCLI.sln -t:Rebuild -p:Configuration=Release -p:Platform=x86
MSBuild.exe .\EverythingDopusCLI.sln -t:Rebuild -p:Configuration=Release -p:Platform=x64

New-Item .\Out\ -ItemType Directory
New-Item .\Out\x86\ -ItemType Directory
New-Item .\Out\x64\ -ItemType Directory

Copy-Item -Path .\x64\Release\ed.exe -Destination ".\Out\x64\"
Copy-Item -Path .\Release\ed.exe -Destination ".\Out\x86\"

Copy-Item -Path .\libs\Everything-SDK\dll\Everything64.dll -Destination ".\Out\x64\"
Copy-Item -Path .\libs\Everything-SDK\dll\Everything32.dll -Destination ".\Out\x86\"

& $script:SevenZip a -tzip .\Out\x64\EverythingDopus.osp ".\DirectoryOpus\*"
& $script:SevenZip a -tzip .\Out\x86\EverythingDopus.osp ".\DirectoryOpus\*"

Copy-Item -Path .\resources\EverythingDopus.dcf -Destination ".\Out\x64\"
Copy-Item -Path .\resources\EverythingDopus.dcf -Destination ".\Out\x86\"

& $script:SevenZip a -tzip .\Out\EverythingDopus-x64.zip ".\Out\x64\*"
& $script:SevenZip a -tzip .\Out\EverythingDopus-x86.zip ".\Out\x86\*"
