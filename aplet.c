/* vi: set sw=4 ts=4: */
/*
*
* Sends command (the second argument) to the given file (the first argument).
* Does this by calling the ioctl system call.
* 
* Licensed under GPLv2 or later, see file LICENSE in this source tree.
*/

//config:config IOCTL
//config:  bool "ioctl - Zadatak 2"
//config:  default y

//applet:IF_IOCTL(APPLET(ioctl,BB_DIR_USR_SBIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_IOCTL) += ioctl.o

//usage:#define ioctl_trivial_usage
//usage:  "No.\n"
//usage:#define ioctl_full_usage "\n\n"
//usage:  "Also no.\n"

#include "libbb.h"

int main(int argc, char **argv)
{

}
