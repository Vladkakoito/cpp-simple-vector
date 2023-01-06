#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iterator>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity)
    :capacity_(capacity) {}
    size_t Capacity() {
        return capacity_;
    }
private:
    size_t capacity_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size)
    :size_(size), capacity_(size){
        ArrayPtr<Type> tmp(size_);

        auto end_of_tmp = std::next(tmp.Get(), size_);
        for (auto it = tmp.Get(); it != end_of_tmp; std::advance(it, 1)) {
            *it = std::move(Type());
        }

        simple_vector_.swap(tmp);
    }

    SimpleVector(size_t size, const Type& value) 
    :size_(size), capacity_(size) {
        ArrayPtr<Type> tmp(size_);

        std::fill(tmp.Get(), std::next(tmp.Get(), size_), value);

        simple_vector_.swap(tmp);
    }

    SimpleVector(std::initializer_list<Type> init) {
        capacity_ = size_ = init.size();
        ArrayPtr<Type> tmp(size_);

        std::copy(init.begin(), init.end(), tmp.Get());
        simple_vector_.swap(tmp);
    }

    SimpleVector(const SimpleVector& other) {
        ArrayPtr<Type> tmp(other.GetSize());

        std::copy(other.begin(), other.end(), tmp.Get());
        simple_vector_.swap(tmp);
        size_ = other.size_;
        capacity_ = other.capacity_;
    }

    SimpleVector(ReserveProxyObj cap) {
        ArrayPtr<Type> tmp(cap.Capacity());

        simple_vector_.swap(tmp);
        capacity_ = cap.Capacity();
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (&*this == &rhs) {
            return *this;
        }

        SimpleVector<Type> tmp(rhs);
        swap(tmp);
        return *this;
    }

    SimpleVector(SimpleVector&& other) {
        SimpleVector<Type> tmp(other.GetSize());

        std::move(other.begin(), other.end(), tmp.begin());
        swap(tmp);
        other.size_ = 0;
    }

    void PushBack(const Type& item) {
        if (size_ < capacity_) {
            simple_vector_[size_] = item;
            ++size_;
            return;
        }

        size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        SimpleVector<Type> tmp(new_capacity);

        std::copy(begin(), end(), tmp.begin());
        tmp[size_] = item;

        simple_vector_.swap(tmp.simple_vector_);
        capacity_ = new_capacity;
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            simple_vector_[size_] = std::exchange(item, 0);
            ++size_;
            return;
        }

        size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        SimpleVector<Type> tmp(new_capacity);

        std::move(begin(), end(), tmp.begin());
        tmp[size_] = std::exchange(item, 0);

        simple_vector_.swap(tmp.simple_vector_);
        capacity_ = new_capacity;
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& item) {
        Iterator new_pos = &(*this)[pos - begin()];

        if (size_ < capacity_) {

            std::copy_backward(new_pos, end(), end() + 1);
            *new_pos = item;
            ++size_;
            return new_pos;
        }

        size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        SimpleVector<Type> tmp(new_capacity);
        Iterator new_pos_tmp = tmp.begin() + (new_pos - begin());

        std::copy(begin(), new_pos, tmp.begin());
        std::copy(new_pos, end(), new_pos_tmp + 1);

        *new_pos_tmp = item;
        new_pos = new_pos_tmp;
        simple_vector_.swap(tmp.simple_vector_);

        capacity_ = new_capacity;
        ++size_;
        return new_pos;
    }

    Iterator Insert(ConstIterator pos, Type&& item) {
        Iterator new_pos = &(*this)[pos - begin()];

        if (size_ < capacity_) {

            std::move_backward(new_pos, end(), end() + 1);
            *new_pos = std::exchange(item, 0);
            ++size_;
            return new_pos;
        }

        size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        SimpleVector<Type> tmp(new_capacity);
        Iterator new_pos_tmp = tmp.begin() + (new_pos - begin());

        std::move(begin(), new_pos, tmp.begin());
        std::move(new_pos, end(), new_pos_tmp + 1);

        *new_pos_tmp = std::exchange(item, 0);
        new_pos = new_pos_tmp;
        simple_vector_.swap(tmp.simple_vector_);

        capacity_ = new_capacity;
        ++size_;
        return new_pos;
    }

    void PopBack() noexcept {
        size_ = size_ == 0 ? 0 : size_ - 1;
    }

    Iterator Erase(ConstIterator pos) {
        Iterator new_pos = &(*this)[pos - begin()];
        std::move(new_pos + 1, end(), new_pos);
        --size_;
        return new_pos;
    }

    void swap(SimpleVector& other) noexcept {
        simple_vector_.swap(other.simple_vector_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        return simple_vector_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        return const_cast<Type&>(simple_vector_[index]);
    }

    Type& At(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("");
        }
        return simple_vector_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("");
        }
        return const_cast<Type&>(simple_vector_[index]);
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size <= size_) {
            size_ = new_size;
            return;
        }

        ArrayPtr<Type> tmp(new_size);
        auto end_of_tmp = std::next(tmp.Get(), new_size);

        for (auto it = std::move(begin(), end(), tmp.Get()); it != end_of_tmp; std::advance(it, 1)) {
            *it = std::move(Type());
        }

        simple_vector_.swap(tmp);
        capacity_ = size_ = new_size;
    }

    Iterator begin() noexcept {
        return simple_vector_.Get();
    }

    Iterator end() noexcept {
        return std::next(simple_vector_.Get(), size_);
    }

    ConstIterator begin() const noexcept {
        return const_cast<Type*>(simple_vector_.Get());
    }

    ConstIterator end() const noexcept {
        return const_cast<Type*>(std::next(simple_vector_.Get(), size_));
    }

    ConstIterator cbegin() const noexcept {
        return const_cast<Type*>(simple_vector_.Get());
    }

    ConstIterator cend() const noexcept {
        return const_cast<Type*>(std::next(simple_vector_.Get(), size_));
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }

        ArrayPtr<Type> tmp(new_capacity);
        if (size_ != 0) {
            std::move(begin(), end(), tmp.Get());
        }

        simple_vector_.swap(tmp);
        capacity_ = new_capacity;
    }

private:
    ArrayPtr<Type> simple_vector_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    if (lhs.GetSize() != rhs.GetSize()) {
        return false;
    }
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (lhs < rhs) || (lhs == rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}