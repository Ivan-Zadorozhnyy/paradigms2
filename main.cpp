#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

class Memento {
    friend class DynamicArray;
    friend class CareTaker;

private:
    char* savedData;
    size_t savedSize;
    size_t savedCapacity;

    Memento(const char* data, size_t size, size_t capacity)
            : savedSize(size), savedCapacity(capacity) {
        savedData = new char[savedCapacity];
        std::memcpy(savedData, data, savedSize);
    }

    ~Memento() {
        delete[] savedData;
    }
};

class CareTaker {
private:
    std::vector<Memento*> undoStack;
    std::vector<Memento*> redoStack;

public:
    void saveState(const char* data, size_t size, size_t capacity) {
        auto memento = new Memento(data, size, capacity);
        undoStack.push_back(memento);

        // Clear the redo stack when we save a new state
        for (auto &memento : redoStack) {
            delete memento;
        }
        redoStack.clear();
    }

    void pushToUndo(const char* data, size_t size, size_t capacity) {
        auto memento = new Memento(data, size, capacity);
        undoStack.push_back(memento);
    }

    void pushToRedo(const char* data, size_t size, size_t capacity) {
        auto memento = new Memento(data, size, capacity);
        redoStack.push_back(memento);
    }

    Memento* undo() {
        if (!undoStack.empty()) {
            auto memento = undoStack.back();
            undoStack.pop_back();
            return memento;
        }
        return nullptr;
    }

    Memento* redo() {
        if (!redoStack.empty()) {
            auto memento = redoStack.back();
            redoStack.pop_back();
            return memento;
        }
        return nullptr;
    }

    ~CareTaker() {
        for (auto &memento : undoStack) {
            delete memento;
        }
        for (auto &memento : redoStack) {
            delete memento;
        }
    }
};

class DynamicArray {
private:
    char* data;
    size_t size;
    size_t capacity;
    CareTaker careTaker;
    std::string clipboard;

    void resize(size_t newCapacity) {
        char* newData = new char[newCapacity];
        std::memcpy(newData, data, size);
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    DynamicArray() : size(0), capacity(10) {
        data = new char[capacity];
        data[0] = '\0';
    }

    ~DynamicArray() {
        delete[] data;
    }

    void append(const char* text) {
        careTaker.saveState(data, size, capacity);  // Save state before appending
        size_t len = strlen(text);
        while (size + len >= capacity) {
            resize(capacity * 2);
        }
        std::strcpy(data + size, text);
        size += len;
    }

    void insertAndReplace(size_t pos, const char* substring, size_t replaceLen) {
        careTaker.saveState(data, size, capacity);  // Save state before inserting/replacing
        size_t len = strlen(substring);
        if (size + len - replaceLen >= capacity) {
            resize((size + len - replaceLen) * 2);
        }
        std::memmove(data + pos + len, data + pos + replaceLen, size - pos - replaceLen + 1);
        std::memcpy(data + pos, substring, len);
        size = size + len - replaceLen;
    }

    void deleteText(size_t pos, size_t len) {
        careTaker.saveState(data, size, capacity);  // Save state before deleting
        if (pos >= size || pos + len > size) {
            std::cout << "Invalid position or length.\n";
            return;
        }
        std::memmove(data + pos, data + pos + len, size - pos - len);
        size -= len;
        data[size] = '\0';
    }

    void cutText(size_t pos, size_t len) {
        careTaker.saveState(data, size, capacity);  // Save state before cutting
        if (pos >= size || pos + len > size) {
            std::cout << "Invalid position or length.\n";
            return;
        }
        copyText(pos, len);
        deleteText(pos, len);
    }

    void copyText(size_t pos, size_t len) {
        if (pos >= size || pos + len > size) {
            std::cout << "Invalid position or length.\n";
            return;
        }
        clipboard.assign(data + pos, len);
    }

    void pasteText(size_t pos) {
        careTaker.saveState(data, size, capacity);  // Save state before pasting
        if (pos > size) {
            std::cout << "Invalid position.\n";
            return;
        }
        insertAndReplace(pos, clipboard.c_str(), 0);
    }

    void undo() {
        // Save the current state to the redo stack before undoing
        careTaker.pushToRedo(data, size, capacity);
        Memento* memento = careTaker.undo();
        if (memento) {
            if (capacity != memento->savedCapacity) {
                resize(memento->savedCapacity);
            }
            std::memcpy(data, memento->savedData, memento->savedSize);
            size = memento->savedSize;
            data[size] = '\0';
        } else {
            std::cout << "Cannot undo further.\n";
        }
    }

    void redo() {
        // Save the current state to the undo stack before redoing
        careTaker.pushToUndo(data, size, capacity);
        Memento* memento = careTaker.redo();
        if (memento) {
            if (capacity != memento->savedCapacity) {
                resize(memento->savedCapacity);
            }
            std::memcpy(data, memento->savedData, memento->savedSize);
            size = memento->savedSize;
            data[size] = '\0';
        } else {
            std::cout << "Cannot redo further.\n";
        }
    }

    const char* getText() const {
        return data;
    }

    size_t findText(const char* search) const {
        char* found = std::strstr(data, search);
        return found ? found - data : -1;
    }

    void saveToFile(const std::string& filename) const {
        std::ofstream outFile(filename);
        if (outFile.is_open()) {
            outFile << data;
            outFile.close();
            std::cout << "Saved to " << filename << std::endl;
        } else {
            std::cout << "Failed to save to " << filename << std::endl;
        }
    }

    void loadFromFile(const std::string& filename) {
        std::ifstream inFile(filename);
        if (inFile.is_open()) {
            std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            delete[] data;
            size = content.size();
            capacity = size + 1;
            data = new char[capacity];
            std::strcpy(data, content.c_str());
            inFile.close();
            std::cout << "Loaded from " << filename << std::endl;
        } else {
            std::cout << "Failed to load from " << filename << std::endl;
        }
    }
};

void menu_display() {
    std::cout << "Choose the command:\n"
              << "1. Append text\n"
              << "2. Start new line\n"
              << "3. Save as file\n"
              << "4. Load file\n"
              << "5. Print current saved text\n"
              << "6. Find text\n"
              << "7. Insert text at position\n"
              << "8. Clear console (platform dependent)\n"
              << "9. Undo\n"
              << "10. Redo\n"
              << "11. Delete text\n"
              << "12. Cut text\n"
              << "13. Copy text\n"
              << "14. Paste text\n"
              << "0. Exit\n";
}

int main() {
    DynamicArray arr;

    while (true) {
        menu_display();

        int choice;
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
            case 1: {
                std::cout << "Enter text to append:\n";
                std::string text;
                std::getline(std::cin, text);
                arr.append(text.c_str());
                break;
            }
            case 2: {
                arr.append("\n");
                break;
            }
            case 3: {
                std::cout << "Enter the filename to save:\n";
                std::string filename;
                std::getline(std::cin, filename);
                arr.saveToFile(filename);
                break;
            }
            case 4: {
                std::cout << "Enter the filename to load:\n";
                std::string filename;
                std::getline(std::cin, filename);
                arr.loadFromFile(filename);
                break;
            }
            case 5: {
                std::cout << "Current saved text:\n" << arr.getText() << '\n';
                break;
            }
            case 6: {
                std::cout << "Enter the text to find:\n";
                std::string text;
                std::getline(std::cin, text);
                size_t pos = arr.findText(text.c_str());
                if (pos != -1) {
                    std::cout << "Found text at position " << pos << std::endl;
                } else {
                    std::cout << "Text not found." << std::endl;
                }
                break;
            }
            case 7: {
                std::cout << "Enter the position to insert text:\n";
                size_t pos;
                std::cin >> pos;
                std::cin.ignore(); // consume newline
                std::cout << "Enter the text to insert:\n";
                std::string text;
                std::getline(std::cin, text);
                std::cout << "Enter the number of characters to replace at the insertion point (0 for none):\n";
                size_t replaceLen;
                std::cin >> replaceLen;
                std::cin.ignore(); // consume newline
                arr.insertAndReplace(pos, text.c_str(), replaceLen);
                break;
            }
            case 8: {
                system("clear");  // Adjust this for your platform
                break;
            }
            case 9: {
                arr.undo();
                break;
            }
            case 10: {
                arr.redo();
                break;
            }
            case 11: {
                std::cout << "Enter the starting position and length to delete:\n";
                size_t pos, len;
                std::cin >> pos >> len;
                std::cin.ignore();  // consume the newline
                arr.deleteText(pos, len);
                break;
            }
            case 12: {
                std::cout << "Enter the starting position and length to cut:\n";
                size_t pos, len;
                std::cin >> pos >> len;
                std::cin.ignore();  // consume the newline
                arr.cutText(pos, len);
                break;
            }
            case 13: {
                std::cout << "Enter the starting position and length to copy:\n";
                size_t pos, len;
                std::cin >> pos >> len;
                std::cin.ignore();  // consume the newline
                arr.copyText(pos, len);
                break;
            }
            case 14: {
                std::cout << "Enter the position to paste:\n";
                size_t pos;
                std::cin >> pos;
                std::cin.ignore();  // consume the newline
                arr.pasteText(pos);
                break;
            }
            case 0:
                return 0;
            default:
                std::cout << "Invalid command\n";
                break;
        }
    }
}
