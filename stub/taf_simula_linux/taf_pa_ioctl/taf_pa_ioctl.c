/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "legato.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <unistd.h>
#include <sys/ioctl.h>


/* [Begin]: Which picked from tafFSCrypt.h,
 * ensure the declaration is same as our services */

#define _FS_KEY_DESCRIPTOR_SIZE      8
#define _FS_KEY_IDENTIFIER_SIZE      16

struct _fscrypt_key_specifier
{
    uint32_t type;
    uint32_t __reserved;
    union
    {
        uint8_t __reserved[32];
        uint8_t descriptor[_FS_KEY_DESCRIPTOR_SIZE];
        uint8_t identifier[_FS_KEY_IDENTIFIER_SIZE];
    } u;
};

struct _fscrypt_add_key_arg
{
    struct _fscrypt_key_specifier key_spec;
    uint32_t raw_size;
    uint32_t key_id;
    uint32_t __reserved[8];
    uint8_t raw[];
};

#define _FS_IOC_ADD_ENCRYPTION_KEY  _IOWR('f', 23, struct _fscrypt_add_key_arg)
/* [End] */

typedef int (*orig_ioctl_t)(int, unsigned long, ...);

int ioctl(int fd, unsigned long request, ...) {

    orig_ioctl_t orig_ioctl = (orig_ioctl_t)dlsym(RTLD_NEXT, "ioctl");

    if (request == _FS_IOC_ADD_ENCRYPTION_KEY)
    {
        LE_INFO("[simulation] ioctl request -> FS_IOC_ADD_ENCRYPTION_KEY\n");
    }

    return 0;
}
