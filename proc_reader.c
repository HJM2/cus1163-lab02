#include "proc_reader.h"
#include <errno.h>

static void replace_nuls_with_spaces(char *buf, ssize_t n) {
    for (ssize_t i = 0; i < n; i++) {
        if (buf[i] == '\0') buf[i] = ' ';
    }
}

int list_process_directories(void) {
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("opendir(/proc)");
        return -1;
    }

    struct dirent *ent;
    int count = 0;

    printf("Process directories in /proc:\n");
    printf("%-8s %-20s\n", "PID", "Type");
    printf("%-8s %-20s\n", "---", "----");

    while ((ent = readdir(dir)) != NULL) {
        if (is_number(ent->d_name)) {
            printf("%-8s %-20s\n", ent->d_name, "process");
            count++;
        }
    }

    if (closedir(dir) == -1) {
        perror("closedir(/proc)");
        return -1;
    }

    printf("\nTotal process directories found: %d\n", count);
    return 0;
}

int read_process_info(const char* pid) {
    char filepath[256];

    printf("\n--- Process Information for PID %s ---\n", pid);

    // /proc/[pid]/status
    if (snprintf(filepath, sizeof(filepath), "/proc/%s/status", pid) >= (int)sizeof(filepath)) {
        fprintf(stderr, "Path too long\n");
        return -1;
    }
    if (read_file_with_syscalls(filepath) == -1) {
        fprintf(stderr, "\n[error] failed to read %s\n", filepath);
        return -1;
    }

    // /proc/[pid]/cmdline
    if (snprintf(filepath, sizeof(filepath), "/proc/%s/cmdline", pid) >= (int)sizeof(filepath)) {
        fprintf(stderr, "Path too long\n");
        return -1;
    }
    printf("\n--- Command Line ---\n");
    if (read_file_with_syscalls(filepath) == -1) {
        fprintf(stderr, "\n[error] failed to read %s\n", filepath);
        return -1;
    }

    printf("\n"); // readability
    return 0;
}

int show_system_info(void) {
    const int MAX_LINES = 10;
    char line[1024];
    FILE *fp = NULL;

    printf("\n--- CPU Information (first %d lines) ---\n", MAX_LINES);
    fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        perror("fopen(/proc/cpuinfo)");
        return -1;
    }
    for (int i = 0; i < MAX_LINES && fgets(line, sizeof(line), fp); i++) {
        fputs(line, stdout);
    }
    if (fclose(fp) == EOF) {
        perror("fclose(/proc/cpuinfo)");
        return -1;
    }

    printf("\n--- Memory Information (first %d lines) ---\n", MAX_LINES);
    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("fopen(/proc/meminfo)");
        return -1;
    }
    for (int i = 0; i < MAX_LINES && fgets(line, sizeof(line), fp); i++) {
        fputs(line, stdout);
    }
    if (fclose(fp) == EOF) {
        perror("fclose(/proc/meminfo)");
        return -1;
    }

    return 0;
}

void compare_file_methods(void) {
    const char* test_file = "/proc/version";

    printf("Comparing file reading methods for: %s\n\n", test_file);

    printf("=== Method 1: Using System Calls ===\n");
    read_file_with_syscalls(test_file);

    printf("\n=== Method 2: Using Library Functions ===\n");
    read_file_with_library(test_file);

    printf("\nNOTE: Run this program with strace to see the difference!\n");
    printf("Example: strace -e trace=openat,read,write,close ./lab2\n");
}

int read_file_with_syscalls(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    char buffer[4096];
    while (1) {
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n > 0) {
            // /proc/[pid]/cmdline uses NUL separators; make it readable.
            replace_nuls_with_spaces(buffer, n);
            ssize_t written = 0;
            while (written < n) {
                ssize_t w = write(STDOUT_FILENO, buffer + written, n - written);
                if (w == -1) {
                    perror("write");
                    close(fd);
                    return -1;
                }
                written += w;
            }
        } else if (n == 0) {
            break; // EOF
        } else {
            perror("read");
            close(fd);
            return -1;
        }
    }

    if (close(fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}

int read_file_with_library(const char* filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        fputs(line, stdout);
    }

    if (fclose(fp) == EOF) {
        perror("fclose");
        return -1;
    }
    return 0;
}

int is_number(const char* str) {
    if (!str || *str == '\0') return 0;
    const unsigned char *p = (const unsigned char *)str;
    while (*p) {
        if (!isdigit(*p)) return 0;
        p++;
    }
    return 1;
}
