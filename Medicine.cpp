/*
 * ============================================================
 * Medicine.cpp - Medicine Class Implementation
 * ============================================================
 * 
 * This file DEFINES (implements) all the methods that were
 * DECLARED in Medicine.h.
 * 
 * Notice the syntax:  ReturnType Medicine::methodName()
 * The "Medicine::" part tells the compiler these methods
 * belong to the Medicine class. This is called the
 * "scope resolution operator."
 * ============================================================
 */

#include "Medicine.h"   // Include our own header to know the class declaration
#include <iostream>      // For cout in the display() method

// ---- Default Constructor ----
// Initializes all members to safe default values.
// Without this, numeric members would contain garbage values.
Medicine::Medicine() {
    medicineId = "";
    medicineName = "";
    quantity = 0;
    expiryDate = "";
    dosageInstructions = "";
    notes = "";
    lowStockThreshold = 5;  // Default: warn when stock falls below 5
}

// ---- Parameterized Constructor ----
// Creates a fully initialized Medicine object in one step.
// Example usage: Medicine m("MED001", "Paracetamol", 20, "15/12/2027", "1 tablet after meals", {"08:00","20:00"}, "Store in cool place");
Medicine::Medicine(string id, string name, int qty, string expiry,
                   string dosage, vector<string> reminders, string note) {
    medicineId = id;
    medicineName = name;
    quantity = (qty >= 0) ? qty : 0;  // Prevent negative quantity
    expiryDate = expiry;
    dosageInstructions = dosage;
    reminderTimes = reminders;
    notes = note;
    lowStockThreshold = 5;  // Default threshold
}

// ============================================================
// Getter Methods - Provide read-only access to private data
// ============================================================
// The 'const' keyword after the parameter list means:
// "This method promises NOT to modify any member variables."
// This is good practice for getters — they only read, never write.

string Medicine::getMedicineId() const {
    return medicineId;
}

string Medicine::getMedicineName() const {
    return medicineName;
}

int Medicine::getQuantity() const {
    return quantity;
}

string Medicine::getExpiryDate() const {
    return expiryDate;
}

string Medicine::getDosageInstructions() const {
    return dosageInstructions;
}

vector<string> Medicine::getReminderTimes() const {
    return reminderTimes;
}

string Medicine::getNotes() const {
    return notes;
}

int Medicine::getLowStockThreshold() const {
    return lowStockThreshold;
}

// ============================================================
// Setter Methods - Provide controlled write access
// ============================================================
// Setters can include validation logic. For example, setQuantity()
// rejects negative values. This is a key benefit of encapsulation:
// direct access (medicine.quantity = -10) would allow invalid data,
// but the setter prevents it.

void Medicine::setMedicineId(string id) {
    medicineId = id;
}

void Medicine::setMedicineName(string name) {
    medicineName = name;
}

void Medicine::setQuantity(int qty) {
    if (qty >= 0) {
        quantity = qty;
    } else {
        cout << "  [Warning] Quantity cannot be negative. Value not changed." << endl;
    }
}

void Medicine::setExpiryDate(string expiry) {
    expiryDate = expiry;
}

void Medicine::setDosageInstructions(string dosage) {
    dosageInstructions = dosage;
}

void Medicine::setReminderTimes(vector<string> reminders) {
    reminderTimes = reminders;
}

// Adds a single reminder time to the existing list
// (rather than replacing the whole list)
void Medicine::addReminderTime(string time) {
    reminderTimes.push_back(time);
}

void Medicine::setNotes(string note) {
    notes = note;
}

void Medicine::setLowStockThreshold(int threshold) {
    if (threshold >= 0) {
        lowStockThreshold = threshold;
    } else {
        cout << "  [Warning] Threshold cannot be negative. Value not changed." << endl;
    }
}

// ============================================================
// Display Method
// ============================================================
// Prints all details of this medicine in a readable format.
// This is useful for "View All Medicines" and "Search Medicine."

void Medicine::display() const {
    cout << "----------------------------------------------" << endl;
    cout << "  Medicine ID    : " << medicineId << endl;
    cout << "  Name           : " << medicineName << endl;
    cout << "  Quantity       : " << quantity << endl;
    cout << "  Expiry Date    : " << expiryDate << endl;
    cout << "  Dosage         : " << dosageInstructions << endl;

    // Display reminder times (could be 0, 1, or many)
    cout << "  Reminder Times : ";
    if (reminderTimes.empty()) {
        cout << "None set";
    } else {
        for (size_t i = 0; i < reminderTimes.size(); i++) {
            cout << reminderTimes[i];
            if (i < reminderTimes.size() - 1) {
                cout << ", ";  // Comma between times, but not after the last one
            }
        }
    }
    cout << endl;

    cout << "  Notes          : " << (notes.empty() ? "None" : notes) << endl;
    cout << "  Low Stock Alert: Below " << lowStockThreshold << " units" << endl;
    cout << "----------------------------------------------" << endl;
}

