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

const char *const INTERFACE_NAME = "in.server.autoreply";
const char *const SERVER_BUS_NAME = "in.server";
const char *const OBJECT_PATH_NAME = "/in/server";
const char *const METHOD_NAME = "reply_first_name";

void interface_execute(DBusConnection *conn, DBusMessage *message, const char *interface_name, const char *method_name);

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
        const char *name_server = dbus_message_get_path(message);
        if(strcmp(OBJECT_PATH_NAME,name_server) ==0 )
        {
            interface_execute(conn, message, INTERFACE_NAME, METHOD_NAME);
        }      
    }

    return 0;
}


void print_dbus_error(char *str)
{
    fprintf(stderr, "%s: %s\n", str, dbus_error.message);
    dbus_error_free(&dbus_error);
}

void interface_execute(DBusConnection *conn, DBusMessage *message, const char *interface_name, const char *method_name)
{
    // check if message is a method call with right interface anh method name that server specify
    if (dbus_message_is_method_call(message, interface_name, method_name))
    {
        char *s;
        if (dbus_message_get_args(message, &dbus_error, DBUS_TYPE_STRING, &s, DBUS_TYPE_INVALID))
        {
            printf("Full name client: %s", s);
            char *last_name = strrchr(s,' ');

            //check if receive full name (first name + last name)
            if (last_name && *(last_name + 1))
            {
                // send reply
                DBusMessage *reply;
                char answer[40];

                last_name[strlen(last_name)-1] = '\0';
                sprintf(answer, "Hello %s", last_name + 1);
                //create reply message for method call message coming
                if ((reply = dbus_message_new_method_return(message)) == NULL)
                {
                    fprintf(stderr, "Error in dbus_message_new_method_return\n");
                    exit(1);
                }

                //append argument to the end of message
                DBusMessageIter iter;
                dbus_message_iter_init_append(reply, &iter);
                char *ptr = answer;
                if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &ptr))
                {
                    fprintf(stderr, "Error in dbus_message_iter_append_basic\n");
                    exit(1);
                }

                //send reply to connection
                if (!dbus_connection_send(conn, reply, NULL))
                {
                    fprintf(stderr, "Error in dbus_connection_send\n");
                    exit(1);
                }
                //block until outgoing message queue empty
                dbus_connection_flush(conn);
                //free reply message
                dbus_message_unref(reply);
            }

            else // There was an error
            {
                DBusMessage *dbus_error_msg;
                char error_msg[] = "Error in input, make sure you entered full name";
                //create error message for method call message coming
                if ((dbus_error_msg = dbus_message_new_error(message, DBUS_ERROR_FAILED, error_msg)) == NULL)
                {
                    fprintf(stderr, "Error in dbus_message_new_error\n");
                    exit(1);
                }

                //send reply to connection 
                if (!dbus_connection_send(conn, dbus_error_msg, NULL))
                {
                    fprintf(stderr, "Error in dbus_connection_send\n");
                    exit(1);
                }
                //block until outgoing message queue empty
                dbus_connection_flush(conn);
                //free reply message
                dbus_message_unref(dbus_error_msg);
            }
        }
        else
        {
            print_dbus_error("Error getting message");
        }
    }
}