#include "../include/embedder.h"

namespace {
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

float LengthVec(std::vector<float> &vector){
    double sum2 = 0.0;
    for(int i=0;i<vector.size();i++){
        sum2 += vector[i]*vector[i];
    }
    return static_cast<float>(std::sqrt(sum2));
}

void Normalize(std::vector<float> &vector){
    float lenght = LengthVec(vector);
    if(lenght == 0.0f)return;
    for(int i=0;i<vector.size();i++)vector[i] /= lenght;
}

} //namespace


Embedder::Embedder(size_t dimension):
    dimension_(dimension){}
    
std::vector<float> Embedder::Embed(const std::string& text)const{
    std::vector<float> vector(dimension_, 0.0f);
    std::vector<std::string> tokens = Tokenize(text);

    std::hash<std::string> hasher;
    for(auto& token: tokens)vector[hasher(token) % dimension_] += 1.0;

    Normalize(vector);
    return vector;
}

std::size_t Embedder::Dimension()const{
    return dimension_;
}
