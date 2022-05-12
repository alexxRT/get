#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>

#define MAX_SIZE 1000

typedef struct _win {
    WINDOW* win;
    int highth;
    int width;
    int start_y;
    int start_x;
    int string_num;
} window;

typedef struct _user {
    window win;
    char* name;
    char* status;
} new_user;

void user_init (new_user* user_base, int num, int width, int highth, int start_y, int start_x, int string_num)
{
        user_base [num].win.win  = newwin(highth, width, start_y, start_x);
        user_base [num].win.width = width;
        user_base [num].win.highth = highth;
        user_base [num].win.start_y = start_y;
        user_base [num].win.start_x = start_x;
        user_base [num].win.string_num = string_num;
}

void user_input (WINDOW* display, new_user* user) {
    //make function for window init

    char* message = malloc(sizeof(char)*MAX_SIZE);

    int pointer = 0;
    char c;
    int start_pos = 0;
    int xMAX, yMAX;
    getmaxyx(stdscr, yMAX, xMAX);

    refresh ();

    int heigth;
    heigth = 5;

    WINDOW* input_win = newwin(heigth, user -> win.width - 2, yMAX - 6, user -> win.start_x + 1);
    refresh ();
    nodelay(input_win, true);
    echo();

    box (input_win, 0, 0);
    wmove (input_win, 1, 1);    
    wrefresh (input_win);

    while ((int)c != 27) {

    c = wgetch(input_win);

            if ((int)c == 127 && pointer > 1)
            {
                pointer--;
                wmove (input_win, 1, pointer + 1);
                wclrtoeol(input_win);
                bzero (message + pointer, MAX_SIZE);
                 
                box (input_win, 0, 0);
                wrefresh (input_win);
            }

            else if ((int)c != 10 && (int)c != -1 && (int)c != 127 && (int)c != 27)
            {
                message[pointer] = c;
                pointer ++;
            }
        
            else if ((int)c != -1 && pointer) //нажат enter и сообщение не пусто 
            {   
                char* final_message = malloc(MAX_SIZE);

                strcpy (final_message, "TO");
                strcat (final_message, user -> name);
                strcat (final_message, " ");
                strcat (final_message, message);

                mvwprintw (display,  user -> win.string_num, 1, final_message);
                wrefresh (display);

                bzero (final_message, MAX_SIZE);
                bzero (message, MAX_SIZE);
                free (final_message);

                user -> win.string_num ++;

                pointer = 0;
                wclear(input_win);
                box (input_win, 0, 0);
                wrefresh(input_win);

                wmove (input_win, 1, 1);
            }
            else 
            {
                wmove (input_win, 1, pointer + 1);
                wclrtoeol(input_win);
                 
                box (input_win, 0, 0);
                wrefresh(input_win);
            }
    }
}

void usrwin_open (new_user* user)
{
    WINDOW* dialog_win = user -> win.win;
    refresh();
    box (dialog_win, 0, 0);
    wrefresh(dialog_win);

    user_input (dialog_win, user);

    //usrwin_close();  
}

int main ()
{   
    initscr();
    noecho();

    new_user *user_base = malloc(sizeof(new_user) * MAX_SIZE);

    int yMAX, xMAX;
    getmaxyx(stdscr, yMAX, xMAX);

    //static massiv with users initialization
    for (int i = 0; i < 5; i ++)
    {
        user_base[i].name = malloc (sizeof(char)*MAX_SIZE);
        scanf ("%s", user_base[i].name);
        user_base[i].status = malloc (sizeof(char)*MAX_SIZE);
        user_base[i].status = "offline";
        user_init (user_base, i, (2*xMAX/3) - 3, yMAX - 1, 1, xMAX/3 + 3, 1);
    }

    WINDOW* menu_win = newwin (yMAX - 1, xMAX/3, 1, 1);    
    refresh();

    box (menu_win, 0, 0);
    wrefresh (menu_win);
    keypad (menu_win, true);
    int highlight = 0;

    while (1) 
    {  

    //if (NEW_USER) {
    //  user_init();
    //  user_num ++;    
    //}

    noecho();
    for (int i = 0; i < 5/*user_num*/; i ++) 
    {
        if (highlight == i) {
            wattron (menu_win, A_REVERSE);
        }
        mvwprintw (menu_win, i + 1, 1, user_base[i].name);
        mvwprintw (menu_win, i + 1, xMAX/3 - 9, user_base[i].status);
        wattroff (menu_win, A_REVERSE);
    }

    int c = wgetch (menu_win);

    switch (c)
    {
        case KEY_UP:
            if (highlight == 0){
                highlight = 0;
            }

            else {
            highlight --;
            }

            break;
        case KEY_DOWN:
            if (highlight == 4) {
                highlight = 4;
            }
            else {
                highlight ++;
            }
            break;
        case 10:
            usrwin_open (&user_base[highlight]);
        default:
            break;
    }    
}
    endwin();
    return 0;
}