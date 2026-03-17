/*
 * Simplified CList implementation for sd-event in basu
 * Based on dbus-broker's c-list, stripped down for minimal dependencies
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct CList CList;

struct CList {
        CList *next;
        CList *prev;
};

#define C_LIST_INIT(_var) { .next = &(_var), .prev = &(_var) }

static inline CList *c_list_init(CList *what) {
        *what = (CList)C_LIST_INIT(*what);
        return what;
}

static inline void *c_list_entry_offset(const CList *what, size_t offset) {
        if (what) {
            return (void *)(((uintptr_t)(void *)what) - offset);
        }
        return NULL;
}

#define c_list_entry(_what, _t, _m) \
        ((_t *)c_list_entry_offset((_what), offsetof(_t, _m)))

static inline _Bool c_list_is_linked(const CList *what) {
        return what && what->next != what;
}

static inline _Bool c_list_is_empty(const CList *list) {
        return !c_list_is_linked(list);
}

static inline void c_list_link_before(CList *where, CList *what) {
        CList *prev = where->prev, *next = where;

        next->prev = what;
        what->next = next;
        what->prev = prev;
        prev->next = what;
}
#define c_list_link_tail(_list, _what) c_list_link_before((_list), (_what))

static inline void c_list_link_after(CList *where, CList *what) {
        CList *prev = where, *next = where->next;

        next->prev = what;
        what->next = next;
        what->prev = prev;
        prev->next = what;
}
#define c_list_link_front(_list, _what) c_list_link_after((_list), (_what))

static inline void c_list_unlink_stale(CList *what) {
        CList *prev = what->prev, *next = what->next;

        next->prev = prev;
        prev->next = next;
}

static inline void c_list_unlink(CList *what) {
        if (c_list_is_linked(what)) {
                c_list_unlink_stale(what);
                *what = (CList)C_LIST_INIT(*what);
        }
}

static inline void c_list_swap(CList *list1, CList *list2) {
        CList t;

        t = *list1;
        t.next->prev = list2;
        t.prev->next = list2;
        t = *list2;
        t.next->prev = list1;
        t.prev->next = list1;

        t = *list1;
        *list1 = *list2;
        *list2 = t;
}

static inline void c_list_splice(CList *target, CList *source) {
        if (!c_list_is_empty(source)) {
                source->next->prev = target->prev;
                target->prev->next = source->next;

                source->prev->next = target;
                target->prev = source->prev;

                *source = (CList)C_LIST_INIT(*source);
        }
}

static inline CList *c_list_first(CList *list) {
        return c_list_is_empty(list) ? NULL : list->next;
}

static inline CList *c_list_last(CList *list) {
        return c_list_is_empty(list) ? NULL : list->prev;
}

#define c_list_first_entry(_list, _t, _m) \
        c_list_entry(c_list_first(_list), _t, _m)

#define c_list_for_each(_iter, _list)                                           \
        for (_iter = (_list)->next;                                             \
             (_iter) != (_list);                                                \
             _iter = (_iter)->next)

#define c_list_for_each_safe(_iter, _safe, _list)                               \
        for (_iter = (_list)->next, _safe = (_iter)->next;                      \
             (_iter) != (_list);                                                \
             _iter = (_safe), _safe = (_safe)->next)

#define c_list_for_each_entry(_iter, _list, _m)                                 \
        for (_iter = c_list_entry((_list)->next, __typeof__(*_iter), _m);       \
             &(_iter)->_m != (_list);                                           \
             _iter = c_list_entry((_iter)->_m.next, __typeof__(*_iter), _m))

#define c_list_for_each_entry_safe(_iter, _safe, _list, _m)                     \
        for (_iter = c_list_entry((_list)->next, __typeof__(*_iter), _m),       \
             _safe = c_list_entry((_iter)->_m.next, __typeof__(*_iter), _m);    \
             &(_iter)->_m != (_list);                                           \
             _iter = (_safe),                                                   \
             _safe = c_list_entry((_safe)->_m.next, __typeof__(*_iter), _m))
