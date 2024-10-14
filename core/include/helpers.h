//
// Created by Dominik Gehringer on 06.10.24.
//

#ifndef HELPERS_H
#define HELPERS_H
#include <string>
#include <Eigen/Core>

namespace hr::core {
    enum Guarantee: std::uint8_t {
        EqualSize,
        Sorted,
        SufficientLength,
        Monotonous,
        InBounds,
    };

    struct interpolation_error {
        int code;
        std::string message;
    };

    template<class... T>
    using Result = std::variant<interpolation_error, T...>;


    template<std::size_t By>
    struct element {
        bool operator()(auto &&a, auto &&b) {
            return std::get<By>(a) < std::get<By>(b);
        };
    };

    template<class T>
    T sgn(T val) {
        return static_cast<T>((T(0) < val) - (val < T(0)));
    }

    template<class T>
    bool is_close(T a, T b, T tol = 1e-9) {
        return a == b || std::abs(a - b) <= tol;
    }

    template<class T, template<class...> class Container>
    auto stl_to_eigen(Container<T> &&);

    template<Guarantee ... Gauarantees>
    constexpr bool is_guaranteed(Guarantee &&g) {
        return ((Gauarantees == g) || ...);
    }

    template<Guarantee ... Gauarantees, std::convertible_to<Guarantee> ... Check>
    constexpr bool all_guaranteed(Check &&... g) {
        return (is_guaranteed<Gauarantees...>(std::forward<Guarantee>(g)) && ...);
    }

    template<>
    inline auto stl_to_eigen(std::vector<float> &&vector) {
        return Eigen::Map<Eigen::VectorXf>(vector.data(), static_cast<Eigen::Index>(vector.size()));
    }

    template<>
    inline auto stl_to_eigen(std::vector<double> &&vector) {
        return Eigen::Map<Eigen::VectorXd>(vector.data(), static_cast<Eigen::Index>(vector.size()));
    }

    template<class T, std::size_t Power>
    constexpr T power(T x) {
        if constexpr (Power == 0) {
            return T{1};
        } else if constexpr (Power == 1) {
            return x;
        } else {
            return x * power<T, Power - 1>(x);
        }
    }

}

#endif //HELPERS_H
