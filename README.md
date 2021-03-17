# InSource

Legacy version of InSource, a Source Engine (Alien Swarm branch) modification to fix several problems and implement new features.

> ⚠️ Although the code works, it has several optimization problems. Not suitable for production use.

## Most notable features

> ⚠️ Some of these may be incomplete or unstable.

- **Bots:** 
AI system to create Bots (CPU controlled players) in a similar way to the AI for NPC's.

*If you are only interested in this system I recommend you take a look at this [repository](https://github.com/kolessios/sourcebots).*
- **Director:** 
AI system to create NPC's or Bots on the map depending on various factors, also includes map entities to query their status or trigger events. 
(Inspired by the [Director](https://youtu.be/WbHMxo11HcU) of Left 4 Dead)
- **Channel Sound System:** 
System to easily control sounds programmatically in C++ and Squirrel. 
- **Node Generator:** 
System to create navigation nodes (info_node) based on the Navigation mesh. Generation and settings are controlled by commands.
- **Support for AlienFX:** 
System to dynamically control the lights of a computer with [AlienFX](https://youtu.be/N4cr_jH_yus). (From client and server)

Examples
--------------

Check this [playlist](https://www.youtube.com/playlist?list=PLOUVJcNedgYFn3BOrz6aRPYpKkpLzQcbV) to find some examples.

Dependencies
--------------

- **[CEF](https://bitbucket.org/chromiumembedded/cef)** *(Optional)*: You need to download a copy of the source code of CEF and extract it in */public/cef/*. Compile CEF and copy the resulting files *"libcef.lib"* and *"libcef_dll_wrapper.lib"* to */lib/common/*. Now copy the remaining files from the compilation *(.dll and .bin files)* as well as the files from the */Resources/* folder into the */bin/* folder of your game files.
- **[SQLITE](http://sqlite.org/download.html)** *(Optional)*: If you want to update the SQLite version of the project you need to download the file *"sqlite3.lib"* and place it in the */lib/common/* folder as well as the *"sqlite3.dll"* file and place it in the */bin/* folder of your game.

More information
-----------------------

To get more information about the project, tips, help or want to help me you can contact me by [email](mailto:kolessios@gmail.com) or [Steam](http://steamcommunity.com/profiles/76561198040059089).
