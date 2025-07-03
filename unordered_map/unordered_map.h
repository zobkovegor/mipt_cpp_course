#include <iostream>
#include <memory>
#include <vector>
#include <type_traits>
#include <iterator>
#include <cstdint>

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  using NodeType = std::pair<const Key, Value>;
 private:
 
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
  };

  struct Node : BaseNode {
    NodeType kv;
    uint64_t hash;
  };

  using AllocTraits = std::allocator_traits<Alloc>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;

  BaseNode fakeNode_;
  std::vector<BaseNode*> arr_;
  [[no_unique_address]] NodeAlloc nodeAlloc_;
  [[no_unique_address]] Hash hasher_;
  [[no_unique_address]] Equal equaler_;
  double maxLoadFactor_;
  size_t size_ = 0;

  void create(const NodeType& kv) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }
    uint64_t hash = Hash()(kv.first);
    size_t index = hash % arr_.size();
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      Alloc allocator(nodeAlloc_);
      AllocTraits::construct(allocator, &node->kv, std::move(kv));
      node->hash = hash;
      if (!arr_[index]) {
        node->prev = fakeNode_.prev;
        node->next = &fakeNode_;
        fakeNode_.prev->next = node;
        fakeNode_.prev = node;
        arr_[index] = node;
      } else {
        node->prev = arr_[index]->prev;
        arr_[index]->prev->next = node;
        arr_[index]->prev = node;
        node->next = arr_[index];
        arr_[index] = node;
      }
      ++size_;
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
  }

  void create(NodeType&& kv) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      Alloc allocator(nodeAlloc_);
      AllocTraits::construct(allocator, &node->kv, std::move(kv));
      node->hash = Hash(node->kv.first);
      size_t index = node->hash % arr_.size();
      if (!arr_[index]) {
        node->prev = fakeNode_.prev;
        node->next = &fakeNode_;
        fakeNode_.prev->next = node;
        fakeNode_.prev = node;
        arr_[index] = node;
      } else {
        node->prev = arr_[index]->prev;
        arr_[index]->prev->next = node;
        arr_[index]->prev = node;
        node->next = arr_[index];
        arr_[index] = node;
      }
      ++size_;
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
  }

  void rehash(size_t new_capacity) {
    std::vector<BaseNode*> new_arr(new_capacity, nullptr);
    BaseNode new_fake_node;
    new_fake_node.next = &new_fake_node;
    new_fake_node.prev = &new_fake_node;

    Node* current = static_cast<Node*>(fakeNode_.next);
    while (current != &fakeNode_) {
      Node* next_node = static_cast<Node*>(current->next);
      current->prev->next = current->next;
      current->next->prev = current->prev;
      size_t new_index = current->hash % new_capacity;
      if (new_arr[new_index] == nullptr) {
        current->prev = new_fake_node.prev;
        current->next = &new_fake_node;
        new_fake_node.prev->next = current;
        new_fake_node.prev = current;
        new_arr[new_index] = current;
      } else {
        current->prev = new_arr[new_index]->prev;
        current->next = new_arr[new_index];
        new_arr[new_index]->prev->next = current;
        new_arr[new_index]->prev = current;
        new_arr[new_index] = current;
      }
      current = next_node;
    }
    arr_ = std::move(new_arr);
    fakeNode_.next = new_fake_node.next;
    fakeNode_.prev = new_fake_node.prev;
    fakeNode_.next->prev = &fakeNode_;
    fakeNode_.prev->next = &fakeNode_;
  }

  void clear() {
    Node* current = static_cast<Node*>(fakeNode_.next);
    while (current != &fakeNode_) {
      Node* next_node = static_cast<Node*>(current->next);
      Alloc alloc(nodeAlloc_);
      AllocTraits::destroy(alloc, &current->kv);
      NodeAllocTraits::deallocate(nodeAlloc_, current, 1);
      current = next_node;
    }
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    std::fill(arr_.begin(), arr_.end(), nullptr);
    size_ = 0;
}

  void swap(UnorderedMap& other) {
    std::swap(arr_, other.arr_);
    std::swap(size_, other.size_);
    std::swap(maxLoadFactor_, other.maxLoadFactor_);
    std::swap(fakeNode_, other.fakeNode_);
    if (fakeNode_.next != &fakeNode_) {
      fakeNode_.next->prev = &fakeNode_;
    }
    if (fakeNode_.prev != &fakeNode_) {
      fakeNode_.prev->next = &fakeNode_;
    }
    if (other.fakeNode_.next != &other.fakeNode_) {
      other.fakeNode_.next->prev = &other.fakeNode_;
    }
    if (other.fakeNode_.prev != &other.fakeNode_) {
      other.fakeNode_.prev->next = &other.fakeNode_;
    }
    if (AllocTraits::propagate_on_container_swap::value) {
      std::swap(nodeAlloc_, other.nodeAlloc_);
    }
  }
 
  Node* find_node(const Key& key, uint64_t hash) const {
    if (arr_.empty()) return nullptr;
    size_t index = hash % arr_.size();
    Node* it = static_cast<Node*>(arr_[index]);
    while (it != nullptr && it->hash == hash) {
      if (equaler_(it->kv.first, key)) {
        return it;
      }
      if (it->next == &fakeNode_) break;
      it = static_cast<Node*>(it->next);
    }
    return nullptr;
  }

 public: 
  
  UnorderedMap(const Alloc& allocator, const Hash& hasher, const Equal& equaler, double loadFactor) :
              nodeAlloc_(allocator), hasher_(hasher),
              equaler_(equaler), maxLoadFactor_(loadFactor), size_(0) {
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    reserve(8);
  }

  UnorderedMap(const Alloc& allocator = Alloc()) :
              nodeAlloc_(allocator), maxLoadFactor_(1.0), size_(0) {
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    reserve(8);
  }


  UnorderedMap(const UnorderedMap& other) : UnorderedMap(AllocTraits::select_on_container_copy_construction(other.nodeAlloc_),
                                                        other.hasher_, other.equaler_, other.maxLoadFactor_) {
    arr_.resize(other.arr_.size(), nullptr);
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    Node* HelpNode = static_cast<Node*>(other.fakeNode_.next);
    while (HelpNode != &other.fakeNode_) {
      create(HelpNode->kv);
      HelpNode = static_cast<Node*>(HelpNode->next);
    }
  }

  UnorderedMap(const UnorderedMap& other, Alloc allocator) : UnorderedMap(allocator, other.hasher_, other.equaler_, other.maxLoadFactor_) {
    arr_.resize(other.arr_.size(), nullptr);
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    Node* HelpNode = static_cast<Node*>(other.fakeNode_.next);
    while (HelpNode != &other.fakeNode_) {
      create(HelpNode->kv);
      HelpNode = static_cast<Node*>(HelpNode->next);
    }
  }

  
  UnorderedMap(UnorderedMap&& other)
   : arr_(std::move(other.arr_)),
    nodeAlloc_(std::move(other.nodeAlloc_)),
    hasher_(other.hasher_), equaler_(other.equaler_),
    maxLoadFactor_(other.maxLoadFactor_),
    size_(other.size_) {
    fakeNode_.next = other.fakeNode_.next;
    fakeNode_.prev = other.fakeNode_.prev;
    fakeNode_.next->next = &fakeNode_;
    fakeNode_.prev->prev = &fakeNode_;
    other.fakeNode_.next = &other.fakeNode_;
    other.fakeNode_.prev = &other.fakeNode_;
    other.size_ = 0;
  } 

  UnorderedMap& operator=(const UnorderedMap& other) {
    if (AllocTraits::propagate_on_container_copy_assignment::value) {
      nodeAlloc_ = other.nodeAlloc_;
    }
    UnorderedMap help(other, nodeAlloc_);
    swap(help);
    return *this;
  }
  
  UnorderedMap& operator=(UnorderedMap&& other) {
    if (this != &other) {
      clear();
      if (AllocTraits::propagate_on_container_move_assignment::value) {
        nodeAlloc_ = std::move(other.nodeAlloc_);
      }
      arr_ = std::move(other.arr_);
      maxLoadFactor_ = other.maxLoadFactor_;
      fakeNode_.next = other.fakeNode_.next;
      fakeNode_.prev = other.fakeNode_.prev;
      fakeNode_.next->next = &fakeNode_;
      fakeNode_.prev->prev = &fakeNode_;
      other.fakeNode_.next = &other.fakeNode_;
      other.fakeNode_.prev = &other.fakeNode_;
      size_ = other.size_;
      other.size_ = 0;
    }
    return *this;
  }

  double get_load_factor() const {
    return static_cast<double>(size_) / static_cast<double>(arr_.size());  
  }

  void set_max_load_factor(double newLoadFactor) {
    maxLoadFactor_ = newLoadFactor;
  }

  double get_max_load_factor() const {
    return maxLoadFactor_;
  }

  Value& operator[](Key key) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }
    uint64_t hash = Hash()(key);
    size_t index = hash % arr_.size();
    Node* node = static_cast<Node*>(arr_[index]);
    while (node != nullptr && node->hash == hash) {
      if (Equal()(node->kv.first, key)) {
        return node->kv.second;
      }
      if (node->next == &fakeNode_) {
        break;
      }
      node = static_cast<Node*>(node->next);
    }
    Node* new_node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    NodeType new_kv(std::move(key), Value());
    Alloc allocator(nodeAlloc_);
    AllocTraits::construct(allocator, &new_node->kv, new_kv);
    new_node->hash = hash;

    new_node->prev = fakeNode_.prev;
    new_node->next = &fakeNode_;

    fakeNode_.prev->next = new_node;
    fakeNode_.prev = new_node;

    arr_[index] = new_node;
    ++size_;
    return new_node->kv.second;
  }

  Value& at(const Key& key) {
    uint64_t hash = hasher_(key);
    Node* node = find_node(key, hash);
    if (!node) {
      throw std::out_of_range("Key not found");
    }
    return node->kv.second;
  }

  const Value& at(const Key& key) const {
    uint64_t hash = hasher_(key);
    Node* node = find_node(key, hash);
    if (!node) {
      throw std::out_of_range("Key not found");
    }
    return node->kv.second;
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
    using pointer_type = std::conditional_t<IsConst, const NodeType*, NodeType*>;
    using reference_type = std::conditional_t<IsConst, const NodeType&, NodeType&>;
    friend UnorderedMap;

 private:
    BaseNode* iter_;

    base_iterator(BaseNode* iter) : iter_(iter) {}

    template<bool OtherConst = IsConst, typename = std::enable_if_t<OtherConst>>
    base_iterator(const BaseNode* iter) : iter_(const_cast<BaseNode*>(iter)) {}

 public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = NodeType;
    using difference_type = std::ptrdiff_t;
    using pointer = pointer_type;
    using reference = reference_type;

    base_iterator() : iter_() {}
    base_iterator(const base_iterator&) = default;

    template<bool OtherIsConst, typename = std::enable_if_t<IsConst && !OtherIsConst>>
    base_iterator(const base_iterator<OtherIsConst>& other) : iter_(other.iter_) {}

    base_iterator& operator=(const base_iterator&) = default;

    reference operator*() const {
      return (static_cast<Node*>(iter_)->kv);
    }

    pointer operator->() const {
      return &(static_cast<Node*>(iter_)->kv);
    }

    base_iterator& operator++() {
      iter_ = iter_->next;
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator tmp = *this;
      iter_ = iter_->next;;
      return tmp;
    }

    bool operator==(const base_iterator& other) const {
      return iter_ == other.iter_;
    }

    uint64_t currHash() {
      return iter_->hash;
    }
  };
  
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    return iterator(fakeNode_.next);
  }

  const_iterator begin() const {
    return const_iterator(fakeNode_.next);
  }

  const_iterator cbegin() const {
    return begin();
  }

  iterator end() {
    return iterator(&fakeNode_);
  }

  const_iterator end() const {
    return const_iterator(&fakeNode_);
  }

  const_iterator cend() const {
    return end();
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }

  iterator find(const Key& key) {
    uint64_t hash = hasher_(key);
    Node* node = find_node(key, hash);
    return node ? iterator(node) : end();
  }

  const_iterator find(const Key& key) const {
    uint64_t hash = hasher_(key);
    Node* node = find_node(key, hash);
    return node ? const_iterator(node) : end();
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      Alloc allocator(nodeAlloc_);
      AllocTraits::construct(allocator, &node->kv, std::forward<Args>(args)...);
      node->hash = hasher_(node->kv.first);

      uint64_t hash = node->hash;
      size_t index = hash % arr_.size();
        
      if (arr_[index]) {
        Node* it = static_cast<Node*>(arr_[index]);
        while (it != &fakeNode_ && it->hash == hash) {
          if (equaler_(it->kv.first, node->kv.first)) {
            NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
            return {iterator(it), false};
          }
          it = static_cast<Node*>(it->next);
        }
      }
      node->prev = fakeNode_.prev;
      node->next = &fakeNode_;
      fakeNode_.prev->next = node;
      fakeNode_.prev = node;
      if (!arr_[index]) {
        arr_[index] = node;
      } else {
        node->next = arr_[index];
        arr_[index]->prev = node;
        arr_[index] = node;
      }
      ++size_;
      return {iterator(node), true};
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
  }

  void erase(iterator pos) {
    if (pos == end()) {
      --size_;
      return;
    }
    Node* node_to_erase = static_cast<Node*>(pos.iter_);
    uint64_t hash = node_to_erase->hash;
    size_t index = hash % arr_.size();
    if (arr_[index] == node_to_erase) {
      if (node_to_erase->next != &fakeNode_ && 
        static_cast<Node*>(node_to_erase->next)->hash == hash) {
        arr_[index] = static_cast<Node*>(node_to_erase->next);
      } else {
        arr_[index] = nullptr;
      }
    }
    node_to_erase->prev->next = node_to_erase->next;
    node_to_erase->next->prev = node_to_erase->prev;
    Alloc allocator(nodeAlloc_);
    AllocTraits::destroy(allocator, &node_to_erase->kv);
    NodeAllocTraits::deallocate(nodeAlloc_, node_to_erase, 1);
    --size_;
  }

  void erase(iterator first, iterator last) {
    while (first != last) {
      erase(first++);
    }
  }

  std::pair<iterator, bool> insert(std::pair<Key, Value>&& args) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      Alloc allocator(nodeAlloc_);
      AllocTraits::construct(allocator, &node->kv, std::move(args));
      node->hash = hasher_  (node->kv.first);

      uint64_t hash = node->hash;
      size_t index = hash % arr_.size();
      if (arr_[index]) {
        Node* it = static_cast<Node*>(arr_[index]);
        while (it != nullptr && it->hash == hash) {
          if (equaler_(it->kv.first, node->kv.first)) {
            NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
            return {iterator(it), false};
          }
          ++it;
        }
      }
      if (!arr_[index]) {
        node->prev = fakeNode_.prev;
        node->next = &fakeNode_;

        fakeNode_.prev->next = node;
        fakeNode_.prev = node;

        arr_[index] = node;
      } else {
        node->prev = arr_[index]->prev;
        arr_[index]->prev->next = node;
        arr_[index]->prev = node;
        node->next = arr_[index];
        arr_[index] = node;
      }
      ++size_;
      return {iterator(arr_[index]), true};
    } catch (...) {
      NodeAllocTraits::destroy(nodeAlloc_, node);
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
  }

  template <typename InputIterator>
  void insert(InputIterator first, InputIterator last) {
    for (; first != last; ++first) {
      emplace((*first));  
    }
  }
  
  void reserve(size_t cap) {
    arr_.resize(cap);
  }

  ~UnorderedMap() {
    clear();
  }
};