# MX Bikes Memory Reader Plugin 2 (MXBMRP2)

A plugin for [MX Bikes](https://mx-bikes.com/) that displays bike, track, server information, and other data directly on the in-game screen.

![mxbmrp2](https://github.com/user-attachments/assets/fad6f978-5035-465e-b6dd-b61eec51aeae)

MXBMRP2 leverages the game's plugin system and memory reading to display a customizable HUD in-game. This information can be useful to let people know what bike or server you're on when sharing your screen or streaming, for example.

_This plugin is in early development and may contain bugs or incomplete features. If you encounter any issues or have suggestions, please report them on the [Issues](https://github.com/thomas4f/mxbmrp2/issues) page._

## Installation

Download the latest plugin package (e.g., `mxbmrp2-<verion>.zip`, **not** the source code) from the [releases](https://github.com/thomas4f/mxbmrp2/releases) page and extract the contents to your MX Bikes plugins folder, typically located at:`C:\Program Files (x86)\Steam\steamapps\common\MX Bikes\plugins\`).

Your directory should look like this:

```
MX Bikes\
│   mxbikes.exe
│   mxbikes.ini
│   ...
│
└───plugins\
    │   mxbmrp2.dlo
    │
    └───mxbmrp2_data\
            FontFix_CqMono.fnt
            mxbmrp2.ini
```

Optionally edit the `mxbmrp2.ini` configuration file to your liking.
  
The file contains customizable fields such as:
| Field               | Description  | Example                           |
|---------------------|--------------|-----------------------------------|
| plugin_banner       |              | mxbmrp2 v0.9.1                    |
| rider_name          |              | Thomas                            |
| category            |              | MX2 OEM                           |
| bike_id             |              | MX2OEM_2023_KTM_250_SX-F          |
| bike_name           |              | KTM 250 SX-F 2023                 |
| setup               |              | Default                           |
| track_id            |              | HM_swedish_midsummer_carnage_2    |
| track_name          |              | HM \| Swedish Midsummer Carnage 2 |
| track_length        |              | 965 m                             |
| connection          |              | Host                              |
| server_name         |              | thomas4f                          |
| password            |              | verySecret                        |
| event_type          |              | Race                              |
| session_state       |              | Warmup                            |
| conditions          |              | Clear                             |
| air_temperature     |              | 20°C                              |

## Notes
### Position of the HUD
Currently, there’s no intuitive way to adjust the HUD position or colors in-game. To do this, manually edit the values in the configuration file and restart the game.

### Custom Data Handling
The game's plugin system lacks certain fields (e.g., whether you’re in testing, or if you're a host or client, and a few other things). Instead, this data is extracted from memory. This seems to work well, but let me know if you run into issues. 

## Credits
 - My previous iteration of [MXBMRP](https://github.com/thomas4f/mxbmrp) (stand-alone Python app) for the memory addresses.
 - [CQ Mono Font](https://www.fontspace.com/cq-mono-font-f23980) Designed by Chequered Ink
 - [MaxHUD](https://forum.mx-bikes.com/index.php?topic=180.0) by HornetMaX for the font conversion.
 - @TokisFFS for early testing.