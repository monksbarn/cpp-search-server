#pragma once
#include <ostream>
#include <vector>

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(const Iterator& range_begin, const Iterator& range_end) {
        range_begin_ = range_begin;
        range_end_ = range_end;
    }
    Iterator begin() const {
        return range_begin_;
    }
    Iterator end() const {
        return range_end_;
    }
    auto size() const {
        return distance(range_begin_, range_end_);
    }
private:
    Iterator range_begin_;
    Iterator range_end_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(const Iterator& range_begin, const Iterator& range_end, std::size_t page_size) {
        std::size_t distance_begin_end = distance(range_begin, range_end);
        if (distance_begin_end > page_size) {
            size_ = page_size;
            Iterator first = range_begin;
            for (std::size_t i = 0;i < distance_begin_end;i += page_size) {
                advance(first, i);
                Iterator second = first;
                advance(second, page_size);
                if (distance(first, second) < distance(first, range_end)) {
                    iterator_ranges_.push_back(IteratorRange(first, second));
                }
                else {
                    first = range_begin;
                    advance(first, i);
                    iterator_ranges_.push_back(IteratorRange(first, range_end));
                }
            }
        }
        else {
            iterator_ranges_.push_back(IteratorRange(range_begin, range_end));
            size_ = distance(range_begin, range_end);
        }
    }
    auto begin() const {
        return iterator_ranges_.begin();
    }
    auto end() const {
        return iterator_ranges_.end();
    }
    std::size_t size() const {
        return size_;
    }
private:
    std::vector<IteratorRange<Iterator>> iterator_ranges_;
    std::size_t size_;
};

template <typename Container>
auto Paginate(const Container& c, std::size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& range) {
    for (auto i = range.begin();i != range.end();++i) {
        std::cout << *i;
    }
    return out;
}