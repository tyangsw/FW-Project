/*
 * Hooks for stdio functions targeting UART0.
 *
 * Copyright (C) 2014-2015 North Atlantic Industries, Inc.
 */

#if SERIAL_DEBUG

#include <includes.h>


int _open(const char *filename, int flags, int mode)
{
    // Return an arbitrary constant integer value as the file descriptor
    return 0x42;
}


int _write(int fd, const void *buf, size_t count)
{
    char    *p = (char *)buf;
    u32     len = count;

    while (len--)
    {
        if (*p == '\n')
            XUartPs_SendByte(STDOUT_BASEADDRESS, '\r');
        XUartPs_SendByte(STDOUT_BASEADDRESS, *p++);
    }

    return count;
}


int _read(int fd, void *buffer, size_t buflen)
{
    char    *p = (char *)buffer;
    u32     len = buflen;

    while (len--)
    {
        if (XUartPs_IsReceiveData(STDOUT_BASEADDRESS))
            *p++ = XUartPs_ReadReg(STDOUT_BASEADDRESS, XUARTPS_FIFO_OFFSET);
        else
            return -1;
    }

    return buflen;
}

#endif // #if SERIAL_DEBUG

