#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/utsname.h>

// Explicit function declarations
extern FILE* popen(const char *command, const char *mode);
extern int pclose(FILE *stream);
extern int gethostname(char *name, size_t len);

// ASCII Art Logos for various OS
const char *ubuntu_logo =
    "\033[1;31m   .-.     \n"
    "  (.. |    \n"
    "  <>  |    \n"
    " / --- \\   \n"
    "( |   | |  \n"
    " '-' '-'   \033[0m\n";

const char *arch_logo =
    "\033[1;36m       /\\      \n"
    "      /  \\     \n"
    "     /\\   \\    \n"
    "    /      \\   \n"
    "   /   ,,   \\  \n"
    "  /   |  |  -\\ \n"
    " /_-''    ''-_\\\033[0m\n";

const char *fedora_logo =
    "\033[1;34m      _____\n"
    "     /   __)\\ \n"
    "     |  /  \\ \\\n"
    "  ___|  |__/ /\n"
    " / (_    _) / \n"
    "/ /  |  |  /  \n"
    "\\ \\__/  /  \\  \n"
    " \\(_____/\\__\\ \033[0m\n";

const char *debian_logo =
    "\033[1;31m    _____    \n"
    "   /  __ \\   \n"
    "  |  /    |  \n"
    "  |  \\___-   \n"
    "  -_         \n"
    "    --_      \033[0m\n";

const char *generic_logo =
    "\033[1;32m    .--.     \n"
    "   |o_o |    \n"
    "   |:_/ |    \n"
    "  //   \\ \\   \n"
    " (|     | )  \n"
    "/'\\_   _/`\\  \n"
    "\\___)=(___/  \033[0m\n";

const char *wsl_logo =
    "\033[1;36m    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣸⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ \n"
    "    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ \n"
    "    ⠀⠀⠀⣀⣀⣀⠀⠀⢠⣾⣿⣿⡄⠀⠀⠀⣀⣀⣀⠀⠀⠀ \n"
    "    ⠀⣠⣿⣿⣿⣿⣿⣶⣿⣿⣿⣿⣿⣶⣿⣿⣿⣿⣿⣧⠀⠀ \n"
    "    ⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀ \n"
    "    W  S  L   ⣿⣿        ⠀⠀ \n"
    "    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ \n"
    "    ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ \033[0m\n";

// Color codes for prettier output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_WHITE   "\033[1;37m"

// Safe string copy function
void safe_strcpy(char *dest, const char *src, size_t size) {
    if (dest && src) {
        size_t i;
        for (i = 0; i < size - 1 && src[i]; i++) {
            dest[i] = src[i];
        }
        dest[i] = '\0';
    }
}

// Function to trim whitespace
char* trim(char* str) {
    if (!str) return NULL;

    char* end;

    // Trim leading spaces
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

// Function to run a command and get its output safely
char* run_command(const char* cmd) {
    static char buffer[512];
    memset(buffer, 0, sizeof(buffer)); // Initialize buffer to zeros

    FILE* fp = popen(cmd, "r");
    if (!fp) {
        safe_strcpy(buffer, "N/A", sizeof(buffer));
        return buffer;
    }

    if (fgets(buffer, sizeof(buffer) - 1, fp) == NULL) {
        pclose(fp);
        safe_strcpy(buffer, "N/A", sizeof(buffer));
        return buffer;
    }

    pclose(fp);

    // Remove newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }

    return buffer;
}

// Check if file exists
int file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Check if command exists
int command_exists(const char* cmd) {
    char command[256];
    snprintf(command, sizeof(command), "command -v %s >/dev/null 2>&1", cmd);
    return system(command) == 0;
}

// Check if running in WSL
int is_wsl() {
    return file_exists("/proc/sys/fs/binfmt_misc/WSLInterop") ||
           file_exists("/run/WSL") ||
           (getenv("WSL_DISTRO_NAME") != NULL);
}

// Function to get Linux distribution name
char* get_distro_name() {
    static char distro[64] = "unknown";

    // Check for /etc/os-release first
    if (file_exists("/etc/os-release")) {
        char* id = run_command("awk -F= '/^ID=/ {print $2}' /etc/os-release");
        if (strcmp(id, "N/A") != 0) {
            safe_strcpy(distro, id, sizeof(distro));
            return distro;
        }
    }

    // Try lsb_release if available
    if (command_exists("lsb_release")) {
        char* lsb = run_command("lsb_release -is | tr '[:upper:]' '[:lower:]'");
        if (strcmp(lsb, "N/A") != 0) {
            safe_strcpy(distro, lsb, sizeof(distro));
            return distro;
        }
    }

    // Check common distribution files
    if (file_exists("/etc/debian_version")) {
        safe_strcpy(distro, "debian", sizeof(distro));
    } else if (file_exists("/etc/fedora-release")) {
        safe_strcpy(distro, "fedora", sizeof(distro));
    } else if (file_exists("/etc/arch-release")) {
        safe_strcpy(distro, "arch", sizeof(distro));
    } else if (file_exists("/etc/redhat-release")) {
        safe_strcpy(distro, "redhat", sizeof(distro));
    }

    return distro;
}

// Function to gather system info
void get_system_info() {
    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        printf("Error: Unable to get system information\n");
        return;
    }

    // OS Info
    char* os_name = "Unknown";
    if (file_exists("/etc/os-release")) {
        os_name = run_command("awk -F= '/^PRETTY_NAME/ {print $2}' /etc/os-release");
        os_name = trim(os_name);
        // Remove surrounding quotes if present
        if (os_name[0] == '"' && os_name[strlen(os_name)-1] == '"') {
            os_name++;
            os_name[strlen(os_name)-1] = '\0';
        }
    }

    // Check if WSL
    if (is_wsl()) {
        char wsl_info[256];
        snprintf(wsl_info, sizeof(wsl_info), "%s (WSL)", os_name);
        printf("%sOS:%s %s\n", COLOR_CYAN, COLOR_RESET, wsl_info);
    } else {
        printf("%sOS:%s %s\n", COLOR_CYAN, COLOR_RESET, os_name);
    }

    // Hostname
    char hostname[128] = "Unknown";
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        printf("%sHost:%s %s\n", COLOR_CYAN, COLOR_RESET, hostname);
    } else {
        printf("%sHost:%s Unknown\n", COLOR_CYAN, COLOR_RESET);
    }

    // Kernel Version
    printf("%sKernel:%s %s\n", COLOR_CYAN, COLOR_RESET, sys_info.release);

    // Shell
    char* shell = getenv("SHELL");
    if (shell) {
        printf("%sShell:%s %s\n", COLOR_CYAN, COLOR_RESET, shell);
    }

    // Uptime
    if (command_exists("uptime")) {
        char* uptime = run_command("uptime -p 2>/dev/null || uptime | awk '{print $3 \" \" $4}' | sed 's/,//'");
        printf("%sUptime:%s %s\n", COLOR_CYAN, COLOR_RESET, uptime);
    }

    // Memory Usage - different approaches for different systems
    char* memory = "N/A";
    if (command_exists("free")) {
        memory = run_command("free -h 2>/dev/null | awk '/Mem:/ {print $3\"/\"$2}'");
    } else if (file_exists("/proc/meminfo")) {
        memory = run_command("awk '/MemTotal|MemAvailable/ {print $2}' /proc/meminfo | paste -sd/ | awk '{print $2\"K/\"$1\"K\"}'");
    }
    printf("%sMemory:%s %s\n", COLOR_CYAN, COLOR_RESET, memory);

    // CPU Info - different approaches for different systems
    char* cpu = "N/A";
    if (file_exists("/proc/cpuinfo")) {
        cpu = run_command("grep 'model name' /proc/cpuinfo | head -1 | sed 's/model name[[:space:]]*: //'");
    } else if (command_exists("sysctl")) {
        cpu = run_command("sysctl -n machdep.cpu.brand_string 2>/dev/null");
    }
    printf("%sCPU:%s %s\n", COLOR_CYAN, COLOR_RESET, cpu);

    // Disk Space
    if (command_exists("df")) {
        char* disk = run_command("df -h / | awk 'NR==2 {print $3\"/\"$2}'");
        printf("%sDisk:%s %s\n", COLOR_CYAN, COLOR_RESET, disk);
    }

    // GPU Info - try different approaches
    char* gpu = "N/A";
    if (command_exists("lspci")) {
        gpu = run_command("lspci 2>/dev/null | grep -i 'vga\\|3d\\|display' | head -n1 | sed 's/.*: //g'");
    }
    if (strcmp(gpu, "N/A") == 0 && file_exists("/sys/class/drm")) {
        gpu = run_command("find /sys/devices -name gpu_model -exec cat {} \\; 2>/dev/null | head -1");
    }
    printf("%sGPU:%s %s\n", COLOR_CYAN, COLOR_RESET, gpu);

    // Architecture
    printf("%sArchitecture:%s %s\n", COLOR_CYAN, COLOR_RESET, sys_info.machine);

    // User
    char* user = getenv("USER");
    if (user) {
        printf("%sUser:%s %s\n", COLOR_CYAN, COLOR_RESET, user);
    } else {
        user = run_command("whoami");
        printf("%sUser:%s %s\n", COLOR_CYAN, COLOR_RESET, user);
    }

    // Terminal
    char* term = getenv("TERM");
    if (term) {
        printf("%sTerminal:%s %s\n", COLOR_CYAN, COLOR_RESET, term);
    }

    // Process Count - different approaches
    char* processes = "N/A";
    if (command_exists("ps")) {
        processes = run_command("ps -e --no-headers 2>/dev/null | wc -l");
    } else if (file_exists("/proc")) {
        processes = run_command("ls -1 /proc | grep -c '^[0-9]'");
    }
    printf("%sProcesses:%s %s\n", COLOR_CYAN, COLOR_RESET, processes);

    // Package Count (support for different package managers)
    if (command_exists("dpkg")) {
        char* pkgs = run_command("dpkg --get-selections 2>/dev/null | wc -l");
        printf("%sPackages (dpkg):%s %s\n", COLOR_CYAN, COLOR_RESET, pkgs);
    } else if (command_exists("rpm")) {
        char* pkgs = run_command("rpm -qa 2>/dev/null | wc -l");
        printf("%sPackages (rpm):%s %s\n", COLOR_CYAN, COLOR_RESET, pkgs);
    } else if (command_exists("pacman")) {
        char* pkgs = run_command("pacman -Q 2>/dev/null | wc -l");
        printf("%sPackages (pacman):%s %s\n", COLOR_CYAN, COLOR_RESET, pkgs);
    }

    // WSL version if applicable
    if (is_wsl()) {
        char* wsl_version = "Unknown";
        if (file_exists("/proc/sys/kernel/osrelease")) {
            wsl_version = run_command("grep -o 'Microsoft' /proc/version >/dev/null && echo 'WSL1' || echo 'WSL2'");
        }
        printf("%sWSL Version:%s %s\n", COLOR_CYAN, COLOR_RESET, wsl_version);

        // Windows Version
        char* win_version = run_command("cmd.exe /c ver 2>/dev/null | grep -o 'Version [0-9.]\\+'");
        if (strcmp(win_version, "N/A") != 0) {
            printf("%sWindows Version:%s %s\n", COLOR_CYAN, COLOR_RESET, win_version);
        }
    }
}

int main() {
    // Determine which logo to show
    char* distro = get_distro_name();
    int wsl_detected = is_wsl();

    // Display logo
    if (wsl_detected) {
        printf("%s", wsl_logo);
    } else if (strstr(distro, "ubuntu")) {
        printf("%s", ubuntu_logo);
    } else if (strstr(distro, "arch")) {
        printf("%s", arch_logo);
    } else if (strstr(distro, "fedora")) {
        printf("%s", fedora_logo);
    } else if (strstr(distro, "debian")) {
        printf("%s", debian_logo);
    } else {
        printf("%s", generic_logo);
    }

    // Add a separator line
    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_YELLOW, COLOR_RESET);

    // Display system info
    get_system_info();

    // End with another separator line
    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_YELLOW, COLOR_RESET);

    return 0;
}
