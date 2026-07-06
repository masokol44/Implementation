# matrix.py
import json
from dataclasses import dataclass, asdict
from enum import Enum
from typing import List, Optional
from datetime import datetime
from pathlib import Path

class Quadrant(Enum):
    DO = "Важно и срочно"
    PLAN = "Важно, но не срочно"
    DELEGATE = "Не важно, но срочно"
    ELIMINATE = "Не важно и не срочно"

@dataclass
class Task:
    id: int
    title: str
    description: str
    important: bool
    urgent: bool
    done: bool
    created_at: str

    def quadrant(self) -> Quadrant:
        if self.important and self.urgent:
            return Quadrant.DO
        elif self.important and not self.urgent:
            return Quadrant.PLAN
        elif not self.important and self.urgent:
            return Quadrant.DELEGATE
        else:
            return Quadrant.ELIMINATE

class EisenhowerMatrix:
    def __init__(self):
        self.tasks: List[Task] = []
        self.next_id = 1

    def add_task(self, title: str, desc: str, important: bool, urgent: bool) -> Task:
        task = Task(
            id=self.next_id,
            title=title,
            description=desc,
            important=important,
            urgent=urgent,
            done=False,
            created_at=datetime.now().isoformat()
        )
        self.tasks.append(task)
        self.next_id += 1
        return task

    def find_task(self, task_id: int) -> Optional[Task]:
        return next((t for t in self.tasks if t.id == task_id), None)

    def edit_task(self, task_id: int, title: str = None, desc: str = None,
                  important: bool = None, urgent: bool = None) -> bool:
        task = self.find_task(task_id)
        if not task:
            return False
        if title is not None:
            task.title = title
        if desc is not None:
            task.description = desc
        if important is not None:
            task.important = important
        if urgent is not None:
            task.urgent = urgent
        return True

    def toggle_done(self, task_id: int) -> bool:
        task = self.find_task(task_id)
        if not task:
            return False
        task.done = not task.done
        return True

    def delete_task(self, task_id: int) -> bool:
        task = self.find_task(task_id)
        if task:
            self.tasks.remove(task)
            return True
        return False

    def filter_by_quadrant(self, quadrant: Quadrant) -> List[Task]:
        return [t for t in self.tasks if t.quadrant() == quadrant]

    def filter_by_done(self, done: bool) -> List[Task]:
        return [t for t in self.tasks if t.done == done]

    def get_stats(self) -> dict:
        total = len(self.tasks)
        done_count = sum(1 for t in self.tasks if t.done)
        quad_counts = {q: 0 for q in Quadrant}
        for t in self.tasks:
            quad_counts[t.quadrant()] += 1
        return {
            "total": total,
            "done": done_count,
            "pending": total - done_count,
            "quadrants": quad_counts
        }

    def save_to_file(self, filename: str = "eisenhower_data.json") -> None:
        with open(filename, "w", encoding="utf-8") as f:
            json.dump([asdict(t) for t in self.tasks], f, ensure_ascii=False, indent=2)

    def load_from_file(self, filename: str = "eisenhower_data.json") -> None:
        path = Path(filename)
        if not path.exists():
            return
        with open(filename, "r", encoding="utf-8") as f:
            data = json.load(f)
            self.tasks.clear()
            for item in data:
                task = Task(
                    id=item["id"],
                    title=item["title"],
                    description=item["description"],
                    important=item["important"],
                    urgent=item["urgent"],
                    done=item["done"],
                    created_at=item["created_at"]
                )
                self.tasks.append(task)
                if task.id >= self.next_id:
                    self.next_id = task.id + 1

def print_task(task: Task, show_quadrant: bool = True) -> None:
    done_mark = "✅" if task.done else "⬜"
    print(f"{done_mark} #{task.id} - {task.title}")
    print(f"   Описание: {task.description}")
    if show_quadrant:
        q = task.quadrant()
        print(f"   Квадрант: {q.value} (важно: {task.important}, срочно: {task.urgent})")
    print(f"   Создана: {task.created_at}")

def main():
    matrix = EisenhowerMatrix()
    matrix.load_from_file()

    while True:
        print("\n===== МАТРИЦА ЭЙЗЕНХАУЭРА (Python) =====")
        print("1. Показать все задачи")
        print("2. Добавить задачу")
        print("3. Редактировать задачу")
        print("4. Отметить выполненной / невыполненной")
        print("5. Удалить задачу")
        print("6. Показать статистику")
        print("7. Сохранить в файл")
        print("8. Загрузить из файла")
        print("0. Выход")
        choice = input("Выберите действие: ").strip()

        if choice == "0":
            break
        elif choice == "1":
            if not matrix.tasks:
                print("Задач нет.")
                continue
            # Показать с группировкой по квадрантам
            for q in Quadrant:
                tasks = matrix.filter_by_quadrant(q)
                if tasks:
                    print(f"\n--- {q.value} ---")
                    for t in tasks:
                        print_task(t, show_quadrant=False)
        elif choice == "2":
            title = input("Введите название: ").strip()
            if not title:
                print("Название не может быть пустым.")
                continue
            desc = input("Введите описание: ").strip()
            important = input("Задача важна? (y/n): ").strip().lower() == 'y'
            urgent = input("Задача срочна? (y/n): ").strip().lower() == 'y'
            task = matrix.add_task(title, desc, important, urgent)
            print(f"Задача добавлена с ID {task.id}, квадрант: {task.quadrant().value}")
        elif choice == "3":
            try:
                task_id = int(input("Введите ID задачи: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            task = matrix.find_task(task_id)
            if not task:
                print("Задача не найдена.")
                continue
            print("Оставьте поле пустым, чтобы не менять.")
            new_title = input(f"Новое название ({task.title}): ").strip()
            if new_title:
                task.title = new_title
            new_desc = input(f"Новое описание ({task.description}): ").strip()
            if new_desc:
                task.description = new_desc
            new_imp = input(f"Важна? (y/n) сейчас: {'y' if task.important else 'n'}): ").strip()
            if new_imp:
                task.important = new_imp.lower() == 'y'
            new_urg = input(f"Срочна? (y/n) сейчас: {'y' if task.urgent else 'n'}): ").strip()
            if new_urg:
                task.urgent = new_urg.lower() == 'y'
            print("Задача обновлена.")
        elif choice == "4":
            try:
                task_id = int(input("Введите ID задачи: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            if matrix.toggle_done(task_id):
                print("Статус выполнения изменён.")
            else:
                print("Задача не найдена.")
        elif choice == "5":
            try:
                task_id = int(input("Введите ID задачи для удаления: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            if matrix.delete_task(task_id):
                print("Задача удалена.")
            else:
                print("Задача не найдена.")
        elif choice == "6":
            stats = matrix.get_stats()
            print("\n=== СТАТИСТИКА ===")
            print(f"Всего задач: {stats['total']}")
            print(f"Выполнено: {stats['done']}")
            print(f"Осталось: {stats['pending']}")
            for q, count in stats['quadrants'].items():
                print(f"  {q.value}: {count}")
        elif choice == "7":
            matrix.save_to_file()
            print("Сохранено.")
        elif choice == "8":
            matrix.load_from_file()
            print("Загружено.")
        else:
            print("Неизвестная команда.")

if __name__ == "__main__":
    main()
