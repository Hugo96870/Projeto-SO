#!/bin/bash

inputdir=$1
outputdir=$2
maxthreads=$3

nrArgs=0

for var in "$@"
do
    (( nrArgs++ ))          # Count the number of arguments
done

if [ ! $nrArgs -eq 3 ]      
then 
    echo "Invalid input."
    exit 1
fi

if [ ! -d "$inputdir" ]   
then
    echo "Input directory is not a directory."
    exit 1
fi

if [ ! -d "$outputdir" ]    
then
    echo "Output directory is not a directory."
    exit 1
fi

if [ $maxthreads -le 1 ]
then
    echo "Invalid number of threads."
    exit 1
fi

cd $inputdir
for test in *.txt
do
    for nrThreads in $(seq 1 $maxthreads)
    do
        cd ..
        echo InputFile=$test NumThreads=$nrThreads
        ./tecnicofs "$inputdir/$test" "$outputdir"/${test%.*}-"$nrThreads".txt "$nrThreads" | tail -n 1
        cd $inputdir
    done
done