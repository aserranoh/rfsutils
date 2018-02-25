
#include "config.h"

#include <err.h>    // err
#include <getopt.h> // getopt_long
#include <stdio.h>  // printf
#include <stdlib.h> // exit
#include <time.h>   // nanosleep

#include <rfsgpio.h>

#include "parsenum.h"

#define OPTSTRING   "g:hv"
#define PROGNAME    "ledblink"

// GPIO pin where the LED is connected
gpio_pin_t gpio;

// Blink sequence
const char *blinkseq = 0;

// Print help message and exits
void
print_help()
{
    printf("Usage: " PROGNAME " [options] SEQUENCE\n"
"Options:\n"
"  -h, --help              Show this message and exit.\n"
"  -v, --version           Show version information.\n"
"  -g=GPIO, --gpio=GPIO    Give the GPIO pin where the led is connected.\n\n"

"Report bugs to:\n"
"Antonio Serrano Hernandez (" PACKAGE_BUGREPORT ")\n"
    );
    exit(0);
}

// Print version message and exits
void
print_version()
{
    printf(PROGNAME " (" PACKAGE_NAME ") " PACKAGE_VERSION "\n"
"Copyright (C) 2018 Antonio Serrano\n"
"This is free software; see the source for copying conditions.  There is NO\n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
    );
    exit(0);
}

/* Parse the command line arguments.

   Parameters:
     * argc: number of command line arguments.
     * argv: command line string arguments.
*/
void
parse_args(int argc, char **argv)
{
    const char *strgpio;
    unsigned int lgpio;
    const char *endptr;

    struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"gpio", required_argument, 0, 'g'},
        {0, 0, 0, 0}
    };
    int o;

    do {
        o = getopt_long(argc, argv, OPTSTRING, long_opts, 0);
        switch (o) {
            case 'h':
                print_help();
            case 'v':
                print_version();
            case 'g':
                strgpio = optarg;
                break;
            case '?':
                exit(1);
            default:
                break;
        }
    } while (o != -1);
    // Get the blink sequence
    if (optind == argc) {
        errx(1, "missing blink sequence");
    }
    blinkseq = argv[optind];

    // Check the value of the GPIO pin
    if (!strgpio) {
        errx(1, "missing -g option");
    }
    if (strgpio[0] == '\0') {
        errx(1, "invalid GPIO pin number");
    }
    if (parse_number(strgpio, &endptr, &lgpio)) {
        errx(1, "invalid GPIO pin number");
    }
    if (*endptr != '\0') {
        errx(1, "invalid GPIO pin number");
    }
    gpio = lgpio;
}

/* Check that the blink sequence is correct.
   The blink sequence must be formed by the characters 'o' (ON) and '_' (OFF)
   each followed by a sequence of digits (the time in milliseconds).
*/
void
check_blink_sequence()
{
    const char *ptr = blinkseq, *eptr;
    int pos = 0;
    unsigned int t;

    while (*ptr != '\0') {
        switch (*ptr) {
            case 'o':
            case '_':
                // Check number that follows
                if (parse_number(ptr + 1, &eptr, &t)) {
                    errx(1, "at position %d: wrong time", pos + 1);
                }
                // Check that indeed there's a number after the o or _ chars
                if (eptr == ptr + 1) {
                    errx(1, "at position %d: time expected", pos + 1);
                }
                pos += (eptr - ptr);
                ptr = eptr;
                break;
            default:
                // Wrong character
                errx(1,
                    "at position %d: wrong character '%c' in blink sequence",
                    pos, *ptr);
        }
    }
}

// Execute the blink sequence
void do_sequence() {
    const char *ptr = blinkseq;
    struct gpio_t g = {gpio, 0};
    unsigned int t;
    struct timespec ts;

    // Initialize the GPIO
    if (rfs_gpio_open(&g, RFS_GPIO_OUT_LOW)) {
        err(1, "opening GPIO #%d", gpio);
    }

    while (*ptr != '\0') {
        switch (*ptr) {
            case 'o':
                // Turn ON the led
                rfs_gpio_set_value(&g, RFS_GPIO_HIGH);
                break;
            case '_':
                // Turn OFF the led
                rfs_gpio_set_value(&g, RFS_GPIO_LOW);
                break;
            default: break;
        }
        // Read the number that follows and sleep that number of
        // milliseconds
        parse_number(ptr + 1, &ptr, &t);
        ts.tv_sec = t/1000;
        ts.tv_nsec = (t%1000) * 1000000;
        nanosleep(&ts, NULL);
    }
    // Switch off the LED
    rfs_gpio_set_value(&g, RFS_GPIO_LOW);
    // Close the GPIO pin
    rfs_gpio_close(&g);
}

int
main(int argc, char **argv)
{
    parse_args(argc, argv);
    check_blink_sequence();
    do_sequence();
    return 0;
}

