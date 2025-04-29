#!/bin/bash

# Output CSV file
OUTPUT_FILE="results.csv"

# Compile all three programs
echo "Compiling mm-ser.c, mm-1d.c, mm-2d.c..."
mpicc mm-1d.c -o mm-1d
mpicc mm-2d.c -o mm-2d -lm
gcc mm-ser.c -o mm-ser
echo "Compilation complete."

# Initialize results.csv with header
echo "P,M,N,Q,Method,Run,Time_sec" > $OUTPUT_FILE

# Read all lines into an array
mapfile -t lines < <(tail -n +2 tests.csv)

# Loop through each line
for line in "${lines[@]}"
do
  IFS=, read -r P M N Q METHOD <<< "$line"

  METHOD_UPPER=$(echo "$METHOD" | tr '[:lower:]' '[:upper:]')

  if [ "$METHOD_UPPER" = "SERIAL" ]; then
    for i in {1..5}
    do
      echo "Run $i: Serial Test: M=${M}, N=${N}, Q=${Q}"
      START_TIME=$(date +%s.%N)
      ./mm-ser $M $N $Q
      STATUS=$?
      END_TIME=$(date +%s.%N)

      if [ $STATUS -ne 0 ]; then
        echo "Error: Serial run failed. Skipping..."
        continue
      fi

      ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
      echo "${P},${M},${N},${Q},${METHOD_UPPER},${i},${ELAPSED}" >> $OUTPUT_FILE
      sleep 1
    done
  elif [ "$METHOD_UPPER" = "1D" ]; then
    if [ $((M % P)) -ne 0 ]; then
      echo "Skipping 1D run: M=${M} not divisible by P=${P}"
      continue
    fi
    for i in {1..5}
    do
      echo "Run $i: 1D Test: P=${P}, M=${M}, N=${N}, Q=${Q}"
      START_TIME=$(date +%s.%N)
      mpirun -np ${P} ./mm-1d ${M} ${N} ${Q}
      STATUS=$?
      END_TIME=$(date +%s.%N)

      if [ $STATUS -ne 0 ]; then
        echo "Error: 1D run failed. Skipping..."
        continue
      fi

      ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
      echo "${P},${M},${N},${Q},${METHOD_UPPER},${i},${ELAPSED}" >> $OUTPUT_FILE
      sleep 1
    done
  elif [ "$METHOD_UPPER" = "2D" ]; then
    ROOT_P=$(echo "sqrt($P)" | bc)
    if [ $((ROOT_P * ROOT_P)) -ne $P ]; then
      echo "Skipping 2D run: P=${P} not perfect square."
      continue
    fi
    if [ $((M % ROOT_P)) -ne 0 ] || [ $((Q % ROOT_P)) -ne 0 ]; then
      echo "Skipping 2D run: M=${M} or Q=${Q} not divisible by sqrt(P)=${ROOT_P}"
      continue
    fi
    for i in {1..5}
    do
      echo "Run $i: 2D Test: P=${P}, M=${M}, N=${N}, Q=${Q}"
      START_TIME=$(date +%s.%N)
      mpirun -np ${P} ./mm-2d ${M} ${N} ${Q}
      STATUS=$?
      END_TIME=$(date +%s.%N)

      if [ $STATUS -ne 0 ]; then
        echo "Error: 2D run failed. Skipping..."
        continue
      fi

      ELAPSED=$(echo "$END_TIME - $START_TIME" | bc)
      echo "${P},${M},${N},${Q},${METHOD_UPPER},${i},${ELAPSED}" >> $OUTPUT_FILE
      sleep 1
    done
  else
    echo "Unknown method: $METHOD. Skipping..."
    continue
  fi
done
