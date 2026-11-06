/*
 * ============================================================
 * ReminderManager.h - ReminderManager Class Declaration
 * ============================================================
 * 
 * Responsibilities:
 *   - Fetch current system time (HH:MM) and system date (DD/MM/YYYY)
 *   - Compare current time against stored medicine reminder times
 *   - Display formatted notifications for due doses
 *   - Prevent duplicate alerts using an alert cache (std::set)
 *   - Display a complete daily dose schedule
 *   - Provide simulated time checking for academic testing
 * 
 * OOP Concept: Association / Dependency
 *   ReminderManager has a reference to MedicineManager, allowing it
 *   to inspect medicine schedules without copying the data.
 * ============================================================
 */

#ifndef REMINDERMANAGER_H
#define REMINDERMANAGER_H

#include <string>
#include <vector>
#include <set>
#include "MedicineManager.h"

using namespace std;

class ReminderManager {

private:
    MedicineManager& medManager;     // Reference to the central medicine data
    set<string> triggeredAlerts;     // Cache to prevent duplicate alerts for the same dose

    // Internal helper: generates a unique key for an alert
    // e.g., "06/11/2026_08:00_MED001"
    string createAlertKey(const string& date, const string& time, const string& id) const;

public:
    // Constructor: Takes reference to the central MedicineManager
    ReminderManager(MedicineManager& manager);

    // Get current system time formatted as "HH:MM" (24-hour format)
    string getCurrentSystemTime() const;

    // Get current system date formatted as "DD/MM/YYYY"
    string getCurrentSystemDate() const;

    // Checks current system time against all medicine reminders
    // Returns number of alerts triggered
    int checkReminders();

    // Check reminders against a specific custom/simulated time (e.g., "08:00")
    // Very useful for academic demonstrations and unit testing!
    int checkRemindersForTime(const string& targetTime, const string& targetDate);

    // Displays the full daily dose schedule for all medicines
    void viewDailySchedule() const;

    // Automatic non-blocking check on menu loop (alerts only if a dose is due right now)
    void checkDueRemindersAuto();

    // Interactive sub-menu for dose reminders
    void showReminderMenu();

    // ============================================================
    // ---- REST API Methods (Backend Phase) ----
    // Non-interactive equivalents for use by the HTTP server.
    // ============================================================

    // Struct to hold one reminder alert entry
    struct ReminderAlert {
        string medicineId;
        string medicineName;
        string reminderTime;
        string dosage;
    };

    // Struct to hold one schedule entry (for daily schedule view)
    struct ScheduleEntry {
        string time;       // "HH:MM"
        string medicineId;
        string medicineName;
        string dosage;
    };

    // Returns list of due reminder alerts for now (or empty if none due)
    vector<ReminderAlert> apiCheckReminders();

    // Returns full daily schedule sorted by time
    vector<ScheduleEntry> apiGetDailySchedule() const;
};

#endif // REMINDERMANAGER_H
