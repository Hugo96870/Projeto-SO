#!/bin/bash

inputdir=$1
outputdir=$2
maxthreads=$3
cd $inputdir
for test in *.txt
do
    for i in $(seq 1 $maxthreads)
    do
        cd ..
        echo InputFile=$test NumThreads=$i
        ./tecnicofs "$inputdir/$test" "$outputdir"/"$test%.txt"-"$i".txt "$i" > auxiliar.txt
        sentence=$( tail -n 1 auxiliar.txt)
        rm auxiliar.txt
        echo $sentence
        cd $inputdir
    done
done
