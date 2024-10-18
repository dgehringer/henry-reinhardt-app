//
// Created by Dominik Gehringer on 06.10.24.
//

#ifndef HELPERS_H
#define HELPERS_H
#include <Eigen/Core>
#include <string>

namespace hr::core {
  template <class T> using point = std::pair<T, T>;

  template <class T> using point_list = std::vector<point<T> >;

  template <class T> using step_function = std::pair<T, point_list<T> >;

  enum Guarantee : std::uint8_t {
    Sorted = 0,
    SufficientLength = 1,
    Monotonous = 2,
    InBounds = 3,
  };

  enum DOF : std::uint8_t { Horizontal = 0, Vertical = 1 };

  enum Axis : std::size_t { Grade = 0, Yield = 1 };

  template <Axis Axis, class T> std::pair<T, T> min_and_max(point_list<T> const &points) {
    auto [minel, maxel] = std::minmax_element(points.begin(), points.end());
    return std::make_pair(std::get<Axis>(*minel), std::get<Axis>(*maxel));
  }

  template <class T> using point_bound_list = std::vector<std::tuple<point<T>, point<T>, DOF> >;

  struct interpolation_error {
    std::uint8_t code;
    std::string message;
  };

  template <class... T> using Result = std::variant<interpolation_error, T...>;

  template <class T> T result_value(Result<T> const &result) {
    assert(std::holds_alternative<T>(result));
    return std::get<T>(result);
  }

  template <class T> bool result_ok(Result<T> const &result) {
    return !std::holds_alternative<interpolation_error>(result);
  }

  template <class T> interpolation_error result_error(Result<T> const &result) {
    assert(!result_ok(result));
    return std::get<interpolation_error>(result);
  }

  template <std::size_t By> struct element {
    bool operator()(auto &&a, auto &&b) { return std::get<By>(a) < std::get<By>(b); };
  };

  template <class T> T sgn(T val) { return static_cast<T>((T(0) < val) - (val < T(0))); }

  template <class T> bool is_close(T a, T b, T tol = 1e-9) {
    return a == b || std::abs(a - b) <= tol;
  }

  template <class T, template <class...> class Container> auto stl_to_eigen(Container<T> &&);

  template <Guarantee... Gauarantees> constexpr bool is_guaranteed(Guarantee &&g) {
    return ((Gauarantees == g) || ...);
  }

  template <Guarantee... Gauarantees, std::convertible_to<Guarantee>... Check>
  constexpr bool all_guaranteed(Check &&...g) {
    return (is_guaranteed<Gauarantees...>(std::forward<Guarantee>(g)) && ...);
  }

  template <> inline auto stl_to_eigen(std::vector<float> &&vector) {
    return Eigen::Map<Eigen::VectorXf>(vector.data(), static_cast<Eigen::Index>(vector.size()));
  }

  template <> inline auto stl_to_eigen(std::vector<double> &&vector) {
    return Eigen::Map<Eigen::VectorXd>(vector.data(), static_cast<Eigen::Index>(vector.size()));
  }

  template <class T, std::size_t Power> constexpr T power(T x) {
    if constexpr (Power == 0) {
      return T{1};
    } else if constexpr (Power == 1) {
      return x;
    } else {
      return x * power<T, Power - 1>(x);
    }
  }

  template <class T>
  std::pair<std::vector<T>, std::vector<T> > transpose(point_list<T> const &points) {
    std::vector<T> grade(points.size()), yield(points.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
      grade[i] = points[i].first;
      yield[i] = points[i].second;
    }
    return std::make_pair(grade, yield);
  }

  template <class T> bool in_unit_interval(T value) { return value >= T(0) && value <= T(1); }

  template <class T> std::vector<T> linspace(T start, T end, std::size_t num) {
    if (num == 0) return {};
    std::vector<T> result(num);
    T delta{(end - start) / (num-1)};
    for (std::size_t i = 0; i < num; ++i) {
      result[i] = start + delta * i;
    }
    result.push_back(end);
    return result;
  }

}  // namespace hr::core

#endif  // HELPERS_H
