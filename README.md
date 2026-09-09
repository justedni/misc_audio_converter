# Misc Audio Formats Converter

This tool converts some obscure or rare audio formats used in various video games, new or old. Whenever I couldn't find an existing converter online, I implemented it myself.

## Handled conversions
- [INDYWV](#indywv-and-lab) to WAV (mono, stereo ADPCM, or stereo WVSM)
- WAV to [INDYWV](#indywv-and-lab) (mono ADPCM only)
- [LAB](#indywv-and-lab) (with embedded INDYWVs) to WAVs
- [Cryo APC](#apc) to WAV
- [Xatrix C93, Z94 and M95](#xatrix-c93-z94-and-m95) to VOC to WAV
- [IntiCreates's BIGRP sequences](#bigrp) to MIDI files


## Handled formats

### IndyWV and LAB

IndyWV is a sound file format developed by LucasArts. It was used in multiple games in the late 90s.
LAB was another format used as a collection of files. The tool can convert individual INDYWV files but it can also extract a LAB file that contains multiple INDYWV files and convert them in a batch.

Tested games:
- Star Wars Episode 1: The Phantom Menace (1999, PC) : the file VOICE.LAB on the game disc contains all the dialog files (mono) compressed with the ADPCM algorithm.
- Indiana Jones and the Infernal Machine (1999, PC) : the .NDY files extracted from the two CDs contain all SFX, dialog and music (mono and stereo files), compressed with the WVSM algorithm.

Please let me know if you are aware of other games that also use the INDYWV format! Especially the ADPCM variant.

### APC

This format was used in several point'n click games developed by Cryo in the 90s and early 2000s:
- Egypt 1156 B.C. – Tomb of the Pharaoh (1997) and its sequel, Egypt 2 - The Heliopolis Prophecy (2000)
- China: The Forbidden City (1998)
- Atlantis: The Lost Tales (1997), Atlantis 2 (1999)

### Xatrix C93, Z94 and M95

The game Cyberia (1994) by Xatrix Entertainment uses multiple proprietary formats:
- C93 represents an in-game movie (during the rail shooter sequences). The audio track is made of uncompressed .voc (Creative Voice File) chunks. The tool glues all the chunks together and converts them to .wav.
- Z94 is a container for various game data. Some of the game files contain audio data, stored as compressed chunks. The tool extracts all the entries inside the .z94 container and decompresses them (a Huffman algorithm was used) to .voc then to .wav.

Its sequel, Cyberia 2: Resurrection (1996) introduced a new format:
- M95 is the new container for in-game movies or sequences, its format is very similar to C93. This time, audio is stored as plain RIFF, so no need to convert from VOC anymore.

### BIGRP

Inti Creates' BIGRP files, which uses ICE (Inti Creates Engine), include embedded MIDI sequences in some of their games. Each song is split into smaller MIDI files. This tool can regroup those sequences by song, grouping tracks together and fixing any MIDI-related conversion problems at the same time.
Currently handled games:
- Bloodstained: Curse of the Moon ("Cotm1")
- Bloodstained: Curse of the Moon 2 ("Cotm2")

Important:
- the tool expects to find decrypted .bigrp files (the tool doesn't decrypt the files at the moment)
- you need to specify the game Id when converting Bigrp files, using the -game option. Example: `-game Cotm1`. The tool needs to do some conversions/remappings that are game-specific.

TODO (WIP):
- conversion of the embedded .wav files into a .sf2
- handle conversion of .bcgrp files (equivalent of BIGRP containers on the 3DS)


## Usage
```
-in <FileOrFolderPath> : full path of input file or folder (file types will be auto-deduced)
-out <FileOrFolderPath> : path of output file or output folder
[-unit_test] : performs unit test - checks algorithm integrity (optional)
```

Examples:
- to convert a single WV file and write the output WAV in the same folder:  `convert -in ABM3627.wv -out .`
- to parse a LAB archive file and extract all of its INDYWV files: `convert -in voice.lab -out ".\converted_files" `
- to parse a folder containing hundreds of APC files and convert them to a subfolder: ` convert -in "C:\apc_files" -out "C:\converted_files" `
- to perform the unit test (for developers: to check the algorithm's integrity when you make modifications). The program assumes that the test files are located in a "..\..\UnitTest" subfolder: ` convert -unit_test `


## Credits
- INDYWV ADPCM decompression algorithm reverse-engineered and implemented by myself.
- INDYWV WVSM decompression algorithm implemented by Crt Vavros in the [Urgon Mod Tools repository](https://github.com/smlu/Urgon)
- LABN archive format extraction was done by Guilherme Lampert. See [Reverse Engineering LucasArts Outlaws](https://github.com/glampert/reverse-engineering-outlaws)
- APC specifications and decompression algorithm: [CRYO APC - MultimediaWiki](https://wiki.multimedia.cx/index.php/CRYO_APC)
- ICE format specifications (for Inti BIGRP) inspired by [vgmstream](https://github.com/vgmstream/vgmstream/tree/master)

## Dependencies
- [craigsapp's midifile](https://github.com/craigsapp/midifile)
