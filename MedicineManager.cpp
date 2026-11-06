/*
 * ============================================================
 * MedicineManager.cpp - MedicineManager Class Implementation
 * ============================================================
 * 
 * This file implements all methods declared in MedicineManager.h.
 * 
 * Key logic in this file:
 *   - registerMedicine(): Step-by-step user input with validation
 *   - generateMedicineId(): Auto-increments IDs (MED001, MED002...)
 *   - isValidDateFormat(): Checks DD/MM/YYYY structure
 *   - isValidTimeFormat(): Checks HH:MM structure
 *   - saveToFile(): Writes all records to medicines.txt
 *   - loadFromFile(): Reads records from medicines.txt on startup
 * ============================================================
 */

#include "MedicineManager.h"
#include <iostream>
#include <iomanip>    // For setw, setfill (formatting the ID number)
#include <sstream>    // For stringstream (building the ID string)
#include <limits>     // For numeric_limits (input buffer clearing)
#include <fstream>    // For ofstream (write) and ifstream (read) — file handling

// ---- Constructor ----
// Start the ID counter at 1. The first medicine will be "MED001".
// Set the data file name — all saves and loads use this file.
MedicineManager::MedicineManager() {
    nextIdNumber = 1;
    dataFileName = "medicines.txt";
}

// ============================================================
// Private Helper: Generate Unique Medicine ID
// ============================================================
// Produces IDs like "MED001", "MED002", "MED003", etc.
//
// How it works:
//   1. We use a stringstream — think of it as a string builder.
//   2. We write "MED" followed by the counter, padded to 3 digits.
//   3. setfill('0') means pad with zeros: 1 → "001", 12 → "012"
//   4. setw(3) means the number should occupy at least 3 characters.
//   5. We increment the counter so the next call gives a different ID.

string MedicineManager::generateMedicineId() {
    stringstream ss;
    ss << "MED" << setfill('0') << setw(3) << nextIdNumber;
    nextIdNumber++;
    return ss.str();  // Convert the stringstream to a regular string
}

// ============================================================
// Private Helper: Validate Date Format (DD/MM/YYYY)
// ============================================================
// This performs a STRUCTURAL check — it verifies:
//   - Length is exactly 10 characters
//   - Characters at positions 2 and 5 are '/'
//   - All other characters are digits
//   - Day is 01-31, Month is 01-12, Year is 2000-2099
//
// Note: This doesn't check calendar correctness (e.g., Feb 30).
// For an academic project, structural validation is sufficient.

bool MedicineManager::isValidDateFormat(const string& date) const {
    // Check length
    if (date.length() != 10) return false;

    // Check separator positions
    if (date[2] != '/' || date[5] != '/') return false;

    // Check that all other characters are digits
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;  // Skip the '/' positions
        if (date[i] < '0' || date[i] > '9') return false;
    }

    // Extract day, month, year as integers for range checking
    int day = stoi(date.substr(0, 2));     // First 2 chars
    int month = stoi(date.substr(3, 2));   // Chars at index 3-4
    int year = stoi(date.substr(6, 4));    // Last 4 chars

    // Range validation
    if (day < 1 || day > 31) return false;
    if (month < 1 || month > 12) return false;
    if (year < 2000 || year > 2099) return false;

    return true;
}

// ============================================================
// Private Helper: Validate Time Format (HH:MM)
// ============================================================
// Checks:
//   - Length is exactly 5
//   - Character at position 2 is ':'
//   - Other characters are digits
//   - Hours 00-23, Minutes 00-59

bool MedicineManager::isValidTimeFormat(const string& time) const {
    if (time.length() != 5) return false;
    if (time[2] != ':') return false;

    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;  // Skip the ':'
        if (time[i] < '0' || time[i] > '9') return false;
    }

    int hours = stoi(time.substr(0, 2));
    int minutes = stoi(time.substr(3, 2));

    if (hours < 0 || hours > 23) return false;
    if (minutes < 0 || minutes > 59) return false;

    return true;
}

// ============================================================
// Register Medicine — Interactive User Input
// ============================================================
// This method walks the user through entering all medicine details.
// Each field is validated before moving to the next.
//
// Flow:
//   1. Auto-generate a unique ID
//   2. Ask for medicine name (must not be empty)
//   3. Ask for quantity (must be a positive integer)
//   4. Ask for expiry date (must be DD/MM/YYYY)
//   5. Ask for dosage instructions (must not be empty)
//   6. Ask for reminder times (HH:MM, one or more)
//   7. Ask for optional notes
//   8. Create Medicine object, add to vector, display confirmation

void MedicineManager::registerMedicine() {
    cout << "\n==============================================" << endl;
    cout << "         Register New Medicine                " << endl;
    cout << "==============================================" << endl;

    // ---- Step 1: Auto-generate unique ID ----
    string id = generateMedicineId();
    cout << "\n  Auto-generated Medicine ID: " << id << endl;

    // ---- Step 2: Medicine Name ----
    // We use getline() instead of cin>> because medicine names can have spaces
    // (e.g., "Vitamin B12"). cin>> stops at the first space; getline() reads
    // the entire line until Enter is pressed.
    string name;
    while (true) {
        cout << "  Enter Medicine Name: ";
        getline(cin, name);

        if (name.empty()) {
            cout << "  [Error] Medicine name cannot be empty. Try again.\n";
            continue;
        }

        // QA Feature: Check for existing duplicate medicine name
        int existingIdx = findMedicineIndexByName(name);
        if (existingIdx != -1) {
            cout << "\n  [Duplicate Warning] A medicine named '" 
                 << medicines[existingIdx].getMedicineName() 
                 << "' is already registered under ID: " 
                 << medicines[existingIdx].getMedicineId() << "!" << endl;
            cout << "  (Tip: If adding newly purchased stock, use Option 7 - Restock instead)" << endl;
            cout << "  Do you still want to register this as an additional entry? (y/n): ";
            char proceed;
            cin >> proceed;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (proceed != 'y' && proceed != 'Y') {
                cout << "  [Info] Registration cancelled to prevent duplicate entry.\n" << endl;
                return;
            }
        }
        break;
    }

    // ---- Step 3: Quantity ----
    // We read an integer. If the user types text or a negative number, we re-ask.
    int quantity;
    while (true) {
        cout << "  Enter Quantity: ";
        cin >> quantity;

        if (cin.fail() || quantity < 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  [Error] Please enter a valid positive number.\n";
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
    }

    // ---- Step 4: Expiry Date ----
    string expiryDate;
    while (true) {
        cout << "  Enter Expiry Date (DD/MM/YYYY): ";
        getline(cin, expiryDate);

        if (isValidDateFormat(expiryDate)) {
            break;
        }
        cout << "  [Error] Invalid date format. Use DD/MM/YYYY (e.g., 15/12/2027).\n";
    }

    // ---- Step 5: Dosage Instructions ----
    string dosage;
    while (true) {
        cout << "  Enter Dosage Instructions: ";
        getline(cin, dosage);

        if (!dosage.empty()) {
            break;
        }
        cout << "  [Error] Dosage instructions cannot be empty. Try again.\n";
    }

    // ---- Step 6: Reminder Times ----
    // The user can add one or more reminder times.
    // After each time, we ask if they want to add another.
    vector<string> reminderTimes;
    char addMore;
    cout << "\n  -- Set Dose Reminder Times --" << endl;

    do {
        string time;
        while (true) {
            cout << "  Enter reminder time (HH:MM, 24-hr format): ";
            getline(cin, time);

            if (isValidTimeFormat(time)) {
                reminderTimes.push_back(time);
                cout << "  [OK] Reminder set for " << time << endl;
                break;
            }
            cout << "  [Error] Invalid time format. Use HH:MM (e.g., 08:00, 14:30).\n";
        }

        cout << "  Add another reminder time? (y/n): ";
        cin >> addMore;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

    } while (addMore == 'y' || addMore == 'Y');

    // ---- Step 7: Optional Notes ----
    string notes;
    cout << "  Enter Notes (or press Enter to skip): ";
    getline(cin, notes);

    // ---- Step 8: Create Medicine object and store it ----
    // Using the parameterized constructor from Phase 3
    Medicine newMedicine(id, name, quantity, expiryDate, dosage, reminderTimes, notes);

    // push_back() adds the object to the end of the vector
    medicines.push_back(newMedicine);

    // ---- Step 9: Display confirmation ----
    cout << "\n==============================================" << endl;
    cout << "  Medicine Registered Successfully!           " << endl;
    cout << "==============================================" << endl;
    newMedicine.display();
    cout << "  Total medicines in system: " << medicines.size() << endl;
}

// ============================================================
// Display All Medicines
// ============================================================
// Iterates through the vector and calls display() on each Medicine.
// If the vector is empty, it shows a helpful message instead.

void MedicineManager::displayAllMedicines() const {
    cout << "\n==============================================" << endl;
    cout << "           All Medicine Records               " << endl;
    cout << "==============================================" << endl;

    if (medicines.empty()) {
        cout << "\n  No medicines registered yet." << endl;
        cout << "  Use option 1 to register a new medicine.\n" << endl;
        return;
    }

    cout << "  Total Records: " << medicines.size() << endl;

    // Range-based for loop (C++11 feature):
    // 'const Medicine& med' means we're iterating by reference (no copying)
    // and 'const' means we won't modify the medicine objects
    for (const Medicine& med : medicines) {
        med.display();
    }
}

// ============================================================
// Get Medicine Count
// ============================================================
int MedicineManager::getMedicineCount() const {
    return medicines.size();
}

// ============================================================
// Get Medicines Vector (by reference)
// ============================================================
// Returns a reference to the internal vector.
// Other managers (ReminderManager, InventoryManager) will use this
// to access medicine data without making copies.
vector<Medicine>& MedicineManager::getMedicines() {
    return medicines;
}

// ============================================================
// Private Helper: Split a String by Delimiter
// ============================================================
// Example: splitString("08:00,14:00,21:00", ',')
//          returns {"08:00", "14:00", "21:00"}
//
// How it works:
//   We use a stringstream and getline() with a custom delimiter.
//   getline() normally reads until '\n', but we can tell it to
//   stop at any character (in this case, the comma ',').

vector<string> MedicineManager::splitString(const string& str, char delimiter) const {
    vector<string> tokens;
    stringstream ss(str);    // Wrap the string in a stream
    string token;

    // Read one "token" at a time, stopping at 'delimiter'
    while (getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// ============================================================
// Private Helper: Extract Number from Medicine ID
// ============================================================
// "MED001" → 1,  "MED015" → 15,  "MED100" → 100
// We skip the first 3 characters ("MED") and convert the rest to int.
// This is used when loading from file to set the ID counter correctly
// so that new medicines get IDs that don't collide with existing ones.

int MedicineManager::extractIdNumber(const string& id) const {
    if (id.length() > 3) {
        return stoi(id.substr(3));  // Skip "MED", convert rest to integer
    }
    return 0;
}

// ============================================================
// Save To File — Write all records to medicines.txt
// ============================================================
// File format (one medicine per line):
//   ID|Name|Quantity|ExpiryDate|Dosage|Reminder1,Reminder2,...|Notes|LowStockThreshold
//
// Example line:
//   MED001|Paracetamol|20|15/12/2027|1 tablet after meals|08:00,20:00|Store in cool place|5
//
// How ofstream works:
//   1. Create an ofstream object and open the file
//   2. If the file doesn't exist, it gets created automatically
//   3. By default, ofstream OVERWRITES the file (we write ALL records each time)
//   4. We write fields separated by '|' so we can parse them when loading
//   5. close() flushes the data and releases the file

bool MedicineManager::saveToFile() const {
    ofstream outFile(dataFileName);  // Open file for writing

    // Check if the file opened successfully
    if (!outFile.is_open()) {
        cout << "  [Error] Could not open file '" << dataFileName << "' for saving." << endl;
        return false;
    }

    // Write each medicine as one line
    for (const Medicine& med : medicines) {
        // Write basic fields separated by '|'
        outFile << med.getMedicineId() << "|"
                << med.getMedicineName() << "|"
                << med.getQuantity() << "|"
                << med.getExpiryDate() << "|"
                << med.getDosageInstructions() << "|";

        // Write reminder times as comma-separated values
        vector<string> reminders = med.getReminderTimes();
        for (size_t i = 0; i < reminders.size(); i++) {
            outFile << reminders[i];
            if (i < reminders.size() - 1) {
                outFile << ",";  // Comma between times, not after the last
            }
        }

        // Write remaining fields
        outFile << "|" << med.getNotes()
                << "|" << med.getLowStockThreshold()
                << endl;  // Newline marks end of this record
    }

    outFile.close();  // Close the file
    return true;
}

// ============================================================
// Load From File — Read records from medicines.txt
// ============================================================
// This is called once at program startup.
//
// How ifstream works:
//   1. Create an ifstream object and try to open the file
//   2. If the file doesn't exist, that's OK — it just means no records yet
//   3. Read one line at a time with getline()
//   4. Parse each line by splitting on '|' to extract fields
//   5. Create a Medicine object from the parsed data
//   6. Add it to the medicines vector
//   7. Track the highest ID number seen so new IDs won't collide
//
// Error handling:
//   - File not found: Not an error — the user just hasn't saved yet
//   - Empty file: Not an error — treated the same as no file
//   - Malformed line: Skipped with a warning (doesn't crash the program)

bool MedicineManager::loadFromFile() {
    ifstream inFile(dataFileName);  // Open file for reading

    // If the file doesn't exist, that's fine — no records to load
    if (!inFile.is_open()) {
        cout << "  [Info] No existing data file found. Starting fresh." << endl;
        return true;  // Return true because this isn't an error
    }

    string line;
    int recordsLoaded = 0;
    int highestIdNumber = 0;  // Track the highest ID for the counter

    // Read the file line by line
    while (getline(inFile, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        // Split the line into fields using '|' as delimiter
        vector<string> fields = splitString(line, '|');

        // Each medicine record should have exactly 8 fields
        // ID|Name|Qty|Expiry|Dosage|Reminders|Notes|Threshold
        if (fields.size() < 8) {
            cout << "  [Warning] Skipping malformed record: " << line << endl;
            continue;
        }

        // Extract fields by position
        string id = fields[0];
        string name = fields[1];
        int quantity = stoi(fields[2]);
        string expiry = fields[3];
        string dosage = fields[4];

        // Parse reminder times (comma-separated within field 5)
        vector<string> reminders = splitString(fields[5], ',');

        string notes = fields[6];
        int threshold = stoi(fields[7]);

        // Create a Medicine object using the parameterized constructor
        Medicine med(id, name, quantity, expiry, dosage, reminders, notes);
        med.setLowStockThreshold(threshold);

        // Add to our vector
        medicines.push_back(med);
        recordsLoaded++;

        // Track the highest ID number we've seen
        int idNum = extractIdNumber(id);
        if (idNum > highestIdNumber) {
            highestIdNumber = idNum;
        }
    }

    inFile.close();

    // Set the counter so the next generated ID is one higher than the max
    // Example: if the highest loaded ID is MED005, nextIdNumber becomes 6
    nextIdNumber = highestIdNumber + 1;

    cout << "  [Info] Loaded " << recordsLoaded << " medicine record(s) from file." << endl;
    return true;
}

// ============================================================
// Phase 6: Record Management Helpers & Methods
// ============================================================

// Converts a string to lowercase for case-insensitive matching
string MedicineManager::toLower(string str) const {
    for (char &c : str) {
        c = tolower(static_cast<unsigned char>(c));
    }
    return str;
}

// Finds the index of a medicine by ID in the vector (-1 if not found)
int MedicineManager::findMedicineIndexById(const string& id) const {
    string searchId = toLower(id);
    for (size_t i = 0; i < medicines.size(); i++) {
        if (toLower(medicines[i].getMedicineId()) == searchId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Finds the index of a medicine by exact Name (case-insensitive, -1 if not found)
int MedicineManager::findMedicineIndexByName(const string& name) const {
    string searchName = toLower(name);
    for (size_t i = 0; i < medicines.size(); i++) {
        if (toLower(medicines[i].getMedicineName()) == searchName) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ------------------------------------------------------------
// Search Medicine (by ID or Name)
// ------------------------------------------------------------
void MedicineManager::searchMedicine() const {
    cout << "\n==============================================" << endl;
    cout << "             Search Medicine                  " << endl;
    cout << "==============================================" << endl;

    if (medicines.empty()) {
        cout << "\n  No medicines available to search." << endl;
        return;
    }

    cout << "  1. Search by Medicine ID" << endl;
    cout << "  2. Search by Medicine Name" << endl;
    cout << "  Enter your choice (1-2): ";

    int choice;
    cin >> choice;

    if (cin.fail() || (choice != 1 && choice != 2)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  [Error] Invalid choice." << endl;
        return;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    int matchCount = 0;

    if (choice == 1) {
        string searchId;
        cout << "  Enter Medicine ID to search (e.g., MED001): ";
        getline(cin, searchId);

        int idx = findMedicineIndexById(searchId);
        if (idx != -1) {
            cout << "\n  [Found 1 matching medicine]:" << endl;
            medicines[idx].display();
            matchCount = 1;
        }
    } else {
        string searchName;
        cout << "  Enter Medicine Name or keyword: ";
        getline(cin, searchName);

        string query = toLower(searchName);
        cout << "\n  --- Search Results ---" << endl;
        for (const Medicine& med : medicines) {
            if (toLower(med.getMedicineName()).find(query) != string::npos) {
                med.display();
                matchCount++;
            }
        }
    }

    if (matchCount == 0) {
        cout << "\n  [Notice] No medicine found matching your query." << endl;
    } else {
        cout << "\n  Total matches found: " << matchCount << endl;
    }
}

// ------------------------------------------------------------
// Update Medicine Details
// ------------------------------------------------------------
void MedicineManager::updateMedicine() {
    cout << "\n==============================================" << endl;
    cout << "             Update Medicine                  " << endl;
    cout << "==============================================" << endl;

    if (medicines.empty()) {
        cout << "\n  No medicines available to update." << endl;
        return;
    }

    string id;
    cout << "  Enter Medicine ID to update: ";
    getline(cin, id);

    int idx = findMedicineIndexById(id);
    if (idx == -1) {
        cout << "  [Error] Medicine with ID '" << id << "' not found." << endl;
        return;
    }

    Medicine& med = medicines[idx];
    cout << "\n  Current details for " << med.getMedicineId() << ":" << endl;
    med.display();

    cout << "\n  Select field to update:" << endl;
    cout << "  1. Medicine Name" << endl;
    cout << "  2. Quantity" << endl;
    cout << "  3. Expiry Date" << endl;
    cout << "  4. Dosage Instructions" << endl;
    cout << "  5. Reminder Times" << endl;
    cout << "  6. Notes" << endl;
    cout << "  7. Low Stock Threshold" << endl;
    cout << "  8. Cancel" << endl;
    cout << "  Enter choice (1-8): ";

    int fieldChoice;
    cin >> fieldChoice;
    if (cin.fail()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "  [Error] Invalid choice." << endl;
        return;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    switch (fieldChoice) {
        case 1: {
            string newName;
            while (true) {
                cout << "  Enter New Medicine Name: ";
                getline(cin, newName);
                if (!newName.empty()) {
                    med.setMedicineName(newName);
                    cout << "  [OK] Medicine name updated." << endl;
                    break;
                }
                cout << "  [Error] Name cannot be empty.\n";
            }
            break;
        }
        case 2: {
            int newQty;
            while (true) {
                cout << "  Enter New Quantity: ";
                cin >> newQty;
                if (cin.fail() || newQty < 0) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "  [Error] Please enter a valid positive number.\n";
                } else {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    med.setQuantity(newQty);
                    cout << "  [OK] Quantity updated." << endl;
                    break;
                }
            }
            break;
        }
        case 3: {
            string newExpiry;
            while (true) {
                cout << "  Enter New Expiry Date (DD/MM/YYYY): ";
                getline(cin, newExpiry);
                if (isValidDateFormat(newExpiry)) {
                    med.setExpiryDate(newExpiry);
                    cout << "  [OK] Expiry date updated." << endl;
                    break;
                }
                cout << "  [Error] Invalid format. Use DD/MM/YYYY.\n";
            }
            break;
        }
        case 4: {
            string newDosage;
            while (true) {
                cout << "  Enter New Dosage Instructions: ";
                getline(cin, newDosage);
                if (!newDosage.empty()) {
                    med.setDosageInstructions(newDosage);
                    cout << "  [OK] Dosage updated." << endl;
                    break;
                }
                cout << "  [Error] Dosage instructions cannot be empty.\n";
            }
            break;
        }
        case 5: {
            vector<string> newReminders;
            char addMore;
            cout << "\n  -- Set New Reminder Times --" << endl;
            do {
                string time;
                while (true) {
                    cout << "  Enter reminder time (HH:MM, 24-hr format): ";
                    getline(cin, time);
                    if (isValidTimeFormat(time)) {
                        newReminders.push_back(time);
                        cout << "  [OK] Reminder added: " << time << endl;
                        break;
                    }
                    cout << "  [Error] Invalid time format. Use HH:MM.\n";
                }
                cout << "  Add another reminder time? (y/n): ";
                cin >> addMore;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            } while (addMore == 'y' || addMore == 'Y');

            med.setReminderTimes(newReminders);
            cout << "  [OK] Reminder times updated." << endl;
            break;
        }
        case 6: {
            string newNotes;
            cout << "  Enter New Notes: ";
            getline(cin, newNotes);
            med.setNotes(newNotes);
            cout << "  [OK] Notes updated." << endl;
            break;
        }
        case 7: {
            int newThreshold;
            while (true) {
                cout << "  Enter New Low Stock Threshold: ";
                cin >> newThreshold;
                if (cin.fail() || newThreshold < 0) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "  [Error] Please enter a valid non-negative number.\n";
                } else {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    med.setLowStockThreshold(newThreshold);
                    cout << "  [OK] Low stock threshold updated." << endl;
                    break;
                }
            }
            break;
        }
        case 8:
            cout << "  [Info] Update cancelled." << endl;
            return;
        default:
            cout << "  [Error] Invalid option selected." << endl;
            return;
    }

    // Automatically persist update to file
    saveToFile();
    cout << "  [Info] Changes saved to file." << endl;
}

// ------------------------------------------------------------
// Delete Medicine Record
// ------------------------------------------------------------
void MedicineManager::deleteMedicine() {
    cout << "\n==============================================" << endl;
    cout << "             Delete Medicine                  " << endl;
    cout << "==============================================" << endl;

    if (medicines.empty()) {
        cout << "\n  No medicines available to delete." << endl;
        return;
    }

    string id;
    cout << "  Enter Medicine ID to delete: ";
    getline(cin, id);

    int idx = findMedicineIndexById(id);
    if (idx == -1) {
        cout << "  [Error] Medicine with ID '" << id << "' not found." << endl;
        return;
    }

    cout << "\n  Record to be deleted:" << endl;
    medicines[idx].display();

    char confirm;
    cout << "  Are you sure you want to delete this record? (y/n): ";
    cin >> confirm;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (confirm == 'y' || confirm == 'Y') {
        medicines.erase(medicines.begin() + idx);
        saveToFile();
        cout << "  [Success] Medicine " << id << " deleted successfully." << endl;
        cout << "  Remaining medicines in system: " << medicines.size() << endl;
    } else {
        cout << "  [Info] Deletion cancelled." << endl;
    }
}


// ============================================================
// REST API Methods — Non-interactive CRUD for the HTTP server
// ============================================================

/*
 * apiAddMedicine()
 * ----------------
 * Programmatic alternative to registerMedicine().
 * Validates all fields, checks for duplicate name, creates the
 * Medicine object, adds it to the vector, and saves to file.
 * Returns the new medicine ID on success, or "" on failure.
 */
string MedicineManager::apiAddMedicine(const string& name, int quantity,
                                        const string& expiryDate,
                                        const string& dosage,
                                        const vector<string>& reminderTimes,
                                        const string& notes,
                                        int lowStockThreshold,
                                        string& errMsg) {
    // --- Validation ---
    if (name.empty()) {
        errMsg = "Medicine name cannot be empty.";
        return "";
    }
    if (quantity < 0) {
        errMsg = "Quantity cannot be negative.";
        return "";
    }
    if (!isValidDateFormat(expiryDate)) {
        errMsg = "Invalid expiry date format. Use DD/MM/YYYY.";
        return "";
    }
    if (dosage.empty()) {
        errMsg = "Dosage instructions cannot be empty.";
        return "";
    }
    for (const string& t : reminderTimes) {
        if (!isValidTimeFormat(t)) {
            errMsg = "Invalid reminder time format: '" + t + "'. Use HH:MM.";
            return "";
        }
    }
    if (lowStockThreshold < 0) {
        errMsg = "Low-stock threshold cannot be negative.";
        return "";
    }
    // Check for duplicate name (case-insensitive)
    if (findMedicineIndexByName(name) != -1) {
        errMsg = "A medicine named '" + name + "' already exists.";
        return "";
    }

    // --- Create and store ---
    string newId = generateMedicineId();
    Medicine med(newId, name, quantity, expiryDate, dosage,
                 reminderTimes, notes);
    med.setLowStockThreshold(lowStockThreshold);
    medicines.push_back(med);
    saveToFile();

    errMsg = "";
    return newId;
}

/*
 * apiGetMedicineById()
 * ---------------------
 * Returns a pointer to the Medicine with the given ID,
 * or nullptr if no such medicine exists.
 */
Medicine* MedicineManager::apiGetMedicineById(const string& id) {
    int idx = findMedicineIndexById(id);
    if (idx == -1) return nullptr;
    return &medicines[idx];
}

/*
 * apiDeleteMedicine()
 * --------------------
 * Deletes the medicine with the given ID without asking for
 * confirmation (since the HTTP client already confirmed).
 * Returns true on success, false if ID not found.
 */
bool MedicineManager::apiDeleteMedicine(const string& id) {
    int idx = findMedicineIndexById(id);
    if (idx == -1) return false;
    medicines.erase(medicines.begin() + idx);
    saveToFile();
    return true;
}

/*
 * apiUpdateMedicine()
 * --------------------
 * Updates a single field of the medicine identified by id.
 * fieldName can be: "name", "quantity", "expiry", "dosage",
 *                   "notes", "threshold", "reminders"
 * newValue is always passed as a string; numeric fields are parsed.
 * Returns true on success, false on failure (errMsg set on failure).
 */
bool MedicineManager::apiUpdateMedicine(const string& id,
                                         const string& fieldName,
                                         const string& newValue,
                                         string& errMsg) {
    int idx = findMedicineIndexById(id);
    if (idx == -1) {
        errMsg = "Medicine ID '" + id + "' not found.";
        return false;
    }

    Medicine& med = medicines[idx];

    if (fieldName == "name") {
        if (newValue.empty()) { errMsg = "Name cannot be empty."; return false; }
        // Duplicate check (ignore self)
        int dupIdx = findMedicineIndexByName(newValue);
        if (dupIdx != -1 && dupIdx != idx) {
            errMsg = "Another medicine named '" + newValue + "' already exists.";
            return false;
        }
        med.setMedicineName(newValue);

    } else if (fieldName == "quantity") {
        try {
            int qty = stoi(newValue);
            if (qty < 0) { errMsg = "Quantity cannot be negative."; return false; }
            med.setQuantity(qty);
        } catch (...) {
            errMsg = "Invalid quantity value: '" + newValue + "'.";
            return false;
        }

    } else if (fieldName == "expiry") {
        if (!isValidDateFormat(newValue)) {
            errMsg = "Invalid date format. Use DD/MM/YYYY.";
            return false;
        }
        med.setExpiryDate(newValue);

    } else if (fieldName == "dosage") {
        if (newValue.empty()) { errMsg = "Dosage cannot be empty."; return false; }
        med.setDosageInstructions(newValue);

    } else if (fieldName == "notes") {
        med.setNotes(newValue);

    } else if (fieldName == "threshold") {
        try {
            int thr = stoi(newValue);
            if (thr < 0) { errMsg = "Threshold cannot be negative."; return false; }
            med.setLowStockThreshold(thr);
        } catch (...) {
            errMsg = "Invalid threshold value: '" + newValue + "'.";
            return false;
        }

    } else if (fieldName == "reminders") {
        // newValue is comma-separated times, e.g. "08:00,20:00"
        vector<string> times = splitString(newValue, ',');
        for (const string& t : times) {
            if (!isValidTimeFormat(t)) {
                errMsg = "Invalid time format: '" + t + "'. Use HH:MM.";
                return false;
            }
        }
        med.setReminderTimes(times);

    } else {
        errMsg = "Unknown field: '" + fieldName + "'. Valid fields: name, quantity, expiry, dosage, notes, threshold, reminders.";
        return false;
    }

    saveToFile();
    errMsg = "";
    return true;
}

/*
 * apiIsValidDate() / apiIsValidTime()
 * ------------------------------------
 * Public wrappers that expose the private validation helpers
 * so that server.cpp can validate JSON inputs before calling
 * other API methods.
 */
bool MedicineManager::apiIsValidDate(const string& date) const {
    return isValidDateFormat(date);
}

bool MedicineManager::apiIsValidTime(const string& time) const {
    return isValidTimeFormat(time);
}
