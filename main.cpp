#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QVector>

#include "truncate.hpp"

#define END_TIME_DEFAULT "\1"

using vec = QVector<DataRecord>;

struct SortBy {

#define OPERATOR(floor) askOrDesk ? [](const DataRecord &dr1, const DataRecord &dr2) { return dr1.floor > dr2.floor; } : [](const DataRecord &dr1, const DataRecord &dr2) { return dr1.floor <= dr2.floor; }
    enum Reg : bool {
        ASK,
        DESK
    };
    bool askOrDesk = ASK;
    uint64_t numb  = -1;

    void operator()(vec &v) const {
        switch (numb) {
            case 0: {
                std::sort(v.begin(), v.end(), OPERATOR(id));
            } break;
            case 1: {
                std::sort(v.begin(), v.end(), OPERATOR(getBegDate()));
            } break;
            case 2: {
                std::sort(v.begin(), v.end(), OPERATOR(getEndDate()));
            } break;
            case 3: {
                std::sort(v.begin(), v.end(), OPERATOR(isCrossed));
            } break;
            case 4: {
                std::sort(v.begin(), v.end(), OPERATOR(degreeOfPriority));
            } break;
            case 5: {
                std::sort(v.begin(), v.end(), OPERATOR(text.size()));
            } break;
        }
    }
};

SortBy getSortBy(const QStringList &commandArgs) {
    SortBy result;
    result.askOrDesk = SortBy::Reg::ASK;
    result.numb      = -1;

    if (commandArgs.size() == 1) {

        // Регулярное выражение для проверки формата
        QRegularExpression re("^(ask|desk)@(ID|Beg.Time|End.Time|Crossed|Priority|Text)$");
        QRegularExpressionMatch match = re.match(commandArgs[0]);

        if (match.hasMatch()) {

            const QVector<QString> vs = {"ID", "Beg.Time", "End.Time", "Crossed", "Priority", "Text"};
            result.askOrDesk          = match.captured(1) == "ask" ? SortBy::Reg::ASK : SortBy::Reg::DESK;  // Первое слово (ask/desk)
            result.numb               = vs.indexOf(match.captured(2));                                      // Второе слово
        } else {
            qCritical() << "Неверный формат строки. Ожидается ask|desk@ID|Beg.Time|End.Time|Crossed|Priority|Text";
            exit(1);
        }
    }
    return result;
}

// Класс для работы с записями
class RecordManager {
  private:
    QString fileName;             // Имя файла для хранения записей
    QString separatorRecord;      // Разделитель записей
    QString separatorLine;        // Разделитель строк внутри записи
    QString colorCross1;          // Цвет зачеркнутой записи (основной)
    QString colorCross2;          // Цвет зачеркнутой записи (дополнительный)
    QString colorNoCross1;        // Цвет незачеркнутой записи (основной)
    QString colorNoCross2;        // Цвет незачеркнутой записи (дополнительный)
    vec records;                  // Вектор для хранения записей в памяти

    // Генерация текущей даты в формате "дни:часы:годы"
    QString currentDateTime() const {
        QDateTime now = QDateTime::currentDateTime();
        return now.toString("dd:MM:yyyy");
    }

    // Загрузка записей из файла
    void loadRecords() {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;  // Файл не существует или не может быть открыт
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        QStringList recordStrings = content.split(separatorRecord, Qt::SkipEmptyParts);
        for (const QString &recordStr : recordStrings) {
            QStringList lines = recordStr.split(separatorLine);
            if (lines.size() != 6) continue;  // Неправильный формат записи

            DataRecord record;
            record.id               = lines[0].toULongLong();
            record.text             = lines[1];
            record.begTime          = lines[2];
            record.endTime          = lines[3];
            record.isCrossed        = lines[4].toInt();
            record.degreeOfPriority = lines[5].toULongLong();

            records.append(record);
        }
    }

    // Сохранение записей в файл
    void saveRecords() const {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning("Не удалось открыть файл для записи");
            return;
        }

        QTextStream out(&file);
        for (const DataRecord &record : records) {
            out << record.id << separatorLine
                << record.text << separatorLine
                << record.begTime << separatorLine
                << record.endTime << separatorLine
                << (record.isCrossed ? "1" : "0") << separatorLine
                << record.degreeOfPriority << separatorRecord;
        }
        file.close();
    }

    // Поиск записи по ID
    int findRecordById(size_t id) const {
        for (int i = 0; i < records.size(); ++i) {
            if (records[i].id == id) {
                return i;
            }
        }
        return -1;  // Не найдено
    }

    // Генерация нового уникального ID
    size_t generateNewId() const {
        size_t maxId = 0;
        for (const DataRecord &record : records) {
            if (record.id > maxId) {
                maxId = record.id;
            }
        }
        return maxId + 1;
    }

  public:
    // Конструктор с загрузкой настроек
    RecordManager(const QJsonObject &config) {
        separatorRecord = config.value("separatorRecord").toString("\033\033\033\033");
        separatorLine   = config.value("separatorLine").toString("\033\033");
        fileName        = config.value("fileName").toString("~/.records.records");
        colorCross1     = config.value("colorCross1").toString("1;31");
        colorCross2     = config.value("colorCross2").toString("31");
        colorNoCross1   = config.value("colorNoCross1").toString("1;37");
        colorNoCross2   = config.value("colorNoCross2").toString("37");

        loadRecords();
    }

    // Зачеркнуть/убрать зачеркивание записи
    void crossRecord(size_t id) {
        int index = findRecordById(id);
        if (index == -1) {
            qWarning("Запись с ID %zu не найдена", id);
            return;
        }

        DataRecord &record = records[index];
        record.isCrossed   = !record.isCrossed;
        record.endTime     = record.isCrossed ? currentDateTime() : END_TIME_DEFAULT;

        saveRecords();
    }

    // Удалить запись
    void deleteRecord(size_t id) {
        int index = findRecordById(id);
        if (index == -1) {
            qWarning("Запись с ID %zu не найдена", id);
            return;
        }

        records.remove(index);
        saveRecords();
    }

    // Создать новую запись
    void createRecord(uint64_t degree, const QString &text) {
        if (text.trimmed().isEmpty()) {
            qWarning("Текст записи не может быть пустым");
            return;
        }

        DataRecord newRecord;
        newRecord.id               = generateNewId();
        newRecord.text             = text;
        newRecord.begTime          = currentDateTime();
        newRecord.endTime          = END_TIME_DEFAULT;
        newRecord.isCrossed        = false;
        newRecord.degreeOfPriority = degree;

        records.append(newRecord);
        saveRecords();
    }

    // Вывод записей с фильтрацией по зачеркиванию
    void listRecords(bool showCrossed, bool showNoCrossed, const SortBy sort) const {
        auto records = this->records;

        if (records.isEmpty()) {
            QTextStream(stdout) << "No records to display.\n";
            return;
        }

        sort(records);

        int terminalWidth = 0, terminalHeight = 0;
        getTerminalSize(terminalWidth, terminalHeight);

        int idWidth = 0, textWidth = 0, begTimeWidth = 0, endTimeWidth = 0, priorityWidth = 0;
        calculateColumnWidths(records, idWidth, textWidth, begTimeWidth, endTimeWidth, priorityWidth, terminalWidth, showCrossed, showNoCrossed);

        // Проверяем, помещаются ли колонки в терминал
        int totalWidth = idWidth + textWidth + begTimeWidth + endTimeWidth + priorityWidth;
        if (totalWidth > terminalWidth) {
            // Если не помещаются - уменьшаем самую широкую колонку (обычно text)
            int overflow = totalWidth - terminalWidth;
            textWidth    = std::max(10, textWidth - overflow);  // Минимальная ширина 10
        }

        // Формируем строку заголовка
        QString header = QString("%1%2%3%4%5")
                             .arg(QString("ID").leftJustified(idWidth),
                                  QString("Beg.Time").leftJustified(begTimeWidth),
                                  QString("End.Time").leftJustified(endTimeWidth),
                                  QString("Priority").leftJustified(priorityWidth),
                                  QString("Text").leftJustified(textWidth));

        QTextStream(stdout) << header << "\n";

        // Выводим разделитель
        QTextStream(stdout) << QString().fill('-', header.length()) << "\n";

        // Выводим записи
        for (const auto &record : records) {
            if ((record.isCrossed && !showCrossed) || (!record.isCrossed && !showNoCrossed)) {
                continue;
            }

            const QString &color1 = record.isCrossed ? colorCross1 : colorNoCross1;
            const QString &color2 = record.isCrossed ? colorCross2 : colorNoCross2;

            printf("%s\033[%sm%s%s%s%s\033[%sm%s\033[0m\n",
                   record.isCrossed ? "\033[9m" : "",
                   color1.toStdString().c_str(),
                   QString::number(record.id).leftJustified(idWidth).toStdString().c_str(),
                   record.begTime.leftJustified(begTimeWidth).toStdString().c_str(),
                   record.endTime.leftJustified(endTimeWidth + 1 - (int)record.isCrossed).toStdString().c_str(),
                   QString::number(record.degreeOfPriority).leftJustified(priorityWidth).toStdString().c_str(),
                   color2.toStdString().c_str(),
                   truncateString(record.text, textWidth).leftJustified(textWidth).toStdString().c_str());

            // QTextStream(stdout) << line << "\n";
        }
    }
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Парсинг аргументов командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Менеджер записей с поддержкой кириллицы");
    parser.addHelpOption();
    parser.addPositionalArgument("config", "JSON файл конфигурации (опционально)");
    parser.addPositionalArgument("command", "Команда (create, cross, delete, list, list_nocross, list_cross)");
    parser.addPositionalArgument("args", "Аргументы команды");

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        parser.showHelp(1);
    }

    // Загрузка конфигурации
    QJsonObject config;
    int commandIndex = 0;

    // Если первый аргумент - JSON файл
    if (args[0].endsWith(".json")) {
        QFile configFile(args[0]);
        if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
            config            = doc.object();
        }
        commandIndex = 1;
    } else {
        // Настройки по умолчанию
        config = QJsonObject({{"separatorRecord", "\033\033\033\033"},
                              {"separatorLine", "\033\033"},
                              {"fileName", ".records.records"},
                              {"colorCross1", "1;31"},
                              {"colorCross2", "31"},
                              {"colorNoCross1", "1;37"},
                              {"colorNoCross2", "37"}});
    }

    if (args.size() <= commandIndex) {
        parser.showHelp(1);
    }

    RecordManager manager(config);
    QString command         = args[commandIndex];
    QStringList commandArgs = args.mid(commandIndex + 1);

    // Обработка команд
    if (command == "cross" && commandArgs.size() == 1) {
        manager.crossRecord(commandArgs[0].toULongLong());
    } else if (command == "delete" && commandArgs.size() == 1) {
        manager.deleteRecord(commandArgs[0].toULongLong());
    } else if (command == "create" && commandArgs.size() >= 2) {
        uint64_t degree = commandArgs[0].toULongLong();
        QString text    = commandArgs.mid(1).join(" ");
        manager.createRecord(degree, text);
    } else if (command == "list") {
        manager.listRecords(true, true, getSortBy(commandArgs));
    } else if (command == "list_nocross") {
        manager.listRecords(false, true, getSortBy(commandArgs));
    } else if (command == "list_cross") {
        manager.listRecords(true, false, getSortBy(commandArgs));
    } else {
        qWarning("Неизвестная команда или неправильные аргументы");
        parser.showHelp(1);
    }

    return 0;
}
