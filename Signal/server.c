/*
 *
 *     add-server.c: server program, receives message,
 *                   adds numbers in message and
 *                   gives back result to client
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <dbus/dbus.h>

const char *const SERVER_BUS_NAME = "in.server";
const char *const OBJECT_PATH_NAME = "/in/server";
const char *const INTERFACE_NAME = "in.client.ping";
const char *const SIGNAL_NAME = "signal_ping";

DBusError dbus_error;
void print_dbus_error(char *str);
bool isinteger(char *ptr);

int main(int argc, char **argv)
{
    //create dbuserror and dbusconnection
    DBusConnection *conn;
    int ret;

    dbus_error_init(&dbus_error);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    //check if have error
    if (dbus_error_is_set(&dbus_error))
    {
        print_dbus_error("dbus_bus_get");
        dbus_error_free(&dbus_error);
    }
    if (!conn)
        exit(1);

    // Get a name, this user want to be primary owner
    ret = dbus_bus_request_name(conn, SERVER_BUS_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &dbus_error);

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        fprintf(stderr, "Dbus: not primary owner, ret = %d\n", ret);
        exit(1);
    }

    // Subscribe to a signal
    dbus_bus_add_match(conn, 
        "type='signal',sender='in.client', interface='in.client.ping',member='signal_ping'", 
        &dbus_error); // see signals from the given interface
    dbus_connection_flush(conn);
    if (dbus_error_is_set(&dbus_error)) { 
        fprintf(stderr, "Match Error (%s)\n", dbus_error.message);
        exit(1); 
    }
    // Handle request from clients
    
    while (1)
    {
        // Block until receiving msg from client
        if (!dbus_connection_read_write_dispatch(conn, -1))
        {
            fprintf(stderr, "Not connected now.\n");
            exit(1);
        }
        DBusMessage *message;

        // get first message from incoming message queue
        if ((message = dbus_connection_pop_message(conn)) == NULL)
        {
            fprintf(stderr, "Did not get message\n");
            continue;
        }
        // check destination addr in header of message
        // const char *name_server = dbus_message_get_path(message);
        // printf("%s\n",name_server);
        DBusMessageIter iter;
        dbus_message_iter_init(message, &iter);
        int type = dbus_message_iter_get_arg_type(&iter);
        if (type == DBUS_TYPE_INT32)
        {
            dbus_int32_t signal_number;
            if (dbus_message_get_args(message, &dbus_error, DBUS_TYPE_INT32, &signal_number, DBUS_TYPE_INVALID))
            {
                printf("received signal number: %d\n",signal_number);  
            }
        }
        if (dbus_error_is_set(&dbus_error))
        {
            print_dbus_error("dbus_bus_get");
        }
           
    }

    return 0;
}


void print_dbus_error(char *str)
{
    fprintf(stderr, "%s: %s\n", str, dbus_error.message);
    dbus_error_free(&dbus_error);
}

