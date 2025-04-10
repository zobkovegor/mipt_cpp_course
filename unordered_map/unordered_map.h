#include "list.h"

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  using NodeType = std::pair<const Key, Value>;

 private:
 
  struct Node {
    NodeType kv;
    uint64_t hash;
    Node(const NodeType& val, uint64_t hash) : kv(val), hash(hash) {}
    Node(NodeType&& val, uint64_t hash) : kv(std::move(val)), hash(hash) {}
    Node() = default;
    Node(Node&&) = default;
    Node& operator=(Node&&) = default;
  };

  using AllocTraits = std::allocator_traits<Alloc>;
  using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;
  using ListIterator = typename List<Node, NodeAlloc>::iterator;

  List<Node, NodeAlloc> lst_;
  std::vector<ListIterator> arr_;
  NodeAlloc nodeAlloc_;
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
      NodeAllocTraits::construct(nodeAlloc_, node, kv, hash);
      if (arr_[index].isNull()) {
        lst_.push_back(std::move(*node));
        arr_[index] = --lst_.end();
      } else {
        lst_.insert(arr_[index], std::move(*node));
        --arr_[index];
      }
      ++size_;
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
    NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
  }

  void create(NodeType&& kv) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }
    uint64_t hash = Hash()(kv.first);
    size_t index = hash % arr_.size();
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      NodeAllocTraits::construct(nodeAlloc_, node, std::move(kv), hash);
      if (arr_[index].isNull()) {
        lst_.push_back(std::move(*node));
        arr_[index] = --lst_.end();
      } else {
        lst_.insert(arr_[index], std::move(*node));
        --arr_[index];
      }
      ++size_;
    } catch (...) {
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
    NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
  }

  void clear() {
    lst_.~List();
    arr_.clear();
    size_ = 0;
  }

  void swap(UnorderedMap& other) {
    std::swap(lst_, other.lst_);
    std::swap(arr_, other.arr_);
    std::swap(size_, other.size_);
    std::swap(maxLoadFactor_, other.maxLoadFactor_);
    if (AllocTraits::propagate_on_container_swap::value) {
      std::swap(nodeAlloc_, other.nodeAlloc_);
    }
  }

  void rehash(size_t new_bucket_count) {
    if (new_bucket_count == 0) {
      return;
    }
    std::vector<ListIterator> new_arr(new_bucket_count, ListIterator());
    for (auto it = lst_.begin(); it != lst_.end(); ++it) {
      size_t new_index = it->hash % new_bucket_count;

      if (new_arr[new_index].isNull()) {
        new_arr[new_index] = it;
      } else {
        lst_.insert(new_arr[new_index], std::move(*it));
        new_arr[new_index] = --new_arr[new_index];
      }
    }
    arr_ = std::move(new_arr);
  }
 
 public: 
  
  UnorderedMap(const Alloc& allocator = Alloc()) : lst_(allocator), nodeAlloc_(allocator), maxLoadFactor_(2.0) {}


  UnorderedMap(const UnorderedMap& other) : lst_(AllocTraits::select_on_container_copy_construction(other.lst_.get_allocator())),
    arr_(other.arr_.size(), ListIterator()),
    nodeAlloc_(AllocTraits::select_on_container_copy_construction(other.nodeAlloc_)),
    maxLoadFactor_(other.maxLoadFactor_),
    size_(0) {
    try {
      for (const auto& vals : other.lst_) {
        create(vals.kv);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  UnorderedMap(UnorderedMap&& other) : lst_(std::move(other.lst_)),
    arr_(std::move(other.arr_)),
    nodeAlloc_(std::move(other.nodeAlloc_)),
    maxLoadFactor_(other.maxLoadFactor_),
    size_(other.size_) {
    other.size_ = 0;
  } 

  UnorderedMap& operator=(const UnorderedMap& other) {
    UnorderedMap help(other);
    swap(help);
    return *this;
  }
  
  UnorderedMap& operator=(UnorderedMap&& other) {
    if (this != &other) {
      clear();
      if (AllocTraits::propagate_on_container_move_assignment::value) {
        nodeAlloc_ = std::move(other.nodeAlloc_);
      }
      lst_ = std::move(other.lst_);
      arr_ = std::move(other.arr_);
      maxLoadFactor_ = other.maxLoadFactor_;
      size_ = other.size_;
      other.size_ = 0;
    }
    return *this;
  }

  double get_load_factor() const {
    return static_cast<double>(lst_.size()) / static_cast<double>(arr_.size());  
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
    ListIterator it = arr_[index];
    while (it != ListIterator() && it != lst_.end() && it->hash == hash) {
      if (Equal()(it->kv.first, key)) {
        return it->kv.second;
      }
      ++it;
    }
    NodeType new_kv(std::move(key), Value());
    lst_.push_back(Node{std::move(new_kv), hash});
    ListIterator new_it = --lst_.end();
    arr_[index] = new_it;
    ++size_;
    return new_it->kv.second;
  }

  Value& at(const Key& key) {
    if (arr_.empty()) {
      throw std::out_of_range("Key not found");
    }
    uint64_t hash = Hash()(key);
    size_t index = hash % arr_.size();
    auto it = arr_[index];
    while (it != ListIterator() && it != lst_.end() && it->hash == hash) {
      if (Equal()(it->kv.first, key)) {
        return it->kv.second;
      }
      ++it;
    }
    throw std::out_of_range("Key not found");
  }

  const Value& at(const Key& key) const {
    if (arr_.empty()) {
      throw std::out_of_range("Key not found");
    }
    uint64_t hash = Hash()(key);
    size_t index = hash % arr_.size();
    ListIterator it = arr_[index];
    while (it != ListIterator() && it != lst_.end() && it->hash == hash) {
      if (Equal()(it->kv.first, key)) {
        return it->kv.second;
      }
      ++it;
    }
    throw std::out_of_range("Key not found");
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
    using list_iterator_type = std::conditional_t<IsConst, 
                                                  typename List<Node, NodeAlloc>::const_iterator, 
                                                  typename List<Node, NodeAlloc>::iterator>;
    friend UnorderedMap;

 private:
    list_iterator_type list_iter_;
    uint64_t hash;
    base_iterator(list_iterator_type list_iter) : list_iter_(list_iter) {}
 public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = NodeType;
    using difference_type = std::ptrdiff_t;
    using pointer = pointer_type;
    using reference = reference_type;

    base_iterator() : list_iter_() {}
    base_iterator(const base_iterator&) = default;

    template<bool OtherIsConst, typename = std::enable_if_t<IsConst && !OtherIsConst>>
    base_iterator(const base_iterator<OtherIsConst>& other) : list_iter_(other.list_iter_) {}

    base_iterator& operator=(const base_iterator&) = default;

    reference operator*() const {
      return list_iter_->kv;
    }

    pointer operator->() const {
      return &(list_iter_->kv);
    }

    base_iterator& operator++() {
      ++list_iter_;
      return *this;
    }

    base_iterator operator++(int) {
      base_iterator tmp = *this;
      ++list_iter_;
      return tmp;
    }

    bool operator==(const base_iterator& other) const {
      return list_iter_ == other.list_iter_;
    }

    bool operator!=(const base_iterator& other) const {
      return !(*this == other);
    }
    uint64_t currHash() {
      return list_iter_->hash;
    }
  };
  
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() {
    return iterator(lst_.begin());
  }

  const_iterator begin() const {
    return const_iterator(lst_.begin());
  }

  const_iterator cbegin() const {
    return const_iterator(lst_.cbegin());
  }

  iterator end() {
    return iterator(lst_.end());
  }

  const_iterator end() const {
  return const_iterator(lst_.end());
  }

  const_iterator cend() const {
    return const_iterator(lst_.cend());
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
    uint64_t hash = Hash()(key);
    uint64_t index = hash % arr_.size();
    if (arr_[index] == ListIterator()) {
      return end();
    }
    iterator it = arr_[index];
    while (it != ListIterator() && it != lst_.end() && it.currHash() == hash) {
      if (Equal()(it->first, key)) {
        return it;
      }
      ++it;
    }
    return end();
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }
    Node* node = NodeAllocTraits::allocate(nodeAlloc_, 1);
    try {
      NodeAllocTraits::construct(nodeAlloc_, &node->kv, std::forward<Args>(args)...);
      node->hash = Hash()(node->kv.first);

      uint64_t hash = node->hash;
      size_t index = hash % arr_.size();
      if (!arr_[index].isNull()) {
        ListIterator it = arr_[index];
        while (it != lst_.end() && it->hash == hash) {
          if (Equal()(it->kv.first, node->kv.first)) {
            NodeAllocTraits::destroy(nodeAlloc_, node);
            NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
            return {iterator(it), false};
          }
          ++it;
        }
      }
      ListIterator new_it;
      if (arr_[index].isNull()) {
        lst_.push_back(std::move(*node));
        new_it = --lst_.end();
        arr_[index] = new_it;
      } else {
        lst_.insert(arr_[index], std::move(*node));
        new_it = --arr_[index];
      }
      ++size_;
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      return {iterator(new_it), true};
    } catch (...) {
      NodeAllocTraits::destroy(nodeAlloc_, node);
      NodeAllocTraits::deallocate(nodeAlloc_, node, 1);
      throw;
    }
  }

  void erase(iterator pos) {
    if (pos == end()) {
      return;
    }
    ListIterator list_it = pos.list_iter_;
    uint64_t hash = list_it->hash;
    size_t index = hash % arr_.size();
    if (arr_[index] == list_it) {
      auto next_it = list_it;
      ++next_it;
      if (next_it != lst_.end() && next_it->hash == hash) {
        arr_[index] = next_it;
      } else {
        arr_[index] = ListIterator();
      }
    }
    lst_.erase(list_it);
    --size_;
    return;
  }

  void erase(iterator first, iterator last) {
    if (first == last) {
      return;
    }
    while (first != last) {
      auto list_it = first.list_iter_;
      uint64_t hash = list_it->hash;
      size_t index = hash % arr_.size();
      if (arr_[index] == list_it) {
        auto next_it = list_it;
        ++next_it;
        if (next_it != lst_.end() && next_it->hash == hash) {
          arr_[index] = next_it;
        } else {
          arr_[index] = ListIterator();
        }
      }
      first = iterator(lst_.erase(list_it));
      --size_;
    }
    return;
  } 

  std::pair<iterator, bool> insert(NodeType&& args) {
    if (arr_.empty() || static_cast<double>(size_ + 1) / arr_.size() > maxLoadFactor_) {
      rehash(arr_.empty() ? 16 : arr_.size() * 2);
    }

    NodeType kv(std::move(args));
    uint64_t hash = Hash()(kv.first);
    size_t index = hash % arr_.size();

    if (!arr_[index].isNull()) {
      ListIterator it = arr_[index];
      while (it != lst_.end() && it->hash == hash) {
        if (Equal()(it->kv.first, kv.first)) {
          return {iterator(it), false};
        }
        ++it;
      }
    }
    ListIterator new_it;
    if (arr_[index].isNull()) {
      lst_.push_back(Node{std::move(kv), hash});
      new_it = --lst_.end();
      arr_[index] = new_it;
    } else {
      lst_.insert(arr_[index], Node{std::move(kv), hash});
      --arr_[index];
      new_it = arr_[index];
    }
    ++size_;
    return {iterator(new_it), true};
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

};