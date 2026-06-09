#include "embedder.h"

namespace{
std::vector<std::string> Tokenize(const std::string& text){
    std::vector<std::string> tokens;
    std::string current;

    for(char symbol: text){
        if(std::isalnum(static_cast<unsigned char>(symbol))){ // скипаем знаки.
            current += static_cast<char>(std::tolower(static_cast<unsigned char>(symbol)));
        } else if(!current.empty()){
            tokens.push_back(current);
            current.clear();
        }
    }

    if(!current.empty()) tokens.push_back(current);

    return tokens;
}

} //namespace


class Embedder{
private:
    size_t dimension_ = 0;
public:
    explicit Embedder(size_t dimension = 128):
        dimension_(dimension){}
        
    std::vector<float> Embed(const std::string& text){
        std::vector<float> vector(dimension_, 0.0f);
        std::vector<std::string> tokens = Tokenize(text);

        std::hash<std::string> hasher;
        for(auto& token: tokens){
            vector[hasher(token)] += 1.0;
        }
    }

    size_t Dimension()const{
        return dimension_;
    }
};
