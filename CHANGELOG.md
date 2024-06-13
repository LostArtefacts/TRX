## [Unreleased](https://github.com/LostArtefacts/TR1X/compare/stable...develop) - ××××-××-××
- added creating minidump files on crashes
- added new console commands:
    - `/hp`
    - `/hp [num]`
    - `/heal`
- added unobtainable secrets stat support in the gameflow (#1379)
- fixed config tool and installer missing icons (#1358, regression from 4.0)
- fixed looking forward too far causing an upside down camera frame (#1338)
- fixed the enemy bear behavior in demo mode (#1370, regression since 2.16)
- fixed the FPS counter overlapping the healthbar in demo mode (#1369)
- fixed the Scion being extremely difficult to shoot with the shotgun (#1381)

## [4.1.2](https://github.com/LostArtefacts/TR1X/compare/4.1.1...4.1.2) - 2024-04-28
- fixed pictures display time (#1349, regression from 4.1)

## [4.1.1](https://github.com/LostArtefacts/TR1X/compare/4.1...4.1.1) - 2024-04-27
- fixed reading animated texture data in levels (#1346, regression from 4.1)

## [4.1](https://github.com/LostArtefacts/TR1X/compare/4.0.3...4.1) - 2024-04-26
- added ability to show enemy healthbars only for bosses (#1300)
- added ability to kill specific enemy types (#1313)
- added ability to teleport to nearest specific object (#1312)
- added `/load` and `/save` commands for even quicker savegame operations
- added `/demo` command to quickly play the demo
- added `/title` command to quickly exit to title
- added `/vsync on` and `/vsync off` commands to toggle the VSync setting
- added `/give all` variant of the item cheat
- changed injection files to be placed in its own directory (#1306)
- changed item cheat sound effects
- changed the `/play` command to work immediately in the title screen
- fixed turbo cheat speed setting not saved across game relaunches (#1320)
- fixed turbo cheat behavior with the following game elements (#1341):
    - animated textures animation rate (regression from 4.0.3)
    - 3D pickups animation rate (regression from 4.0.3)
    - healthbar flashing rate
    - UI text flashing rate
    - inventory stats timer
    - underwater wibble effect rate
    - loading screen and credit images display time
    - title screen demo delay
    - fade times
- fixed camera vibrations when using the teleport command in 60 FPS (#1274)
- fixed the camera being thrown through doors for one frame when looked at from fixed camera positions (#954)
- fixed console not retaining changed user settings across game relaunches (#1318)
- fixed passport inventory item not being animated in 60 FPS (#1314)
- fixed object explosions not being animated in 60 FPS (#1314)
- fixed lava emitters not being animated in 60 FPS (#1314)
- fixed underwater bubbles not being animated in 60 FPS (#1314)
- fixed compass needle being too fast in 60 FPS (#1316, regression from 4.0)
- fixed black screen flickers that can occur in 60 FPS (#1295)
- fixed a slight delay with the passport menu selector (#1334)
- decreased initial flicker upon game launch (#1322)

## [4.0.3](https://github.com/LostArtefacts/TR1X/compare/4.0.2...4.0.3) - 2024-04-14
- fixed flickering sprite pickups (#1298)

## [4.0.2](https://github.com/LostArtefacts/TR1X/compare/4.0.1...4.0.2) - 2024-04-11
- fixed Mac binaries not working on x86-64 (eg not Apple Silicon)
- fixed building on Linux outside of the Docker toolchain (#1296, regression from 4.0)

## [4.0.1](https://github.com/LostArtefacts/TR1X/compare/4.0...4.0.1) - 2024-04-10
- fixed trying to pick up a lead bar crashing the game (#1293, regression from 4.0)

## [4.0](https://github.com/LostArtefacts/TR1X/compare/3.1.1...4.0) - 2024-04-09
- added experimental support for 60 FPS, available from the in-game graphics menu
- added ability to slow the game down using the turbo cheat (#1215)
- added /speed command to control the turbo cheat (#1215)
- added the option to change weapon targets by tapping the look key like in TR4+ (#1145)
- added three targeting options: full lock always keeps target lock (OG), semi lock loses target lock if the enemy dies, and no lock loses target lock if the enemy goes out of sight or dies (TR4+) (#1146)
- added an option to the installer to install from a CD drive (#1144)
- added stack traces to logs for better crash debugging (#1165)
- added an option to use PS1 loading screens (#358)
- added high quality images for the Eidos, Unfinished Business title, Unfinished Business credit, and final statistics screens
- added support for macOS builds (for both Apple Silicon and Intel)
- added optional support for OpenGL 3.3 Core Profile
- added Italian localization to the config tool
- added the ability to move the look camera while targeting an enemy in combat (#1187)
- added the ability to skip fade-out in stats screens
- added support for animated room sprites in custom levels and an option to animate plant sprites in The Cistern and Tomb of Tihocan (#449)
- added on-screen messages for certain actions (#1220)
- changed stats no longer disappear during fade-out (#1211)
- changed the way music timestamps are internally handled – resets music position in existing saves
- changed vertex and fragment shaders into unified files that are runtime pre-processed for OpenGL versions 2.1 or 3.3
- changed the `/kill` command to use Lara as a reference point, and kill all creatures that are within a single tile first (#1256)
- changed the config not to save key mappings if they do not deviate from the current version's defaults (#1218)
- changed the item cheat keybind to also work in Gym
- changed the item cheat command to display a relevant message if Lara object is not loaded
- fixed a missing translation for the Spanish config tool for the Eidos logo skip option (#1151)
- fixed a flipmap issue in Natla's Mines that could make the cabin appear stacked and prevent normal gameplay (#1052)
- fixed several texture issues across the majority of levels (#1231)
- fixed broken gorilla animations (#1244, regression from 2.15.3)
- fixed saving and loading the music timestamp when the load current music option is enabled and game sounds in inventory are disabled (#1237)
- fixed the remember played music option always being enabled (#1249, regression from 2.16)
- fixed the underwater SFX playing for one frame at the start of Palace Midas (#1251)
- fixed an incorrect frame in Lara's underwater twist animation (OG bug in TR2 onwards) (#1242)
- fixed Lara saying "no" when taking valid actions in front of a key item receptacle (#1268)
- fixed Lara not saying "no" when using the Scion incorrectly (#1278)
- fixed flickering in bats' death animations and rapid shooting if Lara continues to fire when they are killed (#992)
- fixed an incorrect animation in the door used at the beginning of Colosseum (#1287)

## [3.1.1](https://github.com/LostArtefacts/TR1X/compare/3.1...3.1.1) - 2024-01-19
- changed quick load to show empty passport instead of opening the save game menu when there are no saves (#1141)
- fixed a game crash when the quick load passport is deselected (#1136, regression from 3.1)
- fixed not being able to save in an empty slot using quick save if the load game menu was opened before (#1140, regression from 3.1)
- fixed the passport briefly flashing inaccessible page text (#1137, regression from 3.1)

## [3.1](https://github.com/LostArtefacts/TR1X/compare/3.0.5...3.1) - 2024-01-14
- added the option to use "shell(s)" to give shotgun ammo in the developer console (#1096)
- added the restart level option to the passport in save crystal mode (#1099)
- added the ability to back out of menus with the circle and triangle buttons when using a gamepad (cross acts as confirm) (#1104)
- changed `force_enable_save_crystals` to `force_save_crystals` for custom level authors to force enable or disable the save crystals setting (#1102)
- changed `force_disable_game_modes` to `force_game_modes` for custom level authors to force enable or disable the game modes setting (#1102)
- changed the Scion in The Great Pyramid from spawning blood when hit to a richochet effect if texture fixes enabled (#1121)
- changed the gamepad control menu's 'reset all buttons' bind to held R1 (was held triangle) (#1104)
- changed the number of visible enemies from 8 to 32 (#1122)
- fixed FMVs always playing at 100% volume – now they'll play at the game sound volume (#1110)
- fixed bugs when trying to stack multiple movable blocks (#1079)
- fixed Lara's meshes being swapped in the gym level when using the console to give guns (#1092)
- fixed Midas's touch having unrestricted vertical range (#1094)
- fixed flames not being drawn when Lara is on fire but leaves the room where she caught fire (#1106)
- fixed being able to deselect the passport in quick save, quick load, save crystal, and death modes (#1108)
- fixed inability to save in Unfinished Business in crystals mode as UB doesn't have crystals (#1102)
- fixed items not being added to inventory if the sprite is missing from the level file (#1130)
- fixed differences when looking at items from triggers that do not use fixed cameras when the enhanced look option is enabled (#1026)

## [3.0.5](https://github.com/LostArtefacts/TR1X/compare/3.0.4...3.0.5) - 2023-12-13
- fixed crash when pressing certain keys and the console is disabled (#1116, regression from 3.0)
- fixed lightning bolts wrongly drawn (#1113, regression from 0.9)

## [3.0.4](https://github.com/LostArtefacts/TR1X/compare/3.0.3...3.0.4) - 2023-12-08
- fixed missiles damaging Lara when she is far beyond their damage range (#1090)
- fixed pushblocks moving freely if Lara releases but tries to regrab during the release animation (#1101, regression from 3.0)

## [3.0.3](https://github.com/LostArtefacts/TR1X/compare/3.0.2...3.0.3) - 2023-11-27
- fixed underwater shadow effects rendering always in the same way rather than at random (#1081)

## [3.0.2](https://github.com/LostArtefacts/TR1X/compare/3.0.1...3.0.2) - 2023-11-11
- fixed incorrect usage reference URLs in the gameflow files (#1073)
- fixed random number generation becoming stuck after entering and leaving the inventory, which affected effects and SFX (#1070, #1074)

## [3.0.1](https://github.com/LostArtefacts/TR1X/compare/3.0...3.0.1) - 2023-11-10
- fixed installer not detecting old Tomb1Main installations (#1071)

## [3.0](https://github.com/LostArtefacts/TR1X/compare/2.16...3.0) - 2023-11-09
- renamed the project from Tomb1Main to TR1X in an effort to establish our own unique identity, while respectfully disassociating from TR2Main.
- added developer console (accessible with `/`, see [COMMANDS.md] for details)
- added Linux builds and toolchain
- added an option to allow Lara to roll while underwater, similar to TR2+ (#993)
- added an option to turn off Eidos logo entirely through config (#1044)
- added the bonus level type for custom levels that unlocks if all main game secrets are found (#645)
- added detection for animation commands to play SFX on land, water or both (#999)
- added support for customizable enemy item drops via the gameflow (#967)
- added an option to enable F-key and inventory frame buffering (#591)
- added a pickup overlay for the Midas gold bar when it changes from lead (#1010)
- added an option to allow Lara to creep forwards or backwards further when performing neutral jumps, in line with TR2+ (#998)
- added an option to the installer to choose between the original and fan-made Unfinished Business level sets (#1019)
- fixed baddies dropping duplicate guns (only affects mods) (#1000)
- fixed Lara never using the step back down right animation (#1014)
- fixed dead crocodiles floating in drained rooms (#1031)
- fixed 3d pickups sometimes triggering z-buffer issues (#1015)
- fixed oversized passport in cinematic camera mode (eg when Lara steps on the Midas Hand) (#1009)
- fixed braid being disabled by default unless the player runs the config tool first (#1043)
- fixed various bugs with falling movable blocks (#723)
- fixed the incorrect positioning of door 12 in Tomb of Tihocan (#1063)
- fixed a potential softlock in The Cistern by restoring a missing trigger in room 56 (#1066)
- improved frame scheduling to use less CPU (#985)
- improved and expanded gameflow documentation (#1018)
- rotated the Scion in Tomb of Qualopec to face the the main gate and Qualopec (#1007)

## [2.16](https://github.com/LostArtefacts/TR1X/compare/2.15.3...2.16) - 2023-09-20
- added a new rendering mode called "framebuffer" that lets the game to run at lower resolutions (#114)  
  (forces players to reset their bilinear filter setting)
- added the current music track and timestamp to the savegame so they now persist on load (#419)
- added the triggered music tracks to the savegame so one shot tracks don't replay on load (#371)
- added forward/backward input detection in line with TR2+ for jump-twists (#931)
- added an option to restore the mummy in City of Khamoon room 25, similar to the PS1 version (#886)
- added a flag indicating if new game plus is unlocked to the player config which allows the player to select new game plus or not when making a new game (#966)
- changed sprite-based pickups to 3D pickups when the 3D pickups option is enabled (#257)
- changed the installer to always overwrite all essential files such as the gameflow and injections (#904)
- changed the data injection system to warn when it detects invalid or missing files, rather than preventing levels from loading (#918)
- changed the gameflow to detect and skip over legacy sequence types, rather than preventing the game from starting (#882)
- fixed Natla's gun moving while she is in her semi death state (#878)
- fixed an error message from showing on exiting the game when the gym level is not present in the gameflow (#899)
- fixed the bear pat attack so it does not miss Lara (#450)
- fixed some incorrectly rotated pickups when using the 3D pickups option (#253)
- fixed dead centaurs exploding again after saving and reloading (#924)
- fixed the incorrect starting animation on centaurs that spawn from statues (#926, regression from 2.15)
- fixed jump-twist animations at times being interrupted (#932, regression from 2.15.1)
- fixed walk-run-jump at times breaking when TR2 jumping is enabled (OG bug in TR2+) (#934)
- fixed Lara jumping late with TR2 jumping enabled, as compared to normal TR1 jumping when entering the run animation initially (#975)
- fixed the reset and unbind progress bars in the controls menu for non-default bar scaling (#930)
- fixed original data issues where music triggers are not set as one shot (#939)
- fixed a missing enemy trigger in Tomb of Tihocan (#751)
- fixed incorrect trapdoor triggers in City of Khamoon and a switch trigger in Obelisk of Khamoon (#942)
- fixed the setup of two music triggers in St. Francis' Folly (#865)
- fixed data portal issues in Atlantean Stronghold that could result in a crash (#227)
- fixed the camera in Natla's Mines when pulling the lever in room 67 (#352)
- fixed flame emitter saving and loading which caused rare crashing (#947)
- fixed new game plus not working if enable_game_modes was set to false (#960, regression from 2.8)
- fixed Alt-Enter triggering game actions (#979, regression from 2.15)
- fixed Natla spinning in her semi-death and second phases when more than one is active in the level (#906)
- fixed FPS counter, perspective filter and texture filter not always saved when changed from keyboard (#988)
- moved the enable_game_modes option from the gameflow to the config tool and added a gameflow option to override (#962)
- moved the enable_save_crystals option from the gameflow to the config tool (#962)
- improved Spanish localization for the config tool
- improved support for windowed mode (#896)

## [2.15.3](https://github.com/LostArtefacts/TR1X/compare/2.15.2...2.15.3) - 2023-08-15
- fixed Lara stuttering when performing certain animations (#901, regression from 2.14)
- fixed Lara not grabbing certain edges when the swing-cancel option is enabled (#911)

## [2.15.2](https://github.com/LostArtefacts/TR1X/compare/2.15.1...2.15.2) - 2023-07-17
- fixed Natla not leaving her semi-death state after Lara takes her down for the first time (#892, regression from 2.15.1)

## [2.15.1](https://github.com/LostArtefacts/TR1X/compare/2.15...2.15.1) - 2023-07-14
- fixed the ape not performing the vault animation when climbing (#880)
- fixed holding down up or down to scroll the passport faster (#883, regression from 2.14)
- fixed Lara becoming stuck in a T-pose on rare occasions after performing a jump tiwst (#889)

## [2.15](https://github.com/LostArtefacts/TR1X/compare/2.14...2.15) - 2023-06-08
- added an option to enable TR2+ jump-twist and somersault animations (#88)
- added the ability to unbind the sidestep left and sidestep right keys (#766)
- added a cheat to explode Lara like in TR2 and TR3 (#793)
- added an inverted look camera option (#700)
- added a camera speed option for the manual camera (#815)
- added an option to fix original texture issues (#826)
- added menu specific controls meaning arrow keys, return, and escape now always function in menus (#814, regression from 2.12)
- added forward/backward jumps while looking and looking up/down while hanging if enhanced look is enabled (#848)
- added case insensitive directory and file detection (#845)
- added controller detection during runtime (#850)
- added an option to allow cancelling Lara's ledge-swinging animation (#856)
- added an option to allow Lara to jump at any point while running, similar to TR2+ (#157)
- added the ability to define the anchor room for Bacon Lara in the gameflow (#868)
- changed screen resolution option to apply immediately (#114)
- changed shaders to use GLSL 1.20 which should fix most issues with OpenGL 2.1 (#327, #685)
- changed Bacon Lara to prevent movement after her death (#875)
- fixed sounds stopping instead of pausing if game sounds in inventory are disabled (#717)
- fixed skipping Eidos logo and end credits (#541)
- fixed ceiling heights at times being miscalculated, resulting in camera issues and Lara being able to jump into the ceiling (#323)
- fixed Lara not being able to jump off trapdoors or crumbling floors if the sidestep descent fix is enabled (#830)
- fixed walk to pickups feature (#834, regression from 2.8)
- fixed .mpeg FMVs not working (#844)
- fixed the restart level passport text incorrectly showing new game in Lara's Home (#851)
- fixed quick load creating an invalid save if used when no saves are present (#853)
- fixed Lara entering body hit animations when not appropriate to do so (#857)
- fixed SkateKid causing a game crash when too many enemies are active (#866)
- fixed missiles damaging Lara when she is far beyond their damage range (#871)

## [2.14](https://github.com/LostArtefacts/TR1X/compare/2.13.2...2.14) - 2023-04-05
- added Spanish localization to the config tool
- added an option to launch Unfinished Business from the config tool (#739)
- added dart emitters to the savegame (#774)
- added the ability for level builders to stop all music via triggers (#785)
- added an option to prevent enemy speeches stopping the current music track (#762)
- changed the health, air, and enemy bars to better match the PS1 version (#698)
- fixed Larson's gun textures in Tomb of Qualopec to match the cutscene and Sanctuary of the Scion (#737)
- fixed texture issues in the Cowboy, Kold and Skateboard Kid models (#744)
- fixed the savegame requestor arrow's position with a large number of savegames and long level titles (#756)
- fixed empty holsters when starting a level with the shotgun equipped (#749)
- fixed a crash when taking a screenshot of an opening FMV (#445)
- fixed the animation of Lara's left arm when the shotgun is equipped (#771)
- fixed Lara's braid not turning to gold during the Midas touch animation (#769)
- fixed the equipped weapon's ammo showing on the inventory screen (#777)
- fixed the health, air, and enemy bars from being affected by the text scaling option (#698)
- fixed music triggers with partial masks killing the ambient track (#763)
- fixed the text and bar scaling from being able to be set below the max and min  (#698)
- fixed a data issue in Colosseum, which prevented a bat from triggering (#750)
- fixed lightning and gun flash continuing to animate in the inventory, pause and statistics screens (#767)
- fixed the FPS, healthbar, and arrows from overlapping on the inventory screen (#787)
- improved the control of Lara's braid to result in smoother animation and to detect floor collision (#761)
- increased the number of effects from 100 to 1000 (#623)
- removed the fix_pyramid_secret gameflow sequence (now handled by data injection) (#788)

## [2.13.2](https://github.com/LostArtefacts/TR1X/compare/2.13.1...2.13.2) - 2023-03-10
- fixed depth buffer size causing rendering issues on some hardware (#748, regression from 2.13)
- fixed a game crash when loading a save in which Lara had been struck by an exploding missile (#746)

## [2.13.1](https://github.com/LostArtefacts/TR1X/compare/2.13...2.13.1) - 2023-03-03
- added an option to use the PlayStation Uzi sound effects (#152)
- fixed a few flip effect sounds not playing (#743, regression from 2.12.1)
- fixed a game crash when exiting the game with a controller connected (#663)

## [2.13](https://github.com/LostArtefacts/TR1X/compare/2.12.1...2.13) - 2023-02-19
- added the ability to inject data into levels, with Lara's braid being the initial focus (#27)
- added support for .ogg, .mp3 and .wav formats for audio tracks (#688)
- added the mummy to the level kill stats if Lara touches it and it falls (#701)
- fixed save crystal collision pushing Lara through walls (#682)
- fixed passport animation when deselecting the passport (#703)
- fixed inconsistent wording in config tool health and air color options (#705)
- fixed Scion 1 respawning on load (#707)
- fixed dead water rats looking alive when a room's water is drained (#687, regression from 0.12.0)
- fixed triggered flip effects not working if there are no sound devices (#583)
- fixed the incorrect ceiling textures in Colosseum (#131)

## [2.12.1](https://github.com/LostArtefacts/TR1X/compare/2.12...2.12.1) - 2023-01-16
- fixed crash when using enhanced saves in levels with flame emitters (#693)
- fixed the death counter from breaking old saves if enhanced saves are turned on (#699)

## [2.12](https://github.com/LostArtefacts/TR1X/compare/2.11...2.12) - 2022-12-23
- added collision to save crystals (#654)
- added additional custom control schemes (#636)
- added the ability to unbind unessential keys (#657)
- added the ability to reset control schemes to default (#657)
- added customizable controller support (#659)
- added French localization to the config tool (#664)
- fixed small cracks in the UI borders for PS1-style menus (#643)
- fixed Lara loading inside a movable block if she's on a stack near a room portal (#619)
- fixed a game crash on shutdown if the action button is held down (#646)
- fixed the compass and new game menus at high text scaling (#648)
- fixed save crystals so they are single use (#654)
- fixed demo mode if the do not heal on level finish option is used (#660)
- removed the puzzle key sound effect when using save crystals (#654)
- stopped the default controls from functioning when the user unbound them (#564)

## [2.11](https://github.com/LostArtefacts/TR1X/compare/2.10.3...2.11) - 2022-10-19
- added a .NET-based configuration tool (#633)
- added graphics effects, lava emitters, flame emitters, and waterfalls to the savegame so they now persist on load (#418)
- added an option to turn off sound effect pitching (#625)
- changed passport to highlight latest save at game start (#618)
- fixed some sound effects playing in the inventory when disable_music_in_inventory is true (#486)
- fixed underwater currents breaking in rare cases (#127)
- fixed gameflow option remove_guns preventing weapon pickups in rare situations (#611)
- fixed gameflow option remove_scions causing Lara to equip weapons even if she has none (#605)
- added gameflow option remove_ammo to remove all shotgun, magnum and uzi ammo from the inventory on level start (#599)
- added gameflow option remove_medipacks to remove all medi packs from the inventory on level start (#599)
- improved the UI frame drawing, it will now look consistent across all resolutions and no longer have gaps between the lines
- fixed bridge item in City of Khamoon being incorrectly raised (#627)
- fixed Lara firing blanks indefinitely when she doesn't have pistols and is out of ammo on non-pistol weapons (#629) 

## [2.10.3](https://github.com/LostArtefacts/TR1X/compare/2.10.2...2.10.3) - 2022-09-15
- fixed save crystal mode always saving in the first slot (#607, regression from 2.8)

## [2.10.2](https://github.com/LostArtefacts/TR1X/compare/2.10.1...2.10.2) - 2022-08-03
- fixed revert_to_pistols ignoring gameflow's remove_guns (#603)

## [2.10.1](https://github.com/LostArtefacts/TR1X/compare/2.10...2.10.1) - 2022-07-27
- fixed Lara being able to equip pistols in the gym level (#594)

## [2.10](https://github.com/LostArtefacts/TR1X/compare/2.9.1...2.10) - 2022-07-26
- added a .NET-based installer
- added the option to make Lara revert to pistols on new level start (#557)
- added the PS1 style UI (#517)
- added the "Story so far..." option in the select level menu to view cutscenes and FMVs (#201)

## [2.9.1](https://github.com/LostArtefacts/TR1X/compare/2.9...2.9.1) - 2022-06-03
- fixed crash on centaur hatch (#579, regression from 2.9)

## [2.9](https://github.com/LostArtefacts/TR1X/compare/2.8.2...2.9) - 2022-06-01
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

## [2.8.2](https://github.com/LostArtefacts/TR1X/compare/2.8.1...2.8.2) - 2022-05-20
- fixed Lara not picking up items near the edges of room portals (#563, regression from 2.8)

## [2.8.1](https://github.com/LostArtefacts/TR1X/compare/2.8...2.8.1) - 2022-05-05
- fixed Pierre not resetting across levels (#538, regression from 2.7)
- fixed pushables breaking with flipped rooms when loading a save (#536, regression from 2.8)

## [2.8](https://github.com/LostArtefacts/TR1X/compare/2.7...2.8) - 2022-05-04
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

## [2.7](https://github.com/LostArtefacts/TR1X/compare/2.6.4...2.7) - 2022-03-16
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

## [2.6.4](https://github.com/LostArtefacts/TR1X/compare/2.6.3...2.6.4) - 2022-02-20
- fixed crash when loading a legacy save and saving on a new slot (#442, regression from 2.6)

## [2.6.3](https://github.com/LostArtefacts/TR1X/compare/2.6.2...2.6.3) - 2022-02-18
- fixed croc and rats breaking saves after a flipmap (#441, regression from 2.6)

## [2.6.2](https://github.com/LostArtefacts/TR1X/compare/2.6.1...2.6.2) - 2022-02-17
- fixed equipping gun after starting a demo (#440, regression from 2.6)

## [2.6.1](https://github.com/LostArtefacts/TR1X/compare/2.6...2.6.1) - 2022-02-16
- fixed equipping gun after starting the game (#439, regression from 2.6)

## [2.6](https://github.com/LostArtefacts/TR1X/compare/2.5...2.6) - 2022-02-16
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

## [2.5](https://github.com/LostArtefacts/TR1X/compare/2.4...2.5) - 2022-01-31
- added CHANGELOG.md
- added ability to skip cinematics with the Action key
- added fade animations (#363)
- added a vsync option (#364)
- fixed certain inputs skipping too many things (#359)
- fixed a memory leak in the audio sampler (#369)


## [2.4](https://github.com/LostArtefacts/TR1X/compare/2.3...2.4) - 2022-01-19
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


## [2.3](https://github.com/LostArtefacts/TR1X/compare/2.2.1...2.3) - 2022-01-12
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


## [2.2.1](https://github.com/LostArtefacts/TR1X/compare/2.2...2.2.1) - 2022-01-05
- fixed listing available resolutions (a regression from 2.2)
- fixed Lara's airbar showing up when Lara's dead (a regression from 2.1)


## [2.2](https://github.com/LostArtefacts/TR1X/compare/2.1...2.2) - 2022-01-05
- added ability to control anisotropy filter strength
- changed the engine look for HD FMVs by default for Unfinished Business
- removed tiny screen resolutions (might require setting the resolution again)
- fixed Lara getting set on fire on trapdoors over lava
- fixed letterbox in main menu showing garbage data on certain machines
- fixed save crystals saving before gym level
- fixed black lines appearing on walls and floors
- fixed hang bug for stacked rooms


## [2.1](https://github.com/LostArtefacts/TR1X/compare/...2.1) - 2021-12-21
- added ability to disable healthbar and airbar flashing
- changed the engine look for HD FMVs by default
- increased max active samples to 20 (should fix rare mute sounds issues)
- fixed loading TombATI Atlantis saves
- fixed shotgun shooting when target out of sight
- fixed save selection being offscreen if the first savegame starts with high enough number
- fixed alligators dealing no damage under certain circumstances
- fixed grabbing bridges under certain circumstances
- fixed crash if user presses a key during ring close animation

## [2.0.1](https://github.com/LostArtefacts/TR1X/compare/2.0...2.0.1) - 2021-12-13
Added an icon to the .exe (thanks TRFan94!)


## [2.0](https://github.com/LostArtefacts/TR1X/compare/1.4.0...2.0) - 2021-12-07
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


## [1.4.0](https://github.com/LostArtefacts/TR1X/compare/1.3.0...1.4.0) - 2021-11-16
- added adjustable ingame brightness
- added per-level fog settings
- added control over fog density (in terms of tiles)
- improved TR3 sidesteps
- improved wording in readme
- fixed lighting for 3D pickups
- fixed a crash when drawing lightnings
- fixed a crash when compiling the game on MSVC


## [1.3.0](https://github.com/LostArtefacts/TR1X/compare/1.2.2...1.3.0) - 2021-11-06
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


## [1.2.2](https://github.com/LostArtefacts/TR1X/compare/1.2.1...1.2.2) - 2021-10-17
- added ability to mute music in main menu
- added pausing the music while in pause
- added more screen resolutions
- fixed demos playing oddly when the enhanced look option is enabled
- fixed shadows rendering
- fixed too big healthbar margins on low resolutions
- fixed bilinear filter not working
- fixed resolution width/height being ignored


## [1.2.1](https://github.com/LostArtefacts/TR1X/compare/1.2.0...1.2.1) - 2021-10-17
- added resolution_width and resolution_height to the default settings
- fixed screen resolution regression from 1.2.0


## [1.2.0](https://github.com/LostArtefacts/TR1X/compare/1.1.5...1.2.0) - 2021-10-15
- fixed a common crash on many machines


## [1.1.5](https://github.com/LostArtefacts/TR1X/compare/1.1.4...1.1.5) - 2021-10-13
- fixed a regression resulting in crashes from 1.1.4


## [1.1.4](https://github.com/LostArtefacts/TR1X/compare/1.1.3...1.1.4) - 2021-10-13
- fixed problem with the alt key on certain machines
- fixed a rare crash on certain machines


## [1.1.3](https://github.com/LostArtefacts/TR1X/compare/1.1.2...1.1.3) - 2021-03-30
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


## [1.1.2](https://github.com/LostArtefacts/TR1X/compare/1.1.1...1.1.2) - 2021-03-30
- fixed main menu demo mode not playing correctly (regression from 1.1.1)
- fixed game speeding up on certain machines (regression from 1.1.1)


## [1.1.1](https://github.com/LostArtefacts/TR1X/compare/1.1...1.1.1) - 2021-03-29
- added deactivating game when Alt-Tabbing
- improved pink bar color
- fixed sounds volume slider not working for ingame sounds


## [1.1](https://github.com/LostArtefacts/TR1X/compare/1.0...1.1) - 2021-03-28
- added an alert messagebox whenever something bad (within the code's expectations) happens
- added smooth bars (needs to be explicitly enabled in the settings)
- finished porting the input and sound routines
- fixed custom bar colors not working in certain levels
- fixed RNG not being seeded (no practical consequences on the gameplay)


## [1.0](https://github.com/LostArtefacts/TR1X/compare/0.13.3...1.0) - 2021-03-21
- added pause screen
- added -gold command line switch to run Unfinished Business


## [0.13.3](https://github.com/LostArtefacts/TR1X/compare/0.13.2...0.13.3) - 2021-03-21
- added crystals mode (can be enabled in the gameflow)
- improved navigation through keyboard controls UI
- fixed Unfinished Business gameflow not loading
- fixed OG conflicting controls not flashing after relaunching the game
- fixed drawing Lara's hair when she carries shotgun on her back
- fixed loading custom layouts that conflict with default controls


## [0.13.2](https://github.com/LostArtefacts/TR1X/compare/0.13.1...0.13.2) - 2021-03-19
- fixed lighting issues (Lara being sometimes very brightly lighted)


## [0.13.1](https://github.com/LostArtefacts/TR1X/compare/0.13.0...0.13.1) - 2021-03-19
- changed demo_delay constant to be stored in the gameflow file
- fixed regression in LoadSamples


## [0.13.0](https://github.com/LostArtefacts/TR1X/compare/0.12.7...0.13.0) - 2021-03-19
- added display_time parameter to display_picture (requires overwriting your gameflow file)
- added user controllable UI and bar scaling
- changed limit of max items (moveables in TRLE lingo) from 256 to 10240
- fixed whacky navigation in controls dialog if cheats are enabled
- fixed regression in LoadItems that crashes Atlantis
- fixed skipping pictures displayed before starting the level with the escape key causing inventory to open


## [0.12.7](https://github.com/LostArtefacts/TR1X/compare/0.12.6...0.12.7) - 2021-03-19
- added ability to remap cheat keys (except obscure f11 debug key)
- changed f10 level skip cheat key to 'x' (can be now changed); had to be done because the game does not let mapping to function keys
- changed lots of variables to stay in T1M memory (may cause regressions)
- changed runtime game config to be read and written to a new JSON configuration rather than atiset.cfg
- changed files directory placement to a new directory, cfg/


## [0.12.6](https://github.com/LostArtefacts/TR1X/compare/0.12.5...0.12.6) - 2021-03-18
- fixed loading game in Natla's Mines causing Lara to lose her guns


## [0.12.5](https://github.com/LostArtefacts/TR1X/compare/0.12.4...0.12.5) - 2021-03-17
- fixed collected secrets resetting after using compass


## [0.12.4](https://github.com/LostArtefacts/TR1X/compare/0.12.3...0.12.4) - 2021-03-17
- added showing level stats in compass (can be disabled)
- added ability to disable game mode selection in gameflow
- added fallback gameflow strings (in case someone installs new T1M but forgets to't override the gameflow file)
- added ability to exit level stats with escape
- changed ingame timer to tick also in the inventory (can be disabled)
- changed bar sizes and location to match TR2Main
- fixed reading key configuration for keys that override defaults
- fixed calculating creature distances (fixes Tihocan croc bug)


## [0.12.3](https://github.com/LostArtefacts/TR1X/compare/0.12.2...0.12.3) - 2021-03-17
- add Japanese mode (enemies are 2 times weaker)
- improve skipping cutscenes
- fix crash when FMVs are missing (this doesn't add support for HQ FMVs though)


## [0.12.2](https://github.com/LostArtefacts/TR1X/compare/0.12.1...0.12.2) - 2021-03-14
- changed settings to save after each change
- fixed OG music stopping when playing the secrets chime (can be disabled)
- fixed OG game not saving key layout choice (default vs. user keys)
- fixed OG volume slider not working when starting muted
- fixed OG holding action to skip credit pictures skipping them all at once
- fixed OG holding escape to skip FMVs opening inventory


## [0.12.1](https://github.com/LostArtefacts/TR1X/compare/0.12.0...0.12.1) - 2021-03-14
- huge internal refactors
- improved door open cheat
- changed 4k scaling path to be always enabled (previously known as enable_enhanced_ui)
- fixed killing music underwater
- fixed main menu background for UB


## [0.12.0](https://github.com/LostArtefacts/TR1X/compare/0.11.1...0.12.0) - 2021-03-12
- introduced gameflow sequencer (moves FMVs, cutscenes, level stats etc. logic to the gameflow JSON file); add ability to control number of levels
- refactored gameflow
- added ability to disable cinematic scenes
- changed automatic calculation of secret count to be always enabled
- fixed starting NG+ from gym not working
- fixed cinematics resetting FOV


## [0.11.1](https://github.com/LostArtefacts/TR1X/compare/0.11...0.11.1) - 2021-03-11
- added ability to turn off main menu demos
- added ability to turn off FMVs
- added reporting JSON parsing errors in the logs
- fixed reading config sometimes not working
- fixed killing music in the inventory
- fixed missing Demo Mode text
- fixed showing Eidos logo for too short
- fixed Lara wearing normal clothes in Gym


## [0.11](https://github.com/LostArtefacts/TR1X/compare/0.10.5...0.11) - 2021-03-11
- introduced gameflow file (moves all game strings to a gameflow JSON file, including level paths and names); level number, FMVs etc. are still hardcoded


## [0.10.5](https://github.com/LostArtefacts/TR1X/compare/0.10.4...0.10.5) - 2021-03-10
- added arrows to save/load dialogs
- improved user keys settings dialog - you don't have to hold the key for exactly 1 frame anymore
- made new game dialog smaller
- fixed passport closing when exiting new game mode selection dialog


## [0.10.4](https://github.com/LostArtefacts/TR1X/compare/0.10.3...0.10.4) - 2021-03-08
- fixed load game screen


## [0.10.3](https://github.com/LostArtefacts/TR1X/compare/0.10.2...0.10.3) - 2021-03-08
- added NG/NG+ mode selection


## [0.10.2](https://github.com/LostArtefacts/TR1X/compare/0.10.1...0.10.2) - 2021-03-07
- fixed fly cheat resurrection with lava wedges


## [0.10.1](https://github.com/LostArtefacts/TR1X/compare/0.10...0.10.1) - 2021-03-07
- improved dealing with missing config
- renamed config to .json5
- fixed sound going off after playing a cinematic


## [0.10](https://github.com/LostArtefacts/TR1X/compare/0.9.2...0.10) - 2021-03-06
- added support for opening closest doors


## [0.9.2](https://github.com/LostArtefacts/TR1X/compare/0.9.1...0.9.2) - 2021-03-05
- fixed messged up FMV sequence IDs
- fixed crash when drawing lightnings near Scion


## [0.9.1](https://github.com/LostArtefacts/TR1X/compare/0.9...0.9.1) - 2021-03-04
- fixed bats flying near floor
- fixed typo in Tomb1Main.json causing everything to be disabled


## [0.9](https://github.com/LostArtefacts/TR1X/compare/0.8.3...0.9) - 2021-03-03
- added FOV support (overrides GLrage completely, but should be compatible with it)
- added support for more than 3 pickups at once (for TRLE builders)
- fixed smaller pickup sprites
- fixed showing FPS in the main menu doing weird stuff to the inventory text after starting the game


## [0.8.3](https://github.com/LostArtefacts/TR1X/compare/0.8.2...0.8.3) - 2021-02-28
- improved TR3-like sidesteps
- improved bar flashing modes
- fixed Lara targeting enemies even after death
- fixed version information missing from releases


## [0.8.2](https://github.com/LostArtefacts/TR1X/compare/0.8.1...0.8.2) - 2021-02-28
- fixed Lara drawing guns when loading OG saves


## [0.8.1](https://github.com/LostArtefacts/TR1X/compare/0.8...0.8.1) - 2021-02-27
- fixed AI sometimes having problems to find Lara
- fixed shotgun firing sound after running out of ammo
- fixed OG being able to get pistols by running out of ammo in other weapons, even without having them in the inventory


## [0.8](https://github.com/LostArtefacts/TR1X/compare/0.7.6...0.8) - 2021-02-27
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


## [0.7.6](https://github.com/LostArtefacts/TR1X/compare/0.7.5...0.7.6) - 2021-02-23
- fixed Atlanteans behavior


## [0.7.5](https://github.com/LostArtefacts/TR1X/compare/0.7.4...0.7.5) - 2021-02-22
- fixed ammo text placement
- fixed healthbar placement in the inventory


## [0.7.4](https://github.com/LostArtefacts/TR1X/compare/0.7.3...0.7.4) - 2021-02-22
- added support for user-configured bar colors
- switched configuration format to use JSON5
- moved comments to Tomb5Main.json
- fixed bar placement


## [0.7.3](https://github.com/LostArtefacts/TR1X/compare/0.7.2...0.7.3) - 2021-02-22
- added support for user-configured bar locations
- fixed pickups scaling


## [0.7.2](https://github.com/LostArtefacts/TR1X/compare/0.7.1...0.7.2) - 2021-02-22
- fixed ability to look around while Lara's dead
- fixed UI scaling in controls dialog
- fixed crash for some creatures


## [0.7.1](https://github.com/LostArtefacts/TR1X/compare/0.7...0.7.1) - 2021-02-22
- added inventory cheat
- made fly cheat faster


## [0.7](https://github.com/LostArtefacts/TR1X/compare/0.6...0.7) - 2021-02-21
- added fly cheat
- fixed a crash when hit by a lightning (T1M regression)
- fixed missing "Demo Mode" text (T1M regression)


## [0.6](https://github.com/LostArtefacts/TR1X/compare/0.5.1...0.6) - 2021-02-20
- changed the code to count secrets automatically (useful for custom level builders)
- fixed secret trigger in The Great Pyramid
- fixed a crash when loading levels with more than 1024 textures
- fixed drawing Lara (T1M regression)


## [0.5.1](https://github.com/LostArtefacts/TR1X/compare/0.5...0.5.1) - 2021-02-20
- added fire sprite to shotgun


## [0.5](https://github.com/LostArtefacts/TR1X/compare/0.4.1...0.5) - 2021-02-18
- renamed the project from TR1Main to Tomb1Main on the request of Arsunt
- improved documentation


## [0.4.1](https://github.com/LostArtefacts/TR1X/compare/...0.4.1) - 2021-02-15
- added an option to always show the healthbar
- fixed enemy healthbars in NG+
- fixed no heal mode

## [0.4](https://github.com/LostArtefacts/TR1X/compare/0.3.1...0.4) - 2021-02-14
- added UI scaling
- added ability to look around underwater


## [0.3.1](https://github.com/LostArtefacts/TR1X/compare/...0.3.1) - 2021-02-13
- improved the ability to look around while running

## [0.3](https://github.com/LostArtefacts/TR1X/compare/0.2.1...0.3) - 2021-02-13
- added an option disable magnums
- added an option disable uzis
- added an option disable shotgun
- added ability to look around while running
- added support for using items with numeric keys
- fixed an OG bug with the secret sound in Tomb of Tihocan


## [0.2.1](https://github.com/LostArtefacts/TR1X/compare/0.2...0.2.1) - 2021-02-11
- changed the default configuration to enable enemy healthbars, red healthbar and end of the level freeze fix


## [0.2](https://github.com/LostArtefacts/TR1X/compare/0.1...0.2) - 2021-02-11
- added enemy healthbars
- added a red healthbar


## [0.1](https://github.com/LostArtefacts/TR1X/compare/...0.1) - 2021-02-10

Initial version.
