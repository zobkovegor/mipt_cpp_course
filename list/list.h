#include <iostream>
#include <memory>
#include <type_traits>
#include <iterator>

template<size_t N>
class StackStorage {
  std::byte buffer[N];
  size_t used = 0;

public:
  StackStorage() = default;
  StackStorage(const StackStorage&) = delete;
  StackStorage& operator=(const StackStorage&) = delete;

  void* allocate(size_t size, size_t alignment) {
    void* curr_ptr = buffer + used;
    size_t free = N - used;
    if (std::align(alignment, size, curr_ptr, free)) {
      used = used + size;
      return curr_ptr;
    }
    throw std::bad_alloc();
  }

};

template<typename T, size_t N>
class StackAllocator {
  StackStorage<N>* storage;

public:
  using value_type = T;

  StackAllocator(StackStorage<N>& storage) noexcept : storage(&storage) {}

  template<typename U>
  StackAllocator(const StackAllocator<U, N>& other) noexcept : storage(other.storage) {}

  T* allocate(size_t cap) {
    return static_cast<T*>(storage->allocate(cap * sizeof(T), alignof(T)));
  }

  void deallocate(T*, size_t) noexcept {}

  template <typename U, typename... Args>
  void construct(U* ptr, const Args&... args) {
    new (ptr) U(args...);
  }

  template <typename U>
  void destruct(U* ptr) {
    ptr->~U();
  }

  bool operator==(const StackAllocator& other) const noexcept {
    return storage == other.storage;
  }

  template<typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };
  
  template<typename U, size_t M>
  friend class StackAllocator;
};

template <typename T, typename Alloc = std::allocator<T>>
class List {
 private:

  struct BaseNode {
    BaseNode() {
      prev = this;
      next = this;
    }
    BaseNode(BaseNode* first, BaseNode* second) : prev(first), next(second) {}
    BaseNode* prev;
    BaseNode* next;
  };

  struct Node : BaseNode {
    T value;
    Node(BaseNode* prev, BaseNode* next, const T& val) : BaseNode{prev, next}, value(val) {}
    Node(BaseNode* prev, BaseNode* next) : BaseNode{prev, next}, value() {}
    Node() = default;
  };

  using AllocTraits = std::allocator_traits<Alloc>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;
  
  BaseNode FakeNode;
  [[no_unique_address]] NodeAlloc nodeAlloc_;
  size_t size_; 
 
  void swap(List& other) {
    std::swap(FakeNode, other.FakeNode);    
    if (FakeNode.next != &FakeNode) {
      FakeNode.next->prev = &FakeNode;
    }
    if (FakeNode.prev != &FakeNode) {
      FakeNode.prev->next = &FakeNode;
    }
    
    if (other.FakeNode.next != &other.FakeNode) {
      other.FakeNode.next->prev = &other.FakeNode;
    }
    if (other.FakeNode.prev != &other.FakeNode) {
      other.FakeNode.prev->next = &other.FakeNode;
    }
    
    std::swap(size_, other.size_);
    
    if (AllocTraits::propagate_on_container_swap::value) {
      std::swap(nodeAlloc_, other.nodeAlloc_);
    }
  }

  void deallocNode(Node* node) {
    --size_;
    NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
  }

  void constructNode(Node* node, BaseNode* prev, BaseNode* next, const T& value) {
    ++size_;
    NodeAllocTraits::construct(nodeAlloc_, node, prev, next, value);
  }

  void changeFakeNodePointers(Node* node, size_t index) {
    FakeNode.prev->next = node;
    FakeNode.prev = node;
    if (index == 0) {
      FakeNode.next = node;
    }
  }

public:

  List(const Alloc& allocator = Alloc()) : nodeAlloc_(allocator), size_(0) {}

  List(size_t cap, const Alloc& allocator = Alloc()) : List(allocator) {
    for (size_t i = 0; i < cap; ++i) {
      emplace(end());
    }
  }

  List(size_t cap, const T& val, const Alloc& allocator = Alloc()) : List(allocator) {
    for (size_t i = 0; i < cap; ++i) {
      push_back(val);
    }
  }

  List(const List& other) : List(AllocTraits::select_on_container_copy_construction(other.nodeAlloc_)) {
    for (const auto& item : other) {
      push_back(item);
    }
  }

  List(const List& other, Alloc allocator) : List(allocator) {
    for (const auto& item : other) {
      push_back(item);
    }
  }

  List& operator=(const List& other) {
    if (this == &other) {
      return *this;
    }
    if (AllocTraits::propagate_on_container_copy_assignment::value) {
      nodeAlloc_ = other.nodeAlloc_;
    }
    List help(other, nodeAlloc_);
    swap(help);
    return *this;
  }

  const NodeAlloc get_allocator() const {
    return nodeAlloc_;
  }

  size_t size() const {
    return size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  template<bool IsConst>
  class base_iterator {
   public:
    using pointer_type = std::conditional_t<IsConst, const T*, T*>;
    using reference_type = std::conditional_t<IsConst, const T&, T&>;
    using baseNodePointer = std::conditional_t<IsConst, const BaseNode*, BaseNode*>;
    using NodePointer = std::conditional_t<IsConst, const Node*, Node*>;
    friend List;
   private:
    baseNodePointer pos;
    base_iterator(baseNodePointer pos) : pos(pos) {}
    
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = pointer_type;
    using reference = reference_type;
  
    base_iterator(const base_iterator&) = default;
    base_iterator() : pos(nullptr) {}
  
    template<bool OtherIsConst, typename = std::enable_if_t<IsConst && !OtherIsConst>>
    base_iterator(const base_iterator<OtherIsConst>& other) : pos(other.pos) {}
  
    base_iterator& operator=(const base_iterator&) = default;
  
    reference_type operator*() const {
      return static_cast<NodePointer>(pos)->value;
    }
  
    pointer_type operator->() const {
      return &(static_cast<NodePointer>(pos)->value);
    }

    base_iterator& operator++() {
      pos = pos->next;
      return *this;
    }
  
    base_iterator operator++(int) {
      base_iterator copy = *this;
      pos = pos->next;
      return copy;
    }
  
    base_iterator& operator--() {
      pos = pos->prev;
      return *this;
    }
  
    base_iterator operator--(int) {
      base_iterator copy = *this;
      pos = pos->prev;
      return copy;
    }
  
    base_iterator& operator+=(int x) {
      for (int i = 0; i != x; ++i) {
        pos = pos->next;
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
      for (int i = 0; i != x; ++i) {
        pos = pos->prev;
      }
      return *this;
    }
  
    base_iterator operator-(int x) const {
      base_iterator copy = *this;
      copy -= x;
      return copy;
    }
    
    bool operator==(const base_iterator& other) const {
      return pos == other.pos;
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
    return iterator(FakeNode.next);
  }
  
  const_iterator begin() const {
    return const_iterator(FakeNode.next);
  }
  
  const_iterator cbegin() const {
    return const_iterator(FakeNode.next);
  }
  
  iterator end() {
    return iterator(&FakeNode);
  }
  
  const_iterator end() const {
    return const_iterator(&FakeNode);
  }
  
  const_iterator cend() const {
    return const_iterator(&FakeNode);
  }
  
  reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return std::make_reverse_iterator(end()); }
  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return std::make_reverse_iterator(begin()); }
  const_reverse_iterator crbegin() const { return std::make_reverse_iterator(cend()); }
  const_reverse_iterator crend() const { return std::make_reverse_iterator(cbegin()); }

  void push_back(const T& value) {
    insert(end(), value);
  }
  
  void push_front(const T& value) {
    insert(begin(), value);
  }

  void pop_back() {
    erase(end() - 1);
  }

  void pop_front() {
    erase(begin());
  }


  void insert(const_iterator iter, const T& value) {
    BaseNode* it = const_cast<BaseNode*>(iter.pos);
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      constructNode(node, iter.pos->prev, it, value);
    } catch (...) {
      deallocNode(node);
      throw;
    }
    it->prev->next = node;
    it->prev = node;
  }

  void erase(const_iterator iter) {
    --size_;
    BaseNode* it = const_cast<BaseNode*>(iter.pos);
    Node* node = static_cast<Node*>(it);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    NodeAllocTraits::destroy(nodeAlloc_, node);
    NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
  }

  void emplace(const_iterator iter) {
    BaseNode* it = const_cast<BaseNode*>(iter.pos);
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      NodeAllocTraits::construct(nodeAlloc_, node, it->prev, it);
      ++size_;
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
    it->prev->next = node;
    it->prev = node;
  }

  ~List() {
    while (!empty()) {
      pop_back();
    }
  }
};