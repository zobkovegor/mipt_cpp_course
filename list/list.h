#include <iostream>
#include <memory>
#include <type_traits>
#include <iterator>

template<size_t N>
class StackStorage {
  alignas(alignof(std::max_align_t)) std::byte buffer[N];
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

  template<typename T, size_t M>
  friend class StackAllocator;
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
  void construct(U* ptr, Args&... args) {
    new (ptr) U(args...);
  }

  template <typename U>
  void destruct(U* ptr) {
    ptr->~U();
  }

  bool operator==(const StackAllocator& other) const noexcept {
    return storage == other.storage;
  }

  bool operator!=(const StackAllocator& other) const noexcept {
    return storage != other.storage;
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
    BaseNode* prev;
    BaseNode* next;
  };

  struct Node : BaseNode {
    T value;
    Node(BaseNode* prev, BaseNode* next, const T& val) : BaseNode{prev, next}, value(val) {}
    Node() = default;
  };

  using AllocTraits = std::allocator_traits<Alloc>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;
  
  BaseNode FakeNode;
  NodeAlloc nodeAlloc_;
  size_t size_; 
 
  void swap(List& other) {
    std::swap(FakeNode, other.FakeNode);
    std::swap(size_, other.size_);
    if (AllocTraits::propagate_on_container_swap::value) {
      std::swap(nodeAlloc_, other.nodeAlloc_);
    }
  }

 public:
  
 List(const Alloc& allocator = Alloc()) : nodeAlloc_(allocator), size_(0) {
    FakeNode.next = &FakeNode;
    FakeNode.prev = &FakeNode;
  }

  List(size_t cap, const Alloc& allocator = Alloc()) : nodeAlloc_(allocator), size_(cap) {
    FakeNode.next = &FakeNode;
    FakeNode.prev = &FakeNode;
    try {
      for (size_t i = 0; i != cap; ++i) {
        Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
        NodeAllocTraits::construct(nodeAlloc_, node);
        node->prev = FakeNode.prev;
        node->next = &FakeNode;
        FakeNode.prev->next = node;
        FakeNode.prev = node;
        if (i == 0) {
          FakeNode.next = node;
        }
      }
    } catch (...) {
      BaseNode* current = FakeNode.next;
      while (current != &FakeNode) {
        BaseNode* next = current->next;
        NodeAllocTraits::destroy(nodeAlloc_, static_cast<Node*>(current));
        NodeAllocTraits::deallocate(nodeAlloc_, static_cast<Node*>(current), 1);
        current = next;
      }
      throw;
    }
  }

  List(size_t cap, const T& val, const Alloc& allocator = Alloc()) : nodeAlloc_(allocator), size_(cap) {
    FakeNode.next = &FakeNode;
    FakeNode.prev = &FakeNode;
    try {
      for (size_t i = 0; i != cap; ++i) {
        Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
        NodeAllocTraits::construct(nodeAlloc_, node, FakeNode.prev, &FakeNode, val);
        FakeNode.prev->next = node;
        FakeNode.prev = node;
        if (i == 0) {
          FakeNode.next = node;
        }
      }
    } catch (...) {
      BaseNode* current = FakeNode.next;
      while (current != &FakeNode) {
        BaseNode* next = current->next;
        NodeAllocTraits::destroy(nodeAlloc_, static_cast<Node*>(current));
        NodeAllocTraits::deallocate(nodeAlloc_, static_cast<Node*>(current), 1);
        current = next;
      }
      throw;
    }
  }

  List(const List& other) : nodeAlloc_(AllocTraits::select_on_container_copy_construction(other.nodeAlloc_)), size_(0) {
    FakeNode.next = &FakeNode;
    FakeNode.prev = &FakeNode;
    try {
      for (const auto& item : other) {
        push_back(item);
      }
    } catch (...) {
      while (!empty()) {
        pop_back();
      }
      throw;
    }
  }

  List& operator=(const List& other) {
    if (AllocTraits::propagate_on_container_copy_assignment::value) {
      nodeAlloc_ = other.nodeAlloc_;
    }
    List help(other);
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
  
  void push_back(const T& value) {
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      ++size_;
      NodeAllocTraits::construct(nodeAlloc_, node, FakeNode.prev, &FakeNode, value);
    } catch (...) {
      --size_;
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
    FakeNode.prev->next = node;
    FakeNode.prev = node;
  }
  
  void push_front(const T& value) {
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      ++size_;
      NodeAllocTraits::construct(nodeAlloc_, node, &FakeNode, FakeNode.next, value);
    } catch (...) {
      --size_;
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
    FakeNode.next->prev = node;
    FakeNode.next = node;
  }

  void pop_back() {
    --size_;
    Node* node = static_cast<Node*>(FakeNode.prev);
    FakeNode.prev = node->prev;
    node->prev->next = &FakeNode;
    NodeAllocTraits::destroy(nodeAlloc_, node);
    NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
  }

  void pop_front() {
    --size_;
    Node* node = static_cast<Node*>(FakeNode.next);
    FakeNode.next = node->next;
    node->next->prev = &FakeNode;
    NodeAllocTraits::destroy(nodeAlloc_, node);
    NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
  }

  template<bool IsConst>
  class base_iterator {
   public:
    using pointer_type = std::conditional_t<IsConst, const T*, T*>;
    using reference_type = std::conditional_t<IsConst, const T&, T&>;
    using baseNode_type = std::conditional_t<IsConst, const BaseNode*, BaseNode*>;
    using node_type = std::conditional_t<IsConst, const Node*, Node*>;
    friend List;
   private:
    baseNode_type pos;
    base_iterator(baseNode_type pos) : pos(pos) {}
    
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
      return static_cast<node_type>(pos)->value;
    }
  
    pointer_type operator->() const {
      return &(static_cast<node_type>(pos)->value);
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

  void insert(const_iterator iter, const T& value) {
    BaseNode* it = const_cast<BaseNode*>(iter.pos);
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      ++size_;
      NodeAllocTraits::construct(nodeAlloc_, node, Node{iter.pos->prev, it, value});
    } catch (...) {
      --size_;
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
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

  ~List() {
    while (!empty()) {
      pop_back();
    }
  }
};