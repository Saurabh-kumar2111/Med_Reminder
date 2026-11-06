/*
 * ============================================================
 * Medicine Reminder & Inventory Management System
 * ============================================================
 * 
 * Project:  B.Tech Academic Project
 * Language: C++ with Object-Oriented Programming (OOP)
 * 
 * Description:
 *   A modular console application to manage medicine records,
 *   set dose reminders, track inventory, and monitor expiry.
 * ============================================================
 */

#include <iostream>
#include <string>
#include <limits>
#include <vector>
#include "Medicine.h"
#include "MedicineManager.h"
#include "ReminderManager.h"
#include "InventoryManager.h"

using namespace std;

// ---- Function: Display the Main Menu ----
void displayMenu() {
    cout << endl;
    cout << "==============================================" << endl;
    cout << "  Medicine Reminder & Inventory Management    " << endl;
    cout << "                System v1.0                   " << endl;
    cout << "==============================================" << endl;
    cout << "  1.  Register New Medicine                   " << endl;
    cout << "  2.  View All Medicines                      " << endl;
    cout << "  3.  Search Medicine                         " << endl;
    cout << "  4.  Update Medicine                         " << endl;
    cout << "  5.  Delete Medicine                         " << endl;
    cout << "  6.  Dose Reminder                           " << endl;
    cout << "  7.  Inventory Management                    " << endl;
    cout << "  8.  Expiry Monitoring                       " << endl;
    cout << "  9.  Save Records                            " << endl;
    cout << "  10. Exit                                    " << endl;
    cout << "==============================================" << endl;
    cout << "  Enter your choice (1-10): ";
}

// ---- Function: Get Validated Integer Input ----
int getValidatedChoice() {
    int choice;
    cin >> choice;

    if (cin.fail()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return -1;
    }

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return choice;
}

// ---- Main Function: Entry point of the program ----
int main() {
    int choice;

    // Core Managers
    MedicineManager medManager;
    ReminderManager reminderManager(medManager);
    InventoryManager inventoryManager(medManager);

    // Load persisted records from file on startup
    cout << "==============================================" << endl;
    cout << "  Medicine Reminder & Inventory Management    " << endl;
    cout << "              System v1.0                     " << endl;
    cout << "==============================================" << endl;
    medManager.loadFromFile();

    do {
        // Automatic in-app check: alerts if any dose is scheduled right now
        reminderManager.checkDueRemindersAuto();

        displayMenu();
        choice = getValidatedChoice();

        switch (choice) {
            case 1:
                medManager.registerMedicine();
                break;

            case 2:
                medManager.displayAllMedicines();
                break;

            case 3:
                medManager.searchMedicine();
                break;

            case 4:
                medManager.updateMedicine();
                break;

            case 5:
                medManager.deleteMedicine();
                break;

            case 6:
                reminderManager.showReminderMenu();
                break;

            case 7:
                inventoryManager.showInventoryMenu();
                break;

            case 8:
                inventoryManager.showExpiryMenu();
                break;

            case 9:
                cout << "\n>> Saving records..." << endl;
                if (medManager.saveToFile()) {
                    cout << ">> Records saved successfully to file!" << endl;
                    cout << ">> Total records saved: " << medManager.getMedicineCount() << endl;
                }
                break;

            case 10:
                cout << "\n>> Saving records before exit..." << endl;
                if (medManager.saveToFile()) {
                    cout << ">> Records saved successfully." << endl;
                }
                cout << ">> Exiting the system. Goodbye!" << endl;
                cout << ">> Thank you for using Medicine Reminder System." << endl;
                break;

            default:
                cout << "\n>> Invalid choice! Please enter a number between 1 and 10." << endl;
                break;
        }

    } while (choice != 10);

    return 0;
}
