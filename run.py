# AUTHOR: Ozge BARAN YELIM

# This orchestrator program executes a series of commands to run the following programs respectively based on the result of each program.
# Programs are sample_generator_with_tbit that creates the t_bit copy samples, bin_file_pass_fail.py that checks if t_bit copy
# operations are performed correctly and ea_iid binary from SP800-90B_EntropyAssessment to yield the final result.
# Results from each program affect the other program that will be run. 

import subprocess
import os


def run_sample_generator():
    binary = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sample_generator_with_tbit')
    if not os.path.isfile(binary):
        print(f'[-] Binary not found: {binary}')
        return False, []
    if not os.access(binary, os.X_OK):
        print(f'[-] Binary is not executable: {binary}')
        return False, []

    print('\033[92m[*] Sample generation started.\033[0m')
    k_values = []
    proc = subprocess.Popen([binary], cwd=os.path.dirname(binary), stdout=subprocess.PIPE, stderr=None, text=True)
    for line in proc.stdout:
        print(line, end='')
        if line.startswith('k_values: ['):
            inner = line.strip()[len('k_values: ['):-1]
            k_values = [int(x.strip()) for x in inner.split(',') if x.strip()]
    proc.wait()

    if proc.returncode == 0:
        print('\033[92m[+] Sample generation ended successfully.\033[0m')
        return True, k_values
    else:
        print(f'\033[91m[-] Sample generation ended with return code {proc.returncode}.\033[0m')
        return False, []


def run_pass_fail(k_values, verbose=False):
    print('\033[92m[*] T-bit pass/fail check started.\033[0m')
    script = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'bin_file_pass_fail.py')
    all_passed = True
    for k in k_values:
        args = ['python3', script, str(k)]
        if verbose:
            args.append('-v')
        proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=None, text=True)
        for line in proc.stdout:
            print(line, end='')
            if '[-] Result : FAIL' in line:
                all_passed = False
        proc.wait()
    print('\033[92m[*] T-bit pass/fail check ended.\033[0m')
    return all_passed


def run_ea_iid():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    binary_dir = os.path.normpath(os.path.join(base_dir, '..', 'SP800-90B_EntropyAssessment', 'cpp'))
    binary = os.path.join(binary_dir, 'ea_iid')
    samples_dir = os.path.join(base_dir, 'output_aes_128_cbc_samples')

    if not os.path.isfile(binary):
        print(f'\033[91m[-] ea_iid binary not found: {binary}\033[0m')
        return False
    if not os.access(binary, os.X_OK):
        print(f'\033[91m[-] ea_iid binary is not executable: {binary}\033[0m')
        return False
    if not os.path.isdir(samples_dir):
        print(f'\033[91m[-] Samples directory not found: {samples_dir}\033[0m')
        return False

    files = sorted(f for f in os.listdir(samples_dir) if os.path.isfile(os.path.join(samples_dir, f)))
    if not files:
        print(f'\033[91m[-] No files found in {samples_dir}\033[0m')
        return False

    print('\033[92m[*] ea_iid analysis started.\033[0m')
    rel_samples = os.path.join('..', '..', 'iid_non_iid_sequence_generator', 'output_aes_128_cbc_samples')
    summary = {}
    for filename in files:
        print(f'\033[92m[*] Processing {filename} ...\033[0m')
        proc = subprocess.Popen(
            [binary, '-a', os.path.join(rel_samples, filename), '8'],
            cwd=binary_dir, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
        )
        results = []
        for line in proc.stdout:
            print(line, end='', flush=True)
            stripped = line.strip()
            if stripped.startswith('** Passed') or stripped.startswith('** Failed'):
                results.append(stripped)
        proc.wait()
        summary[filename] = results

    print('\033[92m[*] ea_iid analysis ended.\033[0m')
    print()
    print('=== ea_iid Summary ===')
    for filename, results in summary.items():
        print(f'\n  {filename}:')
        for r in results:
            color = '\033[92m' if r.startswith('** Passed') else '\033[91m'
            print(f'    {color}{r}\033[0m')
    return True


def main():
    success, k_values = run_sample_generator()
    if success and k_values:
        passed = run_pass_fail(k_values)
        if passed:
            run_ea_iid()


if __name__ == '__main__':
    main()
