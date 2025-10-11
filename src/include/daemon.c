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

int putConfig(const char *variableName, int variableValue) {
    FILE *fp = fopen(configScriptPath, "r");
    if(!fp) {
        consoleLog(LOG_LEVEL_ERROR, "putConfig", "Failed to open %s in read-only mode, please try again..", configScriptPath);
        return 127;
    }
    int lineCount = 0;
    bool didItGetChanged = false;
    char lines[64][256];
    while(fgets(lines[lineCount], sizeof(lines[lineCount]), fp)) lineCount++;
    fclose(fp);
    FILE *fpt = fopen(configScriptPath, "w");
    if(!fpt) {
        consoleLog(LOG_LEVEL_ERROR, "putConfig", "Failed to open %s in write-only mode, please try again..", configScriptPath);
        return 127;
    }
    for(int i = 0; i < lineCount; i++) {
        if(strncmp(lines[i], variableName, strlen(variableName)) == 0) {
            fprintf(fpt, "%s=%d\n", variableName, variableValue);
            didItGetChanged = true;
        }
        else fprintf(fpt, "%s", lines[i]);
    }
    fclose(fpt);
    return (didItGetChanged) ? 0 : 1;
}

bool isPackageInList(const char *packageName) {
    FILE *packageFile = fopen(daemonPackageLists, "r");
    if(!packageFile) abort_instance("isPackageInList", "Failed to open the package lists file, please run this command again or report this issue to the devs.");
    char contentFromFile[1028];
    while(fgets(contentFromFile, sizeof(contentFromFile), packageFile)) {
        contentFromFile[strcspn(contentFromFile, "\n")] = 0;
        if(strcmp(contentFromFile, packageName) == 0) {
            fclose(packageFile);
            return true;
        }
    }
    fclose(packageFile);
    return false;
}

bool addPackageToList(const char *packageName) {
    FILE *packageFile = fopen(daemonPackageLists, "a");
    if(!packageFile) abort_instance("addPackageToList", "Failed to open the package lists file, please run this command again or report this issue to the devs.");
    fprintf(packageFile, "\n%s\n", packageName);
    consoleLog(LOG_LEVEL_DEBUG, "addPackageToList", "Successfully added %s into the list, the daemon will add the packages to the list after a short period of time.", packageName);
    fclose(packageFile);
    return true;
}

bool removePackageFromList(const char *packageName) {
    FILE *packageFile = fopen(daemonPackageLists, "r");
    if(!packageFile) abort_instance("removePackageFromList", "Failed to open the package lists file, please run this command again or report this issue to the devs.");
    eraseFile("/data/local/tmp/temp");
    // no need to put ts in w mode to erase previous stuff as the eraseFile itself opens the file in W to remove the previous contents.
    FILE *tempFile = fopen("/data/local/tmp/temp", "a");
    if(!tempFile) abort_instance("removePackageFromList", "Failed to open a temporary file, please run this command again or report this issue to the devs.");
    char contentFromFile[1028];
    bool status = false;
    while(fgets(contentFromFile, sizeof(contentFromFile), packageFile) != NULL) {
        eraseFile(daemonLockFileStuck);
        contentFromFile[strcspn(contentFromFile, "\n")] = 0;
        if(strcmp(contentFromFile, packageName) == 0) {
            consoleLog(LOG_LEVEL_DEBUG, "removePackageFromList", "Found %s on the list, removing the package from the list...", packageName);
            // skip writing to the temp file so we can write other strings aka the packages.
            // use a bool to indicate that we skipped copying it to the temp file.
            status = true;
        }
        else fprintf(tempFile, "%s\n", contentFromFile);
    }
    fclose(tempFile);
    fclose(packageFile);
    if(status) {
        rename("/data/local/tmp/temp", daemonPackageLists);
        eraseFile(daemonLockFileSuccess);
    }
    remove(daemonLockFileStuck);
    return status;
}

bool eraseFile(const char *__file) {
    FILE *fileConstantAgain = fopen(__file, "w");
    if(!fileConstantAgain) return false;
    fclose(fileConstantAgain);
    return true;
}

bool executeShellCommands(const char *command, const char *args[]) {
    pid_t ProcessID = fork();
    switch(ProcessID) {
        case -1:
            abort_instance("executeShellCommands", "Failed to fork to continue.");
        break;
        case 0:
            // throw output to /dev/null
            int devNull = open("/dev/null", O_WRONLY);
            if(devNull == -1) exit(EXIT_FAILURE);
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
            execvp(command, (char *const *)args);
            consoleLog(LOG_LEVEL_ERROR, "executeShellCommands", "Failed to execute command");
        break;
        default:
            consoleLog(LOG_LEVEL_DEBUG, "executeShellCommands", "Waiting for current %d to finish", ProcessID);
            int status;
            wait(&status);
            return (WIFEXITED(status)) ? WEXITSTATUS(status) : false;
    }
    // me: sybau compiler
    // evil gurt: yo
    // me: sybau
    return false;
}

bool isDefaultHosts(const char *filename) {
    FILE *file = fopen(filename, "r");
    if(!file) return false;
    char line[1024];
    while(fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        if(strcmp(line, "127.0.0.1 localhost") != 0 && strcmp(line, "::1 localhost") != 0) {
            fclose(file);
            return true;
        }
    }
    fclose(file);
    return false;
}

bool canDaemonRun(void) {
    char *enableDaemon = grepProp("enable_daemon", configScriptPath);
    char *isDaemonRunning = grepProp("is_daemon_running", configScriptPath);
    if(enableDaemon && isDaemonRunning && strcmp(enableDaemon, "0") + strcmp(isDaemonRunning, "1") == 0) return true;
    return false;
}

char *grepProp(const char *variableName, const char *propFile) {
    FILE *filePointer = fopen(propFile, "r");
    if(!filePointer) {
        consoleLog(LOG_LEVEL_ERROR, "grepProp", "Failed to open properties file: %s", propFile);
        return NULL;
    }
    char theLine[1024];
    size_t lengthOfTheString = strlen(variableName);
    while(fgets(theLine, sizeof(theLine), filePointer)) {
        if(strncmp(theLine, variableName, lengthOfTheString) == 0 && theLine[lengthOfTheString] == '=') {
            strtok(theLine, "=");
            char *value = strtok(NULL, "\n");
            if(value) {
                char *result = strdup(value);
                fclose(filePointer);
                return result;
            }
        }
    }
    fclose(filePointer);
    return NULL;
}

char *getCurrentPackage() {
    static char packageName[100];
    FILE *fptr = popen("timeout 1 dumpsys 2>/dev/null | grep mFocused | awk '{print $3}' | head -n 1 | awk -F'/' '{print $1}'", "r");
    if(!fptr) abort_instance("getCurrentPackage", "Failed to fetch shell output. Are you running on Android shell?");
    while(fgets(packageName, sizeof(packageName), fptr) != NULL) {
        packageName[strcspn(packageName, "\n")] = 0;
        pclose(fptr);
        return packageName;
    }
    pclose(fptr);
    return NULL;
}

char *combineStringsFormatted(const char *format, ...) {
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if(len < 0) {
        va_end(args);
        return NULL;
    }
    char *result = malloc(len + 1);
    if(!result) {
        va_end(args);
        return NULL;
    }
    vsnprintf(result, len + 1, format, args);
    va_end(args);
    if(!result) {
        va_end(args);
        return NULL;
    }
    return result;
}

char *modulePath(const char *Zero) {
    char realpath_buffer[PATH_MAX];
    if(!realpath(Zero, realpath_buffer)) abort_instance("modulePath", "Failed to get module path via argument, please try again.");
    char *tmp = strdup(realpath_buffer);
    if(!tmp) abort_instance("modulePath", "Failed to get module path via argument, please try again.");
    char *dir = dirname(tmp);
    char *result = strdup(dir);
    free(tmp);
    if(!result) abort_instance("modulePath", "Failed to get module path via argument, please try again.");
    return result;
}

void consoleLog(enum elogLevel loglevel, const char *service, const char *message, ...) {
    va_list args;
    va_start(args, message);
    FILE *out = NULL;
    bool toFile = false;
    if(useStdoutForAllLogs) {
        out = stdout;
        if(loglevel == LOG_LEVEL_ERROR || loglevel == LOG_LEVEL_WARN || loglevel == LOG_LEVEL_DEBUG || loglevel == LOG_LEVEL_ABORT) out = stderr;
    }
    else {
        out = fopen(daemonLogs, "a");
        if(!out) exit(EXIT_FAILURE);
        toFile = true;
    }
    switch(loglevel) {
        case LOG_LEVEL_INFO:
            if(!toFile) fprintf(out, "\033[2;30;47mINFO: ");
            else fprintf(out, "INFO: %s: ", service);
        break;
        case LOG_LEVEL_WARN:
            if(!toFile) fprintf(out, "\033[1;33mWARNING: ");
            else fprintf(out, "WARNING: %s: ", service);
        break;
        case LOG_LEVEL_DEBUG:
            if(!toFile) fprintf(out, "\033[0;36mDEBUG: ");
            else fprintf(out, "DEBUG: %s: ", service);
        break;
        case LOG_LEVEL_ERROR:
            if(!toFile) fprintf(out, "\033[0;31mERROR: ");
            else fprintf(out, "ERROR: %s: ", service);
        break;
        case LOG_LEVEL_ABORT:
            if(!toFile) fprintf(out, "\033[0;31mABORT: ");
            else fprintf(out, "ABORT: %s: ", service);
        break;
    }
    vfprintf(out, message, args);
    if(!toFile) fprintf(out, "\033[0m\n");
    else fprintf(out, "\n");
    if(!useStdoutForAllLogs && out) fclose(out);
    va_end(args);
}

void abort_instance(const char *service, const char *format, ...) {
    va_list args;
    va_start(args, format);
    consoleLog(LOG_LEVEL_ABORT, "%s", "%s %s", service, format, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void printBannerWithRandomFontStyle() {
    const char *banner1 =
    "\033[38;2;255;120;120m     ..      ...                             ...     ..      ..                       ..                                                ..      \n"
    "\033[38;2;255;80;80m  :~\"8888x :\"%888x                         x*8888x.:*8888: -\"888:               x .d88\"    x=~                                    < .z@8\"      \n"
    "\033[38;2;255;40;40m 8    8888Xf  8888>                       X   48888X 8888H  8888                5888R    88x.   .e.   .e.                         !@88E        \n"
    "\033[38;2;255;0;0m X88x. ?8888k  8888X       .u             X8x.  8888X  8888X  !888>        u      '888R   '8888X.x888:.x888        u           .    '888E   u    \n"
    "\033[38;2;235;0;0m '8888L'8888X  '%88X    ud8888.           X8888 X8888  88888   \"*8%-    us888u.    888R    8888  888X '888k    us888u.   .udR88N    888E u@8NL  \n"
        "\033[38;2;210;0;0m  \"888X 8888X:xnHH(` :888'8888.          '*888!X8888> X8888  xH8>   .@88 \"8888\"   888R     X888  888X  888X .@88 \"8888\" <888'888k   888E\"88*\"  \n"
        "\033[38;2;180;0;0m    ?8~ 8888X X8888   d888 '88%\"            ?8 8888  X888X X888>   9888  9888    888R     X888  888X  888X 9888  9888  9888 'Y\"    888E .dN.   \n"
        "\033[38;2;150;0;0m  -~   8888> X8888   8888.+\"               -^  '888\"  X888  8888>   9888  9888    888R     X888  888X  888X 9888  9888  9888        888E~8888   \n"
        "\033[38;2;120;0;0m  :H8x  8888  X8888   8888L                    dx '88~x. !88~  8888>   9888  9888    888R    .X888  888X. 888~ 9888  9888  9888        888E '888&  \n"
        "\033[38;2;100;0;0m  8888> 888~  X8888   '8888c. .+             .8888Xf.888x:!    X888X.: 9888  9888   .888B .  %88%\"*888Y\"    9888  9888  ?8888u../   888E  9888. \n"
        "\033[38;2;80;0;0m  48\" '8*~   8888!  \"88888%            :\"\"888\":~\"888\"     888*\"  \"888*\"\"888\"  ^*888%     ~     \"       \"888*\"\"888\"  \"8888P'  '\"888*\" 4888\" \n"
        "\033[38;2;60;0;0m  ^-==\"\"      \"\"       \"YP'                 \"~'    \"~        \"\"     ^Y\"   ^Y'     \"%                        ^Y\"   ^Y'     \"P'       \"\"    \"\"   \n";
        
    const char *banner2 =
        "\033[38;2;255;102;102m:::::::..  .,::::::   .        :    :::.      ::: .::    .   .::::::.       .,-:::::  :::  .   \n"
        "\033[38;2;255;51;51m;;;;`;;;; ;;;;''''   ;;,.    ;;;   ;;;;     ;;; ';;,  ;;  ;;;' ;;;;    ,;;;''  ;;; .;;,.\n"
        "\033[38;2;204;0;0m[[[,/[[['  [[cccc    [[[[, ,[[[[, ,[[ '[[,   [[[  '[[, [[, [[' ,[[ '[[,  [[[         [[[[[/'  \n"
        "\033[38;2;153;0;0m$$$$$$c    $$\"\"\"\"cccc$$$$$$$$\"$$$c$$$cc$$$c  $$'    Y$c$$$c$P c$$$cc$$$c $$$        _$$$$,    \n"
        "\033[38;2;102;0;0m888b \"88bo,888oo,__  888 Y88\" 888o888   888,o88oo,.__\"88\"888   888   888,`88bo,__,o,\"888\"88o, \n"
        "\033[38;2;51;0;0mMMMM   \"W\" \"\"\"\"YUMMM MMM  M'  \"MMMYMM   \"\"` \"\"\"\"YUMMM \"M \"M\"   YMM   \"\"`   \"YUMMMMMP\"MMM \"MMP\"\n";
        
        const char *banner3 =
        "\033[0;31m    ____             __  ___      __                    __            \n"
        "   / __ \\___        /  |/  /___ _/ /      ______ ______/ /__          \n"
        "  / /_/ / _ \\______/ /|_/ / __ `/ / | /| / / __ `/ ___/ //_/       \n"
        " / _, _/  __/_____/ /  / / /_/ / /| |/ |/ / /_/ / /__/ ,<             \n"
        "/_/ |_|\\___/     /_/  /_/\\__,_/_/ |__/|__/\\__,_/\\___/_/|_|           \n";
        
    const char *banner4 =
        "\033[0;31m ______     ______     __    __     ______     __         __     __     ______     ______     __  __    \n"
        "/\\  == \\   /\\  ___\\   /\\ \"-./  \\   /\\  __ \\   /\\ \\       /\\ \\  _ \\ \\   /\\  __ \\   /\\  ___\\   /\\ \\/ /    \n"
        "\\ \\  __<   \\ \\  __\\   \\ \\ \\-./\\ \\  \\ \\  __ \\  \\ \\ \\____  \\ \\ \\/ \".\\ \\  \\ \\  __ \\  \\ \\ \\____  \\ \\  _\"-.  \n"
        " \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\_\\ \\ \\_\\  \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\__/\".~\\_\\  \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\_\\ \\_\\ \n"
        "  \\/_/ /_/   \\/_____/   \\/_/  \\/_/   \\/_/\\/_/   \\/_____/   \\/_/   \\/_/   \\/_/\\/_/   \\/_____/   \\/_/\\/_/ \n";
        
    const char *banner5 =
    "\033[38;2;255;102;102m ██▀███  ▓█████  ███▄ ▄███▓ ▄▄▄       ██▓     █     █░ ▄▄▄       ▄████▄   ██ ▄█▀\n"
    "\033[38;2;255;51;51m▓██ ▒ ██▒▓█   ▀ ▓██▒▀█▀ ██▒▒████▄    ▓██▒    ▓█░ █ ░█░▒████▄    ▒██▀ ▀█   ██▄█▒ \n"
        "\033[38;2;204;0;0m▓██ ░▄█ ▒▒███   ▓██    ▓██░▒██  ▀█▄  ▒██░    ▒█░ █ ░█ ▒██  ▀█▄  ▒▓█    ▄ ▓███▄░ \n"
        "\033[38;2;153;0;0m▒██▀▀█▄  ▒▓█  ▄ ▒██    ▒██ ░██▄▄▄▄██ ▒██░    ░█░ █ ░█ ░██▄▄▄▄██ ▒▓▓▄ ▄██▒▓██ █▄ \n"
        "\033[38;2;102;0;0m░██▓ ▒██▒░▒████▒▒██▒   ░██▒ ▓█   ▓██▒░██████▒░░██▒██▓  ▓█   ▓██▒▒ ▓███▀ ░▒██▒ █▄\n"
        "\033[38;2;51;0;0m░ ▒▓ ░▒▓░░░ ▒░ ░░ ▒░   ░  ░ ▒▒   ▓▒█░░ ▒░▓  ░░ ▓░▒ ▒   ▒▒   ▓▒█░░ ░▒ ▒  ░▒ ▒▒ ▓▒\n";
        
    const char *banner6 =
    "\033[0;31m██████╗ ███████╗    ███╗   ███╗ █████╗ ██╗     ██╗    ██╗ █████╗  ██████╗██╗  ██╗\n"
    "██╔══██╗██╔════╝    ████╗ ████║██╔══██╗██║     ██║    ██║██╔══██╗██╔════╝██║ ██╔╝\n"
    "██████╔╝█████╗█████╗██╔████╔██║███████║██║     ██║ █╗ ██║███████║██║     █████╔╝ \n"
    "██╔══██╗██╔══╝╚════╝██║╚██╔╝██║██╔══██║██║     ██║███╗██║██╔══██║██║     ██╔═██╗ \n"
        "██║  ██║███████╗    ██║ ╚═╝ ██║██║  ██║███████╗╚███╔███╔╝██║  ██║╚██████╗██║  ██╗\n"
        "╚═╝  ╚═╝╚══════╝    ╚═╝     ╚═╝╚═╝  ╚═╝╚══════╝ ╚══╝╚══╝ ╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝\n";
        
    // OK
    const char *banners[] = {banner1, banner2, banner3, banner4, banner5, banner6};
    srand((unsigned int)time(NULL));
    printf("%s\n", banners[rand() % sizeof(banners) / sizeof(banners[0])]);
}

void pauseADBlock() {
    if(access(combineStringsFormatted("%s/hosts.bak", persistDir), F_OK) == 0) consoleLog(LOG_LEVEL_WARN, "pauseADBlock", "Protection is already paused!");
    refreshBlockedCounts(); // refresh it because both default val is set to 0
    if(blockedMod == 0 && blockedSys == 0) abort_instance("pauseADBlock", "You can't pause Re-Malwack while the hosts file is reset. Please reset the hosts file first.");
    consoleLog(LOG_LEVEL_DEBUG, "pauseADBlock", "Trying to pause Re-Malwack's protection...");
    FILE *backupHostsFile = fopen(hostsBackupPath, "w");
    if(!backupHostsFile) abort_instance("pauseADBlock", "Failed to open hosts backup file: %s", hostsBackupPath);
    FILE *hostsFile = fopen(hostsPath, "r+");
    if(!hostsFile) {
        fclose(backupHostsFile);
        abort_instance("pauseADBlock", "Failed to open hosts file: %s", hostsPath);
    }
    char string[1024];
    while(fgets(string, sizeof(string), hostsFile) != NULL) fprintf(backupHostsFile, "%s", string);
    fclose(backupHostsFile);
    fprintf(hostsFile, "127.0.0.1 localhost\n::1 localhost\n");
    fclose(hostsFile);
    chmod(hostsPath, 0644);
    putConfig("adblock_switch", 1);
    refreshBlockedCounts();
    reWriteModuleProp("Status: Protection is temporarily disabled due to the daemon toggling the module for an app ❌");
}

void resumeADBlock() {
    consoleLog(LOG_LEVEL_DEBUG, "resumeADBlock", "Trying to resume protection...");
    if(access(combineStringsFormatted("%s/hosts.bak", persistDir), F_OK) == 0) {
        reWriteModuleProp("Status: Protection is temporarily enabled due to the daemon toggling the module for an app ❌");
        FILE *backupHostsFile = fopen(hostsBackupPath, "r");
        if(!backupHostsFile) abort_instance("resumeADBlock", "Failed to open backup hosts file: %s\nDue to this failure, we are unable to restore previous backed-up hosts.", hostsBackupPath);
        FILE *hostsFile = fopen(hostsPath, "w");
        if(!hostsFile) {
            fclose(backupHostsFile);
            abort_instance("resumeADBlock", "Failed to open hosts file: %s", hostsPath);
        }
        char string[1024];
        while(fgets(string, sizeof(string), backupHostsFile) != NULL) fprintf(hostsFile, "%s", string);
        fclose(hostsFile);
        fclose(backupHostsFile);
        chmod(hostsPath, 0644);
        remove(hostsBackupPath);
        executeShellCommands("sync", (const char *[]){"sync", NULL});
        refreshBlockedCounts();
        putConfig("adblock_switch", 0);
        consoleLog(LOG_LEVEL_DEBUG, "resumeADBlock", "Protection services have been resumed.");
    }
    else {
        consoleLog(LOG_LEVEL_DEBUG, "resumeADBlock", "No backup hosts file found to resume, force resuming protection and running hosts update as a fallback action");
        putConfig("adblock_switch", 0);
        // i've come to the conclusion that i should have an boolean for this action
        // to stop running --update-hosts everytime.
        if(!shouldForceReMalwackUpdate) {
            if(executeShellCommands("/data/adb/modules/Re-Malwack/rmlwk.sh", (const char *[]) {"/data/adb/modules/Re-Malwack/rmlwk.sh", "--update-hosts"}) == 0) shouldForceReMalwackUpdate = true;
        }
    }
}

void help(const char *wehgcfbkfbjhyghxdrbtrcdfv) {
    printf("Usage:\n");
    printf("  %s [OPTION] [ARGUMENTS]\n\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("Options:\n");
    printf("-a  |  --add-app <app_name>\t\tAdd an application to the list to stop ad blocker when the app is opened.\n");
    printf("-r  |  --remove-app <app_name>\tRemove an application from the list.\n");
    printf("-i  |  --import-package-list <file>\tImport the app list from the already exported file.\n");
    printf("-e  |  --export-package-list <file>\tExport the encoded app list to a path for restoration.\n");
    printf("-x  |  --enable-daemon\tEnables \n");
    printf("-d  |  --disable-daemon\n");
    printf("     --help\t\t\tDisplay this help message.\n\n");
    printf("Examples:\n");
    printf("  %s --add-app com.example.myapp\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --remove-app com.example.myapp\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --export-package-list apps.txt\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --import-package-list apps.txt\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --enable-daemon\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --disable-daemon\n", wehgcfbkfbjhyghxdrbtrcdfv);
}

void freePointer(void **ptr) {
    if(ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

void finishMessage(const char *message, ...) {
    va_list args;
    va_start(args, message);
    FILE *out = NULL;
    bool toFile = false;
    if(useStdoutForAllLogs) out = stdout;
    else {
        out = fopen(daemonLogs, "a");
        if(!out) exit(EXIT_FAILURE);
        toFile = true;
    }
    if(!toFile) fprintf(out, "\033[2;30;47m");
    else fprintf(out, "Final: ");
    vfprintf(out, message, args);
    if(!toFile) fprintf(out, "\033[0m\n");
    else fprintf(out, "\n");
    va_end(args);
    exit(EXIT_SUCCESS);
}

void refreshBlockedCounts() {
    // reset it before counting all over again.
    blockedMod = 0;
    blockedSys = 0;
    mkdir(combineStringsFormatted("%s/counts", persistDir), 0755);
    FILE *hostsThingMod = fopen(hostsPath, "r");
    if(!hostsThingMod) abort_instance("refreshBlockedCounts", "Failed to open %s for reading to update blocklist count", hostsPath);
    char contentSizeMod[1024];
    while(fgets(contentSizeMod, sizeof(contentSizeMod), hostsThingMod) != NULL) if(strncmp(contentSizeMod, "0.0.0.0 ", 8) == 0) blockedMod++;
    fclose(hostsThingMod);
    FILE *hostsThingSys = fopen(systemHostsPath, "r");
    if(!hostsThingSys) abort_instance("refreshBlockedCounts", "Failed to open %s for reading to update blocklist count", systemHostsPath);
    char contentSizeSys[1024];
    while(fgets(contentSizeSys, sizeof(contentSizeSys), hostsThingSys) != NULL) if(strncmp(contentSizeSys, "0.0.0.0 ", 8) == 0) blockedSys++;
    fclose(hostsThingSys);
    FILE *sysCountFile = fopen(combineStringsFormatted("%s/counts/blocked_sys.count", persistDir), "w");
    if(!sysCountFile) abort_instance("refreshBlockedCounts", "Failed to open %s to update adblock count", combineStringsFormatted("%s/counts/blocked_sys.count", persistDir));
    fprintf(sysCountFile, "%d", blockedSys);
    fclose(sysCountFile);
    FILE *modCountFile = fopen(combineStringsFormatted("%s/counts/blocked_mod.count", persistDir), "w");
    if(!modCountFile) abort_instance("refreshBlockedCounts", "Failed to open %s to update adblock count", combineStringsFormatted("%s/counts/blocked_mod.count", persistDir));
    fprintf(modCountFile, "%d", blockedMod);
    fclose(modCountFile);
}

void reWriteModuleProp(const char *desk) {
    // "write" instead of "append" to rewrite everything line by line.
    // you know what? forget it, i was just being lame.
    // so basically i hate the way i wrote the property before. it feels like crap.
    FILE *moduleProp = fopen(modulePropFile, "r+");
    if(!moduleProp) abort_instance("reWriteModuleProp", "Failed to open module's property file. Please report this error in my discord!");
    char content[1024];
    while(fgets(content, sizeof(content), moduleProp)) {
        // let's remove the newline if we are in desc.
        if(strstr(content, "description")) { 
            content[strcspn(content, "\n")] = 0;
            fprintf(moduleProp, "description=%s\n", desk);
        }
        else fputs(content, moduleProp);
    }
    fclose(moduleProp);
}

void killDaemonWhenSignaled(int sig) {
    putConfig("is_daemon_running", NOT_RUNNING_CANT_RUN);
}