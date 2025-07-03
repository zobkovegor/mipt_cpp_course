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

template <typename T, typename Alloc >
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
    this->~AllocateSharedControlBlock();
    std::allocator_traits<BlockAlloc>::deallocate(block_alloc, this, 1);
  }
};

template <typename T, typename Deleter = std::default_delete<T>, typename Alloc = std::allocator<T>>
struct DeleterAllocatorControlBlock : BaseControlBlock {
  T* ptr;
  [[no_unique_address]] Deleter deleter;
  [[no_unique_address]] Alloc allocator;
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
    this->~DeleterAllocatorControlBlock();
    std::allocator_traits<BlockAlloc>::deallocate(block_alloc, this, 1);
  }
};

template <typename T>
class SharedPtr {
 private:

  T* ptr_;
  BaseControlBlock* block_;
  
  SharedPtr(BaseControlBlock* control) : block_(control) {}
  
  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  void weakRemove() {
    if (block_) {
      --block_->weak_count;
      if (block_->shared_count == 0 && block_->weak_count == 0) {
        block_->destroy_block();
      }
    }
  }

  void deleteSharedPtr() {
    if (!block_) {
      return;
    }
    --block_->shared_count;
    if (block_->shared_count == 0) {
      block_->destroy_object(); 
      ptr_ = nullptr;
      if (block_->weak_count == 0) {
        block_->destroy_block();
        block_ = nullptr;
      }
    }
  }

  void set_null() {
    ptr_ = nullptr;
    block_ = nullptr;
  }

 public:
  
  SharedPtr() : ptr_(nullptr), block_(nullptr) {}

  SharedPtr(T* ptr, BaseControlBlock* count) : ptr_(ptr), block_(count) {
    ++block_->shared_count;
  }

  template <typename Deleter = std::default_delete<T>, typename Alloc = std::allocator<T>>
  SharedPtr(T* ptr, Deleter deleter = Deleter(), Alloc alloc = Alloc()) : ptr_(ptr) {
    using Block = DeleterAllocatorControlBlock<T, Deleter, Alloc>;
    using BlockAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Block>;
    BlockAlloc block_alloc(alloc);
    block_ = std::allocator_traits<BlockAlloc>::allocate(block_alloc, 1);
    try {
      new (block_) Block(ptr, std::move(deleter), alloc);
    } catch (...) {
      std::allocator_traits<BlockAlloc>::deallocate(block_alloc, static_cast<Block*>(block_), 1);
      throw;
    }
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other) : ptr_(other.ptr_), block_(other.block_) {
    if (block_) {
      ++block_->shared_count;
    }
  }

  SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), block_(other.block_) {
    if (block_) {
      ++block_->shared_count;
    }
  }

  template <typename U>
  SharedPtr<T>& operator=(const SharedPtr<U>& other) {
    if (block_ != other.block_ && ptr_ == other.ptr_) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      block_ = other.block_;
    }
    return *this;
  } 

  SharedPtr<T>& operator=(const SharedPtr<T>& other) {
    if (this != &other) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      block_ = other.block_;
      ++block_->shared_count;
    }
    return *this;
  } 


  template <typename U>
  SharedPtr(SharedPtr<U>&& other) : ptr_(std::move(other.ptr_)), block_(std::move(other.block_)) {
    other.set_null();
  }

  SharedPtr(SharedPtr&& other) : ptr_(std::move(other.ptr_)), block_(std::move(other.block_)) {
    other.set_null();
  }


  template <typename U>
  SharedPtr<T>& operator=(SharedPtr<U>&& other) {
    if (block_ != other.block_ ) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      block_ = other.block_;
      other.set_null();
    } else {
      other.reset();
    }
    return *this;
  }

  SharedPtr<T>& operator=(SharedPtr<T>&& other) {
    if (this != &other) {
      deleteSharedPtr();
      ptr_ = other.ptr_;
      block_ = other.block_;
      other.set_null();
    }
    return *this;
  }


  T& operator*() const {
    return *ptr_;
  }

  T* operator->() const {
    return ptr_;
  }

  size_t use_count() const {
    return block_ ? block_->shared_count : 0;
  }

  const T* get() const {
    return ptr_;
  }

  void reset() {
    deleteSharedPtr();
    set_null();
  }

  void reset(T* ptr) {
    deleteSharedPtr();
    ptr_ = ptr;
    block_ = new DeleterAllocatorControlBlock<T>(ptr, std::default_delete<T>{}, std::allocator<T>{});
  }

  void swap(SharedPtr& other) {
    std::swap(ptr_, other.ptr_);
    std::swap(block_, other.block_);
  }

  ~SharedPtr() {
    deleteSharedPtr();
  }
};


template <typename T>
class WeakPtr {
 private: 
  BaseControlBlock* block_;
  T* ptr_;

  void deleteWeakPtr() {
    if (!block_) {
      return;
    }
    --block_->weak_count;
    if (block_->shared_count == 0 && block_->weak_count == 0) {
      block_->destroy_block();
    }
  }

  template <typename U>
  friend class WeakPtr;

  void set_null() {
    block_ = nullptr;
    ptr_ = nullptr;
  }

 public:
  
  WeakPtr() : block_(nullptr), ptr_(nullptr) {}

  template <typename U>
  WeakPtr(const SharedPtr<U>& other) : block_(other.block_), ptr_(other.ptr_) {
    if (block_) {
      ++block_->weak_count;
    }
  }

  WeakPtr(const WeakPtr& other) : block_(other.block_), ptr_(other.ptr_) {
    if (block_) { 
      ++block_->weak_count;
    }
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& other) : block_(other.block_), ptr_(other.ptr_) {
    if (block_) {
      ++block_->weak_count;
    }
  }

  WeakPtr(WeakPtr&& other) : block_(other.block_), ptr_(other.ptr_) {
    other.set_null();
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) : block_(other.block_), ptr_(other.ptr_) {
    other.set_null();
  }

  template <typename U>
  WeakPtr& operator=(const SharedPtr<U>& other) {
    deleteWeakPtr();
    block_ = other.block_;
    ptr_ = other.ptr_;
    if (block_) {
      ++block_->weak_count;
    }
    return *this;
  }

  WeakPtr& operator=(const WeakPtr& other) {
    if (this != &other) {
      deleteWeakPtr();
      block_ = other.block_;
      ptr_ = other.ptr_;
      if (block_) {
        ++block_->weak_count;
      }
    }
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    if (this != reinterpret_cast<const WeakPtr*>(&other)) {
      deleteWeakPtr();
      block_ = other.block_;
      ptr_ = other.ptr_;
      if (block_) { 
        ++block_->weak_count;
      }
    }
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    if (this != &other) {
      deleteWeakPtr();
      block_ = other.block_;
      ptr_ = other.ptr_;
    }
    other.set_null();    
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) noexcept {
    if (this != reinterpret_cast<const WeakPtr*>(&other)) {
      deleteWeakPtr();
      block_ = other.block_;
      ptr_ = other.ptr_;
    }
    other.set_null();
    return *this;
  }

  size_t use_count() const {
    return block_ ? block_->shared_count : 0;
  }

  bool expired() const {
    return use_count() == 0;
  }

  SharedPtr<T> lock() const {
    if (block_ && block_->shared_count > 0) {
      return SharedPtr<T>(const_cast<T*>(ptr_), block_);
    }
    return SharedPtr<T>();
  }

  ~WeakPtr() {
    deleteWeakPtr();
  }

};

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

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
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