#include <iostream>
#include <iomanip>
#include <set>
#include <fstream>
#include <sstream>
#include <memory>
#include <windows.h>
#include <vector>

using namespace std;

void setConsoleColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void showMenu() {
    cout << "+----------------------------------+" << endl;
    cout << "|               МЕНЮ               |" << endl;
    cout << "+----------------------------------+" << endl;
    cout << "|  1  | Додати повідомлення        |" << endl;
    cout << "|  2  | Показати всі повідомлення  |" << endl;
    cout << "|  3  | Зберегти переписку         |" << endl;
    cout << "|  4  | Завантажити переписку      |" << endl;
    cout << "|  5  | Редагувати повідомлення    |" << endl;
    cout << "|  6  | Очистити переписку         |" << endl;
    cout << "|  7  | Пошук повідомлення         |" << endl;
    cout << "|  8  | Видалити повідомлення      |" << endl;
    cout << "|  9  | Статистика чату            |" << endl;
    cout << "|  0  | Вихід                      |" << endl;
    cout << "+----------------------------------+" << endl;
}

class Message {
protected:
    static int global_id_counter;
    int id;
    string text;

public:
    Message(const string& txt)
        : text(txt), id(++global_id_counter) {}

    Message(const string& txt, int forcedId)
        : text(txt), id(forcedId)
    {
        if (forcedId > global_id_counter) {
            global_id_counter = forcedId;
        }
    }

    virtual ~Message() {}

    void setId(int newId) { id = newId; }
    int getId() const { return id; }

    static int getGlobalCounter() { return global_id_counter; }
    static void setGlobalCounter(int value) { global_id_counter = value; }

    virtual string getText() const {
        return text;
    }

    virtual string getType() const {
        return "Просте";
    }

    virtual void display() const {
        cout << "ID: " << id << " - " << getText() << endl;
    }

    bool operator<(const Message& other) const {
        return id < other.id;
    }
};

int Message::global_id_counter = 0;

class SimpleMessage : public Message {
public:
    SimpleMessage(const string& txt)
        : Message(txt) {}

    SimpleMessage(const string& txt, int forcedId)
        : Message(txt, forcedId) {}

    void display() const override {
        setConsoleColor(8); 
        cout << "ID: " << getId() << " - ";
        string rawText = this->getText();
        applyFormatting(rawText);
        setConsoleColor(8);
    }

protected:
    void applyFormatting(const string& text) const {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        WORD defaultColor = csbi.wAttributes;

        bool bold = false, italic = false;

        for (size_t i = 0; i < text.length(); i++) {
            if (text[i] == '*' && (i == 0 || text[i - 1] != '\\')) {
                bold = !bold;
                setConsoleColor(bold ? 0 : defaultColor);
                continue;
            }
            if (text[i] == '_' && (i == 0 || text[i - 1] != '\\')) {
                italic = !italic;
                cout << (italic ? "\033[3m" : "\033[0m");
                if (!italic && bold) setConsoleColor(0);
                else if (!italic) setConsoleColor(defaultColor);
                continue;
            }
            cout << text[i];
        }

        setConsoleColor(defaultColor);
        cout << "\033[0m" << endl;
    }
};

class MessageDecorator : public Message {
protected:
    shared_ptr<Message> wrappedMessage;

public:
    MessageDecorator(shared_ptr<Message> msg)
        : Message(msg->getText(), msg->getId()), wrappedMessage(msg) {}

    string getText() const override {
        return wrappedMessage->getText();
    }

    virtual void display() const override {
        wrappedMessage->display();
    }
};


class BoldMessageDecorator : public MessageDecorator {
public:
    BoldMessageDecorator(shared_ptr<Message> msg) : MessageDecorator(msg) {}

    string getType() const override {
        return "Bold";
    }

    void display() const override {
        setConsoleColor(0xF0);
        cout << "ID: " << getId() << " - " << getText() << endl;
        setConsoleColor(8);

    }
};

class ItalicMessageDecorator : public MessageDecorator {
public:
    ItalicMessageDecorator(shared_ptr<Message> msg) : MessageDecorator(msg) {}

    string getType() const override {
        return "Italic";
    }

    void display() const override {
        setConsoleColor(8);
        cout << "ID: " << getId() << " - " << "\033[3m" << getText() << "\033[0m" << endl;
        setConsoleColor(8);

    }
};

struct MessageComparator {
    bool operator()(const shared_ptr<Message>& lhs, const shared_ptr<Message>& rhs) const {
        return lhs->getId() < rhs->getId(); 
    }
};

////////////////////////////////////
class MessageStorage {
private:
    set<shared_ptr<Message>, MessageComparator> messages;
    string filename = "messages.txt";

public:
    const set<shared_ptr<Message>, MessageComparator>& getMessages() const {
        return messages;
    }


    void addMessage(shared_ptr<Message> msg) {
        for (const auto& m : messages) {
            if (m->getId() == msg->getId()) {
                cout << "|   Повідомлення з таким ID вже є  |" << endl;
                cout << "+----------------------------------+" << endl;
                return;
            }
        }
        messages.insert(msg);

        // Після додавання оновлюємо глобальний лічильник
        if (msg->getId() > Message::getGlobalCounter()) {
            Message::setGlobalCounter(msg->getId());
        }
    }


    void displayMessages() const {
        if (messages.empty()) {

            cout << "|          Чат порожній.           |" << endl;
            cout << "|      Додайте повідомлення!       |" << endl;
            cout << "+----------------------------------+" << endl;
            return;
        }
        cout << "|           Історія чату           |" << endl;
        cout << "+----------------------------------+" << endl;
        for (const auto& msg : messages) {
            msg->display();
            cout << "+----------------------------------+" << endl;
        }
    }

    bool editMessageById(int idToEdit, const string& newText) {
        for (auto& msg : messages) {
            if (msg->getId() == idToEdit) {
                shared_ptr<Message> editedMsg = make_shared<SimpleMessage>(newText, idToEdit);
                messages.erase(msg);
                messages.insert(editedMsg);

                if (idToEdit > Message::getGlobalCounter()) {
                    Message::setGlobalCounter(idToEdit);
                }

                return true;
            }
        }
        return false;
    }

    void deleteMessageById(int idToDelete) {
        bool found = false;
        for (auto it = messages.begin(); it != messages.end(); ++it) {
            if ((*it)->getId() == idToDelete) {
                messages.erase(it);
                found = true;
                system("cls");
                showMenu();
                cout << "|   Повідомлення видалено успішно  |" << endl;
                cout << "+----------------------------------+" << endl;
                return;
            }
        }
        if (!found) {
            system("cls");
            showMenu();
            cout << "|   Повідомлення з таким ID нема   |" << endl;
            cout << "+----------------------------------+" << endl;
        }
    }


    void showStatistics() const {
        int totalMessages = messages.size();
        int totalWords = 0, totalChars = 0;
        int boldWords = 0, italicWords = 0;

        for (const auto& msg : messages) {
            string txt = msg->getText();
            totalChars += txt.length();

            // Рахуємо слова
            stringstream ss(txt);
            string word;
            while (ss >> word) {
                totalWords++;
            }

            // Рахуємо * і _ по всьому тексту
            int boldCount = count(txt.begin(), txt.end(), '*');
            int italicCount = count(txt.begin(), txt.end(), '_');

            boldWords += boldCount / 2;
            italicWords += italicCount / 2;
        }



        cout << "|        Статистика чату           |" << endl;
        cout << "+----------------------------------+" << endl;
        cout << "| Всього повідомлень          " << setw(4) << totalMessages << " |" << endl;
        cout << "| Загальна кількість слів     " << setw(4) << totalWords << " |" << endl;
        cout << "| Фрагментів жирного          " << setw(4) << boldWords << " |" << endl;
        cout << "| Фрагментів курсиву          " << setw(4) << italicWords << " |" << endl;
        cout << "| Загальна кількість символів " << setw(4) << totalChars << " |" << endl;
        cout << "+----------------------------------+" << endl;
    }



    void saveToFile() {
        ofstream file(filename);
        for (const auto& msg : messages) {
            string text = msg->getText();
            size_t pos = 0;
            while ((pos = text.find('\n', pos)) != string::npos) {
                text.replace(pos, 1, "\\n");
                pos += 2;
            }
            file << "ID: " << msg->getId() << "|" << text << endl;
        }
        system("cls");
        showMenu();
        cout << "|        Переписка збережена!      |" << endl;
        cout << "+----------------------------------+" << endl;
    }

    void loadFromFile() {
        messages.clear();
        ifstream file(filename);

        if (!file.is_open()) {
            system("cls");
            showMenu();
            cout << "|         Файл не знайдено!        |" << endl;
            cout << "+----------------------------------+" << endl;
            return;
        }

        string line;
        int loadedCount = 0;

        while (getline(file, line)) {
            size_t delim = line.find('|');
            if (delim != string::npos) {
                string idPart = line.substr(0, delim);
                string text = line.substr(delim + 1);
                size_t colon = idPart.find(':');

                if (colon != string::npos) {
                    string idStr = idPart.substr(colon + 1);
                    try {
                        int id = stoi(idStr);

                        // Зворотня заміна \\n на реальні переноси
                        size_t pos = 0;
                        while ((pos = text.find("\\n", pos)) != string::npos) {
                            text.replace(pos, 2, "\n");
                            pos += 1;
                        }

                        shared_ptr<Message> msg = make_shared<SimpleMessage>(text, id);
                        messages.insert(msg);
                        loadedCount++;
                    }
                    catch (...) {
                        cout << "Пропущено некоректний рядок: " << line << endl;
                    }
                }
            }
        }

        if (loadedCount == 0) {
            system("cls");
            showMenu();
            cout << "|     Жодне повідомлення не було   |" << endl;
            cout << "|            завантажено           |" << endl;
            cout << "+----------------------------------+" << endl;
        }
        else {
            system("cls");
            showMenu();
            cout << "|      Переписка завантажена!      |" << endl;
            cout << "+----------------------------------+" << endl;
        }
    }


    void searchMessages(const string& keyword) const {
        vector<shared_ptr<Message>> results;

        for (const auto& msg : messages) {
            if (msg->getText().find(keyword) != string::npos) {
                results.push_back(msg);
            }
        }

        if (!results.empty()) {
            system("cls");
            showMenu();
            cout << "|        Результати пошуку         |" << endl;
            cout << "+----------------------------------+" << endl;
            for (const auto& msg : results) {
                msg->display();
            }
        }
        else {
            system("cls");
            showMenu();
            cout << "|     Повідомлення не знайдено     |" << endl;
            cout << "+----------------------------------+" << endl;
        }
    }

    void clearMessages() {
        char confirm;
        system("cls");
        showMenu();
        cout << "|      Ви впевнені, що хочете      |" << endl;
        cout << "|    видалити всі повідомлення?    |" << endl;
        cout << "+----------------------------------+" << endl;
        cout << "(Y/N): ";
        cin >> confirm;
        cin.ignore(); // Очищення буфера

        if (confirm == 'y' || confirm == 'Y') {
            messages.clear();
            system("cls");
            showMenu();
            cout << "|         Переписка очищена        |" << endl;
            cout << "+----------------------------------+" << endl;
        }
        else {
            system("cls");
            showMenu();
            cout << "|        Видалення скасовано       |" << endl;
            cout << "+----------------------------------+" << endl;
        }
    }

};



////////////////////////////////////

bool isCancelled(const string& input) {
    return input == "/cancel";
}

void addMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();
    cout << "|            Підказка:             |" << endl;
    cout << "+----------------------------------+" << endl;
    cout << "|  Щоб зробити жирний або курсив   |" << endl;
    cout << "|   скористуйтеся форматуванням    |" << endl;
    cout << "|       *Жирний* _Курсив_          |" << endl;
    cout << "| Щоб завершити введення, впишіть  |" << endl;
    cout << "|       /0 на новому рядку         |" << endl;
    cout << "| АБО /cancel — щоб вийти без змін |" << endl;
    cout << "+----------------------------------+" << endl;


    cout << "Введіть текст повідомлення: ";

    string line, text;
    while (true) {
        getline(cin, line);
        if (isCancelled(line)) {
            system("cls");
            showMenu();
            cout << "|         Дію скасовано!           |" << endl;
            cout << "+----------------------------------+" << endl;
            return;
        }

        if (line == "/0") break;

        text += line + "\n";
    }


    if (text.empty()) {
        cout << "Повідомлення не може бути порожнім." << endl;
        return;
    }

    if (text.find('|') != string::npos) {
        cout << "Символ '|' заборонено!" << endl;
        return;
    }

    shared_ptr<Message> msg = make_shared<SimpleMessage>(text);

    int stars = count(text.begin(), text.end(), '*');
    int underscores = count(text.begin(), text.end(), '_');
    if (stars % 2 != 0) {
        system("cls");
        showMenu();
        cout << "|             Увага!               |" << endl;
        cout << "|       непарна кількість *        |" << endl;
        cout << "|      форматування може бути      |" << endl;
        cout << "|           некоректним!           |" << endl;
        cout << "+----------------------------------+" << endl;
    }
    if (underscores % 2 != 0) {
        system("cls");
        showMenu();
        cout << "|             Увага!               |" << endl;
        cout << "|       непарна кількість _        |" << endl;
        cout << "|      форматування може бути      |" << endl;
        cout << "|           некоректним!           |" << endl;
        cout << "+----------------------------------+" << endl;
    }

    storage.addMessage(msg);
    system("cls");
    showMenu();
    cout << "|       Повідомлення додано!       |" << endl;
    cout << "+----------------------------------+" << endl;
}

void editMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();

    if (storage.getMessages().empty()) {
        cout << "|         Чат порожній!            |" << endl;
        cout << "|  Додайте спочатку повідомлення   |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    // Підказка
    cout << "|             Підказка:            |" << endl;
    cout << "+----------------------------------+" << endl;
    cout << "|  Щоб зробити жирний або курсив   |" << endl;
    cout << "|   скористуйтеся форматуванням    |" << endl;
    cout << "|       *Жирний* _Курсив_          |" << endl;
    cout << "| Щоб завершити введення, впишіть  |" << endl;
    cout << "|       /0 на новому рядку         |" << endl;
    cout << "| АБО /cancel — щоб вийти без змін |" << endl;
    cout << "+----------------------------------+" << endl;

    // Введення ID
    string inputId;
    cout << "Введіть ID повідомлення для редагування: ";
    getline(cin, inputId);
    if (inputId == "/cancel") {
        system("cls");
        showMenu();
        cout << "|    Дію скасовано користувачем    |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    int id;
    try {
        id = stoi(inputId);
    }
    catch (...) {
        system("cls");
        showMenu();
        cout << "|         Некоректний ввід!        |" << endl;
        cout << "|       Введіть ціле число ID      |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    // Пошук повідомлення з таким ID
    shared_ptr<Message> originalMsg = nullptr;
    for (const auto& msg : storage.getMessages()) {
        if (msg->getId() == id) {
            originalMsg = msg;
            break;
        }
    }

    if (!originalMsg) {
        system("cls");
        showMenu();
        cout << "|  Повідомлення з таким ID нема!  |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    // Введення нового тексту
    string line, newText;
    cout << "Введіть новий текст повідомлення:";
    while (true) {
        getline(cin, line);
        if (line == "/cancel") {
            system("cls");
            showMenu();
            cout << "|       Редагування скасовано!     |" << endl;
            cout << "+----------------------------------+" << endl;
            return;
        }
        if (line == "/0") break;
        newText += line + "\n";
    }

    // Перевірки
    if (newText.empty()) {
        system("cls");
        showMenu();
        cout << "|       Повідомлення не може       |" << endl;
        cout << "|          бути порожнім!          |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    if (newText.find('|') != string::npos) {
        system("cls");
        showMenu();
        cout << "|       Символ '|' заборонено!     |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    // Створення нового повідомлення й оновлення
    shared_ptr<Message> editedMsg = make_shared<SimpleMessage>(newText, id);
    storage.deleteMessageById(id);
    storage.addMessage(editedMsg);

    system("cls");
    showMenu();
    cout << "|    Повідомлення відредаговано!   |" << endl;
    cout << "+----------------------------------+" << endl;
}


void searchMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();

    // Підказка
    cout << "|             Підказка:            |" << endl;
    cout << "+----------------------------------+" << endl;
    cout << "|     Введіть слово, для пошуку    |" << endl;
    cout << "|     /cancel — вихід без змін     |" << endl;
    cout << "+----------------------------------+" << endl;

    string keyword;
    cout << "Введіть слово для пошуку: ";
    getline(cin, keyword);

    if (keyword == "/cancel") {
        system("cls");
        showMenu();
        cout << "|         Пошук скасовано!         |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    if (keyword.empty()) {
        system("cls");
        showMenu();
        cout << "|  Слово для пошуку не може бути   |" << endl;
        cout << "|            порожнім!             |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    system("cls");
    showMenu();
    storage.searchMessages(keyword);
}


void deleteMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();

    // 🔍 Перевірка: якщо чат порожній
    if (storage.getMessages().empty()) {
        cout << "|         Чат порожній!            |" << endl;
        cout << "|  Додайте повідомлення спочатку!  |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    // Підказка
    cout << "|             Підказка:            |" << endl;
    cout << "+----------------------------------+" << endl;
    cout << "|  Введіть ID повідомлення, яке    |" << endl;
    cout << "|        бажаєте видалити          |" << endl;
    cout << "| Введіть /cancel — вихід без змін |" << endl;
    cout << "+----------------------------------+" << endl;

    string inputId;
    cout << "Введіть ID повідомлення для видалення: ";
    getline(cin, inputId);

    if (inputId == "/cancel") {
        system("cls");
        showMenu();
        cout << "|       Видалення скасовано        |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    int id;
    try {
        id = stoi(inputId);
        if (id <= 0 || id > Message::getGlobalCounter()) {
            throw out_of_range("ID поза межами");
        }
    }
    catch (...) {
        int minId = 1;
        int maxId = Message::getGlobalCounter();
        system("cls");
        showMenu();
        cout << "|          Некоректний ID!         |" << endl;
        cout << "|      Введіть ID від " << minId << " до " << maxId << "       |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    // Перевірка: чи існує повідомлення з таким ID
    bool exists = false;
    for (const auto& msg : storage.getMessages()) {
        if (msg->getId() == id) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        int minId = 1;
        int maxId = Message::getGlobalCounter();
        system("cls");
        showMenu();
        cout << "|  Повідомлення з таким ID нема!  |" << endl;
        cout << "|      Введіть ID від " << minId << " до " << maxId << "       |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    // Підтвердження
    string confirm;
    system("cls");
    showMenu();
    cout << "|      Ви впевнені, що хочете      |" << endl;
    cout << "|  видалити повідомлення з ID: " << id << "?  |" << endl;
    cout << "|    (Y — так ; N — ні ; /cancel)  |" << endl;
    cout << "+----------------------------------+" << endl;
    cout << "Ваш вибір: ";
    getline(cin, confirm);

    if (confirm == "/cancel" || confirm == "n" || confirm == "N") {
        system("cls");
        showMenu();
        cout << "|       Видалення скасовано        |" << endl;
        cout << "+----------------------------------+" << endl;
        return;
    }

    if (confirm == "y" || confirm == "Y") {
        storage.deleteMessageById(id);
    }
    else {
        system("cls");
        showMenu();
        cout << "|       Некоректне підтвердження   |" << endl;
        cout << "+----------------------------------+" << endl;
    }
}


bool exitFlow(MessageStorage& storage) {

    string saveInput;
    cout << "Бажаєте зберегти перед виходом? (Y/N): ";
    getline(cin, saveInput);
    system("cls");
    showMenu();

    if (saveInput == "Y" || saveInput == "y") {
        storage.saveToFile();
    }
    else if (saveInput != "N" && saveInput != "n") {
        cout << "Некоректний вибір. Введіть Y або N " << endl;
        return false;
    }

    cout << "Вихід..." << endl;
    return true;
}

void refreshMenu() {
    system("cls");
    showMenu();
}




int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    setConsoleColor(8);

    MessageStorage storage;

    showMenu();
    while (true) {
        string input;
        int choice = -1; 

        cout << "Виберіть дію: ";
        getline(cin, input);

        try {
            choice = stoi(input);
        }
        catch (...) {
            system("cls");
            showMenu();
            cout << "|         Некоректний вибір        |" << endl;
            cout << "|     Введіть число від 0 до 9     |" << endl;
            cout << "+----------------------------------+" << endl;
            continue;
        }

        if (choice < 0 || choice > 9) {
            system("cls");
            showMenu();
            cout << "| Введіть число в межах від 0 до 9 |" << endl;
            cout << "+----------------------------------+" << endl;
            continue;
        }

        switch (choice) {
        case 1:
            addMessageFlow(storage);
            break;
        case 2:
            refreshMenu();
            storage.displayMessages();
            break;
        case 3:
            refreshMenu();
            storage.saveToFile();
            break;
        case 4:
            refreshMenu();
            storage.loadFromFile();
            break;
        case 5: {editMessageFlow(storage); break;}
        case 6:
            refreshMenu();
            storage.clearMessages();
            break;
        case 7: {searchMessageFlow(storage); break;}
        case 8: {deleteMessageFlow(storage); break;}
        case 9:
            refreshMenu();
            storage.showStatistics();
            break;
        case 0: {if (exitFlow(storage)) return 0; break;}
        default:
            cout << "Некоректний вибір, спробуйте знову!" << endl;
        }
    }

    return 0;
}

