/*
 * ============================================================
 * ReminderManager.cpp - ReminderManager Class Implementation
 * ============================================================
 * 
 * Implements dose time retrieval, comparison, schedule viewing,
 * and duplicate alert prevention.
 * ============================================================
 */

#include "ReminderManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <limits>

// Constructor: Initializes the reference to MedicineManager
ReminderManager::ReminderManager(MedicineManager& manager) : medManager(manager) {}

// ============================================================
// Helper: Create unique alert key
// ============================================================
// Format: "DD/MM/YYYY_HH:MM_MEDID"
// Used to prevent firing the exact same alert multiple times
// within the same scheduled time interval.
string ReminderManager::createAlertKey(const string& date, const string& time, const string& id) const {
    return date + "_" + time + "_" + id;
}

// ============================================================
// Get Current System Time (HH:MM, 24-hour)
// ============================================================
// Uses C++ <ctime> library to read OS clock
string ReminderManager::getCurrentSystemTime() const {
    time_t now = time(nullptr);
    tm* ltm = localtime(&now);

    stringstream ss;
    ss << setfill('0') << setw(2) << ltm->tm_hour << ":"
       << setfill('0') << setw(2) << ltm->tm_min;
    return ss.str();
}

// ============================================================
// Get Current System Date (DD/MM/YYYY)
// ============================================================
string ReminderManager::getCurrentSystemDate() const {
    time_t now = time(nullptr);
    tm* ltm = localtime(&now);

    stringstream ss;
    ss << setfill('0') << setw(2) << ltm->tm_mday << "/"
       << setfill('0') << setw(2) << (ltm->tm_mon + 1) << "/"
       << (ltm->tm_year + 1900);
    return ss.str();
}

// ============================================================
// Check Reminders for Current Time
// ============================================================
int ReminderManager::checkReminders() {
    string curTime = getCurrentSystemTime();
    string curDate = getCurrentSystemDate();

    cout << "\n  [Clock] Current System Time: " << curTime 
         << " | Date: " << curDate << endl;

    return checkRemindersForTime(curTime, curDate);
}

// ============================================================
// Check Reminders for Specific Time (Actual or Simulated)
// ============================================================
int ReminderManager::checkRemindersForTime(const string& targetTime, const string& targetDate) {
    vector<Medicine>& medicines = medManager.getMedicines();
    int alertCount = 0;
    int alreadyAlertedCount = 0;

    for (const Medicine& med : medicines) {
        const vector<string>& reminders = med.getReminderTimes();

        for (const string& timeSlot : reminders) {
            if (timeSlot == targetTime) {
                string key = createAlertKey(targetDate, targetTime, med.getMedicineId());

                // Check if this dose was already alerted today at this time
                if (triggeredAlerts.find(key) != triggeredAlerts.end()) {
                    alreadyAlertedCount++;
                    continue; // Skip to avoid duplicate alert
                }

                // Trigger new dose reminder alert
                cout << "\n************************************************" << endl;
                cout << "         *** DOSE REMINDER ALERT! ***          " << endl;
                cout << "************************************************" << endl;
                cout << "  Time Scheduled : " << targetTime << endl;
                cout << "  Medicine Name  : " << med.getMedicineName() << " (" << med.getMedicineId() << ")" << endl;
                cout << "  Dosage         : " << med.getDosageInstructions() << endl;
                cout << "  Available Stock: " << med.getQuantity() << " unit(s)" << endl;
                if (!med.getNotes().empty()) {
                    cout << "  Notes          : " << med.getNotes() << endl;
                }
                cout << "  >> Action: Please take your scheduled dose now!" << endl;
                cout << "************************************************\n" << endl;

                // Cache key so it won't repeatedly alert in the same interval
                triggeredAlerts.insert(key);
                alertCount++;
            }
        }
    }

    if (alertCount == 0 && alreadyAlertedCount == 0) {
        cout << "  No medicine doses scheduled for " << targetTime << "." << endl;
    } else if (alertCount == 0 && alreadyAlertedCount > 0) {
        cout << "  [Info] All " << alreadyAlertedCount 
             << " dose(s) for " << targetTime << " have already been alerted." << endl;
    }

    return alertCount;
}

// ============================================================
// View Daily Dose Schedule
// ============================================================
// Collects all reminder times across all medicines and prints
// a sorted chronological timetable.
void ReminderManager::viewDailySchedule() const {
    cout << "\n==============================================" << endl;
    cout << "           Daily Dose Schedule                " << endl;
    cout << "==============================================" << endl;

    vector<Medicine>& medicines = medManager.getMedicines();

    if (medicines.empty()) {
        cout << "  No medicines registered in the system." << endl;
        return;
    }

    // Structure to represent a single scheduled dose event
    struct ScheduleItem {
        string time;
        string id;
        string name;
        string dosage;
    };

    vector<ScheduleItem> schedule;

    for (const Medicine& med : medicines) {
        for (const string& timeSlot : med.getReminderTimes()) {
            schedule.push_back({timeSlot, med.getMedicineId(), med.getMedicineName(), med.getDosageInstructions()});
        }
    }

    if (schedule.empty()) {
        cout << "  No reminder times have been set for any medicines." << endl;
        return;
    }

    // Sort items chronologically by time
    sort(schedule.begin(), schedule.end(), [](const ScheduleItem& a, const ScheduleItem& b) {
        return a.time < b.time;
    });

    cout << left << setw(10) << "TIME"
         << setw(12) << "ID"
         << setw(24) << "MEDICINE"
         << "DOSAGE" << endl;
    cout << string(65, '-') << endl;

    for (const auto& item : schedule) {
        cout << left << setw(10) << item.time
             << setw(12) << item.id
             << setw(24) << item.name
             << item.dosage << endl;
    }
    cout << string(65, '-') << endl;
    cout << "  Total doses scheduled per day: " << schedule.size() << endl;
}

// ============================================================
// Automatic Background Check (called on application menu loop)
// ============================================================
// Scans silently; only outputs if a dose is due at this very minute
void ReminderManager::checkDueRemindersAuto() {
    string curTime = getCurrentSystemTime();
    string curDate = getCurrentSystemDate();

    vector<Medicine>& medicines = medManager.getMedicines();
    for (const Medicine& med : medicines) {
        for (const string& timeSlot : med.getReminderTimes()) {
            if (timeSlot == curTime) {
                string key = createAlertKey(curDate, curTime, med.getMedicineId());
                if (triggeredAlerts.find(key) == triggeredAlerts.end()) {
                    cout << "\n************************************************" << endl;
                    cout << "  [LIVE ALERT] DOSE REMINDER DUE RIGHT NOW (" << curTime << ")!" << endl;
                    cout << "  Medicine: " << med.getMedicineName() << " (" << med.getMedicineId() << ")" << endl;
                    cout << "  Dosage  : " << med.getDosageInstructions() << endl;
                    cout << "  Stock   : " << med.getQuantity() << " unit(s) remaining" << endl;
                    cout << "************************************************\n" << endl;
                    triggeredAlerts.insert(key);
                }
            }
        }
    }
}

// ============================================================
// Interactive Reminder Menu (Option 6 in Main Menu)
// ============================================================
void ReminderManager::showReminderMenu() {
    int choice;
    do {
        cout << "\n==============================================" << endl;
        cout << "            Dose Reminder System              " << endl;
        cout << "==============================================" << endl;
        cout << "  1. Check Reminders for Current Time (" << getCurrentSystemTime() << ")" << endl;
        cout << "  2. View Full Daily Dose Schedule" << endl;
        cout << "  3. Simulate Reminder (Test with Custom Time)" << endl;
        cout << "  4. Reset Alerted Doses Cache" << endl;
        cout << "  5. Return to Main Menu" << endl;
        cout << "==============================================" << endl;
        cout << "  Enter choice (1-5): ";

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
                checkReminders();
                break;

            case 2:
                viewDailySchedule();
                break;

            case 3: {
                string testTime;
                cout << "  Enter time to simulate (HH:MM, 24-hr format): ";
                getline(cin, testTime);
                if (testTime.length() == 5 && testTime[2] == ':') {
                    cout << "\n  --- Simulating Clock at: " << testTime << " ---" << endl;
                    checkRemindersForTime(testTime, getCurrentSystemDate());
                } else {
                    cout << "  [Error] Invalid format. Please use HH:MM." << endl;
                }
                break;
            }

            case 4:
                triggeredAlerts.clear();
                cout << "  [Success] Reminder alert history cleared for today." << endl;
                break;

            case 5:
                cout << "  Returning to main menu..." << endl;
                break;

            default:
                cout << "  [Error] Invalid choice (1-5)." << endl;
                break;
        }
    } while (choice != 5);
}


// ============================================================
// REST API Methods — Non-interactive reminder data for HTTP server
// ============================================================

/*
 * apiCheckReminders()
 * --------------------
 * Returns a list of ReminderAlert structs for all doses due NOW
 * (matches current HH:MM). Uses the same duplicate-prevention
 * logic as the interactive checkReminders() method.
 */
vector<ReminderManager::ReminderAlert> ReminderManager::apiCheckReminders() {
    vector<ReminderAlert> alerts;
    string currentTime = getCurrentSystemTime();
    string currentDate = getCurrentSystemDate();
    const vector<Medicine>& medicines = medManager.getMedicines();

    for (const Medicine& med : medicines) {
        for (const string& reminderTime : med.getReminderTimes()) {
            if (reminderTime == currentTime) {
                string alertKey = createAlertKey(currentDate, currentTime, med.getMedicineId());
                // Only fire once per session per dose
                if (triggeredAlerts.find(alertKey) == triggeredAlerts.end()) {
                    triggeredAlerts.insert(alertKey);
                    ReminderAlert alert;
                    alert.medicineId   = med.getMedicineId();
                    alert.medicineName = med.getMedicineName();
                    alert.reminderTime = reminderTime;
                    alert.dosage       = med.getDosageInstructions();
                    alerts.push_back(alert);
                }
            }
        }
    }
    return alerts;
}

/*
 * apiGetDailySchedule()
 * ----------------------
 * Returns the full daily dose schedule as a sorted vector of
 * ScheduleEntry structs (sorted by time HH:MM ascending).
 */
vector<ReminderManager::ScheduleEntry> ReminderManager::apiGetDailySchedule() const {
    vector<ScheduleEntry> schedule;
    const vector<Medicine>& medicines = medManager.getMedicines();

    for (const Medicine& med : medicines) {
        for (const string& t : med.getReminderTimes()) {
            ScheduleEntry entry;
            entry.time        = t;
            entry.medicineId  = med.getMedicineId();
            entry.medicineName = med.getMedicineName();
            entry.dosage      = med.getDosageInstructions();
            schedule.push_back(entry);
        }
    }

    // Sort by time (string comparison works for HH:MM format)
    sort(schedule.begin(), schedule.end(),
         [](const ScheduleEntry& a, const ScheduleEntry& b) {
             return a.time < b.time;
         });

    return schedule;
}
