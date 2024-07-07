// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1030r6.pdf

#include <iostream>
#include <llfio/llfio.hpp>

namespace llfio = llfio_v2_b1279174;

namespace std::filesystem {

using llfio::path_view;

struct path_view_like {
  path_view view;

  template <typename T, std::enable_if_t<std::is_convertible_v<T, path_view> &&
                                             !std::is_convertible_v<T, path>,
                                         int> = 0>
  path_view_like(const T &p) : view(p) {}
};

uintmax_t file_size(path_view_like p) {
  return file_size(p.view.path());
}
uintmax_t file_size(path_view_like p, error_code &ec) noexcept {
  return 0;
}
} // namespace std::filesystem

int main(int argc, char** argv) {
  std::cout << std::filesystem::file_size(llfio::path_view(argv[0]));
}
