InSource
====================

InSource is a modification of the engine *Valve Source Engine 1* (Swarm Branch), which acts as the basis for first-person shooter games, contains a large number of corrections and new functions that can be easily deactivated or modified from the code or commands.

Most notable features.
------------------

*Some of these features may be incomplete or have problems in the current version.*

- **Bots:** 
An A.I. for the creation of Bots. (Players controlled by the computer).

*If you are only interested in the Bots system and want to incorporate them in your own engine modification or in [Source SDK 2013](https://developer.valvesoftware.com/wiki/Source_SDK_2013) look at this [repository](https://github.com/WootsMX/sourcebots).*
- **Director:** 
An A.I. for creating NPCs/Bots on the map depending on multiple variables such as players status. (Inspired by the [Director](https://youtu.be/WbHMxo11HcU) of the saga Left 4 Dead)
- **Channel Sound System:** 
A system that makes it possible to play sounds easily and completely in code (C++ and [Squirrel](http://squirrel-lang.org/)), it is possible to create groups of sounds that can be reproduced by a level of desire (A function called GetSoundDesire). The sound of a channel that has the most desire will be the one that reproduces above the others.
- **Node Generation:** 
NPCs Artificial Intelligence continues to use the old navigation system (info_node), to compensate a little the work of putting these nodes on the map was developed a system that automatically places them with reference to the **Navigation Mesh**, includes the creation of hint nodes and nodes to create stairs.
- **Support for AlienFX:** 
The Game can dynamically modify the lights on a player's computer with [AlienFX](https://youtu.be/N4cr_jH_yus). (From client and server)

Alpha Version!
------------------

The current version of InSource is in early development and may contain bugs, memory leaks and incomplete features. **It is not recommended to start a project with the current version.**

Game Files
--------------

For the tests shown in the [videos](https://www.youtube.com/playlist?list=PLOUVJcNedgYFn3BOrz6aRPYpKkpLzQcbV) the mod files "Apocalypse-22" are used.

You can find the mod files [here](https://github.com/WootsMX/insource-games).

Dependencies
--------------

- **[CEF](https://bitbucket.org/chromiumembedded/cef)**: You need to download a copy of the source code of CEF and extract it in */public/cef/*. Compile CEF and copy the resulting files *"libcef.lib"* and *"libcef_dll_wrapper.lib"* to */lib/common/*. Now copy the remaining files from the compilation *(.dll and .bin files)* as well as the files from the */Resources/* folder into the */bin/* folder of your game files.
- **[SQLITE](http://sqlite.org/download.html)** *(Optional)*: If you want to update the SQLite version of the project you need to download the file *"sqlite3.lib"* and place it in the */lib/common/* folder as well as the *"sqlite3.dll"* file and place it in the */bin/* folder of your game.

More information
-----------------------

To get more information about the project, tips, help or want to help me you can contact me by [email](mailto:ivan-bravo@woots.xyz) or [Steam](http://steamcommunity.com/profiles/76561198040059089).

Apocalypse-22
-----------------------

It is the mod for which this version of the engine is being made. You can get more information about the game [here](http://www.moddb.com/mods/apocalypse-22)