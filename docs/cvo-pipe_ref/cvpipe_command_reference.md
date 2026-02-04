# CVPipe Command Reference

**Project:** Converse Prime Research  
**Author:** j (Alaska)  
**System:** CVPipe Pipeline  
**Version:** GMP Edition (Unlimited Range)  

---

## Quick Reference

### Complete Pipeline Execution

```bash
# Full 5-stage pipeline (typical usage)
./gen_candidates_range_gmp <start_n> <end_value> && \
./filter_primes_gmp && \
./check_emirp_gmp && \
./check_palindrome_gmp && \
./check_converse_gmp

# Example: Search 10^23 to 10^24
./gen_candidates_range_gmp 223606797750 1000000000000000000000000 && \
./filter_primes_gmp && \
./check_emirp_gmp && \
./check_palindrome_gmp && \
./check_converse_gmp
```

### Background Execution with Logging

```bash
# Run entire pipeline in background, log to nohup.out
nohup time ./gen_candidates_range_gmp 223606797750 1000000000000000000000000 && \
./filter_primes_gmp && \
./check_emirp_gmp && \
./check_palindrome_gmp && \
./check_converse_gmp &

# Monitor progress
tail -f nohup.out

# Check if still running
ps aux | grep gen_candidates
```

---

## Stage 1: Candidate Generation

### gen_candidates_range_gmp

**Purpose:** Generate candidates of the form n² + (n+1)² within a specified range.

**Compilation:**
```bash
gcc -O3 -fopenmp -Wall -march=native gen_candidates_range_gmp.c -o gen_candidates_range_gmp -lgmp
```

**Usage:**
```bash
./gen_candidates_range_gmp <start_n> <end_value>
```

**Parameters:**
- `<start_n>`: Starting n value (candidate will be start_n² + (start_n+1)²)
- `<end_value>`: Maximum candidate value (not n value, but the actual candidate)

**Input:** Command line parameters only  
**Output:** p01.dat through p16.dat (16 sharded files)

**Examples:**

```bash
# Search up to 10^12 (1 trillion)
./gen_candidates_range_gmp 1 1000000000000

# Search from 10^23 to 10^24
./gen_candidates_range_gmp 223606797750 1000000000000000000000000

# Resume search from specific n (if interrupted)
./gen_candidates_range_gmp 500000000 1000000000000
```

**Output Format:**

Each pXX.dat file contains decimal numbers, one per line:
```
100000000000000000000005
100000000000000000000013
100000000000000000000041
...
```

**Performance Characteristics:**

```
Typical throughput: 500-800 million candidates/minute (nitroIII, 16 threads)
Memory usage: ~100 MB per thread
Disk I/O: Sequential writes, ~2-5 GB/hour
```

**Notes:**
- start_n is the starting n value, NOT the starting candidate value
- end_value is the maximum candidate value to generate
- To convert candidate back to n: n ≈ sqrt(candidate / 2)
- Zone-skipping automatically applied (cannot be disabled)
- Output files are TEXT format (GMP numbers as decimal strings)

**Troubleshooting:**

```bash
# If output files exist from previous run, they are OVERWRITTEN
# To preserve previous run:
mv p01.dat p01.dat.backup
# ... or move to archive directory

# Check progress (while running):
ls -lh p*.dat  # File sizes grow in real-time
wc -l p01.dat  # Count candidates generated so far

# Verify output format:
head -5 p01.dat
# Should show large decimal numbers, one per line
```

---

## Stage 2: Prime Filtering

### filter_primes_gmp

**Purpose:** Test candidates from Stage 1 for primality using Miller-Rabin.

**Compilation:**
```bash
gcc -O3 -fopenmp -Wall -march=native filter_primes_gmp.c -o filter_primes_gmp -lgmp
```

**Usage:**
```bash
./filter_primes_gmp
```

**Parameters:** None (reads from p01.dat - p16.dat)

**Input:** p01.dat through p16.dat  
**Output:** primes01.dat through primes16.dat

**Configuration (in source):**
```c
#define NUM_THREADS 16
#define MR_ROUNDS 25  // Miller-Rabin rounds
```

**Examples:**

```bash
# Standard usage (after gen_candidates)
./filter_primes_gmp

# Run in background with timing
time ./filter_primes_gmp
```

**Output Format:**

Each primesXX.dat file contains primes, one per line:
```
100000000000000000000005
100000000000000000000013
100000000000000000000041
...
```

**Performance Characteristics:**

```
Typical throughput: 2,000-2,500 candidates/sec tested (nitroIII, 16 threads)
Miller-Rabin rounds: 25 (error probability < 2^-50)
Prime density: ~8.5-9% in n² + (n+1)² sequences
Memory usage: ~50 MB per thread
Disk I/O: Read 2-5 GB/hour, write ~200-500 MB/hour
```

**Runtime Estimates:**

```
10^12 range:    ~5 minutes
10^18 range:    ~30 minutes
10^23 range:    ~3-4 hours
10^24 range:    ~3.5 hours (next 10× increment)
```

**Output Statistics:**

Upon completion, displays:
```
[Thread  0] Complete: 2003524800 candidates -> 171731316 primes (8.57%)
[Thread  1] Complete: 2003524800 candidates -> 171725253 primes (8.57%)
...
FILTERING COMPLETE
   Total candidates:   32056396802
   Primes found:        2747758244
   Prime density:            8.57%
   Elapsed time:             12933 seconds
```

**Troubleshooting:**

```bash
# If input files missing:
ls p*.dat
# Should show p01.dat through p16.dat

# If previous output exists, it is OVERWRITTEN
# To preserve:
mkdir archive
mv primes*.dat archive/

# Check progress:
ls -lh primes*.dat  # File sizes grow
wc -l primes01.dat  # Count primes found so far

# Verify output:
head -5 primes01.dat
# Should show primes only
```

---

## Stage 3: Emirp Detection

### check_emirp_gmp

**Purpose:** Find emirps (primes whose reverse is also prime) from Stage 2 output.

**Compilation:**
```bash
gcc -O3 -fopenmp -Wall -march=native check_emirp_gmp.c -o check_emirp_gmp -lgmp
```

**Usage:**
```bash
./check_emirp_gmp
```

**Parameters:** None (reads from primes01.dat - primes16.dat)

**Input:** primes01.dat through primes16.dat  
**Output:** emirps.dat (single merged file)

**Configuration (in source):**
```c
#define NUM_THREADS 16
#define MR_ROUNDS 25  // For testing reversed number
```

**Examples:**

```bash
# Standard usage (after filter_primes)
./check_emirp_gmp

# Run in background
time ./check_emirp_gmp
```

**Output Format:**

emirps.dat contains emirp pairs (prime and its reverse):
```
100000000000000000000041 140000000000000000000001
100000000000000000000061 160000000000000000000001
...
```

**Performance Characteristics:**

```
Typical throughput: 160,000-200,000 primes checked/sec (nitroIII, 16 threads)
Emirp density: ~7-8% of primes are emirps
Memory usage: ~30 MB per thread (with optimized static buffers)
Disk I/O: Read ~500 MB/hour, write ~5-10 GB/hour
```

**Runtime Estimates:**

```
10^12 range:    ~1 minute
10^18 range:    ~5 minutes
10^23 range:    ~20-25 minutes
10^24 range:    ~20-25 minutes (scales with prime count, not range)
```

**Output Statistics:**

Upon completion, displays:
```
[Thread  0] Complete: 171731316 primes, 0 palindromes, 13204757 emirps
[Thread  1] Complete: 171725253 primes, 0 palindromes, 13202580 emirps
...
EMIRP CHECK COMPLETE
   Total primes:        2747758244
   Palindromes:                  0
   Emirps found:         211273732
   Elapsed time:              1308 seconds
```

**Important Notes:**

- **Streaming output:** Results written immediately as found (no memory limit)
- **Palindromes excluded:** Palindromic primes are NOT emirps by definition
- **Critical section:** Minimal lock contention (~0.1% overhead)
- **File size warning:** emirps.dat can be 5+ GB for large searches

**Optimization Status:**

✅ **v2.0 (Jan 2026):** Thread-local static buffers (malloc elimination)
- Replaced per-call malloc/free with static __thread buffer
- Expected speedup: 10-20% over previous version
- Memory stable at ~500 MB total (vs unbounded in old version)

**Troubleshooting:**

```bash
# Monitor progress in real-time:
tail -f emirps.dat
wc -l emirps.dat  # Count as it grows

# If emirps.dat exists from previous run:
# It will be OVERWRITTEN - back up first!
mv emirps.dat emirps_previous.dat

# Verify emirp pairs:
head -5 emirps.dat
# Format: "prime reverse_prime" on each line

# Check for palindromes (should be none):
grep "^\([0-9]*\) \1$" emirps.dat
# No output expected (palindromes are filtered)
```

---

## Stage 3.5: Palindrome Hunter (Otto Primes)

### check_palindrome_gmp

**Purpose:** Find palindromic primes in the n² + (n+1)² sequence.

**Compilation:**
```bash
gcc -O3 -fopenmp -Wall -march=native check_palindrome_gmp.c -o check_palindrome_gmp -lgmp
```

**Usage:**
```bash
./check_palindrome_gmp
```

**Parameters:** None (reads from primes01.dat - primes16.dat)

**Input:** primes01.dat through primes16.dat  
**Output:** otto_primes.dat

**Configuration (in source):**
```c
#define NUM_THREADS 16
```

**Examples:**

```bash
# Standard usage
./check_palindrome_gmp

# Quick check (very fast stage)
time ./check_palindrome_gmp
```

**Output Format:**

otto_primes.dat contains palindromic primes (if any found):
```
12345678987654321
98765432123456789
...
```

**Performance Characteristics:**

```
Typical throughput: 5-10 million primes checked/sec (nitroIII, 16 threads)
Memory usage: ~20 MB per thread
Palindrome density: Unknown (zero found to date)
Runtime: ~30-60 seconds for billion-prime search
```

**Historical Note:**

**Zero Otto primes found to date** after 25 years of searching to 10²⁴.

**Output Statistics:**

Upon completion, displays:
```
[Thread  0] Complete: 171731316 primes, 0 palindromes
[Thread  1] Complete: 171725253 primes, 0 palindromes
...
PALINDROME CHECK COMPLETE
   Total primes:        2747758244
   Otto primes:                  0
   Elapsed time:                40 seconds
```

**Troubleshooting:**

```bash
# Check if any Otto primes found:
wc -l otto_primes.dat
# Typically returns 0

# If any found (highly unlikely):
cat otto_primes.dat
```

---

## Stage 4: Converse Verification

### check_converse_gmp

**Purpose:** Verify if emirps are also converse primes (both p and reverse(p) are consecutive square sums).

**Compilation:**
```bash
gcc -O3 -fopenmp -Wall -march=native check_converse_gmp.c -o check_converse_gmp -lgmp
```

**Usage:**
```bash
./check_converse_gmp
```

**Parameters:** None (reads from emirps.dat)

**Input:** emirps.dat  
**Output:** converse.dat

**Configuration (in source):**
```c
#define NUM_THREADS 16
#define SQRT_SEARCH_WINDOW 10  // Range around sqrt(p/2) to search
```

**Examples:**

```bash
# Standard usage (after check_emirp)
./check_converse_gmp

# Quick run (usually fast)
time ./check_converse_gmp
```

**Output Format:**

converse.dat contains confirmed converse pairs:
```
12641 14621
# Additional pairs if found (none to date beyond this one)
```

**Performance Characteristics:**

```
Typical throughput: 2-3 million emirp pairs checked/sec (nitroIII, 16 threads)
Memory usage: ~30 MB per thread
Converse density: ~1 in 1.36 billion primes (only 1 known)
Runtime: ~1-2 minutes for 200M emirp search
```

**Runtime Estimates:**

```
10^12 range:    ~1 second
10^18 range:    ~10 seconds
10^23 range:    ~90-120 seconds
10^24 range:    ~90-120 seconds (scales with emirp count)
```

**Output Statistics:**

Upon completion, displays:
```
CON-VERSE CHECK COMPLETE
   Emirp pairs checked:     211273732
   CON-VERSE FOUND:                 0
   Elapsed time:                   91 seconds
```

**Known Results:**

Only **one converse pair confirmed** in ranges up to 10²⁴:
```
12641 = 79² + 80²   (prime, emirp)
14621 = 85² + 86²   (prime, emirp, reverse of 12641)
```

**Troubleshooting:**

```bash
# Check results:
cat converse.dat

# If empty (typical):
# No converse pairs found in this range

# Verify algorithm on known pair (should be in output from earlier range):
grep "12641" converse.dat
# Expected: 12641 14621
```

---

## Pipeline Orchestration

### Makefile Automation

**Create Makefile for build automation:**

```makefile
CC = gcc
CFLAGS = -O3 -fopenmp -Wall -march=native
LIBS = -lgmp

ALL = gen_candidates_range_gmp filter_primes_gmp check_emirp_gmp \
      check_palindrome_gmp check_converse_gmp

all: $(ALL)

gen_candidates_range_gmp: gen_candidates_range_gmp.c
   $(CC) $(CFLAGS) $< -o $@ $(LIBS)

filter_primes_gmp: filter_primes_gmp.c
   $(CC) $(CFLAGS) $< -o $@ $(LIBS)

check_emirp_gmp: check_emirp_gmp.c
   $(CC) $(CFLAGS) $< -o $@ $(LIBS)

check_palindrome_gmp: check_palindrome_gmp.c
   $(CC) $(CFLAGS) $< -o $@ $(LIBS)

check_converse_gmp: check_converse_gmp.c
   $(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
   rm -f $(ALL) *.o

cleandata:
   rm -f p*.dat primes*.dat emirps.dat otto_primes.dat converse.dat

.PHONY: all clean cleandata
```

**Usage:**

```bash
# Build all programs
make

# Clean binaries
make clean

# Clean data files (CAUTION: deletes results!)
make cleandata

# Rebuild everything
make clean && make
```

### Shell Script Wrapper

**Create run_pipeline.sh:**

```bash
#!/bin/bash

# CVPipe Full Pipeline Runner
# Usage: ./run_pipeline.sh <start_n> <end_value>

if [ $# -ne 2 ]; then
   echo "Usage: $0 <start_n> <end_value>"
   echo "Example: $0 223606797750 1000000000000000000000000"
   exit 1
fi

START_N=$1
END_VAL=$2

echo "========================================"
echo "CVPipe Full Pipeline"
echo "Range: n=$START_N to candidates=$END_VAL"
echo "Started: $(date)"
echo "========================================"

# Stage 1: Generate candidates
echo "Stage 1: Generating candidates..."
time ./gen_candidates_range_gmp $START_N $END_VAL || exit 1

# Stage 2: Filter primes
echo "Stage 2: Filtering primes..."
time ./filter_primes_gmp || exit 1

# Stage 3: Check emirps
echo "Stage 3: Checking emirps..."
time ./check_emirp_gmp || exit 1

# Stage 3.5: Check palindromes
echo "Stage 3.5: Checking palindromes..."
time ./check_palindrome_gmp || exit 1

# Stage 4: Check converse
echo "Stage 4: Checking converse primes..."
time ./check_converse_gmp || exit 1

echo "========================================"
echo "Pipeline Complete!"
echo "Finished: $(date)"
echo "========================================"

# Display results summary
echo ""
echo "Results Summary:"
echo "----------------"
echo "Candidates generated: $(cat p*.dat | wc -l)"
echo "Primes found:         $(cat primes*.dat | wc -l)"
echo "Emirps found:         $(wc -l < emirps.dat)"
echo "Otto primes:          $(wc -l < otto_primes.dat)"
echo "Converse pairs:       $(wc -l < converse.dat)"
```

**Make executable:**
```bash
chmod +x run_pipeline.sh
```

**Usage:**
```bash
# Run full pipeline
./run_pipeline.sh 223606797750 1000000000000000000000000

# Run in background
nohup ./run_pipeline.sh 223606797750 1000000000000000000000000 > pipeline.log 2>&1 &
```

---

## Data Management

### File Organization

**Recommended directory structure:**

```
cvpipe/
├── src/
│   ├── gen_candidates_range_gmp.c
│   ├── filter_primes_gmp.c
│   ├── check_emirp_gmp.c
│   ├── check_palindrome_gmp.c
│   └── check_converse_gmp.c
├── bin/
│   └── (compiled executables)
├── data/
│   ├── current/
│   │   ├── p*.dat
│   │   ├── primes*.dat
│   │   └── emirps.dat
│   └── archive/
│       └── 10e23-to-24/
│           └── (completed run data)
├── logs/
│   └── (nohup.out files)
└── results/
    └── converse.dat (final results)
```

### Archival Strategy

**After successful run:**

```bash
# Create archive directory
RUN_NAME="10e23-to-24-$(date +%Y%m%d)"
mkdir -p archive/$RUN_NAME

# Move data files
mv p*.dat primes*.dat emirps.dat otto_primes.dat converse.dat archive/$RUN_NAME/

# Copy logs
cp nohup.out archive/$RUN_NAME/run.log

# Compress if space is tight
cd archive/$RUN_NAME
tar czf ../emirps-$RUN_NAME.tar.gz emirps.dat
# Delete uncompressed if needed:
# rm emirps.dat
```

### Disk Space Management

**Typical file sizes (10²³ to 10²⁴ range):**

```
p01.dat - p16.dat:        ~2-3 GB each   (40-50 GB total)
primes01.dat - primes16.dat: ~200-300 MB each (4-5 GB total)
emirps.dat:               ~5-10 GB
otto_primes.dat:          ~0 bytes (empty)
converse.dat:             ~0-100 bytes
```

**Total space needed:** ~50-65 GB for complete run + results

**Space-saving strategies:**

```bash
# Delete candidates after prime filtering
rm p*.dat
# Recoverable by re-running Stage 1

# Compress emirps.dat (very compressible)
gzip emirps.dat
# emirps.dat.gz typically 20-30% of original size

# Keep only essential results
rm primes*.dat  # Can regenerate from p*.dat if needed
```

---

## Monitoring and Validation

### Real-Time Progress Monitoring

**Watch file growth:**
```bash
watch -n 30 'ls -lh p*.dat primes*.dat emirps.dat'
# Updates every 30 seconds
```

**Count lines:**
```bash
watch -n 60 'wc -l p*.dat | tail -1; wc -l primes*.dat | tail -1; wc -l emirps.dat'
# Updates every 60 seconds
```

**Monitor log output:**
```bash
tail -f nohup.out
# Live updates as pipeline runs
```

**Check CPU utilization:**
```bash
htop
# Should show 16 threads at ~100% CPU during compute stages
```

**Monitor disk I/O:**
```bash
iostat -x 5
# 5-second intervals, watch for I/O bottlenecks
```

### Validation Scripts

**Verify candidate format:**
```bash
#!/bin/bash
# check_candidates.sh
for f in p*.dat; do
   echo "Checking $f..."
   # All lines should be numeric
   grep -v '^[0-9]*$' $f && echo "ERROR: Non-numeric in $f" || echo "OK"
done
```

**Verify prime counts match:**
```bash
#!/bin/bash
# check_prime_counts.sh
echo "Candidates per file:"
wc -l p*.dat

echo ""
echo "Primes per file:"
wc -l primes*.dat

echo ""
echo "Prime density check:"
TOTAL_CAND=$(cat p*.dat | wc -l)
TOTAL_PRIME=$(cat primes*.dat | wc -l)
DENSITY=$(echo "scale=4; $TOTAL_PRIME / $TOTAL_CAND * 100" | bc)
echo "Density: $DENSITY% (expected ~8.5%)"
```

**Verify emirp format:**
```bash
#!/bin/bash
# check_emirps.sh
echo "Checking emirps.dat format..."

# Each line should have exactly 2 numbers
awk 'NF != 2 {print "ERROR: Line " NR " has " NF " fields"}' emirps.dat

# Numbers should be different (not palindromes)
awk '$1 == $2 {print "ERROR: Palindrome at line " NR ": " $1}' emirps.dat

echo "Format check complete."
```

---

## Troubleshooting Guide

### Common Issues

**1. "Cannot open p01.dat"**
```
Cause: Stage 2+ run before Stage 1 completed
Fix: Run gen_candidates_range_gmp first
```

**2. "Segmentation fault"**
```
Cause: Usually overflow or GMP memory issue
Fix: Check compiler flags include -lgmp
     Verify sufficient RAM (need ~2 GB)
```

**3. "No space left on device"**
```
Cause: Disk full during candidate generation
Fix: Check available space: df -h
     Need 60-100 GB free for large runs
     Move old data to archive or delete
```

**4. Output files empty**
```
Cause: Range too small or program crashed
Fix: Check nohup.out for error messages
     Verify range parameters are correct
     Ensure programs compiled with -O3
```

**5. Threads not using all cores**
```
Cause: OpenMP not enabled or misconfigured
Fix: Check compilation: must have -fopenmp
     Verify: echo $OMP_NUM_THREADS (should be unset or 16)
```

**6. Very slow performance**
```
Cause: Multiple possibilities
Fix: Check CPU frequency: lscpu | grep MHz
     Check thermal throttling: sensors
     Verify disk not slow: iostat -x 5
     Profile: perf record -g ./program
```

**7. Results differ from previous run**
```
Cause: Miller-Rabin probabilistic, or bug
Fix: For same range, should be identical
     If different, investigate carefully
     Use 40 MR rounds for verification
```

### Emergency Recovery

**Pipeline interrupted mid-run:**

```bash
# Identify last completed stage
ls -lh *.dat

# If p*.dat exist but primes*.dat don't:
# Resume from Stage 2
./filter_primes_gmp && ./check_emirp_gmp && ./check_palindrome_gmp && ./check_converse_gmp

# If primes*.dat exist but emirps.dat doesn't:
# Resume from Stage 3
./check_emirp_gmp && ./check_palindrome_gmp && ./check_converse_gmp

# If emirps.dat exists but converse.dat doesn't:
# Resume from Stage 4
./check_converse_gmp
```

**Corrupted data files:**

```bash
# Check for corruption (should show all numbers)
head -100 suspicious_file.dat
tail -100 suspicious_file.dat

# If corrupted, must re-run from earlier stage
# No automatic recovery possible
```

---

## Performance Tuning

### Compiler Optimization Flags

**Maximum optimization:**
```bash
gcc -O3 -march=native -mtune=native -fopenmp -flto -Wall \
    program.c -o program -lgmp
```

**Flags explained:**
- `-O3`: Maximum optimization level
- `-march=native`: CPU-specific instructions (AVX2, etc.)
- `-mtune=native`: Optimize for current CPU microarchitecture
- `-flto`: Link-time optimization
- `-fopenmp`: OpenMP threading support

**Profile-guided optimization (advanced):**
```bash
# Step 1: Compile with profiling
gcc -O3 -march=native -fprofile-generate program.c -o program -lgmp

# Step 2: Run on representative data
./program <typical_args>

# Step 3: Recompile with profile data
gcc -O3 -march=native -fprofile-use program.c -o program -lgmp

# Step 4: Clean up profile data
rm *.gcda
```

### Runtime Tuning

**Thread count adjustment:**
```bash
# Override default 16 threads
export OMP_NUM_THREADS=8
./program

# Or modify source code NUM_THREADS
```

**CPU affinity (reduce context switching):**
```bash
taskset -c 0-7 ./program  # Pin to specific cores
```

**Nice level (background priority):**
```bash
nice -n 19 ./program  # Lowest priority (don't interfere with other tasks)
```

**I/O priority:**
```bash
ionice -c 3 ./program  # Idle I/O class
```

---

## Batch Processing Scripts

### Range Search Script

**search_ranges.sh:**
```bash
#!/bin/bash
# Search multiple consecutive ranges

# Define ranges (start_n, end_value pairs)
RANGES=(
   "1 1000000000000"
   "707106781 10000000000000"
   "2236067977 100000000000000"
)

for range in "${RANGES[@]}"; do
   START=$(echo $range | cut -d' ' -f1)
   END=$(echo $range | cut -d' ' -f2)
   
   echo "========================================"
   echo "Searching range: $START to $END"
   echo "Started: $(date)"
   echo "========================================"
   
   ./run_pipeline.sh $START $END
   
   # Archive results
   RUN_NAME="range-$START-to-$END"
   mkdir -p archive/$RUN_NAME
   mv *.dat archive/$RUN_NAME/
   
   echo "Range complete. Results in archive/$RUN_NAME/"
   echo ""
done
```

### Resumable Search Script

**resume_search.sh:**
```bash
#!/bin/bash
# Resume search from checkpoint

CHECKPOINT_FILE="search_checkpoint.txt"

if [ -f $CHECKPOINT_FILE ]; then
   LAST_END=$(cat $CHECKPOINT_FILE)
   echo "Resuming from checkpoint: $LAST_END"
else
   LAST_END=1
   echo "Starting fresh search"
fi

# Calculate next range
NEXT_START=$LAST_END
NEXT_END=$(echo "$NEXT_START * 10" | bc)

echo "Searching: $NEXT_START to $NEXT_END"
./run_pipeline.sh $NEXT_START $NEXT_END

# Save checkpoint
echo $NEXT_END > $CHECKPOINT_FILE
```

---

## AWS Cloud Deployment

### EC2 Instance Setup

**Recommended instance:** c5.18xlarge (72 vCPUs, 144 GB RAM)

```bash
# Connect to instance
ssh -i keypair.pem ec2-user@<instance-ip>

# Install dependencies
sudo yum update -y
sudo yum groupinstall "Development Tools" -y
sudo yum install gmp-devel -y

# Upload source code
scp -i keypair.pem *.c ec2-user@<instance-ip>:~/cvpipe/

# Build
make

# Run with all CPUs
export OMP_NUM_THREADS=72
nohup ./run_pipeline.sh <start> <end> &
```

### S3 Data Storage

```bash
# Upload results to S3
aws s3 cp emirps.dat s3://cvpipe-results/run-$(date +%Y%m%d)/
aws s3 cp converse.dat s3://cvpipe-results/run-$(date +%Y%m%d)/

# Download previous results
aws s3 cp s3://cvpipe-results/run-20260101/emirps.dat ./
```

---

## Verification and Testing

### Small Test Run

**Quick test to verify pipeline works:**

```bash
# Small range (should complete in seconds)
./gen_candidates_range_gmp 1 100000 && \
./filter_primes_gmp && \
./check_emirp_gmp && \
./check_palindrome_gmp && \
./check_converse_gmp

# Should find 12641 <-> 14621 pair
cat converse.dat
```

### Known Results Verification

**Test ranges with known results:**

```bash
# Should find the 12641 <-> 14621 pair
./gen_candidates_range_gmp 1 100000
# ... run pipeline ...
grep "12641" converse.dat
# Expected: 12641 14621
```

---

## Appendix: Complete Example Session

```bash
# Starting fresh on nitroIII

# 1. Build all programs
cd ~/cvpipe
make clean && make

# 2. Run 10^23 to 10^24 search
START_TIME=$(date +%s)

nohup time ./gen_candidates_range_gmp 223606797750 1000000000000000000000000 && \
./filter_primes_gmp && \
./check_emirp_gmp && \
./check_palindrome_gmp && \
./check_converse_gmp > run-10e23-to-24.log 2>&1 &

# 3. Monitor progress
tail -f run-10e23-to-24.log

# (Wait ~6 hours)

# 4. Check results
cat converse.dat
wc -l emirps.dat
# emirps.dat: 211,273,732 lines
# converse.dat: 0 lines (no new pairs found)

# 5. Archive results
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
echo "Total runtime: $ELAPSED seconds"

mkdir -p archive/10e23-to-24-$(date +%Y%m%d)
mv *.dat archive/10e23-to-24-$(date +%Y%m%d)/
cp run-10e23-to-24.log archive/10e23-to-24-$(date +%Y%m%d)/

# 6. Compress large files
cd archive/10e23-to-24-$(date +%Y%m%d)/
gzip emirps.dat p*.dat

# Done!
```

---

*Document Version: 1.0*  
*Last Updated: January 31, 2026*  
*Command Reference for CVPipe GMP Edition*
