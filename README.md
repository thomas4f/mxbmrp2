# MX Bikes Memory Reader Plugin 2 (MXBMRP2)

A plugin for [MX Bikes](https://mx-bikes.com/) that displays event and server information directly on the in-game screen.

![mxbmrp2](https://github.com/user-attachments/assets/fad6f978-5035-465e-b6dd-b61eec51aeae)

MXBMRP2 leverages the game's plugin system and memory reading to display a customizable HUD in-game. This information can be useful to let people know what bike or server you're on when sharing your screen or streaming, for example.

_This plugin is in early development and may contain bugs or incomplete features. If you encounter any issues or have suggestions, please report them on the [Issues](https://github.com/thomas4f/mxbmrp2/issues) page. Feature- and/or pull requests are also welcome!_

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

For additional installation help, see the below video:

https://github.com/user-attachments/assets/91b8b4b7-c59b-44e3-bcaa-cda1b985e830

## Configuration

Optionally, you can customize the `mxbmrp2.ini` configuration file to suit your preferences.
  
Below is a brief description of the available fields within the `Draw configuration` section that you set to `true` to enable:
| Field               | Example                           |
|---------------------|-----------------------------------|
| plugin_banner       | mxbmrp2 v0.9.3                    |
| race_number         | _(prefixed to rider_name)_        | 
| rider_name          | 4 Thomas                          |
| category            | MX2 OEM                           |
| bike_id             | MX2OEM_2023_KTM_250_SX-F          |
| bike_name           | KTM 250 SX-F 2023                 |
| setup               | Default                           |
| track_id            | HM_swedish_midsummer_carnage_2    |
| track_name          | HM \| Swedish Midsummer Carnage 2 |
| track_length        | 965 m                             |
| connection          | Host                              |
| server_name         | thomas4f                          |
| server_password     | verySecret                        |
| event_type          | Race                              |
| session_state       | Warmup                            |
| conditions          | Clear                             |
| air_temperature     | 20°C                              |

**Note:** There are additional configurable values available. However, apart from adjusting the HUD position, it's recommended to modify these settings only if you are familiar with their functions to avoid unintended display issues.

### Toggle HUD display
Press `CTRL+R` to toggle the HUD on or off. This will also reload any changes made to the configuration file.

### Position of the HUD
You cannot adjust the HUD position or other settings directly within the game interface. However, you can customize its position by editing the configuration file and toggling the HUD on and off using the `CTRL+R` shortcut.

Here's how to adjust the HUD position:
 - The HUD position is specified using normalized values between `0.0` and `1.0`.
 - `position_x` controls the horizontal placement (from the left)  and `position_y` controls the vertical placement (from the top).
 - To position the HUD (roughly) on the center of the screen, set `position_x=0.5` and `position_y=0.5`.
 - Setting either value to `1.0` or greater will likely move the HUD outside the viewable area, depending on your screen resolution.

## Notes

### The Connection Field
 - `Offline` means you're in testing.
 - `Client` means you've connected to someone's server.
 - `Host` means that you are hosting a server.

### Custom Data Handling
The game's plugin system lacks certain fields (e.g., whether you’re in testing, or if you're a host or client, and a few other things). Instead, this data is extracted from memory. This seems to work well, but let me know if you run into issues. 

## Credits
 - My previous iteration of [MXBMRP](https://github.com/thomas4f/mxbmrp) (stand-alone Python app) for the memory addresses.
 - [CQ Mono Font](https://www.fontspace.com/cq-mono-font-f23980) Designed by Chequered Ink
 - [MaxHUD](https://forum.mx-bikes.com/index.php?topic=180.0) by HornetMaX for the font conversion.
 - @TokisFFS for early testing.
