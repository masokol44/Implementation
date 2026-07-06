// Matrix.java
import java.io.*;
import java.nio.file.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.stream.Collectors;

enum Quadrant {
    DO("Важно и срочно"),
    PLAN("Важно, но не срочно"),
    DELEGATE("Не важно, но срочно"),
    ELIMINATE("Не важно и не срочно");

    private final String display;
    Quadrant(String display) { this.display = display; }
    public String getDisplay() { return display; }
}

record Task(int id, String title, String description, boolean important, boolean urgent, boolean done, String createdAt) implements Serializable {
    public Quadrant quadrant() {
        if (important && urgent) return Quadrant.DO;
        if (important) return Quadrant.PLAN;
        if (urgent) return Quadrant.DELEGATE;
        return Quadrant.ELIMINATE;
    }
}

class EisenhowerMatrix implements Serializable {
    private static final long serialVersionUID = 1L;
    private List<Task> tasks = new ArrayList<>();
    private int nextId = 1;

    public Task addTask(String title, String desc, boolean important, boolean urgent) {
        Task task = new Task(nextId, title, desc, important, urgent, false,
                LocalDateTime.now().format(DateTimeFormatter.ISO_LOCAL_DATE_TIME));
        tasks.add(task);
        nextId++;
        return task;
    }

    public Optional<Task> findTask(int id) {
        return tasks.stream().filter(t -> t.id() == id).findFirst();
    }

    public boolean editTask(int id, String title, String desc, Boolean important, Boolean urgent) {
        Optional<Task> opt = findTask(id);
        if (opt.isEmpty()) return false;
        Task old = opt.get();
        tasks.remove(old);
        Task updated = new Task(
                old.id(),
                title != null ? title : old.title(),
                desc != null ? desc : old.description(),
                important != null ? important : old.important(),
                urgent != null ? urgent : old.urgent(),
                old.done(),
                old.createdAt()
        );
        tasks.add(updated);
        return true;
    }

    public boolean toggleDone(int id) {
        Optional<Task> opt = findTask(id);
        if (opt.isEmpty()) return false;
        Task old = opt.get();
        tasks.remove(old);
        Task updated = new Task(
                old.id(), old.title(), old.description(),
                old.important(), old.urgent(), !old.done(), old.createdAt()
        );
        tasks.add(updated);
        return true;
    }

    public boolean deleteTask(int id) {
        return tasks.removeIf(t -> t.id() == id);
    }

    public List<Task> filterByQuadrant(Quadrant q) {
        return tasks.stream().filter(t -> t.quadrant() == q).collect(Collectors.toList());
    }

    public Map<String, Object> getStats() {
        int total = tasks.size();
        int done = (int) tasks.stream().filter(Task::done).count();
        Map<Quadrant, Long> quadCounts = tasks.stream().collect(Collectors.groupingBy(Task::quadrant, Collectors.counting()));
        Map<String, Object> stats = new HashMap<>();
        stats.put("total", total);
        stats.put("done", done);
        stats.put("pending", total - done);
        stats.put("quadrants", quadCounts);
        return stats;
    }

    public void saveToFile(String filename) throws IOException {
        try (ObjectOutputStream oos = new ObjectOutputStream(Files.newOutputStream(Paths.get(filename)))) {
            oos.writeObject(this);
        }
    }

    public void loadFromFile(String filename) throws IOException, ClassNotFoundException {
        try (ObjectInputStream ois = new ObjectInputStream(Files.newInputStream(Paths.get(filename)))) {
            EisenhowerMatrix loaded = (EisenhowerMatrix) ois.readObject();
            this.tasks = loaded.tasks;
            this.nextId = loaded.nextId;
        }
    }

    public List<Task> getTasks() { return Collections.unmodifiableList(tasks); }
}

public class Matrix {
    private static final Scanner scanner = new Scanner(System.in);

    private static String readString(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private static boolean readBool(String prompt) {
        while (true) {
            String input = readString(prompt);
            if (input.isEmpty()) return false;
            if (input.equalsIgnoreCase("y")) return true;
            if (input.equalsIgnoreCase("n")) return false;
            System.out.println("Введите y или n.");
        }
    }

    private static int readInt(String prompt) {
        while (true) {
            try {
                System.out.print(prompt);
                return Integer.parseInt(scanner.nextLine().trim());
            } catch (NumberFormatException e) {
                System.out.println("Введите число.");
            }
        }
    }

    private static void printTask(Task task, boolean showQuadrant) {
        String doneMark = task.done() ? "✅" : "⬜";
        System.out.printf("%s #%d - %s%n", doneMark, task.id(), task.title());
        System.out.printf("   Описание: %s%n", task.description());
        if (showQuadrant) {
            System.out.printf("   Квадрант: %s (важно: %b, срочно: %b)%n",
                    task.quadrant().getDisplay(), task.important(), task.urgent());
        }
        System.out.printf("   Создана: %s%n", task.createdAt());
    }

    public static void main(String[] args) {
        EisenhowerMatrix matrix = new EisenhowerMatrix();
        try {
            matrix.loadFromFile("eisenhower_data.ser");
        } catch (IOException | ClassNotFoundException e) {
            System.out.println("Не удалось загрузить данные, начинаем с пустой доски.");
        }

        while (true) {
            System.out.println("\n===== МАТРИЦА ЭЙЗЕНХАУЭРА (Java) =====");
            System.out.println("1. Показать все задачи");
            System.out.println("2. Добавить задачу");
            System.out.println("3. Редактировать задачу");
            System.out.println("4. Отметить выполненной / невыполненной");
            System.out.println("5. Удалить задачу");
            System.out.println("6. Показать статистику");
            System.out.println("7. Сохранить в файл");
            System.out.println("8. Загрузить из файла");
            System.out.println("0. Выход");
            String choice = readString("Выберите действие: ");

            switch (choice) {
                case "0" -> { return; }
                case "1" -> {
                    if (matrix.getTasks().isEmpty()) {
                        System.out.println("Задач нет.");
                    } else {
                        for (Quadrant q : Quadrant.values()) {
                            var tasks = matrix.filterByQuadrant(q);
                            if (!tasks.isEmpty()) {
                                System.out.println("\n--- " + q.getDisplay() + " ---");
                                tasks.forEach(t -> printTask(t, false));
                            }
                        }
                    }
                }
                case "2" -> {
                    String title = readString("Введите название: ");
                    if (title.isBlank()) {
                        System.out.println("Название не может быть пустым.");
                        continue;
                    }
                    String desc = readString("Введите описание: ");
                    boolean important = readBool("Задача важна? (y/n): ");
                    boolean urgent = readBool("Задача срочна? (y/n): ");
                    Task task = matrix.addTask(title, desc, important, urgent);
                    System.out.printf("Задача добавлена с ID %d, квадрант: %s%n", task.id(), task.quadrant().getDisplay());
                }
                case "3" -> {
                    int id = readInt("Введите ID задачи: ");
                    var opt = matrix.findTask(id);
                    if (opt.isEmpty()) {
                        System.out.println("Задача не найдена.");
                        continue;
                    }
                    Task old = opt.get();
                    System.out.println("Оставьте поле пустым, чтобы не менять.");
                    String newTitle = readString("Новое название (" + old.title() + "): ");
                    String newDesc = readString("Новое описание (" + old.description() + "): ");
                    Boolean newImp = null;
                    String impInput = readString("Важна? (y/n) сейчас: " + (old.important() ? "y" : "n") + ": ");
                    if (!impInput.isEmpty()) newImp = impInput.equalsIgnoreCase("y");
                    Boolean newUrg = null;
                    String urgInput = readString("Срочна? (y/n) сейчас: " + (old.urgent() ? "y" : "n") + ": ");
                    if (!urgInput.isEmpty()) newUrg = urgInput.equalsIgnoreCase("y");
                    if (matrix.editTask(id,
                            newTitle.isEmpty() ? null : newTitle,
                            newDesc.isEmpty() ? null : newDesc,
                            newImp, newUrg)) {
                        System.out.println("Задача обновлена.");
                    } else {
                        System.out.println("Ошибка.");
                    }
                }
                case "4" -> {
                    int id = readInt("Введите ID задачи: ");
                    if (matrix.toggleDone(id)) {
                        System.out.println("Статус выполнения изменён.");
                    } else {
                        System.out.println("Задача не найдена.");
                    }
                }
                case "5" -> {
                    int id = readInt("Введите ID задачи для удаления: ");
                    if (matrix.deleteTask(id)) {
                        System.out.println("Задача удалена.");
                    } else {
                        System.out.println("Задача не найдена.");
                    }
                }
                case "6" -> {
                    var stats = matrix.getStats();
                    System.out.println("\n=== СТАТИСТИКА ===");
                    System.out.println("Всего задач: " + stats.get("total"));
                    System.out.println("Выполнено: " + stats.get("done"));
                    System.out.println("Осталось: " + stats.get("pending"));
                    @SuppressWarnings("unchecked")
                    Map<Quadrant, Long> quad = (Map<Quadrant, Long>) stats.get("quadrants");
                    for (var entry : quad.entrySet()) {
                        System.out.println("  " + entry.getKey().getDisplay() + ": " + entry.getValue());
                    }
                }
                case "7" -> {
                    try {
                        matrix.saveToFile("eisenhower_data.ser");
                        System.out.println("Сохранено.");
                    } catch (IOException e) {
                        System.out.println("Ошибка сохранения: " + e.getMessage());
                    }
                }
                case "8" -> {
                    try {
                        matrix.loadFromFile("eisenhower_data.ser");
                        System.out.println("Загружено.");
                    } catch (IOException | ClassNotFoundException e) {
                        System.out.println("Ошибка загрузки: " + e.getMessage());
                    }
                }
                default -> System.out.println("Неизвестная команда.");
            }
        }
    }
}
