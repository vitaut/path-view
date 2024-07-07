// file_size implementation from libc++.

#include <sys/stat.h>
#include <filesystem>

using namespace std::filesystem;


template <class T> T error_value();
template <> inline constexpr void error_value<void>() {}
template <> inline bool error_value<bool>() { return false; }
template <> inline uintmax_t error_value<uintmax_t>() { return uintmax_t(-1); }

template <class T> struct ErrorHandler {
  const char *func_name_;
  std::error_code *ec_ = nullptr;
  const char *p1_ = nullptr;
  const path *p2_ = nullptr;

  ErrorHandler(const char *fname, std::error_code *ec, const char *p1 = nullptr,
               const path *p2 = nullptr)
      : func_name_(fname), ec_(ec), p1_(p1), p2_(p2) {
    if (ec_)
      ec_->clear();
  }

  T report(const std::error_code &ec) const {
    if (ec_) {
      *ec_ = ec;
      return error_value<T>();
    }
    std::string what = std::string("in ") + func_name_;
    switch (bool(p1_) + bool(p2_)) {
    case 0:
      __throw_filesystem_error(what, ec);
    case 1:
      __throw_filesystem_error(what, p1_, ec);
    case 2:
      __throw_filesystem_error(what, p1_, *p2_, ec);
    }
    __builtin_unreachable();
  }

  _LIBCPP_ATTRIBUTE_FORMAT(__printf__, 3, 0)
  void report_impl(const std::error_code &ec, const char *msg, va_list) const {
    if (ec_) {
      *ec_ = ec;
      return;
    }
    std::string what = std::string("in ") + func_name_ + ": " + msg;
    switch (bool(p1_) + bool(p2_)) {
    case 0:
      __throw_filesystem_error(what, ec);
    case 1:
      __throw_filesystem_error(what, p1_, ec);
    case 2:
      __throw_filesystem_error(what, p1_, *p2_, ec);
    }
    __builtin_unreachable();
  }

  _LIBCPP_ATTRIBUTE_FORMAT(__printf__, 3, 4)
  T report(const std::error_code &ec, const char *msg, ...) const {
    va_list ap;
    va_start(ap, msg);
    try {
      report_impl(ec, msg, ap);
    } catch (...) {
      va_end(ap);
      throw;
    }
    va_end(ap);
    return error_value<T>();
  }

  T report(std::errc const &err) const { return report(std::make_error_code(err)); }

  _LIBCPP_ATTRIBUTE_FORMAT(__printf__, 3, 4)
  T report(std::errc const &err, const char *msg, ...) const {
    va_list ap;
    va_start(ap, msg);
    try {
      report_impl(make_error_code(err), msg, ap);
    } catch (...) {
      va_end(ap);
      throw;
    }
    va_end(ap);
    return error_value<T>();
  }

private:
  ErrorHandler(ErrorHandler const &) = delete;
  ErrorHandler &operator=(ErrorHandler const &) = delete;
};

inline std::error_code capture_errno() {
  return std::error_code(errno, std::generic_category());
}

using StatT = struct stat;

inline perms posix_get_perms(const StatT &st) noexcept {
  return static_cast<perms>(st.st_mode) & perms::mask;
}

inline file_status create_file_status(std::error_code &m_ec, const char *p,
                                      const StatT &path_stat, std::error_code *ec) {
  if (ec)
    *ec = m_ec;
  if (m_ec && (m_ec.value() == ENOENT || m_ec.value() == ENOTDIR)) {
    return file_status(file_type::not_found);
  } else if (m_ec) {
    ErrorHandler<void> err("posix_stat", ec, p);
    err.report(m_ec, "failed to determine attributes for the specified path");
    return file_status(file_type::none);
  }
  // else

  file_status fs_tmp;
  auto const mode = path_stat.st_mode;
  if (S_ISLNK(mode))
    fs_tmp.type(file_type::symlink);
  else if (S_ISREG(mode))
    fs_tmp.type(file_type::regular);
  else if (S_ISDIR(mode))
    fs_tmp.type(file_type::directory);
  else if (S_ISBLK(mode))
    fs_tmp.type(file_type::block);
  else if (S_ISCHR(mode))
    fs_tmp.type(file_type::character);
  else if (S_ISFIFO(mode))
    fs_tmp.type(file_type::fifo);
  else if (S_ISSOCK(mode))
    fs_tmp.type(file_type::socket);
  else
    fs_tmp.type(file_type::unknown);

  fs_tmp.permissions(posix_get_perms(path_stat));
  return fs_tmp;
}

inline file_status posix_stat(const char *p, StatT &path_stat, std::error_code *ec) {
  std::error_code m_ec;
  if (stat(p, &path_stat) == -1)
    m_ec = capture_errno();
  return create_file_status(m_ec, p, path_stat, ec);
}

uintmax_t file_size_impl(const char *p, std::error_code *ec) {
  ErrorHandler<uintmax_t> err("file_size", ec, p);

  std::error_code m_ec;
  StatT st;
  file_status fst = posix_stat(p, st, &m_ec);
  if (!exists(fst) || !is_regular_file(fst)) {
    std::errc error_kind =
        is_directory(fst) ? std::errc::is_a_directory : std::errc::not_supported;
    if (!m_ec)
      m_ec = make_error_code(error_kind);
    return err.report(m_ec);
  }
  // is_regular_file(p) == true
  return static_cast<uintmax_t>(st.st_size);
}
