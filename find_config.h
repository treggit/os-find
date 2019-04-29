//
// Created by Anton Shelepov on 2019-04-28.
//

#ifndef OS_FIND_FIND_CONFIG_H
#define OS_FIND_FIND_CONFIG_H

#include <string>
#include <optional>
#include <sys/types.h>
#include "linux_dirent.h"
#include <sys/stat.h>

struct find_config {

    enum size_options {
        EQUAL, LESS, GREATER
    };

    std::string path;
    std::optional<ino_t> inum;
    std::optional<std::string> name;
    std::optional<size_options> size_option;
    off_t size;
    std::optional<nlink_t> nlinks;
    std::optional<std::string> exec_path;

    bool accepts(std::string const& path, std::string const& filename) const {
        struct stat buf;
        if (stat(path.c_str(), &buf) == -1) {
            return false;
        }

        if (inum && inum != buf.st_ino) {
            return false;
        }

        if (name && name != filename) {
            return false;
        }

        if (size_option) {
            if (size_option == EQUAL && size != buf.st_size) {
                return false;
            }
            if (size_option == LESS && size <= buf.st_size) {
                return false;
            }

            if (size_option == GREATER && size >= buf.st_size) {
                return false;
            }
        }

        if (nlinks && nlinks != buf.st_nlink) {
            return false;
        }

        return true;
    }

};

#endif //OS_FIND_FIND_CONFIG_H
