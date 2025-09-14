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

// vars:
int blockedMod = 0;
int blockedSys = 0;
bool useStdoutForAllLogs = true;
bool suppressEverythingExceptInfo = false;
char *configScriptPath = NULL;
char *MODPATH = NULL;
char *modulePropFile = NULL;
char *version = NULL;
char *versionCode = NULL;
char *hostsPath = NULL;
char *hostsBackupPath = NULL;
const char *daemonLogs = "/sdcard/Android/data/ishimi.katamari/logs.katamari.log";
const char *persistDir = "/data/adb/Re-Malwack";
const char *daemonPackageLists = "/data/adb/Re-Malwack/remalwack-package-lists.txt";
const char *daemonLockFileStuck = "/data/adb/Re-Malwack/.daemon0";
const char *daemonLockFileSuccess = "/data/adb/Re-Malwack/.daemon1";
const char *daemonLockFileFailure = "/data/adb/Re-Malwack/.daemon2";
const char *systemHostsPath = "/system/etc/hosts";
const char *currentDaemonPIDFile = "/data/adb/Shizuka/currentDaemonPID";

int main(int argc, const char *argv[]) {    
    printBannerWithRandomFontStyle();
    if(getuid()) abort_instance("main-katana", "- This binary should be running as root.");

    // handle args like a boss
    if(argc >= 3 && strcmp(argv[1], "--add-app") == 0) {
        if(isPackageInList(argv[2])) finishMessage("Existing package can't be added once again!");
        addPackageToList(argv[2]);
        return 0;
    }
    else if(argc >= 3 && strcmp(argv[1], "--remove-app") == 0) {
        if(isPackageInList(argv[2])) {
            removePackageFromList(argv[2]);
            finishMessage("Requested package has been removed successfully!");
        }
        else abort_instance("main-katana", "%s is not found in the list.", argv[2]);
    }
    else if(argc >= 2 && strcmp(argv[1], "--export-package-list") == 0) {
        if(!argv[2]) abort_instance("main-katana", "Export path can't be nothing, please put a valid path and try again!");
        if(access(daemonPackageLists, F_OK) != 0) abort_instance("main-katana", "Failed to access the package lists file. It might be corrupted or missing.");
        if(executeShellCommands("cp", (const char*[]) { "cp", "-af", daemonPackageLists, argv[2], NULL })) finishMessage("Successfully copied the file to the requested location. Thank you for using Re-Malwack!");
        else abort_instance("main-katana", "Failed to copy the file to the requested location. Please try again!");
    }
    else if(argc >= 2 && strcmp(argv[1], "--import-package-list") == 0) {
        eraseFile(daemonLockFileStuck);
        if(!argv[2]) abort_instance("main-katana", "Import path can't be nothing, please put a valid path and try again!");
        if(executeShellCommands("cp", (const char*[]) { "cp", "-af", argv[2], daemonPackageLists, NULL })) {
            eraseFile(daemonLockFileSuccess);
            remove(daemonLockFileStuck);
            finishMessage("main-katana", "Successfully imported the package list. Thank you for using Re-Malwack!");
        }
        else {
            abort_instance("main-katana", "Failed to import the package list. Please try again!");
            eraseFile(daemonLockFileFailure);
        }
    }
    else help(argv[0]);
    return 1;
}