# Chunked Pipeline Plan for 10^25+

## Problem
Stage 1 p*.dat files grow ~3.16x per decade. At 10^25 that's 7.7 TB, at 10^26 it's 25.2 TB — exceeding the 18 TB HDD. Even when it fits, the USB HDD is a bottleneck.

## Strategy: NVMe-Only Chunked Pipeline

Divide the n-range into chunks small enough that each chunk's full working set (p*.dat + primes*.dat) fits on the NVMe SSD (~740 GB free). Process each chunk through stages 1–3.5, then append the small result files (emirps, ottos) to cumulative files on the HDD. The next chunk's stages truncate and overwrite the intermediate files in place — no explicit cleanup needed. Run stage 4 once at the end on the merged emirps file.

**Chunk size: ~500 GB of p*.dat per chunk (~535 GB total working space)**

| Decade | Total p*.dat | Chunks | Working space/chunk |
|--------|-------------|--------|-------------------|
| 10^25  | 7.7 TB      | 16     | ~535 GB           |
| 10^26  | 25.2 TB     | 52     | ~540 GB           |
| 10^27  | 82.7 TB     | 170    | ~541 GB           |

This approach scales indefinitely — only disk space for one chunk's working set + accumulated emirps is needed.

## Implementation

### 1. New orchestration script: `run_chunked.sh`

**Location:** `/home/jim/programming/c/BigFermat/gmp-cvo/run_chunked.sh`

**Usage:** `./run_chunked.sh <start_prime> <end_prime> <num_chunks>`
- Example: `./run_chunked.sh 10000000000000000000000000 100000000000000000000000000 16`

**Logic (pseudocode):**
```
parse start_prime, end_prime, num_chunks
n_start = sqrt(start_prime / 2)
n_end = sqrt(end_prime / 2)    # ~end of valid gatekeeper zone
chunk_size = (n_end - n_start) / num_chunks

WORKDIR=/home/jim/cvpipe-run           # on NVMe (pipeline runs here)
RESULTSDIR=/mnt/cvp-working/data/run   # on HDD (cumulative results)

for i in 1..num_chunks:
    chunk_n_start = n_start + (i-1) * chunk_size
    chunk_n_end = n_start + i * chunk_size
    chunk_max_prime = 2 * chunk_n_end^2 + 2 * chunk_n_end + 1

    # Stage 1: generate candidates (truncates & writes p01-p16.dat)
    ./gen_candidates_range_gmp $chunk_n_start $chunk_max_prime

    # Stage 2: prime filter (truncates & writes primes01-16.dat)
    ./filter_primes_gmp

    # Stage 3: emirp detection (truncates & writes emirps.dat)
    ./check_emirp_gmp

    # Stage 3.5: palindrome detection (truncates & writes otto_primes.dat)
    ./check_palindrome_gmp

    # Append results to cumulative files on HDD before next chunk overwrites
    cat emirps.dat >> $RESULTSDIR/emirps_all.dat
    cat otto_primes.dat >> $RESULTSDIR/otto_all.dat

    log "Chunk $i/$num_chunks complete"
done

# Stage 4: converse check on accumulated emirps
cp $RESULTSDIR/emirps_all.dat emirps.dat
./check_converse_gmp

# Save final results to HDD
cp converse.dat $RESULTSDIR/
```

### 2. No C source code changes required

All existing programs work as-is:
- `gen_candidates_range_gmp` takes (start_n, max_prime) and truncates output files (opens "w")
- `filter_primes_gmp` reads p*.dat, writes primes*.dat — truncates on open
- `check_emirp_gmp` reads primes*.dat, writes emirps.dat — truncates on open
- `check_palindrome_gmp` reads primes*.dat, writes otto_primes.dat — truncates on open
- `check_converse_gmp` reads emirps.dat — runs once at end on merged file

No intermediate file deletion needed — each stage overwrites the previous chunk's files.

### 3. Script features

- **Resume support:** Track completed chunks in a progress file (`$RESULTSDIR/progress.log`). On restart, skip already-completed chunks.
- **Logging:** Per-chunk timing + candidate/prime/emirp counts captured from stage stdout.
- **Disk check:** Verify NVMe free space before each chunk (warn if < 200 GB).
- **Big-integer math:** Use python3 one-liners to compute n-ranges and max_prime values (these exceed bash integer limits).

### 4. Disk layout during run

```
/home/jim/cvpipe-run/                (NVMe - working directory, ~535 GB active)
├── gen_candidates_range_gmp         (binary)
├── filter_primes_gmp                (binary)
├── check_emirp_gmp                  (binary)
├── check_palindrome_gmp             (binary)
├── check_converse_gmp               (binary)
├── p01.dat ... p16.dat              (overwritten each chunk by stage 1)
├── primes01.dat ... primes16.dat    (overwritten each chunk by stage 2)
├── emirps.dat                       (overwritten each chunk by stage 3)
├── otto_primes.dat                  (overwritten each chunk by stage 3.5)
└── converse.dat                     (final output from stage 4)

/mnt/cvp-working/data/run_10e25/     (HDD - cumulative results, safe storage)
├── emirps_all.dat                   (appended after each chunk, ~few GB)
├── otto_all.dat                     (appended after each chunk, tiny)
├── progress.log                     (chunk completion tracker for resume)
└── converse.dat                     (final copy)
```

### 5. Files to create/modify

| File | Action | Description |
|------|--------|-------------|
| `run_chunked.sh` | **Create** | Orchestration script (~100 lines bash) |
| C source files | **None** | No changes needed |
| Makefile | **Optional** | Add `make chunked-10e25` convenience target |

### Verification

1. Test with a small range: `./run_chunked.sh 100000000 1000000000 4` (10^8 to 10^9, 4 chunks)
2. Run same range non-chunked: `make test` or equivalent single-pass
3. Diff the emirps and converse output — must be identical (though line order may differ within chunks due to thread interleaving, so sort before diffing)
4. Monitor NVMe free space stays above ~200 GB during chunked run
