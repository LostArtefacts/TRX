<p align="center">
<img alt="TR2X logo" src="/data/tr2/logo.png" width="400"/>
</p>

TR2X is finished with the decompilation and is now able to run without the
original game .exe. The focus is now to clean up the code base and enrich the
game with new enhancements and features.

## Windows / Linux

### Installing (simplified)

1. Head over to GitHub releases: https://github.com/LostArtefacts/TRX/releases
2. Download the TR2X installer. Your browser may complain that the .exe is unsafe, but it's OK to ignore this alert.
3. Mark the installer EXE as safe to run by right-clicking on the .exe, going to properties and clicking "Unblock".
4. Run the installer and proceed with the steps.

### Installing (manual)

1. Head over to GitHub releases: https://github.com/LostArtefacts/TRX/releases
2. Download the TR2X zip file.
3. Extract the TR2X zip file into a directory of your choice.  
   Make sure you choose to overwrite existing directories and files.
4. (First time installation) Put your original game files into the target directory.
5. Optionally you can also install the Golden Mask expansion pack files. Extract the contents of the following zip
   into the target directory.
   https://lostartefacts.dev/aux/tr2x/trgm.zip
5. To play the game, run `TR2X.exe`.
6. To play the Golden Mask expansion pack, run `TR2X.exe -gold`.

If you install everything correctly, your game directory should look more or
less like this (click to expand):

<details>
<p><em>* Will not be present until the game has been launched.</em></p>
<pre>
.
├── cfg
│   ├── TR2X.json5 *
│   ├── TR2X_gameflow.json5
│   ├── TR2X_gameflow_gm.json5
│   ├── TR2X_gameflow_level.json5
│   ├── TR2X_strings.json5
│   ├── TR2X_strings_gm.json5
│   └── TR2X_strings_level.json5
├── data
│   ├── assault.tr2
│   ├── boat.tr2
│   ├── catacomb.tr2
│   ├── cut1.tr2
│   ├── cut2.tr2
│   ├── cut3.tr2
│   ├── cut4.tr2
│   ├── deck.tr2
│   ├── emprtomb.tr2
│   ├── floating.tr2
│   ├── house.tr2
│   ├── icecave.tr2
│   ├── keel.tr2
│   ├── level1.tr2
│   ├── level2.tr2
│   ├── level3.tr2
│   ├── level4.tr2
│   ├── level5.tr2
│   ├── living.tr2
│   ├── main.sfx
│   ├── main_gm.sfx
│   ├── monastry.tr2
│   ├── opera.tr2
│   ├── platform.tr2
│   ├── rig.tr2
│   ├── skidoo.tr2
│   ├── title.tr2
│   ├── title_gm.tr2
│   ├── unwater.tr2
│   ├── venice.tr2
│   ├── wall.tr2
│   ├── xian.tr2
│   ├── images
│   │   ├── credit00_gm.png
│   │   ├── credit01.png
│   │   ├── credit02.png
│   │   ├── credit03.png
│   │   ├── credit04.png
│   │   ├── credit05.png
│   │   ├── credit06.png
│   │   ├── credit07.png
│   │   ├── credit07_gm.png
│   │   ├── credit08.png
│   │   ├── end.png
│   │   ├── legal.png
│   │   ├── title_eu.png
│   │   ├── title_eu_gm.png
│   │   ├── title_us.png
│   │   ├── title_us_gm.png
│   │   └── og
│   │       ├── credit00_gm.pcx
│   │       ├── credit01.pcx
│   │       ├── credit02.pcx
│   │       ├── credit03.pcx
│   │       ├── credit04.pcx
│   │       ├── credit05.pcx
│   │       ├── credit06.pcx
│   │       ├── credit07.pcx
│   │       ├── credit07_gm.pcx
│   │       ├── credit08.pcx
│   │       ├── credit09.pcx
│   │       ├── end.pcx
│   │       ├── legal.pcx
│   │       ├── title.pcx
│   │       ├── title_eu_gm.pcx
│   │       └── title_us_gm.pcx
│   └── injections
│       ├── barkhang_itemrots.bin
│       ├── barkhang_pickup_meshes.bin
│       ├── catacombs_fd.bin
│       └── etc...
├── fmv
│   ├── ancient.rpl
│   ├── crash.rpl
│   ├── end.rpl
│   ├── jeep.rpl
│   ├── landing.rpl
│   ├── logo.rpl
│   ├── modern.rpl
│   └── ms.rpl
├── music
│   ├── 2.mp3
│   ├── 3.mp3
│   └── etc...
├── shaders
│   ├── 2d.glsl
│   ├── 3d.glsl
│   ├── common.glsl
│   ├── fade.glsl
│   └── fbo.glsl
├── TR2X.exe
</pre>
</details>

## macOS

### Installing

1. Head over to GitHub releases: https://github.com/LostArtefacts/TRX/releases
2. Download the `TR2X-Installer.dmg` installer image. Mount the image and drag TR2X to the Applications folder.
3. Run TR2X from the Applications folder. This will show you an error dialog about missing game data files. This is expected at this point, as you have not copied them in yet. However, it's important to run the app first to allow macOS to verify the app bundle's signature.
4. Find TR2X in your Applications folder. Right-click it and click "Show Package Contents".
5. Copy your Tomb Raider 2 game data files into `Contents/Resources`.

## Dev snapshots

To ease the load on our infrastructure, the binary assets such as images and music files are not included in pre-releases and pull request preview builds - they only ship with the full release builds.
However, you can easily download them manually from these urls:

- https://lostartefacts.dev/aux/tr2x/main.zip (main assets)
- https://lostartefacts.dev/aux/tr2x/music.zip (music files)
- https://lostartefacts.dev/aux/tr2x/trgm.zip (TR: The Golden Mask expansion pack)
