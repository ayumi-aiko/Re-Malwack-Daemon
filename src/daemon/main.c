#include <daemon.h>

// vars:
int blockedMod = 0;
int blockedSys = 0;
bool useStdoutForAllLogs = false;
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
    consoleLog(LOG_LEVEL_INFO, "main-daemon", "Re-Malwack Daemon v%s (versionCode: %s) is starting...", version, versionCode);
    printBannerWithRandomFontStyle();
    if(getuid()) abort_instance("main-daemon", "daemon is not running as root.");
    // force stop termux instance if it's found to be in top. Just to be sure that 
    // termux won't handle the loop.
    // termux can't run some basic commands, that's why im stopping termux users.
    if(getCurrentPackage() != NULL && strcmp(getCurrentPackage(), "com.termux") == 0) {
        consoleLog(LOG_LEVEL_WARN, "main-daemon", "Sorry dear termux user, you CANNOT run this daemon in termux. Termux is not supported by Re-Malwack Daemon.");
        executeShellCommands("exit", (const char*[]) { "exit", "1", NULL });
    }
    consoleLog(LOG_LEVEL_INFO, "main-daemon", "Reading encoded package list...");
    FILE *packageLists = fopen(daemonPackageLists, "rb");
    if(!packageLists) abort_instance("main-daemon", "Failed to open package list file.");
    int i = 0;
    char packageArray[MAX_PACKAGES][PACKAGE_NAME_SIZE];
    char stringsToFetch[PACKAGE_NAME_SIZE];
    while (fgets(stringsToFetch, sizeof(stringsToFetch), packageLists) != NULL && i < MAX_PACKAGES) {
        stringsToFetch[strcspn(stringsToFetch, "\n")] = 0;
        strncpy(packageArray[i], stringsToFetch, PACKAGE_NAME_SIZE - 1);
        packageArray[i][PACKAGE_NAME_SIZE - 1] = '\0';
        i++;
    }
    fclose(packageLists);
    consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "Loaded %d packages into blocklist", i);
    for(int j = 0; j < i; j++) consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "packageArray[%d]: %s", j, packageArray[j]);
    consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "Entering blocklist monitoring loop...");
    FILE *writePID = fopen(currentDaemonPIDFile, "w");
    if(!writePID) abort_instance("main-daemon", "Failed to write current PID for the manager application, please run this daemon again!");
    while(1) {
        fprintf(writePID, "%d", getpid());
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
            for(int j = 0; j < i; j++) consoleLog(LOG_LEVEL_DEBUG, "main-daemon", "packageArray[%d]: %s", j, packageArray[j]);
            remove(daemonLockFileSuccess);
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
    return 0;
}