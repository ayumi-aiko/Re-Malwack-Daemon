#include <daemon.h>

int putConfig(const char *__variableName, const int __variableValue) {
    FILE *fp = fopen(configScriptPath, "r");
    if(!fp) abort_instance("putConfig", "Failed to open %s, please try again..", configScriptPath);
    char **lines = NULL;
    size_t count = 0;
    char buffer[1024];
    int found = 0;
    while(fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\r\n")] = 0;
        if(strncmp(buffer, __variableName, strlen(__variableName)) == 0 && buffer[strlen(__variableName)] == '=') {
            char newLine[256];
            snprintf(newLine, sizeof(newLine), "%s=%d", __variableName, __variableValue);
            lines = realloc(lines, sizeof(char*) * (count + 1));
            lines[count++] = strdup(newLine);
            found = 1;
        }
        else {
            lines = realloc(lines, sizeof(char*) * (count + 1));
            lines[count++] = strdup(buffer);
        }
    }
    fclose(fp);
    if(!found) {
        char newLine[256];
        snprintf(newLine, sizeof(newLine), "%s=%d", __variableName, __variableValue);
        lines = realloc(lines, sizeof(char*) * (count + 1));
        lines[count++] = strdup(newLine);
    }
    fp = fopen(configScriptPath, "w");
    if(!fp) abort_instance("putConfig", "Failed to write to %s", configScriptPath);
    for(size_t i = 0; i < count; ++i) {
        fprintf(fp, "%s\n", lines[i]);
        free(lines[i]);
    }
    free(lines);
    fclose(fp);
    return 0;
}

bool isPackageInList(const char *packageName) {
    FILE *packageFile = fopen(daemonPackageLists, "r");
    if(!packageFile) abort_instance("isPackageInList", "Failed to open the package lists file, please run this command again or report this issue to the devs.");
    char contentFromFile[1028];
    while(fgets(contentFromFile, sizeof(contentFromFile), packageFile) != NULL) {
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
    if(!isPackageInList(packageName)) {
        fprintf(packageFile, "\n%s\n", packageName);
        consoleLog(LOG_LEVEL_DEBUG, "addPackageToList", "Successfully added %s into the list, the daemon will add the packages to the list for a short period of time.");
        fclose(packageFile);
        return true;
    }
    else {
        consoleLog(LOG_LEVEL_DEBUG, "addPackageToList", "%s is already present in the lists, please try again with a different application.");
        fclose(packageFile);
        return false;
    }
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
            consoleLog(LOG_LEVEL_DEBUG, "removePackageFromList", "Found %s on the list, removing the package from the list...");
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
            consoleLog(LOG_LEVEL_ERROR, "executeShellCommands", "Failed to execute command: %s", command);
        break;
        default:
            consoleLog(LOG_LEVEL_DEBUG, "executeShellCommands", "Waiting for current %d to finish, exec bin name: %s", ProcessID, command);
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
    static char packageName[8000];
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
    // im not interested to write much, why dont we just change the value and make it pass it in the stderr?
    // fuck you if you comment about my code style bitch!
    suppressEverythingExceptInfo = false;
    consoleLog(LOG_LEVEL_ABORT, "%s", "%s %s", service, format, args);
    va_end(args);
    freePointer((void **)&MODPATH);
    freePointer((void **)&hostsBackupPath);
    freePointer((void **)&hostsPath);
    freePointer((void **)&configScriptPath);
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
    int totalBanners = sizeof(banners) / sizeof(banners[0]);
    srand((unsigned int)time(NULL));
    int index = rand() % totalBanners;
    printf("%s\n", banners[index]);
}

void setAdblockSwitch(const char *configPath, int state) {
    // variable declarations:
    int modified = 0;
    char line[1024];
    char tempPath[1024];

    // check if requested state is valid:
    if(state != 0 && state != 1) {
        consoleLog(LOG_LEVEL_ERROR, "setAdblockSwitch", "Invalid adblock state: %d (must be 0 or 1)", state);
        return;
    }

    // create a temporary file path:
    snprintf(tempPath, sizeof(tempPath), "%s.tmp", configPath);

    // open the original config file for reading and a temporary file for writing:
    FILE *original = fopen(configPath, "r");
    if(!original) {
        consoleLog(LOG_LEVEL_DEBUG, "setAdblockSwitch", " Failed to open config file: %s", configPath);
        return;
    }

    // open a temporary file for writing:
    FILE *temp = fopen(tempPath, "w");
    if(!temp) {
        consoleLog(LOG_LEVEL_DEBUG, "setAdblockSwitch", "Failed to open temporary file: %s", tempPath);
        fclose(original);
        return;
    }

    // read the original file line by line:
    // if the line starts with "adblock_switch=", replace it with the new state:
    while(fgets(line, sizeof(line), original)) {
        if(strncmp(line, "adblock_switch=", 15) == 0) {
            fprintf(temp, "adblock_switch=%d\n", state);
            modified = 1;
        }
        else fputs(line, temp);
    }

    // If "adblock_switch=" was not found, append it at the end
    if(!modified) fprintf(temp, "adblock_switch=%d\n", state);
    
    // close the files after operation:
    fclose(original);
    fclose(temp);
    if(rename(tempPath, configPath) != 0) consoleLog(LOG_LEVEL_DEBUG, "setAdblockSwitch", "Failed to replace original config file: %s", configPath);
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
    putConfig("adblockSwitch", 1);
    refreshBlockedCounts();
    finishMessage("Protection services have been paused. You can resume it by running Shizuka with the --adblock-switch option.");
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
        setAdblockSwitch(configScriptPath, 1);
        consoleLog(LOG_LEVEL_DEBUG, "resumeADBlock", "Protection services have been resumed.");
    }
    else consoleLog(LOG_LEVEL_DEBUG, "resumeADBlock", "No backup hosts file found to resume.");
}

void help(const char *wehgcfbkfbjhyghxdrbtrcdfv) {
    printf("Usage:\n");
    printf("  %s [OPTION] [ARGUMENTS]\n\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("Options:\n");
    printf("  --add-app <app_name>\t\tAdd an application to the list to stop ad blocker when the app is opened.\n");
    printf("  --remove-app <app_name>\tRemove an application from the list.\n");
    printf("  --export-package-list <file>\tExport the encoded app list to a path for restoration.\n");
    printf("  --import-package-list <file>\tImport the app list from the already exported file.\n");
    printf("  --help\t\t\tDisplay this help message.\n\n");
    printf("Examples:\n");
    printf("  %s --add-app com.example.myapp\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --remove-app com.example.myapp\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --export-package-list apps.txt\n", wehgcfbkfbjhyghxdrbtrcdfv);
    printf("  %s --import-package-list apps.txt\n", wehgcfbkfbjhyghxdrbtrcdfv);
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
    freePointer((void **)&MODPATH);
    freePointer((void **)&hostsBackupPath);
    freePointer((void **)&hostsPath);
    freePointer((void **)&configScriptPath);
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
    FILE *moduleProp = fopen(modulePropFile, "w");
    if(!moduleProp) abort_instance("reWriteModuleProp", "Failed to open module prop file: %s", modulePropFile);
    fprintf(moduleProp, "id=Re-Malwack\n");
    fprintf(moduleProp, "name=Re-Malwack | Not just a normal ad-blocker module ✨\n");
    fprintf(moduleProp, "version=%s\n", version);
    fprintf(moduleProp, "versionCode=%s\n", versionCode);
    fprintf(moduleProp, "author=ZG089\n");
    fprintf(moduleProp, "description=%s\n", desk);
    fprintf(moduleProp, "updateJson=https://raw.githubusercontent.com/ZG089/Re-Malwack/main/update.json\n");
    fprintf(moduleProp, "support=https://t.me/Re_Malwack\n");
    fprintf(moduleProp, "donate=https://buymeacoffee.com/zg089\n");
    fprintf(moduleProp, "banner=https://raw.githubusercontent.com/ZG089/Re-Malwack/main/assets/banner.png");
    fclose(moduleProp);
}