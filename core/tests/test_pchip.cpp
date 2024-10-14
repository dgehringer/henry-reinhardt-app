#include "pchip.h"
#include "gtest/gtest.h"

namespace hr::test {
    using namespace hr::core;

    template<class T>
    using test_case_t = std::tuple<std::vector<T>, std::vector<T>, typename cubic_spline<T>::coeff_t, typename
        quadratic_spline<T>::coeff_t, typename quartic_spline<T>::coeff_t>;


    template<class T>
    static test_case_t<T> TEST_CASE_01 = std::make_tuple(
        std::vector<T>{
            0.06381357445296316, 0.0723706301900301, 0.2267888287551135
        },
        std::vector<T>{
            1.052656517085281, 1.341927098655617, 1.9299048420949816
        },
        typename cubic_spline<T>::coeff_t
        {
            {-3.17502412e+05, 5.72854397e+01},
            {2.53282585e+03, -4.23501738e+01},
            {3.53799312e+01, 8.98136458e+00},
            {1.05265652e+00, 1.34192710e+00}
        },
        typename quadratic_spline<T>::coeff_t{
            {-9.52507236e+05, 1.71856319e+02},
            {5.06565171e+03, -8.47003475e+01},
            {3.53799312e+01, 8.98136458e+00}
        },
        typename quartic_spline<T>::coeff_t{
            {-7.93756030e+04, 1.43213599e+01},
            {8.44275285e+02, -1.41167246e+01},
            {1.76899656e+01, 4.49068229e+00},
            {1.05265652e+00, 1.34192710e+00},
            {0.00000000e+00, 1.04063750e-02}
        }
    );

    template<class T>
    static test_case_t<T> TEST_CASE_02 = std::make_tuple(
        std::vector<T>{
            0.08507135747708568,
            0.10893835783384316,
            0.13974237893049465,
            0.18095867514737413,
            0.1969378847428228
        },
        std::vector<T>{
            2.0296424338270302,
            3.2104072896732068,
            3.2224357229553693,
            3.4762676126594876,
            3.526391748815065
        },
        typename cubic_spline<T>::coeff_t{
            {
                -4.78149675e+04, 7.71475075e+02, -4.50068591e+03,
                -5.54387483e+01
            },
            {
                2.43425994e+02, -3.73350858e+01, 3.17828729e+02,
                -5.10592998e+01
            },
            {
                7.08998650e+01, 8.08510117e-01, 7.04498241e-01,
                3.96687723e+00
            },
            {
                2.02964243e+00, 3.21040729e+00, 3.22243572e+00,
                3.47626761e+00
            }
        },
        typename quadratic_spline<T>::coeff_t{
            {
                -1.43444903e+05, 2.31442522e+03, -1.35020577e+04,
                -1.66316245e+02
            },
            {
                4.86851988e+02, -7.46701716e+01, 6.35657459e+02,
                -1.02118600e+02
            },
            {
                7.08998650e+01, 8.08510117e-01, 7.04498241e-01,
                3.96687723e+00
            }
        },
        typename quartic_spline<T>::coeff_t{
            {
                -1.19537419e+04, 1.92868769e+02, -1.12517148e+03,
                -1.38596871e+01
            },
            {
                8.11419980e+01, -1.24450286e+01, 1.05942910e+02,
                -1.70197666e+01
            },
            {
                3.54499325e+01, 4.04255059e-01, 3.52249121e-01,
                1.98343861e+00
            },
            {
                2.02964243e+00, 3.21040729e+00, 3.22243572e+00,
                3.47626761e+00
            },
            {
                0.00000000e+00, 6.58593342e-02, 1.64946275e-01,
                3.02532306e-01
            }
        }
    );

    template<class T>
    std::array<test_case_t<T>, 2> TEST_CASES{TEST_CASE_01<T>, TEST_CASE_02<T>};


    enum Prec {
        Single = 0,
        Double = 1
    };


    class PchipInterpolation : public ::testing::TestWithParam<std::tuple<Prec, int> > {
    public:
        template<class T>
        using input_t = std::tuple<std::vector<T>, std::vector<T> >;

        std::variant<test_case_t<float>, test_case_t<double> > inputs(Prec prec, int _case) {
            switch (prec) {
                case Single:
                    return TEST_CASES<float>[_case];
                case Double:
                    return TEST_CASES<double>[_case];
                default:
                    throw std::invalid_argument("Invalid precision");
            };
        }
    };

    template<class T, std::size_t N>
    void assert_coeffs(spline<T, N> const &spline, const auto &coeffs) {
        for (auto i = 0; i < N + 1; ++i) {
            for (auto j = 0; j < spline.length - 1; ++j)
                ASSERT_NEAR(coeffs(i, j), spline.c(i, j), std::abs(1e-3 * coeffs(i, j)));
        }
    }

    template<class T>
    void test_pchip_interpolation_construct(auto test, auto input) {
        auto inputs = test->inputs(std::get<0>(input), std::get<1>(input));
        ASSERT_TRUE(std::holds_alternative<test_case_t<T>>(inputs));
        const auto [x, y, coeffs, derivative, antiderivative] = std::get<test_case_t<T> >(inputs);
        auto spline = pchip_spline<T, EqualSize, Monotonous, SufficientLength>(x, y);
        ASSERT_EQ(spline.length, x.size());

        for (auto i = 0; i < x.size(); ++i)
            ASSERT_EQ(spline.x(i), x[i]);

        assert_coeffs<T>(spline, coeffs);
    }

    TEST_P(PchipInterpolation, construct) {
        auto p = this->GetParam();
        if (std::get<0>(p) == Single) {
            test_pchip_interpolation_construct<float>(this, p);
        } else {
            test_pchip_interpolation_construct<double>(this, p);
        }
    }


    INSTANTIATE_TEST_SUITE_P(PchipInterpolationConstruction, PchipInterpolation,
                             testing::Combine(testing::Values(Single, Double), testing::Values(0,1)));

    void has_error_code(Result<cubic_spline<double> > const &result, int error_code) {
        ASSERT_TRUE(std::holds_alternative<interpolation_error>(result));
        ASSERT_EQ(std::get<interpolation_error>(result).code, error_code);
    }

    TEST(PchipInterpolationErrorCodes, invalid_inputs) {
        has_error_code(pchip_spline<double>({}, {}), 2);
        has_error_code(pchip_spline<double>({1.0}, {1.0}), 2);
        has_error_code(pchip_spline<double>({1.0}, {}), 1);
        has_error_code(pchip_spline<double>({}, {1.0}), 1);
        has_error_code(pchip_spline<double>({1.0, 1.0, 2.0}, {1.0, 2.0, 3.0}), 3);
    }

    TEST_P(PchipInterpolation, derivative) {
        auto [_, _case] = this->GetParam();
        auto [xraw, y, coeffs, derivative, antiderivative] = std::get<test_case_t<
            double> >(this->inputs(Double, _case));
        auto spline = hr::core::cubic_spline<double>(stl_to_eigen(std::move(xraw)), coeffs);
        ASSERT_EQ(spline.order -1, spline.derivative().order);
        assert_coeffs(spline.derivative(), derivative);
    }

    INSTANTIATE_TEST_SUITE_P(PchipInterpolationDerivative, PchipInterpolation,
                             testing::Combine(testing::Values(Double), testing::Values(0,1)));

    template<class T>
    void test_evaluate() {
        std::vector<T> x{1.0, 2.0, 3.0, 5.0, 6.0, 6.8};
        std::vector<T> y{0.3, 1.0, 3.0, 4.8, 7.0, 7.1};

        auto spline = pchip_spline<T, EqualSize, Monotonous, SufficientLength>(x, y);


        std::vector<T> xeval{
            1., 1.41428571, 1.82857143, 2.24285714, 2.65714286,
            3.07142857, 3.48571429, 3.9, 4.31428571, 4.72857143,
            5.14285714, 5.55714286, 5.97142857, 6.38571429, 6.8
        };
        std::vector<T> yeval{
            0.3, 0.46373696, 0.82450638, 1.38308687, 2.34413198,
            3.08950511, 3.50812251, 3.8198345, 4.11379063, 4.47914049,
            5.05848585, 6.20267498, 6.98960332, 7.07100914, 7.1
        };

        auto ybreak = spline.template operator()<InBounds, Monotonous>(x);
        constexpr auto tolerance = 1e-6;
        for (auto i = 0; i < x.size(); ++i) {
            ASSERT_NEAR(spline. template operator()<InBounds>(x[i]), y[i], tolerance);
            ASSERT_NEAR(ybreak[i], y[i], tolerance);
        }

        auto ycomputed = spline.template operator()<InBounds, Monotonous>(xeval);

        for (auto i = 0; i < xeval.size(); ++i) {
            ASSERT_NEAR(spline.template operator()<InBounds>(xeval[i]), yeval[i], 1.0e-6);
            ASSERT_NEAR(ycomputed[i], yeval[i], 1.0e-6);
        }
    }


    TEST(PchipInterpolation, evaluate) {
        test_evaluate<float>();
        test_evaluate<double>();
    }

    TEST_P(PchipInterpolation, antiderivative) {
        auto [_, _case] = this->GetParam();
        auto [xraw, y, coeffs, derivative, antiderivative] = std::get<test_case_t<
            double> >(this->inputs(Double, _case));
        auto spline = cubic_spline<double>(stl_to_eigen(std::move(xraw)), coeffs);
        ASSERT_EQ(spline.order+1, spline.antiderivative().order);
        assert_coeffs(spline.antiderivative(), antiderivative);
    }

    INSTANTIATE_TEST_SUITE_P(PchipInterpolationAntiderivative, PchipInterpolation,
                             testing::Combine(testing::Values(Double), testing::Values(0,1)));
}
