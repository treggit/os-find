//
// Created by Anton Shelepov on 2019-04-29.
//

#ifndef OS_FIND_LINUX_DIRENT_H
#define OS_FIND_LINUX_DIRENT_H

struct linux_dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[];
};

#endif //OS_FIND_LINUX_DIRENT_H
