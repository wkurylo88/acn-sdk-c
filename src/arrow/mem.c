/* Copyright (c) 2017 Arrow Electronics, Inc.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Apache License 2.0
 * which accompanies this distribution, and is available at
 * http://apache.org/licenses/LICENSE-2.0
 * Contributors: Arrow Electronics, Inc.
 */

#include "arrow/mem.h"

void property_copy(property_t *dst, const property_t const src) {
	if ( !dst ) return;
	P_FREE(*dst);
	switch(src.flags) {
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

void property_n_copy(property_t *dst, const char *src, int n) {
	P_FREE(*dst);
	dst->value = strndup(src, n);
	dst->flags = is_dynamic;
}

void property_free(property_t *dst) {
	if (dst->flags == is_dynamic && dst->value)
		free(dst->value);
	  dst->value = NULL;
	  dst->flags = 0x0;
}
