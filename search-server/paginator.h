#pragma once
#include <vector>
#include <iostream>

using namespace std;
template <typename Iterator>
class IteratorRange  {
    public: 
    IteratorRange(Iterator begg, Iterator endd, size_t size) 
                :begg_(begg),
                 endd_(endd),
                 size_(size)
    { }
       auto begin() const {
        return begg_;
       }

       auto end() const {
        return endd_;
       }

       auto size() const {
        return size_;
       }
    
    private:
    Iterator begg_;
    Iterator endd_;
    size_t size_;
}; 


template <typename Iterator>
class Paginator  {  
public:
    Paginator(Iterator begin, Iterator end, size_t size) {
        bool isbiggerthensize = 1;
        auto tempbegin = begin;
        while (isbiggerthensize) {
        if (distance(tempbegin, end) <= static_cast <int> (size)) {
                pages.push_back(IteratorRange<Iterator>{tempbegin, end, static_cast<size_t>(distance(tempbegin, end))});
                isbiggerthensize = 0;
        }
        else {
            
            advance(tempbegin, size);
            pages.push_back(IteratorRange<Iterator>{begin,tempbegin, size});

        }
        }
    }

        auto begin() const {
        return pages.begin();
       }

       auto end() const {
        return pages.end();
       }

    

private:
vector <IteratorRange<Iterator>> pages;

};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
ostream& operator<<(ostream& os, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        os << *it;

    }
    return os;
}
