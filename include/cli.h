#pragma once

#include "flat_vector.h"
#include "bm25_index.h"

#include "storage.h"

#include <string>
#include <unordered_map>
#include <vector>

class Cli {
public:
    Cli();
    void Run();

private:
    bool running_ = true;
    std::string active_index_ = "FlatVector";
    std::string current_query_;
    std::vector<std::string> active_paths_;
    std::vector<std::string> filters_;
    std::unordered_map<IdType, std::string> flat_file_paths_;
    std::unordered_map<IdType, std::string> bm25_file_paths_;

    DocumentStorage bm25_storage_;
    DocumentStorage flat_storage_;

    FlatVectorIndex flat_index_;
    Bm25Index bm25_index_;

    // active choice
    DocumentStorage* storage_ = nullptr;
    Index* index_ = nullptr; 
    std::unordered_map<IdType, std::string>* file_paths_ = nullptr;

    void Execute(const std::string& line);
    void Help() const;
    void Status() const;
    void Stats() const;
    void SetIndex(const std::string& name);
    void SetFilter(const std::string& args);
    void Load(const std::string& path);
    void Unload();
    void Clear();
    void Search(const std::string& query);

    void ImportMemory();
    void SwitchMemory(const std::string& name);
};
