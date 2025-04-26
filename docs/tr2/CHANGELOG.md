## [Unreleased](https://github.com/LostArtefacts/TRX/compare/tr2-1.0.2...develop) - ××××-××-××

## [1.0.2](https://github.com/LostArtefacts/TRX/compare/tr2-1.0.1...tr2-1.0.2) - 2025-04-26
- changed The Golden Mask strings to default to the OG strings file for the main tables (#2847)
- fixed Lara voiding if she stops on a tile with a closing door, and the door isn't on a portal (#2848)
- fixed guns carried by enemies not being converted to ammo if Lara has picked up the same gun elsewhere in the same level (#2856)
- fixed button mashing triggering load instead of save on a specific passport animation frame (#2863, regression from 1.0)
- fixed guns carried by enemies not being converted to ammo if Lara starts the level with the gun and the game has later been reloaded (#2850, regression from 1.0)
- fixed 1920x1080 screenshots in 16:9 aspect mode being saved as 1919x1080 (#2845, regression from 0.8)
- fixed clicks in audio sounds (#2846, regression from 0.2)

## [1.0.1](https://github.com/LostArtefacts/TRX/compare/tr2-1.0...tr2-1.0.1) - 2025-04-24
- added an option to wraparound when scrolling UI dialogs, such as save/load (#2834)
- changed save to take priority over load when both inputs are held on the same frame, in line with OG (#2833)
- fixed the selected keyboard/controller layout not being saved (#2830, regression from 1.0)
- fixed toggling the PSX FOV option not having an immediate effect (#2831, regression from 1.0)
- fixed changing the aspect ratio not updating the current background image (#2832, regression from 1.0)
- improved graphic settings dialog sizing (#2841)

## [1.0](https://github.com/LostArtefacts/TRX/compare/tr2-0.10...tr2-1.0) - 2025-04-23
- added support for The Golden Mask (#1621)
- added ability to turn off legal screen and FMVs (#2740)
- added ability to turn off ingame cutscenes (#2127)
- added HD images from TR2Main (with Arsunt's consent)
- added sunglasses for graphic options (#1615)
- added control over the fog distances for players and level builders (#1622)
- added control over the water color for players and level builders [see the reference](/docs/GAME_FLOW.md#water-color-table) (#1619)
- added an installer for Windows (#2681)
- added the bonus level game flow type, which allows for levels to be unlocked if all main game secrets are found (#2668)
- added the ability for custom levels to have up to two of each secret type per level (#2674)
- added BSON savegame support, removing the limits imposed by the OG 8KB file size, so allowing for storing more data and offering improved feature support (legacy save files can still be read, similar to TR1) (#2662)
- added NG+, Japanese, and Japanese NG+ game mode options to the New Game page in the passport (#2731)
- added the ability for spike walls to be reset (antitriggered)
- added the current music track and timestamp to the savegame so they now persist on load (#2579)
- added waterfalls to the savegame so that they now persist on load (#2686)
- added support for aspect ratio-specific images (#1840)
- added a guard to ensure the game always starts on a visible screen even after unplugging displays (#2819)
- changed savegame files to be stored in the `saves` directory (#2087)
- changed the default fog distance to 22 tiles cutting off at 30 tiles to match TR1X (#1622)
- changed the number of static mesh slots from 50 to 256 (#2734)
- changed the maximum number of items (moveables) per level from 256 to 10240 (1024 remains the limit for triggered items) (#1794)
- changed the maximum number of visible enemies from 5 to 32 (#1624)
- changed the maximum number of effects (flames, embers, exploding parts etc) from 100 to 1000 (#1581)
- changed default pitch of the save/load dialog ingame - it's now higher.
- fixed the inability to completely mute the sounds, even at sound volume 0 (#2722)
- fixed the final two levels not allowing for secrets to be counted in the statistics (#1582)
- fixed assault course best times not being retained between game relaunches (#1578)
- fixed flares disappearing on the ground when the z buffer is enabled (#1595)
- fixed Lara's holsters being empty if a game flow level removes all weapons but also re-adds the pistols (#2677)
- fixed the console opening when remapping its key (#2641)
- fixed the boat when it explodes after crossing mines, where Lara's hips would appear rather than exploded boat parts (#1605)
- fixed Lara's hips appearing on Bartoli in the Temple of Xian cutscene (#2558)
- fixed collision issues with drawbridges, trapdoors, and bridges when stacked over each other, over slopes, and near the ground (#2752)
- fixed the lift to work in any cardinal direction in custom levels, not just South (#2100)
- fixed the springboard not responding correctly when Lara drives across one on a skidoo (#1903)
- fixed the drawbridge producing dynamic light when open (#2294)
- fixed the scale of several pickup models in The Golden Mask (#2652)
- fixed the shark in The Cold War not making any sounds when biting Lara (#2678)
- fixed the bird monster not having a shadow (#2060)
- fixed the in-game cinematic camera at times yielding invalid positions (and hence views) in custom levels (#2754)
- fixed a softlock in Temple of Xian if the main chamber key is missed (#2042)
- fixed a potential softlock in Floating Islands if returning towards the level start from the gold secret (#2590)
- fixed a potential softlock in Nightmare in Vegas where the bird monster could remain inactive, or the flip map not set (#1851)
- fixed invalid portals in The Deck between rooms 17 and 104, which could result in Lara seeing enemies in disconnected rooms (#2393)
- fixed pushblocks being rotated when Lara grabs them, most noticeable if asymmetric textures have been used (#2776)
- fixed the boat briefly having an underwater hue when Lara first climbs on (#2787)
- fixed destroyed gondolas appearing embedded in the ground after loading a save (#1612)
- fixed a crash in custom levels with large rooms (#2749)
- fixed the viewport not always in sync with the window (#2820)
- fixed inability to move the window to another screen (#2820)
- fixed flares flipped to the right when thrown (regression from 0.10)
- fixed the camera going out of bounds in 60fps near specific invalid floor data (known as no-space) (#2764, regression from 0.10)
- fixed sprites rendering black if no shade value is assigned in the level (#2701, regression from 0.8)
- fixed some 3D pickup items rendering black in software mode (#2792, regression from 0.10)
- fixed Lara at times ending up in incorrect rooms when using the teleport cheat (#2486, regression from 0.3)
- fixed the `/pos` console command reporting the base room number when Lara is actually in a flipped room (#2487, regression from 0.3)
- fixed a crash if an image was missing
- fixed a crash on level load if an animation has no frames (#2746, regression from 0.8)
- fixed flares missing the flicker effect in 60 FPS (#2806, regression from 0.10)
- improved performance when moving the window around
- improved pause exit dialog - it can now be canceled with escape
- removed the need to specify in the game flow levels that have no secrets (secrets will be automatically counted) (#1582)
- removed the hard-coded end-level behaviour of the bird guardian for custom levels (#1583)
- removed the FPS and aspect mode options from the config tool (now available in-game in the graphics options)

## [0.10](https://github.com/LostArtefacts/TRX/compare/tr2-0.9.2...tr2-0.10) - 2025-03-18
- added support for 60 FPS rendering
- added support for more accented characters (#2356)
- added quadrilateral interpolation (#354)
- added a `/cheats` console command
- added a `/wireframe` console command (#2500)
- added a `/fps` console command
- added `/flood` and `/drain` console commands
- added support for `-l`/`--level` argument to play a single level
- added support for `-s`/`--save` argument to immediately start a saved game
- added the ability to specify per-level SFX files rather than enforcing the default (main.sfx) on all levels (#2615)
- added the camera shutter sound to cutscenes for photo mode (#2280)
- added Italian localization to the config tool
- changed injections to a new file format with a smaller footprint, improved applicability tests and similar feature support as TR1 (#1967)
- changed the `/pos` command to show `Demo` and `Cutscene` instead of `Level` when relevant
- changed the `/pos` command to show demo and cutscene numbers starting at 1, in line with `/play`
- changed the `/play` and `/pos` commands to always treat the gym level as the level 0 – even if it's not included
- fixed smashed windows blocking enemy pathing after loading a save (#2535)
- fixed several instances of the camera going out of bounds (#1034)
- fixed Lara getting stuck in a T-pose after jumping/falling and then dying before reaching fast fall speed (#2575)
- fixed missing enemy sound effects in the underwater levels (#2293)
- fixed seaweed collision in Living Quarters preventing Lara from climbing out of the water in room 15 (#2197)
- fixed the scale and rotation of several pickup models, such as the offshore key cards and Barkhang prayer wheels (#1832, #1894)
- fixed a rare issue whereby Lara would be unable to move after disposing a flare (#2545, regression from 0.9)
- fixed flare pickups only adding one flare to Lara's inventory rather than six (#2551, regression from 0.9)
- fixed several issues with pushblocks (#2036/#2193)
    - fixed an invisible wall above stacked pushblocks if near a ceiling portal
    - fixed floor height issues with pushblocks poised to fall in various scenarios
    - fixed being unable to stack multiple pushblocks over multiple rooms
    - fixed falling pushblocks using the enemy grunt sound effect
- fixed play any level causing the game to hang when no gym level is present (#2560, regression from 0.9)
- fixed extremely large item quantities crashing the game (#2497, regression from 0.3)
- fixed missing new game text in the passport when play any level is enabled (#2563, regression from 0.9)
- fixed the play any level dialog not showing in the gym passport (#2564, regression from 0.9)
- fixed losing the NG+ flag when loading a save that has it set (#2566, regression from 0.9.2)
- fixed the ammo counter not showing in demos if NG+ is set (#2574, regression from 0.9)
- fixed being able to play with Lara invisible after using the explosion cheat then the fly cheat (#2584, regression from 0.9)
- fixed the `/pos` command not showing demo and cutscene titles
- fixed the distance travelled stat displaying the wrong value when over 1000m (#2659)
- improved camera mode navigation:
    - improved support for pivoting
    - improved roll support
    - expanded world bounding box by 5 tiles in each direction
    - added support for 60 FPS
- removed the hardcoded title screen image path, replacing it with a game flow file property instead

## [0.9.2](https://github.com/LostArtefacts/TRX/compare/tr2-0.9.1...tr2-0.9.2) - 2025-02-19
- fixed secret rewards not handed out after loading a save (#2528, regression from 0.8)
- fixed music not working on certain Linux setups (#2504, regression from 0.2)

## [0.9.1](https://github.com/LostArtefacts/TRX/compare/tr2-0.9...tr2-0.9.1) - 2025-02-15
- changed passport to be more responsive to player inputs (#1328)
- fixed resolving paths (especially to music files) on case-sensitive filesystems (#1934, #2504)
- fixed loading a game crashing on Linux (#2508, regression from 0.9)
- improved memory usage by shedding ca. 100-110 MB on average

## [0.9](https://github.com/LostArtefacts/TRX/compare/tr2-0.8...tr2-0.9) - 2025-02-14
- added Linux builds and toolchain (#1598)
- added macOS builds (for both Apple Silicon and Intel) (#2226)
- added pause dialog (#1638)
- added a photo mode feature (#2277)
- added fade-out effect to the demos
- added the ability to hold left/right to move through menus more quickly (#2298)
- added the ability to disable exit fade effects alone (#2348)
- added a fade-out effect when completing Lara's Home
- added support for animated sprites (#2401)
- added a `/cut` (alias: `/cutscene`) console command for playing cutscenes
- added a `/gym` (alias: `/home`) console command for playing Lara's Home
- added a `/music` console command that plays a specific music track
- added a console log when using the `/demo` command
- changed the object texture limit from 2048 to unlimited (within game's overall memory cap) (#1795)
- changed the sprite texture limit from 512 to unlimited (within game's overall memory cap) (#1795)
- changed the texture page limit from 32 to 128 (#1796)
- changed default input bindings to let the photo mode binding be compatible with TR1X:
    | Key                           | Old binding | New binding  |
    | ----------------------------- | ----------- | ------------ |
    | Decrease resolution           | Shift+F1    | Shift+F11    |
    | Increase resolution           | F1          | F11          |
    | Decrease internal screen size | Shift+F2    | Shift+F10    |
    | Increase internal screen size | F2          | F10          |
    | Toggle photo mode             | ---         | F1           |
    | Toggle photo mode UI          | ---         | H            |
- changed the `/kill` command with no arguments to look for enemies within 5 tiles (#2297)
- changed the game data to use a separate strings file for text information, removing it from the game flow file
- changed dynamic lighting for gun flashes and explosions to be optional (#2357)
- fixed scale of secret icons on level complete summary (#1631)
- fixed showing inventory ring up/down arrows when uncalled for (#2225)
- fixed Lara never stepping backwards off a step using her right foot (#1602)
- fixed blood spawning on Lara from gunshots using incorrect positioning data (#2253)
- fixed ghost meshes appearing near statics in custom levels (#2310)
- fixed potential memory corruption when reading a custom level with more than 512 sprite textures (#2338)
- fixed the teleporting command sometimes putting Lara in invalid flipmap rooms (#2370)
- fixed teleporting to an item on a ledge sometimes pushing Lara to the room below (#2372)
- fixed the game crashing if a cinematic is triggered but the level contains no cinematic frames (#2413)
- fixed being unable to load a level that contains no sound effect data (#2460)
- fixed issues with sound effects not playing or looping forever in some cases when many other effects are playing (#2494)
- fixed Lara activating triggers one frame too early (#2205, regression from 0.7)
- fixed savegame incompatibility with OG (#2271, regression from 0.8)
- fixed stopwatch showing wrong UI in some circumstances (#2221, regression from 0.8)
- fixed excessive braid movement when dead in windy rooms (#2265, regression from 0.8)
- fixed item counter shown even for a single medipack (#2222, regression from 0.3)
- fixed item counter always hidden in NG+, even for keys (#2223, regression from 0.3)
- fixed the passport object not being selected when exiting to title (#2192, regression from 0.8)
- fixed the upside-down camera fix to no longer limit Lara's vision (#2276, regression from 0.8)
- fixed /kill command freezing the game under rare circumstances (#2297, regression from 0.3)
- fixed wireframe mode discarding transparent pixels (#2315, regression from 0.7)
- fixed sprite pickups not being paused in the pause/inventory screen (#2319, regression from 0.6)
- fixed Skidoo snow wake effects at slow speeds (#2324, regression from 0.6)
- fixed software renderer skybox occlusion issues (#2343, regression from 0.7)
- fixed gunflare from bandits in Tibetan levels spawning too far from their guns (#2365, regression from 0.8)
- fixed guns sometimes appearing in Lara's hands when entering the fly cheat while undrawing weapons (#2376, regression from 0.3)
- fixed the `/play` console command not resetting Lara's inventory (#2267, regression from 0.3)
- fixed flashing text when trying to exit passport while Lara is dead and an action is required (#2263)
- improved rendering to achieve a slight performance boost in big rooms (#2325)
- improved wireframe mode appearance around screen edges

## [0.8](https://github.com/LostArtefacts/TRX/compare/tr2-0.8...tr2-0.8) - 2025-01-01
- completed decompilation efforts – TR2X.dll is gone, Tomb2.exe no longer needed (#1694)
- added the ability to set user-defined FOV (no UI for it yet) (#2177)
- added the ability to turn FMVs off (#2110)
- added an option to use PS1 contrast levels, available under F8 (#1646)
- added an option to use TR3+ side steps (#2111)
- added an option to allow disabling the developer console (#2063)
- added an optional fix for the QWOP glitch (#2122)
- added an optional fix for the step glitch, where Lara can be pushed into walls (#2124)
- added an optional fix for drawing a free flare during the underwater pickup animation (#2123)
- added an optional fix for Lara drifting into walls when collecting underwater items (#2096)
- added an option to control how music is played while underwater (#1937)
- added an optional demo number argument to the `/demo` command
- added an option to set the bar scaling (no UI for it yet) (#1636)
- added an option to set the text scaling (no UI for it yet) (#1636)
- changed demo to be interrupted only by esc or action keys
- changed the turbo cheat to also affect ingame timer (#2167)
- fixed health bar and air bar scaling (#2149)
- fixed text being stretched on non-4:3 aspect ratios (#2012)
- fixed Lara prioritising throwing a spent flare while mid-air, so to avoid missing ledge grabs (#1989)
- fixed Lara at times not being able to jump immediately after going from her walking to running animation (#1587)
- fixed bubbles spawning from flares if Lara is in shallow water (#1590)
- fixed flare sound effects not always playing when Lara is in shallow water (#1590)
- fixed looking forward too far causing an upside down camera frame (#1594)
- fixed music not playing if triggered while the game is muted, but the volume is then increased (#2170)
- fixed game FOV being interpreted as horizontal (#2002)
- fixed the inventory up arrow at times overlapping the health bar (#2180)
- fixed software renderer not applying underwater tint (#2066, regression from 0.7)
- fixed some enemies not looking at Lara (#2080, regression from 0.6)
- fixed the camera getting stuck at the start of Home Sweet Home (#2129, regression from 0.7)
- fixed assault course timer not paused in the inventory (#2153, regression from 0.6)
- fixed Lara spawning air bubbles above water surfaces during the fly cheat (#2115, regression from 0.3)
- fixed demos playing too eagerly (#2068, regression from 0.3)
- fixed Lara sometimes being unable to use switches (#2184, regression from 0.6)
- fixed Lara interacting with airlock switches in unexpected ways (#2186, regression from 0.6)
- fixed input controller remaps not being saved across game relaunches (#2422, regression from 0.6)
- improved the animation of Lara's braid (#2094)

## [0.7.1](https://github.com/LostArtefacts/TRX/compare/tr2-0.7...tr2-0.7.1) - 2024-12-17
- fixed a crash when selecting the sound option (#2057, regression from 0.6)

## [0.7](https://github.com/LostArtefacts/TRX/compare/tr2-0.6...tr2-0.7) - 2024-12-16
- switched to OpenGL rendering (#1844)
    - improved support for non-4:3 aspect ratios (#1647)
    - changed fullscreen behavior to use windowed desktop mode (#1643)
    - added an option for 1-2-3-4× pixel upscaling (available under the F1/Shift-F1 key)
    - added the ability to use the window border option at all times (available under the F2/Shift-F2 key)
    - added the ability to toggle between the software/hardware renderer at runtime (available under the F12 key)
    - added fade effects to the hardware renderer (#1623)
    - added an informative text when toggling various rendering options at runtime (#1873)
    - added a wireframe mode (available with `/set` console command and with Shift+F7)
    - changed the software renderer to use the picture's palette for the background pictures
    - changed the hardware renderer to always use 16-bit textures (#1558)
    - fixed texture corruption after FMVs play (#1562)
    - fixed black borders in windowed mode (#1645)
    - fixed "Failed to create device" when toggling fullscreen (#1842)
    - fixed distant rooms sometimes not appearing, causing the skybox to be visible when it shouldn't (#2000)
    - fixed rendering problems on certain Intel GPUs (#1574)
- replaced the Windows Registry configuration with .json files
    - removed setup dialog support (using `Tomb2.exe -setup` will have no effect on TR2X)
    - removed unused detail level option
    - removed triple buffering option
    - removed dither option
- added support for custom levels to enforce values for any config setting (#1846)
- added an option to fix inventory item usage duplication (#1586)
- added optional automatic key/puzzle inventory item pre-selection (#1884)
- added a search feature to the config tool (#1889)
- added an option to fix rotation on some pickup items to better suit 3D pickup mode (#1613)
- added background for the final game stats (#1584)
- added the ability to turn fade effects on/off (#1623)
- fixed a crash when trying to draw too many rooms at once (#1998)
- fixed Lara getting stuck in her hit animation if she is hit while mounting the boat or skidoo (#1606)
- fixed pistols appearing in Lara's hands when entering the fly cheat during certain animations (#1874)
- fixed wrongly calculated trapdoor size that could affect custom levels (#1904)
- fixed one of the collapsible tiles in Opera House room 184 not triggering (#1902)
- fixed being unable to use the drawbridge key in Tibetan Foothills after the flipmap (#1744)
- fixed missing triggers and ladder in Catacombs of the Talion after the flipmap (#1960)
- fixed incorrect music trigger types at the beginning of Catacombs of the Talion (#1962)
- fixed missing death tiles in Temple of Xian room 91 (#1920)
- fixed the detonator key and gong hammer not activating their target items when manually selected from the inventory (#1887)
- fixed wrongly positioned doors in Ice Palace and Floating Islands, which caused invisible walls (#1963)
- fixed picking up the Gong Hammer in Ice Palace sometimes not opening the nearby door (#1716)
- fixed room 98 in Wreck of the Maria Doria not having water (#1939)
- fixed a potential crash if Lara is on the skidoo in a room with many other adjoining rooms (#1987)
- fixed a softlock in Home Sweet Home if the final cutscene is triggered while Lara is on water surface (#1701)
- fixed Lara's left arm becoming stuck if a flare is drawn just before the final cutscene in Home Sweet Home (#1992)
- fixed resizing game window on the stats dialog cloning the UI elements, eventually crashing the game (#1999)
- fixed exiting the game with Alt+F4 not immediately working in cutscenes
- fixed game freezing when starting demo/credits/inventory offscreen
- fixed problems when trying to launch the game with High DPI mode enabled (#1845)
- fixed clock drift accumulating with time, causing audio desync in cutscenes (#1935, regression from 0.6)
- fixed controllers dialog missing background in the software renderer mode (#1978, regression from 0.6)
- fixed a crash relating to audio decoding (#1895, regression from 0.2)
- fixed depth problems when drawing certain rooms (#1853, regression from 0.6)
- fixed being unable to go from surface swimming to underwater swimming without first stopping (#1863, regression from 0.6)
- fixed Lara continuing to walk after being killed if in that animation (#1880, regression from 0.1)
- fixed some music tracks looping while Lara remained on the same trigger tile (#1899, regression from 0.2)
- fixed some music tracks not playing if they were the last played track and the level had no ambience (#1899, regression from 0.2)
- fixed broken final stats screen in software rendering mode (#1915, regression from 0.6)
- fixed screenshots not capturing level stats (#1925, regression from 0.6)
- fixed screenshots sometimes crashing in the windowed mode (regression from 0.6)
- fixed creatures being able to swim/fly above the ceiling up to one tile (#1936, regression from 0.1)
- fixed the `/kill all` command reporting an incorrect count in some levels (#1995, regression from 0.3)
- removed unused detail level option

## [0.6](https://github.com/LostArtefacts/TRX/compare/tr2-0.5...tr2-0.6) - 2024-11-06
- added a fly cheat key (#1642)
- added an items cheat key (#1641)
- added a level skip cheat key (#1640)
- added a turbo cheat (#1639)
- added the ability to skip end credits with the action and escape keys (#1800)
- added the ability to skip FMVs with the action key (#1650)
- added the ability to hold forward/back to move through menus more quickly (#1644)
- added optional rendering of pickups in the UI as 3D meshes (#1633)
- added optional rendering of pickups on the ground as 3D meshes (#1634)
- added a special target, "pickup", to item-based console commands
- changed the inputs backend from DirectX to SDL (#1695)
    - improved controller support to match TR1X
    - changed the number of custom layouts to 3
    - changed default key bindings according to the following table:
        | Key                           | Old binding | New binding  | Reason
        | ----------------------------- | ----------- | ------------ | -----
        | Flare                         | Comma (,)   | Period (.)   | To maintain forward compatibility with TR3
        | Screenshot                    | S           | Print Screen | To maintain compatibility with TR1X
        | Toggle bilinear filter        | F8          | F3           | To maintain compatibility with TR1X
        | Toggle perspective filter     | Shift+F8    | F4           | To maintain compatibility with TR1X
        | Toggle z-buffer               | F7          | F7           | Likely to be permanently enabled in the future
        | Toggle triple buffering       | Shift+F7    | **Removed**  | Obscure setting, will be either removed or available via the ingame UI at some point
        | Toggle dither                 | F11         | **Removed**  | Obscure setting, will be either removed or available via the ingame UI at some point
        | Toggle fullscreen             | F12         | Alt-Enter    | To maintain compatibility with TR1X
        | Toggle rendering mode         | Shift+F12   | F12          | No more conflict to require Shift
        | Decrease resolution           | F1          | Shift+F1     | F3 and F4 are already taken
        | Increase resolution           | F2          | F1           | F3 and F4 are already taken
        | Decrease internal screen size | F3          | Shift+F2     | F3 and F4 are already taken
        | Increase internal screen size | F4          | F2           | F3 and F4 are already taken
    - removed "falling through" to the default layout, with the exception of keyboard arrows (matching TR1X behavior)
    - removed hardcoded Shift+F7 key binding for toggling triple buffering
    - removed hardcoded `0` key binding for flares
    - removed hardcoded cooldown of 15 frames for medipacks
- changed text backend to accept named sequences (eg. "\{arrow up}" and similar)
- changed inventory to pause the music rather than muting it (#1707)
- changed the `/pos` command to include the level number and title
- changed the `/tp` command to teleport to items in a round-robin fashion
  The first call will teleport Lara to the object that's the closest to her; repeated calls will cycle through all matching objects in the object placement order.
- improved FMV mode appearance - removed black scanlines (#1729)
- improved FMV mode behavior - stopped switching screen resolutions (#1729)
- improved screenshots: now saved in the screenshots/ directory with level titles and timestamps as JPG or PNG, similar to TR1X (#1773)
- improved switch object names
    - Switch Type 1 renamed to "Airlock Switch"
    - Switch Type 2 renamed to "Small Switch"
    - Switch Type 3 renamed to "Switch Button"
    - Switch Type 4 renamed to "Lever/Switch"
    - Switch Type 5 renamed to "Underwater Lever/Switch"
- fixed screenshots not working in windowed mode (#1766)
- fixed screenshots key not getting debounced (#1773)
- fixed `/give` not working with weapons (regression from 0.5)
- fixed the camera being cut off after using the gong hammer in Ice Palace (#1580)
- fixed the audio not being in sync when Lara strikes the gong in Ice Palace (#1725)
- fixed door cheat not working with drawbridges (#1748)
- fixed certain audio samples continuing to play after finishing the level (#1770, regression from 0.2)
- fixed Lara's underwater hue being retained when re-entering a boat (#1596)
- fixed Lara reloading the harpoon gun after every shot in NG+ (#1575)
- fixed the dragon reviving itself after Lara removes the dagger in rare circumstances (#1572)
- fixed grenades counting as double kills in the game statistics (#1560)
- fixed the ammo counter being hidden while a demo plays in NG+ (#1559)
- fixed the game crashing in large rooms with z-buffer disabled (#1761, regression from 0.2)
- fixed the game hanging if exited during the level stats, credits, or final stats (#1585)
- fixed the console not being drawn during credits (#1802)
- fixed grenades launched at too slow speeds (#1760, regression from 0.3)
- fixed the dragon counting as more than one kill if allowed to revive (#1771)
- fixed a crash when firing grenades at Xian guards in statue form (#1561)
- fixed harpoon bolts damaging inactive enemies (#1804)
- fixed enemies that are run over by the skidoo not being counted in the statistics (#1772)
- fixed sound settings resuming the music (#1707)
- fixed being able to use hotkeys in the end-level statistics screen
- fixed the inventory ring spinout animation sometimes running too fast (#1704, regression from 0.3)
- fixed new saves not displaying the save count in the passport (#1591)
- fixed certain erroneous `/play` invocations resulting in duplicated error messages

## [0.5](https://github.com/LostArtefacts/TRX/compare/afaf12a...tr2-0.5) - 2024-10-08
- added `/sfx` command
- added `/nextlevel` alias to `/endlevel` console command
- added `/quit` alias to `/exit` console command
- added the ability to cycle through console prompt history (#1571)
- changed `/set` console command to do fuzzy matching (LostArtefacts/libtrx#38)
- fixed crash in the `/set` console command (regression from 0.3)
- fixed using console in cutscenes immediately exiting the game (regression from 0.3)
- fixed Lara remaining tilted when teleporting off a vehicle while on a slope (LostArtefacts/TR2X#275, regression from 0.3)
- fixed `/endlevel` displaying a success message in the title screen
- fixed very loud music volume set by default (#1614)
- improved vertex movement when looking through water portals (#1493)
- improved console commands targeting creatures and pickups (#1667)

## [0.4]
Version 0.4 was skipped because of a major repository merge with TR1X into TRX.

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
- added the ability to remap the console key (LostArtefacts/TR2X#163)
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
- fixed medipacks staying open after use in Lara's inventory (LostArtefacts/TR2X#69, regression from 0.1)
- fixed pickup sprites UI drawn forever in Lara's Home (LostArtefacts/TR2X#68, regression from 0.1)

## [0.1](https://github.com/rr-/TR2X/compare/...0.1) - 2024-04-26
- added version string to the inventory
- fixed CDAudio not playing on certain versions (uses PaulD patch)
- fixed TGA screenshots crashing the game
