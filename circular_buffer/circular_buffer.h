#include <iostream>
#include <array>
#include <limits>
#include <span>
#include <optional>
#include <type_traits>
#include <iterator>

constexpr std::size_t DYNAMIC_CAPACITY = std::numeric_limits<std::size_t>::max();

template<typename T, std::size_t Capacity>
class container {
 private:
   std::byte arr_[sizeof(T) * Capacity];
 public:
  container() = default;
  container(std::size_t cap) {
    if (cap != Capacity) {
      throw std::invalid_argument("failed");
    }
  }
  T* data() { 
    return reinterpret_cast<T*>(arr_);
  }
  const T* data() const { 
    return reinterpret_cast<const T*>(arr_);
  }
  static constexpr std::size_t capacity() {
    return Capacity;
  }
  T& operator[](size_t i) {
    return reinterpret_cast<T*>(arr_)[i];
  }
  const T& operator[](size_t i) const {
    return reinterpret_cast<const T*>(arr_)[i];
  }
};

template <typename T>
class container<T, DYNAMIC_CAPACITY> {
 private:
  T* arr_;
  size_t cap_;
  void createArr(size_t newcap) {
    arr_ = reinterpret_cast<T*>(new std::byte[newcap * sizeof(T)]);
  }
 public:
  container() = delete;
  container(std::size_t capacity) : cap_(capacity) {
    createArr(cap_);
  }
  container(const container& other) : cap_(other.cap_) {
    createArr(cap_);
  }
  ~container() {
    delete[] reinterpret_cast<std::byte*>(arr_);
  }
  const T* data() const {
    return arr_;
  }
  T* data() {
    return arr_;
  }
  std::size_t capacity() const {
    return cap_;
  }
  T& operator[](size_t i) {
    return arr_[i];
  }
  const T& operator[](size_t i) const {
    return arr_[i];
  }
};

template <typename T, std::size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer {
 private:
  container<T, Capacity> arr_;
  size_t size_ = 0;
  size_t head_ = 0;
  size_t tail_ = 0;
  void deleteArr() {
    for (size_t i = 0; i < size_; ++i) {
      arr_.data()[head_].~T();
      head_ = (head_ + 1) % capacity();
    }
  }
 public: 
  CircularBuffer(std::size_t cap = Capacity) : arr_(cap) {}
  CircularBuffer(const CircularBuffer<T, Capacity>& other) : arr_(other.capacity()), size_(0), head_(other.head_), tail_(other.tail_) {
    try {
      for (; size_ != other.size_; ++size_) {
        new (arr_.data() + (size_ + head_) % capacity()) T(*(other.arr_.data() + (size_ + head_) % capacity()));
      }
    } catch (...) {
      for (int i = 0; i != size_; ++i) {
        (arr_.data() + (i + head_) % capacity())->~T();
      }
      throw;
    }
  }
  CircularBuffer& operator=(const CircularBuffer& other) {
    deleteArr();
    head_ = other.head_;
    size_ = 0;
    tail_ = other.tail_;
    try {
      for (; size_ != other.size_; ++size_) {
        new (arr_.data() + (size_ + head_) % capacity()) T(*(other.arr_.data() + (size_ + head_) % capacity()));
      }
    } catch (...) {
      for (int i = 0; i != size_; ++i) {
        (arr_.data() + (i + head_) % capacity())->~T();
      }
      throw;
    }
    return *this;
  }
  size_t size() const {
    return size_;
  }
  bool empty() const {
    return size_ == 0;
  } 
  size_t capacity() const {
    return arr_.capacity();
  }
  bool full() const {
    return size_ == capacity();
  }
  void push_back(T elem) {
    if (full()) {
      std::swap(arr_.data()[head_], elem);
      head_ = (head_ + 1) % capacity();
      --size_;
    } else {
      try {
        new (arr_.data() + tail_) T(elem);
      } catch (...) {
        throw;
      }
    }
    tail_ = (tail_ + 1) % capacity();
    ++size_;
  }
  void push_front(T elem) {
    if (full()) {
      tail_ = (tail_ - 1 + capacity()) % capacity();
      std::swap(arr_.data()[tail_], elem);
      --size_;
    } else {
      try {
        new (arr_.data() + (head_ - 1  + capacity()) % capacity()) T(elem);
      } catch (...) {
        throw;
      }
    }
    head_ = (head_ - 1 + capacity()) % capacity();
    ++size_;
  }
  void pop_back() {
    tail_ = (tail_ - 1 + capacity()) % capacity();
    arr_.data()[tail_].~T();
    --size_;
  }
  void pop_front() {
    arr_.data()[head_].~T();
    head_ = (head_ + 1) % capacity();
    --size_;
  }
  T& operator[](size_t i) {
    return arr_[(head_ + i) % capacity()];
  }
  const T& operator[](size_t i) const {
    return arr_[(head_ + i) % capacity()];
  }
  T& at(size_t i) {
    if (i < 0 || i >= size_) {
      throw std::out_of_range("Index out of range");
    }
    return arr_[(head_ + i) % capacity()];
  }
  const T& at(size_t i) const {
    if (i < 0 || i >= size_) {
      throw std::out_of_range("Index out of range");
    }
    return arr_[(head_ + i) % capacity()];
  }
  ~CircularBuffer() {
    deleteArr();
  }

  class dynamIter {
   protected:
    size_t cap_ = 0;
    dynamIter(size_t cap) : cap_(cap) {}
    size_t capacity() const {
      return cap_;
    }
  };

  class statIter {
    protected:
     size_t capacity() const {
       return Capacity;
     }
  };

  using iter = std::conditional_t<Capacity == DYNAMIC_CAPACITY, dynamIter, statIter>;

  template<bool IsConst>
  class base_iterator : iter {
   public:
    using pointer_type = std::conditional_t<IsConst, const T*, T*>;
    using reference_type = std::conditional_t<IsConst, const T&, T&>;
    friend class CircularBuffer<T, Capacity>;
  
   private:
    pointer_type arr_;
    size_t head_;
    size_t pos;
    size_t size_;
    base_iterator(pointer_type data, size_t head, size_t pos, size_t size)
        : arr_(data), head_(head), pos(pos), size_(size) {}
  
    base_iterator(pointer_type data, size_t head, size_t pos, size_t size, size_t cap)
        : iter(cap), arr_(data), head_(head), pos(pos), size_(size) {}
  
   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = pointer_type;
    using reference = reference_type;
  
    base_iterator(const base_iterator&) = default;
    base_iterator() : arr_(nullptr), head_(0), pos(0), size_(0) {}
  
    template<bool OtherIsConst, typename = std::enable_if_t<IsConst && !OtherIsConst>>
    base_iterator(const base_iterator<OtherIsConst>& other)
        : arr_(other.arr_), head_(other.head_), pos(other.pos), size_(other.size_) {}
  
    base_iterator& operator=(const base_iterator&) = default;
  
    reference_type operator*() const {
      return arr_[(head_ + pos) % iter::capacity()];
    }
  
    pointer_type operator->() const {
      return arr_ + (head_ + pos) % iter::capacity();
    }
  
    base_iterator& operator++() {
      if (pos < size_) {
        ++pos;
      }
      return *this;
    }
  
    base_iterator operator++(int) {
      base_iterator copy = *this;
      if (pos < size_) { 
        ++pos;
      }
      return copy;
    }
  
    base_iterator& operator--() {
      if (pos > 0) {
        --pos;
      }
      return *this;
    }
  
    base_iterator operator--(int) {
      base_iterator copy = *this;
      if (pos > 0) { 
        --pos;
      }
      return copy;
    }
  
    base_iterator& operator+=(int x) {
      pos += x;
      if (pos > size_) {
        pos = size_;
      }
      return *this;
    }
    
    friend base_iterator operator+(int x, const base_iterator& it) {
      return (it + x);
    }

    base_iterator operator+(int x) const {
      base_iterator copy = *this;
      copy += x;
      return copy;
    }
  
    base_iterator& operator-=(int x) {
      if (x > static_cast<int>(pos)) {
        pos = 0;
      } else {
        pos -= x;
      }
      return *this;
    }
  
    base_iterator operator-(int x) const {
      base_iterator copy = *this;
      copy -= x;
      return copy;
    }
  
    bool operator==(const base_iterator& other) const {
      return head_ == other.head_ && pos == other.pos;
    }
  
    bool operator!=(const base_iterator& other) const {
      return !(*this == other);
    }
  
    bool operator<(const base_iterator& other) const {
      return pos < other.pos;
    }
  
    bool operator<=(const base_iterator& other) const {
      return *this < other || *this == other;
    }
  
    bool operator>(const base_iterator& other) const {
      return other < *this;
    }
  
    bool operator>=(const base_iterator& other) const {
      return *this > other || *this == other;
    }
  
    difference_type operator-(const base_iterator& other) const {
      return static_cast<difference_type>(pos - other.pos);
    }
  };
  
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  
  iterator begin() {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return iterator(arr_.data(), head_, 0, size_, arr_.capacity());
    } else {
      return iterator(arr_.data(), head_, 0, size_);
    }
  }
  
  const_iterator begin() const {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return const_iterator(arr_.data(), head_, 0, size_, arr_.capacity());
    } else {
      return const_iterator(arr_.data(), head_, 0, size_);
    }
  }
  
  const_iterator cbegin() const {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return const_iterator(arr_.data(), head_, 0, size_, arr_.capacity());
    } else {
      return const_iterator(arr_.data(), head_, 0, size_);
    }
  }
  
  iterator end() {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return iterator(arr_.data(), head_, size_, size_, arr_.capacity());
    } else {
      return iterator(arr_.data(), head_, size_, size_);
    }
  }
  
  const_iterator end() const {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return const_iterator(arr_.data(), head_, size_, size_, arr_.capacity());
    } else {
      return const_iterator(arr_.data(), head_, size_, size_);
    }
  }
  
  const_iterator cend() const {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return const_iterator(arr_.data(), head_, size_, size_, arr_.capacity());
    } else {
      return const_iterator(arr_.data(), head_, size_, size_);
    }
  }
  
  reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
  const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }
  
  void insert(iterator it, T elem) {
    while (it != end()) {
      std::swap(elem, *it);
      it += 1;
    }
    push_back(elem);
  }
  void erase(iterator it) {
    while (it != (end() - 1)) {
      std::swap(*it, *(it + 1));
      it += 1;
    }
    pop_back();
  }
};