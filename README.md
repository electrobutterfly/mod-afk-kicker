# mod-afk-kicker

![banner](banner.jpg)

A module for [AzerothCore](https://www.azerothcore.org/) that automatically kicks players who remain AFK (Away From Keyboard) for longer than a configurable duration.

This is useful for high-population servers where inactive sessions consume player slots, memory, and CPU. By removing long-term AFK players, the module helps keep the realm healthy and frees up capacity for active players.

---

Not running Azerothcore? Check the [port for Trinitycore 3.3.5](https://github.com/electrobutterfly/trinitycore-afk-kicker)

------

## What It Does

While the world server is running, this module periodically checks every connected session:

1. Every `AFK.CheckInterval` milliseconds, it scans all active sessions.
2. For any player flagged as AFK (`/afk` or the AFK status set by the client), it records the timestamp the first time it sees them AFK.
3. If a player remains continuously AFK for at least `AFK.KickTime` milliseconds, they are disconnected with a kick reason such as `AFK for 30 minutes`.
4. As soon as a player stops being AFK (any movement, chat, action, etc.), their AFK timer is cleared.
5. Game Masters (accounts with a security level above `SEC_PLAYER`) are skipped by default, but this behavior can be changed with `AFK.KickGM`.

The module's activity is written to the `server.loading` logger at info level, so you can monitor its behavior in the worldserver console and logs.

---

## Installation

Clone the module into your AzerothCore `modules` directory (from the root of your AzerothCore source tree):

```bash
cd modules
git clone https://github.com/blah.blah mod-afk-kicker

```



Then re-run CMake and rebuild the server:

bash

```
cd ..
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/your/server
make -j$(nproc)
make install
```



After `make install`, the distribution configuration file (`conf/mod_afk_kicker.conf.dist`) is automatically copied into the module configuration directory of your server installation, typically:

text

```
<server-root>/etc/modules/mod_afk_kicker.conf.dist
```



Rename/copy it to `mod_afk_kicker.conf` (dropping the `.dist` suffix) and edit it as needed:

bash

```
cp etc/modules/mod_afk_kicker.conf.dist etc/modules/mod_afk_kicker.conf
```



Restart the worldserver to load the module.

------

## Configuration

All options live in `mod_afk_kicker.conf`.

### `AFK.Enabled`

Enable or disable the AFK kicker module.

| Value | Meaning  |
| :---- | :------- |
| `1`   | Enabled  |
| `0`   | Disabled |

Default: `1`

### `AFK.CheckInterval`

How often (in milliseconds) the module checks all sessions for AFK players. Lower values give more precise kick timing at the cost of slightly more CPU; higher values are lighter but can delay kicks.

Default: `30000` (30 seconds)

### `AFK.KickTime`

How long (in milliseconds) a player may remain continuously AFK before being kicked.

Default: `1800000` (30 minutes)

### `AFK.KickGM`

Whether Game Masters are treated like normal players by the AFK kicker.

| Value | Meaning                       |
| :---- | :---------------------------- |
| `0`   | Do not kick GMs (recommended) |
| `1`   | Kick GMs like normal players  |

Default: `0`

### Example

ini

```
AFK.Enabled       = 1
AFK.CheckInterval = 30000
AFK.KickTime      = 1800000
AFK.KickGM        = 0
```



------

## Logging

The module uses the `server.loading` logger. You can control its output through the standard AzerothCore logger configuration, for example:

ini

```
Logger.modules.afk = 3,Console Server
```



- `3` is the log level (see AzerothCore logging levels).
- `Console Server` means it outputs to both the console and the server log file.

Set the level to `0` or remove the line to silence the module's output.

------

## How the AFK Timer Works

- The timer starts the first time the module observes a player in AFK state.
- The timer is cleared the moment the player is no longer AFK.
- The timer is also cleared when the player is kicked.
- Timers are tracked per player GUID in memory only, so they reset if the worldserver restarts. This is intentional: after a restart, players get a fresh AFK window.

------

## Notes

- Only connected sessions with a valid `Player` object are considered. Sessions still at character selection or in the login process are ignored.
- A GM with `SEC_PLAYER`-level security is treated as a normal player, even if `AFK.KickGM = 0`.
- The kick reason shown to the player is formatted as `AFK for <N> minutes`.

------

## License

This module is licensed under the GNU General Public License v2 (or later). See the header comments in the source files for details.

Copyright (C) 2026 Klaus Simon
