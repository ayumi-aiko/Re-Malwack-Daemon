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
const char *daemonLogs = "/sdcard/Android/data/ishimi.katamari/logs.katamari.log";
const char *persistDir = "/data/adb/Re-Malwack";
const char *daemonPackageLists = "/data/adb/Re-Malwack/remalwack-package-lists.txt";
const char *daemonLockFileStuck = "/data/adb/Re-Malwack/.daemon0";
const char *daemonLockFileSuccess = "/data/adb/Re-Malwack/.daemon1";
const char *daemonLockFileFailure = "/data/adb/Re-Malwack/.daemon2";
const char *systemHostsPath = "/system/etc/hosts";
const char *currentDaemonPIDFile = "/data/adb/Shizuka/currentDaemonPID";

// never used struct that much, but here i'm using this 
// "class" kinda thing.
struct option longOptions[] = {
    {"help", no_argument, 0, 'h'},
    {"add-app", required_argument, 0, 'a'},
    {"remove-app", no_argument, 0, 'rm'},
    {"import-package-list", no_argument, 0, 'im'},
    {"export-package-list", no_argument, 0, 'ex'},
    {0,0,0,0}
};

int main(int argc, char *argv[]) {
    printBannerWithRandomFontStyle();
    if(getuid()) abort_instance("main-katana", "This binary should be running as root.");
    int opt;
    int longindex = 0;
    while((opt = getopt_long(argc, argv, "ha:rm:im:ex:", longOptions, &longindex)) != -1) {
        switch(opt) {
            default:
            case 'h':
                help(argv[0]);
            break;
            case 'a':
                if(isPackageInList(argv[optind])) finishMessage("Existing package can't be added once again!");
                addPackageToList(argv[optind]);
                return 0;
            break;
            case 'rm':
                if(isPackageInList(argv[optind])) {
                    removePackageFromList(argv[optind]);
                    finishMessage("Requested package has been removed successfully!");
                }
                else abort_instance("main-katana", "%s is not found in the list.", argv[optind]);
            break;
            case 'im':
                eraseFile(daemonLockFileStuck);
                if(executeShellCommands("cp", (const char*[]) { "cp", "-af", argv[optind], daemonPackageLists, NULL })) {
                    eraseFile(daemonLockFileSuccess);
                    remove(daemonLockFileStuck);
                    finishMessage("main-katana", "Successfully imported the package list. Thank you for using Re-Malwack!");
                }
                else {
                    abort_instance("main-katana", "Failed to import the package list. Please try again!");
                    eraseFile(daemonLockFileFailure);
                }
            break;
            case 'ex':
                if(access(daemonPackageLists, F_OK) != 0) abort_instance("main-katana", "Failed to access the package lists file. It might be corrupted or missing.");
                if(executeShellCommands("cp", (const char*[]) { "cp", "-af", daemonPackageLists, argv[optind], NULL })) finishMessage("Successfully copied the file to the requested location. Thank you for using Re-Malwack!");
                else abort_instance("main-katana", "Failed to copy the file to the requested location. Please try again!");
            break;
        }
    }
    return 1;
}