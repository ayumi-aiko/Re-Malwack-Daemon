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
bool useStdoutForAllLogs = false;
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
const char *previousDaemonPackageLists = "/data/adb/Re-Malwack/previousDaemonList";
const char *daemonLockFileStuck = "/data/adb/Re-Malwack/.daemon0";
const char *daemonLockFileSuccess = "/data/adb/Re-Malwack/.daemon1";
const char *daemonLockFileFailure = "/data/adb/Re-Malwack/.daemon2";
const char *systemHostsPath = "/system/etc/hosts";
const char *currentDaemonPIDFile = "/data/adb/Shizuka/currentDaemonPID";

int main(int argc, const char *argv[]) {
    // module path
    MODPATH = strdup(modulePath(argv[0]));
    if(!MODPATH) abort_instance("main-daemon", "Failed to retrieve module path");

    // module prop file:
    modulePropFile = combineStringsFormatted("%s/module.prop", MODPATH);
    if(!modulePropFile) abort_instance("main-daemon", "Failed to retrieve module.prop path");

    // version:
    version = grepProp("version", modulePropFile);
    if(!version) abort_instance("main-daemon", "Could not find 'version' in module.prop");

    // version:
    versionCode = grepProp("versionCode", modulePropFile);
    if(!versionCode) abort_instance("main-daemon", "Could not find 'versionCode' in module.prop");

    // hosts temp backup file
    hostsBackupPath = combineStringsFormatted("%s/hosts.bak", MODPATH);
    if(!hostsBackupPath) abort_instance("main-daemon", "Failed to retrieve hosts backup file path");

    // hosts file
    hostsPath = combineStringsFormatted("%s/system/etc/hosts", MODPATH);
    if(!hostsPath) abort_instance("main-daemon", "Failed to retrieve hosts file path");

    // config file
    configScriptPath = combineStringsFormatted("%s/scripts/config.sh", MODPATH);
    if(!configScriptPath) abort_instance("main-daemon", "Failed to retrieve config script path");

    // loop daemon action.
    consoleLog(LOG_LEVEL_INFO, "main-daemon", "Re-Malwack v%s (versionCode: %s) is starting...", version, versionCode);
    printBannerWithRandomFontStyle();
    if(getuid()) abort_instance("main-daemon", "daemon is not running as root.");
    // force stop termux instance if it's found to be in top. Just to be sure that 
    // termux won't handle the loop.
    // termux can't run some basic commands, that's why im stopping termux users.
    if(getCurrentPackage() != NULL && strcmp(getCurrentPackage(), "com.termux") == 0) {
        consoleLog(LOG_LEVEL_WARN, "main-daemon", "Sorry dear termux user, you CANNOT run this daemon in termux. Termux is not supported by Re-Malwack Daemon.");
        executeShellCommands("exit", (const char*[]) { "exit", "1", NULL });
    }
    // always have a backup of the daemonPackageLists because we need to have to use this
    // backup as a failsafe method when the crp didn't import properly.
    if(executeShellCommands("su", (const char*[]) {"su", "-c", "cp", "-af", daemonPackageLists, "/data/adb/Re-Malwack/previousDaemonList", NULL}) != 0) {
        abort_instance("main-daemon", "Failed to backup the daemon package lists, please try again!");
    }
    consoleLog(LOG_LEVEL_INFO, "main-daemon", "Reading encoded package list...");
    FILE *packageLists = fopen(daemonPackageLists, "rb");
    if(!packageLists) abort_instance("main-daemon", "Failed to open package list file.");
    int i = 0;
    char packageArray[MAX_PACKAGES][PACKAGE_NAME_SIZE];
    char stringsToFetch[PACKAGE_NAME_SIZE];
    while(fgets(stringsToFetch, sizeof(stringsToFetch), packageLists) != NULL && i < MAX_PACKAGES) {
        stringsToFetch[strcspn(stringsToFetch, "\n")] = 0;
        strncpy(packageArray[i], stringsToFetch, PACKAGE_NAME_SIZE - 1);
        packageArray[i][PACKAGE_NAME_SIZE - 1] = '\0';
        i++;
    }
    fclose(packageLists);
    consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "Loaded %d packages into blocklist", i);
    consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "Entering blocklist monitoring loop...");
    while(1) {
        // write the pid so we can kill it later inside the app.
        writeCurrentProcessID();
        if(strcmp(grepProp("enableDaemon", configScriptPath), "1") == 0) {
            if(access(daemonLockFileStuck, F_OK) == 0) {
                consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "Waiting for user configurations to finish...");
                usleep(500000);
                continue;
            }
            if(access(daemonLockFileSuccess, F_OK) == 0) {
                consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "A package list update was triggered. Reloading packages...");
                FILE *packageLists = fopen(daemonPackageLists, "r");
                if(!packageLists) abort_instance("main-daemon", "Failed to reopen package list file.");
                i = 0;
                while(fgets(stringsToFetch, sizeof(stringsToFetch), packageLists) != NULL && i < MAX_PACKAGES) {
                    stringsToFetch[strcspn(stringsToFetch, "\n")] = 0; // strip newline
                    strncpy(packageArray[i], stringsToFetch, PACKAGE_NAME_SIZE - 1);
                    packageArray[i][PACKAGE_NAME_SIZE - 1] = '\0';
                    i++;
                }
                fclose(packageLists);
                consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "Reloaded %d packages into blocklist", i);
                remove(daemonLockFileSuccess);
            }
            else if(access(daemonLockFileFailure, F_OK) == 0) {
                consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "An reset was triggered. Reverting to previous state...");
                if(executeShellCommands("su", (const char*[]) {"su", "-c", "cp", "-af", previousDaemonPackageLists, daemonPackageLists, NULL}) == 0) {
                    abort_instance("main-daemon", "Failed to restore the package list, please try again!");
                }
                else {
                    remove(daemonLockFileFailure);
                    consoleLog(LOG_LEVEL_INFO, "main-daemon", "Reset finished successfully! Skipping this loop and building list again...");
                    // skip this after doing this because i dont want to code too much!
                    continue;
                }
            }
            char *currentPackage = getCurrentPackage();
            if(currentPackage == NULL) {
                consoleLog(LOG_LEVEL_WARN, "main-daemon", "Failed to get current package, retrying...");
                usleep(500000);
                continue;
            }
            for(int k = 0; k < i; k++) {
                if(strcmp(currentPackage, packageArray[k]) == 0) {
                    consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "%s is currently running. Handling blocklist actions.", packageArray[k]);
                    pauseADBlock();
                }
                // call ts, dw as the function will ignore if it's resumed already.
                else resumeADBlock();
            }
            // hmm, let's not fry the cpu.
            usleep(500000);
        }
        else {
            // kill ourselves!
            freePointer((void **)&MODPATH);
            freePointer((void **)&hostsBackupPath);
            freePointer((void **)&hostsPath);
            freePointer((void **)&configScriptPath);
            exit(EXIT_SUCCESS);
        }
    }
    freePointer((void **)&MODPATH);
    freePointer((void **)&hostsBackupPath);
    freePointer((void **)&hostsPath);
    freePointer((void **)&configScriptPath);
    return 0;
}