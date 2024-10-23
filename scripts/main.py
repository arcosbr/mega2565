import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import serial
import threading
import time
from TimerRepeater import TimerRepeater

# Apply a dark theme to the interface
def apply_dark_theme(root):
    """Applies a dark style to the window."""
    style = ttk.Style(root)
    root.tk.call('source', 'theme/azure.tcl')
    root.tk.call("set_theme", "dark")
    return style

class SerialApp:
    """Main application class for the Serial COM interface."""

    def __init__(self, root):
        """Initializes the GUI and serial communication."""
        self.root = root
        self.root.title("6502 Terminal")
        self.root.geometry('640x480')
        self.root.resizable(0, 0)

        # Apply a dark theme to the interface
        self.style = apply_dark_theme(self.root)

        # Serial port configuration
        self.serial_port = None
        self.serial_thread = None
        self.is_serial_connected = False

        # GUI components
        self.create_widgets()

        # TimerRepeater for periodic tasks (e.g., polling serial port)
        self.poll_timer = TimerRepeater("SerialPoller", 0.1, self.poll_serial_port)
        self.poll_timer.start()

    def create_widgets(self):
        """Creates the GUI widgets."""
        # Frame for serial port configuration
        config_frame = ttk.Frame(self.root)
        config_frame.pack(pady=10)

        ttk.Label(config_frame, text="Port:").grid(row=0, column=0, padx=5, pady=5)
        self.port_entry = ttk.Entry(config_frame, width=10)
        self.port_entry.grid(row=0, column=1, padx=5, pady=5)
        self.port_entry.insert(0, "COM3")  # Default port

        ttk.Label(config_frame, text="Baud Rate:").grid(row=0, column=2, padx=5, pady=5)
        self.baudrate_entry = ttk.Entry(config_frame, width=10)
        self.baudrate_entry.grid(row=0, column=3, padx=5, pady=5)
        self.baudrate_entry.insert(0, "115200")  # Default baud rate

        self.connect_button = ttk.Button(config_frame, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=4, padx=5, pady=5)

        # Frame for command buttons
        command_frame = ttk.Frame(self.root)
        command_frame.pack(pady=10)

        self.reset_button = ttk.Button(command_frame, text="Reset CPU", command=self.reset_cpu, state='disabled')
        self.reset_button.grid(row=0, column=0, padx=5, pady=5)

        self.halt_button = ttk.Button(command_frame, text="Halt CPU", command=self.halt_cpu, state='disabled')
        self.halt_button.grid(row=0, column=1, padx=5, pady=5)

        self.continue_button = ttk.Button(command_frame, text="Continue CPU", command=self.continue_cpu, state='disabled')
        self.continue_button.grid(row=0, column=2, padx=5, pady=5)

        self.step_button = ttk.Button(command_frame, text="Step CPU", command=self.step_cpu, state='disabled')
        self.step_button.grid(row=0, column=3, padx=5, pady=5)

        # Frame for memory operations
        memory_frame = ttk.LabelFrame(self.root, text="Memory Operations")
        memory_frame.pack(fill='x', padx=10, pady=10)

        ttk.Label(memory_frame, text="Address (hex):").grid(row=0, column=0, padx=5, pady=5)
        self.address_entry = ttk.Entry(memory_frame, width=10)
        self.address_entry.grid(row=0, column=1, padx=5, pady=5)

        ttk.Label(memory_frame, text="Data (hex):").grid(row=0, column=2, padx=5, pady=5)
        self.data_entry = ttk.Entry(memory_frame, width=10)
        self.data_entry.grid(row=0, column=3, padx=5, pady=5)

        self.read_button = ttk.Button(memory_frame, text="Read Memory", command=self.read_memory, state='disabled')
        self.read_button.grid(row=0, column=4, padx=5, pady=5)

        self.write_button = ttk.Button(memory_frame, text="Write Memory", command=self.write_memory, state='disabled')
        self.write_button.grid(row=0, column=5, padx=5, pady=5)

        # Output console
        self.console = scrolledtext.ScrolledText(self.root, state='disabled', height=15)
        self.console.pack(fill='both', padx=10, pady=10)

    def toggle_connection(self):
        """Connects or disconnects from the serial port."""
        if not self.is_serial_connected:
            port = self.port_entry.get()
            baudrate = int(self.baudrate_entry.get())
            try:
                self.serial_port = serial.Serial(port, baudrate, timeout=0)
                self.is_serial_connected = True
                self.connect_button.config(text="Disconnect")
                self.update_gui_state('normal')
                self.log_message(f"Connected to {port} at {baudrate} baud.")
            except serial.SerialException as e:
                messagebox.showerror("Connection Error", str(e))
        else:
            self.poll_timer.stop()
            self.serial_port.close()
            self.is_serial_connected = False
            self.connect_button.config(text="Connect")
            self.update_gui_state('disabled')
            self.log_message("Disconnected from serial port.")

    def update_gui_state(self, state):
        """Enables or disables GUI components based on connection state."""
        self.reset_button.config(state=state)
        self.halt_button.config(state=state)
        self.continue_button.config(state=state)
        self.step_button.config(state=state)
        self.read_button.config(state=state)
        self.write_button.config(state=state)

    def log_message(self, message):
        """Logs a message to the console."""
        self.console.config(state='normal')
        self.console.insert(tk.END, message + '\n')
        self.console.see(tk.END)
        self.console.config(state='disabled')

    def reset_cpu(self):
        """Sends the reset command to the CPU."""
        self.send_command(b'R')
        self.log_message("Sent: Reset CPU")

    def halt_cpu(self):
        """Sends the halt command to the CPU."""
        self.send_command(b'H')
        self.log_message("Sent: Halt CPU")

    def continue_cpu(self):
        """Sends the continue command to the CPU."""
        self.send_command(b'C')
        self.log_message("Sent: Continue CPU")

    def step_cpu(self):
        """Sends the step command to the CPU."""
        self.send_command(b'S')
        self.log_message("Sent: Step CPU")

    def read_memory(self):
        """Reads memory from the specified address."""
        address = self.address_entry.get()
        if not address:
            messagebox.showwarning("Input Error", "Please enter an address.")
            return

        try:
            address_int = int(address, 16)
            address_bytes = address_int.to_bytes(2, 'big')
            self.send_command(b'M' + address_bytes)
            self.log_message(f"Sent: Read Memory at 0x{address.upper()}")
        except ValueError:
            messagebox.showerror("Input Error", "Invalid address format.")

    def write_memory(self):
        """Writes data to the specified memory address."""
        address = self.address_entry.get()
        data = self.data_entry.get()
        if not address or not data:
            messagebox.showwarning("Input Error", "Please enter both address and data.")
            return

        try:
            address_int = int(address, 16)
            data_int = int(data, 16)
            address_bytes = address_int.to_bytes(2, 'big')
            data_byte = data_int.to_bytes(1, 'big')
            self.send_command(b'W' + address_bytes + data_byte)
            self.log_message(f"Sent: Write 0x{data.upper()} to Memory at 0x{address.upper()}")
        except ValueError:
            messagebox.showerror("Input Error", "Invalid address or data format.")

    def send_command(self, command):
        """Sends a command to the serial port."""
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.write(command)
        else:
            messagebox.showerror("Serial Error", "Serial port is not open.")

    def poll_serial_port(self):
        """Polls the serial port for incoming data."""
        if self.serial_port and self.serial_port.is_open:
            try:
                data = self.serial_port.readline()
                if data:
                    message = data.decode('utf-8', errors='ignore').strip()
                    self.log_message(f"Received: {message}")
            except Exception as e:
                self.log_message(f"Error reading serial port: {e}")

    def on_close(self):
        """Handles application closing."""
        if self.is_serial_connected:
            self.poll_timer.stop()
            self.serial_port.close()
        self.root.destroy()

# Main execution
if __name__ == "__main__":
    window = tk.Tk()
    app = SerialApp(window)
    window.protocol("WM_DELETE_WINDOW", app.on_close)
    window.mainloop()
