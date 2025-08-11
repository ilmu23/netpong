// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<vector.h>>

#pragma once

#include "defs.h"

#include <stdlib.h>

typedef struct __vector *	vector;

#define VECTOR_INDEX_OUT_OF_BOUNDS	((void *)-1)

#define	vector(type, count, free)	(__vec_new(sizeof(type), count, free))
vector	__vec_new(const size_t size, const size_t count, void (*free)(void *));

#define	vector_delete(vector)	(__vec_del(vector))
void	__vec_del(vector);

#define	vector_push(vector, value)	(__vec_psh(vector, (const void *)(uintptr_t)&value))
u8		__vec_psh(vector vec,  const void *val);

#define	vector_pop(vector)	(__vec_pop(vector))
void	__vec_pop(vector vec);

#define	vector_get(vector, i)	(__vec_get(vector, i))
void	*__vec_get(const vector vec, const size_t i);

#define	vector_set(vector, i, value)	(__vec_set(vector, i, (const void *)(uintptr_t)&value))
u8		__vec_set(vector vec, const size_t i, const void *val);

#define	vector_size(vector)	(__vec_sze(vector))
size_t	__vec_sze(const vector vec);

#define	vector_capacity(vector)	(__vec_cap(vector))
size_t	__vec_cap(const vector vec);

#define vector_resize(vector, size)	(__vec_rsz(vector, size))
u8		__vec_rsz(vector vec, const size_t size);

#define vector_shrink_to_fit(vector)	(__vec_stf(vector))
u8		__vec_stf(vector vec);

#define vector_clear(vector)	(__vec_clr(vector))
void	__vec_clr(vector vec);

#define vector_insert(vector, i, value)	(__vec_ins(vector, i, (const void *)(uintptr_t)&value))
u8		__vec_ins(vector vec, const size_t i, const void *val);

#define vector_erase(vector, i)	(__vec_ers(vector, i))
u8		__vec_ers(vector vec, const size_t i);
