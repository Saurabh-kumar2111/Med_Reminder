/*
 * ============================================================
 * Medicine.h - Medicine Class Declaration (Header File)
 * ============================================================
 * 
 * This file DECLARES the Medicine class:
 *   - What data members it has (private)
 *   - What methods it provides (public)
 * 
 * It does NOT contain the method implementations (those go in Medicine.cpp).
 * 
 * OOP Concepts:
 *   - Class definition
 *   - Encapsulation (private data, public interface)
 *   - Constructor declaration
 *   - Getter and Setter method declarations
 * ============================================================
 */

// ---- Include Guard ----
// Purpose: Prevents this header from being included more than once in the same
// compilation unit. Without this, if two files both #include "Medicine.h",
// the compiler would see the class defined twice and throw an error.
#ifndef MEDICINE_H
#define MEDICINE_H

#include <string>
#include <vector>

using namespace std;

class Medicine {

// ---- Private Section ----
// These members can ONLY be accessed from inside this class.
// Outside code must use the public getters and setters below.
private:
    string medicineId;           // Unique identifier (e.g., "MED001")
    string medicineName;         // Name of the medicine (e.g., "Paracetamol")
    int quantity;                // Available stock count
    string expiryDate;           // Expiry date in DD/MM/YYYY format
    string dosageInstructions;   // How to take it (e.g., "1 tablet after meals")
    vector<string> reminderTimes; // Scheduled dose times (e.g., {"08:00", "20:00"})
    string notes;                // Optional additional notes
    int lowStockThreshold;       // Minimum quantity before low-stock warning

// ---- Public Section ----
// These methods are accessible from anywhere.
// They form the "public interface" of the class.
public:

    // ---- Constructors ----

    // Default Constructor: Creates a Medicine with empty/default values.
    // Needed when we want to create a Medicine object and fill it later.
    Medicine();

    // Parameterized Constructor: Creates a Medicine with all values provided.
    // Used when we have all the data ready at creation time.
    Medicine(string id, string name, int qty, string expiry,
             string dosage, vector<string> reminders, string note);

    // ---- Getter Methods ----
    // These return the value of a private member (read access).
    // They are marked 'const' because they do NOT modify the object.
    string getMedicineId() const;
    string getMedicineName() const;
    int getQuantity() const;
    string getExpiryDate() const;
    string getDosageInstructions() const;
    vector<string> getReminderTimes() const;
    string getNotes() const;
    int getLowStockThreshold() const;

    // ---- Setter Methods ----
    // These allow controlled modification of private members (write access).
    void setMedicineId(string id);
    void setMedicineName(string name);
    void setQuantity(int qty);
    void setExpiryDate(string expiry);
    void setDosageInstructions(string dosage);
    void setReminderTimes(vector<string> reminders);
    void addReminderTime(string time);
    void setNotes(string note);
    void setLowStockThreshold(int threshold);

    // ---- Display Method ----
    // Prints all medicine details to the console in a formatted way.
    void display() const;
};

#endif // MEDICINE_H

