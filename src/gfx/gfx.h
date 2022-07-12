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

typedef gfx_pipe_t;

typedef gfx_vrtx_t;

typedef gfx_txtr_t;

typedef gfx_dscr_t;

gfx_t* gfx_init(int8_t);

gfx_win_t* gfx_win_init(gfx_t*, void*);

gfx_cmd_t* gfx_cmd_init(gfx_t*);

void gfx_rndr_init(gfx_t*, gfx_win_t*);

void gfx_swap_init(gfx_t*, gfx_win_t*);

void gfx_dpth_init(gfx_t*, gfx_win_t*);

void gfx_frme_init(gfx_t*, gfx_win_t*);

gfx_pipe_t* gfx_pipe_init(gfx_t*, gfx_win_t*, int8_t*, int8_t*, gfx_vrtx_t*, gfx_dscr_t*, uint64_t);

void gfx_bfr_rfsh(gfx_t*, gfx_bfr_t*, void*, uint64_t);

void gfx_set(gfx_t*, gfx_win_t*);

gfx_vrtx_t* gfx_vrtx_init(gfx_t*, uint32_t, uint32_t, uint64_t);

void gfx_vrtx_bind(gfx_vrtx_t*, uint32_t, uint32_t);

void gfx_vrtx_attr(gfx_vrtx_t*, uint32_t, uint32_t, int8_t, uint32_t);

void gfx_vrtx_in(gfx_vrtx_t*);

void gfx_vrtx_rfsh(gfx_t*, gfx_vrtx_t*, uint32_t, void*, uint64_t);

gfx_bfr_t* gfx_indx_init(gfx_t*, uint64_t);

gfx_bfr_t* gfx_unif_init(gfx_t*, uint64_t);

gfx_txtr_t* gfx_txtr_init(gfx_t*, gfx_cmd_t*, uint8_t*, uint32_t, uint32_t);

gfx_dscr_t* gfx_dscr_init(gfx_t*, uint32_t);

void gfx_dscr_writ(gfx_t*, gfx_dscr_t*, uint32_t, gfx_bfr_t*, void*, uint64_t, gfx_txtr_t*);

void gfx_clr(gfx_win_t*, uint8_t, uint8_t, uint8_t);

void gfx_next(gfx_t*, gfx_win_t*, gfx_cmd_t*);

void gfx_draw(gfx_t*, gfx_win_t*, gfx_pipe_t*, gfx_cmd_t*, gfx_bfr_t*, gfx_vrtx_t*, gfx_dscr_t*, void*, uint64_t, uint32_t, uint32_t, uint32_t);

void gfx_swap(gfx_t*, gfx_win_t*, gfx_cmd_t*);

void gfx_resz(gfx_t*, gfx_win_t*, uint32_t, uint32_t);

void gfx_bfr_free(gfx_t*, gfx_bfr_t*);

void gfx_img_free(gfx_t*, gfx_img_t*);

void gfx_vrtx_free(gfx_t*, gfx_vrtx_t*);

void gfx_txtr_free(gfx_t*, gfx_txtr_t*);

void gfx_dscr_free(gfx_t*, gfx_dscr_t*);

void gfx_pipe_free(gfx_t*, gfx_pipe_t*);

void gfx_cmd_free(gfx_t*, gfx_cmd_t*);

void gfx_win_free(gfx_t*, gfx_win_t*);

void gfx_free(gfx_t*);

#endif
