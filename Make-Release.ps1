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

&'C:\Program Files\7-Zip\7z.exe' a -tzip .\Out\x64\EverythingDopus.osp ".\DirectoryOpus\*"
&'C:\Program Files\7-Zip\7z.exe' a -tzip .\Out\x86\EverythingDopus.osp ".\DirectoryOpus\*"
