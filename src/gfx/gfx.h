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

typedef gfx_t;

typedef gfx_bfr_t;

typedef gfx_img_t;

typedef gfx_win_t;

typedef gfx_cmd_t;

typedef gfx_vrtx_t;

gfx_t* gfx_init();

gfx_win_t* gfx_init_win(gfx_t*, void*);

gfx_cmd_t* gfx_init_cmd(gfx_t*);

void gfx_init_rndr(gfx_t*, gfx_win_t*);

void gfx_init_swap(gfx_t*, gfx_win_t*);

void gfx_init_dpth(gfx_t*, gfx_win_t*);

void gfx_init_pipe(gfx_t*, gfx_win_t*, uint8_t*, uint8_t*, gfx_vrtx_t*, uint64_t);

void gfx_init_frme(gfx_t*, gfx_win_t*);

void gfx_rfsh_bfr(gfx_t*, gfx_bfr_t*, void*, uint64_t);

void gfx_set(gfx_t*, gfx_win_t*);

void gfx_resz(gfx_t*, gfx_win_t*, uint32_t, uint32_t);

gfx_vrtx_t* gfx_init_vrtx(gfx_t*, uint64_t);

void gfx_init_vrtx_bind(gfx_vrtx_t*, uint32_t);

void gfx_set_vrtx_bind(gfx_vrtx_t*, uint32_t, uint32_t);

void gfx_init_vrtx_attr(gfx_vrtx_t*, uint32_t);

void gfx_set_vrtx_attr(gfx_vrtx_t*, uint32_t, uint32_t, int8_t, uint32_t);

void gfx_set_vrtx_in(gfx_vrtx_t*);

void gfx_rfsh_vrtx(gfx_t*, gfx_vrtx_t*, void*, uint64_t);

gfx_bfr_t* gfx_init_indx(gfx_t*, uint64_t);

void gfx_set_clr(gfx_win_t*, uint8_t, uint8_t, uint8_t);

void gfx_draw(gfx_t*, gfx_win_t*, gfx_cmd_t*, gfx_bfr_t*, gfx_vrtx_t*, void*, void*, uint64_t, uint32_t, uint32_t, uint32_t);

void gfx_free_bfr(gfx_t*, gfx_bfr_t*);

void gfx_free_img(gfx_t*, gfx_img_t*);

void gfx_free_vrtx(gfx_t*, gfx_vrtx_t*);

void gfx_free_cmd(gfx_t*, gfx_cmd_t*);

void gfx_free_win(gfx_t*, gfx_win_t*);

void gfx_free(gfx_t*);

#endif
