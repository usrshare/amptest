# amptest

This is a very experimental program that tries to use Winamp's plugins (version 2.95 or newer) to play audio files, while imitating the classic Winamp interface and (potentially) expanding on its features.

Currently, the program depends on the following files from a Winamp installation:

 * plugins\\in\_mp3.dll (or any other input plugin, provided you have the appropriate media files)
 * plugins\\out\_wave.dll

The program's source code also borrows the input and output plugin structures from the official Winamp SDK (as available [here](http://wiki.winamp.com/wiki/SDK_Contents)) in what I _really_ hope constitutes fair use.
