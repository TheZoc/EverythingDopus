![EverythingDopusCLI][1] EverythingDopusCLI
===========================================

A command line interface to display [Everything][2] search results inside
[Directory Opus][3], enabling the full range of Directory Opus commands to be
used with the results.


Setup
-----

Extract the application package somewhere you're comfortable with (e.g. I use
`D:\Utils\EverythingDopusCLI`). Please **avoid** folder where you require
Administrator permissions to run (i.e. *avoid Program Files*!)

*(Optional)* If you plan to use EverythingDopusCLI straight from the command
line, add it to your path environment variables.


Directory Opus Setup
--------------------

- Pending


CLI Usage
---------

### Regular search
```
ed searchstring
```

### Regex search
```
ed /searchstring/
```


About
-----

After trying [SearchEverythingCoreCLI][4], I decided to write a similar
application that suit my needs, seamlessly integrating [Everything][2]
and [Directory Opus][3].


[1]: resources/EverythingDopusCLI-24x24.png
[2]: https://www.voidtools.com/
[3]: https://www.gpsoft.com.au/
[4]: https://github.com/devocalypse/SearchEverythingCoreCLI