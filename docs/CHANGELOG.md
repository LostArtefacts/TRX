## [Unreleased](https://github.com/LostArtefacts/TRX/compare/trx-1.0.3...develop) - ××××-××-××
- added the ability to control whether or not allies are hostile towards Lara via Lua (#3873)
- added the ability to control via Lua which enemies are allies and which are ones that will fight with allies (#3873)
- added support for 3D secret objects, and provided defaults for OG levels in TR2 (#4380)
- added catalog object IDs to Lua
- added support for locked cameras, similar to TR4+ (#2040)
- changed the swinging axe to be defined separately from other pendulums (use object `O_SWINGING_AXE` in catalogs)
- changed ember emitters in TR2 to use the `SFX_LAVA_FOUNTAIN` sample (#4376)
- changed the following trap types to support being reset (#3993)
  - collapsible tiles
  - Damocles swords
  - ember emitters
  - falling ceiling
  - hooks
  - icicles
  - lava wedge
  - pendulums
  - pushblocks (via timed triggers only)
  - spike ceilings
- changed the fonts to no longer use hardcoded character widths
- changed the fonts to use dedicated sprites for accented characters instead of composing them at runtime
- fixed Bacon Lara not always being drawn perfectly in sync with Lara's animation (#4210)
- fixed Lara standing two clicks below `O_FALLING_BLOCK_3` items in TR1 rather than directly on top (#4374)
- fixed the scuba diver's death SFX not playing (#4386)
- fixed skybox faces with transparent pixels always rendering in front of all other faces (#4351, regression from 1.0)
- fixed unbound inputs not being saved between game launches (#4360, regression from TR1X 4.14/TR2X 1.4)
- fixed Lara drawing a flare when the draw weapons input is pressed, and she already has an active flare but no weapons (#4361, regression from TR2X 1.4)
- fixed Lara automatically being given TR2 weapons in TR1 NG+ when playing the OG levels (#4365, regression from 1.0)
- fixed Lara's pistol holster meshes appearing in TR1 NG+ in place of her Uzi holster meshes (#4368, regression from 1.0)

## [1.0.3](https://github.com/LostArtefacts/TRX/compare/trx-1.0.2...trx-1.0.3) - 2025-11-27
- fixed the conveyer belt fuse in Natla's Mines not appearing after using the nearby switch (#4349, regression from 1.0)

## [1.0.2](https://github.com/LostArtefacts/TRX/compare/trx-1.0.1...trx-1.0.2) - 2025-11-26
- fixed Lara being unable to interact with keyholes after picking up an item if animated interactions are enabled (#4342, regression from 1.0)

## [1.0.1](https://github.com/LostArtefacts/TRX/compare/trx-1.0...trx-1.0.1) - 2025-11-25
- changed default master volume to 80% in TR2 to match TR1 (#4337)
- fixed 2D sprites not appearing in the UI (#4338, regression since 1.0)

## [1.0](https://github.com/LostArtefacts/TRX/compare/76109a8855da99f3304ca4d9a3f5882dada2dd40...trx-1.0) - 2025-11-23
Showcase: https://youtu.be/vVU9vbUXTXc
**Common**:
- added LUA scripting engine
    Supports basic events, item interactions, teleporting and much more.
    See [the documentation](../06-lua) for details.
- added a game flow option for cold water in custom levels, similar to TR3 (#4021)
- added a splash effect when Lara jumps in wading depth water, similar to TR3+ (#3975)
- added bounding box debugging (`/debug 1` or `/set debug-cuboids 1`)
- added support for object, music, sound, flip effects, Lara state, and Lara animation slots overrides through CSV catalogs  
    Lets builders link hardcoded logic to slots of their choice, allowing object sharing between games (for example, use TR1 bats in TR2).  
    This feature is experimental — some objects may not behave correctly. Please report any bugs encountered! 🩷  
    See [the documentation](../07-CATALOGS.md) for details.
- added `enable_debug_camera` setting that shows camera position in realtime (reachable via `/debug` and `/set`)
- added the ability to fast-forward through cutscenes with the right button (+5 s) or with slow+right (+1 s)
- added support for dark theme on Windows
- added support for triangular geometry
- added support for additive blending in textures
- improved bilinear filtering for smoother edge blending when multiple objects overlap in depth
- improved rendering of statics and items in overlapping rooms (#2005)
- improved ricochets placement
    - fixed dart and disc ricochets being placed mid-air (#4063)
    - fixed ricochets not showing on slopes
- improved bar setting UIs in various ways
    - added two new options: "Show bars" on/off, and "Flash bars" on/off
    - changed the bars options to be placed in its own tab
    - changed the appearance labels to better align with expectations (#4025)
    - removed the look modes for every bar (except enemy bars that retain the "boss only" setting)
    - fixed health bar flicker on medi packs when cycling the inventory ring (#4211, regression from TR1X 4.14 / TR2X 1.4)
- changed the `/debug` command to accept optional option name argument (for example: `/debug pos 1`)
- changed dart emitters and disc emitters to have separate slots (so with catalogs, both can be used in the same level simultaneously)
- changed the debug position UI to no longer be hidden in photo mode
- changed the unrestricted look mode option to include Lara being able to look freely while shooting an enemy (#4090)
- changed the `ambient_tracks` property to be only available on the root level
- changed music triggers that match the level's default ambient track to automatically be treated as ambient if omitted from `ambient_tracks` (#4181)
- changed the `-q`/`--quiet` argument to no longer silence warnings
- changed the `Remember Guns between Levels` option to also apply to whether or not Lara starts with those guns equipped
- changed the FOV formula to be consistent between games
    - changed the FOV default increment from 10 to 5 (#4026)
    - removed "Vertical FOV" option
    - removed "Use PS1 FOV" option
- fixed missing footstep sound effects when Lara climbs off a ladder and when she finishes a handstand (#4030)
- fixed a crash in custom levels if a flip effect that expects to act on an item is used in a regular trigger (#4085)
- fixed a crash if trying to kill an enemy by name but there is no naming definition for that object
- fixed photo mode camera clipping through overlapping rooms (#1674)
- fixed bogus warnings about resume info in logs when playing cutscenes and in the title level
- fixed title bar size being too small on HiDPI screens on Windows platform (#2837)
- fixed statics and items not getting rendered when all portals leading to them are offscreen (#2005)
- fixed Lara's arms getting stuck in the M16 gun firing animation while she dies (#4130)
- fixed Lara jittering in the QWOP state
- fixed doors and trapdoors not interpolating when using the door cheat
- fixed credit images and loading images showing black screen if the file is missing (#4325)
- fixed caustics not affecting underwater plant sprites (#4317)

**TR1**:
- added a new easter egg command
- added support for flares (for OG levels, use `/give flare`) (#4121)
- added support for TR2 weapons (for OG levels, use `/give moreguns`)
- added support for custom levels to use Lara's extra animations from TR2
- added new hidden settings (available via LUA and the `/set` command):
    - `flow.lockout_option_ring`
    - `flow.load_save_disabled`
    - `flow.play_any_level`
    - `flow.cheat_keys`
- added the ability for the sound system to use Lara's position instead of camera's position (#1438) (Sound Options → Misc → Microphone near Lara)
- added an option to use TR2-style inventory ring backgrounds in custom levels (Graphic Options → UI → Inventory background) (#4264)
- added an option to use TR2-style statistics dialog backgrounds in custom levels (Graphic Options → UI → Stats background) (#4264)
- improved the positions of some 3D pickup items, such as the scion that Pierre drops
- improved the quality of the PS1 Uzi SFX (#4024)
- changed the following game flow options to become hidden settings (available via LUA and the `/set` command):
    - `flow.demo_delay`
    - `gameplay.enable_killer_pushblocks`
- changed Select Level and Story So Far features placement to the New Game menu
- changed the input buffering option to separately tackle F-keys and Inventory (Gameplay → Input → Buffering (F-keys), Gameplay → Input → Buffering (Inventory))
- changed exploded meshes to trigger a splash effect when they hit water, similar to TR2
- changed LOS algorithm to TR2+ implementation
- changed save crystal collision to make them easier to activate
- changed cutscene data (e.g. `cut1.phd`, as opposed to in-game cinematics) to match TR2 format, where Lara (as `O_LARA`) must be defined as an item in the level file
- changed the Remove shotguns, Remove Uzis and Remove Magnus into a single "Remove extra guns" option
- changed toggling Lara's braid in-game to swap out her head and torso meshes appropriately without the need to reload the level (#2399)
- removed the `Enhanced shotgun targeting` option in favour of using the common weapon lock mode (Gameplay → Controls → Weapon lock mode)
- fixed Lara being able to push blocks through toggle opacity 1 portals (#4129)
- fixed Lara drifting during the Atlantis cutscene while the camera focuses in on Natla (#4153)
- fixed Lara retaining her hit animation if nudged by an enemy at the same time as starting a special animation such as picking up a scion (#4212)
- fixed Lara being drawn if the explosion cheat has been used and Bacon Lara is active (#4148)
- fixed ambient music not playing in demo levels (#4046, regression from 4.13)
- fixed caustics stopping after spending roughly 12 minutes in a level (#4109, regression from 4.10)
- fixed legacy UB crashing the game (#4113, regression from 4.12)
- fixed select level feature to also be available to games started with `/play`
- fixed select level feature slot status not updated on save
- fixed "Story So Far..." showing loading screens
- fixed matrix stack overflow crash when moving through overlapping or dome portals (#2685)
- fixed the gun-draw SFX playing when holstering the shotgun (#3755)
- fixed Lara's braid remaining reflective if the fly cheat is used to resurrect her on the Midas hand
- fixed invulnerability cheat not getting disabled during the demos (regression from 4.13)
- fixed crash when loading OG saves made in City of Khamoon, while the "Restore PS1 enemies" option is on (#4217, regression from 2.16)
- fixed the jump lock mode UI option remaining visible when responsive jumping is disabled (#4027, regression from 4.13)
- fixed a slight misalignment in the PDA in its open state (#4247)
- fixed Lara being unable to exit the water in (for example) room 41 of Return to Egypt (#4315, regression from 4.12)

**TR2**:
- added loading screens (Gameplay Options → General → Loading screens) (#1620)  
    Tomb Raider II 3×2 upscales done by Arsunt.  
    Tomb Raider II: The Golden Mask images done by Lito Perezito.
- added Restart Level option when Lara dies (#1555)
- added Play Previous Levels feature (available in the New Game screen)
- added Story So Far… feature (available in the New Game screen)
- added game mode selection to the Play Any Level feature
- added support for custom levels to use Lara's extra animations from TR1
- added an option to use TR1-style inventory ring backgrounds (Graphic Options → UI → Inventory background) (#3923)
- added an option to use TR1-style statistics dialog backgrounds (Graphic Options → UI → Stats background) (#3923)
- added extended statistics support (#2578)
    - added pickup count and death count support in the stats screen (Graphic Options → UI → Statistics details)
    - added max pickup, secret and kills support (Graphic Options → UI → Statistics details)
    - added deaths counter support (Gameplay Options → General → Count Lara's death)
    - added unobtainable secrets, pickups and kills stats support in the gameflow
- added an option to disable final statistics (Gameplay options → General → Final statistics screen)
- added an option to disable all medipacks (Gameplay options → Mods → Remove medipacks)
- added an option to disable all guns except Pistols (Gameplay options → Mods → Remove extra guns)
- added an option for pickup aids, which will show an intermittent twinkle when Lara is nearby pickup items (Graphic Options → Visuals → Pickup aids) (#4057)
- added an option for animated interactions with pickups and switches (Gameplay → Controls → Animated interactions) (#4067)
- added an option to change max savegame slot count (Gameplay → General → Number of save slots)
- added an option to turn off Inventory input buffering (Gameplay → Input → Buffering (Inventory))
- added an option to turn on TR1-style F-keys input buffering (Gameplay → Input → Buffering (F-keys))
- added an option to draw Shotgun flashes (Graphic Options → Visuals → Shotgun flash)
- added support for TR1-like secret triggers (#2047)
- added support to disable wading, like TR1 (Gameplay → Controls → Wading) (hidden by default)
- added support to disable responsive running jumps, like TR1 (Gameplay → Controls → Responsive jumping)
- added support to disable responsive swim cancel, like TR1 (Gameplay → Controls → Responsive swim cancel)
- added support for game-flow defined enemy item drops, similar to OG TR1 levels; regular level-defined drops will continue to work normally
- improved the quality of the PS1 barefoot SFX (#4024)
- changed the following game flow options to become hidden settings (available via LUA and the `/set` command):
    - `flow.lockout_option_ring`
    - `flow.load_save_disabled`
    - `flow.play_any_level`
    - `flow.demo_delay`
    - `flow.cheat_keys`
    - `gameplay.enable_killer_pushblocks`
- changed the Pause key to no longer work when Lara's dead (similar to TR1)
- changed sprites to respect the water tint if placed underwater
- removed the following game flow options:
    - `cmd_init`
    - `cmd_title`
    - `cmd_death_in_demo`
    - `cmd_death_in_game`
    - `cmd_demo_end`
    - `cmd_demo_interrupt`
    - `single_level`
    - `is_demo_version`
- fixed the new game modes dialog requiring the Action key to show up (TR2 dialogs don't need this.)
- fixed Lara's pistols not being removed from holsters when she equips during a cutscene (#4136)
- fixed potential softlocks in custom levels with enemies who have end-level flip effects but the player uses the grenade launcher to kill them (#4261)
- fixed Lara not having holstered pistols after she changes costumes in the Diving Area cutscene (#4142)
- fixed ambient music not playing in demo levels (#4046, regression from 1.3)
- fixed twists not adhering to original game movement (#4078, regression from 1.4)
- fixed legacy saves in Opera House and Vegas crashing the game (#4103, regression from 1.5)
- fixed caustics stopping after spending roughly 12 minutes in a level (#4109, regression from 1.4)
- fixed Lara being able to push blocks through toggle opacity 1 portals (#4129, regression from 1.5)
- fixed pistols disappearing from Lara's holsters in the cutscene following The Great Wall (#4145, regression from 0.9)
- fixed Lara's thigh meshes defaulting if entering the fly cheat while holding a flare and she doesn't currently have holstered weapons (#4143, regression from 1.3)
- fixed wrong lighting of exploded body parts
- fixed weird clipping when moving through overlapping or dome portals (#2685)
- fixed Lara reloading the harpoon gun if she draws the weapon and does not have any harpoons (#4259)
- fixed Lara holding on to the grenade launcher for too long when undrawing it (#3474)
- fixed the holster SFX playing when drawing rifle type weapons (#3755)
- fixed missing SFX in the harpoon drawing and undrawing animations (#3755)
- fixed invulnerability cheat not getting disabled during the demos (regression from 1.3)
- fixed disable targeting allies option not working (#4184, regression from 1.5)
- fixed Lara losing forward momentum on springboards when the wall glitch mode option is set to `Fixed` (#4187, regression from 1.2)
- fixed the M16 accuracy option not taking effect until restarting the game (#4227, regression from 0.3)
- fixed incorrect keys object orientation in the inventory ring (#4239, regression from 0.3)
- fixed underwater hum when Microphone near Lara option is on (#2188)
- fixed 3D pickups not rendering if the associated sprite is not present in the level file (#4275, regression from 0.6)
