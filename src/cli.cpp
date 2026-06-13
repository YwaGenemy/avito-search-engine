#include "../include/cli.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace {
void ShowBanner(){
    std::cout << R"(
   ____                 __     ____          _         
  / __/__ ___ _________/ /    / __/__  ___ _(_)__  ___ 
 _\ \/ -_) _ `/ __/ __/ _ \  / _// _ \/ _ `/ / _ \/ -_)
/___/\__/\_,_/_/  \__/_//_/ /___/_//_/\_, /_/_//_/\__/ 
                                     /___/              
)";
    std::cout << "BM25 | FlatVector | HNSW\n\n";
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
        std::cout << "\n>  ";
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
    } else { std::cout << "unknown command: " << command << '\n'; }
}

void Cli::Help() const{
    std::cout << "commands:\n"
              << "  /load <path>          load file or directory\n"
              << "  /status               show index, memory and active paths\n"
              << "  /filter <categories>  set category filters\n"
              << "  /unload               clear loaded paths and index\n"
              << "  /help                 show commands\n"
              << "  /stats                show index stats\n"
              << "  /index <flat|bm25|hnsw>\n"
              << "  /clear                reset filters and current query\n"
              << "  <query>               search in loaded documents\n"
              << "  /exit                 quit\n";
}

void Cli::Status() const{
    std::cout << "active index: " << active_index_ << '\n';
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
        std::cout << "active index: FlatVector\n";
    } else if(name == "bm25"){
        active_index_ = "BM25";
        std::cout << "BM25 is selected, but implementation is not connected yet\n";
    } else if(name == "hnsw"){
        active_index_ = "HNSW";
        std::cout << "HNSW is selected, but implementation is not connected yet\n";
    } else {
        std::cout << "usage: /index <flat|bm25|hnsw>\n";
    }
}

void Cli::SetFilter(const std::string& args){
    filters_ = SplitWords(args);
    PrintFilters(filters_);
}

void Cli::Load(const std::string& path_str){
    if(path_str.empty()){
        std::cout << "usage: /load <path>\n";
        return;
    }
    if(active_index_ != "FlatVector"){
        std::cout << "only FlatVector is connected now\n";
        return;
    }

    const std::filesystem::path path(path_str);
    if(!std::filesystem::exists(path)){
        std::cout << "path not found: " << path_str << '\n';
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
        loaded++;
    }

    if(loaded > 0){
        active_paths_.push_back(path_str);
    }

    std::cout << "loaded files: " << loaded << '\n';
}

void Cli::Unload(){
    active_paths_.clear();
    filters_.clear();
    current_query_.clear();
    flat_index_.Clear();
    std::cout << "index and active paths cleared\n";
}

void Cli::Clear(){
    filters_.clear();
    current_query_.clear();
    std::cout << "filters and current query cleared\n";
}

void Cli::Search(const std::string& query){
    const auto clean_query = Trim(query);
    if(clean_query.empty() && current_query_.empty()){
        std::cout << "usage: type query without slash\n";
        return;
    }
    if(active_index_ != "FlatVector"){
        std::cout << "only FlatVector search is connected now\n";
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
        std::cout << "no results\n";
        return;
    }

    for(const auto& result : results){
        const auto ad = flat_index_.Get(result.ad_id);
        if(!ad.has_value())continue;

        std::cout << '[' << result.ad_id << "] "
                  << ad->title
                  << " | category: " << ad->category
                  << " | score: " << std::fixed << std::setprecision(4) << result.score
                  << '\n';
    }
}
