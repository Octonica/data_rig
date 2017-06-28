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
PG_FUNCTION_INFO_V1(fact_union);
PG_FUNCTION_INFO_V1(fact_consistent);
PG_FUNCTION_INFO_V1(fact_penalty);
PG_FUNCTION_INFO_V1(fact_contains);
PG_FUNCTION_INFO_V1(fact_contained);
PG_FUNCTION_INFO_V1(fact_picksplit);
PG_FUNCTION_INFO_V1(fact_intersect);
PG_FUNCTION_INFO_V1(fact_same);
PG_FUNCTION_INFO_V1(to_fact_number);

Datum
to_fact_number(PG_FUNCTION_ARGS)
{
	int32_t dimension = PG_GETARG_INT32(0);
	int32_t value = PG_GETARG_INT32(1);
	PG_RETURN_INT32((dimension<<24) + value);
}

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
static bool
isort(int32_t *a, int len)
{
	bool		r = false;

	qsort_arg(a, len, sizeof(int32_t), isort_cmp, (void *) &r);
	return r;
}

static void adjust_fact(FACT* result, int dim)
{
	int32_t			i,size;
	if(isort(result->x,dim) && dim>1)
		{
			int current = 1;
			int32_t element = result->x[0];
			for (i = 1; i < dim; i++)
			{
				if(element != result->x[i])
				{
					element = result->x[current] = result->x[i];
					current++;
				}
			}
			size = FACT_SIZE(current);
			SET_DIM(result, current);
			SET_VARSIZE(result, size);
		}
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

	adjust_fact(result, dim);

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

Datum
fact_union(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	int		   *sizep = (int *) PG_GETARG_POINTER(1);
	FACT	   *out = (FACT *) NULL;
	FACT	   *tmp;
	int			i,j,k,n;
	int32_t		*da,*db,na,nb;
	int			dim = DIM((FACT*)entryvec->vector[0].key);

	out = (FACT*)palloc(FACT_SIZE(dim));
	tmp = (FACT*)entryvec->vector[0].key;

	for(i = 0; i < dim; i++)
	{
		out->x[i] = tmp->x[i];
	}

	for (n = 1; n < entryvec->n; n++)
	{
		tmp = (FACT*)entryvec->vector[n].key;
		da = out->x;
		na = dim;
		db = tmp->x;
		nb = DIM(tmp);


		i = j = k = 0;
			while (i < na && j < nb)
			{
				if (da[i] < db[j])
					i++;
				else if (da[i] == db[j])
				{
					if (k == 0 || da[k - 1] != db[j])
						da[k++] = db[j];
					i++;
					j++;
				}
				else
					j++;
			}
		dim = k;
	}




	*sizep = FACT_SIZE(dim);
	SET_DIM(out, dim);
	SET_VARSIZE(out, *sizep);


	PG_RETURN_POINTER(out);
}

static bool
contains(int32_t *da, int na, int32_t *db, int nb)
{
	int			i,
				j,
				n;

	i = j = n = 0;
	while (i < na && j < nb)
	{
		if (da[i] < db[j])
			i++;
		else if (da[i] == db[j])
		{
			n++;
			i++;
			j++;
		}
		else
			break;				/* db[j] is not in da */
	}

	return (n == nb) ? TRUE : FALSE;
}


static bool
contains_internal(int32_t *da, int na, int32_t *db, int nb)
{
	int			i,
				j;
	
	bool afail = false;
	bool bfail = false;

	i = j = 0;
	while (i < na && j < nb)
	{	

		
		if (da[i] < db[j])
		{
			i++;
			if(GET_CLS(da[i]) != GET_CLS(db[i]))
			{
				afail = false;
				bfail = true;
			}
			else
			{
				if(bfail)
					return FALSE;
				afail = true;
			}
		}
		else if (da[i] == db[j])
		{
			i++;
			j++;
		}
		else
		{
			j++;
			
			if(GET_CLS(da[i]) != GET_CLS(db[i]))
			{
				afail = false;
				bfail = true;
			}
			else
			{
				if(afail)
					return FALSE;
				bfail = true;
			}
		}
	}

	return TRUE;
}

Datum
fact_consistent(PG_FUNCTION_ARGS)
{
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	FACT	   *key = (FACT*) entry->key;
	FACT	   *query = (FACT*) PG_GETARG_POINTER(1);
	//StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);

	/* Oid		subtype = PG_GETARG_OID(3); */
	bool	   *recheck = (bool *) PG_GETARG_POINTER(4);
	bool		res;

	/* All cases served by this function are exact */
	*recheck = false;

	/*
	 * if entry is not leaf, use g_cube_internal_consistent, else use
	 * g_cube_leaf_consistent
	 */
	if (GIST_LEAF(entry))
		res = contains(query->x,DIM(query),key->x,DIM(key));
	else
		res = contains_internal(key->x,DIM(key),query->x,DIM(query));

	PG_FREE_IF_COPY(query, 1);
	PG_RETURN_BOOL(res);
}


static int
countloss(int32_t *da, int na, int32_t *db, int nb)
{
	int			i,
				j,
				n;

	if(na==0 || nb==0)
		return na;

	i = j = n = 0;
	while (i < na && j < nb)
	{
		if (da[i] < db[j])
		{
			i++;
			n++;
		}
		else if (da[i] == db[j])
		{
			i++;
			j++;
		}
		else
		{
			j++;
		}
	}

	return n;
}

static int
countdiff(int32_t *da, int na, int32_t *db, int nb)
{
	int			i,
				j,
				n;

	if(na==0 || nb==0)
		return na+nb;

	i = j = n = 0;
	while (i < na && j < nb)
	{
		if (da[i] < db[j])
		{
			i++;
			n++;
		}
		else if (da[i] == db[j])
		{
			i++;
			j++;
		}
		else
		{
			j++;
			n++;
		}
	}

	return n;
}

Datum
fact_penalty(PG_FUNCTION_ARGS)
{
	GISTENTRY  *origentry = (GISTENTRY *) PG_GETARG_POINTER(0);
	GISTENTRY  *newentry = (GISTENTRY *) PG_GETARG_POINTER(1);
	float	   *result = (float *) PG_GETARG_POINTER(2);
	FACT	   *n = (FACT*)newentry->key;
	FACT	   *o = (FACT*)origentry->key;


	*result = countloss(o->x,DIM(o),n->x,DIM(n))+(1/(DIM(o)+1.0));



	PG_RETURN_FLOAT8(*result);
}

Datum
fact_contains(PG_FUNCTION_ARGS)
{
	FACT	   *a = (FACT*) PG_GETARG_POINTER(0),
			   *b = (FACT*) PG_GETARG_POINTER(1);
	bool		res;

	res = contains(a->x, DIM(a), b->x, DIM(b));

	PG_FREE_IF_COPY(a, 0);
	PG_FREE_IF_COPY(b, 1);
	PG_RETURN_BOOL(res);
}

Datum
fact_contained(PG_FUNCTION_ARGS)
{
	FACT	   *a = (FACT*) PG_GETARG_POINTER(0),
			   *b = (FACT*) PG_GETARG_POINTER(1);
	bool		res;

	res = contains(b->x, DIM(b), a->x, DIM(a));

	PG_FREE_IF_COPY(a, 0);
	PG_FREE_IF_COPY(b, 1);
	PG_RETURN_BOOL(res);
}


static	FACT *
fact_union_internal(FACT *a, FACT *b)
{
	int			i,j,k;
	FACT	   *result;
	int			dim;
	int			size;
	int32_t		*da,*db,*dr,na,nb;

	/* trivial case */
	if (a == b)
		return a;

	size = FACT_SIZE(Min(DIM(a),DIM(b)));
		result = palloc0(size);

	dr = result->x;

	da = a->x;
			na = DIM(a);
			db = b->x;
			nb = DIM(b);


			i = j = k = 0;
				while (i < na && j < nb)
				{
					if (da[i] < db[j])
						i++;
					else if (da[i] == db[j])
					{
						if (k == 0 || dr[k - 1] != db[j])
							dr[k++] = db[j];
						i++;
						j++;
					}
					else
						j++;
				}
			dim = k;

	size = FACT_SIZE(dim);
	SET_VARSIZE(result, size);
	SET_DIM(result, dim);


	return (result);
}


Datum
fact_intersect(PG_FUNCTION_ARGS)
{
	FACT	   *a = (FACT*) PG_GETARG_POINTER(0),
			   *b = (FACT*) PG_GETARG_POINTER(1);
	FACT		*res;

	res = fact_union_internal(b, a);

	PG_FREE_IF_COPY(a, 0);
	PG_FREE_IF_COPY(b, 1);
	PG_RETURN_POINTER(res);
}

Datum
fact_picksplit(PG_FUNCTION_ARGS)
{
	GistEntryVector *entryvec = (GistEntryVector *) PG_GETARG_POINTER(0);
	GIST_SPLITVEC *v = (GIST_SPLITVEC *) PG_GETARG_POINTER(1);
	OffsetNumber i,
				j;
	FACT	   *datum_alpha,
			   *datum_beta;
	FACT	   *datum_l,
			   *datum_r;
	FACT	   *union_dl,
			   *union_dr;
	bool		firsttime;
	double		size_alpha,
				size_beta;
	double		size_waste,
				waste;
	double		size_l,
				size_r;
	int			nbytes;
	OffsetNumber seed_1 = 1,
				seed_2 = 2;
	OffsetNumber *left,
			   *right;
	OffsetNumber maxoff;

	maxoff = entryvec->n - 2;
	nbytes = (maxoff + 2) * sizeof(OffsetNumber);
	v->spl_left = (OffsetNumber *) palloc(nbytes);
	v->spl_right = (OffsetNumber *) palloc(nbytes);

	firsttime = true;
	waste = 0.0;

	for (i = FirstOffsetNumber; i < maxoff; i = OffsetNumberNext(i))
	{
		datum_alpha = DatumGetFact(entryvec->vector[i].key);
		for (j = OffsetNumberNext(i); j <= maxoff; j = OffsetNumberNext(j))
		{
			datum_beta = DatumGetFact(entryvec->vector[j].key);


			size_waste = countdiff(datum_alpha->x,DIM(datum_alpha),datum_beta->x,DIM(datum_beta));

			/*
			 * are these a more promising split than what we've already seen?
			 */

			if (size_waste > waste || firsttime)
			{
				waste = size_waste;
				seed_1 = i;
				seed_2 = j;
				firsttime = false;
			}
		}
	}

	left = v->spl_left;
	v->spl_nleft = 0;
	right = v->spl_right;
	v->spl_nright = 0;

	datum_alpha = DatumGetFact(entryvec->vector[seed_1].key);
	datum_l = datum_alpha;
	size_l = DIM(datum_l);
	datum_beta = DatumGetFact(entryvec->vector[seed_2].key);
	datum_r = datum_beta;
	size_r = DIM(datum_r);

	/*
	 * Now split up the regions between the two seeds.  An important property
	 * of this split algorithm is that the split vector v has the indices of
	 * items to be split in order in its left and right vectors.  We exploit
	 * this property by doing a merge in the code that actually splits the
	 * page.
	 *
	 * For efficiency, we also place the new index tuple in this loop. This is
	 * handled at the very end, when we have placed all the existing tuples
	 * and i == maxoff + 1.
	 */

	maxoff = OffsetNumberNext(maxoff);
	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		/*
		 * If we've already decided where to place this item, just put it on
		 * the right list.  Otherwise, we need to figure out which page needs
		 * the least enlargement in order to store the item.
		 */

		if (i == seed_1)
		{
			*left++ = i;
			v->spl_nleft++;
			continue;
		}
		else if (i == seed_2)
		{
			*right++ = i;
			v->spl_nright++;
			continue;
		}

		/* okay, which page needs least enlargement? */
		datum_alpha = DatumGetFact(entryvec->vector[i].key);
		union_dl = fact_union_internal(datum_l, datum_alpha);
		union_dr = fact_union_internal(datum_r, datum_alpha);
		size_alpha = DIM(union_dl);
		size_beta = DIM(union_dr);

		/* pick which page to add it to */
		if (size_alpha - size_l < size_beta - size_r)
		{
			datum_l = union_dl;
			size_l = size_alpha;
			*left++ = i;
			v->spl_nleft++;
		}
		else
		{
			datum_r = union_dr;
			size_r = size_beta;
			*right++ = i;
			v->spl_nright++;
		}
	}
	*left = *right = FirstOffsetNumber; /* sentinel value, see dosplit() */

	v->spl_ldatum = PointerGetDatum(datum_l);
	v->spl_rdatum = PointerGetDatum(datum_r);

	PG_RETURN_POINTER(v);
}

static	int32
fact_cmp(FACT *a, FACT *b)
{
	int			i;
	int			dim;

	if(DIM(a) != DIM(b))
		return DIM(a) - DIM(b);

	dim = DIM(a);

	for(i = 0; i < dim; i++)
	{
		if(a->x[i]!=b->x[i])
			return a->x[i] - b->x[i];
	}
	return 0;
}

Datum
fact_same(PG_FUNCTION_ARGS)
{
	FACT	   *b1 = (FACT*)PG_GETARG_POINTER(0);
	FACT	   *b2 = (FACT*)PG_GETARG_POINTER(1);
	bool	   *result = (bool *) PG_GETARG_POINTER(2);

	if (fact_cmp(b1, b2) == 0)
		*result = TRUE;
	else
		*result = FALSE;

	PG_RETURN_POINTER(result);
}
