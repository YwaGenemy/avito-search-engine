#pragma once

#include <cstdint>
#include <string>

struct Ad {
    int id = 0;

    std::string title;
    std::string description;
    std::string category;

    std::string Text() const { return title + ' ' + description; }

    void SetTermsCount(size_t count) { terms_count_ = count; }

    size_t GetTermsCount() const { return terms_count_; }

   private:
    size_t terms_count_;
};
