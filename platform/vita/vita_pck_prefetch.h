/**************************************************************************/
/*  vita_pck_prefetch.h                                                   */
/**************************************************************************/

#pragma once

#include "core/string/ustring.h"

bool vita_pck_prefetch_is_enabled();
bool vita_pck_prefetch_try_read(const String &p_pack, uint64_t p_offset, uint8_t *p_dst, uint64_t p_length);
void vita_pck_prefetch_schedule(const String &p_pack, uint64_t p_offset, uint64_t p_length);
void vita_pck_prefetch_shutdown();

