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
              << "  debug              Show debug information\n"
              << "  interactive        Start interactive mode\n"
              << "  version            Show YNS version\n"
              << "  updateyns          Update YNS to latest version\n\n"
              << "Interactive Mode:\n"
              << "  Run 'yns interactive' to enter interactive mode where you can\n"
              << "  execute multiple commands without prefix. Type 'help' in\n"
              << "  interactive mode for available commands.\n";
}

bool needs_sudo(const std::string& command) {
    return command == "update" || command == "install" || 
           command == "remove" || command == "upgrade" || 
           command == "interactive" || command == "list" ||
           command == "debug" || command == "version" || command == "updateyns";
}

std::string get_self_path() {
    char path[1024];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
    return std::string(path, (count > 0) ? count : 0);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: No command provided" << std::endl;
        std::cout << "Usage: yns <command> [package_name]" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    PackageManager pm;

    try {
        if (command == "update") {
            pm.update();
        } else if (command == "install" && argc == 3) {
            pm.install(argv[2]);
        } else if (command == "remove" && argc == 3) {
            pm.remove(argv[2]);
        } else if (command == "upgrade" && argc == 3) {
            pm.upgrade(argv[2]);
        } else if (command == "list") {
            pm.list();
        } else if (command == "debug") {
            pm.debug();
        } else if (command == "interactive") {
            pm.interactive_mode();
        } else if (command == "version") {
            pm.version();
        } else if (command == "updateyns") {
            pm.updateYns();
        } else {
            std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
            std::cout << "Usage: yns <command> [package_name]" << std::endl;
            std::cout << "\nCommands:\n";
            std::cout << "  update              Update package cache\n";
            std::cout << "  install <package>   Install a package\n";
            std::cout << "  remove <package>    Remove a package\n";
            std::cout << "  upgrade <package>   Upgrade a package\n";
            std::cout << "  list               List all packages\n";
            std::cout << "  debug              Show debug information\n";
            std::cout << "  interactive        Start interactive mode\n";
            std::cout << "  version            Show YNS version\n";
            std::cout << "  updateyns          Update YNS to latest version\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 