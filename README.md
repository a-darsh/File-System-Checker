# File-System-Checker

## Project Description
This project involves developing a file system checker, named fcheck, that validates the consistency of an xv6 file system image. The program is designed to operate on an image file, performing a series of tests to ensure the integrity of the file system structure. The following points provide a detailed overview of the project requirements and the specific conditions the checker program will verify:

1. Validates that each inode is either unallocated or one of the valid types (T_FILE, T_DIR, T_DEV). If not, it prints ERROR: bad inode.
2. Ensures that for in-use inodes, each block address used by the inode is valid. If not, it prints appropriate error messages for bad direct or indirect addresses.
3. Checks if the root directory exists, its inode number is 1, and the parent of the root directory is itself. If these conditions are not met, it prints ERROR: root directory does not exist.
4. Ensures that each directory contains . and .. entries, and the . entry points to the directory itself. If not, it prints ERROR: directory not properly formatted.
5. Validates that for in-use inodes, each block address in use is also marked in use in the bitmap. If not, it prints ERROR: address used by inode but marked free in bitmap.
6. Checks that blocks marked in-use in the bitmap are actually in use in an inode or indirect block somewhere. If not, it prints ERROR: bitmap marks block in use but it is not in use.
7. Confirms that for in-use inodes, each direct address in use is only used once. If not, it prints ERROR: direct address used more than once.
8. Ensures that for in-use inodes, each indirect address in use is only used once. If not, it prints ERROR: indirect address used more than once.
9. Checks that for all inodes marked in use, each must be referred to in at least one directory. If not, it prints ERROR: inode marked use but not found in a directory.
10. Verifies that for each inode number referred to in a valid directory, it is actually marked in use. If not, it prints ERROR: inode referred to in directory but marked free.
11. Ensures that reference counts (number of links) for regular files match the number of times file is referred to in directories (i.e., hard links work correctly). If not, it prints ERROR: bad reference count for file.
12. Ensures that no extra links are allowed for directories (each directory only appears in one other directory). If not, it prints ERROR: directory appears more than once in file system.

## Compilation
The fcheck program can be compiled using the GCC (GNU Compiler Collection) using the following command:

gcc fcheck.c -o fcheck -Wall -Werror -O

This command compiles the fcheck.c source file, outputs the compiled program as fcheck, enables all compiler's warnings with -Wall, treats warnings as errors with -Werror, and applies the standard optimization with -O.
