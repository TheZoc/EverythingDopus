& ./Make-Release.ps1

Copy-Item -Path .\Out\x64\EverythingDopus.osp -Destination "$Env:AppData\GPSoftware\Directory Opus\Script AddIns\" -Verbose

