//
// Created by Dominik Gehringer on 06.10.24.
//

#ifndef SPLINE_H
#define SPLINE_H

#include <Eigen/Core>

#include "fmt/core.h"
#include "helpers.h"

namespace hr::core {
  template <class, std::size_t> struct spline_base {};

  template <std::size_t N> struct spline_base<double, N> {
    using vec_t = Eigen::VectorXd;
    using coeff_t = Eigen::Matrix<double, N + 1, Eigen::Dynamic>;
  };

  template <std::size_t N> struct spline_base<float, N> {
    using vec_t = Eigen::VectorXf;
    using coeff_t = Eigen::Matrix<float, N + 1, Eigen::Dynamic>;
  };

  template <class T, std::size_t N> T evaluate(auto const &coeffs, T x) {
    return [_x = x]<std::size_t... Index>(auto &&_coeffs,
                                          std::integer_sequence<std::size_t, Index...>) -> T {
      return ((_coeffs(Index) * power<T, N - Index>(_x)) + ...);
    }(std::forward<decltype(coeffs)>(coeffs), std::make_index_sequence<N + 1>{});
  }

  template <class T, std::size_t N> struct spline : spline_base<T, N> {
    using vec_t = typename spline_base<T, N>::vec_t;
    using coeff_t = typename spline_base<T, N>::coeff_t;
    static constexpr std::size_t order = N;
    vec_t x;
    coeff_t c;
    std::size_t length;

    spline(vec_t x, coeff_t c) : x(x), c(c), length(x.size()) {}

    spline(vec_t &x, coeff_t &&c) : x(x), c(std::move(c)), length(x.size()) {}

    T operator()(T val, int interval) const {
      return evaluate<T, N>(c.col(interval), val - x(interval));
    }

    template <Guarantee... Guarantees>
    std::conditional_t<is_guaranteed<Guarantees...>(InBounds), T, Result<T> > operator()(
        T val) const {
      if constexpr (!is_guaranteed<Guarantees...>(InBounds)) {
        if (!in_bounds(val))
          return interpolation_error{
              InBounds, fmt::format("out of range {} <= {} <= {}", x(0), val, x(length - 1))};
      };
      for (auto interval = 0; interval < length - 1; ++interval) {
        if (in_interval(val, interval)) {
          return operator()(val, interval);
        }
      }
      throw std::out_of_range("out of range");
    }

    template <Guarantee... Guarantees>
    std::conditional_t<all_guaranteed<Guarantees...>(InBounds, Monotonous), std::vector<T>,
                       Result<std::vector<T> > >
    operator()(std::vector<T> const &values) const {
      if constexpr (!is_guaranteed<Guarantees...>(InBounds)) {
        for (auto v : values) {
          if (!in_bounds(v))
            return interpolation_error{
                InBounds, fmt::format("out of range {} <= {} <= {}", x(0), v, x(length - 1))};
        }
      }
      if (values.empty()) return std::vector<T>{};
      if constexpr (!is_guaranteed<Guarantees...>(Monotonous)) {
        for (auto i = 0; i < values.size() - 1; ++i) {
          if (values[i] > values[i + 1])
            return interpolation_error{
                Monotonous,
                fmt::format("input array is not monotonous at indices {} and {}", i, i + 1)};
        }
      }

      std::vector<T> result;
      result.reserve(values.size());
      auto interval{0};
      for (auto i = 0; i < values.size(); i++) {
        while (!(values[i] >= x(interval) && values[i] <= x(interval + 1))) interval++;
        result.push_back(evaluate<T, N>(c.col(interval), values[i] - x(interval)));
      }
      return result;
    }

    spline<T, N - 1> derivative() const {
      if constexpr (N == 0) with_coeffs<0>(typename spline<T, 0>::coeff_t::Zero(length - 1));
      typename spline<T, N - 1>::coeff_t c(N, length - 1);
      for (auto factor = order; factor > 0; --factor) {
        for (auto i = 0; i < length - 1; i++)
          c(order - factor, i) = factor * this->c(order - factor, i);
      }
      return with_coeffs<N - 1>(std::move(c));
    }

    spline<T, N + 1> antiderivative() const {
      typename spline<T, N + 1>::coeff_t c(N + 2, length - 1);
      for (auto factor = order + 1; factor > 0; --factor) {
        for (auto i = 0; i < length - 1; ++i)
          c(order - factor + 1, i) = this->c(order - factor + 1, i) / factor;
      }
      for (auto i = 1; i < length - 1; ++i) {
        c(N + 1, i) = evaluate<T, N + 1>(c.col(i - 1), x(i) - x(i - 1));
      }
      return with_coeffs<N + 1>(std::move(c));
    }

    template <Guarantee... Guarantees>
    std::conditional_t<is_guaranteed<Guarantees...>(InBounds), T, Result<T> > integrate(
        T a, T b, std::optional<spline<T, N + 1> > cache = std::nullopt) const {
      if constexpr (!is_guaranteed<Guarantees...>(InBounds)) {
        if (!in_bounds(a))
          return interpolation_error{
              InBounds,
              fmt::format("lower bound out of range {} <= {} <= {}", x(0), a, x(length - 1))};
        if (!in_bounds(b))
          return interpolation_error{
              InBounds,
              fmt::format("upper bound out of range {} <= {} <= {}", x(0), b, x(length - 1))};
      }

      const bool negate = b < a;
      const T lower{negate ? b : a}, upper{negate ? a : b};

      int i_l{0};
      for (; i_l < length - 1; ++i_l) {
        if (in_interval(lower, i_l)) break;
      }
      int i_u{i_l};
      for (; i_u < length - 1; ++i_u) {
        if (in_interval(upper, i_u)) break;
      }

      if (i_l == length - 1 || i_u == length - 1) throw std::out_of_range("out of range");

      const auto F_ = cache.value_or(this->antiderivative());
      if (i_l == i_u) {
        T area{F_(upper, i_l) - F_(lower, i_l)};
        return negate ? -area : area;
      }

      T area{F_(x(i_l + 1), i_l) - F_(lower, i_l) + F_(upper, i_u) - F_(x(i_u), i_u)};
      for (int i = i_l + 1; i < i_u; ++i) {
        area += F_(x(i + 1), i) - F_(x(i), i);
      }

      return negate ? -area : area;
    }

    private:
        template<std::size_t M>
        spline<T, M> with_coeffs(typename spline<T, M>::coeff_t &&new_coeffs) const {
            return spline<T, M>(x, std::forward<typename spline<T, M>::coeff_t>(new_coeffs));
        }

    bool in_bounds(T val) const { return val >= x(0) && val <= x(length - 1); }

    bool in_interval(T val, int interval) const {
      return val >= x(interval) && val <= x(interval + 1);
    }
  };

  template <class T> using cubic_spline = spline<T, 3>;

  template <class T> using quadratic_spline = spline<T, 2>;

  template <class T> using quartic_spline = spline<T, 4>;
}  // namespace hr::core
#endif  // SPLINE_H
