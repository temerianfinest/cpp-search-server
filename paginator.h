#pragma once

#include <iostream>
#include <vector>

template <typename Iter>
class IteratorRange {
    public:
    
    IteratorRange(Iter begin, size_t size) : range_begin(begin), range_end(begin + size)
    {}
     
    auto begin() const {
    return range_begin;
    }
    
    auto end() const {
    return range_end;
    }
    
    size_t size() const {
    return distance(range_begin, range_end);
    }
     
    private:
    
    Iter range_begin;
    Iter range_end;
};


template <typename Iter>
class Paginator {
public:
    
    Paginator(Iter begin, Iter end, size_t size) {
        for (auto i = begin; i != end; advance(i, size)) {
            if (end - i < size) {
                size -= (end - i);
            }
            pages.push_back(IteratorRange<Iter>(i, size));
        }
    }
    
    auto begin() const {
    return pages.begin();
    }
    
    auto end() const {
    return pages.end();
    }
    
    size_t size() const {
    return pages.size();
    }
    
private:
    
    std::vector<IteratorRange<Iter>> pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iter>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iter>& range) {
    for (auto i = range.begin(); i != range.end(); ++i) {
        os << *i;
    }
    return os;
}