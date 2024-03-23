<p align="center">
<img alt="TR1X logo" src="data/logo-light-theme.png#gh-light-mode-only" width="400"/>
<img alt="TR1X logo" src="data/logo-dark-theme.png#gh-dark-mode-only" width="400"/>
</p>

This is an open source implementation of the classic Tomb Raider I game (1996),
made by reverse engineering the TombATI / GLRage variant of the original game
and replacing proprietary audio/video libraries with open source variants.

See the [Tomb Raider Forums
topic](https://www.tombraiderforums.com/showthread.php?p=8286101).

## Showcase

<table>
    <tr>
        <th>
            Restored braid
            <img src="docs/showcase/braid.jpg"/>
        </th>
        <th>
            Enemy health bar and UI scaling
            <img src="docs/showcase/enemy_health_bar_and_scaling.jpg"/>
        </th>
    </tr>
    <tr>
        <th>
            3D pickups
            <img src="docs/showcase/3d_pickups.jpg"/>
        </th>
        <th>
            Improved stats
            <img src="docs/showcase/compass_stats.jpg"/>
        </th>
    </tr>
    <tr>
        <th>
            Customizable draw distance
            <img src="docs/showcase/draw_distance.webp"/>
        </th>
        <th>
            Fly cheat
            <img src="docs/showcase/fly_cheat.jpg"/>
        </th>
    </tr>
    <tr>
        <th>
            Free camera
            <img src="docs/showcase/free_camera.jpg"/>
        </th>
        <th>
            PS1 UI and new graphics options
            <img src="docs/showcase/ps1_ui_and_gfx.jpg"/>
        </th>
    </tr>
</table>

## Windows / Linux

### Installing (simplified)

1. Head over to GitHub releases: https://github.com/LostArtefacts/TR1X/releases
2. Download the installer. Your browser may complain that the .exe is unsafe, but it's OK to ignore this alert.
3. Mark the installer EXE as safe to run by right-clicking on the .exe, going to properties and clicking "Unblock".
4. Run the installer and proceed with the steps.

We hope that eventually these alerts will go away as the popularity of the project rises.

### Installing (advanced / manual)

1. Head over to GitHub releases: https://github.com/LostArtefacts/TR1X/releases
2. Download the zip file.
3. Extract the zip file into a directory of your choice.  
   Make sure you choose to overwrite existing directories and files
   (`cfg/TR1X_config.json5` can remain, but new features will not be configurable).
4. (First time installation) Put your original game files into the target directory.
    1. For Steam and GOG users, extract the original `GAME.BIN` file using a tool such as UltraISO to your target directory.
       Note that neither the GOG nor the Steam releases ship the music files. You have a few options here:
       - You can download the music files from the link below.  
         https://lostartefacts.dev/aux/tr1x/music.zip
         The legality of this approach is disputable.
       - Rip the assets yourself from a physical PlayStation/SegaSaturn disk.

       Optionally you can also install the Unfinished Business expansion pack files.
       - Either one of these these variants:
         - https://lostartefacts.dev/aux/tr1x/trub-music.zip (fan-made patch to include music triggers)
         - https://lostartefacts.dev/aux/tr1x/trub-vanilla.zip (original level files, which do not include music triggers)
       - Or the more manual link: https://archive.org/details/tomb-raider-i-unfinished-business-pc-eng-full-version_20201225
   2. For TombATI users this means copying the `data`, `fmv` and `music` directories.
5. To play the game, run `TR1X.exe`.
6. To play the Unfinished Expansion pack, run `TR1X.exe -gold`.

If you install everything correctly, your game directory should look more or
less like this (click to expand):

<details>
<p><em>* Will not be present until the game has been launched.</em></p>
<pre>
.
├── cfg
│   ├── TR1X_gameflow.json5
│   ├── TR1X_gameflow_ub.json5
│   ├── TR1X.json5 *
├── data
│   ├── cat.phd
│   ├── cred0.pcx
│   ├── cred1.pcx
│   ├── cred2.pcx
│   ├── cred3.pcx
│   ├── cut1.phd
│   ├── cut2.phd
│   ├── cut3.phd
│   ├── cut4.phd
│   ├── egypt.phd
│   ├── eidospc.pcx
│   ├── eidospc.png
│   ├── end2.phd
│   ├── end.pcx
│   ├── end.phd
│   ├── gym.phd
│   ├── install.pcx
│   ├── level10a.phd
│   ├── level10b.phd
│   ├── level10c.phd
│   ├── level1.phd
│   ├── level2.phd
│   ├── level3a.phd
│   ├── level3b.phd
│   ├── level4.phd
│   ├── level5.phd
│   ├── level6.phd
│   ├── level7a.phd
│   ├── level7b.phd
│   ├── level8a.phd
│   ├── level8b.phd
│   ├── level8c.phd
│   ├── titleh.pcx
│   ├── titleh.png
│   ├── titleh_ub.png
│   └── title.phd
├── fmv
│   ├── cafe.rpl
│   ├── canyon.rpl
│   ├── core.avi
│   ├── end.rpl
│   ├── escape.rpl
│   ├── lift.rpl
│   ├── mansion.rpl
│   ├── prison.rpl
│   ├── pyramid.rpl
│   ├── snow.rpl
│   └── vision.rpl
├── music
│   ├── track02.flac
│   ├── track03.flac
│   ├── track04.flac
│   ├── track05.flac
│   ├── track06.flac
│   ├── track07.flac
│   ├── track08.flac
│   ├── track09.flac
│   ├── track10.flac
│   ├── track11.flac
│   ├── track12.flac
│   ├── track13.flac
│   ├── track14.flac
│   ├── track15.flac
│   ├── track16.flac
│   ├── track17.flac
│   ├── track18.flac
│   ├── track19.flac
│   ├── track20.flac
│   ├── track21.flac
│   ├── track22.flac
│   ├── track23.flac
│   ├── track24.flac
│   ├── track25.flac
│   ├── track26.flac
│   ├── track27.flac
│   ├── track28.flac
│   ├── track29.flac
│   ├── track30.flac
│   ├── track31.flac
│   ├── track32.flac
│   ├── track33.flac
│   ├── track34.flac
│   ├── track35.flac
│   ├── track36.flac
│   ├── track37.flac
│   ├── track38.flac
│   ├── track39.flac
│   ├── track40.flac
│   ├── track41.flac
│   ├── track42.flac
│   ├── track43.flac
│   ├── track44.flac
│   ├── track45.flac
│   ├── track46.flac
│   ├── track47.flac
│   ├── track48.flac
│   ├── track49.flac
│   ├── track50.flac
│   ├── track51.flac
│   ├── track52.flac
│   ├── track53.flac
│   ├── track54.flac
│   ├── track55.flac
│   ├── track56.flac
│   ├── track57.flac
│   ├── track58.flac
│   ├── track59.flac
│   └── track60.flac
├── shaders
│   ├── 2d.fsh
│   ├── 2d.vsh
│   ├── 3d.fsh
│   └── 3d.vsh
├── TR1X.exe
├── TR1X_ConfigTool.exe
</pre>
</details>

### Configuring

To configure TR1X, run the `TR1X_ConfigTool.exe` application. All the
configuration is explained in this tool. Alternatively, after running the game
at least once, you can edit `TR1X.json5` manually in a text editor such
as Notepad.

## macOS

### Installing

1. Head over to GitHub releases: https://github.com/LostArtefacts/TR1X/releases
2. Download the `TR1X-Installer.dmg` installer image. Mount the image and drag TR1X to the Applications folder.
3. Find TR1X in your Applications folder. Right-click it and click "Show Package Contents".
4. Copy your Tomb Raider 1 game data files into `Contents/Resources/data`. (See the Windows / Linux instructions for retrieving game data from e.g. GOG.)

## Improvements over original game

Not all options are turned on by default. Refer to `TR1X_ConfigTool.exe` for details.

#### UI
- added proper UI and bar scaling
- added enemy health bars
- added PS1 style UI
- added fade effects to displayed images
- added an option to use PS1 loading screens
- improved support for windowed mode

#### Gameplay
- added ability to set user-defined FOV
- added ability to select weapons / using items with numeric keys
- added ability to look around while running
- added ability to forward and backward jump while looking
- added ability to look up and down while hanging
- added ability to sidestep like in TR3
- added ability to jump-twist and somersault like in TR2+
- added ability to cancel ledge-swinging animation like in TR2+
- added ability to jump at any point while running like in TR2+
- added ability to automatically walk to items when nearby
- added ability to roll while underwater like in TR2+
- added a pause screen
- added a choice whether to play NG or NG+ without having to play the entire game
- added Japanese mode (guns deal twice the damage, inspired by JP release of TR3); available for both NG and NG+
- added ability to restart level on death
- added ability to restart the adventure from any level when loading a game
- added the "Story so far..." option in the select level menu to view cutscenes and FMVs
- added graphics effects, lava emitters, flame emitters, and waterfalls to the savegame so they now persist on load
- added an option to restore the mummy in City of Khamoon room 25, similar to the PS version
- added a flag indicating if new game plus is unlocked to the player config which allows the player to select new game plus or not when making a new game
- fixed keys and items not working when drawing guns immediately after using them
- fixed counting the secret in The Great Pyramid
- fixed running out of ammo forcing Lara to equip pistols even if she doesn't carry them
- fixed a crash when Lara is on fire and goes too far away from where she caught fire
- fixed flames not being drawn when Lara is on fire and leaves the room where she caught fire
- fixed settings not being saved when exiting the game with Alt+F4
- fixed settings not persisting chosen layout (default vs. user keys)
- fixed the infamous Tihocan crocodile bug (integer overflow causing creatures to deal damage across the entire level)
- fixed missiles damaging Lara when she is far beyond their damage range
- fixed Lara not being able to grab parts of some bridges
- fixed Lara voiding if a badly placed timed door closes on her (doesn't occur in OG levels)
- fixed bats being positioned too high
- fixed alligators dealing no damage if Lara remains still in the water
- fixed shotgun shooting when a locked target moves out of Lara's sight
- fixed shotgun shooting too fast when not aiming at a target
- fixed Lara grabbing ledges she shouldn't in stacked rooms (mainly St. Francis Folly tower)
- fixed rare cases of Lara getting set on fire on a bridge over lava
- fixed saving the game near Bacon Lara breaking her movement
- fixed Lara glitching through static objects into a black void
- fixed Lara pushing blocks through doors
- fixed Lara switching to pistols when completing a level with other guns
- fixed empty mutant shells in Unfinished Business spawning Lara's hips
- fixed gun pickups disappearing in rare circumstances on save load (#406)
- fixed broken dart ricochet effect
- fixed exploded mutant pods sometimes appearing unhatched on reload
- fixed bridges at floor level appearing under the floor
- fixed underwater currents breaking in rare cases
- fixed Lara loading inside a movable block if she's on a stack near a room portal
- fixed a game crash on shutdown if the action button is held down
- fixed Scion 1 respawning on load
- fixed triggered flip effects not working if there are no sound devices
- fixed ceiling heights at times being miscalculated, resulting in camera issues and Lara being able to jump into the ceiling
- fixed the ape not performing the vault animation when climbing
- fixed Natla's gun moving while she is in her semi death state
- fixed the bear pat attack so it does not miss Lara
- fixed dead centaurs exploding again after saving and reloading
- fixed the following floor data issues:
    - **St. Francis' Folly**: moved the music trigger for track 3 in room 4 behind the Neptune door, and restored track 15 to play after using the 4 keys
    - **The Cistern**: missing trigger in room 56 which could result in a softlock
    - **Tomb of Tihocan**: missing trigger in room 62 for enemy 34
    - **City of Khamoon**: incorrect trapdoor trigger types in rooms 31 and 34
    - **Obelisk of Khamoon**: missing switch trigger type in room 66
    - **Atlantean Stronghold**: fixed poorly configured portals between rooms 74 and 12
- fixed various bugs with falling movable blocks
- fixed bugs when trying to stack multiple movable blocks
- fixed Midas's touch having unrestricted vertical range

#### Cheats
- added a fly cheat
- added a level skip cheat
- added a door open cheat (while in fly mode)
- added a cheat to increase the game speed
- added a cheat to explode Lara like in TR2 and TR3

#### Input
- added ability to move camera around with W,A,S,D
- added additional custom control schemes
- added the ability to unbind unessential keys
- added the ability to reset control schemes to default
- added customizable controller support
- added an inverted look camera option
- fixed freeze when holding the Action key during end of level
- fixed inability to switch Control keys when shimmying
- fixed setting user keys being very difficult
- fixed skipping FMVs triggering inventory
- fixed skipping credits working too fast
- fixed not being able to close level stats with Escape
- fixed Lara jumping forever when alt+tabbing out of the game
- stopped the default controls from functioning when the user unbound them
- added the option to change weapon targets by tapping the look key like in TR4+
- added three targeting lock options:
  - full lock: always keep target lock even if the enemy moves out of sight or dies (OG TR1)
  - semi lock: keep target lock if the enemy moves out of sight but lose target lock if the enemy dies
  - no lock: lose target lock if the enemey goes out of sight or dies (TR4+)

#### Statistics
- added ability to keep timer on in inventory
- added optional compass level stats
- added optional final statistics screen
- added optional deaths counter
- added optional total pickups and kills per level
- added unobtainable pickups and kills stats support in the gameflow

#### Visuals
- added optional shotgun flash sprites
- added optional rendering of pickups on the ground as 3D meshes
- added Lara's braid to each level
- added support for displaying more than 3 pickup sprites
- added more control over when to show health bar and air bar
- added customizable health bar and air bar
- added rounded shadows (instead of the default octagon)
- added adjustable in-game brightness
- added support for HD FMVs
- added fanmade 16:9 menu backgrounds
- added optional fade effects
- added a vsync option
- added contextual arrows to menu options
- changed the Scion in The Great Pyramid from spawning blood when hit to a richochet effect
- fixed thin black lines between polygons
- fixed black screen flashing when navigating the inventory
- fixed detail levels text flashing with any option change
- fixed underwater caustics animating at 2x speed
- fixed inconsistencies in some enemy textures
- fixed the animation of Lara's left arm when the shotgun is equipped
- fixed the following room texture issues:
    - **Gym**: incorrect textures in room 9
    - **Caves**: an incorrect texture in room 6 and missing textures in rooms 1, 10, 14 and 30
    - **City of Vilcabamba**: an incorrect texture in room 26 and a missing texture in room 15
    - **Lost Valley**: incorrect textures in rooms 6 and 9, and missing textures in rooms 9, 25, 26, 27, 51, and 90
    - **Tomb of Qualopec**: missing textures in room 8
    - **St. Francis' Folly**: incorrect textures in rooms 18 and 35
    - **Colosseum**: incorrect Midas textures appearing at the roof and missing textures in rooms 2 and 7
    - **Palace Midas**: incorrect textures in room 31 and missing textures in rooms 2, 5, 9, 13, 30, and 53
    - **The Cistern**: missing textures in rooms 3 and 9
    - **Tomb of Tihocan**: incorrect textures in rooms 75 and 89
    - **City of Khamoon**: incorrect textures in rooms 51 and 64, and a missing texture in room 58
    - **Sanctuary of the Scion**: missing textures in rooms 1, 21, 53, and 54
    - **Natla's Mines**: a missing texture in room 35 and overlapping textures in room 55
    - **Atlantis**: incorrect textures in rooms 5, 18, 43, 50, 52, 58, 78, 85 and 87, and a missing texture in room 27
    - **Atlantis Cutscene**: an incorrect texture in room 16
    - **The Great Pyramid**: incorrect textures in rooms 2, 5, 31, 50, 52, 65 and 66, and missing textures in rooms 21, 25, 26, and 66
    - **Return to Egypt**: a missing texture in room 98
    - **Temple of the Cat**: incorrect textures in rooms 50, 70, 71, 76, 78, 87 and 96, and a missing texture in 75
    - **Atlantean Stronghold**: incorrect textures in rooms 2, 6, 7 and 75, and missing textures in rooms 5, 13, 19 and 74
    - **The Hive**: incorrect textures in room 8, 13 and 18

#### Audio
- added music during the credits
- added an option to turn off sound effect pitching
- added an option to use the PlayStation Uzi sound effects
- added the current music track and timestamp to the savegame so they now persist on load
- added the triggered music tracks to the savegame so one shot tracks don't replay on load
- added detection for animation commands to play SFX on land, water or both
- fixed the sound of collecting a secret killing the music
- fixed audio mixer stopping playing sounds on big explosions
- fixed game audio not muting when game is minimized
- fixed underwater ambient sound effect not playing
- fixed sound effects playing rapidly in sound menu if input held down
- fixed sounds stopping instead of pausing when using the inventory or pausing
- fixed the following music triggers:
    - **Caves**: converted track 9 in room 34 to one shot
    - **Tomb of Qualopec**: converted track 17 in room 25 to one shot
    - **St. Francis' Folly**: converted track 7 in room 18 to one shot
    - **Obelisk of Khamoon**: converted track 3 in room 12 and track 4 in room 32 to one shot
    - **Sanctuary of the Scion**: converted track 10 in room 0 to one shot
    - **Natla's Mines**: converted track 3 in room 86 to one shot
    - **Atlantis**: converted track 8 in room 59 to one shot
    - **The Great Pyramid**: converted track 8 in room 36 to one shot
    - **Return to Egypt**: converted track 19 in room 0, track 14 in room 15, track 15 in room 19, track 16 in room 22, track 6 in room 61, and track 11 in room 93 to one shot
    - **Temple of the Cat**: converted track 12 in room 14, track 7 in room 98, and track 20 in room 100 to one shot
    - **Atlantean Stronghold**: converted track 20 in room 4, track 19 in room 13, track 11 in room 17, track 15 in room 20, and track 12 in room 25 to one shot
    - **The Hive**: converted track 9 in room 8, track 6 in room 18, track 12 in room 30, track 18 in room 31, track 3 in room 32, and track 20 in room 35 to one shot

#### Mods
- added developer console (accessible with `/`, see [COMMANDS.md] for details)
- added ability to adjust Lara's starting health (easy no damage mod)
- added ability to disable healing between levels
- added ability to disable certain item pickups (all medpacks, shotgun, Magnums and/or UZIs)
- added ability to disable main menu demos, FMVs and/or cutscenes
- added external game flow (no longer 2 different .exes for TR1 and TR1UB). Refer to [GAMEFLOW.md](GAMEFLOW.md) for details
- added automatic calculation of secret counts (no longer having to fiddle with the .exe to get correct secret stats)
- added save game crystals game mode (enabled via gameflow)
- added per-level customizable water color (with customizable blue component)
- added per-level customizable fog distance

#### Miscellaneous
- added Linux builds
- added macOS builds
- added .jpeg/.png screenshots
- added an option to pause sound in the inventory screen
- added ability to skip FMVs with the Action key
- added ability to make freshly triggered (runaway) Pierre replace an already existing (runaway) Pierre
- expanded internal game memory limit from 3.5 MB to 16 MB
- expanded moveable limit from 256 to 10240
- expanded maximum textures from 2048 to 8192
- expanded maximum texture pages from 32 to 128
- expanded the number of visible enemies from 8 to 32
- ported audio decoding library to ffmpeg
- ported video decoding library to ffmpeg
- ported image decoding library to ffmpeg
- ported audio output library to SDL
- ported input method to SDL
- changed saves to be put in the saves/ directory
- fixed playing the secret sound in Tomb of Tihocan
- fixed reading user settings not restoring the volume

## Q&A

1. **Is the game fully playable from beginning to the end?**

    Yes. If you encounter a bug, please file a ticket.

2. **Can we get HD textures? Reflections? Other visual updates?**

    Eventually, probably yes, but we'd really appreciate help with these.

3. **Can we get braid in every level? Skyboxes? Flyby cameras? New animations? etc.**

    The difficulty here is that these features often require inserting a
    completely new animation, a textured mesh or a sound file and pretend
    they're always been a part of the original game. Work is underway on an
    injection framework, and the braid is now supported in each level.

4. **Can I play this on Mac, Linux, Android...?**

    Currently supported platforms include Windows, Linux and macOS. In the
    future, it might be possible to run the game on Android as well –
    contributions are welcome!

5. **What's the relation to TR2Main?**

    Initially established as TR1Main in 2021, our project's development paths
    deviated, leading us to recognize the need for a distinct name. As a
    result, we rebranded the project as Tomb1Main. However, to further
    differentiate ourselves, we underwent another rebranding in 2023,
    ultimately adopting the name TR1X. TR2Main is a separate project with its
    own unique trajectory and not directly related to our development efforts.

## License

This project is licensed under the GNU General Public License - see the
[COPYING.md](COPYING.md) file for details.

## Copyright

(c) 2021 Marcin Kurczewski. All rights reserved. Original game is created by
Core Design Ltd. in 1996. Lara Croft and Tomb Raider are trademarks of Square
Enix Ltd. Title image by Kidd Bowyer. Loading screens and high quality images
by goblan_oldnewpixel and Posix.
