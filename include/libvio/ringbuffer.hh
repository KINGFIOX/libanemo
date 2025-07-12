#ifndef LIBVIO_RINGBUFFER_HH
#define LIBVIO_RINGBUFFER_HH

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace libvio {

template <typename T>
class ringbuffer;

template <typename T>  
class ringbuffer_iterator;

template <typename T>  
class ringbuffer_const_iterator;

template <typename T>
class ringbuffer {

    private:

        T* buffer;
        size_t max_size;
        size_t first_index;
        size_t last_index;
        
    public:

        using iterator = ringbuffer_iterator<T>;
        using const_iterator = ringbuffer_const_iterator<T>;
    
        ringbuffer(size_t n) {
            buffer = new T[n];
            max_size = n;
            first_index = 0;
            last_index = 0;
        }

        ~ringbuffer() {
            delete[] buffer;
        }

        ringbuffer<T>(const ringbuffer<T>& other) {
            buffer = new T[other.max_size];
            max_size = other.max_size;
            first_index = other.first_index;
            last_index = other.last_index;
            std::copy(other.buffer, other.buffer+max_size, buffer);
        }

        ringbuffer<T>(ringbuffer<T>&& other) {
            buffer = other.buffer;
            max_size = other.max_size;
            first_index = other.first_index;
            last_index = other.last_index;
            other.buffer = nullptr;
            other.max_size = 0;
            other.first_index = 0;
            other.last_index = 0;
        }

        ringbuffer<T>& operator=(const ringbuffer<T>& other) {
            if (this != &other) {
                delete[] buffer;
                buffer = new T[other.max_size];
                max_size = other.max_size;
                first_index = other.first_index;
                last_index = other.last_index;
                std::copy(other.buffer, other.buffer+max_size, buffer);
            }
            return *this;
        }

        size_t firstindex() const {
            return first_index;
        }

        size_t lastindex() const {
            return last_index;
        }

        T& operator[] (size_t index) {
            return buffer[index%max_size];
        }

        const T& operator[](size_t index) const {
            return buffer[index%max_size];
        }        

        void push_back(const T& value) {
            buffer[last_index%max_size] = value;
            ++last_index;
            first_index = (last_index-first_index) > max_size ? last_index-max_size : first_index;
        }

        void pop_back(void) {
            last_index = last_index > first_index ? last_index-1 : first_index;
        }

        /*
        `push_front` and `pop_front` are intentinally not provided.
        The same ringbuffer will be read by multiple consumers in some cases.
        Each consumer should maintain its own front index rather than rely on the state of the ringbuffer.
        */

        constexpr size_t capacity() const {
            return max_size;
        }

        size_t size() const {
            return last_index - first_index;
        }

        size_t empty() const {
            return first_index == last_index;
        }

        iterator begin() {
            return iterator(buffer, max_size, first_index);
        }

        iterator end() {
            return iterator(buffer, max_size, last_index);
        }

        const_iterator begin() const {
            return const_iterator(buffer, max_size, first_index);
        }

        const_iterator end() const {
            return const_iterator(buffer, max_size, last_index);
        }

        const_iterator cbegin() const {
            return begin();
        }
        
        const_iterator cend() const {
            return end();
        }
        
};

template <typename T>
class ringbuffer_iterator {
    friend class ringbuffer_const_iterator<T>;
    private:
        T *buffer;
        size_t max_size;
        size_t index;
    public:
        using iterator_category = std::forward_iterator_tag;  
        using value_type = T;  
        using difference_type = std::ptrdiff_t;  
        using pointer = T*;  
        using reference = T&;
        ringbuffer_iterator(T* buffer, size_t max_size, size_t index): buffer(buffer), max_size(max_size), index(index) {}
        ringbuffer_iterator& operator++() {++index; return *this;}
        T& operator*() const {return buffer[index%max_size];}
        template<typename other_iter>
        bool operator!=(const other_iter& other) const {
            return index != other.index || buffer != other.buffer;
        }
};

template <typename T>  
class ringbuffer_const_iterator {
private:
    const T* buffer;
    size_t max_size;
    size_t index;
public:
    ringbuffer_const_iterator(const ringbuffer_iterator<T>& other): buffer(other.buffer), max_size(other.max_size), index(other.index) {}
    using iterator_category = std::forward_iterator_tag;
    using value_type = const T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T*;
    using reference = const T&;
    ringbuffer_const_iterator(const T* buffer, size_t max_size, size_t index): buffer(buffer), max_size(max_size), index(index) {}
    ringbuffer_const_iterator& operator++() {++index; return *this;}
    const T& operator*() const {return buffer[index%max_size];}
    template<typename other_iter>
    bool operator!=(const other_iter& other) const {
        return index != other.index || buffer != other.buffer;
    }
};


}

#endif
