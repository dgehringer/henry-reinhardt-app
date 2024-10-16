
#include <iostream>
#include <ranges>
#include <vector>
#include <LBFGSB.h>
#include "hr.h"
#include "optimize.h"


namespace views = std::ranges;


using namespace hr;
using namespace hr::core;

template<class T>
std::string to_list(std::vector<T> const &values) {
    if (values.empty()) return "[]";
    std::stringstream ss;
    ss << "[";
    for (auto i = 0; i < values.size() - 1; ++i) ss << fmt::format("{}, ", values[i]);
    ss << fmt::format("{}]", values.back());
    return ss.str();
}

template<class T>
std::vector<T> linspace(T start, T end, std::size_t num) {
    if (num == 0) return {};
    std::vector<T> result(num);
    T delta {(end - start) / num};
    for (std::size_t i = 0; i < num; ++i) {
        result[i] = start + delta * i;
    }
    return result;
}

template<class T>
point_list<T> step_function_points(step_function<T> const &step_function) {
    auto error = validate_step_function<T>(step_function);
    assert(!error.has_value());
    const auto [first_grade, points] = step_function;
    T zero{0.0};
    point_list<T> result{{zero, zero}, {first_grade, zero}, {first_grade, std::get<Yield>(points.front())}};
    for (auto i = 0; i < points.size() - 1; ++i) {
        auto [g1, y1] = points[i];
        auto [g2, y2] = points[i + 1];
        result.push_back({g1, y1});
        result.push_back({g1, y2});
    }
    result.push_back(points.back());
    result.push_back({std::get<Grade>(points.back()), T{1}});
    result.push_back({T{1}, T{1}});
    return result;
}



template<class T>
void plot_hr(step_function<T> const &step_function, T starting_grade, T final_grade) {
    step_function_points(step_function);
    const auto [splineg, spliney] = transpose(result_value(compute_break_points<T>(step_function, starting_grade, final_grade)));
    const auto [stepg, stepy] = transpose(step_function_points(step_function));

    auto intergrowth = result_value(Intergrowth<T>::from_step_function(step_function, starting_grade, final_grade));

    intergrowth.optimize();
    const auto interg = linspace<double>(starting_grade, final_grade, 100);
    const auto intery = result_value(intergrowth.initial(interg));

    std::cout << fmt::format(
        "from matplotlib import pyplot as plt\nplt.plot({}, {})\nplt.scatter({},{}, color=\"r\")\nplt.plot({},{})\nplt.xlim(0, 1)\nplt.ylim(0, 1)\nplt.show()\n"
        , to_list(stepg), to_list(stepy), to_list(splineg), to_list(spliney), to_list(interg), to_list(intery));
}


int main() {
    const step_function<double> step_function{
        0.2, {
            {0.4, 0.3},
            {0.5, 0.5},
            {0.8, 0.8}
        }
    };
    const double starting_grade{0.125}, final_grade{0.925};
    plot_hr(step_function, starting_grade, final_grade);
    return 0;
}
