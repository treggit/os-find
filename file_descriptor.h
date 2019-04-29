//
// Created by Anton Shelepov on 2019-04-28.
//

#ifndef OS_FIND_FILE_DESCRIPTOR_H
#define OS_FIND_FILE_DESCRIPTOR_H

#include <unistd.h>

struct file_descriptor {
    file_descriptor(int fd) : fd(fd) {}
    ~file_descriptor() {
        if (is_valid()) {
            close(fd);
        }
    }

    bool is_valid() {
        return fd != -1;
    }

    int get() {
        return fd;
    }

private:
    int fd = -1;
};

#endif //OS_FIND_FILE_DESCRIPTOR_H
