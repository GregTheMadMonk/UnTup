#if __cplusplus < 202002L
#error "At least C++20 is required to use untup!"
#endif

#ifndef UT_EXPORT
#include <concepts>
#include <functional>
#include <ranges>
#include <type_traits>
#include <tuple>

#ifdef UT_DEBUG
#include <iostream>
#include <source_location>
#define UT_NO_TESTING // prints are not constexpr!
#endif

#define UT_EXPORT
#endif

namespace untup {

/// @brief operator+ overload for folding
namespace tuple_sum {

template <typename... Ts1, typename... Ts2>
inline constexpr
auto operator+(const std::tuple<Ts1...>& t1, const std::tuple<Ts2...>& t2) {
    return std::apply(
        [&t2] (Ts1... els1) {
            return std::apply(
                [&els1...] (Ts2... els2) -> std::tuple<Ts1..., Ts2...> {
                    return { els1..., els2... };
                }, t2
            );
        }, t1
    );
}

} // <-- namespace tuple_sum

// Taken (with modifications) from
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2165r2.pdf
namespace p2165r2 {

template <typename T, std::size_t N>
concept is_tuple_element = requires (T t) { // exposition only
    typename std::tuple_element_t<N, std::remove_const_t<T>>;
    { get<N>(t) } -> std::convertible_to<std::tuple_element_t<N, T>&>;
};

template <typename T>
concept tuple_like // exposition only
= !std::is_reference_v<T> && requires {
    { std::tuple_size_v<T> } -> std::convertible_to<std::size_t>;
} && []<std::size_t... I>(std::index_sequence<I...>)
{ return (is_tuple_element<T, I> && ...); }(std::make_index_sequence<std::tuple_size_v<T>>{});

} // <-- namespace p2165r2

/// @brief Implementation detail
namespace detail {

#ifdef UT_DEBUG

template <std::size_t line, typename T> inline void print_type()
{ std::cerr << std::source_location::current().function_name() << '\n'; }

#define PRINT_TYPE(line, t) ::untup::detail::print_type<line, t>();

#else

#define PRINT_TYPE(line, t) ;

#endif

/// @brief Similar to `std::add_const` but converts `T&` to `const T&`
template <typename T>
struct AddConst { using Type = std::add_const_t<T>; };

template <typename T>
struct AddConst<T&> { using Type = const T&; };

template <typename T>
using AddConst_T = AddConst<T>::Type;

/// @brief A reference that stores the original type. Is used to allow
///        "references to references" while flattening
template <typename T>
struct RefTo {
    using Type = T;
    using StdRef = std::reference_wrapper<std::remove_reference_t<T>>;
    StdRef ref;

    inline constexpr
    RefTo(T& t) : ref(t) {}
};

template <typename T>
inline constexpr
auto untie(RefTo<T> element) {
    PRINT_TYPE(__LINE__, T);
    return std::tuple{ element };
}

template <template <typename...> class Tup, typename... Elements>
requires p2165r2::tuple_like<Tup<Elements...>>
inline constexpr
auto untie(RefTo<Tup<Elements...>> tup) {
    PRINT_TYPE(__LINE__, std::tuple<Elements...>);
    return std::apply(
        [] (Elements&... elements) {
            using namespace tuple_sum;
            return (... + untie(RefTo<Elements>(elements)));
        }, tup.ref.get()
    );
}

template <template <typename...> class Tup, typename... Elements>
requires p2165r2::tuple_like<Tup<Elements...>>
inline constexpr
auto untie(RefTo<const Tup<Elements...>> tup) {
    PRINT_TYPE(__LINE__, const std::tuple<Elements...>);
    return std::apply(
        [] (const Elements&... elements) {
            using namespace tuple_sum;
            return (... + untie(RefTo<AddConst_T<Elements>>(elements)));
        }, tup.ref.get()
    );
}

} // <-- detail

/// @brief Flatten nested tuple-like objects into a tuple of references
UT_EXPORT
template <typename T>
inline constexpr
auto untie(T& t) {
    return std::apply(
        [] <typename... Refs> (Refs... refs) {
            return std::tuple<typename Refs::Type&...>{ refs.ref.get()... };
        }, detail::untie(detail::RefTo<T>{t})
    );
}

/// @brief Prevent binding to temporaries
UT_EXPORT
auto untie(auto&& t) = delete;

/// @brief Flattent nested tuple-like objects into a tuple of copies of
///        elements
UT_EXPORT
template <typename... Elements>
inline constexpr auto untup(const std::tuple<Elements...>& tup) {
    return std::apply(
        [] <typename... Refs> (Refs... refs) {
            return
                std::tuple<
                    typename Refs::Type...
                >{ refs.ref.get()... };
        },
        detail::untie(
            detail::RefTo<std::tuple<Elements...>>{
                const_cast<std::tuple<Elements...>&>(tup)
            }
        ) // Ugly
    );
}

/// @brief Range element flattening
namespace views {

/// @brief Flatten the range element. Useful to apply to the result of
///        `std::zip() | std::enumerate` to allow a singe structured binding
///        in a `for` loop
UT_EXPORT
inline constexpr
auto flatten = std::views::transform(
    [] (const auto& v) { return untup(v); }
);

} // <-- namespace views

#ifndef UT_NO_TESTING
/// @brief Compile-time test
namespace test {

using namespace tuple_sum;

constexpr std::tuple t1{ 10, 20 };
constexpr std::tuple t2{ 'c', 'd' };

static_assert(t1 + t2 == std::tuple{ 10, 20, 'c', 'd' });

consteval int testMethod() {
    auto test1 = std::tuple{ 1, std::tuple{ 2, 3 } };
    auto res1 = untie(test1);
    static_assert(std::same_as<decltype(res1), std::tuple<int&, int&, int&>>);
    if (res1 != std::tuple{ 1, 2, 3 }) return __LINE__;

    const auto test2 = test1;
    auto res2 = untie(test2);
    static_assert(std::same_as<decltype(res2), std::tuple<const int&, const int&, const int&>>);
    if (res2 != std::tuple{ 1, 2, 3 }) return __LINE__;

    std::tuple<std::tuple<int, char>, const int> test3{ { 10, 'c' }, 25 };
    auto res3 = untie(test3);
    static_assert (std::same_as<decltype(res3), std::tuple<int&, char&, const int&>>);
    if (res3 != std::tuple{ 10, 'c', 25 }) return __LINE__;

    const auto& test4 = test3;
    auto res4 = untie(test4);
    static_assert(std::same_as<decltype(res4), std::tuple<const int&, const char&, const int&>>);
    if (res4 != std::tuple{ 10, 'c', 25 }) return __LINE__;

    auto res5 = untup(test3);
    auto res6 = untup(test4);

    // Copying doesn't change the type even if const
    static_assert(std::same_as<decltype(res5), std::tuple<int, char, const int>>);
    static_assert(std::same_as<decltype(res6), std::tuple<int, char, const int>>);

    int i = 0; char c = 'c';
    auto test7 =
        std::tuple<std::size_t, std::tuple<int&, const char&>>{
            10, { i, c }
        };
    auto res7 = untie(test7);
    static_assert(
        std::same_as<
            decltype(res7),
            std::tuple<std::size_t&, int&, const char&>
        >
    );
    auto res8 = untup(test7);
    static_assert(
        std::same_as<
            decltype(res8),
            std::tuple<std::size_t, int&, const char&>
        >
    );
    const auto& test9 = test7;
    auto res9 = untie(test9);
    static_assert(
        std::same_as<
            decltype(res9),
            std::tuple<const std::size_t&, const int&, const char&>
        >
    );
    auto res10 = untup(test9);
    static_assert(
        std::same_as<
            decltype(res10),
            std::tuple<std::size_t, int&, const char&>
        >
    );


    std::tuple<std::size_t, std::pair<int&, const char&>> test11{
        10, { i, c }
    };
    auto res11 = untie(test11);
    static_assert(std::same_as<decltype(res11), decltype(res7)>);
    if (res11 != res7) return __LINE__;
    std::get<1>(res11) = 45;
    if (res11 != res7) return __LINE__;

#if __cplusplus >= 202302L
    std::array vi{ 9,   8,   7   };
    std::array vc{ 'a', 'b', 'c' };
    for (
        auto [ idx, i, c ] : std::views::zip(vi, vc)
                             | std::views::enumerate
                             | views::flatten
    ) {
        if (idx == 1) i = 10;
    }
    if (vi[1] != 10) return __LINE__;
#endif

    return 0;
}

static_assert(testMethod() == 0);

} // <-- namespace test
#endif

} // <-- namespace untup
