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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

// detaches the daemon, we can use detach like we do with bash but
// we are targetting sh.
int main(void) {
    if(getuid() != 0) {
        printf("Failed to gain root privilages, Please run this binary in a rooted environment!\n");
        exit(EXIT_FAILURE);
    }
    
    // checks before running daemon!
    if(access("/data/adb/Re-Malwack/module.prop", F_OK) != 0) {
        printf("Please install Re-Malwack to proceed.\n");
        exit(EXIT_FAILURE);
    }
    if(access("/data/adb/Re-Malwack/currentDaemonPID", F_OK) != 0) system("echo "" > /data/adb/Re-Malwack/currentDaemonPID");
    system("rm -rf /data/adb/Re-Malwack/.daemon1");
    
    // let's not handle unknown because we are not gonna wait for the fork to finish and
    // we be exiting this shit after we make this child (ts soo kevin 😭🥀)
    char *pathToDaemon = "/data/adb/Re-Malwack/remalwack-daemon";
    switch(fork()) {
        case -1:
            printf("Failed to fork a child process, please try running this binary again!\n");
            exit(EXIT_FAILURE);
        break;
        case 0:
            // int execve(const char *pathname, char *const argv[], char *const envp[]);
            setsid();
            execve(pathToDaemon, (char *const[]) {pathToDaemon, NULL}, NULL);
            printf("Failed to start this process..\n");
            exit(EXIT_FAILURE);
        break;
    }
    // we ran the daemon yay!
    exit(EXIT_SUCCESS);
}