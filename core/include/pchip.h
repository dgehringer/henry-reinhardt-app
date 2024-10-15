//
// Created by Dominik Gehringer on 06.10.24.
//

#ifndef PCHIP_H
#define PCHIP_H
#include <vector>
#include "helpers.h"
#include "hr.h"
#include "spline.h"
#include "fmt/core.h"


namespace hr::core {
  template<class T>
  using result_t = Result<spline<T, 3> >;

  template<class T>
  T pchip_edge_case(T h0, T h1, T m0, T m1) {
    auto d{((2 * h0 + h1) * m0 - h0 * m1) / (h0 + h1)};
    auto set_to_zero = sgn(d) != sgn(m0);
    if (set_to_zero) {
      d = 0.0;
    }
    if (!set_to_zero && (sgn(m0) != sgn(m1) && std::abs(d) > 3. * std::abs(m0))) {
      d = 3.0 * m0;
    }
    return d;
  }

  template<class T, Guarantee ... Guarantees>
  std::conditional_t<all_guaranteed<Guarantees...>(SufficientLength, Monotonous), spline<T, 3>, result_t<
    T> > pchip_spline(point_list<T> &&points, T eps = 1e-9) {
    auto l = points.size();
    if constexpr (!is_guaranteed<Guarantees...>(SufficientLength)) {
      if (l < 3) {
        return result_t<T>{
          interpolation_error{SufficientLength, "sizes of input arrays must be at least three"}
        };
      }
    }

    point_list<T> values(points);
    if constexpr (!is_guaranteed<Guarantees...>(Sorted)) {
      std::sort(values.begin(), values.end(), element<0>{});
    }
    std::vector<T> hk(l - 1), mk(l - 1), smk(l - 1);
    for (auto i = 0; i < l - 1; i++) {
      auto [x1, f1] = values[i];
      auto [x2, f2] = values[i + 1];
      if constexpr (!is_guaranteed<Guarantees...>(Monotonous)) {
        if (is_close<T>(x1, x2, eps)) {
          return result_t<T>{
            interpolation_error{
              Monotonous, fmt::format("x values {} and {} are equal or too close", x1, x2)
            }
          };
        }
      }
      auto _hi{x2 - x1};
      auto _mk{(f2 - f1) / _hi};
      hk[i] = _hi;
      mk[i] = _mk;
      smk[i] = sgn(_mk);
    }

    std::vector<T> dk(l, 0.0);
    for (auto i = 0; i < l - 2; i++) {
      if (smk[i + 1] != smk[i] || is_close<T>(mk[i + 1], 0.0) || is_close<T>(mk[i], 0.0)) {
        dk[i + 1] = 0.0;
      } else {
        auto w1{2.0 * hk[i + 1] + hk[i]}, w2{hk[i + 1] + 2.0 * hk[i]};
        dk[i + 1] = (w1 + w2) / (w1 / mk[i] + w2 / mk[i + 1]);
      }
    }
    *dk.begin() = pchip_edge_case(hk[0], hk[1], mk[0], mk[1]);
    *(dk.end() - 1) = pchip_edge_case(hk[l - 2], hk[l - 3], mk[l - 2], mk[l - 3]);

    using vec_t = typename cubic_spline<T>::vec_t;
    using coeff_t = typename cubic_spline<T>::coeff_t;
    coeff_t c(4, l - 1);


    for (auto i = 0; i < l - 1; i++) {
      auto t{(dk[i] + dk[i + 1] - 2.0 * mk[i]) / hk[i]};
      c(0, i) = t / hk[i];
      c(1, i) = (mk[i] - dk[i]) / hk[i] - t;
      c(2, i) = dk[i];
      c(3, i) = std::get<1>(values[i]);
    }

    vec_t xsorted(l);
    for (auto i = 0; i < l; i++) xsorted(i) = std::get<0>(values[i]);
    return spline<T, 3>(xsorted, c);
  }
}

#endif //PCHIP_H
