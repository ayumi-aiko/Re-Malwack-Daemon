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
const char *daemonLogs = "/sdcard/daemon.logs";
const char *persistDir = "/data/adb/Re-Malwack";
const char *daemonPackageLists = "/data/system/remalwack-package-lists.txt";
const char *daemonLockFileStuck = "/data/system/.daemon0";
const char *daemonLockFileSuccess = "/data/system/.daemon1";
const char *systemHostsPath = "/system/etc/hosts";

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
        else abort_instance("main-katana", "Failed to import the package list. Please try again!");
    }
    else help(argv[0]);
    return 1;
}