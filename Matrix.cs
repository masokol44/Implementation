// Matrix.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

public enum Quadrant
{
    [JsonPropertyName("Важно и срочно")]
    Do,
    [JsonPropertyName("Важно, но не срочно")]
    Plan,
    [JsonPropertyName("Не важно, но срочно")]
    Delegate,
    [JsonPropertyName("Не важно и не срочно")]
    Eliminate
}

public record Task(
    int Id,
    string Title,
    string Description,
    bool Important,
    bool Urgent,
    bool Done,
    string CreatedAt
)
{
    public Quadrant GetQuadrant()
    {
        if (Important && Urgent) return Quadrant.Do;
        if (Important) return Quadrant.Plan;
        if (Urgent) return Quadrant.Delegate;
        return Quadrant.Eliminate;
    }
}

public class EisenhowerMatrix
{
    private List<Task> tasks = new();
    private int nextId = 1;

    public IReadOnlyList<Task> Tasks => tasks.AsReadOnly();

    public Task AddTask(string title, string desc, bool important, bool urgent)
    {
        var task = new Task(
            nextId,
            title,
            desc,
            important,
            urgent,
            false,
            DateTime.Now.ToString("o")
        );
        tasks.Add(task);
        nextId++;
        return task;
    }

    public Task? FindTask(int id) => tasks.FirstOrDefault(t => t.Id == id);

    public bool EditTask(int id, string? title, string? desc, bool? important, bool? urgent)
    {
        var old = FindTask(id);
        if (old == null) return false;
        tasks.Remove(old);
        var updated = new Task(
            old.Id,
            title ?? old.Title,
            desc ?? old.Description,
            important ?? old.Important,
            urgent ?? old.Urgent,
            old.Done,
            old.CreatedAt
        );
        tasks.Add(updated);
        return true;
    }

    public bool ToggleDone(int id)
    {
        var old = FindTask(id);
        if (old == null) return false;
        tasks.Remove(old);
        var updated = old with { Done = !old.Done };
        tasks.Add(updated);
        return true;
    }

    public bool DeleteTask(int id) => tasks.RemoveAll(t => t.Id == id) > 0;

    public IEnumerable<Task> FilterByQuadrant(Quadrant q)
        => tasks.Where(t => t.GetQuadrant() == q);

    public Dictionary<string, object> GetStats()
    {
        var total = tasks.Count;
        var done = tasks.Count(t => t.Done);
        var quadrants = tasks.GroupBy(t => t.GetQuadrant())
                             .ToDictionary(g => g.Key.ToString(), g => g.Count());
        return new Dictionary<string, object>
        {
            ["total"] = total,
            ["done"] = done,
            ["pending"] = total - done,
            ["quadrants"] = quadrants
        };
    }

    public void SaveToFile(string filename)
    {
        var options = new JsonSerializerOptions { WriteIndented = true };
        string json = JsonSerializer.Serialize(tasks, options);
        File.WriteAllText(filename, json);
    }

    public void LoadFromFile(string filename)
    {
        if (!File.Exists(filename)) return;
        string json = File.ReadAllText(filename);
        var loaded = JsonSerializer.Deserialize<List<Task>>(json);
        if (loaded != null)
        {
            tasks = loaded;
            nextId = tasks.Any() ? tasks.Max(t => t.Id) + 1 : 1;
        }
    }
}

public static class Program
{
    private static string ReadString(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private static bool ReadBool(string prompt)
    {
        while (true)
        {
            var input = ReadString(prompt);
            if (string.IsNullOrEmpty(input)) return false;
            if (input.Equals("y", StringComparison.OrdinalIgnoreCase)) return true;
            if (input.Equals("n", StringComparison.OrdinalIgnoreCase)) return false;
            Console.WriteLine("Введите y или n.");
        }
    }

    private static int ReadInt(string prompt)
    {
        while (true)
        {
            Console.Write(prompt);
            if (int.TryParse(Console.ReadLine(), out int result))
                return result;
            Console.WriteLine("Введите число.");
        }
    }

    private static void PrintTask(Task task, bool showQuadrant)
    {
        string doneMark = task.Done ? "✅" : "⬜";
        Console.WriteLine($"{doneMark} #{task.Id} - {task.Title}");
        Console.WriteLine($"   Описание: {task.Description}");
        if (showQuadrant)
        {
            Console.WriteLine($"   Квадрант: {task.GetQuadrant()} (важно: {task.Important}, срочно: {task.Urgent})");
        }
        Console.WriteLine($"   Создана: {task.CreatedAt}");
    }

    public static void Main()
    {
        var matrix = new EisenhowerMatrix();
        try { matrix.LoadFromFile("eisenhower_data.json"); }
        catch { Console.WriteLine("Не удалось загрузить данные."); }

        while (true)
        {
            Console.WriteLine("\n===== МАТРИЦА ЭЙЗЕНХАУЭРА (C#) =====");
            Console.WriteLine("1. Показать все задачи");
            Console.WriteLine("2. Добавить задачу");
            Console.WriteLine("3. Редактировать задачу");
            Console.WriteLine("4. Отметить выполненной / невыполненной");
            Console.WriteLine("5. Удалить задачу");
            Console.WriteLine("6. Показать статистику");
            Console.WriteLine("7. Сохранить в файл");
            Console.WriteLine("8. Загрузить из файла");
            Console.WriteLine("0. Выход");
            string choice = ReadString("Выберите действие: ");

            switch (choice)
            {
                case "0": return;
                case "1":
                    if (!matrix.Tasks.Any()) Console.WriteLine("Задач нет.");
                    else
                    {
                        foreach (Quadrant q in Enum.GetValues(typeof(Quadrant)))
                        {
                            var tasks = matrix.FilterByQuadrant(q).ToList();
                            if (tasks.Any())
                            {
                                Console.WriteLine($"\n--- {q} ---");
                                tasks.ForEach(t => PrintTask(t, false));
                            }
                        }
                    }
                    break;
                case "2":
                    string title = ReadString("Введите название: ");
                    if (string.IsNullOrWhiteSpace(title))
                    {
                        Console.WriteLine("Название не может быть пустым.");
                        continue;
                    }
                    string desc = ReadString("Введите описание: ");
                    bool important = ReadBool("Задача важна? (y/n): ");
                    bool urgent = ReadBool("Задача срочна? (y/n): ");
                    var task = matrix.AddTask(title, desc, important, urgent);
                    Console.WriteLine($"Задача добавлена с ID {task.Id}, квадрант: {task.GetQuadrant()}");
                    break;
                case "3":
                    int id = ReadInt("Введите ID задачи: ");
                    var found = matrix.FindTask(id);
                    if (found == null)
                    {
                        Console.WriteLine("Задача не найдена.");
                        continue;
                    }
                    Console.WriteLine("Оставьте поле пустым, чтобы не менять.");
                    string newTitle = ReadString($"Новое название ({found.Title}): ");
                    string newDesc = ReadString($"Новое описание ({found.Description}): ");
                    bool? newImp = null;
                    string impInput = ReadString($"Важна? (y/n) сейчас: {(found.Important ? "y" : "n")}: ");
                    if (!string.IsNullOrEmpty(impInput)) newImp = impInput.Equals("y", StringComparison.OrdinalIgnoreCase);
                    bool? newUrg = null;
                    string urgInput = ReadString($"Срочна? (y/n) сейчас: {(found.Urgent ? "y" : "n")}: ");
                    if (!string.IsNullOrEmpty(urgInput)) newUrg = urgInput.Equals("y", StringComparison.OrdinalIgnoreCase);
                    if (matrix.EditTask(id,
                        string.IsNullOrEmpty(newTitle) ? null : newTitle,
                        string.IsNullOrEmpty(newDesc) ? null : newDesc,
                        newImp, newUrg))
                    {
                        Console.WriteLine("Задача обновлена.");
                    }
                    else Console.WriteLine("Ошибка.");
                    break;
                case "4":
                    int tid = ReadInt("Введите ID задачи: ");
                    if (matrix.ToggleDone(tid)) Console.WriteLine("Статус выполнения изменён.");
                    else Console.WriteLine("Задача не найдена.");
                    break;
                case "5":
                    int did = ReadInt("Введите ID задачи для удаления: ");
                    if (matrix.DeleteTask(did)) Console.WriteLine("Задача удалена.");
                    else Console.WriteLine("Задача не найдена.");
                    break;
                case "6":
                    var stats = matrix.GetStats();
                    Console.WriteLine("\n=== СТАТИСТИКА ===");
                    Console.WriteLine($"Всего задач: {stats["total"]}");
                    Console.WriteLine($"Выполнено: {stats["done"]}");
                    Console.WriteLine($"Осталось: {stats["pending"]}");
                    var quadrants = (Dictionary<string, int>)stats["quadrants"];
                    foreach (var kv in quadrants)
                        Console.WriteLine($"  {kv.Key}: {kv.Value}");
                    break;
                case "7":
                    try { matrix.SaveToFile("eisenhower_data.json"); Console.WriteLine("Сохранено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "8":
                    try { matrix.LoadFromFile("eisenhower_data.json"); Console.WriteLine("Загружено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                default: Console.WriteLine("Неизвестная команда."); break;
            }
        }
    }
}
