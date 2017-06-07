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
