# Windows (installer)

## Installing (simplified)

1. Download the latest combined TRX installer.
2. Choose a destination directory for TRX itself.
3. Select whichever game packs you want to add:
   - `TR1`, `TR2`, and `TR3` from your original Steam, GOG, or disc installs.
   - `TR1:UB`, `TR1 PC Demo`, and `TR2:GM` as direct downloads from Lost Artefacts.
   - `TR3:LA` from your original disc install.
4. Let the installer remap the original files into the combined `games/<game-id>/` hierarchy.

> [!NOTE]
> When downloading TRX, you might see a warning from Windows Defender, your browser, or another security tool. Modern antivirus systems use AI-based heuristics - they flag anything uncommon or unsigned as suspicious, even if it's perfectly safe. TRX can trigger these alerts because:
>
> - It isn't signed with an expensive commercial certificate.
> - It’s a small, community-made project, not a big corporate release, meaning tools err on the side of caution.
> - It comes straight from us, not the Microsoft Store.
>
> TRX is open-source, so if you're curious, you can peek at everything on [GitHub](https://github.com/LostArtefacts/TRX/).

# Windows / Linux

## Installing (manual)

1. Download the combined TRX zip file.
2. Extract the zip file into a directory of your choice.
   Make sure you choose to overwrite existing directories and files.
3. Copy your original game files into the new combined hierarchy. Please refer
   to ["Manually copying original game files"](#manually-copying-original-game-files)
   section in this document.

## Split installs and custom paths

By default, TRX expects a bundled layout where the executable sits next to
the `cfg/` and `games/` directories. If you want to keep the executable in a
different location, such as `/usr/local/bin`, you can override the runtime
paths with environment variables:

- `TRX_CONFIG_DIR`  
  Defaults to `<trx_dir>/cfg`.
- `TRX_CACHE_DIR`  
  Defaults to `<trx_dir>/cache`.
- `TRX_GAMES_DIR`  
  Defaults to `<trx_dir>/games`.
- `TRX_SCREENSHOTS_DIR`  
  Defaults to `<trx_dir>/screenshots`.
- `TRX_SAVES_DIR`  
  Defaults to `<trx_dir>/saves`.

For example, a wrapper script can keep the binary in one location while
pointing TRX at a separate game-data directory:

```sh
#!/bin/sh
export TRX_GAMES_DIR="$HOME/Work/TRX/games"
export TRX_CONFIG_DIR="$HOME/Work/TRX/cfg"
export TRX_SAVES_DIR="$HOME/Work/TRX/saves"
export TRX_SCREENSHOTS_DIR="$HOME/Work/TRX/screenshots"
exec /usr/local/bin/TRX.bin "$@"
```

This is especially useful for package managers and ports that install the
binary outside the main TRX data tree.

## Verifying the installation

If you install everything correctly, your game directory should look more or less like this (click to expand):

<details data-id="file-tree-win">
<pre><code>.
├── cfg
│   ├── presets
│   │   ├── tr1-moveset.json5
│   │   ├── tr1-pc.json5
│   │   ├── tr1-ps1.json5
│   │   ├── tr2-moveset.json5
│   │   ├── tr2-pc.json5
│   │   ├── tr2-ps1.json5
│   │   ├── tr3-moveset.json5
│   │   ├── tr3-pc.json5
│   │   └── tr3-ps1.json5
│   ├── shaders
│   │   ├── 2d.glsl
│   │   ├── billboard.glsl
│   │   ├── common.glsl
│   │   ├── fbo.glsl
│   │   ├── lights.glsl
│   │   ├── lights_common.glsl
│   │   ├── lights_tr3.glsl
│   │   ├── lights_tr4.glsl
│   │   ├── lights_tr12.glsl
│   │   ├── meshes.glsl
│   │   ├── meshes_tr3.glsl
│   │   ├── meshes_tr4.glsl
│   │   ├── meshes_tr12.glsl
│   │   └── ui.glsl
│   ├── base_strings-de.json5
│   ├── base_strings-en-gb.json5
│   ├── base_strings-fr.json5
│   ├── base_strings-gd.json5
│   ├── base_strings-it.json5
│   ├── base_strings-pl.json5
│   ├── base_strings-ru.json5
│   ├── base_strings.json5
│   ├── outfits.json5
│   ├── poses.json5
│   ├── shell.json5*
│   ├── TR1X.json5*
│   ├── TR2X.json5*
│   ├── TR3X.json5*
│   └── ui.json5
├── games
│   ├── tr1
│   │   ├── fmv
│   │   │   ├── cafe.rpl
│   │   │   ├── canyon.rpl
│   │   │   ├── core.avi
│   │   │   ├── end.rpl
│   │   │   ├── escape.rpl
│   │   │   ├── lift.rpl
│   │   │   ├── mansion.rpl
│   │   │   ├── prison.rpl
│   │   │   ├── pyramid.rpl
│   │   │   ├── snow.rpl
│   │   │   └── vision.rpl
│   │   ├── images
│   │   │   ├── atlantis.webp
│   │   │   ├── credits_1.webp
│   │   │   ├── credits_2.webp
│   │   │   ├── credits_3.webp
│   │   │   ├── credits_3_alt.webp
│   │   │   ├── credits_ps1.webp
│   │   │   ├── egypt.webp
│   │   │   ├── eidos.webp
│   │   │   ├── end.webp
│   │   │   ├── greece.webp
│   │   │   ├── greece_saturn.webp
│   │   │   ├── gym.webp
│   │   │   ├── install.webp
│   │   │   ├── peru.webp
│   │   │   ├── title.webp
│   │   │   └── title_og_alt.webp
│   │   ├── injections
│   │   │   ├── atlantis_door_sfx.bin
│   │   │   ├── atlantis_fd.bin
│   │   │   ├── atlantis_itemrots.bin
│   │   │   ├── atlantis_textures.bin
│   │   │   ├── binoculars.bin
│   │   │   ├── bubbles.bin
│   │   │   ├── cat_cameras.bin
│   │   │   ├── cat_crystals.bin
│   │   │   ├── cat_fd.bin
│   │   │   ├── cat_itemrots.bin
│   │   │   ├── cat_meshfixes.bin
│   │   │   ├── cat_textures.bin
│   │   │   ├── caves_fd.bin
│   │   │   ├── caves_itemrots.bin
│   │   │   ├── caves_textures.bin
│   │   │   ├── cistern_fd.bin
│   │   │   ├── cistern_itemrots.bin
│   │   │   ├── cistern_plants.bin
│   │   │   ├── cistern_skybox.bin
│   │   │   ├── cistern_textures.bin
│   │   │   ├── colosseum_fd.bin
│   │   │   ├── colosseum_itemrots.bin
│   │   │   ├── colosseum_skybox.bin
│   │   │   ├── colosseum_textures.bin
│   │   │   ├── crystal.bin
│   │   │   ├── cut1_setup.bin
│   │   │   ├── cut1_textures.bin
│   │   │   ├── cut2_setup.bin
│   │   │   ├── cut3_setup.bin
│   │   │   ├── cut3_textures.bin
│   │   │   ├── cut4_setup.bin
│   │   │   ├── cut4_textures.bin
│   │   │   ├── door58_frames.bin
│   │   │   ├── door59_frames.bin
│   │   │   ├── door59_sfx.bin
│   │   │   ├── door60_frames.bin
│   │   │   ├── door61_sfx.bin
│   │   │   ├── egypt_cameras.bin
│   │   │   ├── egypt_crystals.bin
│   │   │   ├── egypt_fd.bin
│   │   │   ├── egypt_itemrots.bin
│   │   │   ├── egypt_meshfixes.bin
│   │   │   ├── egypt_textures.bin
│   │   │   ├── explosion.bin
│   │   │   ├── folly_fd.bin
│   │   │   ├── folly_itemrots.bin
│   │   │   ├── folly_pickup_meshes.bin
│   │   │   ├── folly_textures.bin
│   │   │   ├── font.bin
│   │   │   ├── gun_glow.bin
│   │   │   ├── gym_textures.bin
│   │   │   ├── hive_crystals.bin
│   │   │   ├── hive_fd.bin
│   │   │   ├── hive_itemrots.bin
│   │   │   ├── hive_textures.bin
│   │   │   ├── inv_background.bin
│   │   │   ├── khamoon_fd.bin
│   │   │   ├── khamoon_itemrots.bin
│   │   │   ├── khamoon_meshfixes.bin
│   │   │   ├── khamoon_mummy.bin
│   │   │   ├── khamoon_textures.bin
│   │   │   ├── lara_animations.bin
│   │   │   ├── lara_extra.bin
│   │   │   ├── lara_feet_sfx.bin
│   │   │   ├── lara_guns.bin
│   │   │   ├── lara_gym_guns.bin
│   │   │   ├── lara_outfits.bin
│   │   │   ├── midas_itemrots.bin
│   │   │   ├── midas_textures.bin
│   │   │   ├── mines_cameras.bin
│   │   │   ├── mines_door_sfx.bin
│   │   │   ├── mines_fd.bin
│   │   │   ├── mines_itemrots.bin
│   │   │   ├── mines_meshfixes.bin
│   │   │   ├── mines_pushblocks.bin
│   │   │   ├── mines_textures.bin
│   │   │   ├── misc_sprites.bin
│   │   │   ├── obelisk_fd.bin
│   │   │   ├── obelisk_itemrots.bin
│   │   │   ├── obelisk_meshfixes.bin
│   │   │   ├── obelisk_skybox.bin
│   │   │   ├── obelisk_textures.bin
│   │   │   ├── panther_sfx.bin
│   │   │   ├── pda_model.bin
│   │   │   ├── photo.bin
│   │   │   ├── pickup_aid.bin
│   │   │   ├── pyramid_fd.bin
│   │   │   ├── pyramid_itemrots.bin
│   │   │   ├── pyramid_textures.bin
│   │   │   ├── qualopec_door_sfx.bin
│   │   │   ├── qualopec_fd.bin
│   │   │   ├── qualopec_itemrots.bin
│   │   │   ├── qualopec_textures.bin
│   │   │   ├── sanctuary_fd.bin
│   │   │   ├── sanctuary_itemrots.bin
│   │   │   ├── sanctuary_meshfixes.bin
│   │   │   ├── sanctuary_scion.bin
│   │   │   ├── sanctuary_textures.bin
│   │   │   ├── scion_alignment.bin
│   │   │   ├── scion_collision.bin
│   │   │   ├── shimmy_sfx.bin
│   │   │   ├── skate_kid_sfx.bin
│   │   │   ├── sprite_alignment.bin
│   │   │   ├── stronghold_crystals.bin
│   │   │   ├── stronghold_fd.bin
│   │   │   ├── stronghold_itemrots.bin
│   │   │   ├── stronghold_textures.bin
│   │   │   ├── tihocan_fd.bin
│   │   │   ├── tihocan_itemrots.bin
│   │   │   ├── tihocan_skybox.bin
│   │   │   ├── tihocan_textures.bin
│   │   │   ├── title_textures.bin
│   │   │   ├── uw_switch_sfx.bin
│   │   │   ├── uzi_sfx.bin
│   │   │   ├── valley_fd.bin
│   │   │   ├── valley_itemrots.bin
│   │   │   ├── valley_skybox.bin
│   │   │   ├── valley_textures.bin
│   │   │   ├── vilcabamba_door_sfx.bin
│   │   │   ├── vilcabamba_itemrots.bin
│   │   │   ├── vilcabamba_textures.bin
│   │   │   ├── wall_switch_sfx.bin
│   │   │   └── winston_model.bin
│   │   ├── levels
│   │   │   ├── cut1.phd
│   │   │   ├── cut2.phd
│   │   │   ├── cut3.phd
│   │   │   ├── cut4.phd
│   │   │   ├── gym.phd
│   │   │   ├── level1.phd
│   │   │   ├── level2.phd
│   │   │   ├── level3a.phd
│   │   │   ├── level3b.phd
│   │   │   ├── level4.phd
│   │   │   ├── level5.phd
│   │   │   ├── level6.phd
│   │   │   ├── level7a.phd
│   │   │   ├── level7b.phd
│   │   │   ├── level8a.phd
│   │   │   ├── level8b.phd
│   │   │   ├── level8c.phd
│   │   │   ├── level10a.phd
│   │   │   ├── level10b.phd
│   │   │   ├── level10c.phd
│   │   │   └── title.phd
│   │   ├── music
│   │   │   ├── track02.flac
│   │   │   ├── track03.flac
│   │   │   ├── track04.flac
│   │   │   ├── track05.flac
│   │   │   ├── track06.flac
│   │   │   ├── track07.flac
│   │   │   ├── track08.flac
│   │   │   ├── track09.flac
│   │   │   ├── track10.flac
│   │   │   ├── track11.flac
│   │   │   ├── track12.flac
│   │   │   ├── track13.flac
│   │   │   ├── track14.flac
│   │   │   ├── track15.flac
│   │   │   ├── track16.flac
│   │   │   ├── track17.flac
│   │   │   ├── track18.flac
│   │   │   ├── track19.flac
│   │   │   ├── track20.flac
│   │   │   ├── track21.flac
│   │   │   ├── track22.flac
│   │   │   ├── track23.flac
│   │   │   ├── track24.flac
│   │   │   ├── track25.flac
│   │   │   ├── track26.flac
│   │   │   ├── track27.flac
│   │   │   ├── track28.flac
│   │   │   ├── track29.flac
│   │   │   ├── track30.flac
│   │   │   ├── track31.flac
│   │   │   ├── track32.flac
│   │   │   ├── track33.flac
│   │   │   ├── track34.flac
│   │   │   ├── track35.flac
│   │   │   ├── track36.flac
│   │   │   ├── track37.flac
│   │   │   ├── track38.flac
│   │   │   ├── track39.flac
│   │   │   ├── track40.flac
│   │   │   ├── track41.flac
│   │   │   ├── track42.flac
│   │   │   ├── track43.flac
│   │   │   ├── track44.flac
│   │   │   ├── track45.flac
│   │   │   ├── track46.flac
│   │   │   ├── track47.flac
│   │   │   ├── track48.flac
│   │   │   ├── track49.flac
│   │   │   ├── track50.flac
│   │   │   ├── track51.flac
│   │   │   ├── track52.flac
│   │   │   ├── track53.flac
│   │   │   ├── track54.flac
│   │   │   ├── track55.flac
│   │   │   ├── track56.flac
│   │   │   ├── track57.flac
│   │   │   ├── track58.flac
│   │   │   ├── track59.flac
│   │   │   └── track60.flac
│   │   ├── scripts
│   │   │   ├── _game.lua
│   │   │   ├── gym.lua
│   │   │   ├── level3b.lua
│   │   │   ├── level8c.lua
│   │   │   └── level10b.lua
│   │   ├── catalog_item_actions.csv
│   │   ├── catalog_lara_anims.csv
│   │   ├── catalog_lara_states.csv
│   │   ├── catalog_music.csv
│   │   ├── catalog_objects.csv
│   │   ├── catalog_samples.csv
│   │   ├── gameflow.json5
│   │   ├── inv_ring.json5
│   │   ├── strings-de.json5
│   │   ├── strings-en-gb.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   ├── strings-ru.json5
│   │   ├── strings.json5
│   │   └── weapons.json5
│   ├── tr1-demo-pc
│   │   ├── gameflow.json5
│   │   ├── strings-de.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   ├── strings-ru.json5
│   │   └── strings.json5
│   ├── tr1-level
│   │   ├── gameflow.json5
│   │   ├── strings-de.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   ├── strings-ru.json5
│   │   └── strings.json5
│   ├── tr1-ub
│   │   ├── images
│   │   │   ├── credits_ub.webp
│   │   │   ├── title_ub.webp
│   │   │   ├── ub_loading1.webp
│   │   │   └── ub_loading2.webp
│   │   ├── levels
│   │   │   ├── cat.phd
│   │   │   ├── egypt.phd
│   │   │   ├── end2.phd
│   │   │   └── end.phd
│   │   ├── scripts
│   │   │   └── _game.lua
│   │   ├── gameflow.json5
│   │   ├── strings-de.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   ├── strings-ru.json5
│   │   └── strings.json5
│   ├── tr2
│   │   ├── fmv
│   │   │   ├── ancient.rpl
│   │   │   ├── crash.rpl
│   │   │   ├── end.rpl
│   │   │   ├── jeep.rpl
│   │   │   ├── landing.rpl
│   │   │   ├── logo.rpl
│   │   │   ├── modern.rpl
│   │   │   └── ms.rpl
│   │   ├── images
│   │   │   ├── 3x2
│   │   │   │   ├── china.webp
│   │   │   │   ├── credit01.webp
│   │   │   │   ├── credit02.webp
│   │   │   │   ├── credit03.webp
│   │   │   │   ├── credit04.webp
│   │   │   │   ├── credit05.webp
│   │   │   │   ├── credit06.webp
│   │   │   │   ├── credit07.webp
│   │   │   │   ├── credit08.webp
│   │   │   │   ├── end.webp
│   │   │   │   ├── legal_eu.webp
│   │   │   │   ├── legal_us.webp
│   │   │   │   ├── mansion.webp
│   │   │   │   ├── rig.webp
│   │   │   │   ├── tibet.webp
│   │   │   │   ├── titan.webp
│   │   │   │   ├── title_eu.webp
│   │   │   │   ├── title_us.webp
│   │   │   │   └── venice.webp
│   │   │   ├── 4x3
│   │   │   │   ├── china.webp
│   │   │   │   ├── credit01.webp
│   │   │   │   ├── credit02.webp
│   │   │   │   ├── credit03.webp
│   │   │   │   ├── credit04.webp
│   │   │   │   ├── credit05.webp
│   │   │   │   ├── credit06.webp
│   │   │   │   ├── credit07.webp
│   │   │   │   ├── credit08.webp
│   │   │   │   ├── end.webp
│   │   │   │   ├── legal_eu.webp
│   │   │   │   ├── legal_us.webp
│   │   │   │   ├── mansion.webp
│   │   │   │   ├── rig.webp
│   │   │   │   ├── tibet.webp
│   │   │   │   ├── titan.webp
│   │   │   │   ├── title_eu.webp
│   │   │   │   ├── title_us.webp
│   │   │   │   └── venice.webp
│   │   │   ├── og
│   │   │   │   ├── china.webp
│   │   │   │   ├── credit01.webp
│   │   │   │   ├── credit02.webp
│   │   │   │   ├── credit03.webp
│   │   │   │   ├── credit04.webp
│   │   │   │   ├── credit05.webp
│   │   │   │   ├── credit06.webp
│   │   │   │   ├── credit07.webp
│   │   │   │   ├── credit08.webp
│   │   │   │   ├── end.webp
│   │   │   │   ├── legal.webp
│   │   │   │   ├── mansion.webp
│   │   │   │   ├── rig.webp
│   │   │   │   ├── tibet.webp
│   │   │   │   ├── titan.webp
│   │   │   │   ├── title_eu.webp
│   │   │   │   ├── title_us.webp
│   │   │   │   └── venice.webp
│   │   │   ├── china.webp
│   │   │   ├── credit01.webp
│   │   │   ├── credit02.webp
│   │   │   ├── credit03.webp
│   │   │   ├── credit04.webp
│   │   │   ├── credit05.webp
│   │   │   ├── credit06.webp
│   │   │   ├── credit07.webp
│   │   │   ├── credit08.webp
│   │   │   ├── end.webp
│   │   │   ├── legal_eu.webp
│   │   │   ├── legal_us.webp
│   │   │   ├── mansion.webp
│   │   │   ├── rig.webp
│   │   │   ├── tibet.webp
│   │   │   ├── titan.webp
│   │   │   ├── title_eu.webp
│   │   │   ├── title_us.webp
│   │   │   └── venice.webp
│   │   ├── injections
│   │   │   ├── barkhang_cameras.bin
│   │   │   ├── barkhang_crystals.bin
│   │   │   ├── barkhang_fd.bin
│   │   │   ├── barkhang_itemrots.bin
│   │   │   ├── barkhang_music_tracks.bin
│   │   │   ├── barkhang_pickup_meshes.bin
│   │   │   ├── barkhang_sfx.bin
│   │   │   ├── barkhang_textures.bin
│   │   │   ├── bartoli_crystals.bin
│   │   │   ├── bartoli_music_tracks.bin
│   │   │   ├── bartoli_secret_fd.bin
│   │   │   ├── bartoli_textures.bin
│   │   │   ├── binoculars.bin
│   │   │   ├── boat_bits.bin
│   │   │   ├── boat_sfx.bin
│   │   │   ├── breakable_tile_sfx.bin
│   │   │   ├── catacombs_crystals.bin
│   │   │   ├── catacombs_fd.bin
│   │   │   ├── catacombs_itemrots.bin
│   │   │   ├── catacombs_music_tracks.bin
│   │   │   ├── catacombs_textures.bin
│   │   │   ├── coldwar_crystals.bin
│   │   │   ├── coldwar_fd.bin
│   │   │   ├── coldwar_itemrots.bin
│   │   │   ├── coldwar_music_tracks.bin
│   │   │   ├── coldwar_objects.bin
│   │   │   ├── coldwar_textures.bin
│   │   │   ├── common_pickup_meshes.bin
│   │   │   ├── common_pickup_meshes_gm.bin
│   │   │   ├── crystal.bin
│   │   │   ├── cut2_setup.bin
│   │   │   ├── cut2_textures.bin
│   │   │   ├── cut3_setup.bin
│   │   │   ├── cut3_textures.bin
│   │   │   ├── cut4_setup.bin
│   │   │   ├── cut4_textures.bin
│   │   │   ├── dagger_sprite.bin
│   │   │   ├── deck_cameras.bin
│   │   │   ├── deck_crystals.bin
│   │   │   ├── deck_fd.bin
│   │   │   ├── deck_itemrots.bin
│   │   │   ├── deck_music_tracks.bin
│   │   │   ├── deck_pickup_meshes.bin
│   │   │   ├── deck_plants.bin
│   │   │   ├── deck_secret_fd.bin
│   │   │   ├── deck_textures.bin
│   │   │   ├── detonator_lights.bin
│   │   │   ├── diving_cameras.bin
│   │   │   ├── diving_crystals.bin
│   │   │   ├── diving_itemrots.bin
│   │   │   ├── diving_music_tracks.bin
│   │   │   ├── diving_pickup_meshes.bin
│   │   │   ├── diving_sfx.bin
│   │   │   ├── diving_textures.bin
│   │   │   ├── door106_sfx.bin
│   │   │   ├── door107_sfx.bin
│   │   │   ├── door108_sfx.bin
│   │   │   ├── door110_sfx.bin
│   │   │   ├── door111_sfx.bin
│   │   │   ├── explosion.bin
│   │   │   ├── fathoms_crystals.bin
│   │   │   ├── fathoms_goon_sfx.bin
│   │   │   ├── fathoms_itemrots.bin
│   │   │   ├── fathoms_music_tracks.bin
│   │   │   ├── fathoms_plants.bin
│   │   │   ├── fathoms_secret_fd.bin
│   │   │   ├── fathoms_textures.bin
│   │   │   ├── floating_crystals.bin
│   │   │   ├── floating_fd.bin
│   │   │   ├── floating_itemrots.bin
│   │   │   ├── floating_music_tracks.bin
│   │   │   ├── floating_pickup_meshes.bin
│   │   │   ├── floating_textures.bin
│   │   │   ├── font.bin
│   │   │   ├── fools_crystals.bin
│   │   │   ├── fools_itemrots.bin
│   │   │   ├── fools_music_tracks.bin
│   │   │   ├── fools_pickup_meshes.bin
│   │   │   ├── fools_textures.bin
│   │   │   ├── furnace_crystals.bin
│   │   │   ├── furnace_itemrots.bin
│   │   │   ├── furnace_music_tracks.bin
│   │   │   ├── furnace_objects.bin
│   │   │   ├── furnace_pickup_meshes.bin
│   │   │   ├── furnace_textures.bin
│   │   │   ├── guardian_death_commands.bin
│   │   │   ├── gym_fd.bin
│   │   │   ├── gym_music_tracks.bin
│   │   │   ├── gym_sfx.bin
│   │   │   ├── gym_textures.bin
│   │   │   ├── house_itemrots.bin
│   │   │   ├── house_music_tracks.bin
│   │   │   ├── house_sfx.bin
│   │   │   ├── house_shower_frames.bin
│   │   │   ├── house_textures.bin
│   │   │   ├── inv_background.bin
│   │   │   ├── kingdom_cameras.bin
│   │   │   ├── kingdom_crystals.bin
│   │   │   ├── kingdom_itemrots.bin
│   │   │   ├── kingdom_music_tracks.bin
│   │   │   ├── kingdom_textures.bin
│   │   │   ├── lair_bartolipos.bin
│   │   │   ├── lair_crystals.bin
│   │   │   ├── lair_music_tracks.bin
│   │   │   ├── lair_textures.bin
│   │   │   ├── lara_animations.bin
│   │   │   ├── lara_extra.bin
│   │   │   ├── lara_guns.bin
│   │   │   ├── lara_gym_guns.bin
│   │   │   ├── lara_house_guns.bin
│   │   │   ├── lara_outfits.bin
│   │   │   ├── lara_rifle_sfx.bin
│   │   │   ├── lara_vegas_guns.bin
│   │   │   ├── living_crystals.bin
│   │   │   ├── living_deck_goon_sfx.bin
│   │   │   ├── living_fd.bin
│   │   │   ├── living_itemrots.bin
│   │   │   ├── living_music_tracks.bin
│   │   │   ├── living_pickup_meshes.bin
│   │   │   ├── living_secret_fd.bin
│   │   │   ├── living_sfx.bin
│   │   │   ├── living_textures.bin
│   │   │   ├── loose_boards_sfx.bin
│   │   │   ├── misc_sprites.bin
│   │   │   ├── opera_crystals.bin
│   │   │   ├── opera_fd.bin
│   │   │   ├── opera_itemrots.bin
│   │   │   ├── opera_music_tracks.bin
│   │   │   ├── opera_sfx.bin
│   │   │   ├── opera_textures.bin
│   │   │   ├── palace_crystals.bin
│   │   │   ├── palace_fd.bin
│   │   │   ├── palace_itemrots.bin
│   │   │   ├── palace_music_tracks.bin
│   │   │   ├── palace_secret_fd.bin
│   │   │   ├── palace_textures.bin
│   │   │   ├── pda_model.bin
│   │   │   ├── photo.bin
│   │   │   ├── pickup_aid.bin
│   │   │   ├── portcullis_sfx.bin
│   │   │   ├── rig_crystals.bin
│   │   │   ├── rig_itemrots.bin
│   │   │   ├── rig_music_tracks.bin
│   │   │   ├── rig_pickup_meshes.bin
│   │   │   ├── rig_textures.bin
│   │   │   ├── scuba_sfx.bin
│   │   │   ├── seaweed_collision.bin
│   │   │   ├── secret_models_gm.bin
│   │   │   ├── secret_models_og.bin
│   │   │   ├── shark_sfx.bin
│   │   │   ├── tibet_crystals.bin
│   │   │   ├── tibet_fd.bin
│   │   │   ├── tibet_itemrots.bin
│   │   │   ├── tibet_music_tracks.bin
│   │   │   ├── tibet_textures.bin
│   │   │   ├── title_textures.bin
│   │   │   ├── vegas_crystals.bin
│   │   │   ├── vegas_fd.bin
│   │   │   ├── vegas_itemrots.bin
│   │   │   ├── vegas_music_tracks.bin
│   │   │   ├── vegas_textures.bin
│   │   │   ├── venice_crystals.bin
│   │   │   ├── venice_fd.bin
│   │   │   ├── venice_itemrots.bin
│   │   │   ├── venice_music_tracks.bin
│   │   │   ├── venice_textures.bin
│   │   │   ├── wall_cameras.bin
│   │   │   ├── wall_crystals.bin
│   │   │   ├── wall_itemrots.bin
│   │   │   ├── wall_music_tracks.bin
│   │   │   ├── wall_switch_sfx.bin
│   │   │   ├── wall_textures.bin
│   │   │   ├── winston_model.bin
│   │   │   ├── wreck_cameras.bin
│   │   │   ├── wreck_crystals.bin
│   │   │   ├── wreck_fd.bin
│   │   │   ├── wreck_goon_sfx.bin
│   │   │   ├── wreck_itemrots.bin
│   │   │   ├── wreck_music_tracks.bin
│   │   │   ├── wreck_pickup_meshes.bin
│   │   │   ├── wreck_plants.bin
│   │   │   ├── wreck_secret_fd.bin
│   │   │   ├── wreck_textures.bin
│   │   │   ├── xian_crystals.bin
│   │   │   ├── xian_fd.bin
│   │   │   ├── xian_itemrots.bin
│   │   │   ├── xian_music_tracks.bin
│   │   │   ├── xian_pickup_meshes.bin
│   │   │   ├── xian_sfx.bin
│   │   │   └── xian_textures.bin
│   │   ├── levels
│   │   │   ├── assault.tr2
│   │   │   ├── boat.tr2
│   │   │   ├── catacomb.tr2
│   │   │   ├── cut1.tr2
│   │   │   ├── cut2.tr2
│   │   │   ├── cut3.tr2
│   │   │   ├── cut4.tr2
│   │   │   ├── deck.tr2
│   │   │   ├── emprtomb.tr2
│   │   │   ├── floating.tr2
│   │   │   ├── house.tr2
│   │   │   ├── icecave.tr2
│   │   │   ├── keel.tr2
│   │   │   ├── living.tr2
│   │   │   ├── monastry.tr2
│   │   │   ├── opera.tr2
│   │   │   ├── platform.tr2
│   │   │   ├── rig.tr2
│   │   │   ├── skidoo.tr2
│   │   │   ├── title.tr2
│   │   │   ├── unwater.tr2
│   │   │   ├── venice.tr2
│   │   │   ├── wall.tr2
│   │   │   └── xian.tr2
│   │   ├── music
│   │   │   ├── 2.mp3
│   │   │   ├── 3.mp3
│   │   │   ├── 4.mp3
│   │   │   ├── 5.mp3
│   │   │   ├── 6.mp3
│   │   │   ├── 7.mp3
│   │   │   ├── 8.mp3
│   │   │   ├── 9.mp3
│   │   │   ├── 10.mp3
│   │   │   ├── 11.mp3
│   │   │   ├── 12.mp3
│   │   │   ├── 13.mp3
│   │   │   ├── 14.mp3
│   │   │   ├── 15.mp3
│   │   │   ├── 16.mp3
│   │   │   ├── 17.mp3
│   │   │   ├── 18.mp3
│   │   │   ├── 19.mp3
│   │   │   ├── 20.mp3
│   │   │   ├── 21.mp3
│   │   │   ├── 22.mp3
│   │   │   ├── 23.mp3
│   │   │   ├── 24.mp3
│   │   │   ├── 25.mp3
│   │   │   ├── 26.mp3
│   │   │   ├── 27.mp3
│   │   │   ├── 28.mp3
│   │   │   ├── 29.mp3
│   │   │   ├── 30.mp3
│   │   │   ├── 31.mp3
│   │   │   ├── 32.mp3
│   │   │   ├── 33.mp3
│   │   │   ├── 34.mp3
│   │   │   ├── 35.mp3
│   │   │   ├── 36.mp3
│   │   │   ├── 37.mp3
│   │   │   ├── 38.mp3
│   │   │   ├── 39.mp3
│   │   │   ├── 40.mp3
│   │   │   ├── 41.mp3
│   │   │   ├── 42.mp3
│   │   │   ├── 43.mp3
│   │   │   ├── 44.mp3
│   │   │   ├── 45.mp3
│   │   │   ├── 46.mp3
│   │   │   ├── 47.mp3
│   │   │   ├── 48.mp3
│   │   │   ├── 49.mp3
│   │   │   ├── 50.mp3
│   │   │   ├── 51.mp3
│   │   │   ├── 52.mp3
│   │   │   ├── 53.mp3
│   │   │   ├── 54.mp3
│   │   │   ├── 55.mp3
│   │   │   ├── 56.mp3
│   │   │   ├── 57.mp3
│   │   │   ├── 58.mp3
│   │   │   ├── 59.mp3
│   │   │   ├── 60.mp3
│   │   │   └── 61.mp3
│   │   ├── scripts
│   │   │   ├── _game.lua
│   │   │   ├── assault.lua
│   │   │   ├── cut3.lua
│   │   │   ├── floating.lua
│   │   │   ├── house.lua
│   │   │   └── monastry.lua
│   │   ├── catalog_item_actions.csv
│   │   ├── catalog_lara_anims.csv
│   │   ├── catalog_lara_states.csv
│   │   ├── catalog_music.csv
│   │   ├── catalog_objects.csv
│   │   ├── catalog_samples.csv
│   │   ├── gameflow.json5
│   │   ├── inv_ring.json5
│   │   ├── main.sfx
│   │   ├── strings-de.json5
│   │   ├── strings-en-gb.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   ├── strings.json5
│   │   └── weapons.json5
│   ├── tr2-gm
│   │   ├── images
│   │   │   ├── 3x2
│   │   │   │   ├── credit00_gm.webp
│   │   │   │   ├── credit07_gm.webp
│   │   │   │   ├── gm_level1.webp
│   │   │   │   ├── gm_level2.webp
│   │   │   │   ├── gm_level3.webp
│   │   │   │   ├── gm_level4.webp
│   │   │   │   ├── gm_level5.webp
│   │   │   │   ├── legal_eu_gm.webp
│   │   │   │   ├── legal_us_gm.webp
│   │   │   │   ├── title_eu_gm.webp
│   │   │   │   └── title_us_gm.webp
│   │   │   ├── 4x3
│   │   │   │   ├── credit00_gm.webp
│   │   │   │   ├── credit07_gm.webp
│   │   │   │   ├── gm_level1.webp
│   │   │   │   ├── gm_level2.webp
│   │   │   │   ├── gm_level3.webp
│   │   │   │   ├── gm_level4.webp
│   │   │   │   ├── gm_level5.webp
│   │   │   │   ├── legal_eu_gm.webp
│   │   │   │   ├── legal_us_gm.webp
│   │   │   │   ├── title_eu_gm.webp
│   │   │   │   └── title_us_gm.webp
│   │   │   ├── og
│   │   │   │   ├── credit00_gm.webp
│   │   │   │   ├── credit07_gm.webp
│   │   │   │   ├── title_eu_gm.webp
│   │   │   │   └── title_us_gm.webp
│   │   │   ├── credit00_gm.webp
│   │   │   ├── credit07_gm.webp
│   │   │   ├── gm_level1.webp
│   │   │   ├── gm_level2.webp
│   │   │   ├── gm_level3.webp
│   │   │   ├── gm_level4.webp
│   │   │   ├── gm_level5.webp
│   │   │   ├── legal_eu_gm.webp
│   │   │   ├── legal_us_gm.webp
│   │   │   ├── title_eu_gm.webp
│   │   │   └── title_us_gm.webp
│   │   ├── levels
│   │   │   ├── level1.tr2
│   │   │   ├── level2.tr2
│   │   │   ├── level3.tr2
│   │   │   ├── level4.tr2
│   │   │   ├── level5.tr2
│   │   │   └── title.tr2
│   │   ├── scripts
│   │   │   ├── _game.lua
│   │   │   ├── level1.lua
│   │   │   ├── level3.lua
│   │   │   └── level4.lua
│   │   ├── gameflow.json5
│   │   ├── main.sfx
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
│   ├── tr3
│   │   ├── audio
│   │   │   └── cdaudio.wad
│   │   ├── cuts
│   │   │   ├── cut1.tr2
│   │   │   ├── cut2.tr2
│   │   │   ├── cut3.tr2
│   │   │   ├── cut4.tr2
│   │   │   ├── cut5.tr2
│   │   │   ├── cut6.tr2
│   │   │   ├── cut7.tr2
│   │   │   ├── cut8.tr2
│   │   │   ├── cut9.tr2
│   │   │   ├── cut11.tr2
│   │   │   └── cut12.tr2
│   │   ├── fmv
│   │   │   ├── crsh_eng.rpl
│   │   │   ├── endgame.rpl
│   │   │   ├── intr_eng.rpl
│   │   │   ├── logo.rpl
│   │   │   └── sail_eng.rpl
│   │   ├── images
│   │   │   ├── 3x2
│   │   │   │   ├── antarc.webp
│   │   │   │   ├── credit01.webp
│   │   │   │   ├── credit02.webp
│   │   │   │   ├── credit03.webp
│   │   │   │   ├── credit04.webp
│   │   │   │   ├── credit05.webp
│   │   │   │   ├── credit06.webp
│   │   │   │   ├── credit07.webp
│   │   │   │   ├── credit08.webp
│   │   │   │   ├── credit09.webp
│   │   │   │   ├── house.webp
│   │   │   │   ├── india.webp
│   │   │   │   ├── legal_eu.webp
│   │   │   │   ├── legal_us.webp
│   │   │   │   ├── london.webp
│   │   │   │   ├── nevada.webp
│   │   │   │   ├── southpac.webp
│   │   │   │   ├── theend2.webp
│   │   │   │   ├── theend.webp
│   │   │   │   ├── title_eu.webp
│   │   │   │   └── title_us.webp
│   │   │   ├── 4x3
│   │   │   │   ├── antarc.webp
│   │   │   │   ├── credit01.webp
│   │   │   │   ├── credit02.webp
│   │   │   │   ├── credit03.webp
│   │   │   │   ├── credit04.webp
│   │   │   │   ├── credit05.webp
│   │   │   │   ├── credit06.webp
│   │   │   │   ├── credit07.webp
│   │   │   │   ├── credit08.webp
│   │   │   │   ├── credit09.webp
│   │   │   │   ├── house.webp
│   │   │   │   ├── india.webp
│   │   │   │   ├── legal_eu.webp
│   │   │   │   ├── legal_us.webp
│   │   │   │   ├── london.webp
│   │   │   │   ├── nevada.webp
│   │   │   │   ├── southpac.webp
│   │   │   │   ├── theend2.webp
│   │   │   │   ├── theend.webp
│   │   │   │   ├── title_eu.webp
│   │   │   │   └── title_us.webp
│   │   │   ├── og
│   │   │   │   ├── antarc.webp
│   │   │   │   ├── credit01.webp
│   │   │   │   ├── credit02.webp
│   │   │   │   ├── credit03.webp
│   │   │   │   ├── credit04.webp
│   │   │   │   ├── credit05.webp
│   │   │   │   ├── credit06.webp
│   │   │   │   ├── credit07.webp
│   │   │   │   ├── credit08.webp
│   │   │   │   ├── credit09.webp
│   │   │   │   ├── house.webp
│   │   │   │   ├── india.webp
│   │   │   │   ├── legal_eu.webp
│   │   │   │   ├── legal_us.webp
│   │   │   │   ├── london.webp
│   │   │   │   ├── nevada.webp
│   │   │   │   ├── southpac.webp
│   │   │   │   ├── theend2.webp
│   │   │   │   ├── theend.webp
│   │   │   │   ├── title_eu.webp
│   │   │   │   └── title_us.webp
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
│   │   │   ├── theend.webp
│   │   │   ├── title_eu.webp
│   │   │   └── title_us.webp
│   │   ├── injections
│   │   │   ├── alarm_sfx.bin
│   │   │   ├── aldwych_animating_bounds.bin
│   │   │   ├── aldwych_fd.bin
│   │   │   ├── aldwych_pickup_meshes.bin
│   │   │   ├── aldwych_textures.bin
│   │   │   ├── antarc_airlock.bin
│   │   │   ├── antarc_door134_frames.bin
│   │   │   ├── antarc_fd.bin
│   │   │   ├── antarc_sky.bin
│   │   │   ├── antarc_textures.bin
│   │   │   ├── antarc_wheel_frames.bin
│   │   │   ├── area51_animating_bounds.bin
│   │   │   ├── area51_patrol.bin
│   │   │   ├── area51_sky.bin
│   │   │   ├── area51_textures.bin
│   │   │   ├── bat_sprites.bin
│   │   │   ├── binoculars.bin
│   │   │   ├── cavern_door131_frames.bin
│   │   │   ├── cavern_pickup_meshes.bin
│   │   │   ├── cavern_sky.bin
│   │   │   ├── cavern_textures.bin
│   │   │   ├── caves_textures.bin
│   │   │   ├── city_textures.bin
│   │   │   ├── cliff_animating_bounds.bin
│   │   │   ├── cliff_crystals.bin
│   │   │   ├── cliff_door132_frames.bin
│   │   │   ├── cliff_textures.bin
│   │   │   ├── coastal_airlock.bin
│   │   │   ├── coastal_animating_bounds.bin
│   │   │   ├── coastal_fd.bin
│   │   │   ├── coastal_sky.bin
│   │   │   ├── coastal_spike_sfx.bin
│   │   │   ├── coastal_textures.bin
│   │   │   ├── coastal_wheel_frames.bin
│   │   │   ├── common_pickup_meshes.bin
│   │   │   ├── compound_animating_bounds.bin
│   │   │   ├── compound_cine.bin
│   │   │   ├── compound_patrol.bin
│   │   │   ├── compound_textures.bin
│   │   │   ├── crash_animating_bounds.bin
│   │   │   ├── crash_pickup_meshes.bin
│   │   │   ├── crash_sky.bin
│   │   │   ├── crash_textures.bin
│   │   │   ├── crystal.bin
│   │   │   ├── cut1_setup.bin
│   │   │   ├── cut2_setup.bin
│   │   │   ├── cut3_setup.bin
│   │   │   ├── cut3_shell.bin
│   │   │   ├── cut4_setup.bin
│   │   │   ├── cut5_setup.bin
│   │   │   ├── cut5_textures.bin
│   │   │   ├── cut6_setup.bin
│   │   │   ├── cut7_setup.bin
│   │   │   ├── cut8_setup.bin
│   │   │   ├── cut9_setup.bin
│   │   │   ├── cut11_setup.bin
│   │   │   ├── cut12_setup.bin
│   │   │   ├── drill_collision.bin
│   │   │   ├── fish_sprites.bin
│   │   │   ├── flamethrower_sfx.bin
│   │   │   ├── font.bin
│   │   │   ├── ganges_animating_bounds.bin
│   │   │   ├── ganges_door131_frames.bin
│   │   │   ├── ganges_textures.bin
│   │   │   ├── globe_model.bin
│   │   │   ├── gym_fd.bin
│   │   │   ├── gym_sky.bin
│   │   │   ├── gym_textures.bin
│   │   │   ├── hiss_sfx.bin
│   │   │   ├── india_sky.bin
│   │   │   ├── inv_background.bin
│   │   │   ├── jungle_fd.bin
│   │   │   ├── jungle_textures.bin
│   │   │   ├── lara_animations.bin
│   │   │   ├── lara_extra.bin
│   │   │   ├── lara_guns.bin
│   │   │   ├── lara_gym_guns.bin
│   │   │   ├── lara_outfits.bin
│   │   │   ├── lara_rifle_sfx.bin
│   │   │   ├── london_sky.bin
│   │   │   ├── luds_animating_bounds.bin
│   │   │   ├── luds_diver_animation.bin
│   │   │   ├── luds_fd.bin
│   │   │   ├── luds_textures.bin
│   │   │   ├── madubu_spike_sfx.bin
│   │   │   ├── madubu_textures.bin
│   │   │   ├── menu_artefacts.bin
│   │   │   ├── menu_artefacts_london.bin
│   │   │   ├── mines_textures.bin
│   │   │   ├── misc_sprites.bin
│   │   │   ├── nevada_animating_bounds.bin
│   │   │   ├── nevada_door132_frames.bin
│   │   │   ├── nevada_sky.bin
│   │   │   ├── nevada_textures.bin
│   │   │   ├── pda_model.bin
│   │   │   ├── pickup_aid.bin
│   │   │   ├── pipeman_sfx.bin
│   │   │   ├── puna_pickup_meshes.bin
│   │   │   ├── puna_textures.bin
│   │   │   ├── rapids_sky.bin
│   │   │   ├── reunion_crystals.bin
│   │   │   ├── reunion_flames.bin
│   │   │   ├── reunion_textures.bin
│   │   │   ├── scotland_crystals.bin
│   │   │   ├── scotland_sky.bin
│   │   │   ├── scotland_textures.bin
│   │   │   ├── sparks_gfx.bin
│   │   │   ├── stpaul_animating_bounds.bin
│   │   │   ├── stpaul_textures.bin
│   │   │   ├── temple_itemrots.bin
│   │   │   ├── temple_textures.bin
│   │   │   ├── thames_animating_bounds.bin
│   │   │   ├── thames_fd.bin
│   │   │   ├── thames_textures.bin
│   │   │   ├── tinnos_cameras.bin
│   │   │   ├── tinnos_fd.bin
│   │   │   ├── tinnos_flames.bin
│   │   │   ├── tinnos_textures.bin
│   │   │   ├── title_textures.bin
│   │   │   ├── undersea_animating_bounds.bin
│   │   │   ├── undersea_animating_ext.bin
│   │   │   ├── undersea_crystals.bin
│   │   │   ├── undersea_fd.bin
│   │   │   ├── undersea_objects.bin
│   │   │   ├── undersea_pickup_meshes.bin
│   │   │   ├── undersea_textures.bin
│   │   │   ├── undersea_train.bin
│   │   │   ├── undersea_wheel_frames.bin
│   │   │   ├── water_sfx.bin
│   │   │   ├── willsden_crystals.bin
│   │   │   ├── willsden_heli.bin
│   │   │   ├── willsden_textures.bin
│   │   │   ├── winston_model.bin
│   │   │   ├── zoo_crystals.bin
│   │   │   ├── zoo_textures.bin
│   │   │   └── zoo_train.bin
│   │   ├── levels
│   │   │   ├── antarc.tr2
│   │   │   ├── area51.tr2
│   │   │   ├── chamber.tr2
│   │   │   ├── city.tr2
│   │   │   ├── compound.tr2
│   │   │   ├── crash.tr2
│   │   │   ├── house.tr2
│   │   │   ├── jungle.tr2
│   │   │   ├── mines.tr2
│   │   │   ├── nevada.tr2
│   │   │   ├── office.tr2
│   │   │   ├── quadchas.tr2
│   │   │   ├── rapids.tr2
│   │   │   ├── roofs.tr2
│   │   │   ├── sewer.tr2
│   │   │   ├── shore.tr2
│   │   │   ├── stpaul.tr2
│   │   │   ├── temple.tr2
│   │   │   ├── title.tr2
│   │   │   ├── tonyboss.tr2
│   │   │   ├── tower.tr2
│   │   │   └── triboss.tr2
│   │   ├── scripts
│   │   │   ├── _game.lua
│   │   │   ├── antarc.lua
│   │   │   ├── area51.lua
│   │   │   ├── chamber.lua
│   │   │   ├── city.lua
│   │   │   ├── compound.lua
│   │   │   ├── crash.lua
│   │   │   ├── cut8.lua
│   │   │   ├── house.lua
│   │   │   ├── jungle.lua
│   │   │   ├── mines.lua
│   │   │   ├── nevada.lua
│   │   │   ├── office.lua
│   │   │   ├── quadchas.lua
│   │   │   ├── rapids.lua
│   │   │   ├── roofs.lua
│   │   │   ├── sewer.lua
│   │   │   ├── shore.lua
│   │   │   ├── temple.lua
│   │   │   ├── tonyboss.lua
│   │   │   ├── tower.lua
│   │   │   └── triboss.lua
│   │   ├── catalog_item_actions.csv
│   │   ├── catalog_lara_anims.csv
│   │   ├── catalog_lara_states.csv
│   │   ├── catalog_music.csv
│   │   ├── catalog_objects.csv
│   │   ├── catalog_samples.csv
│   │   ├── gameflow.json5
│   │   ├── inv_ring.json5
│   │   ├── main.sfx
│   │   ├── strings-de.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   ├── strings.json5
│   │   ├── tombpc.dat
│   │   └── weapons.json5
│   ├── tr3-la
│   │   ├── images
│   │   │   ├── 3x2
│   │   │   │   ├── chunnel.webp
│   │   │   │   ├── credit01_eu_la.webp
│   │   │   │   ├── credit01_us_la.webp
│   │   │   │   ├── credit02_la.webp
│   │   │   │   ├── credit03_la.webp
│   │   │   │   ├── credit04_la.webp
│   │   │   │   ├── credit05_la.webp
│   │   │   │   ├── credit06_la.webp
│   │   │   │   ├── credit07_la.webp
│   │   │   │   ├── credit08_la.webp
│   │   │   │   ├── credit09_la.webp
│   │   │   │   ├── highland.webp
│   │   │   │   ├── legal_eu_la.webp
│   │   │   │   ├── legal_us_la.webp
│   │   │   │   ├── slinc.webp
│   │   │   │   ├── theend2_la.webp
│   │   │   │   ├── theend_la.webp
│   │   │   │   ├── title_eu_la.webp
│   │   │   │   ├── title_us_la.webp
│   │   │   │   ├── undersea.webp
│   │   │   │   ├── willard.webp
│   │   │   │   └── zoo.webp
│   │   │   ├── 4x3
│   │   │   │   ├── chunnel.webp
│   │   │   │   ├── credit01_eu_la.webp
│   │   │   │   ├── credit01_us_la.webp
│   │   │   │   ├── credit02_la.webp
│   │   │   │   ├── credit03_la.webp
│   │   │   │   ├── credit04_la.webp
│   │   │   │   ├── credit05_la.webp
│   │   │   │   ├── credit06_la.webp
│   │   │   │   ├── credit07_la.webp
│   │   │   │   ├── credit08_la.webp
│   │   │   │   ├── credit09_la.webp
│   │   │   │   ├── highland.webp
│   │   │   │   ├── legal_eu_la.webp
│   │   │   │   ├── legal_us_la.webp
│   │   │   │   ├── slinc.webp
│   │   │   │   ├── title_eu_la.webp
│   │   │   │   ├── title_us_la.webp
│   │   │   │   ├── undersea.webp
│   │   │   │   ├── willard.webp
│   │   │   │   └── zoo.webp
│   │   │   ├── chunnel.webp
│   │   │   ├── credit01_eu_la.webp
│   │   │   ├── credit01_us_la.webp
│   │   │   ├── credit02_la.webp
│   │   │   ├── credit03_la.webp
│   │   │   ├── credit04_la.webp
│   │   │   ├── credit05_la.webp
│   │   │   ├── credit06_la.webp
│   │   │   ├── credit07_la.webp
│   │   │   ├── credit08_la.webp
│   │   │   ├── credit09_la.webp
│   │   │   ├── highland.webp
│   │   │   ├── legal_eu_la.webp
│   │   │   ├── legal_us_la.webp
│   │   │   ├── slinc.webp
│   │   │   ├── theend2_la.webp
│   │   │   ├── theend_la.webp
│   │   │   ├── title_eu_la.webp
│   │   │   ├── title_us_la.webp
│   │   │   ├── undersea.webp
│   │   │   ├── willard.webp
│   │   │   └── zoo.webp
│   │   ├── levels
│   │   │   ├── chunnel.tr2
│   │   │   ├── scotland.tr2
│   │   │   ├── slinc.tr2
│   │   │   ├── title.tr2
│   │   │   ├── undersea.tr2
│   │   │   ├── willsden.tr2
│   │   │   └── zoo.tr2
│   │   ├── scripts
│   │   │   ├── _game.lua
│   │   │   ├── chunnel.lua
│   │   │   ├── slinc.lua
│   │   │   ├── undersea.lua
│   │   │   └── zoo.lua
│   │   ├── gameflow.json5
│   │   ├── main.sfx
│   │   ├── strings-de.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   └── strings.json5
│   ├── tr3-level
│   │   ├── gameflow.json5
│   │   ├── strings-fr.json5
│   │   ├── strings-gd.json5
│   │   ├── strings-it.json5
│   │   ├── strings-pl.json5
│   │   └── strings.json5
│   ├── tr4
│   │   ├── injections
│   │   │   ├── font.bin
│   │   │   ├── karnak_fd.bin
│   │   │   ├── lara_animations.bin
│   │   │   ├── lara_outfits.bin
│   │   │   └── sparks_gfx.bin
│   │   ├── modules
│   │   │   ├── cutscenes.lua
│   │   │   └── race_timer.lua
│   │   ├── scripts
│   │   │   ├── _game.lua
│   │   │   ├── alexhub2.lua
│   │   │   ├── alexhub.lua
│   │   │   ├── ang_race.lua
│   │   │   ├── angkor1.lua
│   │   │   ├── bikebit.lua
│   │   │   ├── cortyard.lua
│   │   │   ├── csplit1.lua
│   │   │   ├── csplit2.lua
│   │   │   ├── highstrt.lua
│   │   │   ├── joby1a.lua
│   │   │   ├── joby2.lua
│   │   │   ├── joby3b.lua
│   │   │   ├── joby4a.lua
│   │   │   ├── joby4b.lua
│   │   │   ├── joby4c.lua
│   │   │   ├── joby5a.lua
│   │   │   ├── joby5b.lua
│   │   │   ├── karnak1.lua
│   │   │   ├── lake.lua
│   │   │   ├── libend.lua
│   │   │   ├── library.lua
│   │   │   ├── lowstrt.lua
│   │   │   ├── nutrench.lua
│   │   │   ├── palaces2.lua
│   │   │   ├── palaces.lua
│   │   │   ├── semer2.lua
│   │   │   ├── semer.lua
│   │   │   ├── settomb1.lua
│   │   │   ├── settomb2.lua
│   │   │   ├── title.lua
│   │   │   └── train.lua
│   │   ├── catalog_item_actions.csv
│   │   ├── catalog_lara_anims.csv
│   │   ├── catalog_lara_states.csv
│   │   ├── catalog_music.csv
│   │   ├── catalog_music_files.csv
│   │   ├── catalog_objects.csv
│   │   ├── catalog_samples.csv
│   │   ├── gameflow.json5
│   │   ├── inv_ring.json5
│   │   ├── strings-de.json5
│   │   ├── strings-it.json5
│   │   ├── strings.json5
│   │   └── weapons.json5
│   └── tr4-level
│       ├── gameflow.json5
│       ├── strings-it.json5
│       └── strings.json5
├── modules
│   ├── save_crystal.lua
│   └── water_color.lua
└── TRX.exe</code></pre>
</details>

*\* Will not be present until the game has been launched.*

## Playing the game

- To play the last selected game or expansion, run `TRX.exe`.
- To launch a specific base game from a shortcut, use one of the following commands:
    - `TRX.exe --mod tr1`
    - `TRX.exe --mod tr2`
    - `TRX.exe --mod tr3`
- To launch a specific expansion pack from a shortcut, use one of the following commands:
    - `TRX.exe --mod tr1-ub`
    - `TRX.exe --mod tr2-gm`
    - `TRX.exe --mod tr3-la`

## Migrating from previous TRX installations

If TRX shows an error like `Mixed mod layout detected: found legacy mod data`,
your install contains a mix of the old layout and the new one. Older combined
builds stored mod definitions under `cfg/<game-id>/`, with the matching game
files in top-level folders such as `data/`, `fmv/`, `music/`, `audio/`, and
`cuts/`. Current combined builds keep each game together under
`games/<game-id>/`. That means each game now has its own directory, such as:

- `games/tr1/`
- `games/tr2/`
- `games/tr3/`

Inside each of those folders you will find the game's own files, for example
`levels/`, `fmv/`, `music/`, `audio/`, `cuts/`, and the mod's `gameflow.json5`. To fix the error:

1. Keep the shared `cfg/` directory from the current combined TRX build.
2. Remove any legacy per-game folders from `cfg/`, such as:
   - `cfg/tr1/`,
   - `cfg/tr1-demo-pc`,
   - `cfg/tr1-level/`,
   - `cfg/tr1-ub/`,
   - `cfg/tr2/`,
   - `cfg/tr2-gm/`,
   - `cfg/tr2-level/`,
   - `cfg/tr3/`,
   - `cfg/tr3-la/`,
   - `cfg/tr3-level/`.
3. Move or re-copy your original game files into the matching
   `games/<game-id>/` folders listed above.
4. Do not keep the same game in both places at once. For example, if you use
   `games/tr1/`, do not also keep `cfg/tr1/` and `data/`.

If you are updating from an older combined install, the simplest fix is usually
to start from a fresh extract of the latest TRX zip and then copy your original
game files into `games/<game-id>/` again.



# macOS

## Installing

1. Download the latest combined TRX installer image, for example,
   `TRX-1.6-Mac.dmg`. This filename will be used in examples forward, although
   the newest version may be different.

2. Mount the `.dmg` image.  
   Desktop: double-click `TRX-1.6-Mac.dmg` in Finder.  
   Terminal: `hdiutil attach TRX-1.6-Mac.dmg`.

3. Copy `TRX.app` to the Application folder.  
   Desktop: drag `TRX.app` to the Applications folder.  
   Terminal: `cp -R /Volumes/TRX/TRX.app /Applications/`

3. Run `TRX` from the Applications folder.  
   Desktop: double click `TRX` in the Application folder.  
   Terminal: `open /Applications/TRX.app`

   It is important to run the app first to allow macOS to verify the app bundle's signature.  
   The game should launch and allow playing Tomb Raider I PC Demo and Tomb
   Raider II: The Golden Mask expansion pack alone. This is expected at this
   point, as the game is still missing the original game files, shipping only
   with content that can be freely distributed.  

4. Copy original game files.  
   Desktop: find `TRX` in your Applications folder, right-click it and click "Show Package Contents". Then, copy your original game files into `Contents/Resources/games/<game-id>/`.  
   Terminal: `cp -R /path/to/original/game/files/...` `/Applications/TRX.app/Contents/Resources/games/<game-id>/`.

   Please refer to ["Manually copying original game files"](#manually-copying-original-game-files)
   section in this document to see where to put which file.

5. Unmount the installer  
   Terminal: `hdiutil detach /Volumes/TRX`

In case you see a popup "TRX is damaged" when you run the game, please run
`xattr -cr /Applications/TRX.app` in Terminal.

## Verifying the installation

Inside `TRX.app`, `Contents/Resources` should use the same combined layout as the Windows and Linux zip:

<details data-id="file-tree-mac">
<pre><code>.
└── Contents
    ├── Resources
    │   ├── cfg
    │   │   ├── presets
    │   │   │   ├── tr1-moveset.json5
    │   │   │   ├── tr1-pc.json5
    │   │   │   ├── tr1-ps1.json5
    │   │   │   ├── tr2-moveset.json5
    │   │   │   ├── tr2-pc.json5
    │   │   │   ├── tr2-ps1.json5
    │   │   │   ├── tr3-moveset.json5
    │   │   │   ├── tr3-pc.json5
    │   │   │   └── tr3-ps1.json5
    │   │   ├── shaders
    │   │   │   ├── 2d.glsl
    │   │   │   ├── billboard.glsl
    │   │   │   ├── common.glsl
    │   │   │   ├── fbo.glsl
    │   │   │   ├── lights.glsl
    │   │   │   ├── lights_common.glsl
    │   │   │   ├── lights_tr3.glsl
    │   │   │   ├── lights_tr4.glsl
    │   │   │   ├── lights_tr12.glsl
    │   │   │   ├── meshes.glsl
    │   │   │   ├── meshes_tr3.glsl
    │   │   │   ├── meshes_tr4.glsl
    │   │   │   ├── meshes_tr12.glsl
    │   │   │   └── ui.glsl
    │   │   ├── base_strings-de.json5
    │   │   ├── base_strings-en-gb.json5
    │   │   ├── base_strings-fr.json5
    │   │   ├── base_strings-gd.json5
    │   │   ├── base_strings-it.json5
    │   │   ├── base_strings-pl.json5
    │   │   ├── base_strings-ru.json5
    │   │   ├── base_strings.json5
    │   │   ├── outfits.json5
    │   │   ├── poses.json5
    │   │   ├── shell.json5*
    │   │   ├── TR1X.json5*
    │   │   ├── TR2X.json5*
    │   │   ├── TR3X.json5*
    │   │   └── ui.json5
    │   ├── games
    │   │   ├── tr1
    │   │   │   ├── fmv
    │   │   │   │   ├── cafe.rpl
    │   │   │   │   ├── canyon.rpl
    │   │   │   │   ├── core.avi
    │   │   │   │   ├── end.rpl
    │   │   │   │   ├── escape.rpl
    │   │   │   │   ├── lift.rpl
    │   │   │   │   ├── mansion.rpl
    │   │   │   │   ├── prison.rpl
    │   │   │   │   ├── pyramid.rpl
    │   │   │   │   ├── snow.rpl
    │   │   │   │   └── vision.rpl
    │   │   │   ├── images
    │   │   │   │   ├── atlantis.webp
    │   │   │   │   ├── credits_1.webp
    │   │   │   │   ├── credits_2.webp
    │   │   │   │   ├── credits_3.webp
    │   │   │   │   ├── credits_3_alt.webp
    │   │   │   │   ├── credits_ps1.webp
    │   │   │   │   ├── egypt.webp
    │   │   │   │   ├── eidos.webp
    │   │   │   │   ├── end.webp
    │   │   │   │   ├── greece.webp
    │   │   │   │   ├── greece_saturn.webp
    │   │   │   │   ├── gym.webp
    │   │   │   │   ├── install.webp
    │   │   │   │   ├── peru.webp
    │   │   │   │   ├── title.webp
    │   │   │   │   └── title_og_alt.webp
    │   │   │   ├── injections
    │   │   │   │   ├── atlantis_door_sfx.bin
    │   │   │   │   ├── atlantis_fd.bin
    │   │   │   │   ├── atlantis_itemrots.bin
    │   │   │   │   ├── atlantis_textures.bin
    │   │   │   │   ├── binoculars.bin
    │   │   │   │   ├── bubbles.bin
    │   │   │   │   ├── cat_cameras.bin
    │   │   │   │   ├── cat_crystals.bin
    │   │   │   │   ├── cat_fd.bin
    │   │   │   │   ├── cat_itemrots.bin
    │   │   │   │   ├── cat_meshfixes.bin
    │   │   │   │   ├── cat_textures.bin
    │   │   │   │   ├── caves_fd.bin
    │   │   │   │   ├── caves_itemrots.bin
    │   │   │   │   ├── caves_textures.bin
    │   │   │   │   ├── cistern_fd.bin
    │   │   │   │   ├── cistern_itemrots.bin
    │   │   │   │   ├── cistern_plants.bin
    │   │   │   │   ├── cistern_skybox.bin
    │   │   │   │   ├── cistern_textures.bin
    │   │   │   │   ├── colosseum_fd.bin
    │   │   │   │   ├── colosseum_itemrots.bin
    │   │   │   │   ├── colosseum_skybox.bin
    │   │   │   │   ├── colosseum_textures.bin
    │   │   │   │   ├── crystal.bin
    │   │   │   │   ├── cut1_setup.bin
    │   │   │   │   ├── cut1_textures.bin
    │   │   │   │   ├── cut2_setup.bin
    │   │   │   │   ├── cut3_setup.bin
    │   │   │   │   ├── cut3_textures.bin
    │   │   │   │   ├── cut4_setup.bin
    │   │   │   │   ├── cut4_textures.bin
    │   │   │   │   ├── door58_frames.bin
    │   │   │   │   ├── door59_frames.bin
    │   │   │   │   ├── door59_sfx.bin
    │   │   │   │   ├── door60_frames.bin
    │   │   │   │   ├── door61_sfx.bin
    │   │   │   │   ├── egypt_cameras.bin
    │   │   │   │   ├── egypt_crystals.bin
    │   │   │   │   ├── egypt_fd.bin
    │   │   │   │   ├── egypt_itemrots.bin
    │   │   │   │   ├── egypt_meshfixes.bin
    │   │   │   │   ├── egypt_textures.bin
    │   │   │   │   ├── explosion.bin
    │   │   │   │   ├── folly_fd.bin
    │   │   │   │   ├── folly_itemrots.bin
    │   │   │   │   ├── folly_pickup_meshes.bin
    │   │   │   │   ├── folly_textures.bin
    │   │   │   │   ├── font.bin
    │   │   │   │   ├── gun_glow.bin
    │   │   │   │   ├── gym_textures.bin
    │   │   │   │   ├── hive_crystals.bin
    │   │   │   │   ├── hive_fd.bin
    │   │   │   │   ├── hive_itemrots.bin
    │   │   │   │   ├── hive_textures.bin
    │   │   │   │   ├── inv_background.bin
    │   │   │   │   ├── khamoon_fd.bin
    │   │   │   │   ├── khamoon_itemrots.bin
    │   │   │   │   ├── khamoon_meshfixes.bin
    │   │   │   │   ├── khamoon_mummy.bin
    │   │   │   │   ├── khamoon_textures.bin
    │   │   │   │   ├── lara_animations.bin
    │   │   │   │   ├── lara_extra.bin
    │   │   │   │   ├── lara_feet_sfx.bin
    │   │   │   │   ├── lara_guns.bin
    │   │   │   │   ├── lara_gym_guns.bin
    │   │   │   │   ├── lara_outfits.bin
    │   │   │   │   ├── midas_itemrots.bin
    │   │   │   │   ├── midas_textures.bin
    │   │   │   │   ├── mines_cameras.bin
    │   │   │   │   ├── mines_door_sfx.bin
    │   │   │   │   ├── mines_fd.bin
    │   │   │   │   ├── mines_itemrots.bin
    │   │   │   │   ├── mines_meshfixes.bin
    │   │   │   │   ├── mines_pushblocks.bin
    │   │   │   │   ├── mines_textures.bin
    │   │   │   │   ├── misc_sprites.bin
    │   │   │   │   ├── obelisk_fd.bin
    │   │   │   │   ├── obelisk_itemrots.bin
    │   │   │   │   ├── obelisk_meshfixes.bin
    │   │   │   │   ├── obelisk_skybox.bin
    │   │   │   │   ├── obelisk_textures.bin
    │   │   │   │   ├── panther_sfx.bin
    │   │   │   │   ├── pda_model.bin
    │   │   │   │   ├── photo.bin
    │   │   │   │   ├── pickup_aid.bin
    │   │   │   │   ├── pyramid_fd.bin
    │   │   │   │   ├── pyramid_itemrots.bin
    │   │   │   │   ├── pyramid_textures.bin
    │   │   │   │   ├── qualopec_door_sfx.bin
    │   │   │   │   ├── qualopec_fd.bin
    │   │   │   │   ├── qualopec_itemrots.bin
    │   │   │   │   ├── qualopec_textures.bin
    │   │   │   │   ├── sanctuary_fd.bin
    │   │   │   │   ├── sanctuary_itemrots.bin
    │   │   │   │   ├── sanctuary_meshfixes.bin
    │   │   │   │   ├── sanctuary_scion.bin
    │   │   │   │   ├── sanctuary_textures.bin
    │   │   │   │   ├── scion_alignment.bin
    │   │   │   │   ├── scion_collision.bin
    │   │   │   │   ├── shimmy_sfx.bin
    │   │   │   │   ├── skate_kid_sfx.bin
    │   │   │   │   ├── sprite_alignment.bin
    │   │   │   │   ├── stronghold_crystals.bin
    │   │   │   │   ├── stronghold_fd.bin
    │   │   │   │   ├── stronghold_itemrots.bin
    │   │   │   │   ├── stronghold_textures.bin
    │   │   │   │   ├── tihocan_fd.bin
    │   │   │   │   ├── tihocan_itemrots.bin
    │   │   │   │   ├── tihocan_skybox.bin
    │   │   │   │   ├── tihocan_textures.bin
    │   │   │   │   ├── title_textures.bin
    │   │   │   │   ├── uw_switch_sfx.bin
    │   │   │   │   ├── uzi_sfx.bin
    │   │   │   │   ├── valley_fd.bin
    │   │   │   │   ├── valley_itemrots.bin
    │   │   │   │   ├── valley_skybox.bin
    │   │   │   │   ├── valley_textures.bin
    │   │   │   │   ├── vilcabamba_door_sfx.bin
    │   │   │   │   ├── vilcabamba_itemrots.bin
    │   │   │   │   ├── vilcabamba_textures.bin
    │   │   │   │   ├── wall_switch_sfx.bin
    │   │   │   │   └── winston_model.bin
    │   │   │   ├── levels
    │   │   │   │   ├── cut1.phd
    │   │   │   │   ├── cut2.phd
    │   │   │   │   ├── cut3.phd
    │   │   │   │   ├── cut4.phd
    │   │   │   │   ├── gym.phd
    │   │   │   │   ├── level1.phd
    │   │   │   │   ├── level2.phd
    │   │   │   │   ├── level3a.phd
    │   │   │   │   ├── level3b.phd
    │   │   │   │   ├── level4.phd
    │   │   │   │   ├── level5.phd
    │   │   │   │   ├── level6.phd
    │   │   │   │   ├── level7a.phd
    │   │   │   │   ├── level7b.phd
    │   │   │   │   ├── level8a.phd
    │   │   │   │   ├── level8b.phd
    │   │   │   │   ├── level8c.phd
    │   │   │   │   ├── level10a.phd
    │   │   │   │   ├── level10b.phd
    │   │   │   │   ├── level10c.phd
    │   │   │   │   └── title.phd
    │   │   │   ├── music
    │   │   │   │   ├── track02.flac
    │   │   │   │   ├── track03.flac
    │   │   │   │   ├── track04.flac
    │   │   │   │   ├── track05.flac
    │   │   │   │   ├── track06.flac
    │   │   │   │   ├── track07.flac
    │   │   │   │   ├── track08.flac
    │   │   │   │   ├── track09.flac
    │   │   │   │   ├── track10.flac
    │   │   │   │   ├── track11.flac
    │   │   │   │   ├── track12.flac
    │   │   │   │   ├── track13.flac
    │   │   │   │   ├── track14.flac
    │   │   │   │   ├── track15.flac
    │   │   │   │   ├── track16.flac
    │   │   │   │   ├── track17.flac
    │   │   │   │   ├── track18.flac
    │   │   │   │   ├── track19.flac
    │   │   │   │   ├── track20.flac
    │   │   │   │   ├── track21.flac
    │   │   │   │   ├── track22.flac
    │   │   │   │   ├── track23.flac
    │   │   │   │   ├── track24.flac
    │   │   │   │   ├── track25.flac
    │   │   │   │   ├── track26.flac
    │   │   │   │   ├── track27.flac
    │   │   │   │   ├── track28.flac
    │   │   │   │   ├── track29.flac
    │   │   │   │   ├── track30.flac
    │   │   │   │   ├── track31.flac
    │   │   │   │   ├── track32.flac
    │   │   │   │   ├── track33.flac
    │   │   │   │   ├── track34.flac
    │   │   │   │   ├── track35.flac
    │   │   │   │   ├── track36.flac
    │   │   │   │   ├── track37.flac
    │   │   │   │   ├── track38.flac
    │   │   │   │   ├── track39.flac
    │   │   │   │   ├── track40.flac
    │   │   │   │   ├── track41.flac
    │   │   │   │   ├── track42.flac
    │   │   │   │   ├── track43.flac
    │   │   │   │   ├── track44.flac
    │   │   │   │   ├── track45.flac
    │   │   │   │   ├── track46.flac
    │   │   │   │   ├── track47.flac
    │   │   │   │   ├── track48.flac
    │   │   │   │   ├── track49.flac
    │   │   │   │   ├── track50.flac
    │   │   │   │   ├── track51.flac
    │   │   │   │   ├── track52.flac
    │   │   │   │   ├── track53.flac
    │   │   │   │   ├── track54.flac
    │   │   │   │   ├── track55.flac
    │   │   │   │   ├── track56.flac
    │   │   │   │   ├── track57.flac
    │   │   │   │   ├── track58.flac
    │   │   │   │   ├── track59.flac
    │   │   │   │   └── track60.flac
    │   │   │   ├── scripts
    │   │   │   │   ├── _game.lua
    │   │   │   │   ├── gym.lua
    │   │   │   │   ├── level3b.lua
    │   │   │   │   ├── level8c.lua
    │   │   │   │   └── level10b.lua
    │   │   │   ├── catalog_item_actions.csv
    │   │   │   ├── catalog_lara_anims.csv
    │   │   │   ├── catalog_lara_states.csv
    │   │   │   ├── catalog_music.csv
    │   │   │   ├── catalog_objects.csv
    │   │   │   ├── catalog_samples.csv
    │   │   │   ├── gameflow.json5
    │   │   │   ├── inv_ring.json5
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-en-gb.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   ├── strings-ru.json5
    │   │   │   ├── strings.json5
    │   │   │   └── weapons.json5
    │   │   ├── tr1-demo-pc
    │   │   │   ├── gameflow.json5
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   ├── strings-ru.json5
    │   │   │   └── strings.json5
    │   │   ├── tr1-level
    │   │   │   ├── gameflow.json5
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   ├── strings-ru.json5
    │   │   │   └── strings.json5
    │   │   ├── tr1-ub
    │   │   │   ├── images
    │   │   │   │   ├── credits_ub.webp
    │   │   │   │   ├── title_ub.webp
    │   │   │   │   ├── ub_loading1.webp
    │   │   │   │   └── ub_loading2.webp
    │   │   │   ├── levels
    │   │   │   │   ├── cat.phd
    │   │   │   │   ├── egypt.phd
    │   │   │   │   ├── end2.phd
    │   │   │   │   └── end.phd
    │   │   │   ├── scripts
    │   │   │   │   └── _game.lua
    │   │   │   ├── gameflow.json5
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   ├── strings-ru.json5
    │   │   │   └── strings.json5
    │   │   ├── tr2
    │   │   │   ├── fmv
    │   │   │   │   ├── ancient.rpl
    │   │   │   │   ├── crash.rpl
    │   │   │   │   ├── end.rpl
    │   │   │   │   ├── jeep.rpl
    │   │   │   │   ├── landing.rpl
    │   │   │   │   ├── logo.rpl
    │   │   │   │   ├── modern.rpl
    │   │   │   │   └── ms.rpl
    │   │   │   ├── images
    │   │   │   │   ├── 3x2
    │   │   │   │   │   ├── china.webp
    │   │   │   │   │   ├── credit01.webp
    │   │   │   │   │   ├── credit02.webp
    │   │   │   │   │   ├── credit03.webp
    │   │   │   │   │   ├── credit04.webp
    │   │   │   │   │   ├── credit05.webp
    │   │   │   │   │   ├── credit06.webp
    │   │   │   │   │   ├── credit07.webp
    │   │   │   │   │   ├── credit08.webp
    │   │   │   │   │   ├── end.webp
    │   │   │   │   │   ├── legal_eu.webp
    │   │   │   │   │   ├── legal_us.webp
    │   │   │   │   │   ├── mansion.webp
    │   │   │   │   │   ├── rig.webp
    │   │   │   │   │   ├── tibet.webp
    │   │   │   │   │   ├── titan.webp
    │   │   │   │   │   ├── title_eu.webp
    │   │   │   │   │   ├── title_us.webp
    │   │   │   │   │   └── venice.webp
    │   │   │   │   ├── 4x3
    │   │   │   │   │   ├── china.webp
    │   │   │   │   │   ├── credit01.webp
    │   │   │   │   │   ├── credit02.webp
    │   │   │   │   │   ├── credit03.webp
    │   │   │   │   │   ├── credit04.webp
    │   │   │   │   │   ├── credit05.webp
    │   │   │   │   │   ├── credit06.webp
    │   │   │   │   │   ├── credit07.webp
    │   │   │   │   │   ├── credit08.webp
    │   │   │   │   │   ├── end.webp
    │   │   │   │   │   ├── legal_eu.webp
    │   │   │   │   │   ├── legal_us.webp
    │   │   │   │   │   ├── mansion.webp
    │   │   │   │   │   ├── rig.webp
    │   │   │   │   │   ├── tibet.webp
    │   │   │   │   │   ├── titan.webp
    │   │   │   │   │   ├── title_eu.webp
    │   │   │   │   │   ├── title_us.webp
    │   │   │   │   │   └── venice.webp
    │   │   │   │   ├── og
    │   │   │   │   │   ├── china.webp
    │   │   │   │   │   ├── credit01.webp
    │   │   │   │   │   ├── credit02.webp
    │   │   │   │   │   ├── credit03.webp
    │   │   │   │   │   ├── credit04.webp
    │   │   │   │   │   ├── credit05.webp
    │   │   │   │   │   ├── credit06.webp
    │   │   │   │   │   ├── credit07.webp
    │   │   │   │   │   ├── credit08.webp
    │   │   │   │   │   ├── end.webp
    │   │   │   │   │   ├── legal.webp
    │   │   │   │   │   ├── mansion.webp
    │   │   │   │   │   ├── rig.webp
    │   │   │   │   │   ├── tibet.webp
    │   │   │   │   │   ├── titan.webp
    │   │   │   │   │   ├── title_eu.webp
    │   │   │   │   │   ├── title_us.webp
    │   │   │   │   │   └── venice.webp
    │   │   │   │   ├── china.webp
    │   │   │   │   ├── credit01.webp
    │   │   │   │   ├── credit02.webp
    │   │   │   │   ├── credit03.webp
    │   │   │   │   ├── credit04.webp
    │   │   │   │   ├── credit05.webp
    │   │   │   │   ├── credit06.webp
    │   │   │   │   ├── credit07.webp
    │   │   │   │   ├── credit08.webp
    │   │   │   │   ├── end.webp
    │   │   │   │   ├── legal_eu.webp
    │   │   │   │   ├── legal_us.webp
    │   │   │   │   ├── mansion.webp
    │   │   │   │   ├── rig.webp
    │   │   │   │   ├── tibet.webp
    │   │   │   │   ├── titan.webp
    │   │   │   │   ├── title_eu.webp
    │   │   │   │   ├── title_us.webp
    │   │   │   │   └── venice.webp
    │   │   │   ├── injections
    │   │   │   │   ├── barkhang_cameras.bin
    │   │   │   │   ├── barkhang_crystals.bin
    │   │   │   │   ├── barkhang_fd.bin
    │   │   │   │   ├── barkhang_itemrots.bin
    │   │   │   │   ├── barkhang_music_tracks.bin
    │   │   │   │   ├── barkhang_pickup_meshes.bin
    │   │   │   │   ├── barkhang_sfx.bin
    │   │   │   │   ├── barkhang_textures.bin
    │   │   │   │   ├── bartoli_crystals.bin
    │   │   │   │   ├── bartoli_music_tracks.bin
    │   │   │   │   ├── bartoli_secret_fd.bin
    │   │   │   │   ├── bartoli_textures.bin
    │   │   │   │   ├── binoculars.bin
    │   │   │   │   ├── boat_bits.bin
    │   │   │   │   ├── boat_sfx.bin
    │   │   │   │   ├── breakable_tile_sfx.bin
    │   │   │   │   ├── catacombs_crystals.bin
    │   │   │   │   ├── catacombs_fd.bin
    │   │   │   │   ├── catacombs_itemrots.bin
    │   │   │   │   ├── catacombs_music_tracks.bin
    │   │   │   │   ├── catacombs_textures.bin
    │   │   │   │   ├── coldwar_crystals.bin
    │   │   │   │   ├── coldwar_fd.bin
    │   │   │   │   ├── coldwar_itemrots.bin
    │   │   │   │   ├── coldwar_music_tracks.bin
    │   │   │   │   ├── coldwar_objects.bin
    │   │   │   │   ├── coldwar_textures.bin
    │   │   │   │   ├── common_pickup_meshes.bin
    │   │   │   │   ├── common_pickup_meshes_gm.bin
    │   │   │   │   ├── crystal.bin
    │   │   │   │   ├── cut2_setup.bin
    │   │   │   │   ├── cut2_textures.bin
    │   │   │   │   ├── cut3_setup.bin
    │   │   │   │   ├── cut3_textures.bin
    │   │   │   │   ├── cut4_setup.bin
    │   │   │   │   ├── cut4_textures.bin
    │   │   │   │   ├── dagger_sprite.bin
    │   │   │   │   ├── deck_cameras.bin
    │   │   │   │   ├── deck_crystals.bin
    │   │   │   │   ├── deck_fd.bin
    │   │   │   │   ├── deck_itemrots.bin
    │   │   │   │   ├── deck_music_tracks.bin
    │   │   │   │   ├── deck_pickup_meshes.bin
    │   │   │   │   ├── deck_plants.bin
    │   │   │   │   ├── deck_secret_fd.bin
    │   │   │   │   ├── deck_textures.bin
    │   │   │   │   ├── detonator_lights.bin
    │   │   │   │   ├── diving_cameras.bin
    │   │   │   │   ├── diving_crystals.bin
    │   │   │   │   ├── diving_itemrots.bin
    │   │   │   │   ├── diving_music_tracks.bin
    │   │   │   │   ├── diving_pickup_meshes.bin
    │   │   │   │   ├── diving_sfx.bin
    │   │   │   │   ├── diving_textures.bin
    │   │   │   │   ├── door106_sfx.bin
    │   │   │   │   ├── door107_sfx.bin
    │   │   │   │   ├── door108_sfx.bin
    │   │   │   │   ├── door110_sfx.bin
    │   │   │   │   ├── door111_sfx.bin
    │   │   │   │   ├── explosion.bin
    │   │   │   │   ├── fathoms_crystals.bin
    │   │   │   │   ├── fathoms_goon_sfx.bin
    │   │   │   │   ├── fathoms_itemrots.bin
    │   │   │   │   ├── fathoms_music_tracks.bin
    │   │   │   │   ├── fathoms_plants.bin
    │   │   │   │   ├── fathoms_secret_fd.bin
    │   │   │   │   ├── fathoms_textures.bin
    │   │   │   │   ├── floating_crystals.bin
    │   │   │   │   ├── floating_fd.bin
    │   │   │   │   ├── floating_itemrots.bin
    │   │   │   │   ├── floating_music_tracks.bin
    │   │   │   │   ├── floating_pickup_meshes.bin
    │   │   │   │   ├── floating_textures.bin
    │   │   │   │   ├── font.bin
    │   │   │   │   ├── fools_crystals.bin
    │   │   │   │   ├── fools_itemrots.bin
    │   │   │   │   ├── fools_music_tracks.bin
    │   │   │   │   ├── fools_pickup_meshes.bin
    │   │   │   │   ├── fools_textures.bin
    │   │   │   │   ├── furnace_crystals.bin
    │   │   │   │   ├── furnace_itemrots.bin
    │   │   │   │   ├── furnace_music_tracks.bin
    │   │   │   │   ├── furnace_objects.bin
    │   │   │   │   ├── furnace_pickup_meshes.bin
    │   │   │   │   ├── furnace_textures.bin
    │   │   │   │   ├── guardian_death_commands.bin
    │   │   │   │   ├── gym_fd.bin
    │   │   │   │   ├── gym_music_tracks.bin
    │   │   │   │   ├── gym_sfx.bin
    │   │   │   │   ├── gym_textures.bin
    │   │   │   │   ├── house_itemrots.bin
    │   │   │   │   ├── house_music_tracks.bin
    │   │   │   │   ├── house_sfx.bin
    │   │   │   │   ├── house_shower_frames.bin
    │   │   │   │   ├── house_textures.bin
    │   │   │   │   ├── inv_background.bin
    │   │   │   │   ├── kingdom_cameras.bin
    │   │   │   │   ├── kingdom_crystals.bin
    │   │   │   │   ├── kingdom_itemrots.bin
    │   │   │   │   ├── kingdom_music_tracks.bin
    │   │   │   │   ├── kingdom_textures.bin
    │   │   │   │   ├── lair_bartolipos.bin
    │   │   │   │   ├── lair_crystals.bin
    │   │   │   │   ├── lair_music_tracks.bin
    │   │   │   │   ├── lair_textures.bin
    │   │   │   │   ├── lara_animations.bin
    │   │   │   │   ├── lara_extra.bin
    │   │   │   │   ├── lara_guns.bin
    │   │   │   │   ├── lara_gym_guns.bin
    │   │   │   │   ├── lara_house_guns.bin
    │   │   │   │   ├── lara_outfits.bin
    │   │   │   │   ├── lara_rifle_sfx.bin
    │   │   │   │   ├── lara_vegas_guns.bin
    │   │   │   │   ├── living_crystals.bin
    │   │   │   │   ├── living_deck_goon_sfx.bin
    │   │   │   │   ├── living_fd.bin
    │   │   │   │   ├── living_itemrots.bin
    │   │   │   │   ├── living_music_tracks.bin
    │   │   │   │   ├── living_pickup_meshes.bin
    │   │   │   │   ├── living_secret_fd.bin
    │   │   │   │   ├── living_sfx.bin
    │   │   │   │   ├── living_textures.bin
    │   │   │   │   ├── loose_boards_sfx.bin
    │   │   │   │   ├── misc_sprites.bin
    │   │   │   │   ├── opera_crystals.bin
    │   │   │   │   ├── opera_fd.bin
    │   │   │   │   ├── opera_itemrots.bin
    │   │   │   │   ├── opera_music_tracks.bin
    │   │   │   │   ├── opera_sfx.bin
    │   │   │   │   ├── opera_textures.bin
    │   │   │   │   ├── palace_crystals.bin
    │   │   │   │   ├── palace_fd.bin
    │   │   │   │   ├── palace_itemrots.bin
    │   │   │   │   ├── palace_music_tracks.bin
    │   │   │   │   ├── palace_secret_fd.bin
    │   │   │   │   ├── palace_textures.bin
    │   │   │   │   ├── pda_model.bin
    │   │   │   │   ├── photo.bin
    │   │   │   │   ├── pickup_aid.bin
    │   │   │   │   ├── portcullis_sfx.bin
    │   │   │   │   ├── rig_crystals.bin
    │   │   │   │   ├── rig_itemrots.bin
    │   │   │   │   ├── rig_music_tracks.bin
    │   │   │   │   ├── rig_pickup_meshes.bin
    │   │   │   │   ├── rig_textures.bin
    │   │   │   │   ├── scuba_sfx.bin
    │   │   │   │   ├── seaweed_collision.bin
    │   │   │   │   ├── secret_models_gm.bin
    │   │   │   │   ├── secret_models_og.bin
    │   │   │   │   ├── shark_sfx.bin
    │   │   │   │   ├── tibet_crystals.bin
    │   │   │   │   ├── tibet_fd.bin
    │   │   │   │   ├── tibet_itemrots.bin
    │   │   │   │   ├── tibet_music_tracks.bin
    │   │   │   │   ├── tibet_textures.bin
    │   │   │   │   ├── title_textures.bin
    │   │   │   │   ├── vegas_crystals.bin
    │   │   │   │   ├── vegas_fd.bin
    │   │   │   │   ├── vegas_itemrots.bin
    │   │   │   │   ├── vegas_music_tracks.bin
    │   │   │   │   ├── vegas_textures.bin
    │   │   │   │   ├── venice_crystals.bin
    │   │   │   │   ├── venice_fd.bin
    │   │   │   │   ├── venice_itemrots.bin
    │   │   │   │   ├── venice_music_tracks.bin
    │   │   │   │   ├── venice_textures.bin
    │   │   │   │   ├── wall_cameras.bin
    │   │   │   │   ├── wall_crystals.bin
    │   │   │   │   ├── wall_itemrots.bin
    │   │   │   │   ├── wall_music_tracks.bin
    │   │   │   │   ├── wall_switch_sfx.bin
    │   │   │   │   ├── wall_textures.bin
    │   │   │   │   ├── winston_model.bin
    │   │   │   │   ├── wreck_cameras.bin
    │   │   │   │   ├── wreck_crystals.bin
    │   │   │   │   ├── wreck_fd.bin
    │   │   │   │   ├── wreck_goon_sfx.bin
    │   │   │   │   ├── wreck_itemrots.bin
    │   │   │   │   ├── wreck_music_tracks.bin
    │   │   │   │   ├── wreck_pickup_meshes.bin
    │   │   │   │   ├── wreck_plants.bin
    │   │   │   │   ├── wreck_secret_fd.bin
    │   │   │   │   ├── wreck_textures.bin
    │   │   │   │   ├── xian_crystals.bin
    │   │   │   │   ├── xian_fd.bin
    │   │   │   │   ├── xian_itemrots.bin
    │   │   │   │   ├── xian_music_tracks.bin
    │   │   │   │   ├── xian_pickup_meshes.bin
    │   │   │   │   ├── xian_sfx.bin
    │   │   │   │   └── xian_textures.bin
    │   │   │   ├── levels
    │   │   │   │   ├── assault.tr2
    │   │   │   │   ├── boat.tr2
    │   │   │   │   ├── catacomb.tr2
    │   │   │   │   ├── cut1.tr2
    │   │   │   │   ├── cut2.tr2
    │   │   │   │   ├── cut3.tr2
    │   │   │   │   ├── cut4.tr2
    │   │   │   │   ├── deck.tr2
    │   │   │   │   ├── emprtomb.tr2
    │   │   │   │   ├── floating.tr2
    │   │   │   │   ├── house.tr2
    │   │   │   │   ├── icecave.tr2
    │   │   │   │   ├── keel.tr2
    │   │   │   │   ├── living.tr2
    │   │   │   │   ├── monastry.tr2
    │   │   │   │   ├── opera.tr2
    │   │   │   │   ├── platform.tr2
    │   │   │   │   ├── rig.tr2
    │   │   │   │   ├── skidoo.tr2
    │   │   │   │   ├── title.tr2
    │   │   │   │   ├── unwater.tr2
    │   │   │   │   ├── venice.tr2
    │   │   │   │   ├── wall.tr2
    │   │   │   │   └── xian.tr2
    │   │   │   ├── music
    │   │   │   │   ├── 2.mp3
    │   │   │   │   ├── 3.mp3
    │   │   │   │   ├── 4.mp3
    │   │   │   │   ├── 5.mp3
    │   │   │   │   ├── 6.mp3
    │   │   │   │   ├── 7.mp3
    │   │   │   │   ├── 8.mp3
    │   │   │   │   ├── 9.mp3
    │   │   │   │   ├── 10.mp3
    │   │   │   │   ├── 11.mp3
    │   │   │   │   ├── 12.mp3
    │   │   │   │   ├── 13.mp3
    │   │   │   │   ├── 14.mp3
    │   │   │   │   ├── 15.mp3
    │   │   │   │   ├── 16.mp3
    │   │   │   │   ├── 17.mp3
    │   │   │   │   ├── 18.mp3
    │   │   │   │   ├── 19.mp3
    │   │   │   │   ├── 20.mp3
    │   │   │   │   ├── 21.mp3
    │   │   │   │   ├── 22.mp3
    │   │   │   │   ├── 23.mp3
    │   │   │   │   ├── 24.mp3
    │   │   │   │   ├── 25.mp3
    │   │   │   │   ├── 26.mp3
    │   │   │   │   ├── 27.mp3
    │   │   │   │   ├── 28.mp3
    │   │   │   │   ├── 29.mp3
    │   │   │   │   ├── 30.mp3
    │   │   │   │   ├── 31.mp3
    │   │   │   │   ├── 32.mp3
    │   │   │   │   ├── 33.mp3
    │   │   │   │   ├── 34.mp3
    │   │   │   │   ├── 35.mp3
    │   │   │   │   ├── 36.mp3
    │   │   │   │   ├── 37.mp3
    │   │   │   │   ├── 38.mp3
    │   │   │   │   ├── 39.mp3
    │   │   │   │   ├── 40.mp3
    │   │   │   │   ├── 41.mp3
    │   │   │   │   ├── 42.mp3
    │   │   │   │   ├── 43.mp3
    │   │   │   │   ├── 44.mp3
    │   │   │   │   ├── 45.mp3
    │   │   │   │   ├── 46.mp3
    │   │   │   │   ├── 47.mp3
    │   │   │   │   ├── 48.mp3
    │   │   │   │   ├── 49.mp3
    │   │   │   │   ├── 50.mp3
    │   │   │   │   ├── 51.mp3
    │   │   │   │   ├── 52.mp3
    │   │   │   │   ├── 53.mp3
    │   │   │   │   ├── 54.mp3
    │   │   │   │   ├── 55.mp3
    │   │   │   │   ├── 56.mp3
    │   │   │   │   ├── 57.mp3
    │   │   │   │   ├── 58.mp3
    │   │   │   │   ├── 59.mp3
    │   │   │   │   ├── 60.mp3
    │   │   │   │   └── 61.mp3
    │   │   │   ├── scripts
    │   │   │   │   ├── _game.lua
    │   │   │   │   ├── assault.lua
    │   │   │   │   ├── cut3.lua
    │   │   │   │   ├── floating.lua
    │   │   │   │   ├── house.lua
    │   │   │   │   └── monastry.lua
    │   │   │   ├── catalog_item_actions.csv
    │   │   │   ├── catalog_lara_anims.csv
    │   │   │   ├── catalog_lara_states.csv
    │   │   │   ├── catalog_music.csv
    │   │   │   ├── catalog_objects.csv
    │   │   │   ├── catalog_samples.csv
    │   │   │   ├── gameflow.json5
    │   │   │   ├── inv_ring.json5
    │   │   │   ├── main.sfx
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-en-gb.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   ├── strings.json5
    │   │   │   └── weapons.json5
    │   │   ├── tr2-gm
    │   │   │   ├── images
    │   │   │   │   ├── 3x2
    │   │   │   │   │   ├── credit00_gm.webp
    │   │   │   │   │   ├── credit07_gm.webp
    │   │   │   │   │   ├── gm_level1.webp
    │   │   │   │   │   ├── gm_level2.webp
    │   │   │   │   │   ├── gm_level3.webp
    │   │   │   │   │   ├── gm_level4.webp
    │   │   │   │   │   ├── gm_level5.webp
    │   │   │   │   │   ├── legal_eu_gm.webp
    │   │   │   │   │   ├── legal_us_gm.webp
    │   │   │   │   │   ├── title_eu_gm.webp
    │   │   │   │   │   └── title_us_gm.webp
    │   │   │   │   ├── 4x3
    │   │   │   │   │   ├── credit00_gm.webp
    │   │   │   │   │   ├── credit07_gm.webp
    │   │   │   │   │   ├── gm_level1.webp
    │   │   │   │   │   ├── gm_level2.webp
    │   │   │   │   │   ├── gm_level3.webp
    │   │   │   │   │   ├── gm_level4.webp
    │   │   │   │   │   ├── gm_level5.webp
    │   │   │   │   │   ├── legal_eu_gm.webp
    │   │   │   │   │   ├── legal_us_gm.webp
    │   │   │   │   │   ├── title_eu_gm.webp
    │   │   │   │   │   └── title_us_gm.webp
    │   │   │   │   ├── og
    │   │   │   │   │   ├── credit00_gm.webp
    │   │   │   │   │   ├── credit07_gm.webp
    │   │   │   │   │   ├── title_eu_gm.webp
    │   │   │   │   │   └── title_us_gm.webp
    │   │   │   │   ├── credit00_gm.webp
    │   │   │   │   ├── credit07_gm.webp
    │   │   │   │   ├── gm_level1.webp
    │   │   │   │   ├── gm_level2.webp
    │   │   │   │   ├── gm_level3.webp
    │   │   │   │   ├── gm_level4.webp
    │   │   │   │   ├── gm_level5.webp
    │   │   │   │   ├── legal_eu_gm.webp
    │   │   │   │   ├── legal_us_gm.webp
    │   │   │   │   ├── title_eu_gm.webp
    │   │   │   │   └── title_us_gm.webp
    │   │   │   ├── levels
    │   │   │   │   ├── level1.tr2
    │   │   │   │   ├── level2.tr2
    │   │   │   │   ├── level3.tr2
    │   │   │   │   ├── level4.tr2
    │   │   │   │   ├── level5.tr2
    │   │   │   │   └── title.tr2
    │   │   │   ├── scripts
    │   │   │   │   ├── _game.lua
    │   │   │   │   ├── level1.lua
    │   │   │   │   ├── level3.lua
    │   │   │   │   └── level4.lua
    │   │   │   ├── gameflow.json5
    │   │   │   ├── main.sfx
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   └── strings.json5
    │   │   ├── tr2-level
    │   │   │   ├── gameflow.json5
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   └── strings.json5
    │   │   ├── tr3
    │   │   │   ├── audio
    │   │   │   │   └── cdaudio.wad
    │   │   │   ├── cuts
    │   │   │   │   ├── cut1.tr2
    │   │   │   │   ├── cut2.tr2
    │   │   │   │   ├── cut3.tr2
    │   │   │   │   ├── cut4.tr2
    │   │   │   │   ├── cut5.tr2
    │   │   │   │   ├── cut6.tr2
    │   │   │   │   ├── cut7.tr2
    │   │   │   │   ├── cut8.tr2
    │   │   │   │   ├── cut9.tr2
    │   │   │   │   ├── cut11.tr2
    │   │   │   │   └── cut12.tr2
    │   │   │   ├── fmv
    │   │   │   │   ├── crsh_eng.rpl
    │   │   │   │   ├── endgame.rpl
    │   │   │   │   ├── intr_eng.rpl
    │   │   │   │   ├── logo.rpl
    │   │   │   │   └── sail_eng.rpl
    │   │   │   ├── images
    │   │   │   │   ├── 3x2
    │   │   │   │   │   ├── antarc.webp
    │   │   │   │   │   ├── credit01.webp
    │   │   │   │   │   ├── credit02.webp
    │   │   │   │   │   ├── credit03.webp
    │   │   │   │   │   ├── credit04.webp
    │   │   │   │   │   ├── credit05.webp
    │   │   │   │   │   ├── credit06.webp
    │   │   │   │   │   ├── credit07.webp
    │   │   │   │   │   ├── credit08.webp
    │   │   │   │   │   ├── credit09.webp
    │   │   │   │   │   ├── house.webp
    │   │   │   │   │   ├── india.webp
    │   │   │   │   │   ├── legal_eu.webp
    │   │   │   │   │   ├── legal_us.webp
    │   │   │   │   │   ├── london.webp
    │   │   │   │   │   ├── nevada.webp
    │   │   │   │   │   ├── southpac.webp
    │   │   │   │   │   ├── theend2.webp
    │   │   │   │   │   ├── theend.webp
    │   │   │   │   │   ├── title_eu.webp
    │   │   │   │   │   └── title_us.webp
    │   │   │   │   ├── 4x3
    │   │   │   │   │   ├── antarc.webp
    │   │   │   │   │   ├── credit01.webp
    │   │   │   │   │   ├── credit02.webp
    │   │   │   │   │   ├── credit03.webp
    │   │   │   │   │   ├── credit04.webp
    │   │   │   │   │   ├── credit05.webp
    │   │   │   │   │   ├── credit06.webp
    │   │   │   │   │   ├── credit07.webp
    │   │   │   │   │   ├── credit08.webp
    │   │   │   │   │   ├── credit09.webp
    │   │   │   │   │   ├── house.webp
    │   │   │   │   │   ├── india.webp
    │   │   │   │   │   ├── legal_eu.webp
    │   │   │   │   │   ├── legal_us.webp
    │   │   │   │   │   ├── london.webp
    │   │   │   │   │   ├── nevada.webp
    │   │   │   │   │   ├── southpac.webp
    │   │   │   │   │   ├── theend2.webp
    │   │   │   │   │   ├── theend.webp
    │   │   │   │   │   ├── title_eu.webp
    │   │   │   │   │   └── title_us.webp
    │   │   │   │   ├── og
    │   │   │   │   │   ├── antarc.webp
    │   │   │   │   │   ├── credit01.webp
    │   │   │   │   │   ├── credit02.webp
    │   │   │   │   │   ├── credit03.webp
    │   │   │   │   │   ├── credit04.webp
    │   │   │   │   │   ├── credit05.webp
    │   │   │   │   │   ├── credit06.webp
    │   │   │   │   │   ├── credit07.webp
    │   │   │   │   │   ├── credit08.webp
    │   │   │   │   │   ├── credit09.webp
    │   │   │   │   │   ├── house.webp
    │   │   │   │   │   ├── india.webp
    │   │   │   │   │   ├── legal_eu.webp
    │   │   │   │   │   ├── legal_us.webp
    │   │   │   │   │   ├── london.webp
    │   │   │   │   │   ├── nevada.webp
    │   │   │   │   │   ├── southpac.webp
    │   │   │   │   │   ├── theend2.webp
    │   │   │   │   │   ├── theend.webp
    │   │   │   │   │   ├── title_eu.webp
    │   │   │   │   │   └── title_us.webp
    │   │   │   │   ├── antarc.webp
    │   │   │   │   ├── credit01.webp
    │   │   │   │   ├── credit02.webp
    │   │   │   │   ├── credit03.webp
    │   │   │   │   ├── credit04.webp
    │   │   │   │   ├── credit05.webp
    │   │   │   │   ├── credit06.webp
    │   │   │   │   ├── credit07.webp
    │   │   │   │   ├── credit08.webp
    │   │   │   │   ├── credit09.webp
    │   │   │   │   ├── house.webp
    │   │   │   │   ├── india.webp
    │   │   │   │   ├── legal_eu.webp
    │   │   │   │   ├── legal_us.webp
    │   │   │   │   ├── london.webp
    │   │   │   │   ├── nevada.webp
    │   │   │   │   ├── southpac.webp
    │   │   │   │   ├── theend2.webp
    │   │   │   │   ├── theend.webp
    │   │   │   │   ├── title_eu.webp
    │   │   │   │   └── title_us.webp
    │   │   │   ├── injections
    │   │   │   │   ├── alarm_sfx.bin
    │   │   │   │   ├── aldwych_animating_bounds.bin
    │   │   │   │   ├── aldwych_fd.bin
    │   │   │   │   ├── aldwych_pickup_meshes.bin
    │   │   │   │   ├── aldwych_textures.bin
    │   │   │   │   ├── antarc_airlock.bin
    │   │   │   │   ├── antarc_door134_frames.bin
    │   │   │   │   ├── antarc_fd.bin
    │   │   │   │   ├── antarc_sky.bin
    │   │   │   │   ├── antarc_textures.bin
    │   │   │   │   ├── antarc_wheel_frames.bin
    │   │   │   │   ├── area51_animating_bounds.bin
    │   │   │   │   ├── area51_patrol.bin
    │   │   │   │   ├── area51_sky.bin
    │   │   │   │   ├── area51_textures.bin
    │   │   │   │   ├── bat_sprites.bin
    │   │   │   │   ├── binoculars.bin
    │   │   │   │   ├── cavern_door131_frames.bin
    │   │   │   │   ├── cavern_pickup_meshes.bin
    │   │   │   │   ├── cavern_sky.bin
    │   │   │   │   ├── cavern_textures.bin
    │   │   │   │   ├── caves_textures.bin
    │   │   │   │   ├── city_textures.bin
    │   │   │   │   ├── cliff_animating_bounds.bin
    │   │   │   │   ├── cliff_crystals.bin
    │   │   │   │   ├── cliff_door132_frames.bin
    │   │   │   │   ├── cliff_textures.bin
    │   │   │   │   ├── coastal_airlock.bin
    │   │   │   │   ├── coastal_animating_bounds.bin
    │   │   │   │   ├── coastal_fd.bin
    │   │   │   │   ├── coastal_sky.bin
    │   │   │   │   ├── coastal_spike_sfx.bin
    │   │   │   │   ├── coastal_textures.bin
    │   │   │   │   ├── coastal_wheel_frames.bin
    │   │   │   │   ├── common_pickup_meshes.bin
    │   │   │   │   ├── compound_animating_bounds.bin
    │   │   │   │   ├── compound_cine.bin
    │   │   │   │   ├── compound_patrol.bin
    │   │   │   │   ├── compound_textures.bin
    │   │   │   │   ├── crash_animating_bounds.bin
    │   │   │   │   ├── crash_pickup_meshes.bin
    │   │   │   │   ├── crash_sky.bin
    │   │   │   │   ├── crash_textures.bin
    │   │   │   │   ├── crystal.bin
    │   │   │   │   ├── cut1_setup.bin
    │   │   │   │   ├── cut2_setup.bin
    │   │   │   │   ├── cut3_setup.bin
    │   │   │   │   ├── cut3_shell.bin
    │   │   │   │   ├── cut4_setup.bin
    │   │   │   │   ├── cut5_setup.bin
    │   │   │   │   ├── cut5_textures.bin
    │   │   │   │   ├── cut6_setup.bin
    │   │   │   │   ├── cut7_setup.bin
    │   │   │   │   ├── cut8_setup.bin
    │   │   │   │   ├── cut9_setup.bin
    │   │   │   │   ├── cut11_setup.bin
    │   │   │   │   ├── cut12_setup.bin
    │   │   │   │   ├── drill_collision.bin
    │   │   │   │   ├── fish_sprites.bin
    │   │   │   │   ├── flamethrower_sfx.bin
    │   │   │   │   ├── font.bin
    │   │   │   │   ├── ganges_animating_bounds.bin
    │   │   │   │   ├── ganges_door131_frames.bin
    │   │   │   │   ├── ganges_textures.bin
    │   │   │   │   ├── globe_model.bin
    │   │   │   │   ├── gym_fd.bin
    │   │   │   │   ├── gym_sky.bin
    │   │   │   │   ├── gym_textures.bin
    │   │   │   │   ├── hiss_sfx.bin
    │   │   │   │   ├── india_sky.bin
    │   │   │   │   ├── inv_background.bin
    │   │   │   │   ├── jungle_fd.bin
    │   │   │   │   ├── jungle_textures.bin
    │   │   │   │   ├── lara_animations.bin
    │   │   │   │   ├── lara_extra.bin
    │   │   │   │   ├── lara_guns.bin
    │   │   │   │   ├── lara_gym_guns.bin
    │   │   │   │   ├── lara_outfits.bin
    │   │   │   │   ├── lara_rifle_sfx.bin
    │   │   │   │   ├── london_sky.bin
    │   │   │   │   ├── luds_animating_bounds.bin
    │   │   │   │   ├── luds_diver_animation.bin
    │   │   │   │   ├── luds_fd.bin
    │   │   │   │   ├── luds_textures.bin
    │   │   │   │   ├── madubu_spike_sfx.bin
    │   │   │   │   ├── madubu_textures.bin
    │   │   │   │   ├── menu_artefacts.bin
    │   │   │   │   ├── menu_artefacts_london.bin
    │   │   │   │   ├── mines_textures.bin
    │   │   │   │   ├── misc_sprites.bin
    │   │   │   │   ├── nevada_animating_bounds.bin
    │   │   │   │   ├── nevada_door132_frames.bin
    │   │   │   │   ├── nevada_sky.bin
    │   │   │   │   ├── nevada_textures.bin
    │   │   │   │   ├── pda_model.bin
    │   │   │   │   ├── pickup_aid.bin
    │   │   │   │   ├── pipeman_sfx.bin
    │   │   │   │   ├── puna_pickup_meshes.bin
    │   │   │   │   ├── puna_textures.bin
    │   │   │   │   ├── rapids_sky.bin
    │   │   │   │   ├── reunion_crystals.bin
    │   │   │   │   ├── reunion_flames.bin
    │   │   │   │   ├── reunion_textures.bin
    │   │   │   │   ├── scotland_crystals.bin
    │   │   │   │   ├── scotland_sky.bin
    │   │   │   │   ├── scotland_textures.bin
    │   │   │   │   ├── sparks_gfx.bin
    │   │   │   │   ├── stpaul_animating_bounds.bin
    │   │   │   │   ├── stpaul_textures.bin
    │   │   │   │   ├── temple_itemrots.bin
    │   │   │   │   ├── temple_textures.bin
    │   │   │   │   ├── thames_animating_bounds.bin
    │   │   │   │   ├── thames_fd.bin
    │   │   │   │   ├── thames_textures.bin
    │   │   │   │   ├── tinnos_cameras.bin
    │   │   │   │   ├── tinnos_fd.bin
    │   │   │   │   ├── tinnos_flames.bin
    │   │   │   │   ├── tinnos_textures.bin
    │   │   │   │   ├── title_textures.bin
    │   │   │   │   ├── undersea_animating_bounds.bin
    │   │   │   │   ├── undersea_animating_ext.bin
    │   │   │   │   ├── undersea_crystals.bin
    │   │   │   │   ├── undersea_fd.bin
    │   │   │   │   ├── undersea_objects.bin
    │   │   │   │   ├── undersea_pickup_meshes.bin
    │   │   │   │   ├── undersea_textures.bin
    │   │   │   │   ├── undersea_train.bin
    │   │   │   │   ├── undersea_wheel_frames.bin
    │   │   │   │   ├── water_sfx.bin
    │   │   │   │   ├── willsden_crystals.bin
    │   │   │   │   ├── willsden_heli.bin
    │   │   │   │   ├── willsden_textures.bin
    │   │   │   │   ├── winston_model.bin
    │   │   │   │   ├── zoo_crystals.bin
    │   │   │   │   ├── zoo_textures.bin
    │   │   │   │   └── zoo_train.bin
    │   │   │   ├── levels
    │   │   │   │   ├── antarc.tr2
    │   │   │   │   ├── area51.tr2
    │   │   │   │   ├── chamber.tr2
    │   │   │   │   ├── city.tr2
    │   │   │   │   ├── compound.tr2
    │   │   │   │   ├── crash.tr2
    │   │   │   │   ├── house.tr2
    │   │   │   │   ├── jungle.tr2
    │   │   │   │   ├── mines.tr2
    │   │   │   │   ├── nevada.tr2
    │   │   │   │   ├── office.tr2
    │   │   │   │   ├── quadchas.tr2
    │   │   │   │   ├── rapids.tr2
    │   │   │   │   ├── roofs.tr2
    │   │   │   │   ├── sewer.tr2
    │   │   │   │   ├── shore.tr2
    │   │   │   │   ├── stpaul.tr2
    │   │   │   │   ├── temple.tr2
    │   │   │   │   ├── title.tr2
    │   │   │   │   ├── tonyboss.tr2
    │   │   │   │   ├── tower.tr2
    │   │   │   │   └── triboss.tr2
    │   │   │   ├── scripts
    │   │   │   │   ├── _game.lua
    │   │   │   │   ├── antarc.lua
    │   │   │   │   ├── area51.lua
    │   │   │   │   ├── chamber.lua
    │   │   │   │   ├── city.lua
    │   │   │   │   ├── compound.lua
    │   │   │   │   ├── crash.lua
    │   │   │   │   ├── cut8.lua
    │   │   │   │   ├── house.lua
    │   │   │   │   ├── jungle.lua
    │   │   │   │   ├── mines.lua
    │   │   │   │   ├── nevada.lua
    │   │   │   │   ├── office.lua
    │   │   │   │   ├── quadchas.lua
    │   │   │   │   ├── rapids.lua
    │   │   │   │   ├── roofs.lua
    │   │   │   │   ├── sewer.lua
    │   │   │   │   ├── shore.lua
    │   │   │   │   ├── temple.lua
    │   │   │   │   ├── tonyboss.lua
    │   │   │   │   ├── tower.lua
    │   │   │   │   └── triboss.lua
    │   │   │   ├── catalog_item_actions.csv
    │   │   │   ├── catalog_lara_anims.csv
    │   │   │   ├── catalog_lara_states.csv
    │   │   │   ├── catalog_music.csv
    │   │   │   ├── catalog_objects.csv
    │   │   │   ├── catalog_samples.csv
    │   │   │   ├── gameflow.json5
    │   │   │   ├── inv_ring.json5
    │   │   │   ├── main.sfx
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   ├── strings.json5
    │   │   │   ├── tombpc.dat
    │   │   │   └── weapons.json5
    │   │   ├── tr3-la
    │   │   │   ├── images
    │   │   │   │   ├── 3x2
    │   │   │   │   │   ├── chunnel.webp
    │   │   │   │   │   ├── credit01_eu_la.webp
    │   │   │   │   │   ├── credit01_us_la.webp
    │   │   │   │   │   ├── credit02_la.webp
    │   │   │   │   │   ├── credit03_la.webp
    │   │   │   │   │   ├── credit04_la.webp
    │   │   │   │   │   ├── credit05_la.webp
    │   │   │   │   │   ├── credit06_la.webp
    │   │   │   │   │   ├── credit07_la.webp
    │   │   │   │   │   ├── credit08_la.webp
    │   │   │   │   │   ├── credit09_la.webp
    │   │   │   │   │   ├── highland.webp
    │   │   │   │   │   ├── legal_eu_la.webp
    │   │   │   │   │   ├── legal_us_la.webp
    │   │   │   │   │   ├── slinc.webp
    │   │   │   │   │   ├── theend2_la.webp
    │   │   │   │   │   ├── theend_la.webp
    │   │   │   │   │   ├── title_eu_la.webp
    │   │   │   │   │   ├── title_us_la.webp
    │   │   │   │   │   ├── undersea.webp
    │   │   │   │   │   ├── willard.webp
    │   │   │   │   │   └── zoo.webp
    │   │   │   │   ├── 4x3
    │   │   │   │   │   ├── chunnel.webp
    │   │   │   │   │   ├── credit01_eu_la.webp
    │   │   │   │   │   ├── credit01_us_la.webp
    │   │   │   │   │   ├── credit02_la.webp
    │   │   │   │   │   ├── credit03_la.webp
    │   │   │   │   │   ├── credit04_la.webp
    │   │   │   │   │   ├── credit05_la.webp
    │   │   │   │   │   ├── credit06_la.webp
    │   │   │   │   │   ├── credit07_la.webp
    │   │   │   │   │   ├── credit08_la.webp
    │   │   │   │   │   ├── credit09_la.webp
    │   │   │   │   │   ├── highland.webp
    │   │   │   │   │   ├── legal_eu_la.webp
    │   │   │   │   │   ├── legal_us_la.webp
    │   │   │   │   │   ├── slinc.webp
    │   │   │   │   │   ├── title_eu_la.webp
    │   │   │   │   │   ├── title_us_la.webp
    │   │   │   │   │   ├── undersea.webp
    │   │   │   │   │   ├── willard.webp
    │   │   │   │   │   └── zoo.webp
    │   │   │   │   ├── chunnel.webp
    │   │   │   │   ├── credit01_eu_la.webp
    │   │   │   │   ├── credit01_us_la.webp
    │   │   │   │   ├── credit02_la.webp
    │   │   │   │   ├── credit03_la.webp
    │   │   │   │   ├── credit04_la.webp
    │   │   │   │   ├── credit05_la.webp
    │   │   │   │   ├── credit06_la.webp
    │   │   │   │   ├── credit07_la.webp
    │   │   │   │   ├── credit08_la.webp
    │   │   │   │   ├── credit09_la.webp
    │   │   │   │   ├── highland.webp
    │   │   │   │   ├── legal_eu_la.webp
    │   │   │   │   ├── legal_us_la.webp
    │   │   │   │   ├── slinc.webp
    │   │   │   │   ├── theend2_la.webp
    │   │   │   │   ├── theend_la.webp
    │   │   │   │   ├── title_eu_la.webp
    │   │   │   │   ├── title_us_la.webp
    │   │   │   │   ├── undersea.webp
    │   │   │   │   ├── willard.webp
    │   │   │   │   └── zoo.webp
    │   │   │   ├── levels
    │   │   │   │   ├── chunnel.tr2
    │   │   │   │   ├── scotland.tr2
    │   │   │   │   ├── slinc.tr2
    │   │   │   │   ├── title.tr2
    │   │   │   │   ├── undersea.tr2
    │   │   │   │   ├── willsden.tr2
    │   │   │   │   └── zoo.tr2
    │   │   │   ├── scripts
    │   │   │   │   ├── _game.lua
    │   │   │   │   ├── chunnel.lua
    │   │   │   │   ├── slinc.lua
    │   │   │   │   ├── undersea.lua
    │   │   │   │   └── zoo.lua
    │   │   │   ├── gameflow.json5
    │   │   │   ├── main.sfx
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   └── strings.json5
    │   │   ├── tr3-level
    │   │   │   ├── gameflow.json5
    │   │   │   ├── strings-fr.json5
    │   │   │   ├── strings-gd.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings-pl.json5
    │   │   │   └── strings.json5
    │   │   ├── tr4
    │   │   │   ├── injections
    │   │   │   │   ├── font.bin
    │   │   │   │   ├── karnak_fd.bin
    │   │   │   │   ├── lara_animations.bin
    │   │   │   │   ├── lara_outfits.bin
    │   │   │   │   └── sparks_gfx.bin
    │   │   │   ├── modules
    │   │   │   │   ├── cutscenes.lua
    │   │   │   │   └── race_timer.lua
    │   │   │   ├── scripts
    │   │   │   │   ├── _game.lua
    │   │   │   │   ├── alexhub2.lua
    │   │   │   │   ├── alexhub.lua
    │   │   │   │   ├── ang_race.lua
    │   │   │   │   ├── angkor1.lua
    │   │   │   │   ├── bikebit.lua
    │   │   │   │   ├── cortyard.lua
    │   │   │   │   ├── csplit1.lua
    │   │   │   │   ├── csplit2.lua
    │   │   │   │   ├── highstrt.lua
    │   │   │   │   ├── joby1a.lua
    │   │   │   │   ├── joby2.lua
    │   │   │   │   ├── joby3b.lua
    │   │   │   │   ├── joby4a.lua
    │   │   │   │   ├── joby4b.lua
    │   │   │   │   ├── joby4c.lua
    │   │   │   │   ├── joby5a.lua
    │   │   │   │   ├── joby5b.lua
    │   │   │   │   ├── karnak1.lua
    │   │   │   │   ├── lake.lua
    │   │   │   │   ├── libend.lua
    │   │   │   │   ├── library.lua
    │   │   │   │   ├── lowstrt.lua
    │   │   │   │   ├── nutrench.lua
    │   │   │   │   ├── palaces2.lua
    │   │   │   │   ├── palaces.lua
    │   │   │   │   ├── semer2.lua
    │   │   │   │   ├── semer.lua
    │   │   │   │   ├── settomb1.lua
    │   │   │   │   ├── settomb2.lua
    │   │   │   │   ├── title.lua
    │   │   │   │   └── train.lua
    │   │   │   ├── catalog_item_actions.csv
    │   │   │   ├── catalog_lara_anims.csv
    │   │   │   ├── catalog_lara_states.csv
    │   │   │   ├── catalog_music.csv
    │   │   │   ├── catalog_music_files.csv
    │   │   │   ├── catalog_objects.csv
    │   │   │   ├── catalog_samples.csv
    │   │   │   ├── gameflow.json5
    │   │   │   ├── inv_ring.json5
    │   │   │   ├── strings-de.json5
    │   │   │   ├── strings-it.json5
    │   │   │   ├── strings.json5
    │   │   │   └── weapons.json5
    │   │   └── tr4-level
    │   │       ├── gameflow.json5
    │   │       ├── strings-it.json5
    │   │       └── strings.json5
    │   ├── modules
    │   │   ├── save_crystal.lua
    │   │   └── water_color.lua
    │   └── icon.icns
    ├── _CodeSignature
    ├── Frameworks
    ├── Info.plist
    └── MacOS</code></pre>
</details>

# Extracting GOG / Steam disk images

Some digital releases still package the original games as disc images or
installer payloads instead of a ready-to-copy folder tree. If your purchase
contains files such as `.bin`, `.cue`, `.iso`, `GAME.BIN`, or `GAME.GOG`,
extract those first, then copy the resulting `data/`, `fmv/`, `music/`,
`audio/`, or `cuts/` folders into TRX using the mapping below.

- **On Windows**
  - For `.iso` images, mount them in Explorer or open them with 7-Zip and copy
    the files out.
  - For `.bin` / `.cue` images, use a disc-image tool such as UltraISO or
    WinCDEmu to open or mount the image, then copy the files out.
  - For GOG installer payloads such as `GAME.GOG`, tools like UniExtract can
    unpack the contents without running the original installer.
- **On Linux**
  - For GOG installers, `innoextract` can unpack the files directly.
  - For `.iso` images, mount them or open them with an archive tool and copy
    the files out.
  - For `.bin` / `.cue` images, first convert or mount the image with a tool
    such as `bchunk` or `bin2iso`, then extract the files from the resulting
    image.

Once extracted, use the folder mapping below.

# Manually copying original game files

Please do not copy your original files into top-level `data/`, `fmv/`,
`music/`, `audio/`, or `cuts/` directories. Instead, place them in the
following structure:

- **Tomb Raider 1**
  - `data/*.phd` → `games/tr1/levels/*.phd`
  - `fmv/*` → `games/tr1/fmv/*`
  - `music/*` → `games/tr1/music/*`
- **Tomb Raider 1: Unfinished Business**
  - `data/cat.phd` → `games/tr1-ub/levels/cat.phd`
  - `data/egypt.phd` → `games/tr1-ub/levels/egypt.phd`
  - `data/end.phd` → `games/tr1-ub/levels/end.phd`
  - `data/end2.phd` → `games/tr1-ub/levels/end2.phd`
- **Tomb Raider 2**
  - `data/*.tr2` → `games/tr2/levels/*.tr2`
  - `data/main.sfx` → `games/tr2/main.sfx`
  - `fmv/*` → `games/tr2/fmv/*`
  - `music/*.mp3` → `games/tr2/music/*.mp3`
- **Tomb Raider 2: Golden Mask**
  - `data/level1.tr2` → `games/tr2-gm/levels/level1.tr2`
  - `data/level2.tr2` → `games/tr2-gm/levels/level2.tr2`
  - `data/level3.tr2` → `games/tr2-gm/levels/level3.tr2`
  - `data/level4.tr2` → `games/tr2-gm/levels/level4.tr2`
  - `data/level5.tr2` → `games/tr2-gm/levels/level5.tr2`
  - `data/title.tr2` → `games/tr2-gm/levels/title.tr2`
  - `data/main.sfx` → `games/tr2-gm/main.sfx`
- **Tomb Raider 3**
  - `data/*.tr2` → `games/tr3/levels/*.tr2`
  - `data/main.sfx` → `games/tr3/main.sfx`
  - `audio/cdaudio.wad` → `games/tr3/audio/cdaudio.wad`
  - `cuts/*.tr2` → `games/tr3/cuts/*.tr2`
  - `fmv/*` → `games/tr3/fmv/*`
- **Tomb Raider 3: The Lost Artifact**
  - `data/chunnel.tr2` → `games/tr3-la/levels/chunnel.tr2`
  - `data/scotland.tr2` → `games/tr3-la/levels/scotland.tr2`
  - `data/slinc.tr2` → `games/tr3-la/levels/slinc.tr2`
  - `data/undersea.tr2` → `games/tr3-la/levels/undersea.tr2`
  - `data/willsden.tr2` → `games/tr3-la/levels/willsden.tr2`
  - `data/zoo.tr2` → `games/tr3-la/levels/zoo.tr2`
  - `data/title.tr2` → `games/tr3-la/levels/title.tr2`
  - `data/main.sfx` → `games/tr3-la/main.sfx`

When in doubt, please refer to the "Verifying installation" detailed file
listings that show where each individual file should go.
