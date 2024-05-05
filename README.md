# IndyWV To WAV converter

IndyWV is a proprietary sound file format developed by LucasArts used in several of their games at the end of the 90s.
The data is compressed and audio cannot be played with commercial media players.
**indywvtowav.exe** can be used to convert those files to playable .wav files.

## Usage
```
-in <FilePath> : full path of input file, INDYWV or LAB. Type will be auto-deduced
-out <Folder> : path of output folder
[-unit_test] : performs unit test - checks algorithm integrity (optional)
```

Examples:
- to convert a single WV file and write the output WAV in the same folder
```
    indywvtowav -in ABM3627.wv -out .
```
- to parse a LAB archive file and extract all of its INDYWV files
```
    indywvtowav -in voice.lab -out .\converted_files\
```
- to perform the unit test (for developers: to check the algorithm's integrity when you make modifications). Files need to be inside a "UnitTest" subfolder.
```
    indywvtowav -unit_test
```

## Games
This tool has been tested on the following games:
- Star Wars Episode 1: The Phantom Menace (1999 - Windows) : the file VOICE.LAB on the game disc contains all the dialog files (mono) compressed with the ADPCM algorithm.
- Indiana Jones and the Infernal Machine (1999 - Windows) : the .NDY files extracted from the two CDs contain all SFX, dialog and music (mono and stereo files), compressed with the WVSM algorithm.

Please let me know if you are aware of other games that also use the INDYWV format! Also, I wasn't able to verify that the ADPCM algorithm works on stereo files so I'd love to add that usecase to the unit test.

## Credits
- WVSM decompression algorithm implemented by Crt Vavros in the [Urgon Mod Tools repository](https://github.com/smlu/Urgon)
- LABN archive format extraction was done by Guilherme Lampert. See [Reverse Engineering LucasArts Outlaws](https://github.com/glampert/reverse-engineering-outlaws)
- ADPCM decompression algorithm reverse-engineered and implemented by myself.
