#!/bin/bash

BLUE=$(tput setaf 4)
RED=$(tput setaf 1)
YELLOW=$(tput setaf 3)
GREEN=$(tput setaf 2)
STD=$(tput sgr0)

NO_OF_PIS=03

echo "    ${BLUE}[1] Cleanup old logfiles${STD}";
#  Clear syslog
ssh pi@pi01 "truncate -s 0 /tmp/rawlog.log"

#  Clear ordered log
for i in {01..03};
do
    ssh pi@pi${i} "rm -rf ordered_log *.sdot" &
done
wait

echo "    ${BLUE}[2] Run bakery on pi01 - pi03, this takes 15 Seconds${STD}";
#  Run bakery
for i in {01..03};
do
    ssh pi@pi${i} "bakery -d -w 15" &
done
wait

echo "    ${BLUE}[3] Collect syslog and causal log${STD}";
#  Collect logs
sftp pi@pi01:/tmp/rawlog.log
for i in {01..03};
do
    sftp pi@pi${i}:ordered_log &
    sftp pi@pi${i}:*.sdot &
done
wait

echo "    ${BLUE}[4] Generate Space-Time Diagram${STD}";
#  Generate Time Space Diagram
echo "digraph G {" > dia_ts.dot
#echo "	start [shape=Mdiamond];" >> dia_ts.dot
#echo " 	end [shape=Msquare];" >> dia_ts.dot
for sdot in *.sdot;
do
    cat $sdot >> dia_ts.dot
    rm $sdot
done
echo "}" >> dia_ts.dot
dot -Tsvg dia_ts.dot > dia_ts.svg
