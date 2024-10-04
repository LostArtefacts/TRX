## [Unreleased](https://github.com/LostArtefacts/TRX/compare/stable...develop) - ××××-××-××
- added `/sfx` command
- added `/nextlevel` alias to `/endlevel` console command
- added `/quit` alias to `/exit` console command
- changed `/set` console command to do fuzzy matching (LostArtefacts/libtrx#38)
- fixed crash in the `/set` console command (regression from 0.3)
- fixed using console in cutscenes immediately exiting the game (regression from 0.3)
- fixed Lara remaining tilted when teleporting off a vehicle while on a slope (LostArtefacts/TR2X#275, regression from 0.3)
- fixed `/endlevel` displaying a success message in the title screen
- improved vertex movement when looking through water portals (#1493)

## [0.3](https://github.com/LostArtefacts/TR2X/compare/0.2...0.3) - 2024-09-20
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
- added an ability to remap the console key (LostArtefacts/TR2X#163)
- added `/tp` console command's ability to teleport to specific items
- added `/fly` console command's ability to open nearest doors
- added an option to fix M16 accuracy while running (LostArtefacts/TR2X#45)
- added a .NET-based configuration tool (LostArtefacts/TR2X#197)
- changed the default flare key from `/` to `.` to avoid conflicts with the console (LostArtefacts/TR2X#163)
- fixed numeric keys interfering with the demos (LostArtefacts/TR2X#172)
- fixed explosions sometimes being drawn too dark (LostArtefacts/TR2X#187)
- fixed killing the T-Rex with a grenade launcher crashing the game (LostArtefacts/TR2X#168)
- fixed secret rewards not displaying shotgun ammo (LostArtefacts/TR2X#159)
- fixed controls dialog remapping being too sensitive (LostArtefacts/TR2X#5)
- fixed `/tp` console command during special animations in HSH and Offshore Rig (LostArtefacts/TR2X#178, regression from 0.2)
- fixed `/hp` console command taking arbitrary integers
- fixed console commands being able to interfere with demos, cutscenes and the title screen (LostArtefacts/TR2X#182, #179, regression from 0.2)
- fixed console registering key inputs too eagerly (regression from 0.2)
- fixed console not being drawn in cutscenes (LostArtefacts/TR2X#180, regression from 0.2)
- fixed sounds not playing under certain circumstances (LostArtefacts/TR2X#113, regression from 0.2)
- fixed the excessive pitch and playback speed correction for music files with sampling rate other than 44100 Hz (LostArtefacts/TR1X#1417, regression from 0.2)
- fixed a crash potential with certain music files (regression from 0.2)
- fixed enemy movement patterns in demo 1 and demo 3 (LostArtefacts/TR2X#98, regression from 0.1)
- fixed underwater creatures dying (LostArtefacts/TR2X#98, regression from 0.1)
- fixed a crash when spawning enemy drops (LostArtefacts/TR2X#125, regression from 0.1)
- fixed how sprites are shaded (LostArtefacts/TR2X#134, regression from 0.1.1)
- fixed enemies unable to climb (LostArtefacts/TR2X#138, regression from 0.1)
- fixed items not being reset between level loads (LostArtefacts/TR2X#142, regression from 0.1)
- fixed pulling the dagger from the dragon not activating triggers (LostArtefacts/TR2X#148, regression from 0.1)
- fixed the music at the beginning of Offshore Rig not playing (LostArtefacts/TR2X#150, regression from 0.1)
- fixed wade animation when moving from deep to shallow water (LostArtefacts/TR2X#231, regression from 0.1)
- fixed the distorted skybox in room 5 of Barkhang Monastery (LostArtefacts/TR2X#196)
- improved initial level load time by lazy-loading audio samples (LostArtefacts/TR2X#114)
- improved crash debug information (LostArtefacts/TR2X#137)
- improved the console caret sprite (LostArtefacts/TR2X#91)

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
- fixed seams around underwater portals (LostArtefacts/TR2X#76, regression from 0.1)
- fixed Lara's climb down camera angle (LostArtefacts/TR2X#78, regression from 0.1)
- fixed healthbar and airbar flashing the wrong way when at low values (LostArtefacts/TR2X#82, regression from 0.1)

## [0.1.1](https://github.com/LostArtefacts/TR2X/compare/0.1...0.1.1) - 2024-04-27
- fixed Lara's shadow with z-buffer option on (LostArtefacts/TR2X#64, regression from 0.1)
- fixed rare camera issues (LostArtefacts/TR2X#65, regression from 0.1)
- fixed flat rectangle colors (LostArtefacts/TR2X#70, regression from 0.1)
- fixed medpacks staying open after use in Lara's inventory (LostArtefacts/TR2X#69, regression from 0.1)
- fixed pickup sprites UI drawn forever in Lara's Home (LostArtefacts/TR2X#68, regression from 0.1)

## [0.1](https://github.com/rr-/TR2X/compare/...0.1) - 2024-04-26
- added version string to the inventory
- fixed CDAudio not playing on certain versions (uses PaulD patch)
- fixed TGA screenshots crashing the game
