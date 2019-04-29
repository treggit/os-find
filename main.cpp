//
// Created by Anton Shelepov on 2019-04-28.
//

#include "find_config.h"
#include "file_descriptor.h"
#include "linux_dirent.h"
#include <iostream>
#include <string>
#include <optional>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <vector>
#include <cstring>
#include <sys/wait.h>

using std::cout;
using std::endl;
using std::string;

void print_usage() {
    cout << "Usage: find path [-inum num] [-name name] [-size [-=+]size] [-nlinks nlinks] [-exec path]" << endl;
}

std::optional<find_config> parse_args(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "Path expected" << endl;
        return {};
    }

    find_config config;
    config.path = argv[1];

    for (int i = 2; i < argc; i += 2) {
        if (i + 1 == argc) {
            cout << "Expected argument for option: " << argv[i] << endl;
            return {};
        }

        string option = argv[i];
        string arg = argv[i + 1];

        if (option == "-inum") {
            config.inum = static_cast<ino_t>(std::stoll(arg));
            continue;
        }

        if (option == "-name") {
            config.name = arg;
            continue;
        }

        if (option == "-size") {
            switch (arg[0]) {
                case '-': config.size_option = find_config::LESS; break;
                case '+': config.size_option = find_config::GREATER; break;
                case '=': config.size_option = find_config::EQUAL; break;
                default:
                    if (isdigit(arg[0])) {
                        config.size_option = find_config::EQUAL;
                        config.size = static_cast<off_t>(std::stoll(arg));
                        continue;
                    }
                    cout << "Can't parse -size option" << endl;
                    return {};
            }

            config.size = static_cast<off_t>(std::stoll(arg.substr(1, arg.size() - 1)));
            continue;
        }

        if (option == "-nlinks") {
            config.nlinks = static_cast<nlink_t>(std::stoll(arg));
            continue;
        }

        if (option == "-exec") {
            config.exec_path = arg;
            continue;
        }

        cout << "Unsupported option" << endl;
        return {};
    }

    return config;
}

void print_error(string const& message) {
    cout << message + ": " + strerror(errno) << endl;
}

namespace {
    const size_t BUFFER_SIZE = 4096;
}

void execute(string const& executable_path, string const& filepath) {
    const pid_t pid = fork();
    if (pid == -1) {
        print_error("Fork failed");
        return;
    }
    if (pid == 0) {
        std::vector<char*> c_args{const_cast<char*>(executable_path.data()), const_cast<char*>(filepath.data()), nullptr};
        if (execv(executable_path.data(), c_args.data()) == -1) {
            print_error("Execution failed");
            exit(-1);
        }

        exit(0);
    }

    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) == -1) {
        print_error("Execution failed");
    }
}

void process_file(string const& name, string const& path, find_config const& config) {
    if (!config.accepts(path, name)) {
        return;
    }

    if (config.exec_path) {
        execute(*config.exec_path, path);
    } else {
        cout << path << endl;
    }
}

void traverse(string const& dir, find_config const& config);

void process_dirents_buffers(char* buffer, int len, string const& dir, find_config const& config) {
    for (int bpos = 0; bpos < len; ) {
        auto dirent = reinterpret_cast<linux_dirent*>(buffer + bpos);

        if (strcmp(dirent->d_name, ".") != 0 && strcmp(dirent->d_name, "..") != 0) {
            string path = dir;
            if (path.back() != '/') {
                path += "/";
            }
            path += dirent->d_name;

            char d_type = *(buffer + bpos + dirent->d_reclen - 1);

            switch (d_type) {
                case DT_REG: process_file(dirent->d_name, path, config); break;
                case DT_DIR: traverse(path, config); break;
                case DT_BLK:
                case DT_FIFO:
                case DT_CHR:
                case DT_SOCK:
                case DT_UNKNOWN:
                case DT_LNK:
                case DT_WHT:
                default:
                    break;
            }
        }

        bpos += dirent->d_reclen;
    }
}

void traverse(string const& dir, find_config const& config) {
    file_descriptor fd = open(dir.c_str(), O_DIRECTORY | O_RDONLY);

    if (!fd.is_valid()) {
        print_error("Couldn't open dir " + dir);
        return;
    }

    linux_dirent* dirent;
    char buffer[BUFFER_SIZE];
    while (true) {
        int len = syscall(SYS_getdents, fd.get(), buffer, BUFFER_SIZE);

        if (len == -1) {
            print_error("Couldn't read dir " + dir);
            break;
        }
        if (len == 0) {
            break;
        }

        process_dirents_buffers(buffer, len, dir, config);
    }
}

int main(int argc, char* argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        print_usage();
        return 0;
    }

    auto optional_config = parse_args(argc, argv);
    if (!optional_config) {
        return 0;
    }

    find_config config = *optional_config;
    traverse(config.path, config);
    return 0;
}
