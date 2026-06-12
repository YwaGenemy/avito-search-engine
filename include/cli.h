#pragma once

#include <string>
#include <vector>

class Cli {
public:
    void Run();

private:
    std::vector<std::string> menu_ = {
        "Select index",
        "Load demo dataset",
        "Add ad",
        "Search",
        "Compare indexes",
        "Stats",
        "Exit"
    };
    std::size_t selected_ = 0;
    bool running_ = true;
    std::string active_index_ = "FlatVector";
    std::size_t loaded_ads_ = 0;
    std::string message_ = "j/k + Enter to move, Enter to select, q + Enter to quit";

    void RenderMenu() const;
    void HandleInput(char input);
    void ActivateSelected();
};
