#include <iostream>
#include <array>
#include <limits>
#include <span>
#include <optional>
#include <type_traits>
#include <iterator>
#include <memory>

constexpr std::size_t DYNAMIC_CAPACITY = std::numeric_limits<std::size_t>::max();

template<typename T, std::size_t Capacity>
class container {
 private:

  alignas(T) std::byte arr_[sizeof(T) * Capacity];
 
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

  constexpr std::size_t capacity() const {
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

  void swap(container& other) {
    std::swap(arr_, other.arr_);
    std::swap(cap_, other.cap_);
  }

};

template <typename T, std::size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer {
 private:


  container<T, Capacity> arr_;
  size_t size_ = 0;
  size_t head_ = 0;
  

  using SpanType = std::conditional_t<
    Capacity == DYNAMIC_CAPACITY,
    std::span<T>,
    std::span<T, Capacity>
  >;

  SpanType get_span() const {
    return SpanType(const_cast<T*>(arr_.data()), arr_.capacity());
  }

  void destroyAllElements() {
    for (size_t i = 0; i < size_; ++i) {
      std::destroy_at(arr_.data() + head_);
      head_ = modCapacity(head_, 1);
    }
    size_ = 0;
  }

  size_t modCapacity(size_t first_elem, size_t second_elem) const {
    return (first_elem + second_elem) % capacity();
  }

  void swap(CircularBuffer& other) {
    arr_.swap(other.arr_);
    std::swap(size_, other.size_);
    std::swap(head_, other.head_);
  }

 public:

  explicit CircularBuffer(size_t cap = Capacity) : arr_(cap) {}

  CircularBuffer(const CircularBuffer<T, Capacity>& other) : arr_(other.capacity()), size_(0), head_(0) {
    try {
      for (size_t i = 0; i < other.size_; ++i) {
        std::construct_at(arr_.data() + i, other[i]);
        ++size_;
      }
    } catch (...) {
      destroyAllElements();
      throw;
    }
  }

  CircularBuffer& operator=(const CircularBuffer& other) requires (Capacity == DYNAMIC_CAPACITY) {
    CircularBuffer tmp(other);
    swap(tmp);
    return *this;
  }
  
  CircularBuffer& operator=(const CircularBuffer& other) requires (Capacity != DYNAMIC_CAPACITY) {
    destroyAllElements();
    head_ = other.head_;
    size_ = 0;
    try {
      for (; size_ != other.size_; ++size_) {
        std::construct_at(arr_.data() + modCapacity(size_, head_), *(other.arr_.data() + modCapacity(size_, head_)));
      }
    } catch (...) {
      destroyAllElements();
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
      head_ = modCapacity(head_, 1);
      --size_;
    } else {
      std::construct_at(arr_.data() + modCapacity(head_, size_), elem);
    }
    ++size_;
  }

  void push_front(T elem) {
    if (full()) {
      std::swap(arr_.data()[modCapacity(head_ + size_, capacity() - 1)], elem);
      --size_;
    } else {
      std::construct_at(arr_.data() + modCapacity(head_, capacity() - 1), elem);
    }
    head_ = modCapacity(head_, capacity() - 1);
    ++size_;
  }

  void pop_back() {
    std::destroy_at(arr_.data() + modCapacity(head_ + size_, capacity() - 1));
    --size_;
  }

  void pop_front() {
    std::destroy_at(arr_.data() + head_);
    head_ = modCapacity(head_, 1);
    --size_;
  }

  T& operator[](size_t i) {
    return (arr_)[modCapacity(head_, i)];
  }

  const T& operator[](size_t i) const {
    return (arr_)[modCapacity(head_, i)];
  }
  
  T& at(size_t i) {
    if (i < 0 || i >= size_) {
      throw std::out_of_range("Index out of range");
    }
    return (arr_)[modCapacity(head_, i)];
  }

  const T& at(size_t i) const {
    if (i < 0 || i >= size_) {
      throw std::out_of_range("Index out of range");
    }
    return (arr_)[modCapacity(head_, i)];
  }

  ~CircularBuffer() {
    destroyAllElements();
  }

  template <bool IsConst>
  class base_iterator {
   private:
    using span_type = std::conditional_t<
      Capacity == DYNAMIC_CAPACITY,
      std::conditional_t<IsConst, const std::span<const T>, std::span<T>>,
      std::conditional_t<IsConst, const std::span<const T, Capacity>, std::span<T, Capacity>>
    >;
    span_type span_;
    size_t head_;
    size_t pos_;

    template <bool OtherIsConst>
    friend class base_iterator;

   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::conditional_t<IsConst, const T, T>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    base_iterator(span_type span, size_t head, size_t pos)
    : span_(span), head_(head), pos_(pos) {}

    base_iterator() = default;

    base_iterator(const base_iterator& other) : span_(other.span_), head_(other.head_), pos_(other.pos_) {}

    template <bool otherIsConst>
    requires (IsConst)
    base_iterator(const base_iterator<otherIsConst>& other) : span_(other.span_), head_(other.head_), pos_(other.pos_) {}

    template <bool otherIsConst>
    requires (IsConst)
    base_iterator& operator=(const base_iterator<otherIsConst>& other) {
      span_ = other.span_;
      head_ = other.head_;
      pos_ = other.pos_;
      return *this;
    }

    base_iterator& operator=(const base_iterator& other) {
      span_ = other.span_;
      head_ = other.head_;
      pos_ = other.pos_;
      return *this;
    }


    reference operator*() const {
      return span_[(head_ + pos_) % span_.size()];
    }

    pointer operator->() const {
      return &span_[(head_ + pos_) % span_.size()];
    }

    base_iterator& operator++() {
      ++pos_;
      return *this;
    }

    base_iterator operator++(int) {
      auto tmp = *this;
      ++*this;
      return tmp;
    }

    base_iterator& operator--() {
      --pos_;
      return *this;
    }

    base_iterator operator--(int) {
      auto tmp = *this;
      --*this;
      return tmp;
    }

    base_iterator& operator+=(difference_type n) {
      pos_ += n;
      return *this;
    }

    friend base_iterator operator+(base_iterator it, difference_type n) {
      it += n;
      return it;
    }

    friend base_iterator operator+(difference_type n, base_iterator it) {
      it += n;
      return it;
    }

    friend base_iterator operator-(base_iterator it, difference_type n) {
      it -= n;
      return it;
    }

    base_iterator& operator-=(difference_type n) {
      pos_ -= n;
      return *this;
    }

    difference_type operator-(const base_iterator& other) const {
      return pos_ - other.pos_;
    }

    bool operator==(const base_iterator& other) const {
      return pos_ == other.pos_ && span_.data() == other.span_.data();
    }

    bool operator!=(const base_iterator& other) const {
      return !(*this == other);
    }
  
    bool operator<(const base_iterator& other) const {
      return pos_ < other.pos_;
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

  };
  
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  
  iterator begin() {
    return iterator(get_span(), head_, 0);
  }

  iterator end() {
    return iterator(get_span(), head_, size_);
  }

  const_iterator begin() const {
    return const_iterator(get_span(), head_, 0);
  }

  const_iterator end() const {
    return const_iterator(get_span(), head_, size_);
  }
  
  const_iterator cbegin() const {
    return begin();
  }
  
  const_iterator cend() const {
    return end();
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