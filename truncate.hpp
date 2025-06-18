#ifndef TRUNCATE_CPP
#define TRUNCATE_CPP

#include <QDate>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <sys/ioctl.h>
#include <unistd.h>

// Функция преобразует строку с датой в формате "dd:MM:yyyy" в QDate
// Возвращает:
//   - валидный QDate, если дата корректна
//   - невалидный QDate (QDate()), если дата некорректна
QDate getTime(const QString &dateStr) {
    QStringList parts = dateStr.split(':');

    // Проверяем формат (должно быть 3 компонента)
    if (parts.size() != 3) {
        qWarning() << "Некорректный формат даты. Ожидается dd:MM:yyyy";
        return QDate();
    }

    // Парсим компоненты даты
    bool ok1, ok2, ok3;
    int day   = parts[0].toInt(&ok1);
    int month = parts[1].toInt(&ok2);
    int year  = parts[2].toInt(&ok3);

    // Проверяем, что все компоненты - числа
    if (!ok1 || !ok2 || !ok3) {
        qWarning() << "Дата содержит нечисловые значения";
        return QDate();
    }

    // Создаем и проверяем дату
    QDate date(year, month, day);
    if (!date.isValid()) {
        qWarning() << "Некорректная дата";
        return QDate();
    }

    return date;
}

// Структура для хранения одной записи
struct DataRecord {
    size_t id;                  // Уникальный идентификатор записи
    QString begTime;            // Время создания
    QString endTime;            // Время зачеркивания (может быть пустым)
    bool isCrossed;             // Флаг зачеркивания
    uint64_t degreeOfPriority;  // Степень приоритета
    QString text;               // Текст записи

    // Конструктор по умолчанию
    DataRecord() : id(0), isCrossed(false), degreeOfPriority(0) {}

    auto getBegDate() const {
        auto date = getTime(begTime);
        if (!date.isValid()) {
            qCritical() << "date error " << id << " " << begTime;
            exit(1);
        }
        return date;
    }
    auto getEndDate() const {
        auto date = getTime(endTime);
        if (!date.isValid()) {
            qCritical() << "date error " << id << " " << endTime;
            exit(1);
        }
        return date;
    }
};

// Получаем размер терминала
void getTerminalSize(int &cols, int &rows) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    cols = w.ws_col;
    rows = w.ws_row;
}

// Вычисляем максимальную ширину для каждого поля
void calculateColumnWidths(const QVector<DataRecord> &records,
                           int &idWidth,
                           int &textWidth,
                           int &begTimeWidth,
                           int &endTimeWidth,
                           int &priorityWidth,
                           const int cols,

                           bool showCrossed = true, bool showNoCrossed = true) {
    idWidth       = 3;  // Минимальная ширина для ID ("ID ")
    textWidth     = 5;  // "Text "
    begTimeWidth  = 8;  // "Beg.Time "
    endTimeWidth  = 8;  // "End.Time "
    priorityWidth = 9;  // "Priority "

    for (const auto &record : records) {
        if ((record.isCrossed && !showCrossed) || (!record.isCrossed && !showNoCrossed)) {
            continue;
        }

        idWidth       = std::max<int>(idWidth, static_cast<int>(QString::number(record.id).length()));
        textWidth     = std::max<int>(textWidth, record.text.length());
        begTimeWidth  = std::max<int>(begTimeWidth, record.begTime.length());
        endTimeWidth  = std::max<int>(endTimeWidth, record.endTime.length());
        priorityWidth = std::max<int>(priorityWidth, static_cast<int>(QString::number(record.degreeOfPriority).length()));
    }

    // Добавляем отступы
    idWidth += 2;
    begTimeWidth += 2;
    endTimeWidth += 2;
    priorityWidth += 2;

    textWidth += 2 + cols - (idWidth + textWidth + begTimeWidth + endTimeWidth + priorityWidth);
}

// Обрезаем строку, если она не помещается в колонку
QString truncateString(const QString &str, int maxWidth) {
    if (str.length() > maxWidth - 3) {
        return str.left(maxWidth - 3) + "...";
    }
    return str;
}


#endif  // TRUNCATE_CPP
