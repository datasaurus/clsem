#!/bin/sh
#
# Usage:
#	clsem.sh n path [-i id] command args ...
#
# Read filenames from standard input.
# Run up to n instances of "command args ... filename"
# Limit instances with a semaphore associated with path and optional
# key identifier id.
#
# clsem must be in PATH.

if [ $# -lt 3 ]
then
    echo "Usage: $0 n path [-i id] command args ..." 1>&2
    exit 1
fi
n=$1
if ! printf '%d' $n > /dev/null
then
    echo "$0: expected integer for count, got $n" 1>&2
    exit 1
fi
shift
path=$1
shift
if [ "$1" = "-i" ]
then
    shift
    id=$1
    shift
fi
trap "clsem -d $path $id" EXIT QUIT HUP TERM INT KILL
clsem -c $path $id > /dev/null || exit 1
clsem $n $path $id || exit 1
while read f
do
    clsem -1 $path $id || exit 1
    (
	trap "clsem 1 $path $id" EXIT QUIT HUP TERM INT KILL
	"$@" "$f"
    ) &
done
wait
