/*
 * AUTHOR: Ozge BARAN YELIM
 * AES-128-CBC T-Bit Binary Sample Generator for NIST SP800-90B IID and Non-IID Tests
 *
 * Generates T-bit copy binary files of length 1,000,000 samples each for different k values.
 * k values are built from the powers of 2 from 8 up to <= VECTOR_SIZE, then vector_size and edge cases (26,100) as the last two elements.
 * 
 * Uses AES-128 in CBC mode via OpenSSL to produce pseudorandom bytes.
 * Each output bit is unpacked into one byte with value 0x00 or 0x01.
 * AES with all-zero key/iv is deterministic — encrypt once, reuse for all k values.
 *
 * Output format for NIST SP800-90B ea_iid and non_ea_iid:
 *   Each Sample is written as a raw binary file: t_bit_X.bin
 *
 * Compile:
 *   g++ -O2 -o sample_generator_with_tbit sample_generator_with_tbit.cpp  -lssl -lcrypto
 *
 * Run:
 *   mkdir -p output_aes_128_cbc_samples && ./sample_generator_with_tbit
 */

#include <openssl/evp.h>
#include <openssl/err.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <stdexcept>


static const int VECTOR_SIZE    = 500000;
static const int OUTPUT_SIZE    = 1000000;
static const int PACKED_BYTES   = (VECTOR_SIZE + 7) / 8;
static const int AES_BLOCK_SIZE = 16;
static const int PADDED_BYTES   = ((PACKED_BYTES + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;

static bool aes128_cbc_encrypt(const unsigned char *key,
                                const unsigned char *iv,
                                unsigned char *out,
                                int out_len)
{
    // Validate inputs
    if (key == nullptr || iv == nullptr || out == nullptr) {
        return false;
    }

    // The input length MUST be a multiple of the AES block size (16 bytes). Otherwise EVP_EncryptFinal_ex will fail.
    if (out_len <= 0 || (out_len % 16) != 0) {
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr) {
        return false;
    }

    bool success = false;
    int len = 0;
    int ciphertext_len = 0;

    // Use a temporary buffer because EVP_EncryptUpdate cannot safely
    // write to the same buffer it reads from.
    unsigned char *tmp = new (std::nothrow) unsigned char[out_len];
    if (tmp == nullptr) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Do-while is used as try/finally block.
    do {
        // Initialize the encryption operation with AES-128-CBC
        if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key, iv) != 1) {
            break;
        }

        // Disable PKCS#7 padding
        if (EVP_CIPHER_CTX_set_padding(ctx, 0) != 1) {
            break;
        }

        // Encrypt the data (read from out, write to tmp)
        if (EVP_EncryptUpdate(ctx, tmp, &len, out, out_len) != 1) {
            break;
        }
        ciphertext_len = len;

        // Finalize encryption. With padding disabled and a properly
        // block-aligned input, this should produce 0 additional bytes.
        if (EVP_EncryptFinal_ex(ctx, tmp + len, &len) != 1) {
            break;
        }
        ciphertext_len += len;

        // Sanity check: encrypted length must equal input length when
        // padding is disabled.
        if (ciphertext_len != out_len) {
            break;
        }

        // Copy the ciphertext back into the caller's buffer
        std::memcpy(out, tmp, out_len);
        success = true;

    } while (false);

    // Clean up
    delete[] tmp;
    EVP_CIPHER_CTX_free(ctx);

    return success;
}

void t_bit_copy(std::vector<unsigned char> &unpacked, std::size_t k)
{
    std::size_t original_size = unpacked.size();
    std::size_t num_chunks = (original_size + k - 1) / k;  // ceil division

    std::vector<unsigned char> result(num_chunks * 2 * k, 0);

    for (std::size_t i = 0; i < num_chunks; ++i)
    {
        std::size_t src_start  = i * k;
        std::size_t chunk_size = std::min(k, original_size - src_start);
        auto src = unpacked.begin() + src_start;
        auto dst = result.begin() + i * 2 * k;
        std::copy_n(src, chunk_size, dst);
        std::copy_n(src, chunk_size, dst + k);
    }

    result.resize(OUTPUT_SIZE);  // truncate to exactly 1,000,000
    unpacked = std::move(result);
}

int main()
{
    std::vector<unsigned char> packed(PADDED_BYTES);
    std::vector<unsigned char> unpacked_base(VECTOR_SIZE);
    static const unsigned char zeros[AES_BLOCK_SIZE] = {};

    // ================== Start : AES128-CBC Encryption ==================
    // AES with all-zero key/IV is deterministic — encrypt once, reuse for all k values
    if (!aes128_cbc_encrypt(zeros, zeros, packed.data(), PADDED_BYTES))
    {
        std::fprintf(stderr, "[-] AES-128-CBC encryption failed\n");
        return 1;
    }
    // ================== END : AES128-CBC Encryption ====================

    // ================== Start: Unpacking packed vector =================
    // Ex: packed[0] = 0xB2 = 10110010, unpacked_base = [ 1, 0, 1, 1, 0, 0, 1, 0,   <- from 0xB2
    int out_idx = 0;
    for (int i = 0; i < PACKED_BYTES && out_idx < VECTOR_SIZE; ++i)
    {
        unsigned char byte = packed[i];
        for (int b = 7; b >= 0 && out_idx < VECTOR_SIZE; --b)
            unpacked_base[out_idx++] = (byte >> b) & 0x01;
    }
    // ================== End: Unpacking packed vector ===================

    // ================== Start : Build k_values ==================
    // Build k_values: powers of 2 from 8 up to <= VECTOR_SIZE, then vector_size and edge cases as the last two elements.
    std::vector<int> k_values;
    for (int k = 8; k <= VECTOR_SIZE; k *= 2)
        k_values.push_back(k);
    if (k_values.back() != VECTOR_SIZE)
        k_values.push_back(VECTOR_SIZE);
    
    // Edge cases : k=26 and k=100 values will be added to k_values vector.
    k_values.push_back(26);
    k_values.push_back(100);

    std::fprintf(stdout, "k_values: [");
    for (std::size_t i = 0; i < k_values.size(); ++i)
        std::fprintf(stdout, i + 1 < k_values.size() ? "%d, " : "%d]\n", k_values[i]);

    //std::vector<int> k_values = {8, 16, 32, 64, 128, 256, ..., 500000, 26, 100};   
    // ================== End : Build k_values ====================

    // ================== Start : K Values Iteration, T-bit Copy Operation & Sample Files Generation ==================
    for (int k : k_values)
    {

        std::vector<unsigned char> unpacked = unpacked_base;

        // T-bit Copy Operation
        t_bit_copy(unpacked, k);

        char filename[128];
        std::snprintf(filename, sizeof(filename),
                      "output_aes_128_cbc_samples/t_bit_k%d.bin", k);
        FILE *fp = std::fopen(filename, "wb");
        
        if (!fp)
        {
            std::perror(filename);
            return 1;
        }

        size_t written = std::fwrite(unpacked.data(), 1, unpacked.size(), fp);
        std::fclose(fp);
        if (written != unpacked.size())
        {
            std::fprintf(stderr, "[-] Failed to write file for k=%d \n", k);
            return 1;
        }
        
        std::fprintf(stdout, "[+] k=%d done\n", k);
    }

    std::fprintf(stdout, "[+] Completed. Written to output_aes_128_cbc_samples/\n");

    // ================== End : K Values Iteration, T-bit Copy Operation & Sample Files Generation ==================
    return 0;
}
