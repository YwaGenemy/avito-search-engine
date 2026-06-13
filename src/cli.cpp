#include "../include/cli.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace {
constexpr const char* kFileColor = "\033[38;2;66;233;253m";
constexpr const char* kPromptColor = "\033[92m";
constexpr const char* kGreenColor = "\033[92m";
constexpr const char* kCyanColor = "\033[96m";
constexpr const char* kYellowColor = "\033[93m";
constexpr const char* kRedColor = "\033[91m";
constexpr const char* kGrayColor = "\033[90m";
constexpr const char* kResetColor = "\033[0m";

void ShowBanner(){
    std::cout << R"(
   ____                 __     ____          _         
  / __/__ ___ _________/ /    / __/__  ___ _(_)__  ___ 
 _\ \/ -_) _ `/ __/ __/ _ \  / _// _ \/ _ `/ / _ \/ -_)
/___/\__/\_,_/_/  \__/_//_/ /___/_//_/\_, /_/_//_/\__/ 
)";
    std::cout << kGrayColor << "[ BM25 ] [ FlatVector ] [ HNSW ]" << kResetColor << "     /___/" << "\n\n";
}

std::string Trim(const std::string& str){
    const auto left = str.find_first_not_of(" \t\n\r");
    if(left == std::string::npos)return "";

    const auto right = str.find_last_not_of(" \t\n\r");
    return str.substr(left, right - left + 1);
}

std::vector<std::string> SplitWords(const std::string& str){
    std::vector<std::string> words;
    std::istringstream stream(str);
    std::string word;
    while(stream >> word){
        words.push_back(word);
    }
    return words;
}

std::string ReadFile(const std::filesystem::path& path){
    std::ifstream file(path);
    if(!file.is_open())return "";

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string CategoryFromPath(const std::filesystem::path& path){
    const auto parent = path.parent_path().filename().string();
    if(!parent.empty())return parent;
    return "default";
}

IdType LastStorageId(const DocumentStorage& storage){
    IdType last_id = 0;
    for(const auto& [id, ad] : storage.All()){
        last_id = std::max(last_id, id);
    }
    return last_id;
}

std::string FullPath(const std::filesystem::path& path){
    return std::filesystem::absolute(path).lexically_normal().string();
}

std::string HighlightFilename(const std::string& path_str){
    const std::filesystem::path path(path_str);
    const auto filename = path.filename().string();
    const auto parent = path.parent_path().string();

    if(filename.empty())return path_str;
    if(parent.empty())return std::string(kFileColor) + filename + kResetColor;

    return parent + std::string(1, std::filesystem::path::preferred_separator) +
           kFileColor + filename + kResetColor;
}

std::string FormatBytes(std::size_t bytes){
    constexpr double kb = 1024.0;
    constexpr double mb = kb * 1024.0;

    std::ostringstream out;
    if(bytes >= static_cast<std::size_t>(mb)){
        out << std::fixed << std::setprecision(2) << bytes / mb << " MB";
    } else if(bytes >= static_cast<std::size_t>(kb)){
        out << std::fixed << std::setprecision(2) << bytes / kb << " KB";
    } else {
        out << bytes << " B";
    }
    return out.str();
}

void PrintPaths(const std::vector<std::string>& paths){
    if(paths.empty()){
        std::cout << "paths: empty\n";
        return;
    }

    std::cout << "paths:\n";
    for(const auto& path : paths){
        std::cout << "  - " << path << '\n';
    }
}

void PrintFilters(const std::vector<std::string>& filters){
    if(filters.empty()){
        std::cout << "filters: empty\n";
        return;
    }

    std::cout << "filters:";
    for(const auto& filter : filters){
        std::cout << ' ' << filter;
    }
    std::cout << '\n';
}
}

Cli::Cli()
    : flat_index_(storage_) {}

void Cli::Run(){
    ShowBanner();
    Help();

    std::string line;
    while(running_){
        std::cout << '\n' << kPromptColor << ">  " << kResetColor;
        if(!std::getline(std::cin, line))break;

        Execute(Trim(line));
    }
}

void Cli::Execute(const std::string& line){
    if(line.empty())return;

    if(line[0] != '/'){
        Search(line);
        return;
    }

    const auto space = line.find(' ');
    const std::string command = line.substr(0, space);
    const std::string args = space == std::string::npos ? "" : Trim(line.substr(space + 1));

    if(command == "/help"){ Help();
    } else if(command == "/status"){ Status();
    } else if(command == "/stats"){ Stats();
    } else if(command == "/index"){ SetIndex(args);
    } else if(command == "/filter"){ SetFilter(args);
    } else if(command == "/load"){ Load(args);
    } else if(command == "/unload"){ Unload();
    } else if(command == "/clear"){ Clear();
    } else if(command == "/exit" || command == "/quit" || command == "/q"){ running_ = false;
    } else { std::cout << kRedColor << "unknown command: " << command << kResetColor << '\n'; }
}

void Cli::Help() const{
    std::cout << "commands:\n"
              << "  " << kCyanColor << "/load " << kYellowColor << "<path>" << kGrayColor << "          load file or directory" << kResetColor << '\n'
              << "  " << kCyanColor << "/status" << kGrayColor << "               show index, memory and active paths" << kResetColor << '\n'
              << "  " << kCyanColor << "/filter " << kYellowColor << "<categories>" << kGrayColor << "  set category filters" << kResetColor << '\n'
              << "  " << kCyanColor << "/unload" << kGrayColor << "               clear loaded paths and index" << kResetColor << '\n'
              << "  " << kCyanColor << "/help" << kGrayColor << "                 show commands" << kResetColor << '\n'
              << "  " << kCyanColor << "/stats" << kGrayColor << "                show index stats" << kResetColor << '\n'
              << "  " << kCyanColor << "/index " << kYellowColor << "<flat|bm25|hnsw>" << kResetColor << '\n'
              << "  " << kCyanColor << "/clear" << kGrayColor << "                reset filters and current query" << kResetColor << '\n'
              << "  " << kCyanColor << "/exit" << kGrayColor << "                 quit" << kResetColor << '\n';
}

void Cli::Status() const{
    std::cout << "active index: " << kGreenColor << active_index_ << kResetColor << '\n';
    PrintFilters(filters_);
    PrintPaths(active_paths_);
}

void Cli::Stats() const{
    const auto stats = flat_index_.Stats();

    std::cout << "documents: " << stats.documents_count << '\n'
              << "categories: " << stats.categories_count << '\n'
              << "embedding dimension: " << stats.embedding_dimension << '\n'
              << "memory: " << FormatBytes(stats.memory_bytes) << '\n';
}

void Cli::SetIndex(const std::string& name){
    if(name == "flat" || name == "flatvector" || name == "FlatVector"){
        active_index_ = "FlatVector";
        std::cout << kGreenColor << "active index: FlatVector" << kResetColor << '\n';
    } else if(name == "bm25"){
        active_index_ = "BM25";
        std::cout << kYellowColor << "BM25 is selected, but implementation is not connected yet" << kResetColor << '\n';
    } else if(name == "hnsw"){
        active_index_ = "HNSW";
        std::cout << kYellowColor << "HNSW is selected, but implementation is not connected yet" << kResetColor << '\n';
    } else {
        std::cout << kRedColor << "usage: /index <flat|bm25|hnsw>" << kResetColor << '\n';
    }
}

void Cli::SetFilter(const std::string& args){
    filters_ = SplitWords(args);
    PrintFilters(filters_);
}

void Cli::Load(const std::string& path_str){
    if(path_str.empty()){
        std::cout << kRedColor << "usage: /load <path>" << kResetColor << '\n';
        return;
    }
    if(active_index_ != "FlatVector"){
        std::cout << kRedColor << "only FlatVector is connected now" << kResetColor << '\n';
        return;
    }

    const std::filesystem::path path(path_str);
    if(!std::filesystem::exists(path)){
        std::cout << kRedColor << "path not found: " << path_str << kResetColor << '\n';
        return;
    }

    std::vector<std::filesystem::path> files;
    if(std::filesystem::is_regular_file(path)){
        files.push_back(path);
    } else if(std::filesystem::is_directory(path)){
        for(const auto& entry : std::filesystem::recursive_directory_iterator(path)){
            if(entry.is_regular_file()){
                files.push_back(entry.path());
            }
        }
    }

    std::size_t loaded = 0;
    for(const auto& file : files){
        const auto text = ReadFile(file);
        if(text.empty())continue;

        flat_index_.Add(Ad(file.filename().string(), text, CategoryFromPath(file)));
        file_paths_[LastStorageId(storage_)] = FullPath(file);
        loaded++;
    }

    if(loaded > 0){
        active_paths_.push_back(path_str);
    }

    std::cout << kGreenColor << "loaded files: " << loaded << kResetColor << '\n';
}

void Cli::Unload(){
    active_paths_.clear();
    filters_.clear();
    current_query_.clear();
    file_paths_.clear();
    flat_index_.Clear();
    std::cout << kGreenColor << "index and active paths cleared" << kResetColor << '\n';
}

void Cli::Clear(){
    filters_.clear();
    current_query_.clear();
    std::cout << kGreenColor << "filters and current query cleared" << kResetColor << '\n';
}

void Cli::Search(const std::string& query){
    const auto clean_query = Trim(query);
    if(clean_query.empty() && current_query_.empty()){
        std::cout << kRedColor << "usage: type query without slash" << kResetColor << '\n';
        return;
    }
    if(active_index_ != "FlatVector"){
        std::cout << kRedColor << "only FlatVector search is connected now" << kResetColor << '\n';
        return;
    }

    if(!clean_query.empty()){
        current_query_ = clean_query;
    }

    SearchOptions options;
    options.top_k = 10;

    std::vector<SearchResult> results;
    if(filters_.empty()){
        results = flat_index_.Search(current_query_, options);
    } else {
        std::unordered_set<IdType> used_ids;
        for(const auto& filter : filters_){
            options.category = filter;
            for(const auto& result : flat_index_.Search(current_query_, options)){
                if(used_ids.insert(result.ad_id).second){
                    results.push_back(result);
                }
            }
        }
        std::sort(results.begin(), results.end(), [](const auto& left, const auto& right){
            return left.score > right.score;
        });
        if(results.size() > options.top_k){
            results.resize(options.top_k);
        }
    }

    if(results.empty()){
        std::cout << kGrayColor << "no results" << kResetColor << '\n';
        return;
    }

    for(const auto& result : results){
        const auto ad = flat_index_.Get(result.ad_id);
        if(!ad.has_value())continue;
        const auto path_it = file_paths_.find(result.ad_id);
        const std::string path = path_it == file_paths_.end()
            ? ad->title
            : HighlightFilename(path_it->second);

        std::cout << '[' << result.ad_id << "] "
                  << path
                  << " | category: " << ad->category
                  << " | score: " << kYellowColor << std::fixed << std::setprecision(4) << result.score << kResetColor
                  << '\n';
    }
}
