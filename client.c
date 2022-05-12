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

int PRINT_FLAG = 0;

// To do:
// !!! In while loop for graphics insert "ifs" that check:
// 1) new messages
// 2) status changes online/offline

// !!! handler for server messages
// 1) specific commands such as status update or new user init
// 2) regular message: get info who sent it and change apropriate flag to 1

// !!!  Queue with users
// each element as a struct would have:
// 1) name 
// 2) pearsonal print flag 
// 3) pointer to beginning of the messages consiquence
// 4) status (already have one, but I will update it in the on_read callback)

// !!! Queue with messages 
// 1) messages

// !!! RESIZING FOR WINDOW in the end, if i would have time for it

//for server we should add ability to store all messages by saving them when the server recives something

char* server_reply;
pthread_mutex_t mutex_read;
pthread_mutex_t mutex_write;

static uv_loop_t* loop = NULL;
//static uv_stream_t* stream = NULL;

typedef struct user_input {
    uv_stream_t* stream;
    char* user_input;
    uv_connect_t* connect;
}user_data;

void on_write(uv_write_t* req, int status)
{
  if (status) {
    perror ("uv_write error ");
        return;
  }
    free(req);
}

void on_close(uv_handle_t* handle)
{
  printw("closed.");
  refresh();
}

void *potok_func (void* input)
{   
    initscr();

    int pointer = 0;
    int pointer_back = 1;
    char c;
    int string_num = 0;
    int start_pos = 0;
    int xMAX, yMAX;
    getmaxyx(stdscr, yMAX, xMAX);

    user_data* new = (user_data*)input;

    printw ("Conection established %p.\n", new -> connect);
    refresh ();
    string_num ++;

    int heigth, start_x;
    heigth = 5;
    start_x = 5;

    WINDOW* win = newwin(heigth, xMAX - 12, yMAX - 6, start_x);
    refresh ();
    nodelay(win, true);
    echo();


    box (win, 0, 0);
    wmove (win, 1, 1);    
    wrefresh (win);

    //user's input function
    while (1) 
    {    
            if (PRINT_FLAG) 
            {
                mvprintw (string_num, start_pos, "%s", server_reply);
                refresh();
                PRINT_FLAG = 0;
                string_num++;
            }

        c = wgetch(win);

            if ((int)c == 127 && pointer_back >= 1) {
                pointer_back --;
                pointer--;
                bzero (new->user_input + pointer, 1);
                wmove (win, 1, pointer_back);
                wclrtoeol(win);
                 
                box (win, 0, 0);
                wrefresh(win);
                
                wrefresh(win);
            }

            else if ((int)c != 10 && (int)c != -1 && (int)c != 127)
            {
                *(new -> user_input + pointer) = c;
                pointer ++;
                pointer_back++;
            }
        
            else if ((int)c != -1 && pointer) //нажат enter и сообщение не пусто 
            {
                uv_buf_t buffer[] = {
                {.base = new -> user_input, .len = strlen (new -> user_input)}
                };
                uv_write_t *req = malloc(sizeof(uv_write_t));
                uv_write(req, new -> stream, buffer, 1, on_write);

                bzero(new -> user_input, MAX_SIZE);
                free (new -> user_input);
                new -> user_input = (char*)calloc(MAX_SIZE, sizeof(char));

                pointer = 0;
                pointer_back = 1;
                wclear(win);
                box (win, 0, 0);
                wrefresh(win);

                wmove (win, 1, 1);
            }
    }
    endwin();
    return 0;
}

void write2(uv_stream_t* stream, char *data, int len2) {
    uv_buf_t buffer[] = {
        {.base = data, .len = len2}
    };
    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_write(req, stream, buffer, 1, on_write);
}

void alloc_buffer (uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    if(buf->len == 0)
    {
        buf -> base = (char*)malloc(suggested_size);
    }
    else
    {
    buf -> base = (char*)realloc(buf -> base, suggested_size);
    }
    buf -> len  = suggested_size;
    uv_handle_set_data(handle, buf -> base);
}

void on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t* buf)
{
    if (nread > 0) {

        server_reply = buf -> base;
        PRINT_FLAG = 1;
        
        while (PRINT_FLAG) {
        
        };

        bzero(buf->base, buf->len);
    }
    else
    {
    uv_close((uv_handle_t*)tcp, on_close);
    }
    free(buf->base);
}

void on_connect (uv_connect_t* connect, int status) 
{ 
    fprintf (stderr, "%d\n", status);
    if (status < 0) 
    {
        fprintf (stderr, "New connection error %s\n", uv_strerror(status));
        return;
    }
    else
    {

        uv_stream_t* stream = connect -> handle; //this is our handle
        write2(stream, "PING\n", 6);

        char* user_input = (char*)calloc(MAX_SIZE, sizeof(char));
        user_data* input = malloc (sizeof(user_data));

        input -> stream = stream;
        input -> user_input = user_input;
        input -> connect = connect;

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        
        pthread_create(&tid, &attr, potok_func, input);
        uv_read_start(stream, alloc_buffer, on_read); //this guy reading answer from server
        
        free (connect);

    }
}


void establishing_connection(char* IP_addr, int nport, uv_tcp_t** user_socket)
{
    uv_connect_t* connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    uv_tcp_init (loop, *user_socket);

    struct sockaddr_in dest; // where to connect
    uv_ip4_addr (IP_addr, nport, &dest); //return a struct with adresses for programm 
    
    uv_tcp_connect(connect, *user_socket, (const struct sockaddr*) &dest, on_connect);
    
}

int main() {

    uv_tcp_t* socket = (uv_tcp_t*)malloc(sizeof(uv_tcp_t)); 

    server_reply = malloc(MAX_SIZE);

    loop = uv_default_loop ();

    establishing_connection("0.0.0.0", 7000, &socket);

    uv_run(loop, UV_RUN_DEFAULT);  

    return 0;
}
