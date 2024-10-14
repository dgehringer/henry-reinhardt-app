//
// Created by Dominik Gehringer on 06.10.24.
//

#ifndef SPLINE_H
#define SPLINE_H

#include <iostream>
#include <Eigen/Eigen>

#include "helpers.h"
#include "fmt/core.h"

namespace hr::core {
    template<class, std::size_t>
    struct spline_base {
    };

    template<std::size_t N>
    struct spline_base<double, N> {
        using vec_t = Eigen::VectorXd;
        using coeff_t = Eigen::Matrix<double, N + 1, Eigen::Dynamic>;
    };

    template<std::size_t N>
    struct spline_base<float, N> {
        using vec_t = Eigen::VectorXf;
        using coeff_t = Eigen::Matrix<float, N + 1, Eigen::Dynamic>;
    };

    template<class T, std::size_t N>
    T evaluate(auto const &coeffs, T x) {
        return [_x = x]<std::size_t... Index>(auto &&_coeffs, std::integer_sequence<std::size_t, Index...>) -> T {
            return ((_coeffs(Index) * power<T, N - Index>(_x)) + ...);
        }(std::forward<decltype(coeffs)>(coeffs), std::make_index_sequence<N + 1>{});
    }


    template<class T, std::size_t N>
    struct spline : spline_base<T, N> {
        using vec_t = typename spline_base<T, N>::vec_t;
        using coeff_t = typename spline_base<T, N>::coeff_t;
        static constexpr std::size_t order = N;
        const vec_t x;
        const coeff_t c;
        const std::size_t length;

        spline(vec_t x, coeff_t c) : x(x), c(c), length(x.size()) {
        }

        spline(vec_t &x, coeff_t &&c) : x(x), c(std::move(c)), length(x.size()) {
        }


        template<Guarantee ... Guarantees>
        std::conditional_t<is_guaranteed<Guarantees...>(InBounds), T, Result<T> > operator()(T val) {
            if constexpr (!is_guaranteed<Guarantees...>(InBounds)) {
                if (val < x.front() || val > x.back())
                    return interpolation_error{
                        4, fmt::format("out of range {} <= {} <= {}", x.front(), val, x.back())
                    };
            };
            auto interval{-1};
            for (auto i = 0; i < length - 1; ++i) {
                if (val >= x[i] && val <= x[i + 1]) {
                    interval = i;
                    break;
                }
            }
            return evaluate<T, N>(c.col(interval), val - x[interval]);
        }

        template<Guarantee ... Guarantees>
        std::conditional_t<all_guaranteed<Guarantees...>(InBounds, Monotonous), std::vector<T>, Result<std::vector<
            T> > > operator(
        )(std::vector<T> const &values) {
            if constexpr (!is_guaranteed<Guarantees...>(InBounds)) {
                for (auto v: values) {
                    if (v < x.front() || v > x.back())
                        return interpolation_error{
                            4, fmt::format("out of range {} <= {} <= {}", x.front(), v, x.back())
                        };
                }
            }
            if (values.empty()) return std::vector<T>{};
            if (values.size() == 1) {
                return std::vector<T>{operator()<Guarantees...>(values[0])};
            }
            if constexpr (!is_guaranteed<Guarantees...>(Monotonous)) {
                for (auto i = 0; i < values.size() - 1; ++i) {
                    if (values[i] > values[i + 1]) return interpolation_error{
                        5, fmt::format("input array is not monotonous at indices {} and {}", i, i + 1)
                    };
                }
            }

            std::vector<T> result;
            result.reserve(values.size());
            auto interval {0};
            for (auto i = 0; i < values.size(); i++) {
                while (!(values[i] >= x(interval) && values[i] <= x(interval + 1))) interval++;
                result.push_back(evaluate<T, N>(c.col(interval), values[i] - x(interval)));
            }
            return result;
        }

        spline<T, N - 1> derivative() {
            if constexpr (N == 0) with_coeffs<0>(typename spline<T, 0>::coeff_t::Zero(length - 1));
            typename spline<T, N - 1>::coeff_t c(N, length - 1);
            for (auto factor = order; factor > 0; --factor) {
                for (auto i = 0; i < length - 1; i++)
                    c(order - factor, i) = factor * this->c(order - factor, i);
            }
            return with_coeffs<N - 1>(std::move(c));
        }


        spline<T, N + 1> antiderivative() {
            typename spline<T, N + 1>::coeff_t c(N + 2, length - 1);
            for (auto factor = order + 1; factor > 0; --factor) {
                for (auto i = 0; i < length - 1; ++i)
                    c(order - factor + 1, i) = this->c(order - factor + 1, i) / factor;
            }
            for (auto i = 1; i < length - 1; ++i) {
                c(N + 1, i) = evaluate<T, N + 1>(c.col(i - 1), x(i) - x(i - 1));
            }
            return with_coeffs<N + 1>(std::move(c));;
        }

    private
    :
        template
        <
            std::size_t M
        >
        spline<T, M> with_coeffs(typename spline<T, M>::coeff_t &&new_coeffs) {
            return spline<T, M>(x, std::forward<typename spline<T, M>::coeff_t>(new_coeffs));
        }
    };

    template<class T>
    using cubic_spline = spline<T, 3>;

    template<class T>
    using quadratic_spline = spline<T, 2>;

    template<class T>
    using quartic_spline = spline<T, 4>;
}
#endif //SPLINE_H
