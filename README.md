![EverythingDopus][1] EverythingDopus
=====================================

A command line interface to display [voidtools' Everything][2] search results
inside a collection in [Directory Opus][3], enabling the full range of Directory
Opus commands to be used with the results.

You need to have [voidtools' Everything][2] installed and running to use this
utility.

If you find any bug or difficulty using this utility, check the
[support topic][4] in the Directory Opus forums.


⚙️ Setup
-------

Extract the application package somewhere you're comfortable with (e.g. I use
`D:\Utils\EverythingDopus`). Please **avoid** folder where you require
Administrator permissions to run (i.e. *avoid Program Files*!)


💻 Directory Opus Setup
-----------------------

Once you have the application in a directory in your computer, open it in
Directory Opus and enable toolbar customization mode:

![CustomizeToolbars][6]

Drag the file `EverythingDopus.dcf` to your toolbar:

![DragToToolbar][7]

Close the Customize dialog in Directory Opus.

Copy the file `EverythingDopus.osp` to `/scripts`. If you have trouble
accessing that folder, that is just a shortcut for:
`%AppData%\GPSoftware\Directory Opus\Script AddIns`

And you're done!

The first time you attempt to search using the new toolbar button, it will ask
for the location of `ed.exe`, just choose it from the directory you previously
extracted the files.

If for some reason you need to change the selected executable (e.g., you pick
the wrong one), hold `shift` and click the button, and it will ask again for the
executable path.


🖥️ CLI Usage
------------

*(Optional)* If you plan to use EverythingDopusCLI straight from the command
line, you can add it to your path environment variables.


### Regular search
```
ed searchstring
```

### Regex search
```
ed /searchstring/
```


📌 About
--------

After trying [SearchEverythingCoreCLI][8], I decided to write a similar
application that suit my needs, seamlessly integrating [Everything][2] and
[Directory Opus][3] - so this project is heavily inspired by it.

The [Directory Opus script][5] also uses a utility function by [Wowbagger][9]
to aid the selection of the executable from the toolbar.


[1]: resources/EverythingDopusCLI-24x24.png
[2]: https://www.voidtools.com/
[3]: https://www.gpsoft.com.au/
[4]: https://resource.dopus.com/t/everythingdopus-an-utility-to-integrate-everything-with-directory-opus/43844
[5]: DirectoryOpus/EverythingDopus.js
[6]: resources/CustomizeToolbars.png
[7]: resources/DragToToolbar.png
[8]: https://github.com/devocalypse/SearchEverythingCoreCLI
[9]: https://resource.dopus.com/u/wowbagger
