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
