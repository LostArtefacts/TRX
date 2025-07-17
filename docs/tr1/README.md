<p align="center">
<img alt="TR1X logo" src="/data/tr1/logo.png" width="400"/>
</p>

## Windows / Linux

### Installing (simplified)

1. Head over to GitHub releases: https://github.com/LostArtefacts/TRX/releases
2. Download the TR1X installer. Your browser may complain that the .exe is unsafe, but it's OK to ignore this alert.
3. Mark the installer EXE as safe to run by right-clicking on the .exe, going to properties and clicking "Unblock".
4. Run the installer and proceed with the steps.

We hope that eventually these alerts will go away as the popularity of the project rises.

### Installing (advanced / manual)

1. Head over to GitHub releases: https://github.com/LostArtefacts/TRX/releases
2. Download the TR1X zip file.
3. Extract the zip file into a directory of your choice.  
   Make sure you choose to overwrite existing directories and files.
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
6. To play the Unfinished Business expansion pack, run `TR1X.exe -gold`.

If you install everything correctly, your game directory should look more or
less like this (click to expand):

<details>
<p><em>* Will not be present until the game has been launched.</em></p>
<pre>
.
├── cfg
│   ├── TR1X.json5 *
│   ├── TR1X_gameflow.json5
│   ├── TR1X_gameflow_demo_pc.json5
│   ├── TR1X_gameflow_level.json5
│   ├── TR1X_gameflow_ub.json5
│   ├── TR1X_strings.json5
│   ├── TR1X_strings_demo_pc.json5
│   ├── TR1X_strings_level.json5
│   └── TR1X_strings_ub.json5
├── data
│   ├── cat.phd
│   ├── cut1.phd
│   ├── cut2.phd
│   ├── cut3.phd
│   ├── cut4.phd
│   ├── egypt.phd
│   ├── end2.phd
│   ├── end.phd
│   ├── gym.phd
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
│   ├── title.phd
│   ├── images
│   │   ├── atlantis.webp
│   │   ├── credits_1.webp
│   │   ├── credits_2.webp
│   │   ├── credits_3.webp
│   │   ├── credits_3_alt.webp
│   │   ├── credits_ps1.webp
│   │   ├── egypt.webp
│   │   ├── eidos.webp
│   │   ├── end.webp
│   │   ├── greece.webp
│   │   ├── greece_saturn.webp
│   │   ├── gym.webp
│   │   ├── install.webp
│   │   ├── peru.webp
│   │   ├── title.webp
│   │   ├── title_og_alt.webp
│   │   ├── title_ub.webp
│   │   └── og
│   │       ├── cred0.pcx
│   │       ├── cred1.pcx
│   │       ├── cred2.pcx
│   │       ├── cred3.pcx
│   │       ├── eidospc.pcx
│   │       ├── end.pcx
│   │       ├── install.pcx
│   │       ├── titleh.pcx
│   │       └── titleh_ub.pcx
│   └── injections
│       ├── atlantis_fd.bin
│       ├── atlantis_textures.bin
│       ├── backpac.bin
│       └── etc...
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
│   ├── 2d.glsl
│   ├── 3d.glsl
│   ├── common.glsl
│   ├── fbo.glsl
│   └── sprites.glsl
├── TR1X.exe
</pre>
</details>

## macOS

### Installing

1. Head over to GitHub releases: https://github.com/LostArtefacts/TRX/releases
2. Download the `TR1X-Installer.dmg` installer image. Mount the image and drag TR1X to the Applications folder.
3. Run TR1X from the Applications folder. This will show you an error dialog about missing game data files. This is expected at this point, as you have not copied them in yet. However, it's important to run the app first to allow macOS to verify the app bundle's signature.
4. Find TR1X in your Applications folder. Right-click it and click "Show Package Contents".
5. Copy your Tomb Raider 1 game data files into `Contents/Resources`. (See the Windows / Linux instructions for retrieving game data from e.g. GOG.)

If you install everything correctly, your game directory should look more or
less like this (click to expand):

<details>
<p><em>* Will not be present until the game has been launched.</em></p>
<pre>
.
└── Contents
    ├── _CodeSignature
    ├── Framworks
    ├── info.plist
    ├── MacOS
    └── Resources
        ├── cfg
        │   ├── TR1X.json5 *
        │   ├── TR1X_gameflow.json5
        │   ├── TR1X_gameflow_demo_pc.json5
        │   └── TR1X_gameflow_ub.json5
        ├── data
        │   ├── cat.phd
        │   ├── cut1.phd
        │   ├── cut2.phd
        │   ├── cut3.phd
        │   ├── cut4.phd
        │   ├── egypt.phd
        │   ├── end2.phd
        │   ├── end.phd
        │   ├── gym.phd
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
        │   │── title.phd
        │   │── images
        │   │   ├── atlantis.webp
        │   │   ├── credits_1.webp
        │   │   ├── credits_2.webp
        │   │   ├── credits_3.webp
        │   │   ├── credits_3_alt.webp
        │   │   ├── credits_ps1.webp
        │   │   ├── egypt.webp
        │   │   ├── eidos.webp
        │   │   ├── end.webp
        │   │   ├── greece.webp
        │   │   ├── greece_saturn.webp
        │   │   ├── gym.webp
        │   │   ├── install.webp
        │   │   ├── peru.webp
        │   │   ├── title.webp
        │   │   ├── title_og_alt.webp
        │   │   ├── title_ub.webp
        │   │   └── og
        │   │       ├── cred0.pcx
        │   │       ├── cred1.pcx
        │   │       ├── cred2.pcx
        │   │       ├── cred3.pcx
        │   │       ├── eidospc.pcx
        │   │       ├── end.pcx
        │   │       ├── install.pcx
        │   │       ├── titleh.pcx
        │   │       └── titleh_ub.pcx
        │   └── injections
        │       ├── atlantis_fd.bin
        │       ├── atlantis_textures.bin
        │       ├── backpac.bin
        │       └── etc...
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
        ├── icon.icns
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
        └── shaders
            ├── 2d.glsl
            ├── 3d.glsl
            ├── common.glsl
            ├── fbo.glsl
            └── sprites.glsl
</pre>
</details>

## Dev snapshots

To ease the load on our infrastructure, the binary assets such as images and music files are not included in pre-releases and pull request preview builds - they only ship with the full release builds.
However, you can easily download them manually from these urls:

- https://lostartefacts.dev/aux/tr1x/main.zip (main assets)
- https://lostartefacts.dev/aux/tr1x/music.zip (music files)
- https://lostartefacts.dev/aux/tr1x/trub-music.zip (TR: Unfinished Business expansion pack with fan-patched music)
- https://lostartefacts.dev/aux/tr1x/trub-vanilla.zip (TR: Unfinished Business expansion pack in its original form)
