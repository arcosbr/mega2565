/*
 * 6502 Emulator Interface using ATmega2560
 *
 * This program interfaces a 6502 CPU with an ATmega2560 microcontroller.
 * It simulates memory for the 6502, allowing the PC to read and write memory,
 * control execution, and monitor the CPU via serial communication.
 *
 * Author: Anderson Costa
 * Date: 24-10-2023
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

// Define CPU control pins
#define CPU_RESET       PD0
#define CPU_RW          PD1
#define CPU_IRQ         PD2
#define CPU_NMI         PD3
#define CPU_SYNC        PD4
#define CPU_CLOCK       PD5

// Define data bus port (e.g., PORTA)
#define DATA_BUS        PORTA
#define DATA_DIR        DDRA
#define DATA_PIN        PINA

// Define address bus ports (e.g., PORTC and PORTL for 16 bits)
#define ADDR_BUS_LOW    PINC // Lower 8 bits
#define ADDR_BUS_HIGH   PINL // Higher 8 bits

// Define control signals
#define CONTROL_PORT    PORTD
#define CONTROL_DIR     DDRD
#define CONTROL_PIN     PIND

// Configurable baud rate (default to 9600)
#define BAUD_RATE       9600

// Memory and breakpoint definitions
#define MEMORY_SIZE     4096 // 4KB of memory
#define MAX_BREAKPOINTS 10   // Maximum number of breakpoints

// Function prototypes
void init_cpu_interface(void);
void init_serial(uint32_t baud_rate);
void simulate_memory(void);
void handle_serial_command(void);
uint8_t write_memory(uint16_t address, uint8_t data);
uint8_t read_memory(uint16_t address, uint8_t *data);
void reset_cpu(void);
void halt_cpu(void);
void release_cpu(void);
void step_cpu(void);
uint8_t receive_byte(void);
void send_byte(uint8_t data);
void send_byte_hex(uint8_t data);
void send_string(const char *str);
uint8_t calculate_checksum(uint8_t *data, uint16_t length);

// Global variables
volatile uint8_t cpu_running = 1;
uint8_t memory[MEMORY_SIZE];           // Memory array to simulate 4KB of memory
uint16_t breakpoints[MAX_BREAKPOINTS]; // Array to store breakpoints
uint8_t breakpoint_count = 0;          // Number of breakpoints set

int main(void)
{
    // Initialize CPU interface and serial communication
    init_cpu_interface();
    init_serial(BAUD_RATE);

    // Enable global interrupts
    sei();

    // Main loop
    while (1)
    {
        // Check for serial commands from the PC
        if (UCSR0A & (1 << RXC0))
        {
            handle_serial_command();
        }

        // If CPU is running, simulate memory access
        if (cpu_running)
        {
            simulate_memory();
        }
    }

    return 0;
}

/**
 * Initialize CPU interface pins and ports.
 */
void init_cpu_interface(void)
{
    // Set control pins as outputs
    CONTROL_DIR |=
        (1 << CPU_RESET) | (1 << CPU_IRQ) | (1 << CPU_NMI) | (1 << CPU_CLOCK);
    CONTROL_DIR &= ~((1 << CPU_RW) | (1 << CPU_SYNC)); // RW and SYNC are inputs

    // Set data bus as input by default
    DATA_DIR = 0x00;

    // Initialize control signals
    CONTROL_PORT |= (1 << CPU_RESET);  // Keep CPU in reset initially
    CONTROL_PORT |= (1 << CPU_IRQ);    // Set IRQ high (inactive)
    CONTROL_PORT |= (1 << CPU_NMI);    // Set NMI high (inactive)
    CONTROL_PORT &= ~(1 << CPU_CLOCK); // Initialize clock to low
}

/**
 * Initialize serial communication (UART) with configurable baud rate.
 */
void init_serial(uint32_t baud_rate)
{
    // Calculate UBRR value based on baud rate and CPU frequency
    uint16_t ubrr_value = (F_CPU / (16UL * baud_rate)) - 1;

    // Set baud rate
    UBRR0H = (uint8_t)(ubrr_value >> 8);
    UBRR0L = (uint8_t)(ubrr_value & 0xFF);

    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // Set frame format: 8 data bits, no parity, 1 stop bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/**
 * Simulate memory for the 6502 CPU.
 * This function should be called continuously in the main loop.
 */
void simulate_memory(void)
{
    uint16_t address;
    uint8_t data;

    // Read address bus
    address = ((uint16_t)ADDR_BUS_HIGH << 8) | ADDR_BUS_LOW;

    // Check if a breakpoint is reached
    for (uint8_t i = 0; i < breakpoint_count; i++)
    {
        if (address == breakpoints[i])
        {
            halt_cpu();
            send_string("Breakpoint reached at address: 0x");
            send_byte_hex(address >> 8);
            send_byte_hex(address & 0xFF);
            send_string("\n");
            break;
        }
    }

    // Check if CPU is performing a read or write operation
    if (CONTROL_PIN & (1 << CPU_RW))
    {
        // CPU is reading from memory
        if (read_memory(address, &data))
        {
            // Set data bus as output and put data on bus
            DATA_DIR = 0xFF;
            DATA_BUS = data;

            // Small delay to ensure data is stable
            _delay_us(1);

            // Set data bus back to input to avoid bus contention
            DATA_DIR = 0x00;
        }
        else
        {
            // Address out of range; provide default data
            DATA_DIR = 0xFF;
            DATA_BUS = 0xFF;
            _delay_us(1);
            DATA_DIR = 0x00;
        }
    }
    else
    {
        // CPU is writing to memory
        // Set data bus as input
        DATA_DIR = 0x00;

        // Read data from data bus
        data = DATA_PIN;

        // Store data in memory
        write_memory(address, data);
    }
}

/**
 * Handle serial commands received from the PC.
 * Commands can be used to read/write memory, control CPU, etc.
 */
void handle_serial_command(void)
{
    uint8_t command = receive_byte(); // Read the command

    switch (command)
    {
    case 'R': // Reset CPU
        reset_cpu();
        send_string("CPU reset.\n");
        break;

    case 'H': // Halt CPU
        halt_cpu();
        send_string("CPU halted.\n");
        break;

    case 'C': // Continue CPU
        release_cpu();
        send_string("CPU continued.\n");
        break;

    case 'S': // Step CPU
        step_cpu();
        send_string("CPU stepped one instruction.\n");
        break;

    case 'W': // Write memory
    {
        // Read address (2 bytes)
        uint16_t address = ((uint16_t)receive_byte() << 8) | receive_byte();

        // Read data
        uint8_t data = receive_byte();

        if (write_memory(address, data))
        {
            send_string("Memory written at address 0x");
            send_byte_hex(address >> 8);
            send_byte_hex(address & 0xFF);
            send_string(".\n");
        }
        else
        {
            send_string("Error: Invalid address.\n");
        }

        break;
    }

    case 'M': // Read memory
    {
        // Read address (2 bytes)
        uint16_t address = ((uint16_t)receive_byte() << 8) | receive_byte();
        uint8_t data;

        if (read_memory(address, &data))
        {
            send_byte(data); // Send data back to PC
        }
        else
        {
            send_string("Error: Invalid address.\n");
        }

        break;
    }

    case 'L': // Load data into memory
    {
        // Read address (2 bytes)
        uint16_t address = ((uint16_t)receive_byte() << 8) | receive_byte();

        // Read size (2 bytes)
        uint16_t size = ((uint16_t)receive_byte() << 8) | receive_byte();

        // Read data
        uint8_t data;
        uint16_t i;

        for (i = 0; i < size; i++)
        {
            data = receive_byte();
            if (!write_memory(address + i, data))
            {
                send_string("Error: Invalid address during load.\n");
                break;
            }
        }

        if (i == size)
        {
            send_string("Data loaded successfully.\n");
        }

        break;
    }

    case 'B': // Set breakpoint
    {
        if (breakpoint_count >= MAX_BREAKPOINTS)
        {
            send_string("Error: Maximum number of breakpoints reached.\n");
            break;
        }

        // Read address (2 bytes)
        uint16_t address = ((uint16_t)receive_byte() << 8) | receive_byte();
        breakpoints[breakpoint_count++] = address;
        send_string("Breakpoint set at address 0x");
        send_byte_hex(address >> 8);
        send_byte_hex(address & 0xFF);
        send_string(".\n");
        break;
    }

    case 'G': // Get CPU registers (not implemented)
    {
        send_string("Error: Register reading not supported.\n");
        break;
    }

    default:
        // Unknown command
        send_string("Error: Unknown command.\n");
        break;
    }
}

/**
 * Write a byte to memory at the specified address.
 * Returns 1 if successful, 0 if the address is invalid.
 */
uint8_t write_memory(uint16_t address, uint8_t data)
{
    if (address < MEMORY_SIZE)
    {
        memory[address] = data;
        return 1;
    }
    else
    {
        return 0; // Address out of range
    }
}

/**
 * Read a byte from memory at the specified address.
 * Returns 1 if successful, 0 if the address is invalid.
 */
uint8_t read_memory(uint16_t address, uint8_t *data)
{
    if (address < MEMORY_SIZE)
    {
        *data = memory[address];
        return 1;
    }
    else
    {
        *data = 0xFF; // Default value
        return 0;     // Address out of range
    }
}

/**
 * Reset the 6502 CPU.
 */
void reset_cpu(void)
{
    // Pull RESET low for a short time
    CONTROL_PORT &= ~(1 << CPU_RESET);
    _delay_ms(10);
    CONTROL_PORT |= (1 << CPU_RESET);
    cpu_running = 1;
}

/**
 * Halt the 6502 CPU by stopping memory simulation.
 */
void halt_cpu(void)
{
    cpu_running = 0;
}

/**
 * Release the 6502 CPU to continue execution.
 */
void release_cpu(void)
{
    cpu_running = 1;
}

/**
 * Step the 6502 CPU by simulating one instruction.
 */
void step_cpu(void)
{
    // Halt the CPU to ensure control
    halt_cpu();

    // Execute one instruction by monitoring SYNC signal
    uint8_t sync_high = 0;

    do
    {
        // Generate a clock pulse
        CONTROL_PORT |= (1 << CPU_CLOCK);  // Clock high
        _delay_us(1);                      // Small delay
        CONTROL_PORT &= ~(1 << CPU_CLOCK); // Clock low
        _delay_us(1);                      // Small delay

        // Simulate memory access during this cycle
        simulate_memory();

        // Check SYNC signal
        if (CONTROL_PIN & (1 << CPU_SYNC))
        {
            if (!sync_high)
            {
                // SYNC just went high, beginning of a new instruction
                sync_high = 1;
            }
        }
        else
        {
            if (sync_high)
            {
                // SYNC just went low, instruction complete
                break;
            }
        }
    } while (1);
}

/**
 * Receive a byte from the serial port.
 */
uint8_t receive_byte(void)
{
    // Wait for data to be received
    while (!(UCSR0A & (1 << RXC0)))
        ;
    // Return received data
    return UDR0;
}

/**
 * Send a byte via the serial port.
 */
void send_byte(uint8_t data)
{
    // Wait for empty transmit buffer
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    // Put data into buffer, sends the data
    UDR0 = data;
}

/**
 * Send a byte in hexadecimal format via the serial port.
 */
void send_byte_hex(uint8_t data)
{
    const char hex_digits[] = "0123456789ABCDEF";
    send_byte(hex_digits[(data >> 4) & 0x0F]);
    send_byte(hex_digits[data & 0x0F]);
}

/**
 * Send a string via the serial port.
 */
void send_string(const char *str)
{
    while (*str)
    {
        send_byte(*str++);
    }
}

/**
 * Calculate a simple checksum for data integrity verification.
 */
uint8_t calculate_checksum(uint8_t *data, uint16_t length)
{
    uint8_t checksum = 0;

    // Loop through each byte of the data array
    for (uint16_t i = 0; i < length; i++)
    {
        checksum ^= data[i]; // XOR the current byte with the checksum
    }

    return checksum;
}
