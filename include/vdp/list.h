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

#ifndef _VDP_LIST_H_
#define _VDP_LIST_H_

#include "vdp/types.h"

struct vdp_list
{
    struct vdp_list* prev;
    struct vdp_list* next;
};

/*
 * Private interface.
 */

static __inline void __vdp_list_add(struct vdp_list* nw,
    struct vdp_list* prev,
    struct vdp_list* next)
{
    next->prev = nw;
    nw->next = next;
    nw->prev = prev;
    prev->next = nw;
}

static __inline void __vdp_list_remove(struct vdp_list* prev,
    struct vdp_list* next)
{
    next->prev = prev;
    prev->next = next;
}

/*
 * Public interface.
 */

#define VDP_DECLARE_LIST(name) struct vdp_list name = { &(name), &(name) }

static __inline void vdp_list_init(struct vdp_list* list)
{
    list->next = list;
    list->prev = list;
}

static __inline void vdp_list_add(struct vdp_list* head, struct vdp_list* nw)
{
    __vdp_list_add(nw, head, head->next);
}

static __inline void vdp_list_add_tail(struct vdp_list* head, struct vdp_list* nw)
{
    __vdp_list_add(nw, head->prev, head);
}

static __inline void vdp_list_remove(struct vdp_list* entry)
{
    __vdp_list_remove(entry->prev, entry->next);
    vdp_list_init(entry);
}

static __inline int vdp_list_empty(const struct vdp_list* head)
{
    return ((head->next == head) && (head->prev == head));
}

#define vdp_list_first(container_type, iter, head, member) iter = vdp_containerof((head)->next, container_type, member)

#define vdp_list_last(container_type, iter, head, member) iter = vdp_containerof((head)->prev, container_type, member)

/*
 * Iterate over list in direct and reverse order.
 */

#define vdp_list_for_each(container_type, iter, head, member) \
    for (iter = vdp_containerof((head)->next, container_type, member); \
         &iter->member != (head); \
         iter = vdp_containerof(iter->member.next, container_type, member))

#define vdp_list_for_each_reverse(container_type, iter, head, member) \
    for (iter = vdp_containerof((head)->prev, container_type, member); \
         &iter->member != (head); \
         iter = vdp_containerof(iter->member.prev, container_type, member))

/*
 * Iterate over list in direct and reverse order, safe to list entries removal.
 */

#define vdp_list_for_each_safe(container_type, iter, tmp_iter, head, member) \
    for (iter = vdp_containerof((head)->next, container_type, member), \
         tmp_iter = vdp_containerof(iter->member.next, container_type, member); \
         &iter->member != (head); \
         iter = tmp_iter, tmp_iter = vdp_containerof(tmp_iter->member.next, container_type, member))

#define vdp_list_for_each_safe_reverse(container_type, iter, tmp_iter, head, member) \
    for (iter = vdp_containerof((head)->prev, container_type, member), \
         tmp_iter = vdp_containerof(iter->member.prev, container_type, member); \
         &iter->member != (head); \
         iter = tmp_iter, tmp_iter = vdp_containerof(tmp_iter->member.prev, container_type, member))

#endif
