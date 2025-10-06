![rmwlk](https://github.com/ZG089/Re-Malwack/raw/main/assets/Re-Malwack.png)
# Re-Malwack Daemon
An unofficial daemon program written in C for app-specific adblocking feature.
- This CLI version will never get deprecated because the android app depends on the CLI code.

## To build:
- Edit the `CC_ROOT` with your path to the ROOT of the android-ndk if you are building it for the first time.
```
make all SDK=28 ARCH=arm64
```

### Argument requirements:
- `SDK` should be the minimum supported version.
- `ARCH` should be `arm64`, `arm`, `x86` and `x86-64`.

### Build requirements:
- Android NDK, atleast `r27d` is required
- WSL or a Standard GNU/Linux with basic bash tools and make

# katana
- Katana is a program for managing the daemon in CLI.
- Below are argument usage examples:
```
katana --add-app <app package name, ex: com.android.conan> | Adds the app from the blocklist

katana --remove-app <app package name, ex: com.android.conan> | Removes the app from the blocklist

katana --export-package-list <path, ex: /sdcard/export-katana.txt> | For exporting the package lists

katana --import-package-list <path, ex: /sdcard/export-katana.txt> | For importing the package lists
```

## daemon
- This daemon should only **RUN** in init or any prefered mode but not termux because termux is not suitable forever.
- This daemon doesn't have any arguments handler because this app is only going to run in background.