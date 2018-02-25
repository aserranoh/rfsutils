
#include "config.h"

#include <err.h>        // err
#include <getopt.h>     // getopt_long
#include <signal.h>     // sigaction
#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <string.h>     // strcmp
#include <sys/wait.h>   // waitpid
#include <time.h>       // clock_gettime
#include <unistd.h>     // fork

#include <rfsgpio.h>

#include "daemon.h"
#include "dictint.h"
#include "list.h"
#include "parsenum.h"
#include "readline.h"

// Short options
//   * h: help
//   * v: version
#define OPTSTRING   "hvc:dp:"

// Name of the program, to use it in the version and help string
#define PROGNAME    "buttonsd"

// Configuration file, that contains the actions that must be executed upon
// the clicks in the buttons.
#define DEFAULT_CONFIGFILE  SYSCONFDIR "/buttonsd.conf"

// Time that must be elapsed between pressing a button and releasing it to
// consider it a long click (the time is in seconds).
#define TIME_LONG_CLICK 2.0

// Transform an element of struct timespec to a double. Converting the struct
// timespec to a double is useful to substract them and calculate time deltas.
#define TS_TO_DOUBLE(ts)    ((double)ts.tv_sec \
                            + (double)(ts.tv_nsec)/1000000000.0)

// Timeout to interrupt the poll system call (in ms). This is necessary to
// avoid certain race conditions when the flag stop is set.
#define POLL_TIMEOUT    5000

// Enumeration of event types.
// Event types are the different events that a button can generate. For now
// only two are implemented:
//   * Short click (or simply, click).
//   * Long click.
enum event_type_t {EVENT_CLICK, EVENT_LONG_CLICK};

// Struct that contains an event type and an action that is executed upon that
// event. The action is actually a string that contains a command to be
// executed with the 'system' syscall.
struct event_t {
    // The event type
    enum event_type_t type;

    // The action string, command to be executed with system
    char *action;
};

// Struct that contains the list of events and actions related to a given
// button. The button is identified by the GPIO pin number where it is
// connected.
struct button_t {
    // The GPIO pin where the button is
    struct gpio_t gpio;

    // Timestamp of the last button down event. Used to calculate the time
    // elapsed when the button is released.
    struct timespec timestamp;

    // The list of events and actions. This list can contain several times the
    // same event type.
    struct list_t events;
};

// Dictionary that stores the buttons indexed by the GPIO pin number where they
// are connected.
struct dictint_t dict_buttons;

// Dictionary that stores the buttons indexed by the file descriptor number
// used to poll that button's GPIO.
struct dictint_t dict_fds;

// List of poll descriptors, used to poll for changes in state in the GPIO
// pins of all the buttons.
struct pollfd *poll_descriptors = NULL;

// Path to the configuration file
const char *config = DEFAULT_CONFIGFILE;

// Flag that tells if this process must be daemonized
int is_daemon = 0;

const char *pidfile = 0;

// Number of poll descriptors in the previous array
size_t npoll_descriptors = 0;

// Flag that indicates that the process must stop. This flag is set by a
// signal handler.
int stop = 0;

/* Signals handler.
   This handler is executed upon reception of the signals SIGINT and SIGTERM.
   The stop flag it is set, which indicates that the process must finish.

   Parameters:
     * signum: the signal received (although it is not used).
*/
void
signal_handler(int signum)
{
    // Set the done flag
    stop = 1;
}

// Print a help message and exit.
void
print_help()
{
    printf("Usage: " PROGNAME " [options]\n"
"Options:\n"
"  -h, --help                  Show this message and exit.\n"
"  -v, --version               Show version information.\n"
"  -c PATH, --config PATH      Give the configuration file.\n"
"  -d, --daemonize             Daemonize this process.\n"
"  -p PATH, --pidfile PATH     Create a pidfile.\n\n"

"Report bugs to:\n"
"Antonio Serrano Hernandez (" PACKAGE_BUGREPORT ")\n"
    );
    exit(0);
}

// Print a version message and exits
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
    struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"config", required_argument, 0, 'c'},
        {"daemonize", no_argument, 0, 'd'},
        {"pidfile", required_argument, 0, 'p'},
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
            case 'c':
                config = optarg;
                break;
            case 'd':
                is_daemon = 1;
                break;
            case 'p':
                pidfile = optarg;
                break;
            case '?':
                exit(1);
            default:
                break;
        }
    } while (o != -1);
}

// Set the handler for the signals SIGINT and SIGTERM, to stop this process.
void
set_signals()

{
    struct sigaction sa;
    sigset_t mask;

    sigemptyset(&mask);
    sa.sa_handler = signal_handler;
    sa.sa_mask = mask;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* Initialize some data structures that the application must use

   Return 0 if the initialization was correct, 1 otherwise.
*/
int
init_data_structures()
{
    // Initialize the buttons dictionary
    if (dictint_init(&dict_buttons)) {
        warn("error creating buttons dictionary");
        return 1;
    }
    // Initialize the file descriptor's dictionary
    if (dictint_init(&dict_fds)) {
        warn("error creating descriptors dictionary");
        return 1;
    }
    return 0;
}

/* Return a button given the GPIO pin number where it is connected.

   Parameters:
     * gpio: the GPIO pin number where the button is.
*/
struct button_t*
get_button_by_pin(gpio_pin_t gpio)
{
    return dictint_get(&dict_buttons, gpio);
}

/* Return a button given the file descriptor of its opened value file.
   The button's (GPIO) value file is opened when it is being polled.

   Parameters:
     * fd: the file descriptor used to poll that button's GPIO.
*/
struct button_t*
get_button_by_fd(int fd)
{
    return dictint_get(&dict_fds, fd);
}

/* Add a new button to the dictionary dict_buttons. Then, return the new
   button.

   Parameters:
     * gpio: the GPIO pin number where the button is connected.

   Return the button created, or NULL if error.
*/
struct button_t*
add_button(gpio_pin_t gpio)
{
    struct button_t *g;

    if (!(g = (struct button_t*)malloc(sizeof(struct button_t)))) {
        return NULL;
    }

    // Initialize the instance g
    g->gpio.pin = gpio;
    g->gpio.flags = 0;
    if (rfs_gpio_open(&(g->gpio), RFS_GPIO_IN)) {
        warn("cannot open GPIO #%d", gpio);
        return NULL;
    }
    // Add the element to the dictionary
    if (dictint_add(&dict_buttons, gpio, g)) {
        warn("adding new button to dictionary");
        rfs_gpio_close(&(g->gpio));
        return NULL;
    }
    // Initialize the list of events
    if (list_init(&(g->events))) {
        warn("error initializing list of events");
        return NULL;
    }
    return g;
}

/* Add an event/action couple to a given GPIO button.

   Parameters:
     * button: the button where the event must be added.
     * event: the event to add.

   Return 0 if the event was correctly added to the button, 1 otherwise.
*/
int
add_event(struct button_t *button, struct event_t *event)
{
    if (list_add(&(button->events), event)) {
        warn("adding event");
        return 1;
    }
    return 0;
}

/* Parse a single line that contains the information about an event/action for
   a button:

   <gpio_pin_number>_<event>=<action>

   Where:
     * gpio_pin_number: the GPIO pin number where the button is connected.
     * event: click or long_click.
     * action: command to execute with the 'system' syscall.

   Parameters:
     * line: line in the configuration file that contains the pin/event and
         the action (contains a string ended by a null character).
     * linenum: line number in the file (for error reporting purposes).
     * gpio_pin: output number of the GPIO pin for that event/action.

   Return the parsed event in case of success, NULL otherwise.
*/
struct event_t*
parse_button_action(char *line, int linenum, gpio_pin_t *gpio_pin)
{
    char *ptr, *eptr;
    struct event_t *event;
    unsigned int pin;

    // Allocate the event
    event = (struct event_t*)malloc(sizeof(struct event_t));
    if (!event) {
        warnx("allocating event");
        return NULL;
    }
    // Parse the event/action
    // Search the '=' sign
    ptr = line;
    while (*ptr != '=' && *ptr != '\0') ptr++;
    if (*ptr == '\0') {
        warnx("%s: at line %d: expected '='", config, linenum);
        return NULL;
    }
    // Parse the pin number
    // A new pin variable is used to recover the pin number because we are not
    // sure if gpio_pin_t and unsigned int have the same size.
    if (parse_number(line, (const char **)&eptr, &pin)) {
        warnx("%s: at line %d: GPIO pin out of range", config, linenum);
        return NULL;
    }
    *gpio_pin = pin;
    // Parse the event
    *ptr = '\0';
    if (strcmp(eptr, "_click") == 0) {
        // Simple click event
        event->type = EVENT_CLICK;
    } else if (strcmp(eptr, "_long_click") == 0) {
        // Long click event
        event->type = EVENT_LONG_CLICK;
    } else {
        warnx("%s: at line %d: unknown event", config, linenum);
        return NULL;
    }
    event->action = strdup(ptr + 1);
    if (!event->action) {
        warn("error in strdup");
        return NULL;
    }
    return event;
}

/* Read the configuration file.
   The configuration file has variables that describe the actions to take
   upon click on any of the buttons.
   Empty lines and lines that start by '#' are ignored.
   The rest of the lines must be of the form:

   <number>_<click_type>=<action>

   Where:
     * number: is the GPIO pin where the button is connected.
     * click_type: must be 'click' or 'long_click'. Identifies the type of
         click to be performed to the button. A click is executed immediately
         when the user releases the button. For a long click to take effect
         the user must hold the button at least 2 seconds.
     * action: action to be executed. It must be a command line that will be
         executed with the 'system' syscall.

   Return 0 if the configuration file was correctly read, 1 otherwise.
*/
int
read_configuration_file()
{
    FILE *f;
    struct line_t l = LINE_INIT;
    int linenum = 1;
    gpio_pin_t gpio_pin;
    struct button_t *button;
    struct event_t *event;
    int r;

    // Open the configuration file
    f = fopen(config, "r");
    if (!f) {
        warn("cannot open '%s'", config);
        return 1;
    }
    // Iterate over the lines of the configuration file to load the actions
    while ((r = readline(&l, f)) > 0) {
        // Jump blank lines and comment lines
        if (l.line[0] != '\n' && l.line[0] != '#') {
            event = parse_button_action(l.line, linenum, &gpio_pin);
            if (!event) {
                // Error parsing the line with the button/event/action
                return 1;
            } else {
                // Add the button GPIO/event info
                // Check if the button is already in the dictionary
                button = get_button_by_pin(gpio_pin);
                if (!button) {
                    // The button is not in the dictionary, create it
                    button = add_button(gpio_pin);
                }
                if (button) {
                    // The button was in the dictionary or was correctly added
                    // Add the event
                    if (add_event(button, event)) {
                        // Error adding event
                        return 1;
                    }
                } else {
                    // Error adding the button
                    return 1;
                }
            }
        }
        linenum++;
    }
    fclose(f);
    if (r < 0) {
        warn("error reading configuration file");
        return 1;
    }
    readline_free(&l);
    return 0;
}

/* Set all the buttons in a listening state. That means setting the edge
   that they listen (listen for both raising and falling edges, so pressing
   and releasing the buttons) and obtaining the fd for polling.

   Return 0 if all the descriptors are get, 1 in case of error.
*/ 
int
get_poll_descriptors()
{
    int gpio;
    struct button_t *b;
    struct dictint_iterator_t i;
    int nfd = 0;

    // Allocate the descriptors array
    npoll_descriptors = dictint_size(&dict_buttons);
    poll_descriptors = (struct pollfd*)malloc(
        sizeof(struct pollfd) * npoll_descriptors);
    if (!poll_descriptors) {
        warn("error alloc");
        return 1;
    }
    // Fill the array of poll descriptors
    dictint_iterator_begin(&dict_buttons, &i);
    while (dictint_iterator_next(&i, &gpio, (void **)&b)) {
        // First, configure the edge to listen of the button's GPIO
        // listen both, raising and falling
        if (rfs_gpio_set_edge(&(b->gpio), RFS_GPIO_BOTH)) {
            warn("error setting edge in GPIO %d", b->gpio.pin);
            return 1;
        }
        // Then get the poll descriptors
        if (rfs_gpio_get_poll_descriptors(
            &(b->gpio), &(poll_descriptors[nfd])))
        {
            warn("error getting descriptor from GPIO %d", b->gpio.pin);
            return 1;
        }
        // Add the button to the dictionary indexed by fd
        if (dictint_add(&dict_fds, b->gpio.fd, b)) {
            warn("adding new button to dictionary by fd");
            return 1;
        }
        nfd++;
    }
    return 0;
}

/* Execute the actions described by a type of event and a button.

   Parameters:
     * button: the button clicked.
     * event: the type of event.
*/
void
execute_actions(struct button_t *button, enum event_type_t event)
{
    struct list_iterator_t i;
    struct event_t *e;
    pid_t pid;

    list_iterator_begin(&(button->events), &i);
    while (list_iterator_next(&i, (void **)&e)) {
        if (e->type == event) {
            pid = fork();
            if (pid < 0) {
                warn("cannot fork");
            } else if (pid == 0) {
                exit(WEXITSTATUS(system(e->action)));
            }
        }
    }
}

/* Main loop that waits for events in the buttons and executes the attached
   actions.

   Return 0 if the loop is stoped without errors, 1 otherwise.
*/
int
run()
{
    struct button_t *button;
    enum gpio_value_t state;
    struct timespec current_ts;
    double t0, t1;

    while (!stop) {
        // Block until any button changes its state
        if (poll(poll_descriptors, npoll_descriptors, POLL_TIMEOUT) > 0) {
            // Search among the poll descriptors which ones have input events
            for (int i = 0; i < npoll_descriptors; i++) {
                if (poll_descriptors[i].revents & POLLPRI) {
                    // Obtain the button related to that fd
                    button = get_button_by_fd(poll_descriptors[i].fd);
                    // Check the state (low or high) of the button
                    state = rfs_gpio_get_value(&(button->gpio));
                    if (state < 0) {
                        warn("cannot obtain GPIO state");
                        return 1;
                    }
                    if (state == RFS_GPIO_HIGH) {
                        // The button is pressed, timestamp the event
                        clock_gettime(CLOCK_MONOTONIC, &(button->timestamp));
                    } else {
                        // The button was released, determine if it was a short
                        // click or a long click and execute the action
                        clock_gettime(CLOCK_MONOTONIC, &current_ts);
                        t0 = TS_TO_DOUBLE(button->timestamp);
                        t1 = TS_TO_DOUBLE(current_ts);
                        if (t1 - t0 < TIME_LONG_CLICK) {
                            // It was a short click
                            execute_actions(button, EVENT_CLICK);
                        } else {
                            // It was a long click
                            execute_actions(button, EVENT_LONG_CLICK);
                        }
                    }
                    // Get again the poll descriptors
                    if (rfs_gpio_get_poll_descriptors(
                        &(button->gpio), &(poll_descriptors[i])))
                    {
                        warn("error getting descriptor from GPIO %d",
                            button->gpio.pin);
                        return 1;
                    }
                }
            }
        }
        // Cleanup zombies
        waitpid(-1, NULL, WNOHANG);
    }
    return 0;
}

// Close (unexport) the GPIOs used by the buttons.
void
close_gpios()
{
    struct dictint_iterator_t i;
    int gpio;
    struct button_t *b;

    dictint_iterator_begin(&dict_buttons, &i);
    while (dictint_iterator_next(&i, &gpio, (void**)&b)) {
        rfs_gpio_close(&(b->gpio));
    }
}

/* Main entry point.

   Parameters:
     * argc: number of command line arguments.
     * argv: array of command line arguments.
*/
int
main(int argc, char **argv)
{
    int e = 0;

    // Parse the command line arguments
    parse_args(argc, argv);
    // Set a handler for the signals SIGINT and SIGTERM to have a mechanism
    // to stop this process.
    set_signals();
    // Init some global data structures
    if (init_data_structures()) {
        return 1;
    }
    // Read the configuration file that contains the actions to execute upon
    // the events on the buttons 
    if (read_configuration_file()) {
        e = 1;
        goto end;
    }
    // Daemonize, if demanded
    if (is_daemon) {
        if (daemonize(pidfile)) {
            return 1;
        }
    }
    // Compile a list of poll descriptors, used to poll all the buttons for
    // changes of state
    if (get_poll_descriptors()) {
        e = 1;
        goto end;
    }
    // Execute the main loop
    if (run()) {
        e = 1;
    }
end:
    close_gpios();
    return e;
}

