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

static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
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
    if (!curl) {
        print_error("Failed to initialize CURL");
        return false;
    }
    
    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    char error_buffer[CURL_ERROR_SIZE];
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        print_error("Failed to download: " + std::string(error_buffer));
        return false;
    }
    
    std::ofstream output_file(output_path, std::ios::trunc);
    if (!output_file) {
        print_error("Failed to open file for writing: " + output_path);
        return false;
    }
    
    output_file << response_data;
    output_file.close();
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
        return false;
    }
    
    try {
        std::ifstream cache_file(CACHE_FILE);
        if (!cache_file) {
            print_error("Failed to read downloaded cache file");
            return false;
        }
        repo_cache = json::parse(cache_file);
        print_progress("Updating package cache", 100);
        print_success("Package cache updated successfully");
        return true;
    } catch (const std::exception& e) {
        print_error("Failed to parse repository data: " + std::string(e.what()));
        return false;
    }
}

json PackageManager::read_cache() {
    try {
        std::ifstream cache_file(CACHE_FILE);
        if (!cache_file) {
            print_error("Cache not found. Run 'yns update' first");
            return json::object();
        }
        return json::parse(cache_file);
    } catch (const std::exception& e) {
        print_error("Failed to read cache: " + std::string(e.what()));
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
    const int bar_width = 50;
    int filled_width = bar_width * percentage / 100;
    
    std::cout << BLUE << "\r[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled_width) std::cout << "=";
        else if (i == filled_width) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << percentage << "% " << message << RESET;
    std::cout.flush();
    if (percentage == 100) std::cout << std::endl;
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

bool PackageManager::confirm_action(const std::string& action) {
    std::cout << YELLOW << "Do you want to " << action << "? [y/N] " << RESET;
    std::string response;
    std::getline(std::cin, response);
    return (response == "y" || response == "Y");
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
            std::cout << "\nNew version available: " + repo_version + " (currently installed: " + installed_version + ")\n";
            if (!confirm_action("upgrade to version " + repo_version)) {
                print_success("Keeping current version " + installed_version);
                return true;
            }
            return upgrade(package_name);
        }
    }

    if (!confirm_action("install " + package_name + " version " + repo_version)) {
        std::cout << "Installation cancelled.\n";
        return false;
    }
    
    std::string install_script = package["install"];
    std::string temp_script = "/tmp/yns_install_" + package_name + ".sh";
    print_progress("Downloading installation script", 0);
    
    if (!download_file(install_script, temp_script)) {
        print_error("Failed to download installation script");
        return false;
    }

    print_progress("Downloading installation script", 100);
    print_progress("Installing " + package_name + "@" + repo_version, 0);
    
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

    print_progress("Installing " + package_name + "@" + repo_version, 100);
    
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

    std::string version = installed_packages[package_name]["version"];
    if (!confirm_action("remove " + package_name + " version " + version)) {
        std::cout << "Removal cancelled.\n";
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
    
    print_progress("Downloading removal script", 100);
    print_progress("Removing " + package_name, 0);
    
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

    print_progress("Removing " + package_name, 100);
    
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

bool PackageManager::debug() {
    std::cout << "\nRepository URL: " << REPO_URL << "\n";
    std::cout << "Cache file: " << CACHE_FILE << "\n";
    std::cout << "Installed DB: " << INSTALLED_DB << "\n\n";

    std::cout << "Cache contents:\n";
    std::cout << "===============\n";
    try {
        std::ifstream cache_file(CACHE_FILE);
        if (!cache_file) {
            print_error("Cache file not found");
            return false;
        }
        json cache = json::parse(cache_file);
        std::cout << cache.dump(2) << "\n\n";
    } catch (const std::exception& e) {
        print_error("Failed to read cache: " + std::string(e.what()));
        return false;
    }

    std::cout << "Installed packages:\n";
    std::cout << "==================\n";
    try {
        std::ifstream db_file(INSTALLED_DB);
        if (!db_file) {
            print_error("No installed packages database found");
            return false;
        }
        json db = json::parse(db_file);
        std::cout << db.dump(2) << "\n";
    } catch (const std::exception& e) {
        print_error("Failed to read installed packages: " + std::string(e.what()));
        return false;
    }

    return true;
}

void PackageManager::version() {
    std::cout << "YNS Package Manager version " << VERSION << std::endl;
}

void PackageManager::updateYns() {
    std::cout << "Checking for YNS updates..." << std::endl;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        print_error("Failed to initialize CURL");
        return;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/spitkov/ynspkg/releases/latest");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "YNS Package Manager");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    print_progress("Checking for updates", 0);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        print_error("Failed to check for updates");
        return;
    }

    try {
        json release = json::parse(response);
        std::string latest_version = release["tag_name"];
        if (latest_version[0] == 'v') {
            latest_version = latest_version.substr(1);
        }

        print_progress("Checking for updates", 100);

        if (latest_version == VERSION) {
            print_success("No new YNS version found. Latest version is " + std::string(VERSION));
            return;
        }

        std::cout << "\nNew version " << latest_version << " found (current: " << VERSION << ")" << std::endl;
        
        if (!confirm_action("update YNS to version " + latest_version)) {
            std::cout << "Update cancelled.\n";
            return;
        }

        print_progress("Downloading update", 0);

        std::string download_url = release["assets"][0]["browser_download_url"];
        std::string temp_file = "/tmp/yns_update";
        
        curl = curl_easy_init();
        FILE* fp = fopen(temp_file.c_str(), "wb");
        if (!fp || !curl) {
            print_error("Failed to prepare update");
            if (fp) fclose(fp);
            if (curl) curl_easy_cleanup(curl);
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, download_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        res = curl_easy_perform(curl);
        fclose(fp);
        curl_easy_cleanup(curl);

        print_progress("Downloading update", 100);

        if (res != CURLE_OK) {
            print_error("Failed to download update");
            return;
        }

        print_progress("Installing update", 0);

        std::string cmd = "chmod +x " + temp_file + " && ";
        cmd += "sudo cp " + temp_file + " /usr/bin/yns && ";
        cmd += "sudo cp " + temp_file + " /bin/yns && ";
        cmd += "rm " + temp_file;

        if (system(cmd.c_str()) != 0) {
            print_error("Failed to install update");
            return;
        }

        print_progress("Installing update", 100);
        print_success("Successfully updated to version " + latest_version);
    } catch (const std::exception& e) {
        print_error("Failed to parse update information");
    }
} 