# MX Bikes Memory Reader Plugin 2 (MXBMRP2)

A plugin for [MX Bikes](https://mx-bikes.com/).

![mxbmrp2](https://github.com/user-attachments/assets/fad6f978-5035-465e-b6dd-b61eec51aeae)

> **Stay on track** - MXBMRP2 aims to keep live session, server, and rider data on-screen, so you never have to pit or alt-tab for crucial info.

## Features
 - **Customizable HUD**: Select which data fields to display (rider, bike, track, session, etc.) and position them anywhere on the screen.
 - **Time Tracking**: Tracks your actual on-track time, reporting both your per-track/bike time and your overall cumulative time, persisted across sessions.
 - **Discord Rich Presence**: Optional integration to broadcast your MX Bikes activity directly to Discord.
 - **OBS Studio Integration**: Optionally export an HTML overlay that you can add as a Browser Source in OBS Studio for fully customizable display.
 
_If you encounter any issues or have suggestions, please report them on the [Issues](https://github.com/thomas4f/mxbmrp2/issues) page. Feature- and/or pull requests are also welcome!_

## Installation

### Using the Installer
Download and run the latest setup (e.g., `mxbmrp2-<version>-Setup.exe`) from the [releases](https://github.com/thomas4f/mxbmrp2/releases) page.

### Manually
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
       CQ Mono.fnt
       discord_game_sdk.dll
```

Note: This plugin requires the Microsoft Visual C++ Redistributable ("vc_redist"). On most systems, it’s already installed, but if you don’t have it, install the latest version from [Microsoft](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version).

For additional installation help, see the below video:

https://github.com/user-attachments/assets/f664fd29-057e-4a7e-b82b-f8635bfa4caf

## Basic configuration

Optionally, you can customize `mxbmrp2.ini` in your MX Bikes profile directory (`%USERPROFILE%\Documents\Piboso\MX Bikes\mxbmrp2\`) to suit your preferences.

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
 | remaining_tearoffs | 21                                |                                   |
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
| enable_html_export  |                                   | Creates `mxbmrp2.html` that can be included in OBS Studio. |

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

To generate additional fonts, you can use the `fontgen` utility provided by PiBoSo. For details, see [this forum post](https://forum.piboso.com/index.php?topic=1458.msg20183#msg20183) and refer to `fontgen.cfg`.

### Time tracking
The plugin tracks your actual on-track time, reporting both your "combo" time (i.e. total time spent on a specific track with a specific bike) and your cumulative total time across all tracks and bikes.

The recorded times can be viewed in `mxbmrp2-times.csv` in your MX Bikes profile directory (to reset the stats, remove `mxbmrp2.dat`).

### Discord Rich Presence
To broadcast your in-game status such as current track, session type, party size, and server name, set `enable_discord_rich_presence=true` in the configuration file.

### OBS Studio Integration
To add the overlay in OBS, perform the following:
 1. In `mxbmrp2.ini`, set `enable_html_export` to `true`. This will create and update `mxbmrp2.html` in your MX Bikes profile directory.
 2. Restart MX Bikes (or press CTRL+R) to reload the plugin with HTML export enabled.
 3. In OBS, add a `Browser` source, tick `Local file` and set the path to the file (e.g. `%USERPROFILE%\Documents\Piboso\MX Bikes\mxbmrp2\mxbmrp2.html`).
 4. Tweak the layout by editing the `Custom CSS` in OBS, or, preferably, create `mxbmrp2.css` in the same directory as the .html file. See examples below.
 5. Adjust the width, height, scaling and position of the overlay as necessary.
 
 _The contents will refresh automatically, and display the fields enabled in the configuration file. Also consider setting `default_enabled` to `false` to avoid having multiple overlays!_
 
#### Example CSS
Here's a few examples of how to customize the HTML layout:

**PiBoSo-like**:

<img width="1769" height="995" alt="Image" src="https://github.com/user-attachments/assets/a759b91c-5079-4680-bb07-cc2bed29d55f" />

```css
/* MXBMRP2 CSS Example: PiBoSo-like */

@import url('https://fonts.cdnfonts.com/css/enter-sansman');

html, body {
  margin: 0;
  font: italic 0.9em 'Enter Sansman';
  font-weight: 800;
}

.data {
  background: rgba(30, 30, 30, .8);
  color: rgb(240, 240, 240);
  display: grid;
  grid-template-columns: max-content 1fr;
  padding: 0.5em;
}

.data__item {
  display: contents;
}

.data__label {
  padding-right: .25em;
}

.data__value {
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
}

.data__value.plugin_banner {
  font-size: 2em;
  grid-column: 1 / -1;
  color: #ffb74d;
}
```

**Minimal with icons**:

<img width="1769" height="995" alt="Image" src="https://github.com/user-attachments/assets/78ba0b35-d922-4f46-a400-b2fd3783a0a1" />

```css
/* MXBMRP2 CSS Example: Minimal with icons */

@import url('https://fonts.cdnfonts.com/css/enter-sansman');
@import url('https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.7.2/css/all.min.css');

.data {
  font: italic 1rem 'Enter Sansman';
  color: #fff;
  display: grid;
  grid-template-columns: max-content 1fr;
  align-items: center;
}

.data__item {
  display: contents;
}

.data__label, .data__value {
  padding: 0.25rem;
} 

.data__label, .plugin_banner, .no_data {
  font-size: 0;
}

.data__label::before {
  font-family: "Font Awesome 6 Free";
  font-weight: 600;
  font-style: normal;
  font-size: 1.2rem;}

/* Hide everything except ... */
.data__item:not(
  .track_name,
  .session_duration,
  .server_name,
  .server_password,
  .server_clients) {
  display: none;
}

/* And add some modifiers those */
.data__label.track_name::before        { content: "\f11e"; }
.data__label.session_duration::before  { content: "\f2f2"; }
.data__label.server_name::before       { content: "\f233"; }
.data__label.server_password::before   { content: "\f084"; }
.data__label.server_clients::before    { content: "\f500"; }
```

**Table-layout**:

<img width="1769" height="995" alt="Image" src="https://github.com/user-attachments/assets/0d383da0-c554-4d03-9704-9304884b8e8a" />

```css
/* MXBMRP2 CSS Example: Table-layout */

html, body {
  margin: 0;
  font: clamp(1.5rem, 2.5vw, 4rem) sans-serif;
}

.data {
  background: rgba(30, 30, 30, .95);
  color: #fff;
  border-radius: 0.5rem;
  display: grid;
  grid-template-columns: max-content 1fr;
}

.data__item {
  display: contents;
}

.data__label, .data__value {
  border-bottom:1px solid #444;
  padding: 0.5rem;
} 
 
.data__item:last-child > * {
  border-bottom: none;
}

.data__label {
  font-weight: 700;
  padding-right: .25em;
}

.data__value {
  overflow: hidden;
  white-space: nowrap;
  text-overflow: ellipsis;
}

.data__value.plugin_banner {
  grid-column: 1 / -1;
  color: #ffb74d;
  font-weight: 700;
}
```
If you'd like build a HTML/CSS from scratch or do something else with the data, you can set `enable_json_export=true` and grab it from there.

## Final notes

### Memory reading
The game's plugin system lacks certain fields (e.g., whether you’re in testing, or if you're a host or client, and a few other things). Instead, this data is extracted from memory. This seems to work well, but it has been noted that reading the server_name may fail.

## Licensing and Third-Party Software
This project is licensed under the [MIT License](LICENSE.txt). However, the included **Discord Game SDK is **not** covered by the MIT license. It is provided under Discord's proprietary terms and is redistributed here solely as permitted by Discord's [Developer Terms of Service](https://dis.gd/discord-developer-terms-of-service).

## Credits
 - My previous iteration of [MXBMRP](https://github.com/thomas4f/mxbmrp) (stand-alone Python app) for the memory addresses.
 - [CQ Mono Font](https://www.fontspace.com/cq-mono-font-f23980) designed by Chequered Ink.
 - @TokisFFS and everyone who contributed to early testing and feedback.
 - @stars for the excellent [Improved MX Bikes Status in Discord plugin](https://mxb-mods.com/improved-discord-rich-presence-discord-rpc/), which inspired aspects of this plugin's Discord integration. For a lightweight Discord-only solution, be sure to check out their project.
 