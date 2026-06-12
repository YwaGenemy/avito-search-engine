#pragma once

#include "flat_vector.h"
#include "storage.h"

#include <string>
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
    DocumentStorage storage_;
    FlatVectorIndex flat_index_;

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
};
