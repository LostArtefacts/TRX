<p align="center">
<img alt="TR2X logo" src="/data/tr2/logo-light-theme.png#gh-light-mode-only" width="400"/>
<img alt="TR2X logo" src="/data/tr2/logo-dark-theme.png#gh-dark-mode-only" width="400"/>
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
└── TR2X_ConfigTool.exe
</pre>
</details>

### Configuring

To configure TR2X, run the `TR2X_ConfigTool.exe` application. All the
configuration is explained in this tool. Alternatively, after running the game
at least once, you can edit `TR2X.json5` manually in a text editor such
as Notepad.

## macOS

### Installing

1. Head over to GitHub releases: https://github.com/LostArtefacts/TRX/releases
2. Download the `TR2X-Installer.dmg` installer image. Mount the image and drag TR2X to the Applications folder.
3. Run TR2X from the Applications folder. This will show you an error dialog about missing game data files. This is expected at this point, as you have not copied them in yet. However, it's important to run the app first to allow macOS to verify the app bundle's signature.
4. Find TR2X in your Applications folder. Right-click it and click "Show Package Contents".
5. Copy your Tomb Raider 2 game data files into `Contents/Resources`.

## Improvements over original game

#### UI
- added support for more accented characters
- added fade effects to displayed images
- added a wireframe mode
- added sunglasses for graphic options
- improved support for windowed mode

#### Gameplay
- added an option to fix M16 accuracy while running
- added optional rendering of pickups in the UI as 3D meshes
- added optional automatic key/puzzle inventory item pre-selection
- added optional fixes for the following gameplay glitches:
  - QWOP animation
  - step bug
  - free flare from underwater pickup
  - drifting into walls during underwater pickups
- added support for 60 FPS rendering
- added a pause screen
- added a photo mode feature
- added combined support for The Golden Mask
- added NG+, Japanese, and Japanese NG+ game mode options to the New Game page in the passport
- changed inventory to pause the music rather than muting it
- fixed killing the T-Rex with a grenade launcher crashing the game
- fixed secret rewards not displaying shotgun ammo
- fixed numeric keys interfering with the demos
- fixed the ammo counter being hidden while a demo plays in NG+
- fixed explosions sometimes being drawn too dark
- fixed controls dialog remapping being too sensitive
- fixed Lara reloading the harpoon gun after every shot in NG+
- fixed the dragon reviving itself after Lara removes the dagger in rare circumstances
- fixed grenades counting as double kills in the game statistics
- fixed the game hanging if exited during the level stats, credits, or final stats
- fixed a crash when firing grenades at Xian guards in statue form
- fixed harpoon bolts damaging inactive enemies
- fixed the distorted skybox in room 5 of Barkhang Monastery
- fixed new saves not displaying the save count in the passport
- fixed Lara getting stuck in her hit animation if she is hit while mounting the boat or skidoo
- fixed the detonator key and gong hammer not activating their target items when manually selected from the inventory
- fixed a potential crash if Lara is on the skidoo in a room with many other adjoining rooms
- fixed a softlock in Home Sweet Home if the final cutscene is triggered while Lara is on water surface
- fixed Lara's left arm becoming stuck if a flare is drawn just before the final cutscene in Home Sweet Home
- fixed game freezing when starting demo/credits/inventory offscreen
- fixed exiting the game with Alt+F4 not immediately working in cutscenes
- fixed a crash when trying to draw too many rooms at once
- fixed Lara prioritising throwing a spent flare while mid-air, so to avoid missing ledge grabs
- fixed Lara at times not being able to jump immediately after going from her walking to running animation
- fixed looking forward too far causing an upside down camera frame
- fixed several instances of the camera going out of bounds
- fixed Lara never stepping backwards off a step using her right foot
- fixed the following floor data issues:
    - **Opera House**: fixed the trigger under item 203 to trigger it rather than item 204
    - **Wreck of the Maria Doria**: fixed room 98 not having water
    - **Tibetan Foothills**: added missing triggers for the drawbridge in room 96 (after the flipmap)
    - **Catacombs of the Talion**: changed some music triggers to pads near the first yeti, and added missing triggers and ladder in room 116 (after the flipmap)
    - **Ice Palace**: fixed door 143's position to resolve the invisible wall in front of it, and added an extra pickup trigger beside the Gong Hammer in room 29
    - **Temple of Xian**: fixed missing death tiles in room 91
    - **Floating Islands**: fixed door 72's position to resolve the invisible wall in front of it
- fixed the game crashing if a cinematic is triggered but the level contains no cinematic frames
- fixed smashed windows blocking enemy pathing after loading a save
- fixed Lara getting stuck in a T-pose after jumping/falling and then dying before reaching fast fall speed
- fixed collision issues with drawbridges, trapdoors, and bridges when stacked over each other, over slopes, and near the ground
- fixed several issues with pushblocks:
    - fixed an invisible wall above stacked pushblocks if near a ceiling portal
    - fixed floor height issues with pushblocks poised to fall in various scenarios
    - fixed being unable to stack multiple pushblocks over multiple rooms
    - fixed falling pushblocks using the enemy grunt sound effect
- fixed seaweed collision in Living Quarters preventing Lara from climbing out of the water in room 15
- fixed the scale and rotation of several pickup models:
    - increased auto pistol ammo size
    - increased M16 ammo size
    - increased grenade size
    - reduced Offshore Rig and Diving Area key card sizes, and fixed inventory rotation
    - reduced Wreck of the Maria Doria circuit breaker size
    - increased Wreck of the Maria Doria rest room key size
    - increased Living Quarters theatre key size
    - increased The Deck cabin key size
    - reduced Barkhang Monastery prayer wheel size
    - increased Barkhang Monastery gemstone size
    - increased Barkhang Monastery rooftops key size
    - increased Temple of Xian dragon seal size, and fixed inventory rotation
    - fixed Floating Islands mystic plaque inventory rotation
- improved the animation of Lara's braid

#### Cheats
- added a fly cheat
- added a level skip cheat
- added a door open cheat (while in fly mode)
- added a cheat to increase the game speed
- added a cheat to explode Lara like in TR2 and TR3

#### Input
- added ability to sidestep like in TR3
- added ability to hold arrows to move through menus more quickly
- added additional custom control schemes
- added customizable controller support
- fixed setting user keys being very difficult
- fixed skipping FMVs triggering inventory
- fixed skipping credits working too fast

#### Statistics
- fixed the dragon counting as more than one kill if allowed to revive
- fixed enemies that are run over by the skidoo not being counted in the statistics
- fixed the final two levels not allowing for secrets to be counted in the statistics

#### Visuals
- added quadrilateral texture correction
- added ability to set user-defined FOV
- added support for HD FMVs
- added wireframe mode
- added an option for 1-2-3-4× pixel upscaling
- added ability to use the window border option at all times
- added ability to toggle between the software/hardware renderer at runtime
- added optional fade effects to the hardware renderer
- added text information when changing rendering options at runtime
- added support for animated sprites
- added the ability for custom levels to have up to two of each secret type per level
- changed the hardware renderer to always use 16-bit textures
- changed the software renderer to use the picture's palette for the background pictures
- changed fullscreen behavior to use windowed desktop mode
- changed dynamic lighting for gun flashes and explosions to be optional
- fixed fullscreen issues
- fixed black borders in windowed mode
- fixed "Failed to create device" when toggling fullscreen
- fixed TGA screenshots crashing the game
- fixed the camera being cut off after using the gong hammer in Ice Palace
- fixed Lara's underwater hue being retained when re-entering a boat
- fixed distant rooms sometimes not appearing, causing the skybox to be visible when it shouldn't
- fixed rendering problems on certain Intel GPUs
- fixed bubbles spawning from flares if Lara is in shallow water
- fixed the inventory up arrow at times overlapping the health bar
- fixed blood spawning on Lara from gunshots using incorrect positioning data
- fixed the drawbridge producing dynamic light when open
- improved FMV mode behavior - stopped switching screen resolutions
- improved vertex movement when looking through water portals
- improved support for non-4:3 aspect ratios

#### Audio
- added an option to control how music is played while underwater rather than simply muting it
- fixed music not playing with certain game versions
- fixed the audio not being in sync when Lara strikes the gong in Ice Palace
- fixed sound settings resuming the music
- fixed wrong default music volume (being very loud on some setups)
- fixed flare sound effects not always playing when Lara is in shallow water
- fixed music not playing if triggered while the game is muted, but the volume is then increased
- fixed being unable to load a level that contains no sound effect data
- fixed missing enemy sound effects in the underwater levels

#### Mods
- added developer console (accessible with `/`, see [COMMANDS.md](COMMANDS.md) for details)
- added ability to disable FMVs
- added per-level customizable fog distance
- removed the hard-coded end-level behaviour of the bird guardian for custom levels

#### Miscellaneous
- added Linux builds
- added macOS builds
- added .jpeg/.png screenshots
- added BSON savegame support, removing the limits imposed by the OG 8KB file size, so allowing for storing more data and offering improved feature support
- added ability to skip FMVs with both the Action key
- added ability to skip end credits with the Action and Escape keys
- added the ability to specify per-level SFX files rather than enforcing the default (main.sfx) on all levels
- added the ability to define bonus levels in the game flow, which unlock when all main game secrets are found
- added -l/--level and -s/--save command line arguments
- expanded internal game memory limit from 7.5 MB to unlimited (within system memory cap)
- expanded maximum object textures from 2048 to unlimited (within game's overall memory cap)
- expanded maximum sprite textures from 512 to unlimited (within game's overall memory cap)
- expanded maximum texture pages from 32 to 128
- expanded the number of static mesh slots from 50 to 256
- expanded maximum number of items (moveables) from 256 to 10240 (1024 remains the limit for triggered items)
- expanded maximum number of visible enemies from 5 to 32
- expanded the maximum number of effects (flames, embers, exploding parts etc) from 100 to 1000
- ported audio decoding library to ffmpeg
- ported video decoding library to ffmpeg
- ported input backend to SDL
- ported audio backend to SDL
- ported video backend to SDL
- fixed screenshots not working in windowed mode
- fixed screenshots key not getting debounced
- changed screenshots to be put in the screenshots/ directory
- changed saves to be put in the saves/ directory
- removed `-setup` dialog
