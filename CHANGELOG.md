## [Unreleased](https://github.com/rr-/Tomb1Main/compare/stable...develop)
- fixed lara not reverting to pistols on new level start

## [2.9.1](https://github.com/rr-/Tomb1Main/compare/2.9...2.9.1) - 2022-06-03
- fixed crash on centaur hatch (#579, regression from 2.9)

## [2.9](https://github.com/rr-/Tomb1Main/compare/2.8.2...2.9) - 2022-06-01
- added generic SDL-based controller support (#278)
- added the ability to make freshly triggered (runaway) Pierre replace an already existing (runaway) Pierre (#532)
- added a fade out when completing Lara's Home (#383)
- added the config option to change the number of save slots (#170)
- changed default save slot count to 25 (#170)
- fixed Tihocan chain block sound (#433)
- fixed passport menu with high UI scaling (#546, regression from 2.7)
- fixed passport menu border being off by one pixel (#547)
- fixed the new game and save game passport options using the wrong closing animation (#542, regression from 2.7)
- fixed bridges at floor level appearing under the floor (#523)
- fixed Lara's outfit in Lara's Home when replaying the level (#571, regression from 2.7)
- fixed crash when dying in the gym level with no saves (#576, regression from 2.8)
- fixed exiting select level menu causing deaths in a new game incremented in that slot (#575, regression from 2.8)
- removed DInput-based XBox controller support

## [2.8.2](https://github.com/rr-/Tomb1Main/compare/2.8.1...2.8.2) - 2022-05-20
- fixed Lara not picking up items near the edges of room portals (#563, regression from 2.8)

## [2.8.1](https://github.com/rr-/Tomb1Main/compare/2.8...2.8.1) - 2022-05-05
- fixed Pierre not resetting across levels (#538, regression from 2.7)
- fixed pushables breaking with flipped rooms when loading a save (#536, regression from 2.8)

## [2.8](https://github.com/rr-/Tomb1Main/compare/2.7...2.8) - 2022-05-04
- added the option to pause sound in the inventory screen (#309)
- added level selection to the load game menu (#197)
- added the ability to pick up multiple items at once with walk to items enabled (#505)
- added the ability to skip pictures during fade animation (#510)
- added a cheat to increase the game speed (#135)
- added a matrix stack overflow error check and message if GetRoomBounds runs infinitely (#506)
- added ability to turn off trex collision (#437)
- changed the savegame dialog to remember the user's requested slot number (#514)
- changed the new game dialog to always fall back to new game
- fixed ghost margins during fade animation on HiDPI screens (#438)
- fixed music rolling over to the main menu if main menu music disabled (#490)
- fixed Unfinished Business gameflow not using basic / detailed stats strings (#497, regression from 2.7)
- fixed picking up multiple underwater pickups with walk to items enabled (#500)
- fixed incorrect Lara health when restarting a level
- fixed pushables breaking with flipped rooms when loading a save (#496, regression from 2.6)
- fixed pictures displayed before starting a level causing a black screen (custom levels only)
- fixed underwater caustics animating at 2x speed (#109)
- fixed new game plus infinite ammo carrying over to a loaded game (#535, regression from 2.6)

## [2.7](https://github.com/rr-/Tomb1Main/compare/2.6.4...2.7) - 2022-03-16
- added ability to automatically walk to pickups when nearby (#18)
- added ability to automatically walk to switches when nearby (#222)
- added ability to turn off detailed end of the level stats (#447)
- added contextual arrows to passport navigation (#420)
- added contextual arrows to sound option navigation (#459)
- added contextual arrows to controls option navigation (#461)
- added contextual arrows to graphics option navigation (#462)
- added a final statistics screen (#385)
- added music during the credits (#356)
- added fade effects to displayed images (#476)
- added unobtainable pickups and kills stats support in the gameflow (#470)
- fixed exploded mutant pods sometimes appearing unhatched on reload (#423)
- fixed sound effects playing rapidly in sound menu if input held down (#467)

## [2.6.4](https://github.com/rr-/Tomb1Main/compare/2.6.3...2.6.4) - 2022-02-20
- fixed crash when loading a legacy save and saving on a new slot (#442, regression from 2.6)

## [2.6.3](https://github.com/rr-/Tomb1Main/compare/2.6.2...2.6.3) - 2022-02-18
- fixed croc and rats breaking saves after a flipmap (#441, regression from 2.6)

## [2.6.2](https://github.com/rr-/Tomb1Main/compare/2.6.1...2.6.2) - 2022-02-17
- fixed equipping gun after starting a demo (#440, regression from 2.6)

## [2.6.1](https://github.com/rr-/Tomb1Main/compare/2.6...2.6.1) - 2022-02-16
- fixed equipping gun after starting the game (#439, regression from 2.6)

## [2.6](https://github.com/rr-/Tomb1Main/compare/2.5...2.6) - 2022-02-16
- added deaths counter (#388, requires new saves)
- added total pickups and kills per level to the compass and end level stats screens (#362)
- added new, more resilient savegame format (#277)
- added ability to give Lara various items in the gameflow file
- added restart level to passport menu on death (#48)
- changed Lara's starting health to be configurable; useful for no damage runs (#365)
- changed saves to be put in the saves/ directory (#87)
- changed fade animations to block the main menu inventory ring like in PS1 (#379)
- changed fade animations to be FPS-independent
- changed fade animations to run faster in the main menu
- changed compass text order to be consistent with level stats (#415)
- fixed detail levels text flashing with any option change (#380)
- fixed main menu demo playing even when the passport is open (#410, regression from 2.1)
- fixed broken poses at the end of cinematics (#390)
- fixed libavcodec-related memory leaks (#389)
- fixed crash in custom levels that call `level_stats` after playing an FMV (#393, regression from 2.5)
- fixed calling `level_stats` for different levels (#336, requires new saves)
- fixed sounds playing after demo mode ends when game is minimized (#399)
- fixed glitched floor in the Natla cutscene (#405)
- fixed gun pickups disappearing in rare circumstances on save load (#406)
- fixed equipping gun after loading a legacy save (#427, regression from 2.4)
- fixed empty mutant shells in Unfinished Business spawning Lara's hips (#250)
- fixed rare audio distance glitch (#421)
- fixed Lara not getting her pistols in Atlantis if the player finishes Natla's Mines without picking up any gun (#424)
- fixed broken dart ricochet effect (#429)

## [2.5](https://github.com/rr-/Tomb1Main/compare/2.4...2.5) - 2022-01-31
- added CHANGELOG.md
- added ability to skip cinematics with the Action key
- added fade animations (#363)
- added a vsync option (#364)
- fixed certain inputs skipping too many things (#359)
- fixed a memory leak in the audio sampler (#369)


## [2.4](https://github.com/rr-/Tomb1Main/compare/2.3...2.4) - 2022-01-19
- added ability to skip FMVs with the action key (#334)
- changed shaders to use GLSL version 1.30 (#327)
- changed savegames to consume less space
- fixed ingame overlay (bars and ammo) being sometimes shown in the menus
- fixed menu backgrounds not being shown on certain platforms (#324)
- fixed Lara reverting back to pistols when finishing a level with another gun (#338)
- fixed lava wedge not setting Lara on fire (#353, regression from 2.2)
- fixed fallback game strings not working (#335, regression from 2.3)
- fixed high DPI window scaling on Windows (#280)
- fixed not all sounds being muted when minimizing the game (#349)
- fixed ability to push movable blocks through doors (#46)
- fixed showing inventory ring up/down arrows when uncalled for (#337)
- fixed Tomb1Main.log to be placed in the game directory rather than the current working directory
- fixed a crash when exiting the game (regression from 2.3)
- fixed a crash when shader compilation fails


## [2.3](https://github.com/rr-/Tomb1Main/compare/2.2.1...2.3) - 2022-01-12
- added ability to hold down forward/back to move through saves faster (#171)
- changed screenshots to be saved in its own folder and with more meaningful names (#255)
- fixed audible clicks near the end of samples (#281)
- fixed secret chime not playing if the secret sound fix is disabled, and nothing plays between consecutive secret pickups (#310)
- fixed ambient noises not pausing on pause screen (#316)
- fixed underwater sound effect playing only once (#305)
- fixed UZI sound stopping near big mutant explosions
- fixed switching inventory rings briefly displaying black frames (#75)
- fixed top offscreen load game selection (#273, #304)
- fixed Lara voiding through static objects (#299)
- fixed step left controller input not working (#302, regression from 2.0)
- fixed memory leaks


## [2.2.1](https://github.com/rr-/Tomb1Main/compare/2.2...2.2.1) - 2022-01-05
- fixed listing available resolutions (a regression from 2.2)
- fixed Lara's airbar showing up when Lara's dead (a regression from 2.1)


## [2.2](https://github.com/rr-/Tomb1Main/compare/2.1...2.2) - 2022-01-05
- added ability to control anisotropy filter strength
- changed the engine look for HD FMVs by default for Unfinished Business
- removed tiny screen resolutions (might require setting the resolution again)
- fixed Lara getting set on fire on trapdoors over lava
- fixed letterbox in main menu showing garbage data on certain machines
- fixed save crystals saving before gym level
- fixed black lines appearing on walls and floors
- fixed hang bug for stacked rooms


## [2.1](https://github.com/rr-/Tomb1Main/compare/...2.1) - 2021-12-21
- added ability to disable healthbar and airbar flashing
- changed the engine look for HD FMVs by default
- increased max active samples to 20 (should fix rare mute sounds issues)
- fixed loading TombATI Atlantis saves
- fixed shotgun shooting when target out of sight
- fixed save selection being offscreen if the first savegame starts with high enough number
- fixed alligators dealing no damage under certain circumstances
- fixed grabbing bridges under certain circumstances
- fixed crash if user presses a key during ring close animation

## [2.0.1](https://github.com/rr-/Tomb1Main/compare/2.0...2.0.1) - 2021-12-13
Added an icon to the .exe (thanks TRFan94!)


## [2.0](https://github.com/rr-/Tomb1Main/compare/1.4.0...2.0) - 2021-12-07
Shipped our own .exe! Tomb1Main is now fully open source and no longer needs injecting itself to the game. It also no longer depends on any of the TombATI .dll files. You can have both versions installed in the same folder.
- added support for HD FMVs
- added support for .png and .jpg pictures
- added support for .png and .jpg screenshots
- added fanmade 16:9 menu backgrounds
- added wine support
- added ability to run the game from any directory (its CWD no longer needs to point to the game's directory)
- changed music player to SDL
- changed sample player to SDL
- changed FMV player to libavcodec and SDL
- changed Eidos logo and initial FMVs to be stored in the gameflow file
- changed Unfinished Business to no longer play cafe.rpl
- changed the game no longer switches resolution back and forth in windowed mode
- changed T1M no longer reads atiset.dat
- improved shaders readability (chroma key is now stored in the texture alpha channel)
- improved shader performance a bit when the bilinear filter is off
- improved 3D rendering performance a bit (no more C++ exception handling)
- fixed brightness not being saved
- fixed game exiting with "Fatal DirectInput error" when losing focus early


## [1.4.0](https://github.com/rr-/Tomb1Main/compare/1.3.0...1.4.0) - 2021-11-16
- added adjustable ingame brightness
- added per-level fog settings
- added control over fog density (in terms of tiles)
- improved TR3 sidesteps
- improved wording in readme
- fixed lighting for 3D pickups
- fixed a crash when drawing lightnings
- fixed a crash when compiling the game on MSVC


## [1.3.0](https://github.com/rr-/Tomb1Main/compare/1.2.2...1.3.0) - 2021-11-06
- added version in the bottom right corner
- added movable camera on W,A,S,D
- added Xbox One Controller support
- added rounded shadows (instead of the default octagon)
- added per-level customizable water color (with customizable blue component)
- added rendering of pickups on the ground as 3D meshes
- added the ability to change resolution in-game
- added optional fixes for the following original game glitches:
  - slope/wall bug ("bonk to ascend" bug)
  - breakable tiles bug ("sidestep to descend" bug)
  - qwop
- changed maximum textures from 2048 to 8192
- changed maximum texture pages from 32 to 128
- changed default level skip cheat key from X to L
- removed hard limit of 1024 rooms
- fixed level skip working in inventory (it would apply only after closing the inventory)
- fixed bats being positioned too high
- fixed flashing conflicts when cheat buttons are disabled
- fixed ability to rebind the pause button


## [1.2.2](https://github.com/rr-/Tomb1Main/compare/1.2.1...1.2.2) - 2021-10-17
- added ability to mute music in main menu
- added pausing the music while in pause
- added more screen resolutions
- fixed demos playing oddly when the enhanced look option is enabled
- fixed shadows rendering
- fixed too big healthbar margins on low resolutions
- fixed bilinear filter not working
- fixed resolution width/height being ignored


## [1.2.1](https://github.com/rr-/Tomb1Main/compare/1.2.0...1.2.1) - 2021-10-17
- added resolution_width and resolution_height to the default settings
- fixed screen resolution regression from 1.2.0


## [1.2.0](https://github.com/rr-/Tomb1Main/compare/1.1.5...1.2.0) - 2021-10-15
- fixed a common crash on many machines


## [1.1.5](https://github.com/rr-/Tomb1Main/compare/1.1.4...1.1.5) - 2021-10-13
- fixed a regression resulting in crashes from 1.1.4


## [1.1.4](https://github.com/rr-/Tomb1Main/compare/1.1.3...1.1.4) - 2021-10-13
- fixed problem with the alt key on certain machines
- fixed a rare crash on certain machines


## [1.1.3](https://github.com/rr-/Tomb1Main/compare/1.1.2...1.1.3) - 2021-03-30
- changed smooth bars to be enabled by default
- changed end of level freeze fix can no longer be disabled
- changed creature distance fix can no longer be disabled
- changed pistols + key triggers fix can no longer be disabled
- changed illegal gun equip fix can no longer be disabled
- changed FMV escape key fix can no longer be disabled
- changed input to DirectInput
- fixed switchin Control keys when shimmying causing Lara to drop
- fixed some anomalies around FPS counter within ingame menus
- fixed controls UI missing its borders


## [1.1.2](https://github.com/rr-/Tomb1Main/compare/1.1.1...1.1.2) - 2021-03-30
- fixed main menu demo mode not playing correctly (regression since 1.1.1)
- fixed game speeding up on certain machines (regression since 1.1.1)


## [1.1.1](https://github.com/rr-/Tomb1Main/compare/1.1...1.1.1) - 2021-03-29
- added deactivating game when Alt-Tabbing
- improved pink bar color
- fixed sounds volume slider not working for ingame sounds


## [1.1](https://github.com/rr-/Tomb1Main/compare/1.0...1.1) - 2021-03-28
- added an alert messagebox whenever something bad (within the code's expectations) happens
- added smooth bars (needs to be explicitly enabled in the settings)
- finished porting the input and sound routines
- fixed custom bar colors not working in certain levels
- fixed RNG not being seeded (no practical consequences on the gameplay)


## [1.0](https://github.com/rr-/Tomb1Main/compare/0.13.3...1.0) - 2021-03-21
- added pause screen
- added -gold command line switch to run Unfinished Business


## [0.13.3](https://github.com/rr-/Tomb1Main/compare/0.13.2...0.13.3) - 2021-03-21
- added crystals mode (can be enabled in the gameflow)
- improved navigation through keyboard controls UI
- fixed Unfinished Business gameflow not loading
- fixed OG conflicting controls not flashing after relaunching the game
- fixed drawing Lara's hair when she carries shotgun on her back
- fixed loading custom layouts that conflict with default controls


## [0.13.2](https://github.com/rr-/Tomb1Main/compare/0.13.1...0.13.2) - 2021-03-19
- fixed lighting issues (Lara being sometimes very brightly lighted)


## [0.13.1](https://github.com/rr-/Tomb1Main/compare/0.13.0...0.13.1) - 2021-03-19
- changed demo_delay constant to be stored in the gameflow file
- fixed regression in LoadSamples


## [0.13.0](https://github.com/rr-/Tomb1Main/compare/0.12.7...0.13.0) - 2021-03-19
- added display_time parameter to display_picture (requires overwriting your gameflow file)
- added user controllable UI and bar scaling
- changed limit of max items (moveables in TRLE lingo) from 256 to 10240
- fixed whacky navigation in controls dialog if cheats are enabled
- fixed regression in LoadItems that crashes Atlantis
- fixed skipping pictures displayed before starting the level with the escape key causing inventory to open


## [0.12.7](https://github.com/rr-/Tomb1Main/compare/0.12.6...0.12.7) - 2021-03-19
- added ability to remap cheat keys (except obscure f11 debug key)
- changed f10 level skip cheat key to 'x' (can be now changed); had to be done because the game does not let mapping to function keys
- changed lots of variables to stay in T1M memory (may cause regressions)
- changed runtime game config to be read and written to a new JSON configuration rather than atiset.cfg
- changed files directory placement to a new directory, cfg/


## [0.12.6](https://github.com/rr-/Tomb1Main/compare/0.12.5...0.12.6) - 2021-03-18
- fixed loading game in Natla's Mines causing Lara to lose her guns


## [0.12.5](https://github.com/rr-/Tomb1Main/compare/0.12.4...0.12.5) - 2021-03-17
- fixed collected secrets resetting after using compass


## [0.12.4](https://github.com/rr-/Tomb1Main/compare/0.12.3...0.12.4) - 2021-03-17
- added showing level stats in compass (can be disabled)
- added ability to disable game mode selection in gameflow
- added fallback gameflow strings (in case someone installs new T1M but forgets to't override the gameflow file)
- added ability to exit level stats with escape
- changed ingame timer to tick also in the inventory (can be disabled)
- changed bar sizes and location to match TR2Main
- fixed reading key configuration for keys that override defaults
- fixed calculating creature distances (fixes Tihocan croc bug)


## [0.12.3](https://github.com/rr-/Tomb1Main/compare/0.12.2...0.12.3) - 2021-03-17
- add Japanese mode (enemies are 2 times weaker)
- improve skipping cutscenes
- fix crash when FMVs are missing (this doesn't add support for HQ FMVs though)


## [0.12.2](https://github.com/rr-/Tomb1Main/compare/0.12.1...0.12.2) - 2021-03-14
- changed settings to save after each change
- fixed OG music stopping when playing the secrets chime (can be disabled)
- fixed OG game not saving key layout choice (default vs. user keys)
- fixed OG volume slider not working when starting muted
- fixed OG holding action to skip credit pictures skipping them all at once
- fixed OG holding escape to skip FMVs opening inventory


## [0.12.1](https://github.com/rr-/Tomb1Main/compare/0.12.0...0.12.1) - 2021-03-14
- huge internal refactors
- improved door open cheat
- changed 4k scaling path to be always enabled (previously known as enable_enhanced_ui)
- fixed killing music underwater
- fixed main menu background for UB


## [0.12.0](https://github.com/rr-/Tomb1Main/compare/0.11.1...0.12.0) - 2021-03-12
- introduced gameflow sequencer (moves FMVs, cutscenes, level stats etc. logic to the gameflow JSON file); add ability to control number of levels
- refactored gameflow
- added ability to disable cinematic scenes
- changed automatic calculation of secret count to be always enabled
- fixed starting NG+ from gym not working
- fixed cinematics resetting FOV


## [0.11.1](https://github.com/rr-/Tomb1Main/compare/0.11...0.11.1) - 2021-03-11
- added ability to turn off main menu demos
- added ability to turn off FMVs
- added reporting JSON parsing errors in the logs
- fixed reading config sometimes not working
- fixed killing music in the inventory
- fixed missing Demo Mode text
- fixed showing Eidos logo for too short
- fixed Lara wearing normal clothes in Gym


## [0.11](https://github.com/rr-/Tomb1Main/compare/0.10.5...0.11) - 2021-03-11
- introduced gameflow file (moves all game strings to a gameflow JSON file, including level paths and names); level number, FMVs etc. are still hardcoded


## [0.10.5](https://github.com/rr-/Tomb1Main/compare/0.10.4...0.10.5) - 2021-03-10
- added arrows to save/load dialogs
- improved user keys settings dialog - you don't have to hold the key for exactly 1 frame anymore
- made new game dialog smaller
- fixed passport closing when exiting new game mode selection dialog


## [0.10.4](https://github.com/rr-/Tomb1Main/compare/0.10.3...0.10.4) - 2021-03-08
- fixed load game screen


## [0.10.3](https://github.com/rr-/Tomb1Main/compare/0.10.2...0.10.3) - 2021-03-08
- added NG/NG+ mode selection


## [0.10.2](https://github.com/rr-/Tomb1Main/compare/0.10.1...0.10.2) - 2021-03-07
- fixed fly cheat resurrection with lava wedges


## [0.10.1](https://github.com/rr-/Tomb1Main/compare/0.10...0.10.1) - 2021-03-07
- improved dealing with missing config
- renamed config to .json5
- fixed sound going off after playing a cinematic


## [0.10](https://github.com/rr-/Tomb1Main/compare/0.9.2...0.10) - 2021-03-06
- added support for opening closest doors


## [0.9.2](https://github.com/rr-/Tomb1Main/compare/0.9.1...0.9.2) - 2021-03-05
- fixed messged up FMV sequence IDs
- fixed crash when drawing lightnings near Scion


## [0.9.1](https://github.com/rr-/Tomb1Main/compare/0.9...0.9.1) - 2021-03-04
- fixed bats flying near floor
- fixed typo in Tomb1Main.json causing everything to be disabled


## [0.9](https://github.com/rr-/Tomb1Main/compare/0.8.3...0.9) - 2021-03-03
- added FOV support (overrides GLrage completely, but should be compatible with it)
- added support for more than 3 pickups at once (for TRLE builders)
- fixed smaller pickup sprites
- fixed showing FPS in the main menu doing weird stuff to the inventory text after starting the game


## [0.8.3](https://github.com/rr-/Tomb1Main/compare/0.8.2...0.8.3) - 2021-02-28
- improved TR3-like sidesteps
- improved bar flashing modes
- fixed Lara targeting enemies even after death
- fixed version information missing from releases


## [0.8.2](https://github.com/rr-/Tomb1Main/compare/0.8.1...0.8.2) - 2021-02-28
- fixed Lara drawing guns when loading OG saves


## [0.8.1](https://github.com/rr-/Tomb1Main/compare/0.8...0.8.1) - 2021-02-27
- fixed AI sometimes having problems to find Lara
- fixed shotgun firing sound after running out of ammo
- fixed OG being able to get pistols by running out of ammo in other weapons, even without having them in the inventory


## [0.8](https://github.com/rr-/Tomb1Main/compare/0.7.6...0.8) - 2021-02-27
- added optional TR3-like sidesteps
- added "never" to healthbar display modes (so that you can run without ever knowing your health!)
- added airbar display modes (so that you can swim without ever knowing your remaining oxygen)
- added experimental braid, off by default (works only in Lost Valley due to other levels having no braid meshes)
- added version information (it's in file properties)
- changed turning fly cheat on above water no longer causes Lara to create bubbles for 1 frame
- changed turning fly cheat on after stepping on Midas hand and getting eaten by T-Rex now resets Lara's appearance back to normal
- change turning fly cheat on while burning now extinguishes Lara
- increase the chance for the player to resurrect Lara with fly cheat after dying (up to 10 s, but it has to be the first keystroke they press)
- fixed T1M bug - holding fly cheat and WALK resulting in hoisting Lara up
- fixed T1M bug - added ability to draw last selected weapon with numkeys
- fixed OG bug - keys and puzzles not triggering after drawing guns
- fixed OG bug - having to draw guns via inventory after picking them up in Natla's Mines
- fixed OG crash when Lara is on fire and walks too far away from where she caught fire


## [0.7.6](https://github.com/rr-/Tomb1Main/compare/0.7.5...0.7.6) - 2021-02-23
- fixed Atlanteans behavior


## [0.7.5](https://github.com/rr-/Tomb1Main/compare/0.7.4...0.7.5) - 2021-02-22
- fixed ammo text placement
- fixed healthbar placement in the inventory


## [0.7.4](https://github.com/rr-/Tomb1Main/compare/0.7.3...0.7.4) - 2021-02-22
- added support for user-configured bar colors
- switched configuration format to use JSON5
- moved comments to Tomb5Main.json
- fixed bar placement


## [0.7.3](https://github.com/rr-/Tomb1Main/compare/0.7.2...0.7.3) - 2021-02-22
- added support for user-configured bar locations
- fixed pickups scaling


## [0.7.2](https://github.com/rr-/Tomb1Main/compare/0.7.1...0.7.2) - 2021-02-22
- fixed ability to look around while Lara's dead
- fixed UI scaling in controls dialog
- fixed crash for some creatures


## [0.7.1](https://github.com/rr-/Tomb1Main/compare/0.7...0.7.1) - 2021-02-22
- added inventory cheat
- made fly cheat faster


## [0.7](https://github.com/rr-/Tomb1Main/compare/0.6...0.7) - 2021-02-21
- added fly cheat
- fixed a crash when hit by a lightning (T1M regression)
- fixed missing "Demo Mode" text (T1M regression)


## [0.6](https://github.com/rr-/Tomb1Main/compare/0.5.1...0.6) - 2021-02-20
- changed the code to count secrets automatically (useful for custom level builders)
- fixed secret trigger in The Great Pyramid
- fixed a crash when loading levels with more than 1024 textures
- fixed drawing Lara (T1M regression)


## [0.5.1](https://github.com/rr-/Tomb1Main/compare/0.5...0.5.1) - 2021-02-20
- added fire sprite to shotgun


## [0.5](https://github.com/rr-/Tomb1Main/compare/0.4.1...0.5) - 2021-02-18
- renamed the project from TR1Main to Tomb1Main on the request of Arsunt
- improved documentation


## [0.4.1](https://github.com/rr-/Tomb1Main/compare/...0.4.1) - 2021-02-15
- added an option to always show the healthbar
- fixed enemy healthbars in NG+
- fixed no heal mode

## [0.4](https://github.com/rr-/Tomb1Main/compare/0.3.1...0.4) - 2021-02-14
- added UI scaling
- added ability to look around underwater


## [0.3.1](https://github.com/rr-/Tomb1Main/compare/...0.3.1) - 2021-02-13
- improved the ability to look around while running

## [0.3](https://github.com/rr-/Tomb1Main/compare/0.2.1...0.3) - 2021-02-13
- added an option disable magnums
- added an option disable uzis
- added an option disable shotgun
- added ability to look around while running
- added support for using items with numeric keys
- fixed an OG bug with the secret sound in Tomb of Tihocan


## [0.2.1](https://github.com/rr-/Tomb1Main/compare/0.2...0.2.1) - 2021-02-11
- changed the default configuration to enable enemy healthbars, red healthbar and end of the level freeze fix


## [0.2](https://github.com/rr-/Tomb1Main/compare/0.1...0.2) - 2021-02-11
- added enemy healthbars
- added a red healthbar


## [0.1](https://github.com/rr-/Tomb1Main/compare/...0.1) - 2021-02-10

Initial version.
