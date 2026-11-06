/* ============================================================
   MediPulse C++ | Frontend Controller & REST API Client
   ============================================================ */

const API_BASE = window.location.origin; // Connects to the host serving the page (http://localhost:8080)

// Application Global State
let allMedicines = [];
let inventoryData = null;
let reminderScheduleData = null;
let currentAlarmMed = null;
let audioCtx = null;

// Chart Instances
let stockChartInstance = null;
let expiryChartInstance = null;

// New Medicine Form Reminder Tags
let newMedicineReminderTags = ['08:00', '20:00'];

// ============================================================
// Initialization on Page Load
// ============================================================
document.addEventListener('DOMContentLoaded', () => {
    initClock();
    initNavigation();
    initTagInput();
    initApiConsole();
    
    // Initial Data Fetch
    fetchAllData();

    // Auto-poll for Due Reminders every 10 seconds
    setInterval(pollDueReminders, 10000);

    // Latency & Health check every 30 seconds
    setInterval(checkServerHealth, 30000);

    // Initial Health Check
    checkServerHealth();
});

// ============================================================
// Live Clock
// ============================================================
function initClock() {
    function update() {
        const now = new Date();
        const timeStr = now.toLocaleTimeString('en-US', { hour12: false });
        const dateStr = now.toLocaleDateString('en-GB', { day: '2-digit', month: '2-digit', year: 'numeric' });
        
        const clockEl = document.getElementById('liveClock');
        const dateEl = document.getElementById('liveDate');
        if (clockEl) clockEl.textContent = timeStr;
        if (dateEl) dateEl.textContent = dateStr;
    }
    update();
    setInterval(update, 1000);
}

// ============================================================
// Navigation Tabs
// ============================================================
function initNavigation() {
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            const tabId = item.getAttribute('data-tab');
            switchTab(tabId);
        });
    });

    // Mobile Sidebar Toggle
    const mobileBtn = document.getElementById('mobileMenuBtn');
    const sidebar = document.getElementById('sidebar');
    if (mobileBtn && sidebar) {
        mobileBtn.addEventListener('click', () => {
            sidebar.classList.toggle('open');
        });
    }

    // Modal Triggers
    const btnOpenAdd = document.getElementById('btnOpenAddModal');
    if (btnOpenAdd) {
        btnOpenAdd.addEventListener('click', () => openModal('addMedicineModal'));
    }

    const btnSimulate = document.getElementById('btnSimulateAlarm');
    if (btnSimulate) {
        btnSimulate.addEventListener('click', triggerSimulatedAlarm);
    }

    const btnRefresh = document.getElementById('btnRefreshMedicines');
    if (btnRefresh) {
        btnRefresh.addEventListener('click', () => {
            showToast('Syncing with C++ backend...', 'info');
            fetchAllData();
        });
    }

    const btnCheckReminders = document.getElementById('btnCheckRemindersNow');
    if (btnCheckReminders) {
        btnCheckReminders.addEventListener('click', () => {
            showToast('Checking active due reminders...', 'info');
            pollDueReminders(true);
        });
    }

    // Search and filter inputs
    const searchInput = document.getElementById('medicineSearchInput');
    const filterStock = document.getElementById('filterStockStatus');
    const filterExpiry = document.getElementById('filterExpiryStatus');

    if (searchInput) searchInput.addEventListener('input', renderMedicinesGrid);
    if (filterStock) filterStock.addEventListener('change', renderMedicinesGrid);
    if (filterExpiry) filterExpiry.addEventListener('change', renderMedicinesGrid);
}

function switchTab(tabId) {
    document.querySelectorAll('.nav-item').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.tab-pane').forEach(el => el.classList.remove('active'));

    const navLink = document.querySelector(`.nav-item[data-tab="${tabId}"]`);
    const targetPane = document.getElementById(`tab-${tabId}`);
    
    if (navLink) navLink.classList.add('active');
    if (targetPane) targetPane.classList.add('active');

    // Update Titles
    const titleEl = document.getElementById('pageTitle');
    const subtitleEl = document.getElementById('pageSubtitle');
    
    const titles = {
        'dashboard': ['Executive Health Dashboard', 'Real-time prescription monitoring, dose scheduling & automated stock tracking'],
        'medicines': ['Prescription Catalog', 'Complete registry of tracked medications with live dosages and schedules'],
        'schedule': ['24-Hour Dose Master Schedule', 'Chronological daily dose timetable with proactive compliance alerts'],
        'inventory': ['Inventory & Supply Ledger', 'Real-time stock depletion tracking, safety thresholds, and refill management'],
        'expiry': ['Expiry Radar & Safety Protocol', 'Proactive shelf-life tracking based on C++ Julian calendar calculation'],
        'api-console': ['C++ REST Backend Inspector', 'Direct REST endpoint debugging and payload evaluation for academic viva']
    };

    if (titles[tabId]) {
        titleEl.textContent = titles[tabId][0];
        subtitleEl.textContent = titles[tabId][1];
    }

    // Close mobile menu if open
    const sidebar = document.getElementById('sidebar');
    if (sidebar) sidebar.classList.remove('open');

    // Trigger Chart Resize if needed
    if (tabId === 'dashboard') {
        setTimeout(renderCharts, 100);
    }
}

// ============================================================
// Data Fetching: All Endpoints
// ============================================================
async function fetchAllData() {
    try {
        const [medsRes, invRes, schedRes, expiryRes] = await Promise.all([
            fetch(`${API_BASE}/api/medicines`),
            fetch(`${API_BASE}/api/inventory/status`),
            fetch(`${API_BASE}/api/reminders/schedule`),
            fetch(`${API_BASE}/api/expiry/report`)
        ]);

        if (medsRes.ok) allMedicines = await medsRes.json();
        if (invRes.ok) inventoryData = await invRes.json();
        if (schedRes.ok) reminderScheduleData = await schedRes.json();
        const expiryData = expiryRes.ok ? await expiryRes.json() : [];

        // Update All Views
        updateKpis();
        renderCharts();
        renderMedicinesGrid();
        renderDashboardSchedule();
        renderDashboardLowStock();
        renderFullSchedule();
        renderInventoryTable();
        renderExpiryTable(expiryData);
        updateNavBadges();
        populateRestockSelect();

    } catch (err) {
        console.error('Error fetching data from C++ API:', err);
        showToast('Failed to connect to C++ backend. Is MedicineServer.exe running?', 'danger');
        updateServerStatus(false);
    }
}

// ============================================================
// KPI Cards Update
// ============================================================
function updateKpis() {
    const total = allMedicines.length;
    let inStock = 0;
    let lowStock = 0;
    let expired = 0;

    allMedicines.forEach(m => {
        if (m.stockStatus === 'IN_STOCK') inStock++;
        else if (m.stockStatus === 'LOW_STOCK' || m.stockStatus === 'OUT_OF_STOCK') lowStock++;
        
        if (m.expiryStatus === 'EXPIRED' || m.expiryStatus === 'CRITICAL') expired++;
    });

    document.getElementById('kpiTotal').textContent = total;
    document.getElementById('kpiInStock').textContent = inStock;
    document.getElementById('kpiLowStock').textContent = lowStock;
    document.getElementById('kpiExpired').textContent = expired;

    // Critical Banner Visibility
    const alertBanner = document.getElementById('criticalAlertBanner');
    const bannerContent = document.getElementById('bannerContent');
    if (alertBanner && bannerContent) {
        const issues = [];
        if (expired > 0) issues.push(`<strong>${expired} medicine(s)</strong> are expired or reaching critical date.`);
        if (lowStock > 0) issues.push(`<strong>${lowStock} medicine(s)</strong> are low or out of stock.`);

        if (issues.length > 0) {
            bannerContent.innerHTML = issues.join(' &bull; ');
            alertBanner.classList.remove('hidden');
        } else {
            alertBanner.classList.add('hidden');
        }
    }
}

function updateNavBadges() {
    const countEl = document.getElementById('navMedicineCount');
    const schedEl = document.getElementById('navRemindersCount');
    const expEl = document.getElementById('navExpiredCount');

    if (countEl) countEl.textContent = allMedicines.length;
    if (schedEl && reminderScheduleData && reminderScheduleData.schedule) {
        schedEl.textContent = reminderScheduleData.schedule.length;
    }
    
    let expiredCount = allMedicines.filter(m => m.expiryStatus === 'EXPIRED').length;
    if (expEl) expEl.textContent = expiredCount;
}

// ============================================================
// Charts Render (Chart.js)
// ============================================================
function renderCharts() {
    // 1. Stock Distribution Doughnut Chart
    const ctxStock = document.getElementById('stockDistributionChart');
    if (ctxStock) {
        let inStock = 0, lowStock = 0, outStock = 0;
        allMedicines.forEach(m => {
            if (m.stockStatus === 'IN_STOCK') inStock++;
            else if (m.stockStatus === 'LOW_STOCK') lowStock++;
            else if (m.stockStatus === 'OUT_OF_STOCK') outStock++;
        });

        if (stockChartInstance) stockChartInstance.destroy();

        stockChartInstance = new Chart(ctxStock, {
            type: 'doughnut',
            data: {
                labels: ['Optimal Stock', 'Low Stock', 'Out of Stock'],
                datasets: [{
                    data: [inStock, lowStock, outStock],
                    backgroundColor: ['#10B981', '#F59E0B', '#EF4444'],
                    borderColor: '#111827',
                    borderWidth: 3,
                    hoverOffset: 6
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: { color: '#94A3B8', font: { family: 'Plus Jakarta Sans', size: 12 } }
                    }
                },
                cutout: '72%'
            }
        });
    }

    // 2. Expiry Timeline Horizontal Bar Chart
    const ctxExpiry = document.getElementById('expiryTimelineChart');
    if (ctxExpiry) {
        const sortedMeds = [...allMedicines].sort((a, b) => a.daysToExpiry - b.daysToExpiry).slice(0, 6);
        const labels = sortedMeds.map(m => m.name.length > 14 ? m.name.substring(0, 12) + '...' : m.name);
        const dataValues = sortedMeds.map(m => m.daysToExpiry);
        const colors = sortedMeds.map(m => {
            if (m.daysToExpiry <= 0) return '#EF4444';
            if (m.daysToExpiry <= 7) return '#F97316';
            if (m.daysToExpiry <= 30) return '#F59E0B';
            return '#10B981';
        });

        if (expiryChartInstance) expiryChartInstance.destroy();

        expiryChartInstance = new Chart(ctxExpiry, {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Days to Expiry',
                    data: dataValues,
                    backgroundColor: colors,
                    borderRadius: 6
                }]
            },
            options: {
                indexAxis: 'y',
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: { display: false }
                },
                scales: {
                    x: {
                        grid: { color: 'rgba(255,255,255,0.05)' },
                        ticks: { color: '#94A3B8', font: { family: 'JetBrains Mono' } }
                    },
                    y: {
                        grid: { display: false },
                        ticks: { color: '#F8FAFC', font: { family: 'Plus Jakarta Sans', weight: 600 } }
                    }
                }
            }
        });
    }
}

// ============================================================
// Dashboard Streams (Schedule & Priority Restock)
// ============================================================
function renderDashboardSchedule() {
    const stream = document.getElementById('dashboardScheduleStream');
    if (!stream) return;

    if (!reminderScheduleData || !reminderScheduleData.schedule || reminderScheduleData.schedule.length === 0) {
        stream.innerHTML = '<div class="empty-state">No scheduled doses for today.</div>';
        return;
    }

    const html = reminderScheduleData.schedule.map(item => `
        <div class="schedule-item">
            <span class="schedule-time-badge">${item.time}</span>
            <div class="schedule-item-info">
                <div class="schedule-item-title">${item.medicineName} <span class="badge-cpp" style="font-size:9px">${item.medicineId}</span></div>
                <div class="schedule-item-dosage">${item.dosage}</div>
            </div>
            <button class="btn btn-sm btn-secondary" onclick="quickTakeDose('${item.medicineId}')" title="Quick Take dose & deduct 1 pill">
                <i class="fa-solid fa-check"></i> Take
            </button>
        </div>
    `).join('');

    stream.innerHTML = html;
}

function renderDashboardLowStock() {
    const container = document.getElementById('dashboardLowStockList');
    if (!container) return;

    const lowMeds = allMedicines.filter(m => m.stockStatus !== 'IN_STOCK');
    if (lowMeds.length === 0) {
        container.innerHTML = '<div class="empty-state"><i class="fa-solid fa-circle-check" style="color:var(--primary);font-size:24px;margin-bottom:8px;display:block;"></i>All medicine stock levels are currently healthy!</div>';
        return;
    }

    const html = lowMeds.map(m => `
        <div class="stock-action-item">
            <div class="stock-action-meta">
                <h4>${m.name}</h4>
                <span>Available: <strong style="color:${m.quantity === 0 ? 'var(--danger)' : 'var(--warning)'}">${m.quantity} units</strong> (Min: ${m.lowStockThreshold})</span>
            </div>
            <button class="btn btn-sm btn-primary" onclick="openRestockModalFor('${m.id}')">
                <i class="fa-solid fa-plus"></i> Restock
            </button>
        </div>
    `).join('');

    container.innerHTML = html;
}

// ============================================================
// Tab 2: Medicine Catalog Grid
// ============================================================
function renderMedicinesGrid() {
    const container = document.getElementById('medicinesContainer');
    if (!container) return;

    const query = (document.getElementById('medicineSearchInput')?.value || '').toLowerCase().trim();
    const stockFilter = document.getElementById('filterStockStatus')?.value || 'ALL';
    const expiryFilter = document.getElementById('filterExpiryStatus')?.value || 'ALL';

    const filtered = allMedicines.filter(m => {
        const matchesQuery = m.name.toLowerCase().includes(query) ||
                             m.id.toLowerCase().includes(query) ||
                             m.notes.toLowerCase().includes(query);

        const matchesStock = (stockFilter === 'ALL') || (m.stockStatus === stockFilter);
        const matchesExpiry = (expiryFilter === 'ALL') || (m.expiryStatus === expiryFilter);

        return matchesQuery && matchesStock && matchesExpiry;
    });

    if (filtered.length === 0) {
        container.innerHTML = `
            <div class="glass-card" style="grid-column: 1 / -1; text-align: center; padding: 48px;">
                <i class="fa-solid fa-magnifying-glass" style="font-size: 36px; color: var(--text-muted); margin-bottom: 12px;"></i>
                <h3 style="color:var(--text-primary)">No medicines matched your criteria</h3>
                <p style="color:var(--text-muted)">Try adjusting your search query or status filter.</p>
            </div>
        `;
        return;
    }

    const html = filtered.map(m => {
        // Stock progress percentage
        const maxVal = Math.max(m.quantity, m.lowStockThreshold * 3, 30);
        const percent = Math.min(100, Math.round((m.quantity / maxVal) * 100));
        
        let fillClass = 'fill-safe';
        if (m.stockStatus === 'OUT_OF_STOCK') fillClass = 'fill-danger';
        else if (m.stockStatus === 'LOW_STOCK') fillClass = 'fill-warning';

        // Expiry Badge
        let expBadge = `<span class="badge badge-success"><i class="fa-solid fa-shield-check"></i> Safe (${m.daysToExpiry}d left)</span>`;
        if (m.expiryStatus === 'EXPIRED') {
            expBadge = `<span class="badge badge-danger"><i class="fa-solid fa-ban"></i> EXPIRED (${Math.abs(m.daysToExpiry)}d ago)</span>`;
        } else if (m.expiryStatus === 'CRITICAL') {
            expBadge = `<span class="badge badge-danger"><i class="fa-solid fa-skull"></i> Critical (${m.daysToExpiry}d left)</span>`;
        } else if (m.expiryStatus === 'NEAR_EXPIRY') {
            expBadge = `<span class="badge badge-warning"><i class="fa-solid fa-triangle-exclamation"></i> Near Expiry (${m.daysToExpiry}d)</span>`;
        }

        // Reminder Pills
        const reminderPills = (m.reminderTimes && m.reminderTimes.length > 0)
            ? m.reminderTimes.map(t => `<span class="pill-time"><i class="fa-regular fa-clock"></i> ${t}</span>`).join('')
            : '<span style="font-size:11px;color:var(--text-muted)">No reminders configured</span>';

        return `
            <div class="med-card" id="card-${m.id}">
                <div>
                    <div class="med-card-top">
                        <span class="med-id-tag">${m.id}</span>
                        ${expBadge}
                    </div>

                    <h3 class="med-card-name">${m.name}</h3>
                    <div class="med-card-dosage">
                        <i class="fa-solid fa-prescription"></i> ${m.dosage}
                    </div>

                    <div class="stock-progress-wrap">
                        <div class="stock-progress-meta">
                            <span>Quantity in Stock</span>
                            <strong>${m.quantity} units (Threshold: ${m.lowStockThreshold})</strong>
                        </div>
                        <div class="progress-track">
                            <div class="progress-fill ${fillClass}" style="width: ${percent}%;"></div>
                        </div>
                    </div>

                    <div class="med-card-details">
                        <div class="med-detail-row">
                            <span>Expiry Date:</span>
                            <strong style="color:var(--text-primary)">${m.expiryDate}</strong>
                        </div>
                        <div class="med-detail-row">
                            <span>Stock Condition:</span>
                            <span class="badge ${m.stockStatus === 'IN_STOCK' ? 'badge-success' : (m.stockStatus === 'LOW_STOCK' ? 'badge-warning' : 'badge-danger')}">${m.stockStatus.replace('_', ' ')}</span>
                        </div>
                        <div style="margin-top: 4px;">
                            <span style="font-size:11px;">Scheduled Dose Reminders:</span>
                            <div class="reminder-pills-row">${reminderPills}</div>
                        </div>
                        ${m.notes ? `<div style="margin-top:6px;font-style:italic;color:var(--text-secondary)">"${m.notes}"</div>` : ''}
                    </div>
                </div>

                <div class="med-card-actions">
                    <button class="btn btn-sm btn-success" style="flex:1" onclick="quickTakeDose('${m.id}')" title="Deduct 1 pill">
                        <i class="fa-solid fa-check"></i> Take Dose
                    </button>
                    <button class="btn btn-sm btn-secondary" onclick="openRestockModalFor('${m.id}')" title="Add Stock">
                        <i class="fa-solid fa-plus"></i>
                    </button>
                    <button class="btn btn-sm btn-secondary" onclick="promptEditQuantity('${m.id}', ${m.quantity})" title="Direct Stock Adjust">
                        <i class="fa-solid fa-pen"></i>
                    </button>
                    <button class="btn btn-sm btn-danger" onclick="confirmDeleteMedicine('${m.id}', '${m.name}')" title="Delete Medicine">
                        <i class="fa-solid fa-trash"></i>
                    </button>
                </div>
            </div>
        `;
    }).join('');

    container.innerHTML = html;
}

// ============================================================
// Tab 3: Full 24-Hour Schedule View
// ============================================================
function renderFullSchedule() {
    const container = document.getElementById('scheduleTimeline');
    if (!container) return;

    if (!reminderScheduleData || !reminderScheduleData.schedule || reminderScheduleData.schedule.length === 0) {
        container.innerHTML = '<div class="empty-state">No scheduled doses set in the system.</div>';
        return;
    }

    const html = `
        <div style="margin-bottom: 16px; display:flex; align-items:center; justify-content:space-between;">
            <span style="color:var(--text-secondary);font-size:13px;">System Date: <strong>${reminderScheduleData.date}</strong> &bull; Current Time: <strong style="color:var(--cyan)">${reminderScheduleData.currentTime}</strong></span>
            <span class="badge badge-info">${reminderScheduleData.schedule.length} Doses Scheduled Today</span>
        </div>
        <div class="table-responsive">
            <table class="modern-table">
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Medicine ID</th>
                        <th>Medicine Name</th>
                        <th>Prescribed Dosage</th>
                        <th>Action</th>
                    </tr>
                </thead>
                <tbody>
                    ${reminderScheduleData.schedule.map(entry => `
                        <tr>
                            <td><span class="schedule-time-badge">${entry.time}</span></td>
                            <td><span class="med-id-tag">${entry.medicineId}</span></td>
                            <td><strong style="color:var(--text-primary)">${entry.medicineName}</strong></td>
                            <td>${entry.dosage}</td>
                            <td>
                                <button class="btn btn-sm btn-primary" onclick="quickTakeDose('${entry.medicineId}')">
                                    <i class="fa-solid fa-check"></i> Log Taken (-1)
                                </button>
                            </td>
                        </tr>
                    `).join('')}
                </tbody>
            </table>
        </div>
    `;

    container.innerHTML = html;
}

// ============================================================
// Tab 4: Inventory Table
// ============================================================
function renderInventoryTable() {
    const tbody = document.getElementById('inventoryTableBody');
    if (!tbody) return;

    if (allMedicines.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" class="text-center">No medicines in inventory database.</td></tr>';
        return;
    }

    const html = allMedicines.map(m => {
        let statusBadge = '<span class="badge badge-success">IN STOCK</span>';
        if (m.stockStatus === 'LOW_STOCK') statusBadge = '<span class="badge badge-warning">LOW STOCK</span>';
        else if (m.stockStatus === 'OUT_OF_STOCK') statusBadge = '<span class="badge badge-danger">DEPLETED</span>';

        return `
            <tr>
                <td><span class="med-id-tag">${m.id}</span></td>
                <td><strong style="color:var(--text-primary)">${m.name}</strong></td>
                <td><strong style="font-size:15px;color:${m.quantity <= m.lowStockThreshold ? 'var(--danger)' : 'var(--text-primary)'}">${m.quantity}</strong> units</td>
                <td>${m.lowStockThreshold} units</td>
                <td>${statusBadge}</td>
                <td>
                    <div style="display:flex;gap:6px;">
                        <button class="btn btn-sm btn-primary" onclick="openRestockModalFor('${m.id}')" title="Restock">
                            <i class="fa-solid fa-plus"></i> Restock
                        </button>
                        <button class="btn btn-sm btn-secondary" onclick="quickConsumeDose('${m.id}')" title="Consume 1 unit">
                            <i class="fa-solid fa-minus"></i>
                        </button>
                        <button class="btn btn-sm btn-secondary" onclick="promptSetThreshold('${m.id}', ${m.lowStockThreshold})" title="Set Threshold">
                            <i class="fa-solid fa-sliders"></i>
                        </button>
                    </div>
                </td>
            </tr>
        `;
    }).join('');

    tbody.innerHTML = html;
}

// ============================================================
// Tab 5: Expiry Radar Table & Metrics
// ============================================================
function renderExpiryTable(expiryList) {
    const tbody = document.getElementById('expiryTableBody');
    if (!tbody) return;

    let expiredCount = 0;
    let nearCount = 0;
    let safeCount = 0;

    expiryList.forEach(m => {
        if (m.daysToExpiry <= 0) expiredCount++;
        else if (m.daysToExpiry <= 30) nearCount++;
        else safeCount++;
    });

    document.getElementById('expiryExpiredCount').textContent = expiredCount;
    document.getElementById('expiryNearCount').textContent = nearCount;
    document.getElementById('expirySafeCount').textContent = safeCount;

    if (expiryList.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" class="text-center">No medicines in database to audit.</td></tr>';
        return;
    }

    const html = expiryList.map(m => {
        let catBadge = '';
        let protocol = '';

        if (m.daysToExpiry < 0) {
            catBadge = `<span class="badge badge-danger"><i class="fa-solid fa-skull"></i> EXPIRED (${Math.abs(m.daysToExpiry)} days ago)</span>`;
            protocol = `<span style="color:var(--danger);font-weight:600;"><i class="fa-solid fa-triangle-exclamation"></i> Immediate Safe Disposal Required</span>`;
        } else if (m.daysToExpiry <= 7) {
            catBadge = `<span class="badge badge-danger"><i class="fa-solid fa-circle-exclamation"></i> CRITICAL (${m.daysToExpiry} days left)</span>`;
            protocol = `<span style="color:#F97316;font-weight:600;">Replace prescription within 48h</span>`;
        } else if (m.daysToExpiry <= 30) {
            catBadge = `<span class="badge badge-warning"><i class="fa-solid fa-clock"></i> NEAR EXPIRY (${m.daysToExpiry} days)</span>`;
            protocol = `Schedule prescription refill with pharmacy`;
        } else {
            catBadge = `<span class="badge badge-success"><i class="fa-solid fa-check"></i> SAFE (${m.daysToExpiry} days)</span>`;
            protocol = `Normal usage permitted`;
        }

        return `
            <tr>
                <td><span class="med-id-tag">${m.id}</span></td>
                <td><strong style="color:var(--text-primary)">${m.name}</strong></td>
                <td>${m.expiryDate}</td>
                <td><strong>${m.daysToExpiry}</strong></td>
                <td>${catBadge}</td>
                <td>${protocol}</td>
            </tr>
        `;
    }).join('');

    tbody.innerHTML = html;
}

// ============================================================
// Actions: Quick Consume / Take Dose & Restock
// ============================================================
async function quickTakeDose(medId) {
    try {
        const res = await fetch(`${API_BASE}/api/inventory/consume`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id: medId, quantity: 1 })
        });

        const data = await res.json();
        if (res.ok) {
            playAudioTone(880, 0.15); // gentle success beep
            showToast(`Dose taken! Deducted 1 unit for ${data.name}. Remaining: ${data.quantity}`, 'success');
            fetchAllData();
        } else {
            showToast(`Cannot deduct dose: ${data.error}`, 'danger');
        }
    } catch (err) {
        showToast('Network error while deducting dose.', 'danger');
    }
}

async function quickConsumeDose(medId) {
    await quickTakeDose(medId);
}

function promptEditQuantity(medId, currentQty) {
    const newQtyStr = prompt(`Update stock quantity for ${medId}:`, currentQty);
    if (newQtyStr === null) return;
    const newQty = parseInt(newQtyStr, 10);
    if (isNaN(newQty) || newQty < 0) {
        showToast('Invalid quantity entered.', 'warning');
        return;
    }

    fetch(`${API_BASE}/api/medicines/${medId}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ field: 'quantity', value: newQty.toString() })
    })
    .then(r => r.json())
    .then(data => {
        if (data.error) showToast(data.error, 'danger');
        else {
            showToast(`Updated ${data.name} quantity to ${data.quantity}!`, 'success');
            fetchAllData();
        }
    })
    .catch(() => showToast('Error communicating with backend.', 'danger'));
}

function promptSetThreshold(medId, currentThreshold) {
    const val = prompt(`Set low stock alert threshold for ${medId}:`, currentThreshold);
    if (val === null) return;
    const num = parseInt(val, 10);
    if (isNaN(num) || num < 0) {
        showToast('Invalid threshold value.', 'warning');
        return;
    }

    fetch(`${API_BASE}/api/inventory/threshold`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id: medId, threshold: num })
    })
    .then(r => r.json())
    .then(data => {
        if (data.error) showToast(data.error, 'danger');
        else {
            showToast(`Updated safety threshold for ${data.name} to ${data.lowStockThreshold}!`, 'success');
            fetchAllData();
        }
    });
}

function confirmDeleteMedicine(medId, name) {
    if (!confirm(`Are you sure you want to permanently delete '${name}' (${medId})?\nThis cannot be undone.`)) {
        return;
    }

    fetch(`${API_BASE}/api/medicines/${medId}`, {
        method: 'DELETE'
    })
    .then(r => r.json())
    .then(data => {
        if (data.error) showToast(data.error, 'danger');
        else {
            showToast(`Medicine ${medId} deleted successfully!`, 'success');
            fetchAllData();
        }
    })
    .catch(() => showToast('Error deleting record.', 'danger'));
}

// ============================================================
// Modal: Add Medicine
// ============================================================
function initTagInput() {
    renderReminderTags();
    const tagInput = document.getElementById('reminderTimeInput');
    if (tagInput) {
        tagInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                e.preventDefault();
                addReminderTagFromInput();
            }
        });
    }
}

function renderReminderTags() {
    const container = document.getElementById('reminderTagsContainer');
    if (!container) return;

    const existingTags = container.querySelectorAll('.tag-badge');
    existingTags.forEach(el => el.remove());

    const inputEl = document.getElementById('reminderTimeInput');

    newMedicineReminderTags.forEach(time => {
        const badge = document.createElement('span');
        badge.className = 'tag-badge';
        badge.innerHTML = `<i class="fa-regular fa-clock"></i> ${time} <span class="tag-remove" onclick="removeReminderTag('${time}')">&times;</span>`;
        container.insertBefore(badge, inputEl);
    });
}

function addReminderTagFromInput() {
    const input = document.getElementById('reminderTimeInput');
    if (!input) return;
    const val = input.value.trim();
    const timeRegex = /^([01]\d|2[0-3]):[0-5]\d$/;
    
    if (!timeRegex.test(val)) {
        showToast('Invalid time format. Please use 24-hour HH:MM (e.g. 08:00, 14:30)', 'warning');
        return;
    }

    if (!newMedicineReminderTags.includes(val)) {
        newMedicineReminderTags.push(val);
        renderReminderTags();
    }
    input.value = '';
}

function removeReminderTag(time) {
    newMedicineReminderTags = newMedicineReminderTags.filter(t => t !== time);
    renderReminderTags();
}

async function handleCreateMedicine(e) {
    e.preventDefault();

    const name = document.getElementById('addName').value.trim();
    const quantity = parseInt(document.getElementById('addQuantity').value, 10);
    const expiry = document.getElementById('addExpiry').value.trim();
    const threshold = parseInt(document.getElementById('addThreshold').value, 10);
    const dosage = document.getElementById('addDosage').value.trim();
    const notes = document.getElementById('addNotes').value.trim();

    if (!name || !dosage || !expiry) {
        showToast('Please fill in all required fields.', 'warning');
        return;
    }

    const payload = {
        name: name,
        quantity: quantity,
        expiryDate: expiry,
        dosage: dosage,
        reminderTimes: newMedicineReminderTags,
        notes: notes,
        lowStockThreshold: threshold
    };

    try {
        const res = await fetch(`${API_BASE}/api/medicines`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        const data = await res.json();
        if (res.ok) {
            showToast(`Success! Created ${data.name} with ID ${data.id}`, 'success');
            closeModal('addMedicineModal');
            document.getElementById('addMedicineForm').reset();
            newMedicineReminderTags = ['08:00', '20:00'];
            renderReminderTags();
            fetchAllData();
        } else {
            showToast(`Error: ${data.error}`, 'danger');
        }
    } catch (err) {
        showToast('Network error while saving medicine to C++ server.', 'danger');
    }
}

// ============================================================
// Modal: Restock
// ============================================================
function populateRestockSelect() {
    const sel = document.getElementById('restockMedicineSelect');
    if (!sel) return;
    sel.innerHTML = allMedicines.map(m => `
        <option value="${m.id}">${m.name} (${m.id}) - Current: ${m.quantity}</option>
    `).join('');
}

function openRestockModalFor(medId) {
    populateRestockSelect();
    const sel = document.getElementById('restockMedicineSelect');
    if (sel) sel.value = medId;
    openModal('restockModal');
}

function openRestockModal() {
    populateRestockSelect();
    openModal('restockModal');
}

async function handleRestockSubmit(e) {
    e.preventDefault();
    const medId = document.getElementById('restockMedicineSelect').value;
    const qty = parseInt(document.getElementById('restockQuantity').value, 10);

    if (!medId || isNaN(qty) || qty <= 0) {
        showToast('Please select a medicine and enter a valid quantity.', 'warning');
        return;
    }

    try {
        const res = await fetch(`${API_BASE}/api/inventory/restock`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id: medId, quantity: qty })
        });

        const data = await res.json();
        if (res.ok) {
            showToast(`Restocked ${data.name}! New quantity: ${data.quantity}`, 'success');
            closeModal('restockModal');
            fetchAllData();
        } else {
            showToast(`Restock failed: ${data.error}`, 'danger');
        }
    } catch (err) {
        showToast('Network error while restocking.', 'danger');
    }
}

// ============================================================
// Proactive Live Reminder Alarms & Web Audio Synthesizer
// ============================================================
async function pollDueReminders(isManual = false) {
    try {
        const res = await fetch(`${API_BASE}/api/reminders/check`);
        if (!res.ok) return;
        const data = await res.json();

        if (data.alerts && data.alerts.length > 0) {
            // Trigger alarm modal for the first due medicine
            const alert = data.alerts[0];
            triggerAlarmModal(alert);
        } else if (isManual) {
            showToast(`No due doses at current time (${data.currentTime}).`, 'info');
        }
    } catch (err) {
        // Backend might be restarting
    }
}

function triggerAlarmModal(alertData) {
    currentAlarmMed = alertData;

    document.getElementById('alarmMedName').textContent = alertData.medicineName;
    document.getElementById('alarmMedTime').textContent = `${alertData.reminderTime}`;
    document.getElementById('alarmMedDosage').textContent = alertData.dosage;
    document.getElementById('alarmMedNotes').textContent = `Prescription reminder due right now!`;

    // Hook up take button
    const btnTake = document.getElementById('btnTakeDoseAndDeduct');
    btnTake.onclick = async () => {
        await quickTakeDose(alertData.medicineId);
        dismissAlarm();
    };

    // Play synthesized medical chime
    playMedicalChime();

    openModal('reminderAlarmModal');
}

function triggerSimulatedAlarm() {
    // Pick the first available medicine or fallback
    const med = (allMedicines.length > 0) ? allMedicines[0] : {
        id: 'MED001',
        name: 'Paracetamol 500mg',
        reminderTimes: ['08:00'],
        dosage: '1 tablet after meals'
    };

    const simulatedAlert = {
        medicineId: med.id,
        medicineName: med.name,
        reminderTime: (med.reminderTimes && med.reminderTimes[0]) || '08:00',
        dosage: med.dosage
    };

    triggerAlarmModal(simulatedAlert);
}

function dismissAlarm() {
    closeModal('reminderAlarmModal');
}

// High-tech synthesized multi-tone chime using Web Audio API
function playMedicalChime() {
    try {
        if (!audioCtx) {
            audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        }
        if (audioCtx.state === 'suspended') {
            audioCtx.resume();
        }

        const notes = [523.25, 659.25, 783.99, 1046.50]; // C5, E5, G5, C6
        notes.forEach((freq, idx) => {
            const osc = audioCtx.createOscillator();
            const gain = audioCtx.createGain();

            osc.type = 'sine';
            osc.frequency.setValueAtTime(freq, audioCtx.currentTime + idx * 0.12);

            gain.gain.setValueAtTime(0, audioCtx.currentTime + idx * 0.12);
            gain.gain.linearRampToValueAtTime(0.2, audioCtx.currentTime + idx * 0.12 + 0.02);
            gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + idx * 0.12 + 0.4);

            osc.connect(gain);
            gain.connect(audioCtx.destination);

            osc.start(audioCtx.currentTime + idx * 0.12);
            osc.stop(audioCtx.currentTime + idx * 0.12 + 0.45);
        });
    } catch (e) {
        console.warn('Web Audio playback blocked or not supported yet:', e);
    }
}

function playAudioTone(freq, duration) {
    try {
        if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        if (audioCtx.state === 'suspended') audioCtx.resume();

        const osc = audioCtx.createOscillator();
        const gain = audioCtx.createGain();
        osc.type = 'triangle';
        osc.frequency.value = freq;
        gain.gain.setValueAtTime(0.1, audioCtx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + duration);
        osc.connect(gain);
        gain.connect(audioCtx.destination);
        osc.start();
        osc.stop(audioCtx.currentTime + duration);
    } catch(e) {}
}

// ============================================================
// Tab 6: C++ REST Console (For Viva / Demonstrations)
// ============================================================
function initApiConsole() {
    const btn = document.getElementById('btnExecuteApiProbe');
    if (!btn) return;

    btn.addEventListener('click', async () => {
        const ep = document.getElementById('apiEndpointSelect').value;
        const viewer = document.getElementById('apiResponseViewer');
        const statusBadge = document.getElementById('apiResponseStatus');

        statusBadge.textContent = 'Probing...';
        statusBadge.className = 'badge badge-warning';
        viewer.textContent = 'Awaiting C++ response...';

        const startTime = performance.now();
        try {
            const res = await fetch(`${API_BASE}${ep}`);
            const duration = Math.round(performance.now() - startTime);
            const data = await res.json();

            statusBadge.textContent = `${res.status} OK (${duration}ms)`;
            statusBadge.className = 'badge badge-success';
            viewer.textContent = JSON.stringify(data, null, 2);
        } catch (err) {
            statusBadge.textContent = 'Connection Error';
            statusBadge.className = 'badge badge-danger';
            viewer.textContent = `Error connecting to ${ep}: ${err.message}`;
        }
    });
}

// ============================================================
// Server Status & Health Check
// ============================================================
async function checkServerHealth() {
    const dot = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    const latencyEl = document.getElementById('serverLatency');

    const start = performance.now();
    try {
        const res = await fetch(`${API_BASE}/api/health`, { cache: 'no-cache' });
        const latency = Math.round(performance.now() - start);

        if (res.ok) {
            if (dot) dot.style.backgroundColor = '#10B981';
            if (text) text.textContent = 'C++ Server Online';
            if (latencyEl) latencyEl.textContent = `${latency}ms`;
        } else {
            updateServerStatus(false);
        }
    } catch (err) {
        updateServerStatus(false);
    }
}

function updateServerStatus(online) {
    const dot = document.getElementById('statusDot');
    const text = document.getElementById('statusText');
    const latencyEl = document.getElementById('serverLatency');

    if (dot) dot.style.backgroundColor = online ? '#10B981' : '#EF4444';
    if (text) text.textContent = online ? 'C++ Server Online' : 'C++ Server Offline';
    if (latencyEl) latencyEl.textContent = online ? 'Normal' : 'Unreachable';
}

// ============================================================
// Modal Utilities
// ============================================================
function openModal(modalId) {
    const el = document.getElementById(modalId);
    if (el) el.classList.add('active');
}

function closeModal(modalId) {
    const el = document.getElementById(modalId);
    if (el) el.classList.remove('active');
}

// Close modal on click outside
window.addEventListener('click', (e) => {
    if (e.target.classList.contains('modal-overlay')) {
        e.target.classList.remove('active');
    }
});

// ============================================================
// Toast Notification Engine
// ============================================================
function showToast(message, type = 'info') {
    const container = document.getElementById('toastContainer');
    if (!container) return;

    const icons = {
        'success': 'fa-circle-check',
        'warning': 'fa-triangle-exclamation',
        'danger': 'fa-circle-xmark',
        'info': 'fa-circle-info'
    };

    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `
        <i class="fa-solid ${icons[type] || 'fa-bell'}" style="font-size:16px;"></i>
        <span>${message}</span>
    `;

    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateX(40px)';
        toast.style.transition = 'all 0.3s ease';
        setTimeout(() => toast.remove(), 300);
    }, 4000);
}

