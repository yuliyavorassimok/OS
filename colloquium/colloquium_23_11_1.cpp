#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

// Интерфейс стратегии для выполнения действия над числами
class ActionStrategy {
public:
    virtual float execute(const std::vector<float>& numbers) const = 0;
    virtual ~ActionStrategy() {}
};

// Конкретная стратегия для сложения
class AdditionStrategy : public ActionStrategy {
public:
    float execute(const std::vector<float>& numbers) const override {
        float sum = 0.0f;
        for (float num : numbers) {
            sum += num;
        }
        return sum;
    }
};

// Конкретная стратегия для умножения
class MultiplicationStrategy : public ActionStrategy {
public:
    float execute(const std::vector<float>& numbers) const override {
        float result = 1.0f;
        for (float num : numbers) {
            result *= num;
        }
        return result;
    }
};

// Конкретная стратегия для суммы квадратов
class SquareSumStrategy : public ActionStrategy {
public:
    float execute(const std::vector<float>& numbers) const override {
        float sum = 0.0f;
        for (float num : numbers) {
            sum += num * num;
        }
        return sum;
    }
};

// Функция, выполняемая в отдельном потоке для обработки одного файла
void processFile(const std::string& filename, const ActionStrategy& strategy, float& result, std::mutex& mutex) {
    std::ifstream file(filename);
    if (file.is_open()) {
        std::vector<float> numbers;
        float num;
        while (file >> num) {
            numbers.push_back(num);
        }
        file.close();

        float partialResult = strategy.execute(numbers);

        std::lock_guard<std::mutex> lock(mutex);
        result += partialResult;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./program <directory> <num_threads>\n";
        return 1;
    }

    std::string directory = argv[1];
    int numThreads = std::stoi(argv[2]);

    std::vector<std::thread> threads;
    std::mutex mutex;
    float finalResult = 0.0f;

    // Создание потоков для обработки файлов
    for (int i = 1; i <= numThreads; ++i) {
        std::string filename = directory + "/in_" + std::to_string(i) + ".dat";
        threads.emplace_back(processFile, filename, AdditionStrategy(), std::ref(finalResult), std::ref(mutex));
    }

    // Ожидание завершения всех потоков
    for (auto& thread : threads) {
        thread.join();
    }

    // Запись результата в файл out.dat
    std::ofstream outFile(directory + "/out.dat");
    if (outFile.is_open()) {
        outFile << finalResult;
        outFile.close();
        std::cout << "Result has been written to out.dat\n";
    }
    else {
        std::cout << "Failed to open out.dat for writing\n";
        return 1;
    }

    return 0;
}