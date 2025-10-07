/*
* Programming Assignment 02: lsv1.4.0
* Added alphabetical sorting
* Added horizontal column display with -x option
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
void do_ls_horizontal(const char *dir);
void print_long_format(const char *dir, const char *filename);
int get_terminal_width();
void print_in_columns(char **filenames, int count, int terminal_width);
void print_horizontal(char **filenames, int count, int terminal_width);
int compare_strings(const void *a, const void *b);

// Display mode constants
#define DISPLAY_SIMPLE 0
#define DISPLAY_LONG   1
#define DISPLAY_COLUMN 2
#define DISPLAY_HORIZONTAL 3

int main(int argc, char *argv[])
{
    int display_mode = DISPLAY_COLUMN;  // Default: column display
    
    // Parse command line options
    int opt;
    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                display_mode = DISPLAY_LONG;
                break;
            case 'x':
                display_mode = DISPLAY_HORIZONTAL;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    const char *dir = ".";
    if (optind < argc) {
        dir = argv[optind];
    }
    
    // Call appropriate display function based on mode
    switch (display_mode) {
        case DISPLAY_LONG:
            do_ls_long(dir);
            break;
        case DISPLAY_HORIZONTAL:
            do_ls_horizontal(dir);
            break;
        case DISPLAY_COLUMN:
        default:
            do_ls_columns(dir);
            break;
    }
    
    return 0;
}

// Comparison function for qsort (alphabetical order)
int compare_strings(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
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

// Print files in horizontal format (across then down)
void print_horizontal(char **filenames, int count, int terminal_width)
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
    
    int current_pos = 0;
    
    for (int i = 0; i < count; i++) {
        int needed_width = strlen(filenames[i]) + 2;
        
        // Check if we need a new line
        if (current_pos > 0 && current_pos + needed_width > terminal_width) {
            printf("\n");
            current_pos = 0;
        }
        
        // Print the filename with padding
        printf("%s", filenames[i]);
        current_pos += strlen(filenames[i]);
        
        // Add spacing (except for last file)
        if (i < count - 1) {
            // Calculate how much space to add
            int spaces = (max_length + 2) - strlen(filenames[i]);
            if (current_pos + spaces <= terminal_width) {
                printf("%*s", spaces, "");
                current_pos += spaces;
            } else {
                // If we can't fit the full spacing, just add minimal space
                printf(" ");
                current_pos += 1;
            }
        }
    }
    printf("\n");
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
    int column_width = max_length + 2;
    int columns = terminal_width / column_width;
    if (columns == 0) columns = 1;
    if (columns > count) columns = count;
    
    int rows = (count + columns - 1) / columns;
    
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

// Common function to read directory entries and sort them
int read_directory_entries(const char *dir, char ***filenames_ptr)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return 0;
    }
    
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
    
    // SORT THE FILENAMES ALPHABETICALLY
    if (count > 0) {
        qsort(filenames, count, sizeof(char *), compare_strings);
    }
    
    *filenames_ptr = filenames;
    return count;
}

// Free allocated filenames array
void free_filenames(char **filenames, int count)
{
    for (int i = 0; i < count; i++) {
        free(filenames[i]);
    }
    free(filenames);
}

// Horizontal display function
void do_ls_horizontal(const char *dir)
{
    char **filenames;
    int count = read_directory_entries(dir, &filenames);
    
    if (count > 0) {
        int terminal_width = get_terminal_width();
        print_horizontal(filenames, count, terminal_width);
    }
    
    free_filenames(filenames, count);
}

// Column display function
void do_ls_columns(const char *dir)
{
    char **filenames;
    int count = read_directory_entries(dir, &filenames);
    
    if (count > 0) {
        int terminal_width = get_terminal_width();
        print_in_columns(filenames, count, terminal_width);
    }
    
    free_filenames(filenames, count);
}

// Long listing function (needs to be updated for sorting)
void do_ls_long(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }
    
    // First, read all entries and sort them
    int count = 0;
    int capacity = 100;
    char **filenames = malloc(capacity * sizeof(char *));
    
    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        
        if (count >= capacity) {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
        }
        
        filenames[count] = malloc(strlen(entry->d_name) + 1);
        strcpy(filenames[count], entry->d_name);
        count++;
    }
    
    if (errno != 0) {
        perror("readdir failed");
    }
    closedir(dp);
    
    // SORT THE FILENAMES for long listing too
    if (count > 0) {
        qsort(filenames, count, sizeof(char *), compare_strings);
    }
    
    // Print in long format (sorted)
    for (int i = 0; i < count; i++) {
        print_long_format(dir, filenames[i]);
        free(filenames[i]);
    }
    free(filenames);
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
