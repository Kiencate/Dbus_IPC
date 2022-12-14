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

const char *const INTERFACE_NAME = "in.server.autoreply";
const char *const SERVER_BUS_NAME = "in.server";
const char *const SERVER_OBJECT_PATH_NAME = "/in/server";
const char *const METHOD_NAME = "reply_first_name";
const char *const CLIENT_BUS_NAME = "in.client.autoreply";
const char *const CLIENT_OBJECT_PATH_NAME = "/in/client";

DBusError dbus_error;
void print_dbus_error(char *str);

int main(int argc, char **argv)
{
    //create dbuserror and dbusconnection
    DBusConnection *conn;
    int ret;
    char input[80];

    dbus_error_init(&dbus_error);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);

    if (dbus_error_is_set(&dbus_error))
        print_dbus_error("dbus_bus_get");

    if (!conn)
        exit(1);

    printf("Please type your full name: ");
    while (fgets(input, 78, stdin) != NULL)
    {

        // Get a well known name
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

        //create method call message
        DBusMessage *request;

        if ((request = dbus_message_new_method_call(SERVER_BUS_NAME, SERVER_OBJECT_PATH_NAME,
                                                    INTERFACE_NAME, METHOD_NAME)) == NULL)
        {
            fprintf(stderr, "Error in dbus_message_new_method_call\n");
            exit(1);
        }

        //create dbusmessageiter to append argument in method call message
        DBusMessageIter iter;
        dbus_message_iter_init_append(request, &iter);
        char *ptr = input;
        if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &ptr))
        {
            fprintf(stderr, "Error in dbus_message_iter_append_basic\n");
            exit(1);
        }
        //create dbuspendingcall to wait for receiving reply of method call message, using timeout = -1 is default timeout(25s)
        DBusPendingCall *pending_return;
        if (!dbus_connection_send_with_reply(conn, request, &pending_return, -1))
        {
            fprintf(stderr, "Error in dbus_connection_send_with_reply\n");
            exit(1);
        }
        //if the connection disconect -> pending_return is NULL
        if (pending_return == NULL)
        {
            fprintf(stderr, "pending return is NULL");
            exit(1);
        }

        //block until outgoing message queue is empty
        dbus_connection_flush(conn);

        //free method call message
        dbus_message_unref(request);

        //block until the pending call is completed
        dbus_pending_call_block(pending_return);

        //get the reply message, dbus_pending_call_steal_reply is call only once after dbus_pending_call_block
        DBusMessage *reply;
        if ((reply = dbus_pending_call_steal_reply(pending_return)) == NULL)
        {
            fprintf(stderr, "Error in dbus_pending_call_steal_reply");
            exit(1);
        }

        //free the dbuspendingcall
        dbus_pending_call_unref(pending_return);

        //get argument in reply message
        char *s;
        if (dbus_message_get_args(reply, &dbus_error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID))
        {
            printf("Server reply: %s\n", s);
        }
        else
        {
            fprintf(stderr, "Did not get arguments in reply\n");
            exit(1);
        }
        //free reply message
        dbus_message_unref(reply);

        //unasign name of connection from bus 
        if (dbus_bus_release_name(conn, CLIENT_BUS_NAME, &dbus_error) == -1)
        {
            fprintf(stderr, "Error in dbus_bus_release_name\n");
            exit(1);
        }

        printf("Please type your full name: ");
    }

    return 0;
}

void print_dbus_error(char *str)
{
    fprintf(stderr, "%s: %s\n", str, dbus_error.message);
    dbus_error_free(&dbus_error);
}