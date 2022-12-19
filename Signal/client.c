/*
 *
 *     add-client.c: client program, takes two numbers as input,
 *                   sends to server for addition,
 *                   gets result from server,
 *                   prints the result on the screen
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <dbus/dbus.h>

const char *const INTERFACE_NAME = "in.client.ping";
const char *const SIGNAL_NAME = "signal_ping";
const char *const CLIENT_BUS_NAME = "in.client";
const char *const CLIENT_OBJECT_PATH_NAME = "/in/client";

DBusError dbus_error;
void print_dbus_error(char *str);

int main(int argc, char **argv)
{
    //create dbuserror and dbusconnection
    DBusConnection *conn;
    int ret;

    dbus_error_init(&dbus_error);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    if (dbus_error_is_set(&dbus_error))
        print_dbus_error("dbus_bus_get");

    if (!conn)
        exit(1);
    // Get a well known name
    dbus_int32_t signal_number = 0;
    while(1)
    {
        while (1)
        {
            ret = dbus_bus_request_name(conn, CLIENT_BUS_NAME, 0, &dbus_error);

            //if this connection is owner of CLIENT_BUS_NAME
            if (ret == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
                break;
            //if this connection is in queue to own CLIENT_BUS_NAME
            if (ret == DBUS_REQUEST_NAME_REPLY_IN_QUEUE)
            {
                fprintf(stderr, "Waiting for the bus ... \n");
                sleep(1);
                continue;
            }
            //check if have error
            if (dbus_error_is_set(&dbus_error))
                print_dbus_error("dbus_bus_get");
        }

        //create signal message
        DBusMessage *signal_msg;

        if ((signal_msg = dbus_message_new_signal(CLIENT_OBJECT_PATH_NAME,
                                                        INTERFACE_NAME, SIGNAL_NAME)) == NULL)
        {
            fprintf(stderr, "Error in dbus_message_new_method_call\n");
            exit(1);
        }

        // create dbusmessageiter to append argument in signal message
        DBusMessageIter iter;
        dbus_message_iter_init_append(signal_msg, &iter);
        // dbus_int32_t *number_ping = &signal_number;
        if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &signal_number))
        {
            fprintf(stderr, "Error in dbus_message_iter_append_basic\n");
            exit(1);
        }
        
        
        if (!dbus_connection_send(conn, signal_msg, NULL)) { 
            fprintf(stderr, "Out Of Memory!\n"); 
            exit(1);
        }

        //block until outgoing message queue is empty
        dbus_connection_flush(conn);

        //free signal message
        dbus_message_unref(signal_msg);
        printf("sent signal number %d\n",signal_number);
        signal_number++;
        sleep(2);
        if (dbus_bus_release_name(conn, CLIENT_BUS_NAME, &dbus_error) == -1)
        {
            fprintf(stderr, "Error in dbus_bus_release_name\n");
            exit(1);
        }
    }
    
    return 0;
}




void print_dbus_error(char *str)
{
    fprintf(stderr, "%s: %s\n", str, dbus_error.message);
    dbus_error_free(&dbus_error);
}