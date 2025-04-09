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
        setConsoleColor(8);  // Серый цвет
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
        return lhs->getId() < rhs->getId(); // тепер сортування буде по ID
    }
};


class MessageStorage {
private:
    set<shared_ptr<Message>, MessageComparator> messages;
    string filename = "messages.txt";

public:
    void addMessage(shared_ptr<Message> msg) {
        messages.insert(msg);
    }

    void displayMessages() const {
        if (messages.empty()) {
            cout << "Чат порожній. Додайте повідомлення!" << endl;
            return;
        }
        cout << "|         Історія чату:           |" << endl;
        cout << "+---------------------------------+" << endl;
        for (const auto& msg : messages) {
            msg->display();
        }
    }

    void editMessageById(int idToEdit) {
        bool found = false;
        vector<shared_ptr<Message>> temp(messages.begin(), messages.end());

        for (auto& msg : temp) {
            if (msg->getId() == idToEdit) {
                found = true;
                string newText;
                cout << "Введіть новий текст повідомлення: ";
                getline(cin, newText);

                shared_ptr<Message> editedMsg = make_shared<SimpleMessage>(newText, idToEdit);


                // ✅ Правильний спосіб — через геттер
                if (idToEdit > Message::getGlobalCounter()) {
                    Message::setGlobalCounter(idToEdit);
                }

                messages.erase(msg);
                messages.insert(editedMsg);
                break;
            }
        }

        if (!found) {
            cout << "|  Повідомлення з таким ID нема   |" << endl;
            cout << "+---------------------------------+" << endl;
        }
    }

    void deleteMessageById(int idToDelete) {
        bool found = false;
        for (auto it = messages.begin(); it != messages.end(); ++it) {
            if ((*it)->getId() == idToDelete) {
                messages.erase(it);
                found = true;
                cout << "|  Повідомлення видалено успішно  |" << endl;
                cout << "+---------------------------------+" << endl;
                return;
            }
        }
        if (!found) {
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

            stringstream ss(txt);
            string word;
            while (ss >> word) {
                totalWords++;

                // Якщо слово починається і закінчується на * або _
                if (word.size() >= 2) {
                    if (word.front() == '*' && word.back() == '*')
                        boldWords++;
                    else if (word.front() == '_' && word.back() == '_')
                        italicWords++;
                }

                // Пошук багатослівних конструкцій типу "*жирний текст*"
                // через підрахунок кількості зірочок/підкреслень
                int boldCount = count(txt.begin(), txt.end(), '*');
                int italicCount = count(txt.begin(), txt.end(), '_');

                // Кожна пара символів формує один блок форматування
                boldWords += boldCount / 2;
                italicWords += italicCount / 2;
                break;
            }
        }

        cout << "|        Статистика чату:         |" << endl;
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
            file << msg->getId() << "|" << msg->getType() << "|" << msg->getText() << endl;
        }
        cout << "|       Переписка збережена!      |" << endl;
        cout << "+---------------------------------+" << endl;
    }

    void loadFromFile() {
        messages.clear();
        ifstream file(filename);
        string line, type, text;

        while (getline(file, line)) {
            size_t firstDelim = line.find('|');
            size_t secondDelim = line.find('|', firstDelim + 1);

            if (firstDelim != string::npos && secondDelim != string::npos) {
                string idStr = line.substr(0, firstDelim);
                type = line.substr(firstDelim + 1, secondDelim - firstDelim - 1);
                text = line.substr(secondDelim + 1);

                int idFromFile = stoi(idStr);

                shared_ptr<Message> msg = make_shared<SimpleMessage>(text, idFromFile);
                messages.insert(msg);
            }
        }

        cout << "|      Переписка завантажена!     |" << endl;
        cout << "+---------------------------------+" << endl;
    }



    void searchMessages(const string& keyword) const {
        bool found = false;

        for (const auto& msg : messages) {
            if (msg->getText().find(keyword) != string::npos) {
                cout << "|        Результати пошуку:       |" << endl;
                cout << "+---------------------------------+" << endl;
                msg->display();
                found = true;
            }
        }
        if (!found) {
            cout << "|    Повідомлення не знайдено     |" << endl;
            cout << "+---------------------------------+" << endl;
        }
    }


    void clearMessages() {
        messages.clear();
        cout << "|        Переписка очищена        |" << endl;
        cout << "+---------------------------------+" << endl;
    }
};

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
    cout << "|  8  | Статистика чату           |" << endl;
    cout << "|  9  | Видалити повідомлення     |" << endl;
    cout << "| 10  | Вихід                     |" << endl;
    cout << "+---------------------------------+" << endl;
}


int main() {
    setConsoleColor(8);
    //shared_ptr<Message> msg = make_shared<SimpleMessage>("Привет, это *жирный* и _курсив_ текст!");
    //msg->display();
    MessageStorage storage;
    int choice;

    //system("pause");
    
    showMenu();
    do {
                              
        cout << "Виберіть дію: ";
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1: {
            system("cls");
            showMenu();
            cout << "|            Підказка:            |" << endl;
            cout << "+---------------------------------+" << endl;
            cout << "|  Щоб зробити жирний або курсив  | " << endl;
            cout << "|   скористуйтеся форматуванням   | " << endl;
            cout << "|       *Жирний* _Курсив_         | " << endl;
            cout << "+---------------------------------+" << endl;
            string text;
            cout << "Введіть текст повідомлення: ";
            getline(cin, text);
            shared_ptr<Message> msg = make_shared<SimpleMessage>(text);

            storage.addMessage(msg);

            break;
        }
        case 2:
            system("cls");
            showMenu();
            storage.displayMessages();
            break;
        case 3:
            system("cls");
            showMenu();
            storage.saveToFile();
            break;
        case 4:
            system("cls");
            showMenu();
            storage.loadFromFile();
            break;
        case 5: {
            system("cls");
            showMenu();
            int id;
            cout << "Введіть ID повідомлення для редагування: ";
            cin >> id;
            cin.ignore();
            storage.editMessageById(id);
            system("cls");
            showMenu();
            cout << "|   Повідомлення відредаговано!   |" << endl;
            cout << "+---------------------------------+" << endl;
            break;
        }
        case 6:
            system("cls");
            showMenu();
            storage.clearMessages();
            break;
        case 7: {
            system("cls");
            showMenu();
            string keyword;
            cout << "Введіть слово для пошуку: ";
            getline(cin, keyword);
            system("cls");
            showMenu();
            storage.searchMessages(keyword);
            break;
        }
        case 8: {
            system("cls");
            showMenu();
            storage.showStatistics();
            break;
        }
        case 9: {
            system("cls");
            showMenu();
            int id;
            cout << "Введіть ID повідомлення для видалення: ";
            cin >> id;
            cin.ignore();
            storage.deleteMessageById(id);
            break;
        }
        case 10:
            system("cls");
            char saveChoice;
            cout << "Бажаєте зберегти перед виходом? (Y/N): ";
            cin >> saveChoice;
            if (saveChoice == 'Y' || saveChoice == 'y') {
                storage.saveToFile();
            }
            cout << "Вихід..." << endl;
            break;

        default:
            cout << "Некоректний ввід, спробуйте знову" << endl;
        }


    } while (choice != 10);



    return 0;
}