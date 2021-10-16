#! /bin/bash

set -e

gcc -o exp handin.c -lpthread

rm -f log.txt
for n in {0..19}
do
	./exp >> log.txt &
	sleep 0.5s
done

C="`cat log.txt | grep "You win" | wc -l`"

if [ $C == "0" ]; then
	echo "Failure: Failed to print the wanted string."
	echo "{\"scores\": {\"Correctness\": 0}}"
else
	echo "Success: Your string is correct."
	echo "{\"scores\": {\"Correctness\": 5}}"
fi

killall -9 exp || true
