# matrix.rb
require 'json'
require 'date'

class Quadrant
  DO = "Важно и срочно"
  PLAN = "Важно, но не срочно"
  DELEGATE = "Не важно, но срочно"
  ELIMINATE = "Не важно и не срочно"
end

class Task
  attr_accessor :id, :title, :description, :important, :urgent, :done, :created_at

  def initialize(id, title, description, important, urgent, done = false, created_at = DateTime.now.iso8601)
    @id = id
    @title = title
    @description = description
    @important = important
    @urgent = urgent
    @done = done
    @created_at = created_at
  end

  def quadrant
    if @important && @urgent
      Quadrant::DO
    elsif @important && !@urgent
      Quadrant::PLAN
    elsif !@important && @urgent
      Quadrant::DELEGATE
    else
      Quadrant::ELIMINATE
    end
  end

  def to_h
    {
      id: @id,
      title: @title,
      description: @description,
      important: @important,
      urgent: @urgent,
      done: @done,
      created_at: @created_at
    }
  end

  def self.from_h(hash)
    Task.new(hash[:id], hash[:title], hash[:description], hash[:important], hash[:urgent], hash[:done], hash[:created_at])
  end
end

class EisenhowerMatrix
  attr_reader :tasks

  def initialize
    @tasks = []
    @next_id = 1
  end

  def add_task(title, description, important, urgent)
    task = Task.new(@next_id, title, description, important, urgent)
    @tasks << task
    @next_id += 1
    task
  end

  def find_task(id)
    @tasks.find { |t| t.id == id }
  end

  def edit_task(id, title: nil, description: nil, important: nil, urgent: nil)
    task = find_task(id)
    return false unless task
    task.title = title if title
    task.description = description if description
    task.important = important unless important.nil?
    task.urgent = urgent unless urgent.nil?
    true
  end

  def toggle_done(id)
    task = find_task(id)
    return false unless task
    task.done = !task.done
    true
  end

  def delete_task(id)
    task = find_task(id)
    return false unless task
    @tasks.delete(task)
    true
  end

  def filter_by_quadrant(quadrant)
    @tasks.select { |t| t.quadrant == quadrant }
  end

  def filter_by_done(done)
    @tasks.select { |t| t.done == done }
  end

  def stats
    total = @tasks.size
    done_count = @tasks.count(&:done)
    quad_counts = Hash.new(0)
    @tasks.each { |t| quad_counts[t.quadrant] += 1 }
    {
      total: total,
      done: done_count,
      pending: total - done_count,
      quadrants: quad_counts
    }
  end

  def save_to_file(filename = "eisenhower_data.json")
    File.write(filename, JSON.pretty_generate(@tasks.map(&:to_h)))
  end

  def load_from_file(filename = "eisenhower_data.json")
    return unless File.exist?(filename)
    data = JSON.parse(File.read(filename), symbolize_names: true)
    @tasks.clear
    data.each do |item|
      task = Task.from_h(item)
      @tasks << task
      @next_id = task.id + 1 if task.id >= @next_id
    end
  rescue JSON::ParserError
    puts "Ошибка чтения файла."
  end
end

def print_task(task, show_quadrant = true)
  done_mark = task.done ? "✅" : "⬜"
  puts "#{done_mark} ##{task.id} - #{task.title}"
  puts "   Описание: #{task.description}"
  if show_quadrant
    puts "   Квадрант: #{task.quadrant} (важно: #{task.important}, срочно: #{task.urgent})"
  end
  puts "   Создана: #{task.created_at}"
end

def main
  matrix = EisenhowerMatrix.new
  matrix.load_from_file

  loop do
    puts "\n===== МАТРИЦА ЭЙЗЕНХАУЭРА (Ruby) ====="
    puts "1. Показать все задачи"
    puts "2. Добавить задачу"
    puts "3. Редактировать задачу"
    puts "4. Отметить выполненной / невыполненной"
    puts "5. Удалить задачу"
    puts "6. Показать статистику"
    puts "7. Сохранить в файл"
    puts "8. Загрузить из файла"
    puts "0. Выход"
    print "Выберите действие: "
    choice = gets.chomp

    case choice
    when "0"
      break
    when "1"
      if matrix.tasks.empty?
        puts "Задач нет."
      else
        [Quadrant::DO, Quadrant::PLAN, Quadrant::DELEGATE, Quadrant::ELIMINATE].each do |q|
          tasks = matrix.filter_by_quadrant(q)
          if tasks.any?
            puts "\n--- #{q} ---"
            tasks.each { |t| print_task(t, false) }
          end
        end
      end
    when "2"
      print "Введите название: "
      title = gets.chomp
      next if title.empty?
      print "Введите описание: "
      desc = gets.chomp
      print "Задача важна? (y/n): "
      important = gets.chomp.downcase == 'y'
      print "Задача срочна? (y/n): "
      urgent = gets.chomp.downcase == 'y'
      task = matrix.add_task(title, desc, important, urgent)
      puts "Задача добавлена с ID #{task.id}, квадрант: #{task.quadrant}"
    when "3"
      print "Введите ID задачи: "
      id = gets.chomp.to_i
      task = matrix.find_task(id)
      unless task
        puts "Задача не найдена."
        next
      end
      puts "Оставьте поле пустым, чтобы не менять."
      print "Новое название (#{task.title}): "
      new_title = gets.chomp
      task.title = new_title unless new_title.empty?
      print "Новое описание (#{task.description}): "
      new_desc = gets.chomp
      task.description = new_desc unless new_desc.empty?
      print "Важна? (y/n) сейчас: #{task.important ? 'y' : 'n'}: "
      new_imp = gets.chomp
      task.important = (new_imp.downcase == 'y') unless new_imp.empty?
      print "Срочна? (y/n) сейчас: #{task.urgent ? 'y' : 'n'}: "
      new_urg = gets.chomp
      task.urgent = (new_urg.downcase == 'y') unless new_urg.empty?
      puts "Задача обновлена."
    when "4"
      print "Введите ID задачи: "
      id = gets.chomp.to_i
      if matrix.toggle_done(id)
        puts "Статус выполнения изменён."
      else
        puts "Задача не найдена."
      end
    when "5"
      print "Введите ID задачи для удаления: "
      id = gets.chomp.to_i
      if matrix.delete_task(id)
        puts "Задача удалена."
      else
        puts "Задача не найдена."
      end
    when "6"
      stats = matrix.stats
      puts "\n=== СТАТИСТИКА ==="
      puts "Всего задач: #{stats[:total]}"
      puts "Выполнено: #{stats[:done]}"
      puts "Осталось: #{stats[:pending]}"
      stats[:quadrants].each do |q, count|
        puts "  #{q}: #{count}"
      end
    when "7"
      matrix.save_to_file
      puts "Сохранено."
    when "8"
      matrix.load_from_file
      puts "Загружено."
    else
      puts "Неизвестная команда."
    end
  end
end

main if __FILE__ == $0
