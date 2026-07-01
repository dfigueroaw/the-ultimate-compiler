

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T> class Environment {
private:
  std::vector<std::unordered_map<std::string, T>> ribs;

  std::optional<std::size_t> search_rib(const std::string &var) const {
    for (std::size_t idx = ribs.size(); idx > 0; --idx) {
      const std::size_t ribIndex = idx - 1;
      if (ribs[ribIndex].count(var))
        return ribIndex;
    }
    return std::nullopt;
  }

public:
  Environment() = default;

  void add_level() { ribs.emplace_back(); }

  bool remove_level() {
    if (ribs.empty())
      return false;
    ribs.pop_back();
    return true;
  }

  void clear() { ribs.clear(); }

  void add_var(const std::string &var, const T &value) {
    if (ribs.empty()) {
      throw std::logic_error("[Environment] sin niveles al agregar '" + var +
                             "'");
    }
    ribs.back()[var] = value;
  }

  bool check(const std::string &x) const { return search_rib(x).has_value(); }

  bool check_current(const std::string &x) const {
    if (ribs.empty())
      return false;
    return ribs.back().count(x) > 0;
  }

  std::optional<T> lookup(const std::string &x) const {
    const auto idx = search_rib(x);
    if (!idx)
      return std::nullopt;
    return ribs[*idx].at(x);
  }
};

#endif
