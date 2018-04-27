/* Copyright (c) 2017 Arrow Electronics, Inc.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Apache License 2.0
 * which accompanies this distribution, and is available at
 * http://apache.org/licenses/LICENSE-2.0
 * Contributors: Arrow Electronics, Inc.
 */

#include "data/property.h"
#include <debug.h>

void property_init(property_t *dst) {
    dst->flags = 0x00;
    dst->value = NULL;
}

void property_copy(property_t *dst, const property_t src) {
    if ( !dst ) return;
    property_free(dst);
    // don't copy weak pointer
    if ( (src.flags & is_weak) ) return;
    switch ( src.flags & property_mask ) {
    case is_stack:
        dst->value = strdup(src.value);
        dst->flags = is_dynamic;
        break;
    case is_const:
        dst->value = src.value;
        dst->flags = is_const;
        break;
    case is_dynamic:
        dst->value = src.value;
        dst->flags = is_dynamic;
        break;
    }
}

void property_weak_copy(property_t *dst, const property_t src) {
    if ( !dst ) return;
    property_init(dst);
    dst->flags = is_weak;
    dst->value = src.value;
}


void property_move(property_t *dst, property_t *src) {
    property_copy(dst, *src);
    property_free(src);
}

void property_n_copy(property_t *dst, const char *src, int n) {
    if ( !dst ) return;
    property_init(dst);
    dst->value = strndup(src, n);
    dst->flags = is_dynamic;
}

void property_free(property_t *dst) {
    // free memory if dst is not weak pointer
  if ( dst && ! (dst->flags & is_weak)  ) {
      if (dst->flags == is_dynamic && dst->value)
          free(dst->value);
      dst->value = NULL;
      dst->flags = 0x0;
  }
}

int property_cmp(property_t *src, property_t *dst) {
    if ( strcmp(src->value, dst->value) == 0 ) return 0;
    return -1;
}
