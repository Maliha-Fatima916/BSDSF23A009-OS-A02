/*
* Programming Assignment 02: lsv1.2.0
* Added column display (down then across)
* Added -l option for long listing format
*/
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

// Function prototypes
void do_ls_simple(const char *dir);
void do_ls_long(const char *dir);
void do_ls_columns(const char *dir);
void print_long_format(const char *dir, const char *filename);
int get_terminal_width();
void print_in_columns(char **filenames, int count, int terminal_width);

int main(int argc, char *argv[])
{
    int long_listing = 0;
    
    // Simple argument parsing
    if (argc > 1 && strcmp(argv[1], "-l") == 0) {
        long_listing = 1;
    }
    
    const char *dir = ".";
    if (long_listing && argc > 2) {
        dir = argv[2];
    } else if (!long_listing && argc > 1) {
        dir = argv[1];
    }
    
    if (long_listing) {
        do_ls_long(dir);
    } else {
        do_ls_columns(dir);  // Changed from do_ls_simple to do_ls_columns
    }
    
    return 0;
}

// Get terminal width using ioctl
int get_terminal_width()
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 80;  // Fallback to 80 columns if ioctl fails
    }
    return w.ws_col;
}

// Print files in column format (down then across)
void print_in_columns(char **filenames, int count, int terminal_width)
{
    if (count == 0) return;
    
    // Find the longest filename
    int max_length = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_length) {
            max_length = len;
        }
    }
    
    // Calculate column layout
    int column_width = max_length + 2;  // Add 2 spaces between columns
    int columns = terminal_width / column_width;
    if (columns == 0) columns = 1;  // At least 1 column
    if (columns > count) columns = count;
	    
    int rows = (count + columns - 1) / columns;  // Ceiling division
    
    // Print in "down then across" order
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            int index = row + col * rows;
            if (index < count) {
                // For all but the last column, print with padding
                if (col < columns - 1) {
                    printf("%-*s", max_length + 2, filenames[index]);
                } else {
                    // Last column - no extra padding
                    printf("%s", filenames[index]);
                }
            }
        }
        printf("\n");
    }
}

// New column display function
void do_ls_columns(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }
    
    // First pass: count files and get names
    int count = 0;
    int capacity = 100;
    char **filenames = malloc(capacity * sizeof(char *));
    
    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        
        // Resize array if needed
        if (count >= capacity) {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
        }
        
        // Allocate and copy filename
        filenames[count] = malloc(strlen(entry->d_name) + 1);
        strcpy(filenames[count], entry->d_name);
        count++;
    }
    
    if (errno != 0) {
        perror("readdir failed");
    }
    closedir(dp);
    
    // Get terminal width and print in columns
    int terminal_width = get_terminal_width();
    print_in_columns(filenames, count, terminal_width);
    
    // Free allocated memory
    for (int i = 0; i < count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

// Keep the existing long listing functions (unchanged)
void do_ls_long(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }
    
    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        print_long_format(dir, entry->d_name);
    }

    if (errno != 0) {
        perror("readdir failed");
    }
    closedir(dp);
}

void print_long_format(const char *dir, const char *filename)
{
    struct stat statbuf;
    char path[1024];
    char modestr[11] = "----------";
    char timebuf[80];
    struct tm *timeinfo;
    
    // Construct full path
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    
    // Get file status
    if (stat(path, &statbuf) == -1) {
        perror("stat failed");
        return;
    }
    
    // Simple permission string
    if (S_ISDIR(statbuf.st_mode)) modestr[0] = 'd';
    if (S_ISLNK(statbuf.st_mode)) modestr[0] = 'l';
    
    // User permissions
    if (statbuf.st_mode & S_IRUSR) modestr[1] = 'r';
    if (statbuf.st_mode & S_IWUSR) modestr[2] = 'w';
    if (statbuf.st_mode & S_IXUSR) modestr[3] = 'x';
    
    // Group permissions
    if (statbuf.st_mode & S_IRGRP) modestr[4] = 'r';
    if (statbuf.st_mode & S_IWGRP) modestr[5] = 'w';
    if (statbuf.st_mode & S_IXGRP) modestr[6] = 'x';
    
    // Other permissions
    if (statbuf.st_mode & S_IROTH) modestr[7] = 'r';
    if (statbuf.st_mode & S_IWOTH) modestr[8] = 'w';
    if (statbuf.st_mode & S_IXOTH) modestr[9] = 'x';
    
    // Get user and group names
    struct passwd *pwd = getpwuid(statbuf.st_uid);
    struct group *grp = getgrgid(statbuf.st_gid);
    
    // Format time
    timeinfo = localtime(&statbuf.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", timeinfo);
    
    // Print long format
    printf("%s %2ld %-8s %-8s %8ld %s %s\n",
           modestr,
           statbuf.st_nlink,
           pwd ? pwd->pw_name : "unknown",
           grp ? grp->gr_name : "unknown",
           statbuf.st_size,
           timebuf,
           filename);
}
