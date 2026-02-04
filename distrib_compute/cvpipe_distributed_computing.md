# CVPipe Distributed Computing Guide

**Project:** Converse Prime Research - Distributed Architecture  
**Author:** j (Alaska)  
**Hardware:** nitroIII + nitroII (Multi-Machine Pipeline)  
**Date:** February 2026  

---

## Executive Summary

This guide documents the distributed computing approach for CVPipe, enabling parallel searches across multiple machines. By leveraging both nitroIII (Ryzen 7, 16 threads) and nitroII (i7-10510U, 8 threads), search times can be reduced by ~33-50% compared to single-machine execution.

**Key Benefits:**
- **Faster searches:** ~1.5× speedup for 10²⁴ to 10²⁵ range (6h → 4h)
- **No coordination overhead:** Machines run independently
- **Simple merging:** Results combine trivially
- **Fault tolerance:** One machine failing doesn't affect the other
- **Scalable:** Can add more machines easily

---

## Architecture Overview

### Single-Machine Architecture (Traditional)

```
nitroIII runs entire pipeline:
┌────────────────────────────────┐
│ Range: 10^24 to 10^25          │
│ Runtime: ~6 hours              │
│ All 5 stages on one machine    │
└────────────────────────────────┘
```

### Distributed Architecture (New)

```
Split range between machines:
┌─────────────────────────────────────────────────────────┐
│ nitroIII (66% of range)        nitroII (33% of range)   │
│ ┌──────────────────────┐       ┌──────────────────────┐│
│ │ 10^24 to 6.94×10^24  │       │ 6.94×10^24 to 10^25  ││
│ │ Runtime: ~4 hours    │       │ Runtime: ~4 hours    ││
│ │ All 5 stages         │       │ All 5 stages         ││
│ └──────────────────────┘       └──────────────────────┘│
└─────────────────────────────────────────────────────────┘
                  ↓                         ↓
            ┌────────────────────────────────────┐
            │  Merge results (5 minutes)         │
            │  Combined: all_primes.dat          │
            │            emirps.dat              │
            │            converse.dat            │
            └────────────────────────────────────┘
```

**Effective speedup:** Both finish in 4 hours vs 6 hours single-machine = **1.5× faster**

---

## Hardware Requirements

### Minimum Requirements Per Machine

```
CPU:      4+ cores (8 threads recommended)
RAM:      8 GB minimum, 16+ GB recommended
Storage:  100 GB free per 10^X range increment
Network:  Any (only for file transfer afterward)
```

### Current Hardware Profile

**nitroIII:**
```
CPU:      AMD Ryzen 7 4700U (8 cores, 16 threads)
RAM:      64 GB DDR4
Storage:  ~900 GB free
OS:       Linux (BunsenLabs)
Performance: 100% (baseline)
```

**nitroII:**
```
CPU:      Intel i7-10510U (4 cores, 8 threads)
RAM:      40 GB DDR4
Storage:  ~820 GB free
OS:       Linux (BunsenLabs)
Performance: ~50% of nitroIII
```

### Memory Usage Reality Check

**Actual CVPipe memory consumption:**
```
Stage 1:   ~100 MB/thread × threads = 800 MB (nitroIII) / 400 MB (nitroII)
Stage 2:   ~50 MB/thread  × threads = 400 MB (nitroIII) / 200 MB (nitroII)
Stage 3:   ~30 MB/thread  × threads = 240 MB (nitroIII) / 120 MB (nitroII)

Total working set: < 2 GB per machine
Plus OS overhead:  ~ 2 GB
Grand total:       < 4 GB per machine

nitroII's 40GB RAM? Over 10× what's needed! ✓
```

---

## Range Calculation Methodology

### Mathematical Foundation

For consecutive square sums: **candidate = n² + (n+1)²**

Rearranging: **n ≈ √(candidate / 2)**

### Example: 10²⁴ to 10²⁵

**Step 1: Calculate boundary n values**
```python
import math

start_candidate = 10**24
end_candidate = 10**25

start_n = int(math.sqrt(start_candidate / 2))
# → 707106781186

end_n = int(math.sqrt(end_candidate / 2))
# → 2236067977499
```

**Step 2: Determine split based on performance ratio**

nitroIII is ~2× faster than nitroII, so give it 66% of work:

```python
split_candidate = int(start_candidate + (end_candidate - start_candidate) * 0.66)
# → 6940000000000001182793728

split_n = int(math.sqrt(split_candidate / 2))
# → 1862793601019
```

**Step 3: Assign ranges**

```
nitroIII: n=707106781186 to candidate=6940000000000001182793728
nitroII:  n=1862793601019 to candidate=10000000000000000000000000
```

Both machines finish in approximately the same time (~4 hours).

### General Formula for Any Range

```python
def calculate_distributed_ranges(start_exp, end_exp, machine_ratios):
   """
   Args:
      start_exp: Starting exponent (e.g., 24 for 10^24)
      end_exp: Ending exponent (e.g., 25 for 10^25)
      machine_ratios: List of relative speeds [1.0, 0.5] means machine1 is 2× faster
   
   Returns:
      List of (start_n, end_candidate) tuples for each machine
   """
   start_candidate = 10**start_exp
   end_candidate = 10**end_exp
   total_range = end_candidate - start_candidate
   
   # Normalize ratios to percentages
   total_ratio = sum(machine_ratios)
   percentages = [r / total_ratio for r in machine_ratios]
   
   ranges = []
   current_candidate = start_candidate
   
   for pct in percentages:
      next_candidate = int(current_candidate + total_range * pct)
      start_n = int(math.sqrt(current_candidate / 2))
      
      ranges.append((start_n, next_candidate))
      current_candidate = next_candidate
   
   return ranges

# Example usage:
ranges = calculate_distributed_ranges(24, 25, [1.0, 0.5])
# → [(707106781186, 6940000000000001182793728),
#    (1862793601019, 10000000000000000000000000)]
```

---

## Setup Instructions

### Prerequisites

**On both machines:**

1. **Install dependencies:**
   ```bash
   sudo apt-get install build-essential libgmp-dev
   ```

2. **Compile CVPipe:**
   ```bash
   cd ~/cvpipe
   make clean && make
   ```

3. **Verify compilation:**
   ```bash
   ./gen_candidates_range_gmp --version  # Should not error
   ./filter_primes_gmp  # Should show usage
   ```

### File Distribution

**Copy scripts to both machines:**

```bash
# On nitroIII (your primary machine):
scp run_nitroIII_10e24-25.sh nitroII:~/cvpipe/
scp run_nitroII_10e24-25.sh nitroII:~/cvpipe/
scp merge_results_10e24-25.sh ~/cvpipe/

# Make executable:
chmod +x ~/cvpipe/run_nitroIII_10e24-25.sh
ssh nitroII 'chmod +x ~/cvpipe/run_nitroII_10e24-25.sh'
```

### Pre-Flight Check

**Test on small range first (10¹² to 10¹³):**

```bash
# nitroIII:
cd ~/cvpipe
./gen_candidates_range_gmp 707106 7071067811 && ./filter_primes_gmp
# Should complete in ~30 seconds

# nitroII:
ssh nitroII
cd ~/cvpipe
./gen_candidates_range_gmp 2236067 22360679774 && ./filter_primes_gmp
# Should complete in ~60 seconds (2× slower as expected)
```

If both complete successfully, you're ready for the big search!

---

## Execution Workflow

### Launch Sequence

**Terminal 1 (nitroIII):**
```bash
cd ~/cvpipe
nohup ./run_nitroIII_10e24-25.sh > nitroIII-10e24-25.log 2>&1 &
tail -f nitroIII-10e24-25.log
```

**Terminal 2 (nitroII via SSH):**
```bash
ssh nitroII
cd ~/cvpipe
nohup ./run_nitroII_10e24-25.sh > nitroII-10e24-25.log 2>&1 &
tail -f nitroII-10e24-25.log
```

**Both machines now running independently!**

### Monitoring Progress

**On nitroIII:**
```bash
# Watch the log
tail -f nitroIII-10e24-25.log

# Check current stage
ps aux | grep gen_candidates  # Stage 1 running
ps aux | grep filter_primes   # Stage 2 running
# etc.

# Monitor disk usage
df -h /home

# Watch file growth
watch -n 60 'ls -lh 10e24-25-nitroIII-*/emirps.dat'
```

**On nitroII (from nitroIII via SSH):**
```bash
ssh nitroII 'tail -100 ~/cvpipe/nitroII-10e24-25.log'

ssh nitroII 'ps aux | grep filter_primes'

ssh nitroII 'df -h /home'
```

### Expected Timeline

```
T+0h:     Both machines start Stage 1 (candidate generation)
T+2h:     Both transition to Stage 2 (prime filtering)
T+3.5h:   Both transition to Stage 3 (emirp detection)
T+4h:     Both machines complete!
T+4h 5m:  Merge results
```

**Variance:** ±30 minutes depending on exact prime density in each range

---

## Results Merging

### Transfer Results from nitroII

**Option 1: SCP (Secure Copy)**
```bash
# On nitroIII, copy nitroII results:
scp -r nitroII:~/cvpipe/10e24-25-nitroII-* ~/cvpipe/
```

**Option 2: rsync (Faster for large files)**
```bash
rsync -avz --progress nitroII:~/cvpipe/10e24-25-nitroII-* ~/cvpipe/
```

### Execute Merge Script

```bash
cd ~/cvpipe

./merge_results_10e24-25.sh \
   10e24-25-nitroIII-20260201-120000 \
   10e24-25-nitroII-20260201-120000
```

**Output:**
```
Merged directory: 10e24-25-merged-20260201-160530/
   ├── all_candidates.dat       (~50 GB)
   ├── all_primes.dat           (~8 GB)
   ├── emirps.dat               (~15 GB)
   ├── otto_primes.dat          (0 bytes, typically)
   ├── converse.dat             (~100 bytes)
   └── search_metadata.txt      (summary)
```

### Validation

**Sanity check merged results:**

```bash
cd 10e24-25-merged-*/

# Count totals
TOTAL_CANDIDATES=$(wc -l < all_candidates.dat)
TOTAL_PRIMES=$(wc -l < all_primes.dat)
TOTAL_EMIRPS=$(wc -l < emirps.dat)

# Verify prime density (~8.5%)
PRIME_DENSITY=$(echo "scale=4; $TOTAL_PRIMES / $TOTAL_CANDIDATES * 100" | bc)
echo "Prime density: ${PRIME_DENSITY}% (expected ~8.5%)"

# Verify emirp density (~7.7% of primes)
EMIRP_DENSITY=$(echo "scale=4; $TOTAL_EMIRPS / $TOTAL_PRIMES * 100" | bc)
echo "Emirp density: ${EMIRP_DENSITY}% (expected ~7.7%)"

# Check for converse pairs
cat converse.dat
```

---

## Troubleshooting

### Common Issues

**Problem:** nitroII shows "Cannot allocate memory"  
**Cause:** Unlikely with 40GB RAM, but possible if many other processes running  
**Solution:**
```bash
# Check memory usage
free -h

# Kill unnecessary processes
# Reduce thread count in script: export OMP_NUM_THREADS=4
```

**Problem:** Machines finish at very different times  
**Cause:** Incorrect performance ratio or one machine throttling  
**Solution:**
```bash
# Check CPU frequency on both machines
lscpu | grep MHz

# On nitroII, check for thermal throttling:
sensors

# Adjust split percentage for next run
```

**Problem:** "No space left on device"  
**Cause:** Insufficient disk space  
**Solution:**
```bash
# Check space before starting:
df -h /home

# Clean up old runs:
rm -rf ~/cvpipe/10e23-24-*  # Old searches

# Compress large intermediate files:
gzip p*.dat primes*.dat
```

**Problem:** Network transfer too slow  
**Cause:** Large files (10+ GB) over WiFi  
**Solution:**
```bash
# Option 1: Compress before transfer
tar czf nitroII-results.tar.gz 10e24-25-nitroII-*/
scp nitroII-results.tar.gz nitroIII:~/

# Option 2: Only transfer final results
scp nitroII:~/cvpipe/10e24-25-nitroII-*/emirps.dat ./
scp nitroII:~/cvpipe/10e24-25-nitroII-*/converse.dat ./
# Skip transferring p*.dat if not needed
```

### Recovery from Failures

**If nitroIII fails mid-run:**
```bash
# Check which stage failed
tail -100 nitroIII-10e24-25.log

# If Stage 1 failed: Delete all p*.dat, restart script
# If Stage 2 failed: p*.dat intact, delete primes*.dat, restart from Stage 2
# If Stage 3 failed: Delete emirps.dat, restart from Stage 3
```

**If nitroII becomes unreachable:**
```bash
# nitroIII can still complete its portion
# Later, when nitroII is back:
ssh nitroII
cd ~/cvpipe
./run_nitroII_10e24-25.sh  # Resume or restart

# Or if already partially complete, check logs and resume manually
```

---

## Performance Analysis

### Single Machine (nitroIII only)

```
Range: 10^24 to 10^25
Runtime: ~6 hours

Breakdown:
   Stage 1: 2.5h
   Stage 2: 3h
   Stage 3: 25min
   Stage 4: 5min
Total: 6 hours
```

### Distributed (nitroIII + nitroII)

```
Range: 10^24 to 10^25 (same total work)
Runtime: ~4 hours (both finish together)

nitroIII (66%):           nitroII (33%):
   Stage 1: 1.7h             Stage 1: 1.7h
   Stage 2: 2h               Stage 2: 2h
   Stage 3: 17min            Stage 3: 17min
   Stage 4: 3min             Stage 4: 3min
   Total: 4h                 Total: 4h

Merge: 5 minutes
Grand total: 4h 5min

Speedup: 6h / 4h = 1.5×
```

### Resource Utilization

**nitroIII during search:**
```
CPU:  1600% (16 threads × 100%)
RAM:  ~3 GB / 64 GB (5% used)
Disk: Writing at ~500 MB/min
```

**nitroII during search:**
```
CPU:  800% (8 threads × 100%)
RAM:  ~2 GB / 40 GB (5% used)
Disk: Writing at ~250 MB/min
```

Both machines are **CPU-bound**, not memory or I/O bound. This is ideal.

---

## Scaling to More Machines

### Adding a Third Machine

**Example: nitroI (older laptop, 4 cores)**

```
Total work split:
   nitroIII: 50% (fastest)
   nitroII:  30% (medium)
   nitroI:   20% (slowest)

Expected runtime: Still ~4h (all finish together)
Speedup vs single machine: ~1.8×
```

**Range calculation:**
```python
ranges = calculate_distributed_ranges(24, 25, [1.0, 0.6, 0.4])
# → nitroIII: 50%
#    nitroII:  30%
#    nitroI:   20%
```

### Cloud Scaling (AWS)

**For 10²⁶ or larger searches:**

```
Launch 10× c5.18xlarge instances (72 vCPUs each)
Each handles 10% of range
All finish in ~4 hours (vs 40+ hours single machine)

Cost: ~$3/hour × 10 instances × 4 hours = $120
Speedup: 10×

Alternative: 100 smaller instances
Cost: Similar
Speedup: ~10× (diminishing returns due to coordination overhead)
```

---

## Best Practices

### Before Starting a Distributed Search

1. **Test on small range** (10¹² to 10¹³) to verify:
   - All machines can compile and run CVPipe
   - Network connectivity for file transfer works
   - Performance ratios are as expected

2. **Check disk space:**
   ```bash
   df -h /home
   # Need ~100 GB free per 10^X increment
   ```

3. **Verify nobody else using machines:**
   ```bash
   uptime  # Check load
   who     # Check logged-in users
   ```

4. **Set CPU governor to performance:**
   ```bash
   sudo cpupower frequency-set -g performance
   ```

### During the Search

1. **Monitor both machines periodically** (every hour)
2. **Check disk space doesn't fill up**
3. **Watch for thermal throttling** (especially on laptops)
4. **Keep logs of runtime for future planning**

### After Completion

1. **Archive results immediately:**
   ```bash
   mkdir -p ~/cvpipe/archive/10e24-25-$(date +%Y%m%d)
   mv 10e24-25-merged-* ~/cvpipe/archive/
   ```

2. **Compress large intermediate files:**
   ```bash
   cd archive/10e24-25-*/
   gzip all_candidates.dat all_primes.dat
   # Keep emirps.dat uncompressed for analysis
   ```

3. **Document runtime for future estimates:**
   ```bash
   cat search_metadata.txt  # Review timing
   # Update your performance estimates
   ```

---

## Future Enhancements

### Automatic Load Balancing

**Current:** Manual percentage split based on known performance ratio  
**Future:** Dynamic work stealing

```
If nitroIII finishes its 66% in 3.5h but nitroII still has 1h left:
   → nitroIII could pick up some of nitroII's remaining work
   → Both finish in ~3.8h instead of 4h

Requires: Coordination server + work queue
Complexity: Medium
Benefit: 5-10% additional speedup
```

### Fault Tolerance

**Current:** If machine fails, restart manually  
**Future:** Automatic checkpointing

```
Every 30 minutes, save progress:
   - Which n values completed
   - Which stage running
   
On restart:
   - Resume from last checkpoint
   - No work lost

Complexity: Medium
Benefit: Robustness for long searches (days/weeks)
```

### Kubernetes Orchestration

**For cloud deployment:**

```yaml
apiVersion: batch/v1
kind: Job
metadata:
   name: cvpipe-10e25-10e26
spec:
   parallelism: 10
   completions: 10
   template:
      spec:
         containers:
         - name: cvpipe-worker
           image: cvpipe:latest
           env:
           - name: START_N
             value: "..." # Computed per pod
           - name: END_CANDIDATE
             value: "..."
```

**Benefits:**
- Auto-scaling based on workload
- Automatic retry on failure
- Cost optimization (spot instances)

---

## Cost Analysis

### Local Hardware (Current Setup)

```
Capital cost:
   nitroIII: $800 (amortized over 3 years)
   nitroII:  $600 (amortized over 3 years)

Operational cost (electricity):
   nitroIII: 15W TDP × 4h = 60 Wh = $0.01
   nitroII:  15W TDP × 4h = 60 Wh = $0.01

Total per search: ~$0.02 (essentially free!)

Advantage: No recurring costs
Limitation: Fixed capacity (can't scale beyond 2 machines)
```

### Cloud (AWS c5.18xlarge)

```
Instance cost: $3.06/hour

Single instance (10^24 to 10^25):
   72 vCPUs, ~2× faster than nitroIII
   Runtime: ~3 hours
   Cost: $9.18

10 instances (10^25 to 10^26):
   Each handles 10% of range
   Runtime: ~4 hours
   Cost: $122.40

Advantage: Massive scalability
Limitation: Recurring costs add up
```

### Hybrid Approach

```
Use local hardware for:
   - Development and testing
   - Smaller searches (10^24 to 10^25)
   - Ongoing verification

Use cloud for:
   - Very large searches (10^26+)
   - Time-critical searches
   - One-off massive computations
```

---

## Appendix: Quick Reference

### Command Cheat Sheet

```bash
# Setup (one-time)
make clean && make
chmod +x run_*.sh merge_*.sh

# Launch distributed search
# Terminal 1 (nitroIII):
nohup ./run_nitroIII_10e24-25.sh > n3.log 2>&1 &

# Terminal 2 (nitroII):
ssh nitroII 'cd ~/cvpipe && nohup ./run_nitroII_10e24-25.sh > n2.log 2>&1 &'

# Monitor
tail -f n3.log
ssh nitroII 'tail -f ~/cvpipe/n2.log'

# After completion, transfer and merge
scp -r nitroII:~/cvpipe/10e24-25-nitroII-* ~/cvpipe/
./merge_results_10e24-25.sh <nitroIII_dir> <nitroII_dir>

# Verify results
cd 10e24-25-merged-*/
wc -l *.dat
cat search_metadata.txt
```

### Range Calculator

```python
import math

def calc_ranges(start_exp, end_exp, ratio_nitroIII=0.66):
   start = 10**start_exp
   end = 10**end_exp
   
   split = int(start + (end - start) * ratio_nitroIII)
   
   n3_start = int(math.sqrt(start / 2))
   n3_end = split
   
   n2_start = int(math.sqrt(split / 2))
   n2_end = end
   
   print(f"nitroIII: {n3_start} to {n3_end}")
   print(f"nitroII:  {n2_start} to {n2_end}")

# Usage:
calc_ranges(24, 25)
calc_ranges(25, 26)
```

### Expected Runtimes

```
Range            Single (nitroIII)    Distributed (both)    Speedup
10^12 to 10^13   5 min                3 min                 1.7×
10^18 to 10^19   30 min               20 min                1.5×
10^23 to 10^24   3.5 hours            2.5 hours             1.4×
10^24 to 10^25   6 hours              4 hours               1.5×
10^25 to 10^26   ~9 hours             ~6 hours              1.5×
```

---

## Conclusion

Distributed computing with CVPipe is straightforward and highly effective:

1. **Simple setup:** Copy scripts, adjust thread counts
2. **No coordination needed:** Machines run independently
3. **Trivial merging:** Cat files together
4. **Significant speedup:** 1.5× with two machines
5. **Fault tolerant:** One failure doesn't affect the other
6. **Scalable:** Can add more machines or move to cloud

For the 10²⁴ to 10²⁵ search, using both nitroIII and nitroII reduces runtime from **6 hours to 4 hours** - a worthwhile improvement, especially for larger searches ahead.

Happy hunting! 🎯

---

*Document Version: 1.0*  
*Last Updated: February 1, 2026*  
*Hardware: nitroIII + nitroII*  
*Next Target: 10²⁵ to 10²⁶*
