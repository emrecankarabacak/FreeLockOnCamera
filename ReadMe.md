## FreeLockOnCamera
###### Version: 0.1.1

This is a mod for Elden Ring v1.10.

### Description
The camera can be rotated freely during lock-on. Includes some additional changes like how lock-on targets are selected.

See this youtube video for a demonstration:  
https://www.youtube.com/watch?v=-FiB5SYhJls

### Installation
0. Download and install [Elden Mod Loader](https://www.nexusmods.com/eldenring/mods/117) and [Anti-Cheat Toggler](https://www.nexusmods.com/eldenring/mods/90/). Disable anti-cheat if not done already.
1. Download the file `FreeLockOnCamera.zip` from [Releases](https://github.com/SchuhBaum/FreeLockOnCamera/releases/tag/v0.1.1).
2. Extract its content in the folder `[Steam]\SteamApps\common\ELDEN RING\Game\`.
3. Start the game as normal. Make sure that the in-game option `Launch Setting` is set to `Play Offline`.  

### Bug reports
I am still new to Elden Ring modding. If you find bugs or side effects let me know. Please describe step-by-step how to reproduce the issue. You can post them in the [Issues](https://github.com/SchuhBaum/FreeLockOnCamera/issues) tab.

### License  
See the LICENSE-MIT.md file.

### Changelog
v0.1.2:
- Added parameter `lock_camera` to make free cam optional for players who want to keep the lock as is but change other parameters

v0.1.1:
- Fixed a bug where the camera would not zoom out when locking on certain large enemies. You can disable it in the file `config.ini` if you prefer no zoom.

v0.1.0:
- Changed the parameter `is_target_switching_enabled` to `target_switching_mode`. Can be set to `vanilla_switch`, `modded_keep` or `modded_switch` (default). The value vanilla_switch disables changes made to the switching logic. This might have some value if you prefer to not move the camera during lock-on.

v0.0.9:
- Added the parameter `is_target_switching_enabled` to the config.ini file. Set to true by default.

v0.0.8:
- Initial release.
