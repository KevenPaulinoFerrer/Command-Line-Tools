
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <stdexcept>

namespace fs = std::filesystem;
using std::vector, std::string;

// Platform-specific headers
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#else
#error Unsupported operating system
#endif

// ==========================================
// ERROR HANDLING
// ==========================================

string error_message(const string &error)
{
    if (error == "missing_command")
        return "Error: Missing command. Use 'help' to see available commands.";

    if (error == "unknown_command")
        return "Error: Unknown command. Use 'help' to see available commands.";

    if (error == "invalid_arguments")
        return "Error: Invalid number of arguments. Use 'help' for command syntax.";

    if (error == "invalid_number")
        return "Error: Arguments must be valid numbers.";

    if (error == "invalid_st")
        return "Error: Sequential time must be greater than zero.";

    if (error == "invalid_pt")
        return "Error: Parallel time must be greater than zero.";

    if (error == "invalid_processors")
        return "Error: Number of processors must be greater than zero.";

    if (error == "invalid_speedup")
        return "Error: Speedup must be greater than zero.";

    if (error == "report_error")
        return "Error: Unable to create or write the report.";

    if (error == "directory_error")
        return "Error: Unable to create the report directory.";

    return "Error: Unknown error.";
}

void print_error(const string &error)
{
    std::cerr << error_message(error) << '\n';
}

// ==========================================
// FILESYSTEM
// ==========================================

fs::path get_executable_directory()
{
    fs::path executable;

#ifdef _WIN32

    std::vector<wchar_t> buffer(32768);

    DWORD size = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

    if (size == 0 || size >= buffer.size())
        throw std::runtime_error("Unable to retrieve executable path.");

    executable = fs::path(buffer.data());

#elif defined(__APPLE__)

    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);

    vector<char> buffer(size);

    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
        throw std::runtime_error("Unable to retrieve executable path.");

    executable = fs::path(buffer.data());

    executable = fs::canonical(executable);

#elif defined(__linux__)

    executable = fs::read_symlink("/proc/self/exe");

#endif

    return executable.parent_path();
}

// ==========================================
// HELP
// ==========================================

string help()
{
    return "========================================\n"
           "       PARALLEL PERFORMANCE TOOL\n"
           "========================================\n\n"

           "USAGE:\n"
           "  program <command> [arguments]\n\n"

           "AVAILABLE COMMANDS:\n\n"

           "  help\n"
           "      Displays the available commands.\n\n"

           "  spd <sequential_time> <parallel_time>\n"
           "      Calculates the speedup.\n"
           "      Example: program spd 100 25\n\n"

           "  efc <speedup> <processors>\n"
           "      Calculates the parallel efficiency.\n"
           "      Example: program efc 4 8\n\n"

           "  spdefc <sequential_time> <parallel_time> <processors>\n"
           "      Calculates both speedup and efficiency.\n"
           "      Example: program spdefc 100 25 8\n\n"

           "OUTPUT:\n"
           "  Generates a report containing the calculation results.\n"
           "  The report is saved in the executable's directory.\n\n"

           "========================================\n";
}

// ==========================================
// CALCULATIONS
// ==========================================

double calc_speedup(double st, double pt)
{
    return st / pt;
}

double calc_efficiency(double speedup, double processors)
{
    return speedup / processors;
}

// ==========================================
// INPUT VALIDATION
// ==========================================

double parse_number(const char *argument)
{
    try
    {
        size_t position = 0;

        double value = std::stod(argument, &position);

        // Reject partially valid numbers, such as "123abc"
        if (position != string(argument).size() || !std::isfinite(value))
            throw std::invalid_argument("Invalid number.");

        return value;
    }
    catch (const std::exception &)
    {
        throw std::invalid_argument("Invalid number.");
    }
}

// ==========================================
// REPORT GENERATION
// ==========================================

string formulate_report_text(const vector<string> &titles, const vector<double> &values)
{
    if (titles.size() != values.size())
        throw std::invalid_argument("Mismatched report data.");

    string text;

    for (size_t i = 0; i < titles.size(); i++)
    {
        text.append(
            titles.at(i) + ": " +
            std::to_string(values.at(i)) + '\n');
    }

    return text;
}

void create_report(fs::path &directory, const string &report_name, const string &text)
{
    directory /= "Reports";

    std::error_code ec;
    fs::create_directories(directory, ec);

    if (ec)
    {
        print_error("directory_error");
        std::cerr << "Details: " << ec.message() << '\n';
        throw std::runtime_error("Directory creation failed.");
    }

    fs::path report_path = directory / report_name;

    std::ofstream file(report_path);

    if (!file.is_open())
        throw std::runtime_error("Unable to create report.");

    file << text;

    if (!file.good())
        throw std::runtime_error("Unable to write report.");

    file.close();

    if (file.fail())
        throw std::runtime_error("Unable to close report.");

    std::cout << "Successfully created report at: "
              << report_path << '\n';
}

// ==========================================
// TIMESTAMP
// ==========================================

string get_current_timestamp()
{
    time_t current_time = time(nullptr);
    tm *local_time = localtime(&current_time);

    if (local_time == nullptr)
        throw std::runtime_error("Unable to retrieve current time.");

    std::stringstream ss;

    ss << std::put_time(local_time, "%Y-%m-%d_%H-%M-%S");

    return ss.str();
}

// ==========================================
// MAIN
// ==========================================

int main(int argc, char *argv[])
{
    const string EFFICIENCY = "efc";
    const string SPEEDUP = "spd";
    const string SPEEDUP_EFFICIENCY = "spdefc";
    const string HELP = "help";

    // Missing command
    if (argc < 2)
    {
        print_error("missing_command");
        std::cout << '\n'
                  << help();
        return 1;
    }

    // Convert command to lowercase
    string arg1 = argv[1];

    for (char &c : arg1)
    {
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c)));
    }

    // Help command
    if (arg1 == HELP)
    {
        if (argc != 2)
        {
            print_error("invalid_arguments");
            return 1;
        }

        std::cout << help();
        return 0;
    }

    // Check whether command exists
    if (arg1 != SPEEDUP && arg1 != EFFICIENCY && arg1 != SPEEDUP_EFFICIENCY)
    {
        print_error("unknown_command");
        return 1;
    }

    // Validate argument count
    int expected_argc = (arg1 == SPEEDUP_EFFICIENCY) ? 5 : 4;

    if (argc != expected_argc)
    {
        print_error("invalid_arguments");
        return 1;
    }

    vector<double> values;
    vector<string> titles;

    try
    {
        // ======================================
        // SPEEDUP
        // ======================================

        if (arg1 == SPEEDUP)
        {
            double st = parse_number(argv[2]);
            double pt = parse_number(argv[3]);

            if (st <= 0)
                throw std::runtime_error("invalid_st");

            if (pt <= 0)
                throw std::runtime_error("invalid_pt");

            double speedup = calc_speedup(st, pt);

            values = {st, pt, speedup};

            titles = {
                "Sequential Time",
                "Parallel Time",
                "Speedup"};
        }

        // ======================================
        // EFFICIENCY
        // ======================================

        else if (arg1 == EFFICIENCY)
        {
            double speedup = parse_number(argv[2]);
            double processors = parse_number(argv[3]);

            if (speedup <= 0)
                throw std::runtime_error("invalid_speedup");

            if (processors <= 0)
                throw std::runtime_error("invalid_processors");

            double efficiency = calc_efficiency(speedup, processors);

            values = {speedup, processors, efficiency};

            titles = {
                "Speedup",
                "Processors",
                "Efficiency"};
        }

        // ======================================
        // SPEEDUP + EFFICIENCY
        // ======================================

        else if (arg1 == SPEEDUP_EFFICIENCY)
        {
            double st = parse_number(argv[2]);
            double pt = parse_number(argv[3]);
            double processors = parse_number(argv[4]);

            if (st <= 0)
                throw std::runtime_error("invalid_st");

            if (pt <= 0)
                throw std::runtime_error("invalid_pt");

            if (processors <= 0)
                throw std::runtime_error("invalid_processors");

            double speedup = calc_speedup(st, pt);
            double efficiency = calc_efficiency(speedup, processors);

            values = {st, pt, processors, speedup, efficiency};

            titles = {
                "Sequential Time",
                "Parallel Time",
                "Processors",
                "Speedup",
                "Efficiency"};
        }

        // ======================================
        // CREATE REPORT
        // ======================================

        string timestamp = get_current_timestamp();

        string text = formulate_report_text(titles, values);

        fs::path directory = get_executable_directory();

        string report_name = "Report_" + timestamp + ".txt";

        create_report(directory, report_name, text);
    }
    catch (const std::runtime_error &e)
    {
        string error = e.what();

        if (error == "invalid_st" ||
            error == "invalid_pt" ||
            error == "invalid_processors" ||
            error == "invalid_speedup")
        {
            print_error(error);
        }
        else
        {
            std::cerr << "Error: " << error << '\n';
        }

        return 1;
    }
    catch (const std::invalid_argument &)
    {
        print_error("invalid_number");
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
