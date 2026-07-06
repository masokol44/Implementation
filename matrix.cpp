// matrix.cpp
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <memory>
#include <variant>
#include <map>

using namespace std;

enum class Quadrant { DO, PLAN, DELEGATE, ELIMINATE };

string quadrantToString(Quadrant q) {
    switch (q) {
        case Quadrant::DO: return "Важно и срочно";
        case Quadrant::PLAN: return "Важно, но не срочно";
        case Quadrant::DELEGATE: return "Не важно, но срочно";
        case Quadrant::ELIMINATE: return "Не важно и не срочно";
        default: return "";
    }
}

struct Task {
    int id;
    string title;
    string description;
    bool important;
    bool urgent;
    bool done;
    time_t createdAt;

    Task(int id, const string& title, const string& desc, bool important, bool urgent, bool done, time_t created)
        : id(id), title(title), description(desc), important(important), urgent(urgent), done(done), createdAt(created) {}

    Quadrant getQuadrant() const {
        if (important && urgent) return Quadrant::DO;
        if (important) return Quadrant::PLAN;
        if (urgent) return Quadrant::DELEGATE;
        return Quadrant::ELIMINATE;
    }

    string formatTime() const {
        char buf[20];
        tm* tm_info = localtime(&createdAt);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
        return string(buf);
    }
};

class EisenhowerMatrix {
private:
    vector<Task> tasks;
    int nextId = 1;

public:
    Task& addTask(const string& title, const string& desc, bool important, bool urgent) {
        Task task(nextId, title, desc, important, urgent, false, time(nullptr));
        tasks.push_back(task);
        nextId++;
        return tasks.back();
    }

    Task* findTask(int id) {
        auto it = find_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
        return it != tasks.end() ? &(*it) : nullptr;
    }

    bool editTask(int id, const string* title, const string* desc, const bool* important, const bool* urgent) {
        Task* task = findTask(id);
        if (!task) return false;
        if (title) task->title = *title;
        if (desc) task->description = *desc;
        if (important) task->important = *important;
        if (urgent) task->urgent = *urgent;
        return true;
    }

    bool toggleDone(int id) {
        Task* task = findTask(id);
        if (!task) return false;
        task->done = !task->done;
        return true;
    }

    bool deleteTask(int id) {
        auto it = find_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
        if (it == tasks.end()) return false;
        tasks.erase(it);
        return true;
    }

    vector<Task> filterByQuadrant(Quadrant q) const {
        vector<Task> result;
        for (const auto& t : tasks) {
            if (t.getQuadrant() == q) result.push_back(t);
        }
        return result;
    }

    map<string, int> getStats() const {
        map<string, int> stats;
        stats["total"] = tasks.size();
        int done = 0;
        map<string, int> quadCounts;
        for (const auto& t : tasks) {
            if (t.done) done++;
            quadCounts[quadrantToString(t.getQuadrant())]++;
        }
        stats["done"] = done;
        stats["pending"] = tasks.size() - done;
        for (const auto& kv : quadCounts) {
            stats[kv.first] = kv.second;
        }
        return stats;
    }

    void saveToFile(const string& filename = "eisenhower_data.txt") {
        ofstream out(filename);
        if (!out) return;
        for (const auto& t : tasks) {
            out << t.id << '|'
                << t.title << '|'
                << t.description << '|'
                << t.important << '|'
                << t.urgent << '|'
                << t.done << '|'
                << t.createdAt << '\n';
        }
    }

    void loadFromFile(const string& filename = "eisenhower_data.txt") {
        ifstream in(filename);
        if (!in) return;
        tasks.clear();
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            string idStr, title, desc, impStr, urgStr, doneStr, timeStr;
            getline(ss, idStr, '|');
            getline(ss, title, '|');
            getline(ss, desc, '|');
            getline(ss, impStr, '|');
            getline(ss, urgStr, '|');
            getline(ss, doneStr, '|');
            getline(ss, timeStr, '|');
            int id = stoi(idStr);
            bool important = impStr == "1";
            bool urgent = urgStr == "1";
            bool done = doneStr == "1";
            time_t created = stoll(timeStr);
            tasks.emplace_back(id, title, desc, important, urgent, done, created);
            if (id >= nextId) nextId = id + 1;
        }
    }

    const vector<Task>& getTasks() const { return tasks; }
};

string readString(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    return input;
}

bool readBool(const string& prompt) {
    while (true) {
        string input = readString(prompt);
        if (input == "y" || input == "Y") return true;
        if (input == "n" || input == "N") return false;
        cout << "Введите y или n.\n";
    }
}

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        try {
            return stoi(input);
        } catch (...) {
            cout << "Введите число.\n";
        }
    }
}

void printTask(const Task& task, bool showQuadrant) {
    string doneMark = task.done ? "✅" : "⬜";
    cout << doneMark << " #" << task.id << " - " << task.title << "\n";
    cout << "   Описание: " << task.description << "\n";
    if (showQuadrant) {
        cout << "   Квадрант: " << quadrantToString(task.getQuadrant())
             << " (важно: " << (task.important ? "да" : "нет")
             << ", срочно: " << (task.urgent ? "да" : "нет") << ")\n";
    }
    cout << "   Создана: " << task.formatTime() << "\n";
}

int main() {
    EisenhowerMatrix matrix;
    matrix.loadFromFile();

    while (true) {
        cout << "\n===== МАТРИЦА ЭЙЗЕНХАУЭРА (C++) =====" << endl;
        cout << "1. Показать все задачи\n";
        cout << "2. Добавить задачу\n";
        cout << "3. Редактировать задачу\n";
        cout << "4. Отметить выполненной / невыполненной\n";
        cout << "5. Удалить задачу\n";
        cout << "6. Показать статистику\n";
        cout << "7. Сохранить в файл\n";
        cout << "8. Загрузить из файла\n";
        cout << "0. Выход\n";
        string choice = readString("Выберите действие: ");

        if (choice == "0") break;

        if (choice == "1") {
            if (matrix.getTasks().empty()) {
                cout << "Задач нет.\n";
            } else {
                vector<Quadrant> quads = {Quadrant::DO, Quadrant::PLAN, Quadrant::DELEGATE, Quadrant::ELIMINATE};
                for (auto q : quads) {
                    auto tasks = matrix.filterByQuadrant(q);
                    if (!tasks.empty()) {
                        cout << "\n--- " << quadrantToString(q) << " ---\n";
                        for (const auto& t : tasks) {
                            printTask(t, false);
                        }
                    }
                }
            }
        } else if (choice == "2") {
            string title = readString("Введите название: ");
            if (title.empty()) {
                cout << "Название не может быть пустым.\n";
                continue;
            }
            string desc = readString("Введите описание: ");
            bool important = readBool("Задача важна? (y/n): ");
            bool urgent = readBool("Задача срочна? (y/n): ");
            Task& task = matrix.addTask(title, desc, important, urgent);
            cout << "Задача добавлена с ID " << task.id << ", квадрант: " << quadrantToString(task.getQuadrant()) << "\n";
        } else if (choice == "3") {
            int id = readInt("Введите ID задачи: ");
            Task* task = matrix.findTask(id);
            if (!task) {
                cout << "Задача не найдена.\n";
                continue;
            }
            cout << "Оставьте поле пустым, чтобы не менять.\n";
            string newTitle = readString("Новое название (" + task->title + "): ");
            string newDesc = readString("Новое описание (" + task->description + "): ");
            string impInput = readString("Важна? (y/n) сейчас: " + string(task->important ? "y" : "n") + ": ");
            string urgInput = readString("Срочна? (y/n) сейчас: " + string(task->urgent ? "y" : "n") + ": ");
            bool* newImp = nullptr;
            bool* newUrg = nullptr;
            bool impVal, urgVal;
            if (!impInput.empty()) { impVal = impInput == "y" || impInput == "Y"; newImp = &impVal; }
            if (!urgInput.empty()) { urgVal = urgInput == "y" || urgInput == "Y"; newUrg = &urgVal; }
            if (matrix.editTask(id,
                                newTitle.empty() ? nullptr : &newTitle,
                                newDesc.empty() ? nullptr : &newDesc,
                                newImp, newUrg)) {
                cout << "Задача обновлена.\n";
            } else {
                cout << "Ошибка.\n";
            }
        } else if (choice == "4") {
            int id = readInt("Введите ID задачи: ");
            if (matrix.toggleDone(id)) {
                cout << "Статус выполнения изменён.\n";
            } else {
                cout << "Задача не найдена.\n";
            }
        } else if (choice == "5") {
            int id = readInt("Введите ID задачи для удаления: ");
            if (matrix.deleteTask(id)) {
                cout << "Задача удалена.\n";
            } else {
                cout << "Задача не найдена.\n";
            }
        } else if (choice == "6") {
            auto stats = matrix.getStats();
            cout << "\n=== СТАТИСТИКА ===\n";
            cout << "Всего задач: " << stats["total"] << "\n";
            cout << "Выполнено: " << stats["done"] << "\n";
            cout << "Осталось: " << stats["pending"] << "\n";
            for (const auto& kv : stats) {
                if (kv.first != "total" && kv.first != "done" && kv.first != "pending") {
                    cout << "  " << kv.first << ": " << kv.second << "\n";
                }
            }
        } else if (choice == "7") {
            matrix.saveToFile();
            cout << "Сохранено.\n";
        } else if (choice == "8") {
            matrix.loadFromFile();
            cout << "Загружено.\n";
        } else {
            cout << "Неизвестная команда.\n";
        }
    }
    return 0;
}
