#include <iostream>
#include <type_traits>
#include <initializer_list>
#include <utility>

template <typename... Ts>
concept AllDefaultConstructible = (std::is_default_constructible_v<Ts> && ...);

template <typename... Ts>
concept AllCopyConstructible = (std::is_copy_constructible_v<Ts> && ...);

template <typename T>
concept ImplicitlyCopyConvertible = std::is_convertible_v<const T&, T>;

template <typename T, typename... Ts>
struct AllSameSize : std::bool_constant<
  ((sizeof(T) == sizeof(Ts)) && ...)
> {};

template <typename... Ts>
struct AllSizeGreaterThanOne : std::bool_constant<
    ((sizeof(std::remove_reference_t<Ts>) > 1) && ...)
> {};

template <typename T, typename U>
constexpr bool check_const_ref =
    std::is_constructible_v<T, U> &&
    (!std::is_reference_v<T> || 
     (std::is_reference_v<T> && std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>));

template <typename TupleT>
concept IsTuple = requires {
  typename std::remove_cvref_t<TupleT>::tuple_tag;
};


template <size_t N, typename T>
struct TupleVal {
    T value;
    TupleVal() requires(!std::is_reference_v<T> && std::is_default_constructible_v<T>) : value() {}
 
    template <typename U>
    TupleVal(U&& val) : value(std::forward<U>(val)) {}

    template <typename U>
    TupleVal(const U& val) : value(val) {}

    template <typename U>
    TupleVal(U& val) : value(val) {}

    template <typename U>
    requires(std::is_rvalue_reference_v<T> && 
             std::is_lvalue_reference_v<U> && 
             std::is_same_v<std::remove_reference_t<T>, std::remove_reference_t<U>>)
    TupleVal(U&& val) : value(std::move(val)) {}

    TupleVal(const TupleVal& other) : value(other.value) {}

    TupleVal(TupleVal&& other) : value(std::move(other.value)) {}

    TupleVal& operator=(const TupleVal& other) {
        value = other.value;
        return *this;
    }
    TupleVal& operator=(TupleVal&& other) {
        value = std::move(other.value);
        return *this;
    }
};

template <size_t N, typename... Items>
struct TuplePos;

template <typename... Items>
using Tuple = TuplePos<0, Items...>;

template <size_t N>
struct TuplePos<N> {
    using tuple_tag = void;
    TuplePos() = default;
    template <typename... UTypes>
    TuplePos(const Tuple<UTypes...>&) {}
    static constexpr size_t tuple_size() noexcept { return 0; }
};


template <size_t N, typename Head>
struct TuplePos<N, Head> : TupleVal<N, Head> {
    using tuple_tag = void;

    static constexpr size_t tuple_size() noexcept { return 1; }

    
    TuplePos() = default;
    
    template <typename UHead>
    requires (IsTuple<UHead>)
    TuplePos(UHead&& arg) : TupleVal<N, Head>(std::move(get<N>(arg))) {}
    
    template <typename UHead>
    TuplePos(UHead&& arg) : TupleVal<N, Head>(std::forward<UHead>(arg)) {}

    template <typename UHead>
    requires (IsTuple<UHead>)
    TuplePos(UHead& arg) : TupleVal<N, Head>(get<N>(arg)) {}


    template <typename UHead>
    TuplePos(UHead& arg) : TupleVal<N, Head>(arg) {}

    template <typename... UTypes>
    TuplePos(const Tuple<UTypes...>& other)
    requires (sizeof...(UTypes) == 1)
        : TupleVal<N, Head>(get<N>(other)) {}

    template <typename... UTypes>
    TuplePos& operator=(const Tuple<UTypes...>& other)
    requires (sizeof...(UTypes) == 1)
    {
        TupleVal<N, Head>::value = get<N>(other);
        return *this;
    }

    template <typename UTypes>
    TuplePos(Tuple<UTypes>&& other)
        : TupleVal<N, Head>(std::forward<UTypes>(get<N>(other))) {}

    template <typename UTypes>
    TuplePos& operator=(Tuple<UTypes>&& other)
    {
        TupleVal<N, Head>::value = std::forward<UTypes>(get<N>(other));
        return *this;
    }

};

template <size_t N, typename Head, typename... Tail>
Head& get(TuplePos<N, Head, Tail...>& tuple) {
    return tuple.TupleVal<N, Head>::value;
}

template <size_t N, typename Head, typename... Tail>
const Head& get(const TuplePos<N, Head, Tail...>& tuple) {
    return tuple.TupleVal<N, Head>::value;
}

template <size_t N, typename Head, typename... Tail>
decltype(auto) get(TuplePos<N, Head, Tail...>&& tuple) {
    if constexpr (std::is_lvalue_reference_v<Head>) {
        return tuple.TupleVal<N, Head>::value;
    }
    else if constexpr (std::is_rvalue_reference_v<Head>) {
        return std::move(tuple.TupleVal<N, Head>::value);
    }
    else {
        return std::move(tuple.TupleVal<N, Head>::value);
    }
}

template <size_t N, typename Head, typename... Tail>
decltype(auto) get(const TuplePos<N, Head, Tail...>&& tuple) {
    if constexpr (std::is_lvalue_reference_v<Head>) {
        return tuple.TupleVal<N, Head>::value;
    }
    else if constexpr (std::is_rvalue_reference_v<Head>) {
        return std::move(tuple.TupleVal<N, Head>::value);
    }
    else {
        return std::move(tuple.TupleVal<N, Head>::value);
    }
}


template <typename T, typename... Ts>
struct type_index;

template <typename T, typename... Ts>
struct type_index<T, T, Ts...> : std::integral_constant<size_t, 0> {};

template <typename T, typename U, typename... Ts>
struct type_index<T, U, Ts...> : 
    std::integral_constant<size_t, 1 + type_index<T, Ts...>::value> {};

template <typename T, typename... Ts>
struct type_count;

template <typename T>
struct type_count<T> : std::integral_constant<size_t, 0> {};

template <typename T, typename U, typename... Ts>
struct type_count<T, U, Ts...> : 
    std::integral_constant<size_t, (std::is_same_v<T, U> ? 1 : 0) + type_count<T, Ts...>::value> {};

template <typename T, typename... Ts>
decltype(auto) get(Tuple<Ts...>& tuple) {
    static_assert(type_count<T, Ts...>::value == 1);
    constexpr size_t idx = type_index<T, Ts...>::value;
    return get<idx>(tuple);
}


template <typename T, typename... Ts>
decltype(auto) get(const Tuple<Ts...>& tuple) {
    static_assert(type_count<T, Ts...>::value == 1);
    constexpr size_t idx = type_index<T, Ts...>::value;
    return get<idx>(tuple);
}


template <typename T, typename... Ts>
decltype(auto) get(Tuple<Ts...>&& tuple) {
    static_assert(type_count<T, Ts...>::value == 1);
    constexpr size_t idx = type_index<T, Ts...>::value;
    return get<idx>(std::forward<Tuple<Ts...>>(tuple));
}

template <typename T, typename... Ts>
decltype(auto) get(const Tuple<Ts...>&& tuple) {
    static_assert(type_count<T, Ts...>::value == 1);
    constexpr size_t idx = type_index<T, Ts...>::value;
    return get<idx>(std::forward<const Tuple<Ts...>>(tuple));
}

template <typename T1, typename T2>
TuplePos(const std::pair<T1, T2>&) -> TuplePos<0, T1, T2>;

template <typename T1, typename T2>
TuplePos(std::pair<T1, T2>&&) -> TuplePos<0, T1, T2>;

template <size_t Start, typename TupleT, typename... Ts>
concept AllAssignableFromGet = []<size_t... Is>(std::index_sequence<Is...>) {
    return (std::is_assignable_v<Ts&, decltype(get<Start + Is>(std::declval<TupleT>()))> && ...);
}(std::make_index_sequence<sizeof...(Ts)>{});

template <size_t Start, typename TupleT, typename... Ts>
concept AllConstructibleFromGet = []<size_t... Is>(std::index_sequence<Is...>) {
    return (std::is_constructible_v<Ts, decltype(get<Start + Is>(std::declval<TupleT>()))> && ...);
}(std::make_index_sequence<sizeof...(Ts)>{});

template <size_t Start, typename TupleT, typename... Ts>
concept AllConvertibleFromGet = []<size_t... Is>(std::index_sequence<Is...>) {
    return (std::is_convertible_v<decltype(get<Start + Is>(std::declval<TupleT>())), Ts> && ...);
}(std::make_index_sequence<sizeof...(Ts)>{});


template <size_t N, typename Head, typename... Tail>
struct TuplePos<N, Head, Tail...> :
    TupleVal<N, Head>,
    TuplePos<N + 1, Tail...>
{
    static constexpr size_t tuple_size() noexcept { return sizeof...(Tail) + 1; }
    using tuple_tag = void;
  
    TuplePos() requires AllDefaultConstructible<Head, Tail...> {}

    explicit TuplePos() requires (AllDefaultConstructible<Head, Tail...> && 
        !((std::is_constructible_v<Head, {}> && 
           (!std::is_constructible_v<Tail, {}> && ...))))
    {}

    explicit(!(std::is_convertible_v<const Head&, Head> && (std::is_convertible_v<const Tail&, Tail> && ...)))
    TuplePos(Head&& arg, Tail&&... tail)
    requires ((std::is_copy_constructible_v<Head> || 
              (std::is_rvalue_reference_v<Head> && 
               std::is_lvalue_reference_v<std::remove_reference_t<Head>&>)) &&
              (std::is_copy_constructible_v<Tail> && ...))
        : TupleVal<N, Head>(std::forward<Head>(arg)),
          TuplePos<N + 1, Tail...>(std::forward<Tail>(tail)...)
    {}

    template <typename UHead, typename... UTail>
    requires ((sizeof...(Tail) == sizeof...(UTail)) &&
              sizeof...(Tail) >= 1 &&
              (std::is_constructible_v<Head, UHead> || 
               (std::is_rvalue_reference_v<Head> && 
                std::is_lvalue_reference_v<UHead> && 
                std::is_same_v<std::remove_reference_t<Head>, std::remove_reference_t<UHead>>)) &&
              (std::is_constructible_v<Tail, UTail> && ...))
    explicit(!(std::is_convertible_v<Head, UHead> && (std::is_convertible_v<Tail, UTail> && ...)))
    TuplePos(UHead&& arg, UTail&&... tail) 
        : TupleVal<N, Head>(std::forward<UHead>(arg)),
          TuplePos<N+1, Tail...>(std::forward<UTail>(tail)...)
    {}

    template <typename UHead, typename... UTail>    
    explicit(!AllConvertibleFromGet<N, const Tuple<UHead, UTail...>&, Head, Tail...>)
    TuplePos(const Tuple<UHead, UTail...>& other)
    requires (AllConstructibleFromGet<N, const Tuple<UHead, UTail...>&, Head, Tail...> &&
    (sizeof...(Tail) + 1 != 1 ||
     !(std::is_convertible_v<const Tuple<UHead, UTail...>&, Head> ||
       std::is_constructible_v<Head, const Tuple<UHead, UTail...>&> ||
       std::is_same_v<Head, UHead, UTail...>)))
        : TupleVal<N, Head>(get<N>(std::forward<decltype(other)>(other))),
          TuplePos<N + 1, Tail...>(static_cast<const TuplePos<N + 1, UTail...>&>(other))
    {}
    
    template <typename UHead, typename... UTail>
    explicit(!AllConvertibleFromGet<N, const Tuple<UHead, UTail...>&, Head, Tail...>)
    TuplePos(Tuple<UHead, UTail...>&& other)
    requires (AllConstructibleFromGet<N, const Tuple<UHead, UTail...>&, Head, Tail...> &&
    (sizeof...(Tail) + 1 != 1 ||
     !(std::is_convertible_v<const Tuple<UHead, UTail...>&, Head> ||
       std::is_constructible_v<Head, const Tuple<UHead, UTail...>&> ||
       std::is_same_v<Head, UHead, UTail...>)))
        : TupleVal<N, Head>(get<N>(std::forward<decltype(other)>(other))),
          TuplePos<N + 1, Tail...>(std::forward<decltype(static_cast<const TuplePos<N + 1, UTail...>&>(other))>(other))
    {}

    TuplePos(const TuplePos& other)
    requires (std::is_copy_constructible_v<Head> && (std::is_copy_constructible_v<Tail> && ...)) = default;

    TuplePos& operator=(const TuplePos& other)
    requires (std::is_copy_assignable_v<Head> && (std::is_copy_assignable_v<Tail> && ...))
      {
      TupleVal<N, Head>::value = get<N>(other);
      TuplePos<N + 1, Tail...>::operator=(static_cast<const TuplePos<N + 1, Tail...>&>(other));
      return *this;
    }

    TuplePos& operator=(TuplePos& other)
    requires (std::is_copy_assignable_v<Head> && (std::is_copy_assignable_v<Tail> && ...))
      {
      TupleVal<N, Head>::value = get<N>(other);
      TuplePos<N + 1, Tail...>::operator=(static_cast<TuplePos<N + 1, Tail...>&>(other));
      return *this;
    }

    TuplePos(TuplePos&& other)
    requires(std::is_move_constructible_v<Head> && (std::is_move_constructible_v<Tail> && ...) || 
    (std::is_reference_v<Head> && (std::is_reference_v<Tail> && ...)))
        : TupleVal<N, Head>(std::move(get<N>(other))),
          TuplePos<N + 1, Tail...>(std::move(other))
    {}

    TuplePos& operator=(TuplePos&& other)
    requires ((std::is_move_assignable_v<Head>) && (std::is_move_assignable_v<Tail> && ...))
    {
        TupleVal<N, Head>::value = std::forward<Head>(get<N>(other));
        TuplePos<N + 1, Tail...>::operator=((other));
        return *this;
    }

    template <typename UHead, typename... UTail>
    TuplePos& operator=(const Tuple<UHead, UTail...>& other)
    requires (sizeof...(UTail) == sizeof...(Tail) && 
              AllAssignableFromGet<N, const Tuple<UHead, UTail...>&, Head, Tail...>)
    {
        TupleVal<N, Head>::value = std::forward<const Head>(get<N>(other));
        TuplePos<N + 1, Tail...>::operator=(other);
        return *this;
    }

    template <typename UHead, typename... UTail>
    TuplePos& operator=(Tuple<UHead, UTail...>& other)
    requires (sizeof...(UTail) == sizeof...(Tail) && 
              AllAssignableFromGet<N, Tuple<UHead, UTail...>&, Head, Tail...>)
    {
        TupleVal<N, Head>::value = get<N>(other);
        TuplePos<N + 1, Tail...>::operator=(other);
        return *this;
    }

    template <typename UHead, typename... UTail>
    TuplePos& operator=(Tuple<UHead, UTail...>&& other)
    requires (sizeof...(UTail) == sizeof...(Tail) && 
              AllAssignableFromGet<N, const Tuple<UHead, UTail...>&, Head, Tail...>) {
        TupleVal<N, Head>::value = std::forward<UHead>(get<N>(other));
      TuplePos<N + 1, Tail...>::operator=(other);
      return *this;
    }

    template <typename U1, typename U2>
    TuplePos(const std::pair<U1, U2>& p)
        requires (sizeof...(Tail) == 1 && 
                std::is_constructible_v<Head, const U1&> && 
                std::is_constructible_v<Tail..., const U2&>)
        : TupleVal<N, Head>(p.first),
        TuplePos<N + 1, Tail...>(p.second) {}

    template <typename U1, typename U2>
    TuplePos(std::pair<U1, U2>& p)
        requires (sizeof...(Tail) == 1 && 
                std::is_constructible_v<Head, const U1&> && 
                std::is_constructible_v<Tail..., const U2&>)
        : TupleVal<N, Head>(p.first),
        TuplePos<N + 1, Tail...>(p.second) {}

    template <typename U1, typename U2>
    TuplePos(std::pair<U1, U2>&& p)
        requires (sizeof...(Tail) == 1 && 
                std::is_constructible_v<Head, U1&&> && 
                std::is_constructible_v<Tail..., U2&&>)
        : TupleVal<N, Head>(std::forward<U1>(p.first)),
        TuplePos<N + 1, Tail...>(std::forward<U2>(p.second)) {}    

    TuplePos(TuplePos&& other) = default;
};

template <typename... Ts>
Tuple<std::decay_t<Ts>...> makeTuple(Ts&&... args) {
    return Tuple<std::decay_t<Ts>...>(std::forward<Ts>(args)...);
}