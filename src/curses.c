#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
//gcc -o console src/curses.c -lncurses

    /*
     * Thoughts:
     *  Create a string array of size LINES - 1 which holds the output to the
     *  console. It is a cyclical array and protected by a mutex. This way, 
     *  every time we get new input to the console, we can redraw the screen
     *  with correct output. This also might be the best way to handle it for
     *  a readline implementation if ncurses doesn't work out.
     */

static int quit = 0;

void 
sigint_handler (int sig)
{
    quit = 1;
}

static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;
static WINDOW *console_window;

void
console_log (char ch)
{
    pthread_mutex_lock(&console_mutex);
    wprintw(console_window, "%c\n", ch);
    wrefresh(console_window);
    pthread_mutex_unlock(&console_mutex);
}

void *
print_thread (void *arg)
{
    sleep(2);
    console_log('%');
    sleep(1);
    console_log('%');
    sleep(3);
    console_log('%');
    sleep(1);
    console_log('%');
    return NULL;
}

int main()
{   
    const char *welcome = "Dialogue v0.0 with Lua v5.2\n"
                          "↳  type `exit` to quit.\n";
    const int max_input = 256;
    char input[max_input];
    WINDOW *interpreter = NULL;
    int max_row, max_col;
    int i, ch, index = 0;
    pthread_t thread;

    signal(SIGINT, sigint_handler);

    initscr();
    start_color();
    getmaxyx(stdscr, max_row, max_col);
    cbreak();

    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    /* big area for console output */
    console_window = newwin(LINES - 1, max_col, 0, 0);
    wbkgd(console_window, COLOR_PAIR(1));

    /* only need single line for interpreter input */
    interpreter = newwin(1, max_col, LINES - 1, 0);
    wbkgd(interpreter, COLOR_PAIR(2));

    wprintw(console_window, welcome);
    wrefresh(console_window);

    wprintw(interpreter, "> ");
    wrefresh(interpreter);

    /* only start the console after we've printed all we need to the console */
    pthread_create(&thread, NULL, print_thread, NULL);

    nodelay(interpreter, TRUE);
    timeout(25);

    for (i = 0; i < index; i++)
        input[i] = '\0';

    for (;;) {
        ch = wgetch(interpreter);

        if (ch != ERR) {
            console_log(ch);
            werase(interpreter);

            //if (ch == KEY_ENTER) {
            //    printf("%s\n", input);
            //    wprintw(console, "%s\n", input);
            //    wrefresh(console);
            //    werase(interpreter);

            //    for (i = 0; i < index; i++)
            //        input[i] = '\0';

            //    index = 0;
            //} else {
            //    input[index] = ch;
            //    index++;
            //}
        }

        if (quit)
            break;
    }

    //while (!quit) {
    //    wgetnstr(interpreter, input, max_input);

    //    if (strlen(input) > 0) {
    //        wprintw(console, "%s\n", input);
    //        wrefresh(console);
    //    }
    //}

    pthread_join(thread, NULL);
    delwin(console_window);
    delwin(interpreter);
    endwin();
    return 0;
}
