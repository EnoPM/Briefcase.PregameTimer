# Briefcase Pregame Timer

Pregame Timer changes the lobby countdown on a Windows x64 Deceive Inc. dedicated server.

## Install

1. Install the [latest BriefcaseNative Windows server release](https://github.com/EnoPM/BriefcaseNative/releases/latest) and stop the dedicated server.
2. Download `Briefcase.PregameTimer-windows-x64-<version>.zip` from this mod's latest release.
3. Extract the ZIP directly into the server's `DeceiveInc/Binaries/Win64` directory, beside `DeceiveIncServer-Win64-Shipping.exe`.
4. Open `ue4ss/Mods/mods.txt` and add this line if it is not already present:

```text
BriefcasePregameTimer : 1
```

5. Start `DeceiveIncServer-Win64-Shipping.exe` with Win64 as its working directory. The Briefcase `version.dll` loads this mod through UE4SS.

When upgrading from an older Briefcase mod package, copy your desired settings and remove the old `Briefcase/Mods/briefcase.pregame-timer` folder before starting the server, so both versions do not run together. Keep your existing `Data/config.json` when replacing this UE4SS mod.

## Configure

Edit `ue4ss/Mods/BriefcasePregameTimer/Data/config.json` while the server is stopped:

```json
{
  "durationSeconds": 30,
  "diagnostics": false
}
```

`durationSeconds` accepts 1–3600 seconds. Leave `diagnostics` at `false` for normal play. Restart the server after editing the file.

## Remove

Stop the server, remove `ue4ss/Mods/BriefcasePregameTimer`, and remove its line from `ue4ss/Mods/mods.txt`. Restart the server.

## License

This mod is licensed under the [MIT License](LICENSE). Third-party components retain their own licenses.
