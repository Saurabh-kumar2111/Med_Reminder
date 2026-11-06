/*
 * ============================================================
 * InventoryManager.h - InventoryManager Class Declaration
 * ============================================================
 * 
 * Responsibilities:
 *   - Monitor stock levels for all medicines
 *   - Add stock (restocking new purchases)
 *   - Reduce stock (tracking medicine usage / consumption)
 *   - Set custom low-stock thresholds per medicine
 *   - Display low-stock and out-of-stock alerts
 *   - Compare expiry dates with current system calendar (Phase 9)
 *   - Identify expired and near-expiry medicines (Phase 9)
 *   - Display health & safety expiry warnings (Phase 9)
 * 
 * OOP Concept: Single Responsibility Principle
 *   Inventory management logic is kept completely separate
 *   from scheduling/reminders and file handling.
 * ============================================================
 */

#ifndef INVENTORYMANAGER_H
#define INVENTORYMANAGER_H

#include <string>
#include <vector>
#include "MedicineManager.h"

using namespace std;

class InventoryManager {

private:
    MedicineManager& medManager;     // Reference to the central medicine data

    // Helper: Finds the index of a medicine by ID in MedicineManager
    int findMedicineIndex(const string& id) const;

public:
    // Constructor: Takes a reference to the central MedicineManager
    InventoryManager(MedicineManager& manager);

    // Displays a formatted table of all medicines and their stock statuses
    void displayStockSummary() const;

    // Adds newly purchased stock to an existing medicine
    void addStock();

    // Deducts consumed or used stock with underflow protection
    void reduceStock();

    // Configures the low-stock threshold for a specific medicine
    void updateThreshold();

    // Scans all medicines and displays low-stock and out-of-stock warnings
    void checkLowStockAlerts() const;

    // Sub-menu for Inventory Management (Main Menu Option 7)
    void showInventoryMenu();

    // ---- Phase 9: Expiry Monitoring Methods ----

    // Calculates days remaining until expiry (negative number = already expired)
    int calculateDaysToExpiry(const string& expiryDate) const;

    // Helper: Gets today's system date in DD/MM/YYYY
    string getCurrentSystemDate() const;

    // Displays full status report for all medicines (Expired / Near Expiry / Safe)
    void displayExpiryStatus() const;

    // Filters and displays only expired medicines with disposal warnings
    void showExpiredMedicines() const;

    // Filters and displays medicines expiring within a given threshold (default 30 days)
    void showNearExpiryMedicines(int daysThreshold = 30) const;

    // Sub-menu for Expiry Monitoring (Main Menu Option 8)
    void showExpiryMenu();

    // ============================================================
    // ---- REST API Methods (Backend Phase) ----
    // Non-interactive equivalents for use by the HTTP server.
    // ============================================================

    // Add stock to a medicine by ID. Returns false if ID not found or qty <= 0.
    bool apiAddStock(const string& id, int quantity, string& errMsg);

    // Reduce stock by ID. Returns false if ID not found, qty <= 0, or insufficient stock.
    bool apiReduceStock(const string& id, int quantity, string& errMsg);

    // Update low-stock threshold by ID. Returns false if ID not found or threshold < 0.
    bool apiUpdateThreshold(const string& id, int threshold, string& errMsg);

    // Returns a vector of medicines that are low/out of stock (qty <= threshold)
    vector<Medicine> apiGetLowStockMedicines() const;

    // Returns a vector of expired medicines (days <= 0)
    vector<Medicine> apiGetExpiredMedicines() const;

    // Returns a vector of medicines expiring within daysThreshold days
    vector<Medicine> apiGetNearExpiryMedicines(int daysThreshold = 30) const;
};

#endif // INVENTORYMANAGER_H
