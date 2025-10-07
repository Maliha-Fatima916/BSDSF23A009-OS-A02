# Operating Systems Programming Assignment - 02 Report
# BSDSF23A009-OS-A02


## Feature 2: Long Listing Format Report

### 1. Difference between stat() and lstat()
The crucial difference between `stat()` and `lstat()` is in how they handle symbolic links. `stat()` follows symbolic links and returns information about the target file, while `lstat()` returns information about the symbolic link itself. In the context of the ls command, it's more appropriate to use `lstat()` when we want to detect and properly handle symbolic links without following them.

### 2. Using st_mode with Bitwise Operators
The `st_mode` field in `struct stat` is an integer that contains both file type and permission bits. We can use bitwise operators and predefined macros to extract this information:
- **File Type**: Use macros like `S_ISDIR(mode)`, `S_ISREG(mode)`, `S_ISLNK(mode)` to check file type
- **Permissions**: Use bitwise AND with masks like `S_IRUSR`, `S_IWUSR`, `S_IXUSR` to check specific permission bits
- **Example**: `if (mode & S_IRUSR)` checks if the owner has read permission

---

## Feature 3: Column Display Report

### 1. "Down Then Across" Printing Logic
The "down then across" logic requires pre-calculation because we need to print files column by column, not row by row. A simple loop through filenames would print them horizontally (across then down), but we need to print the first item of each column first, then the second item of each column, etc. This requires calculating the grid layout (rows and columns) before printing.

### 2. ioctl System Call Purpose
The ioctl system call with `TIOCGWINSZ` request gets the terminal dimensions (width and height). Without it, we'd have to use a fixed width (like 80 columns), which wouldn't adapt to different terminal sizes or user preferences. The limitation of using a fixed-width fallback is that the output wouldn't be optimized for the user's current terminal size.

---

## Feature 4: Horizontal Display Report

### 1. Implementation Complexity Comparison
The "down then across" (vertical) logic requires more pre-calculation because we need to determine the entire grid layout (rows and columns) before printing. The "across" (horizontal) logic is simpler because we just print files sequentially and wrap when we reach the terminal width, without needing complex grid calculations.

### 2. Display Mode Management Strategy
We used an integer flag (`display_mode`) with constants for each mode (`DISPLAY_LONG`, `DISPLAY_COLUMN`, `DISPLAY_HORIZONTAL`). The `getopt()` loop sets this flag based on command-line options, and then a switch statement calls the appropriate display function based on the mode.

---

## Feature 5: Alphabetical Sort Report

### 1. Why Read All Entries Into Memory Before Sorting?
We need to read all directory entries into memory first because sorting requires having the complete set of data. We can't sort files as we read them one by one - we need to see all filenames to determine their correct alphabetical order. The potential drawback is that for directories containing millions of files, this could use significant memory.

### 2. qsort() Comparison Function
The comparison function takes two `const void*` arguments because `qsort()` is a generic sorting function that can sort any data type. We cast these to `char**` (pointer to string pointer) and then use `strcmp()` to compare the actual strings. The function returns negative if the first string should come before, zero if equal, or positive if the first string should come after the second string.

---

## Feature 6: Colorized Output Report

### 1. ANSI Escape Codes for Color
ANSI escape codes are special character sequences that control terminal formatting. To print text in green:
- Start with `\033[0;32m` (green foreground)
- Print your text
- End with `\033[0m` (reset to default)

Example: `printf("\033[0;32m%s\033[0m", "executable");`

### 2. Checking Executable Permission Bits
To determine if a file is executable, we check the `st_mode` field using bitwise AND:
- Owner executable: `mode & S_IXUSR`
- Group executable: `mode & S_IXGRP`  
- Others executable: `mode & S_IXOTH`
If any of these is non-zero, the file has execute permissions. We also check if it's a regular file using `S_ISREG(mode)` to avoid coloring directories as executables.

---

## Feature 7: Recursive Listing Report

### 1. Base Case in Recursive Function
The base case in our recursive ls is when we reach a directory that has no subdirectories (or only contains "." and ".."). The recursion naturally stops when we've processed all directories in the tree. We explicitly avoid infinite recursion by skipping the "." and ".." directory entries, which would otherwise create circular references.

### 2. Importance of Full Path Construction
Constructing the full path (e.g., "parent_dir/subdir") is essential because the recursive function needs the complete path to open and read the subdirectory. If we just passed "subdir", the function would look for it in the current working directory instead of the parent directory's context, leading to "directory not found" errors.

---

## Summary
This assignment successfully implemented a feature-rich ls command with: long listing format, column displays, horizontal display, alphabetical sorting, colorized output, and recursive directory traversal - all while maintaining proper Git workflow with versioned releases.
