/******************************************************************************
  contrib/data_rig/data_rig.c

  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"

#include <float.h>
#include <math.h>

#include "access/gist.h"
#include "access/stratnum.h"
#include "utils/array.h"
#include "utils/builtins.h"

#include "data_rig.h"

PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(fact_in);
PG_FUNCTION_INFO_V1(fact_out);
PG_FUNCTION_INFO_V1(fact_ia);
PG_FUNCTION_INFO_V1(fact_compress);
PG_FUNCTION_INFO_V1(fact_decompress);

Datum
fact_in(PG_FUNCTION_ARGS)
{
	//char	   *str = PG_GETARG_CSTRING(0);
	//FACT	   *result;

	ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("fact_in() not implemented")));
		PG_RETURN_DATUM(0);

	//PG_RETURN_NDBOX(result);
}

Datum
fact_out(PG_FUNCTION_ARGS)
{
	FACT	   *cube = (FACT*)PG_GETARG_DATUM(0);
	StringInfoData buf;
	int			dim = DIM(cube);
	int			i;

	initStringInfo(&buf);

	appendStringInfoChar(&buf, '(');
	for (i = 0; i < dim; i++)
	{
		if (i > 0)
			appendStringInfoString(&buf, ", ");
		appendStringInfoString(&buf, float8out_internal((double)cube->x[i]));
	}
	appendStringInfoChar(&buf, ')');


	PG_FREE_IF_COPY(cube, 0);
	PG_RETURN_CSTRING(buf.data);

}

/*
 * Taken from the intarray contrib header
 */
#define ARRPTR(x)  ( ARR_DATA_PTR(x) )
#define ARRNELEMS(x)  ArrayGetNItems( ARR_NDIM(x), ARR_DIMS(x))

static int
isort_cmp(const void *a, const void *b, void *arg)
{
	int32_t		aval = *((const int32_t *) a);
	int32_t		bval = *((const int32_t *) b);

	if (aval < bval)
		return -1;
	if (aval > bval)
		return 1;

	/*
	 * Report if we have any duplicates.  If there are equal keys, qsort must
	 * compare them at some point, else it wouldn't know whether one should go
	 * before or after the other.
	 */
	*((bool *) arg) = true;
	return 0;
}

/* Sort the given data (len >= 2).  Return true if any duplicates found */
bool
isort(int32_t *a, int len)
{
	bool		r = false;

	qsort_arg(a, len, sizeof(int32_t), isort_cmp, (void *) &r);
	return r;
}

Datum
fact_ia(PG_FUNCTION_ARGS)
{
	ArrayType  *ur = PG_GETARG_ARRAYTYPE_P(0);
	FACT	   *result;
	int			i;
	int			dim;
	int			size;
	int32_t	   *dur;

	if (array_contains_nulls(ur))
		ereport(ERROR,
				(errcode(ERRCODE_ARRAY_ELEMENT_ERROR),
				 errmsg("cannot work with arrays containing NULLs")));

	dim = ARRNELEMS(ur);

	dur = (int32_t *) ARRPTR(ur);

	size = FACT_SIZE(dim);
	result = (FACT *) palloc0(size);
	SET_VARSIZE(result, size);
	SET_DIM(result, dim);

	for (i = 0; i < dim; i++)
		result->x[i] = dur[i];

	if(isort(result->x,dim))
	{
		int current = 0;
		for (i = 0; i < dim; i++)
		{
			if(result->x[current] != result->x[i])
			{
				result->x[current] = result->x[i];
				current++;
			}
		}
	}

	PG_RETURN_POINTER(result);
}

Datum
fact_compress(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

Datum
fact_decompress(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}
