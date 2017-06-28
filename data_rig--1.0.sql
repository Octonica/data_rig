-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION data_rig" to load this file. \quit

CREATE FUNCTION fact_in(cstring)
RETURNS fact
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_out(fact)
RETURNS cstring
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE fact (
	INTERNALLENGTH = variable,
	INPUT = fact_in,
	OUTPUT = fact_out,
	ALIGNMENT = integer
);

CREATE FUNCTION fact(integer[]) RETURNS fact
AS 'MODULE_PATHNAME', 'fact_ia'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_contains(fact, fact)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_contained(fact, fact)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_intersect(fact, fact)
RETURNS fact
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;



CREATE OPERATOR @> (
	LEFTARG = fact, RIGHTARG = fact, PROCEDURE = fact_contains,
	COMMUTATOR = '<@',
	RESTRICT = contsel, JOIN = contjoinsel
);


CREATE OPERATOR <@ (
	LEFTARG = fact, RIGHTARG = fact, PROCEDURE = fact_contained,
	COMMUTATOR = '@>',
	RESTRICT = contsel, JOIN = contjoinsel
);


CREATE FUNCTION fact_consistent(internal,fact,smallint,oid,internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_decompress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_penalty(internal,internal,internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_union(internal, internal)
RETURNS cube
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION fact_same(cube, cube, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OPERATOR CLASS gist_fact_ops
    DEFAULT FOR TYPE fact USING gist AS
	OPERATOR	7	@> ,
	OPERATOR	8	<@ ,

	FUNCTION	1	fact_consistent (internal, fact, smallint, oid, internal),
	FUNCTION	2	fact_union (internal, internal),
	FUNCTION	3	fact_compress (internal),
	FUNCTION	4	fact_decompress (internal),
	FUNCTION	5	fact_penalty (internal, internal, internal),
	FUNCTION	6	fact_picksplit (internal, internal),
	FUNCTION	7	fact_same (cube, cube, internal),
	FUNCTION	9	fact_decompress (internal);

CREATE FUNCTION to_fact_number(integer, integer)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
