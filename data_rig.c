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

	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("fact_out() not implemented")));
	PG_RETURN_DATUM(0);
	/*
	FACT	   *cube = PG_GETARG_NDBOX(0);
	StringInfoData buf;
	int			dim = DIM(cube);
	int			i;

	initStringInfo(&buf);

	appendStringInfoChar(&buf, '(');
	for (i = 0; i < dim; i++)
	{
		if (i > 0)
			appendStringInfoString(&buf, ", ");
		appendStringInfoString(&buf, float8out_internal(LL_COORD(cube, i)));
	}
	appendStringInfoChar(&buf, ')');

	if (!cube_is_point_internal(cube))
	{
		appendStringInfoString(&buf, ",(");
		for (i = 0; i < dim; i++)
		{
			if (i > 0)
				appendStringInfoString(&buf, ", ");
			appendStringInfoString(&buf, float8out_internal(UR_COORD(cube, i)));
		}
		appendStringInfoChar(&buf, ')');
	}

	PG_FREE_IF_COPY(cube, 0);
	PG_RETURN_CSTRING(buf.data);*/

}
