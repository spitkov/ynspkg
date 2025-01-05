#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class PackageManager {
public:
    PackageManager();
    
    bool update();
    bool install(const std::string& package_name);
    bool remove(const std::string& package_name);
    bool upgrade(const std::string& package_name);
    bool list();
    bool interactive_mode();
    
private:
    static constexpr const char* REPO_URL = "https://raw.githubusercontent.com/spitkov/ynsrepo/refs/heads/main/repo.json";
    static constexpr const char* CACHE_DIR = "/var/cache/yns/";
    static constexpr const char* CACHE_FILE = "/var/cache/yns/repo.json";
    static constexpr const char* INSTALLED_DB = "/var/lib/yns/installed.json";
    
    bool download_file(const std::string& url, const std::string& output_path);
    bool execute_script(const std::string& script_path);
    bool cache_repo();
    json read_cache();
    json read_installed_db();
    void save_installed_db(const json& db);
    void print_progress(const std::string& message, int percentage);
    void print_error(const std::string& message);
    void print_success(const std::string& message);
    void print_interactive_help();
    
    json repo_cache;
    json installed_packages;
}; 