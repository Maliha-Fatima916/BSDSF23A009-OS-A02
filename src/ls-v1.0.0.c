
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>

extern int errno;

// Function prototypes
void do_ls_simple(const char *dir);
void do_ls_long(const char *dir);
void print_long_format(const char *dir, const char *filename);
void mode_to_string(mode_t mode, char *str);

int main(int argc, char *argv[])
{
    int opt;
    int long_listing = 0;

    // Parse command line options
    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // If no directories specified, use current directory
    if (optind >= argc) {
        if (long_listing) {
            do_ls_long(".");
        } else {
            do_ls_simple(".");
        }
    } else {
        // Process each directory
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1) {
                printf("%s:\n", argv[i]);
            }
            if (long_listing) {
                do_ls_long(argv[i]);
            } else {
                do_ls_simple(argv[i]);
            }
            if (argc - optind > 1 && i < argc - 1) {
                printf("\n");
            }
        }
    }
    return 0;
}

// Original simple listing function
void do_ls_simple(const char *dir)
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
        printf("%s\n", entry->d_name);
    }

    if (errno != 0) {
        perror("readdir failed");
    }
    closedir(dp);
}

// New long listing function
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

// Convert file mode to permission string
void mode_to_string(mode_t mode, char *str)
{
    // File type
    if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else if (S_ISCHR(mode)) str[0] = 'c';
    else if (S_ISBLK(mode)) str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else str[0] = '-';

    // User permissions
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    
    // Group permissions
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    
    // Other permissions
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';

    // Special bits
    if (mode & S_ISUID) str[3] = (str[3] == 'x') ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (str[6] == 'x') ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (str[9] == 'x') ? 't' : 'T';
}

// Print file information in long format
void print_long_format(const char *dir, const char *filename)
{
    struct stat statbuf;
    char path[1024];
    char modestr[11];
    char timebuf[80];
    struct tm *timeinfo;
    
    // Construct full path
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    
    // Get file status
    if (lstat(path, &statbuf) == -1) {
        perror("lstat failed");
        return;
    }
    
    // Convert mode to string
    mode_to_string(statbuf.st_mode, modestr);
    
    // Get user and group names
    struct passwd *pwd = getpwuid(statbuf.st_uid);
    struct group *grp = getgrgid(statbuf.st_gid);
    
    // Format time
    timeinfo = localtime(&statbuf.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", timeinfo);
    
    // Print long format
    printf("%s %2ld %-8s %-8s %8ld %s %s",
           modestr,
           statbuf.st_nlink,
           pwd ? pwd->pw_name : "unknown",
           grp ? grp->gr_name : "unknown",
           statbuf.st_size,
           timebuf,
           filename);
    
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
