#include "pchip.h"
#include "gtest/gtest.h"

namespace hr::test {
    using namespace hr::core;

    template<class T>
    using test_case_t = std::tuple<std::vector<T>, std::vector<T>, typename cubic_spline<T>::coeff_t, typename
        quadratic_spline<T>::coeff_t, typename quartic_spline<T>::coeff_t>;

    template<class T>
    using test_case_eval_t = std::tuple<std::vector<T>, std::vector<T>, std::vector<T>, std::vector<T> >;

    template<class T>
    point_list<T> zip_points(std::vector<T> const &x, std::vector<T> const &y) {
        assert(x.size() == y.size());
        point_list<T> result(x.size());
        for (size_t i = 0; i < x.size(); ++i) result[i] = std::make_pair(x[i], y[i]);
        return result;
    }

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

    template<class T>
    static test_case_eval_t<T> TEST_CASE_EVAL = std::make_tuple(
        std::vector<T>{1.0, 2.0, 3.0, 5.0, 6.0, 6.8},
        std::vector<T>{0.3, 1.0, 3.0, 4.8, 7.0, 7.1},
        std::vector<T>{
            1., 1.41428571, 1.82857143, 2.24285714, 2.65714286,
            3.07142857, 3.48571429, 3.9, 4.31428571, 4.72857143,
            5.14285714, 5.55714286, 5.97142857, 6.38571429, 6.8
        },
        std::vector<T>{
            0.3, 0.46373696, 0.82450638, 1.38308687, 2.34413198,
            3.08950511, 3.50812251, 3.8198345, 4.11379063, 4.47914049,
            5.05848585, 6.20267498, 6.98960332, 7.07100914, 7.1
        }
    );

    enum Prec {
        Single = 0,
        Double = 1
    };


    class PchipInterpolationFixture : public ::testing::TestWithParam<std::tuple<Prec, int> > {
    public:
        template<class T>
        using input_t = std::tuple<std::vector<T>, std::vector<T> >;


        std::variant<test_case_t<float>, test_case_t<double> > inputs(Prec prec, int _case) {
            if (prec == Single) {
                return TEST_CASES<float>[_case];
            }
            if (prec == Double) {
                return TEST_CASES<double>[_case];
            }
            throw std::invalid_argument("Prec not supported");
        }

        std::variant<test_case_eval_t<float>, test_case_eval_t<double> > eval_inputs(Prec prec) {
            if (prec == Single) {
                return TEST_CASE_EVAL<float>;
            }
            if (prec == Double) {
                return TEST_CASE_EVAL<double>;
            }
            throw std::invalid_argument("Prec not supported");
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
    void test_pchip_interpolation_construct(auto const &test) {
        auto inputs = test->inputs(std::get<0>(test->GetParam()), std::get<1>(test->GetParam()));
        ASSERT_TRUE(std::holds_alternative<test_case_t<T>>(inputs));
        const auto [x, y, coeffs, derivative, antiderivative] = std::get<test_case_t<T> >(inputs);
        auto spline = pchip_spline<T, Monotonous, SufficientLength>(zip_points(x, y));
        ASSERT_EQ(spline.length, x.size());

        for (auto i = 0; i < x.size(); ++i)
            ASSERT_EQ(spline.x(i), x[i]);

        assert_coeffs<T>(spline, coeffs);
    }

    TEST_P(PchipInterpolationFixture, construct) {
        auto p = this->GetParam();
        if (std::get<0>(p) == Single) {
            test_pchip_interpolation_construct<float>(this);
        } else {
            test_pchip_interpolation_construct<double>(this);
        }
    }


    template<class T>
    void has_error_code(Result<T> const &result, int error_code) {
        ASSERT_TRUE(std::holds_alternative<interpolation_error>(result));
        ASSERT_EQ(std::get<interpolation_error>(result).code, error_code);
    }

    TEST(PchipInterpolationErrorCodes, invalid_inputs) {
        has_error_code(pchip_spline<double>({}), SufficientLength);
        has_error_code(pchip_spline<double>({{1.0, 0.0}}), SufficientLength);
        has_error_code(pchip_spline<double>({{1.0, 0.0}, {2.0, 1.0}}), SufficientLength);
        has_error_code(pchip_spline<double>({{1.0, 1.0}, {1.0, 2.0}, {2.0, 3.0}}), Monotonous);
    }

    template<class T>
    void test_pchip_interpolation_derivative(auto const &test) {
        auto [prec, _case] = test->GetParam();
        auto [xraw, y, coeffs, derivative, _] = std::get<test_case_t<T> >(test->inputs(prec, _case));
        auto spline = hr::core::cubic_spline<T>(stl_to_eigen(std::move(xraw)), coeffs);
        ASSERT_EQ(spline.order -1, spline.derivative().order);
        assert_coeffs(spline.derivative(), derivative);
    }

    TEST_P(PchipInterpolationFixture, derivative) {
        if (std::get<0>(this->GetParam()) == Single) {
            test_pchip_interpolation_derivative<float>(this);
        } else {
            test_pchip_interpolation_derivative<double>(this);
        }
    }

    template<class T>
    void test_pchip_interpolation_antiderivative(auto const &test) {
        auto [prec, _case] = test->GetParam();
        auto [xraw, y, coeffs, _, antiderivative] = std::get<test_case_t<T> >(test->inputs(prec, _case));
        auto spline = hr::core::cubic_spline<T>(stl_to_eigen(std::move(xraw)), coeffs);
        ASSERT_EQ(spline.order +1, spline.antiderivative().order);
        assert_coeffs(spline.antiderivative(), antiderivative);
    }

    TEST_P(PchipInterpolationFixture, antiderivative) {
        if (std::get<0>(this->GetParam()) == Single) {
            test_pchip_interpolation_antiderivative<float>(this);
        } else {
            test_pchip_interpolation_antiderivative<double>(this);
        }
    }


    template<class T>
    void test_pchip_interpolation_evaluate(auto const &test, Prec prec) {
        auto [x, y, xeval, yeval] = std::get<test_case_eval_t<T> >(test->eval_inputs(prec));
        auto spline = pchip_spline<T, Monotonous, SufficientLength>(zip_points(x, y));

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


    TEST_P(PchipInterpolationFixture, evaluate) {
        auto [prec, _case] = this->GetParam();
        if (_case)
            GTEST_SKIP();
        if (prec == Single) {
            test_pchip_interpolation_evaluate<float>(this, prec);
        } else {
            test_pchip_interpolation_evaluate<double>(this, prec);
        }
    }

    template<class T>
    void test_pchip_interpolation_integrate(auto const &test, Prec prec) {
        // conbinations 2 = int_xeval[0]^xeval[1]
        std::vector<std::tuple<std::size_t, std::size_t, T> > test_intervals = {
            {0, 0, 0.0},
            {0, 2, 0.41144843912296114},
            {0, 4, 1.6198621518037255},
            {0, 6, 4.135566167644992},
            {0, 8, 7.298119862154731},
            {0, 10, 11.034228821466355},
            {0, 12, 16.124252080885434},
            {0, 14, 21.976334561345535},
            {2, 2, 0.0},
            {2, 4, 1.2084137126807644},
            {2, 6, 3.724117728522031},
            {2, 8, 6.88667142303177},
            {2, 10, 10.622780382343393},
            {2, 12, 15.712803641762473},
            {2, 14, 21.564886122222575},
            {4, 4, 0.0},
            {4, 6, 2.5157040158412665},
            {4, 8, 5.678257710351006},
            {4, 10, 9.414366669662629},
            {4, 12, 14.504389929081707},
            {4, 14, 20.356472409541812},
            {6, 6, 0.0},
            {6, 8, 3.1625536945097394},
            {6, 10, 6.898662653821361},
            {6, 12, 11.988685913240442},
            {6, 14, 17.840768393700543},
            {8, 8, 0.0},
            {8, 10, 3.7361089593116223},
            {8, 12, 8.826132218730702},
            {8, 14, 14.678214699190804},
            {10, 10, 0.0},
            {10, 12, 5.09002325941908},
            {10, 14, 10.942105739879183},
            {12, 12, 0.0},
            {12, 14, 5.852082480460103},
            {14, 14, 0.0}
        };
        auto [x, y, xv, _] = std::get<test_case_eval_t<T> >(test->eval_inputs(prec));
        auto spline = pchip_spline<T, Monotonous, SufficientLength>(
            zip_points(x, y));

        for (const auto [li, ui, area]: test_intervals) {
            ASSERT_NEAR(spline.template integrate<InBounds>(xv[li], xv[ui]), area, 1.0e-4);
            ASSERT_NEAR(spline.template integrate<InBounds>(xv[ui], xv[li]), -area, 1.0e-4);
        }

        // test intervals on break points of the polynomial itself
        std::vector<std::tuple<std::size_t, std::size_t, T> > test_intervals_break = {
            {0, 1, 0.5677469135802469},
            {0, 2, 2.546166666666667},
            {0, 3, 10.331550125313285},
            {0, 4, 16.324121762579153},
            {0, 5, 21.976334561345535},
            {1, 2, 1.97841975308642},
            {1, 3, 9.763803211733038},
            {1, 4, 15.756374848998908},
            {1, 5, 21.40858764776529},
            {2, 3, 7.785383458646617},
            {2, 4, 13.777955095912487},
            {2, 5, 19.43016789467887},
            {3, 4, 5.99257163726587},
            {3, 5, 11.644784436032253},
            {4, 5, 5.652212798766382}
        };

        for (const auto [li, ui, area]: test_intervals_break) {
            ASSERT_NEAR(spline.template integrate<InBounds>(x[li], x[ui]), area, 1.0e-4);
            ASSERT_NEAR(spline.template integrate<InBounds>(x[ui], x[li]), -area, 1.0e-4);
        }
    }

    TEST_P(PchipInterpolationFixture, integrate) {
        auto [prec, _case] = this->GetParam();
        if (_case)
            GTEST_SKIP();
        if (prec == Single) {
            test_pchip_interpolation_integrate<float>(this, prec);
        } else {
            test_pchip_interpolation_integrate<double>(this, prec);
        }
    }


    INSTANTIATE_TEST_SUITE_P(PchipInterpolation, PchipInterpolationFixture,
                             testing::Combine(testing::Values(Single, Double), testing::Values(0,1)),
                             [](testing::TestParamInfo<PchipInterpolationFixture::ParamType> const& info) {
                             return fmt::format("{}_case_{}", std::get<0>(info.param) == Single ? "float" :
                                 "double", std::get<1>(info.param));
                             });
}
