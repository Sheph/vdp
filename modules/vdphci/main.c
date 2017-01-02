/*
 * Copyright (c) 2017, Stanislav Vorobiov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/module.h>
#include <linux/init.h>
#include "print.h"
#include "vdphci_platform_driver.h"
#include "vdphci_controllers.h"

MODULE_AUTHOR("Stanislav Vorobiov");
MODULE_LICENSE("Dual BSD/GPL");

int vdphci_major = 0;

module_param(vdphci_major, int, S_IRUGO);

int vdphci_init(void)
{
    int ret = vdphci_platform_driver_register();

    if (ret != 0) {
        return ret;
    }

    ret = vdphci_controllers_add();

    if (ret != 0) {
        vdphci_platform_driver_unregister();

        return ret;
    }

    print_info("module loaded\n");

    return 0;
}

void vdphci_cleanup(void)
{
    vdphci_controllers_remove();

    vdphci_platform_driver_unregister();

    print_info("module unloaded\n");
}

module_init(vdphci_init);
module_exit(vdphci_cleanup);
