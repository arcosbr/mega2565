# ATmega2560 & 6502 Serial Communication Interface

This project provides a serial communication interface between a 6502 CPU and an ATmega2560 microcontroller. The ATmega2560 simulates memory and allows the PC to interact with the 6502 via commands sent through the serial port. The PC can send commands to read/write memory, reset the CPU, halt, and step through CPU execution.

## Setup

This example assumes that the 6502's address and data buses are connected to the I/O ports of the ATmega2560. Make sure to adjust the pin assignments according to your hardware setup.

```
ATmega2560                 6502
-----------               ------
PA7 (A0)  <-------->  D0  (Pin 9)
PA6 (A1)  <-------->  D1  (Pin 10)
PA5 (A2)  <-------->  D2  (Pin 11)
PA4 (A3)  <-------->  D3  (Pin 12)
PA3 (A4)  <-------->  D4  (Pin 13)
PA2 (A5)  <-------->  D5  (Pin 14)
PA1 (A6)  <-------->  D6  (Pin 15)
PA0 (A7)  <-------->  D7  (Pin 16)

PC0 (D37) <-------->  A0  (Pin 19)
...                  ...
PC7 (D30) <-------->  A7  (Pin 26)
PL0 (D49) <-------->  A8  (Pin 27)
...                  ...
PL7 (D42) <-------->  A15 (Pin 34)

PD2 (D19) ----> RESET     (Pin 40)
PD3 (D18) <---- R/W       (Pin 34)
PD4 (D38) ----> IRQ       (Pin 4)
PD5 (D39) ----> NMI       (Pin 6)
PD6 (D40) <---- SYNC      (Pin 7)
PD7 (D41) ----> CLOCK     (Pin 3)

GND ----------> Vss       (Pins 1, possibly others)
+5V ----------> Vcc       (Pins 40, possibly others)
```

## Key Features

### ATmega2560 Firmware (C Code)

- **Initialization Functions:**
  - `init_cpu_interface()`: Configures control pins (`RESET`, `IRQ`, `NMI`, `CLOCK`, etc.) and sets data bus directions.
  - `init_serial()`: Initializes UART for serial communication between the ATmega2560 and the PC.

- **Memory Simulation:**
  - Simulates 8KB (or more) of RAM for the 6502 CPU using the `memory` array.
  - `simulate_memory()`: Continuously monitors the CPU's address and data buses. For read operations, it retrieves data from simulated memory. For write operations, it stores data into the simulated memory.

- **Serial Communication and Command Handling:**
  - `handle_serial_command()`: Processes commands from the PC, allowing control over the 6502 CPU. Supports resetting, halting, stepping through, and reading/writing memory.
  - `receive_byte()` and `send_byte()`: Helper functions for communication with the PC.

- **CPU Control Functions:**
  - `reset_cpu()`: Resets the 6502 CPU by toggling the `RESET` line.
  - `halt_cpu()`: Stops the CPU by halting the memory simulation.
  - `release_cpu()`: Resumes CPU execution.
  - `step_cpu()`: Steps through one CPU cycle, using the `SYNC` signal for precise control.

- **New Commands Implemented:**
  - `'R'`: Reset the CPU.
  - `'H'`: Halt the CPU.
  - `'C'`: Continue CPU execution.
  - `'S'`: Step the CPU through one instruction cycle.
  - `'W'`: Write to memory (address and data sent by the PC).
  - `'M'`: Read memory (address sent by the PC).

### Python Serial Communication Application

The Python application provides a graphical interface (GUI) for communicating with the ATmega2560 over a serial port. This tool enables users to send commands to the 6502 CPU, read/write memory, and observe real-time responses from the CPU.

- **Features:**
  - Connect and disconnect to the ATmega2560 via a serial port.
  - Send commands to reset, halt, continue, and step the 6502 CPU.
  - Read and write memory addresses directly from the GUI.
  - Real-time console to display sent commands and received responses.
  - Periodic polling of the serial port using a `TimerRepeater` class for non-blocking data retrieval.

- **Commands Supported:**
  - **Reset CPU:** Resets the 6502 CPU (`'R'`).
  - **Halt CPU:** Stops the CPU (`'H'`).
  - **Continue CPU:** Resumes CPU execution (`'C'`).
  - **Step CPU:** Executes one instruction cycle on the CPU (`'S'`).
  - **Read Memory:** Reads data from a specified memory address (`'M'` followed by address).
  - **Write Memory:** Writes data to a specified memory address (`'W'` followed by address and data).

### Improvements from Previous Versions

- **User Interface and Communication:**
  - The Python GUI is user-friendly, with buttons for various CPU operations and memory commands.
  - Real-time feedback via a console allows users to observe the results of their commands.

- **Modular Design:**
  - The ATmega2560 firmware is modular and can easily be extended to add new commands or modify existing ones.
  - Clear separation between control logic, memory simulation, and serial communication.

- **Scalability and Flexibility:**
  - The system is designed to handle more advanced features like breakpoints, memory tracing, or debugging tools if needed.
  - The memory space can be easily adjusted beyond 8KB, and additional hardware features can be supported with minimal code changes.

### Memory Management Enhancements

- The memory simulation supports up to 64KB of memory, and memory read/write operations are optimized for both speed and accuracy.
- You can extend this simulation to handle advanced memory-mapping features or bank-switching by modifying the `simulate_memory()` function.

### Error Handling

- Basic error handling for unknown commands is implemented. For example, if the PC sends an invalid command, the ATmega2560 will return an error message.
- The Python application also includes error messages for connection issues and invalid inputs in the GUI.

### Command Usage Examples

- **Reset the CPU:**
  - Send the character `'R'` from the PC to reset the 6502 CPU.

- **Write to Memory:**
  - Send `'W'`, followed by the memory address (2 bytes) and data (1 byte). For example:
    - Command: `W 0200 55` will write `0x55` to address `0x0200`.

- **Read from Memory:**
  - Send `'M'`, followed by the memory address (2 bytes). The ATmega2560 will respond with the data at that address.
    - Command: `M 0200` will read the data from address `0x0200`.

### Application Setup

1. **Hardware Setup:**
   - Ensure the ATmega2560 is properly connected to the 6502 CPU.
   - Double-check the pin assignments for the address/data buses and control signals (`RESET`, `IRQ`, `NMI`, etc.).

2. **ATmega2560 Firmware:**
   - Upload the provided firmware to the ATmega2560 using your preferred method (e.g., Arduino IDE, Atmel Studio).

3. **Python GUI Application:**
   - Install Python 3.x and the required dependencies (e.g., `pyserial`).
   - Run the Python GUI to communicate with the ATmega2560 and 6502 CPU.

### Conclusion

This project provides a comprehensive framework for interacting with a 6502 CPU through the ATmega2560. With the modular design of the firmware and the Python-based GUI application, it allows full control over the CPU via serial communication. The framework is scalable, allowing for the addition of advanced features like breakpoints or memory-mapped I/O.

### Future Work

- Add more advanced debugging tools, such as memory tracing or register monitoring.
- Expand the memory simulation to support bank-switching or external devices.
- Improve the graphical interface with additional functionality, such as data plotting or real-time memory visualization.

---

**Disclaimer:** Always ensure your hardware connections are correct and stable. Use the software responsibly and test all features thoroughly before deploying in a critical environment.
