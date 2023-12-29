# UnTup

A small C++20 library to flatten nested tuple-like objects into a single tuple
of copies or references.

also known as

A sliiightly overengineered solution to a simple problem of having a single
structured binding in a `for` loop that uses `std::views::zip` and
`std::views::enumerate`.

### Does
What is claims to do (no warranty though), `constexpr`-enabled. By default,
performs some tests in compile-time, too.

### Includes

* `untup::untie()` - converts nested tuple-like objects into a tuple of
references:
```c++
auto t = std::tuple{ std::pair{ 'c', 3.1415 }, 2 };
auto [ c, pi, two ] = untup::untie(t);
pi = 3.0; // t is changed!
```
* `untup::untup()` - converts nested tuple-like objects into a tuple of copies:
```c++
auto t = std::tuple{ std::pair{ 'c', 3.1415 }, 2 };
auto [ c, pi, two ] = untup::untup(t);
pi = 3.0; // Ehm.. OK? `t` doesn't care
```
* `untup::views::flatten` - a shortcut to `std::views::transform` that calls
`untup::untup()` on the element of the range:
```c++
for (
    auto [ idx, e1, e2 ] : std::views::zip(v1, v2)
                           | std::views::enumerate
                           | untup::views::flatten
) println("{}: {} and {}", idx, e1, e2);
// Instead of
for (
    auto [ idx, pair ] : std::views::zip(v1, v2)
                         | std::views::enumerate
                         | untup::views::flatten
) {
    auto [ e1, e2 ] = pair; // Eww, why?
    println("{}: {} and {}", idx, e1, e2);
}
```

### Preserves
Reference types when copying with `untup::untup()`!

### Requires
At least C++20.

### Known issues
Doesn't build as a module on GCC.

### Adding to your project

`git clone` this repo into your project. Add
```cmake
set( UT_HEADERONLY ON ) # Only if you don't want the module
add_subdirectory( path/to/UnTup )

target_link_libraries( YourTarget UnTup ) # Or UnTup::UnTup
```
to your `CMakeLists.txt`.
Use with `#include <untup.hh>` or `import untup;`.

#### Alternatively

Just copy `untup.hh` somewhere in your project and `#include` it.
