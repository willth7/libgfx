//   Copyright 2022 Will Thomas
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#ifndef _GFX_GFX_H
#define _GFX_GFX_H

#include <stdint.h>

void gfx_init(void*);

void gfx_set();

void gfx_resz(void*, uint32_t, uint32_t);

void gfx_init_vrtx(void*, uint64_t);

void gfx_init_vrtx_bind(uint32_t);

void gfx_set_vrtx_bind(uint32_t, uint32_t);

void gfx_init_vrtx_attr(uint32_t);

void gfx_set_vrtx_attr(uint32_t, uint32_t, uint32_t);

void gfx_init_indx(void*, uint64_t);

void gfx_init_unif(void*, uint64_t);

void gfx_init_txtr(uint8_t*, uint32_t, uint32_t);

void gfx_init_push(void*, uint64_t);

void gfx_init_desc(uint32_t);

void gfx_set_desc();

void gfx_set_shdr(int8_t*, int8_t*);

void gfx_set_clr(uint8_t, uint8_t, uint8_t);

void gfx_draw(uint64_t);

void gfx_term();

#endif
