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
    cout << "+---------------------------------+" << endl;
    cout << "|              МЕНЮ               |" << endl;
    cout << "+---------------------------------+" << endl;
    cout << "|  1  | Додати повідомлення       |" << endl;
    cout << "|  2  | Показати всі повідомлення |" << endl;
    cout << "|  3  | Зберегти переписку        |" << endl;
    cout << "|  4  | Завантажити переписку     |" << endl;
    cout << "|  5  | Редагувати повідомлення   |" << endl;
    cout << "|  6  | Очистити переписку        |" << endl;
    cout << "|  7  | Пошук повідомлення        |" << endl;
    cout << "|  8  | Видалити повідомлення     |" << endl;
    cout << "|  9  | Статистика чату           |" << endl;
    cout << "|  0  | Вихід                     |" << endl;
    cout << "+---------------------------------+" << endl;
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
    void addMessage(shared_ptr<Message> msg) {
        for (const auto& m : messages) {
            if (m->getId() == msg->getId()) {
                cout << "|  Повідомлення з таким ID вже є  |" << endl;
                cout << "+---------------------------------+" << endl;
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
            cout << "Чат порожній. Додайте повідомлення!" << endl;
            return;
        }
        cout << "|         Історія чату            |" << endl;
        cout << "+---------------------------------+" << endl;
        for (const auto& msg : messages) {
            msg->display();
            cout << endl;
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
                cout << "|  Повідомлення видалено успішно  |" << endl;
                cout << "+---------------------------------+" << endl;
                return;
            }
        }
        if (!found) {
            system("cls");
            showMenu();
            cout << "|  Повідомлення з таким ID нема   |" << endl;
            cout << "+---------------------------------+" << endl;
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



        cout << "|        Статистика чату          |" << endl;
        cout << "+---------------------------------+" << endl;
        cout << "| Всього повідомлень          " << setw(3) << totalMessages << " |" << endl;
        cout << "| Загальна кількість слів     " << setw(3) << totalWords << " |" << endl;
        cout << "| Фрагментів жирного          " << setw(3) << boldWords << " |" << endl;
        cout << "| Фрагментів курсиву          " << setw(3) << italicWords << " |" << endl;
        cout << "| Загальна кількість символів " << setw(3) << totalChars << " |" << endl;
        cout << "+---------------------------------+" << endl;
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
            file << "-----------------------------------" << endl;
        }
        cout << "|       Переписка збережена!      |" << endl;
        cout << "+---------------------------------+" << endl;
    }

    void loadFromFile() {
        messages.clear();
        ifstream file(filename);

        if (!file.is_open()) {
            cout << "|        Файл не знайдено!        |" << endl;
            cout << "+---------------------------------+" << endl;
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
            cout << "|     Жодне повідомлення не було  |" << endl;
            cout << "|          завантажено            |" << endl;
            cout << "+---------------------------------+" << endl;
        }
        else {
            cout << "|      Переписка завантажена!     |" << endl;
            cout << "+---------------------------------+" << endl;
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
            cout << "|        Результати пошуку        |" << endl;
            cout << "+---------------------------------+" << endl;
            for (const auto& msg : results) {
                msg->display();
            }
        }
        else {
            cout << "|    Повідомлення не знайдено     |" << endl;
            cout << "+---------------------------------+" << endl;
        }
    }

    void clearMessages() {
        char confirm;
        cout << "| Ви впевнені, що хочете видалити всі повідомлення? (y/n): ";
        cin >> confirm;
        cin.ignore(); // Очищення буфера

        if (confirm == 'y' || confirm == 'Y') {
            messages.clear();
            cout << "|        Переписка очищена        |" << endl;
            cout << "+---------------------------------+" << endl;
        }
        else {
            cout << "|     Видалення скасовано.        |" << endl;
            cout << "+---------------------------------+" << endl;
        }
    }

};

////////////////////////////////////
void addMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();
    cout << "|            Підказка:            |" << endl;
    cout << "+---------------------------------+" << endl;
    cout << "|  Щоб зробити жирний або курсив  |" << endl;
    cout << "|   скористуйтеся форматуванням   |" << endl;
    cout << "|       *Жирний* _Курсив_         |" << endl;
    cout << "| Щоб завершити введення, впишіть |" << endl;
    cout << "|       /0 на новому рядку        |" << endl;
    cout << "+---------------------------------+" << endl;

    cout << "Введіть текст повідомлення: ";

    string line, text;
    while (true) {
        getline(cin, line);
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
        cout << "Увага: непарна кількість * — форматування може бути некоректним." << endl;
    }
    if (underscores % 2 != 0) {
        cout << "Увага: непарна кількість _ — форматування може бути некоректним." << endl;
    }

    storage.addMessage(msg);
    system("cls");
    showMenu();
    cout << "|      Повідомлення додано!       |" << endl;
    cout << "+---------------------------------+" << endl;
}


void editMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();
    string inputId;
    cout << "Введіть ID повідомлення для редагування: ";
    getline(cin, inputId);
    int id;
    system("cls");
    showMenu();
    try {
        id = stoi(inputId);
        if (id < 0) throw invalid_argument("Negative ID");
    }
    catch (...) {
        cout << "Некоректний ID!" << endl;
        return;
    }

    // Зчитування нового тексту в кілька рядків
    cout << "|            Підказка:            |" << endl;
    cout << "+---------------------------------+" << endl;
    cout << "|  Щоб зробити жирний або курсив  |" << endl;
    cout << "|   скористуйтеся форматуванням   |" << endl;
    cout << "|       *Жирний* _Курсив_         |" << endl;
    cout << "| Щоб завершити введення, впишіть |" << endl;
    cout << "|       /0 на новому рядку        |" << endl;
    cout << "+---------------------------------+" << endl;

    cout << "Введіть текст нового повідомлення: ";
    string line, newText;
    while (true) {
        getline(cin, line);
        if (line == "/0") break;
        newText += line + "\n";
    }

    if (newText.empty()) {
        cout << "Повідомлення не може бути порожнім!" << endl;
        return;
    }

    if (newText.find('|') != string::npos) {
        cout << "Символ '|' заборонено!" << endl;
        return;
    }

    // Виклик редагування
    bool success = storage.editMessageById(id, newText);
    if (success) {
        shared_ptr<Message> editedMsg = make_shared<SimpleMessage>(newText, id);
        storage.deleteMessageById(id);        // видалити старе
        storage.addMessage(editedMsg);        // додати нове з тим самим ID
    }

    system("cls");
    showMenu();
    if (success) {
        cout << "|   Повідомлення відредаговано!   |" << endl;
    }
    else {
        cout << "|  Повідомлення з таким ID нема   |" << endl;
    }
    cout << "+---------------------------------+" << endl;
}



void searchMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();
    string keyword;
    cout << "Введіть слово для пошуку: ";
    getline(cin, keyword);

    if (keyword.empty()) {
        cout << "Слово для пошуку не може бути порожнім!" << endl;
        return;
    }

    system("cls");
    showMenu();
    storage.searchMessages(keyword);
}

void deleteMessageFlow(MessageStorage& storage) {
    system("cls");
    showMenu();
    string inputId;
    cout << "Введіть ID повідомлення для видалення: ";

    getline(cin, inputId);
    int id;

    try {
        id = stoi(inputId);
    }
    catch (...) {
        cout << "Некоректний ID!" << endl;
        return;
    }

    storage.deleteMessageById(id);
}

bool exitFlow(MessageStorage& storage) {
    system("cls");
    string saveInput;
    cout << "Бажаєте зберегти перед виходом? (Y/N): ";
    getline(cin, saveInput);

    if (saveInput == "Y" || saveInput == "y") {
        storage.saveToFile();
    }
    else if (saveInput != "N" && saveInput != "n") {
        cout << "Некоректний вибір. Введіть Y або N." << endl;
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
            cout << "Некоректний вибір. Введіть число від 0 до 9" << endl;
            continue;
        }

        if (choice < 0 || choice > 9) {
            cout << "Введіть число в межах від 0 до 9" << endl;
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

