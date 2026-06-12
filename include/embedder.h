#pragma once

#include "index.h"
#include "index_stats.h"
#include "ad.h"

#include <cstddef>
#include <string>
#include <vector>
#include <math.h>

class Embedder{
private:
    size_t dimension_ = 0;
public:
    explicit Embedder(size_t dimension = 128);
        
    std::vector<float> Embed(const std::string& text) const;
    size_t Dimension()const;
};
