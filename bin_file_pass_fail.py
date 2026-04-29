# AUTHOR: Ozge BARAN YELIM
# 
# This program checks if the T-bit Copy operation is performed correctly.
# It reads sample files created via "sample_generator_with_tbit" program.
# Sample files it reads reside in "output_aes_128_cbc_samples" folder.


import sys

if len(sys.argv) < 2 or len(sys.argv) > 3:
    print(f'Usage: {sys.argv[0]} <k> [-v]')
    sys.exit(1)

# Verbose only prints first 5 chunks. Even though pass/fail is decided based on the entire data.
verbose = '-v' in sys.argv

k_arg = next(a for a in sys.argv[1:] if a != '-v')
try:
    k = int(k_arg)
    if k <= 0:
        raise ValueError
except ValueError:
    print(f'[-] k must be a positive integer, got: {k_arg}')
    sys.exit(1)

path = f'output_aes_128_cbc_samples/t_bit_k{k}.bin'
try:
    data = open(path, 'rb').read()
except FileNotFoundError:
    print(f'[-] File not found: {path}')
    sys.exit(1)
except PermissionError:
    print(f'[-] Permission denied: {path}')
    sys.exit(1)
except OSError as e:
    print(f'[-] Could not read file: {e}')
    sys.exit(1)

print(f'File       : {path}')
print(f'k Value    : {k}')

num_chunks = len(data) // (2 * k)

ok = True
for i in range(num_chunks):
    orig = data[i*2*k : i*2*k+k]
    copy = data[i*2*k+k : (i+1)*2*k]
    if verbose and i < 5:
        print(f'Chunk {i}: orig={list(orig)}, copy={list(copy)}, match={orig==copy}')
    if orig != copy:
        if verbose:
            print(f'FAIL at chunk {i}')
        ok = False

# partial last chunk
last = num_chunks
orig = data[last*2*k : last*2*k+k]
copy = data[last*2*k+k : len(data)]
if verbose and (orig or copy):
    print(f'Partial last chunk (i={last}):')
    print(f'  orig ({len(orig)} bytes): {list(orig)}')
    print(f'  copy ({len(copy)} bytes): {list(copy)}')
    print(f'  copy matches first {len(copy)} bytes of orig: {orig[:len(copy)] == copy}')

print('[+] Result : PASS' if ok else '[-] Result : FAIL')
