#
# Copyright (C) 2025 愛子あゆみ <ayumi.aiko@outlook.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# build arguments, its good to use tons of variable for portability.
CC_ROOT = /home/ayumi/android-ndk-r27d/toolchains/llvm/prebuilt/linux-x86_64/bin
CFLAGS = -std=c23 -O3 -static
INCLUDE = ./src/include
SRCS = ./src/include/daemon.c
TARGET = ./src/daemon/main.c
BUILD_LOGFILE = ./build/logs/logs
OUTPUT_DIR = ./build

# Avoid SDK/ARCH checks for 'clean'
ifneq ($(filter clean help,$(MAKECMDGOALS)),clean help)
	ifeq ($(ARCH),arm64)
		CC := $(CC_ROOT)/aarch64-linux-android$(SDK)-clang
	endif
	ifeq ($(ARCH),arm)
		CC := $(CC_ROOT)/armv7a-linux-androideabi$(SDK)-clang
	endif
	ifeq ($(ARCH),x86)
		CC := $(CC_ROOT)/i686-linux-android$(SDK)-clang
	endif
	ifeq ($(ARCH),x86_64)
		CC := $(CC_ROOT)/x86_64-linux-android$(SDK)-clang
	endif
endif

# checks args
checkArgs:
	@if [ -z "$(SDK)" ]; then \
	  printf "\033[0;31mmake: Error: SDK is not set. Please specify the Android API level, e.g., 'SDK=30'\033[0m\n"; \
	  exit 1; \
	fi; \
	if [ -z "$(ARCH)" ]; then \
	  printf "\033[0;31mmake: Error: ARCH is not set. Please specify the target architecture, e.g., 'ARCH=arm64'\033[0m\n"; \
	  exit 1; \
	fi

# check the existance of compiler before trying to build.
checkCompilerExistance: checkArgs
	@[ -x "$(CC)" ] || { \
		printf "\033[0;31mmake: Error: Compiler '%s' not found or not executable. Please check the path or install it.\033[0m\n" "$(CC)"; \
		exit 1; \
	}

# this builds the program after checking the existance of the clang or gcc compiler existance.
daemon: checkCompilerExistance banner
	@echo "\e[0;35mmake: Info: Trying to build Re-Malwack daemon..\e[0;37m"
	@$(CC) $(CFLAGS) -I$(INCLUDE) $(SRCS) $(TARGET) -o $(OUTPUT_DIR)/remalwack-daemon 2>./$(BUILD_LOGFILE) || { \
		printf "\033[0;31mmake: Error: Build failure, check the logs for information. File can be found at $(BUILD_LOGFILE)\033[0m\n"; \
		exit 1; \
	}
	@echo "\e[0;36mmake: Info: Build finished without errors, be sure to check logs if concerned. Thank you!\e[0;37m"

# this builds the program after checking dependencies, this is for managing the daemon.
katana: checkCompilerExistance banner
	@echo "\e[0;35mmake: Info: Trying to build Re-Malwack daemon manager..\e[0;37m"
	@$(CC) $(CFLAGS) -I$(INCLUDE) $(SRCS) ./src/program/main.c -o $(OUTPUT_DIR)/remalwack-katana 2>./$(BUILD_LOGFILE) || { \
		printf "\033[0;31mmake: Error: Build failure, check the logs for information. File can be found at $(BUILD_LOGFILE)\033[0m\n"; \
		exit 1; \
	}
	@echo "\e[0;36mmake: Info: Build finished without errors, be sure to check logs if concerned. Thank you!\e[0;37m"

daemonStarter: checkCompilerExistance banner
	@echo "\e[0;35mmake: Info: Trying to build daemonStarter..\e[0;37m"
	@$(CC) $(CFLAGS) ./src/daemon/daemonStarter.c -o $(OUTPUT_DIR)/daemonStarter 2>./$(BUILD_LOGFILE) || { \
		printf "\033[0;31mmake: Error: Build failure, check the logs for information. File can be found at $(BUILD_LOGFILE)\033[0m\n"; \
		exit 1; \
	}
	@echo "\e[0;36mmake: Info: Build finished without errors, be sure to check logs if concerned. Thank you!\e[0;37m"

# builds all:
all: daemon katana daemonStarter

# prints the banner of the program.
banner:
	@printf "\e[38;2;255;120;120m%s\e[0m\n" "     ..      ...                             ...     ..      ..                       ..                                                ..      "
	@printf "\e[38;2;255;80;80m%s\e[0m\n" "  :~\"8888x :\"%%888x                         x*8888x.:*8888: -\"888:               x .d88\"    x=~                                    < .z@8\"      "
	@printf "\e[38;2;255;40;40m%s\e[0m\n" " 8    8888Xf  8888>                       X   48888X 8888H  8888                5888R    88x.   .e.   .e.                         !@88E        "
	@printf "\e[38;2;255;0;0m%s\e[0m\n" " X88x. ?8888k  8888X       .u             X8x.  8888X  8888X  !888>        u      '888R   '8888X.x888:.x888        u           .    '888E   u    "
	@printf "\e[38;2;235;0;0m%s\e[0m\n" " '8888L'8888X  '%%88X    ud8888.           X8888 X8888  88888   \"*8%%-    us888u.    888R    8888  888X '888k    us888u.   .udR88N    888E u@8NL  "
	@printf "\e[38;2;210;0;0m%s\e[0m\n" "  \"888X 8888X:xnHH(\` :888'8888.          '*888!X8888> X8888  xH8>   .@88 \"8888\"   888R     X888  888X  888X .@88 \"8888\" <888'888k   888E\"88*\"  "
	@printf "\e[38;2;180;0;0m%s\e[0m\n" "    ?8~ 8888X X8888   d888 '88%%\"            ?8 8888  X888X X888>   9888  9888    888R     X888  888X  888X 9888  9888  9888 'Y\"    888E .dN.   "
	@printf "\e[38;2;150;0;0m%s\e[0m\n" "  -~   8888> X8888   8888.+\"               -^  '888\"  X888  8888>   9888  9888    888R     X888  888X  888X 9888  9888  9888        888E~8888   "
	@printf "\e[38;2;120;0;0m%s\e[0m\n" "  :H8x  8888  X8888   8888L                    dx '88~x. !88~  8888>   9888  9888    888R    .X888  888X. 888~ 9888  9888  9888        888E '888&  "
	@printf "\e[38;2;100;0;0m%s\e[0m\n" "  8888> 888~  X8888   '8888c. .+             .8888Xf.888x:!    X888X.: 9888  9888   .888B .  %%88%%\"*888Y\"    9888  9888  ?8888u../   888E  9888. "
	@printf "\e[38;2;80;0;0m%s\e[0m\n" "  48\" '8*~   8888!  \"88888%%            :\"\"888\":~\"888\"     888*\"  \"888*\"\"888\"  ^*888%%     ~     \"       \"888*\"\"888\"  \"8888P'  '\"888*\" 4888\" "
	@printf "\e[38;2;60;0;0m%s\e[0m\n" "  ^-==\"\"      \"\"       \"YP'                 \"~'    \"~        \"\"     ^Y\"   ^Y'     \"%%                        ^Y\"   ^Y'     \"P'       \"\"    \"\"   "

help:
	@echo "\033[1;36mUsage:\033[0m make <target> [SDK=<level>] [ARCH=<arch>]"
	@echo ""
	@echo "\033[1;36mTargets:\033[0m"
	@echo "  \033[0;32mdaemon\033[0m     Build the Re-Malwack daemon binary"
	@echo "  \033[0;32mkatana\033[0m     Build the Re-Malwack daemon manager"
	@echo "  \033[0;32mdaemonStarter\033[0m     Build the Re-Malwack's daemon starter"
	@echo "  \033[0;32mclean\033[0m      Remove build artifacts"
	@echo "  \033[0;32mhelp\033[0m       Show this help message"
	@echo ""
	@echo "\033[1;36mExample:\033[0m"
	@echo "  make \033[0;32mdaemon\033[0m SDK=30 ARCH=arm64"

# removes the stuff made by compiler and makefile.
clean:
	@rm -f $(BUILD_LOGFILE) $(OUTPUT_DIR)/remalwack-daemon $(OUTPUT_DIR)/remalwack-katana $(OUTPUT_DIR)/daemonStarter
	@echo "\033[0;32mmake: Info: Clean complete.\033[0m"

.PHONY: daemon katana clean checkArgs checkCompilerExistance banner all daemonStarter