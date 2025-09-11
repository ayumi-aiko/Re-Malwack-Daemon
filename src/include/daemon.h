#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>

// vars
extern int blockedMod;
extern int blockedSys;
extern bool useStdoutForAllLogs;
extern bool suppressEverythingExceptInfo;
extern char *MODPATH;
extern char *modulePropFile;
extern char *version;
extern char *versionCode;
extern char *configScriptPath;
extern char *hostsPath;
extern char *hostsBackupPath;
extern const char *persistDir;
extern const char *daemonLogs;
extern const char *daemonLockFile;
extern const char *daemonLockFileStuck;
extern const char *daemonPackageLists;
extern const char *daemonLockFileSuccess;
extern const char *systemHostsPath;
extern const char *currentDaemonPIDFile;
enum elogLevel {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_ABORT
};

// macros:
#define MAX_PACKAGES 100
#define PACKAGE_NAME_SIZE 256

// functions:
int putConfig(const char *__variableName, const int __variableValue);
bool isPackageInList(const char *packageName);
bool removePackageFromList(const char *packageName);
bool addPackageToList(const char *packageName);
bool eraseFile(const char *__file);
bool executeShellCommands(const char *command, const char *args[]);
bool isDefaultHosts(const char *filename);
char *grepProp(const char *variableName, const char *propFile);
char *combineStringsFormatted(const char *format, ...);
char *getCurrentPackage();
char *modulePath(const char *Zero);
void consoleLog(enum elogLevel loglevel, const char *service, const char *message, ...);
void abort_instance(const char *service, const char *format, ...);
void printBannerWithRandomFontStyle();
void setAdblockSwitch(const char *configPath, int state);
void pauseADBlock();
void resumeADBlock();
void help(const char *wehgcfbkfbjhyghxdrbtrcdfv);
void freePointer(void **ptr);
void finishMessage(const char *message, ...);
void refreshBlockedCounts();
void reWriteModuleProp(const char *desk);