#include <iostream>
#include <iomanip>
#include <set>
#include <fstream>
#include <sstream>
#include <memory>
#include <windows.h>
#include <ctime>

using namespace std;

void setConsoleColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

class Message {
protected:
    string text;
    string timestamp;

public:
    Message(const string& txt) : text(txt) {
        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);
        char buffer[6];
        strftime(buffer, sizeof(buffer), "%H:%M", &ltm);
        timestamp = string(buffer);
    }

    virtual ~Message() {}

    virtual string getText() const {
        return text + " [" + timestamp + "]";
    }

    virtual string getType() const {
        return "Просте";
    }

    virtual void display() const {
        cout << getText() << endl;
    }

    bool operator<(const Message& other) const {
        return text < other.text;
    }


    string getTimestamp() const {
        return timestamp;
    }
};


class SimpleMessage : public Message {
public:
    SimpleMessage(const string& txt) : Message(txt) {}

    void display() const override {
        setConsoleColor(8);  // Серый цвет для обычного текста
        applyFormatting(getText());  // Вызываем новый метод форматирования
        setConsoleColor(8);
    }


private:
    void applyFormatting(const string& text) const {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        WORD defaultColor = csbi.wAttributes; // Сохраняем текущий цвет

        bool bold = false, italic = false;

        for (size_t i = 0; i < text.length(); i++) {
            if (text[i] == '*' && (i == 0 || text[i - 1] != '\\')) {
                bold = !bold;
                setConsoleColor(bold ? 0 : defaultColor); // Черный цвет
                continue;
            }
            if (text[i] == '_' && (i == 0 || text[i - 1] != '\\')) {
                italic = !italic;
                cout << (italic ? "\033[3m" : "\033[0m");

                // Если курсив выключается, но жирный еще включен — черный цвет
                if (!italic && bold) {
                    setConsoleColor(0);
                }
                else if (!italic) {
                    setConsoleColor(defaultColor);
                }

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
        : Message(msg->getText()), wrappedMessage(msg) {}

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
        cout << getText() << endl;
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
        cout << "\033[3m" << getText() << "\033[0m" << endl;
        setConsoleColor(8);
    }
};


struct MessageComparator {
    bool operator()(const shared_ptr<Message>& lhs, const shared_ptr<Message>& rhs) const {
        return lhs->getText() < rhs->getText();
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
            cout << "\nЧат порожній. Додайте повідомлення!\n" << endl;
            return;
        }
        cout << "\nІсторія чату:\n";
        cout << "+-------------------------------+\n";
        for (const auto& msg : messages) {
            msg->display();
        }
    }

    void saveToFile() {
        ofstream file(filename);
        for (const auto& msg : messages) {
            file << msg->getType() << "|" << msg->getTimestamp() << "|" << msg->getText() << endl;
        }
        cout << "Переписка збережена в " << filename << endl;
    }


    void loadFromFile() {
        messages.clear();
        ifstream file(filename);
        string line, type, text, time;
        while (getline(file, line)) {
            size_t firstDelim = line.find('|');
            size_t secondDelim = line.find('|', firstDelim + 1);
            if (firstDelim != string::npos && secondDelim != string::npos) {
                type = line.substr(0, firstDelim);
                time = line.substr(firstDelim + 1, secondDelim - firstDelim - 1);
                text = line.substr(secondDelim + 1);

                shared_ptr<Message> msg = make_shared<SimpleMessage>(text);
              

                messages.insert(msg);
            }
        }
        cout << "Переписка завантажена з " << filename << endl;
    }


    void clearMessages() {
        messages.clear();
        cout << "Переписка очищена." << endl;
    }
};

int main() {
    setConsoleColor(8);
    shared_ptr<Message> msg = make_shared<SimpleMessage>("Привет, это *жирный* и _курсив_ текст!");
    msg->display();

    MessageStorage storage;
    int choice;
    cout << "+-------------------------------+" << endl;
    cout << "|             МЕНЮ              |" << endl;
    cout << "+-------------------------------+" << endl;
    cout << "| 1 | Додати повідомлення       |" << endl;
    cout << "| 2 | Показати всі повідомлення |" << endl;
    cout << "| 3 | Зберегти переписку        |" << endl;
    cout << "| 4 | Завантажити переписку     |" << endl;
    cout << "| 5 | Очистити переписку        |" << endl;
    cout << "| 6 | Вихід                     |" << endl;
    cout << "+-------------------------------+" << endl;
    do {
        cout << "Виберіть дію: ";
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1: {
            string text;
            int format;
            cout << "Введіть текст повідомлення: ";
            getline(cin, text);
            shared_ptr<Message> msg = make_shared<SimpleMessage>(text);
            storage.addMessage(msg);

            break;
        }
        case 2:
            storage.displayMessages();
            break;
        case 3:
            storage.saveToFile();
            break;
        case 4:
            storage.loadFromFile();
            break;
        case 5:
            storage.clearMessages();
            break;
        case 6:
            cout << "Вихід..." << endl;
            break;
        default:
            cout << "Некоректний ввід, спробуйте знову." << endl;
        }
    } while (choice != 6);
    return 0;
}