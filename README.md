# 🎓 Grade Management System

A simple multithreaded **Client-Server** application in C++ for managing student grades. Users can log in as **Students** or **TAs** (Teaching Assistants) with different permissions for viewing and updating grades.

---

## 📂 Project Structure

- `GradeClient.cpp` – Command-line client used by students and TAs.
- `GradeServer.cpp` – Multithreaded server handling login, read, update, and list requests.
- `makefile` – Compile instructions (not included in this README).

---

## 🖥️ Features

### 👩‍🎓 Student
- ✅ Login with ID and password
- 📄 Read own grade
- 🚫 Cannot view or modify other students' grades

### 🧑‍🏫 Teaching Assistant (TA)
- ✅ Login with ID and password
- 📄 Read any student’s grade
- ✏️ Update student grades
- 📋 View list of all grades

---

## 🔌 How It Works

1. **Client connects** to the server using TCP sockets.
2. Sends commands like `Login`, `ReadGrade`, `GradeList`, and `UpdateGrade`.
3. Server **validates** credentials and **responds** with results.
4. Multiple client requests are handled concurrently using **pthreads**.

---

## 🧵 Multithreading

- Server initializes a **fixed thread pool**.
- Incoming requests are queued and **dispatched to threads** for processing.
- Thread-safe operations are ensured using **mutexes** and **condition variables**.

---

## 📁 Input Files

- `students.txt`: List of student accounts in `id:password` format.
- `assistants.txt`: List of TA accounts in `id:password` format.

---

## 🚀 How to Run

### Compile

```bash
Start Server
bash
Copy
Edit
./GradeServer <port>
Start Client
bash
Copy
Edit
./GradeClient <host> <port>
make
This project is for educational purposes.
