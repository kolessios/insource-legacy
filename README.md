# InSource

Legacy version of InSource, a Source Engine (Alien Swarm branch) modification to fix several problems and implement new features.

> ⚠️ Although the code works, this project was the first C++ project of the developer and it has several optimization problems. **Not suitable for production use.**

> 🌎 Most of the comments and debug messages are in Spanish.

## Most notable features

> ⚠️ Some of these may be incomplete or unstable.

- [**Bots:**](https://github.com/kolessios/insource-legacy/tree/master/game/server/in/bots) 
AI system to create Bots (CPU controlled players) in a similar way to the NPC's AI.
*💡 If you are only interested in this system take a look at this [repository](https://github.com/kolessios/sourcebots).*
- [**Director:**](https://github.com/kolessios/insource-legacy/blob/master/game/server/in/director.cpp) 
AI system to create NPC's or Bots on the map depending on various factors, also includes [map entities](https://github.com/kolessios/insource-legacy/blob/master/game/server/in/info_director.cpp) to trigger events. 
(Inspired by the [Director](https://youtu.be/WbHMxo11HcU) of Left 4 Dead)
- [**Sound System:**](https://github.com/kolessios/insource-legacy/blob/master/game/shared/in/sound_instance.cpp) 
System to easily control sounds programmatically in C++ and Squirrel. 
- [**Node Generator:**](https://github.com/kolessios/insource-legacy/blob/master/game/server/in/nodes_generation.cpp) 
System to create navigation nodes (info_node) based on the Navigation mesh. Generation and settings are controlled by commands.
- [**Support for AlienFX:**](https://github.com/kolessios/insource-legacy/blob/master/game/client/in/alienfx.cpp) 
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

To get more information about the project you can contact me via [email](mailto:kolessios@gmail.com) or [Steam](http://steamcommunity.com/profiles/76561198040059089).
