#!/usr/bin/env bash
# Measure latency/throughput of sharedlist endpoints. Needs only bash + curl + awk.
#
# Usage:
#   USERNAME=me PASSWORD=pw SHAREDLIST_ID=1 ./scripts/bench.sh
#
# Env:
#   BASE            server url            (default http://127.0.0.1:18808)
#   USERNAME/PASSWORD   account used for authed endpoints (required for those)
#   SHAREDLIST_ID   a sharedlist owned by that user (required for list endpoints)
#   N               requests per endpoint (default 200)
#   C               concurrency           (default 1; run again with e.g. C=16)
#   SIGNIN_N        /signin requests (password hashing is slow; default 20)
#   PERF=1          also run `perf record` against the server for the whole run
#   SERVER_PID      pid to profile (default: pgrep -f ./build)
#   OUT             dir for perf data     (default ./bench_out)
#
# Run it twice, C=1 for per-request latency and C=16 for contention/throughput.

set -u
BASE=${BASE:-http://127.0.0.1:18808}
N=${N:-200}
C=${C:-1}
SIGNIN_N=${SIGNIN_N:-20}
OUT=${OUT:-./bench_out}
mkdir -p "$OUT"

# stats <file of seconds>  -> prints ms percentiles
stats() {
  sort -n "$1" | awk -v name="$2" -v wall="$3" '
    { v[NR]=$1*1000; s+=$1*1000 }
    END {
      if (!NR) { printf "%-34s no samples\n", name; exit }
      printf "%-34s n=%-5d min=%7.2f p50=%7.2f p95=%7.2f p99=%7.2f max=%7.2f mean=%7.2f ms  %6.0f req/s\n",
        name, NR, v[1], v[int(NR*0.50+0.5)], v[int(NR*0.95+0.5)], v[int(NR*0.99+0.5)], v[NR], s/NR, (wall>0 ? NR/wall : 0)
    }'
}

# bench <label> <n> <curl args...>   (responses != $EXPECT, default 200, are reported)
bench() {
  local label=$1 n=$2; shift 2
  local f="$OUT/$(echo "$label" | tr -c 'A-Za-z0-9\n' _).times"
  local start end
  start=$(date +%s.%N)
  seq "$n" | xargs -P "$C" -I{} curl -s -o /dev/null -w '%{http_code} %{time_total}\n' "$@" > "$f.raw"
  end=$(date +%s.%N)
  local bad
  bad=$(awk -v e="${EXPECT:-200}" '$1!=e' "$f.raw" | wc -l)
  awk -v e="${EXPECT:-200}" '$1==e {print $2}' "$f.raw" > "$f"
  stats "$f" "$label" "$(awk -v e="$end" -v s="$start" 'BEGIN{print e-s}')"
  [ "$bad" -gt 0 ] && echo "    !! $bad non-200 responses (codes: $(awk -v e="${EXPECT:-200}" '$1!=e{print $1}' "$f.raw" | sort | uniq -c | tr '\n' ' '))"
}

# ---- optional perf on the server ----
PERF_PID=""
if [ "${PERF:-0}" = 1 ]; then
  pid=${SERVER_PID:-$(pgrep -f '\./build' | head -1)}
  if [ -z "$pid" ]; then echo "PERF=1 but no server pid found (set SERVER_PID)"; exit 1; fi
  perf record -F 99 -g -p "$pid" -o "$OUT/perf.data" 2>"$OUT/perf.log" &
  PERF_PID=$!
  echo "perf recording pid $pid -> $OUT/perf.data"
fi

echo "== $BASE  N=$N C=$C =="

# ---- unauthenticated ----
echo "-- baseline (no db, no auth)"
bench "GET /test"            "$N" "$BASE/test"

echo "-- auth rejection path (middleware only)"
EXPECT=401 bench "GET /me (no cookie, 401)" "$N" "$BASE/me"

# ---- signin (expensive: password hash + session insert) ----
COOKIE=""
if [ -n "${USERNAME:-}" ] && [ -n "${PASSWORD:-}" ]; then
  echo "-- signin"
  body=$(printf '{"username":"%s","password":"%s"}' "$USERNAME" "$PASSWORD")
  bench "POST /signin" "$SIGNIN_N" -X POST -d "$body" "$BASE/signin"
  # grab one token to use for authed requests. Cookie is sent by hand since it is marked Secure.
  tok=$(curl -s -D - -o /dev/null -X POST -d "$body" "$BASE/signin" | tr -d '\r' | sed -n 's/^[Ss]et-[Cc]ookie: session_token=\([^;]*\).*/\1/p')
  [ -n "$tok" ] && COOKIE="Cookie: session_token=$tok"
else
  echo "-- skipping authed endpoints (set USERNAME and PASSWORD)"
fi

if [ -n "$COOKIE" ]; then
  echo "-- authed: session lookup + small queries"
  bench "GET /me"               "$N" -H "$COOKIE" "$BASE/me"

  if [ -n "${SHAREDLIST_ID:-}" ]; then
    id=$SHAREDLIST_ID
    bench "GET /sharedlist_info"          "$N" -H "$COOKIE" "$BASE/sharedlist_info?sharedlist_id=$id"
    echo "-- track list pagination (the batch-loading path)"
    for lim in 10 50 200 1000; do
      bench "GET /sharedlist limit=$lim off=0"    "$N" -H "$COOKIE" "$BASE/sharedlist?sharedlist_id=$id&offset=0&limit=$lim"
    done
    bench "GET /sharedlist limit=50 off=5000"     "$N" -H "$COOKIE" "$BASE/sharedlist?sharedlist_id=$id&offset=5000&limit=50"
    echo "-- authz failures (should be cheap)"
    EXPECT=403 bench "GET /sharedlist forbidden id"      "$N" -H "$COOKIE" "$BASE/sharedlist?sharedlist_id=999999999&offset=0&limit=50"
  else
    echo "-- skipping /sharedlist endpoints (set SHAREDLIST_ID)"
  fi
fi

# ---- wrap up perf ----
if [ -n "$PERF_PID" ]; then
  kill -INT "$PERF_PID" 2>/dev/null; wait "$PERF_PID" 2>/dev/null
  echo
  echo "perf done. inspect with:"
  echo "  perf report -i $OUT/perf.data --no-children"
  echo "  perf script -i $OUT/perf.data | stackcollapse-perf.pl | flamegraph.pl > $OUT/flame.svg"
fi

echo
echo "raw timings in $OUT/ (one seconds-value per line, per endpoint)"
