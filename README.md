# MX Bikes Memory Reader Plugin 2 (MXBMRP2)

A plugin for [MX Bikes](https://mx-bikes.com/) that displays bike, track, server information, and other data directly on the in-game screen.

![mxbmrp2](https://github.com/user-attachments/assets/fad6f978-5035-465e-b6dd-b61eec51aeae)

MXBMRP2 leverages the game's plugin system and memory reading to display a customizable HUD in-game.

## Installation

Download the latest ZIP file from the releases page and extract the contents to your MX Bikes plugins folder, typically located at:`'C:\Program Files (x86)\Steam\steamapps\common\MX Bikes\plugins\`).

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

Optionally edit `mxbmrp2.ini` configuration file to your liking.
  
The file contains customizable fields such as:
| Field               | Example                          |
|---------------------|----------------------------------|
| plugin_banner       | mxbmrp2 v0.9                     |
| rider_name          | Thomas                           |
| category            | MX2 OEM                          |
| bike_id             | MX2OEM_2023_KTM_250_SX-F         |
| bike_name           | KTM 250 SX-F 2023                |
| setup               | Default                          |
| track_id            | 2024_ARLMX_Rd8_Washougal_Pro     |
| track_name          | 2024 ARLMX RD8 WASHOUGAL PRO     |
| track_length        | 2472 m                           |
| connection          | Client                           |
| server_name         | 16 Hate Racing Team | OEM OPEN   |
| event_type          | Race                             |
| session_state       | Race 1                           |
| conditions          | Sunny                            |
| air_temperature     | 23°C                             |

## Notes
### Position of the HUD
Currently, there’s no intuitive way to adjust the HUD position in-game. To reposition it, edit the `position_x` and `position_y` values in and restart the game.

### Custom Data Handling
The game's plugin system lacks certain indicators (e.g., whether you’re in testing, or if you're a host or client). This data is extracted from memory, which usually works well. However, to avoid invalid entries, server names must be at least 4 characters long to be considered valid.

## Credits
 - My previous iteration of [MXBMRP](https://github.com/thomas4f/mxbmrp) (stand-alone Python app) for the memory addresses.
 - [CQ Mono Font](https://www.fontspace.com/cq-mono-font-f23980) Designed by Chequered Ink
 - [MaxHUD](https://forum.mx-bikes.com/index.php?topic=180.0) by HornetMaX for the the conversion.

