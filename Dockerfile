# ============================================================
# Dockerfile for MediPulse C++ on Render / Cloud
# ============================================================

# Step 1: Lightweight GCC compiler environment
FROM gcc:latest

# Working directory inside the container
WORKDIR /app

# Copy the entire project repository
COPY . .

# Compile the native C++ HTTP server
RUN g++ -O2 -std=c++17 -I. -o MedicineServer \
    server.cpp Medicine.cpp MedicineManager.cpp ReminderManager.cpp InventoryManager.cpp \
    -lpthread

# Expose standard cloud web port
EXPOSE 8080

# Run the compiled C++ REST API and web server
CMD ["./MedicineServer"]

