#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <cmath>
#include <sstream>

class Action {
public:
    virtual double perform(const std::vector<double>& numbers) const = 0;
    virtual ~Action() = default;
};

class Addition : public Action {
public:
    double perform(const std::vector<double>& numbers) const override {
        double result = 0;
        for (double num : numbers) {
            result += num;
        }
        return result;
    }
};

class Multiplication : public Action {
public:
    double perform(const std::vector<double>& numbers) const override {
        double result = 1;
        for (double num : numbers) {
            result *= num;
        }
        return result;
    }
};

class SquareSum : public Action {
public:
    double perform(const std::vector<double>& numbers) const override {
        double result = 0;
        for (double num : numbers) {
            result += std::pow(num, 2);
        }
        return result;
    }
};

void processFile(const std::string& filename, const Action& action, double& result, std::mutex& mutex) {
    std::ifstream file(filename);
    if (file.is_open()) {
        std::vector<double> numbers;
        double num;
        while (file >> num) {
            numbers.push_back(num);
        }
        file.close();

        double partialResult = action.perform(numbers);

        std::lock_guard<std::mutex> lock(mutex);
        result += partialResult;
    }
    else {
        std::cerr << "Error opening file: " << filename << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <directory_path> <num_threads>" << std::endl;
        return 1;
    }

    std::string directoryPath = argv[1];
    int numThreads = std::stoi(argv[2]);

    Addition addition;
    Multiplication multiplication;
    SquareSum squareSum;

    std::vector<std::thread> threads;
    double totalResult = 0;
    std::mutex resultMutex;

    for (int i = 1; i <= numThreads; ++i) {
        std::string filename = directoryPath + "/in_" + std::to_string(i) + ".dat";
        int actionType = i % 3 + 1;

        const Action* action = nullptr; // Добавляем значение по умолчанию

        switch (actionType) {
        case 1:
            action = &addition;
            break;
        case 2:
            action = &multiplication;
            break;
        case 3:
            action = &squareSum;
            break;
        }

        // Проверка, что переменная action инициализирована
        if (!action) {
            std::cerr << "Error: Unsupported action type." << std::endl;
            return 1;
        }

        threads.emplace_back(processFile, filename, std::ref(*action), std::ref(totalResult), std::ref(resultMutex));
    }

    for (std::thread& thread : threads) {
        thread.join();
    }

    std::ofstream outFile(directoryPath + "/out.dat");
    if (outFile.is_open()) {
        std::stringstream resultStream;
        resultStream << "Total Result: " << totalResult << std::endl;
        outFile << resultStream.str();
        outFile.close();
    }
    else {
        std::cerr << "Error opening output file." << std::endl;
        return 1;
    }

    return 0;
}
