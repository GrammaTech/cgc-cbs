/*
* Handle privilage management
*
* Copyright (C) 2014 - Brian Caswell <bmc@lungetech.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#define _GNU_SOURCE

#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include "privileges.h"
#include "utils.h"

extern int insecure_flag;

void setup_chroot(const char *directory) {
    VERIFY(chdir, directory);
    if (insecure_flag == 1) {
        return;
    }

    VERIFY(chroot, directory);
    VERIFY(chdir, "/");
}

uid_t get_unused_uid(void) {
    struct passwd *pd;
    uid_t uid;
    unsigned int count = 0;

    for (count = 0; count <= 1000; count++) {
        get_random((void *) &uid, sizeof(uid));

        pd = getpwuid(uid);

        if (pd == NULL)
            return uid;
    }

    err(-1, "unable to get an unused uid");
}

gid_t get_unused_gid(void) {
    struct group *gd;
    gid_t gid;
    unsigned int count = 0;

    for (count = 0; count <= 1000; count++) {
        get_random((void *) &gid, sizeof(gid));

        gd = getgrgid(gid);

        if (gd == NULL)
            return gid;
    }

    err(-1, "unable to get an unused gid");
}

void drop_privileges(const uid_t uid, const gid_t gid) {
    VERIFY(setsid);
    if (insecure_flag == 1) {
        return;
    }
#ifndef RANDOM_UID
    {
        struct passwd *pd;
        pd = getpwuid(uid);
        VERIFY_EXP(pd != NULL);
        setup_chroot(pd->pw_dir);
    }
#endif
#ifdef HAVE_SETRESGID
    VERIFY(setresgid, gid, gid, gid);
#else
    VERIFY(setregid, gid, gid);
#endif
    VERIFY(setgroups, 1, &gid);
    VERIFY(setgid, gid);
    VERIFY(setegid, gid);
#ifdef HAVE_SETRESGID
    VERIFY(setresuid, uid, uid, uid);
#else
    VERIFY(setreuid, UID_MAX, uid);
#endif
    VERIFY(setuid, uid);
    VERIFY(seteuid, uid);
#ifdef HAVE_SETRESGID
    {
        uid_t real;
        uid_t effective;
        uid_t saved;
        VERIFY(getresuid, &real, &effective, &saved);
        VERIFY_EXP(uid == real);
        VERIFY_EXP(uid == effective);
        VERIFY_EXP(uid == saved);

        VERIFY(getresgid, &real, &effective, &saved);
        VERIFY_EXP(gid == real);
        VERIFY_EXP(gid == effective);
        VERIFY_EXP(gid == saved);
    }
#else
    VERIFY_EXP(geteuid() == uid);
    VERIFY_EXP(getuid() == uid);
    VERIFY_EXP(getegid() == gid);
    VERIFY_EXP(getgid() == gid);
#endif
}
