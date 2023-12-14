#pragma once

#include <valarray>

template<typename T>
class Array2 {
public:
    Array2()
        : width_{0}
        , height_{0}
        , array_(0)
    {}
    Array2(size_t width, size_t height, T value)
        : width_{ width }
        , height_{ height }
        , array_(value, width * height)
    {}

    size_t width() const { return width_; }
    size_t height() const { return height_; }
    size_t size() const { return width_ * height_; }

    T& operator()(size_t x, size_t y) {return array_[y * width_ + x];}
    const T& operator()(size_t x, size_t y) const {return array_[y * width_ + x];}

    T& operator[](size_t i) { return array_[i]; }
    const T& operator[](size_t i) const { return array_[i]; }

    T* data() { return std::begin(array_); }
    const T* data() const { return std::begin(array_); }
    T* begin() { return data(); }
    const T* begin() const { return data(); }
    T* end() { return data() + size(); }
    const T* end() const { return data() + size(); }

private:
    size_t width_;
    size_t height_;
    std::valarray<T> array_;
};
