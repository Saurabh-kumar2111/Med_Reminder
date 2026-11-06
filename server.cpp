/*
 * ============================================================
 * server.cpp - Medicine Reminder REST API Server
 * ============================================================
 *
 * This is the C++ HTTP backend for the Medicine Reminder &
 * Inventory Management System. It wraps the existing console
 * application classes in a REST API layer using:
 *
 *   - cpp-httplib  (lib/httplib.h)  : Lightweight HTTP server
 *   - nlohmann/json (lib/json.hpp)  : JSON serialization
 *
 * The server listens on http://localhost:8080 and exposes:
 *
 *   GET    /api/medicines               — All medicines (JSON array)
 *   POST   /api/medicines               — Add new medicine
 *   GET    /api/medicines/:id           — Single medicine by ID
 *   PUT    /api/medicines/:id           — Update a field
 *   DELETE /api/medicines/:id           — Delete medicine
 *
 *   GET    /api/inventory/status        — Stock summary
 *   GET    /api/inventory/alerts        — Low-stock alert list
 *   POST   /api/inventory/restock       — Add stock {id, quantity}
 *   POST   /api/inventory/consume       — Reduce stock {id, quantity}
 *   PUT    /api/inventory/threshold     — Update threshold {id, threshold}
 *
 *   GET    /api/reminders/schedule      — Full daily dose schedule
 *   GET    /api/reminders/check         — Due alerts right now
 *
 *   GET    /api/expiry/report           — Full expiry status report
 *   GET    /api/expiry/expired          — Only expired medicines
 *   GET    /api/expiry/near-expiry      — Medicines near expiry (<=30 days)
 *
 *   GET    /api/health                  — Server health check
 *
 * Static frontend files are served from the frontend/ directory.
 * ============================================================
 */

// ---- Windows socket setup (must be before httplib) ----
#ifdef _WIN32
    // Target Windows 10 or later (required by cpp-httplib)
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0A00
    #endif
    #define WIN32_LEAN_AND_MEAN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include "lib/httplib.h"
#include "lib/json.hpp"

#include "MedicineManager.h"
#include "ReminderManager.h"
#include "InventoryManager.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

using namespace std;
using json = nlohmann::json;

// ============================================================
// JSON Helper: Convert a Medicine object to a JSON object
// ============================================================
json medicineToJson(const Medicine& med, InventoryManager& inv) {
    // Build reminder times array
    json reminderArr = json::array();
    for (const string& t : med.getReminderTimes()) {
        reminderArr.push_back(t);
    }

    // Calculate expiry info
    int daysToExpiry = inv.calculateDaysToExpiry(med.getExpiryDate());
    string expiryStatus;
    if (daysToExpiry < 0)       expiryStatus = "EXPIRED";
    else if (daysToExpiry <= 7) expiryStatus = "CRITICAL";
    else if (daysToExpiry <= 30) expiryStatus = "NEAR_EXPIRY";
    else                        expiryStatus = "SAFE";

    // Stock status
    string stockStatus;
    if (med.getQuantity() == 0)                              stockStatus = "OUT_OF_STOCK";
    else if (med.getQuantity() <= med.getLowStockThreshold()) stockStatus = "LOW_STOCK";
    else                                                      stockStatus = "IN_STOCK";

    return {
        {"id",                med.getMedicineId()},
        {"name",              med.getMedicineName()},
        {"quantity",          med.getQuantity()},
        {"expiryDate",        med.getExpiryDate()},
        {"daysToExpiry",      daysToExpiry},
        {"expiryStatus",      expiryStatus},
        {"dosage",            med.getDosageInstructions()},
        {"reminderTimes",     reminderArr},
        {"notes",             med.getNotes()},
        {"lowStockThreshold", med.getLowStockThreshold()},
        {"stockStatus",       stockStatus}
    };
}

// ============================================================
// CORS Helper: Add CORS headers to every response so that the
// browser-based frontend can call the API without being blocked.
// ============================================================
void addCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Accept");
}

// ============================================================
// main() — Server entry point
// ============================================================
int main() {

    cout << "=============================================" << endl;
    cout << "  Medicine Reminder REST API Server" << endl;
    cout << "  Port: 8080" << endl;
    cout << "=============================================" << endl;

    // ---- Initialize managers ----
    MedicineManager   medMgr;
    ReminderManager   remMgr(medMgr);
    InventoryManager  invMgr(medMgr);

    // Load existing data from medicines.txt
    if (medMgr.loadFromFile()) {
        cout << "[Info] Loaded " << medMgr.getMedicineCount()
             << " medicine(s) from file." << endl;
    } else {
        cout << "[Info] No existing data file — starting fresh." << endl;
    }

    // ---- Create HTTP server ----
    httplib::Server svr;

    // ---- Serve static frontend files ----
    // Any file in the frontend/ folder is served as-is.
    // e.g. http://localhost:8080/index.html
    svr.set_mount_point("/", "./frontend");

    // ============================================================
    // PREFLIGHT — Handle browser's CORS pre-flight OPTIONS request
    // ============================================================
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        res.status = 204;
    });

    // ============================================================
    // HEALTH CHECK  GET /api/health
    // ============================================================
    svr.Get("/api/health", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json body = {
            {"status",  "ok"},
            {"server",  "Medicine Reminder API"},
            {"version", "1.0"},
            {"medicines", medMgr.getMedicineCount()}
        };
        res.set_content(body.dump(2), "application/json");
    });

    // ============================================================
    // MEDICINES
    // ============================================================

    // GET /api/medicines — Return all medicines as JSON array
    svr.Get("/api/medicines", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const Medicine& med : medMgr.getMedicines()) {
            arr.push_back(medicineToJson(med, invMgr));
        }
        res.set_content(arr.dump(2), "application/json");
    });

    // POST /api/medicines — Add a new medicine
    // Expected JSON body:
    // {
    //   "name": "Paracetamol",
    //   "quantity": 50,
    //   "expiryDate": "31/12/2027",
    //   "dosage": "1 tablet twice daily",
    //   "reminderTimes": ["08:00", "20:00"],
    //   "notes": "Take after meals",
    //   "lowStockThreshold": 10
    // }
    svr.Post("/api/medicines", [&](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            json body = json::parse(req.body);

            // Extract fields (with defaults for optional ones)
            string name        = body.value("name", "");
            int    quantity    = body.value("quantity", 0);
            string expiry      = body.value("expiryDate", "");
            string dosage      = body.value("dosage", "");
            string notes       = body.value("notes", "");
            int    threshold   = body.value("lowStockThreshold", 5);

            vector<string> reminders;
            if (body.contains("reminderTimes") && body["reminderTimes"].is_array()) {
                for (auto& t : body["reminderTimes"]) {
                    reminders.push_back(t.get<string>());
                }
            }

            string errMsg;
            string newId = medMgr.apiAddMedicine(name, quantity, expiry,
                                                  dosage, reminders, notes,
                                                  threshold, errMsg);
            if (newId.empty()) {
                res.status = 400;
                res.set_content(json({{"error", errMsg}}).dump(), "application/json");
            } else {
                Medicine* med = medMgr.apiGetMedicineById(newId);
                res.status = 201;
                res.set_content(medicineToJson(*med, invMgr).dump(2), "application/json");
            }
        } catch (const exception& e) {
            res.status = 400;
            res.set_content(json({{"error", string("Invalid JSON: ") + e.what()}}).dump(),
                            "application/json");
        }
    });

    // GET /api/medicines/:id — Get one medicine by ID
    svr.Get(R"(/api/medicines/([A-Za-z0-9]+))", [&](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        string id = req.matches[1];
        Medicine* med = medMgr.apiGetMedicineById(id);
        if (!med) {
            res.status = 404;
            res.set_content(json({{"error", "Medicine '" + id + "' not found."}}).dump(),
                            "application/json");
        } else {
            res.set_content(medicineToJson(*med, invMgr).dump(2), "application/json");
        }
    });

    // PUT /api/medicines/:id — Update one field
    // Expected JSON body: { "field": "quantity", "value": "60" }
    svr.Put(R"(/api/medicines/([A-Za-z0-9]+))", [&](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        string id = req.matches[1];
        try {
            json body = json::parse(req.body);
            string field    = body.value("field", "");
            string newValue = body.value("value", "");

            if (field.empty()) {
                res.status = 400;
                res.set_content(json({{"error", "Missing 'field' in request body."}}).dump(),
                                "application/json");
                return;
            }

            string errMsg;
            bool ok = medMgr.apiUpdateMedicine(id, field, newValue, errMsg);
            if (!ok) {
                res.status = (errMsg.find("not found") != string::npos) ? 404 : 400;
                res.set_content(json({{"error", errMsg}}).dump(), "application/json");
            } else {
                Medicine* med = medMgr.apiGetMedicineById(id);
                res.set_content(medicineToJson(*med, invMgr).dump(2), "application/json");
            }
        } catch (const exception& e) {
            res.status = 400;
            res.set_content(json({{"error", string("Invalid JSON: ") + e.what()}}).dump(),
                            "application/json");
        }
    });

    // DELETE /api/medicines/:id — Delete a medicine
    svr.Delete(R"(/api/medicines/([A-Za-z0-9]+))", [&](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        string id = req.matches[1];
        if (!medMgr.apiDeleteMedicine(id)) {
            res.status = 404;
            res.set_content(json({{"error", "Medicine '" + id + "' not found."}}).dump(),
                            "application/json");
        } else {
            res.set_content(json({{"message", "Medicine " + id + " deleted successfully."}}).dump(),
                            "application/json");
        }
    });

    // ============================================================
    // INVENTORY
    // ============================================================

    // GET /api/inventory/status — Full stock summary
    svr.Get("/api/inventory/status", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const Medicine& med : medMgr.getMedicines()) {
            arr.push_back(medicineToJson(med, invMgr));
        }
        int inStock  = 0, lowStock = 0, outOfStock = 0;
        for (auto& m : arr) {
            string s = m["stockStatus"];
            if (s == "IN_STOCK")    inStock++;
            else if (s == "LOW_STOCK") lowStock++;
            else                    outOfStock++;
        }
        json body = {
            {"medicines",   arr},
            {"summary",     {{"total", medMgr.getMedicineCount()},
                             {"inStock", inStock},
                             {"lowStock", lowStock},
                             {"outOfStock", outOfStock}}}
        };
        res.set_content(body.dump(2), "application/json");
    });

    // GET /api/inventory/alerts — Only low/out-of-stock medicines
    svr.Get("/api/inventory/alerts", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const Medicine& med : invMgr.apiGetLowStockMedicines()) {
            arr.push_back(medicineToJson(med, invMgr));
        }
        res.set_content(arr.dump(2), "application/json");
    });

    // POST /api/inventory/restock — Add stock
    // Body: { "id": "MED001", "quantity": 50 }
    svr.Post("/api/inventory/restock", [&](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            json body    = json::parse(req.body);
            string id    = body.value("id", "");
            int quantity = body.value("quantity", 0);

            string errMsg;
            if (!invMgr.apiAddStock(id, quantity, errMsg)) {
                res.status = (errMsg.find("not found") != string::npos) ? 404 : 400;
                res.set_content(json({{"error", errMsg}}).dump(), "application/json");
            } else {
                Medicine* med = medMgr.apiGetMedicineById(id);
                res.set_content(medicineToJson(*med, invMgr).dump(2), "application/json");
            }
        } catch (const exception& e) {
            res.status = 400;
            res.set_content(json({{"error", string("Invalid JSON: ") + e.what()}}).dump(),
                            "application/json");
        }
    });

    // POST /api/inventory/consume — Deduct stock
    // Body: { "id": "MED001", "quantity": 2 }
    svr.Post("/api/inventory/consume", [&](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            json body    = json::parse(req.body);
            string id    = body.value("id", "");
            int quantity = body.value("quantity", 0);

            string errMsg;
            if (!invMgr.apiReduceStock(id, quantity, errMsg)) {
                res.status = (errMsg.find("not found") != string::npos) ? 404 : 400;
                res.set_content(json({{"error", errMsg}}).dump(), "application/json");
            } else {
                Medicine* med = medMgr.apiGetMedicineById(id);
                res.set_content(medicineToJson(*med, invMgr).dump(2), "application/json");
            }
        } catch (const exception& e) {
            res.status = 400;
            res.set_content(json({{"error", string("Invalid JSON: ") + e.what()}}).dump(),
                            "application/json");
        }
    });

    // PUT /api/inventory/threshold — Update low-stock threshold
    // Body: { "id": "MED001", "threshold": 10 }
    svr.Put("/api/inventory/threshold", [&](const httplib::Request& req, httplib::Response& res) {
        addCorsHeaders(res);
        try {
            json body    = json::parse(req.body);
            string id    = body.value("id", "");
            int threshold = body.value("threshold", -1);

            string errMsg;
            if (!invMgr.apiUpdateThreshold(id, threshold, errMsg)) {
                res.status = (errMsg.find("not found") != string::npos) ? 404 : 400;
                res.set_content(json({{"error", errMsg}}).dump(), "application/json");
            } else {
                Medicine* med = medMgr.apiGetMedicineById(id);
                res.set_content(medicineToJson(*med, invMgr).dump(2), "application/json");
            }
        } catch (const exception& e) {
            res.status = 400;
            res.set_content(json({{"error", string("Invalid JSON: ") + e.what()}}).dump(),
                            "application/json");
        }
    });

    // ============================================================
    // REMINDERS
    // ============================================================

    // GET /api/reminders/schedule — Full sorted daily schedule
    svr.Get("/api/reminders/schedule", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const auto& entry : remMgr.apiGetDailySchedule()) {
            arr.push_back({
                {"time",         entry.time},
                {"medicineId",   entry.medicineId},
                {"medicineName", entry.medicineName},
                {"dosage",       entry.dosage}
            });
        }
        json body = {
            {"currentTime", remMgr.getCurrentSystemTime()},
            {"date",        remMgr.getCurrentSystemDate()},
            {"schedule",    arr}
        };
        res.set_content(body.dump(2), "application/json");
    });

    // GET /api/reminders/check — Due alerts for the current minute
    svr.Get("/api/reminders/check", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const auto& alert : remMgr.apiCheckReminders()) {
            arr.push_back({
                {"medicineId",   alert.medicineId},
                {"medicineName", alert.medicineName},
                {"reminderTime", alert.reminderTime},
                {"dosage",       alert.dosage}
            });
        }
        json body = {
            {"currentTime", remMgr.getCurrentSystemTime()},
            {"alerts",      arr},
            {"count",       (int)arr.size()}
        };
        res.set_content(body.dump(2), "application/json");
    });

    // ============================================================
    // EXPIRY
    // ============================================================

    // GET /api/expiry/report — Expiry status for every medicine
    svr.Get("/api/expiry/report", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const Medicine& med : medMgr.getMedicines()) {
            json m = medicineToJson(med, invMgr);
            arr.push_back(m);
        }
        // Sort by daysToExpiry ascending (most urgent first)
        sort(arr.begin(), arr.end(), [](const json& a, const json& b) {
            return a["daysToExpiry"].get<int>() < b["daysToExpiry"].get<int>();
        });
        res.set_content(arr.dump(2), "application/json");
    });

    // GET /api/expiry/expired — Only expired medicines
    svr.Get("/api/expiry/expired", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const Medicine& med : invMgr.apiGetExpiredMedicines()) {
            arr.push_back(medicineToJson(med, invMgr));
        }
        res.set_content(arr.dump(2), "application/json");
    });

    // GET /api/expiry/near-expiry — Medicines expiring within 30 days
    svr.Get("/api/expiry/near-expiry", [&](const httplib::Request&, httplib::Response& res) {
        addCorsHeaders(res);
        json arr = json::array();
        for (const Medicine& med : invMgr.apiGetNearExpiryMedicines(30)) {
            arr.push_back(medicineToJson(med, invMgr));
        }
        res.set_content(arr.dump(2), "application/json");
    });

    // ============================================================
    // Start listening (supports local 8080 or cloud PORT env var)
    // ============================================================
    int port = 8080;
    const char* envPort = std::getenv("PORT");
    if (envPort) {
        try {
            port = std::stoi(envPort);
        } catch (...) {
            port = 8080;
        }
    }

    cout << endl;
    cout << "[Info] Server starting on http://localhost:" << port << endl;
    cout << "[Info] Frontend: http://localhost:" << port << "/index.html" << endl;
    cout << "[Info] API base: http://localhost:" << port << "/api/" << endl;
    cout << "[Info] Press Ctrl+C to stop the server." << endl;
    cout << "---------------------------------------------" << endl;

    // Listen on all interfaces
    if (!svr.listen("0.0.0.0", port)) {
        cerr << "[Error] Failed to start server on port " << port << "." << endl;
        cerr << "        Is another process already using port " << port << "?" << endl;
        return 1;
    }

    return 0;
}
