#include "package_manager.hpp"
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <sys/wait.h>

namespace fs = std::filesystem;

const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string BLUE = "\033[34m";
const std::string YELLOW = "\033[33m";
const std::string RESET = "\033[0m";

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

PackageManager::PackageManager() {
    fs::create_directories(CACHE_DIR);
    fs::create_directories("/var/lib/yns");
    curl_global_init(CURL_GLOBAL_DEFAULT);
    installed_packages = read_installed_db();
}

bool PackageManager::download_file(const std::string& url, const std::string& output_path) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) return false;
    
    std::ofstream output_file(output_path);
    if (!output_file) return false;
    
    output_file << response_data;
    return true;
}

bool PackageManager::execute_script(const std::string& script_path) {
    std::string cmd = "chmod +x " + script_path + " && " + script_path;
    int exit_code = system(cmd.c_str());
    if (WIFEXITED(exit_code)) {
        int status = WEXITSTATUS(exit_code);
        if (status != 0) {
            print_error("Script failed with exit code: " + std::to_string(status));
            return false;
        }
        return true;
    }
    print_error("Script terminated abnormally");
    return false;
}

bool PackageManager::cache_repo() {
    print_progress("Updating package cache", 0);
    if (!download_file(REPO_URL, CACHE_FILE)) {
        print_error("Failed to download repository data");
        return false;
    }
    print_progress("Updating package cache", 100);
    print_success("Package cache updated successfully");
    return true;
}

json PackageManager::read_cache() {
    try {
        std::ifstream cache_file(CACHE_FILE);
        if (!cache_file) return json::object();
        return json::parse(cache_file);
    } catch (...) {
        return json::object();
    }
}

json PackageManager::read_installed_db() {
    try {
        std::ifstream db_file(INSTALLED_DB);
        if (!db_file) return json::object();
        return json::parse(db_file);
    } catch (...) {
        return json::object();
    }
}

void PackageManager::save_installed_db(const json& db) {
    std::ofstream db_file(INSTALLED_DB);
    db_file << db.dump(4);
}

void PackageManager::print_progress(const std::string& message, int percentage) {
    std::cout << BLUE << "[" << percentage << "%] " << message << RESET << std::endl;
}

void PackageManager::print_error(const std::string& message) {
    std::cerr << RED << "Error: " << message << RESET << std::endl;
}

void PackageManager::print_success(const std::string& message) {
    std::cout << GREEN << message << RESET << std::endl;
}

bool PackageManager::update() {
    return cache_repo();
}

bool PackageManager::install(const std::string& package_name) {
    if (!update()) return false;
    
    repo_cache = read_cache();
    
    if (!repo_cache["packages"].contains(package_name)) {
        print_error("Package '" + package_name + "' not found");
        return false;
    }
    
    auto package = repo_cache["packages"][package_name];
    std::string repo_version = package["version"];
    
    if (installed_packages.contains(package_name)) {
        std::string installed_version = installed_packages[package_name]["version"];
        if (installed_version == repo_version) {
            print_success(package_name + " is already installed (version " + installed_version + ")");
            return true;
        } else {
            print_progress("New version available: " + repo_version + " (currently installed: " + installed_version + ")", 0);
            std::cout << "Would you like to upgrade? [y/N] ";
            std::string response;
            std::getline(std::cin, response);
            if (response == "y" || response == "Y") {
                return upgrade(package_name);
            } else {
                print_success("Keeping current version " + installed_version);
                return true;
            }
        }
    }
    
    std::string install_script = package["install"];
    std::string temp_script = "/tmp/yns_install_" + package_name + ".sh";
    print_progress("Downloading installation script", 0);
    
    if (!download_file(install_script, temp_script)) {
        print_error("Failed to download installation script");
        return false;
    }
    
    print_progress("Installing " + package_name + "@" + repo_version, 50);
    
    std::string cmd = "chmod +x " + temp_script + " && " + temp_script;
    int exit_code = system(cmd.c_str());
    fs::remove(temp_script);

    if (!WIFEXITED(exit_code)) {
        print_error("Installation script terminated abnormally");
        return false;
    }

    int status = WEXITSTATUS(exit_code);
    if (status != 0) {
        print_error("Installation failed with exit code: " + std::to_string(status));
        return false;
    }
    
    installed_packages[package_name] = {
        {"version", repo_version}
    };
    save_installed_db(installed_packages);
    
    print_success(package_name + "@" + repo_version + " installed successfully");
    return true;
}

bool PackageManager::remove(const std::string& package_name) {
    if (!installed_packages.contains(package_name)) {
        print_error("Package '" + package_name + "' is not installed");
        return false;
    }
    
    repo_cache = read_cache();
    if (!repo_cache["packages"].contains(package_name)) {
        print_error("Package information not found in repository");
        return false;
    }
    
    std::string remove_script = repo_cache["packages"][package_name]["remove"];
    std::string temp_script = "/tmp/yns_remove_" + package_name + ".sh";
    
    print_progress("Downloading removal script", 0);
    if (!download_file(remove_script, temp_script)) {
        print_error("Failed to download removal script");
        return false;
    }
    
    print_progress("Removing " + package_name, 50);
    
    std::string cmd = "chmod +x " + temp_script + " && " + temp_script;
    int exit_code = system(cmd.c_str());
    fs::remove(temp_script);

    if (!WIFEXITED(exit_code)) {
        print_error("Removal script terminated abnormally");
        return false;
    }

    int status = WEXITSTATUS(exit_code);
    if (status != 0) {
        print_error("Removal failed with exit code: " + std::to_string(status));
        return false;
    }
    
    installed_packages.erase(package_name);
    save_installed_db(installed_packages);
    
    print_success(package_name + " removed successfully");
    return true;
}

bool PackageManager::upgrade(const std::string& package_name) {
    if (!installed_packages.contains(package_name)) {
        print_error("Package '" + package_name + "' is not installed");
        return false;
    }
    
    if (!update()) return false;
    
    repo_cache = read_cache();
    if (!repo_cache["packages"].contains(package_name)) {
        print_error("Package information not found in repository");
        return false;
    }
    
    std::string installed_version = installed_packages[package_name]["version"];
    std::string repo_version = repo_cache["packages"][package_name]["version"];
    
    if (installed_version == repo_version) {
        print_success(package_name + " is already up to date (" + installed_version + ")");
        return true;
    }
    
    std::string update_script = repo_cache["packages"][package_name]["update"];
    std::string temp_script = "/tmp/yns_update_" + package_name + ".sh";
    
    print_progress("Downloading update script", 0);
    if (!download_file(update_script, temp_script)) {
        print_error("Failed to download update script");
        return false;
    }
    
    print_progress("Updating " + package_name + " from " + installed_version + " to " + repo_version, 50);
    
    std::string cmd = "chmod +x " + temp_script + " && " + temp_script;
    int exit_code = system(cmd.c_str());
    fs::remove(temp_script);

    if (!WIFEXITED(exit_code)) {
        print_error("Update script terminated abnormally");
        return false;
    }

    int status = WEXITSTATUS(exit_code);
    if (status != 0) {
        print_error("Update failed with exit code: " + std::to_string(status));
        return false;
    }
    
    installed_packages[package_name]["version"] = repo_version;
    save_installed_db(installed_packages);
    
    print_success(package_name + " updated to version " + repo_version);
    return true;
}

bool PackageManager::list() {
    if (!update()) return false;
    
    repo_cache = read_cache();
    
    std::cout << "\nAvailable packages:\n";
    std::cout << "==================\n";
    
    for (const auto& [name, package] : repo_cache["packages"].items()) {
        std::string repo_version = package["version"];
        std::string status;
        
        if (installed_packages.contains(name)) {
            std::string installed_version = installed_packages[name]["version"];
            if (installed_version == repo_version) {
                status = GREEN + "[installed " + installed_version + "]" + RESET;
            } else {
                status = YELLOW + "[installed " + installed_version + ", update available " + repo_version + "]" + RESET;
            }
        } else {
            status = BLUE + "[available " + repo_version + "]" + RESET;
        }
        
        std::cout << name << " " << status << "\n";
    }
    
    return true;
}

void PackageManager::print_interactive_help() {
    std::cout << "\nAvailable commands:\n"
              << "  help                Show this help message\n"
              << "  update              Update package cache\n"
              << "  install <package>   Install a package\n"
              << "  remove <package>    Remove a package\n"
              << "  upgrade <package>   Upgrade a package\n"
              << "  list               List all packages\n"
              << "  clear              Clear the screen\n"
              << "  exit               Exit interactive mode\n\n";
}

bool PackageManager::interactive_mode() {
    std::cout << GREEN << "YNS Package Manager Interactive Mode\n" << RESET;
    std::cout << "Type 'help' for available commands or 'exit' to quit\n";

    std::string line;
    while (true) {
        std::cout << BLUE << "yns> " << RESET;
        if (!std::getline(std::cin, line) || line == "exit") {
            std::cout << "Goodbye!\n";
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "help") {
            print_interactive_help();
        }
        else if (command == "update") {
            update();
        }
        else if (command == "list") {
            list();
        }
        else if (command == "clear") {
            system("clear");
        }
        else if (command == "install" || command == "remove" || command == "upgrade") {
            std::string package_name;
            if (!(iss >> package_name)) {
                print_error("Package name required for " + command + " command");
                continue;
            }

            if (command == "install") {
                install(package_name);
            }
            else if (command == "remove") {
                remove(package_name);
            }
            else {
                upgrade(package_name);
            }
        }
        else {
            print_error("Unknown command '" + command + "'. Type 'help' for available commands.");
        }
    }
    return true;
} 