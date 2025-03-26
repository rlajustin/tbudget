# tbudget

A simple TUI budget management application designed for you know exactly the type of person you are if you are at all interested in this application which solves a nonexistent problem. Built with ncurses and probably like 100 Cursor calls to claude-3.7 since the C language is too hard for me. Built and tested on Apple silicon so compatibility mileage may vary. If you find a bug and fix it that would be nice. Assume the rest of this readme was AI-generated.

## Features

- Set up a monthly total budget
- Create and manage budget categories
- Allocate portions of your budget to each category
- Track transactions and assign them to categories
- View budget allocation percentages and remaining funds
- Two different display modes: menu-based or full-screen dashboard

## Requirements

- GCC compiler
- ncurses library
- make

## Installation

### On Linux/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install gcc make libncurses5-dev
```

### On macOS:

```bash
brew install gcc ncurses
```

## Building

To compile the application, run:

```bash
make
```

This will generate an executable named `tbudget`.

## System-wide Installation

To install TBudget system-wide, allowing you to run it from anywhere with just the `tbudget` command:

```bash
sudo make install
```

This will install the binary to `/usr/local/bin/`. If you want to install it to a different location, use:

```bash
sudo make install PREFIX=/your/custom/path
```

To uninstall:

```bash
sudo make uninstall
```

## Running

To run the application, execute:

```bash
tbudget [OPTION]
```

If not installed system-wide, run it from the build directory:

```bash
./tbudget [OPTION]
```

### Command-line Options

The application supports two different display modes:

- **Menu-based Mode** (default): Navigate through separate screens for budget setup and transactions
  ```bash
  tbudget
  tbudget -m
  tbudget --menu
  tbudget 1
  ```

- **Dashboard Mode**: View budget summary and transactions in a single full-screen interface
  ```bash
  tbudget -d
  tbudget --dashboard
  tbudget 2
  ```

- **Export Data**: Export budget data to a CSV file
  ```bash
  tbudget -e
  tbudget --export
  ```

- **Import Data**: Import budget data from a CSV file
  ```bash
  tbudget -i filename.csv
  tbudget --import filename.csv
  ```

- **View Export History**: View a list of all timestamped export files
  ```bash
  tbudget -l
  tbudget --history
  ```

- **Help**
  ```bash
  tbudget -h
  tbudget --help
  ```

## Usage

### Menu-based Mode

The application consists of two main screens:

1. **Budget Setup**
   - Set your total monthly budget
   - Create categories for your expenses (e.g., Rent, Food, Transportation)
   - Allocate amounts to each category
   - View allocation percentages and remaining unallocated funds

2. **Transaction Management**
   - Add new transactions
   - Assign each transaction to a category
   - View a list of all transactions

### Dashboard Mode

The dashboard mode provides a full-screen overview with:

- **Budget Summary Panel** (top half)
  - Shows total budget and all budget categories
  - Displays allocation percentages and remaining funds

- **Transaction History Panel** (bottom half)
  - Shows all recorded transactions with details

**Keyboard Controls in Dashboard Mode:**
- `q` - Quit the application
- `b` - Go to budget setup
- `a` - Add a transaction
- `r` - Refresh the display

### Navigation

The application supports multiple ways to navigate through menus:

- **Arrow keys**: Use ↑ and ↓ to move between menu options
- **Number keys**: Press the number corresponding to a menu option to select it directly
- **Enter key**: Select the currently highlighted menu option
- **Backspace**: Go back to the previous menu
- **Escape**: Universal key to exit the current dialog or cancel the current operation

The currently selected menu item is highlighted for better visibility.

### Input Operations

When prompted to enter text or numeric values:

- Type your input and press Enter to confirm
- Universal ESC key works across all dialogs to cancel and return to the previous screen
- Backspace works to delete characters as expected

## Data Storage

TBudget stores all your data in a standard location based on your operating system:

- **Linux**: `~/.config/tbudget/`
- **macOS**: `~/Library/Application Support/tbudget/`
- **Windows**: `%LOCALAPPDATA%\tbudget\`

Within this directory, your data is organized as follows:

- `tbudget.dat` - Main data file with your budget and categories
- `tbudget_export.csv` - Latest CSV export of your budget data
- `data_storage/` - Directory containing timestamped backups of your exports

All data is automatically saved when you make changes and is loaded when you start the application.

## CSV Import/Export

TBudget automatically exports your data to CSV whenever you make changes, ensuring you always have an up-to-date export:

### Automatic Export

- Data is automatically exported whenever you:
  - Exit the application
  - Update your total budget
  - Add a new category
  - Add a new transaction

### Manual Import/Export

In addition to automatic exports, TBudget supports manual import/export operations:

- **Manual Export**
  ```bash
  tbudget -e
  tbudget --export
  ```
  This exports your data to the CSV file in your data directory.

- **Manual Import**
  ```bash
  tbudget -i filename.csv
  tbudget --import filename.csv
  ```
  This imports data from the specified CSV file.

### CSV Format

The exported CSV file follows this structure:

1. A header section with total budget information
2. A CATEGORIES section listing all budget categories with amounts and percentages
3. A TRANSACTIONS section listing all transactions with descriptions, amounts, categories, and dates

This file can be opened directly in any spreadsheet application for additional analysis or reporting.

## License

This project is open source and available under the MIT License.

## Future Enhancements

- Ability to edit/delete categories and transactions
- Reports and analytics
- Monthly budget summaries
- Import/export to other formats (Excel, JSON)

## Data Storage and CSV Import/Export

TBudget automatically stores your budget data in a file called `tbudget.dat`, ensuring your information persists between sessions. The data is also simultaneously exported to `tbudget_export.csv` in a format compatible with spreadsheet applications.

### Automatic Data Storage and Export

- Data is automatically saved and exported whenever you:
  - Exit the application
  - Update your total budget
  - Add a new category
  - Add a new transaction

- Data is automatically loaded when you start the application

- A timestamped history of all your export files is maintained in the `exports` directory, providing a complete record of your budget changes over time

### CSV Import/Export

In addition to automatic saving and exporting, TBudget supports manual import/export operations via command-line options:

- **Manual Export**
  ```bash
  ./tbudget -e
  ./tbudget --export
  ```
  This exports your data to `tbudget_export.csv`.

- **Manual Import**
  ```bash
  ./tbudget -i filename.csv
  ./tbudget --import filename.csv
  ```
  This imports data from the specified CSV file.

### CSV Format

The exported CSV file (`tbudget_export.csv`) follows this structure:

1. A header section with total budget information
2. A CATEGORIES section listing all budget categories with amounts and percentages
3. A TRANSACTIONS section listing all transactions with descriptions, amounts, categories, and dates

This file is always up-to-date with your latest changes and can be opened directly in any spreadsheet application for additional analysis or reporting. 