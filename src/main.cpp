#include "package_manager.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

void print_usage() {
    std::cout << "Usage: yns <command> [package_name]\n\n"
              << "Commands:\n"
              << "  update              Update package cache\n"
              << "  install <package>   Install a package\n"
              << "  remove <package>    Remove a package\n"
              << "  upgrade <package>   Upgrade a package\n"
              << "  list               List all packages\n"
              << "  interactive        Start interactive mode\n\n"
              << "Interactive Mode:\n"
              << "  Run 'yns interactive' to enter interactive mode where you can\n"
              << "  execute multiple commands without prefix. Type 'help' in\n"
              << "  interactive mode for available commands.\n";
}

bool needs_sudo(const std::string& command) {
    return command == "update" || command == "install" || 
           command == "remove" || command == "upgrade" || 
           command == "interactive" || command == "list";
}

std::string get_self_path() {
    char path[1024];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
    return std::string(path, (count > 0) ? count : 0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    
    if (needs_sudo(command) && geteuid() != 0) {
        std::string self = get_self_path();
        std::string sudo_cmd = "sudo " + self;
        
        for (int i = 1; i < argc; i++) {
            sudo_cmd += " ";
            sudo_cmd += argv[i];
        }
        
        std::cout << "This operation requires root privileges.\n";
        return system(sudo_cmd.c_str());
    }

    PackageManager pm;

    try {
        if (command == "update") {
            return pm.update() ? 0 : 1;
        }
        else if (command == "list") {
            return pm.list() ? 0 : 1;
        }
        else if (command == "interactive") {
            return pm.interactive_mode() ? 0 : 1;
        }
        else if (command == "install" || command == "remove" || command == "upgrade") {
            if (argc < 3) {
                std::cerr << "Error: Package name required for " << command << " command\n";
                print_usage();
                return 1;
            }
            
            std::string package_name = argv[2];
            
            if (command == "install") {
                return pm.install(package_name) ? 0 : 1;
            }
            else if (command == "remove") {
                return pm.remove(package_name) ? 0 : 1;
            }
            else {
                return pm.upgrade(package_name) ? 0 : 1;
            }
        }
        else {
            std::cerr << "Error: Unknown command '" << command << "'\n";
            print_usage();
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\033[31mError: " << e.what() << "\033[0m\n";
        return 1;
    }
} 