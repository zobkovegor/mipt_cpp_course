#include <iostream>
#include <type_traits>
#include <cstring>
#include <functional>


template <typename T>
class Function;

template <typename Ret, typename... Args>
class Function<Ret(Args...)> {
 private:
  static const size_t BUFFER_SIZE = 32;
  alignas(max_align_t) char buffer[BUFFER_SIZE];
  void* fptr;

  using invoke_ptr_t = Ret(*)(void*, Args...);
  using destroy_ptr_t = void(*)(void*);
  using copy_ptr_t = void(*)(void*, const void*);

  invoke_ptr_t invoke_ptr;
  destroy_ptr_t destroy_ptr;
  copy_ptr_t copy_ptr;

  template <typename Class, typename Method>
  class pmf_wrapper {
   private:
    Method Class::*wrp;

   public:
    pmf_wrapper(Method Class::*pmf) : wrp(pmf) {}

    Ret operator()(Args... args) {
      return std::invoke(wrp, std::forward<Args>(args)...);
    }
  };

  template <typename F>
  F&& cast_mem_fn(F&& f) {
    return std::forward<F>(f);
  }

  template <typename CLASS, typename METHOD>
  pmf_wrapper<CLASS, METHOD> cast_mem_fn(METHOD CLASS::*pmf) {
    return pmf_wrapper<CLASS, METHOD>(pmf);
  }

 public:
  Function() : fptr(nullptr), invoke_ptr(nullptr), destroy_ptr(nullptr), copy_ptr(nullptr) {}

  Function(std::nullptr_t) : fptr(nullptr), invoke_ptr(nullptr), destroy_ptr(nullptr), copy_ptr(nullptr) {}

  Function(Ret (*func)(Args...)) {
    static_assert(std::is_copy_constructible_v<decltype(func)>);
    std::memcpy(buffer, &func, sizeof(func));
    fptr = buffer;
    invoke_ptr = [](void* ptr, Args... args) -> Ret {
      Ret (*f)(Args...);
      std::memcpy(&f, ptr, sizeof(f));
      return f(std::forward<Args>(args)...);
    };
    destroy_ptr = [](void*) {};
    copy_ptr = [](void* dest, const void* src) {
      std::memcpy(dest, src, sizeof(Ret (*)(Args...)));
    };
  }

  template <typename T, typename... Ts>
  Function(T (*func)(Ts...)) {
    static_assert(std::is_copy_constructible_v<decltype(func)>);
    std::memcpy(buffer, &func, sizeof(func));
    fptr = buffer;
    invoke_ptr = [](void* ptr, Args... args) -> Ret {
      T (*f)(Ts...);
      std::memcpy(&f, ptr, sizeof(f));
      return f(std::forward<Ts>(args)...);
    };
    destroy_ptr = [](void*) {};
    copy_ptr = [](void* dest, const void* src) {
      std::memcpy(dest, src, sizeof(Ret (*)(Args...)));
    };
  }

  template <typename F>
  requires (!std::is_same_v<std::decay_t<F>, Function> && std::is_copy_constructible_v<F> &&
            (std::is_invocable_r_v<Ret, F, Args...>))
  Function(F&& functor) : Function(1, cast_mem_fn(std::forward<F>(functor))) {}

  Ret operator()(Args... args) const {
    if (!invoke_ptr) {
      throw std::bad_function_call();
    }
    return (*invoke_ptr)(fptr, static_cast<Args>(args)...);
  }

  ~Function() {
    if (destroy_ptr) {
      destroy_ptr(fptr);
    }
  }

  Function(const Function& other) : invoke_ptr(other.invoke_ptr), destroy_ptr(other.destroy_ptr), copy_ptr(other.copy_ptr) {
    if (copy_ptr) {
      copy_ptr(buffer, other.buffer);
      fptr = buffer;
    } else {
      fptr = nullptr;
    }
  }

  Function& operator=(const Function& other) {
    if (this != &other) {
      if (destroy_ptr) {
        destroy_ptr(fptr);
      }
      invoke_ptr = other.invoke_ptr;
      destroy_ptr = other.destroy_ptr;
      copy_ptr = other.copy_ptr;
      if (copy_ptr) {
        copy_ptr(buffer, other.buffer);
        fptr = buffer;
      } else {
        fptr = nullptr;
      }
    }
    return *this;
  }

  Function(Function&& other) : invoke_ptr(other.invoke_ptr), destroy_ptr(other.destroy_ptr), copy_ptr(other.copy_ptr), fptr(other.fptr) {
    std::memcpy(buffer, other.buffer, BUFFER_SIZE);
    other.invoke_ptr = nullptr;
    other.destroy_ptr = nullptr;
    other.copy_ptr = nullptr;
    other.fptr = nullptr;
  }

  Function& operator=(Function&& other) {
    if (this != &other) {
      if (destroy_ptr) {
        destroy_ptr(fptr);
      }
      invoke_ptr = other.invoke_ptr;
      destroy_ptr = other.destroy_ptr;
      copy_ptr = other.copy_ptr;
      fptr = other.fptr;
      std::memcpy(buffer, other.buffer, BUFFER_SIZE);
      other.invoke_ptr = nullptr;
      other.destroy_ptr = nullptr;
      other.copy_ptr = nullptr;
      other.fptr = nullptr;
    }
    return *this;
  }

  template <typename F>
  requires (!std::is_same_v<std::decay_t<F>, Function> &&
            std::is_invocable_r_v<Ret, F, Args...>)
  Function& operator=(F&& functor) {
    if (destroy_ptr) {
      destroy_ptr(fptr);
    }
    auto&& forwarded = cast_mem_fn(std::forward<F>(functor));
    using ForwardedType = std::decay_t<decltype(forwarded)>;

    new (buffer) ForwardedType(forwarded);
    fptr = buffer;
    invoke_ptr = [](void* ptr, Args... args) -> Ret {
      return (*reinterpret_cast<ForwardedType*>(ptr))(std::forward<Args>(args)...);
    };
    destroy_ptr = [](void* ptr) {
      reinterpret_cast<ForwardedType*>(ptr)->~ForwardedType();
    };
    copy_ptr = [](void* dest, const void* src) {
      new (dest) ForwardedType(*reinterpret_cast<const ForwardedType*>(src));
    };
    return *this;
  }

  void* target() const {
    if (!invoke_ptr) {
      return nullptr;
    }
    return fptr;
  }

  explicit operator bool() const noexcept {
    return invoke_ptr != nullptr;
  }

 private:
  template <typename F>
  Function(int, const F& func) : invoke_ptr(reinterpret_cast<invoke_ptr_t>(&invoke<F>)),
                                 destroy_ptr(reinterpret_cast<destroy_ptr_t>(&destroy<F>)) {
    if constexpr (sizeof(F) > BUFFER_SIZE) {
      fptr = new F(func);
    } else {
      new (buffer) F(func);
      fptr = buffer;
    }
    copy_ptr = [](void* dest, const void* src) {
      new (dest) F(*reinterpret_cast<const F*>(src));
    };
  }

  template <typename F>
  static Ret invoke(F* fptr, Args... args) {
    return (*fptr)(std::forward<Args>(args)...);
  }

  template <typename F>
  static void destroy(F* fptr) {
    if constexpr (sizeof(F) > BUFFER_SIZE) {
      delete fptr;
    } else {
      fptr->~F();
    }
  }
};

template <typename Ret, typename... Args>
Function(Ret (*)(Args...)) -> Function<Ret(Args...)>;

template <typename Ret, typename... Args>
bool operator==(const Function<Ret(Args...)>& curr, const Function<Ret(Args...)>& other) {
  if (&curr == &other) {
    return true;
  }
  return curr.target() == other.target();
}

template <typename Ret, typename... Args>
bool operator!=(const Function<Ret(Args...)>& curr, const Function<Ret(Args...)>& other) {
  return !(curr == other);
}

template <typename Ret, typename... Args>
bool operator==(const Function<Ret(Args...)>& func, std::nullptr_t) noexcept {
  return !func;
}

template <typename Ret, typename... Args>
bool operator==(std::nullptr_t, const Function<Ret(Args...)>& func) noexcept {
  return func == nullptr;
}

template <typename Ret, typename... Args>
bool operator!=(const Function<Ret(Args...)>& func, std::nullptr_t) noexcept {
  return !(func == nullptr);
}

template <typename Ret, typename... Args>
bool operator!=(std::nullptr_t, const Function<Ret(Args...)>& func) noexcept {
  return func != nullptr;
}