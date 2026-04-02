# Treasure Run Engine – Multi-Language Game System (C + Python)

This project is a multi-language game system that combines a high-performance C-based game engine with a Python application layer using ctypes for integration.

The system follows a Model–View–Controller (MVC) architecture, where Python manages application logic and UI, while the C backend handles core game state and performance-critical operations.

---

## 🚀 Features

- Cross-language integration (C + Python via ctypes)
- Terminal-based UI using curses
- MVC architecture (Model–View–Controller)
- Persistent player profiles using JSON
- Modular and testable system design
- Real-time game interaction and rendering

---

## 🧠 Architecture

- Model: Python classes wrapping C engine logic  
- View: Curses-based UI (terminal rendering)  
- Controller: Game engine coordination and flow management  

The C engine is responsible for:
- Game state  
- World logic  
- Performance-critical operations  

Python handles:
- UI rendering  
- Input handling  
- High-level control flow  

---

## 🛠 Technologies

- C  
- Python  
- ctypes  
- curses (terminal UI)  
- JSON  
- MVC Architecture  

---

## ▶️ How to Run

### Requirements
- Linux environment (recommended)
- Python 3
- GCC

### Steps

1. Compile the C engine:
make

2. Run the game:
python3 src/python_app/run_game.py –config <config_file> –profile <profile_file>

---

## 📸 Demo

Screenshots of the game UI can be found in the `/docs` folder.

---

## 🧩 Concepts Demonstrated

- Systems design and modular architecture  
- Cross-language interoperability  
- Separation of concerns (MVC)  
- Persistence using structured data (JSON)  
- Terminal-based UI development  

---

## ⚠️ Notes

This project was developed in a Linux-based environment. Terminal UI behavior may vary across systems.

---

## 👨‍💻 Author

Developed as part of a systems-focused software engineering project, emphasizing architecture, integration, and clean design.