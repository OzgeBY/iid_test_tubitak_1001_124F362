AES-128-CBC T-Bit Binary Sample Generator for NIST SP800-90B IID and Non-IID Tests
 
Generates T-bit copy binary files of length 1,000,000 samples each for different k values.
k values are built from the powers of 2 from 8 up to <= VECTOR_SIZE, then vector_size and edge cases (26,100) as the last two elements.
 
Uses AES-128 in CBC mode via OpenSSL to produce pseudorandom bytes.
Each output bit is unpacked into one byte with value 0x00 or 0x01.
AES with all-zero key/iv is deterministic — encrypt once, reuse for all k values.
 
Output format for NIST SP800-90B ea_iid and non_ea_iid:
Each Sample is written as a raw binary file: t_bit_X.bin
Compile:
   g++ -O2 -o sample_generator_with_tbit sample_generator_with_tbit.cpp  -lssl -lcrypto
 
 Run:
   mkdir -p output_aes_128_cbc_samples && ./sample_generator_with_tbit
