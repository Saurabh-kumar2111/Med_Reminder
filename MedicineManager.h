/*
 * ============================================================
 * MedicineManager.h - MedicineManager Class Declaration
 * ============================================================
 * 
 * This class is the CENTRAL HUB of the system.
 * It manages a collection (vector) of Medicine objects.
 * 
 * OOP Concept: Composition
 *   MedicineManager "has-a" vector<Medicine>.
 *   It doesn't inherit from Medicine — it CONTAINS Medicine objects
 *   and provides operations to manage them.
 * 
 * Responsibilities:
 *   - Register new medicines (with user input and validation)
 *   - Display all medicines
 *   - Search, update, delete (coming in Phase 6)
 *   - Save to file and load from file (Phase 5)
 * ============================================================
 */

#ifndef MEDICINEMANAGER_H
#define MEDICINEMANAGER_H

#include <vector>
#include <string>
#include "Medicine.h"

using namespace std;

class MedicineManager {

private:
    vector<Medicine> medicines;  // The collection of all medicine records
    int nextIdNumber;            // Counter for auto-generating unique IDs
    string dataFileName;         // Name of the file for saving/loading data

    // ---- Private Helper Methods ----
    // These are internal utilities — not meant to be called from outside.
    // Keeping them private is part of encapsulation.

    // Generates the next unique ID like "MED001", "MED002", etc.
    string generateMedicineId();

    // Validates date format (DD/MM/YYYY) — basic structural check
    bool isValidDateFormat(const string& date) const;

    // Validates time format (HH:MM) — basic structural check
    bool isValidTimeFormat(const string& time) const;

    // Splits a comma-separated string into a vector of strings
    // Used to parse reminder times from the file (e.g., "08:00,20:00" → {"08:00","20:00"})
    vector<string> splitString(const string& str, char delimiter) const;

    // Extracts the numeric part from a medicine ID (e.g., "MED003" → 3)
    // Used when loading from file to set the ID counter correctly
    int extractIdNumber(const string& id) const;

    // Helper: Converts a string to lowercase for case-insensitive searching
    string toLower(string str) const;

    // Helper: Finds the index of a medicine by its unique ID (-1 if not found)
    int findMedicineIndexById(const string& id) const;

    // Helper: Finds the index of a medicine by its exact name (case-insensitive, -1 if not found)
    int findMedicineIndexByName(const string& name) const;

public:

    // ---- Constructor ----
    MedicineManager();

    // ---- Core Methods ----

    // Interactive registration: prompts user for all fields, validates, stores
    void registerMedicine();

    // Display all stored medicines
    void displayAllMedicines() const;

    // Search medicine by ID or by Name (Phase 6)
    void searchMedicine() const;

    // Update existing medicine details (Phase 6)
    void updateMedicine();

    // Delete a medicine record (Phase 6)
    void deleteMedicine();

    // Returns how many medicines are currently stored
    int getMedicineCount() const;

    // Provides access to the medicines vector (needed by other managers later)
    vector<Medicine>& getMedicines();

    // ---- File Handling Methods (Phase 5) ----

    // Saves all medicine records to the data file
    // Returns true if save was successful, false otherwise
    bool saveToFile() const;

    // Loads medicine records from the data file into the vector
    // Returns true if load was successful (or file doesn't exist yet)
    bool loadFromFile();

    // ============================================================
    // ---- REST API Methods (Backend Phase) ----
    // Non-interactive equivalents of the interactive CRUD methods.
    // These accept parameters directly (no cin) and return
    // structured results (no cout) — suitable for JSON API use.
    // ============================================================

    // Add a new medicine programmatically.
    // Returns the new medicine ID on success, or "" on failure.
    // errMsg is set to a human-readable error if validation fails.
    string apiAddMedicine(const string& name, int quantity,
                          const string& expiryDate,
                          const string& dosage,
                          const vector<string>& reminderTimes,
                          const string& notes,
                          int lowStockThreshold,
                          string& errMsg);

    // Get a single medicine by ID.
    // Returns pointer to the Medicine, or nullptr if not found.
    Medicine* apiGetMedicineById(const string& id);

    // Delete a medicine by ID (no confirmation prompt).
    // Returns true on success, false if ID not found.
    bool apiDeleteMedicine(const string& id);

    // Update one field of a medicine by ID.
    // fieldName: "name", "quantity", "expiry", "dosage", "notes", "threshold"
    // Returns true on success, false on invalid ID or bad value.
    bool apiUpdateMedicine(const string& id, const string& fieldName,
                           const string& newValue, string& errMsg);

    // Public wrappers for internal helpers needed by the API layer
    bool apiIsValidDate(const string& date) const;
    bool apiIsValidTime(const string& time) const;
};

#endif // MEDICINEMANAGER_H

