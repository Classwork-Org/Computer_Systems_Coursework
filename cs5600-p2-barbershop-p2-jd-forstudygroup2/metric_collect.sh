#!/bin/bash

output_params_to_file ()
{
    echo "METRIC: $1 $2 " >> $3
}

compile_with_params () {
    make clean;
    make $1=$2 part-3-KNOBS
}

run_part_3 ()
{
    ./part-3 10000 | grep INFO >> $1
}

rm -f metrics.data
for metric in BARBER_SLEEP_TIME CUSTOMER_SLEEP_TIME CUSTOMER_COUNT MAX_CHAIR_COUNT
do
    for (( i=1; i<20;i++))
    do
        compile_with_params $metric $i;
        output_params_to_file $metric $i metrics.data
        run_part_3 metrics.data
    done
done
