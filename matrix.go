// matrix.go
package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
)

type Task struct {
	ID          int       `json:"id"`
	Title       string    `json:"title"`
	Description string    `json:"description"`
	Important   bool      `json:"important"`
	Urgent      bool      `json:"urgent"`
	Done        bool      `json:"done"`
	CreatedAt   time.Time `json:"created_at"`
}

func (t Task) Quadrant() string {
	if t.Important && t.Urgent {
		return "Важно и срочно"
	} else if t.Important && !t.Urgent {
		return "Важно, но не срочно"
	} else if !t.Important && t.Urgent {
		return "Не важно, но срочно"
	} else {
		return "Не важно и не срочно"
	}
}

type EisenhowerMatrix struct {
	tasks   []Task
	nextID  int
}

func NewEisenhowerMatrix() *EisenhowerMatrix {
	return &EisenhowerMatrix{
		tasks:  []Task{},
		nextID: 1,
	}
}

func (m *EisenhowerMatrix) AddTask(title, desc string, important, urgent bool) Task {
	task := Task{
		ID:          m.nextID,
		Title:       title,
		Description: desc,
		Important:   important,
		Urgent:      urgent,
		Done:        false,
		CreatedAt:   time.Now(),
	}
	m.tasks = append(m.tasks, task)
	m.nextID++
	return task
}

func (m *EisenhowerMatrix) FindTask(id int) *Task {
	for i := range m.tasks {
		if m.tasks[i].ID == id {
			return &m.tasks[i]
		}
	}
	return nil
}

func (m *EisenhowerMatrix) EditTask(id int, title, desc *string, important, urgent *bool) bool {
	task := m.FindTask(id)
	if task == nil {
		return false
	}
	if title != nil {
		task.Title = *title
	}
	if desc != nil {
		task.Description = *desc
	}
	if important != nil {
		task.Important = *important
	}
	if urgent != nil {
		task.Urgent = *urgent
	}
	return true
}

func (m *EisenhowerMatrix) ToggleDone(id int) bool {
	task := m.FindTask(id)
	if task == nil {
		return false
	}
	task.Done = !task.Done
	return true
}

func (m *EisenhowerMatrix) DeleteTask(id int) bool {
	for i, t := range m.tasks {
		if t.ID == id {
			m.tasks = append(m.tasks[:i], m.tasks[i+1:]...)
			return true
		}
	}
	return false
}

func (m *EisenhowerMatrix) FilterByQuadrant(quadrant string) []Task {
	var result []Task
	for _, t := range m.tasks {
		if t.Quadrant() == quadrant {
			result = append(result, t)
		}
	}
	return result
}

func (m *EisenhowerMatrix) Stats() map[string]interface{} {
	total := len(m.tasks)
	doneCount := 0
	quadCounts := make(map[string]int)
	for _, t := range m.tasks {
		if t.Done {
			doneCount++
		}
		quadCounts[t.Quadrant()]++
	}
	return map[string]interface{}{
		"total":     total,
		"done":      doneCount,
		"pending":   total - doneCount,
		"quadrants": quadCounts,
	}
}

func (m *EisenhowerMatrix) SaveToFile(filename string) error {
	data, err := json.MarshalIndent(m.tasks, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filename, data, 0644)
}

func (m *EisenhowerMatrix) LoadFromFile(filename string) error {
	data, err := os.ReadFile(filename)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}
	var loaded []Task
	if err := json.Unmarshal(data, &loaded); err != nil {
		return err
	}
	m.tasks = loaded
	for _, t := range loaded {
		if t.ID >= m.nextID {
			m.nextID = t.ID + 1
		}
	}
	return nil
}

func readString(prompt string) string {
	fmt.Print(prompt)
	reader := bufio.NewReader(os.Stdin)
	input, _ := reader.ReadString('\n')
	return strings.TrimSpace(input)
}

func readBool(prompt string) bool {
	for {
		input := readString(prompt)
		if input == "" {
			return false
		}
		if input == "y" || input == "Y" {
			return true
		} else if input == "n" || input == "N" {
			return false
		}
		fmt.Println("Введите y или n.")
	}
}

func readInt(prompt string) int {
	for {
		input := readString(prompt)
		if val, err := strconv.Atoi(input); err == nil {
			return val
		}
		fmt.Println("Введите число.")
	}
}

func printTask(task Task, showQuadrant bool) {
	doneMark := "✅"
	if !task.Done {
		doneMark = "⬜"
	}
	fmt.Printf("%s #%d - %s\n", doneMark, task.ID, task.Title)
	fmt.Printf("   Описание: %s\n", task.Description)
	if showQuadrant {
		fmt.Printf("   Квадрант: %s (важно: %t, срочно: %t)\n", task.Quadrant(), task.Important, task.Urgent)
	}
	fmt.Printf("   Создана: %s\n", task.CreatedAt.Format("2006-01-02 15:04:05"))
}

func main() {
	matrix := NewEisenhowerMatrix()
	if err := matrix.LoadFromFile("eisenhower_data.json"); err != nil {
		fmt.Println("Ошибка загрузки:", err)
	}

	for {
		fmt.Println("\n===== МАТРИЦА ЭЙЗЕНХАУЭРА (Go) =====")
		fmt.Println("1. Показать все задачи")
		fmt.Println("2. Добавить задачу")
		fmt.Println("3. Редактировать задачу")
		fmt.Println("4. Отметить выполненной / невыполненной")
		fmt.Println("5. Удалить задачу")
		fmt.Println("6. Показать статистику")
		fmt.Println("7. Сохранить в файл")
		fmt.Println("8. Загрузить из файла")
		fmt.Println("0. Выход")
		choice := readString("Выберите действие: ")

		switch choice {
		case "0":
			return
		case "1":
			if len(matrix.tasks) == 0 {
				fmt.Println("Задач нет.")
			} else {
				quadrants := []string{"Важно и срочно", "Важно, но не срочно", "Не важно, но срочно", "Не важно и не срочно"}
				for _, q := range quadrants {
					tasks := matrix.FilterByQuadrant(q)
					if len(tasks) > 0 {
						fmt.Printf("\n--- %s ---\n", q)
						for _, t := range tasks {
							printTask(t, false)
						}
					}
				}
			}
		case "2":
			title := readString("Введите название: ")
			if title == "" {
				fmt.Println("Название не может быть пустым.")
				continue
			}
			desc := readString("Введите описание: ")
			important := readBool("Задача важна? (y/n): ")
			urgent := readBool("Задача срочна? (y/n): ")
			task := matrix.AddTask(title, desc, important, urgent)
			fmt.Printf("Задача добавлена с ID %d, квадрант: %s\n", task.ID, task.Quadrant())
		case "3":
			id := readInt("Введите ID задачи: ")
			task := matrix.FindTask(id)
			if task == nil {
				fmt.Println("Задача не найдена.")
				continue
			}
			fmt.Println("Оставьте поле пустым, чтобы не менять.")
			newTitle := readString(fmt.Sprintf("Новое название (%s): ", task.Title))
			if newTitle != "" {
				task.Title = newTitle
			}
			newDesc := readString(fmt.Sprintf("Новое описание (%s): ", task.Description))
			if newDesc != "" {
				task.Description = newDesc
			}
			newImp := readString(fmt.Sprintf("Важна? (y/n) сейчас: %t: ", task.Important))
			if newImp != "" {
				task.Important = newImp == "y" || newImp == "Y"
			}
			newUrg := readString(fmt.Sprintf("Срочна? (y/n) сейчас: %t: ", task.Urgent))
			if newUrg != "" {
				task.Urgent = newUrg == "y" || newUrg == "Y"
			}
			fmt.Println("Задача обновлена.")
		case "4":
			id := readInt("Введите ID задачи: ")
			if matrix.ToggleDone(id) {
				fmt.Println("Статус выполнения изменён.")
			} else {
				fmt.Println("Задача не найдена.")
			}
		case "5":
			id := readInt("Введите ID задачи для удаления: ")
			if matrix.DeleteTask(id) {
				fmt.Println("Задача удалена.")
			} else {
				fmt.Println("Задача не найдена.")
			}
		case "6":
			stats := matrix.Stats()
			fmt.Println("\n=== СТАТИСТИКА ===")
			fmt.Printf("Всего задач: %d\n", stats["total"])
			fmt.Printf("Выполнено: %d\n", stats["done"])
			fmt.Printf("Осталось: %d\n", stats["pending"])
			quadrants := stats["quadrants"].(map[string]int)
			for q, count := range quadrants {
				fmt.Printf("  %s: %d\n", q, count)
			}
		case "7":
			if err := matrix.SaveToFile("eisenhower_data.json"); err != nil {
				fmt.Println("Ошибка сохранения:", err)
			} else {
				fmt.Println("Сохранено.")
			}
		case "8":
			if err := matrix.LoadFromFile("eisenhower_data.json"); err != nil {
				fmt.Println("Ошибка загрузки:", err)
			} else {
				fmt.Println("Загружено.")
			}
		default:
			fmt.Println("Неизвестная команда.")
		}
	}
}
