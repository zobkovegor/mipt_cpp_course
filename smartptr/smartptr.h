#include <iostream>

template <typename T>
class WeakPtr;


template <typename T>
class SharedPtr;

struct BaseControlBlock {
  size_t shared_count = 0;
  size_t weak_count = 0;
  virtual ~BaseControlBlock() = default;
  virtual void destroy_object() = 0;
  virtual void destroy_block() = 0;
};

template <typename T>
struct PointerControlBlock : BaseControlBlock {
  T* ptr;
  PointerControlBlock(T* p) : ptr(p) {
    shared_count = 1;
    weak_count = 0;
  }
  void destroy_object() override {
    delete ptr;
  }
  void destroy_block() override {
    delete this;
  }
};

template <typename T, typename Deleter>
struct DeleterControlBlock : BaseControlBlock {
  T* ptr;
  Deleter deleter;
  DeleterControlBlock(T* p, Deleter d) : ptr(p), deleter(std::move(d)) {}
  void destroy_object() override {
    deleter(ptr);
  }
  void destroy_block() override {
    delete this;
  }
};

template <typename T>
struct MakeSharedControlBlock : BaseControlBlock {
  alignas(T) unsigned char object_storage[sizeof(T)];
  T* get_object() {
    return reinterpret_cast<T*>(object_storage);
  }
  template <typename... Args>
  MakeSharedControlBlock(Args&&... args) {
    new (get_object()) T(std::forward<Args>(args)...);
  }
  void destroy_object() override {
    get_object()->~T();
  }
  void destroy_block() override {
    delete this;
  }
};

template <typename T, typename Alloc>
struct AllocateSharedControlBlock : BaseControlBlock {
  alignas(T) unsigned char object_storage[sizeof(T)];
  Alloc allocator;
  T* get_object() {
    return reinterpret_cast<T*>(object_storage);
  }
  template <typename... Args>
  AllocateSharedControlBlock(const Alloc& alloc, Args&&... args) : allocator(alloc) {
    std::allocator_traits<Alloc>::construct(allocator, get_object(), std::forward<Args>(args)...);
  }
  void destroy_object() override {
    std::allocator_traits<Alloc>::destroy(allocator, get_object());
  }
  void destroy_block() override {
    using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<AllocateSharedControlBlock>;
    BlockAlloc block_alloc(allocator);
    std::allocator_traits<BlockAlloc>::deallocate(block_alloc, this, 1);
  }
};

template <typename T, typename Deleter, typename Alloc>
struct DeleterAllocatorControlBlock : BaseControlBlock {
  T* ptr;
  Deleter deleter;
  Alloc allocator;
  DeleterAllocatorControlBlock(T* p, Deleter d, Alloc alloc)
    : ptr(p), deleter(std::move(d)), allocator(alloc) {
    shared_count = 1;
    weak_count = 0;
  }
  void destroy_object() override {
    deleter(ptr);
  }
  void destroy_block() override {
    using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<DeleterAllocatorControlBlock>;
    BlockAlloc block_alloc(allocator);
    std::allocator_traits<BlockAlloc>::deallocate(block_alloc, this, 1);
  }
};

template <typename T>
class SharedPtr {
  public:

  T* ptr_;
  BaseControlBlock* count_;
  
  SharedPtr(BaseControlBlock* control) {}
  
  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;


  void weakRemove() {
    if (count_) {
      --count_->weak_count;
      if (count_->shared_count == 0 && count_->weak_count == 0) {
        delete count_;
      }
    }
  }

  SharedPtr(T* ptr, BaseControlBlock* count) : ptr_(ptr), count_(count) {
    ++count_->shared_count;
  }

  void deleteSharedPtr() {
    if (!count_) {
      return;
    }
    --count_->shared_count;
    if (count_->shared_count == 0) {
      count_->destroy_object(); 
      ptr_ = nullptr;
      if (count_->weak_count == 0) {
        count_->destroy_block();
        count_ = nullptr;
      }
    }
  }
 public:
  
  SharedPtr() : ptr_(nullptr), count_(nullptr) {}

  SharedPtr(T* ptr) : ptr_(ptr), count_(new PointerControlBlock<T>(ptr)) {}


  template <typename Deleter>
  SharedPtr(T* ptr, Deleter deleter) : ptr_(ptr), count_(new DeleterControlBlock<T, Deleter>(ptr, std::move(deleter))) {
    count_->shared_count = 1;
    count_->weak_count = 0;
  }

  template <typename Deleter, typename Alloc>
  SharedPtr(T* ptr, Deleter deleter, Alloc alloc) : ptr_(ptr) {
    using Block = DeleterAllocatorControlBlock<T, Deleter, Alloc>;
    using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Block>;
    BlockAlloc block_alloc(alloc);
    count_ = std::allocator_traits<BlockAlloc>::allocate(block_alloc, 1);
    try {
      new (count_) Block(ptr, std::move(deleter), alloc);
    } catch (...) {
      std::allocator_traits<BlockAlloc>::deallocate(block_alloc, static_cast<Block*>(count_), 1);
      throw;
    }
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other) : ptr_(other.ptr_), count_(other.count_) {
    if (count_) {
      ++count_->shared_count;
    }
  }

  SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), count_(other.count_) {
    if (count_) {
      ++count_->shared_count;
    }
  }

  template <typename U>
  SharedPtr<T>& operator=(const SharedPtr<U>& other) {
    if (count_ != other.count_ && ptr_ == other.ptr_) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      count_ = other.count_;
    }
    return *this;
  } 

  SharedPtr<T>& operator=(const SharedPtr<T>& other) {
    if (this != &other) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      count_ = other.count_;
      ++count_->shared_count;
    }
    return *this;
  } 


  template <typename U>
  SharedPtr(SharedPtr<U>&& other) : ptr_(std::move(other.ptr_)), count_(std::move(other.count_)) {
    other.count_ = nullptr;
    other.ptr_ = nullptr;
  }

  SharedPtr(SharedPtr&& other) : ptr_(std::move(other.ptr_)), count_(std::move(other.count_)) {
    other.ptr_ = nullptr;
    other.count_ = nullptr;
  }


  template <typename U>
  SharedPtr<T>& operator=(SharedPtr<U>&& other) {
    if (count_ != other.count_ ) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      count_ = other.count_;
      other.ptr_ = nullptr;
      other.count_ = nullptr;
    } else {
      --other.count_->shared_count;
      other.ptr_ = nullptr;
      other.count_ = nullptr;
    }
    return *this;
  }

  SharedPtr<T>& operator=(SharedPtr<T>&& other) {
    if (this != &other) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      count_ = other.count_;
      other.ptr_ = nullptr;
      other.count_ = nullptr;
    }
    return *this;
  }


  T& operator*() const {
    return *ptr_;
  }

  T* operator->() const {
    return ptr_;
  }

  void swap(SharedPtr& other) {
    std::swap(ptr_, other.ptr_);
    std::swap(count_, other.count_);
  }

  size_t use_count() const {
    return count_ ? count_->shared_count : 0;
  }

  const T* get() const {
    return ptr_;
  }

  void reset() {
    deleteSharedPtr();
    ptr_ = nullptr;
    count_ = nullptr;
  }

  void reset(T* ptr) {
    deleteSharedPtr();
    ptr_ = ptr;
    count_ = new PointerControlBlock<T>(ptr);
  }

  ~SharedPtr() {
    deleteSharedPtr();
  }
};


template <typename T>
class WeakPtr {
 private: 
  BaseControlBlock* count_;
  T* ptr_;

  void deleteWeakPtr() {
    if (!count_) {
      return;
    }
    --count_->weak_count;
    if (count_->shared_count == 0 && count_->weak_count == 0) {
      count_->destroy_block();
    }
  }

  template <typename U>
  friend class WeakPtr;

 public:
  
  WeakPtr() : ptr_(nullptr), count_(nullptr) {}

  template <typename U>
  WeakPtr(const SharedPtr<U>& other) : count_(other.count_), ptr_(other.ptr_) {
    if (count_) {
      ++count_->weak_count;
    }
  }

  WeakPtr(const WeakPtr& other) : count_(other.count_), ptr_(other.ptr_) {
    if (count_) { 
      ++count_->weak_count;
    }
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& other) : count_(other.count_), ptr_(other.ptr_) {
    if (count_) {
      ++count_->weak_count;
    }
  }

  WeakPtr(WeakPtr&& other) : count_(other.count_), ptr_(other.ptr_) {
    other.count_ = nullptr;
    other.ptr_ = nullptr;
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) : count_(other.count_), ptr_(other.ptr_) {
    other.count_ = nullptr;
    other.ptr_ = nullptr;
  }

  template <typename U>
  WeakPtr& operator=(const SharedPtr<U>& other) {
    deleteWeakPtr();
    count_ = other.count_;
    ptr_ = other.ptr_;
    if (count_) {
      ++count_->weak_count;
    }
    return *this;
  }

  WeakPtr& operator=(const WeakPtr& other) {
    if (this != &other) {
      deleteWeakPtr();
      count_ = other.count_;
      ptr_ = other.ptr_;
      if (count_) {
        ++count_->weak_count;
      }
    }
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    if (this != reinterpret_cast<const WeakPtr*>(&other)) {
      deleteWeakPtr();
      count_ = other.count_;
      ptr_ = other.ptr_;
      if (count_) { 
        ++count_->weak_count;
      }
    }
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    if (this != &other) {
      deleteWeakPtr();
      count_ = other.count_;
      ptr_ = other.ptr_;
    }
    other.count_->weak_count;
    other.count_ = nullptr;
    other.ptr_ = nullptr;
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) noexcept {
    if (this != reinterpret_cast<const WeakPtr*>(&other)) {
      deleteWeakPtr();
      count_ = other.count_;
      ptr_ = other.ptr_;
    }
    other.count_->weak_count;
    other.count_ = nullptr;
    other.ptr_ = nullptr;
    return *this;
    return *this;
  }

  size_t use_count() const {
    return count_ ? count_->shared_count : 0;
  }

  bool expired() const {
    return use_count() == 0;
  }

  SharedPtr<T> lock() const {
    if (count_ && count_->shared_count > 0) {
      return SharedPtr<T>(const_cast<T*>(ptr_), count_);
    }
    return SharedPtr<T>();
  }

  ~WeakPtr() {
    deleteWeakPtr();
  }

};


template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  auto* control = new MakeSharedControlBlock<T>(std::forward<Args>(args)...);
  SharedPtr<T> sp(control->get_object(), static_cast<BaseControlBlock*>(control));
  return sp;
}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using Block = AllocateSharedControlBlock<T, Alloc>;
  using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Block>;
  using BlockTraits = std::allocator_traits<BlockAlloc>;  
  BlockAlloc block_alloc(alloc);
  Block* control_block = BlockTraits::allocate(block_alloc, 1);
  try {
    new (control_block) Block(alloc, std::forward<Args>(args)...);
    SharedPtr<T> sp(control_block->get_object(), static_cast<BaseControlBlock*>(control_block));
    return sp;
  } catch (...) {
    BlockTraits::deallocate(block_alloc, control_block, 1);
    throw;
  }
}

template <typename T>
struct enable_shared_from_this {
  WeakPtr<T> sptr;

  SharedPtr<T> shared_from_this() {
    auto ptr = sptr.lock();
    if (!ptr) {
      throw;
    }
    return ptr;
  }

  SharedPtr<const T> shared_from_this() const {
    auto ptr = sptr.lock();
    if (!ptr) {
      throw ;
    }
    return ptr;
  }
};