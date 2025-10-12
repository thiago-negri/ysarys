/* ISC License
 *
 * Copyright (c) 2025 Thiago Negri
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef INTDEF_H
#define INTDEF_H

#define STATIC_ASSERT_SIZE(type, size)                                         \
	static char __static_assert_##type_has_size_##size[(                   \
	    sizeof(type) == size ? 1 : -1)]

#ifndef TYPE_I8
#define TYPE_I8 char
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
STATIC_ASSERT_SIZE(TYPE_I8, 1);
#pragma GCC diagnostic pop
typedef TYPE_I8 unsigned u8;
typedef TYPE_I8 i8;

#ifndef TYPE_I16
#define TYPE_I16 short
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
STATIC_ASSERT_SIZE(TYPE_I16, 2);
#pragma GCC diagnostic pop
typedef TYPE_I16 unsigned u16;
typedef TYPE_I16 i16;

#ifndef TYPE_I32
#define TYPE_I32 int
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
STATIC_ASSERT_SIZE(TYPE_I32, 4);
#pragma GCC diagnostic pop
typedef TYPE_I32 unsigned u32;
typedef TYPE_I32 i32;

#ifndef TYPE_I64
#define TYPE_I64 long
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
STATIC_ASSERT_SIZE(TYPE_I64, 8);
#pragma GCC diagnostic pop
typedef TYPE_I64 unsigned u64;
typedef TYPE_I64 i64;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
STATIC_ASSERT_SIZE(void *, 8);
#pragma GCC diagnostic pop
typedef u64 usize;
typedef i64 isize;

#ifndef TYPE_F32
#define TYPE_F32 float
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
STATIC_ASSERT_SIZE(TYPE_F32, 4);
#pragma GCC diagnostic pop
typedef TYPE_F32 f32;

#ifndef TYPE_F64
#define TYPE_F64 double
#endif
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
STATIC_ASSERT_SIZE(TYPE_F64, 8);
#pragma GCC diagnostic pop
typedef TYPE_F64 f64;

#endif /* !INTDEF_H */
