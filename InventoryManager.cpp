/*
 * ============================================================
 * InventoryManager.cpp - InventoryManager Class Implementation
 * ============================================================
 * 
 * Implements stock monitoring, restocking, usage deduction,
 * threshold setting, and low-stock alerts.
 * ============================================================
 */

#include "InventoryManager.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <ctime>
#include <sstream>

// Constructor: Initializes the reference to MedicineManager
InventoryManager::InventoryManager(MedicineManager& manager) : medManager(manager) {}

// ============================================================
// Helper: Find medicine index by ID (case-insensitive)
// ============================================================
int InventoryManager::findMedicineIndex(const string& id) const {
    vector<Medicine>& medicines = medManager.getMedicines();

    auto toLowerStr = [](string s) {
        for (char& c : s) c = tolower(static_cast<unsigned char>(c));
        return s;
    };

    string searchId = toLowerStr(id);
    for (size_t i = 0; i < medicines.size(); i++) {
        if (toLowerStr(medicines[i].getMedicineId()) == searchId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ============================================================
// Display Stock Summary
// ============================================================
void InventoryManager::displayStockSummary() const {
    cout << "\n==============================================" << endl;
    cout << "             Stock Level Summary              " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();

    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    cout << left << setw(10) << "ID"
         << setw(22) << "NAME"
         << setw(12) << "QUANTITY"
         << setw(14) << "THRESHOLD"
         << "STATUS" << endl;
    cout << string(68, '-') << endl;

    for (const Medicine& med : medicines) {
        int qty = med.getQuantity();
        int threshold = med.getLowStockThreshold();
        string status;

        if (qty == 0) {
            status = "[OUT OF STOCK]";
        } else if (qty <= threshold) {
            status = "[LOW STOCK]";
        } else {
            status = "[IN STOCK]";
        }

        cout << left << setw(10) << med.getMedicineId()
             << setw(22) << med.getMedicineName()
             << setw(12) << qty
             << setw(14) << threshold
             << status << endl;
    }
    cout << string(68, '-') << endl;
    cout << "  Total distinct medicines: " << medicines.size() << endl;
}

// ============================================================
// Add Newly Purchased Stock (Restock)
// ============================================================
void InventoryManager::addStock() {
    cout << "\n==============================================" << endl;
    cout << "            Add Purchased Stock               " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();
    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    string id;
    cout << "  Enter Medicine ID to restock: ";
    getline(cin, id);

    int idx = findMedicineIndex(id);
    if (idx == -1) {
        cout << "  [Error] Medicine with ID '" << id << "' not found." << endl;
        return;
    }

    Medicine& med = medicines[idx];
    cout << "  Selected: " << med.getMedicineName() 
         << " (Current Quantity: " << med.getQuantity() << ")" << endl;

    int addedQty;
    while (true) {
        cout << "  Enter quantity to add: ";
        cin >> addedQty;

        if (cin.fail() || addedQty <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  [Error] Please enter a positive number greater than 0.\n";
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
    }

    int oldQty = med.getQuantity();
    int newQty = oldQty + addedQty;
    med.setQuantity(newQty);

    // Save changes to file
    medManager.saveToFile();

    cout << "\n  [Success] Stock updated successfully!" << endl;
    cout << "  " << med.getMedicineName() << " (" << med.getMedicineId() << "): "
         << oldQty << " -> " << newQty << " units." << endl;
}

// ============================================================
// Reduce Stock (Medicine Used / Consumed)
// ============================================================
void InventoryManager::reduceStock() {
    cout << "\n==============================================" << endl;
    cout << "           Record Medicine Usage              " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();
    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    string id;
    cout << "  Enter Medicine ID: ";
    getline(cin, id);

    int idx = findMedicineIndex(id);
    if (idx == -1) {
        cout << "  [Error] Medicine with ID '" << id << "' not found." << endl;
        return;
    }

    Medicine& med = medicines[idx];
    int currentQty = med.getQuantity();

    cout << "  Selected: " << med.getMedicineName() 
         << " (Available Stock: " << currentQty << ")" << endl;

    if (currentQty == 0) {
        cout << "  [Warning] This medicine is completely OUT OF STOCK!" << endl;
        return;
    }

    int usedQty;
    while (true) {
        cout << "  Enter quantity used/consumed: ";
        cin >> usedQty;

        if (cin.fail() || usedQty <= 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  [Error] Please enter a positive number greater than 0.\n";
        } else if (usedQty > currentQty) {
            cout << "  [Error] Insufficient stock! Only " << currentQty << " unit(s) available.\n";
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
    }

    int newQty = currentQty - usedQty;
    med.setQuantity(newQty);

    // Save changes to file
    medManager.saveToFile();

    cout << "\n  [Success] Quantity deducted successfully!" << endl;
    cout << "  " << med.getMedicineName() << ": " << currentQty 
         << " - " << usedQty << " = " << newQty << " unit(s) remaining." << endl;

    // Immediate low-stock / out-of-stock check
    if (newQty == 0) {
        cout << "\n  [CRITICAL ALERT] " << med.getMedicineName() 
             << " is now OUT OF STOCK!" << endl;
    } else if (newQty <= med.getLowStockThreshold()) {
        cout << "\n  [LOW STOCK WARNING] " << med.getMedicineName() 
             << " has reached or fallen below threshold (" 
             << med.getLowStockThreshold() << " units)!" << endl;
    }
}

// ============================================================
// Update Low-Stock Threshold
// ============================================================
void InventoryManager::updateThreshold() {
    cout << "\n==============================================" << endl;
    cout << "       Update Low-Stock Alert Threshold       " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();
    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    string id;
    cout << "  Enter Medicine ID: ";
    getline(cin, id);

    int idx = findMedicineIndex(id);
    if (idx == -1) {
        cout << "  [Error] Medicine with ID '" << id << "' not found." << endl;
        return;
    }

    Medicine& med = medicines[idx];
    cout << "  Selected: " << med.getMedicineName() 
         << " (Current Alert Threshold: " << med.getLowStockThreshold() << " units)" << endl;

    int newThreshold;
    while (true) {
        cout << "  Enter new minimum threshold: ";
        cin >> newThreshold;

        if (cin.fail() || newThreshold < 0) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  [Error] Threshold must be a non-negative number.\n";
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
    }

    med.setLowStockThreshold(newThreshold);
    medManager.saveToFile();

    cout << "  [Success] Low-stock threshold for " << med.getMedicineName() 
         << " updated to " << newThreshold << " units." << endl;
}

// ============================================================
// Check Low-Stock Alerts
// ============================================================
void InventoryManager::checkLowStockAlerts() const {
    cout << "\n==============================================" << endl;
    cout << "             Low-Stock Warnings               " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();
    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    int warningCount = 0;

    for (const Medicine& med : medicines) {
        int qty = med.getQuantity();
        int threshold = med.getLowStockThreshold();

        if (qty == 0) {
            cout << "  [CRITICAL] " << left << setw(12) << med.getMedicineId()
                 << setw(20) << med.getMedicineName()
                 << "-> OUT OF STOCK (0 units available)" << endl;
            warningCount++;
        } else if (qty <= threshold) {
            cout << "  [WARNING]  " << left << setw(12) << med.getMedicineId()
                 << setw(20) << med.getMedicineName()
                 << "-> Stock: " << qty << " units (Threshold: " << threshold << ")" << endl;
            warningCount++;
        }
    }

    if (warningCount == 0) {
        cout << "  [OK] All medicines have healthy stock levels above their thresholds." << endl;
    } else {
        cout << "\n  Action needed: " << warningCount 
             << " medicine(s) require reordering/restocking." << endl;
    }
}

// ============================================================
// Interactive Inventory Sub-Menu (Main Menu Option 7)
// ============================================================
void InventoryManager::showInventoryMenu() {
    int choice;
    do {
        cout << "\n==============================================" << endl;
        cout << "            Inventory Management              " << endl;
        cout << "==============================================" << endl;
        cout << "  1. View Stock Summary & Status" << endl;
        cout << "  2. Add Purchased Stock (Restock)" << endl;
        cout << "  3. Record Used Medicine (Reduce Stock)" << endl;
        cout << "  4. Set Low-Stock Alert Threshold" << endl;
        cout << "  5. Check Low-Stock Warnings" << endl;
        cout << "  6. Return to Main Menu" << endl;
        cout << "==============================================" << endl;
        cout << "  Enter choice (1-6): ";

        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  [Error] Invalid choice." << endl;
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1:
                displayStockSummary();
                break;
            case 2:
                addStock();
                break;
            case 3:
                reduceStock();
                break;
            case 4:
                updateThreshold();
                break;
            case 5:
                checkLowStockAlerts();
                break;
            case 6:
                cout << "  Returning to main menu..." << endl;
                break;
            default:
                cout << "  [Error] Invalid choice (1-6)." << endl;
                break;
        }
    } while (choice != 6);
}

// ============================================================
// Phase 9: Expiry Monitoring Implementation
// ============================================================

// Helper: Formats today's date as DD/MM/YYYY
string InventoryManager::getCurrentSystemDate() const {
    time_t now = time(nullptr);
    tm* ltm = localtime(&now);
    stringstream ss;
    ss << setfill('0') << setw(2) << ltm->tm_mday << "/"
       << setfill('0') << setw(2) << (ltm->tm_mon + 1) << "/"
       << (ltm->tm_year + 1900);
    return ss.str();
}

// Calculates the signed difference in calendar days between today and expiry
// Negative = Expired, Positive = Days remaining until expiry
int InventoryManager::calculateDaysToExpiry(const string& expiryDate) const {
    if (expiryDate.length() != 10 || expiryDate[2] != '/' || expiryDate[5] != '/') {
        return 99999;
    }

    int day = stoi(expiryDate.substr(0, 2));
    int month = stoi(expiryDate.substr(3, 2));
    int year = stoi(expiryDate.substr(6, 4));

    // Normalize today to midnight for precise whole-day comparison
    time_t now = time(nullptr);
    tm nowTm = *localtime(&now);
    nowTm.tm_hour = 0;
    nowTm.tm_min = 0;
    nowTm.tm_sec = 0;
    time_t todayMidnight = mktime(&nowTm);

    // Normalize expiry date to midnight
    tm expTm = {};
    expTm.tm_mday = day;
    expTm.tm_mon = month - 1; // tm_mon is 0-indexed (Jan = 0)
    expTm.tm_year = year - 1900;
    expTm.tm_hour = 0;
    expTm.tm_min = 0;
    expTm.tm_sec = 0;
    expTm.tm_isdst = -1;
    time_t expMidnight = mktime(&expTm);

    double diffSeconds = difftime(expMidnight, todayMidnight);
    int days = static_cast<int>(diffSeconds / (60 * 60 * 24));
    return days;
}

// ------------------------------------------------------------
// Full Expiry Status Table
// ------------------------------------------------------------
void InventoryManager::displayExpiryStatus() const {
    cout << "\n==============================================" << endl;
    cout << "           Expiry Monitoring Report           " << endl;
    cout << "==============================================" << endl;
    cout << "  Today's System Date: " << getCurrentSystemDate() << endl;

    vector<Medicine>& medicines = medManager.getMedicines();
    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    int expiredCount = 0;
    int nearExpiryCount = 0;
    int safeCount = 0;

    cout << left << setw(10) << "ID"
         << setw(20) << "NAME"
         << setw(14) << "EXPIRY"
         << setw(16) << "DAYS LEFT"
         << "STATUS" << endl;
    cout << string(74, '-') << endl;

    for (const Medicine& med : medicines) {
        int days = calculateDaysToExpiry(med.getExpiryDate());
        string daysStr = to_string(days);
        string status;

        if (days < 0) {
            status = "[EXPIRED - DANGER]";
            daysStr = to_string(-days) + " days ago";
            expiredCount++;
        } else if (days <= 30) {
            status = "[NEAR EXPIRY - WARNING]";
            nearExpiryCount++;
        } else {
            status = "[VALID]";
            safeCount++;
        }

        cout << left << setw(10) << med.getMedicineId()
             << setw(20) << med.getMedicineName()
             << setw(14) << med.getExpiryDate()
             << setw(16) << daysStr
             << status << endl;
    }
    cout << string(74, '-') << endl;

    cout << "\n  Summary:" << endl;
    cout << "    - Safe & Valid Medicines : " << safeCount << endl;
    cout << "    - Medicines Nearing Expiry: " << nearExpiryCount << endl;
    cout << "    - Expired Medicines       : " << expiredCount << endl;
}

// ------------------------------------------------------------
// Filter: Expired Medicines Only
// ------------------------------------------------------------
void InventoryManager::showExpiredMedicines() const {
    cout << "\n==============================================" << endl;
    cout << "        Expired Medicines Alert Center        " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();
    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    int count = 0;
    for (const Medicine& med : medicines) {
        int days = calculateDaysToExpiry(med.getExpiryDate());
        if (days < 0) {
            if (count == 0) {
                cout << "\n  [CRITICAL HEALTH & SAFETY ALERT]" << endl;
                cout << "  The following medicine(s) have passed their expiry date!" << endl;
                cout << "  Expired medications can lose efficacy and become toxic." << endl;
                cout << "  >> DO NOT CONSUME! Please safely dispose of them immediately.\n" << endl;
            }
            cout << "  * " << med.getMedicineName() << " (" << med.getMedicineId() << ")" << endl;
            cout << "    Expired on : " << med.getExpiryDate() << " (" << -days << " days ago)" << endl;
            cout << "    Stock left : " << med.getQuantity() << " unit(s)" << endl;
            cout << "    Notes      : " << (med.getNotes().empty() ? "None" : med.getNotes()) << endl;
            cout << "  --------------------------------------------" << endl;
            count++;
        }
    }

    if (count == 0) {
        cout << "\n  [OK] None of your stored medicines are expired." << endl;
    } else {
        cout << "\n  Total expired medicines requiring disposal: " << count << endl;
    }
}

// ------------------------------------------------------------
// Filter: Medicines Nearing Expiry (Within 30 Days)
// ------------------------------------------------------------
void InventoryManager::showNearExpiryMedicines(int daysThreshold) const {
    cout << "\n==============================================" << endl;
    cout << "      Medicines Nearing Expiry Warning        " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();
    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    int count = 0;
    for (const Medicine& med : medicines) {
        int days = calculateDaysToExpiry(med.getExpiryDate());
        if (days >= 0 && days <= daysThreshold) {
            if (count == 0) {
                cout << "\n  [EARLY WARNING] Medicines expiring within " 
                     << daysThreshold << " days:" << endl;
                cout << "  Consider using current stock or planning for replacement.\n" << endl;
            }
            cout << "  * " << med.getMedicineName() << " (" << med.getMedicineId() << ")" << endl;
            cout << "    Expiry Date   : " << med.getExpiryDate() << " (in " << days << " days)" << endl;
            cout << "    Available Qty : " << med.getQuantity() << " unit(s)" << endl;
            cout << "  --------------------------------------------" << endl;
            count++;
        }
    }

    if (count == 0) {
        cout << "\n  [OK] No medicines are expiring within the next " 
             << daysThreshold << " days." << endl;
    } else {
        cout << "\n  Total near-expiry medicines: " << count << endl;
    }
}

// ------------------------------------------------------------
// Interactive Expiry Monitoring Sub-Menu (Option 8)
// ------------------------------------------------------------
void InventoryManager::showExpiryMenu() {
    int choice;
    do {
        cout << "\n==============================================" << endl;
        cout << "              Expiry Monitoring               " << endl;
        cout << "==============================================" << endl;
        cout << "  1. View Complete Expiry Status Report" << endl;
        cout << "  2. View Expired Medicines (Critical Alert)" << endl;
        cout << "  3. View Near-Expiry Medicines (Within 30 Days)" << endl;
        cout << "  4. Return to Main Menu" << endl;
        cout << "==============================================" << endl;
        cout << "  Enter choice (1-4): ";

        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  [Error] Invalid choice." << endl;
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1:
                displayExpiryStatus();
                break;
            case 2:
                showExpiredMedicines();
                break;
            case 3:
                showNearExpiryMedicines(30);
                break;
            case 4:
                cout << "  Returning to main menu..." << endl;
                break;
            default:
                cout << "  [Error] Invalid choice (1-4)." << endl;
                break;
        }
    } while (choice != 4);
}


// ============================================================
// REST API Methods — Non-interactive inventory & expiry ops
// ============================================================

/*
 * apiAddStock()
 * -------------
 * Adds `quantity` units to the medicine identified by `id`.
 * Returns false if the ID is not found or quantity is invalid.
 */
bool InventoryManager::apiAddStock(const string& id, int quantity, string& errMsg) {
    if (quantity <= 0) {
        errMsg = "Quantity to add must be greater than zero.";
        return false;
    }
    int idx = findMedicineIndex(id);
    if (idx == -1) {
        errMsg = "Medicine ID '" + id + "' not found.";
        return false;
    }
    vector<Medicine>& medicines = medManager.getMedicines();
    int current = medicines[idx].getQuantity();
    medicines[idx].setQuantity(current + quantity);
    medManager.saveToFile();
    errMsg = "";
    return true;
}

/*
 * apiReduceStock()
 * ----------------
 * Deducts `quantity` units from the medicine identified by `id`.
 * Protects against over-deduction (stock cannot go below zero).
 */
bool InventoryManager::apiReduceStock(const string& id, int quantity, string& errMsg) {
    if (quantity <= 0) {
        errMsg = "Quantity to reduce must be greater than zero.";
        return false;
    }
    int idx = findMedicineIndex(id);
    if (idx == -1) {
        errMsg = "Medicine ID '" + id + "' not found.";
        return false;
    }
    vector<Medicine>& medicines = medManager.getMedicines();
    int current = medicines[idx].getQuantity();
    if (quantity > current) {
        errMsg = "Cannot reduce by " + to_string(quantity) +
                 " — only " + to_string(current) + " units in stock.";
        return false;
    }
    medicines[idx].setQuantity(current - quantity);
    medManager.saveToFile();
    errMsg = "";
    return true;
}

/*
 * apiUpdateThreshold()
 * ---------------------
 * Updates the low-stock alert threshold for a medicine.
 */
bool InventoryManager::apiUpdateThreshold(const string& id, int threshold, string& errMsg) {
    if (threshold < 0) {
        errMsg = "Threshold cannot be negative.";
        return false;
    }
    int idx = findMedicineIndex(id);
    if (idx == -1) {
        errMsg = "Medicine ID '" + id + "' not found.";
        return false;
    }
    medManager.getMedicines()[idx].setLowStockThreshold(threshold);
    medManager.saveToFile();
    errMsg = "";
    return true;
}

/*
 * apiGetLowStockMedicines()
 * -------------------------
 * Returns a copy of medicines whose quantity <= lowStockThreshold.
 */
vector<Medicine> InventoryManager::apiGetLowStockMedicines() const {
    vector<Medicine> result;
    const vector<Medicine>& medicines = medManager.getMedicines();
    for (const Medicine& m : medicines) {
        if (m.getQuantity() <= m.getLowStockThreshold()) {
            result.push_back(m);
        }
    }
    return result;
}

/*
 * apiGetExpiredMedicines()
 * ------------------------
 * Returns medicines whose expiry date has already passed.
 */
vector<Medicine> InventoryManager::apiGetExpiredMedicines() const {
    vector<Medicine> result;
    const vector<Medicine>& medicines = medManager.getMedicines();
    for (const Medicine& m : medicines) {
        int days = calculateDaysToExpiry(m.getExpiryDate());
        if (days <= 0) {
            result.push_back(m);
        }
    }
    return result;
}

/*
 * apiGetNearExpiryMedicines()
 * ---------------------------
 * Returns medicines expiring within `daysThreshold` days (but not yet expired).
 */
vector<Medicine> InventoryManager::apiGetNearExpiryMedicines(int daysThreshold) const {
    vector<Medicine> result;
    const vector<Medicine>& medicines = medManager.getMedicines();
    for (const Medicine& m : medicines) {
        int days = calculateDaysToExpiry(m.getExpiryDate());
        if (days > 0 && days <= daysThreshold) {
            result.push_back(m);
        }
    }
    return result;
}
