/*
* Programming Assignment 02: lsv1.6.0
* Added recursive listing with -R option
* Added colorized output based on file type
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

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_CYAN    "\033[0;36m"
#define COLOR_REVERSE "\033[7m"

// Function prototypes
void do_ls(const char *dir, int display_mode, int recursive, int is_first);
void do_ls_long(const char *dir, int recursive, int is_first);
void do_ls_columns(const char *dir, int recursive, int is_first);
void do_ls_horizontal(const char *dir, int recursive, int is_first);
void print_long_format(const char *dir, const char *filename);
int get_terminal_width();
void print_in_columns(const char *dir, char **filenames, int count, int terminal_width);
void print_horizontal(const char *dir, char **filenames, int count, int terminal_width);
int compare_strings(const void *a, const void *b);
const char *get_color_code(const char *filename, mode_t mode);
void print_colored_name(const char *filename, mode_t mode);
int read_directory_entries(const char *dir, char ***filenames_ptr, int include_dots);
void free_filenames(char **filenames, int count);

// Display mode constants
#define DISPLAY_SIMPLE 0
#define DISPLAY_LONG   1
#define DISPLAY_COLUMN 2
#define DISPLAY_HORIZONTAL 3

int main(int argc, char *argv[])
{
    int display_mode = DISPLAY_COLUMN;  // Default: column display
    int recursive = 0;                  // -R flag
    
    // Parse command line options
    int opt;
    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l':
                display_mode = DISPLAY_LONG;
                break;
            case 'x':
                display_mode = DISPLAY_HORIZONTAL;
                break;
            case 'R':
                recursive = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    // If no directories specified, use current directory
    if (optind >= argc) {
        do_ls(".", display_mode, recursive, 1);
    } else {
        // Process each directory
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1) {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], display_mode, recursive, 1);
            if (argc - optind > 1 && i < argc - 1) {
                printf("\n");
            }
        }
    }
    
    return 0;
}

// Main directory listing function (now supports recursion)
void do_ls(const char *dir, int display_mode, int recursive, int is_first)
{
    // Print directory header (for recursive mode or multiple directories)
    if (!is_first || recursive) {
        printf("\n%s:\n", dir);
    }
    
    // Call appropriate display function based on mode
    switch (display_mode) {
        case DISPLAY_LONG:
            do_ls_long(dir, recursive, is_first);
            break;
        case DISPLAY_HORIZONTAL:
            do_ls_horizontal(dir, recursive, is_first);
            break;
        case DISPLAY_COLUMN:
        default:
            do_ls_columns(dir, recursive, is_first);
            break;
    }
}

// Get color code based on file type and permissions
const char *get_color_code(const char *filename, mode_t mode)
{
    // Directories - Blue
    if (S_ISDIR(mode)) {
        return COLOR_BLUE;
    }
    
    // Symbolic links - Magenta (Pink)
    if (S_ISLNK(mode)) {
        return COLOR_MAGENTA;
    }
    
    // Executable files - Green
    if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH) {
        return COLOR_GREEN;
    }
    
    // Archive files - Red (check by extension)
    const char *ext = strrchr(filename, '.');
    if (ext != NULL) {
        if (strcmp(ext, ".tar") == 0 || strcmp(ext, ".gz") == 0 || 
            strcmp(ext, ".zip") == 0 || strcmp(ext, ".rar") == 0 ||
            strcmp(ext, ".7z") == 0 || strcmp(ext, ".bz2") == 0) {
            return COLOR_RED;
        }
    }
    
    // Special files (character/block devices, sockets, FIFOs) - Reverse video
    if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
        return COLOR_REVERSE;
    }
    
    // Regular files - No color (use reset)
    return COLOR_RESET;
}

// Print filename with appropriate color
void print_colored_name(const char *filename, mode_t mode)
{
    const char *color = get_color_code(filename, mode);
    printf("%s%s%s", color, filename, COLOR_RESET);
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

// Print files in horizontal format (across then down) - COLORIZED
void print_horizontal(const char *dir, char **filenames, int count, int terminal_width)
{
    if (count == 0) return;
    
    int current_pos = 0;
    char fullpath[1024];
    
    for (int i = 0; i < count; i++) {
        // Construct full path for stat - handle root directory case
        if (strcmp(dir, "/") == 0) {
            snprintf(fullpath, sizeof(fullpath), "/%s", filenames[i]);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, filenames[i]);
        }
        
        // Get file stats for color determination
        struct stat statbuf;
        if (lstat(fullpath, &statbuf) == -1) {
            // If stat fails, just print without color
            printf("%s", filenames[i]);
        } else {
            print_colored_name(filenames[i], statbuf.st_mode);
        }
        
        current_pos += strlen(filenames[i]);
        
        // Add spacing (except for last file)
        if (i < count - 1) {
            printf("  ");
            current_pos += 2;
            
            // Check if we need to wrap
            if (current_pos + strlen(filenames[i+1]) > terminal_width) {
                printf("\n");
                current_pos = 0;
            }
        }
    }
    printf("\n");
}

// Print files in column format (down then across) - COLORIZED
void print_in_columns(const char *dir, char **filenames, int count, int terminal_width)
{
    if (count == 0) return;
    
    // Find the longest filename (for column alignment)
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
    
    char fullpath[1024];
    
    // Print in "down then across" order
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            int index = row + col * rows;
            if (index < count) {
                // Construct full path for stat - handle root directory case
                if (strcmp(dir, "/") == 0) {
                    snprintf(fullpath, sizeof(fullpath), "/%s", filenames[index]);
                } else {
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, filenames[index]);
                }
                
                // Get file stats for color - use lstat to handle symlinks correctly
                struct stat statbuf;
                if (lstat(fullpath, &statbuf) == -1) {
                    // If stat fails, print without color
                    printf("%-*s", max_length + 2, filenames[index]);
                } else {
                    // Print with color and proper padding
                    const char *color = get_color_code(filenames[index], statbuf.st_mode);
                    printf("%s%-*s%s", color, max_length + 2, filenames[index], COLOR_RESET);
                }
            }
        }
        printf("\n");
    }
}

// Common function to read directory entries and sort them
int read_directory_entries(const char *dir, char ***filenames_ptr, int include_dots)
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
        if (!include_dots && entry->d_name[0] == '.')
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

// Horizontal display function with recursion support
void do_ls_horizontal(const char *dir, int recursive, int is_first)
{
    char **filenames;
    int count = read_directory_entries(dir, &filenames, 0);  // Don't include dot files
    
    if (count > 0) {
        int terminal_width = get_terminal_width();
        print_horizontal(dir, filenames, count, terminal_width);
    }
    
    // RECURSIVE PART: Process subdirectories
    if (recursive) {
        char **all_filenames;
        int all_count = read_directory_entries(dir, &all_filenames, 1);  // Include dot files to find all directories
        
        for (int i = 0; i < all_count; i++) {
            char fullpath[1024];
            if (strcmp(dir, "/") == 0) {
                snprintf(fullpath, sizeof(fullpath), "/%s", all_filenames[i]);
            } else {
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, all_filenames[i]);
            }
            
            // Check if it's a directory (and not . or ..)
            struct stat statbuf;
            if (lstat(fullpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
                if (strcmp(all_filenames[i], ".") != 0 && strcmp(all_filenames[i], "..") != 0) {
                    // Recursive call for subdirectory
                    do_ls(fullpath, DISPLAY_HORIZONTAL, recursive, 0);
                }
            }
            free(all_filenames[i]);
        }
        free(all_filenames);
    }
    
    free_filenames(filenames, count);
}

// Column display function with recursion support
void do_ls_columns(const char *dir, int recursive, int is_first)
{
    char **filenames;
    int count = read_directory_entries(dir, &filenames, 0);  // Don't include dot files
    
    if (count > 0) {
        int terminal_width = get_terminal_width();
        print_in_columns(dir, filenames, count, terminal_width);
    }
    
    // RECURSIVE PART: Process subdirectories
    if (recursive) {
        char **all_filenames;
        int all_count = read_directory_entries(dir, &all_filenames, 1);  // Include dot files to find all directories
        
        for (int i = 0; i < all_count; i++) {
            char fullpath[1024];
            if (strcmp(dir, "/") == 0) {
                snprintf(fullpath, sizeof(fullpath), "/%s", all_filenames[i]);
            } else {
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, all_filenames[i]);
            }
            
            // Check if it's a directory (and not . or ..)
            struct stat statbuf;
            if (lstat(fullpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
                if (strcmp(all_filenames[i], ".") != 0 && strcmp(all_filenames[i], "..") != 0) {
                    // Recursive call for subdirectory
                    do_ls(fullpath, DISPLAY_COLUMN, recursive, 0);
                }
            }
            free(all_filenames[i]);
        }
        free(all_filenames);
    }
    
    free_filenames(filenames, count);
}

// Long listing function with recursion support
void do_ls_long(const char *dir, int recursive, int is_first)
{
    char **filenames;
    int count = read_directory_entries(dir, &filenames, 0);  // Don't include dot files
    
    // Print in long format (sorted)
    for (int i = 0; i < count; i++) {
        print_long_format(dir, filenames[i]);
        free(filenames[i]);
    }
    free(filenames);
    
    // RECURSIVE PART: Process subdirectories
    if (recursive) {
        char **all_filenames;
        int all_count = read_directory_entries(dir, &all_filenames, 1);  // Include dot files to find all directories
        
        for (int i = 0; i < all_count; i++) {
            char fullpath[1024];
            if (strcmp(dir, "/") == 0) {
                snprintf(fullpath, sizeof(fullpath), "/%s", all_filenames[i]);
            } else {
                snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, all_filenames[i]);
            }
            
            // Check if it's a directory (and not . or ..)
            struct stat statbuf;
            if (lstat(fullpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
                if (strcmp(all_filenames[i], ".") != 0 && strcmp(all_filenames[i], "..") != 0) {
                    // Recursive call for subdirectory
                    do_ls(fullpath, DISPLAY_LONG, recursive, 0);
                }
            }
            free(all_filenames[i]);
        }
        free(all_filenames);
    }
}

// Updated long format printing with colors
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
    if (lstat(path, &statbuf) == -1) {
        perror("lstat failed");
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
    
    // Print long format (metadata without color)
    printf("%s %2ld %-8s %-8s %8ld %s ",
           modestr,
           statbuf.st_nlink,
           pwd ? pwd->pw_name : "unknown",
           grp ? grp->gr_name : "unknown",
           statbuf.st_size,
           timebuf);
    
    // Print filename WITH COLOR
    print_colored_name(filename, statbuf.st_mode);
    
    // If it's a symbolic link, show where it points
    if (S_ISLNK(statbuf.st_mode)) {
        char linkbuf[1024];
        ssize_t len = readlink(path, linkbuf, sizeof(linkbuf) - 1);
        if (len != -1) {
            linkbuf[len] = '\0';
            printf(" -> %s", linkbuf);
        }
    }
    printf("\n");
}
