#include <stdarg.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

#include <isc/result.h>
#include <dhcpctl.h>

int main (int argc, char **argv) {
        dhcpctl_data_string ipaddrstring = NULL;
        dhcpctl_data_string value = NULL;
        dhcpctl_handle connection = NULL;
        dhcpctl_handle lease = NULL;
        isc_result_t waitstatus;
        struct in_addr convaddr;
        time_t thetime;

        dhcpctl_initialize ();

        dhcpctl_connect (&connection, "127.0.0.1",
                         7911, 0);
        
        dhcpctl_new_object (&lease, connection,
                            "lease");

        memset (&ipaddrstring, 0, sizeof
                ipaddrstring);

        inet_pton(AF_INET, "10.0.0.101",
                  &convaddr);

        omapi_data_string_new (&ipaddrstring,
                               4, MDL);
        memcpy(ipaddrstring->value, &convaddr.s_addr, 4);

        dhcpctl_set_value (lease, ipaddrstring,
                           "ip-address");

        dhcpctl_open_object (lease, connection, 0);

        dhcpctl_wait_for_completion (lease,
                                     &waitstatus);
        if (waitstatus != ISC_R_SUCCESS) {
                /* server not authoritative */
                exit (0);
        }

        dhcpctl_data_string_dereference(&ipaddrstring,
                                        MDL);

        dhcpctl_get_value (&value, lease, "ends");

        memcpy(&thetime, value->value, value->len);

        dhcpctl_data_string_dereference(&value, MDL);

        fprintf (stdout, "ending time is %s",
                 ctime(&thetime));
}
