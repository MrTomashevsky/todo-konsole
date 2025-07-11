# todo-konsole

> Программа todo-лист для написания задач и их редактирования в консоли.

## Оглавление
1. [Требования](#требования)
2. [Установка](#установка)
3. [Использование](#использование)
4. [Сборка](#сборка)

## Требования
- C++14
- CMake 3.16+
- Qt 5+

## Установка
### Linux (Ubuntu/Debian)
```bash
sudo apt install clang cmake qt5-base
mkdir build && cd build
cmake ..
make
```

## Использование
Программа хранит записи в виде структуры

```cpp
struct DataRecord {
    size_t id;
    QString text;
    QString begTime;
    QString endTime;
    bool isCrossed;
    uint64_t degreeOfPriority;
};
```

Где:
- id - автополе с уникальным id записи;
- text - поле с самой записью; не может быть пустым или содержать только пробельные символы;
- begTime - автополе с датой создания записи формата "дни: часы: годы";
- endTime - поле с датой зачеркивания записи. Может быть пустым;
- isCrossed - если запись зачеркнута - равна true. Если равна false - endTime сбрасывается, иначе - обновляется.
- degreeOfPriority - число, означающее степень приоритетности записи.

Все записи отделяются друг от друга строкой separatorRecord, все строки внутри записей - separatorLine.

Программа предоставляет следующие функции:
- ```cross <id>``` - зачеркнуть запись (если зачеркнута - убрать зачеркивание);
- ```delete <id>``` - удалить запись;
- ```create <degreeOfPriority> <text>``` - создать новую запись;
- ```list``` - вывод всех записей;
- ```list_nocross``` - вывод незачеркнутых записей;
- ```list_cross``` - вывод зачеркнутых записей.

Записи отрисовываются в формате таблицы, но без рамок. В случае, если запись не зачеркнута - она раскрашивается цветом colorNoCross1, иначе - colorCross1. Поле isCrossed, если запись не зачеркнута, окрашивается цветом colorNoCross2, иначе - colorCross2.

Все разукрашивание реализуется посредством escape-последовательностей.

В командной строке для программы можно задать имя json-файла настроек. Вид по умолчанию json-файла:

```json
{
  "separatorRecord": "\u001b\u001b\u001b\u001b",
  "separatorLine": "\u001b\u001b",
  "fileName": ".records.records",
  "colorCross1": "1;31",
  "colorCross2": "31",
  "colorNoCross1": "1;37",
  "colorNoCross2": "37"
}
```

Если json-файл не задан, то используются вышеуказанные настройки по умолчанию.

По итогу вид запуска программы из командной строки будет следующий:

```bash
./todo <имя json файла> <имя функции> <ее аргументы>
```

Если <имя json файла> не указано, то в директории с программой ищется .records.records, иначе он создается с нуля.


