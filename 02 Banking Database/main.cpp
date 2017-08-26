// Kai Brooks
// 02 Banking Database
// January 2015
//
// This is a banking database. It must load a database file to begin. A user must be loaded (in the menu system) to access some
// options. USERMODE determines the level of access the program has on running.
//
// GitHub: Classwork code sample


#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>

using namespace std;

// Declare constants
const int USERMODE = 1;             // 3 for customer, 2 for banker, 1 for manager
const int MAXCUSTOMERS = 50;        // Array size, the maximum number of customers we can have
const int OVERDRAFTFEE = 35;        // Dollar cost of overdrafting
const int OPENINGDEPOSIT = 25;      // Minimum deposit to open an account

const string currentDateTime() {    // Used to generate date and time for logs
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X ", &tstruct);
    return buf;
}

// Declare functions
struct Customer {
    string name;        // Customer's name
    int acctNumber;     // Customer's account number
    string acctType;    // The type of account (checking or savings)
    int routNumber;     // Customers routing number, based on where account was opened
    string state;       // US State account was opened in
    long double balance;// Customer's account balance
    long phone;         // Customer's phone number
    string status;      // This determines the status of the account: active, suspended, or closed.
    bool overdraftable; // This determines if the account can be overdrawn or not
    int pin;            // PIN used for authentication
    int openDate;       // Date account was opened
};

struct Passthrough {
    int activeAccount;  // Account number of currently active account
    string activeName;  // Customer's name of currently active account
    int arrayPos = -1;  // Array postition of active account
    int glPos;          // Array position of the general ledger
    string filePath;    // Location of the bank database file
};

struct DataCollection {
    int noTransactions;                     // How many daily transactions there are
    string manager;                         // Log (transactionary)
    string admin;                           // Log (administrative)
    string loadTime = currentDateTime();    // The time the program started running
    double transactionAmount;               // The dollar amount of all daily transactions
    bool writeToDatabase = false;           // This determines if the database file should be written to after certain actions
};

Passthrough Loaded;

DataCollection Log;

Customer arr[MAXCUSTOMERS];

enum AccountType {
    unknown = -1, Checking, Savings, CreditCard, InstantAccess
};


int kill();                     // Ends the program
int creditApplication();        // Apply for a line of credit
int viewTotals();               // View the total dollars, transactions, and number of accounts
int customerSearch();           // Find a customer and load it
int balanceInquiry();           // Check a balance
int withdrawFunds();            // Withdraw funds
void showMenu();                // Show the menu
int addCustomer();              // Add a new customer to the database
int removeCustomer();           // Close an account
int viewLog();                  // View the detailed log
void exitActiveAccount();       // Unload a loaded customer
int depositFunds();             // Deposit funds
int transferFunds();            // Transfer funds between accounts
int loadDatabase();             // Load and parse the initial database
int debugArrayData();           // DEBUG: outputs the entire banking database
int writeDatabase();            // Writes the struct to the database text file
int generalLedger();            // General ledger functions for the manager

int main () {
    cout << "Assignment 02 Banking Database - Kai Brooks" << endl;
    
    // Declare variables
    char selection;             // User input in the menu
    
    loadDatabase();             // Load the database file and parse the data
    showMenu();                 // Display the main menu
    
    // Customer input
    while (selection != 'q') {
        
        if (Log.writeToDatabase){   // If we need to write to the database, we do it before anything else happens
            writeDatabase();
        }
        
        cout << "Enter selection or [M]enu: ";
        cin >> selection;
        cin.ignore(1000, '\n');
        switch (selection) {
            case 'm':
                showMenu();
                break;
            case 's':
                customerSearch();
                break;
            case 'a':
                addCustomer();
                break;
            case 'r':
                removeCustomer();
                break;
            case 'c':
                creditApplication();
                break;
            case 'v':
                viewTotals();
                break;
            case 'l':
                viewLog();
                break;
            case 'e':
                exitActiveAccount();
                break;
            case 'b':
                balanceInquiry();
                break;
            case 'd':
                depositFunds();
                break;
            case 't':
                transferFunds();
                break;
            case 'w':
                withdrawFunds();
                break;
            case 'q':
                kill();
                break;
            case 'p':
                writeDatabase();
                break;
            case 'g':
                generalLedger();
                break;
            default:
                cout << "Invalid option." << endl;
                break;
        }
    }
    
    kill(); // Kill the program (fail-safe to prevent a "bad" loop)
}

int kill() {
    cout << endl << "[End] ";   // End the program
    cin.clear();
    getchar();
    return 0;
}

void showMenu() {
    cout << endl;
    if (USERMODE <=3) {                 // If you are at least a customer
        if (Loaded.arrayPos >= 0) {     // Only show these options if a customer is loaded
            cout << "[B]alance inquiry" << endl;
            cout << "[D]eposit funds" << endl;
            cout << "[W]ithdraw funds" << endl;
            cout << "[T]ransfer funds" << endl;
        } else {
            cout << "(No customer loaded, please search)" << endl;
            cout << "[S]earch for account" << endl;
        }
    }
    if (USERMODE <= 2) {                // If you are at least a banker
        cout << "[A]dd new customer" << endl;
        cout << "[R]emove customer" << endl;
        cout << "[C]redit application" << endl;
    }
    if (USERMODE <= 1) {                // If you are at least a manager
        cout << "[V]iew totals" << endl;
        cout << "[L]og of transactions" << endl;
        cout << "[G]eneral ledger options" << endl;
    }
    if (Loaded.arrayPos >= 0) {         // This option appears only if we have a customer loaded
        cout << "[E]xit " << arr[Loaded.arrayPos].name << endl;
    }
    cout << "[Q]uit program" << endl;
}

int customerSearch() {
    int searchNum;
    int searchPin;
    bool searchFound = false;
    string searchName;
    string cellData;        // The data contained in the array in which we are comparing our search to
    char selection;         // Yes/No character
    
    cout << "Enter account number: ";
    cin >> searchNum;
    cin.ignore(1000, '\n');
    
    if (USERMODE == 3) {        // "Customer" should also need to enter their PIN, to prevent opening others' accounts
        cout << "Enter PIN: ";
        cin >> searchPin;
        cin.ignore(1000, '\n');
    }
    
    for (int i = 0; i < MAXCUSTOMERS; i++) {
        if (arr[i].acctNumber == searchNum && arr[i].status != "gl") { // If we find a non-gl match on account number..
            if (USERMODE <= 2) {                                // And we are a banker or manager..
                Loaded.arrayPos = i;                            // Load the account
                cout << arr[i].name << " found (account " << arr[i].acctNumber << ")" << endl;
                return 0;
            } else {                                            // If we find a match and we are a customer
                if (arr[i].pin == searchPin) {                  // And our PIN matches the PIN on file
                    Loaded.arrayPos = i;                        // Load the account
                    cout << arr[i].name << " found (account " << arr[i].acctNumber << ")" << endl;
                    return 0;
                } else {                                        // If the PIN doesn't match
                    cout << "No results (customer)." << endl;   // Don't want to give up any customer information
                    return 0;
                }
            }
        }
    }
    
    // If we are here, our search by account number has failed, and we transfer to search by name (sequential search)
    // We do not want the general ledger to appear in our searches
    if (USERMODE <= 2) {        // If we are a banker or higher
        cout << "No results found. Search by name: ";
        cin >> searchName;
        cin.ignore(1000, '\n');
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);    // Convert cin to lowercase
    
        for(int i = 0; i < MAXCUSTOMERS; i++) { // Loop the data in the array
            cellData = arr[i].name;             // Fill "cellData" with what we will compare our search term to
            std::transform(cellData.begin(), cellData.end(), cellData.begin(), ::tolower);      // Convert data to lowercase
            
            if ((cellData.find(searchName) != string::npos) && arr[i].status != "gl") {    // If we get a non-gl match
                cout << arr[i].name << " found (account " << arr[i].acctNumber << "). Is this correct? (y/n/a) " ;
                cin >> selection;
                if (selection == 'a') {                         // We want to be able to abort our search in case of bad entry
                    cout << "Search cancelled. No customer loaded." << endl;
                    return 0;
                }
                if ( selection == 'y') {                        // This is the correct customer
                    searchFound = true;                         // Note it
                    Loaded.arrayPos = i;                        // Remember the position of this customer in the array
                    return 0;
                }
            // Keep going if it's not correct
            }
            // No match found
        }
        
    }   // End if USERMODE
    
    cout << "No results matching your search found." << endl;   // Don't want to give up any customer information
    return 0;
}

int addCustomer() {
    string strData;     // Input
    int intData;        // Input
    string nameData;    // Input to combine first and last name
    string name;        // Customer's name
    int acctNumber;     // Customer's account number
    //int routNumber;   // Customers routing number, based on where account was opened
    string state;       // US State account was opened in
    long double balance;// Customer's account balance
    long phone;         // Customer's phone number
    string status;      // This determines the status of the account: active, suspended, or closed.
    int pin;            // PIN used for authentication
    string type;        // Checking or savings account
    
    char selection;     // Yes/No input
    int arrPosition;    // Position in the array in which the customer will be added
    
    // Check privileges
    if (USERMODE > 2) {                     // If we are not a banker or a manager
        cout << "Invalid option." << endl;  // Don't give away any information, for security
        return 0;                           // Return to the menu
    }
    
    // Get user Input
    cout << "Adding new customer" << endl;
    cout << "Account number: ";
    cin >> acctNumber;
    cin.ignore(1000,'\n');
    for (int i = 0; i < MAXCUSTOMERS; i++) {    // Check to see if that account number is in use
        if (intData == arr[i].acctNumber) {     // If we find a match
            cout << "That account number cannot be used." << endl;  // Don't give out any info
            return 0;
        }
    }
    
    for (int i = 0; i < MAXCUSTOMERS; i++) {    // Find the first empty position in the array to add the customer to
        if (arr[i].name == "EMPTY") {             // doing so randomizes the position in the array the customers are located
            arrPosition = i;                       // and makes it possible to pass array positions between inter-bank systems
            break;
        }
        if (i == MAXCUSTOMERS-1) {              // If we don't have any room, do this.
            cout << "Error: Database full, no new customers can be added" << endl;
            return 0;
        }
    }
    
    cout << "Name: ";
    cin >> name;
    nameData += name;       // This puts the name in "Firstname Lastname" format
    nameData += " ";
    cin >> strData;
    nameData += strData;
    cin.ignore(1000,'\n');
    
    
    //cout << "Routing number state: ";
    
    cout << "Opening deposit amount: $";
    cin >> balance;
    if (balance < OPENINGDEPOSIT) {         // Opening deposit must be some amount, set by bank policy
        cout << "Opening deposit must be at least $" << OPENINGDEPOSIT << endl;
        return 0;
    }
    cin.ignore(1000,'\n');
    
    cout << "Account type (checking/savings): ";
    cin >> type;
    if (type == "checking" || type == "c") {     // It's okay to write to the struct here since this choice doesn't do anything on its own.
        arr[arrPosition].acctType = "checking";
    } else if (type == "savings" || type == "s") {
        arr[arrPosition].acctType = "savings";
    } else {
        cout << "Invalid account type (checking/savings)" << endl;
        return 0;
    }
    
    
    cout << "Phone number: ";
    cin >> phone;
    cin.ignore(1000,'\n');
    
    cout << "Allow account to be overdrawn? ($" << OVERDRAFTFEE << ") (y/n)? ";
    cin >> selection;
    cin.ignore(1000,'\n');
    if (selection == 'y') {     // It's okay to write to the struct here since this choice doesn't do anything on its own.
        arr[arrPosition].overdraftable = true;  // Converting (y/n) to boolean value
    } else {
        arr[arrPosition].overdraftable = false;
    }
    
    cout << "PIN: ";
    cin >> pin;
    cin.ignore(1000,'\n');
    if (pin > 9999) {           // PIN has to be 0000 to 9999
        cout << "PIN cannot exceed 4 digits" << endl;
        return 0;
    } else if (pin < 1) {       // Making sure no negatives are used
        cout << "PIN cannot be negative" << endl;
        return 0;
    }
    
    // Run confirmation before adding customer
    cout << "Name     : " << nameData << endl;
    cout << "Account  : " << acctNumber << endl;
    cout << "Type     : " << arr[arrPosition].acctType;
    cout << "Balance  : $" << balance << endl;
    cout << "Phone    : " << phone << endl;
    if (selection == 'y') {
        cout << "Overdraft: Yes" << endl;
    } else {
        cout << "Overdraft: No" << endl;
    }
    cout << "PIN      : " << pin << endl;
    cout << "Confirm customer? (y/n) ";
    cin >> selection;
    cin.ignore(1000,'\n');
    
    if (selection == 'y') {                         // If yes, write to the struct the variables we just entered
        arr[arrPosition].name = nameData;
        arr[arrPosition].acctNumber = acctNumber;
        arr[arrPosition].balance = balance;
        arr[arrPosition].phone = phone;
        arr[arrPosition].pin = pin;
        arr[arrPosition].status = "active";         // Make the account active
        
        // Load the account and tell the user
        Loaded.activeAccount = arr[arrPosition].acctNumber;
        Loaded.activeName = arr[arrPosition].name;
        Loaded.arrayPos = arrPosition;
        Log.noTransactions++;                       // Count the opening deposit as a transaction
        Log.transactionAmount += balance;           // Since we required an opening deposit, it is added here
        cout << "Account for " << arr[arrPosition].name << " added and loaded." << endl;
        Log.writeToDatabase = true;
        return 0;
    }
    cout << "Add customer cancelled" << endl;
    
    return 0;
}

int removeCustomer() {
    char selection;

    // Check privileges
    if (USERMODE > 2) {                     // If we are not a banker or a manager
        cout << "Invalid option." << endl;  // Don't give away any information, for security
        return 0;                           // Return to the menu
    }
    
    if (Loaded.arrayPos < 0) {                      // Check if there is an account loaded
        cout << "No active customer, please search first." << endl;
        return 0;
    }
    if (arr[Loaded.arrayPos].balance != 0) {        // Warn if we have a non-zero balance
        cout << "Note: This account does not have a zero balance ($" << arr[Loaded.arrayPos].balance << ")" << endl;
    }
    cout << "This account is " << arr[Loaded.arrayPos].status << ". Do you want it active, suspended, or closed? (a/s/c) ";
    cin >> selection;
    cin.ignore(1000,'\n');
    if (selection == 'a') {
        arr[Loaded.arrayPos].status = "active";
        cout << "Account active." << endl;
        Log.writeToDatabase = true;
        return 0;
    } else if (selection == 's') {
        arr[Loaded.arrayPos].status = "suspended";
        cout << "Account suspended." << endl;
        Log.writeToDatabase = true;
        return 0;
    } else if (selection == 'c') {
        arr[Loaded.arrayPos].status = "closed";
        cout << "Account closed." << endl;
        Log.writeToDatabase = true;
        return 0;
    }
    cout << "No changes made." << endl;
    return 0;
}
    
int creditApplication() {
    double loanInterestOffer;
    double loanLimitOffer;
    
    // Check privileges
    if (USERMODE > 2) {                     // If we are not a banker or a manager
        cout << "Invalid option." << endl;  // Don't give away any information, for security
        return 0;                           // Return to the menu
    }
    if (Loaded.arrayPos < 0) {                      // Check if there is an account loaded
        cout << "No active customer, please search first." << endl;
        return 0;
    }
    if (arr[Loaded.arrayPos].status == "closed") {  // Check if the account is closed
        cout << "This account is closed, no transactions can occur." << endl;
        return 0;
    }
    
    int salary;
    int months;
    char selection;
    
    cout << "Enter annual salary: " ;
    cin >> salary;
    cout << "Enter number of months at your employer: ";
    cin >> months;
    loanLimitOffer = (salary/12);                                           // Calculate the loan limit
    loanInterestOffer = 10-((loanLimitOffer*0.5) / (0.5*salary));           // Calculate the interest amount
    cout << "Your loan will be $" << loanLimitOffer << " at " << loanInterestOffer << "%. Proceed? (y/n) ";
    cin >> selection;
    if (selection == 'y') {                                                 // Proceed with the loan distribution
        if (loanInterestOffer > arr[Loaded.glPos].balance) {                // Check that the GL has enough balance
            cout << "Error: Cannot process loan for that amount" << endl;   // General ledger doesn't have balance for the loan
            return 0;
        }
        arr[Loaded.glPos].balance -= loanLimitOffer;                        // Remove money from GL
        arr[Loaded.arrayPos].balance += loanLimitOffer;                     // Add money to acccount
        cout << "$" << loanLimitOffer << " deposited to account " << arr[Loaded.arrayPos].acctNumber << endl;
        cout << "Current balance: $" << arr[Loaded.arrayPos].balance << endl;
    }
    Log.writeToDatabase = true;
    return 0;
}

int viewTotals() {
    // Check privileges
    if (USERMODE > 3) {                     // If we are not a manager
        cout << "Invalid option." << endl;  // Don't give away any information, for security
        return 0;                           // Return to the menu
    }

    double totalActiveAccounts = 0;
    double totalSuspendedAccounts = 0;
    double totalClosedAccounts = 0;
    double emptyAccounts = 0;
    double totalDeposits = 0;

    // Find balances
    for (int i = 0; i < MAXCUSTOMERS; i++) {
        if (arr[i].name != "EMPTY") {
        
            if (arr[i].status == "active") {
                totalActiveAccounts++;
            }
            if (arr[i].status == "suspended") {
                totalSuspendedAccounts++;
            }
            if (arr[i].status == "closed") {
                totalClosedAccounts++;
            }
            
            if (arr[i].status != "gl") {
                totalDeposits += arr[i].balance;
            }
        }
        if (arr[i].name == "EMPTY") {
            emptyAccounts++;
        }
    }
    // Output results
    cout << "Bank totals as of " << currentDateTime() << endl;
    cout << "Active accounts:    " << totalActiveAccounts << endl;
    cout << "Suspended accounts: " << totalSuspendedAccounts << endl;
    cout << "Closed accounts:    " << totalClosedAccounts << endl;
    cout << "Total balances:     $" << setprecision(2) << fixed << showpoint << totalDeposits << endl;
    cout << "Average balance:    $" << totalDeposits / (totalActiveAccounts + totalSuspendedAccounts + totalClosedAccounts) << endl;
    cout << "General ledger:     $" << arr[Loaded.glPos].balance << endl;
    cout << "Retention rate:     " << ((totalActiveAccounts + totalSuspendedAccounts) / (totalActiveAccounts + totalSuspendedAccounts + totalClosedAccounts))*100 << "%" << endl;
    cout << "Available accounts: " << noshowpoint << setprecision(0) << emptyAccounts << endl; // Turn off decimals here also
    cout << "Daily transactions: " << Log.noTransactions << endl;
    return 0;
}

int viewLog() {
    // Check privileges
    if (USERMODE > 3) {                     // If we are not a manager
        cout << "Invalid option." << endl;  // Don't give away any information, for security
        return 0;                           // Return to the menu
    }

    cout << "Log for " << Log.loadTime << "to " << currentDateTime() << endl;
    if (Log.noTransactions == 1) {      // Check to pluralize "transactions" or not, small detail
        cout << Log.noTransactions << " transaction totalling $" << Log.transactionAmount << endl;
    } else {
        cout << Log.noTransactions << " transactions totalling $" << Log.transactionAmount << endl;
    }
    return 0;
}

int balanceInquiry() {
    if (Loaded.arrayPos < 0) {                          // Check if there is an account loaded
        cout << "No active customer, please search first." << endl;
        return 0;
    }
    cout << "Balances for " << arr[Loaded.arrayPos].name << "" << endl;
        if (arr[Loaded.arrayPos].status == "closed") {  // Check if the account is closed
            cout << "Account " << arr[Loaded.arrayPos].acctNumber << ": $" << arr[Loaded.arrayPos].balance << " (Account closed)" << endl;
            return 0;
        }
    cout << "Account " << arr[Loaded.arrayPos].acctNumber << ": $" << arr[Loaded.arrayPos].balance << endl;
    return 0;
}

int depositFunds() {
    double deposit;
    char selection;
    if (Loaded.arrayPos < 0) {
        cout << "No active customer, please search first." << endl;
        return 0;
    }
    if (arr[Loaded.arrayPos].status == "closed") {  // Check if the account is closed
        cout << "This account is closed, no transactions can occur." << endl;
        return 0;
    }
    
    cout << "Enter amount to be deposited: $";
    cin >> deposit;
    cout << "Confirm $" << deposit << " (y/n)? ";
    cin >> selection;
    if (selection == 'y') {
        arr[Loaded.arrayPos].balance += deposit;
        cout << "Deposit accepted. Current balance: $" << arr[Loaded.arrayPos].balance << endl;
        Log.noTransactions++;
        Log.transactionAmount += deposit;
        Log.writeToDatabase = true;
        return 0;
    }
    cout << "Deposit cancelled." << endl;
    return 0;
}

int withdrawFunds() {
    long double withdrawal;;
    char selection;
    if (Loaded.arrayPos < 0) {                          // Check if there is an account loaded
        cout << "No active customer, please search first." << endl;
        return 0;
    }
    
    if (arr[Loaded.arrayPos].status == "closed") {      // Check if the account is closed
        cout << "This account is closed, no transactions can occur." << endl;
        return 0;
    }
    
    cout << "Enter amount to be withdrawn: $";
    cin >> withdrawal;
    
    if (withdrawal > arr[Loaded.arrayPos].balance && arr[Loaded.arrayPos].overdraftable == false) {
        cout << "$" << withdrawal << " is greater than your balance of $"<< arr[Loaded.arrayPos].balance << ". You have previous elected to disalow overdrafting. Please see a banker to change this." << endl;
        return 0;
    }
    if (withdrawal > arr[Loaded.arrayPos].balance) {    // If you try to withdraw more than you have
        cout << "NOTE: $" << withdrawal << " is greater than your balance of " << arr[Loaded.arrayPos].balance << ", if you continue, you will be assessed a $" << OVERDRAFTFEE << " overdraft fee. Continue? (y/n) ";
        cin >> selection;
        if (selection == 'y') {                         // If you agree to the overdraft fee
            arr[Loaded.arrayPos].balance -= (withdrawal+OVERDRAFTFEE);
            arr[Loaded.glPos].balance += OVERDRAFTFEE;
            cout << "Withdrawal accepted. Current balance: $" << arr[Loaded.arrayPos].balance << endl;
            Log.noTransactions++;
            Log.transactionAmount -= withdrawal;
            return 0;
        } else {                                        // If you do not agree to the overdraft fee
            cout << "Withdrawal cancelled." << endl;
            return 0;
        }
    }
    cout << "Confirm $" << withdrawal << " (y/n)? ";
    cin >> selection;
    if (selection == 'y') {
        arr[Loaded.arrayPos].balance -= withdrawal;
        cout << "Withdrawal accepted. Current balance: $" << arr[Loaded.arrayPos].balance << endl;
        Log.noTransactions++;
        Log.transactionAmount -= withdrawal;
        Log.writeToDatabase = true;
        return 0;
    }
    cout << "Withdrawal cancelled." << endl;
    return 0;
}

int transferFunds() {
    // Check privileges
    if (USERMODE > 2) {                     // If we are not a banker or a manager
        cout << "Invalid option." << endl;  // Don't give away any information, for security
        return 0;                           // Return to the menu
    }

    double amount;
    int destination;
    char selection;
    if (Loaded.arrayPos < 0) {                          // Check if there is an account loaded
        cout << "No active customer, please search first." << endl;
        return 0;
    }
    if (arr[Loaded.arrayPos].status == "closed") {      // Check if the account is closed
        cout << "This account is closed, no transactions can occur." << endl;
        return 0;
    }
    cout << "Enter receiving account number: ";
    cin >> destination;
    cin.ignore(1000, '\n');
    cout << "Enter amount to transfer: $";
    cin >> amount;
    cin.ignore(1000, '\n');
    
    if (amount > arr[Loaded.arrayPos].balance) {    // If you try to withdraw more than you have
        cout << "Insufficient funds. Current balance: $" << arr[Loaded.arrayPos].balance << endl;
        return 0;
    }
    
    cout << "Confirm transfer of $" << amount << " to " << destination << " (y/n) ";
    cin >> selection;
    cin.ignore(1000, '\n');
    
    if (selection != 'y') {                           // If the user doesn't confirm, cancel here
        cout << "Transfer cancelled." << endl;
        return 0;
    }
    
    for (int i = 0; i < MAXCUSTOMERS; i++) {        // Search to make sure the account to transfer to is valid
        if (arr[i].acctNumber == destination && arr[i].status != "closed") {
            arr[Loaded.arrayPos].balance -= amount;
            arr[i].balance += amount;
            cout << "Transferred " << amount << " to " << destination <<". Your cuurrent balance: $" << arr[Loaded.arrayPos].balance << endl;
            return 0;
        }
    }
    cout << "Error: Cannot transfer" << endl;       // The destination account is closed or the account number is invalid
    Log.writeToDatabase = true;
    return 0;
}

void exitActiveAccount() {
    if (Loaded.arrayPos >= 0) {                          // Check if there is an account loaded
    cout <<  arr[Loaded.arrayPos].name << " saved and exited." << endl;
    Loaded.arrayPos = -1;
        Log.writeToDatabase == true;
    } else {
        cout << "No active customer." << endl;
    }
}

int stateRouting(string state) {
    int routNumber = 0;
    
    // These routing numbers should be set for the registered numbers of the bank
    // "state" can be passed through to autofill the routing number
    if (state == "de") {
        routNumber = 1;
    } else if (state == "pe") {
        routNumber = 2;
    } else if (state == "nj") {
        routNumber = 3;
    } else if (state == "ge") {
        routNumber = 4;
    } else if (state == "cn") {
        routNumber = 5;
    } else if (state == "ms") {
        routNumber = 6;
    } else if (state == "ma") {
        routNumber = 7;
    } else if (state == "sc") {
        routNumber = 8;
    } else if (state == "nh") {
        routNumber = 9;
    } else if (state == "vi") {
        routNumber = 10;
    } else if (state == "ny") {
        routNumber = 11;
    } else if (state == "nc") {
        routNumber = 12;
    } else if (state == "ri") {
        routNumber = 13;
    } else if (state == "ve") {
        routNumber = 14;
    } else if (state == "ke") {
        routNumber = 15;
    } else if (state == "te") {
        routNumber = 16;
    } else if (state == "oh") {
        routNumber = 17;
    } else if (state == "lo") {
        routNumber = 18;
    } else if (state == "in") {
        routNumber = 19;
    } else if (state == "mi") {
        routNumber = 20;
    } else if (state == "il") {
        routNumber = 21;
    } else if (state == "al") {
        routNumber = 22;
    } else if (state == "me") {
        routNumber = 23;
    } else if (state == "mo") {
        routNumber = 24;
    } else if (state == "ar") {
        routNumber = 25;
    }
    
    return routNumber;
}

int loadDatabase() {
    string filePath;
    ifstream inFile;
    string lineData;
    
    cout << "Enter database file path: ";
    cin >> filePath;
    cin.ignore(1000, '\n');
    
    Loaded.filePath = filePath;
    
    // Check validity
    while (true) {
        inFile.open(filePath.c_str()); // Open the file
        if (!inFile) {
            cout << "Invalid file: " << filePath << endl; // Kill program if invalid location
            cout << "Enter file path: ";
            cin >> filePath;
            continue;
        }
        cout << "Database found: " << filePath << endl; // Tell the user the location, for security
        break;
    }
    
    // Load array and parse data -- FUNCTIONIZE
    for (int i = 0; i < MAXCUSTOMERS; i++) {                         // Loop until the file runs out
        if (i >= MAXCUSTOMERS) {                    // Kill the program if the database is too small to fit everyone
            cout << "ERROR: Database too small (Max " << MAXCUSTOMERS << " entries)";
            kill();
        }
        
        if (inFile.eof()) {
            arr[i].name = "EMPTY";
            arr[i].acctNumber = -1;
            arr[i].routNumber = -1;
            arr[i].state = "";
            arr[i].balance = -0;
            arr[i].phone = -1;
            arr[i].status = "";
            arr[i].overdraftable = false;
            arr[i].pin = -1;
            arr[i].openDate = -1;
            continue;
        }
        
        inFile.ignore(256,'|');                     // Move pointer to the pipe |
        getline(inFile, lineData);                  // Grab the entire line (post-pipe)
        arr[i].name = lineData;                     // Populate the array
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].acctNumber = atoi(lineData.c_str()); // atoi: Convert lineData string to int for population
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].acctType = atoi(lineData.c_str());
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].routNumber = atoi(lineData.c_str());
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].state = lineData;
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].balance = atoi(lineData.c_str());
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].phone = atol(lineData.c_str());      // atol: Convert lineData string to long for population
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].status = lineData;
        
        if (lineData == "gl") {                     // If we find a general ledger entry, log its position here
            Loaded.glPos = i;
        }
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        if (lineData == "true") {                   // Convert overdraft option to boolean value
            arr[i].overdraftable = true;
        } else {
            arr[i].overdraftable = false;
        }
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].pin = atoi(lineData.c_str());
        
        inFile.ignore(256,'|');
        getline(inFile, lineData);
        arr[i].openDate = atoi(lineData.c_str());
    }
    return 0;
}

int debugArrayData() {   // If we want to see all the raw data stored in the array
    /*  THIS IS A DEBUG FUNCTION
        It represents a possible serious security breach. For this reason, there are odd checks to make sure it doesn't get
        turned on by accident.
        It cannot be used by any user of the three standard usermodes, a different usermode must be set and an option must be
        added to the menu.
     */
    
    char selection;
    
    if (USERMODE == 1 || USERMODE == 1 || USERMODE == 3) {
        return 0;   // Return without even acknowleding we are inside this debug function.
    }
    
    cout << "THIS IS A DEBUG FUNCTION. DO YOU WANT TO OUTPUT *ALL* DATABASE DATA (y/n)? ";
    cin >> selection;
    if (selection == 'y') { // Even in debug, we still want to confirm this is what we want to do
        // Begin output
        for (int i = 0; i < MAXCUSTOMERS; i++) {
            cout << i << " nam : " << arr[i].name << endl;
            cout << i << " acc : "  << arr[i].acctNumber << endl;
            cout << i << " rou : "  << arr[i].routNumber << endl;
            cout << i << " sta : "  << arr[i].state << endl;
            cout << i << " bal : "  << arr[i].balance << endl;
            cout << i << " pho : "  << arr[i].phone << endl;
            cout << i << " stu : "  << arr[i].status << endl;
            cout << i << " ove : "  << arr[i].overdraftable << endl;
            cout << i << " pin : "  << arr[i].pin << endl;
            cout << i << " ope : "  << arr[i].openDate << endl << endl;
        }
    }
    return 0;
}

int writeDatabase() {
    ofstream outFile;
    
    outFile.open("tempdb.txt", ios::binary);
    
    for (int i = 0; i < MAXCUSTOMERS; i++) {
        outFile << "name|" << arr[i].name << "\n";
        outFile << "acctNumber|" << arr[i].acctNumber << "\n";
        outFile << "acctType|" << arr[i].acctType << "\n";
        outFile << "routNumber|" << arr[i].routNumber << "\n";
        outFile << "state|" << arr[i].state << "\n";
        outFile << "balance|" << arr[i].balance << "\n";
        outFile << "phone|" << arr[i].phone << "\n";
        outFile << "status|" << arr[i].status << "\n";
        outFile << "overdraftable|" << arr[i].overdraftable << "\n";
        outFile << "pin|" << arr[i].pin << "\n";
        outFile << "opendate|" << arr[i].openDate << "\n";
    }
    string filePath = Loaded.filePath;
    
    remove(filePath.c_str());
    rename("tempdb.txt", filePath.c_str());
    
    Log.writeToDatabase = false;
    return 0;
}

int generalLedger() { // Check privileges
    if (USERMODE > 3) {                     // If we are not a manager
        cout << "Invalid option." << endl;  // Don't give away any information, for security
        return 0;                           // Return to the menu
    }
    
    cout << "The General Ledger balance is currently $" << arr[Loaded.glPos].balance << endl;
    
    // Expansions for GL options, like refunding fees.
    // Also, the manager should be the only one with access to the GL, for withdrawals and transfers and such. Other parts of
    // the program prevent access to the gl, and it won't appear in searches.
    
    return 0;

}
