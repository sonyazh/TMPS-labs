#include <algorithm>
#include <charconv>
#include <cctype>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

struct Book {
    int id;
    std::string title;
    bool available;
    bool referenceOnly;
};

class BookFactory {
public:
    static Book create(int id, const std::string& title, bool referenceOnly) {
        return {id, title, true, referenceOnly};
    }
};

class IBookRepository {
public:
    virtual ~IBookRepository() = default;
    virtual std::vector<Book> getAll() const = 0;
    virtual std::optional<Book> findById(int id) const = 0;
    virtual void add(const Book& book) = 0;
    virtual bool update(const Book& book) = 0;
};

class InMemoryBookRepository : public IBookRepository {
public:
    InMemoryBookRepository()
        : books_{BookFactory::create(1, "The Hobbit", false),
                 BookFactory::create(2, "Clean Code", false),
                 BookFactory::create(3, "Oxford English Dictionary", true)} {}

    std::vector<Book> getAll() const override {
        return books_;
    }

    std::optional<Book> findById(int id) const override {
        const auto book = std::find_if(books_.begin(), books_.end(),
                                       [id](const Book& item) { return item.id == id; });
        if (book == books_.end()) {
            return std::nullopt;
        }
        return *book;
    }

    void add(const Book& book) override {
        books_.push_back(book);
    }

    bool update(const Book& updatedBook) override {
        const auto book = std::find_if(books_.begin(), books_.end(),
                                       [&updatedBook](const Book& item) {
                                           return item.id == updatedBook.id;
                                       });
        if (book == books_.end()) {
            return false;
        }
        *book = updatedBook;
        return true;
    }

private:
    std::vector<Book> books_;
};

class ILoanPolicy {
public:
    virtual ~ILoanPolicy() = default;
    virtual bool canBorrow(const Book& book) const = 0;
};

class StandardLoanPolicy : public ILoanPolicy {
public:
    bool canBorrow(const Book& book) const override {
        return !book.referenceOnly;
    }
};

struct LibraryEvent {
    std::string action;
    Book book;
};

class ILibraryObserver {
public:
    virtual ~ILibraryObserver() = default;
    virtual void onLibraryEvent(const LibraryEvent& event) = 0;
};

class LibraryService {
public:
    LibraryService(IBookRepository& repository, const ILoanPolicy& loanPolicy)
        : repository_(repository), loanPolicy_(loanPolicy), nextId_(1) {
        for (const Book& book : repository_.getAll()) {
            nextId_ = std::max(nextId_, book.id + 1);
        }
    }

    void addObserver(ILibraryObserver& observer) {
        observers_.push_back(&observer);
    }

    std::vector<Book> listBooks() const {
        return repository_.getAll();
    }

    std::vector<Book> searchBooks(const std::string& query) const {
        std::string normalizedQuery = toLower(query);
        std::vector<Book> matches;
        for (const Book& book : repository_.getAll()) {
            if (toLower(book.title).find(normalizedQuery) != std::string::npos) {
                matches.push_back(book);
            }
        }
        return matches;
    }

    std::string borrowBook(int id) {
        std::optional<Book> book = repository_.findById(id);
        if (!book) {
            return "No book found with that ID.";
        }
        if (!book->available) {
            return "That book is already borrowed.";
        }
        if (!loanPolicy_.canBorrow(*book)) {
            return "That book is for reference only and cannot be borrowed.";
        }
        book->available = false;
        if (!repository_.update(*book)) {
            return "Could not update the book.";
        }
        notifyObservers({"borrowed", *book});
        return "Book borrowed.";
    }

    std::string returnBook(int id) {
        std::optional<Book> book = repository_.findById(id);
        if (!book) {
            return "No book found with that ID.";
        }
        if (book->available) {
            return "That book is already in the library.";
        }
        book->available = true;
        if (!repository_.update(*book)) {
            return "Could not update the book.";
        }
        notifyObservers({"returned", *book});
        return "Book returned.";
    }

    int addBook(const std::string& title, bool referenceOnly) {
        const int id = nextId_++;
        repository_.add(BookFactory::create(id, title, referenceOnly));
        return id;
    }

private:
    void notifyObservers(const LibraryEvent& event) const {
        for (ILibraryObserver* observer : observers_) {
            observer->onLibraryEvent(event);
        }
    }

    static std::string toLower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                       });
        return value;
    }

    IBookRepository& repository_;
    const ILoanPolicy& loanPolicy_;
    int nextId_;
    std::vector<ILibraryObserver*> observers_;
};

class ConsoleActivityObserver : public ILibraryObserver {
public:
    void onLibraryEvent(const LibraryEvent& event) override {
        std::cout << "Activity: \"" << event.book.title << "\" was "
                  << event.action << ".\n";
    }
};

class ConsoleUI {
public:
    explicit ConsoleUI(LibraryService& library) : library_(library) {}

    void run() {
        std::cout << "Library Management System\n";
        bool running = true;
        while (running) {
            printMenu();
            const std::string choice = readLine("Choose an option: ");
            if (choice == "1") {
                printBooks(library_.listBooks());
            } else if (choice == "2") {
                searchBooks();
            } else if (choice == "3") {
                std::cout << library_.borrowBook(readId()) << '\n';
            } else if (choice == "4") {
                std::cout << library_.returnBook(readId()) << '\n';
            } else if (choice == "5") {
                addBook();
            } else if (choice == "0") {
                running = false;
            } else {
                std::cout << "Please choose a listed option.\n";
            }
        }
        std::cout << "Goodbye.\n";
    }

private:
    static void printMenu() {
        std::cout << "\n1. List books\n"
                     "2. Search by title\n"
                     "3. Borrow a book\n"
                     "4. Return a book\n"
                     "5. Add a book\n"
                     "0. Exit\n";
    }

    static std::string readLine(const std::string& prompt) {
        std::cout << prompt;
        std::string value;
        std::getline(std::cin, value);
        return value;
    }

    static bool parseId(const std::string& input, int& id) {
        const char* begin = input.data();
        const char* end = begin + input.size();
        const auto result = std::from_chars(begin, end, id);
        return result.ec == std::errc{} && result.ptr == end && id > 0;
    }

    static int readId() {
        while (true) {
            const std::string input = readLine("Book ID: ");
            int id = 0;
            if (parseId(input, id)) {
                return id;
            }
            std::cout << "Enter a positive whole-number ID.\n";
        }
    }

    static void printBooks(const std::vector<Book>& books) {
        if (books.empty()) {
            std::cout << "No books found.\n";
            return;
        }
        for (const Book& book : books) {
            std::cout << '[' << book.id << "] " << book.title << " - "
                      << (book.available ? "Available" : "Borrowed")
                      << (book.referenceOnly ? " (Reference only)" : "") << '\n';
        }
    }

    void searchBooks() const {
        const std::string query = readLine("Title to search for: ");
        printBooks(library_.searchBooks(query));
    }

    void addBook() {
        std::string title = readLine("Book title: ");
        if (title.empty()) {
            std::cout << "A title cannot be empty.\n";
            return;
        }
        const std::string answer = readLine("Reference only? (y/n): ");
        if (answer != "y" && answer != "Y" && answer != "n" && answer != "N") {
            std::cout << "Enter y or n.\n";
            return;
        }
        const int id = library_.addBook(title, answer == "y" || answer == "Y");
        std::cout << "Book added with ID " << id << ".\n";
    }

    LibraryService& library_;
};

int main() {
    InMemoryBookRepository repository;
    StandardLoanPolicy loanPolicy;
    LibraryService library(repository, loanPolicy);
    ConsoleActivityObserver activityObserver;
    library.addObserver(activityObserver);
    ConsoleUI console(library);
    console.run();
    return 0;
}