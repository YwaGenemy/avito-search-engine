#include "../include/cli.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#include <iostream>

namespace{
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

void ClearScreen(){
    std::cout << "\033[H\033[J";
}

void EnterAlternateScreen(){
    std::cout << "\033[?1049h\033[?25l";
}

void LeaveAlternateScreen(){
    std::cout << "\033[?25h\033[?1049l";
}

char ReadKey(){
#ifdef _WIN32
    return static_cast<char>(_getch());
#else
    termios old_term{};
    tcgetattr(STDIN_FILENO, &old_term);

    termios raw = old_term;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    char key = 0;
    read(STDIN_FILENO, &key, 1);

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    return key;
#endif
}
}

void Cli::Run() {
    EnterAlternateScreen();

    while(running_){
        RenderMenu();
        HandleInput(ReadKey());
    }

    LeaveAlternateScreen();
}

void Cli::RenderMenu() const {
    ClearScreen();
    ShowBanner();

    std::cout << "+-------------------------+--------------------------------------+\n";
    std::cout << "| Menu                    | Status                               |\n";
    std::cout << "+-------------------------+--------------------------------------+\n";

    for(std::size_t i = 0; i < menu_.size(); ++i){
        std::cout << "| ";
        std::cout << (i == selected_ ? "> " : "  ");
        std::cout << menu_[i];

        for(std::size_t pad = menu_[i].size(); pad < 20; ++pad){
            std::cout << ' ';
        }

        if(i == 0){
            std::cout << "| Active index: " << active_index_;
            for(std::size_t pad = active_index_.size(); pad < 20; ++pad){
                std::cout << ' ';
            }
            std::cout << "|\n";
        } else if(i == 1){
            std::cout << "| Loaded ads: " << loaded_ads_;
            std::cout << "                         |\n";
        } else if(i == 3){
            std::cout << "| " << message_;
            if(message_.size() < 36){
                for(std::size_t pad = message_.size(); pad < 36; ++pad){
                    std::cout << ' ';
                }
            }
            std::cout << " |\n";
        } else {
            std::cout << "|                                      |\n";
        }
    }

    std::cout << "+-------------------------+--------------------------------------+\n";
    std::cout << "\n[j] down  [k] up  [Enter] select  [q] quit";
}

void Cli::HandleInput(char input) {
    if(input == 'j'){
        selected_ = (selected_ + 1) % menu_.size();
    } else if(input == 'k'){
        selected_ = (selected_ + menu_.size() - 1) % menu_.size();
    } else if(input == 'q'){
        running_ = false;
    } else if(input == '\n' || input == '\r'){
        ActivateSelected();
    } else {
        message_ = "unknown input";
    }
}

void Cli::ActivateSelected() {
    switch(selected_){
        case 0:
            message_ = "index selector is not implemented yet";
            break;
        case 1:
            loaded_ads_ = 4;
            message_ = "demo dataset loaded";
            break;
        case 2:
            message_ = "add ad is not implemented yet";
            break;
        case 3:
            message_ = "search is not implemented yet";
            break;
        case 4:
            message_ = "compare is not implemented yet";
            break;
        case 5:
            message_ = "stats is not implemented yet";
            break;
        case 6:
            running_ = false;
            break;
    }
}
