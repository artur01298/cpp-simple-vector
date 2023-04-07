#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <iterator>

#include "array_ptr.h"


class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity_to_reserve) : capacity_(capacity_to_reserve) {}
    size_t Reserve_capacity() { return capacity_; }
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

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) : SimpleVector(size, std::move(Type{})) {}

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) : simple_vector_(size), size_(size), capacity_(size)
    {
        std::fill(begin(), end(), value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) : simple_vector_(init.size()), size_(init.size()), capacity_(init.size())
    {
        std::copy(std::move_iterator(init.begin()), std::move_iterator(init.end()), begin());
    }

    SimpleVector(const SimpleVector& other) {
        ArrayPtr<Type> replicator(other.size_);
        size_ = other.size_;
        capacity_ = other.capacity_;
        std::copy(other.begin(), other.end(), &replicator[0]);
        simple_vector_.swap(replicator);
    }

    SimpleVector(SimpleVector&& other) : simple_vector_(other.size_)
    {
        size_ = std::move(other.size_);
        capacity_ = std::move(other.capacity_);
        simple_vector_.swap(other.simple_vector_);
        other.Clear();
    }

    SimpleVector(ReserveProxyObj capacity_to_reserve)
    {
        Reserve(capacity_to_reserve.Reserve_capacity());
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs)
        {
            SimpleVector replicator(rhs);
            swap(replicator);
        }
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (capacity_ > size_)
        {
            simple_vector_[size_++] = item;
            return;
        }
        if (capacity_ > 0)
        {
            ArrayPtr<Type> replicator(capacity_ *= 2);
            std::copy(begin(), end(), &replicator[0]);
            simple_vector_.swap(replicator);
            simple_vector_[size_++] = item;
        }
        else
        {
            ArrayPtr<Type> replicator(++capacity_);
            std::copy(begin(), end(), &replicator[0]);
            simple_vector_.swap(replicator);
            simple_vector_[size_++] = item;
            return;
        }
    }

    void PushBack(Type&& item) {
        if (capacity_ > size_)
        {
            simple_vector_[size_++] = std::move(item);
            return;
        }
        if (capacity_ > 0)
        {
            ArrayPtr<Type> replicator(capacity_ *= 2);
            std::move(begin(), end(), &replicator[0]);
            simple_vector_.swap(replicator);
            simple_vector_[size_++] = std::move(item);
        }
        else
        {
            ArrayPtr<Type> replicator(++capacity_);
            std::move(begin(), end(), &replicator[0]);
            simple_vector_.swap(replicator);
            simple_vector_[size_++] = std::move(item);
            return;
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        if ((begin() <= pos) && (end() >= pos))
        {
            auto interval = std::distance(cbegin(), pos);
            if (size_ == capacity_)
            {
                if (size_ > 0)
                {
                    ArrayPtr<Type> replicator(size_ *= 2);
                    std::copy(begin(), end(), &replicator[0]);
                    simple_vector_.swap(replicator);
                    size_ = size_;
                    capacity_ = size_ * 2;
                }
                else
                {
                    ArrayPtr<Type> replicator(++capacity_);
                    std::copy(begin(), end(), &replicator[0]);
                    simple_vector_.swap(replicator);
                    capacity_ = 1;
                }
            }
            for (size_t i = size_; i > (size_t)interval; --i)
            {
                simple_vector_[i] = simple_vector_[i - 1];
            }
            size_++;
            simple_vector_[interval] = value;
            return const_cast<Iterator>(interval + begin());
        }
        else
        {
            throw std::out_of_range("Nonexistent vector element.");
        }
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        if ((begin() <= pos) && (end() >= pos))
        {
            if (capacity_ == 0)
            {
                ArrayPtr<Type> replicator(++capacity_);
                std::move(begin(), end(), &replicator[0]);
                simple_vector_.swap(replicator);
                simple_vector_[size_++] = std::move(value);
                return begin();
            }
            else if (capacity_ <= size_)
            {
                auto interval = std::distance(begin(), const_cast<Iterator>(pos));
                ArrayPtr<Type> replicator(capacity_ *= 2);
                std::move(begin(), end(), &replicator[0]);
                std::copy_backward(std::make_move_iterator(const_cast<Iterator>(pos)), std::make_move_iterator(begin() + size_), (&replicator[1 + size_]));
                replicator[interval] = std::move(value);
                size_++;
                simple_vector_.swap(replicator);
                return Iterator(&simple_vector_[interval]);
            }
            else
            {
                std::copy_backward(std::make_move_iterator(const_cast<Iterator>(pos)), std::make_move_iterator(end()), (&simple_vector_[++size_ + 1]));
                *const_cast<Iterator>(pos) = std::move(value);
                return const_cast<Iterator>(pos);
            }
        }
        else
        {
            throw std::out_of_range("Nonexistent vector element.");
        }
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_ > 0)
        {
            size_--;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        if (begin() <= pos && end() >= pos)
        {
            auto interval = std::distance(cbegin(), pos);
            std::move(&simple_vector_[interval + 1], end(), const_cast<Iterator>(pos));
            size_--;
            return const_cast<Iterator>(pos);
        }
        else
        {
            throw std::out_of_range("Nonexistent vector element.");
        }
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        simple_vector_.swap(other.simple_vector_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity > capacity_)
        {
            ArrayPtr<Type> replicator(new_capacity);
            std::copy(begin(), end(), &replicator[0]);
            simple_vector_.swap(replicator);
            capacity_ = new_capacity;
        }
        else
        {
            return;
        }
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return simple_vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return simple_vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_)
        {
            throw std::out_of_range("Nonexistent vector element.");
        }
        return simple_vector_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_)
        {
            throw std::out_of_range("Nonexistent vector element.");
        }
        return simple_vector_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (size_ >= new_size)
        {
            size_ = new_size;
            return;
        }
        else if ((size_ < new_size) && (capacity_ > new_size))
        {
            for (auto iter = begin() + new_size; iter != end(); --iter) {
                *iter = std::move(Type{});
            }
            size_ = new_size;
            return;
        }
        else
        {
            ArrayPtr<Type> replicator(new_size);
            std::move(begin(), end(), &replicator[0]);
            simple_vector_.swap(replicator);
            size_ = new_size;
            capacity_ = new_size * 2;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator(&simple_vector_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator(&simple_vector_[size_]);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return cend();
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return ConstIterator(&simple_vector_[0]);
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return ConstIterator(&simple_vector_[size_]);
    }
private:
    ArrayPtr<Type> simple_vector_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
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
    return ((lhs < rhs) || (lhs == rhs));
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}