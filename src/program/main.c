//
// Copyright (C) 2025 愛子あゆみ <ayumi.aiko@outlook.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <daemon.h>
#include <getopt.h>

// vars:
int blockedMod = 0;
int blockedSys = 0;
bool useStdoutForAllLogs = false;
bool shouldForceReMalwackUpdate = false;
char *version = NULL;
char *versionCode = NULL;
const char *configScriptPath = "/data/adb/Re-Malwack/config.sh";
const char *MODPATH = "/data/adb/modules/Re-Malwack";
const char *modulePropFile = "/data/adb/modules/Re-Malwack/module.prop";
const char *hostsPath = "/data/adb/modules/Re-Malwack/system/etc/hosts";
const char *hostsBackupPath = "/data/adb/modules/Re-Malwack/hosts.bak";
const char *daemonLogs = "/sdcard/Android/data/alya.roshidere.lana/logs.yuki.log";
const char *persistDir = "/data/adb/Re-Malwack";
const char *daemonPackageLists = "/data/adb/Re-Malwack/remalwack-package-lists.txt";
const char *daemonLockFileStuck = "/data/adb/Re-Malwack/.daemon0";
const char *daemonLockFileSuccess = "/data/adb/Re-Malwack/.daemon1";
const char *daemonLockFileFailure = "/data/adb/Re-Malwack/.daemon2";
const char *systemHostsPath = "/system/etc/hosts";

// never used struct that much, but here i'm using this 
// "class" kinda thing.
struct option longOptions[] = {
    {"help", no_argument, 0, 'h'},
    {"add-app", required_argument, 0, 'a'},
    {"remove-app", no_argument, 0, 'r'},
    {"import-package-list", no_argument, 0, 'i'},
    {"export-package-list", no_argument, 0, 'e'},
    {"enable-daemon", no_argument, 0, 'x'},
    {"disable-daemon", no_argument, 0, 'd'},
    {"is-yuki", required_argument, 0, 'y'},
    {0,0,0,0}
};

int main(int argc, const char *argv[]) {
    printBannerWithRandomFontStyle();
    if(getuid()) abort_instance("main-roshidere", "This binary should be running as root.");
    int opt;
    int longindex = 0;
    while((opt = getopt_long(argc, argv, "ha:r:i:e:xdy:", longOptions, &longindex)) != -1) {
        switch(opt) {
            default:
            case 'h':
                help(argv[0]);
            break;
            case 'a':
                if(isPackageInList(optarg)) consoleLog(LOG_LEVEL_INFO, "main-roshidere", "Existing package can't be added once again!");
                else addPackageToList(optarg);
                return 0;
            break;
            case 'r':
                if(isPackageInList(optarg)) {
                    removePackageFromList(optarg);
                    finishMessage("Requested package has been removed successfully!");
                }
                else abort_instance("main-roshidere", "%s is not found in the list.", optarg);
            break;
            case 'i':
                eraseFile(daemonLockFileStuck);
                if(executeShellCommands("cp", (const char*[]) { "cp", "-af", optarg, daemonPackageLists, NULL })) {
                    eraseFile(daemonLockFileSuccess);
                    remove(daemonLockFileStuck);
                    finishMessage("main-roshidere", "Successfully imported the package list. Thank you for using Re-Malwack!");
                }
                else {
                    abort_instance("main-roshidere", "Failed to import the package list. Please try again!");
                    eraseFile(daemonLockFileFailure);
                }
            break;
            case 'e':
                if(access(daemonPackageLists, F_OK) != 0) abort_instance("main-roshidere", "Failed to access the package lists file. It might be corrupted or missing.");
                if(executeShellCommands("cp", (const char*[]) { "cp", "-af", daemonPackageLists, optarg, NULL })) finishMessage("Successfully copied the file to the requested location. Thank you for using Re-Malwack!");
                else abort_instance("main-roshidere", "Failed to copy the file to the requested location. Please try again!");
            break;
            case 'x':
                if(putConfig("enable_daemon", ENABLE_ENABLED) != 0) abort_instance("main-roshidere", "Failed to enable the daemon, please try again!");
                finishMessage("main-roshidere", "Successfully enabled the daemon, enjoy!");
            break;
            case 'd':
                if(putConfig("enable_daemon", DISABLE_DISABLED) != 0) abort_instance("main-roshidere", "Failed to disable the daemon, please try again!");
                finishMessage("main-roshidere", "Successfully enabled the daemon, enjoy!");
            break;
            case 'y':
                if(strcmp(optarg, "-r") == 0 || strcmp(optarg, "-R") == 0) {
                    if(strcmp(grepProp("is_daemon_running", configScriptPath), "0") != 0) return 1;
                    return 0;
                }
                else if(strstr(optarg, "e") == 0 || strstr(optarg, "E") == 0) {
                    if(strcmp(grepProp("enable_daemon", configScriptPath), "0") != 0) return 1;
                    return 0;
                }
            break;
        }
    }
    return 1;
}