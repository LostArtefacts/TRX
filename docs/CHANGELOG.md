## [Unreleased](https://github.com/LostArtefacts/TRX/compare/trx-1.10.2...develop) - ××××-××-××

**Lara's movement**
- Added an option to allow Lara to sidestep in swamps (Gameplay → Controls → Swamp sidesteps) (#6250 / TRX1117)
- Fixed Lara entering the step down animation when walking backwards in a swamp room (OG bug) (#6251 / TRX1118)
- Fixed certain SFX, such as Lara's footsteps, playing in swamp rooms (#6248 / TRX1115, regression from 1.0)
- Fixed Lara being able to turn too quickly in swamp rooms (regression from 1.0) (TRX1129)
- Fixed Lara being able to vault or crawl through breakable walls that stand on the edge of a tile (Gameplay → Fixes → Fix breakable wall clipping) (OG bug) (TRX1024)
- Fixed Lara's rope-grab reach being shorter from certain directions (OG bug) (TRX1143)

**UI**
- Added a fullscreen setting, so the window mode can be switched from the menu rather than only with Alt+Enter (Graphic Options → Rendering) (#6187 / TRX1036)
- Added a flat yellow color to the PS1 bar palettes (Graphic Options → UI → Bars) (#5227 / TRX1135)
- Added an option to open the save and load screens instantly (Graphic Options → UI → Instant save/load screen) (TRX1170)
- Changed Lara's outfit setting to offer only the outfits the current level can dress her in (TRX1086)
- Changed moving Lara in photo mode to follow the direction the camera looks, rather than the way she faces (TRX1153)
- Changed settings dialogs to stay readable at large text sizes, with shorter setting names and tighter layouts where needed
- Changed the Fix one-shot music triggers option to sit with the other music settings (Sound → Misc) (TRX1045)
- Changed the PS1 poison healthbar to flat yellow, as the PS1 releases had it (#5227 / TRX1135)
- Changed the TR3 breeze mode to read TR3/4, as it covers both games (Graphic Options → Visuals → Breeze) (TRX1133)
- Changed the preset confirmation to offer Apply and Go back as choices, rather than leaving the keys unsaid (#6258 / TRX1126)
- Changed the F9 key to cycle the lighting model in TR3 and TR4, and the lighting contrast in TR1 and TR2
- Changed vertex snapping to offer Disabled, 320x240, and Upscale Res modes (#6278 / TRX1137)
- Fixed a setting description showing a question mark in place of a key that is bound to a combination, such as Alt+Enter (TRX1136)
- Fixed ability to open the inventory ring while a flyby sequence has Lara's control (TRX1057)
- Fixed several dialogs running past the screen edges or overlapping headings at large text sizes (#6293 / TRX1154)
- Fixed the game flashing over the black bars beside the picture when a frame is advanced in photo mode (regression from 1.10) (TRX1068)
- Fixed the icons beside the volume settings, which now show a note for the music and a speaker for the sound effects (TRX1040)
- Fixed the photo mode camera drifting upwards and overshooting when it is moved while pitched up or down (TRX1142)
- Fixed the photo mode camera flickering and refusing to turn over when it is pitched past straight up or down, and turning in coarse steps while aimed near vertical (TRX1152)
- Fixed the statistics overlapping inventory text at large text sizes (#6295 / TRX1156)

**Developer console**
- Added the `/outfit` console command, which shows or changes what Lara is wearing (TRX1070)
- Added the `/golden` console command, which casts Lara in gold (TRX1070)
- Changed the `/flip` console command to take a flip group, so `/flip 3` moves that group alone while `/flip` on its own moves them all (TRX173)
- Changed the `/set` console command to complete the values a setting accepts, such as its enum values or on and off (TRX1174)
- Changed the developer console to sort completions alphabetically, prioritizing those that start with the text typed (TRX1175)

**Weapons and ammunition**
- Added an option to keep Lara firing the M16/MP5 from her hip while the action key is held, rather than shouldering the gun the moment she stops moving (Gameplay → Controls → M16/MP5 aiming variants) (#3861 / TRX1048)
- Fixed Lara taking out a two-handed weapon in wading-depth water only to put it away at once (OG bug) (#6253 / TRX1120)
- Fixed Lara taking out a weapon while she is fully submerged in a swamp (#6255 / TRX1122)

**Level and game data**
- Added Natla as an outfit for Lara, selectable in every game (Graphic Options → Visuals → Lara's outfit) (TRX1050)
- Added injection support for putting a room in a flip group, which only TR4 levels carry themselves (#5336 / TRX173)
- Added a `requires_alert` property to the sentry gun, which lets a plain trigger set it firing where it would otherwise wait for a security laser (TRX1141)
- Changed a missing or unknown `lara_outfit` in a level to fall back to the default outfit, rather than stopping the game from starting (TRX1087)
- Removed the golden outfits, which the engine now produces from any outfit, freeing their model slots for outfits of your own (TRX1070)

**Saves and settings**
- Added smoke, sparks, mist and bubbles to saves (Gameplay → General → Save effects)
- Changed the save crystal behavior to give Lara a crystal when starting a game, if the mode is set to Saving (pickups), in line with the TR3 PS1 version (Gameplay → General → Crystal mode) (TRX1101)

**Music and sound**
- Added an option to have Lara's sliding SFX stop as soon as she leaves a slope (#6294 / TRX1155)
- Fixed crystal sound effects not playing if Lara collects one underwater (OG bug) (TRX1111)

**Rendering**
- Added affine texture mapping, the uncorrected texturing of the PlayStation, so textures warp across large surfaces as the camera moves; the PlayStation presets turn it on (Graphic Options → Rendering → Affine texture mapping)
- Added PlayStation RGB555 dithering and changed Dithering to offer Disabled, Software Renderer and PS1 (Graphic Options → Rendering → Dithering)
- Added the PlayStation depth cue (Graphic Options → Rendering → PlayStation fog)
- Added the PlayStation lighting model, which deepens bright surfaces into their own color rather than white (Graphic Options → Rendering → Lighting model)
- Changed the lighting contrast option to appear in TR1 and TR2 only, as the other games light dynamic sources their own way (Graphic Options → Rendering → Lighting contrast)
- Changed TR3 to light geometry on the brighter curve its hardware renderer used (Graphic Options → Rendering → Lighting model)
- Fixed a visible seam across the sky in TR4 levels (TRX563, regression from 1.9)

**TR1**
- Changed Lara to retain her equipment when turning to gold on the Midas Hand, with the equipment also turning to gold (TRX1073)
- Fixed Lara's arm remaining in the flare pose if holding one on the Midas Hand (TRX1073)

**TR3**
- Added crystals to each of the levels in The Lost Artefact, and made the crystal mode option visible (Gameplay → General → Crystal mode) (TRX1111)
- Fixed z-fighting in rooms 21, 67 amd 122 in Jungle, and fixed incorrect lighting in room 87 (OG bugs) (TRX1088)
- Fixed Vultures in The River Ganges and Nevada Desert having incorrect animation bounds (OG bug) (#6303 / TRX1163)
- Fixed missing alpha blending on the MP5 and M16 gun flare in the gym (regression from 1.7) (TRX1066)
- Fixed Lara being able to turn too quickly when wading (regression from 1.0) (TRX1129)
- Fixed Lara's sliding SFX continuing after she leaves a slope (#6294 / TRX1155, regression from 1.0)
- Fixed underwater blood clouds appearing in the air when the security lasers hurt Lara at the water surface (OG bug) (#6323 / TRX1180)

**TR4**
- Added the ability to skip in-game cutscenes (TRX1051)
- Added waterfalls, which run and play their loop, and the mist that rises where they land (TRX1067)
- Added the in-game cutscenes of Angkor Wat and Race for the Iris, with the characters speaking their lines and the outcome of the race deciding which ending plays
- Added Boar control (TRX1185)
- Changed a flip to move the group of rooms the trigger names, rather than every flip room in the level (TRX173)
- Fixed animations that move an item sideways playing with the item standing still, such as TR4's guide shimmy (TRX1062)
- Fixed creatures walking through squares a pushable block stands on (TRX1060)
- Fixed rooms and their contents sometimes disappearing while an in-game cutscene plays (TRX1052)
- Fixed parts of the level dropping out of the picture during the cutscene that opens Karnak (TRX1092)
- Fixed the parked jeep showing in the temple during the cutscene that opens Karnak (TRX1092)
- Fixed a crash while an in-game cutscene poses Lara (TRX1059)
- Fixed Lara's shadow and the flames around her when she burns staying where an in-game cutscene began, rather than following her (TRX911)
- Fixed Von Croy's shadow floating at the height of his knees during the scenes he plays in Angkor Wat (TRX911)
- Fixed Lara's shadow being too small to read as the jeep's during the cutscene that opens Karnak (TRX911)
- Fixed the geometry in Karnak causing the opening cutscene camera to be in the void, and some rooms not rendering fully as a result (TRX1184)

**Miscellaneous**
- Added an option to cast Lara in gold whatever she is wearing (Graphic Options → Visuals → Golden Lara) (TRX1070)
- Added a `/version` command, which shows the version this build reports
- Changed the message shown when there is nothing to play to name each game it passed over and say what is wrong with it, rather than leaving the reason in the log (TRX1083)
- Changed a game named with `--mod` to say why it cannot be played, rather than quietly starting a different one (TRX1083)
- Changed a broken settings, strings or game data file to say what is wrong with it and where (TRX1112)
- Changed Lara turning to gold on the Midas hand to gild the outfit she has on, rather than swap her for a golden model (TRX1070)
- Fixed the game closing when a language with a broken strings file is picked (TRX1112)
- Fixed the game closing when another game is switched to while Lara is riding a vehicle (TRX1148)
- Fixed a false warning that the settings could not be saved (TRX1112)

**Lua**
- Added the full weapon definition to `trx.weapons`, so a script can read and change what a weapon does: its damage, reach and accuracy, its aim limits, its ammunition, the animations it is drawn by, and the flash, glow, smoke and shells it throws (TRX1091)
- Added `trx.math.Color` and `trx.math.color()`, so a color is a value with its channels and its hex text on it rather than a string (TRX1091)
- Added `trx.cutscenes.actor_count`, `trx.cutscenes.set_actor_visible()`, `trx.cutscenes.set_node_mesh()` and `trx.cutscenes.clear_node_mesh()`, for hiding an actor in the scene on screen or putting another object's mesh on one (TRX1058)
- Added a new Lua module, `trx.waypoints`, for how far along a level's own progression Lara has got, which TR4 marks out and its guides follow; it is saved with the game and reports the furthest she has ever reached as well as where she is now
- Added `trx.lara.speech_face`, for the face Lara talks with, which follows the outfit she is wearing rather than the one a level carries
- Added `trx.cutscenes.set_lara_shadow_bounds()`, for the box a cutscene gives Lara's shadow, so a scene can make it read as something she rides in (TRX911)
- Changed `trx.cutscenes.play()` to take whether to fade out first, so a scene that opens a level begins on the black screen the level loaded behind (TRX1063)
- Changed `trx.game.end_level()` to end the level silently, leaving the "Level complete!" message to the `/endlevel` console command
- Changed a color setting to read as a `trx.math.Color` rather than as hex text, and to be written with either (TRX1091)
- Changed the `trx.weapons` functions that take a weapon id to be deprecated, the weapon itself now answering what it is available as, what it is carried as, and what it is fed (TRX1091)
- Fixed creatures taking wrong routes where crossing to the next square needs a jump or a monkey swing (TRX1061)



## [1.10.2](https://github.com/LostArtefacts/TRX/compare/trx-1.10.1...trx-1.10.2) - 2026-08-20
**Lara's movement**
- Fixed Lara being able to continue shimmying on monkeybars after death (#6300 / TRX1161, regression from 1.0)

**Saves and settings**
- Fixed save crystals activating even while Crystal mode is set to Disabled (Gameplay → General → Crystal mode) (regression from 1.10)

**Rendering**
- Fixed scenery in an adjoining room being cut off along the edges of the portal it is seen through
- Fixed objects showing through rooms that share their space with the room the object stands in
- Fixed objects that reach into a neighbouring room being cut off or vanishing when seen through that room
- Fixed the boulder in room 78 getting drawn in the overlapping room 74 in Tomb of Tihocan (regression from 1.10.1)

**TR3**
- Fixed the kayak at times not being drawn in certain rooms (regression from 1.10.1)
- Fixed some of Lara's skin joints incorrectly being drawn when riding the kayak (regression from 1.10)



## [1.10.1](https://github.com/LostArtefacts/TRX/compare/trx-1.10...trx-1.10.1) - 2026-08-16
**UI**
- Changed the tab arrows to no longer vanish once the selection moved into the list below them
- Fixed the controls key list jumping after switching to a tab with fewer keys (regression from 1.3)
- Fixed the settings and controls dialog tabs ignoring rebound step left and step right keys
- Fixed the settings dialogs scrolling their list back to the top when the selection moved up to the tabs
- Fixed the water color setting staying editable while a PS1 water color preset was picked (#6265)

**Options and menus**
- Changed the TR1 PC bars appearance to be named TR1, the bars being the ones both TR1 releases draw (Graphic Options → Bars → Bars appearance) (#6260)
- Changed the PlayStation presets to place the health and air bars on the right, where the PlayStation releases put them (#6261)
- Changed the PC presets to name the corners the health and air bars sit in, which they had been taking from the defaults
- Fixed the TR1 PS1 preset picking the TR2 PS1 bars, where TR1 draws the same bars on both platforms (#6260)

**Rendering**
- Fixed the water particles and the lightning bolts growing as the supersampling factor is raised, and following the resolution besides (regression from 1.10)
- Fixed the sparks that keep a fixed size, such as ricochets, shrinking as the supersampling factor is raised (regression from 1.10)
- Fixed the wireframe and debug portal lines thinning as the supersampling factor is raised (regression from 1.10)

**Miscellaneous**
- Fixed the Cabin in Natla's Mines remaining visible after dropping to the floor (regression from 1.10)
- Fixed flames not being drawn if TR4 is launched and the game is then switched to a different mod (regression from 1.10)
- Fixed doors disappearing when seen from the far side of their doorway after loading a save (regression from 1.9)
- Fixed breeze defaulting to being off in fresh installations (regression from 1.10)
- Fixed Lara having the wrong state while grabbing a zipline (regression from 1.10)
- Fixed key icons ignoring the active keyboard layout, which swapped Z and Y on QWERTZ (#6257)
- Fixed the PS1 water color preset showing as unnamed text in The Lost Artefact and Golden Mask (#6265)
- Fixed a lit flare's sparks hanging in the air in front of the camera while the binoculars are up (#6269)



## [1.10](https://github.com/LostArtefacts/TRX/compare/trx-1.9.3...trx-1.10) - 2026-08-12
Showcase: https://youtu.be/DKpqz_Yum6o

**Lara's movement**
- Added the ability for Lara to turn on the spot by pressing walk and roll (Gameplay → Controls → Alternative turns) (#5756)
- Added the ability for Lara to turn on the spot on monkeybars by pressing roll (Gameplay → Controls → Alternative turns)
- Added the ability for Lara to shimmy more quickly (Gameplay → Controls → Fast shimmying) (#5638)
- Added the ability for Lara to pull up from ledges more quickly (Gameplay → Controls → Fast pull up) (#4857)
- Improved state change handling when shimmying is requested while in the slow swing-in state on thin ledges (#5161)
- Improved the pickup embed fix to prevent Lara becoming clamped when picking up items placed on sloped floors with low ceilings (Gameplay → Fixes → Fix pickup embed glitch) (OG bug)
- Fixed Lara attempting to pull up into gaps that would not allow her to stand, resulting in her being pushed out (#5891)
- Fixed Lara being teleported into the ceiling if a door shuts on the ledge she is climbing onto (Gameplay → Fixes → Wall glitch mode) (#6047)
- Fixed differing ladder/hanging behavior when Lara comes to a stop from shimmying when corner shimmying is enabled (regression from 1.9)
- Fixed Lara getting stuck on ropes if she enters the fly cheat while still using one (regression from 1.9)
- Fixed a shaky start to the QWOP animation if the option to fix the step glitch is enabled (regression from TR2X 0.8)
- Fixed Lara incorrectly performing a controlled drop when attempting to grab a perpendicular ledge or when running off a diagonal ledge (regression from TR1X 4.14 / TR2X 1.4)

**Controls**
- Added an option to turn off controller support, for players who remap their controller with external software (Gameplay → Controls → Controller support)
- Added a bindable hotkey for using the binoculars
- Added bindable keys for fast forward and slow motion that last only while held, which start out unbound and leave the turbo key as it was (#2699)
- Added the ability to rebind switching between fullscreen and windowed mode, which stays Alt+Enter unless changed (#2251)
- Changed a multi-key shortcut to answer only to the side of Ctrl, Shift or Alt it was bound to, where a shortcut of a single key still takes either
- Changed the Target change option to allow selecting TR4 behavior, where tapping Look switches target and holding it looks around (Gameplay → Controls → Target change)
- Fixed touch controls not turning themselves on the first time the game is launched on a device that has a touchscreen
- Fixed the key icons in the controls dialog sitting at slightly different heights, most visibly the left trigger and the shoulder buttons (#6151)
- Fixed the + joining the two keys of a shortcut sitting lower than the icons beside it (#6151)

**Camera and binoculars**
- Added TR4 camera mode, which is similar to TR3 but more responsive to Lara's actions such as picking up items
- Changed Lara to say "No" when attempting to use the binoculars when it is not possible to do so
- Fixed a crash when advancing through a flyby sequence in photo mode and the sequence reaches its end (regression from 1.9)
- Fixed the camera snapping aggressively to Lara after some flyby sequences (regression from 1.9)
- Fixed flyby sequences set to loop playing only once, then swinging the camera back to Lara
- Fixed a brief field of view flicker when exiting photo mode during Lara's special animations, such as turning to gold (regression from 1.5)
- Fixed the black surround of the binoculars turning grey when looking in certain directions (regression from 1.9)

**Inventory and pickups**
- Added an option for Lara to collect stacked pickups individually, as per OG TR4 (Gameplay → Controls → Multiple pickups)
- Added support for Lara to use animated interactions for the following:
  - picking up thrown flares
  - grabbing zipline handles
  - using detonators
  - using gongs
  - elevated scion pickups (i.e. Tomb of Qualopec/Sanctuary of the Scion)
- Added an option for a ring to open on the entry it was left on, rather than on its first one, for the item ring or for the keys ring as well (Gameplay → General → Remember inventory position) (#3273)
- Changed boxes of ammunition in the inventory to always show what Lara is really carrying, where finishing a level could round them down
- Fixed Lara being able to light a free/ghost flare by selecting one from the inventory while crawling (Gameplay → Fixes → Fix free flare glitch)
- Fixed the inventory ring being cut off at the sides when the game window is taller than it is wide (#4278)
- Fixed the key that skips a cutscene or a flyby sequence also opening the inventory ring behind it
- Fixed Lara receiving twice the number of flares if given via the game flow (regression from 1.9)
- Fixed the inventory hint for using an item showing Enter rather than the key bound to Action (regression from 1.8.1)
- Fixed Lara being able to collect plinth pickups while ducked (regression from 1.9)
- Fixed Lara being able to collect normal and plinth pickups at the same time if they share a common position (regression from 1.9)
- Fixed Lara moving too quickly towards targets when animated interactions are enabled, which could result in sliding in some cases during long transitions, or not being able to interact with the target at all (regression from Tomb1Main 2.7)
- Fixed Lara being unable to collect some pickups that are very close to edges with a drop when animated interactions are enabled (regression from Tomb1Main 2.7)
- Fixed Lara repeating a pickup animation if jump and roll are held during the pickup (regression from TR1X 4.14 / TR2X 1.4)
- Fixed not being able to perform the item duplication glitch (regression from 1.2)
- Fixed Lara being able to pull out flares while crawling after picking up an item, and dropping flares when using the draw input to enter crouch state (regression from 1.3)

**Weapons and ammunition**
- Added an option for the draw weapon key to pick a weapon Lara can use underwater, such as the harpoon gun (Gameplay → Controls → Underwater weapon draw) (#4119)
- Added an `ammo.infinite` key to `weapons.json5`, saying which weapons and flares never run out (#176)
- Changed the ammunition keys in `weapons.json5` to say what they count; refer to migration notes
- Fixed Lara's arms dropping for a moment when she changes target and there is nothing else to switch to
- Fixed the alternative ammunition pickups, such as the second kind of shotgun ammunition, going into the inventory without loading the weapon

**Enemies**
- Fixed a crash in custom levels when an armed enemy turned ally shoots the last remaining enemy (#5973)
- Fixed shoals of fish and piranhas jumping back to their starting spot when loading a save
- Fixed flying enemies chasing Lara up into ceilings and back out of solid rock, most visible with the wasps in Lost City of Tinnos (#5563, OG bug)
- Fixed Bacon Lara flickering if she dies in a room different to where she fell from
- Fixed Bacon Lara remaining targetable after death
- Fixed Bacon Lara dying prematurely in some geometry setups
- Fixed Bacon Lara mimicking some of Lara's movements, such as looking or using weapons, before being triggered
- Fixed jittery interpolation on Bacon Lara when falling from great heights
- Fixed crawler mutants killed by Lara's allies not being included in the stats when the option to include ally kills is enabled (#5691, regression from 1.7)
- Fixed the body bag trigger to also collect Bacon Lara, Qualopec Mummy and Skidoo Driver's corpses
- Fixed RX Worker 3 in custom levels always looking at Lara when setup with patrol objects, and not properly detecting when she comes into range

**Lifts, vehicles and machinery**
- Added internal collision to lifts when they are moving, so that Lara cannot exit through the meshes
- Changed lift collision to force Lara out of her climbing and vaulting animations if one collides with her (#5899, #5911)
- Changed lift collision to be optional (Gameplay → Fixes → Fix lift collision)
- Changed skidoo and quad bike crashes to not kill Lara when she is immune
- Fixed trains using extreme tilt angles when they come to a stop at a wall (#5886)
- Fixed Lara attempting to climb ladders from inside lifts (#5890)
- Fixed Lara not getting killed by lifts if standing on top of one and the ceiling space becomes too low
- Fixed the detonator box returning to its original position after loading a save (OG bug)
- Fixed the game sometimes crashing when Lara rides a quad bike into deep water or a swamp

**Weather and effects**
- Added the ability to change how heavily the rain or snow falls, from Lua and with the `/weather` console command (#6080)
- Added the glow around gun flashes to enemies firing, where Lara's weapons alone had it (#6127)
- Changed the Breeze option to allow selecting TR2 or TR3 behavior (Graphic Options → Visuals → Breeze)
- Fixed the sun's glare staying on screen in cutscenes once the camera has looked away from it
- Fixed the glow around gun and flare flashes lagging behind them (#5920)
- Fixed Lara leaving ripples on ceilings that lie below the waterline (regression from 1.1)
- Fixed rain and snow starting over when a save is loaded (#5901)
- Fixed a new effect spending its first frame sliding in from where the last one in its slot ended up

**Rendering**
- Added an option for the wobbly geometry of the PlayStation releases (Graphic Options → Rendering → Vertex snapping)
- Added an option to reduce the picture to 8-bit color with a dither pattern, for the look of the software-rendered releases (Graphic Options → Rendering → Dithering)
- Added two anti-aliasing options (Graphic Options → Rendering → Supersampling, Multisampling) (#166)
- Added a filter option for FMVs, so they can be shown smoothed or with visible pixels (Graphic Options → Rendering → FMV filter)
- Added a water color preset, offering the underwater tint each release shipped with, per level where the PlayStation ones varied it; picking one holds the water color below it, and Custom gives the player's own back (Graphic Options → Visuals → Water color preset) (#1619)
- Added a brightness option for background images and patterns, so that brightening the UI leaves them alone (Graphic Options → Rendering → Background brightness) (#6074)
- Added an option for the PS1 raindrops, which are pale rather than blue (Graphic Options → Visuals → PS1 raindrops) (#5206)
- Added a message naming the graphics driver when it is too old to run the game, where the game used to close without any explanation (#5655)
- Changed the brightness options to be given as percentages, as the volume options are
- Changed reflections UV mapping to be more correct
- Fixed a crash when drawing an animating object that has no frame data (#5869)
- Fixed colored dynamic lights in TR1 and TR2 being drawn at the wrong brightness
- Fixed FMVs costing frame rate at high resolutions, where every frame was resized twice on its way to the screen (#6152, #262)
- Fixed loading and legal screens ignoring the upscaling factor and filter
- Fixed sprite shadows tinting the ground they lie on with the water color whenever the camera is underwater (Graphic Options → Visuals → Shadows shape)
- Fixed the picture breaking up into blocks for the length of the first fade after the game is launched, on some graphics drivers (#5506)

**Music and sound**
- Added an option to keep the music playing when Lara dies (Sound Options → Misc → Play music after death) (#4221)
- Changed cutscenes played at turbo speed to speed their music up to match, rather than skipping through it
- Fixed the sound stuttering when a piece of music starts playing (#3094)
- Fixed the game pausing the first time a sound effect is played, which was most noticeable with longer ones (#2286)
- Fixed the music turning to noise when the system audio output is changed while the game is running (#2489)
- Fixed music briefly restarting from its beginning when loading a save (#1265)

**Options and menus**
- Added the language being taken from the operating system on the first launch, where the game ships it (#4460)
- Improved the option descriptions, which now say which original game a setting matches, give the fog distances the originals used, and show the key each cheat is bound to (#4027)
- Changed the Neutral twists option to Alternative turns, which now turns Lara on the spot and on monkeybars as well as twisting her in the air (Gameplay → Controls → Alternative turns)
- Changed the presets to be listed by name rather than in the order they were found (Gameplay → Presets)
- Changed a setting a script is holding to be greyed out, where the row carried only the star saying who took it
- Fixed settings text running off both sides of the screen in the longer languages, most visibly at 4:3 (#6000)
- Fixed the list of settings a preset would change spilling outside its dialog, and its text now wraps (#6000)
- Fixed the load and save game dialogs covering the rest of the screen at larger text scales, where the list now shows as many slots as there is room for (#6175)

**Saves and settings**
- Changed the save crystals option to a mode: crystals can save on the spot, be collected to save from the inventory as on PS1 TR3, heal, or be collectibles (Gameplay → General → Crystal mode) (#5939, #5940)
- Removed the carrying over of legacy settings from TRX 1.4 and older; a small number of settings from files that old may revert to defaults
- Fixed quick saves bypassing the save crystals mode
- Fixed changing the number of save or quick save slots mid-game discarding the progress of the levels played so far (#6054)
- Fixed settings belonging to another game being dropped from the settings file

**Game modes**
- Added the ability to clear the gym's best times, by holding the key shown below them
- Changed the game mode selection option so that Never starts every new game as a regular one (Gameplay → General → Game mode selection)
- Removed the Japanese NG and Japanese NG+ game modes; a Japanese NG save continues as a regular game, and a Japanese NG+ save as New Game+; refer to migration notes
- Fixed a new game, or a level started from Play Any Level, running in the game mode the previous game was started in

**Developer console**
- Added the ability to paste commands in the developer console (with Ctrl+V)
- Added autocompletion to the developer console (with Tab and Shift+Tab to cycle the matches)
- Added a `/dry` console command, to dry Lara off after a swim
- Added a `/rule` console command, to inspect and change the numbers the game plays by, such as how quickly the cold gets to Lara
- Added a `/disco` console command, which sends colored lights spinning around Lara
- Improved error messages related to bad command invocations
- Changed the `/music` console command to list the available tracks when given no argument, as `/sfx` does; `/music status` now reports what is playing
- Changed the large medipack to answer to "big medipack" as well, so `/give big medi` reaches it
- Changed the `/secret` console command to take a secret number on its own, to offer the numbers it can act on in autocompletion, and to say what it expected when given something else
- Changed the `/give` console command to autocomplete what it can hand over and to reach the savegame crystal by name; `/give keys` now covers the plot items alone, and `/keys`, `/guns` and `/moreguns` are commands of their own
- Changed the `/spawn`, `/kill` and `/tp` console commands to accept a family such as `pickup`, `door` or `enemy` in place of a name, and to offer the families each of them can act on in autocompletion
- Changed the `/tp` console command to place Lara better at what she is sent to
- Changed the developer console to accept the numpad Enter key for issuing commands (#6056)
- Changed console commands to carry aliases, so `/help` lists one line per command and `--help` names the other words that reach it
- Changed the `/abortion` console command to `/die`, keeping the Natla easter egg under `natlasucks` and `natla-stinks`
- Changed the `/cutscene` console command to say up front when a game ships none, as `/demo` does
- Changed the `/flood` and `/drain` console commands to say which room they changed, and to warn when it is already in that state rather than report a silent success
- Fixed some console commands being able to target unintended items
- Fixed the `/spawn` and `/kill` console commands not reaching army Winston
- Fixed the `/teatime` console command summoning army Winston rather than Winston himself
- Fixed console commands treating an item that has been removed from the level as though it were still alive
- Fixed the `/trigger` and `/untrigger` console commands crashing or doing nothing on some objects, so they now act on any item exactly as a level trigger would
- Fixed the `/trigger`, `/untrigger` and `/kill` console commands being usable in demos and cutscenes
- Fixed long messages in the developer console running past the right edge of the screen by 1 character
- Fixed the `/cutscene` and `/demo` console commands playing the one after the number they were given
- Fixed the `/play` console command starting the playthrough over only on the gym, so the rules and how wet Lara was carried over from the last run

**Recordings**
- Added `skip start` and `skip end` events to recordings, which stop the drawing while the frames keep running, so a stretch passes as fast as the machine manages
- Fixed a recording pausing along with the game when the window loses focus, which fires the rest of it into a game that is not running
- Fixed a recording not playing back the same way twice, where a fade or any other effect measured in seconds lasted however many frames the machine managed to deliver
- Fixed a headless run having no music at all, so anything waiting on a track behaved differently there than in a run that draws

**Installer and mods**
- Changed the installer to explain when it cannot download the files it needs, and to offer to try again (#6052)
- Fixed mesh debug spheres not rendering after switching mods (regression from 1.5)
- Fixed the title screen starting on the wrong item after switching mods, such as the home photo in place of the passport (regression from 1.5)
- Fixed the demos not starting from the first one after switching mods (regression from 1.5)
- Fixed the game displaying a GUI error dialog when it fails to start in headless mode
- Fixed the mixed mod layout error message running off the edge of the screen, leaving the paths it names unreadable (#6048)
- Fixed the game not running when its folder name contains characters outside the system language, such as Chinese (#573)
- Fixed the installer closing with no window and no message when the .NET runtime it needs is damaged, so it now names what to install (#1088)

**Level and game data**
- Added the teleporter object from TR5, which moves Lara to where it is placed when it is triggered
- Added a `damage` property to the `O_POWER_SAW` object
- Added `show_pickup_aid`, `rotation` and `glow_color` properties for all pickup types
- Changed object properties to take effect as soon as they change, rather than only when the item is first set up
- Changed outfits to support joints, up to two braids per outfit, and to allow positional offset adjustments for equipment meshes; refer to migration notes
- Changed where an item a defeated enemy carried lands, and which way it faces, by defining rules settable in Lua; refer to documentation (#5883)
- Changed the gameflow's `main_menu_picture` to be optional: a game that names no picture shows its title level behind the menu, and its title script says what plays there
- Changed `O_SCION_ITEM_1` handling to use a pickup mode of `PLINTH_SCION`; refer to migration guide

**TR1**
- Added pickup aids to the scions in Tomb of Qualopec and Sanctuary of the Scion
- Changed New Game+ so that flares Lara has collected are kept between levels rather than taken away at the start of each one
- Changed weather to be affected by the breeze
- Changed Bacon Lara's anchor room to be an optional object property rather than a game flow event; refer to migration notes
- Fixed Lara's braid floating or being aligned to the water height when vaulting out of wading depth water (#5900)
- Fixed static meshes in Obelisk of Khamoon and Sanctuary of the Scion interfering with animated interaction pickups
- Fixed the lever sound playing twice in Natla's Mines, Atlantis, Atlantean Stronghold and The Hive (OG bug) (#4371)
- Fixed the Scion that Pierre drops in Tomb of Tihocan being embedded in the floor when 3D pickups are disabled

**TR2**
- Changed weather to be affected by the breeze
- Fixed Lara being able to use a detonator box or a gong a second time by selecting its key in the inventory
- Fixed Lara's braid floating or being aligned to the water height when vaulting out of wading depth water (#5900)
- Fixed the Silver and Jade Dragon secrets being listed in the wrong order in the Floating Islands statistics (OG bug); saves made before this version have the two swapped
- Fixed music triggered from one-shot switches playing more than once (OG bug)
- Fixed the collapsible tiles in Wreck of the Maria Doria room 68 not triggering if Lara jumps over them, and fixed a missing trigger for barrels item 123
- Fixed the lever sound playing twice in 40 Fathoms, Wreck of the Maria Doria, Living Quarters, The Deck, Tibetan Foothills and The Cold War (OG bug) (#4371)
- Fixed the helicopter at the start of The Great Wall and The Cold War remaining visible after stopping (regression from 1.0)

**TR3**
- Added flame blast SFX to the fireheads in Lost City of Tinnos and Highland Fling (Sound → Misc → PS1 SFX replacements)
- Changed the flamethrower blast SFX, which was added in 1.6, to be optional (Sound → Misc → PS1 SFX replacements)
- Changed the Hand of Rathmore in Reunion to not show pickup aids, in line with the other artefacts
- Removed the hardcoded glow color and rotation speed of artefact pickups, and moved to Lua properties instead; refer to migration guide
- Fixed being unable to drop to the secret ledge in Jungle room 76 from the ledge above (#5818)
- Fixed Willard being visible outside the hut at the beginning of the cutscene following Antarctica (resolves #5929)
- Fixed scenery that leans out of its room, such as the streetlight glow in It's a Madhouse!, flickering as the camera turns (#6082, OG bug)
- Fixed enemies and objects that reach out of their room, such as the Loch Ness monster in Highland Fling, flickering as the camera turns (OG bug)
- Fixed flame emitter 103 in Jungle not triggering when entering the temple from room 0 (#6057)
- Fixed Lara, when on fire, not extinguishing at the right water depth compared with OG (regression from 1.1)
- Fixed the waterfall and drowning mist bunching up in one spot instead of spreading out along the water (regression from 1.2)
- Fixed a crash when a level holds fewer raptors than its raptor emitters can send out
- Fixed jittery train interpolation (regression from 1.4)
- Fixed one-shot music playing more than once (#5312)
- Fixed blood spawning from sentry guns when Lara shoots them (regression from 1.5)
- Fixed the wheel switches jumping back to their starting position once Lara has finished turning them (#6055, OG bug)

**TR4**
- Added the main menu playing the title level behind it, alternating its flybys with the cutscenes their triggers start, as in the original game
- Added partial support for in-game cutscenes
- Added reflections, which give objects a sheen drawn from a reflection texture
- Added fires, which burn with visible flames
- Added Lara catching fire from fires and other hazards; she stays alight across level transitions
- Added ricochets: a shot that hits a wall or other hard surface throws out sparks and leaves a puff of smoke
- Added dead enemies fading away a few seconds after they fall, as in the original game
- Added Lara's outfits, TR4 Classic and TR4 Young, each with a golden form
- Added seamless body and braid joints to the outfits, as in the original game
- Added water droplets dripping off Lara after she leaves water, also enabled in TR3 (Graphic Options → Visuals → Water droplets)
- Added a small splash when spent shells land on water (also enabled in TR3)
- Added floor switches
- Added crowbar switches
- Added jump switches
- Added pulley switches, with properties for the number of pulls they need and whether they are single use
- Added underwater ceiling switches
- Added reach/receptacle switches
- Added shove button switches
- Added hidden reach-in pickups
- Added pickups that need to be pried with the crowbar
- Added Sarcophagi and the pickups hidden inside them
- Added an option to let fires and other dynamic lights illuminate static meshes (Graphic Options → Visuals → Static mesh lighting)
- Improved crouch turning, so that Lara moves into crouch idle, crouch roll and crawl idle more responsively
- Changed binoculars to not make Lara put her current weapon away
- Fixed pixel sparks, such as blood and the shell splash, drawing as squares instead of the original streaks
- Fixed the camera snapping to elevation and angle changes in instances such as opening floor trapdoors
- Fixed the camera getting stuck when Lara traverses around corner ladders (OG bug)
- Fixed the camera not reacting to Lara using a shove button switch (OG bug)
- Fixed Lara's braid and pigtails spinning around their own axis (OG bug)
- Fixed Lara's shadow being darker than in the original game
- Fixed Lara's arm swinging while she runs or crouches with a flare
- Fixed a missing texture on Young Lara's left hand
- Fixed a misaligned texture on Young Lara's hips
- Fixed animated textures, such as water surfaces, cycling at half their original speed
- Fixed exploding deaths showing no flames or explosions
- Fixed flares burning red and trailing sparks and bubbles, so they now burn green and cast only their light, as in the original game
- Fixed flares playing a sound while they burn; they are silent, as in the original game
- Fixed Lara being able to light a flare while crawling; she now refuses, as in the original game
- Fixed Lara sidestepping in the wrong direction when turning off floor/crowbar switches
- Fixed Lara attempting to interact with one-shot pulley switches
- Fixed pulley switch animations not synchronizing with Lara if she continues to pull one indefinitely
- Fixed Lara using the wrong animation on switch item 2 in The Sphinx Complex (OG bug)
- Fixed one of the braziers on the title screen going out for good once the menu's flybys had passed it (OG bug)
- Fixed pickups that are pried with the crowbar animating indefinitely if the game is saved and loaded during the animation (OG bug)
- Fixed Lara being able to collect Sarcophagi pickups by crouching and glitching through the surrounding casing (OG bug)

**Lua**

The Lua integration was rewritten and existing scripts will need updating; refer to migration notes.

*New modules*
- Added a new Lua module, `trx.math`, with the engine's own fixed-point trigonometry and the `DEG_1`, `DEG_45`, `DEG_90` and `WALL_L` constants
- Added a new Lua module, `trx.strings`, with `fuzzy_match()`, `regex_match()`, `collapse_ranges()` and `dedent()`
- Added a new Lua module, `trx.json`, to write a value out as JSON
- Added a new Lua module, `trx.rules`, holding the numbers the game plays by
- Added a new Lua module, `trx.argparse`, a declarative argument parser inspired by Python's argparse
- Added a new Lua module, `trx.locale`, for the text the player reads, looked up by key
- Added a new Lua module, `trx.weather`, to read and set the runtime weather
- Added a new Lua module, `trx.mod`, to list the game's mods and read the loaded one
- Added a new Lua module, `trx.savegame`, to read the save slots and start a saved game
- Added a new Lua module, `trx.lua`, with `eval_expr()` and `eval_file()`, to evaluate Lua code at runtime
- Added a new Lua module, `trx.cutscenes`, for playing TR4's cutscenes, reading or rewriting which of them have run, and framing one, with the `on_cutscene_trigger`, `on_cutscene_start` and `on_cutscene_end` events
- Added a new Lua module, `trx.stats`, for what a level keeps count of – its secrets, pickups, kills, crystals, timer and deaths – alongside how much of each there was to find, for any level rather than only the one being played
- Added a new Lua module, `trx.fx`, with `emit_light()` and `emit_fog()`, for a light or a ball of fog put in the world for as long as a script keeps asking for it; the volumetric fog that was TR4's alone now shows in every game
- Added a new Lua module, `trx.random`, with `random()`, `randint()`, `choice()`, `choices()`, `angle()` and `chance()`; it draws from the sequence the game itself runs on, so what a script draws comes back the same after a reload
- Added a new Lua module, `trx.inventory`, for reading and changing what Lara carries: it counts and walks her inventory an entry at a time, says what she holds of each weapon and how many shots she has for it, and reaches any level's own inventory the same way through `trx.game.Level.inventory`
- Added a new Lua module, `trx.weapons`, for what a weapon is rather than what Lara has of it: whether the game allows it, which pickup it is, and what a box of its ammunition is worth
- Added a new Lua module, `trx.zones`: script-defined trigger regions, as a box, a sphere or a single sector, reporting what enters them, what leaves, and what a flyby camera passes through

*Items and rooms*
- Added `item:take_damage()`, which hurts an item the way a weapon does, and reports through `on_hit` and `on_kill`
- Added `trx.items.spawn()`, to place a new item in the level at runtime
- Added `trx.objects.swap_sprite()`, to exchange the sprites two objects are drawn from
- Added `trx.objects.query`, `trx.items.query` and `trx.rooms.query`, composable filters over a level's objects, items and rooms that match names, families, state and the rooms a position is in, and combine with `&`, `|` and `~`
- Added pickup families to `trx.objects.query` – `gun`, `ammo`, `supply`, `tool`, `key`, `puzzle`, `quest`, `examine`, `collectible` and `secret` – so a script can ask what a pickup is
- Added `trx.items.get()`, `trx.items.count()`, `trx.rooms.get()`, `trx.rooms.count()` and `trx.objects.get()`, replacing the `fn` namespaces
- Added `trx.rooms.find_valid_pos()`, to nudge a position into valid room geometry
- Added `trx.rooms.floor_height()` and the room method `floor_height()`, the height of the floor under a position
- Added `trx.rooms.flipped`, whether the room map is currently flipped
- Added item methods, `activate()`, `deactivate()`, `trigger()`, `destroy()`, `die()`, `shatter()`, `distance_to()`, `is_valid()`, `get_property()`, `set_property()` and `get_property_names()`, letting a script fire a trigger or antitrigger at an item exactly as a level would
- Added the writable Lua item field `is_one_shot`
- Added object methods, `get_property()`, `set_property()` and `get_property_names()`
- Added the object methods `get_names()` and `get_default_names()`, every name an object answers to in the player's language and the English names a lookup falls back on before a language file is loaded
- Added new Lua item fields, `anim_state`, `goal_anim_state`, `speed`, `fall_speed`, `gravity`, `collidable`, `mesh_bits`, `touch_bits`, `max_hit_points`, `index`, `is_triggered`, `trigger_mask`, `is_reversed`, `was_hit`, `is_simulated`, `is_present`, `is_visible`, `is_finished`, `is_in_play`, `is_alive`, `is_targetable`, `is_hostile` and `is_killed`, replacing the `item.status` enum, and made `timer` writable
- Added new Lua object fields, `loaded`, `is_intelligent`, `mesh_count`, `anim_count`, `radius`, `shadow_size`, `smartness`, `pivot_length` and `semi_transparent`
- Added indexing and the length operator to `trx.items`, `trx.rooms` and `trx.objects`, so `trx.items[0]` is the first item and `#trx.items` is how many the level has
- Added `room:is_valid()`, so a room handle held across a level change can be checked the way an item handle can
- Changed `trx.items` and `trx.rooms` to hand out opaque handles rather than `{ idx = ... }` tables, so a handle to a killed item now raises instead of silently addressing the item that took its slot
- Changed handles to compare equal when they name the same thing, so `trx.items[0] == trx.items[0]`
- Changed `trx.items` and `trx.rooms` to count from zero, matching the item and room numbers level editors show, and made `pairs()` walk them keyed by that number
- Changed room handles to go stale at a level change rather than quietly naming a different room
- Changed the item, flip slot and music track trigger masks to the editor's own numbering, 0 to 31, where they carried their floordata bit positions
- Changed `room.idx` to `room.num`
- Changed `item.object_id` to be read-only
- Changed `trx.objects[id]` to return `nil` for an unknown id, where it used to return an object that answered to nothing
- Removed the raw `item.flags` field; its trigger bits are read through `trigger_mask`, `is_reversed`, `is_triggered`, `is_killed` and `is_one_shot`
- Removed `trx.items.find()` and `trx.items.first()`; `trx.items.query` does both
- Removed the `trx.pickup` module; its enum is now `trx.items.PickupMode`

*Lara*
- Added `trx.lara.cure_poison()` and `trx.lara.extinguish()`, to clear Lara's poison and put her out
- Added `trx.lara.dry()` and `trx.lara.is_wet`, to dry Lara off after a swim and to check whether she needs it
- Added `trx.lara.is_flying`, to read and toggle the fly-mode cheat
- Added `trx.lara.teleport()`, to move Lara to a position, as `/tp` does
- Added `trx.lara.set_mesh()` and `trx.lara.clear_mesh()`, to put another object's mesh on one of Lara's own and to take it back off, so that a level can dress her from its own geometry
- Added new Lua Lara state, `trx.lara.poison`, `trx.lara.electric`, `trx.lara.is_burning`, `trx.lara.is_crouched`, `trx.lara.is_climbing`, `trx.lara.water_status`, `trx.lara.gun_status`, `trx.lara.hit_direction`, `trx.lara.requested_gun` and the dive, death, sprint and pose timers
- Added the braid and crowbar constants to `trx.lara.ExtraMesh`
- Changed `trx.lara.is_burning` to be writable, so setting it lights Lara or puts her out
- Changed `trx.lara.extra_anim` to a boolean, where it used to be the relative animation number, or -1
- Changed `trx.lara.mesh` and `trx.lara.extra_mesh` to declared enums, `trx.lara.Mesh` and `trx.lara.ExtraMesh`

*Events*
- Added `trx.events.on_flip_effect()`, letting a script handle a flipeffect run by a trigger or an animation command (#4108)
- Added `trx.events.on_room_change()`, which happens whenever an item changes rooms
- Added `trx.events.on_level_unload()`, which happens as the engine lets go of a level, while the world the script was written against is still there to read
- Added `room:on_enter()` and `room:on_exit()`, which happen when Lara – or, with `watch = "all"`, any item – changes rooms
- Added `trx.events.on_trigger()` and the per-item `item:on_trigger()`, to react to a trigger of any kind being aimed at an item, with the trigger's type, mask, timer and one-shot flag
- Added `trx.events.on_show()` and `trx.events.on_hide()`, with the per-item `item:on_show()` and `item:on_hide()`, which happen when an item becomes visible or hidden
- Added `trx.events.on_finish()` and the per-item `item:on_finish()`, which happen when an item finishes its run, such as a sprung trap or a thrown switch
- Added `trx.events.on_enter_sim()` and `trx.events.on_leave_sim()`, with the per-item `item:on_enter_sim()` and `item:on_leave_sim()`, which happen when an item starts or stops being simulated
- Added `trx.events.on_activate()` and `trx.events.on_deactivate()`, with the per-item `item:on_activate()` and `item:on_deactivate()`, which happen when a trigger activates or an antitrigger deactivates an item
- Added `trx.events.on_destroy()` and the per-item `item:on_destroy()`, which happen as an item is removed from the game
- Added `trx.events.on_enter_world()` and `trx.events.on_leave_world()`, with the per-item `item:on_enter_world()` and `item:on_leave_world()`, which happen when an item enters the world as a runtime spawn or leaves it
- Added `trx.events.on_hit()` and the per-item `item:on_hit()`, which happen when an item takes damage, with the amount
- Added `trx.events.on_kill()` and the per-item `item:on_kill()`, which happen when damage takes an item's hit points to zero
- Changed the event hooks to hand back a `trx.events.Listener` rather than a number, which detaches itself with `listener:detach()` and says whether it was still attached
- Changed `trx.events` handlers to no longer receive a dummy argument in `before_control` and `after_control`
- Changed the Lua level events to a single `on_game_start`, which every kind of level fires, with `on_title_start` for the title screen; refer to migration notes
- Removed `trx.events.EventType`

*Game and levels*
- Added `trx.game.gym`, the gym level, or `nil` where a game has none
- Added `trx.game.Level.key`, what a level is called after the file it loads
- Added new Lua game state, `trx.game.is_loaded` and `trx.game.is_playable`
- Added `trx.game.is_ngplus`, which tells whether the run started from the passport's bonus entry
- Added `trx.game.LevelType.TITLE`, and a `demo` level type to the game flow
- Added `trx.game.play_gym()`, to start the gym
- Added `trx.game.screenshot()`, to save a screenshot
- Added `trx.game.end_level()`, to end the current level
- Added `trx.game.exit_to_title()`, to leave the current game for the title screen
- Added `trx.game.exit_game()`, to close the game
- Added new Lua level fields, `script_path`, `lara_outfit`, `music_track`, `water_particles` and the unobtainable pickup, kill and secret counts
- Changed `trx.game.levels` and `trx.game.play_level` to leave out the gym, so numbering no longer shifts and the last level is reachable
- Changed the Lua level field `name` to `title`

*Settings*
- Added `trx.config.reset()`, to put a setting back to its default
- Added `trx.config.describe()`, to read a setting's shape and accepted values
- Added `trx.config.format_value()`, the current value spelled the way the console prints it
- Added `trx.config.accepted_values()`, what a setting takes, as text for an error message
- Added `trx.config.declare()`, for a game to add settings of its own: they are saved and loaded with the player's, translated from the game's own strings, and shown in the settings menu where the declaration asks for
- Added `trx.config.on_change()`, to hear what a setting holds now and whenever it moves
- Added new Lua config functions, `trx.config.override()`, `trx.config.restore()` and `trx.config.is_overridden()`, to change a setting without overwriting the player's own value
- Changed `trx.config.get()` to return the option's own type rather than always a string
- Removed `trx.game.settings`, which duplicated `trx.config`

*Music and sound*
- Added `trx.music.tracks`, the level's tracks as `trx.music.Track` handles keyed by id, each with `:play()` and `:path()`
- Added `trx.music.current_track` and `trx.music.looped_track`, the playing and ambient tracks as `trx.music.Track` handles
- Added `trx.music.streams`, the soundtrack's streams as `trx.music.Stream` handles, each of which can be paused, resumed, sought and stopped on its own
- Added `trx.sound.samples`, the level's samples as `trx.sound.Sample` handles keyed by id, each with `:play()`, `:stop()` and its `volume`, `range`, `randomness` and `pitch`
- Added `trx.sound.streams`, the sound effects playing now as `trx.sound.Stream` handles, each of which can be paused, resumed and stopped on its own
- Changed `trx.sound.play`, `trx.sound.stop` and `trx.music.play` to take a sound or track by catalog name, which works across games, rather than the level's own slot; the slot is reached through the `samples`/`tracks` handles, and `play` hands back the stream it starts
- Removed `trx.music.play_track`, an undocumented alias of `trx.music.play`

*Camera*
- Added `trx.camera.pos` and `trx.camera.target_pos`, where the camera is and what it is looking at, with `trx.camera.room_num` and `trx.camera.target_room_num` for the rooms they sit in
- Added `trx.camera.play_flyby()`, to start a flyby camera sequence, and `trx.events.on_flyby_end()`, which happens when one reaches its last camera
- Added `trx.camera.is_flyby_active` and `trx.camera.cancel_flyby()`, to see and stop a flyby sequence

*Assault course*
- Added new Lua assault course functions, `trx.assault.finish()`, `trx.assault.is_running()` and `trx.assault.is_visible()`, and a new property, `trx.assault.active_track`
- Added the track to the assault course record functions, so the quad bike's records can be read and written

*Console and logging*
- Added `p`, a global shorthand for `trx.console.log`, and made the console log functions take any value, pretty-printing a table
- Added `trx.console.register()`, for a script to add its own console command
- Added `trx.console.commands()`, every registered command in the order they were added, and `trx.console.command()`, the one a name or alias reaches, matched as the console matches when it dispatches
- Added `trx.log.generic()` and the `trx.log.LogLevel` enum, to log at a level chosen at runtime
- Added `trx.log.warning()`, an alias of `trx.log.warn()`
- Changed the Lua logging functions to take a single message rather than a list of strings
- Removed `trx.console.log.LogLevel`, a duplicate of `trx.log.LogLevel`

*Catalogs and enums*
- Added new Lua catalog functions, `trx.catalog.to_slot()` and `trx.catalog.from_slot()`, to convert between a TRX id and the slot the current game's own files use for it
- Added new Lua enums, `trx.items.Status`, `trx.items.SwitchMode`, `trx.lara.WaterState`, `trx.lara.GunState`, `trx.game.LevelTable`, `trx.catalog.Context` and `trx.rooms.FlipStatus`
- Changed Lua enums to answer to a constant's name in any case, so `trx.catalog.objects.wolf` and `trx.catalog.objects.WOLF` are the same constant
- Changed Lua enums to be read-only, including the table `pairs()` used to hand out; writing to one used to break every later lookup

*Scripts and the API*
- Added `require()` to a game's scripts, taking a name that carries the directory it lives in: `tr1.my_module` for a script belonging to a game, `common.my_module` for one in the shared pool beside the executable
- Added `trx.api.strict()`, which checks a script's arguments against the API's own declarations
- Added a script watchdog: a script that runs for over 5 seconds without handing control back is stopped with a script error, where it used to freeze the game
- Changed Lua scripts to be found by name rather than declared in the game flow: a level loading `wall.tr2` runs `scripts/wall.lua`, and `scripts/_game.lua` runs as the game starts, for what a game sets up rather than a level – refer to migration notes
- Changed `trx.api` to hold only `strict` and `is_strict` once sealed, and `trx.api.strict()` to check handle method arguments too
- Changed a position table to require all three coordinates, rather than reading a missing one as zero; a position is documented as `trx.math.Vec3` and an orientation as `trx.math.Rot`
- Changed the names that stand for a number to say which kind they are - `item.num`, `item.anim_num`, `slot_num`, `object_id` and the like; refer to migration notes
- Changed writes that a field cannot hold to raise rather than truncate, so `item.hit_points = 99999` no longer wraps
- Fixed a number too large for the engine wrapping into range rather than being refused, so `trx.items[4294967297]` no longer reads as the first item



## [1.9.3](https://github.com/LostArtefacts/TRX/compare/trx-1.9.2...trx-1.9.3) - 2026-07-19
- added support to inject object and item properties, for example via TombEditor
- fixed flyby cameras not working when part of an injection embedded in a level file (regression from 1.9)
- fixed the microphone's position not updating during flyby sequences (regression from 1.9)



## [1.9.2](https://github.com/LostArtefacts/TRX/compare/trx-0.1...trx-1.9.2) - 2026-07-14
- fixed TR1 and TR2 skyboxes being 2× too bright (regression from 1.9)
- fixed Lara being unable to use binoculars when fixed cameras or track path flyby sequences are active (regression from 1.9)
- fixed an interpolation issue when Lara performs inner-corner climbing (regression from 1.9)



## [1.9.1](https://github.com/LostArtefacts/TRX/compare/trx-1.9...trx-1.9.1) - 2026-07-12
- fixed Lara's arms becoming locked if she draws a flare on a specific frame after pulling into a crawlspace from a ladder (#5801, regression from 1.3)
- fixed bad animation frames on some switches in Lud's Gate (#5804, regression from 1.6)
- fixed enemies and objects animating erratically in savegames carried over from an earlier version of the game (#5802, regression from 1.9)



## [1.9](https://github.com/LostArtefacts/TRX/compare/trx-1.8.1...trx-1.9) - 2026-07-12
Showcase: https://youtu.be/FapipqrYQI0
- added footprints to savegames
- added `O_GENERIC_TRAP_1…10` for custom levels, with properties to set damage, blood intensity and collision details; animations are fully offloaded to data
- added `O_ANIMATING_11…16`
- added a `collidable` property to `O_ANIMATING_1…16` objects
- added support for custom levels to use plinth/pedestal pickups by defining the `pickup_mode` property of collectable items; refer to object documentation (#5007)
- added animation details to the UI debug overlay for Lara's arms, visible when `enable_debug_anim` is on
- added barefoot landing SFX to each of Lara's relevant outfits (#5210)
- added support for levels generated by TombEditor to name items
- added support for `.bik` FMV files
- added properties to the `O_LIFT` object:
  - `wait_time`, which defines how long to wait after activation before beginning to move
  - `travel_distance`, which defines how many clicks the lift will travel vertically
  - `speed`, which defines how many world units the lift will travel per frame
- added properties to the `O_BIG_BOWL` object:
  - `pour_time`, which defines how long to pour liquid from the bowl
  - `flip_slot`, which defines the flip map slot index to alter once liquid has finished pouring
- added a `flip_slot` property to the `O_PORTACABIN` object to define the flip map slot index to alter once the cabin has landed
- added support for flyby camera sequences
- added the ability for Lara to shimmy around corners, TR4 style (Gameplay → Controls → Corner shimmying) (#1375, #4370)
- added an option to make Binoculars available in the inventory (Gameplay → General → Binoculars)
- added rope mechanics for custom levels
- added climbing poles for custom levels
- added a setting to control whether the opening story FMV plays on new game or at launch (Gameplay → General → Intro FMV timing) (#5658)
- added an option to render gun/flare flashes semi-transparent with a glow bloom effect, like the TR2 PS1 version, for TR1/TR2 (Graphic Options → Visuals → Gun glow) (#1709)
- improved the `-l` command line option to match installed level titles when no file path is found
- improved handling of dead enemies used as switch triggers in custom levels by not altering their activation status and by adding detection for having been exploded (#5682)
- changed `Game mode selection` to allow hiding the dialog even after having completed the game (Gameplay → General → Game mode selection)
- changed opening story FMVs to play on new game rather than on every launch/switch by default (#5658)
- changed `O_SPARKS_GFX` sprite ordering and contents - refer to migration guide
- removed the hard-coded spawn distance between Puna and his Lizards (#5686)
- fixed the Gameplay settings dialog layout so settings lists and presets use the available space more consistently
- fixed Lara having an empty left holster if the option to remember guns between levels is used and she finished with the Desert Eagle (#5697)
- fixed Lara getting stuck in the flare throwing animation if she is killed in this state
- fixed ghost flare lighting/effects spawning if Lara is killed while drawing or undrawing a flare
- fixed a crash if loading a save that was made on the final frame of Lara's flare control transition animation (#5683)
- fixed inconsistent tread SFX when Lara turns to the right in shallow water as compared with turning left
- fixed a missing footstep sound when Lara finishes a handstand and when she climbs onto a ledge from a ladder (#5070)
- fixed missing SFX when Lara transitions from hanging to crouching (#5070)
- fixed Lara's torso remaining rotated if she grabs a ledge while looking
- fixed Lara having guns in her hands in custom levels that begin with a cinematic scene if she finished the previous level with guns equipped
- fixed Lara's shadow appearing at the top of a pushblock after having pulled it until she returns to a standstill (#1576)
- fixed Lara being killed by pushblocks if she pulls one onto a trapdoor that is set to be triggered by that pushblock
- fixed Lara being killed by Thor's Hammer in custom levels even when the hammer does not touch her head
- fixed Lara incorrectly landing when falling into a void set up with `disable_floor`
- fixed an invisible block appearing below Thor's Hammer in custom levels when it is not used at floor level
- fixed Lara becoming clamped and softlocked if a lift descends on top of her
- fixed Lara moving through the floor of a lift if she picks up a flare while it's moving
- fixed Lara moving through the floor of a lift if she performs a neutral twist while it's moving (regression from TR1X 4.14/TR2X 1.4)
- fixed Lara not travelling at the same rate as a moving lift if she hits wall while inside or on top of it (regression from TR1X 4.12/TR2X 1.2)
- fixed Lara persisting to crouch after landing when either crawling backwards or jumping out of a crawlspace and the crouch toggle option is enabled (#5703, regression from 1.3)
- fixed an extra pickup being included in the total statistics if a dragon is used in custom levels independently of Bartoli (regression from 1.3)
- fixed persistent splashing effects if Lara falls onto spikes in one-click high water (regression from 1.0)
- fixed game mode options not displaying after having beaten the game, if the option itself is switched off (regression from 1.0)
- fixed Lara being able to break out of locked cameras activated by heavy triggers by equipping her weapons (regression from 1.1)
- fixed the installer not completing normally if an error is encountered during directory clean-up
- fixed debug status overlays visible over inventory ring, pause screen and FMVs, including the end credits
- fixed Lara being able to push/pull a block that is sitting on a collapsible tile about to drop (#5786, regression from 1.8)
- fixed Lara being unable to pull a block that is sitting directly below a room portal and she has a ceiling directly above her
- fixed Lara continuing to travel in the zip line state despite the zip line having stopped, allowing her to void in some cases
- fixed Lara jittering when letting go of zip lines in some cases (#2823)
- fixed Lara's lower meshes re-appearing after saving and loading in the kayak (regression from 1.3)

**TR1**:
- added Lara's PS1 shimmying sound effects (Sound → Misc → PS1 SFX replacements) (#943)
- added the TR2 inventory background as an option for the inventory, pause and stats screens
- removed the option to fix the chain block sound via the UI in Unfinished Business
- fixed missing hand grab SFX when Lara is climbing a ladder (#4266)

**TR2**:
- added the option to toggle FMVs via the UI in The Golden Mask
- fixed Lara not greeting the player at the start of the assault course (regression from 1.8)
- fixed missing hand grab SFX when Lara is climbing a ladder (#4266)
- fixed missing and mistimed closing SFX on doors type 1 and 2 in Barkhang Monastery (#4417, #4418)
- fixed missing SFX when jumping into the boat in Bartoli's Hideout (#4434)
- fixed geometry issues in rooms 60 and 115 in Furnace of the Gods, which could cause the TR3 camera to become stuck and allow Lara to fly into the ceiling

**TR3**:
- added support for embedded injections in level files
- added the TR2 inventory background as an option for the inventory, pause and stats screens
- fixed water ripples appearing too intense compared to OG (regression from 1.1)
- fixed missing portals in Aldwych room 87, which could lead to Lara becoming softlocked
- fixed the ticket booth SFX playing in Aldwych when Lara treads in shallow water
- fixed the underwater swimming SFX playing when Lara turns to the right in shallow water
- fixed missing knees shuffle SFX when Lara climbs onto a ledge (#5070)
- fixed missing SFX when Lara is shimmying (#4996)
- fixed missing SFX in the Pipeman's death animation (#5036)
- fixed geometry issues in room 53 in Madubu Gorge, which could cause the TR3 camera to become stuck and allow Lara to fly into the ceiling
- fixed the stats level counter not taking into account level progression and using the level's default number instead (#5757, regression from 1.2)

**TR4**:
- added very rudimentary support for TR4 levels
- added the ability to skip flyby sequences (Gameplay → General → Cinematic skips)
- added the ability to pause during flyby sequences
- changed Angkor Wat's UV Rotate value from 8 to 4
- changed Crowbar doors to prompt the inventory ring instead of silently triggering
- fixed tracking path flyby camera sequences not starting from the nearest position to Lara



## [1.8.1](https://github.com/LostArtefacts/TRX/compare/trx-1.8...trx-1.8.1) - 2026-06-15
- added config presets to allow restoring Lara's original moveset with respect to each game
- improved texture injections for OG levels to avoid overwriting texture pages in files that don't match the injection source (#5669)
- fixed a potential crash when resurrecting Lara with the fly cheat with an expired flare in her hand (regression from 1.1)
- fixed a crash in the New Game dialog if the gameflow has a reference to an FMV which itself is not defined in the gameflow (regression from 1.0)
- fixed menu functions (show info, value adjustment, cutscene seek) being affected by gameplay control rebinds (#5666)
- fixed Assault Course and Quad Bike best times sometimes appearing in the wrong order in the stats screen

**TR1**:
- fixed Lara taking exposure damage in certain rooms with injected skybox/wind properties (#5664, regression from 1.8)
- fixed Lara's braid in some rooms in Lost Valley and Colosseum not being affected by the breeze (regression from 1.4)



## [1.8](https://github.com/LostArtefacts/TRX/compare/trx-1.7.1...trx-1.8) - 2026-06-13
Showcase: https://youtu.be/dwb3eT2zRHU
- added French translation (thanks to Wronschien)
- added an option for Lara to pick items up more quickly, similar to TR4+ (Gameplay → Controls → Fast pickups) (#1365)
- added an option to allow Lara to push/pull movable blocks continuously across tiles without stopping each time, similar to TR4+ (Gameplay → Controls → Continuous pushblocks) (#1354)
- added `O_DOLPHIN`, which behaves in the same way as `O_ORCA` but has collision underwater
- added `O_ANIMATING_EXT_1…10`
- added a new console command - `/burn` - to toggle whether or not Lara is on fire
- added `trx.assault` Lua functions to control the Assault Course and Quad Bike timers
- added poison Lua property to `O_DART` and `O_DISC`, so that they can poison Lara just like `O_POISON_DART`
- added Lara's TR2 alpha bomber jacket outfit (#5594)
- added the ability for skidoos and quad bikes that are inside or on top of lifts to move when the lift itself moves (#4353)
- added the ability for skidoos, quad bikes and kayaks to activate heavy triggers (#4354)
- added the ability to configure exposure zones (including above water) and visible breath separately per room
- added the ability for fish shoals to be affected by surrounding lighting and fog via a `use_room_lighting` property (#5618)
- added animated interaction support to keyholes and puzzle slots (#4471)
- added an option to control snap interactions for targets such as pickups and switches (Gameplay → Controls → Snap interactions)
- improved injected gun/ammo model consistency between each game
- changed `/music` to show the current track, deferred ambient, and any active overlay tracks when used without arguments
- changed `/music` to accept `stop` and list playable track ranges for invalid track IDs
- changed vehicles to allow defining in Lua whether or not they can activate heavy triggers; refer to migration notes
- changed the Skidoo and Quad Bike to allow defining in Lua whether or not they should collide with static meshes (#4285)
- changed Assault Course Lua record access from `trx.assault_stats` to `trx.assault.stats`
- changed hard-coded music tracks from Snowmobiles, mine carts, and RIBs to be configurable via Lua
- changed hard-coded fish and piranha setup to be configurable via Lua
- changed hard-coded small Cobra radius setup to be configurable via Lua
- changed the hard-coded creature melee and hitscanner damage values to be configurable via Lua. Exemption: visible projectiles.
- changed the hard-coded traps damage to be configurable via Lua. Exemption: one-shot traps such as crushing boulders.
- removed the hard-coded movement speed and moved it to Lua instead for the following objects:
    - `O_CEILING_SPIKES`
    - `O_LAVA_WEDGE`
    - `O_SPIKE_WALL`
- removed the limitation of only having two fish sprite types in any level
- removed the limit of at most 8 fish/piranha shoals per level
- removed `O_DISPOSABLE_ANIMATING_1…10` objects (use `O_ANIMATING_EXT_*` with `kill_on_trigger` property set to true instead)
- fixed missing and incorrect textures on Lara's TR2 bomber jacket outfit
- fixed being able to start the Quad Bike track timer without Lara getting onto the quad bike first (regression from 1.1)
- fixed missed Assault Course targets not penalizing player's time if Lara crosses the finish line before they collapse (regression from 1.2)
- fixed fish sometimes disappearing when crossing room boundaries (OG bug)
- fixed fish shoals not picking up the underwater tint correctly
- fixed Lara's underwater hue being retained when re-entering a RIB and the responsive mesh tint is disabled
- fixed a crash with old custom levels that have sectors pointing to invalid floor data (#5568)
- fixed `O_SCION_ITEM_3` items exploding at inconsistent times if multiple are used in the same level
- fixed `O_KILL_ALL_TRIGGERED` not fully disabling enemies, allowing Lara to continue targeting them (OG bug)
- fixed the enemy health bar disappearing later than Lara's own bar when holstering weapons (regression from Tomb1Main 0.2)
- fixed crystal totals being shown incorrectly after loading a save (#5589, regression from 1.5)
- fixed a very rare crash when loading saves from console during a pause or photo mode
- fixed Lara embedding into walls during the sprint-slide animation if she tried to interact with a keyhole or puzzle slot at the same time (regression from TR1X 4.14, TR2X 1.4)
- fixed recording replays from different mods on the same engine as last played loading the wrong mood
- fixed recording replays switching the last played mod

**TR1**:
- added support for fish and piranhas without sacrificing explosion sprites (refer to notes in migration guide) (#4358)
- added support for bat emitters without sacrificing explosion sprites (refer to notes in migration guide)
- added the abilty to use animated spikes
- added support for Lara's breath to show in cold rooms (requires `O_SPARKS_GFX`)
- fixed bad visibility bounds on palm trees throughout the Egypt levels, causing them to be clipped out of view too eagerly (#5648)
- fixed Lead Bars not being examinable after picking them up (regression from TR1X 4.6)

**TR2**:
- added support for fish and piranhas without sacrificing explosion sprites (refer to notes in migration guide) (#4358)
- added support for bat emitters without sacrificing explosion sprites (refer to notes in migration guide)
- added the abilty to use animated spikes
- added support for Lara's breath to show in cold rooms (requires `O_SPARKS_GFX`)
- fixed TR2:GM missing opening Eidos Interactive / Core Design FMV (#5609)
- fixed Lara wading in shallower water compared to OG (#5574, regression from 1.3)
- fixed Lara shifting too far down in 60fps after grabbing a ladder (regression from TR2X 0.10)
- fixed the lighting on the submarine at the start of 40 Fathoms not flickering as it falls (regression from 1.2)

**TR3**:
- added the ability to define the flame interval of `O_FLAME_EMITTER_SIDE` objects
- added an option to fix animated spikes resetting when loading a save (Gameplay → Fixes → Fix animated spikes)
- improved the scaling of the grenades item inside the inventory
- changed spark effects to no longer cap their screen size, so they scale more naturally when zooming in photo mode
- changed spark effects to use `O_SPARKS_GFX` rather than `O_EXPLOSION_1`
- changed piranhas, tropical fish and bat emitters to use dedicated sprite objects; refer to migration guide
- changed artefact pickup end-level behavior in specific levels to be configurable via Lua
- changed `O_ORCA` special collision in `Sleeping with the Fishes` by introducing a `O_DOLPHIN` instead
- changed lava swamp burning behavior to no longer reference specific levels
- changed the longer `O_FLAME_EMITTER_SIDE` interval in level 7 to be configurable via Lua
- changed the strobe light alarm behavior in level 15 to be configurable via Lua
- changed animated spikes sound effects in specific levels to use animation commands instead
- changed animated spikes to work outside levels 5 and 7
- changed `O_AI_PATROL_1` behavior in High Security Compound and Area 51 to be configurable; refer to migration docs
- removed the hard-coded Aldwych drill speed override and moved it to Lua instead
- removed the option to fix animated sprites, which has no relevance in TR3/TR3LA
- fixed handheld flare sparks appearing inside the flare instead of above the tip
- fixed TR3:LA referring to a non-existing FMV in the startup logs
- fixed texture bleeding and missized textures on piranhas, tropical fish and bats
- fixed Security Guards 81 and 212 in Thames Wharf and 17 in Lud's Gate not following their patrol paths correctly (OG bug)
- fixed bats disappearing after flying out of the room where they spawned
- fixed the drill in the Shakespeare Cliff not rotating
- fixed the animating diver in Sleeping with the Fishes remaining visible in a default pose after diving into the water
- fixed the animating diver in Sleeping with the Fishes disappearing before jumping into the water
- fixed seeing the fish in Coastal Village spawn due to delayed triggers, most noticeable when fades are disabled
- fixed hissing SFX in RX-Tech Mines and water SFX in Sleeping with the Fishes not looping properly
- fixed visible gaps in Nevada room 31 where the waterfalls meet the ceiling below the water line
- fixed dropped pickups not keeping their original facing (regression from 1.2)
- fixed Sophia not aiming at Lara properly in Reunion (regression from 1.4)
- fixed Lara becoming immune to Puna's attacks after getting zapped once with the fly cheat (regression from 1.3)
- fixed Lara snapping to keyholes, puzzle slots and switches instead of waiting to be at a complete stand-still first (regression from 1.1)
- fixed wasps that spawn from emitters having incorrect shading, especially noticeable when they are killed (regression from 1.6)
- fixed Lara shifting too far down in 60fps after grabbing a ladder (regression from 1.1)



## [1.7.1](https://github.com/LostArtefacts/TRX/compare/trx-1.7...trx-1.7.1) - 2026-05-28
- added `O_FLARE_ITEM.burn_time` so custom levels and Lua scripts can change how long flares last (#5544)  
  Example: `trx.objects[trx.catalog.objects.flare_item].properties.burn_time = 1`
- changed Lua object and item properties to accept whole numbers for decimal values
- changed Lua object access to support catalog names such as `trx.objects.flare_item`
- fixed string-backed configuration settings having wrong values on first launch
- fixed Puna crashing if there is no Lizard to summon present in the same room
- fixed a potential crash when switching to games that have camera edit injections
- fixed weapons gaining extra ammo after finishing a level (regression from 1.6)

**TR2**:
- changed the FMV path resolution to prioritize .avi over .rpl

**TR3**:
- changed the FMV path resolution to prioritize .avi over .rpl
- fixed pickup aids appearing from hidden overlapping rooms



## [1.7](https://github.com/LostArtefacts/TRX/compare/trx-1.6...trx-1.7) - 2026-05-25
Showcase: https://youtu.be/L1g4tavx23Y
- added object and item property tables for Lua scripts, including `max_hit_points` defaults
- added `trx.events.before_item_setup` and `trx.events.after_item_setup` for Lua scripts to adjust item properties before items are initialized
- added the ability for custom levels to use more than one of TR3's vehicle types in the same level; define the following animation objects for Lara:
    - `O_LARA_QUAD_BIKE`
    - `O_LARA_MOUNTED_GUN`
    - `O_LARA_KAYAK`
    - `O_LARA_UPV`
    - `O_LARA_RIB`
    - `O_LARA_MINE_CART`
- added Lara's Antarctica beta outfit (#5190)
- added support for heavy switch, heavy antitrigger, crouch, climb, and monkeyswing trigger types in custom levels
- improved pathfinding checks to guard against potential crashes when enemies are in invalid locations
- improved vaulting logic for Lara, allowing her to grab the lowest reachable ledge in front of her when only action is held, regardless of room layout (#5205)
- improved crouch turning for Lara to allow for more responsive transitions to crouch idle, crouch roll, and crawl idle states
- changed weather to spawn at the camera's position when fixed cameras are in use (#5516)
- changed `/set` to refuse level-enforced settings unless `--force` is used
- changed ally kills to optionally count towards the kill total (Gameplay Options → General → Count ally kills)
- changed environment kills counting towards the kill total to be optional (Gameplay Options → General → Count environment kills)
- changed the underwater crawling fix to be optional (Gameplay Options → Fixes → Fix underwater crawling)
- changed max upscaling factor from 8 to 10 (#5347)
- changed Lua item maximum HP setup to use `item.properties.max_hit_points` instead of `item.max_hit_points`
- changed flooding and drowning enemies to contribute towards kill count
- fixed `trx.game.current_level` in Lua being offset by one (#5444)
- fixed potential crashes in old custom levels that contain invalid room visibility portals (#5447)
- fixed transparent pixels on TR3 outfit heads when bilinear filtering is enabled (#5438)
- fixed transparent pixels on the TR2 Bomber Jacket outfit sleeve
- fixed passport dialog appearing off-center when showing long save names (#249)
- fixed stutters on certain levels with VSync off on some GPUs (#4975, regression from 1.0)
- fixed demos being able to overwrite gameplay settings when changing other options during playback (regression from TR1X 4.14 / TR2X 1.4)
- fixed replay scenario files saved with a UTF-8 signature not being read correctly
- fixed Lara getting stuck in void after loading saves made while hanging from ladders (regression from 1.2)
- fixed Lara not being able to crawl in one-click water rooms (regression from 1.6)
- fixed water height checks when `Fix wall geometry` is not enabled (regression from 1.6)
- fixed level music tracks continuing to play when returning to the main menu if the option not to play the title music is enabled (#5470, regression from TR1X 4.13 / TR2X 1.3)
- fixed being unable to smash items that are placed inside geometry but have bounds that extend into regular space (#5483, regression from 1.3)
- fixed a potential crash when colliding with items that have no animation data (#5488)
- fixed image files not falling back to other supported formats
- fixed Lara not being able to use keys/puzzles if animated interactions are enabled and previous pickup attempts have failed due to other object collision (#5496)
- fixed a potential crash if 3D pickups are enabled but an item's 3D model isn't loaded or contains no animation data
- fixed certain traps triggering Lara's healthbar when she's invulnerable
- fixed Lara's right arm twitching at the end of the crouch roll animation (#5531)

**TR1**:
- added support for reverb in custom levels
- added support for the Mine Cart in custom levels
- fixed a stray face on the Magnums model (#2073)
- fixed the game freezing if Lara tries to pick up a flare while crouched (#5512, regression from 1.3)

**TR2**:
- added support for reverb in custom levels
- added support for the Mine Cart in custom levels
- fixed transparent pixels on the CD player in each level (#4072)
- fixed a stray face on the Automatic Pistols and Uzis models (#2073)
- fixed transparent pixels on the Automatic Pistols in Golden Mask levels (#2073)
- fixed the menu SFX not playing when opening the Controls option (#5476, regression from 1.0)
- fixed Lara entering fast fall speed slightly later than OG when dropping from a ledge (regression from TR2X 1.2)

**TR3**:
- added support for the `/teatime` console command
- changed Quad Bike level 3 (The River Ganges) music tracks to be no longer hardcoded; moved the setup to LUA
- fixed all skyboxes being much too bright and not being affected by the gamma option
- fixed harsh lighting transitions on vertices using glow effect
- fixed a missing texture on Winston's nose
- fixed missing "The End" text in the first end credit image (#5441)
- fixed Hand of Rathmore rotating in Sleeping with the Fishes
- fixed the Circuit Bulbs in Sleeping with the Fishes not rotating on a central axis in the inventory
- fixed the inactive seaweed at the start of Sleeping with the Fishes
- fixed the light beams in the cutscene before Meteorite Cavern being clipped with low draw distance values (#5440, #5372)
- fixed missing and misaligned ropes in RX-Tech Mines where the submarine is lowered (#5432)
- fixed several incorrect and missing textures in RX-Tech Mines rooms 2, 7, 22, 32, 69 and 70
- fixed the camera being cut off early after placing the Oceanic Masks in Lost City of Tinnos (OG bug)
- fixed the push button mesh being offset too far from the wall in each level it appears (OG bug)
- fixed transparent pixels on the CD player in each level (#4072)
- fixed a missing face on the push button in Antarctica (#5428)
- fixed the push buttons in Lost City of Tinnos rotating oddly when used
- fixed several missing, stretched and misaligned textures in Lara's Home (#4890)
- fixed the briefcase in the cutscene following Antarctica jumping positions at various points in the scene (#5430)
- fixed Punks repeating their alert sounds and not targeting Lara in all cases (#5456, regression from 1.6)
- fixed the boat (RIB) briefly having an underwater hue when Lara first climbs on (OG bug)
- fixed height checks that could make Lara refuse to dismount the Mine Cart in custom levels
- fixed the Strobe Light being clipped out of view too soon in several levels (missing animation bounds) (regression from 1.2)
- fixed the menu SFX not playing when opening the Controls option (#5476, regression from 1.1)
- fixed Lara entering fast fall speed slightly later than OG when dropping from a ledge (regression from 1.1)
- fixed the MP5 dealing slightly less damage than OG (regression from 1.1)
- fixed MP 2 and RX Worker 1 not firing at Lara when starting to walk towards her (OG bug)
- fixed incorrect textures in It's a Madhouse! room 97 and missing/incorrect textures on lamps and orbs
- fixed incorrect texture sounds in It's a Madhouse rooms 66, 69 and 133
- fixed floating spikes in It's a Madhouse! (#5520)
- fixed a missing alarm sound in Shakespeare Cliff (#5517)
- fixed incorrectly rotated textures in Shakespeare Cliff rooms 22/23
- fixed moving geometry in Highland Fling room 128
- fixed incorrect textures in Willard's Lair rooms 11, 52 and 56
- fixed being unable to obtain the final secret in Aldwych due to Punk behaviour (#5533, regression from 1.6)
- fixed Punks 190 and 199 having swapped fire sticks (#5533, regression from 1.4)
- fixed incorrectly defined dummy triggers in certain old custom levels conflicting with normal triggers (#5537, regression from TR1X 4.8)



## [1.6](https://github.com/LostArtefacts/TRX/compare/trx-1.5...trx-1.6) - 2026-05-02
Showcase: https://youtu.be/yTW99iecK3U
- added on-screen touch controls with a virtual D-pad and action buttons, configurable opacity/scale/deadzone, dynamic button glyphs, and full remap support including button macros (Gameplay Options → Controls → Touch controls)
- added the ability to do a forward roll without releasing sprint first (#5270)
- added the ability for Lara to align herself with floor tilts when crawling (Gameplay Options → Controls → Crawl tilt) (#4945)
- added the ability to turn off or censor blood effects (Graphic Options → Visuals → Blood effects)
- added the ability to delete saves directly from the passport save and load screens (#5309)
- added the ability to pause FMVs with the Pause input (#1754)
- added an option to stop the game from advancing when the window loses focus (Gameplay Options → General → Pause when focus lost) (#3978)
- added `O_DISPOSABLE_ANIMATING_1`...`O_DISPOSABLE_ANIMATING_10`, which will behave like regular animating objects but are removed from being drawn when deactivated
- added an option to fix inaccurate wall geometry in original levels (Gameplay Options → Fixes → Fix wall geometry)
- added an option to control how land creatures behave in water (Gameplay Options → General → Creature drown policy) (#5387)
- improved weapon setup so picking up a weapon can now give a different amount of ammo than picking up its matching ammo item (#5352)
- improved `--level PATH` so it accepts relative paths and reports clearer startup errors when the level cannot be launched
- improved savegame loading if item counts have changed between making the save and loading it
- changed the PS1 crystal tint option to take effect without having to reload the level
- changed the `ITEM_ACTION_FLOOD` sound effect to play when underwater rather than only when above water
- fixed max stats not refreshing after changing unobtainable pickups, kills, or secrets in the gameflow
- fixed TR1 and TR2 camera modes potentially going out of bounds in some rare scenarios
- fixed Lara rapidly switching animations when shimmying across the top of a ladder (#5295)
- fixed Lara being able to crawl and crouch-roll too far into water from land
- fixed incorrect transparent and yellow pixels on TR2 and TR3 outfit heads when bilinear filtering is enabled (#5300)
- fixed rotating 3D pickup notifications following the UI filter setting instead of the in-game texture filter
- fixed climbing issues on ladders that are against walls that (incorrectly) contain tilt data within them (#5304, regression from 1.1)
- fixed Lara not transitioning immediately to run after vaulting two clicks when forward is held (#5305, regression from 1.3)
- fixed settings list auto-scroll sometimes stopping after switching to a different mod (regression from 1.4)
- fixed first-time keyboard keybindings conflicting after switching to a different mod (regression from 1.4)
- fixed boulders stopping too soon on some slopes with low ceilings (#5337, regression from 1.2)
- fixed persistent damage restoring Lara to full health after inter-level cutscenes (#5364, regression from 1.2)
- fixed missing default object names for `O_ANIMATING_7`...`O_ANIMATING_10` and `O_FLICKERING_LIGHT` (regression from 1.4)
- fixed empty centaur statues incorrectly referencing other level items when the centaur object is not loaded
- fixed TR3 camera mode potentially behaving erratically when loading a level and the look input is held (regression from 1.1)
- fixed Lara being able to go from run or sprint to crouch while holding a rifle-type weapon (regression from 1.3)
- fixed Lara not using hit animations when she is struck while in the crouch idle state (regression from 1.1)
- fixed compatibility of Linux releases with Debian-based distros

**TR1**:
- added savegame crystals to Unfinished Business (#1525)
- fixed not being able to hear the flood/drain sound effect when using the lever in Tomb of Tihocan room 23
- fixed transparent pixels on Lara's pistols
- fixed missing textures in Tomb of Qualopec rooms 42 and 12, and similarly in cutscene 1 rooms 3 and 6
- fixed z-fighting and transparent pixels on static meshes in Palace Midas room 69
- fixed an incorrect texture in City of Khamoon room 69
- fixed vertex shading in Obelisk of Khamoon room 19
- fixed Lara not stopping against one-click raised slopes (#5400, regression from 1.4)
- fixed Lara attempting to vault onto steep slopes when running into them with action held (#5400, regression from 1.4)

**TR2**:
- added savegame crystals to base levels and The Golden Mask
- added an option to disable body bag triggers, so that killed enemies will always be visible
- fixed missing faces on the lanterns in Venice
- fixed z-fighting on static meshes in Opera House room 105
- fixed texture inconsistencies on the Airplane Propeller between Opera House and Offshore Rig
- fixed misaligned ice static meshes in Tibetan Foothills room 130
- fixed an incorrect texture in The Cold War room 6
- fixed Lara not stopping against one-click raised slopes (#5400, regression from 1.4)
- fixed Lara attempting to vault onto steep slopes when running into them with action held (#5400, regression from 1.4)
- fixed loose Harpoon ammo count displaying 2 for each pickup instead of 3 (OG bug)

**TR3**:
- added Boat (RIB) control
- added Mine Cart control
- added Willard control
- added RX-Tech Worker 1 control
- added RX-Tech Worker 2 control
- added RX-Tech Worker 3 control
- added Crawler Mutant control
- added Dying Mutant control
- added Hybrid Mutant control
- added Wasp Mutant control
- added Wasp Mutant Emitter control
- added Claw Mutant control
- added Fire Head control
- added Disposable Animating control (Tinnos light shaft)
- added an option to disable body bag triggers, so that killed enemies will always be visible
- restored the animated mine cart tracks in RX-Tech Mines
- restored the missing flamethrower blast sound effect in RX-Tech Mines and Meteorite Cavern
- changed Sophia's final height to follow the level setup instead of using a fixed value
- removed Lara's Home from TR3:LA to stay compatible with the OG and other expansion packs
- removed the need to define Wasp items near to Wasp Emitters
- fixed Wasp Emitters not spawning items if previously spawned Wasps are still active and unreachable
- fixed letterboxing of images on 16:10 resolution
- fixed TR3:LA images missing from release zips and the installer (#5390, regression from 1.5)
- fixed Fire Lighting option having no effect
- fixed missing conveyor belt animations in High Security Compound
- fixed delayed lighting updates on Lara during movement, particularly noticeable on ladders (regression from 1.1)
- fixed z-fighting in High Security Compound rooms 135/179
- fixed transparent and magenta pixels on grating textures in High Security Compound and Area 51
- fixed an incorrect window texture in High Security Compound room 105
- fixed the satellite dish in High Security Compound room 44 being clipped out of view too soon (missing animation bounds) (#5297, regression from 1.1)
- fixed It's a Madhouse! street lamps being too bright
- fixed menu artefacts not appearing in consistent positions between levels
- fixed TR3:LA playing TR3 intro FMV
- fixed Coastal Village and Lost City of Tinnos having the wrong pickup count
- fixed Willard's Lair having wrong kill count
- fixed Reunion having wrong pickup and secret count
- fixed being able to re-use switches that are intended to only be used once (#5328, regression from 1.1)
- fixed capitalization of the "Empty Slot" text in passport
- fixed the second boulder at the beginning of Reunion stopping too early (regression from 1.2)
- fixed Willard increasing the kill count each time he collapses (OG bug)
- fixed Wasp Emitters generating too many spawns if activated from non one-shot triggers and the player stands for too long on the trigger
- fixed Lara attempting to vault onto steep slopes when running into them with action held (#5400)
- fixed Lara being unable to pull up on specific ledges near walls that have invalid triangles within them (regression from 1.1)
- fixed potential crashes when using grenades on enemies in levels that use the body bag feature (#5378, regression from 1.1)
- fixed activated one-shot antitriggers not being remembered when loading a save (regression from 1.2)
- fixed the Ora Dagger appearing too low in the inventory in Meteorite Cavern (regression from 1.2)
- fixed quest item pickup counts incrementing if the item cheat is used and then save/load is repeatedly used (regression from 1.2)
- fixed not being able to give quest items to Lara on level start via the game flow
- fixed inaccurate maximum pickup count in Lud's Gate
- fixed the Assault Course timer not stopping after playing the Race Track Course first (regression from 1.1)
- fixed friendly Punk behaviour when looking at Lara (regression from 1.4)
- fixed Punks leaving AI Guard positions when hostility policy is set to individual and Lara hurts a different Punk (regression from 1.4)
- fixed patrolling Monkeys becoming agitated when Lara hurts a different Monkey, but remaining fixated on their AI targets instead of Lara (regression from 1.2)
- fixed UPVs not being able to fire if Lara has harpoons but has not picked up the Harpoon Gun yet (#228, regression from 1.4)
- fixed loose Harpoon ammo count displaying 2 for each pickup instead of 3 (TRX bug)



## [1.5](https://github.com/LostArtefacts/TRX/compare/trx-1.4.2...trx-1.5) - 2026-04-04
Showcase: https://youtu.be/TTlajgcM9-8
- added multi-key combo shortcuts (up to 3 keys) with two binding slots per action for both keyboard and controller
- added remembering of the last played mod
- added a new console command, `/tp enemy`, to cycle Lara through hostile creatures in the current level
- added a new animation command, `ITEM_ACTION_TURN_90`, which will rotate the affected item 90°
- added dynamic mod discovery from the games/ directory using new `extends` and `name` fields in gameflow.json5
- added expanded statistics screen customization, including per-row toggles and a choice between bare and bordered layouts (Graphic Options → Stats)
- added an option to show or hide the version text in the title inventory ring (#5235)
- added optional save/heal crystal counts to level and final statistics (#5180)
- added the ability for security lasers to activate heavy triggers when tripped by Lara (#5225)
- added `trx.camera.reset()` to Lua, which will reposition the camera based on Lara's position
- added an option to disable cinematics at the start of levels (Offshore Rig, Home Sweet Home and High Security Compound) (Gameplay → General → Cinematics) (#5284)
- improved rendering line segments (poison darts, rain drops, SWAT laser sights)
- changed `--level` to no longer require `-e/--engine` to work
- changed background images on title, inventory, and statistics screens to always use smooth bilinear filtering instead of the pixel-sharp look
- fixed recordings keeping unbound hotkeys active during playback
- fixed being unable to change FOV after using photo mode without restarting the level (#5246, regression from TR1X 4.15)
- fixed Lara getting stuck if using crouch-roll near very low ceilings (#5248)
- fixed Bell in room 48 being shootable from room 55 again (#4949, regression from TRX 1.4 Sophia Reunion targeting fix)
- fixed certain TR1 1.1 savegames refusing to load (#5252, regression from 1.2)
- fixed the total kill count including allies if hostility policy is set to individual and Lara shoots a hostile enemy (#5255, regression from 1.2)
- fixed quick-load remaining unavailable while Lara is in her death animation (#5264, regression from 1.3)
- fixed destroying the Fuse Box to defeat Sophia in City and Reunion not counting as a kill in the level statistics
- fixed potential freezing issues after moving an item to a different room via Lua
- fixed crash when issuing `/mod tr1-ub` when playing late TR1 levels
- fixed Lara being unable to draw weapons if animated interactions are enabled and she is hit by an enemy while moving towards a pickup (#5288, regression from TR1X 4.13)
- fixed interaction issues with pushblocks when in shallow water (regression from 1.0)

**TR2**:
- changed the Detonator Box to no longer hard-code dynamic light output; refer to the migration guide for custom levels
- fixed the total possible kill count in Furnace of the Gods being inaccurate if the monks are attacked (#5229)

**TR3**:
- added Security Laser (Alarm) control
- added Security Laser (Damage) control
- added Security Laser (Kill) control
- added Rotating Laser control
- added Sentry Gun control
- added Civilian control
- added Detonator Box control
- added Prisoner control
- added MP 1 control
- added MP 2 control
- added Orca control
- added Area 51 Rocket control
- added Hook control
- added pickup aids (Graphic Options → Visuals → Pickup aids) (#5239)
- added high-resolution 16:9 and 4:3 images for TR3:LA  
    To download the new images ahead of a stable release, please see the [TRX data](https://github.com/LostArtefacts/TRX-data) repository.
- restored the cutscene at the beginning of High Security Compound
- changed Area 51 Rocket to no longer hardcode room 52 as the fire blast room; instead it checks for presence of an upwards portal pointing to the rocket room
- changed enemies who are killed by lasers to be included in the stats
- fixed Sophia's staff having a shadow in the City cutscene
- fixed some doors having a bad rotation when closing, mostly visible when using the door cheat
- fixed the Area 51 sliding doors being offset too far from the floor
- fixed bad vertices in staircase static meshes in Aldwych and Lud's Gate, allowing for visible gaps in geometry (#5182)
- fixed bad positioning of light static meshes in Aldwych that could result in Lara not grabbing certain ledges (#5181)
- fixed several missing textures in Lud's Gate room 77
- fixed Tony's fireballs flying the wrong way and piling up after loading a save (regression from 1.1)
- fixed rockets exploding underwater being able to create a water splash in the wrong place (regression from 1.1)



## [1.4.2](https://github.com/LostArtefacts/TRX/compare/trx-1.4.1...trx-1.4.2) - 2026-03-24
- fixed 3D pickups and inventory ring view still affected by fog (regression from 1.4)

**TR2**:
- fixed the Detonator Box being difficult to activate when selecting the key manually from the inventory (#5215, regression from TR2X 1.3)
- fixed the Detonator Box rotating if Lara interacts with it but then doesn't pick the key from the inventory (#5215, regression from TR2X 1.3)



## [1.4.1](https://github.com/LostArtefacts/TRX/compare/trx-1.4...trx-1.4.1) - 2026-03-23
- fixed Lara using the ladder-to-crouch animation in some rare cases despite there being headroom in front of her (regression from 1.2)
- fixed toggle-sprint key failing to cancel sprint mid-run (#5174, regression from 1.4)
- fixed toggle-duck key failing to keep Lara ducked in run-to-duck and sprint-to-duck paths (#5177, regression from 1.4)
- fixed flipped state of "Pause music in inventory", changed to "Enable music in inventory" (regression from 1.4)
- fixed final statistics in City of Khamoon counting the optional PS1 mummy when Restore PS1 enemies is disabled (#5188, regression from 1.1)
- fixed bouncy grenades getting stuck in certain geometry (#5202, regression from 1.2)
- fixed thrown flares getting stuck in certain geometry (#5202, regression from 1.4)

**TR3**:
- fixed door 34 in Thames Wharf closing permanently during the flipmap puzzle (#5170, regression from 1.1)
- fixed Lara being unable to move after grabbing ladders in specific geometry (#5169, regression from 1.1)
- fixed a missing camera shake effect in room 135 in Aldwych (#5183)
- fixed a missing sound effect during the flip map in the Egyptian room in Lud's Gate (#5183)
- fixed potential framerate drops during audio playback when no active sound effects are playing (regression from 1.3)
- fixed some enemies automatically being hostile when triggered if other enemies have been killed (#5203, regression from 1.1)



## [1.4](https://github.com/LostArtefacts/TRX/compare/trx-1.3.1...trx-1.4) - 2026-03-21
Showcase: https://youtu.be/8SavYv2SawI
- added an option to let Lara stay crouched without holding the button (Gameplay → Controls → Toggle crouch) (#5006)
- added an option to let Lara keep sprinting without holding the button (Gameplay → Controls → Toggle sprint) (#5006)
- added three additional outfits for Lara
- added a new console command, `/mod {name}`, to switch between installed game/mod packs without relaunching
- added a new option in the New Game dialog, "Switch Game", to switch between installed game/mod packs without relaunching
- added experimental support for config presets (Gameplay Options → Presets)
     Currently very basic presets available only – looking for help with improving them :)
- added new backgrounds to Inventory Ring / Pause screen / Stats screen styles:
    - Transparent: like TR2 PS1 Pause Screen
    - Black: like the Remasters
    - Monochrome (cool): like TR3 PS1 Inventory Screen
    - Monochrome (warm): like TR3 PS1 Pause Screen
- added support for TR4-style trigger-triggerers (named `O_TRIGGER_GATE` in TRX) for custom levels
- added support for `.wma` music files for broader custom level compatibility
- added support for `.ogv` and `.fmv` FMV extensions, with `.ogv` preferred over `.fmv` for remaster compatibility
- added audio fallback for FMV files that lack an audio stream (e.g. remastered `.ogv`), probing alternative extensions for a companion audio track
- added an option to move Ammo counter location (Graphic Options → UI → Ammo counter location) (#5076)
- added simple level-load caching by introducing a `cache/` folder
- added an option for Lara to wear semi-transparent sunglasses (Graphic Options → Lara's sunglasses)
- added four additional general animating object slots, `O_ANIMATING_7` to `O_ANIMATING_10`
- added `O_FLICKERING_LIGHT`, which is similar to `O_ELECTRICAL_LIGHT` but is permanently flickering
- improved level loading times by 15%
- improved FMV audio to play through the game's existing audio mixer instead of opening a separate audio device
- changed the reflections option to be available in all game modes (Graphic Options → Enable reflections)
- changed the delay in performing a running jump by one frame less, when jump lock mode is set to disabled (Gameplay → Controls → Jump lock mode) (#3841)
- fixed High lighting contrast not attenuating brightness properly in TR1 and TR2 (regression from 1.1)
- fixed the photo mode red frame not covering the full screen when using integer upscaling
- fixed boulders that have moved vertically reactivating for a frame after loading a save (regression from 1.2)
- fixed low fog distances affecting 3D pickups and inventory ring view
- fixed cheat and weapon hotkeys affecting gameplay during demos (#5163, regression from 1.0)

**TR1**:
- added the ability to use flames on Pendulums, similar to TR3
- changed skyboxes in TR1 to be drawn only if the appropriate room flag is set
- changed the scion pickup in Sanctuary of the Scion to not be displayed on-screen briefly before the level ends (#3682)
- fixed Scion taking damage before activation (regression from 1.0)

**TR2**:
- added the ability to use flames on Pendulums, similar to TR3
- changed demos to show accurate gun meshes before Lara draws her pre-selected weapon (#3585)
- fixed Bartoli appearing frozen towards the end of the Opera House cutscene
- fixed pulling the Dagger of Xian from dragon's corpse not counting as a pickup (regression from 1.0)
- fixed thrown flares falling through trapdoors and becoming stuck in the void if thrown underwater near the floor (#3708)
- fixed flamethrowers and Dragon's breath doing weird animation when hitting floor (#5104, regression from 1.3)
- fixed yetis dealing no damage during their charge attack (#5126, regression from TR2X 0.8)

**TR3**:
- added UPV control
- added Train control
- added Patrol Dog control
- added Crow control
- added Sophia control
- added Fuse Box control
- added Electric Cleaner control
- added Punk control, including `O_PUNK_2`, which was unused in OG
- added Security Guard control
- added Propeller control
- added SWAT control
- added Diver control
- added Pendulum control
- added 60 FPS interpolation to:
    - sparks
    - weather effects
    - water effects
    - wake effects
    - explosion rings
    - bat emitters
- added PSX-style underwater water particles to Madubu Gorge, Aldwych, and Lud's Gate
- changed Ammo Counter to appear in red when the Menu Style is set to PS1
- changed Punks to have friendliness assignable through Lua, so removing the hard-coded behaviour in the Lud's Gate level sequence
- changed Trains to no longer hard-code speed based on the level number and instead take it from their default animation
- changed Pendulums that have flames to be setup by placing a flame emitter at the same position, rather than setting the item's timer via its trigger
- changed Meteorite Artefacts to be exempted from drop tile centering
- fixed a soft lock preventing Lara from picking up the artefact, when saving/loading during boss explosion sequence (regression from 1.2)
- fixed the helicopter in Highland Fling briefly disappearing when crossing room portals (regression from 1.1)
- fixed Lara dying from touching Trains that haven't yet been activated
- fixed harpoons from Divers not spawning blood when they hit Lara
- fixed `O_KILL_ALL_TRIGGERED` removing unused Save Crystals (#5035)
- fixed TR1/TR2-only options showing up in TR3 gameplay settings (#5055)
- fixed Lara stopping against one-click raised slopes when running instead of beginning to slide (#5038)
- fixed rain not spawning in outside rooms in the Thames Wharf cutscene
- fixed the punk in the cutscene before Lud's Gate walking through a wall
- fixed Lara appearing frozen at the beginning of the cutscene before City
- fixed incorrect texturing on the fish in City
- fixed the Eye of Isis not showing in the inventory in All Hallows
- fixed too low volume in all FMVs (except logo which used a different codec)
- fixed Lara by default being unable to climb out of water onto steep slopes (change manually in Gameplay → Fixes → Fix water exit)
- fixed thrown flares falling through trapdoors (regression from 1.1)
- fixed some level textures appearing slightly misaligned on room geometry
- fixed potential AI behavioural differences in the South Pacific Mercenary (regression from 1.2)
- fixed smoke from Lara's guns persisting between levels (regression from 1.1)
- fixed sound effects potentially playing after completing a level and entering into the globe select screen (regression from 1.2)
- fixed audio lag and framerate drops near the end of an audio track that's playing when no active sound effects are playing (#5167, regression from 1.3)



## [1.3.1](https://github.com/LostArtefacts/TRX/compare/trx-1.3...trx-1.3.1) - 2026-03-11
- fixed main.sfx resolution being enforced (regression from 1.3)
- fixed the microphone entering underwater mode too eagerly when `Microphone near Lara` is enabled (#5057, #4888)
- fixed save counters sometimes drifting after dying and reloading (#5054, regression from TR1X 4.9 / TRX 1.0)
- fixed flare and gun flash being drawn with a water tint when in shallow water regardless of responsive tint option (#5072, regression from 1.2)
- fixed fade transitions using the wrong picture size when upscaling or borders are enabled (#5081, regression)

**TR2**:
- fixed guns as secret rewards not being converted to the equivalent ammo if Lara already has the gun

**TR3**:
- fixed reverb affecting inventory ring sounds (#5056)
- fixed Pause text color
- fixed the secret sound not playing in some installations, whereby `cdaudio.wad` contains invalid track sizes (#5049)
- fixed mounting a UPV causing Lara's braid to stand upright (OG)



## [1.3](https://github.com/LostArtefacts/TRX/compare/trx-1.2.2...trx-1.3) - 2026-03-06
Showcase: https://youtu.be/FgB9JgDM65E
- added the ability to freely rotate examinable items
- added a color editor dialog for fog and water colors in Graphic Options → Visuals
- added an option for Lara to wear sunglasses (Graphic Options → Visuals → Sunglasses) (#4869)
- added `O_SWITCH_TYPE_WHEEL`, which is similar to `O_SWITCH_TYPE_AIRLOCK` but can be used more than once
- added `O_SMASH_OBJECT_3`, which can only be broken with triggers or the Crash Site gun
- added `O_SMASH_OBJECT_4`, which behaves like `O_SMASH_OBJECT_1` but uses `SFX_SHUTTERS_BREAK`
- added `O_TREX_ALPHA`, which can target raptors and be distracted by flares
- added the ability to trigger dragons independently of Bartoli in custom levels (#5011)
  - place an `O_DRAGON_BACK` item in the editor and trigger it normally
  - the dragon will spawn immediately when triggered and will be one-phase, so no dagger needs to be pulled
- added a new Lua item query helpers, `trx.items.find(query)` and `trx.items.first(query)`, with support for `object_id` and `room_num` filters
- added a new Lua catalog, `trx.catalog.weapons`, for weapon identifiers
- added a new Lua property, `trx.lara.equipped_gun`, to read Lara's currently equipped gun type
- added a new Lua property, `trx.lara.target`, to read Lara's current locked target item
- added a new Lua property, `trx.Item.flags`, to read current item flags (related to triggers)
- added a new Lua property, `trx.Item.timer`, to read current item timer value (related to triggers)
- added support for using more sound slots than originally possible in custom levels (#3898)
- added support for actual quick saves with round-robin quicksave slot cycling. (#1897)  
  Note: This feature is disabled by default and needs the player to manually bind new inputs.
- added quick-save/load command aliases:
  - `/save quick`, `/quicksave`, `/qs`
  - `/load quick [slot]`, `/load q[slot]`, `/quickload [slot]`, `/ql [slot]`
- added blood effects when enemies shoot any other creature (not just Lara)
- added support for the globe-style level selection mechanic in the new game for level builders (#4920)
- added an option to control how Lara swings on thin ledges (Gameplay → Controls → Slow ledge swing) (#3341)
- added `/tp precise {x} {y} {z}` to teleport using raw world-space coordinates (no `/1024` scaling – matches TRView)
- added the ability to use glide cameras when using TR3 camera mode
- added an option to toggle glide cameras (Graphic Options → Visuals → Glide cameras)
- improved error reporting for gameflow issues to now display full key paths for faulty nodes
- changed PC and PS1 UI colors to no longer be hardcoded by moving it to `ui.json5` (#5003)
- changed Fog start and Fog end to change by 10 by default, with Slow allowing 1-step precision (#5015)
- changed `O_WINDOW_1` and `O_WINDOW_2` to `O_SMASH_OBJECT_1` and `O_SMASH_OBJECT_2` respectively
- changed `O_MINI_COPTER` to no longer hardcode direction
- changed Earthquake to support being reset
- changed loading screens setting to use modes (`disabled`, `always`, `new-games`). Previously, they were hardcoded to not show for saves (#1290)
- changed logs to no longer emit ANSI color characters when the game's output is piped to a file / process
- changed the degenerate static mesh collision check to only apply when all axes have an empty size
- fixed Lara teleporting after vaulting 2 or 3 clicks when there is a room below the target position that has no immediately adjoining portal (#4530)
- fixed Lara attempting to jump up (using action) despite the ceiling above her making it impossible to grab any ledge (#3558)
- fixed Lara not being able to grab ledges when under low ceilings (#4093)
- fixed Lara sometimes falling when vaulting 2 or 3 clicks onto a ledge that has triangulation
- fixed NG+ always forcing Lara's default equipped gun at level start even when "remember guns between levels" is enabled (#4711)
- fixed not restoring Lara's back weapon mesh between levels when "remember guns" is enabled and a rifle-type weapon is equipped at level end
- fixed a missing footstep sound when Lara starts to sprint
- fixed Lara's flare undraw animation being skippable on specific late draw frames (#1593)
- fixed UI bar scale option not updating the padding and borders (regression from 1.2)
- fixed Blade stopping in the wrong position when anti-triggered (#4894)
- fixed very distant Boulders causing camera shake (similar to the Tihocan crocodile targeting bug)
- fixed drawing debug triggers using wrong orientation in some triangular geometry
- fixed heavy triggers with no `TO_TARGET` / `TO_CAMERA` resetting cameras
- fixed Lua `trx.catalog` only exposing `objects` and `flip_effects`; it now also exposes `lara_states`, `lara_anims`, `music`, and `samples`
- fixed a freeze if firing a grenade very close to room portals (#4938, regression from 1.2)
- fixed non-deterministic Inventory Ring control (transition speeds depended on v-sync / wall clock timing)
- fixed game logic speeding up while the game was fading out after quitting
- fixed Lara being able to shoot smashable objects located in unreachable overlapping rooms (#4949, regression from TR1X 4.14 / TR2X 1.4)
- fixed touching Lava Wedges causing endless Flame effect spawns when the immunity cheat is on
- fixed touching Lava tiles causing reduced Flame effect when the immunity cheat is on
- fixed collision issues on bridges, trapdoors, breakable tiles and pushblocks if positioned over a triangle portal (regression from 1.0)
- fixed Lara being able to sprint through swamps when responsive sprinting is enabled
- fixed bar borders scaling poorly (off-by-1px errors, regression from 1.2)
- fixed death counter not being preserved in saves after changing levels, causing stopwatch and final statistics to sometimes show 0 deaths

**TR1**:
- added an option to allow Lara to crouch and crawl (Gameplay → Controls → Crawling)
- added support for monkey bars
- changed Lara to be able to grab ealier when performing forward jumps, like TR3
- fixed a very rare case of raptors using an incorrect death animation
- fixed Lara unable to run around in random spots at the bottom of The Great Pyramid's starting pit

**TR2**:
- added an option to allow Lara to crouch and crawl (Gameplay → Controls → Crawling)
- added support for monkey bars
- changed Lara to be able to grab ealier when performing forward jumps, like TR3
- removed the requirement to use `main.sfx` in custom levels (#3898)
- fixed secret reward in Venice giving Magnums ammo instead of Automatic Pistol Clips (#4951, regression from 1.1)
- fixed flickering switches and spike ceilings in Temple of Xian and Floating Islands (#4874)
- fixed Airlock door handles not getting drawn from certain angles (#4886, regression from 1.0)
- fixed loading screens showing before playing FMVs on most levels
- fixed Lara not being able to move after exiting water, having used an underwater lever with the animated interactions setting enabled (#4912, regression from 1.0)
- fixed Bell in room 48 being shootable from room 55 (#4949, regression from TR2X 1.4)
- fixed "Disable T-Rex Collision" option missing from The Golden Mask (there are T-Rex enemies in Nightmare in Vegas)

**TR3**:
- added reverb support
- added Kayak control
- added Compsognathus control
- added Mounted Gun control
- added Tribe Axeman control
- added Tribe Pipeman control
- added Tribe Boss control
- added Lizard control
- added Crocodile control
- added Carcass control (hanging Raptor)
- added T-Rex control
- added Raptor control
- added Raptor Emitter control
- added Bat Emitter control with save/load support
- added South Pacific Mercenary control
- added Smashable Wall control
- added Smashable Shutters control
- added a slide-to-sprint animation state change for Lara, similar to TR1 and TR2
- added a new gameplay option to toggle Lara's crouch roll (Gameplay → Controls → Crouch roll)
- added an option to allow Lara to jump out of crawlspaces (Gameplay → Controls → Crawl exit jump)
- added crouching/crawling enhancements (Gameplay → Controls → Responsive crawling)
  - added the ability to resume crawling more quickly after coming to a stop
  - added transitions from run/sprint to crawl without first coming to a stop
  - added a transition from crawl to crouch-roll without having to manually crouch first
  - added the ability to turn while in the crouch idle state
  - restored an unused pickup animation when in the crawling state, bypassing the crouch transition
- added a transition from ladder to crawlspaces instead of first having to drop and re-grab the ladder (#4954)
- restored the ability for Lara to perform grab cancels, like TR1 and TR2
- restored glide camera functionality
- removed the limitation of one Carcass instance per level working with Piranhas
- removed the limitation of Piranhas only attacking Carcass instances if the level sequence matches Crash Site's
- fixed Uzis having wrong clips capacity (was 80, is now 40 – sorry!)
- fixed Lara briefly switching from run back to wade when crossing from 2-click to 1-click water depth
- fixed Lara unable to climb small ledges with low crawlspaces
- fixed Lara using the thin-ledge swing hang animation instead of the normal hang in some 1-click ledge cases
- fixed Lara being unable to transition from slow swing at the base of a ladder to being able to climb the ladder
- fixed Lara's cutscene gun shots not rendering muzzle flashes, gun smoke and shell ejections (e.g., Tony cutscene)
- fixed water ripples triggering z-fighting with 0-click ground surfaces
- fixed footprints rendering with an excessive Y offset
- fixed wheel switches only being usable once
- fixed wheel switch triggers activating too early
- fixed Kayak voiding and teleporting on large slopes
- fixed Kayak wake effects sometimes clipping through complex geometry
- fixed loading screens showing before playing FMVs in Antarctica
- fixed end credits referencing non-existing image file
- fixed Puna to no longer hardcode Lizard locations, and instead use relative offsets
- fixed Puna's summoned Lizards counting towards total level kill count
- fixed Tony briefly appearing for a single frame when loading a save after his death
- fixed Lara sometimes getting stuck when crawling backwards off a tilted ledge (#4956)
- fixed the Tribe Pipeman sometimes not being able to aim darts at Lara correctly (Gameplay → Fixes → Fix Pipeman aim)
- fixed Lara's footprints sometimes spawning when standing on a bridge, trapdoor or pushblock
- fixed Lara being unable to walk or sidestep at times when standing on a bridge that sits over a steep slope (regression from 1.1)
- fixed Lara's left arm elevating when holding a flare and performing a crouch pickup (regression from 1.1)



## [1.2.2](https://github.com/LostArtefacts/TRX/compare/trx-1.2.1...trx-1.2.2) - 2026-02-13
- fixed a potential `GL_OUT_OF_MEMORY` error that could occur after reloading levels many times (regression from <1.0)



## [1.2.1](https://github.com/LostArtefacts/TRX/compare/trx-1.2...trx-1.2.1) - 2026-02-11
- fixed title ring music inheriting the wrong audio volume (regression from 1.2)
- fixed settings dialog changing size when cycling through non-scrollable tabs (regression from 1.2)
- fixed Play Previous Level feature not restoring Lara's equipment correctly for pre-1.2 saves (regression from 1.2)
- fixed Play Previous Level feature causing Lara to instantly die for pre-1.2 saves, when the Persistent Damage option is on (regression from 1.2)
    Note: for those 1.0/1.1 saves, this feature will restore her health to full, as it was not stored correctly. 1.2 will continue to restore the correct HP value.
- fixed TR2 delayed music triggers not working (regression from 1.1)
- fixed TR3 using delayed music triggers (TR2-only feature)



## [1.2](https://github.com/LostArtefacts/TRX/compare/trx-1.1...trx-1.2) - 2026-02-11
Showcase: https://www.youtube.com/watch?v=jeq8rQONaic
- added globe level selection mechanic
- added Bubble Emitter control (#4629)
- added dynamic light objects:
    - added Red Light control
    - added Green Light control
    - added Blue Light control
    - added Amber Light control
    - added White Light control
    - added Strobe Light control
    - added Pulse Light control
    - added Beacon Light control
    - added On/Off Light control
- added the ability in Lua to hook into control loop events during cutscenes
- added an option to change Lara's outfit, with 20 variants included by default; custom levels can provide up to 32 outfits (Visuals → General → Lara's outfit) (#4383)
- added an option to control UI brightness (Graphic Options → Rendering → UI brightness); renamed "Brightness" to "Game brightness"
- added an option to allow Lara's underwater mesh tint to be more responsive based on position, as per TR3 (Visuals → General → Responsive mesh tint)
- added the ability for custom levels to define Lara's braid position relative to her head (#110)
- added the ability to disable manual camera (Gameplay → Controls → Manual camera)
- added the ability to enable bouncy grenades (Gameplay → General → Enable bouncy grenades)
- added the ability to toggle TR1/2 and TR3 projectile area damage – TR3 often deals double damage (Gameplay → Mods → Projectile Area Damage)
- added the ability to hide pickup notifications in the bottom-right corner (Graphic Options → UI → Pickups overlay)
- added a new Lua event, `trx.events.on_game_start`, which fires when the level finishes loading and the game is about to start
- added a new Lua function, `trx.rooms.flip()`, to toggle the flip map (#4704)
- added a new Lua function, `trx.rooms.flip_effect()`, to set the active flip effect with an optional timer (#4704)
- added a new Lua catalog, `trx.catalog.flip_effects` for name-based flip effect catalog IDs
- added a new Lua music play mode, `trx.music.PlayMode.OVERLAY` for playing on top of currently played track
- added new Lua catalogs for Lara states, Lara anims, music, and samples
- added a new Lua module, `trx.camera`, with camera getters and `trx.camera.shake()`
- added a new Lua property, `trx.rooms.Room.num`
- added support for cross-fades to the title screen
- added visual previews of bar colors (Graphic Options → Bars)
- added the ability to change PS1 bar colors
- added shadow rendering to all cutscene actors
- added endless sprint (available previously via the `/restless` command) to the UI settings (Gameplay options → Mods → Endless sprint)
- added endless flare time cheat (Gameplay options → Mods → Endless flare time)
- added `O_VULTURE` for custom levels
- added `O_ROLLING_BALL_4` (giant Temple of Puna boulder) for custom levels
- added an option to control whether or not moving boulders should shake the camera (Gameplay options → General → Enable boulder shake)
- added an option to make Lara stumble if she hops backwards and there is a slope behind her (Gameplay options → Controls → Backwards slope stumble)
- added `/trigger` and `/untrigger` console commands, with support for targeting by item ID, item name, or object name
- added the ability to seek backwards through cutscenes with left button
- added the ability to trigger collapsible tiles from heavy triggers, regardless of Lara's position (#4807)
- added floor height change detection for boulders when stopped, so they will drop if the floor below them drops (#4808)
- added splash effects to neutral twists and rolls (#4793)
- improved rendering performance
- improved the ability to seek through cutscenes to support even faster seeks (Slow = ±1 s, default = ±5 s, new: Draw = ±15 s)
- improved `/tp` to accept `room`/`item` prefixes and `rN`/`iN` shortcuts
- improved inventory ring active item highlight for smoother appearance
- improved savegame file size by reducing it about 20–30%.
- improved indentation for nested bullets in the UIs
- changed `debug.debug_cuboids` option name from "debug cuboids" to "debug bounding boxes" (`/debug bounding-boxes` or `/set debug-bounding-boxes 1`)
- changed `debug.enable_debug_pos` option to split into `enable_debug_pos` and `enable_debug_anim`
- changed `debug.enable_invulnerability` option to only show the marker if the setting `enable_debug_status` is on (off by default) (#4631)
- changed `audio.load_music_triggers` (Gameplay → Fixes → Fix one-shot music triggers) to be enabled by default
- changed photo mode to no longer show "Entering photo mode" in the console
- changed photo mode to always display a red frame around the game view when active (not visible in screenshots)
- changed stats dialog to include allies in kill count if they turn hostile. This applies to all levels that follow, and the final stats screen.
- changed rooms-to-draw tracking to no longer stop at the 100-room limit
- changed boulders to stop if the ceiling height is lower than their height
- changed all UI bar colors from hardcoded to configurable via `cfg/ui.json5`, enabling some customization for PS1 bars
- changed `/debug [0|1]` command to no longer spam about settings that aren't changed
- changed `/set` command to always use hyphens for enum option values, and accept both underscores and hyphens
- changed Lua catalog keys to strip `O_` prefixes and use lowercase
- changed Lua event callback names to be more consistent:
    - `on_level_init` → `before_level_file`
    - `on_level_start` → `after_level_file`
    - `on_level_load` → `after_level_state`
    - `on_control` → `before_control`
    - `on_control_post` → `after_control`
- changed turbo cheat to auto‑reset to normal speed if pushed past limit, making it easier for new players to recover from accidental changes
- changed Blades to support being reset
- changed the barefoot SFX option toggle in TR2 to no longer require reloading the level for changes to take effect
- changed triggers that target pickup items to support antitriggers, switches and bitmasks
- removed support for legacy (TombATI / TR2 GOG/Steam) and pre-1.0 (TR1X/TR2X) savegame files
- fixed random face dropouts on levels with more than 32k textures
- fixed a small hiccup when launching the game on certain GPUs
- fixed inconsistent music volume in the statistics screens (#4499)
- fixed shadows to support 60 FPS interpolation
- fixed soft static mesh collision not working right with statics that appear in overlapping rooms
- fixed drawing debug triggers using random tint near water sources
- fixed drawing debug triggers glitching through triangular portals
- fixed Lara being force-resurfaced near split-triangle water portals in certain spots
- fixed custom levels that contain invalid room static mesh references not being able to load (#4770)
- fixed the tip of Lara's braid using an invalid offset position on the first frame of a level (#4821)
- fixed drawing shadows twice when item intersects a portal (#4640, regression from 1.0)
- fixed drawing circle/octagon shadows in TR2/TR3 cutscenes using wrong positions
- fixed being unable to use the manual camera in TR3 camera mode when Lara is idle (#4670, regression from 1.1)
- fixed grenades not killing more than a single enemy
- fixed running `/title` and similar commands leaving the "Examine" button briefly visible in the key items ring (old regression)
- fixed running `/title` and similar commands when examining an item causing incorrect item rotation next time the ring opens (old regression)
- fixed endless sprint cheat setting not retained between game relaunches
- fixed Cobras not being counted in level kill count
- fixed stats dialog retaining friendly status for allies that become enemy types in later levels, causing them to get excluded from kill count
- fixed targeting hostile ex-allies not working if "Enable ally targeting" option is off
- fixed `/play` and similar commands fading out instead of running instantly on stats/title screens
- fixed `/play` and similar commands sometimes preserving cutscene camera tilt if invoked while a cutscene was paused
- fixed Cheats description showing arrows in the indented bullets (#4753, regression from TRX 1.1)
- fixed game freezing on exit on certain platforms when there are no active sound devices (SDL bug)
- fixed Lara twitching when trying to step back onto death tiles
- fixed Lara's look head rotation/tilt limits being hardcoded to the engine version rather than camera mode
- fixed Lara rotating around an incorrect origin in photo mode during cutscenes
- fixed pushblocks being able to fall into rooms below despite no portals being present (#4788, regression from TR1X 4.15/TR2X 1.5)
- fixed one-shot triggers for hidden pickup items making the items permanently invisible (#4784)
- fixed secret tracks played at low quality when "fix secrets killing music" option is on
- fixed secret tracks not restored from the savegame when "fix secrets killing music" option is on
- fixed slow-forward seeking through cutscenes (right+slow) not working (regression from 1.0)
- fixed statics marked collidable but with zero‑size hitboxes causing phantom collisions
- fixed Lara being displaced during the sprint-slide animation if she tried to pick up an item at the same time (#4843, regression from TR1X 4.14, TR2X 1.4)

**TR1**:
- added Unfinished Business loading screens (#1310, thanks to rockahub)
- fixed save crystal reflections rendering upside down (regression from 4.14)
- fixed underwater wobble effect acting twitchy with camera movement
- fixed several texture issues on each of Lara's outfits and guns
- fixed gun injections overwriting Lara's footstep SFX in all levels (#4733, regression from 1.1)
- fixed pushblocks in Natla's Mines becoming unusable after loading a save made in earlier versions (#4735, regression from 1.1)
- fixed low-quality texture palette on injected TR2/3 weapons and flares
- fixed baddie speeches played at low quality when "fix speeches killing music" option is on
- fixed baddie speeches not restored from the savegame when "fix speeches killing music" option is on

**TR2**:
- added "Sound Options → Misc → Layered secret music" option
- added "Gameplay → Fixes → Fix one-shot music triggers" option
- changed Assault Course stats to show scroll indicators (#3510)
- changed statistics screen rows to be more compact
- fixed wrong line played when finishing the Assault Course for the first time (#4667, regression from 1.1)
- fixed underwater wobble effect acting twitchy with camera movement
- fixed several texture issues on each of Lara's outfits and guns
- fixed a deviation in water current behaviour that could result in Lara stopping too early (#4706, regression from TR2X 1.1)
- fixed gun injections overwriting Lara's footstep SFX in underwater levels (#4733, regression from 1.1)
- fixed exploding Armed Snowmobile not disappearing the vehicle (#4762)
- fixed the polar bear in Furnace of the Gods twitching if killed when in its reared state (#4624)
- fixed incorrect textures on the MP5 when equipped or on Lara's back

**TR3**:
- added "Sound Options → Misc → Layered secret music" option
- added "Gameplay → Fixes → Fix one-shot music triggers" option
- added new UI bar appearances, "TR3 PC" and "TR3 PS1" (Graphic Options → Bars → Bars appearance)
- added new water currents
- added new blood effects
- added underwater blood spills
- added poison mechanic
- added heal crystals
- added animated puzzle holes support
- added new creature explosions effects
- added meteorite artifacts support
- added examine item feature for certain items
- added Monkey control
- added Shiva control
- added Tony control
- added Spikes animation in Coastal Village and Madubu Gorge
- added Electric Fence control
- added Aldwych Drill control (Spike Ceiling with timer=1 to descend faster)
- added TR3 behavior patterns to Tiger control
- added Kill All Triggered control
- added Vulture control
- added Boulder control
- added Poison Dart control
- added Earthquake control
- added dynamic light objects:
    - added Red Light control
    - added Green Light control
    - added Blue Light control
    - added Amber Light control
    - added White Light control
    - added Strobe Light control
    - added Pulse Light control
    - added Beacon Light control
    - added On/Off Light control
- added Lara's backwards-hop stumble if there is a slope behind her
- added "Sound Options → Misc → Layered secret music" option
- improved look camera stability to reduce idle-breathing camera bobbing/roll
- improved Monkeys to no longer hardcode hostility status based on Tiger presence
- changed Assault Course stats to show scroll indicators (#3510)
- changed statistics screen rows to be more compact
- changed hostile Monkeys to share hostility status, like TR2 Barkhang monks (the original TR3 behavior can be restored in Gameplay → General → Ally hostility policy)
- changed enemy drops to appear at the tile center, to conform with the OG
- fixed several texture issues on each of Lara's outfits and guns
- fixed actors jumping to their start frame at the end of cutscenes
- fixed Flame in Cutscene 4 and 6 appearing static
- fixed Swamp Map rotation
- fixed seaweed disappearing too quickly in certain levels
- fixed Hand of Rathmore not rotating in Sleeping with the Fishes
- fixed Icicles not having sound
- fixed Spike Walls not having sound
- fixed colored exhaust smokes on Quad Bike for 1 frame
- fixed Cobras and Rattlesnakes being immune to explosives in their sleeping state
- fixed Quad Bikes not restoring their state from savegames properly
- fixed exploding Assault Targets in Lara's Home counting as penalties
- fixed surface and underwater effects simulation speed
- fixed underwater wobble effect amplitude
- fixed animated textures speed
- fixed inconsistent Meteor Artifacts names
- fixed wrong item selection sound in the inventory ring
- fixed flame emitters not getting restored when loading from a save
- fixed Lara holding onto ledges after dying if the Action key wasn't released
- fixed Shiva death smoke effects getting misplaced if the player saves and reloads mid-battle
- fixed Grenade, Rocket Launcher, and Harpoons damage
- fixed being unable to antitrigger Poison Dart Emitters
- fixed ally Lua API not working with most of the TR3 enemies supported so far
- fixed one-shot antitriggers / antipads behavior
- fixed Blades in Coastal Village not respecting antitrigger
- fixed some Poison Darts disappearing 1 frame early
- fixed running down an enemy with a Quad not counting as a kill
- fixed killing Cobras with a manually-aimed projectile not counting as a kill
- fixed smoke and spark rotation snapping at 180° instead of rotating smoothly
- fixed Lara burning instead of getting electrocuted when touching the top of the electric fence
- fixed driving over Winston with a Quad Bike or shooting him with the Harpoon Gun causing him to bleed
- fixed driving over Assault Target with a Quad Bike or shooting it with the Harpoon Gun causing it to spawn blood
- fixed skybox data in Scotland TR3:LA levels to show correct top and bottom colors



## [1.1](https://github.com/LostArtefacts/TRX/compare/trx-1.0.3...trx-1.1) - 2026-01-17
Showcase: https://www.youtube.com/watch?v=veVYyr--H1A
- added a fade-in and fade-out effect to patterned inventory backgrounds
- added the ability to use monochrome image for inventory and statistic screens backgrounds
- added the ability to use very dark image for inventory and statistic screens backgrounds (#4469)
- added the ability to change pause screen background
- added the ability to control whether or not allies are hostile towards Lara via Lua (#3873)
- added the ability to control via Lua which enemies are allies and which are ones that will fight with allies (#3873)
- added the ability to control Lara's air timer via Lua (#4592)
- added the ability to fine-tune the fade effects between the inventory ring, the pause screen, and the stats screen (Graphic Options → UI → Inventory/Pause/Stats fade effects)
- added gamma control (TR3-style) to all games (Graphic Options → Rendering → Gamma)
- added support for TR3 weather effects to all games (#3881)
- added support for 3D secret objects, and provided defaults for OG levels in TR2 (#4380)
- added catalog object IDs to Lua
- added the ability to swap meshes in Lua
- added support for locked cameras, similar to TR4+ (#2040)
- added support to use `O_DINO_WARRIOR` and `O_FISH` as aliases for `O_TREX` and `O_BARRACUDA` respectively
- added the ability to define gun types, flash shade and offset positions in `cfg/weapons.json5`
- added the ability to define ammo pickup quantities per weapon in `cfg/weapons.json5` (#4518)
- added a new input, that lets the player toggle in-game textures on/off, available by default under F8
- added a new console command, `/textures`, that lets the player toggle in-game textures on/off
- added a new console command, `/weather`, that lets the player control the weather
- added a new console command, `/spawn`, that lets the builder spawn an entity of their choice to test things around
- added Animating Item 1-6 control
- added the option to use TR3 sprite-based shadows (Visuals → Shadows shape)
- added an option for soft static mesh collision; this also allows for arbitrary mesh rotation in custom levels and retaining accurate collision (Gameplay → Controls → Soft mesh collision) (#3654)
- added an option to use the TR3 camera (Visuals → Camera Mode)
- improved a fade-in and fade-out effect on loading screens – they now smoothly transition to the game screen
- improved fog behavior to be less dependent on camera rotation
- changed the 3D pickups option to try the simplified 3D meshes first, if available, before falling back to inventory items
- changed the 2D and 3D statics limit from 256 to unlimited
- changed the lighting contrast key binding to F9
- changed underwater statics to be affected by caustics, even if they don't get merged into level geometry (#4430)
- changed Magnums and Automatic Pistols to be separate objects, so both can appear in the same level (#4475)
- changed the M16 and MP5 to be separate objects, so both can appear in the same level
- changed the swinging axe to be defined separately from other pendulums (use object `O_SWINGING_AXE` in catalogs)
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
- changed the fonts to use dedicated sprites for similar-looking characters instead of using aliases
- changed the reset keybindings bars appearance to be more visible
- changed the default exposure bar PC color to blue 2
- changed lua music `PlayMode` constant names
- removed the `scripting/trx` directory – internal TRX LUA scripts now get embedded in the exe
- fixed broken final statistic counters (#4432, regression from 1.0)
- fixed undefined behavior (crashes and/or texture glitches) in levels with a lot of textures
- fixed a crash if a pickup aid spawns against an item whose 3D model isn't present
- fixed Bacon Lara not always being drawn perfectly in sync with Lara's animation (#4210)
- fixed gondolas not being drawn with an underwater tint when they have sunk (#4428)
- fixed the teleport-to-item command not succeeding if used in succession with the same type and an out of bounds item is encountered (#4468)
- fixed skybox faces with transparent pixels always rendering in front of all other faces (#4351, regression from 1.0)
- fixed unbound inputs not being saved between game launches (#4360, regression from TR1X 4.14/TR2X 1.4)
- fixed Lara drawing a flare when the draw weapons input is pressed, and she already has an active flare but no weapons (#4361, regression from TR2X 1.4)
- fixed wading splashes spawning when using the fly cheat (#4400, regression from 1.0)
- fixed grenades not exploding floating water creatures (#4399, regression from TR2X 1.3)
- fixed water enemies not getting tinted when dead and floating (#4407, regression from 1.0)
- fixed Lara not colliding with mines/gondolas when underwater (#4424, regression from TR2X 1.3)
- fixed flare box pickups containing only one flare if Lara has none in her inventory at that time (#4423, regression from 1.0)
- fixed water enemies appearing untinted for a frame after dying and moving to the water surface (#4420, regression from TR2X 0.1)
- fixed the interactive fly cheat breaking with animated interactions enabled (#4444, regression from TR1X 4.14)
- fixed switch triggers using an incorrect state check, which could result in fixed camera behavior that deviated from OG (#4456, regression from 1.0)
- fixed ambient music triggers to no longer kill active normal music tracks (#4463)
- fixed game crashing when Lara passes through light sources in certain levels
- fixed waterfall mist not brightening when holding a flare (#4486)
- fixed resetting camera in the photo mode not clearing the underwater tint
- fixed developer console text editing (backspace, moving the caret) doing weird things with Unicode characters
- fixed Lara jumping if player holds the swim button when exiting the fly cheat (#4470)
- fixed game refusing to load savegames made with the JP mode (#4558)

**TR1**:
- added the ability to change inventory and statistics background styles (pattern + wave are not implemented in TR1)
- added Automatic Pistols, the Desert Eagle, the MP5, and the Rocket Launcher to the `/moreguns` console command
- fixed Lara standing two clicks below `O_FALLING_BLOCK_3` items rather than directly on top (#4374)
- fixed missing menu guns SFX in Lara's Home
- fixed several OG texture issues in Caves (rooms 0, 1, 2, 6, 24, 30 and 32)
- fixed Lara automatically being given TR2 weapons in NG+ when playing the OG levels (#4365, regression from 1.0)
- fixed Lara's pistol holster meshes appearing in NG+ in place of her Uzi holster meshes (#4368, regression from 1.0)
- fixed Lara's footstep sounds being very quiet when weapons are equipped (#4451, regression from 1.0)
- fixed the grenade blast SFX not always playing in succession (#4628, regression from 1.0)

**TR2**:
- added unused gym voice line at level start if Lara has any logged assault course attempts (#2822)
- added high-resolution 16:9 and 4:3 loading screens
- added high-resolution 16:9 and 4:3 game end screen  
    To download the new images ahead of a stable release, please see the [TRX data](https://github.com/LostArtefacts/TRX-data) repository.
- added Magnums, the Desert Eagle, the MP5, and the Rocket Launcher to the `/moreguns` console command
- changed Tibetan Foothills to have snow (you can disable this via Graphic Options → Visuals → Weather)
- changed ember emitters to use the `SFX_LAVA_FOUNTAIN` sample (#4376)
- fixed the scuba diver's death SFX not playing (#4386)
- fixed a missing trigger for tiger 6 in Ice Palace (#4390)
- fixed missing music triggers in Venice room 11 and Floating Islands room 80
- fixed a missing death tile in Floating Islands room 91
- fixed vertex lighting and stretched textures in Lara's Home room 28 and Home Sweet Home room 27
- fixed z-fighting on fences in Barkhang Monastery and gondola poles in Venice
- fixed missing oxygen tanks in Offshore Rig room 82
- fixed the monk in the Diving Area cutscene not having a complete death animation
- fixed demos not using loading screens
- fixed reading room lights for custom TR2 levels (regression from 1.0)
- fixed the switch in room 46 of Opera House randomly disappearing
- fixed game crashing when Lara passes through light sources in levels compiled with dxtre3D
- fixed Snowmobile music not getting resumed (#4519)
- fixed Stopwatch position in the inventory ring (#2014)
- fixed static lighting on broken ice/windows (#4506, regression from 1.0)

**TR3**:

A lot of our TR3 work builds on *TOMB3*, which Troye and ChocolateFan kindly let us dive into and expand on.
Their hard work gave us the perfect base to push TRX further, and made the climb a lot less vertical!

- added support for monkey bar mechanics
- added support for crawlspace mechanics
- added RGB lighting system support
- added flame effects
- added swamp and water surfaces wave effect
- added underwater caustics
- added proper bubbles
- added water splash and ripple effects
- added waterfall mist effect
- added per-mesh underwater tinting (Lara only)
- added `cdaudio.wad` music playback support
- added weather effects
- added sprite-based shadows
- added footprints
- added surface-based step sounds
- added cold breath effects
- added gun shells
- added gun projectiles
- added gun smoke effects
- added new ricochets
- added flare lighting and sparks
- added monochrome inventory backgrounds
- added TR3 inventory ring lighting
- added high-resolution 16:9 and 4:3 loading screens
- added high-resolution 16:9 and 4:3 title and game end screens
- added high-resolution 16:9 and 4:3 credit images
    To download the new images ahead of a stable release, please see the [TRX data](https://github.com/LostArtefacts/TRX-data) repository.
- added support for the serif font
- added support for colored text
- added Assault Course and Race Track course mechanics
- added Quad Bike control
- added Animating Item 1-6 control
- added Electrical Light control
- added Smoke Emitters control
- added Steam Emitter control
- added Flame Emitter 1-3 and Side Flame Emitter control
- added Piranhas and Tropical Fish control
- added Desert Eagle control
- added MP5 control
- added Rocket Launcher control
- added Magnums, the Automatic Pistols, and the M16 to the `/moreguns` console command
- added all weapons to Lara's Home (accessible with cheats or via the console only)
- added Assault Course target control
- added Assault Course penalty system
- added an option to fix the MP5 accuracy while running
- added TR3 camera control and look functionality
- improved run-to-crawl transition
- improved text colors of the Assault Course statistics and timers
- improved Assault Course targets to spawn ricochets
- changed The River Ganges, City and All Hallows to have rain
- fixed sample reading to support correct pitch and volume
- fixed pool edges shifting along with the water effect
- fixed Lara's thigh being drawn when a flare is in Lara's hand or has been discarded
- fixed gun flashes being drawn in white
- fixed disabling lighting system not working
- fixed skybox data to show correct top and bottom colors
- fixed Assault Course timer remaining indefinitely on screen
- fixed Quad Bike low visibility of exhaust smokes at high speeds
- fixed Quad Bike wheels appearing to spin backwards at high speeds
- fixed the skybox's blue lid for the Thames Wharf and City cutscenes
- fixed fish schools to no longer swim at supersonic speeds if their triggers do not have timers set, or reuse the same timer
- fixed Lara letting go of some ledges
- fixed shadow sizes dependent on Lara's placement instead of their owner's



## [1.0.3](https://github.com/LostArtefacts/TRX/compare/trx-1.0.2...trx-1.0.3) - 2025-11-27
- fixed the conveyor belt fuse in Natla's Mines not appearing after using the nearby switch (#4349, regression from 1.0)



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
    See [the documentation](../trx/lua) for details.
- added a game flow option for cold water in custom levels, similar to TR3 (#4021)
- added a splash effect when Lara jumps in wading depth water, similar to TR3+ (#3975)
- added bounding box debugging (`/debug 1` or `/set debug-cuboids 1`)
- added support for object, music, sound, flip effects, Lara state, and Lara animation slots overrides through CSV catalogs  
    Lets builders link hardcoded logic to slots of their choice, allowing object sharing between games (for example, use TR1 bats in TR2).  
    This feature is experimental — some objects may not behave correctly. Please report any bugs encountered! 🩷  
    See [the documentation](../CATALOGS.md) for details.
- added `enable_debug_camera` setting that shows camera position in realtime (reachable via `/debug` and `/set`)
- added the ability to fast-forward through cutscenes with the right button (+5 s) or with slow+right (+1 s)
- added support for dark theme on Windows
- added support for triangular geometry
- added support for additive blending in textures
- added support for quicksand rooms
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
- changed the Remove shotguns, Remove Uzis and Remove Magnums into a single "Remove extra guns" option
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

**TR3**:
- added basic TR3 level loader (nothing is working yet!)
