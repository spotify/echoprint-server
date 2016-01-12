import base64
import zlib
import shutil
import itertools
from echoprint_server_c import _create_index_block


def split_seq(iterable, size):
    it = iter(iterable)
    item = list(itertools.islice(it, size))
    while item:
        yield item
        item = list(itertools.islice(it, size))


def decode_echoprint(echoprint_b64_zipped):
    '''
    Decode an echoprint string as output by `echoprint-codegen`.
    The function returns offsets and codes as list of integers.
    '''
    zipped = base64.urlsafe_b64decode(echoprint_b64_zipped)
    unzipped = zlib.decompress(zipped)
    N = len(unzipped)
    offsets = [int(''.join(o), 16) for o in split_seq(unzipped[:N/2], 5)]
    codes =  [int(''.join(o), 16) for o in split_seq(unzipped[N/2:], 5)]
    return offsets, codes


def create_inverted_index(songs, output_path):
    '''
    Create an inverted index from an iterable of song codes.
    For large number of songs (>= 65535) several files will be created,
    output_path_0001, output_path_0002, ...
    '''
    n_batches = 0

    for batch_index, batch in enumerate(split_seq(songs, 65535)):
        batch_output_path = output_path + ('_%04d' % batch_index)
        _create_index_block(list(batch), batch_output_path)
        n_batches += 1
    if n_batches == 1:
        shutil.move(batch_output_path, output_path)


def parsed_code_streamer(fstream):
    '''
    Convenience generator for reading comma-separated list of integers
    '''
    for line in fstream:
        yield [int(c) for c in line.strip().split(',')]

def parsing_code_streamer(fstream):
    '''
    Convenience generator for converting echoprint strings into codes
    '''
    for line in fstream:
        yield decode_echoprint(line.strip())[1]
