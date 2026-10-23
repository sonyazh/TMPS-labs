# Library Management System

## Project goal

Create a small terminal-based library system to demonstrate three SOLID
principles and three design patterns. The application lets a user list and
search books, borrow and return them, and add new books.

## SOLID principles selected

1. **Single Responsibility Principle (SRP):** `Book` stores book data,
   `InMemoryBookRepository` manages the collection, `LibraryService` handles
   library operations, and `ConsoleUI` handles terminal interaction.
2. **Open/Closed Principle (OCP):** borrowing rules are represented by the
   `ILoanPolicy` abstraction. A new policy can be introduced without changing
   `LibraryService`.
3. **Dependency Inversion Principle (DIP):** `LibraryService` depends on
   `IBookRepository` and `ILoanPolicy` abstractions rather than constructing or
   owning concrete storage and policy implementations.

## Design patterns

### Strategy

**Theory:** Strategy defines a family of interchangeable algorithms, puts each
one behind a common interface, and lets a client use one without depending on
its concrete implementation.

**In this project:** `ILoanPolicy` defines the borrowing rule and
`StandardLoanPolicy` implements it. `LibraryService` calls `canBorrow` rather
than embedding the rule in its borrowing workflow. A different policy can be
provided without rewriting that workflow.

### Factory

**Theory:** A Factory centralizes object creation, so client code can request an
object without needing to know the details of how it is initialized.

**In this project:** `BookFactory::create` creates a `Book` with its ID, title,
availability, and reference-only status. Both the sample catalogue and the
add-book operation use the factory, keeping the default available state in one
place.

### Observer

**Theory:** Observer defines a one-to-many notification relationship. When the
subject's relevant state changes, it notifies registered observers, without
needing to know the specific work each observer performs.

**In this project:** `LibraryService` is the subject. After a successful borrow
or return, it sends a `LibraryEvent` to registered `ILibraryObserver`
instances. `ConsoleActivityObserver` listens and prints an activity message.
Failed operations do not emit events.

## Current implementation

- In-memory sample catalogue, including a reference-only title.
- Case-insensitive title search.
- Borrowing and returning with checks for missing, already borrowed, and
  reference-only books.
- Adding books with automatically assigned IDs.
- A Factory that centralizes creation of new and sample books.
- An Observer that prints activity messages after successful borrow and return
  operations.
- Input validation for menu choices, IDs, titles, and reference-only status.