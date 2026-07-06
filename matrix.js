// matrix.js
const fs = require('fs').promises;
const readline = require('readline');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

const question = (prompt) => new Promise(resolve => rl.question(prompt, resolve));

class Task {
    constructor(id, title, description, important, urgent, done = false, createdAt = new Date().toISOString()) {
        this.id = id;
        this.title = title;
        this.description = description;
        this.important = important;
        this.urgent = urgent;
        this.done = done;
        this.createdAt = createdAt;
    }

    getQuadrant() {
        if (this.important && this.urgent) return 'Важно и срочно';
        if (this.important) return 'Важно, но не срочно';
        if (this.urgent) return 'Не важно, но срочно';
        return 'Не важно и не срочно';
    }
}

class EisenhowerMatrix {
    constructor() {
        this.tasks = [];
        this.nextId = 1;
    }

    addTask(title, description, important, urgent) {
        const task = new Task(this.nextId, title, description, important, urgent);
        this.tasks.push(task);
        this.nextId++;
        return task;
    }

    findTask(id) {
        return this.tasks.find(t => t.id === id);
    }

    editTask(id, title, description, important, urgent) {
        const task = this.findTask(id);
        if (!task) return false;
        if (title !== undefined) task.title = title;
        if (description !== undefined) task.description = description;
        if (important !== undefined) task.important = important;
        if (urgent !== undefined) task.urgent = urgent;
        return true;
    }

    toggleDone(id) {
        const task = this.findTask(id);
        if (!task) return false;
        task.done = !task.done;
        return true;
    }

    deleteTask(id) {
        const index = this.tasks.findIndex(t => t.id === id);
        if (index === -1) return false;
        this.tasks.splice(index, 1);
        return true;
    }

    filterByQuadrant(quadrant) {
        return this.tasks.filter(t => t.getQuadrant() === quadrant);
    }

    getStats() {
        const total = this.tasks.length;
        const done = this.tasks.filter(t => t.done).length;
        const quadrants = {};
        for (const t of this.tasks) {
            const q = t.getQuadrant();
            quadrants[q] = (quadrants[q] || 0) + 1;
        }
        return { total, done, pending: total - done, quadrants };
    }

    async saveToFile(filename = 'eisenhower_data.json') {
        await fs.writeFile(filename, JSON.stringify(this.tasks, null, 2));
    }

    async loadFromFile(filename = 'eisenhower_data.json') {
        try {
            const data = await fs.readFile(filename, 'utf8');
            const loaded = JSON.parse(data);
            this.tasks = loaded.map(t => Object.assign(new Task(0), t));
            this.nextId = this.tasks.reduce((max, t) => Math.max(max, t.id), 0) + 1;
        } catch (err) {
            if (err.code !== 'ENOENT') throw err;
        }
    }
}

function printTask(task, showQuadrant = true) {
    const doneMark = task.done ? '✅' : '⬜';
    console.log(`${doneMark} #${task.id} - ${task.title}`);
    console.log(`   Описание: ${task.description}`);
    if (showQuadrant) {
        console.log(`   Квадрант: ${task.getQuadrant()} (важно: ${task.important}, срочно: ${task.urgent})`);
    }
    console.log(`   Создана: ${task.createdAt}`);
}

async function main() {
    const matrix = new EisenhowerMatrix();
    await matrix.loadFromFile();

    while (true) {
        console.log('\n===== МАТРИЦА ЭЙЗЕНХАУЭРА (JavaScript) =====');
        console.log('1. Показать все задачи');
        console.log('2. Добавить задачу');
        console.log('3. Редактировать задачу');
        console.log('4. Отметить выполненной / невыполненной');
        console.log('5. Удалить задачу');
        console.log('6. Показать статистику');
        console.log('7. Сохранить в файл');
        console.log('8. Загрузить из файла');
        console.log('0. Выход');
        const choice = await question('Выберите действие: ');

        if (choice === '0') break;

        switch (choice) {
            case '1': {
                if (matrix.tasks.length === 0) {
                    console.log('Задач нет.');
                } else {
                    const quadrants = ['Важно и срочно', 'Важно, но не срочно', 'Не важно, но срочно', 'Не важно и не срочно'];
                    for (const q of quadrants) {
                        const tasks = matrix.filterByQuadrant(q);
                        if (tasks.length) {
                            console.log(`\n--- ${q} ---`);
                            tasks.forEach(t => printTask(t, false));
                        }
                    }
                }
                break;
            }
            case '2': {
                const title = await question('Введите название: ');
                if (!title.trim()) {
                    console.log('Название не может быть пустым.');
                    continue;
                }
                const desc = await question('Введите описание: ');
                const important = (await question('Задача важна? (y/n): ')).toLowerCase() === 'y';
                const urgent = (await question('Задача срочна? (y/n): ')).toLowerCase() === 'y';
                const task = matrix.addTask(title, desc, important, urgent);
                console.log(`Задача добавлена с ID ${task.id}, квадрант: ${task.getQuadrant()}`);
                break;
            }
            case '3': {
                const id = parseInt(await question('Введите ID задачи: '));
                const task = matrix.findTask(id);
                if (!task) {
                    console.log('Задача не найдена.');
                    continue;
                }
                console.log('Оставьте поле пустым, чтобы не менять.');
                const newTitle = await question(`Новое название (${task.title}): `);
                const newDesc = await question(`Новое описание (${task.description}): `);
                const impInput = await question(`Важна? (y/n) сейчас: ${task.important ? 'y' : 'n'}: `);
                const urgInput = await question(`Срочна? (y/n) сейчас: ${task.urgent ? 'y' : 'n'}: `);
                const important = impInput ? impInput.toLowerCase() === 'y' : undefined;
                const urgent = urgInput ? urgInput.toLowerCase() === 'y' : undefined;
                if (matrix.editTask(id,
                    newTitle || undefined,
                    newDesc || undefined,
                    important,
                    urgent)) {
                    console.log('Задача обновлена.');
                } else {
                    console.log('Ошибка.');
                }
                break;
            }
            case '4': {
                const id = parseInt(await question('Введите ID задачи: '));
                if (matrix.toggleDone(id)) {
                    console.log('Статус выполнения изменён.');
                } else {
                    console.log('Задача не найдена.');
                }
                break;
            }
            case '5': {
                const id = parseInt(await question('Введите ID задачи для удаления: '));
                if (matrix.deleteTask(id)) {
                    console.log('Задача удалена.');
                } else {
                    console.log('Задача не найдена.');
                }
                break;
            }
            case '6': {
                const stats = matrix.getStats();
                console.log('\n=== СТАТИСТИКА ===');
                console.log(`Всего задач: ${stats.total}`);
                console.log(`Выполнено: ${stats.done}`);
                console.log(`Осталось: ${stats.pending}`);
                for (const [q, count] of Object.entries(stats.quadrants)) {
                    console.log(`  ${q}: ${count}`);
                }
                break;
            }
            case '7':
                try {
                    await matrix.saveToFile();
                    console.log('Сохранено.');
                } catch (err) {
                    console.log('Ошибка сохранения:', err.message);
                }
                break;
            case '8':
                try {
                    await matrix.loadFromFile();
                    console.log('Загружено.');
                } catch (err) {
                    console.log('Ошибка загрузки:', err.message);
                }
                break;
            default:
                console.log('Неизвестная команда.');
        }
    }
    rl.close();
}

main().catch(console.error);
