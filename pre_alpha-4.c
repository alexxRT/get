#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_CONNECTIONS 20
#define MAX_MESSAGE 6144
void reverse(char *buf) {
    for (int i = 0; i < strlen(buf)/2; i++) {
            char tmp = buf[i];
            buf[i] = buf[strlen(buf) - 1 - i];
            buf[strlen(buf) - 1 -i] = tmp;
    }
}

void itoa(int num, char* buf) {
    if ( num != 0 ){
        int i, signum = 0;
        if ( num < 0) {
            signum = 1;
            num = -num;
        }
        i = 0;
        while ( num > 0) {
            buf[i++] = (char)(num%10) + '0';
            num/=10;
        }
        if ( signum ) buf[i++] = '-';
        buf[i] = '\0';
        reverse(buf);
        return ;
    }
    int i = 0;
    buf[i++] = '0';
    buf[i] = '\0';
    reverse(buf);
}

typedef int (*cmd_handler)(uv_buf_t* buf, uv_tcp_t* client);

int cmd_quit();
int cmd_to();
int cmd_name_set();
int cmd_ping();
void on_conn_write();

struct _command {
	char name[16];
	cmd_handler handler;
};

struct _command COMMANDS[] = {
	{"QUIT", cmd_quit},
	{"TO", cmd_to},
    {"NAME_SET", cmd_name_set},
    {"PING", cmd_ping},
	{"", NULL}
};

typedef struct _User{
    uv_tcp_t* client ;
    char* name;
} User;

struct _User users[MAX_CONNECTIONS];
uv_loop_t *loop;

void init_users(User* users){
    for(int i = 0; i < MAX_CONNECTIONS; i ++){
        users[i].client = NULL;
        users[i].name = (char*)realloc(users[i].name, MAX_LINE*sizeof(char));
    }
    fprintf(stderr, "Users are intialized\n");
}

int send_message(uv_buf_t* buf, int offset, uv_stream_t* recipient, uv_tcp_t* client) {
    char message[MAX_MESSAGE];
    bzero(message, MAX_MESSAGE);
    sscanf( buf->base + offset + 1, "%[^\n]", message);
    fprintf(stderr, "message read %s\n", message);
    if ( strlen(message) < MAX_MESSAGE - 1 ) {
        message[strlen(message)] = '\n';
        message[strlen(message) + 1] = '\0';
    }
    else {
        message[MAX_MESSAGE - 2] = '\n';
        message[MAX_MESSAGE - 1] = '\0';
    }
    fprintf( stderr, "%s!", message);
    char* name = (char*)malloc(MAX_MESSAGE * sizeof(char));
    bzero(name, MAX_MESSAGE);
    for(int i = 0; i < MAX_CONNECTIONS; i++){
        if (users[i].client == client){
            strcpy(name, users[i].name);
            break;
        }
    }
    char dots[] = ": ";
    strcat(name, dots);
    strcat(name, message);
    uv_buf_t* recbuf = (uv_buf_t*)malloc(sizeof(uv_buf_t));
    recbuf->base = name;
    recbuf->len = strlen(name);
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
	uv_write(req, recipient, recbuf, 1, on_conn_write);
    recbuf->len = 0;
    free(recbuf->base);
    free(recbuf);
    buf->len = snprintf(buf->base, buf->len, "Message sent\n");
    return 0;
}

int read_name(uv_buf_t* buf, char* name, int offset, uv_stream_t** recipient) {
    sscanf(buf->base + offset, "%s", name);
    fprintf(stderr, "%s\n", name);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if ( !strcmp(users[i].name, name) ){
            *recipient = (uv_stream_t*)users[i].client;
            fprintf(stderr, "name read %s\n", users[i].name);
            return 1;
        }
    }
    return 0;
}

int cmd_quit(uv_buf_t* buf, uv_tcp_t* client) {
	buf->len = snprintf(buf->base, buf->len, "GOOD BYE!\n");
	return 1;
}

int cmd_ping(uv_buf_t* buf, uv_tcp_t* client) {
	buf->len = snprintf(buf->base, buf->len, "PINGED\n");
	return 0;
}

int cmd_to( uv_buf_t* buf, uv_tcp_t* client) {
    char *name = (char*)calloc(MAX_LINE, sizeof(char));
    
    uv_stream_t** recipient = (uv_stream_t**)malloc(sizeof(uv_stream_t*));;
    if ( read_name(buf, name, 2, recipient) ){
        send_message(buf, 2 + strlen(name), *recipient, client);
    } else buf->len = snprintf(buf->base, buf->len, "Wrong name!\n");
    return 0;
}

int cmd_name_set(uv_buf_t* buf, uv_tcp_t* client) {
    char *name = (char*)calloc(MAX_LINE, sizeof(char));
    uv_stream_t** check = (uv_stream_t**)malloc(sizeof(uv_stream_t*));
    if ( read_name(buf, name, 8, check) ){
        buf->len = snprintf(buf->base, buf->len, "Name is taken!\n");
    } else {
        for( int i = 0; i < MAX_CONNECTIONS; i++) {
            if ( users[i].client == client){
                users[i].name = name;
                fprintf(stderr, "name sat %s\n", users[i].name);
                i = MAX_CONNECTIONS;
            }
        }
        buf->len = snprintf(buf->base, buf->len, "Name sat!\n");
    }
    return 0;
}


void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = uv_handle_get_data(handle);
    buf->base = (char*)realloc(buf->base, suggested_size);
    buf->len = suggested_size;
    uv_handle_set_data(handle, buf->base);
}

void on_conn_close(uv_handle_t* client) {
	void* buf = uv_handle_get_data(client);
	if (buf) {
		fprintf(stderr, "Freeing buf\n");
		free(buf);
	}
	fprintf(stderr, "Freeing client\n");
    for(int i = 0; i < MAX_CONNECTIONS; i++ ) {
        if ( users[i].client == (uv_tcp_t*)client) {
            users[i].client = NULL;
            //free(users[i].name);
            fprintf(stderr, "%p\n", users[i].client);
            fprintf(stderr, "%p\n", users[i].name);
            i = MAX_CONNECTIONS;
        }
    }
	free(client);
}


void on_conn_write(uv_write_t* req, int status) {
	if (status) {
		fprintf( stderr, "Write error: %s\n", uv_strerror(status));
	}
	free(req);
}

int process_userinput(ssize_t nread, uv_buf_t *buf, uv_tcp_t* client) {
	struct _command* cmd;
	for(cmd = COMMANDS; cmd->handler; cmd++ ) {
		if( !strncmp(buf->base, cmd->name, strlen(cmd->name)) ) {
			return cmd->handler(buf, client);
		}
	}
    //buf->len = snprintf(buf->base, buf->len, "UNKNOWN COMMAND '%3s'\n", buf->base); works incorrect . why?
	buf->len = snprintf(buf->base, buf->len, "UNKNOWN COMMAND \n"/*, buf->base*/); //there was '%3s'
	return 0;
}

void on_client_read(uv_stream_t* client, ssize_t nread, const uv_buf_t *buf) {
	if(nread < 0) {
		if(nread != UV_EOF){
		fprintf(stderr, "read error %s\n", uv_err_name(nread));
		uv_close((uv_handle_t*)client, on_conn_close);
		}
	} else if (nread > 0) {
		int res;
		uv_buf_t wrbuf = uv_buf_init(buf->base, buf->len);
		res = process_userinput(nread, &wrbuf, (uv_tcp_t*)client);
		uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
	    uv_write(req, client, &wrbuf, 1, on_conn_write);	
        bzero(buf->base, buf->len);
		if( res ) {
			uv_close((uv_handle_t*)client, on_conn_close);
		}
	}
}

void on_new_connection( uv_stream_t* server, int status) {
    if (status < 0) {
        fprintf( stderr, "Conection error...%s\n", uv_strerror(status));
        return;
    }
    int client_id = -1;
    for (int i = 0; i < MAX_CONNECTIONS; i++ ) {
        if (users[i].client == NULL){
            fprintf(stderr, "Place found\n");
            client_id = i;
            i = MAX_CONNECTIONS;
        }
    }
    if ( client_id == -1 ){
        fprintf( stderr, "Max number of users...\n");
        return;
    }
    uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    fprintf(stderr, "client allocated\n");
    users[client_id].client = client;
    users[client_id].name = (char*)realloc(users[client_id].name, MAX_LINE*sizeof(char));
    itoa(client_id, users[client_id].name);
    fprintf(stderr, "CLIENT NAME %s \n", users[client_id].name);
    bzero(users[client_id].client, sizeof(uv_tcp_t));
    uv_tcp_init(loop, users[client_id].client);
    if ( uv_accept( server, (uv_stream_t*)users[client_id].client) == 0 ){
        fprintf(stderr, "Client %p accepted\n",  users[client_id].client);
        //need to know the name of user
        uv_read_start((uv_stream_t*)users[client_id].client, alloc_buffer, on_client_read);
    } else {
        fprintf(stderr, "Error while accepting...\n");
        uv_close((uv_handle_t*)users[client_id].client, on_conn_close);
    }
}


int main(){
    int result;
    struct sockaddr_in addr;
    uv_tcp_t server;
    loop = uv_default_loop();
    uv_tcp_init( loop, &server);
    uv_ip4_addr("0.0.0.0", 7000, &addr );
    uv_tcp_bind( &server, (const struct sockaddr *)&addr, 0);
    init_users(users);
    int r = uv_listen((uv_stream_t *)&server, 1024, on_new_connection);
    if ( r ){
            fprintf(stderr, "Listen error...%s\n", uv_strerror(r));
            return 1;
    }
    result = uv_run(loop, UV_RUN_DEFAULT);
    fprintf(stderr, "Finalization...\n");
    return result;
}