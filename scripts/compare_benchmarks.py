import xml.etree.ElementTree as ET
import sys
import os
from collections import defaultdict

def parse_benchmark_results(xml_file):
    """Parses the Catch2 XML benchmark results including mean, standard deviation."""
    tree = ET.parse(xml_file)
    root = tree.getroot()
    
    results = defaultdict(dict)

    for testcase in root.findall(".//TestCase"):
        testcaseName = testcase.get("name")
        for benchmark in testcase.findall(".//BenchmarkResults"):
            name = benchmark.get("name")
            mean_elem = benchmark.find("mean")
            std_dev_elem = benchmark.find("standardDeviation")

            if mean_elem is not None and std_dev_elem is not None:
                mean = float(mean_elem.get("value"))
                std_dev = float(std_dev_elem.get("value"))

                results[testcaseName][name] = {
                    "mean": mean,
                    "std_dev": std_dev
                }

    return results


def compare_benchmarks(base_file, pr_file, threshold=0.05):
    """Compares benchmark results and generates a markdown table with sections."""
    base_results = parse_benchmark_results(base_file)
    pr_results = parse_benchmark_results(pr_file)

    markdown_output = "## Benchmark Comparison Results\n\n"
    regressions = []

    for section, base_benchmarks in base_results.items():
        pr_benchmarks = pr_results.get(section, {})

        markdown_output += f"### {section}\n\n"
        markdown_output += "| Benchmark | Base Mean (ms) | PR Mean (ms) | Change (%) | Base StdDev | PR StdDev |\n"
        markdown_output += "|-----------|---------------:|-------------:|-----------:|------------:|----------:|\n"

        for name, base_data in base_benchmarks.items():
            pr_data = pr_benchmarks.get(name)

            if pr_data is not None:
                base_mean = base_data["mean"] / 1e6  # Convert ns to ms
                pr_mean = pr_data["mean"] / 1e6      # Convert ns to ms
                base_std_dev = base_data["std_dev"] / 1e6  # Convert ns to ms
                pr_std_dev = pr_data["std_dev"] / 1e6      # Convert ns to ms

                percent_change = ((pr_mean - base_mean) / base_mean) * 100
                change_str = f"{percent_change:+.2f}%"

                if percent_change > threshold * 100:
                    regressions.append((name, base_mean, pr_mean, percent_change))

                markdown_output += f"| {name} | {base_mean:.3f} | {pr_mean:.3f} | **{change_str}** | {base_std_dev:.3f} | {pr_std_dev:.3f} |\n"

        markdown_output += "\n"

    if "GITHUB_STEP_SUMMARY" in os.environ :
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as f :
            print(markdown_output, file=f)

    # Set environment variable if regressions exist
    if regressions:
        env_file = os.getenv('GITHUB_ENV')
        with open(env_file, "a") as myfile:
            myfile.write("REGRESSED=true")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: compare_benchmarks.py <base_results.xml> <pr_results.xml>")
        sys.exit(1)

    compare_benchmarks(sys.argv[1], sys.argv[2])