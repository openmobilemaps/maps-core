#include <vector>
#include <algorithm>

// Convert std::vector<double> to std::vector<float>
std::vector<float> convertToFloat(const std::vector<double>& input) {
    std::vector<float> output;
    output.reserve(input.size()); // Preallocate memory
    std::transform(input.begin(), input.end(), std::back_inserter(output),
                   [](double value) { return static_cast<float>(value); });
    return output;
}

// Convert std::vector<float> to std::vector<double>
std::vector<double> convertToDouble(const std::vector<float>& input) {
    std::vector<double> output;
    output.reserve(input.size()); // Preallocate memory
    std::transform(input.begin(), input.end(), std::back_inserter(output),
                   [](float value) { return static_cast<double>(value); });
    return output;
}

// Convert std::vector<double> to std::vector<float> with provided destination
void convertToFloat(const std::vector<double>& input, std::vector<float>& output) {
    output.clear();                // Clear the destination vector
    output.reserve(input.size());   // Preallocate memory
    std::transform(input.begin(), input.end(), std::back_inserter(output),
                   [](double value) { return static_cast<float>(value); });
}

// Convert std::vector<float> to std::vector<double> with provided destination
void convertToDouble(const std::vector<float>& input, std::vector<double>& output) {
    output.clear();                // Clear the destination vector
    output.reserve(input.size());   // Preallocate memory
    std::transform(input.begin(), input.end(), std::back_inserter(output),
                   [](float value) { return static_cast<double>(value); });
}

template <typename T>
std::vector<T> clone(const std::vector<T>& input) {
    return std::vector<T>(input);
}
