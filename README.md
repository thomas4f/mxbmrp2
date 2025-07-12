# MX Bikes Memory Reader Plugin 2 (MXBMRP2)

A plugin for [MX Bikes](https://mx-bikes.com/) that displays event and server information directly on the in-game screen.

![mxbmrp2](https://github.com/user-attachments/assets/fad6f978-5035-465e-b6dd-b61eec51aeae)

MXBMRP2 leverages the game's plugin system and memory reading to display a customizable HUD in-game.

## Features
 - **Customizable HUD**: Select which data fields to display (rider, bike, track, session, etc.) and position them anywhere on the screen.
 - **Time Tracking**: Tracks your actual on-track time, reporting both your per-track/bike time and your overall cumulative time, persisted across sessions.
 - **Discord Rich Presence**: Optional integration to broadcast your MX Bikes activity directly to Discord.
 
_This plugin is in early development and may contain bugs or incomplete features. If you encounter any issues or have suggestions, please report them on the [Issues](https://github.com/thomas4f/mxbmrp2/issues) page. Feature- and/or pull requests are also welcome!_

## Installation

Download the latest plugin package (e.g., `mxbmrp2-<version>.zip`, **not** the source code) from the [releases](https://github.com/thomas4f/mxbmrp2/releases) page and extract the contents to your MX Bikes plugins folder, typically located at: `%ProgramFiles(x86)%\Steam\steamapps\common\MX Bikes\plugins\`).

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
       Enter Sansman Italic.fnt
       discord_game_sdk.dll
       CQ Mono.fnt
```

For additional installation help, see the below video:

https://github.com/user-attachments/assets/5c409833-6a4a-4364-a203-b3dc4c76e45c

## Basic configuration

Optionally, you can customize `mxbmrp2.ini` in your MX Bikes profile directory (`%USERPROFILE%\Documents\Piboso\MX Bikes\`) to suit your preferences.

### Available fields 
Below is a brief description of the available fields within the `Draw configuration` section that you set to `true` to enable:
| Field               | Example                           | Notes                             |
|---------------------|-----------------------------------|-----------------------------------|
| plugin_banner       | mxbmrp2 v0.9.14                   |                                   |
| race_number         |                                   | Prefixed to rider_name            |
| rider_name          | 4 Thomas                          |                                   |
| bike_category       | MX2 OEM                           |                                   |
| bike_id             | MX2OEM_2023_KTM_250_SX-F          |                                   |
| bike_name           | KTM 250 SX-F 2023                 |                                   |
| setup_name          | Default                           | Briefly highlighted in red if "Default" is used. |
| track_id            | HM_swedish_midsummer_carnage_2    |                                   |
| track_name          | HM \| Swedish Midsummer Carnage 2 |                                   |
| track_length        | 965 m                             |                                   |
| connection_type     | Host                              | `Offline` means "Testing", `Client` means you're "Racing" online and `Host` refers to the "Host World" option. | 
| server_name         | thomas4f                          |                                   |
| server_password     | verySecret                        |                                   |
| server_location     | Europe                            | The server location as indicated by the administrator. | 
| server_ping         | 147 ms                            | The response time (ping) when racing online. |
| server_clients      | 12/24                             | The number of players connected to the server. |
| event_type          | Race                              |                                   |
| session_type        | Warmup                            |                                   |
| session_state       | In progress                       |                                   |
| session_duration    | 10:00 +2 laps                     | The elapsed/remaining time and/or number of laps. |
| conditions          | Clear                             |                                   |
| air_temperature     | 20°C                              |                                   |
| combo_time          | 00h 12m                           | Track time on the current bike/track combination. |
| total_time          | 85h 50m                           | Total track time across all combinations. |
| discord_status      | Connected                         | Indicates the status of Discord Rich Presence. |

### Toggle HUD display
Press `CTRL+R` to toggle the HUD on or off. Note that **this will also reload any changes made to the configuration file**.

By default, the HUD is enabled when you start the game. If you prefer it to be disabled by default, set `default_enabled` to `false`.

## Other HUD attributes

### HUD position
Here's how to adjust the HUD position:
 - The HUD position is specified using normalized values between `0.0` and `1.0`.
 - `position_x` controls the horizontal placement (from the left)  and `position_y` controls the vertical placement (from the top).
 - To position the HUD (roughly) on the center of the screen, set `position_x=0.5` and `position_y=0.5`.
 - Setting either value to `1.0` or greater will likely move the HUD outside the viewable area, depending on your screen resolution.

### Font size and colors
Here are some examples on how to configure the other settings:
 - You can adjust the font size by modifying the `font_size` parameter in small increments. Increasing the value from `0.025` (the default) to `0.050` will double the size.
 - `font_color` and `background_color` are specified in **ABGR** (Alpha, Blue, Green Red), meaning that:
   - Red = `0xFF0000FF`,
   - Green =  `0xFF00FF00`, and
   - Blue = `0xFFFF0000`.
 - To make the background fully transparent, set it to `0x00000000`, for fully black, use `0xFF000000`.

### Font family
By default, the plugin uses the `CQ Mono` font to stay consistent with the [MaxHUD](https://forum.mx-bikes.com/index.php?topic=180.0) plugin. 

MX Bikes also makes heavy use of the `Enter Sansman Italic` font, which is included as an alternative.

Note that unlike the other settings, **changing the font family requires a restart of the game**.

To generate additional fonts, you can use the `fontgen` utility provided by PiBoSo. For details, see [this forum post](https://forum.piboso.com/index.php?topic=1458.msg20183#msg20183) and refer to `fontgen.cfg`.

### Time tracking
The plugin tracks your actual on-track time, reporting both your "combo" time (i.e. total time spent on a specific track with a specific bike) and your cumulative total time across all tracks and bikes.

The recorded times can be viewed in `mxbmrp2-times.csv` in your MX Bikes profile directory (to reset the stats, remove `mxbmrp2.dat`).

### Discord Rich Presence
To broadcast your in-game status such as current track, session type, party size, and server name, set `enable_discord_rich_presence=true` in the configuration file.

## Final notes

### Memory reading
The game's plugin system lacks certain fields (e.g., whether you’re in testing, or if you're a host or client, and a few other things). Instead, this data is extracted from memory. This seems to work well, but it has been noted that reading the server_name may fail.

## Credits
 - My previous iteration of [MXBMRP](https://github.com/thomas4f/mxbmrp) (stand-alone Python app) for the memory addresses.
 - [CQ Mono Font](https://www.fontspace.com/cq-mono-font-f23980) designed by Chequered Ink.
 - Enter Sansman Font designed by Digital Graphic Labs.
 - @TokisFFS and everyone who contributed to early testing and feedback.
 - @stars for the excellent [Improved MX Bikes Status in Discord plugin](https://mxb-mods.com/improved-discord-rich-presence-discord-rpc/), which inspired aspects of this plugin's Discord integration. For a lightweight Discord-only solution, be sure to check out their project.
 