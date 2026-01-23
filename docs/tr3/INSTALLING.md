# Windows (installer)

## Installing (simplified)

**The TR3 installer is not yet ready, but we'll eventually provide it.**

> [!NOTE]
> When downloading TRX, you might see a warning from Windows Defender, your browser, or another security tool. Modern antivirus systems use AI‑based heuristics – they flag anything uncommon or unsigned as suspicious, even if it's perfectly safe. TRX can trigger these alerts because:
>
> - It isn't signed with a costly commercial certificate.
> - It's a niche, community‑built project, so not widely recognized.
> - It's a custom build, not from the Microsoft Store.
>
> Don't worry: TRX is open‑source, and you can inspect the code yourself on [GitHub](https://github.com/LostArtefacts/TRX/).

# Windows / Linux

## Installing (manual)

1. Download the TRX zip file.
2. Extract the zip file into a directory of your choice.  
     Make sure you choose to overwrite existing directories and files.
3. If installing for the first time – put your original game files into the target directory.

   Unfortunately, due to legal reasons, we cannot offer an easy packaging of The Lost Artifact expansion pack.

## Verifying the installation

If you install everything correctly, your game directory should look more or less like this (click to expand):

<details data-id="file-tree-win">
<pre><code>.
├── audio
│   └── cdaudio.wad
├── cfg
│   ├── base_strings-de.json5
│   ├── base_strings-en-gb.json5
│   ├── base_strings-fr.json5
│   ├── base_strings-gd.json5
│   ├── base_strings-it.json5
│   ├── base_strings-pl.json5
│   ├── base_strings-ru.json5
│   ├── base_strings.json5
│   ├── catalog_item_actions.csv
│   ├── catalog_lara_anims.csv
│   ├── catalog_lara_states.csv
│   ├── catalog_music.csv
│   ├── catalog_objects.csv
│   ├── catalog_samples.csv
│   ├── inv_ring.json5
│   ├── poses.json5
│   ├── tr2
│   │   ├── gameflow.json5
│   │   ├── strings-de.json5
│   │   ├── strings-en-gb.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   └── strings.json5
│   ├── tr2-gm
│   │   ├── gameflow.json5
│   │   ├── strings-de.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   └── strings.json5
│   ├── tr2-level
│   │   ├── gameflow.json5
│   │   ├── strings-de.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   └── strings.json5
│   ├── TR3X.json5*
│   ├── ui.json5
│   └── weapons.json5
├── cuts
│   ├── cut1.tr2
│   ├── cut2.tr2
│   ├── cut3.tr2
│   ├── cut4.tr2
│   ├── cut5.tr2
│   ├── cut6.tr2
│   ├── cut7.tr2
│   ├── cut8.tr2
│   ├── cut9.tr2
│   ├── cut11.tr2
│   └── cut12.tr2
├── data
│   ├── antarc.tr2
│   ├── area51.tr2
│   ├── chamber.tr2
│   ├── city.tr2
│   ├── compound.tr2
│   ├── crash.tr2
│   ├── house.tr2
│   ├── images
│   │   ├── 4x3
│   │   │   ├── antarc.webp
│   │   │   ├── credit01.webp
│   │   │   ├── credit02.webp
│   │   │   ├── credit03.webp
│   │   │   ├── credit04.webp
│   │   │   ├── credit05.webp
│   │   │   ├── credit06.webp
│   │   │   ├── credit07.webp
│   │   │   ├── credit08.webp
│   │   │   ├── credit09.webp
│   │   │   ├── house.webp
│   │   │   ├── india.webp
│   │   │   ├── legal_eu.webp
│   │   │   ├── legal_us.webp
│   │   │   ├── london.webp
│   │   │   ├── nevada.webp
│   │   │   ├── southpac.webp
│   │   │   ├── theend2.webp
│   │   │   ├── title_eu.webp
│   │   │   └── title_us.webp
│   │   ├── antarc.webp
│   │   ├── credit01.webp
│   │   ├── credit02.webp
│   │   ├── credit03.webp
│   │   ├── credit04.webp
│   │   ├── credit05.webp
│   │   ├── credit06.webp
│   │   ├── credit07.webp
│   │   ├── credit08.webp
│   │   ├── credit09.webp
│   │   ├── house.webp
│   │   ├── india.webp
│   │   ├── legal_eu.webp
│   │   ├── legal_us.webp
│   │   ├── london.webp
│   │   ├── nevada.webp
│   │   ├── og
│   │   │   ├── antarc.webp
│   │   │   ├── credit01.webp
│   │   │   ├── credit02.webp
│   │   │   ├── credit03.webp
│   │   │   ├── credit04.webp
│   │   │   ├── credit05.webp
│   │   │   ├── credit06.webp
│   │   │   ├── credit07.webp
│   │   │   ├── credit08.webp
│   │   │   ├── credit09.webp
│   │   │   ├── house.webp
│   │   │   ├── india.webp
│   │   │   ├── legal_eu.webp
│   │   │   ├── legal_us.webp
│   │   │   ├── london.webp
│   │   │   ├── nevada.webp
│   │   │   ├── nevadafff.webp
│   │   │   ├── southpac.webp
│   │   │   ├── theend2.webp
│   │   │   ├── theend.webp
│   │   │   ├── title_eu.webp
│   │   │   └── title_us.webp
│   │   ├── southpac.webp
│   │   ├── theend2.webp
│   │   ├── title_eu.webp
│   │   └── title_us.webp
│   ├── injections
│   │   ├── barefoot_sfx.bin
│   │   ├── barkhang_cameras.bin
│   │   ├── barkhang_fd.bin
│   │   ├── barkhang_itemrots.bin
│   │   ├── barkhang_music_tracks.bin
│   │   ├── barkhang_pickup_meshes.bin
│   │   ├── barkhang_textures.bin
│   │   ├── bartoli_music_tracks.bin
│   │   ├── bartoli_secret_fd.bin
│   │   ├── bartoli_textures.bin
│   │   ├── boat_bits.bin
│   │   ├── breakable_tile_sfx.bin
│   │   ├── catacombs_fd.bin
│   │   ├── catacombs_itemrots.bin
│   │   ├── catacombs_music_tracks.bin
│   │   ├── catacombs_textures.bin
│   │   ├── coldwar_fd.bin
│   │   ├── coldwar_itemrots.bin
│   │   ├── coldwar_music_tracks.bin
│   │   ├── coldwar_objects.bin
│   │   ├── coldwar_textures.bin
│   │   ├── common_pickup_meshes.bin
│   │   ├── common_pickup_meshes_gm.bin
│   │   ├── cut2_textures.bin
│   │   ├── cut3_setup.bin
│   │   ├── cut3_textures.bin
│   │   ├── cut4_textures.bin
│   │   ├── dagger_sprite.bin
│   │   ├── deck_cameras.bin
│   │   ├── deck_fd.bin
│   │   ├── deck_itemrots.bin
│   │   ├── deck_music_tracks.bin
│   │   ├── deck_pickup_meshes.bin
│   │   ├── deck_plants.bin
│   │   ├── deck_secret_fd.bin
│   │   ├── deck_textures.bin
│   │   ├── diving_cameras.bin
│   │   ├── diving_itemrots.bin
│   │   ├── diving_music_tracks.bin
│   │   ├── diving_pickup_meshes.bin
│   │   ├── diving_sfx.bin
│   │   ├── diving_textures.bin
│   │   ├── door106_sfx.bin
│   │   ├── door107_sfx.bin
│   │   ├── door108_sfx.bin
│   │   ├── door110_sfx.bin
│   │   ├── door111_sfx.bin
│   │   ├── explosion.bin
│   │   ├── fathoms_goon_sfx.bin
│   │   ├── fathoms_itemrots.bin
│   │   ├── fathoms_music_tracks.bin
│   │   ├── fathoms_plants.bin
│   │   ├── fathoms_secret_fd.bin
│   │   ├── fathoms_textures.bin
│   │   ├── floating_fd.bin
│   │   ├── floating_itemrots.bin
│   │   ├── floating_music_tracks.bin
│   │   ├── floating_pickup_meshes.bin
│   │   ├── floating_textures.bin
│   │   ├── font.bin
│   │   ├── fools_itemrots.bin
│   │   ├── fools_music_tracks.bin
│   │   ├── fools_pickup_meshes.bin
│   │   ├── fools_textures.bin
│   │   ├── furnace_itemrots.bin
│   │   ├── furnace_music_tracks.bin
│   │   ├── furnace_objects.bin
│   │   ├── furnace_pickup_meshes.bin
│   │   ├── furnace_textures.bin
│   │   ├── guardian_death_commands.bin
│   │   ├── gym_fd.bin
│   │   ├── gym_music_tracks.bin
│   │   ├── gym_sfx.bin
│   │   ├── gym_textures.bin
│   │   ├── house_itemrots.bin
│   │   ├── house_music_tracks.bin
│   │   ├── house_sfx.bin
│   │   ├── house_shower_frames.bin
│   │   ├── house_textures.bin
│   │   ├── inv_background.bin
│   │   ├── kingdom_cameras.bin
│   │   ├── kingdom_itemrots.bin
│   │   ├── kingdom_music_tracks.bin
│   │   ├── kingdom_textures.bin
│   │   ├── lair_bartolipos.bin
│   │   ├── lair_music_tracks.bin
│   │   ├── lair_textures.bin
│   │   ├── lara_animations.bin
│   │   ├── lara_extra.bin
│   │   ├── lara_guns.bin
│   │   ├── lara_gym_guns.bin
│   │   ├── lara_house_guns.bin
│   │   ├── lara_rifle_sfx.bin
│   │   ├── lara_unwater_guns.bin
│   │   ├── lara_vegas_guns.bin
│   │   ├── living_deck_goon_sfx.bin
│   │   ├── living_fd.bin
│   │   ├── living_itemrots.bin
│   │   ├── living_music_tracks.bin
│   │   ├── living_pickup_meshes.bin
│   │   ├── living_secret_fd.bin
│   │   ├── living_sfx.bin
│   │   ├── living_textures.bin
│   │   ├── loose_boards_sfx.bin
│   │   ├── misc_sprites.bin
│   │   ├── opera_fd.bin
│   │   ├── opera_itemrots.bin
│   │   ├── opera_music_tracks.bin
│   │   ├── opera_sfx.bin
│   │   ├── opera_textures.bin
│   │   ├── palace_fd.bin
│   │   ├── palace_itemrots.bin
│   │   ├── palace_music_tracks.bin
│   │   ├── palace_secret_fd.bin
│   │   ├── palace_textures.bin
│   │   ├── pda_model.bin
│   │   ├── photo.bin
│   │   ├── pickup_aid.bin
│   │   ├── portcullis_sfx.bin
│   │   ├── purple_crystal.bin
│   │   ├── rig_itemrots.bin
│   │   ├── rig_music_tracks.bin
│   │   ├── rig_pickup_meshes.bin
│   │   ├── rig_textures.bin
│   │   ├── scuba_sfx.bin
│   │   ├── seaweed_collision.bin
│   │   ├── secret_models_gm.bin
│   │   ├── secret_models_og.bin
│   │   ├── shark_sfx.bin
│   │   ├── tibet_fd.bin
│   │   ├── tibet_itemrots.bin
│   │   ├── tibet_music_tracks.bin
│   │   ├── tibet_textures.bin
│   │   ├── title_textures.bin
│   │   ├── vegas_fd.bin
│   │   ├── vegas_itemrots.bin
│   │   ├── vegas_music_tracks.bin
│   │   ├── vegas_textures.bin
│   │   ├── venice_fd.bin
│   │   ├── venice_itemrots.bin
│   │   ├── venice_music_tracks.bin
│   │   ├── venice_textures.bin
│   │   ├── wall_cameras.bin
│   │   ├── wall_itemrots.bin
│   │   ├── wall_music_tracks.bin
│   │   ├── wall_textures.bin
│   │   ├── winston_model.bin
│   │   ├── wreck_cameras.bin
│   │   ├── wreck_fd.bin
│   │   ├── wreck_goon_sfx.bin
│   │   ├── wreck_itemrots.bin
│   │   ├── wreck_music_tracks.bin
│   │   ├── wreck_pickup_meshes.bin
│   │   ├── wreck_plants.bin
│   │   ├── wreck_secret_fd.bin
│   │   ├── wreck_textures.bin
│   │   ├── xian_fd.bin
│   │   ├── xian_itemrots.bin
│   │   ├── xian_music_tracks.bin
│   │   ├── xian_pickup_meshes.bin
│   │   ├── xian_sfx.bin
│   │   └── xian_textures.bin
│   ├── jungle.tr2
│   ├── main.sfx
│   ├── mines.tr2
│   ├── nevada.tr2
│   ├── office.tr2
│   ├── quadchas.tr2
│   ├── rapids.tr2
│   ├── roofs.tr2
│   ├── scripts
│   │   ├── assault.lua
│   │   ├── floating.lua
│   │   ├── level1.lua
│   │   ├── level3.lua
│   │   ├── level4.lua
│   │   └── monastry.lua
│   ├── sewer.tr2
│   ├── shore.tr2
│   ├── stpaul.tr2
│   ├── temple.tr2
│   ├── title.tr2
│   ├── tombpc.dat
│   ├── tonyboss.tr2
│   ├── tower.tr2
│   ├── triboss.tr2
│   └── vict.tr2
├── fmv
│   ├── crsh_eng.rpl
│   ├── endgame.rpl
│   ├── intr_eng.rpl
│   ├── logo.rpl
│   └── sail_eng.rpl
├── shaders
│   ├── 2d.glsl
│   ├── billboard.glsl
│   ├── common.glsl
│   ├── fbo.glsl
│   ├── lights.glsl
│   ├── meshes.glsl
│   ├── meshes_tr3.glsl
│   ├── meshes_tr12.glsl
│   └── ui.glsl
└── TRX.exe</code></pre>
</details>

*\* Will not be present until the game has been launched.*

## Playing the game

- To play the game, run `TRX.exe`.
- To play the Lost Artifact expansion pack, run `TRX.exe --gold`.

# macOS

## Installing

1. Download the latest TRX for TR3 installer image (e.g `TRX-0.1-Mac-tr3.dmg`). Mount the image and drag TR3X to the Applications folder.
2. Run TR3X from the Applications folder. This will show you an error dialog about missing game data files. This is expected at this point, as you have not copied them in yet. However, it's important to run the app first to allow macOS to verify the app bundle's signature.
3. Find TR3X in your Applications folder. Right-click it and click "Show Package Contents".
4. Copy your Tomb Raider 3 game data files into `Contents/Resources`. (See the Windows / Linux instructions for retrieving game data from e.g. GOG.)

In case you see a popup "TR3X is damaged" when you run the game, run `xattr -cr /Applications/TR3X.app`.

## Verifying the installation

If you install everything correctly, your game directory should look more or less like this (click to expand):

<details data-id="file-tree-mac">
<pre><code>.
└── Contents
    ├── _CodeSignature
    ├── Frameworks
    ├── info.plist
    ├── MacOS
    └── Resources
        ├── audio
        │   └── cdaudio.wad
        ├── cfg
        │   ├── base_strings-de.json5
        │   ├── base_strings-en-gb.json5
        │   ├── base_strings-fr.json5
        │   ├── base_strings-gd.json5
        │   ├── base_strings-it.json5
        │   ├── base_strings-pl.json5
        │   ├── base_strings-ru.json5
        │   ├── base_strings.json5
        │   ├── catalog_item_actions.csv
        │   ├── catalog_lara_anims.csv
        │   ├── catalog_lara_states.csv
        │   ├── catalog_music.csv
        │   ├── catalog_objects.csv
        │   ├── catalog_samples.csv
        │   ├── inv_ring.json5
        │   ├── poses.json5
        │   ├── tr2
        │   │   ├── gameflow.json5
        │   │   ├── strings-de.json5
        │   │   ├── strings-en-gb.json5
        │   │   ├── strings-fr.json5
        │   │   ├── strings-gd.json5
        │   │   ├── strings-it.json5
        │   │   ├── strings-pl.json5
        │   │   └── strings.json5
        │   ├── tr2-gm
        │   │   ├── gameflow.json5
        │   │   ├── strings-de.json5
        │   │   ├── strings-fr.json5
        │   │   ├── strings-gd.json5
        │   │   ├── strings-it.json5
        │   │   ├── strings-pl.json5
        │   │   └── strings.json5
        │   ├── tr2-level
        │   │   ├── gameflow.json5
        │   │   ├── strings-de.json5
        │   │   ├── strings-fr.json5
        │   │   ├── strings-gd.json5
        │   │   ├── strings-it.json5
        │   │   ├── strings-pl.json5
        │   │   └── strings.json5
        │   ├── ui.json5
        │   └── weapons.json5
        ├── cuts
        │   ├── cut1.tr2
        │   ├── cut2.tr2
        │   ├── cut3.tr2
        │   ├── cut4.tr2
        │   ├── cut5.tr2
        │   ├── cut6.tr2
        │   ├── cut7.tr2
        │   ├── cut8.tr2
        │   ├── cut9.tr2
        │   ├── cut11.tr2
        │   └── cut12.tr2
        ├── data
        │   ├── antarc.tr2
        │   ├── area51.tr2
        │   ├── chamber.tr2
        │   ├── city.tr2
        │   ├── compound.tr2
        │   ├── crash.tr2
        │   ├── house.tr2
        │   ├── images
        │   │   ├── 4x3
        │   │   │   ├── antarc.webp
        │   │   │   ├── credit01.webp
        │   │   │   ├── credit02.webp
        │   │   │   ├── credit03.webp
        │   │   │   ├── credit04.webp
        │   │   │   ├── credit05.webp
        │   │   │   ├── credit06.webp
        │   │   │   ├── credit07.webp
        │   │   │   ├── credit08.webp
        │   │   │   ├── credit09.webp
        │   │   │   ├── house.webp
        │   │   │   ├── india.webp
        │   │   │   ├── legal_eu.webp
        │   │   │   ├── legal_us.webp
        │   │   │   ├── london.webp
        │   │   │   ├── nevada.webp
        │   │   │   ├── southpac.webp
        │   │   │   ├── theend2.webp
        │   │   │   ├── title_eu.webp
        │   │   │   └── title_us.webp
        │   │   ├── antarc.webp
        │   │   ├── credit01.webp
        │   │   ├── credit02.webp
        │   │   ├── credit03.webp
        │   │   ├── credit04.webp
        │   │   ├── credit05.webp
        │   │   ├── credit06.webp
        │   │   ├── credit07.webp
        │   │   ├── credit08.webp
        │   │   ├── credit09.webp
        │   │   ├── house.webp
        │   │   ├── india.webp
        │   │   ├── legal_eu.webp
        │   │   ├── legal_us.webp
        │   │   ├── london.webp
        │   │   ├── nevada.webp
        │   │   ├── og
        │   │   │   ├── antarc.webp
        │   │   │   ├── credit01.webp
        │   │   │   ├── credit02.webp
        │   │   │   ├── credit03.webp
        │   │   │   ├── credit04.webp
        │   │   │   ├── credit05.webp
        │   │   │   ├── credit06.webp
        │   │   │   ├── credit07.webp
        │   │   │   ├── credit08.webp
        │   │   │   ├── credit09.webp
        │   │   │   ├── house.webp
        │   │   │   ├── india.webp
        │   │   │   ├── legal_eu.webp
        │   │   │   ├── legal_us.webp
        │   │   │   ├── london.webp
        │   │   │   ├── nevada.webp
        │   │   │   ├── nevadafff.webp
        │   │   │   ├── southpac.webp
        │   │   │   ├── theend2.webp
        │   │   │   ├── theend.webp
        │   │   │   ├── title_eu.webp
        │   │   │   └── title_us.webp
        │   │   ├── southpac.webp
        │   │   ├── theend2.webp
        │   │   ├── title_eu.webp
        │   │   └── title_us.webp
        │   ├── injections
        │   │   ├── barefoot_sfx.bin
        │   │   ├── barkhang_cameras.bin
        │   │   ├── barkhang_fd.bin
        │   │   ├── barkhang_itemrots.bin
        │   │   ├── barkhang_music_tracks.bin
        │   │   ├── barkhang_pickup_meshes.bin
        │   │   ├── barkhang_textures.bin
        │   │   ├── bartoli_music_tracks.bin
        │   │   ├── bartoli_secret_fd.bin
        │   │   ├── bartoli_textures.bin
        │   │   ├── boat_bits.bin
        │   │   ├── breakable_tile_sfx.bin
        │   │   ├── catacombs_fd.bin
        │   │   ├── catacombs_itemrots.bin
        │   │   ├── catacombs_music_tracks.bin
        │   │   ├── catacombs_textures.bin
        │   │   ├── coldwar_fd.bin
        │   │   ├── coldwar_itemrots.bin
        │   │   ├── coldwar_music_tracks.bin
        │   │   ├── coldwar_objects.bin
        │   │   ├── coldwar_textures.bin
        │   │   ├── common_pickup_meshes.bin
        │   │   ├── common_pickup_meshes_gm.bin
        │   │   ├── cut2_textures.bin
        │   │   ├── cut3_setup.bin
        │   │   ├── cut3_textures.bin
        │   │   ├── cut4_textures.bin
        │   │   ├── dagger_sprite.bin
        │   │   ├── deck_cameras.bin
        │   │   ├── deck_fd.bin
        │   │   ├── deck_itemrots.bin
        │   │   ├── deck_music_tracks.bin
        │   │   ├── deck_pickup_meshes.bin
        │   │   ├── deck_plants.bin
        │   │   ├── deck_secret_fd.bin
        │   │   ├── deck_textures.bin
        │   │   ├── diving_cameras.bin
        │   │   ├── diving_itemrots.bin
        │   │   ├── diving_music_tracks.bin
        │   │   ├── diving_pickup_meshes.bin
        │   │   ├── diving_sfx.bin
        │   │   ├── diving_textures.bin
        │   │   ├── door106_sfx.bin
        │   │   ├── door107_sfx.bin
        │   │   ├── door108_sfx.bin
        │   │   ├── door110_sfx.bin
        │   │   ├── door111_sfx.bin
        │   │   ├── explosion.bin
        │   │   ├── fathoms_goon_sfx.bin
        │   │   ├── fathoms_itemrots.bin
        │   │   ├── fathoms_music_tracks.bin
        │   │   ├── fathoms_plants.bin
        │   │   ├── fathoms_secret_fd.bin
        │   │   ├── fathoms_textures.bin
        │   │   ├── floating_fd.bin
        │   │   ├── floating_itemrots.bin
        │   │   ├── floating_music_tracks.bin
        │   │   ├── floating_pickup_meshes.bin
        │   │   ├── floating_textures.bin
        │   │   ├── font.bin
        │   │   ├── fools_itemrots.bin
        │   │   ├── fools_music_tracks.bin
        │   │   ├── fools_pickup_meshes.bin
        │   │   ├── fools_textures.bin
        │   │   ├── furnace_itemrots.bin
        │   │   ├── furnace_music_tracks.bin
        │   │   ├── furnace_objects.bin
        │   │   ├── furnace_pickup_meshes.bin
        │   │   ├── furnace_textures.bin
        │   │   ├── guardian_death_commands.bin
        │   │   ├── gym_fd.bin
        │   │   ├── gym_music_tracks.bin
        │   │   ├── gym_sfx.bin
        │   │   ├── gym_textures.bin
        │   │   ├── house_itemrots.bin
        │   │   ├── house_music_tracks.bin
        │   │   ├── house_sfx.bin
        │   │   ├── house_shower_frames.bin
        │   │   ├── house_textures.bin
        │   │   ├── inv_background.bin
        │   │   ├── kingdom_cameras.bin
        │   │   ├── kingdom_itemrots.bin
        │   │   ├── kingdom_music_tracks.bin
        │   │   ├── kingdom_textures.bin
        │   │   ├── lair_bartolipos.bin
        │   │   ├── lair_music_tracks.bin
        │   │   ├── lair_textures.bin
        │   │   ├── lara_animations.bin
        │   │   ├── lara_extra.bin
        │   │   ├── lara_guns.bin
        │   │   ├── lara_gym_guns.bin
        │   │   ├── lara_house_guns.bin
        │   │   ├── lara_rifle_sfx.bin
        │   │   ├── lara_unwater_guns.bin
        │   │   ├── lara_vegas_guns.bin
        │   │   ├── living_deck_goon_sfx.bin
        │   │   ├── living_fd.bin
        │   │   ├── living_itemrots.bin
        │   │   ├── living_music_tracks.bin
        │   │   ├── living_pickup_meshes.bin
        │   │   ├── living_secret_fd.bin
        │   │   ├── living_sfx.bin
        │   │   ├── living_textures.bin
        │   │   ├── loose_boards_sfx.bin
        │   │   ├── misc_sprites.bin
        │   │   ├── opera_fd.bin
        │   │   ├── opera_itemrots.bin
        │   │   ├── opera_music_tracks.bin
        │   │   ├── opera_sfx.bin
        │   │   ├── opera_textures.bin
        │   │   ├── palace_fd.bin
        │   │   ├── palace_itemrots.bin
        │   │   ├── palace_music_tracks.bin
        │   │   ├── palace_secret_fd.bin
        │   │   ├── palace_textures.bin
        │   │   ├── pda_model.bin
        │   │   ├── photo.bin
        │   │   ├── pickup_aid.bin
        │   │   ├── portcullis_sfx.bin
        │   │   ├── purple_crystal.bin
        │   │   ├── rig_itemrots.bin
        │   │   ├── rig_music_tracks.bin
        │   │   ├── rig_pickup_meshes.bin
        │   │   ├── rig_textures.bin
        │   │   ├── scuba_sfx.bin
        │   │   ├── seaweed_collision.bin
        │   │   ├── secret_models_gm.bin
        │   │   ├── secret_models_og.bin
        │   │   ├── shark_sfx.bin
        │   │   ├── tibet_fd.bin
        │   │   ├── tibet_itemrots.bin
        │   │   ├── tibet_music_tracks.bin
        │   │   ├── tibet_textures.bin
        │   │   ├── title_textures.bin
        │   │   ├── vegas_fd.bin
        │   │   ├── vegas_itemrots.bin
        │   │   ├── vegas_music_tracks.bin
        │   │   ├── vegas_textures.bin
        │   │   ├── venice_fd.bin
        │   │   ├── venice_itemrots.bin
        │   │   ├── venice_music_tracks.bin
        │   │   ├── venice_textures.bin
        │   │   ├── wall_cameras.bin
        │   │   ├── wall_itemrots.bin
        │   │   ├── wall_music_tracks.bin
        │   │   ├── wall_textures.bin
        │   │   ├── winston_model.bin
        │   │   ├── wreck_cameras.bin
        │   │   ├── wreck_fd.bin
        │   │   ├── wreck_goon_sfx.bin
        │   │   ├── wreck_itemrots.bin
        │   │   ├── wreck_music_tracks.bin
        │   │   ├── wreck_pickup_meshes.bin
        │   │   ├── wreck_plants.bin
        │   │   ├── wreck_secret_fd.bin
        │   │   ├── wreck_textures.bin
        │   │   ├── xian_fd.bin
        │   │   ├── xian_itemrots.bin
        │   │   ├── xian_music_tracks.bin
        │   │   ├── xian_pickup_meshes.bin
        │   │   ├── xian_sfx.bin
        │   │   └── xian_textures.bin
        │   ├── jungle.tr2
        │   ├── main.sfx
        │   ├── mines.tr2
        │   ├── nevada.tr2
        │   ├── office.tr2
        │   ├── quadchas.tr2
        │   ├── rapids.tr2
        │   ├── roofs.tr2
        │   ├── scripts
        │   │   ├── assault.lua
        │   │   ├── floating.lua
        │   │   ├── level1.lua
        │   │   ├── level3.lua
        │   │   ├── level4.lua
        │   │   └── monastry.lua
        │   ├── sewer.tr2
        │   ├── shore.tr2
        │   ├── stpaul.tr2
        │   ├── temple.tr2
        │   ├── title.tr2
        │   ├── tombpc.dat
        │   ├── tonyboss.tr2
        │   ├── tower.tr2
        │   ├── triboss.tr2
        │   └── vict.tr2
        ├── fmv
        │   ├── crsh_eng.rpl
        │   ├── endgame.rpl
        │   ├── intr_eng.rpl
        │   ├── logo.rpl
        │   └── sail_eng.rpl
        ├── icon.icns
        └── shaders
            ├── 2d.glsl
            ├── billboard.glsl
            ├── common.glsl
            ├── fbo.glsl
            ├── lights.glsl
            ├── meshes.glsl
            ├── meshes_tr3.glsl
            ├── meshes_tr12.glsl
            └── ui.glsl</code></pre>
</details>

*\* Will not be present until the game has been launched.*
