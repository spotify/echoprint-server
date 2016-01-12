# echoprint-server #

A C library, with Python bindings, for fast indexing and querying of
echoprint data.


## Usage ##

To build, run `python setup.py install`.
The following documents convenience scripts in the `bin/` directory.

#### WARNING ####

The library uses a custom binary format for speed. At this point,
**ENDIANNESS IS NOT CHECKED** so moving index files between machines
with different architectures might cause problems.


### `echoprint-decode` ###

Convert a codestring as output by `echoprint-codegen` into
the corresponding list of codes represented as comma-separated integers.

Usage:

	echoprint-codegen song.ogg > codegen_output.json
	cat codegen_output.json | jq -r '.[0].code' | echoprint-decode > codes.txt

`codes.txt` will look like:

`150555,1035718,621673,794882,40662,955768,96899,166055,...`

*N.B. This script only outputs only the echoprint codes, not the
 offsets.*


### `echoprint-inverted-index` ###

Takes a series of echoprint strings (one per line) and
an output path. Writes a compact index to disk.

Usage:

    cat ... | ./echoprint-inverted-index index.bin

`index.bin` format is binary, see the implementation details below.

If more than 65535 songs are indexed, the output will be split into
blocks with the following naming scheme:

    index.bin_0000
	index.bin_0001
	...

Optionally the `-i` switch switches the input format to a
comma-separated list of integer codes (one song per line).

### `echoprint-inverted-query` ###

Takes a series of echoprint strings (one per line) and a list of index
blocks. For each query outputs results on stdout as json-encoded
objects.

Usage:

    cat ... | ./echoprint-inverted-query index-file-1 [index-file-2 ...]

where the input is an echoprint string per line;

Each output line looks like the following:

```
{
  "results": [
    {
      "index": 0,
      "score": 0.69340412080287933,
    },
    {
      "index": 8,
      "score": 0.56301175890117883,
    },
    {
      "index": 120,
      "score": 0.31826272477954626,
    },
    ...
```


The `index` field represents the position of the matched song in the
index.

Optionally the `-i` switch switches the input format to a
comma-separated list of integer codes (one song per line).


## REST service ##

The `echoprint-rest-service` script listens for POST requests (by
default on port 5678), with an echoprint string as `echoprint`
parameter. The `test-rest.sh` shows how to query using `curl`.

The request is made to `host:query/<METHOD>` with `<METHOD>` one of

- `jaccard`
- `set_int`
- `set_int_norm_length_first`

Usage:

	echoprint-rest-service index-file-1 [index-file-2 ...]

The optional `--ids-file` accepts a path to a text file where each
line represents an id for the correspondingly-indexed track in the
index. If specified, the returned results will have an `id` field.

## Example: querying from audio ##

Assuming `0005dad86d4d4c6fb592d42d767e117f.ogg` is in the current
directory, let's cut it from 00:30 to 4:30 and re-encode it as 128
kbps mp3 (to show that echoprint is robust to alterations in the
file):


	ffmpeg -i 0005dad86d4d4c6fb592d42d767e117f.ogg \
		-s 30 -t 240 \
		0005dad86d4d4c6fb592d42d767e117f_cut_lowrate.mp3

Run the echoprint codegen, extract the echoprint string:

    ../echoprint-codegen/echoprint-codegen
        0005dad86d4d4c6fb592d42d767e117f_cut_lowrate.mp3 \
        | jq -r '.[0].code' \
        > 0005dad86d4d4c6fb592d42d767e117f_cut_lowrate.echoprint```

Query the service:

    curl -s --data \
        echoprint=`cat 0005dad86d4d4c6fb592d42d767e117f_cut_lowrate.echoprint` \
        <server-path>:5678/query

Results should be similar to

    {
      "results": [
        {
          "id": "0005dad86d4d4c6fb592d42d767e117f",
          "index": 0,
          "score": 0.34932565689086914
        },
        {
          "id": "ee59c151d679413a80ac4e49ac92c662",
          "index": 698096,
          "score": 0.033668458461761475
        },
        {
          "id": "026526e6a02648668ff9f410faab15be",
          "index": 312466,
          "score": 0.015930989757180214
        },
        ...
      ]
    }


## Building the standalone C library ##

Build with CMake.

Depending on the platform, `libechoprinttools.so` (linux) or
`libechoprinttools.dylib` will be created. On linux it might be
necessary to put the library file in `LD_LIBRARY_PATH`.


## Implementation details ##

### Similarity ###

The similarity between two echoprints is computed on their
**bag-of-words** representations. This means that the codes' offsets
are not considered, nor are the codes' multiplicities.

### Inverted index binary format ###

The inverted index is serialized as a memory dump of all the fields of
the `EchoprintInvertedIndex` struct defined in the header file.
