## [Unreleased](https://github.com/LostArtefacts/TR2X/compare/stable...develop) - ××××-××-××
- added `/sfx` command
- added `/nextlevel` alias to `/endlevel` console command
- added `/quit` alias to `/exit` console command
- changed `/set` console command to do fuzzy matching (LostArtefacts/libtrx#38)
- fixed crash in the `/set` console command (regression from 0.3)
- fixed using console in cutscenes immediately exiting the game (regression from 0.3)
- fixed Lara remaining tilted when teleporting off a vehicle while on a slope (#275, regression from 0.3)
- fixed `/endlevel` displaying a success message in the title screen

## [0.3](https://github.com/LostArtefacts/TR2X/compare/0.2-460-g4721b93...0.3) - 2024-09-20
- added new console commands:
    - `/endlevel`
    - `/demo`
    - `/title`
    - `/play [level]`
    - `/load [slot]`
    - `/save [slot]`
    - `/exit`
    - `/fly`
    - `/give`
    - `/kill`
    - `/flip`
    - `/set`
- added an ability to remap the console key (#163)
- added `/tp` console command's ability to teleport to specific items
- added `/fly` console command's ability to open nearest doors
- added an option to fix M16 accuracy while running (#45)
- added a .NET-based configuration tool (#197)
- changed the default flare key from `/` to `.` to avoid conflicts with the console (#163)
- fixed numeric keys interfering with the demos (#172)
- fixed explosions sometimes being drawn too dark (#187)
- fixed killing the T-Rex with a grenade launcher crashing the game (#168)
- fixed secret rewards not displaying shotgun ammo (#159)
- fixed controls dialog remapping being too sensitive (#5)
- fixed `/tp` console command during special animations in HSH and Offshore Rig (#178, regression from 0.2)
- fixed `/hp` console command taking arbitrary integers
- fixed console commands being able to interfere with demos, cutscenes and the title screen (#182, #179, regression from 0.2)
- fixed console registering key inputs too eagerly (regression from 0.2)
- fixed console not being drawn in cutscenes (#180, regression from 0.2)
- fixed sounds not playing under certain circumstances (#113, regression from 0.2)
- fixed the excessive pitch and playback speed correction for music files with sampling rate other than 44100 Hz (LostArtefacts/TR1X#1417, regression from 0.2)
- fixed a crash potential with certain music files (regression from 0.2)
- fixed enemy movement patterns in demo 1 and demo 3 (#98, regression from 0.1)
- fixed underwater creatures dying (#98, regression from 0.1)
- fixed a crash when spawning enemy drops (#125, regression from 0.1)
- fixed how sprites are shaded (#134, regression from 0.1.1)
- fixed enemies unable to climb (#138, regression from 0.1)
- fixed items not being reset between level loads (#142, regression from 0.1)
- fixed pulling the dagger from the dragon not activating triggers (#148, regression from 0.1)
- fixed the music at the beginning of Offshore Rig not playing (#150, regression from 0.1)
- fixed wade animation when moving from deep to shallow water (#231, regression from 0.1)
- fixed the distorted skybox in room 5 of Barkhang Monastery (#196)
- improved initial level load time by lazy-loading audio samples (#114)
- improved crash debug information (#137)
- improved the console caret sprite (#91)

## [0.2](https://github.com/LostArtefacts/TR2X/compare/0.1.1...0.2) - 2024-05-07
- added dev console with the following commands:
    - `/pos`
    - `/tp [room_num]`
    - `/tp [x] [y] [z]`
    - `/hp`
    - `/hp [num]`
    - `/heal`
- changed the music backend from WinMM to libtrx (SDL + libav)
- changed the sound backend from DirectX to libtrx (SDL + libav)
- fixed seams around underwater portals (#76, regression from 0.1)
- fixed Lara's climb down camera angle (#78, regression from 0.1)
- fixed healthbar and airbar flashing the wrong way when at low values (#82, regression from 0.1)

## [0.1.1](https://github.com/LostArtefacts/TR2X/compare/0.1...0.1.1) - 2024-04-27
- fixed Lara's shadow with z-buffer option on (#64, regression from 0.1)
- fixed rare camera issues (#65, regression from 0.1)
- fixed flat rectangle colors (#70, regression from 0.1)
- fixed medpacks staying open after use in Lara's inventory (#69, regression from 0.1)
- fixed pickup sprites UI drawn forever in Lara's Home (#68, regression from 0.1)

## [0.1](https://github.com/rr-/TR2X/compare/...0.1) - 2024-04-26
- added version string to the inventory
- fixed CDAudio not playing on certain versions (uses PaulD patch)
- fixed TGA screenshots crashing the game
