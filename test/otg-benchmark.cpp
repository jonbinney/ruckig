#include <algorithm>

#include "randomizer.hpp"

#include <ruckig/ruckig.hpp>

#ifdef WITH_REFLEXXES
#include <ruckig/alternative/reflexxes.hpp>
#endif


using namespace ruckig;


template<size_t DOFs, class OTGType>
double check_calculation(OTGType& otg, InputParameter<DOFs>& input) {
    OutputParameter<DOFs> output;
    auto result = otg.update(input, output);
    return output.calculation_duration;
}


std::tuple<double, double> analyze(const std::vector<double>& v) {
    double sum = std::accumulate(v.begin(), v.end(), 0.0);
    double mean = sum / v.size();

    std::vector<double> diff(v.size());
    std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
    double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double std_deviation = std::sqrt(sq_sum / v.size());
    return {mean, std_deviation};
}


template<class OTGType>
void benchmark(size_t n, double number_trajectories, bool verbose = true) {
    OTGType otg {0.005};

    std::normal_distribution<double> position_dist {0.0, 4.0};
    std::normal_distribution<double> dynamic_dist {0.0, 0.8};
    std::uniform_real_distribution<double> limit_dist {0.1, 12.0};

    Randomizer<otg.degrees_of_freedom, decltype(position_dist)> p { position_dist, 42 };
    Randomizer<otg.degrees_of_freedom, decltype(dynamic_dist)> d { dynamic_dist, 43 };
    Randomizer<otg.degrees_of_freedom, decltype(limit_dist)> l { limit_dist, 44 };

    InputParameter<otg.degrees_of_freedom> input;
    std::vector<double> average, worst;

    for (size_t j = 0; j < n; ++j) {
        double average_ = 0.0;
        double worst_ = 0.0;
        size_t n {1};

        for (size_t i = 0; i < number_trajectories; ++i) {
            p.fill(input.current_position);
            d.fill_or_zero(input.current_velocity, 0.9);
            d.fill_or_zero(input.current_acceleration, 0.8);
            p.fill(input.target_position);
            d.fill_or_zero(input.target_velocity, 0.7);
            if constexpr (std::is_same<OTGType, Ruckig<otg.degrees_of_freedom, true>>::value) {
                d.fill_or_zero(input.target_acceleration, 0.6);
            }

            l.fill(input.max_velocity, input.target_velocity);
            l.fill(input.max_acceleration, input.target_acceleration);
            l.fill(input.max_jerk);

            if constexpr (std::is_same<OTGType, Ruckig<otg.degrees_of_freedom, true>>::value) {
                if (!otg.validate_input(input)) {
                    continue;
                }
            }

            double time = check_calculation(otg, input);
            average_ = average_ + (time - average_) / n;
            worst_ = std::max(worst_, time);
            ++n;
        }

        average.emplace_back(average_);
        worst.emplace_back(worst_);
    }

    auto [average_mean, average_std] = analyze(average);
    auto [worst_mean, worst_std] = analyze(worst);

    if (verbose) {
        std::cout << "---" << std::endl;
        std::cout << "Benchmark for " << otg.degrees_of_freedom << " DoFs on " << number_trajectories << " trajectories" << std::endl;
        std::cout << "Average Calculation Duration " << average_mean << " pm " << average_std << " [µs]" << std::endl;
        std::cout << "Worst Calculation Duration " << worst_mean << " pm " << worst_std << " [µs]" << std::endl;
    }

    // std::cout << otg.degrees_of_freedom << "\t" << average_mean << "\t" << average_std << "\t" << worst_mean << "\t" << worst_std << std::endl;
}


int main() {
    const size_t n {1}; // Number of iterations
    const size_t number_trajectories {64 * 1024};

    // Main comparison
    benchmark<Ruckig<7, true>>(n, number_trajectories);
    // benchmark<Reflexxes<7>>(n, number_trajectories);

    // Dependence performance vs. DoFs
    // benchmark<Ruckig<1, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<2, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<3, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<4, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<5, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<6, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<7, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<8, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<9, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<10, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<11, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<12, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<13, true>>(n, number_trajectories, false);
    // benchmark<Ruckig<14, true>>(n, number_trajectories, false);

    // 1ms worst case
    // benchmark<Ruckig<82, true>>(n, number_trajectories);
}
