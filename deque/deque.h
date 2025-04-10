#include <iostream>
#include <array>
#include <type_traits>
#include <iterator>

template <typename T>
class Deque {
 private:
  struct Positions {
    int segm = 0;
    int pos = 0;
    Positions(int segm, int pos): segm(segm), pos(pos) {}
    Positions() = default;
    Positions(const Positions& other) = default;
    Positions& operator=(const Positions& other) = default;
    bool operator==(const Positions& other) const {
      return (segm == other.segm) && (pos == other.pos);
    }
    bool operator!=(const Positions& other) const {
      return !((segm == other.segm) && (pos == other.pos));
    }
    bool operator<(const Positions& other) const {
      return (segm < other.segm) || (segm == other.segm && pos < other.pos);
    }
    bool operator<=(const Positions& other) const {
      return !(other < (*this));
    }
    bool operator>(const Positions& other) const {
      return (other < (*this));
    }
    bool operator>=(const Positions& other) const {
      return !((*this) < other);
    }
    Positions& operator+=(int x) {
      pos += x;
      if (pos > 0) {
        segm += (pos / 32);
        pos %= 32;
      } else {
        if (pos % 32 != 0) {
          segm -= std::abs(pos / 32 - 1);
          pos = pos % 32 + 32;
        } else {
          segm -= (pos / 32);
          pos = 0;
        }
      }
      return *this;
    }
    Positions operator+(int x) const {
      Positions newPos = *this;
      return newPos += x;
    }
  };

  int sz_ = 0;
  int segm_count_ = 0;
  Positions head; // first free semgent's num and elem's num
  Positions tail;
  T** arr_ = nullptr;
  static const int N = 32;

  void createArr(int newcap) {
    T** newarr = reinterpret_cast<T**>(new char[newcap * sizeof(T*)]);
    arr_ = newarr;
    segm_count_ = newcap;
    for (size_t i = 0; i != newcap; ++i) {
      reserveBucket(i);
    }
  }
  
  void reserveBucket(int pos) {
    T* bucket = reinterpret_cast<T*>(new char[N * sizeof(T)]);
    arr_[pos] = bucket;
  }
  
  void fillBucket(const T& init, int pos) {
    int index = 0;
    try {
      for (index = 0; index < N; ++index) {
        new(arr_[pos] + index) T(init);
      }
    } catch (...) {
      for (int i = 0; i != index; ++i) {
        (arr_[pos] + index)->~T();
      }
      throw;
    }
  }

  void fillBucket(int pos) {
    int index = 0;
    try {
      for (index = 0; index < N; ++index) {
        new(arr_[pos] + index) T();
      }
    } catch (...) {
      for (int i = 0; i != index; ++i) {
        (arr_[pos] + index)->~T();
      }
      throw;
    }
  }

  void destroyBucket(int pos) {
    for (int i = 0; i != N; ++i) {
      (arr_[pos] + i)->~T();
    }
  }

  void copyBucket(const Deque<T>& other, int start, int end, int bucket_num) {
    for (int index = start; index < end; ++index) {
      new(arr_[bucket_num] + index) T(other.arr_[bucket_num][index]);
    }
  }

  void reallocArr() {
    T** new_arr = reinterpret_cast<T**>(new char[segm_count_ * 3 * sizeof(T*)]);
    size_t index = 0;
    try {
      for (; index < segm_count_; ++index) {
        new (new_arr + index + segm_count_) T*(arr_[index]);
      }
    } catch (...) {
      for (size_t i = 0; i != index; ++i) {
        delete (new_arr + i + segm_count_);
      }
      delete[] reinterpret_cast<char*>(new_arr);
      throw;
    }
    delete[] reinterpret_cast<char*>(arr_);

    arr_ = new_arr;
    head.segm += segm_count_;
    tail.segm += segm_count_;
    for (size_t i = 0; i != segm_count_; ++i) {
      reserveBucket(i);
    }
    for (size_t i = 2 * segm_count_; i != 3 * segm_count_; ++i) {
      reserveBucket(i);
    }
    segm_count_ *= 3;
  }

  void deleteArr() {
    if (arr_ == nullptr) {
      return;
    }
    for (size_t index = 0; index != segm_count_; ++index) {
      if (!(head.segm - tail.segm == 0 && tail.pos - head.pos == 1)) {
        for (size_t i = 0; i != N; ++i) {
          (arr_[index] + i)->~T();
        }
      }
      delete[] reinterpret_cast<char*>(arr_[index]);
    }
    delete[] reinterpret_cast<char*>(arr_);
    arr_ = nullptr;
  }

  void headUp() {
    --head.pos;
    if (head.pos < 0) {
      head.pos = 31;
      --head.segm;
      if (head.segm < 0) {
        reallocArr();
      }
    }
  }

  void headDown() {
    ++head.pos;
    if (head.pos > 31) {
      head.pos = 0;
      ++head.segm;
    }
  }

  void tailUp() {
    ++tail.pos;
    if (tail.pos > 31) {
      tail.pos = 0;
      ++tail.segm;
      if (tail.segm >= segm_count_) {
        reallocArr();
      }
    }
  }

  void tailDown() {
    --tail.pos;
    if (tail.pos < 0) {
      tail.pos = 31;
      --tail.segm;
    }
  }

 public:

  Deque() {}

  Deque(int sz, const T& init): sz_(sz) {
    int newSize = 3 * (sz / N + 1);
    createArr(newSize);
    head = {newSize / 2, 15};
    tail = {newSize / 2, 16};
    if (sz / N > 0) {
      int fullBuckets = sz / N;
      int ind;
      try {
        for (ind = newSize / 2 - fullBuckets / 2; ind < newSize / 2 + fullBuckets - fullBuckets / 2; ++ind) {
          fillBucket(init, ind);
        }
      } catch(...) {
        for (int i = newSize / 2 - fullBuckets / 2; i != ind; ++i) {
          destroyBucket(i);
        }
        deleteArr();
        throw;
      }
      head = {newSize / 2 - fullBuckets / 2 - 1, 31};
      tail = {ind, 0};
      sz -= fullBuckets * 32;
    }
    while (sz > 0) {
      new(arr_[head.segm] + head.pos)  T(init);
      --sz;
      headUp();
      if (sz > 0) {
        new(arr_[tail.segm] + tail.pos) T(init);
        --sz;
        tailUp();
      }
    }
  }

  Deque(int sz): sz_(sz) {
    int newSize = 3 * (sz / N + 1);
    createArr(newSize);
    head = {newSize / 2, 15};
    tail = {newSize / 2, 16};
    if (sz / N > 0) {
      int fullBuckets = sz / N;
      int ind;
      try {
        for (ind = newSize / 2 - fullBuckets / 2; ind < newSize / 2 + fullBuckets - fullBuckets / 2; ++ind) {
          fillBucket(ind);
        }
      } catch(...) {
        for (int i = newSize / 2 - fullBuckets / 2; i != ind; ++i) {
          destroyBucket(i);
        }
        deleteArr();
        throw;
      }
      head = {newSize / 2 - fullBuckets / 2 - 1, 31};
      tail = {ind, 0};
      sz -= fullBuckets * 32;
    }
    while (sz > 0) {
      new(arr_[head.segm] + head.pos)  T();
      --sz;
      headUp();
      if (sz > 0) {
        new(arr_[tail.segm] + tail.pos) T();
        --sz;
        tailUp();
      }
    }
  }

  Deque(const Deque<T>& other): sz_(other.sz_), head(other.head), tail(other.tail), segm_count_(other.segm_count_) {
    if (other.arr_ == nullptr) {
      return;
    }
    createArr(other.segm_count_);
    if (head.pos != 31) {
      copyBucket(other, head.pos, 32, other.head.segm);
    }
    for (int index = other.head.segm + 1; index < other.tail.segm; ++index) {
      copyBucket(other, 0, 32, index);
    }
    if (tail.pos != 0) {
      copyBucket(other, 0, other.tail.pos, other.tail.segm);
    }
  }

  Deque<T>& operator=(const Deque<T>& other) {
    deleteArr();
    head = other.head;
    tail = other.tail;
    segm_count_ = other.segm_count_;
    sz_ = other.sz_;
    if (other.arr_ == nullptr) {
      return *this;
    }
    createArr(other.segm_count_);
    if (head.pos != 31) {
      copyBucket(other, head.pos, 32, other.head.segm);
    }
    for (int index = other.head.segm + 1; index < other.tail.segm; ++index) {
      copyBucket(other, 0, 32, index);
    }
    if (tail.pos != 0) {
      copyBucket(other, 0, other.tail.pos, other.tail.segm);
    }
    return *this;
  }

  T& operator[](int i) {
    return arr_[head.segm + (i + 1) / 32 + (head.pos + (i + 1) % 32) / 32][(head.pos + (i + 1)) % 32];
  }

  const T& operator[](int i) const {
    return arr_[head.segm + (i + 1) / 32 + (head.pos + (i + 1) % 32) / 32][(head.pos + (i + 1)) % 32];
  }

  const T& at(int i) const {
    if (i < 0) {
      throw std::out_of_range("Index out of range");
    }
    int segm = (head.segm + (i + 1) / 32 + (head.pos + (i + 1) % 32) / 32);
    int pos = (head.pos + (i + 1)) % 32;
    if (segm > tail.segm || (segm == tail.segm && pos >= tail.pos)) {
      throw std::out_of_range("Index out of range");
    }
    return arr_[segm][pos];    
  }

  T& at(int i) {
    if (i < 0) {
      throw std::out_of_range("Index out of range");
    }
    int segm = (head.segm + (i + 1) / 32 + (head.pos + (i + 1) % 32) / 32);
    int pos = (head.pos + (i + 1)) % 32;
    if (segm > tail.segm || (segm == tail.segm && pos >= tail.pos)) {
      throw std::out_of_range("Index out of range");
    }
    return arr_[segm][pos];    
  }

  int size() const {
    return sz_;
  }

  void push_back(const T& other) {
    if (arr_ == nullptr) {
      createArr(3);
      head = {segm_count_ / 2, 15};
      tail = {segm_count_ / 2, 16};
    }
    try {
      ++sz_;
      new(arr_[tail.segm] + tail.pos) T(other);
      tailUp();
    } catch (...) {
      --sz_;
      throw;
    }
  }

  void pop_back() {
    --sz_;
    tailDown();
    (arr_[tail.segm] + tail.pos)->~T();
  }

  void push_front(const T& other) {
    if (arr_ == nullptr) {
      createArr(3);
      head = {segm_count_ / 2, 15};
      tail = {segm_count_ / 2, 16};
    }    
    try {
      ++sz_;
      new(arr_[head.segm] + head.pos) T(other);
      headUp();
    } catch (...) {
      --sz_;
      throw;
    }
  }

  void pop_front() {
    --sz_;
    headDown();
    (arr_[head.segm] + head.pos)->~T();
  }

  void Print() const {
    for (size_t i = 0; i != sz_; ++i) {
      std::cout << (*this)[i] << std::endl;
    }
  }

  ~Deque() {
    deleteArr();
  }

  template<bool IsConst>
  class base_iterator {
   public:
    using pointer_type = std::conditional_t<IsConst, const T*, T*>;
    using reference_type = std::conditional_t<IsConst, const T&, T&>;   
   private:
    T** arr;
    Positions place;
    pointer_type ptr;
    base_iterator(pointer_type ptr, int segm, int pos, T** arr): ptr(ptr), place{segm, pos}, arr(arr) {}
    friend class Deque<T>;
  
   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;  
    base_iterator(const base_iterator&) = default;
    base_iterator() = default;
  
    template<bool OtherIsConst, typename = std::enable_if_t<IsConst && !OtherIsConst>>
    base_iterator(const base_iterator<OtherIsConst>& other)
      : arr(other.arr), place(other.place), ptr(other.ptr) {}    
    
    base_iterator& operator=(const base_iterator&) = default;
    
    reference_type operator*() const {
      return *(ptr + place.pos);
    }

    pointer_type operator->() const {
      return (ptr + place.pos);
    }

    base_iterator& operator++() {
      return *this += 1;
    }

    base_iterator operator++(int) {
      base_iterator copy = *this;
      *this += 1;
      return copy;
    }
    base_iterator& operator--() {
      return *this -= 1;
    }

    base_iterator operator--(int) {
      base_iterator copy = *this;
      *this -= 1;
      return copy;
    } 

    bool operator==(const base_iterator& other) const {
      return (place == other.place);
    }
    
    bool operator!=(const base_iterator& other) const {
      return !(place == other.place);
    }

    bool operator<(const base_iterator& other) const {
      return (place < other.place);
    }

    bool operator<=(const base_iterator& other) const {
      return !(other.place < place);
    }
    bool operator>(const base_iterator& other) const {
      return (other.place < place);
    }
    bool operator>=(const base_iterator& other) const {
      return !(place < other.place);
    }

    base_iterator& operator+=(int x) {
      place += x;
      if (arr != nullptr) {
        ptr = arr[place.segm];
      }
      return *this;
    }

    base_iterator operator+(int x) const {
      base_iterator newIter = *this;
      return newIter += x;
    }

    base_iterator& operator-=(int x) {
      place += (-x);
      if (arr != nullptr) {
        ptr = arr[place.segm];
      }
      return *this;
    }

    base_iterator operator-(int x) const {
      base_iterator newIter = *this;
      return newIter -= x;
    }

  difference_type operator-(const base_iterator& other) const {
      return static_cast<difference_type>((place.segm - other.place.segm) * 32 + (place.pos - other.place.pos));
  }

  };

  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  friend iterator;
  friend const_iterator;

  iterator begin() {
    if (arr_ == nullptr) {
      return {nullptr, 0, 0, nullptr};
    }
    int segm = head.segm;
    int pos = head.pos + 1;
    if (pos > 31) {
      pos = 0;
      segm += 1;
    }
    return {arr_[segm], segm, pos, arr_};
  }
  reverse_iterator rbegin() { return std::make_reverse_iterator(this->end()); }

  iterator end() {
    if (arr_ == nullptr) {
      return {nullptr, 0, 0, nullptr};
    }
    return {arr_[tail.segm], tail.segm, tail.pos, arr_};
  }

  reverse_iterator rend() {
    return std::make_reverse_iterator(this->begin());
  }

  const_iterator begin() const {
    if (arr_ == nullptr) {
      return {nullptr, 0, 0, nullptr};
    }
    int segm = head.segm;
    int pos = head.pos + 1;
    if (pos > 31) {
      pos = 0;
      segm += 1;
    }
    return {arr_[segm], segm, pos, arr_};
  }

  const_reverse_iterator rbegin() const { 
    return std::make_reverse_iterator(this->end());
  }

  const_reverse_iterator rend() const {
    return std::make_reverse_iterator(this->begin());
  }

  const_iterator end() const {
    if (arr_ == nullptr) {
      return {nullptr, 0, 0, nullptr};
    }
    return {arr_[tail.segm], tail.segm, tail.pos, arr_};
  }

  const_iterator cbegin() const {
    if (arr_ == nullptr) {
      return {nullptr, 0, 0, nullptr};
    }
    int segm = head.segm;
    int pos = head.pos + 1;
    if (pos > 31) {
      pos = 0;
      segm += 1;
    }
    return {arr_[segm], segm, pos, arr_};
  }

  const_iterator cend() const {
    if (arr_ == nullptr) {
      return {nullptr, 0, 0, nullptr};
    }
    return {arr_[tail.segm], tail.segm, tail.pos, arr_}; 
  }

  const_reverse_iterator crbegin() const { 
    return std::make_reverse_iterator(this->cend());
  }

  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(this->cbegin());
  }

  void insert(iterator it, T elem) {
    iterator start = it;
    try {
      while (it != end()) {
        std::swap(elem, *it);
        it += 1;
      }
    } catch (...) {
      while (it >= start) {
        std::swap(elem, *it);
        it -= 1;
        throw;
      }
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