#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

// Register module for logging
LOG_MODULE_REGISTER(microinverter, LOG_LEVEL_INF);

// --- HARDWARE ---
// Blue Pill LED (PC13)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// --- MOCK STATE ---
static float simulated_voltage = 0.0f;
static bool auto_simulation_mode = true;

// --- THREAD 1: SOLAR PHYSICS SIMULATOR ---
// Runs every 100ms to update voltage
void solar_simulation_thread(void)
{
    bool rising = true;
    
    while (1) {
        if (auto_simulation_mode) {
            // Simulate Day (0V -> 50V) and Night (50V -> 0V)
            if (rising) {
                simulated_voltage += 0.5f;
                if (simulated_voltage >= 50.0f) rising = false;
            } else {
                simulated_voltage -= 0.5f;
                if (simulated_voltage <= 0.0f) rising = true;
            }
        }
        k_msleep(100);
    }
}

// --- THREAD 2: INVERTER CONTROL LOOP ---
// The "Job 1" Logic: Monitor Voltage -> Control Hardware
void control_thread(void)
{
    if (!gpio_is_ready_dt(&led)) return;
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    while (1) {
        // Logic: Inverter only works between 20V and 45V
        bool grid_ok = (simulated_voltage > 20.0f && simulated_voltage < 45.0f);

        if (grid_ok) {
            // Normal Operation: LED Solid ON (Active Low = 1)
            gpio_pin_set_dt(&led, 1); 
            // LOG_INF("Status: GENERATING | Voltage: %d V", (int)simulated_voltage);
        } else {
            // Fault Mode: Blink LED
            gpio_pin_toggle_dt(&led);
            // LOG_WRN("Status: FAULT      | Voltage: %d V", (int)simulated_voltage);
        }

        k_msleep(200); // Check 5 times per second
    }
}

// --- DEFINE THREADS ---
// Create threads with generous stack sizes for safety
K_THREAD_DEFINE(sim_tid, 1024, solar_simulation_thread, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(ctrl_tid, 1024, control_thread, NULL, NULL, NULL, 5, 0, 0);

// --- SHELL COMMANDS ---
// Commands to type in terminal: "sensor set <val>" or "sensor auto"
static int cmd_set_voltage(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_print(shell, "Usage: sensor set <voltage>");
        return -1;
    }
    auto_simulation_mode = false;
    simulated_voltage = (float)atoi(argv[1]);
    shell_print(shell, "Manual Override: %d V", (int)simulated_voltage);
    return 0;
}

static int cmd_auto_mode(const struct shell *shell, size_t argc, char **argv)
{
    auto_simulation_mode = true;
    shell_print(shell, "Auto Simulation Enabled");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensor,
    SHELL_CMD(set, NULL, "Set voltage manually", cmd_set_voltage),
    SHELL_CMD(auto, NULL, "Enable auto simulation", cmd_auto_mode),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(sensor, &sub_sensor, "Sensor Simulation Commands", NULL);